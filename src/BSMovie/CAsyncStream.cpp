#include "BSMovie/CAsyncStream.h"

#include "BSCore/NewOverloads.h"

CAsyncStream::CAsyncStream() {}

CAsyncStream::~CAsyncStream() {}

void CAsyncStream::operator delete(void* apMemory) {
    ourFree(apMemory);
}
