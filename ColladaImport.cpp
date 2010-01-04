#include "StdAfx.h"
#include "Database.h"
#include "ColladaImport.h"
using namespace std;

DAE dae;

map<string, domCOLLADA*> loadedFiles;

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

map<string, Mesh*> loadedMeshes;

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

void loadMaterial(domCOLLADA* doc, string materialName, /* out */ D3DMATERIAL9* material, /* out */ IDirect3DTexture9** texture)
{
	domMaterial* mat = daeSafeCast<domMaterial>(dae.getDatabase()->idLookup(materialName, doc->getDocument()));
	domEffect* effect = daeSafeCast<domEffect>(mat->getInstance_effect()->getUrl().getElement());

	// Get colors
	D3DCOLORVALUE white; white.a = white.r = white.g = white.b = 1;
	D3DCOLORVALUE black; black.a = 1; black.r = black.g = black.b = 0;

	material->Ambient = getD3DColor(effect->getDescendant("ambient"), white);
	material->Diffuse = getD3DColor(effect->getDescendant("diffuse"), white);
	material->Emissive = getD3DColor(effect->getDescendant("emission"), black);
	material->Specular = getD3DColor(effect->getDescendant("specular"), white);
	material->Power = 12.5;

	// Get texture
	domCommon_color_or_texture_type* diffuse = daeSafeCast<domCommon_color_or_texture_type>(effect->getDescendant("diffuse"));
	if (diffuse != NULL && diffuse->getTexture() != NULL) {
		string texName = diffuse->getTexture()->getTexture();
		domImage* tex = daeSafeCast<domImage>(dae.getDatabase()->idLookup(texName, doc->getDocument()));
		string texFilename = tex->getInit_from()->getCharData();
		if (FAILED(D3DXCreateTextureFromFileA(pD3DDevice, ("..\\data\\meshes\\" + texFilename).c_str(), texture))) {
			MessageBoxA(NULL, ("Could not find texture" + texFilename).c_str() , "COLLADA", MB_OK );
			exit(1);
		}
		material->Diffuse = white;
	} else {
		*texture = NULL;
	}
}

Mesh* loadMesh(string filename, string geometryName)
{
	string filenameAndGeometryName = filename + "\\" + geometryName;
	// Cached
	if (loadedMeshes.count(filenameAndGeometryName) > 0) {
		return loadedMeshes[filenameAndGeometryName];
	}

	domCOLLADA* doc = loadCollada(filename);

	// Get the mesh in COLLADA file
	domGeometry_Array geoms = doc->getLibrary_geometries_array().get(0)->getGeometry_array();
	domMeshRef meshRef;
	for(u_int i = 0; i < geoms.getCount(); i++) {
		if (geoms.get(i)->getName() == geometryName) {
			meshRef = geoms.get(i)->getMesh();
			break;
		}
	}
	if (meshRef == NULL) {
		MessageBoxA(NULL, ("Could not mesh " + filenameAndGeometryName).c_str(), "COLLADA", MB_OK);
		exit(1);
	}

	Mesh* mesh = new Mesh();
	loadedMeshes[filenameAndGeometryName] = mesh;  // Cache
	
	// Load the <tristrips/> elements
	// (other types are ignored for now)

	for(u_int i = 0; i < meshRef->getTristrips_array().getCount(); i++) {
		domTristripsRef tristripsRef = meshRef->getTristrips_array().get(i);

		Tristrip ts;
		ts.fvf = 0;

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

		vector<float> vb; // Vertex buffer data

		for(u_int j = 0; j < tristripsRef->getP_array().getCount(); j++) {
			domListOfUInts p = tristripsRef->getP_array().get(j)->getValue();

			ts.vertexCounts.push_back(p.getCount() / pStride);

			for(u_int k = 0; k < p.getCount(); k += pStride) {
				if (posOffset != -1) {
					int index = (int)p.get(k + posOffset);
					vb.push_back((float)posSrc->get(3 * index + 0));
					vb.push_back((float)posSrc->get(3 * index + 1));
					vb.push_back((float)posSrc->get(3 * index + 2));
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
					ts.vbStride = vb.size() * sizeof(float);
				}
			}
		}

		// Copy the buffer to graphic card memory

		pD3DDevice->CreateVertexBuffer(vb.size() * sizeof(float), 0, ts.fvf, D3DPOOL_DEFAULT, &ts.vb, NULL);
		void* vbData;
		ts.vb->Lock(0, vb.size() * sizeof(float), &vbData, 0);
		copy(vb.begin(), vb.end(), (float*)vbData);
		ts.vb->Unlock();

		// Load the material		

		loadMaterial(doc, tristripsRef->getMaterial(), &ts.material, &ts.texure);

		// Done with this <tristrips/>

		mesh->tristrips.push_back(ts);
	}

	return mesh;
}