#include "../StdAfx.h"

const string pipeFilename = "pipe.dae";
static const char* Port = "27182";
const int BuffLen = 2048;

#define ConnLoop } for(set<Connetion*>::iterator connIt = connections.begin(); connIt != connections.end(); connIt++) { Connetion*& conn = *connIt; 

struct Connetion
{
	SOCKET socket;
	Player* player;
	bool nameTransmited;
	vector<UCHAR> inBuffer;
	vector<UCHAR> outBuffer;
};

class Network
{
	hash_map<ID, UCHAR*> lastSendDatas;
	hash_map<ID, UCHAR*> lastRecvDatas;

	Database& db;
	set<Connetion*> connections;
	SOCKET listenSocket;

public:

	Network(Database& _db): listenSocket(0), db(_db) {}

	// Socks
	void StartListening();
	void AcceptNewConnections();
	void Joint(string ip);
	void RecvSocketData();
	void SendSocketData();
	void Shutdown();

	// Entities
	void SendDatabaseUpdate(vector<UCHAR>& out);
	void SendFullDatabase(vector<UCHAR>& out);
	void RecvDatabase(vector<UCHAR>::iterator& in);
	void SendDatabaseUpdateToClients();
	void RecvDatabaseFromServer();

	// Player
	void SendPlayerDataTo(vector<UCHAR>& out, Player* player);
	void RecvPlayerDataFrom(vector<UCHAR>::iterator& in, Player* player);
	void SendPlayerDataToServer();
	void RecvPlayerDataFromClient();

private:

	void Skip(vector<UCHAR> buffer, int count)
	{
		vector<UCHAR> newBuffer;
		copy(buffer.begin() + count, buffer.end(), back_inserter(newBuffer));
		buffer.swap(newBuffer);
	}
};