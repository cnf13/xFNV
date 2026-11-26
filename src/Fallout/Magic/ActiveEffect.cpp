#include "Fallout/Magic/ActiveEffect.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/VampirismEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>

ActiveEffect::ActiveEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : fElapsedSeconds(0.0f), pSpell(apSpell), pEffect(apEffect), bActive(true), bStarted(false), bFinished(false),
    bDone(false), iFlags(0), fMagnitude(static_cast<float>(apEffect->GetMagnitude())),
    fDuration(static_cast<float>(apEffect->GetDuration())), pTarget(nullptr), pCaster(apCaster),
    eSpellType(apSpell->GetSpellType()), pSource(nullptr), pHitEffects(nullptr), pDisplacementSpell(nullptr) {
    if (apEffect && !apEffect->Conditions.IsEmpty()) {
        SetFlag(ActiveEffect::HAS_CONDITIONS);
    }
}

ActiveEffect::~ActiveEffect() {
    for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        MagicHitEffect* peffect = plist->m_item;
        peffect->bFinished = true;
        peffect->pEffect = nullptr;
    }
    if (pHitEffects) {
        pHitEffects->RemoveAll();
    }
    delete pHitEffects;
    if (PersistentSound.IsValid()) {
        PersistentSound.Stop();
        PersistentSound.Release();
    }
}

ActiveEffect* ActiveEffect::Copy() {
    ActiveEffect* pCopy = new ActiveEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void ActiveEffect::Finalize() {
    bool bHasConditions = HasConditions() || ActiveEffect::GetDisplacementSpell();
    if (bHasConditions) {
        EvaluateConditions(0.0, true);
    }
}

uint16_t ActiveEffect::GetSaveSize(Actor* apActor) {
    uint16_t isize = 28;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        isize = 29;
        for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            isize += plist->m_item->GetSaveSize(this, apActor) + 1;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 72u) {
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
            ++isize;
        }
    }
    else {
        isize += 4;
    }
    return isize;
}

void ActiveEffect::SaveGame(Actor* apActor) {
    bool btempNoHitEffect;
    uint8_t chitEffectType;
    uint8_t ccount;
    uint32_t isourceID;
    uint32_t icasterID;
    uint32_t itargetID;
    pSaveLoadGameOLD->SaveGameDataOLD(&fElapsedSeconds, sizeof(fElapsedSeconds));
    pSaveLoadGameOLD->SaveGameDataOLD(&bActive, sizeof(bActive));
    pSaveLoadGameOLD->SaveGameDataOLD(&bStarted, sizeof(bStarted));
    pSaveLoadGameOLD->SaveGameDataOLD(&bFinished, sizeof(bFinished));
    pSaveLoadGameOLD->SaveGameDataOLD(&bDone, sizeof(bDone));
    pSaveLoadGameOLD->SaveGameDataOLD(&bWornEnchantment, sizeof(bWornEnchantment));
    pSaveLoadGameOLD->SaveGameDataOLD(&fMagnitude, sizeof(fMagnitude));
    pSaveLoadGameOLD->SaveGameDataOLD(&fDuration, sizeof(fDuration));
    icasterID = 0;
    if (pCaster) {
        icasterID = pCaster->GetMagicCasterFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&icasterID, 4u);
    itargetID = 0;
    if (pTarget) {
        itargetID = pTarget->GetMagicTargetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&itargetID, 4u);
    isourceID = 0;
    if (pSource) {
        isourceID = pSource->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&isourceID, 4u);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        ccount = 0;
        uint8_t* pcount = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&ccount, sizeof(ccount));
        for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            MagicHitEffect* phitEffect = plist->m_item;
            chitEffectType = phitEffect->GetType();
            pSaveLoadGameOLD->SaveGameDataOLD(&chitEffectType, sizeof(chitEffectType));
            phitEffect->SaveGame(this, apActor);
            ++ccount;
        }
        *pcount = ccount;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 72u) {
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
            btempNoHitEffect = IsFlagSet(ActiveEffect::NO_HITEFFECT_FORCED);
            pSaveLoadGameOLD->SaveGameDataOLD(&btempNoHitEffect, sizeof(btempNoHitEffect));
        }
    }
    else {
        pSaveLoadGameOLD->SaveGameDataOLD(&iFlags, sizeof(iFlags));
    }
}

