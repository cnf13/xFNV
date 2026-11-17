#pragma once

#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Package.h"

class Actor;
class TESObjectCELL;

// The package of an actor that trespasses.
class TrespassPackage : public TESPackage {
public:
    TrespassPackage();
    TrespassPackage(Actor* apCriminal, TESForm* apOwner, TESObjectCELL* OpenSound, int32_t auiWarningLevel);
    ~TrespassPackage() override;

    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    uint16_t GetSaveSize() override;
    void SaveGame() override;
    void LoadGame() override;
    void InitLoadGame() override;
    bool IsPackageOwner(MobileObject*) override {
        return false;
    }
    using TESPackage::InitLoadGame;
    using TESPackage::LoadGame;
    using TESPackage::SaveGame;

    void ModNumberActors(int32_t aiNumber);
    void ModWarningTimer(float aftime);
    void ModWarning(int32_t ainumb);
    uint32_t GetNumberActors();

    static void operator delete(void* apMemory);

    float fwarningtimer;
    int32_t iwarning;
    TESForm* pOwner;
    TESForm* pObjectOwned;
    uint32_t iCrimeNumber;
    int32_t iwarninglevel;
    uint32_t iNumberActors;
};
