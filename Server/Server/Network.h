#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include "Packet.h"

#define DEFAULT_PORT 6883

using namespace std;

struct UserProfile 
{
	int index;
	SOCKET tcpSock;

	sockaddr* userAddr;
	int clientLength;
	string clientIP;
	bool active = false;
};

#pragma once
class Network
{
public:
	Network();
	~Network();

	//UDP socket
	SOCKET udp;
	sockaddr_in udpAddr;

	//TCP connection-only socket
	SOCKET tcp;
	sockaddr_in tcpAddr;

	fd_set master;

	bool listening = false;

	//connected user data
	vector<UserProfile> connectedUsers;
	int clientCount = 0;

public:
	//accept and save new socket data
	void acceptNewClient(int sender, sockaddr* addr, int length);

	//begin listern to socket inputs
	void startUpdates();

	//sends a packet to a client using UDP
	void sendUDP(Packet pack, int clientID);
	//sends a packet to a client using TCP
	void sendTCP(Packet pack, SOCKET sock);

	//process inputs from sockets
	void ProcessTCP(Packet pack);
	void ProcessUDP(Packet pack);

};

