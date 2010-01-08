#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

extern Player* localPlayer;

class Camera: public Layer
{
	float yaw;
	float pitch;
	float distance;

	int lastMouseX;
	int lastMouseY;

public:

	Camera(): yaw(D3DX_PI / 4), pitch(-D3DX_PI / 4), distance(10), lastMouseX(0), lastMouseY(0) {}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		if (bMiddleButtonDown) {
			int deltaX = xPos - lastMouseX;
			int deltaY = yPos - lastMouseY;
			pitch -= (float)deltaY / 200.0f;
			yaw -= (float)deltaX / 200.0f;
			pitch = max(-D3DX_PI/2, min(pitch, D3DX_PI/2));
		}

		if (nMouseWheelDelta != 0) {
			distance -= (float)nMouseWheelDelta / 120.0f * 3;
			distance = max(distance, 4);
		}

		lastMouseX = xPos;
		lastMouseY = yPos;

		return false;
	}

	void PreRender(IDirect3DDevice9* dev)
	{
		// View matrix
		D3DXMATRIXA16 matPitch;
		D3DXMatrixRotationX(&matPitch, pitch);
		D3DXMATRIXA16 matYaw;
		D3DXMatrixRotationY(&matYaw, yaw);
		D3DXMATRIXA16 matYawPitch;
		D3DXMatrixMultiply(&matYawPitch, &matYaw, &matPitch);
		D3DXMATRIXA16 matZoom;
		D3DXMatrixTranslation(&matZoom, 0, 0, distance);
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
	}
};

Camera camera;