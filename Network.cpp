#include "StdAfx.h"
#include "Database.h"
#include <map>
#include <hash_map>
#include <hash_set>

using namespace stdext;

extern Database db;
extern Player* localPlayer;

// Very primitive compression
void WriteInt(vector<UCHAR>& out, UINT32 i)
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

UINT32 ReadInt(vector<UCHAR>::iterator& in)
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
void WriteEntity(vector<UCHAR>& out, Entity* entity, UCHAR* lastData, bool upadeLastData = true)
{
	// Byte encoding:
	//  0x00       End of entity
	//  0x01-0x7F  Unchanged data count
	//  0x80       Optional checksum (followed by UINT)
	//  0x81-0xFF  Changed data count (followed by the data)

	entity->OnSerializing(); // Pre-porcessing steps (strip memory pointers)

	int size = entity->GetSize();
	UCHAR* myData = (UCHAR*)entity;
	UCHAR* myEnd = myData + size;
	myData += 4; // Skip V-Table pointer

	while(true) {
		// Count changed data
		UCHAR* diffStart = myData;
		int numDiff = 0;
	countChangedData:
		while (myData < myEnd && *myData != *lastData && numDiff < 0x7F) {
			numDiff++;
			myData++;
			lastData++;
		}

		// Count unchanged data
		int numSame = 0;
		// Optimization - unrolled loop  - 8 at a time
		while (myData + 7 < myEnd && numSame + 7 < 0x7F) {
			if ((*(myData + 0) != *(lastData + 0)) || (*(myData + 1) != *(lastData + 1)) ||
				(*(myData + 2) != *(lastData + 2)) || (*(myData + 3) != *(lastData + 3)) ||
				(*(myData + 4) != *(lastData + 4)) || (*(myData + 5) != *(lastData + 5)) ||
				(*(myData + 6) != *(lastData + 6)) || (*(myData + 7) != *(lastData + 7))
			   ) break;
			numSame += 8;
			myData += 8;
			lastData += 8;
		}
		while (myData < myEnd && *myData == *lastData && numSame < 0x7F) {
			numSame++;
			myData++;
			lastData++;
		}

		// Optimization - it is not worth reporting just one or two unchanged bytes so report them as changed.
		if (numSame == 1 || numSame == 2 && (numDiff + numSame) <= 0x7F) {
			numDiff += numSame;
			numSame = 0;
			goto countChangedData; // Add more changed data
		}

		// Write the changed data
		if (numDiff > 0) {
			out.push_back(0x80 + numDiff);
			copy(diffStart, diffStart + numDiff, back_inserter(out));
			if (upadeLastData)
				copy(diffStart, diffStart + numDiff, lastData - numDiff - numSame);
		}

		// Write the unchanged data
		if (numSame > 0) {
			out.push_back(numSame);
		}

		// End of entity
		if (myData == myEnd) {
			out.push_back(0x00);
			break;
		}
	}
}

// Reads entity from stream, completly overwritting it
// It reads up to the terminating zero in the stream so type of entity does not matter
void ReadEntity(vector<UCHAR>::iterator& in, Entity* entity, UCHAR* lastData, bool upadeLastData = true)
{
	UCHAR* myData = (UCHAR*)entity;
	myData += 4; // Skip V-Table pointer

	while(true) {
		UCHAR descr = *(in++);
		if (descr == 0) {
			// End of entity
			if (entity->GetSize() != myData - (UCHAR*)entity)
				throw "The data did not have correct length for the entity";
			return;
		} else if (descr <= 0x7F) {
			// Unmodified
			int count = descr;
			copy(lastData, lastData + count, myData);
			myData += count;
			lastData += count;
		} else if (descr == 0x80) {
			// Reserved - error
			throw "Error in network stream: 0x80 seen";
		} else if (descr > 0x80) {
			// Modified
			int count = descr - 0x80;
			copy(in, in + count, myData);
			if (upadeLastData)
				copy(in, in + count, lastData);
			myData += count;
			lastData += count;
			in += count;
		}
	}
}

// Entity ID -> Entity Data
hash_map<ID, UCHAR*> lastDatas;

