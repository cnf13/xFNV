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
    EffectItem* CastingType = apActiveEffect->GetEffectItem();
    if (!CastingType->GetRange()) {
        return CheckAddEffect(apCaster, apSpell, apActiveEffect);
    }
    EffectItem* v6 = apActiveEffect->GetEffectItem();
    bool v8 = v6->IsScriptEffect() && apSpell->IgnoreResistance();
    if (v8 || apSpell->GetNoAbsorbOrReflect()) {
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
    BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList();
    while (ecx0 && !ecx0->IsEmpty()) {
        ActiveEffect* m_item = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if (m_item) {
            MagicItem* Spell = m_item->pSpell;
            if (Spell->GetSpellType() == MagicSystem::SPELL ||
                (Spell->GetSpellType() == MagicSystem::POWER ||
                 (Spell->GetSpellType() == MagicSystem::LESSER_POWER ||
                  (Spell->GetSpellType() == MagicSystem::POTION || Spell->GetSpellType() == MagicSystem::WORTCRAFT)))) {
                m_item->Dispel(abNow);
            }
            else {
                EnchantmentItem* penchantment = dynamic_cast<EnchantmentItem*>(Spell);
                if ((penchantment) && (penchantment->GetCastingType() == MagicSystem::CAST_ONCE)) {
                    m_item->Dispel(abNow);
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
        if (!GetTargetStatsObject()->Get3D() || !GetTargetStatsObject()->GetParentCell() ||
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
    if ((int32_t)iNumHWThreads.UInt() > 1 && AILinearTaskThreadManager::QRunningThreads()) {
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
    while (!bresult && (plist && !plist->IsEmpty())) {
        ActiveEffect* peffect = plist->m_item;
        BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
        bool v9 = peffect && peffect->pSpell == apSpell;
        bool bspellCheck = v9;
        if (bdoAlcoholCheck) {
            AlchemyItem* pthisPotion = dynamic_cast<AlchemyItem*>(apSpell);
            if (pthisPotion) {
                bspellCheck = pthisPotion->eEquipType == BGSEquipType::ET_ALCOHOL;
            }
        }
        if ((peffect) && (!peffect->bDone && bspellCheck)) {
            int32_t v8;
            if (apCaster) {
                v8 = peffect->pCaster == apCaster;
            }
            else {
                v8 = 1;
            }
            if (v8) {
                int32_t v7;
                if (apActiveEffect) {
                    v7 = !peffect->pEffect->MatchesEffect(apActiveEffect->pEffect);
                }
                else {
                    v7 = 1;
                }
                if (v7) {
                    double LastTimeProcessed = peffect->fDuration;
                    float v5 = apActiveEffect->fDuration + LastTimeProcessed;
                    peffect->SetDuration(v5);
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
    for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); !bresult && (plist && !plist->IsEmpty());
         plist = pnext) {
        ActiveEffect* peffect = plist->m_item;
        pnext = plist->m_pkNext;
        if ((peffect) && (!peffect->bDone && peffect->pSpell == apSpell)) {
            int32_t v6;
            if (apCaster) {
                v6 = peffect->pCaster == apCaster;
            }
            else {
                v6 = 1;
            }
            if (v6) {
                int32_t v5;
                if (apActiveEffect) {
                    v5 = !peffect->pEffect->MatchesEffect(apActiveEffect->pEffect);
                }
                else {
                    v5 = 1;
                }
                if (v5) {
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
        BSSimpleList<ActiveEffect*>* ecx0a = GetActiveEffectList();
        while (ecx0a && !ecx0a->IsEmpty()) {
            ActiveEffect* v3 = ecx0a->m_item;
            ecx0a = ecx0a->m_pkNext;
            if ((v3) && (!v3->bDone && v3->GetEnchantmentSource() == apEnchantmentSource)) {
                v3->Dispel(abNow);
            }
        }
    }
}

void MagicTarget::Dispel() {
    BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList();
    while (ecx0 && !ecx0->IsEmpty()) {
        ActiveEffect* v1 = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if (v1) {
            v1->Dispel(true);
        }
    }
}

bool MagicTarget::HasDamageHealthEffect() {
    BSSimpleList<ActiveEffect*>* pIter = GetActiveEffectList();
    bool bIsDetrimental = false;
    while (pIter && (!pIter->IsEmpty() && !bIsDetrimental)) {
        ActiveEffect* pEffect = pIter->m_item;
        if (pEffect) {
            EffectSetting* EffectSetting_ = pEffect->GetEffectSetting();
            if (EffectSetting_->IsAssociatedActorValueUsed()) {
                EffectItem* CastingType = pEffect->pEffect;
                if (CastingType->GetActorValueIndex() == ActorValue::HEALTH) {
                    EffectSetting* v3 = pEffect->GetEffectSetting();
                    bIsDetrimental = v3->IsFlagSet(EffectSetting::DETRIMENTAL);
                }
            }
        }
        pIter = pIter->m_pkNext;
    }
    return bIsDetrimental;
}

void MagicTarget::Dispel(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect) {
    BSSimpleList<ActiveEffect*>* FormHealth_0;
    for (BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList(); ecx0 && !ecx0->IsEmpty(); ecx0 = FormHealth_0) {
        ActiveEffect* pEffect = ecx0->m_item;
        FormHealth_0 = ecx0->m_pkNext;
        if ((pEffect) &&
            (!pEffect->bDone && pEffect->pSpell == apSpell && (!apCaster || pEffect->pCaster == apCaster))) {
            int32_t v5;
            if (apActiveEffect) {
                TESBoundObject* EnchantmentSource = apActiveEffect->GetEnchantmentSource();
                v5 = EnchantmentSource == pEffect->GetEnchantmentSource();
            }
            else {
                v5 = 1;
            }
            if (v5) {
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
                else {
                    if (bUpdating) {
                        QueueSpellDispel(apSpell, apCaster, pEffect);
                    }
                    else {
                        pEffect->Dispel(true);
                    }
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
        if ((peffect) && (!peffect->bDone && peffect->pSpell == apSpell)) {
            int32_t v4;
            if (apCaster) {
                v4 = peffect->pCaster == apCaster;
            }
            else {
                v4 = 1;
            }
            if ((v4) && (peffect->pEffect == apEffect)) {
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
        if ((pitem) &&
            (pitem->pSpell == apSpell && (abIncludeInactiveSpells || pitem->bActive) &&
             !pitem->pEffect->IsUsageMonitorEffect())) {
            bfound = true;
        }

        plist = plist->m_pkNext;
    }
    return bfound;
}

void MagicTarget::UpdateTarget(float afSecondsElapsed) {
    Actor* pactor;
    TESObjectREFR* ptarget;
    Actor* pactor_2;
    BSSimpleList<ActiveEffect*>* plistHead = GetActiveEffectList();
    BSSimpleList<ActiveEffect*>* plist = plistHead;
    if (!MagicTargetIsActor() ||
        ((ptarget = GetTargetStatsObject(), !this) ? pactor = nullptr : pactor = static_cast<Actor*>(this),
         (pactor_2 = pactor) != nullptr && pactor_2->pCurrentProcess)) {
        bUpdating = true;
        while (plist && !plist->IsEmpty()) {
            BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
            ActiveEffect* peffect = plist->m_item;
            if (peffect) {
                bool bdead = peffect->IsQueuedForRemoval();
                bool bshouldUpdate_2 = !bdead;
                if (!bdead && MagicSystem::IsSpellTypePermanent(peffect->eSpellType)) {
                    EffectSetting* EffectSetting_ = peffect->GetEffectSetting();
                    if (EffectSetting_->IsFlagSet(EffectSetting::RECOVER) && peffect->bStarted) {
                        bool bshouldUpdate = peffect->HasConditions() || peffect->GetDisplacementSpell();
                        bshouldUpdate_2 = bshouldUpdate;
                    }
                }
                if (bshouldUpdate_2) {
                    peffect->UpdateEffect(afSecondsElapsed);
                }
                if ((!bdead) && (peffect->bDone)) {
                    MagicItem* pspell = peffect->pSpell;
                    MagicTarget* ptarget_2 = peffect->pTarget;
                    peffect->SetQueuedForRemoval(1);
                    if ((peffect->bActive) && (!peffect->WasDispeled())) {
                        MagicTarget* v4;
                        if (pPlayer) {
                            v4 = &static_cast<MagicTarget&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
                        }
                        else {
                            v4 = nullptr;
                        }
                        if (ptarget_2 == v4 && pspell->GetSpellType() == MagicSystem::POTION &&
                            ptarget_2->IsLastEffect(pspell, peffect)) {
                            char cMessage[268];
                            bool bshowMessage = true;
                            if (ptarget_2->IsAddicted(pspell, true)) {
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
                                if ((ppotion) && (ppotion->GetAddictionItem())) {
                                    TESForm* DefaultObject =
                                        BGSDefaultObjectManager::GetDefaultObject(DO_DRUG_EXPIRE_IMAGE_SPACE);
                                    TESImageSpaceModifier* pwearOffMod =
                                        dynamic_cast<TESImageSpaceModifier*>(DefaultObject);
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
            bool bSkip = pActor->IsPc() && bUnconditionalAbility &&
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

void MagicTarget::Dispel_4(int32_t a1) {
    if (a1 == 1) {
        TESObjectREFR* v2 = GetTargetStatsObject();
        Actor* ptargetActor = dynamic_cast<Actor*>(v2);
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
        BSSimpleList<ActiveEffect*>* pnext_2;
        for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); plist && !plist->IsEmpty(); plist = pnext_2) {
            ActiveEffect* peffect = plist->m_item;
            pnext_2 = plist->m_pkNext;
            if ((peffect) && (!peffect->bDone)) {
                bool bpoisonPotion;
                AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(peffect->pSpell);
                if (ppotion) {
                    bpoisonPotion = ppotion->CanBePoison();
                }
                else {
                    bpoisonPotion = false;
                }
                MagicItem* Spell = peffect->pSpell;
                if (Spell->GetSpellType() == a1 || (bpoisonPotion && a1 == 5)) {
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
    float faddictionChance;
    float faddictionChance_2;
    if (apSpell) {
        faddictionChance = apSpell->data.fAddictionChance;
    }
    else {
        faddictionChance = 0.0f;
    }
    faddictionChance_2 = faddictionChance;
    if (!apSpell || (!apSpell->GetAddictionItem() || faddictionChance_2 == 0.0)) {
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
        faddictionChance_2 = (fusageMonitor - fAddictionUsageMonitorThreshold.Float()) / 100.0;
    }
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_ADDICTION_CHANCE, pactor, &faddictionChance_2);
    int32_t idieRoll = Random() % 100;
    if (!(static_cast<double>(idieRoll) >= faddictionChance_2 * 100.0)) {
        MagicItem* apDisplacedSpell;
        bool v7 = pactor->IsPc() &&
            !pactor->IsAddicted(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)), true);
        if (pusageMonitor) {
            Dispel(pusageMonitor, &static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)), nullptr);
        }
        SpellItem* time = apSpell->GetAddictionItem();
        pactor->CastAddiction(time, false);
        SpellItem* AddictionItem = apSpell->GetAddictionItem();
        if (AddictionItem) {
            apDisplacedSpell = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*AddictionItem));
        }
        else {
            apDisplacedSpell = nullptr;
        }
        SetDisplacementSpell(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apSpell)), apDisplacedSpell);
        if (v7) {
            char msg[268];
            MiscStatManager::Increment(MiscStatManager::MS_ADDICTIONS);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(0x19482u);
            TESImageSpaceModifier* paddictionMod = dynamic_cast<TESImageSpaceModifier*>(FormByNumericID);
            BSsprintf(msg, 260, sChemsAddicted.String(), apSpell->GetFullName());
            Interface::ShowMessage(msg, 1, nullptr, "UIHealthChemsAddicted", 2.0f, false);
            ImageSpaceModifierInstanceForm::Trigger(paddictionMod, 1.0f, nullptr);
            return true;
        }

        return false;
    }

    EvaluateSpellConditions();
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
        // GetEffectName fills kName; the local is destroyed, not the pointer returned.
        FNVD_INLINED_LOCAL(BSStringT<char>, kName, std::nullopt, std::nullopt);
        BSStringT<char>* pName = apActiveEffect->GetEffectItem()->GetEffectName(&kName);
        const char* pEffectName = pName->String();
        const char* pSpellName = apSpell->GetFullName();
        // The resistance is also passed, although the format does not print it.
        Interface::PrintLine(
            "Target: %s FULLY RESISTED Spell: %s Effect: %s", pTargetName, pSpellName, pEffectName,
            static_cast<double>(fResistance)
        );
        // SWEEP-INLINE 0x4037D0
        kName.~BSStringT<char>();
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
        FNVD_INLINED_LOCAL(BSStringT<char>, kName, std::nullopt, std::nullopt);
        BSStringT<char>* pName = apActiveEffect->GetEffectItem()->GetEffectName(&kName);
        const char* pEffectName = pName->String();
        const char* pSpellName = apSpell->GetFullName();
        Interface::PrintLine("Invalid Target: Target: %s Spell: %s Effect: %s", pTargetName, pSpellName, pEffectName);
        // SWEEP-INLINE 0x4037D0
        kName.~BSStringT<char>();
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
        // EXE-INLINED 0x822B40
        Actor* pActor = MagicTargetIsActor() ? static_cast<Actor*>(this) : nullptr;
        if (pActor) {
            float fDuration = apActiveEffect->GetDuration();
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_ADDICTION_DURATION, pActor, &fDuration);
            apActiveEffect->SetDuration(fDuration);
        }
    }
    else if (
        (apSpell->GetSpellType() == MagicSystem::POTION) &&
        (dynamic_cast<AlchemyItem*>(apSpell) && !apActiveEffect->GetEffectItem()->IsHostile())
    ) {
        // EXE-INLINED 0x822B40
        Actor* pActor = MagicTargetIsActor() ? static_cast<Actor*>(this) : nullptr;
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
    FNVD_INLINED_LOCAL(BSStringT<char>, kName, std::nullopt, std::nullopt);
    BSStringT<char>* pName = pNewEffect->GetEffectItem()->GetEffectName(&kName);
    const char* pEffectName = pName->String();
    Interface::PrintLine(
        "Effect %s from spell %s added to %s. Mag: %.2f/%d Dur: %.2f/%d", pEffectName, pSpellName, pTargetName,
        dMagnitude, iItemMagnitude, dDuration, uiItemDuration
    );
    // SWEEP-INLINE 0x4037D0
    kName.~BSStringT<char>();
    return true;
}

bool MagicTarget::IsAddicted(MagicItem* apAddictedToSpell, bool abIncludeInactiveAddictionEffects) {
    MagicItem* formFlags;
    ActiveEffect* v5;
    BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList();
    int8_t v7 = 0;
    while (ecx0 && (!v7)) {
        v5 = ecx0->m_item;
        if (v5) {
            formFlags = v5->pSpell;
        }
        else {
            formFlags = nullptr;
        }
        if ((formFlags) &&
            (((((int32_t)formFlags->GetSpellType()) == 10) &&
              ((!apAddictedToSpell) || (v5->ActiveEffect::GetDisplacementSpell() == apAddictedToSpell))) &&
             (abIncludeInactiveAddictionEffects || v5->bActive))) {
            v7 = 1;
        }

        ecx0 = ecx0->m_pkNext;
    }
    return (bool)v7;
}

bool MagicTarget::IsLastEffect(MagicItem* apSpell, ActiveEffect* apLastEffect) {
    EffectItem* CastingType = apLastEffect->GetEffectItem();
    if (CastingType->IsUsageMonitorEffect()) {
        return false;
    }
    BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList();
    int32_t v9 = 0;
    while (ecx0) {
        ActiveEffect* v7 = ecx0->m_item;
        if ((v7) && (v7->GetSpell() == apSpell) && (v7->bActive)) {
            EffectItem* v5 = v7->GetEffectItem();
            if (!v5->IsUsageMonitorEffect() && v7 != apLastEffect) {
                ++v9;
            }
        }

        ecx0 = ecx0->m_pkNext;
    }
    return v9 == 0;
}

char MagicTarget::HasArchType(EffectArchetypes::ArchetypeID a1, char a2) {
    BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList();
    char v7 = 0;
    while (ecx0 && !v7) {
        ActiveEffect* m_item = ecx0->m_item;
        if (m_item) {
            EffectSetting* EffectSetting_ = m_item->GetEffectSetting();
            if (EffectSetting_->GetEffectArchetype() == a1 && (a2 || m_item->bActive)) {
                v7 = 1;
            }
        }
        ecx0 = ecx0->m_pkNext;
    }
    return v7;
}

bool MagicTarget::ChecksForMagicEffect(EffectSetting* a1) {
    BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList();
    bool v5 = false;
    while (ecx0 && (!ecx0->IsEmpty() && !v5)) {
        ActiveEffect* v3 = ecx0->m_item;
        if (v3) {
            v5 = v3->GetEffectSetting() == a1;
        }
        ecx0 = ecx0->m_pkNext;
    }
    return v5;
}

double MagicTarget::GetSpellUsageCount(MagicItem* a1) {
    EffectItem* UsageMonitorEffect;
    float v7 = 0.0f;
    if (a1) {
        UsageMonitorEffect = a1->GetUsageMonitorEffect();
    }
    else {
        UsageMonitorEffect = nullptr;
    }
    if ((UsageMonitorEffect) && (IsSpellTarget(a1, true))) {
        for (BSSimpleList<ActiveEffect*>* ecx0 = GetActiveEffectList(); ecx0 && !ecx0->IsEmpty();
             ecx0 = ecx0->m_pkNext) {
            ActiveEffect* m_item = ecx0->m_item;
            if ((m_item) && (m_item->pEffect == UsageMonitorEffect)) {
                v7 = m_item->fMagnitude + v7;
            }
        }
    }

    return v7;
}

void MagicTarget::QueueSpellDispel(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect) {
    SpellDispelData* pDispel;
    SpellDispelData* pMemory = static_cast<SpellDispelData*>(ourMalloc(sizeof(SpellDispelData)));
    if (pMemory) {
        pDispel = ::new (pMemory) SpellDispelData;
    }
    else {
        pDispel = nullptr;
    }
    pDispel->pSpell = apSpell;
    pDispel->pCaster = apCaster;
    pDispel->pActiveEffect = apActiveEffect;
    void* pItem = pDispel;
    PostUpdateDispelList.AddHead(pItem);
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
    TESForm* formByNumericID = TESForm::GetFormByNumericID(aiFormID);
    if (formByNumericID && formByNumericID->IsReference()) {
        return static_cast<TESObjectREFR*>(formByNumericID)->GetMagicTarget();
    }

    return nullptr;
}

int32_t MagicTarget::ActiveEffectListCompareFn(ActiveEffect* apCompare1, ActiveEffect* apCompare2) {
    float v4;
    float v5;
    float SequenceWeight;
    float LastTimeProcessed;
    FNVD_INLINED_LOCAL(BSStringT<char>, v9, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSStringT<char>, apString, std::nullopt, std::nullopt);
    char _String1[260];
    char _String2[260];
    EffectItem* pitem = apCompare1->pEffect;
    if (pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
        LastTimeProcessed = 1000.0f;
    }
    else {
        LastTimeProcessed = apCompare1->fDuration;
    }
    if (pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
        SequenceWeight = 1000.0f;
    }
    else {
        SequenceWeight = apCompare1->fMagnitude;
    }
    BSStringT<char>* EffectName = pitem->GetEffectName(&apString);
    std::sprintf(
        _String1, "%c%.30s%.1f%.1f", 65, EffectName->String(), static_cast<double>(SequenceWeight),
        static_cast<double>(LastTimeProcessed)
    );
    apString.~BSStringT<char>();
    pitem = apCompare2->pEffect;
    if (pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
        v5 = 1000.0f;
    }
    else {
        v5 = apCompare2->fDuration;
    }
    if (pitem->pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
        v4 = 1000.0f;
    }
    else {
        v4 = apCompare2->fMagnitude;
    }
    BSStringT<char>* v3 = pitem->GetEffectName(&v9);
    std::sprintf(_String2, "%c%.30s%.1f%.1f", 65, v3->String(), static_cast<double>(v4), static_cast<double>(v5));
    v9.~BSStringT<char>();
    return _tcsicmp(_String1, _String2);
}

bool EnchantmentItem::IsExtendDuration() {
    return GetFlag(2u);
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
