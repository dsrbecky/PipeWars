#include "StdAfx.h"
#include "Database.h"

DAE dae;
map<string, domCOLLADA*> loadedFiles;
map<string, Mesh*> loadedMeshes;
string dataPath = "..\\data\\meshes";

domCOLLADA* loadCollada(string filename)
{
	// Cached
	if (loadedFiles.count(filename) > 0) {
		return loadedFiles[filename];
	} else {
		domCOLLADA* doc = dae.open(filename);
		if (doc == NULL) {
			MessageBoxA(NULL, ("Could not open " + filename).c_str(), "COLLADA", MB_OK);
			exit(1);
		}
		loadedFiles[filename] = doc;
		return doc;
	}
}

void releaseLoadedMeshes()
{
	map<string, Mesh*>::iterator it = loadedMeshes.begin();
	while(it != loadedMeshes.end()) {
		delete it->second;
		it++;
	}
	loadedMeshes.clear();
}

u_int getColor(domListOfFloats* src, int offset = 0)
{
	int r = (int)(src->get(4 * offset + 0) * 255);
	int g = (int)(src->get(4 * offset + 1) * 255);
	int b = (int)(src->get(4 * offset + 2) * 255);
	int a = (int)(src->get(4 * offset + 3) * 255);
	u_int argb = (a << 24) + (r << 16) + (g << 8) + (b);
	return argb;
}

D3DCOLORVALUE getD3DColor(domListOfFloats color)
{
	D3DCOLORVALUE ret;
	ret.r = (float)color.get(0);
	ret.g = (float)color.get(1);
	ret.b = (float)color.get(2);
	ret.a = (float)color.get(3);
	return ret;
}

D3DCOLORVALUE getD3DColor(daeElement* colorElement, D3DCOLORVALUE def)
{
	domCommon_color_or_texture_type* color = daeSafeCast<domCommon_color_or_texture_type>(colorElement);
	if (color != NULL && color->getColor() != NULL) {
		return getD3DColor(color->getColor()->getValue());
	} else {
		return def;
	}
}

void loadMaterial(domCOLLADA* doc, string materialName, /* out */ D3DMATERIAL9* material, /* out */ string* textureFilename)
{
	domMaterial* mat = daeSafeCast<domMaterial>(dae.getDatabase()->idLookup(materialName, doc->getDocument()));
	domEffect* effect = daeSafeCast<domEffect>(mat->getInstance_effect()->getUrl().getElement());

	// Get colors
	D3DCOLORVALUE white = {1, 1, 1, 1};
	D3DCOLORVALUE black = {0, 0, 0, 1};

	material->Ambient = getD3DColor(effect->getDescendant("ambient"), white);
	material->Diffuse = getD3DColor(effect->getDescendant("diffuse"), white);
	material->Emissive = getD3DColor(effect->getDescendant("emission"), black);
	material->Specular = getD3DColor(effect->getDescendant("specular"), white);
	material->Power = 12.5;

	// Get texture
	domCommon_color_or_texture_type* diffuse = daeSafeCast<domCommon_color_or_texture_type>(effect->getDescendant("diffuse"));
	if (diffuse != NULL && diffuse->getTexture() != NULL) {
		string texName = diffuse->getTexture()->getTexture();
		domImage* image = daeSafeCast<domImage>(dae.getDatabase()->idLookup(texName, doc->getDocument()));
		*textureFilename = "..\\data\\meshes\\" + image->getInit_from()->getCharData();
		material->Diffuse = white;
	} else {
		*textureFilename = "";
	}
}

