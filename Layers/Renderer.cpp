#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"
#include <set>
#include <iomanip>
#include "../Math.h"

extern map<string, Mesh*> loadedMeshes;
extern map<string, IDirect3DTexture9*> loadedTextures;
extern Player* localPlayer;

void (*onCameraSet)(IDirect3DDevice9* dev) = NULL; // Event for others

class Renderer: Layer
{
	IDirect3DVertexBuffer9* gridBuffer;

	float cameraYaw;
	float cameraPitch;
	float cameraDistance;
	int lastMouseX;
	int lastMouseY;

	static const int targetFPS = 30;
	float hiQualityPipes;
	int stat_objRendered;
	int stat_pipesRendered;

public:

	Renderer():
	  gridBuffer(NULL), hiQualityPipes(0),
	  cameraYaw(D3DX_PI / 4), cameraPitch(-D3DX_PI / 4), cameraDistance(10),
	  lastMouseX(0), lastMouseY(0) {}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		// Camera movement
		if (bMiddleButtonDown) {
			int deltaX = xPos - lastMouseX;
			int deltaY = yPos - lastMouseY;
			cameraPitch -= (float)deltaY / 200.0f;
			cameraYaw -= (float)deltaX / 200.0f;
			cameraPitch = max(-D3DX_PI/2, min(cameraPitch, D3DX_PI/2));
		}

		if (nMouseWheelDelta != 0) {
			cameraDistance -= (float)nMouseWheelDelta / 120.0f * 3;
			cameraDistance = max(cameraDistance, 4);
		}

		lastMouseX = xPos;
		lastMouseY = yPos;

