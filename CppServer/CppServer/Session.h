#pragma once

#include "Define.h"
#include <queue>
#include <mutex>

class Session {
public:
	Session();

	void Init(UINT32 index, HANDLE iocpHandle);
	UINT32 GetIndex() {
		return index;
	}
	bool IsConnect() {
		return (clientSocket != INVALID_SOCKET);
	}
	SOCKET GetSocket() {
		return clientSocket;
	}
	char* GetRecvBuffer() {
		return recvBuf;
	}
	char* GetAccpetBuffer() {
		return acceptBuf;
	}
	UINT64 GetLatestClosedTimeSec() {
		return latestClosedTime;
	}
	
	bool Accept(SOCKET listenSocket, const UINT64 curTimeSec);
	bool AcceptComplete();
	bool OnConnect(HANDLE iocpHandle, SOCKET clientSocket);
	void Close();
	
	bool BindIOCP(HANDLE iocpHandle);
	bool BindRecv();
	bool SendPacket(UINT32 transferSize, char* packet);
	void SendComplete(UINT32 transferSize);
	

private:
	int index = 0;
	HANDLE IOCPHandle = INVALID_HANDLE_VALUE;
	UINT64 latestClosedTime;
	
	std::queue<OverlappedEx*> sendDataQueue;
	std::mutex sendLock;

	SOCKET clientSocket;
	OverlappedEx RecvOverlappedEx;
	char recvBuf[SOCK_BUF_SIZE];
	
	OverlappedEx acceptContext;
	char acceptBuf[64];

	void Clear(){}
	bool SendIO();
};