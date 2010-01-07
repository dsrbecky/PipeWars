#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

extern map<string, Mesh*> loadedMeshes; // ColladaImport.cpp

class MeshRenderer: public InputLayer
{
public:

	void Render(IDirect3DDevice9* dev)
	{
		D3DVIEWPORT9 viewport;
		D3DXMATRIX matProj;
		D3DXMATRIX matView;
		dev->GetViewport(&viewport);
		dev->GetTransform(D3DTS_PROJECTION, &matProj);
		dev->GetTransform(D3DTS_VIEW, &matView);

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

			if (!keyToggled_Alt['O']) {
				D3DXVECTOR3 screen[8];
				D3DXVec3ProjectArray(
					screen, sizeof(D3DXVECTOR3),
					entity->mesh->boundingBox.corners, sizeof(D3DXVECTOR3),
					&viewport, &matProj, &matView, &matWorld, 8
				);
				D3DXVECTOR3 minVec, maxVec;
				minVec = maxVec = screen[0];
				for(int i = 1; i < 8; i++) {
					minVec = min3(minVec, screen[i]);
					maxVec = max3(maxVec, screen[i]);
				}
				int margin = keyToggled_Alt['M'] ? 100 : 0;
				if (minVec.x > viewport.Width - margin) continue;
				if (minVec.y > viewport.Height - margin) continue;
				if (maxVec.x < margin) continue;
				if (maxVec.y < margin) continue;
			}

			entity->mesh->Render(dev, "OuterWall", "Path", "-Hi");

			if (keyToggled_Alt['B']) {
				RenderBoundingBox(dev, entity->mesh->boundingBox);
			}
		}
	}

	void Render2DBoundingBox(IDirect3DDevice9* dev, D3DXVECTOR3 minVec, D3DXVECTOR3 maxVec)
	{
		float z = 0.001f;
		D3DXVECTOR3 leftBottom(minVec.x, maxVec.y, z);
		D3DXVECTOR3 rightTop(maxVec.x, minVec.y, z);
		minVec.z = maxVec.z = z;

		vector<D3DXVECTOR3> ver;
		ver.push_back(minVec); ver.push_back(rightTop);
		ver.push_back(rightTop); ver.push_back(maxVec);
		ver.push_back(maxVec); ver.push_back(leftBottom);
		ver.push_back(leftBottom); ver.push_back(minVec);
		
		dev->DrawPrimitiveUP(D3DPT_LINELIST, ver.size() / 2, &(ver[0]), sizeof(D3DXVECTOR3));
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