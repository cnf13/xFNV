#pragma once

#include "BSCore/BSTFreeList.h"
#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/ehvec.h"

// A free list that owns its N elements inline.
template <class T, uint32_t uiSize>
class BSTStaticFreeList : public BSTFreeList<T> {
public:
    BSTStaticFreeList() {
        this->Init(uiSize, Elems);
    }
    ~BSTStaticFreeList() override {}

    BSTFreeListElem<T> Elems[uiSize];
};
