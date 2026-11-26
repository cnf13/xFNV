#include "Fallout/AI/Combat/CombatUtilities.h"

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAmmoEffect.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"

#include <cmath>
#include <cstdint>
#include <limits>

class BipedAnim;

// One attacker's combat position while it closes in for a melee attack.
struct MeleeAttackerData {
    CombatController* pCombatController;
    NiPoint3 Location;
    float fAngle;
    char cSlot;
    bool bInMelee;
};

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fLowPerfCombatantVoiceDistance("fLowPerfCombatantVoiceDistance", 1000.0f);
SettingT<INISettingCollection> bDebugCombatTextColorDark("bDebugCombatTextColorDark:Combat", false);
SettingT<INISettingCollection> fDebugCombatTextSize("fDebugCombatTextSize:Combat", 0.5f);
SettingT<INISettingCollection> bDebugCombatProjectileLOS("bDebugCombatProjectileLOS:Combat", false);
SettingT<INISettingCollection> fDebugCombatProjectileLOSTime("fDebugCombatProjectileLOSTime:Combat", 5.0f);
BSPerformanceTimer CombatAI_LOS("Combat AI", "LOS", true, true);

// acSlot is sign-extended; the slot count is divided in as an integer (fidiv) and the result is rounded to a float
// local.
float GetAngleFromSlot(char acSlot) {
    float fAngle = static_cast<float>(acSlot * 360.0 / MAX_SLOTS);
    return fAngle;
}

bool IsSlotReserved(uint32_t aiSlots, char acSlot) {
    if ((aiSlots & 1 << acSlot) != 0) {
        return true;
    }
    for (int32_t i = 1; i <= SLOT_BORDER; ++i) {
        if ((aiSlots & 1 << RotateSlot(acSlot, -static_cast<char>(i))) != 0) {
            return true;
        }
        if ((aiSlots & 1 << RotateSlot(acSlot, i)) != 0) {
            return true;
        }
    }
    return false;
}

char RotateSlot(char acSlot, char acAmount) {
    acSlot = acAmount + acSlot;
    if (acSlot < 0) {
        acSlot += MAX_SLOTS;
    }
    if (acSlot >= MAX_SLOTS) {
        acSlot -= MAX_SLOTS;
    }
    return acSlot;
}

char GetSlotFromAngle(float afAngle) {
    return static_cast<int8_t>(static_cast<int32_t>(afAngle / 360.0 * MAX_SLOTS));
}

uint32_t ReserveSlot(uint32_t aiSlots, char acSlot) {
    aiSlots = aiSlots | 1 << acSlot;
    for (int32_t i = 1; i <= SLOT_BORDER; ++i) {
        aiSlots = aiSlots | 1 << RotateSlot(acSlot, -static_cast<int8_t>(i));
        aiSlots = aiSlots | 1 << RotateSlot(acSlot, i);
    }
    return aiSlots;
}

char FindSlot(uint32_t aiSlots, char acDesiredSlot) {
    for (int32_t i = 0; i < MAX_SLOTS / 2; ++i) {
        char cSlot = RotateSlot(acDesiredSlot, i);
        if (!IsSlotReserved(aiSlots, cSlot)) {
            return cSlot;
        }
        cSlot = RotateSlot(acDesiredSlot, -static_cast<char>(i));
        if (!IsSlotReserved(aiSlots, cSlot)) {
            return cSlot;
        }
    }
    return -1;
}

// Calls hkVector4's non-const operator() on the returned gravity.
void CombatUtilities::Init() {
    hkVector4& kGravity = const_cast<hkVector4&>(TESObjectCELL::ExteriorWorld()->GetGravity());
    fWorldGravity = -HK2NI(kGravity(2));
}

bool CombatUtilities::IsTargetWithinStrikeAngle(Actor* apAttacker, TESObjectREFR* apTarget, float* afDelta) {
    bool bResult = false;
    ANIM_GROUP_ENUM eAttack = ANIM_GROUP_ATTACK_LEFT;
    if (apAttacker == pPlayer && apAttacker->pCurrentProcess) {
        if (apAttacker->pCurrentProcess->GetAnimActionAnimSeq()) {
            if (apAttacker->pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()) {
                eAttack = apAttacker->pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType();
            }
        }
    }

    const NiPoint3& kAttackerPos = apAttacker->GetLocationOnReference();
    const NiPoint3& kTargetPos = apTarget->GetLocationOnReference();
    NiPoint3 movedir = kTargetPos - kAttackerPos;
    float fTargetAngle = GetUnclampedZAngleFromVector(movedir);
    float fAddedHeadingRotation = 0.0f;
    if (apAttacker == pPlayer) {
        float fDeltaHeading = 0.0f;
        float fDeltaLooking = 0.0f;
        NiAVObject* p3D = pPlayer->Get3D(false);
        NiAVObject* pProjectile = BSUtilities::GetObjectByName(p3D, "ProjectileNode");
        pPlayer->ApplyAutoAimActor(
            nullptr,
            pProjectile,
            fDeltaHeading,
            fDeltaLooking,
            nullptr,
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min()
        );
        if (pPlayer->GetAutoAimActor()) {
            fAddedHeadingRotation = fDeltaHeading;
        }
    }
    double dHeading = apAttacker->GetHeading(false);
    float fHeadingDiff = dHeading + fAddedHeadingRotation - fTargetAngle;
    float fDelta = std::fabsf(fHeadingDiff) * 57.2957763671875;
    if (fDelta > 180.0) {
        float fWrapped = fDelta - 360.0;
        fDelta = std::fabs(fWrapped);
    }
    float fAngle = 0.0f;
    if (apAttacker == pPlayer) {
        fAngle = CombatUtilities::GetAngleForMeleeAttack(eAttack);
    }
    else {
        fAngle = fCombatHitConeAngle.Float();
    }
    if (movedir.Length() < 128.0) {
        fAngle = fAngle * 3.0;
    }
    if (fAngle >= static_cast<double>(fDelta)) {
        bResult = true;
    }
    else if (apTarget->IsActor() && apTarget->IsDead(false)) {
        fAngle = fAngle * fCombatDeadActorHitConeMult.Float();
        if (fAngle >= static_cast<double>(fDelta)) {
            bResult = true;
        }
    }
    if (afDelta) {
        *afDelta = fDelta;
    }
    return bResult;
}

Actor* CombatUtilities::GetFacingTarget(Actor* apAttacker, float afRadius) {
    Actor* pTarget = apAttacker->GetCombatController() ? apAttacker->GetCombatController()->GetTarget() : nullptr;
    if (pTarget &&
        CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pTarget, nullptr) &&
        static_cast<double>(afRadius) >= CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pTarget, true)) {
        return pTarget;
    }
    if (pTarget) {
        return nullptr;
    }
    Actor* pClosestTarget = nullptr;
    float fClosestDelta = std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        MobileObject* pMob = ReferenceProcessLists.MobProcessArray.GetItem(i);
        Actor* pActor = nullptr;
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        if (pVATS.eMode != VATS::VATS_PLAYBACK || !pActor || !pActor->IsDead(false)) {
            if (pActor &&
                pActor != apAttacker &&
                (!pActor->IsDead(false) || apAttacker == pPlayer) &&
                pActor->Get3D() &&
                static_cast<double>(afRadius) >=
                    CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pActor, false)) {
                float fDelta = std::numeric_limits<float>::max();
                if (CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pActor, &fDelta) &&
                    fClosestDelta >= static_cast<double>(fDelta)) {
                    pClosestTarget = pActor;
                    fClosestDelta = fDelta;
                }
            }

            if (pActor) {
                ExtraFollower* pFollow = pActor->m_Extra.GetFollowerExtra();
                if (pFollow) {
                    for (Actor* pAct : pFollow->pActorlist->ItemsUntilNull()) {
                        if (pAct &&
                            pAct->GetCurrentProcessType() == PROCESS_TYPE_HIGH &&
                            pAct != apAttacker &&
                            (!pActor->IsDead(false) || apAttacker == pPlayer) &&
                            pAct->Get3D() &&
                            static_cast<double>(afRadius) >=
                                CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pAct, false)) {
                            float fDelta = std::numeric_limits<float>::max();
                            if (CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pAct, &fDelta) &&
                                fClosestDelta >= static_cast<double>(fDelta)) {
                                pClosestTarget = pAct;
                                fClosestDelta = fDelta;
                            }
                        }
                    }
                }
            }
        }
    }
    if (apAttacker != pPlayer) {
        float fPlayerDistance = CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pPlayer, false);
        float fDelta = std::numeric_limits<float>::max();
        bool bInStrikeAngle = CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pPlayer, &fDelta);
        if (afRadius >= static_cast<double>(fPlayerDistance) &&
            bInStrikeAngle &&
            fClosestDelta >= static_cast<double>(fDelta)) {
            pClosestTarget = pPlayer;
        }
    }
    if (pClosestTarget && apAttacker->IsPc() && !pPlayer->LineOfSight(true, pClosestTarget, true, nullptr, false)) {
        return nullptr;
    }

    return pClosestTarget;
}

float CombatUtilities::GetAttackerFiringZOffset(TESObjectREFR* apAttacker, bool abIgnoreCrouch) {
    if (apAttacker->IsActor()) {
        float fScale;
        Actor* pActor = static_cast<Actor*>(apAttacker);
        if (!pActor->IsSneaking() || abIgnoreCrouch) {
            fScale = pActor->GetHeight();
        }
        else {
            fScale = pActor->GetSneakHeight(false);
        }
        return fScale * 0.75;
    }

    NiPoint3 kBoundMax = apAttacker->GetBoundMax();
    NiPoint3 kBoundMin = apAttacker->GetBoundMin();
    return (kBoundMax.z - kBoundMin.z) * 0.5;
}

