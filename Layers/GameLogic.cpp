#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

extern Database db;
extern Player* localPlayer;

class GameLogic: Layer
{
	void FrameMove(double fTime, float fElapsedTime)
	{
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
};

GameLogic gameLogic;