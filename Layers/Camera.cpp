#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

// Sets the camera angle
class Camera: public Layer
{
	float yaw;
	float pitch;
	float distance;

	int lastMouseX;
	int lastMouseY;

public:

	Camera(): yaw(D3DX_PI / 4), pitch(-D3DX_PI / 4), distance(60), lastMouseX(0), lastMouseY(0) {}

	bool Camera::MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
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
			distance = max(distance, 6);
		}

		lastMouseX = xPos;
		lastMouseY = yPos;
		return FALSE;
	}

	void Camera::PreRender()
	{
		// View matrix
		D3DXMATRIXA16 matPitch;
		D3DXMatrixRotationX(&matPitch, pitch);
		D3DXMATRIXA16 matYaw;
		D3DXMatrixRotationY(&matYaw, yaw);
		D3DXMATRIXA16 matYawPitch;
		D3DXMatrixMultiply(&matYawPitch, &matYaw, &matPitch);
		D3DXMATRIXA16 matMove;
		D3DXMatrixTranslation(&matMove, 0, 0, distance);
		D3DXMATRIXA16 matView;
		D3DXMatrixMultiply(&matView, &matYawPitch, &matMove);
		pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

		// Prespective
		D3DXMATRIXA16 matProj;
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, (float)DXUTGetWindowWidth() / DXUTGetWindowHeight(), 1.0f, 4000.0f);
		pD3DDevice->SetTransform(D3DTS_PROJECTION, &matProj);
	}
};

Camera camera;