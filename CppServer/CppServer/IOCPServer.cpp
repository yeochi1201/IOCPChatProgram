#include "IOCPServer.h"
#include "Define.h"

#pragma region Handler
bool IOCPServer::InitIOCPHandler() {
	IOCP_Handler = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, MAX_THREAD_CNT);
	if (IOCP_Handler == NULL) {
		puts("ERROR : Cant Create IOCP");
		return FALSE;
	}
	auto IOCPHandle = CreateIoCompletionPort((HANDLE)listenSocket, IOCP_Handler, (UINT32)0, 0);
	if (IOCPHandle == NULL) {
		puts("ERROR : Cant Create listen IOCP");
		return FALSE;
	}
	return TRUE;
}
#pragma endregion

#pragma region SessionFunc
void IOCPServer::CreateSessions(UINT16 maxClient) {
	std::lock_guard<std::mutex>guard(sessionLock);
	for (int i = 0; i < maxClient; i++) {
		Session* session = new Session;
		session->Init(i, IOCP_Handler);
		Sessions.push_back(session);
	}
}

Session* IOCPServer::GetEmptySession() {
	std::lock_guard<std::mutex>guard(sessionLock);
	for (int i = 0; i < Sessions.size(); i++) {
		if (!Sessions[i]->IsConnect())
			return Sessions[i];
	}
	return nullptr;
}

Session* IOCPServer::GetSession(UINT32 index) {
	std::lock_guard<std::mutex>guard(sessionLock);
	return Sessions[index];
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
	InitIOCPHandler();
	CreateSessions(maxClient);
	CreateAcceptThread();
	CreateWorkerThread();

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
	std::lock_guard<std::mutex>guard(sessionLock);
	for (int i = 0; i < Sessions.size(); i++) {
		CloseClient(Sessions[i]);
	}
}

bool IOCPServer::CloseServer() {
	CloseAllClient();

	DestroyThread();

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

		OverlappedEx* overlappedEx = (OverlappedEx*)pWol;
		if (result == FALSE || (transferSize == 0 && overlappedEx->operation != IOOperation::ACCEPT)) {
			puts(" ClientSocket Disconnect or Server Down");
			CloseClient(pSession);
			continue;
		}

		pSession = GetSession(overlappedEx->sessionIndex);
		switch (overlappedEx->operation) {
		case IOOperation::ACCEPT:
			if (pSession->AcceptComplete()) {
				OnConnect(pSession->GetIndex());
			}
			break;
		case IOOperation::RECV:
			OnRecv(pSession, pSession->GetRecvBuffer(), transferSize);
			pSession->BindRecv();
			break;
		case IOOperation::SEND:
			pSession->SendComplete(transferSize);
			break;
		default:
			break;
		}
	}
	return 0;
}

DWORD WINAPI IOCPServer::AcceptThreadFunc() {
	while (true) {
		auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		for (int i = 0; i<Sessions.size(); i++) {
			Session* session = Sessions[i];
			if (session->IsConnect()) continue;
			if ((UINT64)curTimeSec < session->GetLatestClosedTimeSec())continue;
			if (curTimeSec - session->GetLatestClosedTimeSec() <= RE_USE_SESSION_WAIT_TIMESEC) continue;
			session->Accept(listenSocket, curTimeSec);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(32));
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
