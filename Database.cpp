#include "StdAfx.h"
#include "Database.h"
using namespace std;

LPDIRECT3D9 pD3D = NULL;
LPDIRECT3DDEVICE9 pD3DDevice = NULL;

void Tristrip::Render()
{
	pD3DDevice->SetStreamSource(0, vb, 0, vbStride);
	pD3DDevice->SetFVF(fvf);
	pD3DDevice->SetTexture(0, texure);
	pD3DDevice->SetMaterial(&material);
	int start = 0;
	for(int j = 0; j < (int)vertexCounts.size(); j++) {
		int vertexCount = vertexCounts[j];
		pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, start, vertexCount - 2);
		start += vertexCount;
	}
}

void Mesh::Render()
{
	for(int i = 0; i < (int)tristrips.size(); i++) {
		tristrips[i].Render();
	}
}

Grid::Grid()
{
	fvf = D3DFVF_XYZ;
	size = 10;

	for(int i = -size; i <= size; i++) {
		// Along X
		vb.push_back((float)-size); vb.push_back(0); vb.push_back((float)i);
		vb.push_back((float)+size); vb.push_back(0); vb.push_back((float)i);
		/// Along Z
		vb.push_back((float)i); vb.push_back(0); vb.push_back((float)-size);
		vb.push_back((float)i); vb.push_back(0); vb.push_back((float)+size);
	}

	// Copy the buffer to graphic card memory

	pD3DDevice->CreateVertexBuffer(vb.size() * sizeof(float), 0, fvf, D3DPOOL_DEFAULT, &buffer, NULL);
	void* bufferData;
	buffer->Lock(0, vb.size() * sizeof(float), &bufferData, 0);
	copy(vb.begin(), vb.end(), (float*)bufferData);
	buffer->Unlock();

	ZeroMemory(&material, sizeof(material));
	material.Emissive.r = 0.5;
	material.Emissive.a = 1;
}

void Grid::Render()
{
	pD3DDevice->SetStreamSource(0, buffer, 0, 3 * sizeof(float));
	pD3DDevice->SetFVF(fvf);
	pD3DDevice->SetTexture(0, NULL);
	pD3DDevice->SetMaterial(&material);
	pD3DDevice->DrawPrimitive(D3DPT_LINELIST, 0, vb.size() / 6);
}