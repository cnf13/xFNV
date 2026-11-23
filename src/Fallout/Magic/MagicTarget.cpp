#include "Fallout/Magic/MagicTarget.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/LimbConditionEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "Fallout/Magic/ValueAndConditionsEffect.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/tchar.h"

// The pending dispel of a spell. QueueSpellDispel allocates one with ourMalloc, zeroes it through Construct and stores
// the spell, the caster and the effect; DispelQueuedSpells passes them to Dispel and frees it with ourFree.
struct SpellDispelData {
    static SpellDispelData* Construct(void* apMemory);

    MagicItem* pSpell;
    MagicCaster* pCaster;
    ActiveEffect* pActiveEffect;
};

SpellDispelData* SpellDispelData::Construct(void* apMemory) {
    SpellDispelData* pThis = static_cast<SpellDispelData*>(apMemory);
    pThis->pSpell = nullptr;
    pThis->pCaster = nullptr;
    pThis->pActiveEffect = nullptr;
    return pThis;
}

MagicTarget::MagicTarget() : bShowTargetStats(false), bUpdating(false) {}

MagicTarget::~MagicTarget() {
    // The queued dispel entries are blocks the target owns.
    for (BSSimpleList<void*>* pList = &PostUpdateDispelList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (pList->m_item) {
            ourFree(pList->m_item);
        }
    }
}

bool MagicTarget::AddTarget(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    ActiveEffect* apActiveEffect,
    bool abAreaTarget
) {
    if (!apActiveEffect->GetEffectItem()->GetRange()) {
        return CheckAddEffect(apCaster, apSpell, apActiveEffect);
    }
    bool bIgnoreResistance = apActiveEffect->GetEffectItem()->IsScriptEffect() && apSpell->IgnoreResistance();
    if (bIgnoreResistance || apSpell->GetNoAbsorbOrReflect()) {
        return CheckAddEffect(apCaster, apSpell, apActiveEffect);
    }
    if (CheckReflect(apCaster, apSpell, apActiveEffect)) {
        EffectReflected(apCaster, apActiveEffect);
        return false;
    }
    if (!CheckAbsorb(apCaster, apSpell, apActiveEffect, false)) {
        return CheckAddEffect(apCaster, apSpell, apActiveEffect);
    }
    EffectAbsorbed(apCaster, apActiveEffect);
    return false;
}

void MagicTarget::RemoveEffects(bool abNow) {
    BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList();
    while (pIter && !pIter->IsEmpty()) {
        ActiveEffect* pEffect = pIter->m_item;
        pIter = pIter->m_pkNext;
        if (pEffect) {
            MagicItem* pSpell = pEffect->pSpell;
            if (pSpell->GetSpellType() == MagicSystem::SPELL ||
                pSpell->GetSpellType() == MagicSystem::POWER ||
                pSpell->GetSpellType() == MagicSystem::LESSER_POWER ||
                pSpell->GetSpellType() == MagicSystem::POTION ||
                pSpell->GetSpellType() == MagicSystem::WORTCRAFT) {
                pEffect->Dispel(abNow);
            }
            else {
                EnchantmentItem* penchantment = dynamic_cast<EnchantmentItem*>(pSpell);
                if (penchantment && penchantment->GetCastingType() == MagicSystem::CAST_ONCE) {
                    pEffect->Dispel(abNow);
                }
            }
        }
    }
}

void MagicTarget::ProcessEffectsFromItem(MagicItem* apItem) {
    BSSimpleList<ActiveEffect*>* pList = GetActiveEffectList();
    if (MagicTargetIsActor()) {
        // An actor without a process, or whose reference or cell has no 3D, is skipped.
        Actor* pActor = static_cast<Actor*>(this);
        if (pActor && !pActor->pCurrentProcess) {
            return;
        }
        if (!GetTargetStatsObject()->Get3D() ||
            !GetTargetStatsObject()->GetParentCell() ||
            !GetTargetStatsObject()->GetParentCell()->Get3D()) {
            return;
        }
    }
    while (pList && !pList->IsEmpty()) {
        BSSimpleList<ActiveEffect*>* pNext = pList->m_pkNext;
        ActiveEffect* pEffect = pList->m_item;
        if (pEffect && pEffect->pSpell == apItem) {
            if (pEffect->bStarted) {
                pEffect->Continue();
            }
            else {
                pEffect->UpdateEffect(0.1f);
                pNext = pList->m_pkNext;
            }
            if (pEffect->bDone) {
                RemoveEffect(pEffect, false);
            }
        }
        pList = pNext;
    }
}

