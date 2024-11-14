#ifndef Listener_H
#define Listener_H

#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <SDKDDKVer.h>
#pragma comment(lib, "ws2_32")

#include <list>
#include <iterator>
#include<windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define MAX_THREAD_CNT	8

typedef struct Session {
	SOCKET clientSocket;
	char buffer[8192];
}Session;


class Listener {
public:

	std::list<SOCKET> socket_list;
	CRITICAL_SECTION socket_cs;
	Listener();
	HANDLE IOCP_Handler;

	void SendChattingMessage(char* pszParam, SOCKET clientSocket);
	bool CloseServer();

	//Handler Function
	bool CloseSocketHandler(DWORD dwType);
	//Thread
	DWORD WINAPI CompleteThreadFunc(LPVOID param);
	DWORD WINAPI AcceptThreadFunc(LPVOID param);
private:
	//Main Function
	void StartProgram();

	//Start ListenSocket Func
	bool ResetWinsock();
	bool CreateSocket();
	bool BindPort();
	bool WaitingClient(SOCKET listenSocket);

	void CloseClient(SOCKET clientSocket);
	void CloseAllClient();
	
	//Handler
	bool InitCtrlHandler();
	bool InitIOCPHandler();

};
#endif