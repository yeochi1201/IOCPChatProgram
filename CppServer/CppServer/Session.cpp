#include "Session.h"

Session::Session() {
	ZeroMemory(&RecvOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(&SendOverlappedEx, sizeof(OverlappedEx));
	clientSocket = INVALID_SOCKET;
}
void Session::Init(UINT32 index) {
	this->index = index;
}

bool Session::OnConnect(HANDLE iocpHandle, SOCKET clientSocket) {
	this->clientSocket = clientSocket;

	if (!BindIOCP(iocpHandle))
		return false;
	return BindRecv();
}

void Session::Close() {
	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	clientSocket = INVALID_SOCKET;
}

bool Session::BindIOCP(HANDLE iocpHandle) {
	HANDLE IOCP_h = CreateIoCompletionPort((HANDLE)clientSocket,
		iocpHandle,
		(ULONG_PTR)(this),
		0);
	if (IOCP_h == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

bool Session::BindRecv() {
	DWORD flag = 0;
	DWORD RecvBytesNum = 0;

	RecvOverlappedEx.wsaBuf.len = SOCK_BUF_SIZE;
	RecvOverlappedEx.wsaBuf.buf = recvBuf;
	RecvOverlappedEx.operation = IOOperation::RECV;

	int result = WSARecv(clientSocket,
		&(RecvOverlappedEx.wsaBuf),
		1,
		&RecvBytesNum,
		&flag,
		(LPWSAOVERLAPPED) & (RecvOverlappedEx),
		NULL);
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		puts("ERROR : WSA Recv() occurred");
		return false;
	}
	return true;
}

bool Session::SendPacket(UINT32 transferSize, char* packet) {
	DWORD SendBytesNum;

	CopyMemory(sendBuf, packet, transferSize);

	SendOverlappedEx.wsaBuf.len = transferSize;
	SendOverlappedEx.wsaBuf.buf = sendBuf;
	SendOverlappedEx.operation = IOOperation::SEND;

	int result = WSASend(clientSocket,
		&(SendOverlappedEx.wsaBuf),
		1,
		&SendBytesNum,
		0,
		(LPWSAOVERLAPPED) & (SendOverlappedEx),
		NULL);

	if (result == SOCKET_ERROR || (WSAGetLastError() != ERROR_IO_PENDING))
	{
		puts("ERROR : WSA Send() occured");
		return false;
	}

	return true;
}

void Session::SendComplete(UINT32 transferSize) {
	printf("[Send Complete] bytes : %d\n", transferSize);
}