// The list is walked through GetItem and GetNext, and advances before the effect.
void MagicTarget::EvaluateSpellConditions() {
    for (BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList(); pIter;) {
        ActiveEffect* pEffect;
        pEffect = pIter->GetItem();
        pIter = pIter->GetNext();
        if (pEffect) {
            bool bHasConditions;
            bHasConditions = pEffect->HasConditions();
            if (bHasConditions || pEffect->GetDisplacementSpell()) {
                pEffect->EvaluateConditions(0.0f, true);
            }
        }
    }
}

// Walks the list through IsEmpty, GetItem and GetNext, and advances before the effect.
void MagicTarget::SetDisplacementSpell(MagicItem* apDisplacementSpell, MagicItem* apDisplacedSpell) {
    for (BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList(); pIter;) {
        bool bEmpty;
        bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        ActiveEffect* pEffect;
        pEffect = pIter->GetItem();
        pIter = pIter->GetNext();
        if (pEffect && pEffect->GetSpell() == apDisplacedSpell) {
            pEffect->SetDisplacementSpell(apDisplacementSpell);
            pEffect->EvaluateConditions(0.0f, true);
        }
    }
}

// Destroys the effect through its virtual deleting destructor.
void MagicTarget::RemoveEffect(ActiveEffect* apEffect, bool abImmediate) {
    if (!abImmediate) {
        apEffect->SetQueuedForRemoval(true);
        return;
    }
    GetActiveEffectList()->Remove(apEffect);
    EffectRemoved(apEffect);
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1 && AILinearTaskThreadManager::QRunningThreads()) {
        TaskQueueInterface::GetInstance()->QueueDeleteActiveEffect(apEffect);
    }
    else if (apEffect) {
        delete apEffect;
    }
}

bool MagicTarget::StackEffectDuration(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect) {
    bool bresult = false;
    BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
    bool bdoAlcoholCheck = false;
    if (apSpell->GetSpellType() == MagicSystem::POTION) {
        AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(apSpell);
        if (ppotion) {
            bdoAlcoholCheck = ppotion->eEquipType == BGSEquipType::ET_ALCOHOL;
        }
    }
    while (!bresult && plist && !plist->IsEmpty()) {
        ActiveEffect* peffect = plist->m_item;
        BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
        bool bspellCheck = peffect && peffect->pSpell == apSpell;
        if (bdoAlcoholCheck) {
            AlchemyItem* pthisPotion = dynamic_cast<AlchemyItem*>(apSpell);
            if (pthisPotion) {
                bspellCheck = pthisPotion->eEquipType == BGSEquipType::ET_ALCOHOL;
            }
        }
        if (peffect && !peffect->bDone && bspellCheck) {
            int32_t iCasterMatch;
            if (apCaster) {
                iCasterMatch = peffect->pCaster == apCaster;
            }
            else {
                iCasterMatch = 1;
            }
            if (iCasterMatch) {
                int32_t iDifferentEffect;
                if (apActiveEffect) {
                    iDifferentEffect = !peffect->pEffect->MatchesEffect(apActiveEffect->pEffect);
                }
                else {
                    iDifferentEffect = 1;
                }
                if (iDifferentEffect) {
                    double dOldDuration = peffect->fDuration;
                    float fNewDuration = apActiveEffect->fDuration + dOldDuration;
                    peffect->SetDuration(fNewDuration);
                    bresult = true;
                }
            }
        }

        plist = pnext;
    }
    return bresult;
}

bool MagicTarget::ResetEffectElapsedTime(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect) {
    BSSimpleList<ActiveEffect*>* pnext;
    bool bresult = false;
    for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); !bresult && plist && !plist->IsEmpty();
         plist = pnext) {
        ActiveEffect* peffect = plist->m_item;
        pnext = plist->m_pkNext;
        if (peffect && !peffect->bDone && peffect->pSpell == apSpell) {
            int32_t iCasterMatch;
            if (apCaster) {
                iCasterMatch = peffect->pCaster == apCaster;
            }
            else {
                iCasterMatch = 1;
            }
            if (iCasterMatch) {
                int32_t iDifferentEffect;
                if (apActiveEffect) {
                    iDifferentEffect = !peffect->pEffect->MatchesEffect(apActiveEffect->pEffect);
                }
                else {
                    iDifferentEffect = 1;
                }
                if (iDifferentEffect) {
                    peffect->SetElapsedTime(0.0f);
                    bresult = true;
                }
            }
        }
    }
    return bresult;
}

void MagicTarget::Dispel(TESBoundObject* apEnchantmentSource, bool abNow) {
    if (apEnchantmentSource) {
        BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList();
        while (pIter && !pIter->IsEmpty()) {
            ActiveEffect* pEffect = pIter->m_item;
            pIter = pIter->m_pkNext;
            if (pEffect && !pEffect->bDone && pEffect->GetEnchantmentSource() == apEnchantmentSource) {
                pEffect->Dispel(abNow);
            }
        }
    }
}

