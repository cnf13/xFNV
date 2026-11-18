#include "Fallout/Magic/MagicCaster.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkAabbPhantom.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicCaster.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "SDK/MSVC/rtti.h"

MagicCaster::MagicCaster() : pLight(nullptr), eState(MagicCaster::NO_SPELL) {}

MagicCaster::~MagicCaster() {
    delete pLight;
    pLight = nullptr;
}

// MiddleHighProcess::CastAbility and PlayerCharacter::CastAbility call it directly.
void MagicCaster::CastAbility(SpellItem* apSpell, bool abLoadCast) {
    if (apSpell->GetSpellType() == MagicSystem::ABILITY) {
        if (apSpell) {
            SetCurrentSpell(apSpell);
        }
        else {
            SetCurrentSpell(nullptr);
        }
        FindTargets(0.0f, nullptr, abLoadCast, 1.0f, false);
    }
}

void MagicCaster::CastAddiction(SpellItem* apSpell, bool abLoadCast) {
    if (apSpell->GetSpellType() == MagicSystem::ADDICTION) {
        if (apSpell) {
            SetCurrentSpell(apSpell);
        }
        else {
            SetCurrentSpell(nullptr);
        }
        FindTargets(0.0f, nullptr, abLoadCast, 1.0f, false);
    }
}

void MagicCaster::CastRadiation(SpellItem* apSpell, bool abLoadCast) {
    if (apSpell) {
        SetCurrentSpell(static_cast<MagicItem*>(apSpell));
    }
    else {
        SetCurrentSpell(nullptr);
    }
    FindTargets(0.0, nullptr, abLoadCast, 1.0, false);
}

void MagicCaster::CastSpellImmediate(
    MagicItem* apSpell,
    bool abLoadCast,
    MagicTarget* apDesiredTarget,
    float afEffectivenessMult,
    bool abAdjustOnlyHostileEffectiveness
) {
    if (apDesiredTarget) {
        SetDesiredTarget(apDesiredTarget);
    }
    SetCurrentSpell(apSpell);
    FindTargets(0.0, nullptr, abLoadCast, afEffectivenessMult, abAdjustOnlyHostileEffectiveness);
}

// MiddleHighProcess::TransferDisease and PlayerCharacter::TransferDisease call it directly.
void MagicCaster::TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) {
    if ((apSpell->GetSpellType() == MagicSystem::DISEASE) && (apDesiredTarget)) {
        if (apSpell) {
            SetCurrentSpell(apSpell);
        }
        else {
            SetCurrentSpell(nullptr);
        }
        FindTargets(0.0f, nullptr, abLoadCast, 1.0f, false);
    }
}

void MagicCaster::CastWornEnchantment(MagicItem* apSpell, TESBoundObject* apSource, bool abLoadCast) {
    if (apSpell) {
        SetCurrentSpell(apSpell);
        FindTargets(0.0, apSource, abLoadCast, 1.0, false);
    }
}

MagicTarget* MagicCaster::GetTouchTarget() {
    TESObjectREFR* pcaster;
    double freach;
    float fdelta;
    Actor* pactor = GetCasterAsActor();
    if (pactor) {
        double v10 = pactor->GetReach();
        freach = pactor->GetScale() * v10;
    }
    else {
        freach = fCombatDistance.Float();
    }
    float v19 = freach;
    if (pactor) {
        pcaster = (TESObjectREFR*)pactor;
    }
    else {
        pcaster = GetCasterStatsObject();
    }
    Actor* pclosestTarget = nullptr;
    float fclosestDelta = 3.4028235e+38f;
    if (static_cast<double>(v19) >= CombatUtilities::GetDistanceBetweenFacingActors(pcaster, pPlayer, false)) {
        bool binStrikeAngle;
        fdelta = 3.4028235e+38f;
        if (pactor) {
            binStrikeAngle = CombatUtilities::IsTargetWithinStrikeAngle(pactor, pPlayer, &fdelta);
        }
        else {
            binStrikeAngle = true;
        }
        if (binStrikeAngle && fclosestDelta >= static_cast<double>(fdelta)) {
            pclosestTarget = (Actor*)pPlayer;
            fclosestDelta = fdelta;
        }
    }
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t i = 0;; ++i) {
        Actor* pActor;
        uint32_t ProcessLevelTail = parray->GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pmob = parray->GetItem(i);
        if (pmob && pmob->IsActor()) {
            pActor = static_cast<Actor*>(pmob);
        }
        else {
            pActor = nullptr;
        }
        Actor* ptargetActor = pActor;
        BSSimpleList<Actor*>* ptempfollower = nullptr;
        while (ptargetActor) {
            if ((ptargetActor != pcaster && !ptargetActor->IsDead(false)) && (ptargetActor->Get3D())) {
                double DistanceBetweenFacingActors =
                    CombatUtilities::GetDistanceBetweenFacingActors(pcaster, ptargetActor, false);
                if (v19 >= DistanceBetweenFacingActors) {
                    bool binStrikeAngle_2;
                    float fdelta_2;
                    fdelta_2 = 3.4028235e+38f;
                    if (pactor) {
                        binStrikeAngle_2 = CombatUtilities::IsTargetWithinStrikeAngle(pactor, ptargetActor, &fdelta_2);
                    }
                    else {
                        binStrikeAngle_2 = true;
                    }
                    if (binStrikeAngle_2 && fclosestDelta >= static_cast<double>(fdelta_2)) {
                        pclosestTarget = ptargetActor;
                        fclosestDelta = fdelta_2;
                    }
                }
            }

            if (ptempfollower && ptempfollower->m_item) {
                ptargetActor = dynamic_cast<Character*>(ptempfollower->m_item);
                ptempfollower = ptempfollower->m_pkNext;
                while (ptargetActor && (!ptargetActor->pCurrentProcess || ptargetActor->pCurrentProcess->eLevel)) {
                    ptargetActor = nullptr;
                    if (ptempfollower) {
                        ptargetActor = dynamic_cast<Character*>(ptempfollower->m_item);
                    }
                    if (ptempfollower) {
                        ptempfollower = ptempfollower->m_pkNext;
                    }
                }
            }
            else {
                ptargetActor = nullptr;
            }
        }
    }
    if (pclosestTarget) {
        return static_cast<MagicTarget*>(pclosestTarget);
    }

    return nullptr;
}

void MagicCaster::StartAim() {
    EndAim();
}

bool MagicCaster::CheckCast(
    MagicItem* apSpell,
    float* afEffectStrength,
    MagicItem::CannotCastReason* apReason,
    bool abUseBaseMagicka
) {
    if (afEffectStrength) {
        *afEffectStrength = (float)1.0;
    }
    if (apReason) {
        *apReason = MagicItem::NOCAST_OK;
    }
    return true;
}

bool MagicCaster::TargetSelf(ActiveEffect* apActiveEffect) {
    bool bResult = false;
    MagicTarget* pTarget = dynamic_cast<MagicTarget*>(this);
    if (pTarget) {
        MagicItem* pSpell = GetCurrentSpell();
        return AddTarget(pTarget, NiPoint3_ZERO, pSpell, apActiveEffect, false);
    }
    return bResult;
}

ActiveEffect* MagicCaster::CreateActiveEffect(MagicItem* apSpell, EffectItem* apEffectItem, TESBoundObject* apSource) {
    return ActiveEffect::Activate(this, apSpell, apEffectItem, apSource);
}

