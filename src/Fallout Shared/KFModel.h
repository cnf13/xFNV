#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class BSStream;
class NiControllerSequence;
class TESAnimGroup;

// One loaded .kf file: the controller sequence it holds, its anim group and its reference counts.
class KFModel {
public:
    KFModel(const char* apFilename, BSStream* apStream);
    ~KFModel();

    // Atomically drops one reference.
    void DecRefCount();
    int32_t GetRefCount() const;
    // The anim group.
    TESAnimGroup* GetAnimGroup() const;
    // spSequence's object.
    NiControllerSequence* GetSequence();
    void IncManualRefCount();
    void IncRefCount() {
        NiAtomicIncrement(iRefCount);
    }
    void DecUseCount();
    void InitSequence(const char* apFilename);

    static void operator delete(void* apMemory);

    // the .kf file this was loaded from
    char* pFilename;
    NiPointer<NiControllerSequence> spSequence;
    NiPointer<TESAnimGroup> spAnimGroup;
    int32_t iRefCount;
    int32_t iManualRefCount;
};

// The engine's NiPointer of a .kf model takes and releases references through the model's own IncRefCount and
// DecRefCount.
inline void NiIncRefCount(KFModel* apModel) {
    apModel->IncRefCount();
}

inline void NiDecRefCount(KFModel* apModel) {
    apModel->DecRefCount();
}
