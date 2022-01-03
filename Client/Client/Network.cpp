#include "Network.h"

Network::Network()
{
	//start winsock
	WSADATA data;
	WORD version = MAKEWORD(2, 2);

	int wsOK = WSAStartup(version, &data);
	if(wsOK != 0)
	{
		cerr << "Cannot start winsock: " << wsOK << endl;
		return;
	}

	//setup server udp socket
	udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp == INVALID_SOCKET)
	{
		cerr << "Can't create UDP socket!" << endl;
	}
	udpAddr.sin_family = AF_INET;
	udpAddr.sin_port = htons(DEFAULT_PORT);

	//setup connection tcp socket 
	tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp == INVALID_SOCKET)
	{
		cerr << "Can't create tcp socket!" << endl;
	}
	tcpAddr.sin_family = AF_INET;
	tcpAddr.sin_port = htons(DEFAULT_PORT + 1);
}

Network::~Network()
{
	listening = false;
	closesocket(tcp);
	closesocket(udp);
	WSACleanup();
}

bool Network::connectToServer(string ip)
{
	serverIP = ip;
	inet_pton(AF_INET, ip.c_str(), &tcpAddr.sin_addr);

	int result = connect(tcp, (sockaddr*)&tcpAddr, sizeof(tcpAddr));
	if (result == SOCKET_ERROR)
	{
		cerr << "Can't connect to the server: " << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

void Network::startUpdates()
{
	thread udpUpdate = thread([&]() {
		char* buff = new char[DATA_SIZE];

		while (true)
		{
			int length = sizeof(udpAddr);
			int recieveOK = recvfrom(udp, buff, DATA_SIZE, 0, (sockaddr*)&udpAddr, &length);
			if (recieveOK == SOCKET_ERROR)
			{
				cout << "UDP Packet error: " << WSAGetLastError() << endl;
			}
			else
			{
				Packet pack;
				pack.deseralize(buff);
				ProcessPacket(pack, false);
			}
		}
	});
	udpUpdate.detach();
	thread tcpUpdate = thread([&]() {
		char* buff = new char[DATA_SIZE];

		while (true)
		{
			int recieveOK = recv(tcp, buff, DATA_SIZE, 0);
			if (recieveOK == SOCKET_ERROR)
			{
				cout << "TCP Packet error: " << WSAGetLastError() << endl;
			}
			else
			{
				Packet pack;
				pack.deseralize(buff);
				ProcessPacket(pack, true);
			}
		}
	});
	tcpUpdate.detach();
}

void Network::ProcessPacket(Packet pack, bool useTCP)
{
	vector<string> parsedData = Tokenizer::tokenize(',', pack.data);

	switch (pack.packet_type)
	{
	case PacketType::INIT_CONNECTION:
		if (pack.sender != 0)
		{
			cout << "ERROR: init packet sent by non-server" << endl;
			return;
		}
		if (useTCP)
		{
			index = stoi(parsedData[0]);
			cout << "Initial connection recieved. Index: " + parsedData[0] << endl;

			//trigger connection to udp
			inet_pton(AF_INET, serverIP.c_str(), &udpAddr.sin_addr);
			sendData(INIT_CONNECTION, to_string(index), false);
		}
		else
		{
			if (stoi(parsedData[0]) != index)
			{
				cout << "Connection index mismatch error" << endl;
				return;
			}
			cout << "Connected." << endl;
			connected = true;
		}
		break;
	case PacketType::MESSAGE:
		cout << "User " << to_string(pack.sender) << ": " << parsedData[0] << endl;
		break;
	default:
		cout << "Error: Unhandled packet type." << endl;
	}
}

bool Network::sendData(int packetType, string message, bool useTCP)
{
	//create the packet
	Packet pack;
	strcpy_s(pack.data, message.c_str() + '\0');
	pack.packet_type = packetType;
	pack.sender = index;

	//create packet buffer
	const unsigned int packet_size = sizeof(pack);
	char packet_data[packet_size];
	pack.seralize(packet_data);

	if (useTCP)
	{
		int sendOK = send(tcp, packet_data, packet_size, 0);
		if (sendOK == SOCKET_ERROR)
		{
			cout << "Send Error: " << WSAGetLastError() << endl;
			return false;
		}
	}
	else
	{
		int sendOK = sendto(udp, packet_data, packet_size, 0, (sockaddr*)&udpAddr, sizeof(udpAddr));
		if (sendOK == SOCKET_ERROR)
		{
			cout << "Send Error: " << WSAGetLastError() << endl;
			return false;
		}
	}
	return true;
}