NiPoint3 CombatUtilities::ComputeFaceTargetLocation(
    Actor* apAttacker,
    TESObjectREFR* apTarget,
    const NiPoint3& arTargetLocation,
    float afAttackerReach
) {
    NiPoint3 targetVector = NiPoint3::ZERO;
    Actor* pTargetActor = apTarget->IsActor() ? static_cast<Actor*>(apTarget) : nullptr;
    CombatController* pCombatController = apAttacker->GetCombatController();
    if (pCombatController &&
        pTargetActor &&
        CombatUtilities::CalculateAttackerCombatPositions(pTargetActor, apAttacker)) {
        float fCombatPosition = pCombatController->GetCombatPosition();
        float fAngleRad = fCombatPosition * 0.01745329238474369;
        NiSinCos(fAngleRad, targetVector.x, targetVector.y);
    }
    else {
        NiPoint3 kToAttacker = apAttacker->GetLocationOnReference() - arTargetLocation;
        kToAttacker.Unitize();
    }
    float fScaledReach = CombatUtilities::ComputeFaceTargetDistance(apAttacker, apTarget, afAttackerReach);
    targetVector *= fScaledReach;
    return arTargetLocation + targetVector;
}

bool CombatUtilities::IsActorUsingMelee(Actor* apActor) {
    CombatController* pCombatController = apActor->GetCombatController();
    TESObjectWEAP* pWeapon = pCombatController ? pCombatController->GetCurrentWeapon() : apActor->GetCurrentWeapon();
    if (!pWeapon || pWeapon->IsMeleeWeapon()) {
        return true;
    }
    return pCombatController &&
        (pCombatController->CombatPlanContainsAction(CombatAction::COMBAT_ACTION_ATTACK_MELEE) ||
         pCombatController->CombatPlanContainsAction(CombatAction::COMBAT_ACTION_ATTACK_HAND_TO_HAND));
}

float CombatUtilities::CalculateItemDPS(
    ActorValueOwner* apActor,
    TESObjectWEAP* apWeapon,
    float afCondition,
    bool abApplyPerks,
    ItemChange* apItem,
    bool abAlwaysAddReload,
    bool abMaxJamChance,
    int32_t aiSkillOverride,
    float afCritChance,
    float afCritDamageMult,
    bool abDoubleDamage,
    bool abBonusDamage,
    TESAmmo* apAmmo
) {
    if (afCondition <= 0.0) {
        return 0.0f;
    }
    int32_t iClipSize = 1;
    float fDamagePerHit = 0.0f;
    float fCritChance = 0.0f;
    float fSecondsPerClip = std::numeric_limits<float>::max();
    bool bCreature = false;
    bool bHasClipIncrease = false;
    float fAnimShotsPerSecond = apWeapon ? apWeapon->GetAnimShotsPerSecond() : 1.0f;
    bool bHasSplitBeam = false;
    float fScaledCondition = afCondition * 10.0;
    int32_t iRoundedCondition = static_cast<int32_t>(BSRoundToNearest(fScaledCondition, 1.0f));
    float fBonus = 0.0f;
    // Compares the two forms' addresses.
    if (static_cast<TESForm*>(apAmmo) == static_cast<TESForm*>(apWeapon)) {
        apAmmo = nullptr;
    }
    Actor* pActor = static_cast<Actor*>(apActor);
    bCreature = pActor->IsCreature();
    if (apItem) {
        apItem->HasModEffectActive(WEAPON_MOD_INCREASE_WEAPON_DAMAGE, fBonus);
        bHasClipIncrease = apItem->HasModEffectActive(2);
        fAnimShotsPerSecond = Actor::GetAttackSpeedMult(pActor, apWeapon, 0, apItem);
        bHasSplitBeam = apItem->HasModEffectActive(12);
    }
    if (apWeapon) {
        fDamagePerHit = static_cast<double>(apWeapon->GetAttackDamage()) * fDamageWeaponMult.Float();
        fDamagePerHit = fDamagePerHit + fBonus;
        if (bHasSplitBeam) {
            fDamagePerHit = fDamagePerHit * 1.3;
        }
        TESAmmo* pAmmo = apAmmo ? apAmmo : apWeapon->GetCurrentAmmo(nullptr);
        BSSimpleList<TESAmmoEffect*>* pAmmoEffectList = pAmmo ? pAmmo->GetAmmoEffectList() : nullptr;
        fDamagePerHit = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, pAmmoEffectList, fDamagePerHit);
        if (apWeapon->GetProjectile(nullptr) && apWeapon->GetProjectile(nullptr)->GetExplosion()) {
            double dExplosionDamage = apWeapon->GetProjectile(nullptr)->GetExplosion()->Data.fDamage;
            fDamagePerHit = static_cast<double>(apWeapon->GetNumProjectiles(bHasSplitBeam, false, pActor)) *
                    dExplosionDamage +
                fDamagePerHit;
        }

        switch (apWeapon->GetWeaponType()) {
            case WEAPON_NONE:
            case WEAPON_HAND_TO_HAND:
            case WEAPON_ONE_HAND_MELEE:
            case WEAPON_TWO_HAND_MELEE:
            case WEAPON_ONE_HAND_THROWN: {
                break;
            }
            case WEAPON_ONE_HAND_GRENADE:
            case WEAPON_ONE_HAND_MINE: {
                if (apWeapon->GetProjectile(nullptr) && apWeapon->GetProjectile(nullptr)->GetExplosion()) {
                    BGSProjectile* pProjectile = apWeapon->GetProjectile(nullptr);
                    fDamagePerHit = pProjectile->GetExplosion()->Data.fDamage * fDamageWeaponMult.Float();
                }

                iRoundedCondition = 10;
                break;
            }
            default: {
                iClipSize = apWeapon->GetFormClipRounds(bHasClipIncrease);
                break;
            }
        }
        if (aiSkillOverride == -1) {
            double dSkillMult;
            if (apActor) {
                dSkillMult = apActor->GetClampedActorFloatValue(apWeapon->GetWeaponSkill()) / 100.0;
            }
            else {
                dSkillMult = 1.0;
            }
            fDamagePerHit = (fDamageSkillMult.Float() * dSkillMult + fDamageSkillBase.Float()) * fDamagePerHit;
        }
        else {
            float fSkillOverride = static_cast<double>(aiSkillOverride) / 100.0;
            fDamagePerHit = (fDamageSkillMult.Float() * fSkillOverride + fDamageSkillBase.Float()) * fDamagePerHit;
        }
        if (abApplyPerks) {
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::CALCULATE_WEAPON_DAMAGE,
                pActor,
                apWeapon,
                pActor,
                &fDamagePerHit
            );
        }
        fSecondsPerClip = fAnimShotsPerSecond;
        if (apWeapon->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
            apWeapon->GetWeaponType() != WEAPON_ONE_HAND_MINE &&
            apWeapon->GetWeaponType() != WEAPON_ONE_HAND_THROWN) {
            if (abMaxJamChance) {
                double dClipSize = static_cast<double>(iClipSize);
                double dJams = GetWeaponConditionJamChance(0) * dClipSize;
                double dAnimJamTime = apWeapon->GetAnimJamTime();
                fSecondsPerClip = dAnimJamTime * dJams + fSecondsPerClip;
            }
            else {
                double dClipSize = static_cast<double>(iClipSize);
                double dJams = GetWeaponConditionJamChance(iRoundedCondition) * dClipSize;
                double dAnimJamTime = apWeapon->GetAnimJamTime();
                fSecondsPerClip = dAnimJamTime * dJams + fSecondsPerClip;
            }
        }
        fSecondsPerClip = static_cast<double>(iClipSize) / fSecondsPerClip;
        if ((iClipSize <= iMinClipSizeToAddReloadDelay.Int() || abAlwaysAddReload) &&
            apWeapon->GetWeaponType() != WEAPON_ONE_HAND_MELEE &&
            apWeapon->GetWeaponType() != WEAPON_TWO_HAND_MELEE &&
            apWeapon->GetWeaponType() &&
            apWeapon->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
            apWeapon->GetWeaponType() != WEAPON_ONE_HAND_THROWN &&
            apWeapon->GetWeaponType() != WEAPON_ONE_HAND_MINE) {
            double dAnimReloadTime = apWeapon->GetAnimReloadTime();
            fSecondsPerClip = dAnimReloadTime + fSecondsPerClip;
        }

        if (pActor != pPlayer && apWeapon->IsRangedWeapon()) {
            if (!apWeapon->IsAutomaticWeapon()) {
                double dFireDelayMax = apWeapon->GetSemiAutomaticFireDelayMax();
                float fAvgDelayTime = (apWeapon->GetSemiAutomaticFireDelayMin() + dFireDelayMax) / 2.0;
                float fShotTime = 1.0 / fAnimShotsPerSecond;
                if (fShotTime < static_cast<double>(fAvgDelayTime)) {
                    float fAvgDelayTimeModified = fAvgDelayTime - fShotTime;
                    fSecondsPerClip = static_cast<double>(iClipSize) * fAvgDelayTimeModified + fSecondsPerClip;
                }
            }
            if (apWeapon->IsAutomaticWeapon() || apWeapon->GetBurstShot()) {
                double dBurstTime = fAutomaticWeaponBurstFireTime.Float() + fAutomaticWeaponBurstCooldownTime.Float();
                fSecondsPerClip = dBurstTime / fAutomaticWeaponBurstCooldownTime.Float() * fSecondsPerClip;
            }
        }
        float fCritChancePercent = 0.0;
        if (afCritChance != 0.0) {
            fCritChancePercent = afCritChance * 100.0;
        }
        float fBaseCritChance = apActor ? apActor->GetActorFloatValue(ActorValue::CRITICAL_CHANCE) : fCritChancePercent;
        fCritChance = apWeapon->GetCriticalChanceMult() * (fBaseCritChance * 1.0);
        BSClamp(fCritChance, 0.0f, 100.0f);
        fCritChance = fCritChance * 0.009999999776482582;
        if (apWeapon->IsAutomaticWeapon() && fAnimShotsPerSecond != 0.0) {
            fCritChance = fCritChance / fAnimShotsPerSecond;
        }
    }
    else {
        if (bCreature) {
            fDamagePerHit = static_cast<float>(pActor->GetAttackDamage());
        }
        else {
            float fUnarmedDamage = apActor ? apActor->GetActorFloatValue(ActorValue::UNARMED_DAMAGE) : 0.0f;
            double dScaledUnarmedDamage = fUnarmedDamageMult.Float() * fUnarmedDamage;
            fDamagePerHit = dScaledUnarmedDamage;
            double dUnarmedSkill = apActor ? apActor->GetClampedActorFloatValue(ActorValue::UNARMED) / 100.0 : 0.0;
            fDamagePerHit = fDamageSkillMult.Float() * dUnarmedSkill + fDamagePerHit;
        }
        fSecondsPerClip = bCreature ? fUnarmedCreatureDPSMult.Float() : fUnarmedNPCDPSMult.Float();
    }
    float fConditionMod = CombatFormulas::ApplyWeaponConditionScaling(afCondition);
    fDamagePerHit = fDamagePerHit * fConditionMod;
    float fCriticalDamage;
    if (apWeapon) {
        fCriticalDamage = static_cast<float>(apWeapon->GetCriticalDamage());
    }
    else {
        fCriticalDamage = 0.0f;
    }
    float fScaledCritDamage = fCriticalDamage;
    if (afCritDamageMult != 0.0 && fCriticalDamage != 0.0) {
        fScaledCritDamage = fCriticalDamage * afCritDamageMult;
    }
    float fDoubleMult = abDoubleDamage ? 2.0f : 1.0f;
    float fBonusMult = abBonusDamage ? 1.25f : 1.0f;
    float fDamagePerClip = static_cast<double>(iClipSize) *
        ((fCritChance * fScaledCritDamage + fDamagePerHit) * (fDoubleMult * fBonusMult));
    if (fSecondsPerClip != 0.0) {
        return fDamagePerClip / fSecondsPerClip;
    }
    return 0.0f;
}