void MagicTarget::Dispel() {
    BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList();
    while (pIter && !pIter->IsEmpty()) {
        ActiveEffect* pEffect = pIter->m_item;
        pIter = pIter->m_pkNext;
        if (pEffect) {
            pEffect->Dispel(true);
        }
    }
}

bool MagicTarget::HasDamageHealthEffect() {
    BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList();
    bool bIsDetrimental = false;
    while (pIter && !pIter->IsEmpty() && !bIsDetrimental) {
        ActiveEffect* pEffect = pIter->m_item;
        if (pEffect) {
            if (pEffect->GetEffectSetting()->IsAssociatedActorValueUsed()) {
                if (pEffect->pEffect->GetActorValueIndex() == ActorValue::HEALTH) {
                    bIsDetrimental = pEffect->GetEffectSetting()->IsFlagSet(EffectSetting::DETRIMENTAL);
                }
            }
        }
        pIter = pIter->m_pkNext;
    }
    return bIsDetrimental;
}

void MagicTarget::Dispel(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect) {
    BSSimpleList<ActiveEffect*>* pNext;
    for (BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList(); pIter && !pIter->IsEmpty(); pIter = pNext) {
        ActiveEffect* pEffect = pIter->m_item;
        pNext = pIter->m_pkNext;
        if (pEffect && !pEffect->bDone && pEffect->pSpell == apSpell && (!apCaster || pEffect->pCaster == apCaster)) {
            int32_t iSameSource;
            if (apActiveEffect) {
                TESBoundObject* pSource = apActiveEffect->GetEnchantmentSource();
                iSameSource = pSource == pEffect->GetEnchantmentSource();
            }
            else {
                iSameSource = 1;
            }
            if (iSameSource) {
                if (apActiveEffect) {
                    if (!pEffect->pEffect->MatchesEffect(apActiveEffect->pEffect)) {
                        if (bUpdating) {
                            QueueSpellDispel(apSpell, apCaster, pEffect);
                        }
                        else {
                            pEffect->Dispel(true);
                        }
                        return;
                    }
                }
                else if (bUpdating) {
                    QueueSpellDispel(apSpell, apCaster, pEffect);
                }
                else {
                    pEffect->Dispel(true);
                }
            }
        }
    }
}

void MagicTarget::Dispel(EffectItem* apEffect, MagicItem* apSpell, MagicCaster* apCaster) {
    BSSimpleList<ActiveEffect*>* pnext;
    for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); plist && !plist->IsEmpty(); plist = pnext) {
        ActiveEffect* peffect = plist->m_item;
        pnext = plist->m_pkNext;
        if (peffect && !peffect->bDone && peffect->pSpell == apSpell) {
            int32_t iCasterMatch;
            if (apCaster) {
                iCasterMatch = peffect->pCaster == apCaster;
            }
            else {
                iCasterMatch = 1;
            }
            if (iCasterMatch && peffect->pEffect == apEffect) {
                peffect->Dispel(true);
            }
        }
    }
}

bool MagicTarget::IsSpellTarget(MagicItem* apSpell, bool abIncludeInactiveSpells) {
    BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
    bool bfound = false;
    while (plist && !bfound) {
        ActiveEffect* pitem = plist->m_item;
        if (pitem &&
            pitem->pSpell == apSpell &&
            (abIncludeInactiveSpells || pitem->bActive) &&
            !pitem->pEffect->IsUsageMonitorEffect()) {
            bfound = true;
        }

        plist = plist->m_pkNext;
    }
    return bfound;
}

