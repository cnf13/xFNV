#pragma once

#include "Reconstruction.hpp"

// IDA name; not in the PDB. The byte stream interface the movie player's DirectShow source filter (CAsyncIo) reads
// from; the DirectShow SDK's CAsyncStream, with the SDK's slot order. Slots: the destructor, Seek (ret 8), Read
// (ret 0x10), GetFileSize (ret 4), Alignment, Lock and Unlock.
class CAsyncStream {
public:
	CAsyncStream();
	virtual ~CAsyncStream();

	// The SDK's SetPointer.
	virtual int32_t Seek(int64_t aiPosition) = 0;
	virtual int32_t Read(uint8_t* apBuffer, uint32_t auiLength, int32_t abAligned, uint32_t* apBytesRead) = 0;
	// The SDK's Size(LONGLONG* pSizeAvailable): Length (0x9FE090) passes its int64_t* on, CBSAStream's body
	// (0x82F570) stores two dwords through it and returns edx:eax.
	virtual int64_t GetFileSize(int64_t* apSizeAvailable) = 0;
	// Not named in the PDB or IDA; name chosen here (the SDK's). The read alignment, a power of two (CBSAStream's
	// body, 0x978f30, returns 1 and takes no argument).
	virtual uint32_t Alignment() = 0;
	virtual void Lock() = 0;
	virtual void Unlock() = 0;

	// The exe frees the streams through ourFree (DeletingDestruct, 0x82F160).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable CAsyncStream_vftable{};
