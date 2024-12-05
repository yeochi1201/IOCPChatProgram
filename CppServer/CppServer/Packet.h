#pragma once

#include <windows.h>

class Packet {
public:
	UINT32 GetIdx();
	UINT32 GetDataSize();
	char* GetData();

	void Set(Packet& packet);
	void Set(UINT32 sessionIdx, UINT32 dataSize, char* data);
	void Release();
private:
	UINT32 sessionIdx = 0;
	UINT32 dataSize = 0;
	char* data = nullptr;
};