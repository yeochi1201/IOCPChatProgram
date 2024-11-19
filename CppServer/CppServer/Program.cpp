#include <iostream>
#include "Listener.h"
#include <string>

int main() {
	Listener* listener = new Listener();

	listener->InitSocket(25000, 100);
	while (true) {
		std::string input;
		std::getline(std::cin, input);

		if (input == "quit")
			break;
	}
	listener->CloseServer();
	return 0;
}