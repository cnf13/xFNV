#pragma once

#include "BSCore/BSSpinLock.h"
#include "Reconstruction.hpp"

// A per-thread stack allocator for short-lived scratch memory. Only the members the reconstructed code calls are
// declared.
class ScrapHeap {
public:
	// The block header written before each allocation.
	struct Header;
	// A free block.
	struct FreeBlock;

	ScrapHeap();
	ScrapHeap(uint32_t a1);
	~ScrapHeap();

	void* Allocate(uint32_t aSize, uint32_t aAlignment);
	void Deallocate(void* apMem);
	void Init(uint32_t aiInitialSize);
	void* Allocate(uint32_t aiSize, const char* pcFile, int32_t aiLine, uint32_t aAlignment);
	// The size word of the block header before apMem, its flag bits masked off.
	uint32_t Size(void* apMem);
	void Clean(bool abCheck);
	void CheckForLeaks();
	uint32_t GetAllocationCount();

protected:
	// The PC layout, from ScrapHeap::Init (0xAA57B0): the buffer, the next free byte, the buffer end and the last
	// block. The Xbox PDB's iCurrentAllocSize and iThreadLock are not in the PC class.
	char* pMemHeap;

public:
	char* pCurrentStackLoc;

protected:
	char* pEndHeap;
	Header* pLastBlock;

public:
	// The default alignment; the exe reads it from memory at each call site.
	static inline uint32_t ALIGNMENT = 4;
};

// Allocates nbytes from arScrapHeap at ScrapHeap::ALIGNMENT; defined in ScrapHeap.cpp.
void* operator new(size_t nbytes, ScrapHeap& arScrapHeap);

// The block header (IDA's ScrapHeap::Block): the size, its top bit set once the block is freed (Deallocate 0xAA5610),
// then the previous block. The PDB's iMagic, iSize and pPrev are three words on the Xbox; the PC header is two.
struct ScrapHeap::Header {
	uint32_t uiSizeAndFlags;
	// IDA name; not in the PDB (the PDB's pPrev, a Header*).
	Header* pPrevious;
};

// A free block (IDA's ScrapHeap::FreeBlock; the PDB has none): the header and the links to the free blocks to its
// left and right.
struct ScrapHeap::FreeBlock : ScrapHeap::Header {
	FreeBlock* pLeft;
	FreeBlock* pRight;
};

// Owns the memory buffers the scrap heaps carve from: each buffer's base and committed size, and the lock that guards
// the list.
class ScrapHeapManager {
public:
	struct Buffer {
		char* pBuffer;
		uint32_t uiCommittedSize;
	};

	ScrapHeapManager();
	~ScrapHeapManager();

	// A buffer of at least arSize bytes; arSize receives the size handed out.
	char* RequestBuffer(uint32_t& ariRequestedInitialSize);
	// Releases the committed buffer apBuffer (thiscall, ret 8, 0xAA5F30). VirtualFree's BOOL is
	// left in eax, and no caller reads it.
	void FreeBuffer(char* apBuffer, uint32_t aiCommittedSize);
	// Keeps apBuffer for reuse while there is room in kBuffers; a full list frees it at once.
	void ReleaseBuffer(char* apBuffer, uint32_t aiCommittedSize);
	// Frees every kept buffer.
	void FreeAllBuffers();
	// Frees kept buffers, last first, until at least aiSize bytes are freed or none is left; returns the bytes freed.
	uint32_t FreeBuffers(uint32_t aiSize);
	void ShrinkBuffer(char* apBuffer, uint32_t aiBufferSize, uint32_t aiShrinkSize);
	char* AllocateBuffer(uint32_t aiInitialSize);
	void ExpandBuffer(char* apBuffer, uint32_t aiBufferSize, uint32_t aiAdditionalSize);
	void RemoveBuffer(uint32_t aiIndex);

	// The singleton.
	static ScrapHeapManager* GetSingleton();
	static void Init();
	static void Shutdown();

	// IDA name; not in the PDB (the PDB spells it Buffers, an array of ScrapHeapBuffer).
	Buffer Buffers[64];
	uint32_t iBufferCount;
	uint32_t iBufferTotalSize;
	// IDA: DWORD[6] gap208.
	uint8_t padding208[24];
	BSSpinLock BufferLock;

	static inline ScrapHeapManager* pInstance;
};
