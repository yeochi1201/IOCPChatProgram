#include <iostream>
#include "ChatServer.h"
#include <string>

int main() {
	ChatServer* Server = new ChatServer();

	Server->Run(25000, 10);
	while (true) {
		std::string input;
		std::getline(std::cin, input);

		if (input == "quit")
			break;
	}
	Server->CloseServer();
	return 0;
}