void MagicTarget::UpdateTarget(float afSecondsElapsed) {
    BSSimpleList<ActiveEffect*>* plistHead = GetActiveEffectList();
    BSSimpleList<ActiveEffect*>* plist = plistHead;
    if (MagicTargetIsActor()) {
        Actor* pactor = this ? static_cast<Actor*>(this) : nullptr;
        if (!pactor || !pactor->pCurrentProcess) {
            return;
        }
    }
    bUpdating = true;
    while (plist && !plist->IsEmpty()) {
        BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
        ActiveEffect* peffect = plist->m_item;
        if (peffect) {
            bool bdead = peffect->IsQueuedForRemoval();
            bool bshouldUpdate = !bdead;
            if (!bdead && MagicSystem::IsSpellTypePermanent(peffect->eSpellType)) {
                if (peffect->GetEffectSetting()->IsFlagSet(EffectSetting::RECOVER) && peffect->bStarted) {
                    bshouldUpdate = peffect->HasConditions() || peffect->GetDisplacementSpell();
                }
            }
            if (bshouldUpdate) {
                peffect->UpdateEffect(afSecondsElapsed);
            }
            if (!bdead && peffect->bDone) {
                MagicItem* pspell = peffect->pSpell;
                MagicTarget* ptarget = peffect->pTarget;
                peffect->SetQueuedForRemoval(1);
                if (peffect->bActive && !peffect->WasDispeled()) {
                    MagicTarget* pplayerTarget;
                    if (pPlayer) {
                        pplayerTarget = &static_cast<MagicTarget&>(
                            static_cast<Actor&>(static_cast<Character&>(*pPlayer))
                        );
                    }
                    else {
                        pplayerTarget = nullptr;
                    }
                    if (ptarget == pplayerTarget &&
                        pspell->GetSpellType() == MagicSystem::POTION &&
                        ptarget->IsLastEffect(pspell, peffect)) {
                        char cMessage[268];
                        bool bshowMessage = true;
                        if (ptarget->IsAddicted(pspell, true)) {
                            if (pPlayer->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
                                bshowMessage = false;
                            }
                            else {
                                BSsprintf(cMessage, 260, sChemsWithdrawal.String(), pspell->GetFullName());
                            }
                        }
                        else {
                            BSsprintf(cMessage, 260, sChemsWornOff.String(), pspell->GetFullName());
                        }
                        if (bshowMessage) {
                            Interface::ShowMessage(cMessage, 0, nullptr, "UIHealthChemsWearOff", 2.0f, false);
                            AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(pspell);
                            if (ppotion && ppotion->GetAddictionItem()) {
                                TESForm* pDefaultObject = BGSDefaultObjectManager::GetDefaultObject(
                                    DO_DRUG_EXPIRE_IMAGE_SPACE
                                );
                                TESImageSpaceModifier* pwearOffMod = dynamic_cast<TESImageSpaceModifier*>(
                                    pDefaultObject
                                );
                                if (pwearOffMod) {
                                    ImageSpaceModifierInstanceForm::TriggerIfNotActive(pwearOffMod, 1.0f, nullptr);
                                }
                            }
                        }
                    }
                }
            }

            if (peffect->IsQueuedForRemoval()) {
                if (plist == plistHead) {
                    pnext = plist;
                }
                RemoveEffect(peffect, true);
            }
        }
        plist = pnext;
    }
    bUpdating = false;
    DispelQueuedSpells();
}

Actor* MagicTarget::GetTargetAsActor() {
    if (MagicTargetIsActor()) {
        return static_cast<Actor*>(this);
    }
    return nullptr;
}

// The list walk advances after the effect is processed.
bool MagicTarget::HasRecoverableActorValueEffect(ActorValue::Index aeWhich) {
    bool bRecoverable = false;
    Actor* pActor = MagicTargetIsActor() ? static_cast<Actor*>(this) : nullptr;
    if (!pActor || aeWhich == -1 || aeWhich == ActorValue::ACTOR_VALUE_COUNT) {
        return false;
    }
    for (BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList(); pIter && !bRecoverable;) {
        bool bEmpty;
        bEmpty = pIter->IsEmpty();
        if (bEmpty) {
            break;
        }
        ActiveEffect* pEffect;
        pEffect = pIter->GetItem();
        if (pEffect && pEffect->GetEffectItem()->GetActorValueIndex() == aeWhich) {
            bool bUnconditionalAbility = false;
            if (pEffect->GetSpellType() == MagicSystem::ABILITY) {
                bool bHasConditions;
                bHasConditions = pEffect->HasConditions();
                bUnconditionalAbility = !bHasConditions;
            }
            bool bSkip = pActor->IsPc() &&
                bUnconditionalAbility &&
                pEffect->GetEffectSetting()->IsFlagSet(EffectSetting::RECOVER);
            if (!bSkip && pEffect->GetEffectSetting()->IsFlagSet(EffectSetting::RECOVER)) {
                bRecoverable = true;
            }
        }
        pIter = pIter->GetNext();
    }
    return bRecoverable;
}

uint32_t MagicTarget::GetMagicTargetFormID() {
    if (MagicTargetIsActor()) {
        return static_cast<Actor*>(this)->iFormID;
    }
    NonActorMagicTarget* pNonActorTarget = static_cast<NonActorMagicTarget*>(this);
    return pNonActorTarget->GetTargetStatsObject()->iFormID;
}

