#include <iostream>
#include <string>
#include "Network.h"

using namespace std;

int main() {
	//init network
	Network net = Network();

	net.connectToServer();
	net.startUpdates();

	while (true)
	{
		string command;
		getline(cin, command);

		if (command == "/quit")
		{
			net.listening = false;
			break;
		}
		else if (command == "/test")
		{
			net.sendData(MESSAGE, "ping", false);
		}
		else
		{
			net.sendData(MESSAGE, command, false);
		}
	}
}