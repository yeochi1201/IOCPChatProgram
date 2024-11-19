#include "IOCPServer.h"
#include "Define.h"
#include "Session.h"

#pragma region Handler
bool IOCPServer::InitIOCPHandler() {
	IOCP_Handler = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (IOCP_Handler == NULL) {
		puts("ERROR : Cant Create IOCP");
		return FALSE;
	}
	return TRUE;
}
#pragma endregion

#pragma region Event Func
bool IOCPServer::OnConnect(UINT32 index) {
	printf("%d Client Connect", index);
	return true;
}

bool IOCPServer::OnDisconnect(UINT32 index) {
	printf("%d Client Disconnect", index);
	return true;
}

bool IOCPServer::OnSend(Session* session, char* buf, DWORD transfersize) {
	return true;
}

bool IOCPServer::OnRecv(Session* session, char* buf, DWORD transfersize) {
	return true;
}

#pragma endregion

#pragma region SessionFunc
void IOCPServer::CreateSessions(UINT16 maxClient) {
	for (int i = 0; i < maxClient; i++) {
		Sessions.emplace_back();
		Sessions[i].Init(i);
	}
}

Session* IOCPServer::GetEmptySession() {
	for (Session session : Sessions) {
		if (!session.IsConnect())
			return &session;
	}
	return nullptr;
}

Session* IOCPServer::GetSession(UINT32 index) {
	return &(Sessions[index]);
}
#pragma endregion

#pragma region IOCP Func
bool IOCPServer::SendPacket(UINT32 index, char* packet, int transferSize) {
	Session* pSession = GetSession(index);
	return pSession->SendPacket(transferSize, packet);
}
#pragma endregion

#pragma region Open Func
bool IOCPServer::InitSocket(UINT16 portNum, UINT16 maxClient) {
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

bool IOCPServer::ResetWinsock() {
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("ERROR : Failed to Reset Window Socket");
		return false;
	}
	puts("Reset Window Socket");
	return true;
}

bool IOCPServer::CreateSocket() {
	listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (listenSocket == INVALID_SOCKET) {
		puts("ERROR : Failed to Create Listen Socket");
		return false;
	}
	puts("Create IOCPServer Socket");
	return true;
}

bool IOCPServer::BindPort(UINT16 portNum) {
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

bool IOCPServer::WaitingClient(SOCKET listenSocket) {
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
void IOCPServer::CloseClient(Session* session) {
	session->Close();
	OnDisconnect(session->GetIndex());
}

void IOCPServer::CloseAllClient() {
	for (Session session : Sessions) {
		CloseClient(&session);
	}
}

bool IOCPServer::CloseServer() {
	CloseAllClient();

	DestroyThread();

	::DeleteCriticalSection(&session_cs);
	WSACleanup();
	return TRUE;
}
#pragma endregion

#pragma region Thread Func
DWORD WINAPI IOCPServer::WorkerThreadFunc() {
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
			OnRecv(pSession, pSession->GetRecvBuffer(), transferSize);
			pSession->BindRecv();
			break;
		case IOOperation::SEND:
			OnSend(pSession, pSession->GetSendBuffer(), transferSize);
			pSession->SendComplete(transferSize);
			break;
		default:
			break;
		}
	}
	return 0;
}

DWORD WINAPI IOCPServer::AcceptThreadFunc() {
	int addrSize = sizeof(SOCKADDR);
	SOCKADDR clientAddr;
	SOCKET clientSocket;

	while ((clientSocket = ::accept(listenSocket, &clientAddr, &addrSize))) {
		
		Session* pSession = GetEmptySession();
		if (pSession->GetSocket() == INVALID_SOCKET && pSession != nullptr)
			continue;
		if (pSession->OnConnect(IOCP_Handler, clientSocket)) {
			puts("New Client Connected");
		}
		OnConnect(pSession->GetIndex());
	}

	return 0;
}

bool IOCPServer::CreateWorkerThread() {
	DWORD threadID = 0;
	for (int i = 0; i < MAX_THREAD_CNT; i++) {
		workerThreads.emplace_back([this]() {WorkerThreadFunc(); });
	}

	return true;
}

bool IOCPServer::CreateAcceptThread() {
	DWORD threadID = 0;

	acceptThread = std::thread([this]() {AcceptThreadFunc(); });

	return true;
}

void IOCPServer::DestroyThread() {
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
