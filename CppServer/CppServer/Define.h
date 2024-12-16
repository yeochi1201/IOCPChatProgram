#pragma once

#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <SDKDDKVer.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SOCK_BUF_SIZE 8192
#define MAX_THREAD_CNT	8
#define RE_USE_SESSION_WAIT_TIMESEC 3

enum class IOOperation {
	RECV,
	SEND,
	ACCEPT
};

typedef struct OverlappedEx {
	WSAOVERLAPPED wsaOverlapped;
	UINT32 sessionIndex;
	WSABUF wsaBuf;
	IOOperation operation;
}OverlappedEx;