// The base body of the buffer SaveGame slot.
void ActiveEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
    uint8_t chitEffectType;
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&fElapsedSeconds), sizeof(fElapsedSeconds), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bActive), sizeof(bActive), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bStarted), sizeof(bStarted), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bFinished), sizeof(bFinished), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDone), sizeof(bDone), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWornEnchantment), sizeof(bWornEnchantment), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMagnitude), sizeof(fMagnitude), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDuration), sizeof(fDuration), 0);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFlags), sizeof(iFlags), 0);
    uint32_t icasterID = pCaster ? pCaster->GetMagicCasterFormID() : 0;
    apBuffer->SaveFormID(icasterID, 0);
    uint32_t itargetID = pTarget ? pTarget->GetMagicTargetFormID() : 0;
    apBuffer->SaveFormID(itargetID, 0);
    uint32_t idisplacementSpellID = pDisplacementSpell ? pDisplacementSpell->GetMagicItemFormID() : 0;
    apBuffer->SaveFormID(idisplacementSpellID, 0);
    apBuffer->SaveFormID(pSource, 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apBuffer->StartVariableSizedValue();
    for (MagicHitEffect* phitEffect : *pHitEffects) {
        if (phitEffect) {
            chitEffectType = phitEffect->GetType();
            apBuffer->SaveDataEndian(reinterpret_cast<char*>(&chitEffectType), sizeof(chitEffectType), 0);
            phitEffect->SaveGame(apBuffer);
            ++isaveGameCount;
        }
    }
    apBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void ActiveEffect::LoadGame(Actor* apActor) {
    bool btempNoHitEffect;
    uint8_t chitEffectType;
    MagicHitEffect* phitEffect;
    uint8_t ccount;
    uint32_t isourceID;
    uint32_t icasterID;
    uint32_t itargetID;
    pSaveLoadGameOLD->LoadGameDataOLD(&fElapsedSeconds, sizeof(fElapsedSeconds));
    pSaveLoadGameOLD->LoadGameDataOLD(&bActive, sizeof(bActive));
    pSaveLoadGameOLD->LoadGameDataOLD(&bStarted, sizeof(bStarted));
    pSaveLoadGameOLD->LoadGameDataOLD(&bFinished, sizeof(bFinished));
    pSaveLoadGameOLD->LoadGameDataOLD(&bDone, sizeof(bDone));
    pSaveLoadGameOLD->LoadGameDataOLD(&bWornEnchantment, sizeof(bWornEnchantment));
    pSaveLoadGameOLD->LoadGameDataOLD(&fMagnitude, sizeof(fMagnitude));
    pSaveLoadGameOLD->LoadGameDataOLD(&fDuration, sizeof(fDuration));
    pSaveLoadGameOLD->LoadNumericID(&icasterID, 4u);
    pCaster = reinterpret_cast<MagicCaster*>(static_cast<uintptr_t>(icasterID));
    pSaveLoadGameOLD->LoadNumericID(&itargetID, 4u);
    pTarget = reinterpret_cast<MagicTarget*>(static_cast<uintptr_t>(itargetID));
    pSaveLoadGameOLD->LoadNumericID(&isourceID, 4u);
    pSource = reinterpret_cast<TESBoundObject*>(static_cast<uintptr_t>(isourceID));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&ccount, sizeof(ccount));
        for (uint32_t i = 0; i < ccount; ++i) {
            pSaveLoadGameOLD->LoadGameDataOLD(&chitEffectType, sizeof(chitEffectType));
            phitEffect = nullptr;
            if (chitEffectType == 5) {
                phitEffect = new MagicModelHitEffect;
            }
            else if (chitEffectType == 6) {
                phitEffect = new MagicShaderHitEffect;
            }
            else {
                Warning("SAVELOAD: Unknown hit effect type: %i", chitEffectType);
            }
            phitEffect->LoadGame(this, apActor);
            if (pHitEffects) {
                pHitEffects->AddHead(phitEffect);
            }
            else {
                pHitEffects = new BSSimpleList<MagicHitEffect*>(phitEffect);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 72u) {
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&btempNoHitEffect, sizeof(btempNoHitEffect));
            if (btempNoHitEffect) {
                ForceNoHitEffect();
            }
        }
    }
    else {
        pSaveLoadGameOLD->LoadGameDataOLD(&iFlags, sizeof(iFlags));
    }
}

void ActiveEffect::LoadGame(BGSLoadFormBuffer* apBuffer) {
    uint8_t chitEffectType;
    MagicHitEffect* phitEffect;
    apBuffer->LoadData(&fElapsedSeconds, sizeof(fElapsedSeconds));
    apBuffer->LoadData(&bActive, sizeof(bActive));
    apBuffer->LoadData(&bStarted, sizeof(bStarted));
    apBuffer->LoadData(&bFinished, sizeof(bFinished));
    apBuffer->LoadData(&bDone, sizeof(bDone));
    apBuffer->LoadData(&bWornEnchantment, sizeof(bWornEnchantment));
    apBuffer->LoadData(&fMagnitude, sizeof(fMagnitude));
    apBuffer->LoadData(&fDuration, sizeof(fDuration));
    apBuffer->LoadData(&iFlags, sizeof(iFlags));
    apBuffer->LoadFormID(pCaster);
    apBuffer->LoadFormID(pTarget);
    apBuffer->LoadFormID(pDisplacementSpell);
    uint32_t isourceID = apBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(isourceID);
    pSource = dynamic_cast<TESBoundObject*>(pForm);
    uint32_t isaveGameCount = apBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        chitEffectType = 0;
        apBuffer->LoadData(&chitEffectType, sizeof(chitEffectType));
        phitEffect = nullptr;
        if (chitEffectType == 5) {
            phitEffect = new MagicModelHitEffect;
        }
        else if (chitEffectType == 6) {
            phitEffect = new MagicShaderHitEffect;
        }
        else {
            Warning("SAVELOAD: Unknown magic hit effect type: %i", chitEffectType);
        }
        phitEffect->pEffect = this;
        phitEffect->LoadGame(apBuffer);
        if (pHitEffects) {
            pHitEffects->AddHead(phitEffect);
        }
        else {
            pHitEffects = new BSSimpleList<MagicHitEffect*>(phitEffect);
        }
    }
}

