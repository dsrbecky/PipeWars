#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"
#include "../Math.h"
#include "../Network/Network.h"

extern Player* localPlayer;
extern Resources resources;

extern Database db;
extern Database serverDb;
extern Network clientNetwork;
extern Network serverNetwork;

extern void (*onCameraSet)(IDirect3DDevice9* dev); // Renderer

static int lastMouseX = 0;
static int lastMouseY = 0;

class GameLogic: Layer
{
public:

	GameLogic() {}

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
		if (clientNetwork.clientRunning) {
			PerformClientLogic(db, fTime, fElapsedTime);
		}

		if (serverNetwork.serverRunning) {
			PerformServerLogic(serverDb, fTime, fElapsedTime);
		}

		if (resources.fmodSystem != NULL) {
			resources.fmodSystem->update();
			resources.fmodChannel->setPaused(keyToggled['M'] ^ keyToggled_Alt['M']);
		}
	}

	void PerformClientLogic(Database& database, double fTime, float fElapsedTime)
	{
		// Move local player
		if (localPlayer != NULL) {
			// Set velocity
			localPlayer->velocityForward = localPlayer->velocityRight = 0.0f;
			if (keyDown['W'] || keyDown[VK_UP])    localPlayer->velocityForward += +PlayerMoveSpeed;
			if (keyDown['S'] || keyDown[VK_DOWN])  localPlayer->velocityForward += -PlayerMoveSpeed;
			if (keyDown['D'] || keyDown[VK_RIGHT]) localPlayer->velocityRight   += +PlayerStrafeSpeed;
			if (keyDown['A'] || keyDown[VK_LEFT])  localPlayer->velocityRight   += -PlayerStrafeSpeed;

			MovePlayer(database, localPlayer, fElapsedTime);

			// Rotate the player towards the mouse
			onCameraSet = &RotateLocalPlayer;
		} else {
			onCameraSet = NULL;
		}

		// Send player position to network
		clientNetwork.SendPlayerDataToServer();

		// Predict movement of others
		PredictMovement(database, fElapsedTime, localPlayer);

		// Receive actual positions from network (if availible)
		clientNetwork.RecvDatabaseUpdateFromServer();
	}

	void PerformServerLogic(Database& database, double fTime, float fElapsedTime)
	{
		// Predict movement
		PredictMovement(database, fElapsedTime);

		// Receive actual movement from network
		serverNetwork.AcceptNewConnection();
		serverNetwork.RecvPlayerDataFromClients();

		// Game logic
		vector<Entity*> toDelete;
		{ DbLoop_Meshes(database, it)

			Player* player = dynamic_cast<Player*>(it->second);
			if (player != NULL) {
				// Set mesh
				player->trySelectWeapon(player->selectedWeapon);

				// Fire player's weapon
				if (player->firing && fTime >= player->nextLoadedTime) {
					int* ammo = &player->inventory[Ammo_Revolver + player->selectedWeapon];
					float speed = 15.0;
					float reloadTime = 0.5;
					float range = 10;
					float directions[] = {0, -1, 1, -3, 3, -6, 6, -9, 9, -12, 12, -15, 15};
					for(int i = 0; i < sizeof(directions) / sizeof(float); i++) {
						if (*ammo > 0) {
							Bullet* bullet = new Bullet(player, player->selectedWeapon, player->position, range);
							bullet->rotY = player->rotY + 90 + directions[i];
							bullet->velocityRight = speed;
							database.add(bullet);
							(*ammo)--;
							player->nextLoadedTime = fTime + reloadTime;
						}
					}
				}
			}

			// Remove bullet
			Bullet* bullet = dynamic_cast<Bullet*>(entity);
			if (bullet != NULL) {
				D3DXVECTOR3 pathTraveled = bullet->position - bullet->origin;
				float distTraveled = D3DXVec3Length(&pathTraveled);

				D3DXVECTOR3 offset1(+0.3f, 0, 0.3f);
				D3DXVECTOR3 offset2(-0.3f, 0, 0.3f);
				bool onPath =
					IsPointOnPath(database, bullet->position) || 
					IsPointOnPath(database, bullet->position + offset1) ||
					IsPointOnPath(database, bullet->position - offset1) ||
					IsPointOnPath(database, bullet->position + offset2) ||
					IsPointOnPath(database, bullet->position - offset2);

				if (distTraveled > bullet->range || !onPath) {
					toDelete.push_back(bullet);
					continue;
				}
			}
		}

		// Perform the deletes
		for (vector<Entity*>::iterator it = toDelete.begin(); it != toDelete.end(); it++) {
			database.remove(*it);
		}

		// Send new positions and events to network
		serverNetwork.SendDatabaseUpdateToClients();
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

	void PredictMovement(Database& database, float fElapsedTime, Player* except = NULL)
	{
		{ DbLoop_Meshes(database, it)
			if (entity == except) continue;

			Player* player = dynamic_cast<Player*>(it->second);
			if (player != NULL) {
				// Move and rotate player
				MovePlayer(database, player, fElapsedTime);
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
	}

	bool MovePlayer(Database& database, Player* player, float fElapsedTime)
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
			if (IsPointOnPath(database, pos, &outY)) {
				pos.y = outY + PlayerRaiseAbovePath;
				player->position = pos;
				return true; // Done, moved
			}
		}
		return false; // Can not move
	}

	bool IsPointOnPath(Database& database, D3DXVECTOR3 pos, float* outY = NULL)
	{
		{ DbLoop_Meshes(database, it)
			// Position relative to the mesh
			D3DXVECTOR3 meshPos = entity->ToMeshCoordinates(pos);
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

GameLogic gameLogic;