bool MagicCaster::CastSpell(MagicItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) {
    if (apSpell->IsDisabled()) {
        SetState(MagicCaster::ERR_SPELL_DISABLED, "CAST: Spell is disabled");
        return false;
    }

    MagicSystem::SpellType v8 = apSpell->GetSpellType();
    switch (v8) {
        case MagicSystem::DISEASE: {
            SpellItem* v6 = dynamic_cast<SpellItem*>(apSpell);
            TransferDisease(v6, apDesiredTarget, abLoadCast);
            return true;
        }
        case MagicSystem::ABILITY: {
            SpellItem* v5 = dynamic_cast<SpellItem*>(apSpell);
            CastAbility(v5, abLoadCast);
            return true;
        }
        case MagicSystem::ADDICTION: {
            SpellItem* v7 = dynamic_cast<SpellItem*>(apSpell);
            CastAddiction(v7, abLoadCast);
            return true;
        }
        default: {
            if (GetCurrentSpell()) {
                SetState(MagicCaster::ERR_ALREADY_CASTING, "CAST: Already Casting Spell");
                return false;
            }

            SetCurrentSpell(apSpell);
            SetDesiredTarget(apDesiredTarget);
            if (apSpell->HasTargetEffect() && apDesiredTarget) {
                StartAim();
            }
            else {
                StartCast();
            }
            return true;
        }
    }
}

Actor* MagicCaster::GetCasterAsActor() {
    TESObjectREFR* pActor = GetCasterStatsObject();
    if ((!pActor) || (!pActor->IsActor())) {
        return nullptr;
    }
    if (this) {
        return static_cast<Actor*>(this);
    }

    return nullptr;
}

