#pragma once

#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <SDKDDKVer.h>
#pragma comment(lib, "ws2_32")

#include <list>
#include <iterator>
#include <thread>
#include <vector>
#include "Define.h"
#include "Session.h"

#include<windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

class IOCPServer {
public:
	IOCPServer() {

	}
	~IOCPServer() {
		WSACleanup();
	}


	bool CloseServer();
	bool InitSocket(UINT16 portNum, UINT16 maxClient);
	void DestroyThread();
	bool SendPacket(UINT32 index, char* packet, int transferSize);
	//Event Func 
	virtual bool OnConnect(UINT32 index) {
		printf("%d Client Connect\n", index);
		return true;
	}
	virtual bool OnDisconnect(UINT32 index) {
		printf("%d Client Disconnect\n", index);
		return true;
	}
	virtual bool OnSend(Session* session, char* buf, DWORD transfersize) {
		return true;
	}
	virtual bool OnRecv(Session* session, char* buf, DWORD transfersize) {
		return true;
	}
	
private:
	SOCKET listenSocket = INVALID_SOCKET;

	HANDLE IOCP_Handler = INVALID_HANDLE_VALUE;


	std::list<std::thread> workerThreads;
	std::thread acceptThread;
	std::vector<Session> Sessions;
	std::mutex sessionLock;

	//Handler
	bool InitIOCPHandler();

	//Open Func
	bool ResetWinsock();
	bool CreateSocket();
	bool BindPort(UINT16 portNum);
	bool WaitingClient(SOCKET listenSocket);
	
	//Close Func
	void CloseClient(Session* session);
	void CloseAllClient();
	
	//Session Func
	void CreateSessions(UINT16 maxClient);
	Session* GetEmptySession();
	Session* GetSession(UINT32 index);

	//Thread Func
	
	bool CreateWorkerThread();
	bool CreateAcceptThread();
	DWORD WINAPI WorkerThreadFunc();
	DWORD WINAPI AcceptThreadFunc();

};