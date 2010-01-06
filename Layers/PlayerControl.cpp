#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

const double Epsilon = 0.001f;

extern Player* localPlayer;

class PlayerControl: InputLayer
{
	int mouseX;
	int mouseY;

public:
	PlayerControl(): mouseX(0), mouseY(0) {}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		mouseX = xPos;
		mouseY = yPos;
		return true;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		// Move the player
		if (keyDown['W'] || keyDown['S'] || keyDown['D'] || keyDown['A']) {
			MoveLocalPlayer(fElapsedTime);
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
		float angle = 90 - atan2(dir.z, dir.x) / 2 / D3DX_PI * 360.0f;

		return angle;
	}

	void MoveLocalPlayer(float fElapsedTime)
	{
		D3DXVECTOR3 pos = localPlayer->position;
		float direction = localPlayer->rotY / 360 * 2 * D3DX_PI;

		float speed = 0.0;
		if (keyDown['W']) speed = +PlayerMoveSpeed;
		if (keyDown['S']) speed = -PlayerMoveSpeed;
		pos.z += speed * fElapsedTime * cos(direction);
		pos.x += speed * fElapsedTime * sin(direction);

		float strafeSpeed = 0.0;
		if (keyDown['D']) strafeSpeed = +PlayerStrafeSpeed;
		if (keyDown['A']) strafeSpeed = -PlayerStrafeSpeed;
		pos.x += strafeSpeed * fElapsedTime  * cos(direction);
		pos.z -= strafeSpeed * fElapsedTime  * sin(direction);

		localPlayer->position = pos;
	}
};

PlayerControl playerControl;