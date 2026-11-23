#pragma once

#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"

class TESSound;

// The particle cap and flags of an addon node.
struct ADDON_DATA {
    enum ADDON_FLAGS : uint8_t {
        FLAG_MASTER_PARTICLE_SYSTEM = 0x1,
    };

    // Swaps the particle cap read from a big-endian file.
    void Endian();

    uint16_t usMasterParticleCap;
    uint8_t cFlags;
};

// A model attached to the world by index (a marker or an effect), kept in the data handler's addon array.
class BGSAddonNode : public TESBoundObject, public TESModelTextureSwap {
public:
    BGSAddonNode();
    ~BGSAddonNode() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::Save;

    uint32_t GetIndex();
    void SetIndex(uint32_t aiIndex);
    uint16_t GetMasterParticleCap();
    bool IsMasterParticleSystem();
    uint32_t GetMasterParticleSystemIndex();

    uint32_t iIndex;
    TESSound* pSound;
    ADDON_DATA Data;
    uint32_t iMasterParticleSystemIndex;
};
