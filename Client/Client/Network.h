#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include "Packet.h"
#include "Tokenizer.h"

#pragma comment(lib, "ws2_32.lib")
#define DEFAULT_PORT 6343
#pragma once

using namespace std;

class Network
{
public:
	Network();
	~Network();

	SOCKET udp;
	SOCKET tcp;

	sockaddr_in udpAddr;
	sockaddr_in tcpAddr;

	bool listening = true;
	int index;

	bool connected = false;

	string serverIP = "";

public:
	bool connectToServer(string ip = "127.0.0.1");

	void startUpdates();

	void ProcessPacket(Packet pack, bool useTCP);

	bool sendData(int packetType, string message, bool useTCP = false);
};

