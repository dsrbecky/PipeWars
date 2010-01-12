#include "StdAfx.h"
#include "Resources.h"
#include "Entities.h"
#include "Math.h"

extern Resources resources;

const string dataPath = "..\\data\\meshes\\";
const string imagePath = "..\\data\\images\\";
const string pathMaterialName = "Path";

domCOLLADA* Resources::LoadCollada(string filename)
{
	if (loadedColladas.count(filename) > 0) {
		return loadedColladas[filename];
	} else {
		domCOLLADA* doc = dae.open(filename);
		if (doc == NULL) {
			MessageBoxA(NULL, ("Could not open " + filename).c_str(), "COLLADA", MB_OK);
			exit(1);
		}
		loadedColladas[filename] = doc;
		return doc;
	}
}

u_int GetColor(domListOfFloats* src, int offset = 0)
{
	int r = (int)(src->get(4 * offset + 0) * 255);
	int g = (int)(src->get(4 * offset + 1) * 255);
	int b = (int)(src->get(4 * offset + 2) * 255);
	int a = (int)(src->get(4 * offset + 3) * 255);
	u_int argb = (a << 24) + (r << 16) + (g << 8) + (b);
	return argb;
}

D3DCOLORVALUE GetD3DColor(domListOfFloats color)
{
	D3DCOLORVALUE ret;
	ret.r = (float)color.get(0);
	ret.g = (float)color.get(1);
	ret.b = (float)color.get(2);
	ret.a = (float)color.get(3);
	return ret;
}

D3DCOLORVALUE GetD3DColor(daeElement* colorElement, D3DCOLORVALUE def)
{
	domCommon_color_or_texture_type* color = daeSafeCast<domCommon_color_or_texture_type>(colorElement);
	if (color != NULL && color->getColor() != NULL) {
		return GetD3DColor(color->getColor()->getValue());
	} else {
		return def;
	}
}

void Resources::LoadMaterial(domCOLLADA* doc, string materialName, /* out */ D3DMATERIAL9* material, /* out */ string* textureFilename)
{
	domMaterial* mat = daeSafeCast<domMaterial>(dae.getDatabase()->idLookup(materialName, doc->getDocument()));
	domEffect* effect = daeSafeCast<domEffect>(mat->getInstance_effect()->getUrl().getElement());

	// Get colors
	D3DCOLORVALUE white = {1, 1, 1, 1};
	D3DCOLORVALUE black = {0, 0, 0, 1};

	material->Ambient = GetD3DColor(effect->getDescendant("ambient"), black);
	material->Diffuse = GetD3DColor(effect->getDescendant("diffuse"), white);
	material->Emissive = GetD3DColor(effect->getDescendant("emission"), black);
	material->Specular = GetD3DColor(effect->getDescendant("specular"), black);
	material->Power = 12.5;

	// Get texture
	domCommon_color_or_texture_type* diffuse = daeSafeCast<domCommon_color_or_texture_type>(effect->getDescendant("diffuse"));
	if (diffuse != NULL && diffuse->getTexture() != NULL) {
		string texName = diffuse->getTexture()->getTexture();
		domImage* image = daeSafeCast<domImage>(dae.getDatabase()->idLookup(texName, doc->getDocument()));
		*textureFilename = image->getInit_from()->getCharData();
		material->Diffuse = white;
	} else {
		*textureFilename = "";
	}
}

Mesh* Resources::LoadMesh(string filename, string geometryName)
{
	string filenameAndGeometryName = dataPath + "\\" + filename + "\\" + geometryName;
	// Cached
	if (loadedMeshes.count(filenameAndGeometryName) > 0) {
		return loadedMeshes[filenameAndGeometryName];
	}

	domCOLLADA* doc = LoadCollada(dataPath + "\\" + filename);

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

	Mesh* mesh = new Mesh(filename, geometryName);
	loadedMeshes[filenameAndGeometryName] = mesh;  // Cache
	
	// Load the <tristrips/> elements
	// (other types are ignored for now)

	D3DXVECTOR3 minCorner(0,0,0), maxCorner(0,0,0);
	bool minMaxSet = false;

	for(u_int i = 0; i < meshRef->getTristrips_array().getCount(); i++) {
		domTristripsRef tristripsRef = meshRef->getTristrips_array().get(i);

		Tristrip ts;
		ts.fvf = 0;
		ts.buffer = NULL;

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
						minCorner = maxCorner = D3DXVECTOR3(x, y, z);
						minMaxSet = true;
					} else {
						minCorner = min3(minCorner, D3DXVECTOR3(x, y, z));
						maxCorner = max3(maxCorner, D3DXVECTOR3(x, y, z));
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
					u_int argb = GetColor(colSrc, index);
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
		LoadMaterial(doc, ts.materialName, &ts.material, &ts.textureFilename);

		// Done with this <tristrips/>

		mesh->tristrips.push_back(ts);
	}

	mesh->boundingBox = BoundingBox(minCorner, maxCorner);

	return mesh;
}

