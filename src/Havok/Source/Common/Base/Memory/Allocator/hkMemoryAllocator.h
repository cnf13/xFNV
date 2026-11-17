#pragma once

#include <cstdint>

struct hkpRootCdPoint;

void HK_ASSERT_OBJECT_SIZE_OK_FUNC(uint32_t nbytes);

// sizeof(T), or 1 for void: the element size of hkMemoryAllocator's typed helpers.
template <typename T> struct hkSizeOfTypeOrVoid {
	enum {
		val = sizeof(T)
	};
};
template <> struct hkSizeOfTypeOrVoid<void> {
	enum {
		val = 1
	};
};

// The allocator interface the Havok memory system hands blocks and buffers out through.
class hkMemoryAllocator {
public:
	// An allocator's memory figures, in bytes.
	struct MemoryStatistics {
		uint32_t m_allocated;
		uint32_t m_inUse;
		uint32_t m_peakInUse;
		uint32_t m_available;
		uint32_t m_totalAvailable;
		uint32_t m_largestBlock;
	};

	enum MemoryState {
		MEMORY_STATE_OK = 0,
		MEMORY_STATE_OUT_OF_MEMORY = 1,
	};

	virtual ~hkMemoryAllocator();

	virtual void* blockAlloc(int32_t aiNumBytes) = 0;
	virtual void blockFree(void* apMem, int32_t aiNumBytes) = 0;
	// forwards to blockAlloc
	virtual void* bufAlloc(int32_t& reqBytesInOut);
	// forwards to blockFree
	virtual void bufFree(void* p, int32_t numBytes);
	virtual void* bufRealloc(void* pold, int32_t oldNumBytes, int32_t& reqBytesInOut);
	virtual void blockAllocBatch(void** ptrsOut, int32_t numPtrs, int32_t blockSize);
	virtual void blockFreeBatch(void** ptrsIn, int32_t numPtrs, int32_t blockSize);
	virtual bool canAllocSingleBlock(int32_t numBytes);
	virtual bool canAllocTotal(int32_t) { return true; }
	virtual void getMemoryStatistics(MemoryStatistics& apStats) = 0;
	virtual int32_t getAllocatedSize(const void* apObj, int32_t aiNumBytes) = 0;
	virtual void resetPeakMemoryStatistics() {}

	// Typed helpers: the element count is scaled to bytes and the result cast.
	template <typename TYPE> TYPE* _blockAlloc(int32_t n) {
		return static_cast<TYPE*>(blockAlloc(n * hkSizeOfTypeOrVoid<TYPE>::val));
	}

	template <typename TYPE> void _blockFree(TYPE* p, int32_t n) { blockFree(p, n * hkSizeOfTypeOrVoid<TYPE>::val); }

	template <typename TYPE> TYPE* _bufAlloc(int32_t& reqNumInOut) {
		int32_t n = reqNumInOut * hkSizeOfTypeOrVoid<TYPE>::val;
		void* p = bufAlloc(n);
		reqNumInOut = n / hkSizeOfTypeOrVoid<TYPE>::val;
		return static_cast<TYPE*>(p);
	}

	template <typename TYPE> void _bufFree(void* p, int32_t numElem) {
		bufFree(p, numElem * hkSizeOfTypeOrVoid<TYPE>::val);
	}

	template <typename TYPE> TYPE* _bufRealloc(void* pold, int32_t oldNum, int32_t& reqNumInOut) {
		int32_t n = reqNumInOut * hkSizeOfTypeOrVoid<TYPE>::val;
		void* p = bufRealloc(pold, oldNum * hkSizeOfTypeOrVoid<TYPE>::val, n);
		reqNumInOut = n / hkSizeOfTypeOrVoid<TYPE>::val;
		return static_cast<TYPE*>(p);
	}

	void bufFreeRayCastOutputs(void* p, int32_t numElem);
	void bufFreeArray(void* p, int32_t numElem);
	void bufFreePointers(void* p, int32_t numElem);
	void freeHandlePairBuffer(void* p, int32_t numElem);
};

// The memory system's out-of-memory state (hkOutOfMemoryState): a hkMemoryAllocator::MemoryState, as an int.
inline int32_t hkOutOfMemoryState;