void ActiveEffect::InitLoadGame(Actor* apActor) {
    uint32_t icasterID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster));
    if (icasterID) {
        pCaster = MagicCaster::GetMagicCasterByNumericID(icasterID);
    }
    uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget));
    if (itargetID) {
        pTarget = MagicTarget::GetMagicTargetByNumericID(itargetID);
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSource)));
        pSource = dynamic_cast<TESBoundObject*>(pForm);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            MagicHitEffect* phitEffect = plist->m_item;
            phitEffect->InitLoadGame(this, apActor);
        }
    }
}

void ActiveEffect::InitLoadGame(BGSLoadGameBuffer* apBuffer) {
    MagicCaster* pMagicCaster = nullptr;
    if (pCaster) {
        pMagicCaster = MagicCaster::GetMagicCasterByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster))
        );
    }
    pCaster = pMagicCaster;
    MagicTarget* pMagicTarget = nullptr;
    if (pTarget) {
        pMagicTarget = MagicTarget::GetMagicTargetByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget))
        );
    }
    pTarget = pMagicTarget;
    MagicItem* pMagicItem = nullptr;
    if (pDisplacementSpell) {
        pMagicItem = MagicItem::GetMagicItemByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDisplacementSpell))
        );
    }
    pDisplacementSpell = pMagicItem;
    for (MagicHitEffect* phitEffect : *pHitEffects) {
        if (phitEffect) {
            phitEffect->InitLoadGame(apBuffer);
        }
    }
}

void ActiveEffect::FinishInitLoadGame(Actor* apActor) {
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            MagicHitEffect* phitEffect = plist->m_item;
            phitEffect->FinishInitLoadGame(this, apActor, nullptr);
            ReferenceProcessLists.AddTempEffect(phitEffect);
        }
    }
    EffectSetting* pEffectSetting = GetEffectSetting();
    if (pEffectSetting->IsFlagSet(EffectSetting::PERSIST)) {
        StartSound();
    }
}

// The PC body is the shared empty stub.
void ActiveEffect::Revert(Actor*) {}

bool ActiveEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    if (apCaster != pCaster) {
        return false;
    }
    pCaster = nullptr;
    return true;
}

bool ActiveEffect::IsCausingDamage() {
    return false;
}

void ActiveEffect::FinishLoadGame(BGSLoadFormBuffer* apBuffer) {
    EffectSetting* pEffectSetting = GetEffectSetting();
    if (pEffectSetting->IsFlagSet(EffectSetting::PERSIST)) {
        StartSound();
    }
    for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist; plist = plist->m_pkNext) {
        MagicHitEffect* pHitEffect = plist->m_item;
        if (pHitEffect) {
            pHitEffect->FinishLoadGame(apBuffer);
            ReferenceProcessLists.AddTempEffect(pHitEffect);
        }
    }
}

void ActiveEffect::CopyData(ActiveEffect* apDestination) {
    apDestination->fElapsedSeconds = fElapsedSeconds;
    apDestination->pSpell = pSpell;
    apDestination->pEffect = pEffect;
    apDestination->bActive = bActive;
    apDestination->bStarted = bStarted;
    apDestination->bFinished = bFinished;
    apDestination->bDone = bDone;
    apDestination->bWornEnchantment = bWornEnchantment;
    apDestination->fMagnitude = fMagnitude;
    apDestination->fDuration = fDuration;
    apDestination->pTarget = pTarget;
    apDestination->pCaster = pCaster;
    apDestination->eSpellType = eSpellType;
    apDestination->pSource = pSource;
    apDestination->iFlags = iFlags;
    apDestination->pDisplacementSpell = pDisplacementSpell;
}

bool ActiveEffect::CheckCast(MagicCaster* apCaster) {
    return true;
}

bool ActiveEffect::CheckTarget(MagicTarget* apTarget) {
    return true;
}

MagicItem* ActiveEffect::GetSpell() {
    return pSpell;
}

MagicSystem::SpellType ActiveEffect::GetSpellType() {
    return eSpellType;
}

EffectItem* ActiveEffect::GetEffectItem() {
    return pEffect;
}

TESBoundObject* ActiveEffect::GetEnchantmentSource() {
    return pSource;
}

MagicTarget* ActiveEffect::GetTarget() {
    return pTarget;
}

