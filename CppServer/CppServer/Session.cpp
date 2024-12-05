#include "Session.h"

Session::Session() {
	ZeroMemory(&RecvOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(&recvBuf, sizeof(SOCK_BUF_SIZE));
	clientSocket = INVALID_SOCKET;
}
void Session::Init(UINT32 index) {
	this->index = index;
}

bool Session::OnConnect(HANDLE iocpHandle, SOCKET clientSocket) {
	this->clientSocket = clientSocket;

	Clear();

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
	OverlappedEx* sendOverlappedEx = new OverlappedEx;
	ZeroMemory(sendOverlappedEx, sizeof(OverlappedEx));
	DWORD SendBytesNum = 0;

	sendOverlappedEx->wsaBuf.len = transferSize;
	sendOverlappedEx->wsaBuf.buf = new char[transferSize];
	CopyMemory(sendOverlappedEx->wsaBuf.buf, packet, transferSize);
	sendOverlappedEx->operation = IOOperation::SEND;

	std::lock_guard<std::mutex>guard(sendLock);

	sendDataQueue.push(sendOverlappedEx);
	if (sendDataQueue.size() == 1)
	{
		SendIO();
	}
	return true;
}

bool Session::SendIO()
{
	OverlappedEx* sendData = sendDataQueue.front();
	DWORD recvNum = 0;
	int ret = WSASend(clientSocket,
		&(sendData->wsaBuf),
		1,
		&recvNum,
		0,
		(LPWSAOVERLAPPED)sendData,
		NULL);
	if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		printf("ERROR : WSASend() Function : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void Session::SendComplete(UINT32 transferSize) {
	printf("[Send Complete] bytes : %d\n", transferSize);

	std::lock_guard<std::mutex> guard(sendLock);
	delete[] sendDataQueue.front()->wsaBuf.buf;
	delete sendDataQueue.front();

	sendDataQueue.pop();

	if (sendDataQueue.empty() == false) {
		SendIO();
	}

}