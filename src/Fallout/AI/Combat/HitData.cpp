#include "Fallout/AI/Combat/HitData.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSWeightedRandom.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSPolygon.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAmmoEffect.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FollowerBarks.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

int32_t BSMAX_3(uint16_t ausA, int32_t aiB) {
    if (ausA <= aiB) {
        return aiB;
    }

    return ausA;
}

HitData::HitData() {
    ClearData();
}

void HitData::ReduceDamage(bool abIgnoreBlocking) {
    if (pTarget && pTarget->IsActor()) {
        if (static_cast<MagicTarget&>(*pTarget).IsInvulnerable()) {
            fTotalDamage = 0.0f;
            fHealthDamage = 0.0f;
            fFatigueDamage = 0.0f;
        }
        else {
            fTotalDamage = fHealthDamage;
            float fMinWeaponDamage = fHealthDamage * fMinDamMultiplier.Float();
            if (pWeapon) {
                double dfatigueDamage;
                if (pWeapon->GetCurrentAmmo(pAggressor)) {
                    TESAmmo* pammo = pWeapon->GetCurrentAmmo(pAggressor);
                    BSSimpleList<TESAmmoEffect*>* pEffectList = pammo->GetAmmoEffectList();
                    dfatigueDamage = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_FATIGUE, pEffectList, fFatigueDamage);
                }
                else {
                    dfatigueDamage = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_FATIGUE, nullptr, fFatigueDamage);
                }
                fFatigueDamage = dfatigueDamage;
            }
            float fdamageResistance = pTarget->GetActorFloatValue(ActorValue::DAMAGE_RESISTANCE);
            double dresistanceRatio = BSMIN(fdamageResistance, 100.0f) / 100.0;
            float farmorRating = dresistanceRatio;
            if (pTarget->bPlayerTeammate) {
                float fCharisma = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
                float fNerveDRbonus = fCharisma * 0.05000000074505806 + 1.0;
                farmorRating = farmorRating * fNerveDRbonus;
            }
            if (pWeapon) {
                double darmorRating;
                if (pWeapon->GetCurrentAmmo(pAggressor)) {
                    TESAmmo* pammo = pWeapon->GetCurrentAmmo(pAggressor);
                    BSSimpleList<TESAmmoEffect*>* pEffectList = pammo->GetAmmoEffectList();
                    darmorRating = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DR, pEffectList, farmorRating);
                }
                else {
                    darmorRating = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DR, nullptr, farmorRating);
                }
                farmorRating = darmorRating;
            }
            float fmax = fMaxArmorRating.Float() / 100.0;
            if (fmax < static_cast<double>(farmorRating)) {
                farmorRating = fmax;
            }
            float fdamageToArmor;
            if (fFatigueDamage > 0.0) {
                fdamageToArmor = 0.0f;
            }
            else {
                fdamageToArmor = CombatFormulas::CalcDamageToArmor(fHealthDamage, farmorRating);
            }
            fArmorDamage = fdamageToArmor;
            fHealthDamage = (1.0 - farmorRating) * fHealthDamage;
            float fRatioDDTtoDamage = pTarget->GetActorFloatValue(ActorValue::DAMAGE_THRESHOLD);
            double dclampedDT = BSMAX(fRatioDDTtoDamage, 0.0f);
            float fDDT = dclampedDT;
            if (pTarget->bPlayerTeammate) {
                float fCharisma = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
                float fNerveDRbonus = fCharisma * 0.05000000074505806 + 1.0;
                fDDT = fDDT * fNerveDRbonus;
            }
            float fOldDDT = fDDT;
            if (pWeapon) {
                double dadjustedDT;
                if (pWeapon->GetCurrentAmmo(pAggressor)) {
                    TESAmmo* pammo = pWeapon->GetCurrentAmmo(pAggressor);
                    BSSimpleList<TESAmmoEffect*>* pEffectList = pammo->GetAmmoEffectList();
                    dadjustedDT = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DT, pEffectList, fDDT);
                }
                else {
                    dadjustedDT = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DT, nullptr, fDDT);
                }
                fDDT = dadjustedDT;
            }
            if (!abIgnoreBlocking) {
                bool bblocking = !pTarget->IsParalyzed() && pTarget->GetBlocked();
                BaseProcess* pprocess = pTarget->pCurrentProcess;
                bool bunarmedBlock = !pprocess->GetCurrentWeapon() && bblocking;
                if (bblocking) {
                    bool bwithinStrikeAngle;
                    if (pSourceRef) {
                        bwithinStrikeAngle = CombatUtilities::IsTargetWithinStrikeAngle(pTarget, pSourceRef, nullptr);
                    }
                    else {
                        bwithinStrikeAngle = CombatUtilities::IsTargetWithinStrikeAngle(pTarget, pAggressor, nullptr);
                    }
                    if (bwithinStrikeAngle) {
                        TESObjectWEAP* pTargetWeap;
                        if (pTarget->pCurrentProcess->GetCurrentWeapon()) {
                            pTargetWeap = static_cast<TESObjectWEAP*>(
                                pTarget->pCurrentProcess->GetCurrentWeapon()->pContainerObj
                            );
                        }
                        else {
                            pTargetWeap = nullptr;
                        }
                        SetFlag(HitData::HD_BLOCKED, 1);
                        if (pPlayer == pTarget) {
                            pPlayer->SetBlockCounterTimer(fCounterAttackTimer.Float());
                        }
                        bool bweaponDrawn = false;
                        if (pTargetWeap) {
                            bweaponDrawn = pTarget->IsWeaponDrawn();
                        }
                        int32_t iSkill = 0;
                        if (pWeapon &&
                            (pTargetWeap &&
                             (pTargetWeap->GetWeaponType() == WEAPON_TWO_HAND_MELEE ||
                              pTargetWeap->GetWeaponType() == WEAPON_ONE_HAND_MELEE))) {
                            iSkill = pTarget->GetActorValue(ActorValue::MELEE_WEAPONS);
                        }
                        else if (!pWeapon || (pTargetWeap && pTargetWeap->GetWeaponType() == WEAPON_HAND_TO_HAND)) {
                            iSkill = pTarget->GetActorValue(ActorValue::UNARMED);
                        }
                        if (bunarmedBlock && ((pWeapon && !pWeapon->IsMeleeWeapon()) || pSourceRef)) {
                            fPercentBlocked = 0.0f;
                        }
                        else {
                            fPercentBlocked = CombatFormulas::CalcBlock(iSkill);
                        }
                        if (fPercentBlocked > 0.0) {
                            fDDT = fDDT + fPercentBlocked;
                        }
                        SetFlag(HitData::HD_BLOCK_WITH_WEAPON, bweaponDrawn);
                    }
                }
            }
            float fDecreaseDT = 0.0f;
            if (pAggressor) {
                if (!pWeapon) {
                    pWeapon = static_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(500u));
                }
                BGSEntryPoint::HandleEntryPoint(
                    BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_ATTACKER,
                    pAggressor,
                    pWeapon,
                    pTarget,
                    &fDecreaseDT
                );
            }
            float fIncreaseDT = 0.0f;
            if (pTarget) {
                if (pWeapon) {
                    BGSEntryPoint::HandleEntryPoint(
                        BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER,
                        pTarget,
                        pAggressor,
                        pWeapon,
                        &fIncreaseDT
                    );
                }
                else {
                    BGSEntryPoint::HandleEntryPoint(
                        BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER,
                        pTarget,
                        pAggressor,
                        pFists,
                        &fIncreaseDT
                    );
                }
            }
            fDDT = fDDT - fDecreaseDT + fIncreaseDT;
            bool bArmorPierced = false;
            if (fOldDDT > 0.0) {
                bArmorPierced = fDDT <= 0.0;
            }
            fDDT = BSMAX(0.0f, fDDT);
            if (pAggressor && pAggressor->pCurrentProcess) {
                if (pAggressor->pCurrentProcess->GetCurrentWeapon()) {
                    ItemChange* pweaponItem = pAggressor->pCurrentProcess->GetCurrentWeapon();
                    if (pweaponItem->HasModEffectActive(12) && fDDT > 0.0) {
                        int32_t inumProjectiles = pWeapon->GetNumProjectiles(true, false, pAggressor);
                        fDDT = fDDT / static_cast<double>(inumProjectiles);
                    }
                }
            }

            if (pWeapon && (pWeapon->IsAutomaticWeapon() && pWeapon->IsMeleeWeapon())) {
                uint16_t usattackDamage = static_cast<TESAttackDamageForm*>(pWeapon)->GetAttackDamage();
                float fDam = static_cast<float>(BSMAX_3(usattackDamage, 1));
                fRatioDDTtoDamage = 1.0 - fDDT / fDam;
                fRatioDDTtoDamage = BSMAX(fRatioDDTtoDamage, 0.0f);
                fHealthDamage = fHealthDamage * fRatioDDTtoDamage;
            }
            else {
                fHealthDamage = fHealthDamage - fDDT;
            }
            if (pWeapon) {
                double dhealthDamage;
                if (pWeapon->GetCurrentAmmo(pAggressor)) {
                    TESAmmo* pammo = pWeapon->GetCurrentAmmo(pAggressor);
                    BSSimpleList<TESAmmoEffect*>* pEffectList = pammo->GetAmmoEffectList();
                    dhealthDamage = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, pEffectList, fHealthDamage);
                }
                else {
                    dhealthDamage = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, nullptr, fHealthDamage);
                }
                fHealthDamage = dhealthDamage;
            }
            if (pWeapon) {
                BGSEntryPoint::HandleEntryPoint(
                    BGSEntryPoint::CALCULATE_WEAPON_DAMAGE,
                    pAggressor,
                    pWeapon,
                    pTarget,
                    &fHealthDamage
                );
            }
            if (fFatigueDamage <= 0.0) {
                float fdamageAboveMin = fHealthDamage - fMinWeaponDamage;
                double dabsorbedByDT = BSMIN(fDDT, fdamageAboveMin);
                float fArmorDamageFromDT = fDamageToArmorPercentage.Float() * dabsorbedByDT;
                fArmorDamage = fArmorDamage + fArmorDamageFromDT;
            }
            if (!(fMinWeaponDamage > static_cast<double>(fHealthDamage))) {
                if (bArmorPierced) {
                    if (pTarget == pPlayer) {
                        Interface::ShowDDTIcon(bArmorPierced);
                    }
                    else if (pAggressor == pPlayer && !pTarget->IsDead(false)) {
                        Interface::ShowEnemyDDTIcon(bArmorPierced);
                    }
                }
            }
            else {
                fHealthDamage = fMinWeaponDamage;
                if (pTarget == pPlayer) {
                    Interface::ShowDDTIcon(bArmorPierced);
                }
                else if (pAggressor == pPlayer && !pTarget->IsDead(false)) {
                    Interface::ShowEnemyDDTIcon(bArmorPierced);
                }
                if (pAggressor && pAggressor->bPlayerTeammate) {
                    FollowerBarks::TriggerFollowerBark(pAggressor, FollowerBarks::BARK_WEAPON_INEFFECTIVE);
                }
            }
            if (pWeapon && pWeapon->data.eResistance != ActorValue::VALUE_NONE) {
                ActorValue::Index eresistance = pWeapon->data.eResistance;
                float fresistanceValue = pTarget->GetActorFloatValue(eresistance);
                float fresistance = BSMIN(fresistanceValue, 100.0f) / 100.0;
                if (fresistance > 0.0 && fresistance <= 1.0) {
                    fHealthDamage = (1.0 - fresistance) * fHealthDamage;
                }
            }

            if (fFatigueDamage > 0.0 && fTotalDamage > 0.0) {
                float fpercentHealthDamaged = fHealthDamage / fTotalDamage;
                fFatigueDamage = fFatigueDamage * fpercentHealthDamaged;
            }
            if (pTarget == pPlayer && pVATS.IsInVats()) {
                fHealthDamage = fHealthDamage * fVATSPlayerDamageMult.Float();
                fTotalDamage = fTotalDamage * fVATSPlayerDamageMult.Float();
                fFatigueDamage = fFatigueDamage * fVATSPlayerDamageMult.Float();
                fTargetedLimbDamage = fTargetedLimbDamage * fVATSPlayerDamageMult.Float();
                fArmorDamage = fArmorDamage * fVATSPlayerDamageMult.Float();
                fDamageToWeapon = fDamageToWeapon * fVATSPlayerDamageMult.Float();
            }
        }
    }
}