// Picks the touch target (the desired target instead when there is none, dropped when it is out of reach), then
// activates each effect of the current spell: its effectiveness is scaled by the spell type (medicine and survival
// skills for potions, potion formulas for ingredients), and it goes to the caster (self range), to the touch target and
// the targets in its area (touch range, or a poison), or nowhere. Then explodes the area effect or the actor, fires the
// target effects as a projectile, uses the spell's skill, releases the cast model, clears the current spell, target and
// state and rolls a potion's addiction.
void MagicCaster::FindTargets(
    float afEffectStrength,
    TESBoundObject* apSource,
    bool abLoadCast,
    float afEffectivenessMult,
    bool abAdjustOnlyHostileEffectiveness
) {
    MagicItem* pCurrSpell = GetCurrentSpell();
    if (!pCurrSpell) {
        return;
    }
    SetState(FIND_TARGETS, "Find Targets");
    TESObjectREFR* pCasterRef = GetCasterStatsObject();
    Actor* pCaster = GetCasterAsActor();
    MagicTarget* pTarget = nullptr;
    bool bPotion = pCurrSpell->GetSpellType() == MagicSystem::POTION;
    bool bPoison = bPotion && pCurrSpell->CanBePoison();
    AlchemyItem* pAlchemyItem = bPotion ? dynamic_cast<AlchemyItem*>(pCurrSpell) : nullptr;
    if (pCurrSpell->HasTouchEffect() || bPoison) {
        // Ammo and weapon (when-strikes) enchantments touch what they hit, not a picked target.
        bool bNoTouchPick = apSource ? apSource->GetFormType() == AMMO_ID : false;
        if (!bNoTouchPick && pCurrSpell->GetSpellType() == MagicSystem::ENCHANTMENT) {
            EnchantmentItem* pEnchantment = dynamic_cast<EnchantmentItem*>(pCurrSpell);
            if (pEnchantment) {
                bNoTouchPick = pEnchantment->GetCastingType() == MagicSystem::WHEN_STRIKES;
            }
        }
        if (!bNoTouchPick && (pCaster || !GetDesiredTarget())) {
            pTarget = GetTouchTarget();
        }
        if (!pTarget) {
            pTarget = GetDesiredTarget();
            if (pTarget && !apSource && !bNoTouchPick && pTarget->GetTargetStatsObject()) {
                float fReach = fMagicDefaultTouchDistance.Float();
                if ((pCasterRef->IsActor()) &&
                    (pCasterRef == pPlayer || !pTarget->GetTargetStatsObject()->IsActor() ||
                     pTarget->GetTargetStatsObject()->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING)) {
                    fReach = static_cast<Actor*>(pCasterRef)->GetReach();
                }

                fReach = BSMAX(1.0f, pCasterRef->GetScale()) * fReach;
                if (fReach <
                    CombatUtilities::GetDistanceBetweenFacingActors(
                        static_cast<Actor*>(pCasterRef), static_cast<Actor*>(pTarget->GetTargetStatsObject()), false
                    )) {
                    pTarget = nullptr;
                }
            }
        }
    }
    bool bReleaseModel = false;
    NiNode* pMagicNode = GetMagicNode();
    if (pMagicNode && pMagicNode->GetChildCount() && !apSource) {
        bReleaseModel = true;
    }
    Actor* pTargetActor = pTarget ? static_cast<Actor*>(pTarget->GetTargetStatsObject()) : nullptr;
    bool bSkillUsed = false;
    IngredientItem* pIngredient = dynamic_cast<IngredientItem*>(pCurrSpell);
    EffectItem* pCostliestSelfEffect = pCurrSpell->GetCostliestEffect(MagicSystem::SELF, false);
    EffectItem* pCostliestTouchEffect = pCurrSpell->GetCostliestEffect(MagicSystem::TOUCH, false);
    SpellItem* pSpellItem = dynamic_cast<SpellItem*>(pCurrSpell);
    BSSimpleList<MobileObject*> kTargetList;
    kTargetList.~BSSimpleList<MobileObject*>();
    ::new (&kTargetList) BSSimpleList<MobileObject*>;
    kTargetList.RemoveAll();
    // 0x80: the spell's area is centred on the caster.
    if (pTargetActor) {
        FindExplosionLOS(pCurrSpell, MagicSystem::TOUCH, pTargetActor->GetLocationOnReference(), &kTargetList);
    }
    else if (pCaster && pSpellItem && pSpellItem->GetFlag(0x80)) {
        FindExplosionLOS(pCurrSpell, MagicSystem::TOUCH, pCaster->GetLocationOnReference(), &kTargetList);
    }
    // Walks the effect list's nodes as EffectItemList pointers.
    EffectItemList* pEffectNode = pCurrSpell;
    while (pEffectNode) {
        EffectItem* pEffectItem = pEffectNode->GetItem();
        if (pEffectItem && pCurrSpell->GetSpellType() == MagicSystem::ENCHANTMENT && apSource &&
            !EnchantmentItem::IsValidEffectForEnchantment(apSource, nullptr, pEffectItem->GetEffectSetting(), false)) {
            pEffectItem = nullptr;
        }
        if (pEffectItem) {
            ActiveEffect* pActiveEffect = CreateActiveEffect(pCurrSpell, pEffectItem, apSource);
            float fBaseCost = pActiveEffect->GetEffectItem()->GetEffectSetting()->GetBaseCost();
            if (abLoadCast) {
                pActiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_NIF);
                pActiveEffect->SetFlag(ActiveEffect::NO_INTIAL_FLARE);
            }
            switch (pCurrSpell->GetSpellType()) {
                case MagicSystem::SPELL:
                    if (!abAdjustOnlyHostileEffectiveness || pActiveEffect->GetEffectItem()->IsHostile()) {
                        float fMult = GetMagicEffectivenessModifier(false, 0.0f) * afEffectivenessMult;
                        pActiveEffect->AdjustEffectiveness(fMult);
                    }
                    break;
                case MagicSystem::POTION:
                    if (pAlchemyItem && pAlchemyItem->IsFlagSet(AlchemyItem::MEDICINE)) {
                        float fMedicineMult = pCaster->GetMedicineEffectivenessMultiplier() * afEffectivenessMult;
                        if (pCaster->IsPc() &&
                            pActiveEffect->GetEffectItem()->GetActorValueIndex() == ActorValue::HEALTH) {
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pCaster, &fMedicineMult
                            );
                        }
                        pActiveEffect->AdjustEffectiveness(fMedicineMult);
                    }
                    if (pAlchemyItem && pAlchemyItem->IsFlagSet(AlchemyItem::FOOD_ITEM) &&
                        pActiveEffect->GetEffectItem()->GetActorValueIndex() != ActorValue::RADIATION_RADS) {
                        float fSurvivalMult = pCaster->GetSurvivalEffectivenessMultiplier() * afEffectivenessMult;
                        if (pCaster->IsPc() &&
                            pActiveEffect->GetEffectItem()->GetActorValueIndex() == ActorValue::HEALTH) {
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pCaster, &fSurvivalMult
                            );
                        }
                        pActiveEffect->AdjustEffectiveness(fSurvivalMult);
                    }
                    else if (
                        pAlchemyItem && pAlchemyItem->IsFlagSet(AlchemyItem::FOOD_ITEM) &&
                        pActiveEffect->GetEffectItem()->GetActorValueIndex() == ActorValue::RADIATION_RADS
                    ) {
                        float fRadiationMult = 1.0f;
                        BGSEntryPoint::HandleEntryPoint(
                            BGSEntryPoint::RADIATION_INGESTIBLE_BONUS, pCaster, &fRadiationMult
                        );
                        float fMult = afEffectivenessMult * fRadiationMult;
                        pActiveEffect->AdjustEffectiveness(fMult);
                    }
                    break;
                case MagicSystem::WORTCRAFT:
                    if (pIngredient && pIngredient->IsFood()) {
                        pActiveEffect->SetDuration(1.0f);
                    }
                    else {
                        EffectSetting* pEffectSetting = pActiveEffect->GetEffectSetting();
                        float fMagnitude = 0.0f;
                        float fDuration = 0.0f;
                        // Equipment 6, no hostile effect, no equipment qualities and no poison.
                        if (!pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION) &&
                            !pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
                            GamePlayFormulas::CalculatePotionDurationMagnitude(
                                fMagnitude, fDuration, fBaseCost, afEffectStrength, 6, false, 0.0f, 0.0f, 0.0f, false
                            );
                        }
                        else if (
                            !pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION) &&
                            pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)
                        ) {
                            GamePlayFormulas::CalculatePotionDuration(
                                fDuration, fBaseCost, afEffectStrength, 6, false, 0.0f, 0.0f, 0.0f, false
                            );
                        }
                        else if (
                            pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION) &&
                            !pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)
                        ) {
                            GamePlayFormulas::CalculatePotionMagnitude(
                                fMagnitude, fBaseCost, afEffectStrength, 6, false, 0.0f, 0.0f, 0.0f, false
                            );
                        }
                        fDuration = BSRoundToNearest(fDuration, 1.0f);
                        fMagnitude = BSRoundToNearest(fMagnitude, 1.0f);
                        pActiveEffect->SetDuration(fDuration);
                        pActiveEffect->SetMagnitude(fMagnitude);
                    }
                    break;
                default: break;
            }
            if (pEffectItem->GetRange() == MagicSystem::SELF && !bPoison) {
                if (pEffectItem != pCostliestSelfEffect) {
                    pActiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_NIF);
                    pActiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_SHADER);
                    pActiveEffect->SetFlag(ActiveEffect::NO_SOUND);
                }
                if (TargetSelf(pActiveEffect)) {
                    bSkillUsed = true;
                }
            }
            else if (pEffectItem->GetRange() == MagicSystem::TOUCH || bPoison) {
                // A poison's dominant effect is its costliest self effect.
                EffectItem* pDominantEffect = bPoison ? pCostliestSelfEffect : pCostliestTouchEffect;
                if (pEffectItem != pDominantEffect) {
                    pActiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_NIF);
                    pActiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_SHADER);
                    pActiveEffect->SetFlag(ActiveEffect::NO_SOUND);
                }
                // The area is tested with a signed compare.
                if (pTarget) {
                    if (AddTarget(pTarget, NiPoint3_ZERO, pCurrSpell, pActiveEffect, false)) {
                        bSkillUsed = true;
                    }
                    if (pEffectItem->GetArea() > 0 && pTargetActor) {
                        bool bAreaSkillUsed = false;
                        FindTargetsInArea(
                            pCurrSpell, pActiveEffect, pTargetActor->GetParentCell(),
                            pTargetActor->GetLocationOnReference(), pTarget, nullptr, &kTargetList, bAreaSkillUsed, 1.0f
                        );
                        if (bAreaSkillUsed) {
                            bSkillUsed = false;
                        }
                    }
                }
                else if (!bPoison && pSpellItem && pSpellItem->GetFlag(0x80) && pEffectItem->GetArea() > 0 && pCaster) {
                    bool bAreaSkillUsed = false;
                    FindTargetsInArea(
                        pCurrSpell, pActiveEffect, pCaster->GetParentCell(), pCaster->GetLocationOnReference(), pTarget,
                        nullptr, &kTargetList, bAreaSkillUsed, 1.0f
                    );
                    if (bAreaSkillUsed) {
                        bSkillUsed = false;
                    }
                }
            }
            delete pActiveEffect;
        }
        // An ingredient applies its first effect only.
        if (pCurrSpell->GetSpellType() == MagicSystem::WORTCRAFT) {
            pEffectNode = nullptr;
        }
        else {
            pEffectNode = static_cast<EffectItemList*>(pEffectNode->GetNext());
        }
    }
    EffectSetting* pAVEffect = pCurrSpell->GetAVEffect(MagicSystem::SELF);
    EffectItem* pAreaEffect = pCurrSpell->GetCostliestEffect(MagicSystem::TOUCH, true);
    bool bCasterArea = pSpellItem && pSpellItem->GetFlag(0x80);
    Actor* pExplosionRef = pTargetActor ? pTargetActor : ((pCaster && bCasterArea) ? pCaster : nullptr);
    if (pExplosionRef) {
        NiPoint3 kSourceLoc;
        kSourceLoc.~NiPoint3();
        ::new (&kSourceLoc) NiPoint3;
        NiNode* pCasterMagicNode = (pCaster && !apSource) ? pCaster->GetMagicNode() : nullptr;
        const NiPoint3* pSourceLoc =
            pCasterMagicNode ? &pCasterMagicNode->GetWorldTranslate() : &pExplosionRef->GetLocationOnReference();
        kSourceLoc = *pSourceLoc;
        if (pAreaEffect) {
            DoExplosion(kSourceLoc, pExplosionRef->GetParentCell(), pCurrSpell, pAreaEffect, &kTargetList, 1.0f, 1.0f);
            if (pAVEffect && pAVEffect->TESModel::GetModelLength()) {
                // Faces the reference's heading; the cosine is taken first.
                float fCos = NiCos(-pExplosionRef->GetAngleOnReference().x);
                float fSin = -NiSin(-pExplosionRef->GetAngleOnReference().x);
                NiPoint3 kDirection(fSin, fCos, 0.0f);
                BSTempEffectParticle* pParticle = BSTempEffectParticle::Spawn(
                    pExplosionRef->GetParentCell(), 1.0f, static_cast<TESModel*>(pAVEffect)->GetModel(), kDirection,
                    kSourceLoc, 1.0f, 7, nullptr
                );
                pParticle->PlaySequence("SpecialIdle_AreaEffect");
                if (pParticle->GetParticleObject()) {
                    float fScale =
                        static_cast<float>(static_cast<double>(pAreaEffect->GetArea()) * fMagicAreaScale.Float());
                    if (fMagicAreaScaleMax.Float() < fScale) {
                        fScale = fMagicAreaScaleMax.Float();
                    }
                    else if (fMagicAreaScaleMin.Float() > fScale) {
                        fScale = fMagicAreaScaleMin.Float();
                    }
                    pParticle->GetParticleObject()->SetScale(fScale);
                }
            }
        }
        else if (
            (pExplosionRef->IsActor()) &&
            (pCostliestTouchEffect &&
             pCostliestTouchEffect->GetEffectSetting()->IsFlagSet(EffectSetting::HAVOK_EXPLOSION))
        ) {
            Actor* pVictim = pExplosionRef;
            if (CheckWillKill(pVictim, pCurrSpell)) {
                ExplodeActor(pVictim, kSourceLoc, pCostliestTouchEffect, pCostliestTouchEffect->GetRawCost());
            }
        }
    }
    kTargetList.RemoveAll();
    if (pCurrSpell->HasTargetEffect()) {
        FireProjectile();
    }
    else if (
        (bSkillUsed && pCaster) &&
        (pCurrSpell->GetSpellType() == MagicSystem::SPELL || pCurrSpell->GetSpellType() == MagicSystem::POISON)
    ) {
        pCaster->UseSkill(pCurrSpell->GetAssociatedSkill(), 1u, 0.0f);
    }

    // The model length is compared unsigned.
    if (bReleaseModel && pAVEffect && pAVEffect->TESModel::GetModelLength() > 0) {
        const char* pModelName = static_cast<TESModel*>(pAVEffect)->GetModel();
        pModelLoader->ReleaseModel(pModelName, false, 1);
    }
    SetCurrentSpell(nullptr);
    SetDesiredTarget(nullptr);
    SetState(NO_SPELL, nullptr);
    if (bPotion && pCaster) {
        AlchemyItem* pAddictive = dynamic_cast<AlchemyItem*>(pCurrSpell);
        if (pAddictive) {
            pCaster->CheckAddiction(pAddictive);
        }
    }
}

