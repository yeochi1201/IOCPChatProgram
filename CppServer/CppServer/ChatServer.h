#pragma once
#include "IOCPServer.h"
#include "Packet.h"
#include "Session.h"

#include <deque>
#include <thread>
#include <mutex>

class ChatServer : public IOCPServer {
public:
	virtual bool OnConnect(UINT32 index) override {
		printf("[Connect] %d Client\n", index);
		return true;
	}
	virtual bool OnDisconnect(UINT32 index) override {
		printf("[Disconnect] %d Client\n", index);
		return true;
	}
	virtual bool OnSend(Session* session, char* buf, DWORD transfersize) override {
		return true;
	}
	virtual bool OnRecv(Session* session, char* buf, DWORD transfersize) override {
		printf("[Receive] clientIdx : %d, dataSize : %d", session->GetIndex(), transfersize);

		Packet packet;
		packet.Set(session->GetIndex(), transfersize, buf);

		std::lock_guard<std::mutex> guard(lock);
		packetDeque.push_back(packet);
		return true;
	}

	void Run(UINT16 portNum, UINT16 maxClient);
	void End();
private:
	std::thread processThread;
	std::mutex lock;
	std::deque<Packet> packetDeque;

	void ProcessPacket();
	Packet DequePacket();
};