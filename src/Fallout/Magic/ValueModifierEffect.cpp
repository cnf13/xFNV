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
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::VALUE_MODIFIER,
    &ValueModifierEffect::Instantiate
);

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
    ValueModifierEffect* pCopy = new ValueModifierEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void ValueModifierEffect::Finalize() {
    Actor* pTargetActor;
    if (GetTarget()) {
        pTargetActor = GetTarget()->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    ActorValue::Index eActorValue = GetActorValue();
    if (pEffect->GetEffectSetting()->IsDetrimental(eActorValue)) {
        fMagnitude = -fMagnitude;
        if (pTargetActor && GetActorValue() == ActorValue::HEALTH) {
            if (GetEffectItem()->GetRange()) {
                Actor* pCasterActor;
                if (GetCaster()) {
                    pCasterActor = GetCaster()->GetCasterAsActor();
                }
                else {
                    pCasterActor = nullptr;
                }
                fMagnitude = pTargetActor->DifficultyLevelAdjustHealthModifier(fMagnitude, pCasterActor);
            }
        }
    }
    ActiveEffect::Finalize();
}

void ValueModifierEffect::Update(float afSecondsElapsed) {
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
        if (FromWornEnchantment() ||
            fDuration > 0.0 ||
            eSpellType == MagicSystem::ABILITY ||
            eSpellType == MagicSystem::DISEASE) {
            ModifyActorValue(pActor, fMagnitude * afSecondsElapsed, ActorValue::VALUE_NONE);
            if (eWhichActorValue == ActorValue::HEALTH && pActor == pPlayer) {
                MagicItem* pspell = pSpell;
                if (pspell && pspell->IsFood()) {
                    Warning("Using a food?");
                }
                else if (pspell->GetSpellType() == MagicSystem::POTION) {
                    Warning("Using a stimpak?\n");
                }
            }
            if (eWhichActorValue == ActorValue::ENDURANCE &&
                fMagnitude < 0.0 &&
                pActor->GetActorFloatValue(ActorValue::HEALTH) <= 1.0) {
                Actor* pcasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
                pActor->Kill(pcasterActor, 0.0f);
            }
        }
    }
}

uint16_t ValueModifierEffect::GetSaveSize(Actor* apActor) {
    return ActiveEffect::GetSaveSize(apActor);
}

void ValueModifierEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
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
    pActor->ModActorBaseValue(GetActorValue(), fBaseChange);
    float fTempChange = GetMagnitude();
    pActor->TempModActorValue(GetActorValue(), fTempChange, pCasterActor);
}

bool ValueModifierEffect::IsCausingDamage() {
    EffectSetting* pEffectSetting;
    if (eWhichActorValue == 16) {
        pEffectSetting = ActiveEffect::GetEffectSetting();
        if (pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
            pEffectSetting = ActiveEffect::GetEffectSetting();
            if (!pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
                return true;
            }
        }
    }
    return false;
}

void ValueModifierEffect::CopyData(ActiveEffect* apSource) {
    ActiveEffect::CopyData(apSource);
    ValueModifierEffect* pCopy = dynamic_cast<ValueModifierEffect*>(apSource);
    if (pCopy) {
        pCopy->eWhichActorValue = eWhichActorValue;
    }
}

// Accepts only a live actor: the target's actor, when there is one, must not be dead (IsDead(0)).
bool ValueModifierEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    if (!pActor) {
        return false;
    }
    bool bDead = pActor->IsDead(false);
    return !bDead;
}