// The products are kept in double with the double 2.0, the discriminant is compared with the double 0.0 (a NaN takes
// the pi/4 path) and each named intermediate is rounded to float.
float CombatUtilities::FindAngleOfProjectile(
    float afDeltaXY,
    float afDeltaZ,
    float afSpeed,
    float afGravity,
    bool abLargeRoot
) {
    float fSpeedSq = afSpeed * afSpeed;
    double fSpeedSqD = fSpeedSq;
    float fDiscriminant = static_cast<float>(
        fSpeedSqD * fSpeedSq -
        (static_cast<double>(afGravity) * afDeltaXY * afDeltaXY + afDeltaZ * 2.0 * fSpeedSq) * afGravity
    );
    if (fDiscriminant > 0.0) {
        float fRoot = NiSqrt(fDiscriminant);
        float fLow = static_cast<float>((fSpeedSqD - fRoot) / (static_cast<double>(afDeltaXY) * afGravity));
        float fHigh = static_cast<float>((fSpeedSqD + fRoot) / (static_cast<double>(afDeltaXY) * afGravity));
        float fTan;
        if (abLargeRoot) {
            fTan = BSMAX(fLow, fHigh);
        }
        else {
            fTan = BSMIN(fLow, fHigh);
        }
        float fAngle = static_cast<float>(NiATan(fTan));
        return fAngle;
    }
    float fQuarterPi = static_cast<float>(3.14159274f * 0.25);
    return fQuarterPi;
}

float CombatUtilities::GetDistanceBetweenFacingActors(
    TESObjectREFR* apAttacker,
    TESObjectREFR* apTarget,
    bool abMeleeCombat
) {
    if (!apTarget) {
        return 0.0f;
    }
    float fDistance = apAttacker->GetDistanceFromReference(apTarget, false, false);
    if (fDistance == 3.4028234663852886e+38) {
        return fDistance;
    }
    if (apAttacker->IsActor() && apTarget->IsActor() && apAttacker) {
        bool bAreSwimming = static_cast<Actor*>(apAttacker)->IsSwimming() &&
            static_cast<Actor*>(apTarget)->IsSwimming();
        NiPoint3 kActorPos = apAttacker->GetLocationOnReference();
        NiPoint3 kTargetPos = apTarget->GetLocationOnReference();
        float fHeightDiff = kActorPos.z - kTargetPos.z;
        double dHeightDelta = std::fabsf(fHeightDiff);
        if (bAreSwimming || (abMeleeCombat && fAICombatSlopeDifference.Float() <= dHeightDelta)) {
            kActorPos.z = 0.0;
            kTargetPos.z = 0.0;
            NiPoint3 kActorBoundMax = apAttacker->GetBoundMax();
            float fActorZMax = kActorBoundMax.z + kActorPos.z;
            NiPoint3 kActorBoundMin = apAttacker->GetBoundMin();
            float fActorZMin = kActorBoundMin.z + kActorPos.z;
            NiPoint3 kTargetBoundMax = apTarget->GetBoundMax();
            float fTargetZMax = kTargetBoundMax.z + kTargetPos.z;
            NiPoint3 kTargetBoundMin = apTarget->GetBoundMin();
            float fTargetZMin = kTargetBoundMin.z + kTargetPos.z;
            if ((fTargetZMax >= static_cast<double>(fActorZMax) && fTargetZMin <= static_cast<double>(fActorZMax)) ||
                (fTargetZMax >= static_cast<double>(fActorZMin) && fTargetZMin <= static_cast<double>(fActorZMin))) {
                fDistance = (kActorPos - kTargetPos).Length();
            }
        }
    }

    float fExtents = CombatUtilities::GetAttackerTargetExtentSum(apAttacker, apTarget);
    return fDistance - fExtents;
}

bool CombatUtilities::IsTargetWithinFiringArc(
    Actor* apAttacker,
    const NiPoint3& arTargetLocation,
    float afArcAngle,
    float afLookingAngle
) {
    NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(apAttacker, arTargetLocation);
    if (afArcAngle < 6.2831854820251465) {
        float fHeading = apAttacker->GetActualHeading();
        rot.z = ClampAngle(rot.z);
        float fDir = 0.0f;
        float fAngleDelta = GetAngleDelta(fHeading, rot.z, fDir);
        float fDelta = std::fabs(fAngleDelta);
        if (fDelta > afArcAngle * 0.5) {
            return false;
        }
    }
    if (!(afLookingAngle < 6.2831854820251465)) {
        return true;
    }
    float fLooking = apAttacker->GetLooking();
    rot.x = ClampAngle(rot.x);
    float fDir = 0.0f;
    float fAngleDelta = GetAngleDelta(fLooking, rot.x, fDir);
    float fDelta = std::fabs(fAngleDelta);
    return !(fDelta > afLookingAngle * 0.5);
}

float CombatUtilities::GetAttackerTargetExtentSum(TESObjectREFR* apAttacker, TESObjectREFR* apTarget) {
    float fHeight = 0.0;
    if (apAttacker->IsActor()) {
        fHeight = static_cast<Actor*>(apAttacker)->GetForwardLength();
    }
    else {
        NiPoint3 kBoundMax = apAttacker->GetBoundMax();
        fHeight = apAttacker->GetScale() * kBoundMax.y;
    }
    if (apTarget) {
        if (apTarget->IsActor()) {
            double dForwardLength = static_cast<Actor*>(apTarget)->GetForwardLength();
            return dForwardLength + fHeight;
        }

        NiPoint3 kBoundMax = apTarget->GetBoundMax();
        double dScale = apTarget->GetScale();
        return dScale * kBoundMax.y + fHeight;
    }
    return fHeight;
}

float CombatUtilities::GetWeaponReach(Actor* apAttacker, TESObjectWEAP* apWeapon) {
    float fScale = apAttacker->GetScale();
    if (apWeapon) {
        if (apWeapon->IsMeleeWeapon()) {
            float fReach = apWeapon->GetReach();
            return CombatFormulas::CalcWeaponReach(fReach) * fScale;
        }

        return static_cast<float>(fScale * 128.0);
    }

    return apAttacker->GetReach() * fScale;
}

// afAttackerReach is compared with the double -1.0 (a NaN keeps it) and halved with the double 0.5.
float CombatUtilities::ComputeFaceTargetDistance(Actor* apAttacker, TESObjectREFR* apTarget, float afAttackerReach) {
    float fExtentSum = GetAttackerTargetExtentSum(apAttacker, apTarget);
    if (afAttackerReach == -1.0) {
        afAttackerReach = GetWeaponReach(apAttacker, apAttacker->GetCurrentWeapon());
    }
    float fDistance = static_cast<float>(afAttackerReach * 0.5);
    fDistance = fDistance + fExtentSum;
    return BSMAX(fDistance, 0.0f);
}

