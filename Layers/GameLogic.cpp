#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"
#include "../Math.h"

extern Database db;
extern Player* localPlayer;
extern void (*onCameraSet)(IDirect3DDevice9* dev); // Event for others

class GameLogic: Layer
{
	bool firing;
	static int lastMouseX;
	static int lastMouseY;

	double nextLoadedTime;

public:

	GameLogic(): firing(false), nextLoadedTime(0) {}

	bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		Layer::KeyboardProc(nChar, bKeyDown, bAltDown);

		// Select player weapon
		switch(nChar) {
			case '1': localPlayer->selectWeapon(Weapon_Revolver); return true;
			case '2': localPlayer->selectWeapon(Weapon_Shotgun); return true;
			case '3': localPlayer->selectWeapon(Weapon_AK47); return true;
			case '4': localPlayer->selectWeapon(Weapon_Jackhammer); return true;
			case '5': localPlayer->selectWeapon(Weapon_Nailgun); return true;
		}
		return false;
	}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		firing = bLeftButtonDown;
		lastMouseX = xPos;
		lastMouseY = yPos;
		return firing;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		// Move the player
		MoveLocalPlayer(fElapsedTime);

		// Rotate the player towards the mouse
		onCameraSet = &RotateLocalPlayer;

		// Fire
		if (firing && fTime >= nextLoadedTime) {
			int* ammo = &localPlayer->inventory[Ammo_Revolver + localPlayer->selectedWeapon];
			if (*ammo > 0) {
				float speed = 15.0;
				float reloadTime = 0.5;
				float range = 10;
				Bullet* bullet = new Bullet(localPlayer, localPlayer->selectedWeapon);
				bullet->position = localPlayer->position;
				bullet->rotY = localPlayer->rotY + 90;
				bullet->velocity = RotYToDirecion(localPlayer->rotY) * speed;
				bullet->rangeLeft = range;
				db.add(bullet);
				(*ammo)--;
				nextLoadedTime = fTime + reloadTime;
			}
		}

		// Move entities
		for(list<Entity*>::iterator it = db.entities.begin(); it != db.entities.end();) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(*it);
			if (entity == NULL) {
				it++; continue;
			}

			entity->position += entity->velocity * fElapsedTime;
			entity->rotY += entity->rotY_velocity * fElapsedTime;

			Bullet* bullet = dynamic_cast<Bullet*>(entity);
			if (bullet != NULL) {
				bullet->rangeLeft -= D3DXVec3Length(&entity->velocity) * fElapsedTime;
				D3DXVECTOR3 offset1(+0.3f, 0, 0.3f);
				D3DXVECTOR3 offset2(-0.3f, 0, 0.3f);
				bool onPath = IsPointOnPath(bullet->position) || 
					IsPointOnPath(bullet->position + offset1) || IsPointOnPath(bullet->position - offset1) ||
					IsPointOnPath(bullet->position + offset2) || IsPointOnPath(bullet->position - offset2);
				if (bullet->rangeLeft < 0 || !onPath) {
					it = db.entities.erase(it);
					continue;
				}
			}

			it++;
		}
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
			float speed = 0.0;
			if (keyDown['W']) speed += +PlayerMoveSpeed;
			if (keyDown['S']) speed += -PlayerMoveSpeed;
			pos = pos + RotYToDirecion(direction) * speed * fElapsedTime * cos(dirOffset / 360 * 2 * D3DX_PI);

			// Move left / right
			float strafeSpeed = 0.0;
			if (keyDown['D']) strafeSpeed += +PlayerStrafeSpeed;
			if (keyDown['A']) strafeSpeed += -PlayerStrafeSpeed;
			pos = pos + RotYToDirecion(direction - 90) * strafeSpeed * fElapsedTime * cos(dirOffset / 360 * 2 * D3DX_PI); 

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
};

int GameLogic::lastMouseX = 0;
int GameLogic::lastMouseY = 0;

GameLogic gameLogic;