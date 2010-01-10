#ifndef __RESOURCES__
#define __RESOURCES__

#include "StdAfx.h"

class Database;

// Defines one or more tristrips that share same material
struct Tristrip
{
	DWORD fvf;
	int vbStride_bytes;
	int vbStride_floats;
	vector<float> vb; // Vertex buffer data
	IDirect3DVertexBuffer9* buffer; // The buffer is created on demand
	std::vector<int> vertexCounts; // framgmentation of tristrips into groups
	string materialName;
	D3DMATERIAL9 material;
	string textureFilename;

	string getMaterialName() { return materialName; }

	void Render(IDirect3DDevice9* dev);

	void ReleaseDeviceResources()
	{
		if (buffer != NULL) {
			buffer->Release();
			buffer = NULL;
		}
	}

	bool IntersectsYRay(float x, float z, float* outY = NULL);
};

struct BoundingBox
{
	D3DXVECTOR3 minCorner;
	D3DXVECTOR3 maxCorner;
	D3DXVECTOR3 corners[8];

	BoundingBox(D3DXVECTOR3 _min, D3DXVECTOR3 _max):
		minCorner(_min), maxCorner(_max)
	{
		corners[0] = D3DXVECTOR3(minCorner.x, minCorner.y, minCorner.z);
		corners[1] = D3DXVECTOR3(minCorner.x, minCorner.y, maxCorner.z);
		corners[2] = D3DXVECTOR3(minCorner.x, maxCorner.y, minCorner.z);
		corners[3] = D3DXVECTOR3(minCorner.x, maxCorner.y, maxCorner.z);
		corners[4] = D3DXVECTOR3(maxCorner.x, minCorner.y, minCorner.z);
		corners[5] = D3DXVECTOR3(maxCorner.x, minCorner.y, maxCorner.z);
		corners[6] = D3DXVECTOR3(maxCorner.x, maxCorner.y, minCorner.z);
		corners[7] = D3DXVECTOR3(maxCorner.x, maxCorner.y, maxCorner.z);
	}

	bool Contains(D3DXVECTOR3 vec)
	{
		return
			minCorner.x <= vec.x && vec.x <= maxCorner.x &&
			minCorner.y <= vec.y && vec.y <= maxCorner.y &&
			minCorner.z <= vec.z && vec.z <= maxCorner.z;
	}
};

struct Mesh
{
	string filename;
	string geometryName;
	BoundingBox boundingBox;
	std::vector<Tristrip> tristrips;

	Mesh(): boundingBox(BoundingBox(D3DXVECTOR3(0,0,0), D3DXVECTOR3(0,0,0))) {}

	void Render(IDirect3DDevice9* dev, string hide1 = "", string hide2 = "", string hide3 = "");

	void ReleaseDeviceResources()
	{
		for(int i = 0; i < (int)tristrips.size(); i++) {
			tristrips[i].ReleaseDeviceResources();
		}
	}

	bool IsOnPath(float x, float z, float* outY = NULL);
};

class Resources
{
	DAE dae;
	hash_map<string, domCOLLADA*> loadedColladas;
	hash_map<string, Mesh*> loadedMeshes;
	hash_map<string, IDirect3DTexture9*> loadedTextures;
	ID3DXFont* font;

public:

	Resources(): font(NULL) {}

	~Resources() { Release(); }

	domCOLLADA* LoadCollada(string filename);

	Mesh* LoadMesh(string filename, string geometryName);

	void LoadMaterial(domCOLLADA* doc, string materialName, /* out */ D3DMATERIAL9* material, /* out */ string* textureFilename);
	
	IDirect3DTexture9* Resources::LoadTexture(IDirect3DDevice9* dev, string textureFilename);

	ID3DXFont* LoadFont(IDirect3DDevice9* dev)
	{
		if (font == NULL) {
			D3DXCreateFont( dev, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
							L"Arial", &font );
		}
		return font;
	}

	void LoadTestMap(Database* db);

	void ReleaseDeviceResources()
	{
		for(hash_map<string, Mesh*>::iterator it = loadedMeshes.begin(); it != loadedMeshes.end(); it++) {
			it->second->ReleaseDeviceResources();
		}
		
		for(hash_map<string, IDirect3DTexture9*>::iterator it = loadedTextures.begin(); it != loadedTextures.end(); it++) {
			it->second->Release();
		}
		loadedTextures.clear();

		if (font != NULL) {
			font->Release();
			font = NULL;
		}
	}

	void Release()
	{	
		ReleaseDeviceResources();

		// Release collada files
		dae.clear();
		loadedColladas.clear();

		// Release loaded meshes
		for(hash_map<string, Mesh*>::iterator it = loadedMeshes.begin(); it != loadedMeshes.end(); it++) {
			delete it->second;
		}
		loadedMeshes.clear();
	}
};

#endif