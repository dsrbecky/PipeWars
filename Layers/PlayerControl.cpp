#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

const double Epsilon = 0.001f;

extern Database db;
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
		float angle = atan2(dir.z, dir.x) / 2 / D3DX_PI * 360.0f - 90;

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
		pos.x -= speed * fElapsedTime * sin(direction);

		float strafeSpeed = 0.0;
		if (keyDown['D']) strafeSpeed = +PlayerStrafeSpeed;
		if (keyDown['A']) strafeSpeed = -PlayerStrafeSpeed;
		pos.x += strafeSpeed * fElapsedTime * cos(direction);
		pos.z += strafeSpeed * fElapsedTime * sin(direction);

		for(int i = 0; i < (int)db.entities.size(); i++) {
			// Pipe or tank
			if (dynamic_cast<Pipe*>(db.entities[i]) != NULL || dynamic_cast<Tank*>(db.entities[i]) != NULL) {
				MeshEntity* entity = dynamic_cast<MeshEntity*>(db.entities[i]);
				// Position relative to the mesh
				D3DXVECTOR3 meshPos = RotateY(pos - entity->position, -entity->rotY) / entity->scale;
				
			}
		}

		localPlayer->position = pos;
	}

	// Rotates point cc-wise around Y
	D3DXVECTOR3 RotateY(D3DXVECTOR3 vec, float angleDeg)
	{
		while(angleDeg < 0) angleDeg += 360;
		while(angleDeg >= 360) angleDeg -= 360;
		if (angleDeg ==   0) return vec;
		if (angleDeg ==  90) return D3DXVECTOR3(-vec.z, vec.y, vec.x);
		if (angleDeg == 180) return D3DXVECTOR3(-vec.x, vec.y, -vec.z);
		if (angleDeg == 270) return D3DXVECTOR3(vec.z, vec.y, -vec.x);
		
		float angleRad = angleDeg / 360 * 2 * D3DX_PI;
		float cosAlfa = cos(angleRad);
		float sinAlfa = sin(angleRad);
		float x = vec.x * cosAlfa + vec.z * -sinAlfa;   // cos -sin   vec.x
		float z = vec.x * sinAlfa + vec.z *  cosAlfa;   // sin  cos   vec.z
		return D3DXVECTOR3(x, vec.y, z);
	}
};

PlayerControl playerControl;