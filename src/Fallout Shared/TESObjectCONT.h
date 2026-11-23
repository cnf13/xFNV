#pragma once

#include "BSHavok/bhkShape.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESMagicCasterForm.h"
#include "Fallout Shared/TESMagicTargetForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWeightForm.h"

class MobileObject;
class TESObjectREFR;
class TESSound;

// A container's DATA record.
struct CONT_DATA {
    enum CONT_FLAGS : char {
        FLAG_RESPAWN = 0x2,
    };

    char cContFlags;
};

// A container base form (CONT). Its own members are not declared yet.
class TESObjectCONT : public TESBoundAnimObject,
                      public TESContainer,
                      public TESFullName,
                      public TESModelTextureSwap,
                      public TESScriptableForm,
                      public TESWeightForm,
                      public TESMagicCasterForm,
                      public TESMagicTargetForm,
                      public BGSDestructibleObjectForm,
                      public BGSOpenCloseForm {
public:
    TESObjectCONT();
    ~TESObjectCONT() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjecttoGet,
        int32_t aicount
    ) override;
    void HandleOpenFinish(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef) override;
    bool GetBlockActivatesWhileOpening() override {
        return true;
    }

    using TESForm::Save;

    TESSound* GetLoopingSound();
    // Whether the container's contents respawn (CONT_DATA::FLAG_RESPAWN).
    bool GetRespawn();
    TESSound* GetCloseSound() {
        return pCloseSound;
    }
    TESSound* GetOpenSound() {
        return pOpenSound;
    }
    void SetOpenSound(TESSound* apSound);
    void SetCloseSound(TESSound* apSound);
    void SetLoopingSound(TESSound* apSound);
    bool CanActivate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abAllowLockpicking);

    TESSound* pOpenSound;
    TESSound* pCloseSound;
    TESSound* pLoopingSound;
    CONT_DATA Data;
};

inline MobileObject* pstaticcontainer;