NiPoint3 CombatUtilities::GetTargetFiringLocation(TESObjectREFR* apTarget, ACTOR_SEGMENT_INVIEW aeTargetSegmentInView) {
    NiPoint3 kActorPos = *apTarget->QLocationOnReference();
    kActorPos.z = CombatUtilities::GetTargetFiringZOffset(apTarget, aeTargetSegmentInView, false) + kActorPos.z;
    return kActorPos;
}

NiPoint3 CombatUtilities::GetTargetVelocity(TESObjectREFR* apTarget) {
    NiPoint3 velocity = NiPoint3::ZERO;
    if (apTarget->IsActor()) {
        bhkCharacterController* pCon = static_cast<MobileObject*>(apTarget)->MobileObject::GetCharController();
        if (pCon) {
            pCon->GetLinearVelocity(velocity);
        }
    }
    return velocity;
}

// The velocity is fetched before the firing location.
NiPoint3 CombatUtilities::GetAngleToProjectedTarget(
    NiPoint3 aStartLocation,
    TESObjectREFR* apTarget,
    float afSpeed,
    float afGravity,
    ACTOR_SEGMENT_INVIEW aeTargetSegmentInView
) {
    if (!apTarget) {
        return NiPoint3::ZERO;
    }
    NiPoint3 kVelocity = GetTargetVelocity(apTarget);
    NiPoint3 kTargetLocation = GetTargetFiringLocation(apTarget, aeTargetSegmentInView);
    NiPoint3 kProjectedPoint = GetProjectedPointForTarget(
        aStartLocation,
        kTargetLocation,
        kVelocity,
        afSpeed,
        afGravity,
        false
    );
    return GetAngleToProjectedPoint(aStartLocation, kProjectedPoint);
}

NiPoint3 CombatUtilities::GetAngleToProjectedTarget(
    TESObjectREFR* apAttacker,
    TESObjectREFR* apTarget,
    float afSpeed,
    float afGravity,
    ACTOR_SEGMENT_INVIEW aeTargetSegmentInView
) {
    NiPoint3 shooterLocation = CombatUtilities::GetAttackerFiringLocation(apAttacker);
    NiPoint3 angle = CombatUtilities::GetAngleToProjectedTarget(
        shooterLocation,
        apTarget,
        afSpeed,
        afGravity,
        aeTargetSegmentInView
    );
    angle = angle - apAttacker->GetAngleOnReference();
    angle.y = 0.0f;
    if (angle.x <= -3.1415927410125732) {
        angle.x = angle.x + 6.2831854820251465;
    }
    else if (angle.x > 3.1415927410125732) {
        angle.x = angle.x - 6.2831854820251465;
    }
    if (angle.z <= -3.1415927410125732) {
        angle.z = angle.z + 6.2831854820251465;
    }
    else if (angle.z > 3.1415927410125732) {
        angle.z = angle.z - 6.2831854820251465;
    }
    return angle;
}

NiPoint3 CombatUtilities::GetProjectedPointForTarget(
    NiPoint3 aStartLocation,
    NiPoint3 aTargetLocation,
    NiPoint3 aTargetVelocity,
    float afProjectileSpeed,
    float afProjectileGravity,
    bool abHighArc
) {
    int32_t iAimingNumIterations = ::iAimingNumIterations.Int();
    if (aTargetVelocity.x == 0.0 && aTargetVelocity.y == 0.0 && aTargetVelocity.z == 0.0) {
        iAimingNumIterations = 1;
    }
    float fScalar = 0.0;
    NiPoint3 kProjectedPoint = aTargetLocation;
    if (afProjectileGravity == 0.0) {
        float fDistance = 0.0f;
        for (int32_t i = 0; i < iAimingNumIterations; ++i) {
            fDistance = (kProjectedPoint - aStartLocation).Length();
            if (fDistance < 1.0 || fDistance > 8192.0) {
                break;
            }
            fScalar = fDistance / afProjectileSpeed;
            kProjectedPoint = aTargetLocation + aTargetVelocity * fScalar;
        }
    }
    else {
        float fDeltaXY = 0.0f;
        float fDeltaZ = 0.0f;
        float fAngle = 0.0f;
        for (int32_t j = 0; j < iAimingNumIterations; ++j) {
            NiPoint3 kDelta = kProjectedPoint - aStartLocation;
            fDeltaZ = kDelta.z;
            kDelta.z = 0.0f;
            fDeltaXY = kDelta.Length();
            if (fDeltaZ > 8192.0 || fDeltaXY < 1.0 || fDeltaXY > 8192.0) {
                break;
            }
            fAngle = CombatUtilities::FindAngleOfProjectile(
                fDeltaXY,
                fDeltaZ,
                afProjectileSpeed,
                afProjectileGravity,
                abHighArc
            );
            fScalar = CombatUtilities::ComputeProjectileTime(fDeltaXY, fAngle, afProjectileSpeed);
            kProjectedPoint = aTargetLocation + aTargetVelocity * fScalar;
        }
        NiPoint3 kDelta = kProjectedPoint - aStartLocation;
        kDelta.z = 0.0f;
        fDeltaXY = kDelta.Length();
        double dTan = std::tan(fAngle);
        kProjectedPoint.z = dTan * fDeltaXY + aStartLocation.z;
    }
    return kProjectedPoint;
}

NiPoint3 CombatUtilities::GetProjectedPointForTarget(
    TESObjectREFR* apAttacker,
    TESObjectREFR* apTarget,
    NiPoint3* apReleasePoint,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    bool abHighArc
) {
    ACTOR_SEGMENT_INVIEW eTargetSegmentInView = ACTOR_SEGMENT_INVIEW_MIDDLE;
    if (apAttacker->IsActor()) {
        CombatController* pCombatController = static_cast<Actor*>(apAttacker)->GetCombatController();
        if (pCombatController) {
            eTargetSegmentInView = pCombatController->GetTargetSegmentInView();
        }
    }
    if (!abHighArc || apProjectile->GetGravity() == 0.0) {
        BGSExplosion* pExplosion = apProjectile->GetExplosion();
        bool bFastProjectile = false;
        if (apProjectile->GetHitScan()) {
            bFastProjectile = true;
        }
        else {
            double dSpeed = apProjectile->GetSpeed();
            if (fCombatSplashDamageMaxSpeed.Float() < dSpeed) {
                bFastProjectile = true;
            }
        }
        if (pExplosion) {
            double dExplosionRadius = pExplosion->GetRadiusBSUnits();
            if (fCombatSplashDamageMinRadius.Float() <= dExplosionRadius && !bFastProjectile) {
                double dExplosionDamage = pExplosion->Data.fDamage;
                if (fCombatSplashDamageMinDamage.Float() < dExplosionDamage) {
                    eTargetSegmentInView = ACTOR_SEGMENT_INVIEW_GROUND;
                }
            }
        }
    }
    else {
        eTargetSegmentInView = ACTOR_SEGMENT_INVIEW_TOP;
    }
    NiPoint3 targetLoc = CombatUtilities::GetTargetFiringLocation(apTarget, eTargetSegmentInView);
    NiPoint3 targetVelocity = CombatUtilities::GetTargetVelocity(apTarget);
    return CombatUtilities::GetProjectedPointForTarget(
        static_cast<Actor*>(apAttacker),
        targetLoc,
        targetVelocity,
        apReleasePoint,
        apProjectile,
        afProjectileSpeed,
        abHighArc
    );
}

NiPoint3 CombatUtilities::GetAngleToProjectedPoint(NiPoint3 aStartLocation, NiPoint3 aProjectedPoint) {
    NiPoint3 kDirection = aProjectedPoint - aStartLocation;
    float fDeltaZ = kDirection.z;
    float fLength = kDirection.UnitizeGetLength();
    float fSinPitch = fDeltaZ / fLength;
    float fPitch = NiASin(fSinPitch);
    float fRotX = -fPitch;
    if (fRotX <= -3.1415927410125732) {
        fRotX = fRotX + 6.2831854820251465;
    }
    else if (fRotX > 3.1415927410125732) {
        fRotX = fRotX - 6.2831854820251465;
    }
    float fRotZ = GetUnclampedZAngleFromVector(kDirection);
    if (fRotZ <= -3.1415927410125732) {
        fRotZ = fRotZ + 6.2831854820251465;
    }
    else if (fRotZ > 3.1415927410125732) {
        fRotZ = fRotZ - 6.2831854820251465;
    }
    return NiPoint3(fRotX, static_cast<float>(0.0), fRotZ);
}

NiPoint3 CombatUtilities::GetAngleToProjectedPoint(TESObjectREFR* apAttacker, NiPoint3 aProjectedPoint) {
    NiPoint3 attackerFiringLocation = CombatUtilities::GetAttackerFiringLocation(apAttacker);
    NiPoint3 kSimpleLocation = CombatUtilities::GetAttackerFiringLocationSimple(apAttacker);
    return CombatUtilities::GetAngleToProjectedPoint(attackerFiringLocation, kSimpleLocation, 0.0f, aProjectedPoint);
}

