#include "Fallout/Magic/ValueModifierEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"

// Whether the value-modifier archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::VALUE_MODIFIER, &ValueModifierEffect::Instantiate);

ValueModifierEffect::ValueModifierEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ActiveEffect(apCaster, apItem, apEffect) {
    EffectSetting* psetting = apEffect->pEffectSetting;
    if (psetting->IsAssociatedActorValueFixed()) {
        eWhichActorValue = psetting->data.eAssociatedActorValue;
    }
    else {
        eWhichActorValue = apEffect->GetActorValueIndex();
    }
    if (apEffect->pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
        fMagnitude = 1.0f;
    }
}

ValueModifierEffect::~ValueModifierEffect() {}

ActiveEffect* ValueModifierEffect::Copy() {
    ValueModifierEffect* v2;
    void* v4 = ourMalloc(sizeof(ValueModifierEffect));
    if (v4) {
        v2 = ::new (v4) ValueModifierEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    CopyData(v2);
    return v2;
}

void ValueModifierEffect::Finalize() {
    Actor* TargetAsActor;
    if (GetTarget()) {
        MagicTarget* Unk24 = GetTarget();
        TargetAsActor = Unk24->GetTargetAsActor();
    }
    else {
        TargetAsActor = 0;
    }
    ActorValue::Index v5 = GetActorValue();
    EffectSetting* Time = pEffect->GetEffectSetting();
    if (Time->IsDetrimental(v5)) {
        fMagnitude = -fMagnitude;
        if (TargetAsActor) {
            if (GetActorValue() == ActorValue::HEALTH) {
                EffectItem* CastingType = GetEffectItem();
                if (CastingType->GetRange()) {
                    Actor* CasterAsActor;
                    if (GetCaster()) {
                        MagicCaster* ProcessLevel = GetCaster();
                        CasterAsActor = ProcessLevel->GetCasterAsActor();
                    }
                    else {
                        CasterAsActor = 0;
                    }
                    fMagnitude = TargetAsActor->DifficultyLevelAdjustHealthModifier(fMagnitude, CasterAsActor);
                }
            }
        }
    }
    ActiveEffect::Finalize();
}

void ValueModifierEffect::Update(float afSecondsElapsed) {
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        Actor* pActor;
        if (pTarget) {
            pActor = pTarget->GetTargetAsActor();
        }
        else {
            pActor = 0;
        }
        if (FromWornEnchantment() ||
            (fDuration > 0.0 || (eSpellType == MagicSystem::ABILITY || eSpellType == MagicSystem::DISEASE))) {
            float v3 = fMagnitude * afSecondsElapsed;
            ModifyActorValue(pActor, v3, ActorValue::VALUE_NONE);
            if (eWhichActorValue == ActorValue::HEALTH && pActor == pPlayer) {
                MagicItem* pspell = pSpell;
                if (pspell && pspell->IsFood()) {
                    Warning("Using a food?");
                }
                else {
                    if (pspell->GetSpellType() == MagicSystem::POTION) {
                        Warning("Using a stimpak?\n");
                    }
                }
            }
            if (eWhichActorValue == ActorValue::ENDURANCE && fMagnitude < 0.0 &&
                pActor->GetActorFloatValue(ActorValue::HEALTH) <= 1.0) {
                Actor* pcasterActor;
                if (pCaster) {
                    pcasterActor = pCaster->GetCasterAsActor();
                }
                else {
                    pcasterActor = 0;
                }
                pActor->Kill(pcasterActor, 0.0f);
            }
        }
    }
}

uint16_t ValueModifierEffect::GetSaveSize(Actor* apActor) {
    return (uint16_t)((int16_t)ActiveEffect::GetSaveSize(apActor));
}

void ValueModifierEffect::SaveGame(Actor* a2) {
    ActiveEffect::SaveGame(a2);
}

void ValueModifierEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
}

// Saves of versions 0x5F to 0x61 lost the player's recovering value changes: they are put back as a base value
// change and a temporary one.
void ValueModifierEffect::FinishInitLoadGame(Actor* apActor) {
    ActiveEffect::FinishInitLoadGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x5F || pSaveLoadGameOLD->GetCurrentVersion() >= 0x62) {
        return;
    }
    Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
    if (!pActor || !pActor->IsPc() || !FromWornEnchantment()) {
        return;
    }
    if (!pEffect->GetEffectSetting()->IsFlagSet(EffectSetting::RECOVER)) {
        return;
    }
    Actor* pCasterActor = GetCaster() ? GetCaster()->GetCasterAsActor() : nullptr;
    float fBaseChange = -GetMagnitude();
    pActor->ModActorBaseValue(static_cast<ActorValue::Index>(GetActorValue()), fBaseChange);
    float fTempChange = GetMagnitude();
    pActor->TempModActorValue(static_cast<ActorValue::Index>(GetActorValue()), fTempChange, pCasterActor);
}

