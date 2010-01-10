#include "StdAfx.h"
#include "../Entities.h"
#include "Network.h"

extern Player* localPlayer;

// A small selection of data that the client is allowed to change
// It is small enough so that it can be just copied to this helpper class manually
struct ClientUpdate
{
	static const UCHAR SentinelValue = 0xD5;

	D3DXVECTOR3 position;
	float velocityForward;
	float velocityRight;
	float rotY;
	float rotY_velocity;
	ItemType selectedWeapon;
	bool firing;
	UCHAR sentinel;

	ClientUpdate()
	{
		ZeroMemory(this, sizeof(*this));
	}
};

void Network::SendPlayerDataTo(vector<UCHAR>& out, Player* player)
{
	ClientUpdate update;
	update.position = player->position;
	update.velocityForward = player->velocityForward;
	update.velocityRight = player->velocityRight;
	update.rotY = player->rotY;
	update.rotY_velocity = player->rotY_velocity;
	update.selectedWeapon = player->selectedWeapon;
	update.firing = player->firing;
	update.sentinel = ClientUpdate::SentinelValue;

	copy((UCHAR*)&update, (UCHAR*)(&update + 1), back_inserter(out));
}

void Network::RecvPlayerDataFrom(vector<UCHAR>::iterator& in, Player* player)
{
	ClientUpdate update;

	copy(in, in + sizeof(ClientUpdate), (UCHAR*)&update);
	in += sizeof(ClientUpdate);

	if (update.sentinel != ClientUpdate::SentinelValue)
		throw "Sentinel corrupted";
	
	player->position = update.position;
	player->velocityForward = update.velocityForward;
	player->velocityRight = update.velocityRight;
	player->rotY = update.rotY;
	player->rotY_velocity = update.rotY_velocity;
	player->selectedWeapon = update.selectedWeapon;
	player->firing = update.firing;
}

void Network::SendPlayerDataToServer()
{
	// There will be just one
	{ ConnLoop
		Player* player = localPlayer;
		if (localPlayer == NULL) continue;
		if (!conn->nameTransmited) {
			copy(player->name, player->name + MAX_STR_LEN, back_inserter(conn->outBuffer));
			conn->nameTransmited = true;
		}
		SendPlayerDataTo(conn->outBuffer, conn->player);
	}
}

void Network::RecvPlayerDataFromClient()
{
	{ ConnLoop
		while(true) {
			if (conn->nameTransmited) {
				if (conn->inBuffer.size() > sizeof(ClientUpdate)) {
					RecvPlayerDataFrom(conn->inBuffer.begin(), conn->player);
					Skip(conn->inBuffer, sizeof(ClientUpdate));
					continue;
				}
			} else {
				if (conn->inBuffer.size() > MAX_STR_LEN) {
					copy(conn->inBuffer.begin(), conn->inBuffer.begin() + MAX_STR_LEN, conn->player->name);
					Skip(conn->inBuffer, MAX_STR_LEN);
					conn->nameTransmited = true;
					continue;
				}
			}
			break;
		}
	}
}