void ActiveEffect::ForceNoHitEffect() {
    SetFlag(NO_HITEFFECT_SHADER);
    SetFlag(NO_HITEFFECT_NIF);
}

// KEEP: the exe returns bDone in AL while retaining the upper EAX bytes from `this`.
int8_t ActiveEffect::IsDone() {
    return bDone;
}

void ActiveEffect::SetFlag(ActiveEffect::ActiveEffectFlags aiFlag) {
    iFlags |= aiFlag;
}

// Returns whether the FROM_WORN_ENCHANTMENT bit is set.
bool ActiveEffect::FromWornEnchantment() {
    return (iFlags & FROM_WORN_ENCHANTMENT) != 0;
}

MagicItem* ActiveEffect::GetDisplacementSpell() {
    return pDisplacementSpell;
}

bool ActiveEffect::IsQueuedForRemoval() const {
    return (iFlags & QUEUED_FOR_REMOVAL) != 0;
}

// Sets or clears the queued-for-removal bit that IsQueuedForRemoval tests. Tests the whole byte; bool would assume
// 0/1.
void ActiveEffect::SetQueuedForRemoval(uint8_t abQueued) {
    if (abQueued) {
        iFlags |= QUEUED_FOR_REMOVAL;
    }
    else {
        iFlags &= ~QUEUED_FOR_REMOVAL;
    }
}

void ActiveEffect::UpdateEffect(float afSecondsElapsed) {
    if (HasConditions() || GetDisplacementSpell()) {
        EvaluateConditions(afSecondsElapsed, false);
    }
    if (afSecondsElapsed > 0.0 && !bDone) {
        MagicItem* pspell = pSpell;
        MagicTarget* ptarget = pTarget;
        TESObjectREFR* ptargetStatsObject = ptarget->GetTargetStatsObject();
        bool bpermanent = MagicSystem::IsSpellTypePermanent(eSpellType);
        if (!bStarted && !bFinished && bActive) {
            if (ptargetStatsObject == pPlayer && Interface::IsInMenuMode()) {
                SetFlag(ActiveEffect::DEFERRED_SOUND);
            }
            else {
                StartSound();
            }
            Start();
            if (!bDone && !bFinished) {
                if (!IsFlagSet(ActiveEffect::NO_HITEFFECT_FORCED)) {
                    EffectSetting* pEffectSetting = GetEffectSetting();
                    if (!pEffectSetting->IsFlagSet(EffectSetting::NO_HIT_EFFECT) &&
                        (eSpellType != MagicSystem::ABILITY ||
                         ((GetEffectSetting(), NiAlphaProperty::GetType()) && fMagnitude < 100.0))) {
                        if (eSpellType == MagicSystem::ABILITY) {
                            SetFlag(ActiveEffect::NO_HITEFFECT_NIF);
                        }
                        // Food shows no hit effect: the spell as an ingredient or a potion, each asked through its
                        // EffectItemList's IsFood.
                        IngredientItem* pingredient = nullptr;
                        if (pspell->GetSpellType() == MagicSystem::WORTCRAFT) {
                            pingredient = static_cast<IngredientItem*>(pspell);
                        }
                        AlchemyItem* ppotion = nullptr;
                        if (pspell->GetSpellType() == MagicSystem::POTION) {
                            ppotion = static_cast<AlchemyItem*>(pspell);
                        }
                        bool bfood;
                        if (pingredient) {
                            bfood = static_cast<EffectItemList*>(pingredient)->IsFood();
                        }
                        else if (ppotion) {
                            bfood = static_cast<EffectItemList*>(ppotion)->IsFood();
                        }
                        else {
                            bfood = false;
                        }
                        if (!bfood) {
                            if (ptargetStatsObject == pPlayer && Interface::IsInMenuMode()) {
                                SetFlag(ActiveEffect::DEFERRED_HITEFFECT);
                            }
                            else {
                                if (this->pHitEffects) {
                                    this->pHitEffects->RemoveAll();
                                    BSSimpleList<MagicHitEffect*>* pHitEffects = this->pHitEffects;
                                    if (pHitEffects) {
                                        delete pHitEffects;
                                    }
                                    this->pHitEffects = nullptr;
                                }
                                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                    TaskQueueInterface::GetInstance()->QueueMHEInstantiateHitEffects(this);
                                }
                                else {
                                    this->pHitEffects = MagicHitEffect::InstantiateHitEffects(this);
                                }
                            }
                        }
                    }
                }
                bStarted = true;
            }
        }
        if (!bDone) {
            double dNewElapsedSeconds;
            float fupdateTime = afSecondsElapsed;
            if (bpermanent ||
                FromWornEnchantment() ||
                (dNewElapsedSeconds = fElapsedSeconds + afSecondsElapsed, !(fDuration < dNewElapsedSeconds))) {
                fElapsedSeconds = fElapsedSeconds + afSecondsElapsed;
            }
            else {
                double dDuration = fDuration;
                fupdateTime = dDuration - fElapsedSeconds;
                fElapsedSeconds = fDuration;
            }
            if (bActive) {
                bool bdeferredHitEffect = IsFlagSet(ActiveEffect::DEFERRED_HITEFFECT);
                bool bdeferredSound = IsFlagSet(ActiveEffect::DEFERRED_SOUND);
                if ((bdeferredHitEffect || bdeferredSound) &&
                    ptargetStatsObject == pPlayer &&
                    !Interface::IsInMenuMode()) {
                    if (bdeferredHitEffect) {
                        if (this->pHitEffects) {
                            this->pHitEffects->RemoveAll();
                            BSSimpleList<MagicHitEffect*>* pHitEffects = this->pHitEffects;
                            if (pHitEffects) {
                                delete pHitEffects;
                            }
                            this->pHitEffects = nullptr;
                        }
                        this->pHitEffects = MagicHitEffect::InstantiateHitEffects(this);
                        ClearFlag(ActiveEffect::DEFERRED_HITEFFECT);
                    }
                    if (bdeferredSound) {
                        StartSound();
                        ClearFlag(ActiveEffect::DEFERRED_SOUND);
                    }
                }
                Update(fupdateTime);
            }
            if (!bpermanent && !FromWornEnchantment()) {
                if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
                    bDone = true;
                }
                else if (static_cast<double>(fElapsedSeconds) >= fDuration) {
                    bDone = true;
                }
            }
            if (pTarget->MagicTargetIsActor()) {
                Actor* pActor = static_cast<Actor*>(pTarget);
                if (pActor && pActor->IsDead(true)) {
                    EffectSetting* pEffectSetting = GetEffectSetting();
                    if (!pEffectSetting->IsFlagSet(EffectSetting::NO_DEATH_DISPEL)) {
                        bDone = true;
                    }
                }
            }
        }
    }
    if (bStarted && (bDone || !bActive) && !bFinished) {
        Finish();
        bFinished = true;
        if (PersistentSound.IsValid()) {
            PersistentSound.Stop();
        }
    }
}