bool ValueModifierEffect::IsCausingDamage() {
    EffectSetting* pEffectSetting;
    uint8_t bFalse = 0;
    if (eWhichActorValue == 16) {
        pEffectSetting = ActiveEffect::GetEffectSetting();
        if (pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
            pEffectSetting = ActiveEffect::GetEffectSetting();
            if (!pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
                return (bool)1;
            }
        }
    }
    return (bool)bFalse;
}

void ValueModifierEffect::CopyData(ActiveEffect* apSource) {
    ActiveEffect::CopyData(apSource);
    ValueModifierEffect* v3 = dynamic_cast<ValueModifierEffect*>(apSource);
    if (v3) {
        v3->eWhichActorValue = eWhichActorValue;
    }
}

// Accepts only a live actor: the target's actor, when there is one, must not be dead (IsDead(0)).
bool ValueModifierEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pActor;
    if (apTarget) {
        pActor = apTarget->GetTargetAsActor();
    }
    else {
        pActor = nullptr;
    }
    if (!pActor) {
        return false;
    }
    bool bDead = pActor->IsDead(false);
    return static_cast<uint8_t>(bDead) == 0;
}

void ValueModifierEffect::Start() {
    Actor* pActor;
    if (pTarget) {
        pActor = pTarget->GetTargetAsActor();
    }
    else {
        pActor = 0;
    }
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) &&
        (pEffect->GetDuration() ||
         (FromWornEnchantment() || (eSpellType == MagicSystem::ABILITY || eSpellType == MagicSystem::DISEASE)))) {
        if (pActor == pPlayer && pEffect->GetDuration() > 0 && eWhichActorValue == ActorValue::HEALTH) {
            AlchemyItem* ppotion_2;
            double v10 = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
            float fPlayerHealthDiff_2 = v10 - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
            MagicItem* pspell_2 = pSpell;
            if (pspell_2->GetSpellType() == MagicSystem::POTION) {
                ppotion_2 = dynamic_cast<AlchemyItem*>(pspell_2);
            }
            else {
                ppotion_2 = 0;
            }
            if (ppotion_2) {
                if (ppotion_2->IsFlagSet(AlchemyItem::FOOD_ITEM)) {
                    float afA = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
                    double v3 = BSMIN(afA, fPlayerHealthDiff_2);
                    MiscStatManager::ModVal(MiscStatManager::MS_FOOD_HEALTH_RESTORED, static_cast<int32_t>(v3));
                }
            }
            if (ppotion_2) {
                if (ppotion_2->IsFlagSet(AlchemyItem::MEDICINE)) {
                    float v7 = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
                    double v4 = BSMIN(v7, fPlayerHealthDiff_2);
                    MiscStatManager::ModVal(MiscStatManager::MS_STIMPAK_HEALTH_RESTORED, static_cast<int32_t>(v4));
                }
            }
        }
    }
    else {
        fMagnitude = CheckAdjustNoNegativeMagnitude(fMagnitude);
        if (eWhichActorValue == ActorValue::HEALTH && pActor == pPlayer) {
            AlchemyItem* ppotion;
            double v13 = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
            float fPlayerHealthDiff = v13 - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
            MagicItem* pspell = pSpell;
            if (pspell->GetSpellType() == MagicSystem::POTION) {
                ppotion = dynamic_cast<AlchemyItem*>(pspell);
            }
            else {
                ppotion = 0;
            }
            if (ppotion) {
                if (ppotion->IsFlagSet(AlchemyItem::FOOD_ITEM)) {
                    double v1 = BSMIN(fMagnitude, fPlayerHealthDiff);
                    MiscStatManager::ModVal(MiscStatManager::MS_FOOD_HEALTH_RESTORED, static_cast<int32_t>(v1));
                }
            }
            if (ppotion) {
                if (ppotion->IsFlagSet(AlchemyItem::MEDICINE)) {
                    double v2 = BSMIN(fMagnitude, fPlayerHealthDiff);
                    MiscStatManager::ModVal(MiscStatManager::MS_STIMPAK_HEALTH_RESTORED, static_cast<int32_t>(v2));
                }
            }
        }
        if (eWhichActorValue == ActorValue::ENDURANCE && fMagnitude < 0.0 &&
            pActor->GetActorFloatValue(ActorValue::HEALTH) <= 1.0) {
            Actor* pcasterActor;
            if (pCaster) {
                pcasterActor = pCaster->GetCasterAsActor();
            }
            else {
                pcasterActor = 0;
            }
            pActor->Kill(pcasterActor, 0.0f);
        }
        ModifyActorValue(pActor, fMagnitude, ActorValue::VALUE_NONE);
    }
    if (eWhichActorValue == ActorValue::RADIATION_RADS &&
        pEffect->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
        if (pActor) {
            if (pActor->pCurrentProcess) {
                BaseProcess* afDamage = pActor->pCurrentProcess;
                float zero = fMagnitude;
                afDamage->AddRadiationMagicDelta(zero);
            }
        }
        if (pActor == pPlayer) {
            pPlayer->ReportRadiationChange();
        }
    }
    ShowAttributeReductionMessage();
}

