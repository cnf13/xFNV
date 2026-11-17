#pragma once

#include "BSCore/BSTFreeList.h"
#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

// A free list that owns its N elements inline (the PDB's BSTStaticFreeList<T, N>).
template <class T, uint32_t uiSize> class BSTStaticFreeList : public BSTFreeList<T> {
public:
	BSTStaticFreeList() { this->Init(uiSize, Elems); }
	~BSTStaticFreeList() override {}

	BSTFreeListElem<T> Elems[uiSize];
};
inline constexpr fnvd::Vtable BSTStaticFreeList_PathingMessage_256_vftable{};
inline constexpr fnvd::Vtable BSTStaticFreeList_ActorPathingMessage_256_vftable{};
inline constexpr fnvd::Vtable BSTStaticFreeList_MobileObjectMessage_4096_vftable{};