void MagicCaster::DoExplosion(
    NiPoint3 aSourceLoc,
    TESObjectCELL* apCell,
    MagicItem* apSpell,
    EffectItem* apDominantEffect,
    BSSimpleList<MobileObject*>* aTargetList,
    float afPowerScale,
    float afAreaScale
) {
    CFilter v34;
    alignas(16) NiPoint3 v35;
    NiPoint3 v36;
    NiPoint3 v37;
    NiPoint3 v38;
    if (apDominantEffect->pEffectSetting->IsFlagSet(EffectSetting::HAVOK_EXPLOSION)) {
        int32_t Area = apDominantEffect->GetArea();
        double v45 = static_cast<double>(Area);
        float* v8 = &fMagicUnitsPerFoot.Float();
        float farea = *v8 * v45 * afAreaScale;
        if (farea > 0.0) {
            bool z = apDominantEffect->data.iRange == MagicSystem::TARGET;
            int32_t Magnitude = apDominantEffect->GetMagnitude();
            int32_t y = static_cast<int32_t>(static_cast<double>(Magnitude) * afPowerScale);
            int32_t Duration = apDominantEffect->GetDuration();
            int32_t x = static_cast<int32_t>(static_cast<double>(Duration) * afPowerScale);
            int32_t v18 = apDominantEffect->GetArea();
            float afBaseEffectCost = apDominantEffect->pEffectSetting->data.fBaseCost;
            float fspellCost = MagicFormulas::GetMagicEffectCost(afBaseEffectCost, v18, x, y, z);
            float fareahalf = farea * 0.5;
            hkAabbPhantomCinfo info;
            float za = -fareahalf;
            NiPoint3* v9 = ::new (&v38) NiPoint3(za, za, za);
            NiPoint3* v10 = ::new (&v37) NiPoint3(aSourceLoc + *v9);
            NI2HK(info.aabb.m_min, *v10);
            NiPoint3* v11 = ::new (&v36) NiPoint3(fareahalf, fareahalf, fareahalf);
            NiPoint3* v12 = ::new (&v35) NiPoint3(aSourceLoc + *v11);
            NI2HK(info.aabb.m_max, *v12);
            uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
            info.m_collisionFilterInfo =
                XFNV_RECONSTRUCT(CFilter)(&v34, L_SPELLEXPLOSION, NewSystemGroup, P_OTHER)->iFilter;
            bhkAabbPhantom pphantom(info);
            bhkWorld* pw = apCell->GetbhkWorld();
            pphantom.bhkWorldObject::Add(pw);
            const hkArray<hkpCollidable*, hkContainerHeapAllocator>* parray = pphantom.GetOverlappingCollidables();
            int32_t v30 = 0;
            for (int32_t i = parray->m_size; v30 < i; i = parray->m_size) {
                hkpCollidable* const* v15 = &(*parray)[v30];
                hkpRigidBody* rb = hkpGetRigidBody(*v15);
                if (rb) {
                    this->ExplodeRigidBody(rb, aSourceLoc, apDominantEffect, fspellCost);
                }
                ++v30;
            }
            pphantom.bhkPhantom::Remove();
            BSSimpleList<MobileObject*>* pmob = aTargetList;
            Actor* pcasterActor = this->GetCasterAsActor();
            while (pmob && pmob->m_item) {
                MobileObject* m_item = pmob->m_item;
                Actor* ptargetActor = m_item->IsActor() ? static_cast<Actor*>(m_item) : nullptr;
                if ((ptargetActor) && (ptargetActor != pcasterActor) && (ptargetActor->Get3D())) {
                    float fdistance = ptargetActor->GetDistanceFromPoint(aSourceLoc);
                    if (farea >= static_cast<double>(fdistance) && !ptargetActor->IsDead(false) &&
                        !ptargetActor->GetGhost() && this->CheckWillKill(ptargetActor, apSpell)) {
                        this->ExplodeActor(ptargetActor, aSourceLoc, apDominantEffect, fspellCost);
                    }
                }

                pmob = pmob->m_pkNext;
            }
            if ((pPlayer) && (pPlayer != pcasterActor) && (pPlayer->Get3D())) {
                float fdistance_2 = pPlayer->GetDistanceFromPoint(aSourceLoc);
                if (farea >= static_cast<double>(fdistance_2) && !pPlayer->IsDead(false) && !pPlayer->GetGhost()) {
                    bool v16;
                    if (pPlayer) {
                        v16 = this->CheckWillKill(
                            &static_cast<MagicTarget&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer))), apSpell
                        );
                    }
                    else {
                        v16 = this->CheckWillKill(nullptr, apSpell);
                    }
                    if (v16) {
                        this->ExplodeActor(pPlayer, aSourceLoc, apDominantEffect, fspellCost);
                    }
                }
            }
        }
    }
}

