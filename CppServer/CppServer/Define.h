#pragma once

#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <SDKDDKVer.h>
#pragma comment(lib, "ws2_32")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SOCK_BUF_SIZE 8192
#define MAX_THREAD_CNT	8

enum class IOOperation {
	RECV,
	SEND
};

typedef struct OverlappedEx {
	WSAOVERLAPPED wsaOverlapped;
	SOCKET clientSocket;
	WSABUF wsaBuf;
	IOOperation operation;
}OverlappedEx;

