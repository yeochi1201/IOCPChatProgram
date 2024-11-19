#pragma once
#include "IOCPServer.h"
#include "Packet.h"
#include "Session.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>

class ChatServer : public IOCPServer {
public:
	virtual bool OnConnect(UINT32 index) {
		printf("[Connect] %d Client\n", index);
		return true;
	}
	virtual bool OnDisconnect(UINT32 index) {
		printf("[Disconnect] %d Client\n", index);
		return true;
	}
	virtual bool OnSend(Session* session, char* buf, DWORD transfersize) {
		return true;
	}
	virtual bool OnRecv(Session* session, char* buf, DWORD transfersize) {
		printf("[Receive] clientIdx : %d, dataSize : %d", session->GetIndex(), transfersize);
		return true;
	}

	void Run(UINT portNum, UINT16 maxClient);
	void End();
private:
	std::thread processThread;
	std::mutex lock;
	std::deque<Packet> packetDeque;

	void ProcessPacket();
	Packet DequePacket();
};