// A ray from aStartLocation: straight to aTargetedPoint without gravity, else along the projectile's ballistic arc in
// iBallisticProjectilePathPickSegments straight segments, up to the first segment that hits. The hit 3D's reference,
// the hit point and the hit fraction of the whole path come back through the pointers. Intermediates are kept as double
// and stored values as float. The segment counts are unsigned.
bool CombatUtilities::CalculateProjectileLOS(
    Actor* apActor,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    NiPoint3 aStartLocation,
    NiPoint3 aTargetedPoint,
    NiPoint3* apHitLocation,
    TESObjectREFR** apRefHit,
    float* apDistanceFraction
) {
    CombatAI_LOS.Start();
    CFilter kFilter(0);
    kFilter.SetLayer(L_PROJECTILE);
    // GetCollisionFilter writes the actor's filter word into kActorFilter and returns it; the group is then read
    // through CFilter::GetGroup on that same local.
    CFilter kActorFilter;
    apActor->GetCollisionFilter(&kActorFilter.iFilter);
    kFilter.SetGroup(kActorFilter.GetGroup());
    double dGravity = apProjectile ? apProjectile->GetGravity() * CombatUtilities::fWorldGravity : 0.0;
    float fGravity = static_cast<float>(dGravity);
    NiAVObject* pHitObj = nullptr;
    // Raw stack storage the collector is built in, because a local of the class would run a C++ constructor and
    // destructor.
    alignas(16) uint8_t acCollector[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = reinterpret_cast<hkpAllRayHitCollector*>(acCollector);
    ::new (pCollector) hkpAllRayHitCollector;
    alignas(16) bhkPickData kPick;
    kPick.SetFilter(kFilter);
    kPick.SetAllCollector(pCollector);
    NiPoint3 kFrom = aStartLocation;
    NiPoint3 kTo = aTargetedPoint;
    float fHitFraction = 0.0f;
    uint32_t uiSegment = 1;
    uint32_t uiSegments = 1;
    if (fGravity == 0.0) {
        kPick.NiSetFrom(kFrom);
        kPick.NiSetTo(kTo);
        apActor->GetParentCell()->Pick(kPick);
        pHitObj = Projectile::ProcessTargetPick(
            &kPick,
            nullptr,
            apProjectile,
            &fHitFraction,
            false,
            std::numeric_limits<float>::max()
        );
        if (bDebugCombatProjectileLOS.Bool() &&
            (!Main::pDebugTextReference ||
             Main::pDebugTextReference == static_cast<TESObjectREFR*>(pPlayer) ||
             Main::pDebugTextReference == apActor)) {
            AddLOSDebugGeomtry(kFrom, kTo, pHitObj, fHitFraction, nullptr, nullptr);
        }
    }
    else {
        alignas(16) NiPoint3 kDelta = aTargetedPoint - aStartLocation;
        float fSinPitch = static_cast<float>(static_cast<double>(kDelta.z) / kDelta.Length());
        float fPitch = NiASin(fSinPitch);
        kDelta.z = 0.0f;
        float fXYLength = kDelta.Length();
        float fHeading = GetUnclampedZAngleFromVector(kDelta);
        float fSpeed = afProjectileSpeed;
        if (fSpeed == 0.0) {
            fSpeed = apProjectile->GetSpeed();
        }
        uiSegments = iBallisticProjectilePathPickSegments.UInt();
        float fStep = static_cast<float>(fXYLength / static_cast<double>(uiSegments));
        float fStepX = static_cast<float>(static_cast<double>(std::sin(fHeading)) * fStep);
        float fStepY = static_cast<float>(static_cast<double>(std::cos(fHeading)) * fStep);
        float fTan = std::tan(fPitch);
        float fCos = std::cos(fPitch);
        for (uint32_t i = 0; i < uiSegments; ++i) {
            uiSegment = i + 1;
            kTo = aStartLocation;
            float fDist = static_cast<float>(static_cast<double>(uiSegment) * fStep);
            float fHeight = static_cast<float>(
                static_cast<double>(fDist) * fTan -
                static_cast<double>(fGravity) * fDist * fDist /
                    (static_cast<double>(fSpeed) * fSpeed * fCos * fCos * 2.0)
            );
            kTo.x = static_cast<float>(static_cast<double>(uiSegment) * fStepX + kTo.x);
            kTo.y = static_cast<float>(static_cast<double>(uiSegment) * fStepY + kTo.y);
            kTo.z = kTo.z + fHeight;
            kPick.NiSetFrom(kFrom);
            kPick.NiSetTo(kTo);
            apActor->GetParentCell()->Pick(kPick);
            pHitObj = Projectile::ProcessTargetPick(
                &kPick,
                nullptr,
                apProjectile,
                &fHitFraction,
                false,
                std::numeric_limits<float>::max()
            );
            if (bDebugCombatProjectileLOS.Bool() &&
                (!Main::pDebugTextReference ||
                 Main::pDebugTextReference == static_cast<TESObjectREFR*>(pPlayer) ||
                 Main::pDebugTextReference == apActor)) {
                AddLOSDebugGeomtry(kFrom, kTo, pHitObj, fHitFraction, nullptr, nullptr);
            }
            if (pHitObj) {
                break;
            }
            kFrom = kTo;
        }
    }
    if (pHitObj) {
        if (apRefHit) {
            *apRefHit = TESObjectREFR::FindReferenceFor3D(pHitObj);
        }
        if (apHitLocation) {
            NiPoint3 kDiff = kTo - kFrom;
            alignas(16) NiPoint3 kScaled = kDiff * fHitFraction;
            *apHitLocation = kScaled + kFrom;
        }
        if (apDistanceFraction) {
            if (uiSegments == 1) {
                *apDistanceFraction = fHitFraction;
            }
            else {
                *apDistanceFraction = static_cast<float>(
                    fHitFraction / static_cast<double>(uiSegments) +
                    static_cast<double>(uiSegment - 1) / static_cast<double>(uiSegments)
                );
            }
        }
        CombatAI_LOS.Stop();
        pCollector->~hkpAllRayHitCollector();
        return true;
    }
    CombatAI_LOS.Stop();
    pCollector->~hkpAllRayHitCollector();
    return false;
}

// A projectile with no gravity (the product is rounded to a float, and an unordered compare takes the gravity
// branch) gets one throw of its own speed; otherwise both arcs are tried, the second flipping the high-arc flag, and
// the flag is restored when neither passes. The first throw whose line of sight is clear, whose hit reference is the
// target or whose hit fraction reaches the threshold is the one used; the zero-gravity branch accepts an equal
// fraction, the loop one does not.
bool CombatUtilities::CheckProjectileArc(
    Actor* apAttacker,
    Actor* apTarget,
    BGSProjectile* apProjectile,
    NiPoint3 aStartLocation,
    NiPoint3 aTargetLocation,
    NiPoint3 aTargetVelocity,
    bool& arbHighArc,
    NiPoint3& arTargetedPoint,
    float afMinHitFraction
) {
    if (!apAttacker || !apTarget) {
        return false;
    }
    if (afMinHitFraction == -1.0) {
        afMinHitFraction = fGrenadeThrowHitFractionThreshold.Float();
    }
    float fDistanceFraction = 0.0f;
    TESObjectREFR* pRefHit = nullptr;
    float fGravity = apProjectile->GetGravity() * CombatUtilities::fWorldGravity;
    if (fGravity == 0.0) {
        uint8_t bHighArc = arbHighArc;
        float fSpeed = apProjectile->GetSpeed();
        NiPoint3 kProjected = GetProjectedPointForTarget(
            aStartLocation,
            aTargetLocation,
            aTargetVelocity,
            fSpeed,
            fGravity,
            bHighArc
        );
        bool bHit = CalculateProjectileLOS(
            apAttacker,
            apProjectile,
            0.0f,
            aStartLocation,
            kProjected,
            nullptr,
            &pRefHit,
            &fDistanceFraction
        );
        // An unordered compare passes as well.
        if (!bHit || pRefHit == apTarget || !(afMinHitFraction > fDistanceFraction)) {
            arTargetedPoint = kProjected;
            return true;
        }
        return false;
    }
    uint8_t bOriginalHighArc = arbHighArc;
    for (uint32_t i = 0; i < 2; ++i) {
        if (i == 1) {
            arbHighArc = !arbHighArc;
        }
        float fSpeedMultiplier = arbHighArc ? fGrenadeHighArcSpeedMult.Float() : 1.0f;
        float fSpeed = apProjectile->GetSpeed() * fSpeedMultiplier;
        NiPoint3 kProjected = GetProjectedPointForTarget(
            aStartLocation,
            aTargetLocation,
            aTargetVelocity,
            fSpeed,
            fGravity,
            arbHighArc
        );
        bool bHit = CalculateProjectileLOS(
            apAttacker,
            apProjectile,
            fSpeed,
            aStartLocation,
            kProjected,
            nullptr,
            &pRefHit,
            &fDistanceFraction
        );
        if (!bHit || pRefHit == apTarget || afMinHitFraction < fDistanceFraction) {
            arTargetedPoint = kProjected;
            return true;
        }
    }
    arbHighArc = bOriginalHighArc;
    return false;
}

// The point the attacker fires from: the player's weapon bone (third or first person), else the actor's weapon bone,
// else its fire node, else its reference location raised by the firing height.
NiPoint3 CombatUtilities::GetAttackerFiringLocation(TESObjectREFR* apAttacker) {
    NiPoint3 kFiringLocation;
    if (apAttacker->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apAttacker);
        bool bIsPlayer = apAttacker == pPlayer;
        CombatController* pCombatController = pActor->GetCombatController();
        TESObjectWEAP* pWeapon = pCombatController ? pCombatController->GetCurrentWeapon() : pActor->GetCurrentWeapon();
        if (!pWeapon || pWeapon->IsRangedWeapon()) {
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            if (apAttacker && pProcess) {
                if (!pWeapon ||
                    pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                    pWeapon->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                    pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN ||
                    pWeapon->GetWeaponType() == WEAPON_ONE_HAND_LUNCHBOX_MINE) {
                    BipedAnim* pBiped;
                    if (bIsPlayer) {
                        pBiped = pPlayer->GetBiped(pPlayer->Is3rdPerson());
                    }
                    else {
                        pBiped = pActor->GetCurrentBiped();
                    }
                    NiAVObject* pWeaponBone = pProcess->GetWeaponBone(pBiped);
                    if (pWeaponBone) {
                        kFiringLocation = pWeaponBone->GetWorldTranslate();
                        return kFiringLocation;
                    }
                }
                else {
                    NiAVObject* pFireNode = pActor->GetFireNode();
                    if (pFireNode) {
                        kFiringLocation = pFireNode->GetWorldTranslate();
                        return kFiringLocation;
                    }
                }
            }
        }
    }
    NiPoint3 kLocation = apAttacker->GetLocationOnReference();
    kLocation.z = CombatUtilities::GetAttackerFiringZOffset(apAttacker, false) + kLocation.z;
    kFiringLocation = kLocation;
    return kFiringLocation;
}

