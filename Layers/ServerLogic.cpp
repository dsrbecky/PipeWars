#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Math.h"

extern Database db;

extern bool IsPointOnPath(Database& db, D3DXVECTOR3 pos, float* outY = NULL);

class ServerLogic: Layer
{
	void FrameMove(double fTime, float fElapsedTime)
	{
		vector<Entity*> toDelete;

		DbLoop(it) {
			MeshEntity* entity = dynamic_cast<MeshEntity*>(it->second);
			if (entity == NULL)
				continue;

			Player* player = dynamic_cast<Player*>(it->second);
			if (player != NULL) {
				// Fire player's weapon
				if (player->firing && fTime >= player->nextLoadedTime) {
					int* ammo = &player->inventory[Ammo_Revolver + player->selectedWeapon];
					if (*ammo > 0) {
						float speed = 15.0;
						float reloadTime = 0.5;
						float range = 10;
						Bullet* bullet = new Bullet(player, player->selectedWeapon, player->position, range);
						bullet->rotY = player->rotY + 90;
						bullet->velocityRight = speed;
						db.add(bullet);
						(*ammo)--;
						player->nextLoadedTime = fTime + reloadTime;
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
					IsPointOnPath(db, bullet->position) || 
					IsPointOnPath(db, bullet->position + offset1) ||
					IsPointOnPath(db, bullet->position - offset1) ||
					IsPointOnPath(db, bullet->position + offset2) ||
					IsPointOnPath(db, bullet->position - offset2);

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
	}
};

ServerLogic serverLogic;