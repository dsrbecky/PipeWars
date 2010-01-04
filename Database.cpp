#include "StdAfx.h"
#include "Database.h"

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