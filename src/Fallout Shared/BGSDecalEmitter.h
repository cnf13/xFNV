#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSImpactData;
class BSTempEffectParticle;

// Tracks the remaining decals and particle effect of an impact-data emitter.
class BGSDecalEmitter {
public:
    BGSDecalEmitter(BSTempEffectParticle* a1, BGSImpactData* a2, int32_t a3);
    ~BGSDecalEmitter();

    // Sprays the emitter's decals along its particle's direction while the particle lives; finishes the emitter once
    // the particle, its cell or its 3D are gone.
    void Update();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    int32_t iDecalsToEmit;
    bool bFinished;
    uint8_t padding05[3];
    BGSImpactData* pDecalImpactData;
    NiPointer<BSTempEffectParticle> spParticleData;
};
