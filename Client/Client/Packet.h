#pragma once
#include <string>
#define DATA_SIZE 504

enum PacketType
{
	//New client init
	INIT_CONNECTION = 0,
	//string - TCP
	MESSAGE = 1,
	//ping
	PING = 2,

};

struct Packet
{
	unsigned int packet_type;
	int sender = 0;
	char data[DATA_SIZE];

	void seralize(char* data)
	{
		memcpy(data, this, sizeof(Packet));
	}

	void deseralize(char* data)
	{
		memcpy(this, data, sizeof(Packet));
	}
};