void HitData::DetermineOutcome() {
    if (this->pTarget && (this->pTarget->IsActor() && !static_cast<MagicTarget&>(*this->pTarget).IsInvulnerable()))
    // IsInvulnerable is called through the target's MagicTarget base, with no null test on the adjustment.
    {
        if (fBonusMult > 0.0) {
            if (fBonusMult >= 1.0 && IsFlagSet(HitData::HD_SNEAK_ATTACK)) {
                if (pWeapon && !pWeapon->IsMeleeWeapon()) {
                    fBonusMult = fBonusMult * fCombatDamageBonusSneakingMult.Float();
                }
                else {
                    fBonusMult = fBonusMult * fCombatDamageBonusMeleeSneakingMult.Float();
                }
            }
            fHealthDamage = fHealthDamage * fBonusMult;
            if (pAggressor && (!pAggressor->IsCreature() && eAttackSkill == ActorValue::UNARMED)) {
                fFatigueDamage = fFatigueDamage * fBonusMult;
            }

            fTotalDamage = fTotalDamage * fBonusMult;
        }
        float fshotMultiplier = 1.0f;
        if (pVATSCommand && pVATSCommand->cFireShots > 1u) {
            fshotMultiplier = static_cast<float>(pVATSCommand->cFireShots);
        }

        int32_t iexplodeChance = 0;
        bool balwaysExplode = false;
        bool bexplodeAbsolute = false;
        if (eDamageLimb == BGSBodyPart::LIMB_WEAPON) {
            TESObjectWEAP* ptargetWeapon;
            BaseProcess* pprocess = this->pTarget->pCurrentProcess;
            ItemChange* ptargetWeaponItem = pprocess->GetCurrentWeapon();
            if (ptargetWeaponItem) {
                ptargetWeapon = static_cast<TESObjectWEAP*>(ptargetWeaponItem->pContainerObj);
            }
            else {
                ptargetWeapon = nullptr;
            }
            if (ptargetWeapon) {
                float fcurrentHealth = ptargetWeaponItem->GetItemHealth(false);
                if (fcurrentHealth - fTargetedLimbDamage <= 0.0) {
                    if (this->pTarget->IsCreature() || ptargetWeapon->IsEmbeddedWeapon()) {
                        SetFlag(HitData::HD_DISABLE_WEAPON, 1);
                    }
                    else {
                        SetFlag(HitData::HD_DISARM, 1);
                    }
                }
            }
        }
        else if (eDamageLimb != BGSBodyPart::LIMB_NONE) {
            TESActorBase* pactorBase = this->pTarget->GetActorBase();
            BGSBodyPartData* pbodyPartData = pactorBase->GetBodyPartData();
            if (pbodyPartData) {
                BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart(eDamageLimb);
                if (pbodyPart && pbodyPart->IsExplodablePart()) {
                    iexplodeChance = pbodyPart->GetExplosionChance();
                    bexplodeAbsolute = pbodyPart->IsExplodeChanceAbsolute();
                    balwaysExplode = bexplodeAbsolute && iexplodeChance >= 100;
                    ActorValue::Index econditionAV = static_cast<ActorValue::Index>(pbodyPart->GetActorValue());
                    float flimbCondition = this->pTarget->GetActorFloatValue(econditionAV);
                    if (flimbCondition > 0.0 && flimbCondition - fTargetedLimbDamage <= 0.0) {
                        SetFlag(HitData::HD_CRIPPLE_LIMB, 1);
                    }
                }
            }
        }
        float fcurrentHealth = this->pTarget->GetActorFloatValue(ActorValue::HEALTH);
        if (fcurrentHealth - fHealthDamage * fshotMultiplier <= 0.0) {
            SetFlag(HitData::HD_FATAL, 1);
            if (eDamageLimb != BGSBodyPart::LIMB_NONE) {
                bool bdismember = false;
                bool bexplode = balwaysExplode;
                if (!balwaysExplode) {
                    TESObjectWEAP* phitWeapon;
                    if (pWeapon) {
                        phitWeapon = pWeapon;
                    }
                    else {
                        phitWeapon = eAttackSkill != ActorValue::UNARMED ? nullptr : pFists;
                    }
                    Actor* pTarget = this->pTarget;
                    if (pTarget) {
                        CombatFormulas::CheckDismemberOrExplode(
                            pTarget,
                            phitWeapon,
                            iexplodeChance,
                            0,
                            0,
                            bexplodeAbsolute,
                            bdismember,
                            bexplode
                        );
                    }
                    else {
                        CombatFormulas::CheckDismemberOrExplode(
                            nullptr,
                            phitWeapon,
                            iexplodeChance,
                            0,
                            0,
                            bexplodeAbsolute,
                            bdismember,
                            bexplode
                        );
                    }
                }
                if (bdismember) {
                    SetFlag(HitData::HD_DISMEMBER_LIMB, 1);
                }
                if (bexplode) {
                    SetFlag(HitData::HD_EXPLODE_LIMB, 1);
                }
            }
        }
    }
}