void ActiveEffect::EvaluateConditions(float afTimeDelta, bool abForceUpdate) {
    if (HasConditions() || GetDisplacementSpell()) {
        MagicTarget* pTarget = GetTarget();
        if (pTarget && pTarget->GetTargetStatsObject()) {
            float fNewIntervals;
            double dOldIntervalsFloor;
            float fOldIntervals;
            float fActiveEffectConditionUpdateInterval = ::fActiveEffectConditionUpdateInterval.Float();
            if (abForceUpdate ||
                (fOldIntervals = fElapsedSeconds / fActiveEffectConditionUpdateInterval,
                 (dOldIntervalsFloor = std::floor(fOldIntervals),
                  (fNewIntervals = (fElapsedSeconds + afTimeDelta) / fActiveEffectConditionUpdateInterval,
                   dOldIntervalsFloor != std::floor(fNewIntervals))))) {
                TESObjectREFR* pTargetActor = pTarget->GetTargetStatsObject();
                EffectItem* pEffectItem = GetEffectItem();
                bool bIsTrue = pEffectItem->Conditions.IsTrue(pTargetActor, nullptr);
                bool bActivate = !CheckDisplacementSpellOnTarget() && bIsTrue;
                if (bActivate && !bActive && bFinished && !bDone) {
                    bStarted = false;
                    bFinished = false;
                }
                bActive = bActivate;
            }
        }
    }
}

// Effects cast from worn armour or clothing are marked as coming from a worn enchantment.
void ActiveEffect::SetEnchantmentSource(TESBoundObject* apSource) {
    pSource = apSource;
    if (pSource && (pSource->GetFormType() == ARMO_ID || pSource->GetFormType() == CLOT_ID)) {
        SetFlag(FROM_WORN_ENCHANTMENT);
    }
}

void ActiveEffect::Dispel(bool abNow) {
    bDone = true;
    SetFlag(WAS_DISPELED);
    if (abNow) {
        UpdateEffect(0.0);
    }
}

void ActiveEffect::ClearFlag(ActiveEffect::ActiveEffectFlags aiFlag) {
    iFlags &= ~aiFlag;
}

void ActiveEffect::SetMagnitude(float afMagnitude) {
    fMagnitude = afMagnitude;
}

// Scales the duration of a no-magnitude effect, else the magnitude (floored, at least 1), by afMultiplier; effects with
// neither are left alone. The products are rounded to float before the floors.
void ActiveEffect::AdjustEffectiveness(float afMultiplier) {
    if (GetEffectItem()->GetEffectSetting()->CheckEffectFlag(
            EffectSetting::NO_DURATION | EffectSetting::NO_MAGNITUDE
        )) {
        return;
    }
    if (afMultiplier == 1.0) {
        return;
    }
    if (GetEffectItem()->GetEffectSetting()->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
        float fDurationScaled = GetDuration() * afMultiplier;
        SetDuration(fDurationScaled);
    }
    else {
        float fMagnitudeScaled = GetMagnitude() * afMultiplier;
        float fNewMagnitude;
        if (FloatFloor(fMagnitudeScaled) < 1.0) {
            fNewMagnitude = 1.0f;
        }
        else {
            float fMagnitudeScaledAgain = GetMagnitude() * afMultiplier;
            fNewMagnitude = FloatFloor(fMagnitudeScaledAgain);
        }
        SetMagnitude(fNewMagnitude);
    }
}

