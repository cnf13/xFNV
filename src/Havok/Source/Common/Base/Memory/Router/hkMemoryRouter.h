#pragma once

#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Reconstruction.hpp"

class hkMemoryAllocator;
struct hkpRootCdPoint;

// Frees a block from the thread's stack allocator; the PC build takes only the pointer.
template <typename T> void hkDeallocateStack(T* ptr);
// Takes n elements of T from the thread's stack allocator, the byte count rounded up to a multiple of 128; what names
// the use.
template <typename T> T* hkAllocateStack(int32_t n, const char* what);
// hkAllocateStack<hkpRootCdPoint>: a1 elements' bytes (rounded up
// to 128) from the thread's stack allocator.
// Inferred type: a2: char* from 0x9b2e6f (passed to hkStackMemory::allocateStack 1)
void* hkAllocateStackRootCdPoints(int32_t a1, char* a2);

// The calling thread's set of allocators. Only the members the reconstructed code reads are declared.
class hkMemoryRouter {
public:
	// The 16 bytes easyAlloc and alignedAlloc put in front of the block they return, which easyFree and alignedFree
	// read back. The exe's offsets from the returned pointer: -0x10 the allocator, -0xC the byte count passed to the
	// allocator less the header's 16, -8 (alignedAlloc only) the distance back to the allocator's block.
	struct BlockHeader {
		hkMemoryAllocator* m_allocator;
		int32_t m_numBytes;
		int32_t m_offset;
		// Never written or read.
		int32_t unk0C;
	};

	hkMemoryRouter();
	~hkMemoryRouter();

	// Clears the peak figures the router's allocators keep.
	void resetPeakMemoryStatistics();
	// The temp allocator: short-lived blocks.
	hkMemoryAllocator& temp() { return *m_temp; }
	void setTemp(hkMemoryAllocator* a) { m_temp = a; }
	// The heap allocator: long-lived blocks.
	hkMemoryAllocator& heap() { return *m_heap; }
	void setHeap(hkMemoryAllocator* a) { m_heap = a; }
	hkMemoryAllocator& debug() { return *m_debug; }
	void setDebug(hkMemoryAllocator* a) { m_debug = a; }
	hkMemoryAllocator& solver() { return *m_solver; }
	void setSolver(hkMemoryAllocator* a) { m_solver = a; }
	hkMemoryAllocator& global() { return *m_global; }
	void setGlobal(hkMemoryAllocator* a) { m_global = a; }
	void* getUserData() const { return m_userData; }
	void setUserData(void* p) { m_userData = p; }
	// Name not in the PDB (the field at +0x10); folded accessor, IDA: TESMain::GetThread.
	void* GetHeap();

	// The router of the calling thread.
	static hkMemoryRouter& getInstance();
	// Makes apInstance the calling thread's router. The eax it leaves (TlsSetValue's BOOL) is read by no caller.
	static void replaceInstance(hkMemoryRouter* a);
	// The calling thread's router, or null.
	static hkMemoryRouter* getInstancePtr();
	// Frees a block apPtr that easyAlloc made from apAllocator, reading its size from the block
	// header (cdecl, ret 0, 0xC856C0). IDA types the function int and the exe's eax is the allocator's result.
	static void easyFree(hkMemoryAllocator& b, void* p);
	// Allocates aiNumBytes from apAllocator, aligned to aiAlign (a power of two), with a BlockHeader in front of the
	// returned pointer.
	static void* alignedAlloc(hkMemoryAllocator& b, int32_t nbytes, int32_t alignment);
	// Frees a block alignedAlloc made (cdecl, ret 0, 0xC85660). The exe leaves apPtr in eax, which no caller reads.
	static void alignedFree(hkMemoryAllocator& b, void* p);
	// Allocates aiNumBytes from apAllocator, with a BlockHeader in front of the returned pointer.
	static void* easyAlloc(hkMemoryAllocator& b, int32_t nbytes);

	hkStackMemory m_stack;
	hkMemoryAllocator* m_temp;
	hkMemoryAllocator* m_heap;
	hkMemoryAllocator* m_debug;
	hkMemoryAllocator* m_solver;
	hkMemoryAllocator* m_global;
	void* m_userData;

	// The SDK's name (HK_THREAD_LOCAL); not in the PDB. The TLS slot holding each thread's router.
	static uint32_t s_memoryRouter;
};

// The SDK's inline bodies (0x5E0AA0, 0x6E7620; 0x5E0B60): the thread's router's stack allocator.
template <typename T> T* hkAllocateStack(int32_t n, const char* what) {
	const int32_t numBytes = (n * static_cast<int32_t>(sizeof(T)) + 127) & ~127;
	return static_cast<T*>(hkMemoryRouter::getInstance().m_stack.allocateStack(numBytes, what));
}

template <typename T> void hkDeallocateStack(T* ptr) {
	hkMemoryRouter::getInstance().m_stack.deallocateStack(ptr);
}
