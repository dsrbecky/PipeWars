#include "StdAfx.h"
#include "Entities.h"
#include "Network.h"

const string pipeFilename = "pipe.dae";

// Very primitive compression
inline void SendInt(vector<UCHAR>& out, UINT32 i)
{
	if (i < 0xFF) {
		out.push_back((UCHAR)i);
	} else {
		out.push_back(0xFF);
		out.push_back((i >> 24) & 0xFF);
		out.push_back((i >> 16) & 0xFF);
		out.push_back((i >>  8) & 0xFF);
		out.push_back((i >>  0) & 0xFF);
	}
}

inline UINT32 RecvInt(vector<UCHAR>::iterator& in)
{
	if (*in < 0xFF) {
		return *(in++);
	} else {
		in++;
		int value = 
			(*(in + 0) << 24) + 
			(*(in + 1) << 16) + 
			(*(in + 2) <<  8) + 
			(*(in + 3) <<  0);
		in += 4;
		return value;
	}
}

// Writes the content of the entity to the stream
// The data is encoded so that only modifications are send
void SendEntity(vector<UCHAR>& out, Entity* entity, UCHAR* lastSendData)
{
	// Byte encoding:
	//  0x00       Reserved
	//  0x01-0x7F  Unchanged data count
	//  0x80       Reserved
	//  0x81-0xFF  Changed data count (followed by the data)

	entity->OnSerializing(); // Pre-porcessing steps (strip memory pointers)

	int size = entity->GetSize();
	UCHAR* myData = (UCHAR*)entity;
	UCHAR* myEnd = myData + size;
	myData += sizeof(void*); // Skip V-Table pointer
	lastSendData += sizeof(void*);

	while(true) {
		// Count changed data
		UCHAR* diffStart = myData;
		int numDiff = 0;
	countChangedData:
		while (myData < myEnd && *myData != *lastSendData && numDiff < 0x7F) {
			numDiff++;
			myData++;
			lastSendData++;
		}

		// Count unchanged data
		int numSame = 0;
		// Optimization - 8 at a time
		while (myData + 7 < myEnd && memcmp(myData, lastSendData, 8) == 0 && numSame + 7 < 0x7F) {
			numSame += 8;
			myData += 8;
			lastSendData += 8;
		}
		while (myData < myEnd && *myData == *lastSendData && numSame < 0x7F) {
			numSame++;
			myData++;
			lastSendData++;
		}

		// Optimization - it is not worth reporting just one or two unchanged bytes so report them as changed.
		if ((numSame == 1 || numSame == 2 || numSame == 3) && (numDiff + numSame) <= 0x7F) {
			numDiff += numSame;
			numSame = 0;
			goto countChangedData; // Add more changed data
		}

		// Write the changed data
		if (numDiff > 0) {
			out.push_back(0x80 + numDiff);
			copy(diffStart, diffStart + numDiff, back_inserter(out));
			copy(diffStart, diffStart + numDiff, lastSendData - numDiff - numSame);
		}

		// Write the unchanged data
		if (numSame > 0) {
			out.push_back(numSame);
		}

		// End of entity
		if (myData == myEnd) {
			break;
		}
	}
}

// Reads entity from stream, completly overwritting it
void RecvEntity(vector<UCHAR>::iterator& in, Entity* entity, UCHAR* lastRecvData)
{
	int size = entity->GetSize();
	UCHAR* myData = (UCHAR*)entity;
	UCHAR* myEnd = myData + size;
	myData += sizeof(void*); // Skip V-Table pointer
	lastRecvData += sizeof(void*);

	while(true) {
		if (myData == myEnd) break; // End of data
		if (myData > myEnd)
			throw "Error - writen over end of entity";
		UCHAR descr = *(in++);
		if (descr == 0) {
			throw "Error in network stream: 0x00 seen";
		} else if (descr <= 0x7F) {
			// Unmodified
			int count = descr;
			copy(lastRecvData, lastRecvData + count, myData);
			myData += count;
			lastRecvData += count;
		} else if (descr == 0x80) {
			throw "Error in network stream: 0x80 seen";
		} else if (descr > 0x80) {
			// Modified
			int count = descr - 0x80;
			copy(in, in + count, myData);
			copy(in, in + count, lastRecvData);
			myData += count;
			lastRecvData += count;
			in += count;
		}
	}
}