// Blows apBody apart from aSourceLoc: the offset from the body's centre of mass to the source, unitized, gives the
// direction; the power from the spell cost, the area in feet and that distance is clamped to the maximum setting and
// scaled by the clutter setting, the impulse gets half a unit of lift, and the body is activated and pushed with the
// offset times the power, scaled by its mass. The area is converted as a signed value and both conversions truncate.
// The hkVector4 and hkSimdReal locals are alignas(16), and kOffset is passed by address to UnitizeGetLength on a
// 16-byte boundary.
void MagicCaster::ExplodeRigidBody(
    hkpRigidBody* apRigidBody,
    NiPoint3 aSourceLoc,
    EffectItem* apDominantEffect,
    float afSpellCost
) {
    NiPoint3 kCenterOfMass;
    kCenterOfMass.~NiPoint3();
    ::new (&kCenterOfMass) NiPoint3;
    HK2NI(kCenterOfMass, apRigidBody->getCenterOfMassInWorld());
    alignas(16) NiPoint3 kOffset = kCenterOfMass - aSourceLoc;
    float fDistance = kOffset.UnitizeGetLength();
    uint32_t uiArea = apDominantEffect->GetArea();
    int32_t iAreaFeet = (int32_t)(fMagicUnitsPerFoot.Float() * (double)(int32_t)uiArea);
    int32_t iSpellCost = (int32_t)afSpellCost;
    float fPower = MagicFormulas::CheckExplosionStrength(iSpellCost, iAreaFeet, fDistance, 0, 0, 0.0f);
    if (fPower > 0.0) {
        if (fPower > fMagicExplosionPowerMax.Float()) {
            fPower = fMagicExplosionPowerMax.Float();
        }
        fPower = fPower * fMagicExplosionClutterMult.Float();
        kOffset[2] += 0.5;
        NiPoint3 kImpulseNi = kOffset * fPower;
        alignas(16) hkVector4 kImpulse;
        NI2HK(kImpulse, kImpulseNi);
        alignas(16) hkSimdReal kMass;
        XFNV_RECONSTRUCT(hkSimdReal)(&kMass, apRigidBody->getMass());
        kImpulse.mul4(kMass);
        apRigidBody->activate();
        apRigidBody->applyLinearImpulse(kImpulse);
    }
}

void MagicCaster::ExplodeActor(
    Actor* apTargetActor,
    NiPoint3 aSourceLoc,
    EffectItem* apDominantEffect,
    float afSpellCost
) {
    NiPoint3 v22;
    NiPoint3 targetCenter;
    NiPoint3* v5 = ::new (&v22) NiPoint3(apTargetActor->GetBoundMax());
    float fActorHeight = v5->z - apTargetActor->GetBoundMin().z;
    const NiPoint3* v6 = &apTargetActor->GetLocationOnReference();
    targetCenter.x = v6->x;
    targetCenter.y = v6->y;
    targetCenter.z = fActorHeight * 0.75 + v6->z;
    NiPoint3 direction = targetCenter - aSourceLoc;
    float fdistance = direction.Length();
    float fFatiguePercentage = apTargetActor->GetFatiguePercentage();
    int32_t iLuck = apTargetActor->GetClampedActorValue(ActorValue::LUCK);
    int32_t iAgility = apTargetActor->GetClampedActorValue(ActorValue::AGILITY);
    double Area = static_cast<double>(apDominantEffect->GetArea());
    int32_t v10 = static_cast<int32_t>(fMagicUnitsPerFoot.Float() * Area);
    float fmagnitude = MagicFormulas::CheckExplosionStrength(
        static_cast<int32_t>(afSpellCost), v10, fdistance, iAgility, iLuck, fFatiguePercentage
    );
    float* v7 = &fMagicExplosionPowerMax.Float();
    if (*v7 < static_cast<double>(fmagnitude)) {
        fmagnitude = fMagicExplosionPowerMax.Float();
    }
    float* v8 = &fMagicExplosionPowerMin.Float();
    if (!(*v8 < static_cast<double>(fmagnitude))) {
        if (fmagnitude > 0.0) {
            Actor* pcasterActor = GetCasterAsActor();
            if (pcasterActor) {
                NiPoint3 transformedSourceLoc = NiPoint3_UNIT_Y;
                NiPoint3 transformedTargetLoc = NiPoint3_ZERO;
                NiAVObject* pav = pcasterActor->Get3D();
                if (pav) {
                    NiTransform t;
                    (&pav->GetWorldTransform())->Invert(t);
                    transformedSourceLoc = t * aSourceLoc;
                    transformedTargetLoc = t * targetCenter;
                }
                float v15 = fmagnitude * 2.0;
                apTargetActor->HitMe(pcasterActor, v15, 0.0f, transformedTargetLoc, transformedSourceLoc);
            }
        }
    }
    else {
        BaseProcess* CurrentAIProcess = apTargetActor->pCurrentProcess;
        CurrentAIProcess->KnockExplosion(apTargetActor, aSourceLoc, fmagnitude);
    }
    if (fmagnitude > 0.0 && CombatController::GetShowVerboseMessages()) {
        const char* FullName = apTargetActor->GetFullName();
        Interface::PrintLine("An explosion of %.1f magnitude hits %.20s!", fmagnitude, FullName);
    }
}

bool MagicCaster::CheckWillKill(MagicTarget* apTarget, MagicItem* apSpell) {
    if (!apTarget) {
        return false;
    }
    if (!apSpell) {
        return false;
    }
    if (!apTarget->GetTargetStatsObject()) {
        return false;
    }
    Actor* pactor = static_cast<Actor*>(apTarget->GetTargetStatsObject());
    if (!pactor->IsActor()) {
        return false;
    }
    TESObjectREFR* v7 = apTarget->GetTargetStatsObject();
    if (!v7->Get3D()) {
        return false;
    }
    if (apSpell->IsHostile()) {
        Actor* pcaster = GetCasterAsActor();
        Actor* pactor_3 = static_cast<Actor*>(apTarget->GetTargetStatsObject());
        float fcurrentHealth = pactor_3->GetHealth();
        BSSimpleList<ActiveEffect*>* plist = apTarget->GetActiveEffectList();
        float fdamage = 0.0f;
        while (plist && !plist->IsEmpty()) {
            ActiveEffect* peffect = plist->m_item;
            if ((peffect) && (peffect->pSpell == apSpell)) {
                ActorValue::Index eactorValue;
                EffectSetting* pes = peffect->GetEffectSetting();
                if (pes->IsAssociatedActorValueFixed()) {
                    eactorValue = pes->data.eAssociatedActorValue;
                }
                else {
                    eactorValue = peffect->pEffect->GetActorValueIndex();
                }
                if (eactorValue == ActorValue::HEALTH && pes->IsFlagSet(EffectSetting::DETRIMENTAL) &&
                    !pes->IsFlagSet(EffectSetting::RECOVER)) {
                    float famount = peffect->fMagnitude;
                    if (peffect->fDuration > 0.0) {
                        float a2 = peffect->fDuration;
                        float a2a = BSMAX(0.0f, a2);
                        float fduration = BSMIN(4.0f, a2a);
                        famount = famount * fduration;
                    }
                    fdamage = fdamage + famount;
                }
            }

            plist = plist->m_pkNext;
        }
        float v19 = -pactor_3->DifficultyLevelAdjustHealthModifier(-fdamage, pcaster);
        if (fcurrentHealth <= static_cast<double>(v19)) {
            return true;
        }
    }
    return false;
}