IDirect3DTexture9* Resources::LoadTexture(IDirect3DDevice9* dev, string textureFilename)
{
	if (loadedTextures.count(textureFilename) == 0) {
		IDirect3DTexture9* texture;
		if (FAILED(D3DXCreateTextureFromFileA(dev, (imagePath + textureFilename).c_str(), &texture))) {
			if (FAILED(D3DXCreateTextureFromFileA(dev, (dataPath + textureFilename).c_str(), &texture))) {
				MessageBoxA(NULL, ("Could not find texture " + textureFilename).c_str() , "COLLADA", MB_OK);
				exit(1);
			}
		}
		loadedTextures[textureFilename] = texture;
	}
	return loadedTextures[textureFilename];
}

void Mesh::Render(IDirect3DDevice9* dev)
{
	for(int i = 0; i < (int)tristrips.size(); i++) {
		Tristrip& ts = tristrips[i];
		if (ts.material.Diffuse.a != 0.0f)
			ts.Render(dev);
	}
}

void Tristrip::Render(IDirect3DDevice9* dev)
{
	// Create buffer on demand
	if (buffer == NULL) {
		dev->CreateVertexBuffer(vb.size() * sizeof(float), 0, fvf, D3DPOOL_DEFAULT, &buffer, NULL);
		void* vbData;
		buffer->Lock(0, vb.size() * sizeof(float), &vbData, 0);
		copy(vb.begin(), vb.end(), (float*)vbData);
		buffer->Unlock();
	}

	dev->SetStreamSource(0, buffer, 0, vbStride_bytes);
	dev->SetFVF(fvf);
	if (textureFilename.size() > 0) {
		dev->SetTexture(0, resources.LoadTexture(dev, textureFilename));
	} else {
		dev->SetTexture(0, NULL);
	}
	dev->SetMaterial(&material);
	int start = 0;
	for(int j = 0; j < (int)vertexCounts.size(); j++) {
		int vertexCount = vertexCounts[j];
		dev->DrawPrimitive(D3DPT_TRIANGLESTRIP, start, vertexCount - 2);
		start += vertexCount;
	}
}

bool Mesh::IsOnPath(float x, float z, float* outY)
{
	for(int i = 0; i < (int)tristrips.size(); i++) {
		Tristrip& ts = tristrips[i];
		if (ts.materialName == pathMaterialName) {
			if (ts.IntersectsYRay(x, z, outY))
				return true;
		}
	}
	return false;
}

bool Tristrip::IntersectsYRay(float x, float z, float* outY)
{
	int index = 0;
	for(int i = 0; i < (int)vertexCounts.size(); i++) {
		int vertexCount = vertexCounts[i];
		// Test all triangles in set
		for(int j = 0; j < vertexCount - 2; j++) {
			// Test tringle at current offset 
			bool isIn = Intersect_YRay_Triangle(x, z, 
				vb[(index + 0) * vbStride_floats + 0], vb[(index + 0) * vbStride_floats + 2], // A
				vb[(index + 1) * vbStride_floats + 0], vb[(index + 1) * vbStride_floats + 2], // B
				vb[(index + 2) * vbStride_floats + 0], vb[(index + 2) * vbStride_floats + 2]  // C
			);
			if (isIn) {
				if (outY != NULL) {
					Intersect_YRay_Plane(x, z, outY,
						(D3DXVECTOR3*)&vb[(index + 0) * vbStride_floats],
						(D3DXVECTOR3*)&vb[(index + 1) * vbStride_floats],
						(D3DXVECTOR3*)&vb[(index + 2) * vbStride_floats]
					);
				}
				return true;
			}
			index++; // Next
		}
		index += 2; // Next set
	}
	return false;
}