void ValueModifierEffect::Start() {
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) &&
        (pEffect->GetDuration() ||
         FromWornEnchantment() ||
         eSpellType == MagicSystem::ABILITY ||
         eSpellType == MagicSystem::DISEASE)) {
        if (pActor == pPlayer && pEffect->GetDuration() > 0 && eWhichActorValue == ActorValue::HEALTH) {
            AlchemyItem* ppotion;
            double dPermanent = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
            float fPlayerHealthDiff = dPermanent - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
            MagicItem* pspell = pSpell;
            if (pspell->GetSpellType() == MagicSystem::POTION) {
                ppotion = dynamic_cast<AlchemyItem*>(pspell);
            }
            else {
                ppotion = nullptr;
            }
            if (ppotion && ppotion->IsFlagSet(AlchemyItem::FOOD_ITEM)) {
                float fMaxRestored = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
                double dRestored = BSMIN(fMaxRestored, fPlayerHealthDiff);
                MiscStatManager::ModVal(MiscStatManager::MS_FOOD_HEALTH_RESTORED, static_cast<int32_t>(dRestored));
            }

            if (ppotion && ppotion->IsFlagSet(AlchemyItem::MEDICINE)) {
                float fMaxRestored = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
                double dRestored = BSMIN(fMaxRestored, fPlayerHealthDiff);
                MiscStatManager::ModVal(MiscStatManager::MS_STIMPAK_HEALTH_RESTORED, static_cast<int32_t>(dRestored));
            }
        }
    }
    else {
        fMagnitude = CheckAdjustNoNegativeMagnitude(fMagnitude);
        if (eWhichActorValue == ActorValue::HEALTH && pActor == pPlayer) {
            AlchemyItem* ppotion;
            double dPermanent = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
            float fPlayerHealthDiff = dPermanent - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
            MagicItem* pspell = pSpell;
            if (pspell->GetSpellType() == MagicSystem::POTION) {
                ppotion = dynamic_cast<AlchemyItem*>(pspell);
            }
            else {
                ppotion = nullptr;
            }
            if (ppotion && ppotion->IsFlagSet(AlchemyItem::FOOD_ITEM)) {
                double dRestored = BSMIN(fMagnitude, fPlayerHealthDiff);
                MiscStatManager::ModVal(MiscStatManager::MS_FOOD_HEALTH_RESTORED, static_cast<int32_t>(dRestored));
            }

            if (ppotion && ppotion->IsFlagSet(AlchemyItem::MEDICINE)) {
                double dRestored = BSMIN(fMagnitude, fPlayerHealthDiff);
                MiscStatManager::ModVal(MiscStatManager::MS_STIMPAK_HEALTH_RESTORED, static_cast<int32_t>(dRestored));
            }
        }
        if (eWhichActorValue == ActorValue::ENDURANCE &&
            fMagnitude < 0.0 &&
            pActor->GetActorFloatValue(ActorValue::HEALTH) <= 1.0) {
            Actor* pcasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
            pActor->Kill(pcasterActor, 0.0f);
        }
        ModifyActorValue(pActor, fMagnitude, ActorValue::VALUE_NONE);
    }
    if (eWhichActorValue == ActorValue::RADIATION_RADS &&
        pEffect->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
        if (pActor && pActor->pCurrentProcess) {
            pActor->pCurrentProcess->AddRadiationMagicDelta(fMagnitude);
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
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pActor == pPlayer && pEffect && pEffect->GetDuration() > 0 && eWhichActorValue == ActorValue::HEALTH) {
        // Keeps the permanent value as a double across the second call.
        double dPermanent = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
        float fPlayerHealthDiff = dPermanent - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
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
                MiscStatManager::MS_FOOD_HEALTH_RESTORED,
                static_cast<int32_t>(BSMIN(fRestored, fPlayerHealthDiff))
            );
        }
        if (pPotion && pPotion->IsFlagSet(AlchemyItem::MEDICINE)) {
            float fRestored = static_cast<double>(pEffect->GetDuration()) * fMagnitude;
            MiscStatManager::ModVal(
                MiscStatManager::MS_STIMPAK_HEALTH_RESTORED,
                static_cast<int32_t>(BSMIN(fRestored, fPlayerHealthDiff))
            );
        }
    }
}

