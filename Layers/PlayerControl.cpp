#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

const double Epsilon = 0.001f;

extern Database db;
extern Player* localPlayer;

class PlayerControl: InputLayer
{
	bool mouseDown;
	int mouseX;
	int mouseY;

	double nextLoadedTime;
public:
	PlayerControl(): mouseX(0), mouseY(0), nextLoadedTime(0) {}

	bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		InputLayer::KeyboardProc(nChar, bKeyDown, bAltDown);

		switch(nChar) {
			case '1': localPlayer->selectWeapon(Weapon_Revolver); break;
			case '2': localPlayer->selectWeapon(Weapon_Shotgun); break;
			case '3': localPlayer->selectWeapon(Weapon_AK47); break;
			case '4': localPlayer->selectWeapon(Weapon_Jackhammer); break;
			case '5': localPlayer->selectWeapon(Weapon_Nailgun); break;
		}
		return false;
	}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		mouseDown = bLeftButtonDown;
		mouseX = xPos;
		mouseY = yPos;
		return true;
	}


	void FrameMove(double fTime, float fElapsedTime)
	{
		// Move the player
		MoveLocalPlayer(fElapsedTime);

		// Shoot
		if (mouseDown && fTime >= nextLoadedTime) {
			int* ammo = &localPlayer->inventory[Ammo_Revolver + localPlayer->selectedWeapon];
			if (*ammo > 0) {
				float speed = 15.0;
				float reloadTime = 0.5;
				float range = 10;
				Bullet* bullet = new Bullet(localPlayer, localPlayer->selectedWeapon);
				bullet->position = localPlayer->position;
				bullet->rotY = localPlayer->rotY - 90;
				bullet->velocity = RotYToDirecion(localPlayer->rotY) * speed;
				bullet->rangeLeft = range;
				db.add(bullet);
				(*ammo)--;
				nextLoadedTime = fTime + reloadTime;
			}
		}
	}

	void PreRender(IDirect3DDevice9* dev)
	{
		// Look at the mouse
		localPlayer->rotY = Unproject(dev, mouseX, mouseY);
	}

	float Unproject(IDirect3DDevice9* dev, int x, int y)
	{
        D3DXVECTOR3 curPos_Near((float)x, (float)y, 0.25f);
        D3DXVECTOR3 curPos_Far((float)x, (float)y, 0.5f);
		D3DVIEWPORT9 viewport;
		D3DXMATRIX matProj;
		D3DXMATRIX matView;
		D3DXMATRIX matWorld;
		dev->GetViewport(&viewport);
		dev->GetTransform(D3DTS_PROJECTION, &matProj);
		dev->GetTransform(D3DTS_VIEW, &matView);
		D3DXMatrixIdentity(&matWorld);
		D3DXVECTOR3 pos3D_Near;
		D3DXVECTOR3 pos3D_Far;
		D3DXVec3Unproject(&pos3D_Near, &curPos_Near, &viewport, &matProj, &matView, &matWorld);
		D3DXVec3Unproject(&pos3D_Far,  &curPos_Far,  &viewport, &matProj, &matView, &matWorld);

		float targetY = localPlayer->position.y;

		// (1 - t) * pos3D_Near.y + (t) * pos3D_Far.y = targetY
		// pos3D_Near.y - t * pos3D_Near.y + t * pos3D_Far.y = targetY
		// pos3D_Near.y - targetY = t * (pos3D_Near.y - pos3D_Far.y)
		// t = (pos3D_Near.y - targetY) / (pos3D_Near.y - pos3D_Far.y)

		if (abs(pos3D_Near.y - pos3D_Far.y) < Epsilon)
			return 0;

		float t = (pos3D_Near.y - targetY) / (pos3D_Near.y - pos3D_Far.y);
		
		D3DXVECTOR3 posOnYplane = (1 - t) * pos3D_Near + t * pos3D_Far;
		D3DXVECTOR3 dir = posOnYplane - localPlayer->position;
		float angle = atan2(dir.z, dir.x) / 2 / D3DX_PI * 360.0f - 90;

		return angle;
	}

	void MoveLocalPlayer(float fElapsedTime)
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
				return; // Done, moved
			}
		}
		// Can not move 
	}
};

PlayerControl playerControl;