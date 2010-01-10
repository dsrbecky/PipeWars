#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"
#include "../Math.h"
#include "../Network.h"

extern Database db;
extern Player* localPlayer;
extern Network network;

extern void (*onCameraSet)(IDirect3DDevice9* dev); // Renderer
bool MovePlayer(Player* player, float fElapsedTime);
bool IsPointOnPath(Database& db, D3DXVECTOR3 pos, float* outY = NULL);

extern vector<UCHAR> lastClientUpdate;
extern vector<UCHAR> lastDataBaseUpdate;

class ClientLogic: Layer
{
	static int lastMouseX;
	static int lastMouseY;

public:

	ClientLogic() {}

	bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		Layer::KeyboardProc(nChar, bKeyDown, bAltDown);

		// Select player weapon
		if (localPlayer != NULL) {
			switch(nChar) {
				case '1': localPlayer->trySelectWeapon(Weapon_Revolver); return true;
				case '2': localPlayer->trySelectWeapon(Weapon_Shotgun); return true;
				case '3': localPlayer->trySelectWeapon(Weapon_AK47); return true;
				case '4': localPlayer->trySelectWeapon(Weapon_Jackhammer); return true;
				case '5': localPlayer->trySelectWeapon(Weapon_Nailgun); return true;
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
		vector<UCHAR> bk;
		if (localPlayer != NULL)
			network.SendPlayerData(bk, localPlayer);
		if (lastDataBaseUpdate.size() > 0) {
			network.RecvDatabase(lastDataBaseUpdate.begin(), db);
			if (localPlayer == NULL)
				localPlayer = dynamic_cast<Player*>(db[2]);
		}
		if (bk.size() > 0)
			network.RecvPlayerData(bk.begin(), db);

		if (localPlayer != NULL) {
			// Set velocity forward / back
			localPlayer->velocityForward = 0.0f;
			if (keyDown['W']) localPlayer->velocityForward += +PlayerMoveSpeed;
			if (keyDown['S']) localPlayer->velocityForward += -PlayerMoveSpeed;
			
			// Set velocity left / right
			localPlayer->velocityRight = 0.0f;
			if (keyDown['D']) localPlayer->velocityRight += +PlayerStrafeSpeed;
			if (keyDown['A']) localPlayer->velocityRight += -PlayerStrafeSpeed;

			MovePlayer(localPlayer, fElapsedTime);

			// Rotate the player towards the mouse
			onCameraSet = &RotateLocalPlayer;
		} else {
			onCameraSet = NULL;
		}

		DbLoop(it) {
			if (it->second == localPlayer) continue;

			MeshEntity* entity = dynamic_cast<MeshEntity*>(it->second);
			if (entity == NULL) continue;

			Player* player = dynamic_cast<Player*>(it->second);
			if (player != NULL) {
				// Move player
				MovePlayer(player, fElapsedTime);
				player->rotY += player->rotY_velocity * fElapsedTime;
			} else {
				// Move other entities
				if (entity->velocityForward != 0.0f)
					entity->position += RotateY(entity->rotY) * (entity->velocityForward * fElapsedTime);
				if (entity->velocityRight != 0.0f)
					entity->position += RotateY(entity->rotY - 90) * (entity->velocityRight * fElapsedTime);
				entity->rotY += entity->rotY_velocity * fElapsedTime;
			}
		}

		lastClientUpdate.clear();
		if (localPlayer != NULL)
			network.SendPlayerData(lastClientUpdate, localPlayer);
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

		float time = DXUTGetElapsedTime();
		float velocity = (angle - localPlayer->rotY) / time;
		velocity = max(-360, min(360, velocity));

		localPlayer->rotY = angle;
		localPlayer->rotY_velocity = 0.75f * localPlayer->rotY_velocity + 0.25f * velocity;
	}
};

int ClientLogic::lastMouseX = 0;
int ClientLogic::lastMouseY = 0;

ClientLogic clientLogic;


bool MovePlayer(Player* player, float fElapsedTime)
{
	// Try to move at an angle if you can not go forward
	static float dirOffsets[] = {0, -10, 10, -25, 25, -40, 40, -55, 55, -70, 70, -85, 85};

	for (int i = 0; i < (sizeof(dirOffsets) / sizeof(float)); i++) {
		float dirOffset = dirOffsets[i];

		D3DXVECTOR3 pos = player->position;
		float direction = player->rotY + dirOffset;
		
		// Move forward / back
		pos = pos + RotateY(direction) * (player->velocityForward * fElapsedTime * cos(dirOffset / 360 * 2 * D3DX_PI));

		// Move left / right
		pos = pos + RotateY(direction - 90) * (player->velocityRight * fElapsedTime * cos(dirOffset / 360 * 2 * D3DX_PI)); 

		float outY;
		if (IsPointOnPath(db, pos, &outY)) {
			pos.y = outY + PlayerRaiseAbovePath;
			player->position = pos;
			return true; // Done, moved
		}
	}
	return false; // Can not move
}

bool IsPointOnPath(Database& db, D3DXVECTOR3 pos, float* outY)
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