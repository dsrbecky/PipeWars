#include "StdAfx.h"
#include "../Entities.h"
#include "Network.h"

void Error(string msg)
{
	MessageBoxA(NULL, msg.c_str() , "Network error", MB_OK);
	exit(1);
}

void Network::StartListening()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) Error("WSAStartup failed");

    struct addrinfo *result = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, Port, &hints, &result) != 0) Error("getaddrinfo failed");

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) Error("socket failed");
    if (bind(listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) Error("bind failed");

	freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) Error("listen failed");
}

void Network::AcceptNewConnections()
{
	while (true) {
		fd_set fdset; FD_ZERO(&fdset); FD_SET(listenSocket, &fdset);
		timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;

		// Are there new connections?
		if (select(0, &fdset, NULL, NULL, &timeout) == 0) break;

		Connetion* connection = new Connetion();

		// Accept connection
		connection->socket = accept(listenSocket, NULL, NULL);
		if (connection->socket == INVALID_SOCKET) Error("accept failed");

		// Add player
		connection->player = new Player("New player");
		connection->nameTransmited = false;
		db.add(connection->player);

		SendFullDatabase(connection->outBuffer);

		connections.insert(connection);
	}
}

void Network::Joint(string ip)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) Error("WSAStartup failed");

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

	// Resolve name
	if (getaddrinfo(ip.c_str(), Port, &hints, &result) != 0) Error("getaddrinfo failed");

	// Try all posibilies
	ptr = result;
	for(ptr = result; ptr != NULL; ptr=ptr->ai_next) {
		Connetion* conn = new Connetion();
		conn->player = NULL;
		conn->nameTransmited = false;

		conn->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (conn->socket == INVALID_SOCKET) Error("socket failed");

		// Try connecting
		if (connect(conn->socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
			closesocket(conn->socket);
            conn->socket = INVALID_SOCKET;
            continue;
        }

		connections.insert(conn);
		break;
	}

	freeaddrinfo(result);
}

// Generic receive function used both on client and server
void Network::RecvSocketData()
{
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
				// Connection closed
				closesocket(conn->socket);
				if (conn->player != NULL)
					db.remove(conn->player);
				connections.erase(conn);
				string msg = conn->player != NULL ? "Connection lost to " + string(conn->player->name) : "Connection lost";
				MessageBoxA(NULL, msg.c_str() , "Network error", MB_OK);
			}
		}
	}
}

// Generic send function used both on client and server
void Network::SendSocketData()
{
	{ ConnLoop
		while(true) {
			// Any data to write?
			if (conn->outBuffer.size() == 0) break;

			fd_set fdset; FD_ZERO(&fdset); FD_SET(conn->socket, &fdset);
			timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 0;

			// Can we write?
			if (select(0, NULL, &fdset, NULL, &timeout) == 0) break;

			char buf[BuffLen];
			int count = send(conn->socket, buf, sizeof(buf), 0);
			if (count > 0) {
				copy(buf, buf + count, back_inserter(conn->inBuffer));
			} else {
				// Connection closed
				closesocket(conn->socket);
				if (conn->player != NULL)
					db.remove(conn->player);
				connections.erase(conn);
				string msg = conn->player != NULL ? "Connection lost to " + string(conn->player->name) : "Connection lost";
				MessageBoxA(NULL, msg.c_str() , "Network error", MB_OK);
			}
		}
	}
}

void Network::Shutdown()
{
	{ ConnLoop
		closesocket(conn->socket);
	}
	closesocket(listenSocket);
    WSACleanup();
}
