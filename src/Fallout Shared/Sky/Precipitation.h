#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiGeometry;
class NiParticleSystem;
class TESModel;

// The weather particle systems: rain or snow, kept as a pair of particle roots while the weather type blends
// from one to the other.
class Precipitation {
public:
    Precipitation();
    // The class has a vtable on PC; only the destructor slot is declared here.
    virtual ~Precipitation();

    // Loads the precipitation model apFilename.
    void Load(const char* apFilename);
    // Attaches the precipitation systems under the weather root node.
    void Initialize();
    // Detaches and releases both particle roots.
    void FlushAll();
    // Advances the particle systems by afSecondsPassed.
    void Update(float afSecondsPassed);
    void Load_0(TESModel* apModel);
    NiGeometry*
    CreateShaderGeometry(NiParticleSystem* apParticleSystem, const NiTransform& akWorldTransform, bool abSnow);

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    NiPointer<NiNode> spCurrentPrecipRoot;
    NiPointer<NiNode> spLastPrecipRoot;
    NiNode* pParent;
    float fBirthRate;
    TESModel* pQueuedModel;
};
