#include "StdAfx.h"
#include "Database.h"
#include <map>
#include <hash_map>
#include <hash_set>

using namespace stdext;

extern Database db;
extern Player* localPlayer;

// A small selection of data that the client is allowed to change
// It is small enough so that it can be just copied to this helpper class
struct ClientUpdate
{
	static const int sentinelValue = 0x00DA1D00;
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

	int size = entity->GetSize();
	UCHAR* myData = (UCHAR*)entity;
	UCHAR* myEnd = myData + size;
	myData += 4; // Skip V-Table pointer

	while(true) {
		// Process unchanged data
		int numSame = 0;
		while (*myData == *lastData && numSame < 0x7F && myData < myEnd) {
			numSame++;
			myData++;
			lastData++;
		}
		if (numSame > 0) {
			out.push_back(numSame);
		}

		// Proces changed data
		int numDiff = 0;
		while (*myData != *lastData && numDiff < 0x7F && myData < myEnd) {
			numDiff++;
			myData++;
			lastData++;
		}
		if (numDiff > 0) {
			out.push_back(0x80 + numDiff);
			copy(myData - numDiff, myData, back_inserter(out));
			if (upadeLastData)
				copy(myData - numDiff, myData, lastData - numDiff);
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
hash_map<UINT32, UCHAR*> lastDatas;

// The packet has format
//    <delete count> <id> <id> ...
//    <add count> <type><id><data> <type><id><data> ...
//    <modify count> <id><data> <id><data>
//    <sentinel 0xD5>
void network_SendIncrementalUpdateToClients()
{
	vector<UCHAR> out;

	// Sort all existing entities into groups
	hash_map<UINT32, UCHAR*> deleted(lastDatas);
	vector<Entity*> added;
	vector<Entity*> modified(db.entities.size());
	DbLoop(it) {
		deleted.erase((*it)->id);
		if (lastDatas.count((*it)->id) == 0) {
			added.push_back(*it);
		} else {
			modified.push_back(*it);
		}
	}

	// Send deleted entities
	WriteInt(out, deleted.size());
	for(hash_map<UINT32, UCHAR*>::iterator it = deleted.begin(); it != deleted.end(); it++) {
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

void network_SendFreshUpdateToClient()
{
	vector<UCHAR> out;

	WriteInt(out, 0); // No deleted

	// All entities are send as added
	WriteInt(out, db.entities.size());
	DbLoop(it) {
		Entity* e = *it;

		// Default is all zeros
		int size = e->GetSize();
		UCHAR* lastData = new UCHAR[size];
		ZeroMemory(&lastData, size);

		// Send the entity data
		WriteInt(out, e->GetType());
		WriteInt(out, e->id);
		WriteEntity(out, e, lastData, true);

		delete lastData;
	}

	WriteInt(out, 0); // No modified

	// Sentinel
	WriteInt(out, 0xD5);
}

void netwoek_ReceiveUpdatesFromServer()
{
	vector<UCHAR>::iterator& in;
	
	// Delete entities
	int deletedCount = ReadInt(in);
	for(int i = 0; i < deletedCount; i++) {
		UINT32 id = ReadInt(in);
		db.entities.remove(id);
		delete lastDatas[id];
		lastDatas.erase(id);
	}

	// Add new entities
	int addedCount = ReadInt(in);
	for(int i = 0; i < addedCount; i++) {
		UINT32 id = ReadInt(in);
		UCHAR type = (UCHAR)ReadInt(in);
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
		Entity* entity = db.entities[id];
		UCHAR* lastData = lastDatas[id];
		ReadEntity(in, entity, lastData, true); 
	}

	UINT32 sentinel = ReadInt(in);
	if (sentinel != 0xD5)
		throw "Sentinel not found";
}

// TODO: Ensure that Database does not have any pointers/strings
// TODO: Actual socket communication
// TODO: Debug check sum