void MagicTarget::Dispel_4(int32_t aiSpellType) {
    if (aiSpellType == 1) {
        TESObjectREFR* ptargetRef = GetTargetStatsObject();
        Actor* ptargetActor = dynamic_cast<Actor*>(ptargetRef);
        if (ptargetActor) {
            bool bagain;
            do {
                BSSimpleList<SpellItem*>* pspells = &ptargetActor->GetActorBase()->spellList;
                bagain = false;
                while (pspells && !pspells->IsEmpty()) {
                    BSSimpleList<SpellItem*>* pnext = pspells->m_pkNext;
                    SpellItem* pitem = pspells->m_item;
                    if (pitem && pitem->GetSpellType() == MagicSystem::DISEASE) {
                        ptargetActor->RemoveSpell(pitem);
                        bagain = true;
                        break;
                    }
                    pspells = pnext;
                }
            }
            while (bagain);
        }
    }
    else {
        BSSimpleList<ActiveEffect*>* pnext;
        for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); plist && !plist->IsEmpty(); plist = pnext) {
            ActiveEffect* peffect = plist->m_item;
            pnext = plist->m_pkNext;
            if (peffect && !peffect->bDone) {
                AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(peffect->pSpell);
                bool bpoisonPotion = ppotion ? ppotion->CanBePoison() : false;
                if (peffect->pSpell->GetSpellType() == aiSpellType || (bpoisonPotion && aiSpellType == 5)) {
                    peffect->Dispel(false);
                }
            }
        }
    }
}

void MagicTarget::Dispel(EffectArchetypes::ArchetypeID aeID, MagicCaster* apCaster) {
    BSSimpleList<ActiveEffect*>* pCurrent = GetActiveEffectList();
    while (pCurrent && !pCurrent->IsEmpty()) {
        bool bCasterCheck;
        ActiveEffect* pEffect = pCurrent->m_item;
        if (apCaster) {
            bCasterCheck = apCaster == pEffect->GetCaster();
        }
        else {
            bCasterCheck = true;
        }
        if (pEffect && !pEffect->bDone && pEffect->GetEffectSetting()->GetEffectArchetype() == aeID && bCasterCheck) {
            pEffect->Dispel(true);
            RemoveEffect(pEffect, false);
        }
        else {
            pCurrent = pCurrent->m_pkNext;
        }
    }
}

bool MagicTarget::CheckAddiction(AlchemyItem* apSpell) {
    float faddictionChance = apSpell ? apSpell->data.fAddictionChance : 0.0f;
    if (!apSpell || !apSpell->GetAddictionItem() || faddictionChance == 0.0) {
        return false;
    }
    Actor* pactor = GetTargetAsActor();
    if (!pactor) {
        return false;
    }
    EffectItem* pusageMonitor = apSpell->GetUsageMonitorEffect();
    if (pusageMonitor) {
        float fusageMonitor = GetSpellUsageCount(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)));
        if (fAddictionUsageMonitorThreshold.Float() >= static_cast<double>(fusageMonitor)) {
            return false;
        }
        faddictionChance = (fusageMonitor - fAddictionUsageMonitorThreshold.Float()) / 100.0;
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_ADDICTION_CHANCE, pactor, &faddictionChance);
    int32_t idieRoll = Random() % 100;
    if (static_cast<double>(idieRoll) >= faddictionChance * 100.0) {
        EvaluateSpellConditions();
        return false;
    }

    MagicItem* pdisplacedSpell;
    bool bnewAddiction = pactor->IsPc() &&
        !pactor->IsAddicted(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)), true);
    if (pusageMonitor) {
        Dispel(pusageMonitor, &static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)), nullptr);
    }
    pactor->CastAddiction(apSpell->GetAddictionItem(), false);
    SpellItem* paddictionItem = apSpell->GetAddictionItem();
    if (paddictionItem) {
        pdisplacedSpell = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*paddictionItem));
    }
    else {
        pdisplacedSpell = nullptr;
    }
    SetDisplacementSpell(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)), pdisplacedSpell);
    if (bnewAddiction) {
        char cMessage[268];
        MiscStatManager::Increment(MiscStatManager::MS_ADDICTIONS);
        TESForm* pForm = TESForm::GetFormByNumericID(0x19482u);
        TESImageSpaceModifier* paddictionMod = dynamic_cast<TESImageSpaceModifier*>(pForm);
        BSsprintf(cMessage, 260, sChemsAddicted.String(), apSpell->GetFullName());
        Interface::ShowMessage(cMessage, 1, nullptr, "UIHealthChemsAddicted", 2.0f, false);
        ImageSpaceModifierInstanceForm::Trigger(paddictionMod, 1.0f, nullptr);
        return true;
    }

    return false;
}