void ActiveEffect::SetDuration(float afDuration) {
    fDuration = afDuration;
}

float ActiveEffect::GetDuration() {
    return fDuration;
}

float ActiveEffect::GetMagnitude() {
    return fMagnitude;
}

bool ActiveEffect::CheckDisplacementSpellOnTarget() {
    bool bresult = false;
    bool bignoreAddiction = false;
    if (this->pTarget->MagicTargetIsActor()) {
        Actor* pactor = static_cast<Actor*>(this->pTarget);
        if (pactor && pactor->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS) > 0) {
            bignoreAddiction = true;
        }
    }
    SpellItem* pspell = dynamic_cast<SpellItem*>(pSpell);
    if (pDisplacementSpell && this->pTarget) {
        for (BSSimpleList<ActiveEffect*>* plist = this->pTarget->GetActiveEffectList(); plist && !bresult;
             plist = plist->m_pkNext) {
            ActiveEffect* pitem = plist->m_item;
            if (pitem && pitem->bActive) {
                if (pitem->pSpell == pDisplacementSpell) {
                    bresult = true;
                }
                else if (pitem->pSpell->GetSpellType() == MagicSystem::POTION) {
                    AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(pitem->pSpell);
                    if (ppotion) {
                        bresult = ppotion->GetAddictionItem() == pspell;
                    }
                }
            }
        }
    }

    if (bresult && bignoreAddiction) {
        return false;
    }
    return bresult;
}

void ActiveEffect::StartSound() {
    uint32_t isoundFileID;
    EffectSetting* pEffectSetting = GetEffectSetting();
    if (pEffectSetting->data.pHitSound) {
        pEffectSetting = GetEffectSetting();
        isoundFileID = pEffectSetting->data.pHitSound->iFormID;
    }
    else {
        isoundFileID = 0;
    }
    if (isoundFileID && pTarget) {
        if (pSpell->GetSpellType() != MagicSystem::ABILITY) {
            BSSoundHandle soundHandle;
            Actor* ptargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
            pEffectSetting = GetEffectSetting();
            bool bpersistent = pEffectSetting->IsFlagSet(EffectSetting::PERSIST);
            pEffectSetting = GetEffectSetting();
            if (pEffectSetting->data.pHitSound) {
                pEffectSetting = GetEffectSetting();
                TESSound* pHitSound = pEffectSetting->data.pHitSound;
                bpersistent = TESSound::ConvertLoopFlag(pHitSound->GetData().iFlags) != 0;
            }
            if (ptargetActor) {
                uint32_t uiflags = bpersistent ? 274 : 258;
                soundHandle = ptargetActor->PlaySound(isoundFileID, bpersistent, uiflags, false);
            }
            else {
                TESObjectREFR* ptargetObject = pTarget->GetTargetStatsObject();
                if (ptargetObject) {
                    BSSoundHandle kObjectSound = BSAudio::QInstance().GetSoundHandleByNumericID(isoundFileID, 258u);
                    NiPoint3 kLocation = ptargetObject->GetLocationOnReference();
                    kObjectSound.SetPosition(kLocation.x, kLocation.y, kLocation.z);
                    kObjectSound.Play(bpersistent);
                }
            }
            if (bpersistent) {
                if (PersistentSound.IsValid()) {
                    PersistentSound.Stop();
                    PersistentSound.Release();
                }
                PersistentSound = soundHandle;
            }
        }
    }
}

ActiveEffect* ActiveEffect::Activate(
    MagicCaster* apCaster,
    MagicItem* apMagicItem,
    EffectItem* apEffectItem,
    TESBoundObject* apObject
) {
    ActiveEffect* presult = nullptr;
    EffectSetting* psetting = apEffectItem ? apEffectItem->pEffectSetting : nullptr;
    EffectArchetypes::InstantiateFunction pInstantiate = EffectArchetypes::Archetypes[psetting->GetEffectArchetype()]
                                                             .pInstantiateFunction;
    if (pInstantiate) {
        presult = pInstantiate(apCaster, apMagicItem, apEffectItem);
    }
    if (presult) {
        presult->SetEnchantmentSource(apObject);
    }
    return presult;
}

