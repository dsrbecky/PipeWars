#ifndef __DATABASE__
#define __DATABASE__

#include "StdAfx.h"
using namespace std;

extern LPDIRECT3D9 pD3D;
extern LPDIRECT3DDEVICE9 pD3DDevice;

// Defines one or more tristrips that share same material
class Tristrip
{
public:
	DWORD fvf;
	int vbStride; // in bytes
	IDirect3DVertexBuffer9* vb;
	std::vector<int> vertexCounts; // framgmentation of tristrips into groups
	string materialName;
	D3DMATERIAL9 material;
	IDirect3DTexture9* texure;

	void Render();
};

class Mesh
{
public:
	std::vector<Tristrip> tristrips;

	void Render();
};

class Vec3
{
public:
	float x, y, z;
};

class Entity
{
public:
	Vec3 position;
	float rotY;
	float scale;
	Entity()
	{
		position.x = position.y = position.z = 0;
		rotY = 0;
		scale = 1;
	}
	virtual ~Entity() {}
	virtual void Render() = 0;
};

class Grid: public Entity
{
	int fvf;
	int size;
	vector<float> vb;
	IDirect3DVertexBuffer9* buffer;
	D3DMATERIAL9 material;
public:
	Grid();
	void Render();
};

class MeshEntity: public Entity
{
public:
	Mesh* mesh;
	MeshEntity(Mesh* m): mesh(m) {}
	void Render() { mesh->Render(); };
};

class Database
{
public:
	vector<Entity*> entities;
	void Render();
};

extern Database db;

#endif __DATABASE__
