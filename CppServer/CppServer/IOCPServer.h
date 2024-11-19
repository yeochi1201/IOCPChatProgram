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
	bool CloseServer();
	bool InitSocket(UINT16 portNum, UINT16 maxClient);
	void DestroyThread();
	bool SendPacket(UINT32 index, char* packet, int transferSize);
	//Event Func 
	virtual bool OnConnect(UINT32 index);
	virtual bool OnDisconnect(UINT32 index);
	virtual bool OnSend(Session* session, char* buf, DWORD transfersize);
	virtual bool OnRecv(Session* session, char* buf,DWORD transfersize);
	
private:
	SOCKET listenSocket;
	CRITICAL_SECTION session_cs;

	HANDLE IOCP_Handler;

	std::list<std::thread> workerThreads;
	std::thread acceptThread;
	std::vector<Session> Sessions;

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