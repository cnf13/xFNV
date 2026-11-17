#pragma once

#include "Fallout Shared/TESObjectARMO.h"

// An armor addon (ARMA): an armor form with the same members (both allocate 0x190 bytes).
class TESObjectARMA : public TESObjectARMO {
public:
    TESObjectARMA();
    ~TESObjectARMA() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    // also override TESValueForm's slots of the same signatures
    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void InitItem() override;
    bool IsArmorAddon() override {
        return true;
    }
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESObjectARMO::LoadGame;
    using TESObjectARMO::Save;
    using TESObjectARMO::SaveGame;
};