void HitData::SetFlag(Flag aeFlag, uint8_t abValue) {
    if (abValue) {
        uiFlags |= aeFlag;
    }
    else {
        uiFlags &= ~aeFlag;
    }
}

void HitData::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

void HitData::DecRefCount() {
    if (!NiAtomicDecrement(iRefCount)) {
        ourFree(this);
    }
}

void HitData::ClearData() {
    pAggressor = nullptr;
    pTarget = nullptr;
    pSourceRef = nullptr;
    eAttackSkill = ActorValue::ACTOR_VALUE_COUNT;
    eDamageLimb = BGSBodyPart::LIMB_NONE;
    fHealthDamage = 0.0f;
    fTotalDamage = 0.0f;
    fFatigueDamage = 0.0f;
    fTargetedLimbDamage = 0.0f;
    fPercentBlocked = 0.0f;
    fArmorDamage = 0.0f;
    fDamageToWeapon = 0.0f;
    kHitLocation = NiPoint3::ZERO;
    kHitDirection = NiPoint3::ZERO;
    pWeapon = nullptr;
    fWeaponHealth = 0.0f;
    pCriticalEffect = nullptr;
    pVATSCommand = nullptr;
    uiFlags = 0;
    iRefCount = 0;
    fBonusMult = 0.0f;
}