// The range of apProjectile: its set range for hit-scan or weightless ones, else the flat-ground reach at
// afProjectileSpeed (its own speed when 0). Each double getter's result is rounded to a float local.
float CombatUtilities::GetProjectileMaximumRange(BGSProjectile* apProjectile, float afProjectileSpeed) {
    float fRange = apProjectile->GetRange();
    if (apProjectile->GetHitScan()) {
        return fRange;
    }
    float fGravity = apProjectile->GetGravity();
    if (fGravity <= 0.0) {
        return fRange;
    }
    fGravity = fGravity * CombatUtilities::fWorldGravity;
    float fSpeed = afProjectileSpeed;
    if (afProjectileSpeed == 0.0) {
        fSpeed = apProjectile->GetSpeed();
    }
    // Divides at double precision and rounds to float only on the store.
    float fMaxRange = static_cast<float>(static_cast<double>(fSpeed) * fSpeed / fGravity);
    return fMaxRange;
}

// The explosion radius is fetched before arfMinRange is read, and 256 is taken off the maximum as a double, with the
// result kept as a float.
void CombatUtilities::ComputeOptimalWeaponRanges(
    CombatController* apCombatController,
    TESObjectWEAP* apEquippedWeapon,
    float& arfMinRange,
    float& arfMaxRange,
    float& arfAbsoluteMaxRange
) {
    arfMinRange = 512.0f;
    arfMaxRange = 1024.0f;
    TESCombatStyle* pCombatStyle = apCombatController->GetCurrentCombatStyle();
    if (!apEquippedWeapon) {
        arfMinRange = pCombatStyle->GetWeaponMinRangeMult() * arfMinRange;
        arfMaxRange = pCombatStyle->GetWeaponMaxRangeMult() * arfMaxRange;
        arfAbsoluteMaxRange = arfMaxRange * fCombatAbsoluteMaxRangeMult.Float();
        return;
    }
    arfMinRange = apEquippedWeapon->GetMinRange();
    arfMaxRange = apEquippedWeapon->GetMaxRange();
    if (!apEquippedWeapon->GetIsRangeFixed()) {
        arfMinRange = pCombatStyle->GetWeaponMinRangeMult() * arfMinRange;
        arfMaxRange = pCombatStyle->GetWeaponMaxRangeMult() * arfMaxRange;
    }
    arfAbsoluteMaxRange = arfMaxRange * fCombatAbsoluteMaxRangeMult.Float();
    BGSProjectile* pProjectile = apEquippedWeapon->GetProjectile(apCombatController->GetAttacker());
    if (!pProjectile) {
        return;
    }
    float fRange = GetProjectileMaximumRange(pProjectile, 0.0f);
    float fOptimalRange = fRange * fCombatProjectileMaxRangeOptimalMult.Float();
    arfMaxRange = BSMIN(arfMaxRange, fOptimalRange);
    arfAbsoluteMaxRange = BSMIN(arfAbsoluteMaxRange, fRange);
    BGSExplosion* pExplosion = pProjectile->GetExplosion();
    if (pExplosion) {
        float fRadius = pExplosion->GetRadiusBSUnits();
        arfMinRange = BSMAX(arfMinRange, fRadius);
    }
    float fMinCap = arfMaxRange - 256.0;
    arfMinRange = BSMIN(arfMinRange, fMinCap);
    arfMinRange = BSMAX(arfMinRange, 0.0f);
}

// The setting and TES::pInstance are reloaded for each debug object.
void CombatUtilities::AddLOSDebugGeomtry(
    NiPoint3& arStartLocation,
    NiPoint3& arEndLocation,
    NiAVObject* apHitObj,
    float afHitFraction,
    NiColorA* apLOSColor,
    NiColorA* apHitColor
) {
    NiColorA kHitColor = apHitColor ? *apHitColor : NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
    NiColorA kLOSColor = apLOSColor ? *apLOSColor : NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
    NiPoint3 kHitPoint = apHitObj ? (arEndLocation - arStartLocation) * afHitFraction + arStartLocation : arEndLocation;
    NiLines* pLine = MakeLine(arStartLocation, kLOSColor, kHitPoint, kLOSColor, true);
    TES::pInstance->AddTempDebugObject(pLine, fDebugCombatProjectileLOSTime.Float());
    if (apHitObj) {
        NiTriShape* pPoint = MakeTriPoint(5.0f, kHitColor, true);
        pPoint->UpdateProperties();
        pPoint->SetTranslate(kHitPoint);
        TES::pInstance->AddTempDebugObject(pPoint, fDebugCombatProjectileLOSTime.Float());
        NiLines* pHitLine = MakeLine(kHitPoint, kHitColor, arEndLocation, kHitColor, true);
        TES::pInstance->AddTempDebugObject(pHitLine, fDebugCombatProjectileLOSTime.Float());
    }
}

float CombatUtilities::GetCostForWeaponType(CombatController* apController, COMBAT_WEAPON_TYPE aeType) {
    CombatState* pCombatState = apController->pCombatState;
    float fAttackDPS = pCombatState->GetMaxDPSForWeaponType(aeType);
    if (fAttackDPS == 0.0) {
        return std::numeric_limits<float>::max();
    }

    float fMaxAttackDPS = pCombatState->fMaxModifiedCombatWeaponDPS;
    return fMaxAttackDPS - fAttackDPS;
}

float CombatUtilities::GetTargetFiringZOffset(
    TESObjectREFR* apTarget,
    ACTOR_SEGMENT_INVIEW aeTargetSegmentInView,
    bool abIgnoreCrouch
) {
    double dZOffset;
    if (apTarget->IsActor()) {
        Actor* pTargetActor = static_cast<Actor*>(apTarget);
        bool bIsSneaking = static_cast<Actor*>(apTarget)->Actor::IsSneaking() && !abIgnoreCrouch;
        float fHeight = bIsSneaking ? pTargetActor->GetSneakHeight(false) : pTargetActor->GetHeight();
        if (pTargetActor->IsKnockedOut() || pTargetActor->IsSleeping() || pTargetActor->IsDead(false)) {
            return fHeight * 0.10000000149011612;
        }

        switch (aeTargetSegmentInView) {
            case ACTOR_SEGMENT_INVIEW_BOTTOM: {
                float fOffset = fHeight * 0.25;
                dZOffset = fOffset;
                break;
            }
            case ACTOR_SEGMENT_INVIEW_MIDDLE:
            case ACTOR_SEGMENT_INVIEW_NONE: {
                float fOffset = fHeight * 0.5;
                dZOffset = fOffset;
                break;
            }
            case ACTOR_SEGMENT_INVIEW_TOP: {
                double dScaled = bIsSneaking ? fHeight * 0.8999999761581421 : fHeight * 0.75;
                float fOffset = dScaled;
                dZOffset = fOffset;
                break;
            }
            default: {
                dZOffset = 0.0;
                break;
            }
        }
    }
    else {
        NiPoint3 kBoundMax = apTarget->GetBoundMax();
        double dBoundHeight = kBoundMax.z - apTarget->GetBoundMin().z;
        return apTarget->GetScale() * dBoundHeight * 0.5;
    }
    return dZOffset;
}

void CombatUtilities::CalculateCoverPositions(
    CombatController* apController,
    PathingCoverLocation* apCoverLocation,
    CoverReservationInfo* apCoverData,
    NiPoint3* apCoverPosition,
    NiPoint3* apFirePosition
) {
    Actor* pAttacker = apController->GetAttacker();
    float fActorRadius = pAttacker->GetRadius();
    CombatUtilities::CalculateCoverPositions_1(
        fActorRadius,
        apCoverLocation,
        apCoverData,
        apCoverPosition,
        apFirePosition
    );
}