// The packet has format
//
//    <delete count (-1 = clear)> <id> <id> ...
//    <add count> <type><id><data> <type><id><data> ...
//    <modify count> <id><data> <id><data>
//    <sentinel 0xD5>
//
void Network::SendDatabaseUpdate(vector<UCHAR>& out, Database& db)
{
	// Sort all existing entities into groups
	hash_map<ID, UCHAR*> deleted(lastSendDatas);
	vector<Entity*> added;
	vector<Entity*> modified;
	DbLoop(it) {
		deleted.erase(it->first);
		if (lastSendDatas.count(it->first) == 0) {
			added.push_back(it->second);
		} else {
			MeshEntity* meshEntity = dynamic_cast<MeshEntity*>(it->second);
			if (meshEntity != NULL && meshEntity->meshFilename == pipeFilename)
				continue; // Optimization - Pipes do not change

			// Was it modified?
			Entity* e = it->second;
			UCHAR* lastSendData = lastSendDatas[it->first];
			e->OnSerializing(); // Pre-porcessing steps (strip memory pointers)
			int size = e->GetSize();
			if (memcmp((UCHAR*)e + sizeof(void*), lastSendData + sizeof(void*), size - sizeof(void*)) == 0)
				continue; // Not modified at all

			modified.push_back(it->second);
		}
	}

	// Send deleted entities
	SendInt(out, deleted.size());
	for(hash_map<ID, UCHAR*>::iterator it = deleted.begin(); it != deleted.end(); it++) {
		SendInt(out, it->first);

		delete it->second;
		lastSendDatas.erase(it->first);
	}

	// Send added entities
	SendInt(out, added.size());
	for(vector<Entity*>::iterator it = added.begin(); it != added.end(); it++) {
		Entity* e = *it;

		// Default is all zeros
		int size = e->GetSize();
		UCHAR* lastSendData = new UCHAR[size];
		ZeroMemory(lastSendData, size);
		lastSendDatas[e->id] = lastSendData;

		// Send the entity data
		SendInt(out, e->GetType());
		SendInt(out, e->id);
		SendEntity(out, e, lastSendData);
	}

	// Send modified entites
	SendInt(out, modified.size());
	for(vector<Entity*>::iterator it = modified.begin(); it != modified.end(); it++) {
		Entity* e = *it;
		UCHAR* lastSendData = lastSendDatas[e->id];
		SendInt(out, e->id);
		SendEntity(out, e, lastSendData);
	}

	// Sentinel
	SendInt(out, 0xD5);
}

void Network::SendFullDatabase(vector<UCHAR>& out, Database& db)
{
	SendInt(out, 0xFFFFFFFF); // Clear

	// All entities are send as added
	SendInt(out, db.size());
	DbLoop(it) {
		Entity* e = it->second;

		// Default is all zeros
		int size = e->GetSize();
		UCHAR* lastSendData = new UCHAR[size];
		ZeroMemory(lastSendData, size);

		// Send the entity data
		SendInt(out, e->GetType());
		SendInt(out, e->id);
		SendEntity(out, e, lastSendData);

		delete lastSendData;
	}

	SendInt(out, 0); // No modified

	// Sentinel
	SendInt(out, 0xD5);
}

void Network::RecvDatabase(vector<UCHAR>::iterator& in, Database& db)
{	
	// Delete entities
	int deletedCount = RecvInt(in);
	if (deletedCount == 0xFFFFFFFF) {
		db.clear();
	} else {
		for(int i = 0; i < deletedCount; i++) {
			UINT32 id = RecvInt(in);
			db.remove(id);
			delete lastRecvDatas[id];
			lastRecvDatas.erase(id);
		}
	}

	// Add new entities
	int addedCount = RecvInt(in);
	for(int i = 0; i < addedCount; i++) {
		UCHAR type = (UCHAR)RecvInt(in);
		UINT32 id = RecvInt(in);
		Entity* newEntity = NULL;
		switch(type) {
			case MeshEntity::Type:   newEntity = new MeshEntity(); break;
			case Player::Type:       newEntity = new Player(); break;
			case Bullet::Type:       newEntity = new Bullet(); break;
			case PowerUp::Type:      newEntity = new PowerUp(); break;
			case RespawnPoint::Type: newEntity = new RespawnPoint(); break;
			default: throw "Received unknown entity type";
		}
		UCHAR* lastRecvData = new UCHAR[newEntity->GetSize()];
		ZeroMemory(lastRecvData, newEntity->GetSize());
		lastRecvDatas[id] = lastRecvData;
		RecvEntity(in, newEntity, lastRecvData);
		db.add(newEntity);
	}

	// Modify entities
	int modifiedCount = RecvInt(in);
	for(int i = 0; i < modifiedCount; i++) {
		UINT32 id = RecvInt(in);
		Entity* entity = db[id];
		UCHAR* lastRecvData = lastRecvDatas[id];
		RecvEntity(in, entity, lastRecvData); 
	}

	UINT32 sentinel = RecvInt(in);
	if (sentinel != 0xD5)
		throw "Sentinel corrupted";
}

void Network::SendPlayerData(vector<UCHAR>& out, Player* player)
{
	ClientUpdate update;
	update.playerID = player->id;
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

void Network::RecvPlayerData(vector<UCHAR>::iterator& in, Database& db)
{
	ClientUpdate update;

	copy(in, in + sizeof(ClientUpdate), (UCHAR*)&update);
	in += sizeof(ClientUpdate);

	if (update.sentinel != ClientUpdate::SentinelValue)
		throw "Sentinel corrupted";
	Player* player = dynamic_cast<Player*>(db[update.playerID]);
	if (player == NULL)
		throw "Player is not in dababase";
	player->position = update.position;
	player->velocityForward = update.velocityForward;
	player->velocityRight = update.velocityRight;
	player->rotY = update.rotY;
	player->rotY_velocity = update.rotY_velocity;
	player->selectedWeapon = update.selectedWeapon;
	player->firing = update.firing;
}