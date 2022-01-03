#include "Network.h"

Network::Network()
{
	WSADATA data;
	WORD version = MAKEWORD(2, 2);

	//Start winsock
	int wsOK = WSAStartup(version, &data);
	if (wsOK != 0)
	{
		cerr << "Cannot start winsock: " << wsOK << endl;
		return;
	}

	//Connect UDP
	udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp == INVALID_SOCKET)
	{ 
		cerr << "Can't create UDP socket!" << endl;
	}

	//socket setup
	udpAddr.sin_addr.S_un.S_addr = ADDR_ANY;
	udpAddr.sin_family = AF_INET;
	udpAddr.sin_port = htons(DEFAULT_PORT);
	int udpLength = sizeof(udpAddr);
	if (bind(udp, (sockaddr*)&udpAddr, udpLength) == SOCKET_ERROR)
	{
		cerr << "Can't bind UDP socket" << WSAGetLastError() << endl;
	}

	//Connect TCP
	tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp == INVALID_SOCKET)
	{
		cerr << "Can't create TCP socket!" << endl;
	}

	//socket setup
	tcpAddr.sin_addr.S_un.S_addr = ADDR_ANY;
	tcpAddr.sin_family = AF_INET;
	tcpAddr.sin_port = htons(DEFAULT_PORT+1);
	int tcpLength = sizeof(tcpAddr);
	if (bind(tcp, (sockaddr*)&tcpAddr, tcpLength) == SOCKET_ERROR)
	{
		cerr << "Can't bind TCP socket" << WSAGetLastError() << endl;
	}

	listening = true;

	listen(tcp, SOMAXCONN);

	FD_ZERO(&master);
	FD_SET(tcp, &master);
	FD_SET(udp, &master);


}

Network::~Network()
{
	listening = false;
	FD_CLR(tcp, &master);
	FD_CLR(udp, &master);
	closesocket(tcp);
	closesocket(udp);
	WSACleanup();
}

void Network::acceptNewClient(int sender, sockaddr addr, int length)
{
	if (sender <= connectedUsers.size())
	{
		//TODO: check if the sender already has a profile

		connectedUsers[sender - 1].userAddr = addr;
		connectedUsers[sender - 1].clientLength = length;

		char str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET, &(((sockaddr_in*)&connectedUsers[sender - 1].userAddr)->sin_addr),
			str, INET_ADDRSTRLEN);

		connectedUsers[sender - 1].clientIP = str;
		connectedUsers[sender - 1].active = true;

		cout << "Client " << connectedUsers[sender - 1].index << " has connected." << endl;
	}
	else
	{
		cerr << "Connection Error" << endl;
	}

}

void Network::startUpdates()
{
	cout << "Server is running..." << endl;

	//Command line thread
	thread CommandLine = thread([&](){
		while (listening) 
		{
			string command;
			getline(cin, command);
			
			if (command == "/quit") 
			{
				listening = false;
			}
		}
	});
	CommandLine.detach();

	while (listening)
	{
		fd_set copy = master;
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++)
		{
			cout << "Packet Recieved: ";
			SOCKET sock = copy.fd_array[i];

			//new client connection
			if (sock == tcp)
			{
				//create a tcp socket for the client
				SOCKET client = accept(tcp, nullptr, nullptr);

				//setup the new connection
				UserProfile newProfile = UserProfile();
				newProfile.tcpSock = client;
				newProfile.index = ++clientCount;
				FD_SET(client, &master);
				connectedUsers.push_back(newProfile);

				//send outgoing connection packet back to client
				Packet initPack;
				initPack.sender = 0;
				initPack.packet_type = INIT_CONNECTION;
				strcpy_s(initPack.data, (to_string(newProfile.index)).c_str() + '\0');
				sendTCP(initPack, client);
			}
			//upd socket
			else if (sock == udp)
			{
				char* buff = new char[DATA_SIZE];
				sockaddr clientAddr;
				int clientLength;

				int recieveOK = recvfrom(udp, buff, DATA_SIZE, 0, &clientAddr, &clientLength);
				if(recieveOK == SOCKET_ERROR)
				{ 
					cout << "UPD recieve error: " << WSAGetLastError() << endl;
				}
				else
				{
					Packet pack;
					pack.deseralize(buff);

					if (pack.packet_type == PacketType::INIT_CONNECTION)
					{
						cout << "UDP Connected" << endl;
						vector<string> parsedData = Tokenizer::tokenize(',', pack.data);

						acceptNewClient(stoi(parsedData[0]), clientAddr, clientLength);
					}
					else 
					{
						processPacket(pack, false);
					}
				}
			}
			//client tcp socket
			else
			{
				char* buff = new char[DATA_SIZE];

				int recieveOK = recv(sock, buff, DATA_SIZE, 0);
				if (recieveOK == SOCKET_ERROR)
				{
					cout << "TCP recieve error: " << WSAGetLastError() << endl;
				}
				//graceful disconnection
				else if (recieveOK == 0)
				{
					for (UserProfile profile : connectedUsers)
					{
						if (sock == profile.tcpSock)
						{
							profile.active = false;
							cout << "Client " << profile.index << " Disconnected." << endl;
						}
					}

					//drop the client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else 
				{
					Packet pack;
					pack.deseralize(buff);
					processPacket(pack, true);
				}
			}
		}
	}
}

void Network::sendUDP(Packet pack, int clientID)
{
	const unsigned int packet_size = sizeof(pack);
	char packet_data[packet_size];

	pack.seralize(packet_data);
	int sendOK = sendto(udp, packet_data, packet_size, 0,
		&connectedUsers[clientID - 1].userAddr,
		connectedUsers[clientID - 1].clientLength);

	if (sendOK == SOCKET_ERROR)
	{
		cerr << "Send Error: " << WSAGetLastError() << endl;
	}

}

void Network::sendTCP(Packet pack, SOCKET sock)
{
	const unsigned int packet_size = sizeof(pack);
	char packet_data[packet_size];

	pack.seralize(packet_data);
	int sendOK = send(sock, packet_data, packet_size, 0);
	if (sendOK == SOCKET_ERROR)
	{
		cerr << "Send Error: " << WSAGetLastError() << endl;
	}
}

void Network::relay(Packet pack, bool useTCP)
{
	for (int counter = 1; counter < connectedUsers.size(); counter++)
	{
		if (counter == pack.sender) 
		{
			continue;
		}
		const unsigned int packet_size = sizeof(pack);
		char packet_data[packet_size];

		pack.seralize(packet_data);
		int sendOK;
		if (useTCP) {
			sendOK = send(connectedUsers[counter].tcpSock, packet_data, packet_size, 0);
		}
		else
		{
			sendOK = sendto(udp, packet_data, packet_size, 0, &connectedUsers[counter].userAddr, connectedUsers[counter].clientLength);
		}
		if (sendOK == SOCKET_ERROR)
		{
			cerr << "Send Error: " << WSAGetLastError() << endl;
		}
	}
}

void Network::processPacket(Packet pack, bool useTCP)
{
	if (verbose)
	{
		cout << pack.data;
	}

	switch (pack.packet_type)
	{
	case PacketType::MESSAGE:
		relay(pack, true);
		break;
	default:
		cout << "Error: Unhandled packet Type";
		break;
	}
}