void ValueModifierEffect::Finish() {
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        if (pActor && fMagnitude > 0.0) {
            float fadjust = CheckAdjustNoNegativeMagnitude(-fMagnitude) + fMagnitude;
            if (GetActorValue() == ActorValue::HEALTH && fMagnitude + 1.0 >= pActor->GetHealth() + fadjust) {
                fadjust = fadjust + 1.0;
            }
            if (fadjust != 0.0) {
                if (pCaster) {
                    Actor* pCasterActor = pCaster->GetCasterAsActor();
                    pActor->DamageModActorValue(eWhichActorValue, fadjust, pCasterActor);
                }
                else {
                    pActor->DamageModActorValue(eWhichActorValue, fadjust, nullptr);
                }
            }
        }

        ModifyActorValue(pActor, -fMagnitude, ActorValue::VALUE_NONE);
        if (pActor &&
            pActor->IsDead(false) &&
            (eWhichActorValue == ActorValue::HEALTH || eWhichActorValue == ActorValue::ENDURANCE)) {
            float fHealth = static_cast<float>(pActor->GetActorValue(ActorValue::HEALTH));
            if (fHealth > 0.0) {
                pActor->DamageModActorValue(ActorValue::HEALTH, -fHealth, static_cast<Actor*>(nullptr));
            }
        }
    }
    if (eWhichActorValue == ActorValue::RADIATION_RADS &&
        pEffect->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
        if (pActor && pActor->pCurrentProcess) {
            pActor->pCurrentProcess->RemoveRadiationMagicDelta(fMagnitude);
        }

        if (pActor == pPlayer) {
            pPlayer->ReportRadiationChange();
        }
    }
}

void ValueModifierEffect::ModifyActorValue(Actor* apActor, float afValue, ActorValue::Index aeActorValue) {
    if (apActor) {
        ActorValue::Index eactorValue = aeActorValue == ActorValue::VALUE_NONE ? eWhichActorValue : aeActorValue;
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
            if (apActor->IsPc() && babilityCheck && pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
                apActor->PermanentModActorValue(eactorValue, afValue, nullptr);
            }
            else {
                Actor* pcasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
                Actor* pAttacker = pcasterActor;
                if (!pEffect->data.iRange || !pEffect->IsHostile()) {
                    pAttacker = nullptr;
                }
                if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
                    apActor->TempModActorValue(eactorValue, afValue, pAttacker);
                }
                else {
                    if (pAttacker &&
                        afValue < 0.0 &&
                        eactorValue == ActorValue::HEALTH &&
                        pAttacker->IsPc() &&
                        apActor != pAttacker &&
                        apActor->pCurrentProcess &&
                        !apActor->GetEssential()) {
                        apActor->pCurrentProcess->ModTrackedDamage(pPlayer, -afValue);
                    }

                    apActor->DamageModActorValue(eactorValue, afValue, pAttacker);
                    if (afValue < 0.0 && eactorValue == ActorValue::HEALTH) {
                        TaskQueueInterface::GetInstance()->QueueScriptFunctionCall(0x116Cu, apActor, -afValue);
                    }
                }
            }
        }
    }
}

void ValueModifierEffect::ShowAttributeReductionMessage() {
    MagicTarget* pPlayerTarget = static_cast<MagicTarget*>(pPlayer);
    if (pTarget == pPlayerTarget) {
        if ((ActorValue::IsSPECIAL(pEffect->GetActorValueIndex()) ||
             ActorValue::IsSkill(pEffect->GetActorValueIndex())) &&
            pEffect->IsHostile()) {
            if (pSpell->GetSpellType() != MagicSystem::ABILITY &&
                pEffect->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
                const char* pcVerb;
                char msg[208];
                bool bDrained = pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) || FromWornEnchantment();
                msg[207] = bDrained;
                if (bDrained) {
                    pcVerb = sAttributeDrained.String();
                }
                else {
                    pcVerb = sAttributeDamaged.String();
                }
                const char* pcName = ActorValue::GetActorValueName(pEffect->GetActorValueIndex());
                const char* pcYour = sYour.String();
                std::sprintf(msg, "%s %s %s", pcYour, pcName, pcVerb);
                Interface::ShowMessage(msg, 0, nullptr, nullptr, 2.0f, false);
            }
        }
    }
}

float ValueModifierEffect::CheckAdjustNoNegativeMagnitude(float afMagnitude) {
    if (HasConditions()) {
        return afMagnitude;
    }

    if ((ActorValue::IsSPECIAL(GetActorValue()) ||
         ActorValue::IsDerivedAttribute(GetActorValue()) ||
         ActorValue::IsSkill(GetActorValue())) &&
        afMagnitude <= 0.0) {
        Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
        ActorValue::Index eActorValue = GetActorValue();
        float fNewValue = pTargetActor->GetActorFloatValue(eActorValue) + afMagnitude;
        if (fNewValue < 0.0) {
            return afMagnitude - fNewValue;
        }
    }

    return afMagnitude;
}

ActiveEffect* ValueModifierEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new ValueModifierEffect(apCaster, apSpell, apEffect);
}
