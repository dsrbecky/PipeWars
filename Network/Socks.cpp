#include "StdAfx.h"
#include "../Entities.h"
#include "Network.h"

void Error(string msg)
{
	MessageBoxA(NULL, msg.c_str(), "Network error", MB_OK);
	exit(1);
}

static bool WSAStarted = false;

void WSAStart()
{
	if (!WSAStarted) {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) Error("WSAStartup failed");
		WSAStarted = true;
	}
}

void Network::StartListening()
{
	WSAStart();

    struct addrinfo *result = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, Port, &hints, &result) != 0) Error("getaddrinfo failed");

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) Error("socket failed");
    if (bind(listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) Error("Bind failed (server already running?)");

	freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) Error("listen failed");

	// Set the database to "unmodified" state
	vector<UCHAR> updateData;
	SendDatabaseUpdate(updateData);

	this->serverRunning = true;
}

int nextPlayerColourId = 0;

Player* Network::AcceptNewConnection()
{
	// NOTE: Only one connection can be accepted per server frame!
	//
	// (or adjust the code so that the same initial database is send to
	// all newcomers - ie. the second player must not recive the first
	// player in the database.  This is because all player additions
	// will be reported in next update)

	assert(this->serverRunning);

	fd_set fdset; FD_ZERO(&fdset); FD_SET(listenSocket, &fdset);
	timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;

	// Are there new connections?
	if (select(0, &fdset, NULL, NULL, &timeout) == 0) return NULL;

	Connetion* connection = new Connetion();

	// Accept connection
	connection->socket = accept(listenSocket, NULL, NULL);
	if (connection->socket == INVALID_SOCKET) Error("accept failed");

	// Must do before Player is added (database without the player)
	SendFullDatabaseToClient(connection);

	// Add player
	connection->player = new Player("(new player)", nextPlayerColourId++);
	connection->nameTransmited = false;
	this->database.add(connection->player);

	connections.insert(connection);	

	return connection->player;
}

// If the connection is non-blocking, we assume sucess and continue
bool Network::Joint(string ip, string playerName, bool nonBlocking)
{
	WSAStart();

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

	// Resolve name
	if (getaddrinfo(ip.c_str(), Port, &hints, &result) != 0) {
		MessageBoxA(NULL, "Can not resolve name", "Network error", MB_OK);
		return false;
	}

	// Try all posibilies
	ptr = result;
	for(ptr = result; ptr != NULL; ptr=ptr->ai_next) {
		Connetion* conn = new Connetion();

		conn->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (conn->socket == INVALID_SOCKET) Error("socket failed");

		// Enter non-blocking mode
		if (nonBlocking) {
			u_long nonBlocking = 1;
			ioctlsocket(conn->socket, FIONBIO, &nonBlocking);
		}

		// Try connecting
		if (connect(conn->socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				closesocket(conn->socket);
				delete conn;
				continue; // Try other adress
			}
        }

		char name[MAX_STR_LEN];
		ZeroMemory(name, sizeof(name));
		playerName.copy(name, sizeof(name));
		copy(name, name + MAX_STR_LEN, back_inserter(conn->outBuffer));
		conn->nameTransmited = true;

		connections.insert(conn);
		freeaddrinfo(result);
		this->clientRunning = true;

		return true; // Success
	}

	MessageBoxA(NULL, "Can not connect to server", "Network error", MB_OK);
	return false;
}

void Network::CloseConn(Connetion* conn)
{
	string msg = this->serverRunning ? "Connection lost to " + string(conn->player->name) : "Connection to server lost";
	MessageBoxA(NULL, msg.c_str() , "Network error", MB_OK);

	closesocket(conn->socket);
	if (conn->player != NULL)
		this->database.remove(conn->player);
	this->connections.erase(conn);
	delete conn;

	if (this->clientRunning) {
		exit(0); // Connection to server lost
	}
}

// Generic receive function used both on client and server
void Network::RecvSocketData()
{
	assert(this->serverRunning || this->clientRunning);

loop:
	{ ConnLoop
		while(true) {
			fd_set fdset; FD_ZERO(&fdset); FD_SET(conn->socket, &fdset);
			timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;

			// Any data to be read?
			if (select(0, &fdset, NULL, NULL, &timeout) == 0) break;

			char buf[BuffLen];
			int count = recv(conn->socket, buf, sizeof(buf), 0);
			if (count > 0) {
				copy(buf, buf + count, back_inserter(conn->inBuffer));
			} else {
				CloseConn(conn);
				goto loop;
			}
		}
	}
}

// Generic send function used both on client and server
void Network::SendSocketData()
{
	assert(this->serverRunning || this->clientRunning);

loop:
	{ ConnLoop
		while(true) {
			// Any data to write?
			if (conn->outBuffer.size() == 0) break;

			fd_set fdset; FD_ZERO(&fdset); FD_SET(conn->socket, &fdset);
			timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;

			// Can we write?
			if (select(0, NULL, &fdset, NULL, &timeout) == 0) break;

			int count = send(conn->socket, (char*)&conn->outBuffer[0], conn->outBuffer.size(), 0);
			if (count > 0) {
				Skip(conn->outBuffer, count);
			} else {
				CloseConn(conn);
				goto loop;
			}
		}
	}
}

Network::~Network()
{
	for(hash_map<ID, pair<UCHAR, UCHAR*>>::iterator it = lastSendDatas.begin(); it != lastSendDatas.end(); it++) {
		delete it->second.second;
	}
	for(hash_map<ID, UCHAR*>::iterator it = lastRecvDatas.begin(); it != lastRecvDatas.end(); it++) {
		delete it->second;
	}
	{ ConnLoop
		closesocket(conn->socket);
		delete conn;
	}
	connections.clear();
	if (listenSocket != INVALID_SOCKET)
		closesocket(listenSocket);
	if (WSAStarted) {
		WSACleanup();
		WSAStarted = false;
	}
}
