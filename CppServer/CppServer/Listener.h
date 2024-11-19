#ifndef Listener_H
#define Listener_H

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

#include<windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

class Listener {
public:
	SOCKET listenSocket;
	CRITICAL_SECTION session_cs;
	
	HANDLE IOCP_Handler;

	std::list<std::thread> workerThreads;
	std::thread acceptThread;
	std::vector<Session> Sessions;

	bool CloseServer();
	bool InitSocket(UINT16 portNum, UINT16 maxClient);
	//Event Func 
	virtual bool OnConnect(Session* Session);
	virtual bool OnDisconnect(Session* session);
	virtual bool OnSend(Session* session, char* buf, DWORD transfersize);
	virtual bool OnRecv(Session* session, char* buf,DWORD transfersize);

	//Thread

private:
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

	//IOCP Func
	bool BindIOCP(Session* session);
	bool BindRecv(Session* session);
	bool SendMsg(Session* session, char* msg, int msgLen);
	
	//Thread Func
	void DestroyThread();
	bool CreateWorkerThread();
	bool CreateAcceptThread();
	DWORD WINAPI WorkerThreadFunc();
	DWORD WINAPI AcceptThreadFunc();

};
#endif