#include "Fallout/Magic/MagicHitEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

NiRTTI MagicHitEffect::ms_RTTI("MagicHitEffect", &BSTempEffect::ms_RTTI);

MagicHitEffect::MagicHitEffect()
  : BSTempEffect(nullptr, std::numeric_limits<float>::max()), pEffect(nullptr), pTarget(nullptr), fAge(0.0f),
    bFinished(false) {}

MagicHitEffect::MagicHitEffect(TESObjectREFR* apTarget, ActiveEffect* apEffect)
  : BSTempEffect(apTarget ? apTarget->TESObjectREFR::GetParentCell() : nullptr, std::numeric_limits<float>::max()),
    pEffect(apEffect), pTarget(apTarget), fAge(0.0f), bFinished(false) {}

MagicHitEffect::~MagicHitEffect() {
    pTarget = nullptr;
    if (pEffect) {
        pEffect->RemoveHitEffect(this);
    }
    pEffect = nullptr;
}

bool MagicHitEffect::Update(float afTimeDelta) {
    fAge = fAge + afTimeDelta;
    if (!pTarget || !pTarget->Get3D() || pTarget->TESForm::GetDelete()) {
        return false;
    }
    if (fLifetime < static_cast<double>(fAge)) {
        bFinished = true;
    }
    return true;
}

// fAge (4) and bFinished (1), then fLifetime (4) from save version 0x72 on.
uint16_t MagicHitEffect::GetSaveSize(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    uint16_t usSize = 0;
    usSize += 4;
    usSize += 1;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x72) {
        usSize += 4;
    }
    return usSize;
}

// The temp effect's data, the age and finished flag, then the target's form ID.
void MagicHitEffect::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    BSTempEffect::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAge), sizeof(fAge), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bFinished), sizeof(bFinished), 0);
    apSaveGameBuffer->SaveFormID(pTarget, 0);
}

void MagicHitEffect::SaveGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    pSaveLoadGameOLD->SaveGameDataOLD(&fAge, sizeof(fAge));
    pSaveLoadGameOLD->SaveGameDataOLD(&bFinished, sizeof(bFinished));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x72) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fLifetime, sizeof(fLifetime));
    }
}

// The target is loaded as its form ID; InitLoadGame resolves it.
void MagicHitEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSTempEffect::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAge), sizeof(fAge));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bFinished), sizeof(bFinished));
    apLoadGameBuffer->LoadFormID(pTarget);
}

void MagicHitEffect::LoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    pSaveLoadGameOLD->LoadGameDataOLD(&fAge, sizeof(fAge));
    pSaveLoadGameOLD->LoadGameDataOLD(&bFinished, sizeof(bFinished));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x72) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fLifetime, sizeof(fLifetime));
    }
    pEffect = apEffect;
    pTarget = apTarget;
}

void MagicHitEffect::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef = nullptr;
    if (pTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pTarget = pRef;
}

void MagicHitEffect::InitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    if (apTarget) {
        pCell = apTarget->TESObjectREFR::GetParentCell();
    }
}

void MagicHitEffect::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSTempEffect::FinishLoadGame(apLoadGameBuffer);
}

void MagicHitEffect::UpdateParentCell(NiAVObject* apRoot) {
    if (pTarget) {
        pCell = pTarget->GetParentCell();
        if (apRoot && pCell && pCell->Get3D()) {
            if (apRoot->m_pkParent) {
                NiNode* pCell3D = pCell->Get3D();
                if (pCell3D == apRoot->m_pkParent->m_pkParent) {
                    return;
                }
            }
            NiNode* pDynamicNode = static_cast<NiNode*>(pCell->GetDynamicNode());
            pDynamicNode->AttachChild(apRoot, 1);
        }
    }
}

BSSimpleList<MagicHitEffect*>* MagicHitEffect::InstantiateHitEffects(ActiveEffect* apEffect) {
    MagicHitEffect* peffect;
    if (apEffect->pTarget->GetTargetAsActor()) {
        Actor* pTargetActor = apEffect->pTarget->GetTargetAsActor();
        TESCreature* pcreature = dynamic_cast<TESCreature*>(pTargetActor->GetActorBase());
        pTargetActor = apEffect->pTarget->GetTargetAsActor();
        Character* pcharacter = dynamic_cast<Character*>(pTargetActor);
        if ((!pcreature || !pcreature->IsHumanoidCreature()) && pcharacter) {
            pcharacter->IsNpc();
        }
    }
    BSSimpleList<MagicHitEffect*>* plist = nullptr;
    peffect = nullptr;
    EffectSetting* pEffectSetting = apEffect->GetEffectSetting();
    if (pEffectSetting->IsFlagSet(EffectSetting::GORY_VISUALS) && bDisableAllGore.Bool()) {
        return nullptr;
    }
    if (!apEffect->IsFlagSet(ActiveEffect::NO_HITEFFECT_NIF) && !Main::GetSceneIsComplex()) {
        peffect = new MagicModelHitEffect(apEffect->pTarget->GetTargetStatsObject(), apEffect);
        if (peffect->Init()) {
            ReferenceProcessLists.AddTempEffect(peffect);
            plist = new BSSimpleList<MagicHitEffect*>(peffect);
        }
        else if (peffect) {
            delete peffect;
        }
    }
    if (!apEffect->IsFlagSet(ActiveEffect::NO_HITEFFECT_SHADER)) {
        peffect = new MagicShaderHitEffect(apEffect->pTarget->GetTargetStatsObject(), apEffect);
        if (peffect->Init()) {
            ReferenceProcessLists.AddTempEffect(peffect);
            if (plist) {
                plist->AddHead(peffect);
            }
            else {
                BSSimpleList<MagicHitEffect*>* pnewList;
                pnewList = new BSSimpleList<MagicHitEffect*>(peffect);
                return pnewList;
            }
        }
        else if (peffect) {
            delete peffect;
        }
    }
    return plist;
}

void ActiveEffect::RemoveHitEffect(MagicHitEffect* apHitEffect) {
    if (pHitEffects) {
        pHitEffects->Remove(apHitEffect);
    }
}
