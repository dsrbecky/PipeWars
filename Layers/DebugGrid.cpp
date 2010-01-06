#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

class DebugGrid: public Layer
{
	static const int size = 10;

	vector<float> vb;
	IDirect3DVertexBuffer9* buffer;
	D3DMATERIAL9 material;

public:

	DebugGrid(): buffer(NULL) {
		ZeroMemory(&material, sizeof(material));
	}

	void Render()
	{
		static int fvf = D3DFVF_XYZ;

		// Create buffer on demand
		if (buffer == NULL) {
			vb.clear();
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

			material.Emissive.r = 0.5;
			material.Emissive.a = 1;
		}

		D3DXMATRIXA16 matWorld;
		D3DXMatrixIdentity(&matWorld);
		pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

		pD3DDevice->SetStreamSource(0, buffer, 0, 3 * sizeof(float));
		pD3DDevice->SetFVF(fvf);
		pD3DDevice->SetTexture(0, NULL);
		pD3DDevice->SetMaterial(&material);
		pD3DDevice->DrawPrimitive(D3DPT_LINELIST, 0, vb.size() / 6);
	}

	void ReleaseDeviceResources()
	{
		if (buffer != NULL) {
			buffer->Release();
			buffer = NULL;
		}
	}
};

DebugGrid debugGrid;