bool CombatUtilities::CalculateLOS(
    Actor* apAttacker,
    TESObjectREFR* apTarget,
    const NiPoint3& arAttackerLocation,
    const NiPoint3& arTargetLocation,
    bool abIgnoreCrouch,
    int32_t aiCollisionLayer
) {
    CFilter kColFilter;
    CombatAI_LOS.Start();
    COL_LAYER eCollilsionLayer = L_LOS;
    if (aiCollisionLayer != -1) {
        eCollilsionLayer = static_cast<COL_LAYER>(aiCollisionLayer);
    }
    NiPoint3 attackerLocation = arAttackerLocation;
    attackerLocation.z = CombatUtilities::GetAttackerFiringZOffset(apAttacker, true) + attackerLocation.z;
    alignas(16) NiPoint3 targetLocation = arTargetLocation;
    if (apTarget) {
        targetLocation.z = CombatUtilities::GetTargetFiringZOffset(apTarget, ACTOR_SEGMENT_INVIEW_TOP, true) +
            targetLocation.z;
    }
    bhkPickData pickData;
    apAttacker->GetCollisionFilter(&kColFilter.iFilter);
    if (apTarget && apTarget->IsMobileObject()) {
        MobileObject* pMobileTarget = static_cast<MobileObject*>(apTarget);
        uint32_t uiTargetFilter;
        kColFilter.iFilter = *pMobileTarget->MobileObject::GetCollisionFilter(&uiTargetFilter);
    }

    SpecificItemCollector collector(eCollilsionLayer, kColFilter);
    pickData.SetClosestCollector(&collector);
    uint32_t uiAttackerFilter;
    kColFilter.iFilter = *apAttacker->GetCollisionFilter(&uiAttackerFilter);
    kColFilter.SetLayer(eCollilsionLayer);
    pickData.SetFilter(kColFilter);
    pickData.NiSetFrom(attackerLocation);
    pickData.NiSetTo(targetLocation);
    NiAVObject* pObjHit = apAttacker->GetParentCell()->Pick(pickData);
    if (!pObjHit) {
        CombatAI_LOS.Stop();
        return true;
    }
    TESObjectREFR* pHitRef = TESObjectREFR::FindReferenceFor3D(pObjHit);
    if (apTarget && pHitRef == apTarget) {
        CombatAI_LOS.Stop();
        return true;
    }

    CombatAI_LOS.Stop();
    return false;
}

float CombatUtilities::GetWorldGravity() {
    return CombatUtilities::fWorldGravity;
}

float CombatUtilities::ComputeProjectileTime(float afDeltaXY, float afAngle, float afSpeed) {
    return afDeltaXY / (NiCos(afAngle) * afSpeed);
}

bool CombatUtilities::CalculateAttackerCombatPositions(Actor* apTarget, Actor* apAttacker) {
    bool bResult = false;
    if (!apTarget) {
        return bResult;
    }
    CombatTargetData* pAttackerArray = pCombatManager->pTargetMap->GetTargetAttackerArray(apTarget);
    if (pAttackerArray) {
        MeleeAttackerData attackerData[64];
        NiPoint3 targetLocation = *apTarget->QLocationOnReference();
        ourMemset(attackerData, 0, sizeof(attackerData));
        uint32_t uiMeleeAttackers = 0;
        uint32_t uiMeleeAttackersInMelee = 0;
        uint32_t uiReservedSlots = 0;
        uint32_t uiNumAttackers = pAttackerArray->Attackers.iSize;
        uiNumAttackers = BSMIN(uiNumAttackers, 64u);
        for (uint32_t i = 0; i < uiNumAttackers; ++i) {
            Actor* pAttacker = pAttackerArray->Attackers[i];
            CombatController* pCombatController = pAttacker ? pAttacker->GetCombatController() : nullptr;
            if (pCombatController &&
                (pCombatController->CombatPlanContainsAction(7) || pCombatController->CombatPlanContainsAction(8))) {
                attackerData[uiMeleeAttackers].pCombatController = pCombatController;
                attackerData[uiMeleeAttackers].Location = *pAttacker->QLocationOnReference();
                double dZAngle = GetZAngleFromVector(attackerData[uiMeleeAttackers].Location - targetLocation);
                attackerData[uiMeleeAttackers].fAngle = dZAngle * 57.2957763671875;
                attackerData[uiMeleeAttackers].cSlot = GetSlotFromAngle(attackerData[uiMeleeAttackers].fAngle);
                if (pCombatController->pMovementProcedure) {
                    CombatProcedure* pCombatProcedure = pCombatController->pMovementProcedure;
                    if (pCombatProcedure->GetType() == CombatProcedure::ATTACK_MELEE) {
                        attackerData[uiMeleeAttackers].bInMelee = true;
                        ++uiMeleeAttackersInMelee;
                    }
                }
                ++uiMeleeAttackers;
                if (pAttacker == apAttacker) {
                    bResult = true;
                }
            }
        }
        if (uiMeleeAttackers) {
            if (uiMeleeAttackersInMelee) {
                for (uint32_t i = 0; i < uiMeleeAttackers; ++i) {
                    if (attackerData[i].bInMelee) {
                        char cFoundSlot = FindSlot(uiReservedSlots, attackerData[i].cSlot);
                        if (cFoundSlot != -1) {
                            uiReservedSlots = ReserveSlot(uiReservedSlots, cFoundSlot);
                            float fAngle = attackerData[i].fAngle;
                            if (cFoundSlot != attackerData[i].cSlot) {
                                fAngle = GetAngleFromSlot(cFoundSlot);
                            }
                            attackerData[i].pCombatController->SetCombatPosition(fAngle);
                        }
                    }
                }
            }
            for (uint32_t i = 0; i < uiMeleeAttackers; ++i) {
                if (!attackerData[i].bInMelee) {
                    char cFoundSlot = FindSlot(uiReservedSlots, attackerData[i].cSlot);
                    if (cFoundSlot != -1) {
                        uiReservedSlots = ReserveSlot(uiReservedSlots, cFoundSlot);
                        float fAngle = attackerData[i].fAngle;
                        if (cFoundSlot != attackerData[i].cSlot) {
                            fAngle = GetAngleFromSlot(cFoundSlot);
                        }
                        attackerData[i].pCombatController->SetCombatPosition(fAngle);
                    }
                }
            }
        }
    }
    return bResult;
}

float CombatUtilities::GetAngleForMeleeAttack(const ANIM_GROUP_ENUM eAttack) {
    double dAngle;
    if (eAttack < ANIM_GROUP_ATTACK_NORMAL_POWER || eAttack > ANIM_GROUP_POWER_ATTACK_STOP) {
        return fCombatHitConeAngle.Float();
    }

    switch (eAttack) {
        case ANIM_GROUP_ATTACK_NORMAL_POWER:
        case ANIM_GROUP_ATTACK_FORWARD_POWER: {
            dAngle = fCombatOverheadHitConeAngle.Float();
            break;
        }
        case ANIM_GROUP_ATTACK_BACK_POWER:
        case ANIM_GROUP_ATTACK_LEFT_POWER:
        case ANIM_GROUP_ATTACK_RIGHT_POWER: {
            dAngle = fCombatSweepHitConeAngle.Float();
            break;
        }
        case ANIM_GROUP_ATTACK_CUSTOM_1_POWER: {
            dAngle = fCombatUppercutHitConeAngle.Float();
            break;
        }
        default: {
            return fCombatHitConeAngle.Float();
        }
    }

    return dAngle;
}

char CombatUtilities::CalculateGrenadeThrow_(
    Actor* apActor,
    Actor* apTarget,
    BGSProjectile* apProjectile,
    NiPoint3* apReleasePoint,
    bool* isHighArc,
    NiPoint3* targetPointOut,
    float grenadeThrowHitFractionThreshold
) {
    if (!apActor || !apTarget) {
        return 0;
    }
    NiPoint3 kStartLocation;
    if (apReleasePoint) {
        kStartLocation = *apReleasePoint;
    }
    else {
        kStartLocation = CombatUtilities::GetAttackerFiringLocation(apActor);
    }
    NiPoint3 kTargetLocation = CombatUtilities::GetTargetFiringLocation(apTarget, ACTOR_SEGMENT_INVIEW_TOP);
    NiPoint3 kTargetVelocity = CombatUtilities::GetTargetVelocity(apTarget);
    return CombatUtilities::CheckProjectileArc(
        apActor,
        apTarget,
        apProjectile,
        kStartLocation,
        kTargetLocation,
        kTargetVelocity,
        *isHighArc,
        *targetPointOut,
        grenadeThrowHitFractionThreshold
    );
}

float CombatUtilities::FindAngleOfProjectile_0(
    Projectile* apProjectile,
    NiPoint3 akToTarget,
    float afSpeed,
    bool abHighArc
) {
    float fVDistance = akToTarget.z;
    akToTarget.z = 0.0;
    float fHDistance = akToTarget.Length();
    if (afSpeed == 0.0) {
        afSpeed = apProjectile->GetSpeed();
    }
    float fGravity = apProjectile->GetProjectileBase()->GetGravity() * CombatUtilities::fWorldGravity;
    return CombatUtilities::FindAngleOfProjectile(fHDistance, fVDistance, afSpeed, fGravity, abHighArc);
}