void MagicCaster::UpdateLight(float afDelta) {
    if (pLight) {
        pLight->Update(afDelta);
        if ((!GetCurrentSpell()) && (!pLight->IsFading())) {
            if (pLight) {
                delete pLight;
            }
            pLight = nullptr;
        }
    }
}

// Releases the current spell: once CheckCast succeeds it enters RELEASE, applies and fires the spell's effects and
// fades the cast light; otherwise it collects the cannot-cast message (shown only for a player caster, and only when
// the source ingredient is food or there is none), fails the spell and clears the spell and the desired target.
// fEffectStrength starts at 0 and CheckCast fills it in. The sound branch reads a local that is never set, so it never
// runs; only its GetAVEffect call stays, as the game does.
bool MagicCaster::ReleaseCast(TESBoundObject* apSource) {
    if (!GetCurrentSpell()) {
        return false;
    }
    float fEffectStrength = 0.0f;
    MagicItem* pSpell = GetCurrentSpell();
    IngredientItem* pIngredient = dynamic_cast<IngredientItem*>(pSpell);
    MagicItem::CannotCastReason eReason = MagicItem::NOCAST_OK;
    if (CheckCast(GetCurrentSpell(), &fEffectStrength, &eReason, false)) {
        SetState(RELEASE, "Release Cast");
        MagicItem* pReleasedSpell = GetCurrentSpell();
        FindTargets(fEffectStrength, apSource, false, 1.0f, false);
        SpellCast(pReleasedSpell, true);
        if (pLight) {
            MagicLight* pMagicLight = pLight;
            pMagicLight->Fade(false, 0.75f);
        }
        return true;
    }
    Actor* pCaster = GetCasterAsActor();
    GetCurrentSpell()->GetAVEffect(MagicSystem::SELF);
    // GetCannotCastString is handed uninitialised storage; the callee constructs the string.
    alignas(BSStringT<char>) uint8_t aCannotCastStorage[sizeof(BSStringT<char>)];
    BSStringT<char>* pCannotCast = reinterpret_cast<BSStringT<char>*>(aCannotCastStorage);
    GetCurrentSpell()->GetCannotCastString(pCannotCast, eReason);
    if ((pCaster) && (pCaster->IsPc() && (!pIngredient || static_cast<EffectItemList*>(pIngredient)->IsFood()))) {
        Interface::ShowMessage(pCannotCast->String(), 0, nullptr, nullptr, 2.0f, false);
    }

    // SetState takes the message as char* while String() returns const char*, hence the const_cast.
    SetState(ERR_CANNOT_CAST, const_cast<char*>(pCannotCast->String()));
    SpellCast(GetCurrentSpell(), false);
    SetCurrentSpell(nullptr);
    SetDesiredTarget(nullptr);
    pCannotCast->~BSStringT<char>();
    return false;
}

void MagicCaster::EndAim() {
    StartCast();
}

void MagicCaster::InterruptCast() {
    Actor* pActor = GetCasterAsActor();
    bool bRemovedChildren = false;
    if (pActor) {
        NiNode* pMagicNode = static_cast<MagicCaster*>(pActor)->GetMagicNode();
        if (pMagicNode && pMagicNode->GetChildCount()) {
            bRemovedChildren = true;
            pMagicNode->RemoveAllChildren();
        }
        if (pLight) {
            MagicLight* pMagicLight = pLight;
            if (pMagicLight) {
                delete pMagicLight;
            }
        }
        pLight = nullptr;
    }
    if (!GetCurrentSpell()) {
        return;
    }
    SetState(ERR_CANNOT_CAST, "Cannot Cast");
    SpellCast(GetCurrentSpell(), false);
    if (bRemovedChildren) {
        EffectSetting* pEffect = GetCurrentSpell()->GetAVEffect(MagicSystem::SELF);
        if (pEffect && static_cast<TESModel*>(pEffect)->GetModelLength()) {
            TESModel* pModel = pEffect;
            pModelLoader->ReleaseModel(pModel->GetModel(), false, 1);
        }
    }
    SetCurrentSpell(nullptr);
    SetDesiredTarget(nullptr);
}

uint32_t MagicCaster::GetMagicCasterFormID() {
    Actor* casterAsActor = GetCasterAsActor();
    NonActorMagicCaster* v4 = dynamic_cast<NonActorMagicCaster*>(this);
    if (casterAsActor) {
        return casterAsActor->TESForm::GetFormID();
    }
    if ((!v4) || (!static_cast<MagicCaster*>(v4)->GetCasterStatsObject())) {
        return (uint32_t)0;
    }
    TESObjectREFR* v2 = static_cast<MagicCaster*>(v4)->GetCasterStatsObject();
    return v2->TESForm::GetFormID();
}

bool MagicCaster::AddTarget(
    MagicTarget* apTarget,
    NiPoint3 aSourceLoc,
    MagicItem* apSpell,
    ActiveEffect* apActiveEffect,
    bool abAreaTarget
) {
    return (bool)((int32_t)apTarget->AddTarget(this, apSpell, apActiveEffect, abAreaTarget));
}

void MagicCaster::FireProjectile() {
    NiPoint3 v20;
    NiPoint3 v24;
    if (GetCurrentSpell()) {
        TESObjectREFR* ptargetObject;
        TESObjectREFR* pcasterObject = GetCasterStatsObject();
        Actor* pcasterActor = GetCasterAsActor();
        if (GetDesiredTarget()) {
            MagicTarget* pTarget = GetDesiredTarget();
            ptargetObject = pTarget->GetTargetStatsObject();
        }
        else {
            ptargetObject = nullptr;
        }
        MagicItem* v1 = GetCurrentSpell();
        EffectItem* pcostliestTargetEffect = v1->GetCostliestEffect(MagicSystem::TARGET, false);
        MagicItem* v2 = GetCurrentSpell();
        EffectSetting* pavEffect = v2->GetAVEffect(MagicSystem::SELF);
        if ((pcostliestTargetEffect) && (pcasterObject)) {
            MagicCaster* v12;
            NiPoint3 origin;
            bool blaunch = true;
            NiPoint3 vect = NiPoint3_ZERO;
            NiPoint3 angle = NiPoint3_ZERO;
            if (pcasterActor) {
                angle.z = pcasterActor->GetHeading(false);
                angle.x = pcasterActor->GetLooking();
                origin = pcasterObject->GetLocationOnReference();
                NiPoint3* v3 = &(v24 = pcasterObject->GetBoundMax());
                float casterHeight = v3->z - pcasterObject->GetBoundMin().z;
                vect.z = casterHeight * 0.699999988079071;
                origin.operator+=(vect);
            }
            else {
                double MaximumLeafAngle = pavEffect->data.fSpeed;
                float fspeed = fMagicProjectileBaseSpeed.Float() * MaximumLeafAngle;
                angle = pcasterObject->GetAngleOnReference() +
                    CombatUtilities::GetAngleToProjectedTarget(
                            pcasterObject, ptargetObject, fspeed, 0.0f, ACTOR_SEGMENT_INVIEW_TOP
                    );
                origin = pcasterObject->GetLocationOnReference();
                NiPoint3* v5 = &(v20 = pcasterObject->GetBoundMax());
                origin.z = (v5->z - pcasterObject->GetBoundMin().z) * 0.5 + origin.z;
            }
            NiNode* pmagicNode = GetMagicNode();
            if (pmagicNode) {
                origin = pmagicNode->GetWorldTranslate();
            }
            if (pPlayer) {
                v12 = &static_cast<MagicCaster&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
            }
            else {
                v12 = nullptr;
            }
            if ((this != v12) && (pcasterActor) && (ptargetObject)) {
                ACTOR_SEGMENT_INVIEW segment;
                float v11;
                if (pavEffect) {
                    v11 = pavEffect->data.fSpeed;
                }
                else {
                    v11 = 1.0f;
                }
                float fspeed_2 = fMagicProjectileBaseSpeed.Float() * v11;
                float fgravity = 0.0f;
                CombatController* pcombat = pcasterActor->GetCombatController();
                if (pcombat) {
                    segment = pcombat->GetTargetSegmentInView();
                }
                else {
                    segment = ACTOR_SEGMENT_INVIEW_MIDDLE;
                }
                angle = CombatUtilities::GetAngleToProjectedTarget(origin, ptargetObject, fspeed_2, fgravity, segment);
            }

            if ((pcostliestTargetEffect) && (blaunch)) {
                NiPoint3 v41 = origin;
                MagicItem* v7 = GetCurrentSpell();
                MagicProjectile::Launch(this, nullptr, pLight, v7, pcostliestTargetEffect, pavEffect, v41, angle);
                pLight = nullptr;
            }

            if (pLight) {
                MagicLight* apThis = pLight;
                if (apThis) {
                    delete apThis;
                }
            }
            pLight = nullptr;
        }
    }
}

