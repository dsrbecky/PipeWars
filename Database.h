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

class Grid
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

#endif __DATABASE__
