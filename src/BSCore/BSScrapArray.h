#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class Actor;
class TESObjectCELL;

// A BSSimpleArray whose buffer comes from the scrap heap of the thread that built it; used for short-lived stack
// arrays.
template <class T, uint32_t uiGrowStep = 1024>
class BSScrapArray : public BSSimpleArray<T, uiGrowStep> {
public:
    BSScrapArray() : BSScrapArray(0, 0, nullptr) {}

    // Room for aiReserveSize items (at least aiInitialSize), the first aiInitialSize constructed, on apHeap or the
    // calling thread's scrap heap. The setup runs after the heap is set, so the buffer comes from it.
    BSScrapArray(uint32_t aiReserveSize, uint32_t aiInitialSize, ScrapHeap* apHeap)
      : pScrapHeap(apHeap ? apHeap : MemoryManager::Instance().GetThreadScrapHeap()) {
        if (aiInitialSize > aiReserveSize) {
            aiReserveSize = aiInitialSize;
        }
        if (aiReserveSize) {
            this->pBuffer = _Allocate(aiReserveSize);
            this->iReservedSize = aiReserveSize;
        }
        if (aiInitialSize) {
            this->_ConstructItems(this->pBuffer, aiInitialSize);
            this->iSize = aiInitialSize;
        }
    }

    explicit BSScrapArray(uint32_t aiInitialSize) : BSScrapArray(aiInitialSize, aiInitialSize, nullptr) {}
    // The buffer goes back to the scrap heap while this class's _Deallocate is still the one called.
    ~BSScrapArray() override {
        this->Clear(true);
    }

    // The buffer comes from pScrapHeap.
    T* _Allocate(uint32_t aiSizeInItem) override;
    void _Deallocate(T* apBuffer) override;
    T* _Reallocate(T* apBuffer, uint32_t aiNewSizeInItem) override;

    // 0x10 the heap the buffer comes from
    ScrapHeap* pScrapHeap;
};

template <class T, uint32_t uiGrowStep>
T* BSScrapArray<T, uiGrowStep>::_Allocate(uint32_t aiSizeInItem) {
    return static_cast<T*>(pScrapHeap->Allocate(sizeof(T) * aiSizeInItem, ScrapHeap::ALIGNMENT));
}

template <class T, uint32_t uiGrowStep>
void BSScrapArray<T, uiGrowStep>::_Deallocate(T* apBuffer) {
    pScrapHeap->Deallocate(apBuffer);
}

// A new buffer from the scrap heap, the current items' bytes copied over, the old buffer freed.
template <class T, uint32_t uiGrowStep>
T* BSScrapArray<T, uiGrowStep>::_Reallocate(T* apBuffer, uint32_t aiNewSizeInItem) {
    T* pNewBuffer = static_cast<T*>(pScrapHeap->Allocate(sizeof(T) * aiNewSizeInItem, ScrapHeap::ALIGNMENT));
    ourMemcpy(pNewBuffer, apBuffer, sizeof(T) * this->iSize);
    pScrapHeap->Deallocate(apBuffer);
    return pNewBuffer;
}

using TeleportNodeArray = BSSimpleArray<AStarNode<ParentSpaceNode, TeleportLink>*>;
using TeleportNodeScrapArray = BSScrapArray<AStarNode<ParentSpaceNode, TeleportLink>*>;
