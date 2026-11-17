#pragma once

#include <cstdint>

// The byte stream interface the movie player's DirectShow source filter (CAsyncIo) reads from; the DirectShow SDK's
// CAsyncStream, with the SDK's virtual function order: the destructor, Seek, Read, GetFileSize, Alignment, Lock and
// Unlock.
class CAsyncStream {
public:
	CAsyncStream();
	virtual ~CAsyncStream();

	// The SDK's SetPointer.
	virtual int32_t Seek(int64_t aiPosition) = 0;
	virtual int32_t Read(uint8_t* apBuffer, uint32_t auiLength, int32_t abAligned, uint32_t* apBytesRead) = 0;
	// Corresponds to the DirectShow Size(LONGLONG* pSizeAvailable); CBSAStream's implementation stores the 64-bit size
	// through apSizeAvailable and also returns it.
	virtual int64_t GetFileSize(int64_t* apSizeAvailable) = 0;
	// The read alignment, a power of two (CBSAStream returns 1).
	virtual uint32_t Alignment() = 0;
	virtual void Lock() = 0;
	virtual void Unlock() = 0;

	// The streams are freed through ourFree.
	static void operator delete(void* apMemory);
};
