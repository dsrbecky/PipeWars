#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Maths.h"
#include "../Resources.h"
#include "../Util.h"

extern Database db;
extern Database serverDb;
extern Player* localPlayer;
extern Resources resources;

extern float stat_netDatabaseUpdateSize;

void (*onCameraSet)(IDirect3DDevice9* dev) = NULL; // Event for others

const string PipeFilename = "pipe.dae";

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
	  cameraYaw(D3DX_PI / 4), cameraPitch(-D3DX_PI / 4 * 0.75f), cameraDistance(15),
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
		float flux1 = 0;
		float flux2 = 0;
		if (localPlayer == NULL) {
			flux1 = cos((float)DXUTGetTime() / 4) * D3DX_PI * 0.01f;
			flux2 = sin((float)DXUTGetTime() / 3) * D3DX_PI * 0.01f;
		}
		D3DXMATRIXA16 matPitch;
		D3DXMatrixRotationX(&matPitch, cameraPitch + flux1);
		D3DXMATRIXA16 matYaw;
		D3DXMatrixRotationY(&matYaw, cameraYaw + flux2);
		D3DXMATRIXA16 matYawPitch;
		D3DXMatrixMultiply(&matYawPitch, &matYaw, &matPitch);
		D3DXMATRIXA16 matZoom;
		if (localPlayer == NULL) {
			D3DXMatrixTranslation(&matZoom, 0, 0, 20);
		} else {
			D3DXMatrixTranslation(&matZoom, 0, 0, cameraDistance);
		}
		D3DXMATRIXA16 matView;
		D3DXMatrixMultiply(&matView, &matYawPitch, &matZoom);
		D3DXMATRIXA16 matMove;
		if (localPlayer == NULL) {
			D3DXMatrixTranslation(&matMove, 0, -2, -7);
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
		dev->SetRenderState(D3DRS_SPECULARENABLE, !keyToggled_Alt['S']);
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

		stat_objRendered = 0;
		stat_pipesRendered = 0;

		if (keyToggled_Alt['E']) {
			if (!keyToggled_Alt['F']) RenderFrameStats(dev);
			return;
		}

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
		{ DbLoop_Meshes(db, it)
			if (entity->getMesh()->isPipeOrTank) {
				D3DXVECTOR3 playerPos(0, 0, 0);
				if (localPlayer != NULL) playerPos = localPlayer->position;
				D3DXVECTOR3 entityPos = entity->ToWorldCoordinates(entity->getMesh()->boundingBox.centre);
				D3DXVECTOR3 delta = entityPos - playerPos;
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

		// Render meshes
		{ DbLoop_Meshes(db, it)
			Mesh* mesh = entity->getMesh();

			// Render only the level the player is in
			if (localPlayer != NULL && !keyToggled_Alt['L']) {
				float relY = entity->position.y - localPlayer->position.y;
				// Upper level
				if (relY > 1.75)
					continue; // Do not show at all
				// Lower level
				float minY;
				if (mesh->boundingBox.maxCorner.y >= 2.7) // The level up piece
					minY = -2.25;
				else 
					minY = -0.25;
				entity->showOutside = relY < minY;
				entity->showInside = relY > minY - 2;
			} else {
				entity->showOutside = false;
				entity->showInside = true;
			}

			// Is it hidden powerup?
			PowerUp* powerUp = dynamic_cast<PowerUp*>(entity);
			if (powerUp != NULL && !powerUp->present)
				continue;

			// Set the WORLD for the this entity

			D3DXMATRIXA16 matWorld;
			D3DXMatrixIdentity(&matWorld);

			D3DXMATRIXA16 matScale;
			D3DXMatrixScaling(&matScale, entity->scale, entity->scale, entity->scale);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matScale);

			D3DXMATRIXA16 matRot;
			D3DXMatrixRotationY(&matRot, -(entity->rotY + entity->rotY_multiplyByTime * (float)DXUTGetTime()) / 360 * 2 * D3DX_PI);
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
						D3DXVECTOR3 bbCorner = mesh->boundingBox.corners[j];
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

			D3DCOLOR on = 0xFFFFFFFF;
			D3DCOLOR off = 0;
			if (mesh->isPipeOrTank) {
				if (entity->showInside) {
					mesh->SetMaterialColor("-Low", entity->hiQuality ? off : on);
					mesh->SetMaterialColor("-Hi",  entity->hiQuality ? on : off);
					mesh->SetMaterialColor("InnerWall",  on);
				} else {
					mesh->SetMaterialColor("*", off);
				}
				mesh->SetMaterialColor("Path", keyToggled_Alt['P'] ? (DWORD)0xFFF800000 : off);
				mesh->SetMaterialColor("OuterWall", entity->showOutside ? on : off);
			}
			if (entity->GetType() == Player::Type) {
				mesh->SetMaterialColor("Armour", ((Player*)entity)->colorArmour);
				mesh->SetMaterialColor("ArmourDetail", ((Player*)entity)->colorArmoutDetail);
			}

			mesh->Render(dev);
			entity->hiQuality = false; // Reset
			stat_objRendered++;
			if (mesh->isPipeOrTank)
				stat_pipesRendered++;

			if (keyToggled_Alt['B']) {
				RenderBoundingBox(dev, mesh->boundingBox);
			}

			dev->SetTransform(D3DTS_VIEW, &oldView);
		}

		// Render player names
		if (!keyToggled_Alt['N'])
			RenderPlayerNames(dev, matProj, matView);

		hiQualityPipes = min(hiQualityPipes, stat_pipesRendered + 4); // Do not outgrow by more then 4

		if (keyToggled_Alt['G']) RenderGrid(dev);
		if (!keyToggled_Alt['F']) RenderFrameStats(dev);
	}

	void RenderPlayerNames(IDirect3DDevice9* dev, D3DXMATRIX& matProj, D3DXMATRIX& matView)
	{
		dev->SetRenderState(D3DRS_ZENABLE, false);
		{ DbLoop_Players(db, it)
			if (player != localPlayer) {
				D3DVIEWPORT9 unitViewport = {0, 0, 100, 100, 0, 1};
				D3DXVECTOR3 worldPos = player->position;
				worldPos.y += 0.6f;
				D3DXVECTOR3 screenPos;
				D3DXMATRIXA16 matWorld;
				D3DXMatrixIdentity(&matWorld);
				D3DXVec3Project(&screenPos, &worldPos, &unitViewport, &matProj, &matView, &matWorld);

				// Clip the point to screen corners if it is behind you
				screenPos.x = (screenPos.x / 50) - 1;
				screenPos.y = (screenPos.y / 50) - 1;
				if (screenPos.z > unitViewport.MaxZ) {
					screenPos.y /= abs(screenPos.x) + (float)Epsilon;
					screenPos.x = screenPos.x > 0 ? 1.0f : -1.0f;
					if (screenPos.y < -1 || screenPos.y > 1) {
						screenPos.x /= abs(screenPos.y);
						screenPos.y = screenPos.y > 0 ? 1.0f : -1.0f;
					}
					screenPos.x *= -1;
					screenPos.y *= -1;
				}
				screenPos.x = (screenPos.x + 1) / 2 * DXUTGetWindowWidth();
				screenPos.y = (screenPos.y + 1) / 2 * DXUTGetWindowHeight();

				RECT textSize = {0, 0, 0, 0};
				resources.LoadFont(dev)->DrawTextA(NULL, player->name, -1, &textSize, DT_NOCLIP | DT_SINGLELINE | DT_CALCRECT, player->colorArmoutDetail);
				RECT rect = {(int)screenPos.x - textSize.right / 2, (int)screenPos.y - textSize.bottom, 0, 0};
				rect.left = max(4, min(DXUTGetWindowWidth() - textSize.right - 4, rect.left));
				rect.top  = max(keyToggled_Alt['F'] ? 4 : 24, min(DXUTGetWindowHeight() - textSize.bottom - 64 - 4, rect.top));
				resources.LoadFont(dev)->DrawTextA(NULL, player->name, -1, &rect, DT_NOCLIP | DT_SINGLELINE, player->colorArmoutDetail);
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

	static const int gridSize = 25;

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
				lineCount++;
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
		if (localPlayer != NULL) {
			msg << "Net-in = " << stat_netDatabaseUpdateSize / DXUTGetElapsedTime() / 1000 << " kb/s" << "    ";
			msg << "Pos = " << localPlayer->position.x << ","<< localPlayer->position.y << "," << localPlayer->position.z << "    ";
			// msg << "Rot = " << localPlayer->rotY << " ("<< localPlayer->rotY_velocity << ")" << "    ";
		}
		msg << "Press H for help or ESC to exit.";
		
		textX = 8; textY = 8;
		RenderText(dev, msg.str());
	}

	void ReleaseDeviceResources()
	{
		if (gridBuffer != NULL) {
			gridBuffer->Release();
			gridBuffer = NULL;
		}
	}
};

Renderer renderer;