// The health a food or medicine potion restores over its duration counts towards the player's misc stats, capped by
// the health the player lacks.
void ValueModifierEffect::Continue() {
    Actor* pActor;
    if (pTarget) {
        pActor = pTarget->GetTargetAsActor();
    }
    else {
        pActor = nullptr;
    }
    if (pActor == pPlayer && pEffect && pEffect->GetDuration() > 0 && eWhichActorValue == ActorValue::HEALTH) {
        // Keeps the permanent value as a double across the second call.
        double fPermanent = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
        float fPlayerHealthDiff = fPermanent - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
        AlchemyItem* pPotion;
        MagicItem* pSpellItem = pSpell;
        if (pSpellItem->GetSpellType() == MagicSystem::POTION) {
            pPotion = dynamic_cast<AlchemyItem*>(pSpellItem);
        }
        else {
            pPotion = nullptr;
        }
        if (pPotion && pPotion->IsFlagSet(AlchemyItem::FOOD_ITEM)) {
            float fRestored = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
            MiscStatManager::ModVal(
                MiscStatManager::MS_FOOD_HEALTH_RESTORED, static_cast<int32_t>(BSMIN(fRestored, fPlayerHealthDiff))
            );
        }
        if (pPotion && pPotion->IsFlagSet(AlchemyItem::MEDICINE)) {
            float fRestored = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
            MiscStatManager::ModVal(
                MiscStatManager::MS_STIMPAK_HEALTH_RESTORED, static_cast<int32_t>(BSMIN(fRestored, fPlayerHealthDiff))
            );
        }
    }
}

void ValueModifierEffect::Finish() {
    Actor* pActor;
    if (pTarget) {
        pActor = pTarget->GetTargetAsActor();
    }
    else {
        pActor = 0;
    }
    if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        if (pActor) {
            if (fMagnitude > 0.0) {
                float fadjust = CheckAdjustNoNegativeMagnitude(-fMagnitude) + fMagnitude;
                if (GetActorValue() == ActorValue::HEALTH && fMagnitude + 1.0 >= pActor->GetHealth() + fadjust) {
                    fadjust = fadjust + 1.0;
                }
                if (fadjust != 0.0) {
                    if (pCaster) {
                        Actor* CasterAsActor = pCaster->GetCasterAsActor();
                        pActor->DamageModActorValue(eWhichActorValue, fadjust, CasterAsActor);
                    }
                    else {
                        pActor->DamageModActorValue(eWhichActorValue, fadjust, 0);
                    }
                }
            }
        }
        ModifyActorValue(pActor, -fMagnitude, ActorValue::VALUE_NONE);
        if (pActor) {
            if (pActor->IsDead(0) &&
                (eWhichActorValue == ActorValue::HEALTH || eWhichActorValue == ActorValue::ENDURANCE)) {
                float v9 = static_cast<float>(pActor->GetActorValue(ActorValue::HEALTH));
                if (v9 > 0.0) {
                    pActor->DamageModActorValue(ActorValue::HEALTH, static_cast<float>(-v9), static_cast<Actor*>(0));
                }
            }
        }
    }
    if (eWhichActorValue == ActorValue::RADIATION_RADS &&
        pEffect->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
        if (pActor) {
            if (pActor->pCurrentProcess) {
                BaseProcess* CurrentAIProcess = pActor->pCurrentProcess;
                float afMagnitudea = fMagnitude;
                CurrentAIProcess->RemoveRadiationMagicDelta(afMagnitudea);
            }
        }
        if (pActor == pPlayer) {
            pPlayer->ReportRadiationChange();
        }
    }
}

