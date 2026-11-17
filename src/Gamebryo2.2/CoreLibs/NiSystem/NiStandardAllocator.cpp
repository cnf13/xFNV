#include "Gamebryo2.2/CoreLibs/NiSystem/NiStandardAllocator.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSmallObjectAllocator.h"

// A 4-aligned block whose size is not given back on deallocation gets four more bytes in front, holding its size
// byte by byte (read back by GetSizeFromAddress). Up to 256 bytes go to the small object allocator.
void* NiStandardAllocator::Allocate(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType,
	int32_t bProvideAccurateSizeOnDeallocate, const char* pcFile, int32_t iLine, const char* pcFunction) {
	bool bPrependSize = false;
	if (stAlignment == 4 && !static_cast<uint8_t>(bProvideAccurateSizeOnDeallocate) && eEventType != NI_ALIGNEDMALLOC &&
		eEventType != NI_ALIGNEDREALLOC) {
		stSizeInBytes += 4;
		bPrependSize = true;
	}
	void* pvMemory;
	if (eEventType != NI_ALIGNEDMALLOC && eEventType != NI_ALIGNEDREALLOC && stSizeInBytes <= 256 && stAlignment == 4) {
		pvMemory = NiStandardAllocator::ms_pkSmallAlloc->Allocate(stSizeInBytes);
	} else {
		pvMemory = _NiExternalAlignedMalloc(stSizeInBytes, stAlignment);
	}
	if (pvMemory && bPrependSize) {
		uint8_t* pucMemory = static_cast<uint8_t*>(pvMemory);
		uint32_t uiSize = static_cast<uint32_t>(stSizeInBytes);
		pucMemory[0] = static_cast<uint8_t>(uiSize);
		pucMemory[1] = static_cast<uint8_t>(uiSize >> 8);
		pucMemory[2] = static_cast<uint8_t>(uiSize >> 16);
		pucMemory[3] = static_cast<uint8_t>(uiSize >> 24);
		return pucMemory + 4;
	}
	return pvMemory;
}

void NiStandardAllocator::Deallocate(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) {
	char* v4 = static_cast<char*>(pvMemory);
	if (pvMemory) {
		uint32_t SizeFromAddress;
		if (eEventType == NI_ALIGNEDFREE || eEventType == NI_ALIGNEDREALLOC) {
			SizeFromAddress = stSizeInBytes;
		} else {
			SizeFromAddress = stSizeInBytes;
			if (stSizeInBytes == -1) {
				v4 = static_cast<char*>(pvMemory) - 4;
				SizeFromAddress = NiStandardAllocator::GetSizeFromAddress(static_cast<char*>(pvMemory) - 4);
			}
		}
		if (SizeFromAddress > 256) {
			_NiExternalAlignedFree(v4);
		} else {
			NiStandardAllocator::ms_pkSmallAlloc->Deallocate(v4, SizeFromAddress);
		}
	}
}

// A block with its size in front (see Allocate) that still fits stays where it is; otherwise a small block moves
// through Allocate, a copy and Deallocate, and a large or aligned one is reallocated externally.
void* NiStandardAllocator::Reallocate(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment,
	NiMemEventType eEventType, int32_t bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcFile,
	int32_t iLine, const char* pcFunction) {
	uint8_t* pucMemory = static_cast<uint8_t*>(pvMemory);
	size_t stCurrent = stSizeCurrent;
	if (eEventType != NI_ALIGNEDFREE && eEventType != NI_ALIGNEDREALLOC && stSizeCurrent == static_cast<size_t>(-1) &&
		!static_cast<uint8_t>(bProvideAccurateSizeOnDeallocate)) {
		pucMemory -= 4;
		stCurrent = NiStandardAllocator::GetSizeFromAddress(pucMemory);
	}
	bool bPrependSize = false;
	if (stAlignment == 4 && !static_cast<uint8_t>(bProvideAccurateSizeOnDeallocate) && eEventType != NI_ALIGNEDMALLOC &&
		eEventType != NI_ALIGNEDREALLOC) {
		stSizeInBytes += 4;
		bPrependSize = true;
	}
	if (stCurrent != static_cast<size_t>(-1) && stSizeInBytes <= stCurrent && bPrependSize) {
		stSizeInBytes = stCurrent;
		return pucMemory + 4;
	}
	void* pvNewMemory;
	if (eEventType == NI_ALIGNEDMALLOC || eEventType == NI_ALIGNEDREALLOC || (stCurrent > 256 && stSizeInBytes > 256)) {
		pvNewMemory = _NiExternalAlignedRealloc(pucMemory, stSizeInBytes, stAlignment);
	} else {
		stSizeInBytes -= 4;
		pvNewMemory = Allocate(stSizeInBytes, stAlignment, eEventType, bProvideAccurateSizeOnDeallocate, pcFile, iLine,
			pcFunction);
		bPrependSize = false;
		if (stCurrent != static_cast<size_t>(-1)) {
			rt::memcpy(pvNewMemory, pucMemory + 4, stCurrent - 4);
			Deallocate(pucMemory, eEventType, stCurrent);
		}
	}
	if (pvNewMemory && bPrependSize) {
		uint8_t* pucNewMemory = static_cast<uint8_t*>(pvNewMemory);
		uint32_t uiSize = static_cast<uint32_t>(stSizeInBytes);
		pucNewMemory[0] = static_cast<uint8_t>(uiSize);
		pucNewMemory[1] = static_cast<uint8_t>(uiSize >> 8);
		pucNewMemory[2] = static_cast<uint8_t>(uiSize >> 16);
		pucNewMemory[3] = static_cast<uint8_t>(uiSize >> 24);
		return pucNewMemory + 4;
	}
	return pvNewMemory;
}