Mesh* loadMesh(string filename, string geometryName)
{
	string filenameAndGeometryName = dataPath + "\\" + filename + "\\" + geometryName;
	// Cached
	if (loadedMeshes.count(filenameAndGeometryName) > 0) {
		return loadedMeshes[filenameAndGeometryName];
	}

	domCOLLADA* doc = loadCollada(dataPath + "\\" + filename);

	// Look for the geometry
	domGeometry* geom = NULL;
	domElement* elem = dae.getDatabase()->idLookup(geometryName, doc->getDocument());
	if (elem != NULL && elem->typeID() == domNode::ID()) {
		// Forward from node name
		domNode* node = daeSafeCast<domNode>(elem);
		geom = daeSafeCast<domGeometry>(node->getInstance_geometry_array().get(0)->getUrl().getElement());
	} else {
		geom = daeSafeCast<domGeometry>(elem);
	}
	if (geom == NULL) {
		MessageBoxA(NULL, ("Could not find mesh " + filenameAndGeometryName).c_str(), "COLLADA", MB_OK);
		exit(1);
	}
	domMeshRef meshRef = geom->getMesh();

	Mesh* mesh = new Mesh();
	loadedMeshes[filenameAndGeometryName] = mesh;  // Cache
	mesh->filename = filename;
	mesh->geometryName = geometryName;
	
	// Load the <tristrips/> elements
	// (other types are ignored for now)

	float minX, minY, minZ, maxX, maxY, maxZ;
	minX = minY = minZ = maxX = maxY = maxZ = 0;
	bool minMaxSet = false;

	for(u_int i = 0; i < meshRef->getTristrips_array().getCount(); i++) {
		domTristripsRef tristripsRef = meshRef->getTristrips_array().get(i);

		Tristrip ts;
		ts.fvf = 0;
		ts.buffer = NULL;
		ts.texture = NULL;

		// Resolve all data sources

		int posOffset = -1; domListOfFloats* posSrc;
		int norOffset = -1; domListOfFloats* norSrc;
		int colOffset = -1; domListOfFloats* colSrc;
		int texOffset = -1; domListOfFloats* texSrc;

		for(u_int j = 0; j < tristripsRef->getInput_array().getCount(); j++) {
			domInputLocalOffsetRef input = tristripsRef->getInput_array().get(j);
			daeElementRef source = input->getSource().getElement();
			// Defined per vertex - forward
			if (source->typeID() == domVertices::ID()) {
				source = daeSafeCast<domVertices>(source)->getInput_array().get(0)->getSource().getElement();
			}
			int offset = (int)input->getOffset();
			domListOfFloats* src = &(daeSafeCast<domSource>(source)->getFloat_array()->getValue());
			if (input->getSemantic() == string("VERTEX")) {
				posOffset = offset;
				posSrc = src;
				ts.fvf |= D3DFVF_XYZ;
			} else if (input->getSemantic() == string("NORMAL")) {
				norOffset = offset;
				norSrc = src;
				ts.fvf |= D3DFVF_NORMAL;
			} else if (input->getSemantic() == string("COLOR")) {
				colOffset = offset;
				colSrc = src;
				ts.fvf |= D3DFVF_DIFFUSE;
			} else if (input->getSemantic() == string("TEXCOORD")) {
				texOffset = offset;
				texSrc = src;
				ts.fvf |= D3DFVF_TEX2;
			}
		}

		// Load the <P/> elementes

		int pStride = 0;
		pStride = max(pStride, posOffset + 1);
		pStride = max(pStride, norOffset + 1);
		pStride = max(pStride, colOffset + 1);
		pStride = max(pStride, texOffset + 1);

		vector<float>& vb = ts.vb; // Vertex buffer data

		for(u_int j = 0; j < tristripsRef->getP_array().getCount(); j++) {
			domListOfUInts p = tristripsRef->getP_array().get(j)->getValue();

			ts.vertexCounts.push_back(p.getCount() / pStride);

			for(u_int k = 0; k < p.getCount(); k += pStride) {
				if (posOffset != -1) {
					int index = (int)p.get(k + posOffset);
					float x = (float)posSrc->get(3 * index + 0);
					float y = (float)posSrc->get(3 * index + 1);
					float z = (float)posSrc->get(3 * index + 2);
					if (!minMaxSet) {
						minX = maxX = x;
						minY = maxY = y;
						minZ = maxZ = z;
						minMaxSet = true;
					} else {
						minX = min(minX, x); maxX = max(maxX, x);
						minY = min(minY, y); maxY = max(maxY, y);
						minZ = min(minZ, z); maxZ = max(maxZ, z);
					}
					vb.push_back(x);
					vb.push_back(y);
					vb.push_back(z);
				}

				if (norOffset != -1) {
					int index = (int)p.get(k + norOffset);
					vb.push_back((float)norSrc->get(3 * index + 0));
					vb.push_back((float)norSrc->get(3 * index + 1));
					vb.push_back((float)norSrc->get(3 * index + 2));
				}

				if (colOffset != -1) {
					int index = (int)p.get(k + colOffset);
					u_int argb = getColor(colSrc, index);
					vb.push_back(*((float*)&argb));
				}

				if (texOffset != -1) {
					int index = (int)p.get(k + texOffset);
					vb.push_back((float)texSrc->get(2 * index + 0));
					vb.push_back(1 - (float)texSrc->get(2 * index + 1));
				}

				// Note vertex buffer stride (bytes)
				if (j == 0 && k == 0) {
					ts.vbStride_floats = vb.size();
					ts.vbStride_bytes = vb.size() * sizeof(float);
				}
			}
		}

		// Load the material		

		ts.materialName = tristripsRef->getMaterial();
		loadMaterial(doc, ts.materialName, &ts.material, &ts.textureFilename);

		// Done with this <tristrips/>

		mesh->tristrips.push_back(ts);
	}

	mesh->boundingBox = BoundingBox(D3DXVECTOR3(minX, minY, minZ), D3DXVECTOR3(maxX, maxY, maxZ));

	return mesh;
}