#pragma once

#include "Define.h"
#include <stdio.h>

class Session {
public:
	Session();

	void Init(UINT32 index);
	UINT32 GetIndex();
	bool IsConnect();
	SOCKET GetSocket();
	char* GetRecvBuffer();
	char* GetSendBuffer();

	bool OnConnect(HANDLE iocpHandle, SOCKET clientSocket);
	void Close();
	
	bool BindIOCP(HANDLE iocpHandle);
	bool BindRecv();
	bool SendPacket(UINT32 transferSize, char* packet);
	void SendComplete(UINT32 transferSize);

private:
	int index = 0;
	SOCKET clientSocket;
	OverlappedEx RecvOverlappedEx;
	OverlappedEx SendOverlappedEx;

	char sendBuf[SOCK_BUF_SIZE];
	char recvBuf[SOCK_BUF_SIZE];
};