// An aligned external block is over-allocated by the alignment's low byte and keeps its offset from the real block
// in the byte before it. Every block is cleared.
void* NiStandardAllocator::AllocateExternal(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType,
	bool bProvideAccurateSizeOnDeallocate, const char* pcFile, int32_t iLine, const char* pcFunction) {
	uint8_t* pucMemory;
	if (eEventType == NI_EXTERNAL_ALIGNEDMALLOC) {
		uint8_t ucAlignment = static_cast<uint8_t>(stAlignment);
		uint8_t* pucBlock = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(stSizeInBytes + ucAlignment));
		// The offset comes from the raw block address's low byte.
		uint8_t ucOffset = static_cast<uint8_t>(ucAlignment -
			(static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pucBlock)) & (ucAlignment - 1)));
		pucMemory = pucBlock + ucOffset;
		pucMemory[-1] = ucOffset;
	} else {
		pucMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(stSizeInBytes));
	}
	return std::memset(pucMemory, 0, stSizeInBytes);
}

// An aligned external block keeps its offset from the real block in the byte before it.
void NiStandardAllocator::DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) {
	if (pvMemory) {
		uint8_t* pucMemory = static_cast<uint8_t*>(pvMemory);
		if (eEventType == NI_EXTERNAL_ALIGNEDFREE) {
			pucMemory -= pucMemory[-1];
		}
		MemoryManager_Singleton.Deallocate(pucMemory);
	}
}

// The aligned form reallocates the real block (the offset byte before pvMemory steps back to it) and aligns the result
// again; a null aligned block is not allocated (null is cleared). Every block is cleared.
void* NiStandardAllocator::ReallocateExternal(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment,
	NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcFile,
	int32_t iLine, const char* pcFunction) {
	if (eEventType != NI_EXTERNAL_ALGINEDREALLOC) {
		void* pvNewMemory = MemoryManager_Singleton.Reallocate(pvMemory, stSizeInBytes);
		return std::memset(pvNewMemory, 0, stSizeInBytes);
	}
	uint8_t ucAlignment = static_cast<uint8_t>(stAlignment);
	if (!pvMemory) {
		return std::memset(nullptr, 0, stSizeInBytes);
	}
	uint8_t* pucMemory = static_cast<uint8_t*>(pvMemory);
	uint8_t* pucBlock = static_cast<uint8_t*>(MemoryManager_Singleton.Reallocate(pucMemory - pucMemory[-1],
		stSizeInBytes + ucAlignment));
	// The offset comes from the raw block address's low byte.
	uint8_t ucOffset = static_cast<uint8_t>(ucAlignment -
		(static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pucBlock)) & (ucAlignment - 1)));
	uint8_t* pucNewMemory = pucBlock + ucOffset;
	pucNewMemory[-1] = ucOffset;
	return std::memset(pucNewMemory, 0, stSizeInBytes);
}

void NiStandardAllocator::Initialize() {
	void* v0 = (&MemoryManager_Singleton)->Allocate(sizeof(NiSmallObjectAllocator));
	if (v0) {
		NiStandardAllocator::ms_pkSmallAlloc = ::new (v0) NiSmallObjectAllocator(0x6400u);
	} else {
		NiStandardAllocator::ms_pkSmallAlloc = nullptr;
	}
}

// Destroys the pool's fixed allocators last to first, then frees the small object allocator.
void NiStandardAllocator::Shutdown() {
	NiSmallObjectAllocator* pkSmallAlloc = NiStandardAllocator::ms_pkSmallAlloc;
	if (pkSmallAlloc) {
		for (int32_t i = 255; i >= 0; --i) {
			pkSmallAlloc->m_kPool[i].Shutdown();
		}
		(&MemoryManager_Singleton)->Deallocate(pkSmallAlloc);
	}
	NiStandardAllocator::ms_pkSmallAlloc = nullptr;
}

// Every address passes.
bool NiStandardAllocator::VerifyAddress(const void* pvMemory) {
	return true;
}

uint32_t NiStandardAllocator::GetSizeFromAddress(void* pMemory) {
	const uint8_t* pucMemory = static_cast<const uint8_t*>(pMemory);
	uint32_t uiSize = static_cast<uint32_t>(pucMemory[0]) | (static_cast<uint32_t>(pucMemory[1]) << 8) |
		(static_cast<uint32_t>(pucMemory[2]) << 16) | (static_cast<uint32_t>(pucMemory[3]) << 24);
	return uiSize;
}