void MagicCaster::FindTargetsInArea(
    MagicItem* apSpell,
    ActiveEffect* apActiveEffect,
    TESObjectCELL* apCell,
    NiPoint3 aLocation,
    MagicTarget* apTarget,
    MagicProjectile* apProjectile,
    BSSimpleList<MobileObject*>* aTargetList,
    bool& abSkillAlreadyUsed,
    float afAreaScale
) {
    if (apCell) {
        EffectItem* CastingType = apActiveEffect->GetEffectItem();
        int32_t Area = CastingType->GetArea();
        if (Area > 0) {
            float* v11 = &fMagicUnitsPerFoot.Float();
            int32_t v23 = Area * static_cast<int32_t>(*v11 * afAreaScale);
            Actor* CasterAsActor = GetCasterAsActor();
            bool v19 = abSkillAlreadyUsed;
            for (BSSimpleList<MobileObject*>* ecx0 = aTargetList; ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
                TESObjectREFR* v18 = ecx0->m_item;
                if (v18) {
                    Actor* TargetAsActor;
                    MagicTarget* v16 = v18->GetMagicTarget();
                    bool v17 = false;
                    if (v16) {
                        TargetAsActor = v16->GetTargetAsActor();
                    }
                    else {
                        TargetAsActor = nullptr;
                    }
                    if (CasterAsActor) {
                        v17 = TargetAsActor == CasterAsActor;
                    }
                    if ((v16) && (!v17 && v16 != apTarget) && (v18->Get3D())) {
                        float DistanceFromPoint = v18->GetDistanceFromPoint(aLocation);
                        if (DistanceFromPoint <= static_cast<double>(v23) &&
                            (!TargetAsActor || !TargetAsActor->GetGhost()) &&
                            (!apProjectile || apProjectile->CheckAddTarget(v16, apActiveEffect)) &&
                            AddTarget(v16, aLocation, apSpell, apActiveEffect, true)) {
                            if (apProjectile) {
                                apProjectile->AddAreaTarget(v16, apActiveEffect);
                            }
                            if ((!apTarget && !v19) && (CasterAsActor) &&
                                (apSpell->GetSpellType() == MagicSystem::SPELL ||
                                 apSpell->GetSpellType() == MagicSystem::POISON)) {
                                v19 = true;
                                abSkillAlreadyUsed = true;
                                ActorValue::Index AssociatedSkill = apSpell->GetAssociatedSkill();
                                CasterAsActor->UseSkill(AssociatedSkill, 1u, static_cast<float>(0.0));
                            }
                        }
                    }
                }
            }
        }
    }
}

void MagicCaster::FindExplosionLOS(
    MagicItem* apSpell,
    MagicSystem::Range aeRange,
    NiPoint3 aSpellLocation,
    BSSimpleList<MobileObject*>* apActorList
) {
    if (apActorList) {
        EffectItemList* plist;
        EffectItemList* plist_2;
        int32_t ihighestArea = 0;
        if (apSpell) {
            plist_2 = static_cast<EffectItemList*>(apSpell);
        }
        else {
            plist_2 = nullptr;
        }
        for (EffectItemList* plist_3 = plist_2; plist_3 && !plist_3->IsEmpty(); plist_3 = plist) {
            EffectItem* peffect = plist_3->m_item;
            if ((peffect) && (peffect->data.iRange == aeRange && peffect->GetArea() > ihighestArea)) {
                ihighestArea = peffect->GetArea();
            }

            BSSimpleList<EffectItem*>* pnext = plist_3->m_pkNext;
            // Each next node is taken as the EffectItemList holding it, with null kept null.
            if (pnext) {
                plist = static_cast<EffectItemList*>(pnext);
            }
            else {
                plist = nullptr;
            }
        }
        if (ihighestArea) {
            SpellItem* pspell = dynamic_cast<SpellItem*>(apSpell);
            bool v6 = pspell && pspell->GetFlag(16u);
            float farea = fMagicUnitsPerFoot.Float() * static_cast<double>(ihighestArea);
            Actor* pcasterActor = GetCasterAsActor();
            ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
            for (uint32_t i = 0;; ++i) {
                uint32_t ProcessLevelTail = parray->GetProcessLevelTail(PROCESS_TYPE_HIGH);
                if (i >= ProcessLevelTail) {
                    break;
                }
                MobileObject* pitem = parray->GetItem(i);
                BSSimpleList<Actor*>* ptempfollower = nullptr;
                while (pitem) {
                    MagicTarget* ptarget = pitem->GetMagicTarget();
                    bool bitemMatchesTarget = false;
                    Actor* ptargetActor = dynamic_cast<Actor*>(pitem);
                    if (pcasterActor) {
                        bitemMatchesTarget = ptargetActor == pcasterActor;
                    }
                    if ((ptarget) && (!bitemMatchesTarget) && (pitem->Get3D())) {
                        float fdistance = pitem->GetDistanceFromPoint(aSpellLocation);
                        if (farea >= static_cast<double>(fdistance) && (!ptargetActor || !ptargetActor->GetGhost()) &&
                            (v6 || RunExplosionLOSPick(aSpellLocation, ptargetActor))) {
                            MobileObject* a2 = (MobileObject*)ptargetActor;
                            apActorList->AddHead(a2);
                        }
                    }

                    if (ptempfollower && ptempfollower->m_item) {
                        pitem = dynamic_cast<Character*>(ptempfollower->m_item);
                        ptempfollower = ptempfollower->m_pkNext;
                        while (pitem && (!pitem->pCurrentProcess || pitem->pCurrentProcess->eLevel)) {
                            if (ptempfollower) {
                                pitem = dynamic_cast<Character*>(ptempfollower->m_item);
                            }
                            else {
                                pitem = nullptr;
                            }
                            if (ptempfollower) {
                                ptempfollower = ptempfollower->m_pkNext;
                            }
                        }
                    }
                    else {
                        pitem = nullptr;
                    }
                }
            }
            if ((pPlayer != pcasterActor) && (pPlayer->Get3D())) {
                float fdistance_2 = pPlayer->GetDistanceFromPoint(aSpellLocation);
                if (farea >= static_cast<double>(fdistance_2) && !pPlayer->GetGhost() &&
                    (v6 || RunExplosionLOSPick(aSpellLocation, pPlayer))) {
                    MobileObject* item = (MobileObject*)pPlayer;
                    apActorList->AddHead(item);
                }
            }
        }
    }
}

