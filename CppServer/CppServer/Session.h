#pragma once

#include "Define.h"
#include <stdio.h>
#include <queue>
#include <mutex>

class Session {
public:
	Session();

	void Init(UINT32 index);
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

	bool OnConnect(HANDLE iocpHandle, SOCKET clientSocket);
	void Close();
	
	bool BindIOCP(HANDLE iocpHandle);
	bool BindRecv();
	bool SendPacket(UINT32 transferSize, char* packet);
	void SendComplete(UINT32 transferSize);

private:
	int index = 0;
	std::queue<OverlappedEx*> sendDataQueue;
	std::mutex sendLock;

	SOCKET clientSocket;
	OverlappedEx RecvOverlappedEx;
	char recvBuf[SOCK_BUF_SIZE];

	void Clear(){}
	bool SendIO();
};