		return false;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		float extraFPS = DXUTGetFPS() - targetFPS;
		if (extraFPS < 0) {
			hiQualityPipes += fElapsedTime * extraFPS * 0.5f;
		} else {
			hiQualityPipes += fElapsedTime * extraFPS * 0.05f;
		}
		hiQualityPipes = max(0, hiQualityPipes);
	}

	void SetupCamera(IDirect3DDevice9* dev)
	{
		D3DXMATRIXA16 matPitch;
		D3DXMatrixRotationX(&matPitch, cameraPitch);
		D3DXMATRIXA16 matYaw;
		D3DXMatrixRotationY(&matYaw, cameraYaw);
		D3DXMATRIXA16 matYawPitch;
		D3DXMatrixMultiply(&matYawPitch, &matYaw, &matPitch);
		D3DXMATRIXA16 matZoom;
		D3DXMatrixTranslation(&matZoom, 0, 0, cameraDistance);
		D3DXMATRIXA16 matView;
		D3DXMatrixMultiply(&matView, &matYawPitch, &matZoom);
		D3DXMATRIXA16 matMove;
		if (localPlayer == NULL) {
			D3DXMatrixIdentity(&matMove);
		} else {
			D3DXMatrixTranslation(&matMove, -localPlayer->position.x, -localPlayer->position.y, -localPlayer->position.z);
		}
		D3DXMATRIXA16 matMoveView;
		D3DXMatrixMultiply(&matMoveView, &matMove, &matView);
		dev->SetTransform(D3DTS_VIEW, &matMoveView);

		// Prespective
		D3DXMATRIXA16 matProj;
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, (float)DXUTGetWindowWidth() / DXUTGetWindowHeight(), NearClip, FarClip);
		dev->SetTransform(D3DTS_PROJECTION, &matProj);

		if (onCameraSet != NULL) {
			onCameraSet(dev);
		}
	}

	void SetupLight(IDirect3DDevice9* dev)
	{
		dev->SetRenderState(D3DRS_LIGHTING, true);
		dev->SetRenderState(D3DRS_AMBIENT, 0);
		dev->SetRenderState(D3DRS_SPECULARENABLE, !keyToggled_Alt['S'] );
		dev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
		dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

		D3DLIGHT9 light;
		ZeroMemory(&light, sizeof(light));

		light.Type = D3DLIGHT_DIRECTIONAL;

		light.Direction.x = -100.0f;
		light.Direction.y = -500.0f;
		light.Direction.z =   0.0f;
		
		D3DCOLORVALUE ambi = {0.3f, 0.3f, 0.3f, 1.0f};
		D3DCOLORVALUE diff = {1.0f, 1.0f, 1.0f, 1.0f};
		D3DCOLORVALUE spec = {0.3f, 0.3f, 0.1f, 1.0f};

		if (!keyToggled_Alt['A']) light.Ambient  = ambi;
		if (!keyToggled_Alt['D']) light.Diffuse  = diff;
		if (!keyToggled_Alt['S']) light.Specular = spec;
		    
		// Don't attenuate.
		light.Attenuation0 = 1.0f; 
		light.Range        = 10000.0f;
		
		dev->SetLight(0, &light);
		dev->LightEnable(0, true);
	}

	void Render(IDirect3DDevice9* dev)
	{
		dev->SetRenderState(D3DRS_FILLMODE, keyToggled_Alt['W'] ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
	    dev->SetRenderState(D3DRS_ZENABLE, !keyToggled_Alt['Z']);
		
		SetupCamera(dev);
		SetupLight(dev);

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
		for(list<Entity*>::iterator it = db.entities.begin(); it != db.entities.end(); it++) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(*it);
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
		for(list<Entity*>::iterator it = db.entities.begin(); it != db.entities.end(); it++) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(*it);
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
			if (!keyToggled_Alt['C']) {

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
			if (keyToggled_Alt['V']) {
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

		if (keyToggled_Alt['G']) {
			RenderGrid(dev);
		}

		if (!keyToggled_Alt['F']) {
			RenderFrameStats(dev);
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

	static const int gridSize = 100;

	void RenderGrid(IDirect3DDevice9* dev)
	{
		int fvf = D3DFVF_XYZ;
		static int lineCount;

		// Create buffer on demand
		if (gridBuffer == NULL) {
			vector<float> vb;
			for(int i = -gridSize; i <= gridSize; i++) {
				// Along X
				vb.push_back((float)-gridSize); vb.push_back(0); vb.push_back((float)i);
				vb.push_back((float)+gridSize); vb.push_back(0); vb.push_back((float)i);
				/// Along Z
				vb.push_back((float)i); vb.push_back(0); vb.push_back((float)-gridSize);
				vb.push_back((float)i); vb.push_back(0); vb.push_back((float)+gridSize);
				lineCount++;
			}

			// Copy the buffer to graphic card memory

			dev->CreateVertexBuffer(vb.size() * sizeof(float), 0, fvf, D3DPOOL_DEFAULT, &gridBuffer, NULL);
			void* bufferData;
			gridBuffer->Lock(0, vb.size() * sizeof(float), &bufferData, 0);
			copy(vb.begin(), vb.end(), (float*)bufferData);
			gridBuffer->Unlock();
		}

		D3DMATERIAL9 material;
		ZeroMemory(&material, sizeof(material));
		material.Emissive.a = 1;
		material.Emissive.r = 0.5;

		D3DXMATRIXA16 matWorld;
		D3DXMatrixIdentity(&matWorld);
		dev->SetTransform(D3DTS_WORLD, &matWorld);

		dev->SetStreamSource(0, gridBuffer, 0, 3 * sizeof(float));
		dev->SetFVF(fvf);
		dev->SetTexture(0, NULL);
		dev->SetMaterial(&material);
		dev->DrawPrimitive(D3DPT_LINELIST, 0, lineCount);
	}

	void RenderFrameStats(IDirect3DDevice9* dev)
	{
		ostringstream msg;
		msg << fixed << std::setprecision(1);
		msg << "FPS = " << DXUTGetFPS() << " (target = " << targetFPS << ")" << "    ";
		msg << "Objects = " << stat_objRendered << " (hq = " << (int)hiQualityPipes << ")" << "    ";
		msg << "Pos = " << localPlayer->position.x << ","<< localPlayer->position.y << ","<< localPlayer->position.z << "    ";
		msg << "Press H for help or ESC to exit.";
		
		textX = 8; textY = 8;
		RenderText(dev, msg.str());
	}

	void ReleaseDeviceResources()
	{
		Layer::ReleaseDeviceResources();

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

		if (gridBuffer != NULL) {
			gridBuffer->Release();
			gridBuffer = NULL;
		}
	}
};

Renderer renderer;