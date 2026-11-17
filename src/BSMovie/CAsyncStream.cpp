#include "BSMovie/CAsyncStream.h"

#include "BSCore/MemoryManager.h"

CAsyncStream::CAsyncStream() {}

CAsyncStream::~CAsyncStream() {}

void CAsyncStream::operator delete(void* apMemory) {
	ourFree(apMemory);
}
