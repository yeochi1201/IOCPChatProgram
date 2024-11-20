#include "Connector.h"
#pragma warning(disable:4996)
Connector::Connector() {
	ResetWinsock();
	CreateSocket();
	BindPort();
	StartThread();
	ChatMessage();
}
DWORD WINAPI ThreadReceive(LPVOID nParam) {
	SOCKET clientSocket = (SOCKET)nParam;
	char szBuffer[128] = { 0 };
	while (1) {
		if (::recv(clientSocket, szBuffer, sizeof(szBuffer), 0) > 0) {
			printf("-> %s\n", szBuffer);
			memset(szBuffer, 0, sizeof(szBuffer));
		}
	}

	puts("Receive Thread End");
	return 0;
}

bool Connector::ResetWinsock() {
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("ERROR : Failed to Reset Window Socket");
		return false;
	}
	puts("Reset Window Socket");
	return true;
}

bool Connector::CreateSocket() {
	clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		puts("ERROR : Failed to Create Socket");
		return false;
	}
	puts("Create Client Socket");
	return true;
}

bool Connector::BindPort() {
	SOCKADDR_IN serverAddress = { 0 };
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(25000);
	serverAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(clientSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		puts("ERROR : Failed Connect Server");
		return false;
	}
	puts("Binding Port");
	return true;
}

void Connector::ChatMessage() {
	char szBuffer[128] = { 0 };

	while (1) {
		memset(szBuffer, 0, sizeof(szBuffer));
		gets_s(szBuffer);
		if (strcmp(szBuffer, "Exit") == 0)
			break;
		::send(clientSocket, szBuffer, strlen(szBuffer) + 1, 0);
	}
}

void Connector::StartThread() {
	DWORD RecvThreadID = 0;
	DWORD SendThreadID = 1;
	recvThread = CreateThread(NULL, 0, ThreadReceive, LPVOID(clientSocket), 0, &RecvThreadID);

}

void Connector::CloseSocket() {
	puts("Close Client Socket");
	::CloseHandle(recvThread);
	::closesocket(clientSocket);
	::WSACleanup();
}