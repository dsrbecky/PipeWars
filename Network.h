#include "StdAfx.h"

// A small selection of data that the client is allowed to change
// It is small enough so that it can be just copied to this helpper class
struct ClientUpdate
{
	static const UCHAR SentinelValue = 0xD5;
	int playerID;
	D3DXVECTOR3 position;
	float velocityForward;
	float velocityRight;
	float rotY;
	float rotY_velocity;
	ItemType selectedWeapon;
	bool firing;
	UCHAR sentinel;
};

class Network
{
	hash_map<ID, UCHAR*> lastSendDatas;
	hash_map<ID, UCHAR*> lastRecvDatas;

public:

	void SendDatabaseUpdate(vector<UCHAR>& out, Database& db);
	void SendFullDatabase(vector<UCHAR>& out, Database& db);
	void RecvDatabase(vector<UCHAR>::iterator& in, Database& db);
	ClientUpdate SendPlayerData(Player* player);
	void RecvPlayerData(Database& db, ClientUpdate& clientUpdate);
};