ActiveEffect* ActiveEffect::LoadActiveEffect(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t ieffectArchetype;
    EffectItem* peffect;
    uint32_t ispellID = 0;
    apLoadGameBuffer->LoadFormID(ispellID);
    uint32_t isavedSpellID = ispellID;
    MagicItem* pspell = ispellID ? MagicItem::GetMagicItemByNumericID(ispellID) : nullptr;
    uint8_t ceffectIndex = 0;
    apLoadGameBuffer->LoadData(&ceffectIndex, sizeof(ceffectIndex));
    uint32_t ieffectArchetypeID = apLoadGameBuffer->LoadVariableSizedValue();
    if (pspell &&
        (peffect = pspell->GetEffectItemByIndex(ceffectIndex)) != nullptr &&
        ((ieffectArchetype = peffect->pEffectSetting->GetEffectArchetype()) == ieffectArchetypeID)) {
        apLoadGameBuffer->LoadVariableSizedValue();
        ActiveEffect* pactiveEffect = ActiveEffect::Activate(nullptr, pspell, peffect, nullptr);
        pactiveEffect->LoadGame(apLoadGameBuffer);
        return pactiveEffect;
    }

    apLoadGameBuffer->AdvanceBuffer(apLoadGameBuffer->LoadVariableSizedValue());
    SaveGameWarning(
        "GAME EFFECTS: Could not load active effect with spell %08X, effect index %i, and archetype ID "
        "%i",
        isavedSpellID,
        ceffectIndex,
        ieffectArchetypeID
    );
    return nullptr;
}

void ActiveEffect::LoadActiveEffectList(
    BGSLoadFormBuffer* apLoadGameBuffer,
    BSSimpleList<ActiveEffect*>* apActiveEffectList
) {
    RevertActiveEffectList(apLoadGameBuffer, apActiveEffectList);
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        ActiveEffect* pEffect = LoadActiveEffect(apLoadGameBuffer);
        if (pEffect) {
            apActiveEffectList->Insert(pEffect, &MagicTarget::ActiveEffectListCompareFn);
        }
    }
}

void ActiveEffect::LoadActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                    1000,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                    1000,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    for (BSSimpleList<ActiveEffect*>* plist = apActiveEffectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        ActiveEffect* peffect = plist->m_item;
        if (peffect) {
            delete peffect;
        }
    }
    apActiveEffectList->RemoveAll();
    pSaveLoadGameOLD->LoadGameDataOLD(&icount, sizeof(icount));
    for (int32_t i = 0; i < icount; ++i) {
        ActiveEffect* peffect = ActiveEffect::LoadActiveEffect(apActor);
        // The game reads apActor's current AI process here (a trivial accessor) and drops the result.
        if (dynamic_cast<VampirismEffect*>(peffect) && apActor) {}

        if (peffect) {
            apActiveEffectList->Insert(peffect, &MagicTarget::ActiveEffectListCompareFn);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        uint8_t* iendBufferPosition = &istartBufferPosition[iblockSize];
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= iendBufferPosition) {
                if (icurrentBufferPosition < iendBufferPosition) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                        1039,
                        pheader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                    1039,
                    pheader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= iendBufferPosition) {
            if (icurrentBufferPosition < iendBufferPosition) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                    1039,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                1039,
                iCurrentVersion
            );
        }
    }
}

void ActiveEffect::RevertActiveEffectList(
    BGSLoadFormBuffer* apLoadGameBuffer,
    BSSimpleList<ActiveEffect*>* apActiveEffectList
) {
    for (ActiveEffect* pactiveEffect : *apActiveEffectList) {
        if (pactiveEffect) {
            pactiveEffect->Revert(apLoadGameBuffer);
            delete pactiveEffect;
        }
    }
    apActiveEffectList->RemoveAll();
}

void ActiveEffect::InitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
    while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
        ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
        pactiveEffect->InitLoadGame(apActor);
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
}

void ActiveEffect::FinishInitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
    while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
        ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
        pactiveEffect->FinishInitLoadGame(apActor);
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
    if (apActor && apActor->pCurrentProcess) {
        BaseProcess* pProcess = apActor->pCurrentProcess;
        pProcess->SetShouldRefreshMagicShadersEffects(true, true, true);
    }
}

void ActiveEffect::FinishLoadActiveEffectList(
    BGSLoadFormBuffer* apLoadGameBuffer,
    BSSimpleList<ActiveEffect*>* apActiveEffectList
) {
    while (apActiveEffectList) {
        ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
        if (pactiveEffect) {
            pactiveEffect->FinishLoadGame(apLoadGameBuffer);
        }
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
}

void ActiveEffect::RevertActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
    while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
        ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
        pactiveEffect->Revert(apActor);
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
}