void ValueModifierEffect::ModifyActorValue(Actor* apActor, float afValue, ActorValue::Index aeActorValue) {
    if (apActor) {
        ActorValue::Index eactorValue;
        if (aeActorValue == ActorValue::VALUE_NONE) {
            eactorValue = eWhichActorValue;
        }
        else {
            eactorValue = aeActorValue;
        }
        if (eactorValue == ActorValue::VALUE_NONE) {
            if (afValue != 0.0) {
                Warning(
                    "ValueModifierEffect tried to modify an actor value without specifying which actor value to "
                    "modify\n"
                );
            }
        }
        else {
            bool babilityCheck = eSpellType == MagicSystem::ABILITY && !HasConditions();
            if (apActor->IsPc() && (babilityCheck && pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER))) {
                apActor->PermanentModActorValue(eactorValue, afValue, 0);
            }
            else {
                Actor* pcasterActor;
                if (pCaster) {
                    pcasterActor = pCaster->GetCasterAsActor();
                }
                else {
                    pcasterActor = 0;
                }
                Actor* v10 = pcasterActor;
                if (!pEffect->data.iRange || !pEffect->IsHostile()) {
                    v10 = 0;
                }
                if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
                    apActor->TempModActorValue(eactorValue, afValue, v10);
                }
                else {
                    if (v10) {
                        if (afValue < 0.0 && eactorValue == ActorValue::HEALTH && v10->IsPc() && apActor != v10) {
                            if (apActor->pCurrentProcess) {
                                if (!apActor->GetEssential()) {
                                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                                    CurrentAIProcess->ModTrackedDamage(pPlayer, -afValue);
                                }
                            }
                        }
                    }
                    apActor->DamageModActorValue(eactorValue, afValue, v10);
                    if (afValue < 0.0 && eactorValue == ActorValue::HEALTH) {
                        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x116Cu, apActor, -afValue);
                    }
                }
            }
        }
    }
}

void ValueModifierEffect::ShowAttributeReductionMessage() {
    MagicTarget* v9;
    if (pPlayer) {
        v9 = &static_cast<MagicTarget&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
    }
    else {
        v9 = 0;
    }
    if (pTarget == v9) {
        ActorValue::Index v2;
        ActorValue::Index ActorValueIndex = pEffect->GetActorValueIndex();
        if (ActorValue::IsSPECIAL(ActorValueIndex) || (v2 = pEffect->GetActorValueIndex(), ActorValue::IsSkill(v2))) {
            if (pEffect->IsHostile()) {
                MagicItem* Spell = pSpell;
                if (Spell->GetSpellType() != MagicSystem::ABILITY &&
                    pEffect->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
                    const char* v6;
                    char msg[208];
                    bool v7 = pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) || FromWornEnchantment();
                    msg[207] = v7;
                    if (v7) {
                        v6 = sAttributeDrained.String();
                    }
                    else {
                        v6 = sAttributeDamaged.String();
                    }
                    ActorValue::Index v3 = pEffect->GetActorValueIndex();
                    const char* time = ActorValue::GetActorValueName(v3);
                    const char* v4 = sYour.String();
                    std::sprintf(msg, "%s %s %s", v4, time, v6);
                    Interface::ShowMessage(msg, 0, 0, 0, 2.0f, 0);
                }
            }
        }
    }
}

float ValueModifierEffect::CheckAdjustNoNegativeMagnitude(float afMagnitude) {
    if (HasConditions()) {
        return afMagnitude;
    }
    else {
        ActorValue::Index v4;
        ActorValue::Index v5;
        ActorValue::Index v3 = GetActorValue();
        if (ActorValue::IsSPECIAL(v3) ||
            ((v4 = GetActorValue(), ActorValue::IsDerivedAttribute(v4)) ||
             (v5 = GetActorValue(), ActorValue::IsSkill(v5)))) {
            if (afMagnitude <= 0.0) {
                Actor* TargetAsActor;
                if (pTarget) {
                    TargetAsActor = pTarget->GetTargetAsActor();
                }
                else {
                    TargetAsActor = 0;
                }
                ActorValue::Index v6 = GetActorValue();
                float v10 = TargetAsActor->GetActorFloatValue(v6) + afMagnitude;
                if (v10 < 0.0) {
                    return afMagnitude - v10;
                }
            }
        }
        return afMagnitude;
    }
}

ActiveEffect* ValueModifierEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(ValueModifierEffect));
    if (v5) {
        return ::new (v5) ValueModifierEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
