#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"
#include <set>

extern map<string, Mesh*> loadedMeshes; // ColladaImport.cpp
extern map<string, IDirect3DTexture9*> loadedTextures;
extern Player* localPlayer;

float hiQualityPipes = 0;
float targetFPS = 30;

int stat_objRendered;
int stat_pipesRendered;

class MeshRenderer: public InputLayer
{
public:
	void FrameMove(double fTime, float fElapsedTime)
	{
		float extraFPS = DXUTGetFPS() - targetFPS;
		if (extraFPS < 0) {
			hiQualityPipes += fElapsedTime * extraFPS * 0.5;
		} else {
			hiQualityPipes += fElapsedTime * extraFPS * 0.05;
		}
		hiQualityPipes = max(0, hiQualityPipes);
	}

	void Render(IDirect3DDevice9* dev)
	{
		// Get the camera settings
		D3DVIEWPORT9 viewport;
		D3DXMATRIX matProj;
		D3DXMATRIX matView;
		dev->GetViewport(&viewport);
		dev->GetTransform(D3DTS_PROJECTION, &matProj);
		dev->GetTransform(D3DTS_VIEW, &matView);

		// Find and mark pipes to be rendered in hi-quality
		// (the closeset ones to the player)
		multiset<pair<float, MeshEntity*>> pipes;
		for(int i = 0; i < (int)db.entities.size(); i++) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(db.entities[i]);
			if (entity != NULL && entity->mesh->filename == PipeFilename) {
				D3DXVECTOR3 delta = entity->position - localPlayer->position;
				float distance = D3DXVec3LengthSq(&delta);
				pipes.insert(pair<float, MeshEntity*>(distance, entity));
				entity->hiQuality = false; // Defualt
			}
		}
		multiset<pair<float, MeshEntity*>>::iterator it = pipes.begin();
		for(int i = 0; i < (int)hiQualityPipes && it != pipes.end(); i++) {
			it->second->hiQuality = true;
			it++;
		}

		stat_objRendered = 0;
		stat_pipesRendered = 0;

		// Render meshes
		for(int i = 0; i < (int)db.entities.size(); i++) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(db.entities[i]);
			if (entity == NULL)
				continue; // Other type

			// Set the WORLD for the this entity

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


			// Clip using frustum
			if (!keyToggled_Alt['F']) {

				// Corners of the screen
				float w = (float)viewport.Width;
				float h = (float)viewport.Height;
				D3DXVECTOR3 screen[8] = {
					D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(w, 0, 0), D3DXVECTOR3(w, h, 0), D3DXVECTOR3(0, h, 0),
					D3DXVECTOR3(0, 0, 1), D3DXVECTOR3(w, 0, 1), D3DXVECTOR3(w, h, 1), D3DXVECTOR3(0, h, 1)
				};

				// Vertecies of the frustum
				D3DXVECTOR3 model[8];
				D3DXVec3UnprojectArray(
					model, sizeof(D3DXVECTOR3),
					screen, sizeof(D3DXVECTOR3),
					&viewport, &matProj, &matView, &matWorld, 8
				);

				// Normals to the frustrum sides (pointing inside)
				D3DXVECTOR3 nTop    = GetFrustumNormal(model[0], model[1], model[4]);
				D3DXVECTOR3 nRight  = GetFrustumNormal(model[1], model[2], model[5]);
				D3DXVECTOR3 nBottom = GetFrustumNormal(model[2], model[3], model[6]);
				D3DXVECTOR3 nLeft   = GetFrustumNormal(model[3], model[0], model[7]);
				D3DXVECTOR3 nNear   = GetFrustumNormal(model[0], model[3], model[1]);
				D3DXVECTOR3 ns[] = {nTop, nRight, nBottom, nLeft, nNear};
				D3DXVECTOR3 nPs[] = {model[0], model[1], model[2], model[3], model[0]};

				// Test against the sides
				bool outsideFrustum;
				for(int i = 0; i < (sizeof(ns) / sizeof(D3DXVECTOR3)); i++) {
					D3DXVECTOR3 n = ns[i];  // Normal
					D3DXVECTOR3 nP = nPs[i];  // Point on the side
					
					outsideFrustum = true;
					for(int j = 0; j < 8; j++) {
						D3DXVECTOR3 bbCorner = entity->mesh->boundingBox.corners[j];
						D3DXVECTOR3 bbCornerRelativeToP = bbCorner - nP;
						bool isIn = D3DXVec3Dot(&n, &bbCornerRelativeToP) > 0;
						if (isIn) {
							outsideFrustum = false;
							break; // Fail, try next side
						}
					}
					if (outsideFrustum) break; // Done
				}

				if (outsideFrustum) continue; // Skip entity
			}

			// Debug frustrum
			D3DXMATRIXA16 oldView;
			dev->GetTransform(D3DTS_VIEW, &oldView);
			if (keyToggled_Alt['D']) {
				D3DXMATRIXA16 newMove;
				D3DXMatrixTranslation(&newMove, 0, 0, 25);
				D3DXMATRIXA16 newView;
				D3DXMatrixMultiply(&newView, &oldView, &newMove);
				dev->SetTransform(D3DTS_VIEW, &newView);
			}

			entity->mesh->Render(dev, "OuterWall", "Path", entity->hiQuality ? "-Low" : "-Hi");
			stat_objRendered++;
			if (entity->mesh->filename == PipeFilename)
				stat_pipesRendered++;


			if (keyToggled_Alt['B']) {
				RenderBoundingBox(dev, entity->mesh->boundingBox);
			}

			dev->SetTransform(D3DTS_VIEW, &oldView);
		}

		hiQualityPipes = min(hiQualityPipes, stat_pipesRendered + 4); // Do not outgrow by more then 4
	}

	D3DXVECTOR3 GetFrustumNormal(D3DXVECTOR3 o, D3DXVECTOR3 a, D3DXVECTOR3 b)
	{
		D3DXVECTOR3 p = a - o;
		D3DXVECTOR3 q = b - o;
		D3DXVECTOR3 n;
		D3DXVec3Cross(&n, &p, &q);
		return n;
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

		map<string, IDirect3DTexture9*>::iterator it2 = loadedTextures.begin();
		while(it2 != loadedTextures.end()) {
			it2->second->Release();
			it2++;
		}
		loadedTextures.clear();
	}
};

MeshRenderer meshRenderer;