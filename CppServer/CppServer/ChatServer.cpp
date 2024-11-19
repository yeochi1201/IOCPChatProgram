#include "ChatServer.h"

void ChatServer::Run(UINT portNum, UINT16 maxClient) {
	processThread = std::thread([this]() {ProcessPacket(); });

	InitSocket(portNum, maxClient);
}

void ChatServer::End() {
	if (processThread.joinable()) {
		processThread.join();
	}
	DestroyThread();
}

void ChatServer::ProcessPacket() {
	while (1) {
		Packet packet = DequePacket();
		if (packet.GetDataSize() != 0) {
			SendPacket(packet.GetIdx(), packet.GetData(), packet.GetDataSize());
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

Packet ChatServer::DequePacket() {
	Packet packet;
	std::lock_guard<std::mutex> guard(lock);
	if (packetDeque.empty())
		return packet;
	packet.Set(packetDeque.front());

	packetDeque.front().Release();
	packetDeque.pop_front();

	return packet;
}