bool MagicTarget::CheckAddEffect(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect) {
    TESObjectREFR* pTargetRef = GetTargetStatsObject();
    if (pTargetRef) {
        ExtraDataList* pExtra = pTargetRef->GetExtra();
        EffectSetting* pSetting = apActiveEffect->GetEffectItem()->GetEffectSetting();
        // The OnMagicEffectHit action flag.
        Script::SetActionFlag(pSetting, pExtra, SCRIPT_ACTION_ONMAGICEFFECTHIT);
    }
    bool bIgnoreResistance = apActiveEffect->GetEffectItem()->IsScriptEffect() && apSpell->IgnoreResistance();
    float fResistance;
    if (apSpell->GetSpellType() == MagicSystem::ABILITY || bIgnoreResistance) {
        fResistance = 1.0f;
    }
    else {
        fResistance = CheckResistance(apCaster, apSpell, apActiveEffect);
    }

    // A NaN resistance takes the adding path.
    if (fResistance <= 0.0) {
        if (!MagicSystem::GetShowMagicStats()) {
            return false;
        }
        const char* pTargetName;
        if (GetTargetStatsObject()) {
            pTargetName = GetTargetStatsObject()->GetFullName();
        }
        else {
            pTargetName = "{NULL}";
        }
        BSStringT<char> kName = apActiveEffect->GetEffectItem()->GetEffectName();
        const char* pEffectName = kName.String();
        const char* pSpellName = apSpell->GetFullName();
        // The resistance is also passed, although the format does not print it.
        Interface::PrintLine(
            "Target: %s FULLY RESISTED Spell: %s Effect: %s",
            pTargetName,
            pSpellName,
            pEffectName,
            static_cast<double>(fResistance)
        );
        return false;
    }

    if (IsInvulnerable() && apActiveEffect->GetEffectItem()->IsHostile()) {
        return false;
    }
    if (!apActiveEffect->CheckTarget(this)) {
        if (!MagicSystem::GetShowMagicStats()) {
            return false;
        }
        const char* pTargetName;
        if (GetTargetStatsObject()) {
            pTargetName = GetTargetStatsObject()->GetFullName();
        }
        else {
            pTargetName = "{NULL}";
        }
        BSStringT<char> kName = apActiveEffect->GetEffectItem()->GetEffectName();
        const char* pEffectName = kName.String();
        const char* pSpellName = apSpell->GetFullName();
        Interface::PrintLine("Invalid Target: Target: %s Spell: %s Effect: %s", pTargetName, pSpellName, pEffectName);
        return false;
    }

    // A usage monitor effect adds its magnitude to the one the target already has.
    EffectItem* pUsageMonitorEffect = apSpell->GetUsageMonitorEffect();
    if (apActiveEffect->GetEffectItem() == pUsageMonitorEffect) {
        if (IsAddicted(apSpell, true)) {
            return false;
        }
        BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList();
        bool bStacked = false;
        while (pIter) {
            bool bEmpty;
            bEmpty = pIter->IsEmpty();
            if (bEmpty || bStacked) {
                break;
            }
            ActiveEffect* pEffect;
            pEffect = pIter->GetItem();
            if (pEffect && pEffect->GetEffectItem() == pUsageMonitorEffect) {
                double dMagnitude = pEffect->GetMagnitude();
                float fMagnitude = static_cast<float>(apActiveEffect->GetMagnitude() + dMagnitude);
                pEffect->SetMagnitude(fMagnitude);
                pEffect->SetElapsedTime(0.0f);
                bStacked = true;
            }
            pIter = pIter->GetNext();
        }
        if (bStacked) {
            return true;
        }
    }

    if (apSpell->GetSpellType() == MagicSystem::ADDICTION) {
        Actor* pActor = GetTargetAsActor();
        if (pActor) {
            float fDuration = apActiveEffect->GetDuration();
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_ADDICTION_DURATION, pActor, &fDuration);
            apActiveEffect->SetDuration(fDuration);
        }
    }
    else if (
        apSpell->GetSpellType() == MagicSystem::POTION &&
        dynamic_cast<AlchemyItem*>(apSpell) &&
        !apActiveEffect->GetEffectItem()->IsHostile()
    ) {
        Actor* pActor = GetTargetAsActor();
        if (pActor) {
            float fDuration = apActiveEffect->GetDuration();
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_POSITIVE_CHEM_DURATION, pActor, &fDuration);
            apActiveEffect->SetDuration(fDuration);
        }
    }

    // The limb effects take the body part the stats menu targets.
    if (apActiveEffect->GetEffectSetting()->GetEffectArchetype() == EffectArchetypes::VALUE_AND_PARTS) {
        ValueAndConditionsEffect* pValueAndConditions = dynamic_cast<ValueAndConditionsEffect*>(apActiveEffect);
        if (pValueAndConditions) {
            pValueAndConditions->SetTargetBodyPart(StatsMenu::GetTargetedBodyPart());
        }
    }
    else if (apActiveEffect->GetEffectSetting()->GetEffectArchetype() == EffectArchetypes::LIMB_CONDITION) {
        LimbConditionEffect* pLimbCondition = dynamic_cast<LimbConditionEffect*>(apActiveEffect);
        if (pLimbCondition) {
            pLimbCondition->SetTargetBodyPart(StatsMenu::GetTargetedBodyPart());
        }
    }

    bool bDispel = true;
    bool bStackDuration = false;
    bool bResetElapsedTime = false;
    if (apActiveEffect->FromWornEnchantment()) {
        bDispel = false;
        // A worn enchantment replaces the effects that use the same associated armour or weapon.
        if (apActiveEffect->GetEffectSetting()->GetFlag(
                EffectSetting::USE_ASSOCIATED_WEAPON | EffectSetting::USE_ASSOCIATED_ARMOR
            )) {
            for (BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList(); pIter;) {
                bool bEmpty;
                bEmpty = pIter->IsEmpty();
                if (bEmpty) {
                    break;
                }
                ActiveEffect* pEffect;
                pEffect = pIter->GetItem();
                if (pEffect) {
                    EffectItem* pOtherItem = pEffect->GetEffectItem();
                    if (EffectItem::SharesAssociatedItem(apActiveEffect->GetEffectItem(), pOtherItem)) {
                        Dispel(pEffect->GetSpell(), nullptr, pEffect);
                        bDispel = false;
                    }
                }
                pIter = pIter->GetNext();
            }
        }
    }
    else {
        switch (apSpell->GetSpellType()) {
            case MagicSystem::POISON:
            case MagicSystem::POTION:
                // Only a duration above 0 stacks (NaN does not).
                bStackDuration = apActiveEffect->GetDuration() > 0.0;
                bDispel = false;
                break;
            case MagicSystem::ENCHANTMENT: {
                EnchantmentItem* pEnchantment = static_cast<EnchantmentItem*>(apSpell);
                if (pEnchantment->IsExtendDuration()) {
                    bDispel = false;
                    bResetElapsedTime = true;
                }
                TESBoundObject* pSource = apActiveEffect->GetEnchantmentSource();
                if (pSource && pSource->GetFormType() == BOOK_ID) {
                    bDispel = false;
                }
                break;
            }
            case MagicSystem::WORTCRAFT: bDispel = false; break;
            default: break;
        }
    }

    if (bDispel) {
        Dispel(apSpell, apCaster, apActiveEffect);
    }
    else if (bStackDuration) {
        if (StackEffectDuration(apSpell, apCaster, apActiveEffect)) {
            return true;
        }
    }
    else if (bResetElapsedTime && ResetEffectElapsedTime(apSpell, apCaster, apActiveEffect)) {
        return true;
    }

    ActiveEffect* pNewEffect = apActiveEffect->Copy();
    if (!pNewEffect) {
        return false;
    }
    pNewEffect->SetTarget(this);
    if (!dynamic_cast<IngredientItem*>(apSpell)) {
        pNewEffect->AdjustEffectiveness(fResistance);
    }
    if (!GetActiveEffectList()) {
        if (pNewEffect) {
            delete pNewEffect;
        }
        return false;
    }
    GetActiveEffectList()->Insert(pNewEffect, &MagicTarget::ActiveEffectListCompareFn);
    EffectAdded(pNewEffect);
    pNewEffect->Finalize();
    if (!MagicSystem::GetShowMagicStats()) {
        return true;
    }
    const char* pTargetName;
    if (pTargetRef) {
        pTargetName = pTargetRef->GetFullName();
    }
    else {
        pTargetName = "{NULL}";
    }
    uint32_t uiItemDuration = pNewEffect->GetEffectItem()->GetDuration();
    double dDuration = pNewEffect->GetDuration();
    int32_t iItemMagnitude = pNewEffect->GetEffectItem()->GetMagnitude();
    double dMagnitude = pNewEffect->GetMagnitude();
    const char* pSpellName = apSpell->GetFullName();
    BSStringT<char> kName = pNewEffect->GetEffectItem()->GetEffectName();
    const char* pEffectName = kName.String();
    Interface::PrintLine(
        "Effect %s from spell %s added to %s. Mag: %.2f/%d Dur: %.2f/%d",
        pEffectName,
        pSpellName,
        pTargetName,
        dMagnitude,
        iItemMagnitude,
        dDuration,
        uiItemDuration
    );
    return true;
}