// The packet has format
//    <delete count> <id> <id> ...
//    <add count> <type><id><data> <type><id><data> ...
//    <modify count> <id><data> <id><data>
//    <sentinel 0xD5>
void network_SendIncrementalUpdateToClients()
{
	vector<UCHAR> out;

	// Sort all existing entities into groups
	hash_map<ID, UCHAR*> deleted(lastDatas);
	vector<Entity*> added;
	vector<Entity*> modified(db.size());
	DbLoop(it) {
		deleted.erase(it->first);
		if (lastDatas.count(it->first) == 0) {
			added.push_back(it->second);
		} else {
			modified.push_back(it->second);
		}
	}

	// Send deleted entities
	WriteInt(out, deleted.size());
	for(hash_map<ID, UCHAR*>::iterator it = deleted.begin(); it != deleted.end(); it++) {
		WriteInt(out, it->first);

		delete it->second;
		lastDatas.erase(it->first);
	}

	// Send added entities
	WriteInt(out, added.size());
	for(vector<Entity*>::iterator it = added.begin(); it != added.end(); it++) {
		Entity* e = *it;

		// Default is all zeros
		int size = e->GetSize();
		UCHAR* lastData = new UCHAR[size];
		ZeroMemory(&lastData, size);
		lastDatas[e->id] = lastData;

		// Send the entity data
		WriteInt(out, e->GetType());
		WriteInt(out, e->id);
		WriteEntity(out, e, lastData, true);
	}

	// Send modified entites
	WriteInt(out, modified.size());
	for(vector<Entity*>::iterator it = modified.begin(); it != modified.end(); it++) {
		Entity* e = *it;
		UCHAR* lastData = lastDatas[e->id];

		WriteInt(out, e->id);
		WriteEntity(out, e, lastData, true);
	}

	// Sentinel
	WriteInt(out, 0xD5);
}

void network_SendFreshUpdateToNewClient()
{
	vector<UCHAR> out;

	WriteInt(out, 0); // No deleted

	// All entities are send as added
	WriteInt(out, db.size());
	DbLoop(it) {
		Entity* e = it->second;

		// Default is all zeros
		int size = e->GetSize();
		UCHAR* lastData = new UCHAR[size];
		ZeroMemory(&lastData, size);

		// Send the entity data
		WriteInt(out, e->GetType());
		WriteInt(out, e->id);
		WriteEntity(out, e, lastData, false);

		delete lastData;
	}

	WriteInt(out, 0); // No modified

	// Sentinel
	WriteInt(out, 0xD5);
}

void netwoek_ReceiveUpdatesFromServer()
{
	vector<UCHAR>::iterator in;
	
	// Delete entities
	int deletedCount = ReadInt(in);
	for(int i = 0; i < deletedCount; i++) {
		UINT32 id = ReadInt(in);
		db.remove(id);
		delete lastDatas[id];
		lastDatas.erase(id);
	}

	// Add new entities
	int addedCount = ReadInt(in);
	for(int i = 0; i < addedCount; i++) {
		UCHAR type = (UCHAR)ReadInt(in);
		UINT32 id = ReadInt(in);
		Entity* newEntity = NULL;
		switch(type) {
			case MeshEntity::Type:   newEntity = new MeshEntity(); break;
			case Player::Type:       newEntity = new Player(); break;
			case Bullet::Type:       newEntity = new Bullet(); break;
			case PowerUp::Type:      newEntity = new PowerUp(); break;
			case RespawnPoint::Type: newEntity = new RespawnPoint(); break;
			default: throw "Received unknown entity type";
		}
		UCHAR* lastData = new UCHAR[newEntity->GetSize()];
		ZeroMemory(&lastData, newEntity->GetSize());
		lastDatas[id] = lastData;
		ReadEntity(in, newEntity, lastData, true); 
	}

	// Modify entities
	int modifiedCount = ReadInt(in);
	for(int i = 0; i < modifiedCount; i++) {
		UINT32 id = ReadInt(in);
		Entity* entity = db[id];
		UCHAR* lastData = lastDatas[id];
		ReadEntity(in, entity, lastData, true); 
	}

	UINT32 sentinel = ReadInt(in);
	if (sentinel != 0xD5)
		throw "Sentinel not found";
}

// TODO: Actual socket communication
// TODO: Debug check sum

// A small selection of data that the client is allowed to change
// It is small enough so that it can be just copied to this helpper class
struct ClientUpdate
{
	static const int sentinelValue = 0xD5D5D5D5;
	int sentinel;
	int clientUpdateSize;
	int playerID;
	D3DXVECTOR3 position;
	float velocityForward;
	float velocityRight;
	float rotY;
	float rotY_velocity;
	ItemType selectedWeapon;
	bool firing;
};

void network_SendUpdateToServer()
{
	ClientUpdate update;
	update.sentinel = ClientUpdate::sentinelValue;
	update.clientUpdateSize = sizeof(update);
	update.playerID = localPlayer->id;
	update.position = localPlayer->position;
	update.velocityForward = localPlayer->velocityForward;
	update.velocityRight = localPlayer->velocityRight;
	update.rotY = localPlayer->rotY;
	update.rotY_velocity = localPlayer->rotY_velocity;
	update.selectedWeapon = localPlayer->selectedWeapon;
	update.firing = localPlayer->firing;
}