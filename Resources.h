#ifndef __RESOURCES__
#define __RESOURCES__

#include "StdAfx.h"
#include "fmod/inc/fmod.hpp"

static const string pipeFileName = "pipe.dae";
static const string tankFileName = "tank.dae";
static const string MusicFilename = "../data/music/Entering the Stronghold.mp3";
static const float Volume = 0.1f;

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
	bool isPipeOrTank;

	Mesh(string _filename, string _geometryName):
		boundingBox(BoundingBox(D3DXVECTOR3(0,0,0), D3DXVECTOR3(0,0,0))),
		isPipeOrTank(_filename == pipeFileName || _filename == tankFileName)
	{}

	void Render(IDirect3DDevice9* dev);

	void SetMaterialColor(string name, DWORD color)
	{
		for(int i = 0; i < (int)tristrips.size(); i++) {
			if (tristrips[i].materialName.find(name) != -1) {
				D3DCOLORVALUE colorValue = {
					(float)((color >> 16) & 0xFF) / 256,
					(float)((color >> 8) & 0xFF) / 256,
					(float)(color & 0xFF) / 256,
					(float)(color >> 24) / 256
				};
				tristrips[i].material.Diffuse = colorValue;
			}
		}
	}

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

	FMOD::System*  fmodSystem;
    FMOD::Sound*   fmodSound;
    FMOD::Channel* fmodChannel;

	Resources(): font(NULL), fmodSystem(NULL) {}

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

	#define FMODCHK(cmd) if (cmd != FMOD_OK) { fmodSystem = NULL; return; }

	void LoadMusic()
	{
		FMODCHK( FMOD::System_Create(&fmodSystem) );
		FMODCHK( fmodSystem->init(1, FMOD_INIT_NORMAL, 0) );
		FMODCHK( fmodSystem->createStream(MusicFilename.c_str(), FMOD_HARDWARE | FMOD_LOOP_NORMAL | FMOD_2D, 0, &fmodSound) );
		FMODCHK( fmodSystem->playSound(FMOD_CHANNEL_FREE, fmodSound, true, &fmodChannel) );
		fmodChannel->setVolume(Volume);
	}

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

		if (fmodSystem != NULL) {
			fmodSound->release();
			fmodSystem->close();
			fmodSystem->release();
			fmodSystem = NULL;
		}
	}
};

#endif