bool MagicTarget::IsAddicted(MagicItem* apAddictedToSpell, bool abIncludeInactiveAddictionEffects) {
    BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
    int8_t bfound = 0;
    while (plist && !bfound) {
        ActiveEffect* peffect = plist->m_item;
        MagicItem* pspell = peffect ? peffect->pSpell : nullptr;
        if (pspell &&
            pspell->GetSpellType() == MagicSystem::ADDICTION &&
            (!apAddictedToSpell || peffect->ActiveEffect::GetDisplacementSpell() == apAddictedToSpell) &&
            (abIncludeInactiveAddictionEffects || peffect->bActive)) {
            bfound = 1;
        }

        plist = plist->m_pkNext;
    }
    return static_cast<bool>(bfound);
}

bool MagicTarget::IsLastEffect(MagicItem* apSpell, ActiveEffect* apLastEffect) {
    if (apLastEffect->GetEffectItem()->IsUsageMonitorEffect()) {
        return false;
    }
    BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
    int32_t icount = 0;
    while (plist) {
        ActiveEffect* peffect = plist->m_item;
        if (peffect && peffect->GetSpell() == apSpell && peffect->bActive) {
            if (!peffect->GetEffectItem()->IsUsageMonitorEffect() && peffect != apLastEffect) {
                ++icount;
            }
        }

        plist = plist->m_pkNext;
    }
    return icount == 0;
}

