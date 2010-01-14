#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"
#include "../Maths.h"
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
			if (keyDown['E'])                      localPlayer->velocityForward = 1.5f * PlayerMoveSpeed;
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
		// Accept new player
		Player* newPlayer = serverNetwork.AcceptNewConnection();
		if (newPlayer != NULL)
			Respawn(database, newPlayer);

		// Predict movement
		PredictMovement(database, fElapsedTime);

		// Receive actual movement from network
		serverNetwork.RecvPlayerDataFromClients();

		// Reload map - for map editing
		//database.clearNonPlayers();
		//resources.LoadTestMap(&database);

		// Game logic
		{ DbLoop_Players(database, it)
			player->position_ServerChanged = false;
			player->selectedWeapon_ServerChanged = false;
		}

		vector<Entity*> toDelete;
		{ DbLoop_Meshes(database, it)

			Player* player = dynamic_cast<Player*>(it->second);
			if (player != NULL) {
				// Set mesh
				player->trySelectWeapon(player->selectedWeapon);

				// Fire player's weapon
				if (player->firing && fTime >= player->nextLoadedTime) {
					FireWeapon(database, player, fTime);
				}
			}

			// Remove bullet
			Bullet* bullet = dynamic_cast<Bullet*>(entity);
			if (bullet != NULL) {
				{ DbLoop_Players(database, it)
					if (player->id == bullet->shooter)
						continue; // Do not shoot yourself

					D3DXVECTOR3 distVec = bullet->position - player->position;
					distVec.y /= 4;
					float dist = D3DXVec3Length(&distVec);
					if (dist <= 0.75) {
						// The player might have disconnected
						Player* shooter = database.players.count(bullet->shooter) != 0 ? database.players[bullet->shooter] : NULL;
						HitPlayer(database, player, shooter);
						
						toDelete.push_back(bullet);
						goto bullets_end;
					}
				}

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
			bullets_end:

			PowerUp* powerUp = dynamic_cast<PowerUp*>(entity);
			if (powerUp != NULL) {
				// Skull is speical
				if (powerUp->itemType == Skull) {
					if (powerUp->rechargeAt < fTime)
						toDelete.push_back(powerUp);

				} else {
					if (powerUp->rechargeAt < fTime)
						powerUp->present = true;
					{ DbLoop_Players(database, it)
						D3DXVECTOR3 distVec = powerUp->position - player->position;
						float dist = D3DXVec3Length(&distVec);
						if (dist <= 0.75 && powerUp->present) {
							UsePowerUp(player, powerUp);
							powerUp->present = false;
							powerUp->rechargeAt = (float)fTime + 60;
						}
					}
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
	
	void FireWeapon(Database& database, Player* player, double fTime)
	{
		ItemType weapon = player->selectedWeapon;
		int* ammo = &player->inventory[Ammo_Revolver + weapon];
		float speed = 15.0;
		float reloadTime = 0.5;
		float range = 10;
		float directions[] = {0, -1, 1, -3, 3, -6, 6, -9, 9};
		float directionsCount = 1;
		string geomName = "RevolverBullet";
		
		switch(weapon) {
			case Weapon_Revolver:
				if (player->inventory[Weapon_Revolver] == 2) reloadTime *= 0.5f;
				geomName = "RevolverBullet";
				break;
			case Weapon_Shotgun:
				directionsCount = 9;
				geomName = "ShotgunPellet";
				break;
			case Weapon_AK47:
				reloadTime *= 0.25f;
				geomName = "AKBullet";
				break;
			case Weapon_Jackhammer:
				reloadTime *= 0.25f;
				geomName = "Nail";
				break;
			case Weapon_Nailgun:
				reloadTime *= 0.75f; directionsCount = 5;
				geomName = "Nail";
				break;
		}

		for(int i = 0; i < directionsCount; i++) {
			if (*ammo > 0) {
				Bullet* bullet = new Bullet(player, geomName, player->position, range);
				bullet->rotY = player->rotY + 90 + directions[i];
				bullet->velocityRight = speed;
				if (geomName == "Nail")
					bullet->scale = 2.0f;
				database.add(bullet);
				(*ammo)--;
				player->nextLoadedTime = fTime + reloadTime;
				if (*ammo == 0) {
					for (int i = Weapon_Nailgun; i >= Weapon_Revolver; i--) {
						if (player->inventory[i] > 0 && player->inventory[Ammo_Revolver + i] > 0) {
							player->trySelectWeapon((ItemType)i);
							player->selectedWeapon_ServerChanged = true;
							break;
						}
					}
				}
			}
		}
	}

	void UsePowerUp(Player* player, PowerUp* powerUp)
	{
		int* inv = player->inventory;
		ItemType item = powerUp->itemType;

		// Weapon
		if (Weapon_Revolver <= item && item <= Weapon_Nailgun) {
			if (inv[item] == 0 && (player->selectedWeapon < item || inv[Ammo_Revolver + player->selectedWeapon] == 0)) {
				player->selectedWeapon = item;
				player->selectedWeapon_ServerChanged = true;
			}
			inv[item] = item == Weapon_Revolver ? 2 : 1;
			inv[Ammo_Revolver + item] += 50;
		}
		
		// Ammo
		if (Ammo_Revolver <= item && item <= Ammo_Nailgun) {
			if (inv[item] == 0 && (player->selectedWeapon < (item - Ammo_Revolver) || inv[Ammo_Revolver + player->selectedWeapon] == 0) && inv[item - Ammo_Revolver] > 0) {
				player->selectedWeapon = (ItemType)(item - Ammo_Revolver);
				player->selectedWeapon_ServerChanged = true;
			}
			inv[item] += 100;
		}

		// Other powerups
		switch(powerUp->itemType) {
			case HealthPack: if (player->health < 100) player->health = 100; break;
			case ArmourPack: if (player->armour < 100) player->armour = 100; break;
			case Shiny: player->health += 25; break;
			case Monkey:
				if (player->health < 100) player->health = 100;
				if (player->armour < 100) player->armour = 100;
				for(int i = 0; i < 5; i++) {
					inv[Weapon_Revolver + i] = 1;
					inv[Ammo_Revolver + i] = max(inv[Ammo_Revolver + i], 50);
				}
				inv[Weapon_Revolver] = 2;
				break;
			case Skull: break;
		}
	}

	void HitPlayer(Database& database, Player* player, Player* shooter)
	{
		int damage = 5 + rand() % 6;

		if (player->armour > 0) {
			player->armour = max(0, player->armour - damage);
		} else {
			player->health = max(0, player->health - damage);
		}

		if (player->health == 0) {
			player->deaths++;
			player->score--;
			shooter->kills++;
			shooter->score++;

			PowerUp* skull = new PowerUp(Skull, "skull.dae", "Skull");
			skull->position = player->position;
			skull->rechargeAt = (float)DXUTGetTime() + 60;
			database.add(skull);

			Respawn(database, player);
		}
	}

	void Respawn(Database& database, Player* player)
	{
		static int index = rand();

		vector<D3DXVECTOR3> respawnPoints;
		DbLoop(database, it) {
			// Obtain list of all valid respawn points
			RespawnPoint* respawnPoint = dynamic_cast<RespawnPoint*>(it->second);
			if (respawnPoint != NULL) {
				float outY;
				if (IsPointOnPath(database, respawnPoint->position, &outY)) {
					respawnPoint->position.y = outY + PlayerRaiseAbovePath; // Exactly allign
					respawnPoints.push_back(respawnPoint->position);
				}
			}
		}

		assert(respawnPoints.size() > 0);
			
		// Respawn the player
		player->Reset();
		player->position = respawnPoints[(index++) % respawnPoints.size()];
		player->position_ServerChanged = true;
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
		localPlayer->rotY_velocity = floor(0.5f * localPlayer->rotY_velocity + 0.5f * velocity);
		if (abs(localPlayer->rotY_velocity) < 10)
			localPlayer->rotY_velocity = 0;
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