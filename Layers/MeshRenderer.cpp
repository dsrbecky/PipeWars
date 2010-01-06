#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

extern map<string, Mesh*> loadedMeshes; // ColladaImport.cpp

class MeshRenderer: public Layer
{
public:
	void FrameMove(double fTime, float fElapsedTime)
	{
		if (fTime > 1 && db.entities.size() == 0) {
			db.loadTestMap();
		}
	}

	void Render(IDirect3DDevice9* dev)
	{
		for(int i = 0; i < (int)db.entities.size(); i++) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(db.entities[i]);
			if (entity == NULL)
				continue; // Other type

			D3DXMATRIXA16 matWorld;
			D3DXMatrixIdentity(&matWorld);

			D3DXMATRIXA16 matScale;
			D3DXMatrixScaling(&matScale, entity->scale, entity->scale, entity->scale);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matScale);

			D3DXMATRIXA16 matRot;
			D3DXMatrixRotationY(&matRot, entity->rotY / 360 * 2 * D3DX_PI);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matRot);

			D3DXMATRIXA16 matTrans;
			D3DXMatrixTranslation(&matTrans, entity->position.x, entity->position.y, entity->position.z);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);

			dev->SetTransform(D3DTS_WORLD, &matWorld);

			entity->Render(dev);
		}
	}

	void ReleaseDeviceResources()
	{
		map<string, Mesh*>::iterator it = loadedMeshes.begin();
		while(it != loadedMeshes.end()) {
			it->second->ReleaseDeviceResources();
			it++;
		}
	}
};

MeshRenderer meshRenderer;