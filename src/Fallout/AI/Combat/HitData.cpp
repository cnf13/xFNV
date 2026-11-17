#include "Fallout/AI/Combat/HitData.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSWeightedRandom.h"
#include "BSCore/MemoryManager.h"
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

int32_t BSMAX_3(uint16_t a1, int32_t a2) {
    if (a1 <= a2) {
        return a2;
    }
    else {
        return a1;
    }
}

HitData::HitData() {
    ClearData();
}

void HitData::ReduceDamage(bool abIgnoreBlocking) {
    double v4;
    double v7;
    float fIncreaseDT;
    float fDecreaseDT;
    if (pTarget) {
        if (pTarget->IsActor()) {
            if (static_cast<MagicTarget&>(*pTarget).IsInvulnerable()) {
                fTotalDamage = 0.0f;
                fHealthDamage = 0.0f;
                fFatigueDamage = 0.0f;
            }
            else {
                float v33;
                fTotalDamage = fHealthDamage;
                float fMinWeaponDamage = fHealthDamage * fMinDamMultiplier.Float();
                if (pWeapon) {
                    double v3;
                    if (pWeapon->GetCurrentAmmo(pAggressor)) {
                        TESAmmo* CurrentAmmo = pWeapon->GetCurrentAmmo(pAggressor);
                        BSSimpleList<TESAmmoEffect*>* pEffectList = CurrentAmmo->GetAmmoEffectList();
                        v3 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_FATIGUE, pEffectList, fFatigueDamage);
                    }
                    else {
                        v3 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_FATIGUE, 0, fFatigueDamage);
                    }
                    fFatigueDamage = v3;
                }
                float a1 = pTarget->GetActorFloatValue(ActorValue::DAMAGE_RESISTANCE);
                v4 = BSMIN(a1, 100.0f) / 100.0;
                float farmorRating = v4;
                if (pTarget->bPlayerTeammate) {
                    float afCharisma = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
                    float fNerveDRbonus = afCharisma * 0.05000000074505806 + 1.0;
                    farmorRating = farmorRating * fNerveDRbonus;
                }
                if (pWeapon) {
                    double v6;
                    if (pWeapon->GetCurrentAmmo(pAggressor)) {
                        TESAmmo* v5 = pWeapon->GetCurrentAmmo(pAggressor);
                        BSSimpleList<TESAmmoEffect*>* AmmoEffectList = v5->GetAmmoEffectList();
                        v6 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DR, AmmoEffectList, farmorRating);
                    }
                    else {
                        v6 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DR, 0, farmorRating);
                    }
                    farmorRating = v6;
                }
                float fmax = fMaxArmorRating.Float() / 100.0;
                if (fmax < static_cast<double>(farmorRating)) {
                    farmorRating = fmax;
                }
                if (!(fFatigueDamage > 0.0)) {
                    v33 = CombatFormulas::CalcDamageToArmor(fHealthDamage, farmorRating);
                }
                else {
                    v33 = 0.0f;
                }
                fArmorDamage = v33;
                fHealthDamage = (1.0 - farmorRating) * fHealthDamage;
                float fRatioDDTtoDamage = pTarget->GetActorFloatValue(ActorValue::DAMAGE_THRESHOLD);
                v7 = BSMAX(fRatioDDTtoDamage, 0.0f);
                float fDDT_2 = v7;
                if (pTarget->bPlayerTeammate) {
                    float v46 = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
                    float v45 = v46 * 0.05000000074505806 + 1.0;
                    fDDT_2 = fDDT_2 * v45;
                }
                float fOldDDT = fDDT_2;
                if (pWeapon) {
                    double fDDT;
                    if (pWeapon->GetCurrentAmmo(pAggressor)) {
                        TESAmmo* v8 = pWeapon->GetCurrentAmmo(pAggressor);
                        BSSimpleList<TESAmmoEffect*>* v32 = v8->GetAmmoEffectList();
                        fDDT = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DT, v32, fDDT_2);
                    }
                    else {
                        fDDT = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DT, 0, fDDT_2);
                    }
                    fDDT_2 = fDDT;
                }
                if (!abIgnoreBlocking) {
                    bool v31 = !pTarget->IsParalyzed() && pTarget->GetBlocked();
                    BaseProcess* CurrentAIProcess = pTarget->pCurrentProcess;
                    bool v29 = !CurrentAIProcess->GetCurrentWeapon() && v31;
                    if (v31) {
                        bool IsTargetWithinStrikeAngle;
                        if (pSourceRef) {
                            IsTargetWithinStrikeAngle =
                                CombatUtilities::IsTargetWithinStrikeAngle(pTarget, pSourceRef, 0);
                        }
                        else {
                            IsTargetWithinStrikeAngle =
                                CombatUtilities::IsTargetWithinStrikeAngle(pTarget, pAggressor, 0);
                        }
                        if (IsTargetWithinStrikeAngle) {
                            TESObjectWEAP* pTargetWeap;
                            BaseProcess* v28 = pTarget->pCurrentProcess;
                            if (v28->GetCurrentWeapon()) {
                                BaseProcess* v27 = pTarget->pCurrentProcess;
                                pTargetWeap = static_cast<TESObjectWEAP*>(v27->GetCurrentWeapon()->pContainerObj);
                            }
                            else {
                                pTargetWeap = 0;
                            }
                            SetFlag(HitData::HD_BLOCKED, 1);
                            if (pPlayer == pTarget) {
                                float* v11 = &fCounterAttackTimer.Float();
                                pPlayer->SetBlockCounterTimer(*v11);
                            }
                            bool IsWeaponDrawn = 0;
                            if (pTargetWeap) {
                                IsWeaponDrawn = pTarget->IsWeaponDrawn();
                            }
                            int32_t iSkill = 0;
                            if (pWeapon &&
                                (pTargetWeap &&
                                 (pTargetWeap->GetWeaponType() == WEAPON_TWO_HAND_MELEE ||
                                  pTargetWeap->GetWeaponType() == WEAPON_ONE_HAND_MELEE))) {
                                iSkill = pTarget->GetActorValue(ActorValue::MELEE_WEAPONS);
                            }
                            else {
                                if (!pWeapon || (pTargetWeap && pTargetWeap->GetWeaponType() == WEAPON_HAND_TO_HAND)) {
                                    iSkill = pTarget->GetActorValue(ActorValue::UNARMED);
                                }
                            }
                            if (v29 && ((pWeapon && !pWeapon->IsMeleeWeapon()) || pSourceRef)) {
                                fPercentBlocked = 0.0f;
                            }
                            else {
                                fPercentBlocked = CombatFormulas::CalcBlock(iSkill);
                            }
                            if (fPercentBlocked > 0.0) {
                                fDDT_2 = fDDT_2 + fPercentBlocked;
                            }
                            SetFlag(HitData::HD_BLOCK_WITH_WEAPON, IsWeaponDrawn);
                        }
                    }
                }
                fDecreaseDT = 0.0f;
                if (pAggressor) {
                    if (!pWeapon) {
                        pWeapon = static_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(500u));
                    }
                    BGSEntryPoint::HandleEntryPoint(
                        BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_ATTACKER, pAggressor, pWeapon, pTarget, &fDecreaseDT
                    );
                }
                fIncreaseDT = 0.0f;
                if (pTarget) {
                    if (pWeapon) {
                        BGSEntryPoint::HandleEntryPoint(
                            BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER, pTarget, pAggressor, pWeapon, &fIncreaseDT
                        );
                    }
                    else {
                        BGSEntryPoint::HandleEntryPoint(
                            BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER, pTarget, pAggressor, pFists, &fIncreaseDT
                        );
                    }
                }
                fDDT_2 = fDDT_2 - fDecreaseDT + fIncreaseDT;
                bool bArmorPierced = 0;
                if (fOldDDT > 0.0) {
                    bArmorPierced = fDDT_2 <= 0.0;
                }
                fDDT_2 = BSMAX(0.0f, fDDT_2);
                if (pAggressor) {
                    if (pAggressor->pCurrentProcess) {
                        BaseProcess* v25 = pAggressor->pCurrentProcess;
                        if (v25->GetCurrentWeapon()) {
                            BaseProcess* v24 = pAggressor->pCurrentProcess;
                            ItemChange* v12 = v24->GetCurrentWeapon();
                            if (v12->HasModEffectActive(12) && fDDT_2 > 0.0) {
                                int32_t NumProjectiles = pWeapon->GetNumProjectiles(1, 0, pAggressor);
                                fDDT_2 = fDDT_2 / static_cast<double>(NumProjectiles);
                            }
                        }
                    }
                }
                if (pWeapon && (pWeapon->IsAutomaticWeapon() && pWeapon->IsMeleeWeapon())) {
                    uint16_t v13 = static_cast<TESAttackDamageForm*>(pWeapon)->GetAttackDamage();
                    float fDam = static_cast<float>(BSMAX_3(v13, 1));
                    float fRatioDDTtoDamage_2 = 1.0 - fDDT_2 / fDam;
                    float fRatioDDTtoDamage_3 = BSMAX(fRatioDDTtoDamage_2, 0.0f);
                    fHealthDamage = fHealthDamage * fRatioDDTtoDamage_3;
                }
                else {
                    fHealthDamage = fHealthDamage - fDDT_2;
                }
                if (pWeapon) {
                    double v15;
                    if (pWeapon->GetCurrentAmmo(pAggressor)) {
                        TESAmmo* v14 = pWeapon->GetCurrentAmmo(pAggressor);
                        BSSimpleList<TESAmmoEffect*>* v22 = v14->GetAmmoEffectList();
                        v15 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, v22, fHealthDamage);
                    }
                    else {
                        v15 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, 0, fHealthDamage);
                    }
                    fHealthDamage = v15;
                }
                if (pWeapon) {
                    BGSEntryPoint::HandleEntryPoint(
                        BGSEntryPoint::CALCULATE_WEAPON_DAMAGE, pAggressor, pWeapon, pTarget, &fHealthDamage
                    );
                }
                if (fFatigueDamage <= 0.0) {
                    float v21 = fHealthDamage - fMinWeaponDamage;
                    double v20 = BSMIN(fDDT_2, v21);
                    float fArmorDamageFromDT = fDamageToArmorPercentage.Float() * v20;
                    fArmorDamage = fArmorDamage + fArmorDamageFromDT;
                }
                if (!(fMinWeaponDamage > static_cast<double>(fHealthDamage))) {
                    if (bArmorPierced) {
                        if (pTarget == pPlayer) {
                            Interface::ShowDDTIcon(bArmorPierced);
                        }
                        else {
                            if (pAggressor == pPlayer && !pTarget->IsDead(0)) {
                                Interface::ShowEnemyDDTIcon(bArmorPierced);
                            }
                        }
                    }
                }
                else {
                    fHealthDamage = fMinWeaponDamage;
                    if (pTarget == pPlayer) {
                        Interface::ShowDDTIcon(bArmorPierced);
                    }
                    else {
                        if (pAggressor == pPlayer && !pTarget->IsDead(0)) {
                            Interface::ShowEnemyDDTIcon(bArmorPierced);
                        }
                    }
                    if (pAggressor) {
                        if (pAggressor->bPlayerTeammate) {
                            FollowerBarks::TriggerFollowerBark(pAggressor, FollowerBarks::BARK_WEAPON_INEFFECTIVE);
                        }
                    }
                }
                if (pWeapon) {
                    if (pWeapon->data.eResistance != ActorValue::VALUE_NONE) {
                        ActorValue::Index WeaponEnchantmentVisuals = pWeapon->data.eResistance;
                        float a1a = pTarget->GetActorFloatValue(WeaponEnchantmentVisuals);
                        float fresistance = BSMIN(a1a, 100.0f) / 100.0;
                        if (fresistance > 0.0 && fresistance <= 1.0) {
                            fHealthDamage = (1.0 - fresistance) * fHealthDamage;
                        }
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
}

void HitData::DetermineOutcome() {
    if (this->pTarget) {
        // IsInvulnerable is called through the target's MagicTarget base, with no null test on the adjustment, as the
        // game does.
        if (this->pTarget->IsActor() && !static_cast<MagicTarget&>(*this->pTarget).IsInvulnerable()) {
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
                if (pAggressor) {
                    if (!pAggressor->IsCreature() && eAttackSkill == ActorValue::UNARMED) {
                        fFatigueDamage = fFatigueDamage * fBonusMult;
                    }
                }
                fTotalDamage = fTotalDamage * fBonusMult;
            }
            float fshotMultiplier = 1.0f;
            if (pVATSCommand) {
                if (pVATSCommand->cFireShots > 1u) {
                    fshotMultiplier = static_cast<float>(pVATSCommand->cFireShots);
                }
            }
            int32_t iexplodeChance = 0;
            bool balwaysExplode = 0;
            bool bexplodeAbsolute = 0;
            if (eDamageLimb == BGSBodyPart::LIMB_WEAPON) {
                TESObjectWEAP* ptargetWeapon;
                BaseProcess* CurrentAIProcess = this->pTarget->pCurrentProcess;
                ItemChange* ptargetWeaponItem = CurrentAIProcess->GetCurrentWeapon();
                if (ptargetWeaponItem) {
                    ptargetWeapon = static_cast<TESObjectWEAP*>(ptargetWeaponItem->pContainerObj);
                }
                else {
                    ptargetWeapon = 0;
                }
                if (ptargetWeapon) {
                    float fcurrentHealth = ptargetWeaponItem->GetItemHealth(0);
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
            else {
                if (eDamageLimb != BGSBodyPart::LIMB_NONE) {
                    TESActorBase* ActorBase = this->pTarget->GetActorBase();
                    BGSBodyPartData* pbodyPartData = ActorBase->GetBodyPartData();
                    if (pbodyPartData) {
                        BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart(eDamageLimb);
                        if (pbodyPart) {
                            if (pbodyPart->IsExplodablePart()) {
                                iexplodeChance = pbodyPart->GetExplosionChance();
                                bexplodeAbsolute = pbodyPart->IsExplodeChanceAbsolute();
                                bool v3 = bexplodeAbsolute && iexplodeChance >= 100;
                                balwaysExplode = v3;
                                ActorValue::Index econditionAV =
                                    static_cast<ActorValue::Index>(pbodyPart->GetActorValue());
                                float flimbCondition = this->pTarget->GetActorFloatValue(econditionAV);
                                if (flimbCondition > 0.0 && flimbCondition - fTargetedLimbDamage <= 0.0) {
                                    SetFlag(HitData::HD_CRIPPLE_LIMB, 1);
                                }
                            }
                        }
                    }
                }
            }
            float fcurrentHealth_2 = this->pTarget->GetActorFloatValue(ActorValue::HEALTH);
            if (fcurrentHealth_2 - fHealthDamage * fshotMultiplier <= 0.0) {
                SetFlag(HitData::HD_FATAL, 1);
                if (eDamageLimb != BGSBodyPart::LIMB_NONE) {
                    bool bdismember = 0;
                    bool bexplode = balwaysExplode;
                    if (!balwaysExplode) {
                        TESObjectWEAP* apWeapon;
                        if (pWeapon) {
                            apWeapon = pWeapon;
                        }
                        else {
                            apWeapon = eAttackSkill != ActorValue::UNARMED ? 0 : pFists;
                        }
                        Actor* pTarget = this->pTarget;
                        if (pTarget) {
                            CombatFormulas::CheckDismemberOrExplode(
                                pTarget, apWeapon, iexplodeChance, 0, 0, bexplodeAbsolute, bdismember, bexplode
                            );
                        }
                        else {
                            CombatFormulas::CheckDismemberOrExplode(
                                0, apWeapon, iexplodeChance, 0, 0, bexplodeAbsolute, bdismember, bexplode
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
    fHealthDamage = (float)0.0;
    fTotalDamage = (float)0.0;
    fFatigueDamage = (float)0.0;
    fTargetedLimbDamage = (float)0.0;
    fPercentBlocked = (float)0.0;
    fArmorDamage = (float)0.0;
    fDamageToWeapon = (float)0.0;
    kHitLocation = NiPoint3_ZERO;
    kHitDirection = NiPoint3_ZERO;
    pWeapon = nullptr;
    fWeaponHealth = (float)0.0;
    pCriticalEffect = nullptr;
    pVATSCommand = nullptr;
    uiFlags = (uint32_t)0;
    iRefCount = (uint32_t)0;
    fBonusMult = (float)0.0;
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
    uiFlags = (uint32_t)((int32_t)apSource->uiFlags);
    fBonusMult = apSource->fBonusMult;
}

void HitData::SetTargetedLimb(Projectile* apProjectile) {
    hkSphere hitSphere;
    hkBool v53;
    NiPoint3 v65;
    int32_t v79;
    void* retaddr;
    int32_t* v74 = &v79;
    if (this->pTarget) {
        if (this->pTarget->IsActor()) {
            TESActorBase* ActorBase = this->pTarget->GetActorBase();
            BGSBodyPartData* pbodyPartData = ActorBase->GetBodyPartData();
            BGSBodyPart* pbodyPart = 0;
            char v69 = 0;
            if (this->pVATSCommand) {
                if (this->pVATSCommand->eLimb == ActorValue::VALUE_NONE) {
                    if (ActionPoints::IsMeleeAttackAction(this->pVATSCommand->eAction)) {
                        if (this->IsFlagSet(HitData::HD_IGNORE_CRITICAL)) {
                            this->eDamageLimb = BGSBodyPart::LIMB_TORSO;
                        }
                        else {
                            v69 = 1;
                        }
                    }
                    else {
                        this->eDamageLimb = BGSBodyPart::LIMB_WEAPON;
                    }
                }
                else {
                    if (pbodyPartData) {
                        BGSBodyPart::LIMB_ENUM PartType;
                        pbodyPart = pbodyPartData->GetBodyPart_005E5130(this->pVATSCommand->eLimb);
                        if (pbodyPart) {
                            PartType = pbodyPart->GetPartType();
                        }
                        else {
                            PartType = BGSBodyPart::LIMB_WEAPON;
                        }
                        this->eDamageLimb = PartType;
                    }
                }
            }
            else {
                if (apProjectile) {
                    v69 = 0;
                    Projectile::ImpactData* pimpactData = apProjectile->GetImpactData(this->pTarget);
                    if (pimpactData) {
                        this->eDamageLimb = pimpactData->eDamageLimb;
                        pbodyPart = pbodyPartData->GetBodyPart(this->eDamageLimb);
                        this->kHitLocation = pimpactData->kLocation;
                        this->kHitDirection = apProjectile->GetMovementDirection();
                        this->kHitDirection.Unitize();
                        NiPoint3* fScalar = &(v65 = -pimpactData->kNormal);
                        this->kHitDirection.operator+=(*fScalar);
                        this->kHitDirection.Unitize();
                    }
                }
                else {
                    v69 = 1;
                }
            }
            if (v69) {
                NiPoint3 viewdir;
                alignas(16) NiPoint3 eyepos;
                NiPoint3 roughHitPos = this->pAggressor->GetWeaponPosition(1);
                this->pAggressor->GetEyeVector(eyepos, viewdir, 0);
                NiPoint3 origin = eyepos;
                NiPoint3 direction = viewdir * 200.0f;
                float fdistance = 200.0f;
                char v58 = 0;
                alignas(16) BSWeightedRandom<BGSBodyPart*> bodyPartChooser;
                TESObjectCELL* pcell = this->pAggressor->GetParentCell();
                if (pcell) {
                    HitCaster* phitCaster =
                        HitCaster::GetInstance(static_cast<HitCaster::HitCasterSize>(this->pAggressor != pPlayer), 1);
                    bhkWorld* v3 = pcell->GetbhkWorld();
                    phitCaster->Add(v3);
                    phitCaster->CastShape(origin, viewdir, fdistance);
                    hkpAllCdPointCollector* pcollector = phitCaster;
                    hkBool* v4 = pcollector->hasHit(&v53);
                    if (v4->operator bool()) {
                        for (int32_t i = 0;; ++i) {
                            int32_t FormHealth_0 = (&pcollector->getHits())->m_size;
                            if (i >= FormHealth_0) {
                                break;
                            }
                            const hkpRootCdPoint* v6 = &(&pcollector->getHits())->operator[]<hkpRootCdPoint>(i);
                            hkpRootCdPoint hitPoint(*v6);
                            NiAVObject* pav = GetAVObjectForCollidable(hitPoint.m_rootCollidableB);
                            Actor* pref = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(pav));
                            if (pref) {
                                if (pref == this->pTarget) {
                                    HK2NI(roughHitPos, (&hitPoint.m_contact)->m_position);
                                    if (hkpGetRigidBody(hitPoint.m_rootCollidableB)) {
                                        BGSBodyPart::LIMB_ENUM elimb;
                                        if (!v58) {
                                            v58 = 1;
                                            hkVector4* fScalara = &(&hitPoint.m_contact)->m_position;
                                            HK2NI(this->kHitLocation, *fScalara);
                                            this->kHitDirection = viewdir;
                                        }
                                        if (pav->IsNode()) {
                                            NiNode* v7 = pav->IsNode();
                                            elimb = this->pTarget->GetDamageLocation(v7);
                                        }
                                        else {
                                            NiNode* fScalarb = pav->m_pkParent;
                                            elimb = this->pTarget->GetDamageLocation(fScalarb);
                                        }
                                        BGSBodyPart::LIMB_ENUM elimb_2 = elimb;
                                        if (elimb != BGSBodyPart::LIMB_NONE && elimb_2 != BGSBodyPart::LIMB_WEAPON) {
                                            BGSBodyPart* ppossibleHit = pbodyPartData->GetBodyPart(elimb_2);
                                            if (ppossibleHit) {
                                                if (!bodyPartChooser.IsResultInSet(ppossibleHit)) {
                                                    uint8_t WaitingForRayCasts = ppossibleHit->Data.cToHitChance;
                                                    bodyPartChooser.AddWeightedResult(ppossibleHit, WaitingForRayCasts);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (!v58) {
                    if (this->pTarget->Get3D()) {
                        if (this->pAggressor->Get3D()) {
                            hkVector4 pos;
                            NiNode* v9 = this->pTarget->Get3D();
                            NiPoint3 victimPos = (&v9->GetWorldBound())->m_kCenter;
                            NiNode* v10 = this->pAggressor->Get3D();
                            alignas(16) NiPoint3 attackerPos = (&v10->GetWorldBound())->m_kCenter;
                            this->kHitLocation =
                                victimPos + (this->pAggressor->GetWeaponPosition(1) - victimPos) / 6.0f;
                            this->kHitDirection = victimPos - attackerPos;
                            ::new (&hitSphere) NiBound;
                            NI2HK(pos, roughHitPos);
                            hitSphere.setPosition(pos);
                            hitSphere.setRadius(200.0f);
                            NiNode* v16 = this->pTarget->Get3D();
                            bhkRigidBody* prb = bhkUtilFunctions::GetClosestBody(v16, hitSphere, 0);
                            if (prb) {
                                v58 = 1;
                                const hkpCollidable* Collidable = prb->GetCollidable();
                                NiAVObject* pav_2 = GetAVObjectForCollidable(Collidable);
                                if (pav_2) {
                                    BGSBodyPart::LIMB_ENUM DamageLocation;
                                    if (pav_2->IsNode()) {
                                        NiNode* v18 = pav_2->IsNode();
                                        DamageLocation = this->pTarget->GetDamageLocation(v18);
                                    }
                                    else {
                                        NiNode* fScalarc = pav_2->m_pkParent;
                                        DamageLocation = this->pTarget->GetDamageLocation(fScalarc);
                                    }
                                    if (DamageLocation != BGSBodyPart::LIMB_NONE) {
                                        BGSBodyPart* ppossibleHit_2 = pbodyPartData->GetBodyPart(DamageLocation);
                                        if (ppossibleHit_2) {
                                            if (!bodyPartChooser.IsResultInSet(ppossibleHit_2)) {
                                                uint8_t v19 = ppossibleHit_2->Data.cToHitChance;
                                                bodyPartChooser.AddWeightedResult(ppossibleHit_2, v19);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (v58) {
                    BGSBodyPart::LIMB_ENUM v31;
                    pbodyPart = bodyPartChooser.PickResult();
                    if (pbodyPart) {
                        v31 = pbodyPart->GetPartType();
                    }
                    else {
                        v31 = BGSBodyPart::LIMB_NONE;
                    }
                    this->eDamageLimb = v31;
                }
            }
            if (this->eDamageLimb == BGSBodyPart::LIMB_WEAPON) {
                this->fTargetedLimbDamage = this->fHealthDamage;
                if (this->pVATSCommand) {
                    float* v20 = &fVATSDamageToWeaponMult.Float();
                    this->fTargetedLimbDamage = this->fTargetedLimbDamage * *v20;
                }
                this->fHealthDamage = 0.0f;
            }
            else {
                if (pbodyPart) {
                    double v21;
                    BGSBodyPart::LIMB_ENUM v30 = pbodyPart->GetPartType();
                    this->eDamageLimb = v30;
                    Actor* pTarget = this->pTarget;
                    if (pTarget) {
                        v21 = CombatFormulas::CalcTargetedLimbDamage(pTarget, pbodyPart, this->fHealthDamage);
                    }
                    else {
                        v21 = CombatFormulas::CalcTargetedLimbDamage(0, pbodyPart, this->fHealthDamage);
                    }
                    this->fTargetedLimbDamage = v21;
                    if (this->pTarget->IsPc()) {
                        float* v22 = &fCombatPlayerLimbDamageMult.Float();
                        this->fTargetedLimbDamage = this->fTargetedLimbDamage * *v22;
                    }
                    float fbonusMult = pbodyPart->Data.fDamageMult;
                    if (!this->pWeapon || this->pWeapon->IsMeleeWeapon()) {
                        fbonusMult = 1.0f;
                    }
                    if (this->pWeapon) {
                        double LimbDamageMult = this->pWeapon->GetLimbDamageMult();
                        this->fTargetedLimbDamage = LimbDamageMult * this->fTargetedLimbDamage;
                    }
                    this->SetBonusMult(fbonusMult);
                }
            }
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
        BGSEntryPoint::CALCULATE_CRITICAL_HIT_CHANCE, pAggressor, pWeapon ? pWeapon : pFists, pTarget, &fCriticalChance
    );
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::MODIFY_ENEMY_CRITICAL_HIT_CHANCE, pTarget, pWeapon ? pWeapon : pFists, pAggressor,
        &fCriticalChance
    );
    if (pVATSCommand) {
        float* pBonus = &fVATSCriticalChanceBonus.Float();
        fCriticalChance = fCriticalChance + *pBonus;
    }
    fCriticalChance = fCriticalChance * 10.0;
    if (pAggressor && pAggressor->IsPc() && pTarget && pTarget->IsActor() && !pTarget->IsDead(0) &&
        pAggressor->IsSneaking() && !pTarget->IsInCombatWithActor(pAggressor)) {
        bool bLineOfSight = pTarget->LineOfSight(0, pAggressor, 1, 0, 0);
        if (pTarget->GetDetectionLevelAgainstActor(0, pAggressor, bLineOfSight, 0, 0, 0, 0) <= 0) {
            float* pMult = &fCombatSneakAttackBonusMult.Float();
            fCriticalChance = fCriticalChance * *pMult;
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
                BGSEntryPoint::CALCULATE_CRITICAL_HIT_DAMAGE, pAggressor, pCritWeapon, pTarget, &fCriticalDamage
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
    if (fBonusMult < ((double)afNewMult)) {
        fBonusMult = afNewMult;
    }
}

// The hit location is the target's location moved a quarter of the way towards the impact, at the impact's height. The
// target's location is read before apTarget is checked for null, as the game does.
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
        NiNode* pBone =
            apTarget->GetClosestBone(apImpactData->kLocation - apImpactData->kNormal, apImpactData->kLocation);
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
    double ModifiedAttackDamage;
    if (apData) {
        bool bapplyDamage;
        float ItemHealth;
        TESObjectWEAP* FormFlags;
        apData->pAggressor = apAggressor;
        apData->pTarget = apTarget;
        if (apWeaponItem && apWeaponItem->pContainerObj->GetFormType() == WEAP_ID) {
            FormFlags = static_cast<TESObjectWEAP*>(apWeaponItem->pContainerObj);
        }
        else {
            FormFlags = 0;
        }
        apData->pWeapon = FormFlags;
        if (apWeaponItem) {
            ItemHealth = apWeaponItem->GetItemHealth(1);
        }
        else {
            ItemHealth = 1.0f;
        }
        apData->fWeaponHealth = ItemHealth;
        if (apData->pVATSCommand) {
            bapplyDamage = apData->pVATSCommand->bSuccess;
        }
        else {
            bapplyDamage = 1;
        }
        bool v20 = bapplyDamage;
        if (apData->pVATSCommand) {
            if (!apData->pVATSCommand->bSuccess && (!apData->pWeapon || apData->pWeapon->IsMeleeWeapon())) {
                v20 = 1;
            }
        }
        Animation* v5 = apAggressor->GetAnimation();
        uint16_t eanimGroup = v5->GetGroup(AGS_WEAPON);
        ANIM_GROUP_ENUM eattackGroup = TESAnimGroup::GetType(eanimGroup);
        if (v20) {
            if (apData->pWeapon) {
                float fdamageMultiplier = 1.0f;
                apData->eAttackSkill = apData->pWeapon->GetWeaponSkill();
                if (!apAggressor->IsCreature() && !apAggressor->IsSneaking()) {
                    float fdamageMultiplier_2;
                    if (abPowerAttack) {
                        int32_t ClampedActorValue = apAggressor->GetClampedActorValue(apData->eAttackSkill);
                        fdamageMultiplier_2 =
                            CombatFormulas::GetPowerAttackDamageMultiplier(ClampedActorValue, eattackGroup);
                    }
                    else {
                        fdamageMultiplier_2 = 1.0f;
                    }
                    fdamageMultiplier = fdamageMultiplier_2;
                }
                if (apAggressor) {
                    ModifiedAttackDamage = apWeaponItem->GetModifiedAttackDamage(apAggressor, fdamageMultiplier, 0);
                }
                else {
                    ModifiedAttackDamage = apWeaponItem->GetModifiedAttackDamage(0, fdamageMultiplier, 0);
                }
                apData->fHealthDamage = ModifiedAttackDamage;
                if (apData->pVATSCommand) {
                    if (apData->pVATSCommand->cFireShots > 1u && apData->IsFlagSet(HitData::HD_PREDICT_DAMAGE)) {
                        int32_t cFireShots = apData->pVATSCommand->cFireShots;
                        ModifiedAttackDamage =
                            static_cast<double>(cFireShots / apData->pWeapon->GetAmmoPerShot()) * apData->fHealthDamage;
                        apData->fHealthDamage = ModifiedAttackDamage;
                    }
                }
                if (apData->pVATSCommand) {
                    if (apData->pWeapon->IsMeleeWeapon() && apData->pWeapon->IsAutomaticWeapon()) {
                        ModifiedAttackDamage = apData->fHealthDamage * fVATSAutomaticMeleeDamageMult.Float();
                        apData->fHealthDamage = ModifiedAttackDamage;
                    }
                }
                if (apData->pVATSCommand) {
                    switch (apData->pVATSCommand->eAction) {
                        case ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK: {
                            ModifiedAttackDamage =
                                apData->pWeapon->specialVATSData.fVATSSpecialMultiplier * apData->fHealthDamage;
                            apData->fHealthDamage = ModifiedAttackDamage;
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK: {
                            ModifiedAttackDamage = apData->fHealthDamage * fUpperCutVatsMultiplier.Float();
                            apData->fHealthDamage = ModifiedAttackDamage;
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2: {
                            ModifiedAttackDamage = apData->fHealthDamage * fCrossVatsMultiplier.Float();
                            apData->fHealthDamage = ModifiedAttackDamage;
                            break;
                        }
                        case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3: {
                            ModifiedAttackDamage = apData->fHealthDamage * fGroundAttackVatsMultiplier.Float();
                            apData->fHealthDamage = ModifiedAttackDamage;
                            break;
                        }
                    }
                }
            }
            else {
                if (apAggressor->IsCreature()) {
                    ModifiedAttackDamage = static_cast<double>(apAggressor->GetAttackDamage());
                    apData->fHealthDamage = ModifiedAttackDamage;
                }
                else {
                    float fdamageMultiplier_3;
                    bool v11;
                    if (apTarget) {
                        v11 = apTarget->IsKnockedOut();
                    }
                    else {
                        v11 = 0;
                    }
                    if (apAggressor) {
                        CombatFormulas::CalcHandToHandDamage(
                            apAggressor, v11, apData->fHealthDamage, apData->fFatigueDamage
                        );
                    }
                    else {
                        CombatFormulas::CalcHandToHandDamage(0, v11, apData->fHealthDamage, apData->fFatigueDamage);
                    }
                    apData->eAttackSkill = ActorValue::UNARMED;
                    if (abPowerAttack) {
                        int32_t v9 = apAggressor->GetClampedActorValue(apData->eAttackSkill);
                        fdamageMultiplier_3 = CombatFormulas::GetPowerAttackDamageMultiplier(v9, eattackGroup);
                    }
                    else {
                        fdamageMultiplier_3 = 1.0f;
                    }
                    ModifiedAttackDamage = fdamageMultiplier_3;
                    if (!apAggressor->IsSneaking()) {
                        ModifiedAttackDamage = fdamageMultiplier_3;
                        apData->SetBonusMult(fdamageMultiplier_3);
                    }
                }
            }
            if (apAggressor) {
                ModifiedAttackDamage = apAggressor->fRefScale * apData->fHealthDamage;
                apData->fHealthDamage = ModifiedAttackDamage;
                if (apAggressor->IsPc()) {
                    if (apData->pWeapon) {
                        if (!apData->pWeapon->IsEmbeddedWeapon()) {
                            ModifiedAttackDamage = CombatFormulas::CalcDamageToWeapon(apData->pAggressor);
                            apData->fDamageToWeapon = ModifiedAttackDamage;
                        }
                    }
                }
            }
            if (apAggressor->bPlayerTeammate) {
                float afCharisma = pPlayer->GetClampedActorFloatValue(ActorValue::CHARISMA);
                float fNerveBonus = afCharisma * 0.05000000074505806 + 1.0;
                apData->fHealthDamage = apData->fHealthDamage * fNerveBonus;
            }
            apData->CheckCriticalStrike();
            apData->ReduceDamage(0);
        }
        apData->SetTargetedLimb(0);
        if (v20) {
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
    TESObjectWEAP* mode;
    if (apData) {
        apData->pAggressor = apAggressor;
        apData->pTarget = apTarget;
        if (apProjectile) {
            mode = apProjectile->GetWeaponSource();
        }
        else {
            mode = nullptr;
        }
        apData->pWeapon = mode;
        if (apWeaponItem) {
            itemHealth = (float)apWeaponItem->GetItemHealth(1);
        }
        else {
            itemHealth = (float)1.0;
        }
        apData->fWeaponHealth = itemHealth;
        apData->pSourceRef = apProjectile;
        apData->fHealthDamage = apProjectile->GetDamage();
        if (apAggressor) {
            apData->fHealthDamage = (float)(apAggressor->fRefScale * apData->fHealthDamage);
        }
        apData->fTotalDamage = apData->fHealthDamage;
        apData->CheckCriticalStrike();
        apData->ReduceDamage(1);
        apData->SetTargetedLimb(apProjectile);
        apData->DetermineOutcome();
    }
}

void HitData::InitializeHitData(HitData* apData, VATS_COMMAND* apCommand) {
    if (apData) {
        if (apCommand) {
            if (apCommand->pTarget) {
                Actor* apTarget;
                apData->pVATSCommand = apCommand;
                bool v5 = ActionPoints::IsMeleeAttackAction(apCommand->eAction) && apCommand->bSuccess;
                if (apCommand->pTarget->IsActor()) {
                    apTarget = static_cast<Actor*>(apCommand->pTarget);
                }
                else {
                    apTarget = 0;
                }
                BaseProcess* CurrentAIProcess = pPlayer->GetCurrentAIProcess();
                ItemChange* v2 = CurrentAIProcess->GetCurrentWeapon();
                HitData::InitializeHitData(apData, pPlayer, apTarget, v2, v5);
            }
        }
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
    HitData hitData;
    if (apVATSCommand) {
        hitData.~HitData();
        ::new (&hitData) HitData;
        hitData.SetFlag(HitData::HD_IGNORE_CRITICAL, 1);
        hitData.SetFlag(HitData::HD_PREDICT_DAMAGE, 1);
        bool bsuccess = apVATSCommand->bSuccess;
        apVATSCommand->bSuccess = 1;
        HitData::InitializeHitData(&hitData, apVATSCommand);
        apVATSCommand->bSuccess = bsuccess;
        return hitData.fHealthDamage;
    }
    else {
        return 0.0f;
    }
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
        if (apSourceRef) {
            if (apSourceRef->IsExplosion()) {
                apData->pWeapon = static_cast<Explosion*>(apSourceRef)->Explosion::GetSourceWeapon();
                apData->fWeaponHealth = 1.0f;
                if (static_cast<Explosion*>(apSourceRef)->GetOwner()) {
                    TESObjectREFR* Owner = static_cast<Explosion*>(apSourceRef)->GetOwner();
                    if (Owner->IsActor()) {
                        Actor* pshooter = static_cast<Actor*>(static_cast<Explosion*>(apSourceRef)->GetOwner());
                        if (pshooter->GetCurrentWeapon() == apData->pWeapon) {
                            float ItemHealth;
                            ItemChange* pcurrentWeaponItem;
                            if (pshooter->pCurrentProcess) {
                                BaseProcess* CurrentAIProcess = pshooter->pCurrentProcess;
                                pcurrentWeaponItem = CurrentAIProcess->GetCurrentWeapon();
                            }
                            else {
                                pcurrentWeaponItem = 0;
                            }
                            if (pcurrentWeaponItem) {
                                ItemHealth = pcurrentWeaponItem->GetItemHealth(1);
                            }
                            else {
                                ItemHealth = 1.0f;
                            }
                            apData->fWeaponHealth = ItemHealth;
                        }
                    }
                }
            }
        }
        apData->pSourceRef = apSourceRef;
        apData->fHealthDamage = afDamage;
        apData->fTotalDamage = apData->fHealthDamage;
        apData->ReduceDamage(1);
        apData->CheckCriticalStrike();
        apData->DetermineOutcome();
    }
}

NiPoint3 Projectile::GetMovementDirection() {
    return MovementDirection;
}
