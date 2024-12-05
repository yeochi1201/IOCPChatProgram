#include "Packet.h"

UINT32 Packet::GetIdx() {
	return sessionIdx;
}

UINT32 Packet::GetDataSize() {
	return dataSize;
}

char* Packet::GetData() {
	return data;
}

void Packet::Set(Packet& packet) {
	this->sessionIdx = packet.GetIdx();
	this->dataSize = packet.GetDataSize();
	this->data = new char[dataSize];
	CopyMemory(data, packet.GetData(), dataSize);
}

void Packet::Set(UINT32 sessionIdx, UINT32 dataSize, char* data) {
	this->sessionIdx = sessionIdx;
	this->dataSize = dataSize;
	this->data = new char[dataSize];
	CopyMemory(this->data, data, dataSize);
}

void Packet::Release() {
	delete data;
}