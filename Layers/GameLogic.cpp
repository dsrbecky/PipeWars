#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"
#include "../Math.h"

extern Database db;
extern Player* localPlayer;

extern void network_SendIncrementalUpdateToClients();

extern void (*onCameraSet)(IDirect3DDevice9* dev); // Renderer

class GameLogic: Layer
{
	static int lastMouseX;
	static int lastMouseY;

	double nextLoadedTime;

public:

	GameLogic(): nextLoadedTime(0) {}

	bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		Layer::KeyboardProc(nChar, bKeyDown, bAltDown);

		// Select player weapon
		if (localPlayer != NULL) {
			switch(nChar) {
				case '1': localPlayer->selectWeapon(Weapon_Revolver); return true;
				case '2': localPlayer->selectWeapon(Weapon_Shotgun); return true;
				case '3': localPlayer->selectWeapon(Weapon_AK47); return true;
				case '4': localPlayer->selectWeapon(Weapon_Jackhammer); return true;
				case '5': localPlayer->selectWeapon(Weapon_Nailgun); return true;
			}
		}
		return false;
	}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		if (localPlayer != NULL)
			localPlayer->firing = bLeftButtonDown;
		lastMouseX = xPos;
		lastMouseY = yPos;
		return false;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		if (localPlayer != NULL) {
			// Move the player
			MoveLocalPlayer(fElapsedTime);

			// Rotate the player towards the mouse
			onCameraSet = &RotateLocalPlayer;

			// Fire
			if (localPlayer->firing && fTime >= nextLoadedTime) {
				int* ammo = &localPlayer->inventory[Ammo_Revolver + localPlayer->selectedWeapon];
				if (*ammo > 0) {
					float speed = 15.0;
					float reloadTime = 0.5;
					float range = 10;
					Bullet* bullet = new Bullet(localPlayer, localPlayer->selectedWeapon, localPlayer->position, range);
					bullet->rotY = localPlayer->rotY + 90;
					bullet->velocityRight = speed;
					db.add(bullet);
					(*ammo)--;
					nextLoadedTime = fTime + reloadTime;
				}
			}
		} else {
			onCameraSet = NULL;
		}

		vector<Entity*> toDelete;

		// Move entities
		DbLoop(it) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(it->second);
			if (entity == NULL || entity == localPlayer)
				continue;

			if (entity->velocityForward != 0.0f)
				entity->position += RotateY(entity->rotY) * (entity->velocityForward * fElapsedTime);
			if (entity->velocityRight != 0.0f)
				entity->position += RotateY(entity->rotY - 90) * (entity->velocityRight * fElapsedTime);
			entity->rotY += entity->rotY_velocity * fElapsedTime;

			Bullet* bullet = dynamic_cast<Bullet*>(entity);
			if (bullet != NULL) {
				D3DXVECTOR3 pathTraveled = bullet->position - bullet->origin;
				float distTraveled = D3DXVec3Length(&pathTraveled);

				D3DXVECTOR3 offset1(+0.3f, 0, 0.3f);
				D3DXVECTOR3 offset2(-0.3f, 0, 0.3f);
				bool onPath = IsPointOnPath(bullet->position) || 
					IsPointOnPath(bullet->position + offset1) || IsPointOnPath(bullet->position - offset1) ||
					IsPointOnPath(bullet->position + offset2) || IsPointOnPath(bullet->position - offset2);

				if (distTraveled > bullet->range || !onPath) {
					toDelete.push_back(bullet);
					continue;
				}
			}
		}

		// Perform the deletes
		for (vector<Entity*>::iterator it = toDelete.begin(); it != toDelete.end(); it++) {
			db.remove(*it);
		}

		network_SendIncrementalUpdateToClients();
	}

	static void RotateLocalPlayer(IDirect3DDevice9* dev)
	{
		D3DXVECTOR3 curPos_Near((float)lastMouseX, (float)lastMouseY, 0.25f);
        D3DXVECTOR3 curPos_Far((float)lastMouseX, (float)lastMouseY, 0.5f);
		D3DVIEWPORT9 viewport;  dev->GetViewport(&viewport);
		D3DXMATRIX matProj;     dev->GetTransform(D3DTS_PROJECTION, &matProj);
		D3DXMATRIX matView;     dev->GetTransform(D3DTS_VIEW, &matView);
		D3DXMATRIX matWorld;    D3DXMatrixIdentity(&matWorld);
		D3DXVECTOR3 pos3D_Near; D3DXVec3Unproject(&pos3D_Near, &curPos_Near, &viewport, &matProj, &matView, &matWorld);
		D3DXVECTOR3 pos3D_Far;  D3DXVec3Unproject(&pos3D_Far,  &curPos_Far,  &viewport, &matProj, &matView, &matWorld);

		float targetY = localPlayer->position.y;

		// (1 - t) * pos3D_Near.y + (t) * pos3D_Far.y = targetY
		// pos3D_Near.y - t * pos3D_Near.y + t * pos3D_Far.y = targetY
		// pos3D_Near.y - targetY = t * (pos3D_Near.y - pos3D_Far.y)
		// t = (pos3D_Near.y - targetY) / (pos3D_Near.y - pos3D_Far.y)

		if (abs(pos3D_Near.y - pos3D_Far.y) < Epsilon) return;

		float t = (pos3D_Near.y - targetY) / (pos3D_Near.y - pos3D_Far.y);
		
		D3DXVECTOR3 posOnYplane = (1 - t) * pos3D_Near + t * pos3D_Far;
		D3DXVECTOR3 dir = posOnYplane - localPlayer->position;
		float angle = atan2(dir.z, dir.x) / 2 / D3DX_PI * 360.0f - 90;

		localPlayer->rotY = angle;
	}

	bool MoveLocalPlayer(float fElapsedTime)
	{
		fElapsedTime = min(0.1f, fElapsedTime);

		// Try to move at an angle if you can not go forward
		static float dirOffsets[] = {0, -10, 10, -25, 25, -40, 40, -55, 55, -70, 70, -85, 85};

		for (int i = 0; i < (sizeof(dirOffsets) / sizeof(float)); i++) {
			float dirOffset = dirOffsets[i];

			D3DXVECTOR3 pos = localPlayer->position;
			float direction = localPlayer->rotY + dirOffset;

			// Move forward / back
			localPlayer->velocityForward = 0.0f;
			if (keyDown['W']) localPlayer->velocityForward += +PlayerMoveSpeed;
			if (keyDown['S']) localPlayer->velocityForward += -PlayerMoveSpeed;
			pos = pos + RotateY(direction) * (localPlayer->velocityForward * fElapsedTime * cos(dirOffset / 360 * 2 * D3DX_PI));

			// Move left / right
			localPlayer->velocityRight = 0.0f;
			if (keyDown['D']) localPlayer->velocityRight += +PlayerStrafeSpeed;
			if (keyDown['A']) localPlayer->velocityRight += -PlayerStrafeSpeed;
			pos = pos + RotateY(direction - 90) * (localPlayer->velocityRight * fElapsedTime * cos(dirOffset / 360 * 2 * D3DX_PI)); 

			float outY;
			if (IsPointOnPath(pos, &outY)) {
				pos.y = outY + PlayerRaiseAbovePath;
				localPlayer->position = pos;
				return true; // Done, moved
			}
		}
		// Can not move
		return false;
	}

	bool IsPointOnPath(D3DXVECTOR3 pos, float* outY = NULL)
	{
		DbLoop(it) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(it->second);
			if (entity == NULL)
				continue;
			// Position relative to the mesh
			D3DXVECTOR3 meshPos = RotateY(-entity->rotY, pos - entity->position) / entity->scale;
			// Quick test - is in BoundingBox?
			if (entity->getMesh()->boundingBox.Contains(meshPos)) {
				if (entity->getMesh()->IsOnPath(meshPos.x, meshPos.z, outY)) {
					if (outY != NULL) {
						*outY = *outY * entity->scale + entity->position.y;
					}
					return true;
				}
			}
		}
		return false;
	}
};

int GameLogic::lastMouseX = 0;
int GameLogic::lastMouseY = 0;

GameLogic gameLogic;