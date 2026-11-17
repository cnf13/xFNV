#pragma once

#include "Reconstruction.hpp"

// What a heap request came from; passed through to the allocator.
enum NiMemEventType : uint32_t {
	NI_UNKNOWN = 0,
	NI_OPER_NEW = 1,
	NI_OPER_NEW_ARRAY = 2,
	NI_OPER_DELETE = 3,
	NI_OPER_DELETE_ARRAY = 4,
	NI_MALLOC = 5,
	NI_REALLOC = 6,
	NI_ALIGNEDMALLOC = 7,
	NI_ALIGNEDREALLOC = 8,
	NI_FREE = 9,
	NI_ALIGNEDFREE = 10,
	NI_EXTERNAL_MALLOC = 11,
	NI_EXTERNAL_REALLOC = 12,
	NI_EXTERNAL_ALIGNEDMALLOC = 13,
	NI_EXTERNAL_ALGINEDREALLOC = 14,
	NI_EXTERNAL_FREE = 15,
	NI_EXTERNAL_ALIGNEDFREE = 16,
};

// Interface of the engine heap behind NiMalloc/NiFree and NiMemObject's new/delete. Sizes and alignments are in-out:
// the allocator may round them up.
class NiAllocator {
public:
	virtual ~NiAllocator();

	// bProvideAccurateSizeOnDeallocate: the exe passes the caller's whole dword on; bool would be zero-extended
	// (NiStandardAllocator::Reallocate pushes its own slot as is; NiStandardAllocator::Allocate compares the low
	// byte). PDB: bool.
	virtual void* Allocate(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType,
		int32_t bProvideAccurateSizeOnDeallocate, const char* pcFile, int32_t iLine, const char* pcFunction) = 0;
	virtual void Deallocate(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) = 0;
	// bProvideAccurateSizeOnDeallocate: the exe passes the caller's whole dword on; bool would be zero-extended
	// (NiStandardAllocator::Reallocate pushes its own slot as is to Allocate). PDB: bool.
	virtual void* Reallocate(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType,
		int32_t bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcFile, int32_t iLine,
		const char* pcFunction) = 0;
	virtual void* AllocateExternal(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType,
		bool bProvideAccurateSizeOnDeallocate, const char* pcFile, int32_t iLine, const char* pcFunction) = 0;
	virtual void DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) = 0;
	virtual void* ReallocateExternal(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment,
		NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcFile,
		int32_t iLine, const char* pcFunction) = 0;
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual bool VerifyAddress(const void* pvMemory) = 0;

	// Frees through MemoryManager::Deallocate, as the exe's deleting destructor does.
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable NiAllocator_vftable{};
