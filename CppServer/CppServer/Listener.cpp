#include "Listener.h"

SOCKET listenSocket;

Listener* pListener;

Listener::Listener() {
	pListener = this;
	StartProgram();
}

bool Listener::CloseSocketHandler(DWORD dwType) {
	if (dwType == CTRL_C_EVENT) {

		CloseServer();
		puts("Socket Shutdown complete");

		::WSACleanup();
		exit(0);
		return TRUE;
	}
	return FALSE;
}

bool WINAPI StaticCloseSocketHandler(DWORD dwType) {
	return (pListener->CloseSocketHandler(dwType));
}

bool Listener::InitCtrlHandler() {
	if (::SetConsoleCtrlHandler((PHANDLER_ROUTINE)(StaticCloseSocketHandler), TRUE) == FALSE) {
		puts("ERROR : Ctrl Handler Setting Failed");
	}
	return TRUE;
}

bool Listener::InitIOCPHandler() {
	IOCP_Handler = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (IOCP_Handler == NULL) {
		puts("ERROR : Cant Create IOCP");
		return FALSE;
	}
	return TRUE;
}

void Listener::SendChattingMessage(char* pszParam, SOCKET clientSocket) {
	int msgLength = strlen(pszParam);
	std::list<SOCKET>::iterator it;

	::EnterCriticalSection(&socket_cs);
	for (it = socket_list.begin(); it != socket_list.end(); ++it) {
		if (*it != clientSocket && *it != listenSocket) {
			::send(*it, pszParam, sizeof(char) * (msgLength + 1), 0);
		}
	}
	::LeaveCriticalSection(&socket_cs);
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
	InitCtrlHandler();
	listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (listenSocket == INVALID_SOCKET) {
		puts("ERROR : Failed to Create Listen Socket");
		return false;
	}
	puts("Create Listener Socket");
	return true;
}

bool Listener::BindPort() {
	SOCKADDR_IN serverAddress = { 0 };
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(25000);
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

void Listener::CloseClient(SOCKET clientSocket) {
	::shutdown(clientSocket, SD_BOTH);
	::closesocket(clientSocket);
	::EnterCriticalSection(&socket_cs);
	socket_list.remove(clientSocket);
	::LeaveCriticalSection(&socket_cs);
}

void Listener::CloseAllClient() {
	std::list<SOCKET>::iterator it;
	::EnterCriticalSection(&socket_cs);
	for (it = socket_list.begin(); it != socket_list.end(); it++) {
		::shutdown(*it, SD_BOTH);
		::closesocket(*it);
	}
	::LeaveCriticalSection(&socket_cs);
}

bool Listener::CloseServer() {
	CloseAllClient();

	::shutdown(listenSocket, SD_BOTH);
	::closesocket(listenSocket);
	listenSocket = NULL;

	::CloseHandle(IOCP_Handler);
	IOCP_Handler = NULL;

	::DeleteCriticalSection(&socket_cs);
	return TRUE;
}

DWORD WINAPI Listener::CompleteThreadFunc(LPVOID param) {
	DWORD transferSize = 0;
	DWORD flag = 0;
	Session* pSession = NULL;
	LPWSAOVERLAPPED pWol = NULL;
	BOOL result;

	while (1) {
		result = GetQueuedCompletionStatus(
			IOCP_Handler, &transferSize, (PULONG_PTR)&pSession, &pWol, INFINITE);

		if (result == TRUE) {
			if (transferSize == 0) {
				CloseClient(pSession->clientSocket);
				delete(pWol);
				delete(pSession);
			}
			else {
				SendChattingMessage(pSession->buffer, pSession->clientSocket);
				memset(pSession->buffer, 0, sizeof(pSession->buffer));

				DWORD receiveSize = 0;
				DWORD flag = 0;
				WSABUF wsaBuf = { 0 };
				wsaBuf.buf = pSession->buffer;
				wsaBuf.len = sizeof(pSession->buffer);

				::WSARecv(pSession->clientSocket, &wsaBuf, 1, &receiveSize, &flag, pWol, NULL);
				if (::WSAGetLastError() != WSA_IO_PENDING)
					puts("ERROR : WSA Recv() occurred");
			}
		}
		else {
			if (pWol == NULL) {
				puts("ERROR : IOCP Handle Close");
				break;
			}
			else {
				if (pSession != NULL) {
					CloseClient(pSession->clientSocket);
					delete pWol;
					delete pSession;

				}
				puts("ERROR : No Server Connection");
			}
		}
	}
	return 0;
}

DWORD WINAPI Listener::AcceptThreadFunc(LPVOID param) {
	LPWSAOVERLAPPED pWol = NULL;
	DWORD recvSize, flag;
	Session* pNewUser;
	int addrSize = sizeof(SOCKADDR);
	WSABUF wsaBuf;
	SOCKADDR clientAddr;
	SOCKET clientSocket;
	int recvResult = 0;

	while ((clientSocket = ::accept(listenSocket, &clientAddr, &addrSize))) {
		puts("New Client Connected");
		::EnterCriticalSection(&socket_cs);
		socket_list.push_back(clientSocket);
		::LeaveCriticalSection(&socket_cs);

		pNewUser = new Session;
		::ZeroMemory(pNewUser, sizeof(Session));
		pNewUser->clientSocket = clientSocket;

		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		::CreateIoCompletionPort((HANDLE)clientSocket, IOCP_Handler, (ULONG_PTR)pNewUser, 0);

		recvSize = 0;
		flag = 0;
		wsaBuf.buf = pNewUser->buffer;
		wsaBuf.len = sizeof(pNewUser->buffer);

		recvResult = ::WSARecv(clientSocket, &wsaBuf, 1, &recvSize, &flag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR : WSARecv not pending error");
	}

	return 0;
}

DWORD WINAPI CompleteThread(LPVOID param) {
	return (pListener->CompleteThreadFunc(param));
}
DWORD WINAPI AcceptThread(LPVOID param) {
	return (pListener->AcceptThreadFunc(param));
}

void Listener::StartProgram() {
	HANDLE thread;
	
	DWORD threadID;
	

	ResetWinsock();
	InitIOCPHandler();
	InitializeCriticalSection(&socket_cs);
	CreateSocket();
	BindPort();
	WaitingClient(listenSocket);

	for (int i = 0; i < MAX_THREAD_CNT; i++) {
		threadID = 0;

		thread = ::CreateThread(NULL, 0, CompleteThread, (LPVOID)NULL, 0, &threadID);

		::CloseHandle(thread);
	}

	thread = ::CreateThread(NULL, 0, AcceptThread, (LPVOID)NULL, 0, &threadID);
	::CloseHandle(thread);

	puts("Start Chat Server");
	while (1)
		getchar();
	return;
}