void HitData::Copy(HitData* apSource) {
    pAggressor = apSource->pAggressor;
    pTarget = apSource->pTarget;
    pSourceRef = apSource->pSourceRef;
    eAttackSkill = apSource->eAttackSkill;
    eDamageLimb = apSource->eDamageLimb;
    fHealthDamage = apSource->fHealthDamage;
    fTotalDamage = apSource->fTotalDamage;
    fFatigueDamage = apSource->fFatigueDamage;
    fTargetedLimbDamage = apSource->fTargetedLimbDamage;
    fPercentBlocked = apSource->fPercentBlocked;
    fArmorDamage = apSource->fArmorDamage;
    fDamageToWeapon = apSource->fDamageToWeapon;
    kHitLocation = apSource->kHitLocation;
    kHitDirection = apSource->kHitDirection;
    pWeapon = apSource->pWeapon;
    fWeaponHealth = apSource->fWeaponHealth;
    pCriticalEffect = apSource->pCriticalEffect;
    pVATSCommand = apSource->pVATSCommand;
    uiFlags = apSource->uiFlags;
    fBonusMult = apSource->fBonusMult;
}

void HitData::SetTargetedLimb(Projectile* apProjectile) {
    hkSphere hitSphere;
    if (this->pTarget && this->pTarget->IsActor()) {
        TESActorBase* pactorBase = this->pTarget->GetActorBase();
        BGSBodyPartData* pbodyPartData = pactorBase->GetBodyPartData();
        BGSBodyPart* pbodyPart = nullptr;
        char cpickLimb = 0;
        if (this->pVATSCommand) {
            if (this->pVATSCommand->eLimb == ActorValue::VALUE_NONE) {
                if (ActionPoints::IsMeleeAttackAction(this->pVATSCommand->eAction)) {
                    if (this->IsFlagSet(HitData::HD_IGNORE_CRITICAL)) {
                        this->eDamageLimb = BGSBodyPart::LIMB_TORSO;
                    }
                    else {
                        cpickLimb = 1;
                    }
                }
                else {
                    this->eDamageLimb = BGSBodyPart::LIMB_WEAPON;
                }
            }
            else if (pbodyPartData) {
                BGSBodyPart::LIMB_ENUM epartType;
                pbodyPart = pbodyPartData->GetBodyPart(this->pVATSCommand->eLimb);
                if (pbodyPart) {
                    epartType = pbodyPart->GetPartType();
                }
                else {
                    epartType = BGSBodyPart::LIMB_WEAPON;
                }
                this->eDamageLimb = epartType;
            }
        }
        else if (apProjectile) {
            cpickLimb = 0;
            Projectile::ImpactData* pimpactData = apProjectile->GetImpactData(this->pTarget);
            if (pimpactData) {
                this->eDamageLimb = pimpactData->eDamageLimb;
                pbodyPart = pbodyPartData->GetBodyPart(this->eDamageLimb);
                this->kHitLocation = pimpactData->kLocation;
                this->kHitDirection = apProjectile->GetMovementDirection();
                this->kHitDirection.Unitize();
                NiPoint3 invNormal = -pimpactData->kNormal;
                this->kHitDirection += invNormal;
                this->kHitDirection.Unitize();
            }
        }
        else {
            cpickLimb = 1;
        }
        if (cpickLimb) {
            NiPoint3 viewdir;
            alignas(16) NiPoint3 eyepos;
            NiPoint3 roughHitPos = this->pAggressor->GetWeaponPosition(true);
            this->pAggressor->GetEyeVector(eyepos, viewdir, false);
            NiPoint3 origin = eyepos;
            float fdistance = 200.0f;
            char chitFound = 0;
            alignas(16) BSWeightedRandom<BGSBodyPart*> bodyPartChooser;
            TESObjectCELL* pcell = this->pAggressor->GetParentCell();
            if (pcell) {
                HitCaster* phitCaster = HitCaster::GetInstance(
                    static_cast<HitCaster::HitCasterSize>(this->pAggressor != pPlayer),
                    true
                );
                bhkWorld* pworld = pcell->GetbhkWorld();
                phitCaster->Add(pworld);
                phitCaster->CastShape(origin, viewdir, fdistance);
                hkpAllCdPointCollector* pcollector = phitCaster;
                hkBool bhasHit = pcollector->hasHit();
                if (bhasHit) {
                    for (int32_t i = 0; i < pcollector->getHits().m_size; ++i) {
                        hkpRootCdPoint hitPoint(pcollector->getHits()[i]);
                        NiAVObject* pav = GetAVObjectForCollidable(hitPoint.m_rootCollidableB);
                        Actor* pref = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(pav));
                        if (pref && pref == this->pTarget) {
                            HK2NI(roughHitPos, hitPoint.m_contact.m_position);
                            if (hkpGetRigidBody(hitPoint.m_rootCollidableB)) {
                                BGSBodyPart::LIMB_ENUM elimb;
                                if (!chitFound) {
                                    chitFound = 1;
                                    HK2NI(this->kHitLocation, hitPoint.m_contact.m_position);
                                    this->kHitDirection = viewdir;
                                }
                                if (pav->IsNode()) {
                                    NiNode* pnode = pav->IsNode();
                                    elimb = this->pTarget->GetDamageLocation(pnode);
                                }
                                else {
                                    NiNode* pnode = pav->m_pkParent;
                                    elimb = this->pTarget->GetDamageLocation(pnode);
                                }
                                if (elimb != BGSBodyPart::LIMB_NONE && elimb != BGSBodyPart::LIMB_WEAPON) {
                                    BGSBodyPart* ppossibleHit = pbodyPartData->GetBodyPart(elimb);
                                    if (ppossibleHit && !bodyPartChooser.IsResultInSet(ppossibleHit)) {
                                        uint8_t cToHitChance = ppossibleHit->Data.cToHitChance;
                                        bodyPartChooser.AddWeightedResult(ppossibleHit, cToHitChance);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!chitFound && this->pTarget->Get3D() && this->pAggressor->Get3D()) {
                hkVector4 pos;
                NiPoint3 victimPos = this->pTarget->Get3D()->GetWorldBound().m_kCenter;
                alignas(16) NiPoint3 attackerPos = this->pAggressor->Get3D()->GetWorldBound().m_kCenter;
                this->kHitLocation = victimPos + (this->pAggressor->GetWeaponPosition(true) - victimPos) / 6.0f;
                this->kHitDirection = victimPos - attackerPos;
                ::new (&hitSphere) NiBound;
                NI2HK(pos, roughHitPos);
                hitSphere.setPosition(pos);
                hitSphere.setRadius(200.0f);
                bhkRigidBody* prb = bhkUtilFunctions::GetClosestBody(this->pTarget->Get3D(), hitSphere, 0);
                if (prb) {
                    chitFound = 1;
                    const hkpCollidable* pcollidable = prb->GetCollidable();
                    NiAVObject* pav = GetAVObjectForCollidable(pcollidable);
                    if (pav) {
                        BGSBodyPart::LIMB_ENUM elimb;
                        if (pav->IsNode()) {
                            NiNode* pnode = pav->IsNode();
                            elimb = this->pTarget->GetDamageLocation(pnode);
                        }
                        else {
                            NiNode* pnode = pav->m_pkParent;
                            elimb = this->pTarget->GetDamageLocation(pnode);
                        }
                        if (elimb != BGSBodyPart::LIMB_NONE) {
                            BGSBodyPart* ppossibleHit = pbodyPartData->GetBodyPart(elimb);
                            if (ppossibleHit && !bodyPartChooser.IsResultInSet(ppossibleHit)) {
                                uint8_t cToHitChance = ppossibleHit->Data.cToHitChance;
                                bodyPartChooser.AddWeightedResult(ppossibleHit, cToHitChance);
                            }
                        }
                    }
                }
            }

            if (chitFound) {
                BGSBodyPart::LIMB_ENUM epickedLimb;
                pbodyPart = bodyPartChooser.PickResult();
                if (pbodyPart) {
                    epickedLimb = pbodyPart->GetPartType();
                }
                else {
                    epickedLimb = BGSBodyPart::LIMB_NONE;
                }
                this->eDamageLimb = epickedLimb;
            }
        }
        if (this->eDamageLimb == BGSBodyPart::LIMB_WEAPON) {
            this->fTargetedLimbDamage = this->fHealthDamage;
            if (this->pVATSCommand) {
                this->fTargetedLimbDamage = this->fTargetedLimbDamage * fVATSDamageToWeaponMult.Float();
            }
            this->fHealthDamage = 0.0f;
        }
        else if (pbodyPart) {
            double dlimbDamage;
            this->eDamageLimb = pbodyPart->GetPartType();
            Actor* pTarget = this->pTarget;
            if (pTarget) {
                dlimbDamage = CombatFormulas::CalcTargetedLimbDamage(pTarget, pbodyPart, this->fHealthDamage);
            }
            else {
                dlimbDamage = CombatFormulas::CalcTargetedLimbDamage(nullptr, pbodyPart, this->fHealthDamage);
            }
            this->fTargetedLimbDamage = dlimbDamage;
            if (this->pTarget->IsPc()) {
                this->fTargetedLimbDamage = this->fTargetedLimbDamage * fCombatPlayerLimbDamageMult.Float();
            }
            float fbonusMult = pbodyPart->Data.fDamageMult;
            if (!this->pWeapon || this->pWeapon->IsMeleeWeapon()) {
                fbonusMult = 1.0f;
            }
            if (this->pWeapon) {
                double dlimbDamageMult = this->pWeapon->GetLimbDamageMult();
                this->fTargetedLimbDamage = dlimbDamageMult * this->fTargetedLimbDamage;
            }
            this->SetBonusMult(fbonusMult);
        }
    }
}

// Rolls a critical hit unless the target is invulnerable or the hit ignores criticals: the chance comes from the
// aggressor's critical chance with the weapon (the fists without one), the perk entry points, the VATS bonus and the
// sneak attack multiplier, in tenths of a percent. A critical adds the weapon's critical damage (the unarmed multiple
// of the health damage without a weapon) and records the weapon's critical effect.
void HitData::CheckCriticalStrike() {
    if (pTarget && static_cast<MagicTarget&>(*pTarget).IsInvulnerable()) {
        return;
    }
    if (IsFlagSet(HitData::HD_IGNORE_CRITICAL)) {
        return;
    }
    float fCriticalDamage = 0.0f;
    float fCriticalChance = CombatFormulas::GetCriticalChance(pAggressor, pWeapon ? pWeapon : pFists);
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::CALCULATE_CRITICAL_HIT_CHANCE,
        pAggressor,
        pWeapon ? pWeapon : pFists,
        pTarget,
        &fCriticalChance
    );
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::MODIFY_ENEMY_CRITICAL_HIT_CHANCE,
        pTarget,
        pWeapon ? pWeapon : pFists,
        pAggressor,
        &fCriticalChance
    );
    if (pVATSCommand) {
        fCriticalChance = fCriticalChance + fVATSCriticalChanceBonus.Float();
    }
    fCriticalChance = fCriticalChance * 10.0;
    if (pAggressor &&
        pAggressor->IsPc() &&
        pTarget &&
        pTarget->IsActor() &&
        !pTarget->IsDead(false) &&
        pAggressor->IsSneaking() &&
        !pTarget->IsInCombatWithActor(pAggressor)) {
        bool bLineOfSight = pTarget->LineOfSight(false, pAggressor, true, nullptr, false);
        if (pTarget->GetDetectionLevelAgainstActor(false, pAggressor, bLineOfSight, false, false, false, nullptr) <=
            0) {
            fCriticalChance = fCriticalChance * fCombatSneakAttackBonusMult.Float();
            SetFlag(HitData::HD_SNEAK_ATTACK, 1);
        }
    }
    int32_t iRoll = Random() % 1000;
    if (iRoll < static_cast<int32_t>(fCriticalChance)) {
        SetFlag(HitData::HD_CRITICAL, 1);
        TESObjectWEAP* pCritWeapon = pWeapon ? pWeapon : pFists;
        if (pCritWeapon) {
            double dCriticalDamage;
            if (pWeapon) {
                dCriticalDamage = static_cast<double>(pWeapon->GetCriticalDamage());
            }
            else {
                dCriticalDamage = fHealthDamage * fCombatUnarmedCritDamageMult.Float();
            }
            fCriticalDamage = dCriticalDamage;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::CALCULATE_CRITICAL_HIT_DAMAGE,
                pAggressor,
                pCritWeapon,
                pTarget,
                &fCriticalDamage
            );
            if (fCriticalDamage > 0.0) {
                fHealthDamage = fHealthDamage + fCriticalDamage;
            }
            SetFlag(HitData::HD_CRITICAL_ON_DEATH, pCritWeapon->GetCritDamageFlags());
            pCriticalEffect = pCritWeapon->GetCriticalEffect();
        }
    }
}

void HitData::SetBonusMult(float afNewMult) {
    if (fBonusMult < static_cast<double>(afNewMult)) {
        fBonusMult = afNewMult;
    }
}

// The hit location is the target's location moved a quarter of the way towards the impact, at the impact's height. The
// target's location is read before apTarget is checked for null.
void HitData::InitializeImpactData(
    HitData* apData,
    [[maybe_unused]] TESObjectREFR* apSourceRef,
    Actor* apTarget,
    float afDamage,
    DamageImpactData* apImpactData
) {
    if (!apData) {
        return;
    }
    apData->pAggressor = nullptr;
    apData->pTarget = apTarget;
    apData->fHealthDamage = afDamage;
    apData->fTotalDamage = afDamage;
    NiPoint3 kHitLocation = apTarget->GetLocationOnReference();
    kHitLocation += 0.25f * (apImpactData->kLocation - apTarget->GetLocationOnReference());
    kHitLocation.z = apImpactData->kLocation.z;
    apData->kHitLocation = kHitLocation;
    apData->kHitDirection = apImpactData->kNormal;
    if (apTarget) {
        NiNode* pBone = apTarget->GetClosestBone(
            apImpactData->kLocation - apImpactData->kNormal,
            apImpactData->kLocation
        );
        apData->eDamageLimb = apTarget->GetDamageLocation(pBone);
    }
    apData->ReduceDamage(true);
    apData->DetermineOutcome();
}

void HitData::InitializeHitData(
    HitData* apData,
    Actor* apAggressor,
    Actor* apTarget,
    ItemChange* apWeaponItem,
    bool abPowerAttack
) {
    double ddamage;
    if (apData) {
        float fitemHealth;
        TESObjectWEAP* pweapon;
        apData->pAggressor = apAggressor;
        apData->pTarget = apTarget;
        if (apWeaponItem && apWeaponItem->pContainerObj->GetFormType() == WEAP_ID) {
            pweapon = static_cast<TESObjectWEAP*>(apWeaponItem->pContainerObj);
        }
        else {
            pweapon = nullptr;
        }
        apData->pWeapon = pweapon;
        if (apWeaponItem) {
            fitemHealth = apWeaponItem->GetItemHealth(true);
        }
        else {
            fitemHealth = 1.0f;
        }
        apData->fWeaponHealth = fitemHealth;
        bool bapplyDamage = apData->pVATSCommand ? apData->pVATSCommand->bSuccess : true;
        bool bcalcDamage = bapplyDamage;
        if (apData->pVATSCommand &&
            (!apData->pVATSCommand->bSuccess && (!apData->pWeapon || apData->pWeapon->IsMeleeWeapon()))) {
            bcalcDamage = true;
        }

        uint16_t eanimGroup = apAggressor->GetAnimation()->GetGroup(AGS_WEAPON);
        ANIM_GROUP_ENUM eattackGroup = TESAnimGroup::GetType(eanimGroup);
        if (bcalcDamage) {
            if (apData->pWeapon) {
                float fdamageMultiplier = 1.0f;
                apData->eAttackSkill = apData->pWeapon->GetWeaponSkill();
                if (!apAggressor->IsCreature() && !apAggressor->IsSneaking()) {
                    if (abPowerAttack) {
                        int32_t iskill = apAggressor->GetClampedActorValue(apData->eAttackSkill);
                        fdamageMultiplier = CombatFormulas::GetPowerAttackDamageMultiplier(iskill, eattackGroup);
                    }
                    else {
                        fdamageMultiplier = 1.0f;
                    }
                }
                if (apAggressor) {
                    ddamage = apWeaponItem->GetModifiedAttackDamage(apAggressor, fdamageMultiplier, false);
                }
                else {
                    ddamage = apWeaponItem->GetModifiedAttackDamage(nullptr, fdamageMultiplier, false);
                }
                apData->fHealthDamage = ddamage;
                if (apData->pVATSCommand &&
                    (apData->pVATSCommand->cFireShots > 1u && apData->IsFlagSet(HitData::HD_PREDICT_DAMAGE))) {
                    int32_t iFireShots = apData->pVATSCommand->cFireShots;
                    ddamage = static_cast<double>(iFireShots / apData->pWeapon->GetAmmoPerShot()) *
                        apData->fHealthDamage;
                    apData->fHealthDamage = ddamage;
                }

                if (apData->pVATSCommand &&
                    (apData->pWeapon->IsMeleeWeapon() && apData->pWeapon->IsAutomaticWeapon())) {
                    ddamage = apData->fHealthDamage * fVATSAutomaticMeleeDamageMult.Float();
                    apData->fHealthDamage = ddamage;
                }

                if (apData->pVATSCommand) {
                    switch (apData->pVATSCommand->eAction) {
                        case ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK: {
                            ddamage = apData->pWeapon->specialVATSData.fVATSSpecialMultiplier * apData->fHealthDamage;
                            apData->fHealthDamage = ddamage;
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK: {
                            ddamage = apData->fHealthDamage * fUpperCutVatsMultiplier.Float();
                            apData->fHealthDamage = ddamage;
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2: {
                            ddamage = apData->fHealthDamage * fCrossVatsMultiplier.Float();
                            apData->fHealthDamage = ddamage;
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: {
                            ddamage = apData->fHealthDamage * fGroundAttackVatsMultiplier.Float();
                            apData->fHealthDamage = ddamage;
                            break;
                        }
                    }
                }
            }
            else if (apAggressor->IsCreature()) {
                ddamage = static_cast<double>(apAggressor->GetAttackDamage());
                apData->fHealthDamage = ddamage;
            }
            else {
                float fdamageMultiplier;
                bool bknockedOut = apTarget ? apTarget->IsKnockedOut() : false;
                if (apAggressor) {
                    CombatFormulas::CalcHandToHandDamage(
                        apAggressor,
                        bknockedOut,
                        apData->fHealthDamage,
                        apData->fFatigueDamage
                    );
                }
                else {
                    CombatFormulas::CalcHandToHandDamage(
                        nullptr,
                        bknockedOut,
                        apData->fHealthDamage,
                        apData->fFatigueDamage
                    );
                }
                apData->eAttackSkill = ActorValue::UNARMED;
                if (abPowerAttack) {
                    int32_t iskill = apAggressor->GetClampedActorValue(apData->eAttackSkill);
                    fdamageMultiplier = CombatFormulas::GetPowerAttackDamageMultiplier(iskill, eattackGroup);
                }
                else {
                    fdamageMultiplier = 1.0f;
                }
                ddamage = fdamageMultiplier;
                if (!apAggressor->IsSneaking()) {
                    ddamage = fdamageMultiplier;
                    apData->SetBonusMult(fdamageMultiplier);
                }
            }
            if (apAggressor) {
                ddamage = apAggressor->fRefScale * apData->fHealthDamage;
                apData->fHealthDamage = ddamage;
                if (apAggressor->IsPc() && apData->pWeapon && !apData->pWeapon->IsEmbeddedWeapon()) {
                    ddamage = CombatFormulas::CalcDamageToWeapon(apData->pAggressor);
                    apData->fDamageToWeapon = ddamage;
                }
            }
            if (apAggressor->bPlayerTeammate) {
                float fCharisma = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
                float fNerveBonus = fCharisma * 0.05000000074505806 + 1.0;
                apData->fHealthDamage = apData->fHealthDamage * fNerveBonus;
            }
            apData->CheckCriticalStrike();
            apData->ReduceDamage(false);
        }
        apData->SetTargetedLimb(nullptr);
        if (bcalcDamage) {
            apData->DetermineOutcome();
        }
    }
}

void HitData::InitializeHitData(
    HitData* apData,
    Actor* apAggressor,
    Actor* apTarget,
    ItemChange* apWeaponItem,
    Projectile* apProjectile
) {
    float itemHealth;
    TESObjectWEAP* pweapon;
    if (apData) {
        apData->pAggressor = apAggressor;
        apData->pTarget = apTarget;
        if (apProjectile) {
            pweapon = apProjectile->GetWeaponSource();
        }
        else {
            pweapon = nullptr;
        }
        apData->pWeapon = pweapon;
        if (apWeaponItem) {
            itemHealth = static_cast<float>(apWeaponItem->GetItemHealth(true));
        }
        else {
            itemHealth = 1.0f;
        }
        apData->fWeaponHealth = itemHealth;
        apData->pSourceRef = apProjectile;
        apData->fHealthDamage = apProjectile->GetDamage();
        if (apAggressor) {
            apData->fHealthDamage = apAggressor->fRefScale * apData->fHealthDamage;
        }
        apData->fTotalDamage = apData->fHealthDamage;
        apData->CheckCriticalStrike();
        apData->ReduceDamage(true);
        apData->SetTargetedLimb(apProjectile);
        apData->DetermineOutcome();
    }
}

void HitData::InitializeHitData(HitData* apData, VATS_COMMAND* apCommand) {
    if (apData && apCommand && apCommand->pTarget) {
        Actor* ptarget;
        apData->pVATSCommand = apCommand;
        bool bpowerAttack = ActionPoints::IsMeleeAttackAction(apCommand->eAction) && apCommand->bSuccess;
        if (apCommand->pTarget->IsActor()) {
            ptarget = static_cast<Actor*>(apCommand->pTarget);
        }
        else {
            ptarget = nullptr;
        }
        BaseProcess* pprocess = pPlayer->GetCurrentAIProcess();
        ItemChange* pweaponItem = pprocess->GetCurrentWeapon();
        HitData::InitializeHitData(apData, pPlayer, ptarget, pweaponItem, bpowerAttack);
    }
}

void HitData::InitializeHitData(HitData* apData, Actor* apAggressor, Actor* apTarget, float afDamage) {
    if (!apData) {
        return;
    }
    apData->pAggressor = apAggressor;
    apData->pTarget = apTarget;
    apData->pSourceRef = apAggressor;
    apData->fHealthDamage = afDamage;
    apData->fTotalDamage = afDamage;
    apData->ReduceDamage(false);
    apData->DetermineOutcome();
}

float HitData::PredictVATSCommandHealthDamage(VATS_COMMAND* apVATSCommand) {
    if (apVATSCommand) {
        HitData hitData;
        hitData.SetFlag(HitData::HD_IGNORE_CRITICAL, 1);
        hitData.SetFlag(HitData::HD_PREDICT_DAMAGE, 1);
        bool bsuccess = apVATSCommand->bSuccess;
        apVATSCommand->bSuccess = true;
        HitData::InitializeHitData(&hitData, apVATSCommand);
        apVATSCommand->bSuccess = bsuccess;
        return hitData.fHealthDamage;
    }

    return 0.0f;
}

void HitData::InitializeHitData(
    HitData* apData,
    Actor* apAggressor,
    Actor* apTarget,
    ItemChange* apWeaponItem,
    bool abPowerAttack,
    Projectile* apProjectile
) {
    if (apProjectile) {
        HitData::InitializeHitData(apData, apAggressor, apTarget, apWeaponItem, apProjectile);
    }
    else {
        HitData::InitializeHitData(apData, apAggressor, apTarget, apWeaponItem, abPowerAttack);
    }
}

void HitData::InitializeExplosionData(
    HitData* apData,
    Actor* apAggressor,
    Actor* apTarget,
    TESObjectREFR* apSourceRef,
    float afDamage
) {
    if (apData) {
        apData->pAggressor = apAggressor;
        apData->pTarget = apTarget;
        apData->SetFlag(HitData::HD_EXPLOSION, 1);
        if (apSourceRef && apSourceRef->IsExplosion()) {
            apData->pWeapon = static_cast<Explosion*>(apSourceRef)->Explosion::GetSourceWeapon();
            apData->fWeaponHealth = 1.0f;
            if (static_cast<Explosion*>(apSourceRef)->GetOwner()) {
                TESObjectREFR* powner = static_cast<Explosion*>(apSourceRef)->GetOwner();
                if (powner->IsActor()) {
                    Actor* pshooter = static_cast<Actor*>(static_cast<Explosion*>(apSourceRef)->GetOwner());
                    if (pshooter->GetCurrentWeapon() == apData->pWeapon) {
                        float fitemHealth;
                        ItemChange* pcurrentWeaponItem;
                        if (pshooter->pCurrentProcess) {
                            BaseProcess* pprocess = pshooter->pCurrentProcess;
                            pcurrentWeaponItem = pprocess->GetCurrentWeapon();
                        }
                        else {
                            pcurrentWeaponItem = nullptr;
                        }
                        if (pcurrentWeaponItem) {
                            fitemHealth = pcurrentWeaponItem->GetItemHealth(true);
                        }
                        else {
                            fitemHealth = 1.0f;
                        }
                        apData->fWeaponHealth = fitemHealth;
                    }
                }
            }
        }

        apData->pSourceRef = apSourceRef;
        apData->fHealthDamage = afDamage;
        apData->fTotalDamage = apData->fHealthDamage;
        apData->ReduceDamage(true);
        apData->CheckCriticalStrike();
        apData->DetermineOutcome();
    }
}

NiPoint3 Projectile::GetMovementDirection() {
    return MovementDirection;
}
