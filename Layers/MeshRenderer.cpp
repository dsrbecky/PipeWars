#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

extern map<string, Mesh*> loadedMeshes; // ColladaImport.cpp

class MeshRenderer: public InputLayer
{
public:

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
			D3DXMatrixRotationY(&matRot, -entity->rotY / 360 * 2 * D3DX_PI);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matRot);

			D3DXMATRIXA16 matTrans;
			D3DXMatrixTranslation(&matTrans, entity->position.x, entity->position.y, entity->position.z);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);

			dev->SetTransform(D3DTS_WORLD, &matWorld);

			entity->mesh->Render(dev, "OuterWall", "Path", "-Hi");
			if (keyToggled_Alt['B']) {
				RenderBoundingBox(dev, entity->mesh->boundingBox);
			}
		}
	}

	void RenderBoundingBox(IDirect3DDevice9* dev, BoundingBox& bb)
	{
		vector<D3DXVECTOR3> ver;
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				if (i < j) {
					D3DXVECTOR3 v1 = bb.corners[i];
					D3DXVECTOR3 v2 = bb.corners[j];
					int same = 0;
					if (v1.x == v2.x) same++;
					if (v1.y == v2.y) same++;
					if (v1.z == v2.z) same++;
					if (same == 2) {
						// Line from i to j
						ver.push_back(v1);
						ver.push_back(v2);
					}
				}
			}
		}
		dev->DrawPrimitiveUP(D3DPT_LINELIST, ver.size() / 2, &(ver[0]), sizeof(D3DXVECTOR3));
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