// The projectile's speed (afSpeed, or GetSpeed() when it is 0) is kept in a local for ComputeProjectileTime only; the
// straight-line division and FindAngleOfProjectile use afSpeed itself, scaled by the high-arc percentage when it is 0.
double CombatUtilities::ComputeProjectileTime_(
    BGSProjectile* apProjectile,
    NiPoint3 aStartPoint,
    NiPoint3 aTargetedPoint,
    bool abHighArc,
    float afSpeed
) {
    NiPoint3 kDelta = aStartPoint - aTargetedPoint;
    float fSpeed = afSpeed;
    if (afSpeed == 0.0) {
        fSpeed = apProjectile->GetSpeed();
    }
    if (apProjectile->GetGravity() == 0.0) {
        double dLength = kDelta.Length();
        return static_cast<float>(dLength / afSpeed);
    }

    if (abHighArc && afSpeed == 0.0) {
        afSpeed = afSpeed * fGrenadeHighArcSpeedMult.Float();
    }
    float fDeltaZ = kDelta.z;
    float fDeltaXY = kDelta.XYLength();
    float fGravity = apProjectile->GetGravity() * CombatUtilities::fWorldGravity;
    float fAngle = CombatUtilities::FindAngleOfProjectile(fDeltaXY, fDeltaZ, afSpeed, fGravity, false);
    return CombatUtilities::ComputeProjectileTime(fDeltaXY, fAngle, fSpeed);
}

NiPoint3 CombatUtilities::GetAttackerFiringLocationSimple(TESObjectREFR* apAttacker) {
    NiPoint3 kLocation = apAttacker->GetLocationOnReference();
    kLocation.z = CombatUtilities::GetAttackerFiringZOffset(apAttacker, false) + kLocation.z;
    return kLocation;
}

NiPoint3 CombatUtilities::GetBackProjectedAttackerFiringLocation(TESObjectREFR* apAttacker) {
    NiPoint3 kFiringLocation = CombatUtilities::GetAttackerFiringLocation(apAttacker);
    if (apAttacker->IsActor()) {
        MobileObject* pMobile = static_cast<MobileObject*>(apAttacker);
        NiPoint3 kLocation = apAttacker->GetLocationOnReference();
        float fHeading = pMobile->GetHeading(false);
        float fSideSin = 0.0;
        float fSideCos = 0.0;
        float fSideHeading = fHeading + 1.5707963705062866;
        NiSinCos(fSideHeading, fSideSin, fSideCos);
        NiPoint2 pt(fSideSin, fSideCos);
        float fOffsetY = kFiringLocation.y - kLocation.y;
        float fOffsetX = kFiringLocation.x - kLocation.x;
        NiPoint2 kOffset(fOffsetX, fOffsetY);
        float fProjection = kOffset.Dot(pt);
        float fProjectedX = pt.x * fProjection + kLocation.x;
        float fProjectedY = pt.y * fProjection + kLocation.y;
        float fLooking = pMobile->GetLooking();
        float fSin = 0.0;
        float fCos = 0.0;
        NiSinCos(fLooking, fSin, fCos);
        float fBackY = kFiringLocation.y - fProjectedY;
        float fBackX = kFiringLocation.x - fProjectedX;
        NiPoint2 kBack(fBackX, fBackY);
        float fBackDistance = kBack.Length();
        float fRise = fBackDistance * fSin / fCos;
        kFiringLocation.x = fProjectedX;
        kFiringLocation.y = fProjectedY;
        kFiringLocation.z = kFiringLocation.z + fRise;
    }
    return kFiringLocation;
}

NiPoint3 CombatUtilities::GetAngleToProjectedPoint(
    NiPoint3 aStartLocation,
    NiPoint3 aPivotLocation,
    float afPivotAngle,
    NiPoint3 aProjectedPoint
) {
    if (aPivotLocation.x == aStartLocation.x && aPivotLocation.y == aStartLocation.y) {
        return CombatUtilities::GetAngleToProjectedPoint(aStartLocation, aProjectedPoint);
    }

    NiPoint3 kDelta = aProjectedPoint - aPivotLocation;
    float fSqrLength = kDelta.SqrLength();
    if (fSqrLength == 0.0) {
        return CombatUtilities::GetAngleToProjectedPoint(aStartLocation, aProjectedPoint);
    }

    if (fSqrLength < fMinProjectedPointDistance * fMinProjectedPointDistance) {
        float fLength = NiSqrt(fSqrLength);
        float fScale = fMinProjectedPointDistance / fLength;
        aProjectedPoint = aPivotLocation + kDelta * fScale;
    }
    return CombatUtilities::GetAngleToProjectedPoint(aStartLocation, aProjectedPoint);
}

void CombatUtilities::CalculateCoverPositions(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation,
    NiPoint3* apCoverPosition,
    NiPoint3* apFirePosition
) {
    CombatUtilities::CalculateCoverPositions(
        apCombatController,
        &apCombatCoverLocation->CoverLocation,
        &apCombatCoverLocation->ReservationInfo,
        apCoverPosition,
        apFirePosition
    );
}

void CombatUtilities::CalculateCoverPositions_1(
    float afActorRadius,
    PathingCoverLocation* apCoverLocation,
    CoverReservationInfo* apReservationInfo,
    NiPoint3* apCoverPosition,
    NiPoint3* apFirePosition
) {
    if (apCoverPosition || apFirePosition) {
        float fBackOffset = afActorRadius + 16.0;
        float fFireOffset = 64.0f;
        float fLedgeOffset = 128.0f;
        float fScalar = -fBackOffset;
        NiPoint3 kBack = apCoverLocation->EdgeDirection * fScalar;
        NiPoint3 kSide = apCoverLocation->EdgeDirection.Cross(NiPoint3::UNIT_Z);
        float fOffset = apReservationInfo->GetOffset();
        NiPoint3 kCoverPosition = apCoverLocation->LeftVertex + kSide * fOffset + kBack;
        if (apCoverLocation->IsLedgeCover()) {
            if (apFirePosition) {
                *apFirePosition = kCoverPosition;
            }
            if (apCoverPosition) {
                fScalar = -fLedgeOffset;
                *apCoverPosition = kCoverPosition + apCoverLocation->EdgeDirection * fScalar;
            }
        }
        else {
            if (apCoverPosition) {
                *apCoverPosition = kCoverPosition;
            }
            if (apFirePosition) {
                if (apCoverLocation->IsStandCover() || apReservationInfo->bHideCover) {
                    if (apReservationInfo->bLeftEdge) {
                        *apFirePosition = apCoverLocation->LeftVertex - kSide * fFireOffset + kBack;
                    }
                    else {
                        *apFirePosition = apCoverLocation->RightVertex + kSide * fFireOffset + kBack;
                    }
                }
                else {
                    *apFirePosition = kCoverPosition;
                }
            }
        }
    }
}

bool CombatUtilities::CanActorExistInSpace(Actor* apActor, const BGSWorldLocation& arWorldLocation) {
    return apActor->GetRefPersists() || arWorldLocation.IsLocationLoaded();
}

bool CombatUtilities::IsDangerousExplosion(BGSExplosion* apExplosion) {
    if (!apExplosion) {
        return false;
    }
    float fBaseExplosionDamage = apExplosion->GetDamage();
    if (fDangerousProjectileExplosionDamage.Float() > static_cast<double>(fBaseExplosionDamage)) {
        return false;
    }
    float fExplosionRadius = apExplosion->GetRadiusBSUnits();
    float fDamageDistance = CombatFormulas::CalcExplosionDamageDistance(
        fExplosionRadius,
        fBaseExplosionDamage,
        fDangerousProjectileExplosionDamage.Float()
    );
    return !(fDangerousProjectileExplosionRadius.Float() > static_cast<double>(fDamageDistance));
}

NiPoint3 CombatUtilities::GetProjectedPointForTarget(
    Actor* apSource,
    const NiPoint3& akTargetLocation,
    NiPoint3* arSourcePosition,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    bool abHighArc
) {
    return CombatUtilities::GetProjectedPointForTarget(
        apSource,
        akTargetLocation,
        NiPoint3::ZERO,
        arSourcePosition,
        apProjectile,
        afProjectileSpeed,
        abHighArc
    );
}

NiPoint3 CombatUtilities::GetProjectedPointForTarget(
    TESObjectREFR* apSource,
    NiPoint3 akTargetLocation,
    NiPoint3 akTargetVelocity,
    NiPoint3* arSourcePosition,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    bool abHighArc
) {
    if (apProjectile->GetHitScan()) {
        return akTargetLocation;
    }

    NiPoint3 releasePoint;
    if (arSourcePosition) {
        releasePoint = *arSourcePosition;
    }
    else {
        releasePoint = CombatUtilities::GetAttackerFiringLocation(apSource);
    }
    float fGravity = apProjectile->GetGravity() * CombatUtilities::fWorldGravity;
    if (afProjectileSpeed == 0.0) {
        afProjectileSpeed = apProjectile->GetSpeed();
    }
    return CombatUtilities::GetProjectedPointForTarget(
        releasePoint,
        akTargetLocation,
        akTargetVelocity,
        afProjectileSpeed,
        fGravity,
        abHighArc
    );
}

SpecificItemCollector::SpecificItemCollector(COL_LAYER aePick, CFilter& arFilter) : iFilter(0) {
    Init(aePick, arFilter);
}

float TESCombatStyle::GetWeaponMinRangeMult() {
    return SimpleData.fWeaponMinRangeMult;
}

// The half width is a double division by 2.0; the segment and the segment length are added and multiplied as
// integers on x87 (fiadd, fimul), and the result is rounded to a float local.
float CoverReservationInfo::GetOffset() {
    float fResult = static_cast<float>((cActorWidth / 2.0 + cSegment) * cSegmentLength);
    return fResult;
}