bool MagicCaster::RunExplosionLOSPick(NiPoint3 aLocation, Actor* apTarget) {
    if (!apTarget) {
        return false;
    }
    bhkWorld* pWorld;
    if (apTarget->GetParentCell()) {
        pWorld = apTarget->GetParentCell()->GetbhkWorld();
    }
    else {
        pWorld = nullptr;
    }
    bhkWorld* pPickWorld = pWorld;
    if (!pPickWorld) {
        return false;
    }
    // Static, anim static, trees, terrain, trap and ground.
    uint32_t uiLayerBits = 0x26206;
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, uiLayerBits);
    alignas(16) NiPoint3 kTo = apTarget->GetLocationOnReference();
    kTo.z = apTarget->GetHeight() * 0.5 + kTo.z;
    alignas(16) bhkPickData kPick;
    kPick.~bhkPickData();
    ::new (&kPick) bhkPickData;
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
    kFilter.SetLayer(L_CUSTOMPICK1);
    if (apTarget->GetCharController()) {
        kFilter.SetGroup(static_cast<bhkCharacterController*>(apTarget->GetCharController())->GetFilterGroup());
    }
    else {
        kFilter.SetGroup(bhkCollisionFilter::GetNewSystemGroup());
    }
    kPick.SetFilter(kFilter);
    kPick.NiSetFrom(aLocation);
    kPick.NiSetTo(kTo);
    return !pPickWorld->PickObject(kPick);
}

void MagicCaster::SetState(State aeState, const char* acMessage) {
    eState = (MagicCaster::State)((uint32_t)aeState);
}

// The skill-used flag and the costliest touch effect are separate locals; the effect list is walked node by node, each
// node converted back to the list.
void MagicCaster::FindTargets_0(
    MagicItem* apSpell,
    TESObjectCELL* apCell,
    NiPoint3 aLocation,
    MagicProjectile* apProjectile,
    TESObjectREFR* apCollidee,
    bool abLoadCast,
    float afPowerScale,
    float afAreaScale
) {
    float feffectivenessModifier;
    EffectItemList* plist_2;
    bool bskillUsed;
    if (apSpell) {
        plist_2 = static_cast<EffectItemList*>(apSpell);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    if (apSpell->GetSpellType()) {
        feffectivenessModifier = 1.0f;
    }
    else {
        feffectivenessModifier = GetMagicEffectivenessModifier(false, 0.0f);
    }
    EffectItem* pdominantTouch = apSpell->GetCostliestEffect(MagicSystem::TARGET, false);
    BSSimpleList<MobileObject*> TargetList;
    TargetList.RemoveAll();
    FindExplosionLOS(apSpell, MagicSystem::TARGET, aLocation, &TargetList);
    bskillUsed = false;
    while (plist_3) {
        EffectItem* pitem = plist_3->m_item;
        if ((pitem) && (pitem->data.iRange == MagicSystem::TARGET)) {
            MagicTarget* apTarget;
            ActiveEffect* pactiveEffect = CreateActiveEffect(apSpell, pitem, nullptr);
            if (pitem != pdominantTouch) {
                pactiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_NIF);
                pactiveEffect->SetFlag(ActiveEffect::NO_HITEFFECT_SHADER);
                pactiveEffect->SetFlag(ActiveEffect::NO_SOUND);
            }
            float feffectivenessModifier_2 = feffectivenessModifier * afPowerScale;
            pactiveEffect->AdjustEffectiveness(feffectivenessModifier_2);
            if ((apCollidee) && (apCollidee->GetMagicTarget()) && (apCollidee != GetCasterStatsObject()) &&
                (!apProjectile || apProjectile->CheckAddTarget(apCollidee->GetMagicTarget(), pactiveEffect))) {
                MagicTarget* v10 = apCollidee->GetMagicTarget();
                if (AddTarget(v10, aLocation, apSpell, pactiveEffect, false)) {
                    Actor* pcasterActor;
                    MagicCaster* pmagicCaster = apProjectile->GetCaster();
                    if (pmagicCaster) {
                        pcasterActor = pmagicCaster->GetCasterAsActor();
                    }
                    else {
                        pcasterActor = nullptr;
                    }
                    if ((pcasterActor && (!bskillUsed && apSpell->GetSpellType() == MagicSystem::SPELL)) ||
                        apSpell->GetSpellType() == MagicSystem::POISON) {
                        bskillUsed = true;
                        ActorValue::Index AssociatedSkill = apSpell->GetAssociatedSkill();
                        pcasterActor->UseSkill(AssociatedSkill, 1u, 0.0f);
                    }
                }
            }

            if (apCollidee) {
                apTarget = apCollidee->GetMagicTarget();
            }
            else {
                apTarget = nullptr;
            }
            FindTargetsInArea(
                apSpell, pactiveEffect, apCell, aLocation, apTarget, apProjectile, &TargetList, bskillUsed, afAreaScale
            );
            if (pactiveEffect) {
                delete pactiveEffect;
            }
        }

        plist_3 = static_cast<EffectItemList*>(plist_3->m_pkNext);
    }
    bool v30 = apProjectile->pAVEffect->GetProjectileType() == MagicSystem::FOG;
    EffectItem* pDominantArea = apSpell->GetCostliestEffect(MagicSystem::TARGET, true);
    if (!v30) {
        if (pDominantArea) {
            DoExplosion(aLocation, apCell, apSpell, pDominantArea, &TargetList, afPowerScale, afAreaScale);
        }
        else {
            if ((apCollidee) && (apCollidee->IsActor()) && (pdominantTouch) &&
                (pdominantTouch->pEffectSetting->IsFlagSet(EffectSetting::HAVOK_EXPLOSION) &&
                 CheckWillKill(static_cast<Actor*>(apCollidee), apSpell))) {
                float a8 = pdominantTouch->GetRawCost();
                ExplodeActor(static_cast<Actor*>(apCollidee), aLocation, pdominantTouch, a8);
            }
        }
    }
    TargetList.RemoveAll();
}

MagicCaster* MagicCaster::GetMagicCasterByNumericID(uint32_t aiFormID) {
    TESForm* formByNumericID = TESForm::GetFormByNumericID(aiFormID);
    if (formByNumericID && formByNumericID->IsReference()) {
        return static_cast<TESObjectREFR*>(formByNumericID)->GetMagicCaster();
    }

    return nullptr;
}

// A box phantom wraps an hkpAabbPhantom; its overlap array's getter is a trivial accessor (this + 0xD0).
hkArray<hkpCollidable*, hkContainerHeapAllocator>* bhkAabbPhantom::GetOverlappingCollidables() {
    hkpAabbPhantom* pphantom = static_cast<hkpAabbPhantom*>(operator*());
    if (pphantom) {
        return &pphantom->m_overlappingCollidables;
    }

    return nullptr;
}
