#ifndef Connector_H
#define Connector_H

#include <WinSock2.h>
#include <stdio.h>
#include <tchar.h>
#include <sdkddkver.h>

#pragma comment (lib, "ws2_32")

class Connector {
public:
	Connector();
private:
	SOCKET clientSocket;

	HANDLE recvThread;

	bool ResetWinsock();
	bool CreateSocket();
	bool BindPort();
	void StartThread();
	void CloseSocket();
	void ChatMessage();
};
#endif
