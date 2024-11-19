#include "Listener.h"
#include "Define.h"

#pragma region Handler
bool Listener::InitIOCPHandler() {
	IOCP_Handler = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (IOCP_Handler == NULL) {
		puts("ERROR : Cant Create IOCP");
		return FALSE;
	}
	return TRUE;
}
#pragma endregion

#pragma region Event Func
bool Listener::OnConnect(Session* session) {
	return true;
}
bool Listener::OnDisconnect(Session* session) {
	return true;
}
bool Listener::OnSend(Session* session, char* buf, DWORD transfersize) {
	return true;
}
bool Listener::OnRecv(Session* session, char* buf, DWORD transfersize) {
	return true;
}
#pragma endregion

#pragma region SessionFunc
void Listener::CreateSessions(UINT16 maxClient) {
	for (int i = 0; i < maxClient; i++) {
		Sessions.emplace_back();
		Sessions[i].index = i;
	}
}

Session* Listener::GetEmptySession() {
	for (Session session : Sessions) {
		if (session.clientSocket == INVALID_SOCKET)
			return &session;
	}
	return nullptr;
}

#pragma endregion

#pragma region IOCP Func
bool Listener::BindIOCP(Session* session) {
	HANDLE IOCP_h = CreateIoCompletionPort((HANDLE)session->clientSocket, 
		IOCP_Handler, 
		(ULONG_PTR)session, 
		0);
	if (IOCP_h == NULL || IOCP_Handler != IOCP_h)
		return false;
	return true;
}

bool Listener::BindRecv(Session* session) {
	DWORD flag = 0;
	DWORD RecvBytesNum = 0;

	session->RecvOverlappedEx.wsaBuf.len = SOCK_BUF_SIZE;
	session->RecvOverlappedEx.wsaBuf.buf = session->recvBuf;
	session->RecvOverlappedEx.operation = IOOperation::RECV;

	int result = WSARecv(session->clientSocket,
		&(session->RecvOverlappedEx.wsaBuf),
		1,
		&RecvBytesNum,
		&flag,
		(LPWSAOVERLAPPED) & (session->RecvOverlappedEx),
		NULL);
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		puts("ERROR : WSA Recv() occurred");
		return false;
	}
	return true;
}

bool Listener::SendMsg(Session* session, char* msg, int msgLen) {
	DWORD SendBytesNum;

	CopyMemory(session->sendBuf, msg, msgLen);

	session->SendOverlappedEx.wsaBuf.len = msgLen;
	session->SendOverlappedEx.wsaBuf.buf = session->sendBuf;
	session->SendOverlappedEx.operation = IOOperation::SEND;

	int result = WSASend(session->clientSocket,
		&(session->SendOverlappedEx.wsaBuf),
		1,
		&SendBytesNum,
		0,
		(LPWSAOVERLAPPED) & (session->SendOverlappedEx),
		NULL);
	if (result == SOCKET_ERROR || (WSAGetLastError() != ERROR_IO_PENDING))
	{
		puts("ERROR : WSA Send() occured");
		return false;
	}

	return true;
}
#pragma endregion

#pragma region Open Func
bool Listener::InitSocket(UINT16 portNum, UINT16 maxClient) {
	ResetWinsock();
	CreateSocket();
	BindPort(portNum);
	WaitingClient(listenSocket);
	CreateSessions(maxClient);
	InitIOCPHandler();
	CreateWorkerThread();
	CreateAcceptThread();

	return true;
}

bool Listener::ResetWinsock() {
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("ERROR : Failed to Reset Window Socket");
		return false;
	}
	puts("Reset Window Socket");
	return true;
}

bool Listener::CreateSocket() {
	listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (listenSocket == INVALID_SOCKET) {
		puts("ERROR : Failed to Create Listen Socket");
		return false;
	}
	puts("Create Listener Socket");
	return true;
}

bool Listener::BindPort(UINT16 portNum) {
	SOCKADDR_IN serverAddress = { 0 };
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNum);
	serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		puts("ERROR : Failed to bind Port");
		return false;
	}
	puts("Binding Port");
	return true;
}

bool Listener::WaitingClient(SOCKET listenSocket) {
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		puts("ERROR : Failed to Listen");
		CloseServer();
		return false;
	}
	puts("Waiting Client");
	return true;
}
#pragma endregion

#pragma region Close Func
void Listener::CloseClient(Session* session) {
	shutdown(session->clientSocket, SD_BOTH);
	closesocket(session->clientSocket);
	session->clientSocket = INVALID_SOCKET;
	OnDisconnect(session);
}

void Listener::CloseAllClient() {
	for (Session session : Sessions) {
		CloseClient(&session);
	}
}

bool Listener::CloseServer() {
	CloseAllClient();

	DestroyThread();

	::DeleteCriticalSection(&session_cs);
	WSACleanup();
	return TRUE;
}
#pragma endregion

#pragma region Thread Func
DWORD WINAPI Listener::WorkerThreadFunc() {
	DWORD transferSize = 0;
	Session* pSession = NULL;
	LPOVERLAPPED pWol = NULL;
	BOOL result;

	while (1) {
		result = GetQueuedCompletionStatus(
			IOCP_Handler, &transferSize, (PULONG_PTR)&pSession, &pWol, INFINITE);
		
		if (result == TRUE && transferSize == 0 && pWol == NULL) {
			continue;
		}
		if (result == FALSE || (transferSize == 0 && result == TRUE)) {
			puts(" ClientSocket Disconnect or Server Down");
			CloseClient(pSession);
			continue;
		}

		OverlappedEx* pOverlappedEx = (OverlappedEx*)pWol;
		switch (pOverlappedEx->operation) {
		case IOOperation::RECV:
			OnRecv(pSession, pSession->recvBuf, transferSize);
			SendMsg(pSession, pSession->recvBuf, transferSize);
			BindRecv(pSession);
			break;
		case IOOperation::SEND:
			OnSend(pSession, pSession->sendBuf, transferSize);
			break;
		default:
			break;
		}
	}
	return 0;
}

DWORD WINAPI Listener::AcceptThreadFunc() {
	int addrSize = sizeof(SOCKADDR);
	SOCKADDR clientAddr;
	SOCKET clientSocket;

	while ((clientSocket = ::accept(listenSocket, &clientAddr, &addrSize))) {
		
		Session* pSession = GetEmptySession();
		if (pSession->clientSocket == INVALID_SOCKET && pSession!=nullptr)
			continue;

		if (!BindIOCP(pSession))
			return 1;

		if (!BindRecv(pSession))
			return 1;

		puts("New Client Connected");
		OnConnect(pSession);
	}

	return 0;
}

bool Listener::CreateWorkerThread() {
	DWORD threadID = 0;
	for (int i = 0; i < MAX_THREAD_CNT; i++) {
		workerThreads.emplace_back([this]() {WorkerThreadFunc(); });
	}

	return true;
}

bool Listener::CreateAcceptThread() {
	DWORD threadID = 0;

	acceptThread = std::thread([this]() {AcceptThreadFunc(); });

	return true;
}

void Listener::DestroyThread() {
	CloseHandle(IOCP_Handler);

	for (auto& th : workerThreads) {
		if (th.joinable())
			th.join();
	}

	closesocket(listenSocket);
	if (acceptThread.joinable())
		acceptThread.join();
}
#pragma endregion