void ActiveEffect::SaveActiveEffectList(
    BGSSaveFormBuffer* apSaveGameBuffer,
    BSSimpleList<ActiveEffect*>* apActiveEffectList
) {
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    while (apActiveEffectList) {
        ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
        if (pactiveEffect) {
            ActiveEffect::SaveActiveEffect(apSaveGameBuffer, pactiveEffect);
            ++isaveGameCount;
        }
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void ActiveEffect::SaveActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block tag 'BLOK', written as a little-endian 4-byte value ("KOLB").
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
        ActiveEffect::SaveActiveEffect(apActiveEffectList->m_item, apActor);
        ++icount;
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                icurrentBufferPosition - itempStartPosition,
                pheader->iFormID,
                pEditorID,
                iFlags,
                992,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                icurrentBufferPosition - itempStartPosition,
                992,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size is the byte distance from its size field to the buffer's end, unsigned compares; the
        // size field is raw save-buffer memory, read here as bytes.
        uint8_t* pend = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pstart = reinterpret_cast<uint8_t*>(pblockSize);
        if (pend > pstart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp",
                992
            );
        }
        *pblockSize = static_cast<uint16_t>(pend - pstart);
    }
}

void ActiveEffect::SaveActiveEffect(ActiveEffect* apEffect, Actor* apActor) {
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        uint16_t usSaveSize = apEffect->GetSaveSize(apActor);
        pSaveLoadGameOLD->SaveGameDataOLD(&usSaveSize, sizeof(usSaveSize));
    }
    uint32_t uiSpellID = apEffect->pSpell->GetMagicItemFormID();
    pSaveLoadGameOLD->SaveNumericID(&uiSpellID, 4u);
    uint8_t ucEffectIndex = apEffect->pSpell->GetEffectItemIndex(apEffect->pEffect);
    pSaveLoadGameOLD->SaveGameDataOLD(&ucEffectIndex, sizeof(ucEffectIndex));
    apEffect->SaveGame(apActor);
}

uint16_t ActiveEffect::GetActiveEffectSaveSize(ActiveEffect* apEffect, Actor* apActor) {
    int16_t iSizeFieldSize = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
        iSizeFieldSize = 2;
    }
    return apEffect->GetSaveSize(apActor) + (iSizeFieldSize + 5);
}

uint16_t ActiveEffect::GetActiveEffectListSaveSize(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
    int16_t iBlockHeaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iBlockHeaderSize = 6;
    }
    uint16_t isize = iBlockHeaderSize + 2;
    while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
        isize += ActiveEffect::GetActiveEffectSaveSize(apActiveEffectList->m_item, apActor);
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                968,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                968,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp"
            );
        }
    }
    return isize;
}

void ActiveEffect::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ActiveEffect* ActiveEffect::LoadActiveEffect(Actor* apActor) {
    EffectItem* peffect;
    uint8_t ceffectIndex;
    uint32_t ispellID;
    uint16_t usSaveSize = 28;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x2Au) {
        pSaveLoadGameOLD->LoadGameDataOLD(&usSaveSize, sizeof(usSaveSize));
    }
    pSaveLoadGameOLD->LoadNumericID(&ispellID, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&ceffectIndex, sizeof(ceffectIndex));
    MagicItem* pspell = MagicItem::GetMagicItemByNumericID(ispellID);
    if (pspell && (peffect = static_cast<EffectItemList*>(pspell)->GetEffectItemByIndex(ceffectIndex)) != nullptr) {
        ActiveEffect* pactiveEffect = ActiveEffect::Activate(nullptr, pspell, peffect, nullptr);
        pactiveEffect->LoadGame(apActor);
        return pactiveEffect;
    }

    pSaveLoadGameOLD->AdvanceBuffer(usSaveSize);
    Warning(
        "GAME EFFECTS: Could not load active effect with actor effect %08X and effect index %i",
        ispellID,
        ceffectIndex
    );
    return nullptr;
}

void ActiveEffect::InitLoadActiveEffectList(
    BGSLoadFormBuffer* apLoadGameBuffer,
    BSSimpleList<ActiveEffect*>* apActiveEffectList
) {
    while (apActiveEffectList) {
        ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
        if (pactiveEffect) {
            pactiveEffect->InitLoadGame(apLoadGameBuffer);
        }
        apActiveEffectList = apActiveEffectList->m_pkNext;
    }
}

void ActiveEffect::SaveActiveEffect(BGSSaveFormBuffer* apSaveGameBuffer, ActiveEffect* apEffect) {
    uint32_t uiSpellID = apEffect->pSpell ? apEffect->pSpell->GetMagicItemFormID() : 0;
    apSaveGameBuffer->SaveFormID(uiSpellID, 0);
    char cEffectIndex = apEffect->pSpell->GetEffectItemIndex(apEffect->pEffect);
    apSaveGameBuffer->SaveDataEndian(&cEffectIndex, sizeof(cEffectIndex), 0);
    uint32_t uiArchetypeID = apEffect->pEffect->pEffectSetting->GetEffectArchetype();
    apSaveGameBuffer->SaveVariableSizedValue(uiArchetypeID);
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    int32_t iStartPosition = apSaveGameBuffer->iBufferPosition;
    apEffect->SaveGame(apSaveGameBuffer);
    int32_t iEndPosition = apSaveGameBuffer->iBufferPosition;
    apSaveGameBuffer->SaveVariableSizedValue(iEndPosition - iStartPosition, uiPosition);
}
