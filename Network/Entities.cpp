#include "StdAfx.h"
#include "../Entities.h"
#include "Network.h"

extern Player* localPlayer;

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

// Reads entity from stream
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
//    <modify count> <type><id><data> <type><id><data>
//    <sentinel 0xD5>
//
void Network::SendDatabaseUpdate(vector<UCHAR>& out)
{
	// Sort all existing entities into groups
	hash_map<ID, pair<UCHAR, UCHAR*>> deleted(lastSendDatas);
	vector<Entity*> added;
	vector<Entity*> modified;
	DbLoop(this->database, it) {
		int id = it->first;
		Entity* entity = it->second;

		deleted.erase(id);
		if (lastSendDatas.count(id) == 0) {
			added.push_back(entity);
		} else {
			// Was the entity at the given ID replaced by completly other type?
			if (entity->GetType() != lastSendDatas[id].first) {
				deleted[id] = lastSendDatas[id];
				added.push_back(entity);
				continue;
			}

			//MeshEntity* meshEntity = dynamic_cast<MeshEntity*>(e);
			//if (meshEntity != NULL && meshEntity->meshFilename == pipeFilename)
			//	continue; // Optimization - Pipes do not change

			// Was it modified?
			UCHAR* lastSendData = lastSendDatas[id].second;
			entity->OnSerializing(); // Pre-porcessing steps (strip memory pointers)
			int size = entity->GetSize();
			if (memcmp((UCHAR*)entity + sizeof(void*), lastSendData + sizeof(void*), size - sizeof(void*)) == 0)
				continue; // Not modified at all

			modified.push_back(entity);
		}
	}

	if (deleted.size() == 0 && added.size() == 0 && modified.size() == 0)
		return; // No updates

	// Send deleted entities
	SendInt(out, deleted.size());
	for(hash_map<ID, pair<UCHAR, UCHAR*>>::iterator it = deleted.begin(); it != deleted.end(); it++) {
		SendInt(out, it->first);

		delete it->second.second;
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
		lastSendDatas[e->id] = pair<UCHAR, UCHAR*>(e->GetType(), lastSendData);

		// Send the entity data
		SendInt(out, e->GetType());
		SendInt(out, e->id);
		SendEntity(out, e, lastSendData);
	}

	// Send modified entites
	SendInt(out, modified.size());
	for(vector<Entity*>::iterator it = modified.begin(); it != modified.end(); it++) {
		Entity* e = *it;
		UCHAR* lastSendData = lastSendDatas[e->id].second;
		SendInt(out, e->GetType());
		SendInt(out, e->id);
		SendEntity(out, e, lastSendData);
	}

	// Sentinel
	SendInt(out, 0xD5);
}

void Network::SendFullDatabase(vector<UCHAR>& out)
{
	SendInt(out, 0xFFFFFFFF); // Clear

	// All entities are send as added
	SendInt(out, this->database.size());
	DbLoop(this->database, it) {
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

void Network::RecvDatabaseUpdate(vector<UCHAR>::iterator& in)
{	
	// Delete entities
	int deletedCount = RecvInt(in);
	if (deletedCount == 0xFFFFFFFF) {
		this->database.clear();
	} else {
		for(int i = 0; i < deletedCount; i++) {
			UINT32 id = RecvInt(in);
			this->database.remove(id);
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
		this->database.add(newEntity);
	}

	// Modify entities
	int modifiedCount = RecvInt(in);
	for(int i = 0; i < modifiedCount; i++) {
		UCHAR type = (UCHAR)RecvInt(in);
		UINT32 id = RecvInt(in);
		Entity* entity = this->database[id];
		if (type != entity->GetType())
			throw "Received type did not match the one already in database";
		UCHAR* lastRecvData = lastRecvDatas[id];
		RecvEntity(in, entity, lastRecvData); 
	}

	UINT32 sentinel = RecvInt(in);
	if (sentinel != 0xD5)
		throw "Sentinel corrupted";
}

// Format <int32-updateSize><int32-yourID><...update...>
void Network::SendDatabaseUpdateToClients()
{
	assert(this->serverRunning);

	vector<UCHAR> updateData;
	SendDatabaseUpdate(updateData);
	int updateSize = updateData.size();
	{ ConnLoop
		copy((CHAR*)&updateSize, ((CHAR*)&updateSize) + 4, back_inserter(conn->outBuffer));
		ID yourId = conn->player->id;
		copy((CHAR*)&yourId, ((CHAR*)&yourId) + 4, back_inserter(conn->outBuffer));
		copy(updateData.begin(), updateData.end(), back_inserter(conn->outBuffer));
	}

	SendSocketData();
}

void Network::SendFullDatabaseToClient(Connetion* conn)
{
	assert(this->serverRunning);

	vector<UCHAR> updateData;
	SendFullDatabase(updateData);
	int updateSize = updateData.size();

	copy((CHAR*)&updateSize, ((CHAR*)&updateSize) + 4, back_inserter(conn->outBuffer));
	ID yourId = 0; // Do not send ID (player not added to database yet)
	copy((CHAR*)&yourId, ((CHAR*)&yourId) + 4, back_inserter(conn->outBuffer));
	copy(updateData.begin(), updateData.end(), back_inserter(conn->outBuffer));

	SendSocketData();
}

void Network::RecvDatabaseUpdateFromServer()
{
	assert(this->clientRunning);

	RecvSocketData();

	// We want to keep the local player position
	vector<UCHAR> localPlayerData;
	if (localPlayer != NULL) // When we join we do not know who we are yet
		SendPlayerDataTo(localPlayerData, localPlayer);

	assert(connections.size() == 1);

	{ ConnLoop
		while(true) {
			if (conn->inBuffer.size() < 8) break;
			int updateSize;
			ID myId;
			copy(conn->inBuffer.begin() + 0, conn->inBuffer.begin() + 4, (UCHAR*)&updateSize);
			copy(conn->inBuffer.begin() + 4, conn->inBuffer.begin() + 8, (UCHAR*)&myId);
			if ((int)conn->inBuffer.size() < updateSize + 8) break;
			if (updateSize != 0)
				RecvDatabaseUpdate(conn->inBuffer.begin() + 8);
			Skip(conn->inBuffer, updateSize + 8);

			if (myId != 0)
				localPlayer = dynamic_cast<Player*>(this->database[myId]);
		}
	}

	// Restore local player data
	if (localPlayerData.size() > 0)
		RecvPlayerDataFrom(localPlayerData.begin(), localPlayer);
}