Mesh* MeshEntity::getMesh()
{
	if (meshPtrCache == NULL)
		meshPtrCache = resources.LoadMesh(meshFilename, meshGeometryName);
	return meshPtrCache;
}

void Resources::LoadTestMap(Database* db)
{
	// Preload meshs
	LoadMesh("Merman.dae", "Revolver");
	LoadMesh("Bullets.dae", "RevolverBullet");

	db->add(4, 0, 6, 180,      new MeshEntity("suzanne.dae", "Suzanne"));
	
	db->add(-10, 0.5, 8, 0,    new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(-10, 0.5, 0, 0,    new MeshEntity("pipe.dae", "LongStraight"));
	db->add(-6, 0.5, -2, 180,  new MeshEntity("pipe.dae", "UTurn"));
	db->add(-6, 0.5, 0, 0,     new MeshEntity("pipe.dae", "LevelUp"));
	db->add(-10, 2.5, 8, 0,    new MeshEntity("pipe.dae", "UTurn"));
	db->add(-10, 2.5, 6, 180,  new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(-6, 4.5, -6, 90,   new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(-8, 4.5, -8, 90,   new MeshEntity("tank.dae", "Tank5x5"));
	db->add(-20, 4.5, -10, 0,  new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(-16, 4.5, -16, 90, new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(-10, 4.5, -12, 180,new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(-4, 4.5, -6, 270,  new MeshEntity("pipe.dae", "LongStraight"));

	db->add(10, 2.5, -6, 90,   new MeshEntity("pipe.dae", "LevelUp"));
	db->add(12, 2.5, -6, 270,  new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(16, 2.5, -12, 180, new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(6, 2.5, -12, 270,  new MeshEntity("pipe.dae", "STurn2"));
	db->add(4, 2.5, -12, 90,   new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(0, 0.5, 0, 180,    new MeshEntity("pipe.dae", "LevelUp"));
	db->add(0, 0.5, 2, 0,      new MeshEntity("pipe.dae", "LongStraight"));
	db->add(-2, 0.5, 12, 270,  new MeshEntity("tank.dae", "Tank3x5"));
	
	db->add(6, 0.5, 12, 270,   new MeshEntity("pipe.dae", "LongStraight"));
	db->add(16, 0.5, 8, 0,     new MeshEntity("tank.dae", "Tank5x7"));
	db->add(18, -1.5, -2, 0,   new MeshEntity("pipe.dae", "LevelUp"));
	db->add(22, -1.5, -8, 90,  new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(30, -3.5, -8, 90,  new MeshEntity("pipe.dae", "LevelUp"));
	db->add(32, -3.5, -4, 270, new MeshEntity("pipe.dae", "UTurn"));
	db->add(30, -3.5, -4, 90,  new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(22, -3.5, 8, 270,  new MeshEntity("tank.dae", "Tank5x7"));
	db->add(34, -3.5, 6, 270,  new MeshEntity("pipe.dae", "STurn2"));

	db->add(30, -1.5, 12, 90,  new MeshEntity("pipe.dae", "LevelUp"));
	db->add(38, -3.5, 12, 90,  new MeshEntity("pipe.dae", "LevelUp"));
	db->add(40, -3.5, 12, 270, new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(44, -3.5, 6, 180,  new MeshEntity("pipe.dae", "LeftTurn"));
	
	//db->add(10, 4.5, 30, 0,    new MeshEntity("tank.dae", "Tank3x5"));
	//db->add(2, 4.5, 28, 0,     new MeshEntity("pipe.dae", "LeftTurn"));
	//db->add(10, 4.5, 20, 0,    new MeshEntity("pipe.dae", "LongStraight"));
	//db->add(0, 4.5, 20, 0,     new MeshEntity("pipe.dae", "STurn1"));
	//db->add(10, 4.5, 18, 180,  new MeshEntity("pipe.dae", "LeftTurn"));
	//db->add(4, 4.5, 14, 90,    new MeshEntity("pipe.dae", "LeftTurn"));
	
	//db->add(20, 2.5, 32, 90,   new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(22, 2.5, 32, 270,  new MeshEntity("pipe.dae", "UTurn"));
	//db->add(14, 0.5, 28, 270,  new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(12, 0.5, 28, 90,   new MeshEntity("pipe.dae", "UTurn"));
	//db->add(20, -1.5, 32, 90,  new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(22, -1.5, 32, 270, new MeshEntity("pipe.dae", "LongStraight"));
	//db->add(30, -1.5, 32, 270, new MeshEntity("pipe.dae", "UTurn"));
	//db->add(22, -3.5, 28, 270, new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(20, -3.5, 24, 90,  new MeshEntity("pipe.dae", "UTurn"));
	
	//db->add(26, -3.5, 12, 0,   new MeshEntity("pipe.dae", "LongStraight"));
	//db->add(22, -3.5, 24, 270, new MeshEntity("pipe.dae", "LeftTurn"));
	
	//db->add(18, 0.5, 20, 0,    new MeshEntity("pipe.dae", "LeftTurn"));
	//db->add(28, 0.5, 28, 180,  new MeshEntity("pipe.dae", "LeftTurn"));
	//db->add(28, 0.5, 30, 0,    new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(24, 2.5, 42, 270,  new MeshEntity("pipe.dae", "LeftTurn"));
	//db->add(22, 2.5, 42, 90,   new MeshEntity("pipe.dae", "LevelUp"));
	//db->add(10, 4.5, 38, 0,    new MeshEntity("pipe.dae", "LeftTurn"));

	db->add(18, -1.5, 26, 180, new MeshEntity("pipe.dae", "LevelUp"));
	db->add(18, -1.5, 28, 0,   new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(24, -1.5, 32, 270, new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(28, -1.5, 26, 180, new MeshEntity("pipe.dae", "STurn1"));
	db->add(26, -3.5, 12, 0,   new MeshEntity("pipe.dae", "LevelUp"));
	
	db->add(0, 0.5, 16, 0,     new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(12, -1.5, 20, 90,  new MeshEntity("pipe.dae", "LevelUp"));
	db->add(14, -1.5, 20, 270, new MeshEntity("pipe.dae", "UTurn"));
	db->add(6, -3.5, 16, 270,  new MeshEntity("pipe.dae", "LevelUp"));
	db->add(0, -3.5, 12, 0,    new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(4, -3.5, 6, 90,    new MeshEntity("pipe.dae", "LeftTurn"));
	db->add(6, -3.5, 6, 270,   new MeshEntity("pipe.dae", "LongStraight"));
	db->add(12, -3.5, 6, 270,  new MeshEntity("pipe.dae", "LongStraight"));
	
	db->add(0, 0, 8, 0,       new PowerUp(Weapon_Revolver, "Weapons.dae", "Revolver"));
	db->add(44, -4, 8, 0,     new PowerUp(Weapon_Revolver, "Weapons.dae", "Revolver"));
	db->add(21.6, 0, 9.4, 0,  new PowerUp(Weapon_Revolver, "Weapons.dae", "Revolver"));
	db->add(23, -2, 32, 0,    new PowerUp(Weapon_Revolver, "Weapons.dae", "Revolver"));
	db->add(-8, 2, 9, 0,      new PowerUp(Weapon_Shotgun, "Weapons.dae", "SawedOff"));
	db->add(14.5, 0, 17.5, 0, new PowerUp(Weapon_Shotgun, "Weapons.dae", "SawedOff"));
	db->add(0, 0, 12, 0,      new PowerUp(Weapon_AK47, "Weapons.dae", "AK47"));
	db->add(20, -2, -6.8, 0,  new PowerUp(Weapon_AK47, "Weapons.dae", "AK47"));
	db->add(31.5, -2, 12, 0,  new PowerUp(Weapon_Jackhammer, "Weapons.dae", "Jackhammer"));
	db->add(15, -2, 17.5, 0,  new PowerUp(Weapon_Jackhammer, "Weapons.dae", "Jackhammer"));
	db->add(24, -4, 7, 0,     new PowerUp(Weapon_Nailgun, "Weapons.dae", "Nailgun"));
	db->add(14, 2, -7.2, 0,   new PowerUp(Weapon_Nailgun, "Weapons.dae", "Nailgun"));
	//db->add(25.5, -2.2, 28, 0,  new PowerUp(Weapon_Shotgun, "Weapons.dae", "SawedOff"));
	
	db->add(new RespawnPoint(5.5, -4, 16));
	db->add(new RespawnPoint(33.0, -4, -7.0));
	db->add(new RespawnPoint(-8, 0, -3));
	db->add(new RespawnPoint(0, 0, 3));
	db->add(new RespawnPoint(9, 0, 12));
	db->add(new RespawnPoint(40, -4, 12));
	db->add(new RespawnPoint(23, -2, -8));
	db->add(new RespawnPoint(18, -2, 27));
	db->add(new RespawnPoint(0, -4, 12));
	
	db->add(6.0, 0, 20, 0,    new PowerUp(HealthPack, "HealthPack.dae", "MediBox"));
	db->add(16.0, 0, 12, 0,   new PowerUp(HealthPack, "HealthPack.dae", "MediBox"));
	db->add(44.0, -4, 6, 0,   new PowerUp(ArmourPack, "HealthPack.dae", "Armour"));
	db->add(0.0, 4, -6, 0,    new PowerUp(Shiny, "Shiny.dae", "Green"));
	db->add(0.0, 0, 7, 0,     new PowerUp(Shiny, "Shiny.dae", "Blue"));
	db->add(21, -4, 9, 0,     new PowerUp(Shiny, "Shiny.dae", "Red"));
	db->add(-10, 4, -6, 0,    new PowerUp(Monkey, "Monkey.dae", "LeChuck"));
	//db->add(10.0, 4.0, 22, 0,   new PowerUp(HealthPack, "HealthPack.dae", "MediBox"));
	//db->add(4.0, 4.0, 14.0, 0,  new PowerUp(ArmourPack, "HealthPack.dae", "Armour"));
	//db->add(28.0, 1.0, 31, 0,   new PowerUp(Shiny, "Shiny.dae", "Red"));
	//db->add(26.0, -4.0, 14, 0,  new PowerUp(Skull, "skull.dae", "Skull"));

	db->add(31.0, -4, -8, 0,  new PowerUp(Ammo_Revolver, "Ammo.dae", "RevolverAmmo"));
	db->add(18.0, 0, 15, 0,   new PowerUp(Ammo_Revolver, "Ammo.dae", "RevolverAmmo"));
	db->add(26.5, -2, 31, 0,  new PowerUp(Ammo_Shotgun, "Ammo.dae", "ShotgunShells"));
	db->add(5, 2, -12, 0,     new PowerUp(Ammo_Shotgun, "Ammo.dae", "ShotgunShells"));
	db->add(0.0, -4, 10, 0,   new PowerUp(Ammo_AK47, "Ammo.dae", "AKAmmo"));
	db->add(28.0, -4, 2, 0,   new PowerUp(Ammo_AK47, "Ammo.dae", "AKAmmo"));
	db->add(-7.0, 0, -3.0, 0, new PowerUp(Ammo_Jackhammer, "Ammo.dae", "JackhammerAmmo"));
	db->add(14.0, -4, 6.0, 0, new PowerUp(Ammo_Jackhammer, "Ammo.dae", "JackhammerAmmo"));
	db->add(10.0, -4, 6, 0,   new PowerUp(Ammo_Nailgun, "Ammo.dae", "NailgunAmmo"));
	db->add(31, -4, 9, 0,     new PowerUp(Ammo_Nailgun, "Ammo.dae", "NailgunAmmo"));
	//db->add(26.0, -2.0, 32, 0,  new PowerUp(Ammo_Nailgun, "Ammo.dae", "NailgunAmmo"));
	//db->add(23.0, 2.0, 29.0, 0, new PowerUp(Ammo_Shotgun, "Ammo.dae", "ShotgunShells"));
	//db->add(22.0, 2.0, 42.0, 0, new PowerUp(Ammo_Shotgun, "Ammo.dae", "ShotgunShells"));
}