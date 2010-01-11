#include "../StdAfx.h"

const string pipeFilename = "pipe.dae";
static const char* Port = "27182";
const int BuffLen = 2048;

#define ConnLoop } for(set<Connetion*>::iterator connIt = connections.begin(); connIt != connections.end(); connIt++) { Connetion* conn = *connIt; 

struct Connetion
{
	SOCKET socket;
	Player* player;
	bool nameTransmited;
	vector<UCHAR> inBuffer;
	vector<UCHAR> outBuffer;

	Connetion(): socket(INVALID_SOCKET), player(NULL), nameTransmited(false) {}
};

class Network
{
	hash_map<ID, UCHAR*> lastSendDatas;
	hash_map<ID, UCHAR*> lastRecvDatas;

	Database& database;
	set<Connetion*> connections;
	SOCKET listenSocket;

public:

	bool serverRunning;
	bool clientRunning;

	Network(Database& _db):
		listenSocket(INVALID_SOCKET), database(_db),
		serverRunning(false), clientRunning(false)
	{
	}

	~Network();

	// Socks
	void StartListening();
	void AcceptNewConnection();
	bool Joint(string ip, string playerName, bool nonBlocking);
	void RecvSocketData();
	void SendSocketData();
	void CloseConn(Connetion* conn);

	// Entities
	void SendDatabaseUpdate(vector<UCHAR>& out);
	void SendFullDatabase(vector<UCHAR>& out);
	void RecvDatabaseUpdate(vector<UCHAR>::iterator& in);
	void SendDatabaseUpdateToClients();
	void SendFullDatabaseToClient(Connetion* conn);
	void RecvDatabaseUpdateFromServer();

	// Player
	void SendPlayerDataTo(vector<UCHAR>& out, Player* player);
	void RecvPlayerDataFrom(vector<UCHAR>::iterator& in, Player* player);
	void SendPlayerDataToServer();
	void RecvPlayerDataFromClients();

private:

	void Skip(vector<UCHAR>& buffer, int count)
	{
		vector<UCHAR> newBuffer;
		copy(buffer.begin() + count, buffer.end(), back_inserter(newBuffer));
		buffer.swap(newBuffer);
	}
};