char MagicTarget::HasArchType(EffectArchetypes::ArchetypeID aeID, char abIncludeInactive) {
    BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
    char bfound = 0;
    while (plist && !bfound) {
        ActiveEffect* peffect = plist->m_item;
        if (peffect) {
            if (peffect->GetEffectSetting()->GetEffectArchetype() == aeID && (abIncludeInactive || peffect->bActive)) {
                bfound = 1;
            }
        }
        plist = plist->m_pkNext;
    }
    return bfound;
}

bool MagicTarget::ChecksForMagicEffect(EffectSetting* apEffectSetting) {
    BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList();
    bool bfound = false;
    while (plist && !plist->IsEmpty() && !bfound) {
        ActiveEffect* peffect = plist->m_item;
        if (peffect) {
            bfound = peffect->GetEffectSetting() == apEffectSetting;
        }
        plist = plist->m_pkNext;
    }
    return bfound;
}

double MagicTarget::GetSpellUsageCount(MagicItem* apSpell) {
    float fusageCount = 0.0f;
    EffectItem* pusageMonitor = apSpell ? apSpell->GetUsageMonitorEffect() : nullptr;
    if (pusageMonitor && IsSpellTarget(apSpell, true)) {
        for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            ActiveEffect* peffect = plist->m_item;
            if (peffect && peffect->pEffect == pusageMonitor) {
                fusageCount = peffect->fMagnitude + fusageCount;
            }
        }
    }

    return fusageCount;
}

void MagicTarget::QueueSpellDispel(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect) {
    SpellDispelData* pDispel = new SpellDispelData;
    pDispel->pSpell = apSpell;
    pDispel->pCaster = apCaster;
    pDispel->pActiveEffect = apActiveEffect;
    PostUpdateDispelList.AddHead(pDispel);
}

void MagicTarget::DispelQueuedSpells() {
    for (BSSimpleList<void*>* pList = &PostUpdateDispelList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        SpellDispelData* pData = static_cast<SpellDispelData*>(pList->m_item);
        if (pData) {
            Dispel(pData->pSpell, pData->pCaster, pData->pActiveEffect);
            ourFree(pData);
        }
    }
    PostUpdateDispelList.RemoveAll();
}

MagicTarget* MagicTarget::GetMagicTargetByNumericID(uint32_t aiFormID) {
    TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
    if (pForm && pForm->IsReference()) {
        return static_cast<TESObjectREFR*>(pForm)->GetMagicTarget();
    }

    return nullptr;
}

int32_t MagicTarget::ActiveEffectListCompareFn(ActiveEffect* apCompare1, ActiveEffect* apCompare2) {
    char cKey1[260];
    char cKey2[260];
    EffectItem* pitem = apCompare1->pEffect;
    float fDuration = pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION) ? 1000.0f : apCompare1->fDuration;
    float fMagnitude = pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE) ? 1000.0f : apCompare1->fMagnitude;
    std::sprintf(
        cKey1,
        "%c%.30s%.1f%.1f",
        65,
        pitem->GetEffectName().String(),
        static_cast<double>(fMagnitude),
        static_cast<double>(fDuration)
    );
    pitem = apCompare2->pEffect;
    fDuration = pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION) ? 1000.0f : apCompare2->fDuration;
    fMagnitude = pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE) ? 1000.0f : apCompare2->fMagnitude;
    std::sprintf(
        cKey2,
        "%c%.30s%.1f%.1f",
        65,
        pitem->GetEffectName().String(),
        static_cast<double>(fMagnitude),
        static_cast<double>(fDuration)
    );
    return _tcsicmp(cKey1, cKey2);
}

bool EnchantmentItem::IsExtendDuration() {
    return GetFlag(EXTEND_DURATION);
}

void ActiveEffect::SetDisplacementSpell(MagicItem* apObject) {
    pDisplacementSpell = apObject;
}

bool ActiveEffect::WasDispeled() {
    return (iFlags & WAS_DISPELED) != 0;
}

void ActiveEffect::SetElapsedTime(float afValue) {
    fElapsedSeconds = afValue;
}
