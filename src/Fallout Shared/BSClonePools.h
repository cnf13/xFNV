#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiAVObject;

// Pools of ready-made clones of loaded models: one pool per model, handing out clones and taking them back.
class BSClonePools {
public:
    // The clones of one model that are not in use.
    struct ClonePool {
        ClonePool();
        ~ClonePool();

        // Empties the pool: drops each clone, then destroys the list.
        void Free();

        // The deleting destructor frees through ourFree.
        static void operator delete(void* apMemory);

        NiObject* pObject;
        BSSimpleList<NiPointer<NiRefObject>> kAvailableClones;
    };

    BSClonePools();

    void KillClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount);
    void MakeClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount);
    NiAVObject* RequestClone(NiAVObject* apLoadedObject);

    static BSClonePools* GetInstance(bool abCreate);

    BSSimpleList<ClonePool*> ClonePools;

    static inline BSClonePools* pInstance;
};
