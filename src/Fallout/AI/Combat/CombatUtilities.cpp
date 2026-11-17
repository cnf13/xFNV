#include "Fallout/AI/Combat/CombatUtilities.h"

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdint>

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
    char acSlota = acAmount + acSlot;
    if (acSlota < 0) {
        acSlota += MAX_SLOTS;
    }
    if (acSlota >= MAX_SLOTS) {
        acSlota -= MAX_SLOTS;
    }
    return acSlota;
}

char GetSlotFromAngle(float afAngle) {
    return (int8_t)((int32_t)((afAngle / 360.0) * ((double)MAX_SLOTS)));
}

uint32_t ReserveSlot(uint32_t aiSlots, char acSlot) {
    int32_t i;
    uint32_t aiSlotsb;
    uint32_t aiSlotsa = aiSlots | (1 << acSlot);
    for (i = 1; i <= SLOT_BORDER; ++i) {
        aiSlotsb = aiSlotsa | (1 << RotateSlot(acSlot, -((int8_t)i)));
        aiSlotsa = aiSlotsb | (1 << RotateSlot(acSlot, i));
    }
    return aiSlotsa;
}

char FindSlot(uint32_t aiSlots, char acDesiredSlot) {
    for (int32_t i = 0; i < MAX_SLOTS / 2; ++i) {
        char v3 = RotateSlot(acDesiredSlot, i);
        if (!IsSlotReserved(aiSlots, v3)) {
            return v3;
        }
        char v4 = RotateSlot(acDesiredSlot, -static_cast<char>(i));
        if (!IsSlotReserved(aiSlots, v4)) {
            return v4;
        }
    }
    return -1;
}

double NiATan(float afX) {
    return std::atan(afX);
}

// Calls hkVector4's non-const operator() on the returned gravity.
void CombatUtilities::Init() {
    hkVector4& kGravity = const_cast<hkVector4&>(TESObjectCELL::ExteriorWorld()->GetGravity());
    fWorldGravity = -HK2NI(kGravity.operator()(2));
}

bool CombatUtilities::IsTargetWithinStrikeAngle(Actor* apAttacker, TESObjectREFR* apTarget, float* afDelta) {
    bool bresult = false;
    ANIM_GROUP_ENUM eAttack = ANIM_GROUP_ATTACK_LEFT;
    if ((apAttacker == pPlayer) && (apAttacker->pCurrentProcess)) {
        BaseProcess* CurrentAIProcess = apAttacker->pCurrentProcess;
        if (CurrentAIProcess->GetAnimActionAnimSeq()) {
            BaseProcess* v14 = apAttacker->pCurrentProcess;
            BSAnimGroupSequence* v3 = v14->GetAnimActionAnimSeq();
            if (v3->GetAnimGroup()) {
                BaseProcess* v13 = apAttacker->pCurrentProcess;
                BSAnimGroupSequence* v4 = v13->GetAnimActionAnimSeq();
                eAttack = v4->GetAnimGroup()->GetType();
            }
        }
    }

    const NiPoint3* a1 = &apAttacker->GetLocationOnReference();
    const NiPoint3* v5 = &apTarget->GetLocationOnReference();
    NiPoint3 movedir = *v5 - *a1;
    float ftargetAngle = GetUnclampedZAngleFromVector(movedir);
    float faddedHeadingRotation = 0.0f;
    if (apAttacker == pPlayer) {
        float fdeltaHeading = 0.0f;
        float fdeltaLooking = 0.0f;
        NiAVObject* v6 = pPlayer->Get3D(false);
        NiAVObject* pprojectile = BSUtilities::GetObjectByName(v6, "ProjectileNode");
        pPlayer->ApplyAutoAimActor(
            nullptr, pprojectile, fdeltaHeading, fdeltaLooking, nullptr, 1.1754944e-38f, 1.1754944e-38f
        );
        if (pPlayer->GetAutoAimActor()) {
            faddedHeadingRotation = fdeltaHeading;
        }
    }
    double v7 = apAttacker->GetHeading(false);
    float v12 = v7 + faddedHeadingRotation - ftargetAngle;
    float fdelta = std::fabsf(v12) * 57.2957763671875;
    if (fdelta > 180.0) {
        float v11 = fdelta - 360.0;
        fdelta = std::fabs(v11);
    }
    float fangle = 0.0f;
    if (apAttacker == pPlayer) {
        fangle = CombatUtilities::GetAngleForMeleeAttack(eAttack);
    }
    else {
        fangle = fCombatHitConeAngle.Float();
    }
    if (movedir.Length() < 128.0) {
        fangle = fangle * 3.0;
    }
    if (!(fangle >= static_cast<double>(fdelta))) {
        if (apTarget->IsActor() && apTarget->IsDead(false)) {
            float* v8 = &fCombatDeadActorHitConeMult.Float();
            fangle = fangle * *v8;
            if (fangle >= static_cast<double>(fdelta)) {
                bresult = true;
            }
        }
    }
    else {
        bresult = true;
    }
    if (afDelta) {
        *afDelta = fdelta;
    }
    return bresult;
}

Actor* CombatUtilities::GetFacingTarget(Actor* apAttacker, float afRadius) {
    Actor* ptarget;
    float fdelta_3;
    float fdelta_2;
    float fdelta;
    if (apAttacker->GetCombatController()) {
        ptarget = apAttacker->GetCombatController()->GetTarget();
    }
    else {
        ptarget = nullptr;
    }
    if (ptarget &&
        (CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, ptarget, nullptr) &&
         static_cast<double>(afRadius) >= CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, ptarget, true))) {
        return ptarget;
    }
    if (ptarget) {
        return nullptr;
    }
    Actor* pclosestTarget = nullptr;
    float fclosestDelta = 3.4028235e+38f;
    MobileObject* v19 = nullptr;
    TESObjectREFR* v21 = nullptr;
    for (uint32_t i = 0;; ++i) {
        Actor* pactor;
        uint32_t ProcessLevelTail = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        MobileObject* pmob = (&ReferenceProcessLists.MobProcessArray)->GetItem(i);
        if (pmob && pmob->IsActor()) {
            pactor = static_cast<Actor*>(pmob);
        }
        else {
            pactor = nullptr;
        }
        if (pVATS.eMode != VATS::VATS_PLAYBACK || (!pactor || !pactor->IsDead(false))) {
            if ((pactor) && (pactor != apAttacker && (!pactor->IsDead(false) || apAttacker == pPlayer)) &&
                (pactor->Get3D()) &&
                (static_cast<double>(afRadius) >=
                 CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pactor, false))) {
                fdelta = 3.4028235e+38f;
                if (CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pactor, &fdelta) &&
                    fclosestDelta >= static_cast<double>(fdelta)) {
                    pclosestTarget = pactor;
                    fclosestDelta = fdelta;
                }
            }

            if (pactor) {
                ExtraFollower* pfollow = (&pactor->m_Extra)->GetFollowerExtra();
                if (pfollow) {
                    for (Actor* pact : pfollow->pActorlist->ItemsUntilNull()) {
                        if ((pact) &&
                            (pact->GetCurrentProcessType() == PROCESS_TYPE_HIGH && pact != apAttacker &&
                             (!pactor->IsDead(false) || apAttacker == pPlayer)) &&
                            (pact->Get3D()) &&
                            (static_cast<double>(afRadius) >=
                             CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pact, false))) {
                            fdelta_2 = 3.4028235e+38f;
                            if (CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pact, &fdelta_2) &&
                                fclosestDelta >= static_cast<double>(fdelta_2)) {
                                pclosestTarget = pact;
                                fclosestDelta = fdelta_2;
                            }
                        }
                    }
                }
            }
        }
    }
    if (apAttacker != pPlayer) {
        float fplayerDistance = CombatUtilities::GetDistanceBetweenFacingActors(apAttacker, pPlayer, false);
        fdelta_3 = 3.4028235e+38f;
        bool binStrikeAngle = CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, pPlayer, &fdelta_3);
        if (afRadius >= static_cast<double>(fplayerDistance) && binStrikeAngle &&
            fclosestDelta >= static_cast<double>(fdelta_3)) {
            pclosestTarget = pPlayer;
        }
    }
    if ((pclosestTarget) && (apAttacker->IsPc() && !pPlayer->LineOfSight(true, pclosestTarget, true, nullptr, false))) {
        return nullptr;
    }

    return pclosestTarget;
}

float CombatUtilities::GetAttackerFiringZOffset(TESObjectREFR* apAttacker, bool abIgnoreCrouch) {
    NiPoint3 v9;
    if (apAttacker->IsActor()) {
        float fScale;
        Actor* pActor = static_cast<Actor*>(apAttacker);
        if (!static_cast<Actor*>(apAttacker)->IsSneaking() || abIgnoreCrouch) {
            fScale = pActor->GetHeight();
        }
        else {
            fScale = pActor->GetSneakHeight(false);
        }
        return fScale * 0.75;
    }

    NiPoint3 v8;
    NiPoint3* v3 = &(v9 = apAttacker->GetBoundMax());
    return (v3->z - (&(v8 = apAttacker->GetBoundMin()))->z) * 0.5;
}

NiPoint3 CombatUtilities::ComputeFaceTargetLocation(
    Actor* apAttacker,
    TESObjectREFR* apTarget,
    const NiPoint3& arTargetLocation,
    float afAttackerReach
) {
    NiPoint3 result;
    NiPoint3 targetVector = NiPoint3_ZERO;
    Actor* ptargetActor = apTarget->IsActor() ? static_cast<Actor*>(apTarget) : nullptr;
    CombatController* pcombatController = apAttacker->GetCombatController();
    if (pcombatController &&
        (ptargetActor && CombatUtilities::CalculateAttackerCombatPositions(ptargetActor, apAttacker))) {
        float fcombatPosition = pcombatController->GetCombatPosition();
        float fAngleRad = fcombatPosition * 0.01745329238474369;
        NiSinCos(fAngleRad, targetVector.x, targetVector.y);
    }
    else {
        const NiPoint3* v5 = &apAttacker->GetLocationOnReference();
        NiPoint3 targetVector_2 = *v5 - arTargetLocation;
        targetVector_2.Unitize();
    }
    float fScaledReach = CombatUtilities::ComputeFaceTargetDistance(apAttacker, apTarget, afAttackerReach);
    targetVector.operator*=(fScaledReach);
    result = arTargetLocation + targetVector;
    return result;
}

bool CombatUtilities::IsActorUsingMelee(Actor* apActor) {
    TESObjectWEAP* pWeapon;
    CombatController* pCombatController = apActor->GetCombatController();
    if (pCombatController) {
        pWeapon = pCombatController->GetCurrentWeapon();
    }
    else {
        pWeapon = apActor->GetCurrentWeapon();
    }
    if ((!pWeapon) || pWeapon->IsMeleeWeapon()) {
        return true;
    }
    return (bool)((uint8_t)(pCombatController &&
                            (pCombatController->CombatPlanContainsAction(7) ||
                             pCombatController->CombatPlanContainsAction(8))));
}

float CombatUtilities::CalculateItemDPS(
    ActorValueOwner* apActor,
    TESObjectWEAP* apWeapon,
    float afCondition,
    bool a4,
    ItemChange* apItem,
    bool a6,
    bool a7,
    int32_t a8,
    float a9,
    float a10,
    bool a11,
    bool a12,
    TESAmmo* apAmmo
) {
    double v15;
    double v27;
    float fcritChance;
    float fdamagePerHit;
    float fBonus;
    if (!(afCondition <= 0.0)) {
        float v29;
        float v30;
        float fCriticalDamage;
        Actor* pactor;
        float fAnimShotsPerSecond;
        float fresult = 0.0f;
        int32_t iclipSize = 1;
        fdamagePerHit = 0.0f;
        fcritChance = 0.0f;
        float fsecondsPerClip = 3.4028235e+38f;
        bool bcreature = false;
        bool bHasClipIncrease = false;
        bool bHasSpeedIncrease = false;
        if (apWeapon) {
            fAnimShotsPerSecond = apWeapon->GetAnimShotsPerSecond();
        }
        else {
            fAnimShotsPerSecond = 1.0f;
        }
        float fAnimShotsPerSecond_2 = fAnimShotsPerSecond;
        bool HasModEffectActive_0 = false;
        float v48 = afCondition * 10.0;
        int32_t iroundedCondition = static_cast<int32_t>(BSRoundToNearest(v48, 1.0f));
        fBonus = 0.0f;
        // Compares the two forms' addresses.
        if (static_cast<TESForm*>(apAmmo) == static_cast<TESForm*>(apWeapon)) {
            apAmmo = nullptr;
        }
        if (apActor) {
            pactor = static_cast<Actor*>(apActor);
        }
        else {
            pactor = nullptr;
        }
        bcreature = pactor->IsCreature();
        if (apItem) {
            apItem->HasModEffectActive(WEAPON_MOD_INCREASE_WEAPON_DAMAGE, fBonus);
            bHasClipIncrease = apItem->HasModEffectActive(2);
            bHasSpeedIncrease = apItem->HasModEffectActive(8);
            fAnimShotsPerSecond_2 = Actor::GetAttackSpeedMult(pactor, apWeapon, 0, apItem);
            HasModEffectActive_0 = apItem->HasModEffectActive(12);
        }
        if (apWeapon) {
            float v35;
            BSSimpleList<TESAmmoEffect*>* pAmmoEffectList;
            TESAmmo* pAmmo;
            float* v14 = &fDamageWeaponMult.Float();
            fdamagePerHit = static_cast<double>(apWeapon->GetAttackDamage()) * *v14;
            fdamagePerHit = fdamagePerHit + fBonus;
            if (HasModEffectActive_0) {
                fdamagePerHit = fdamagePerHit * 1.3;
            }
            if (apAmmo) {
                pAmmo = apAmmo;
            }
            else {
                pAmmo = apWeapon->GetCurrentAmmo(nullptr);
            }
            if (pAmmo) {
                pAmmoEffectList = pAmmo->GetAmmoEffectList();
            }
            else {
                pAmmoEffectList = nullptr;
            }
            v15 = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_DAM, pAmmoEffectList, fdamagePerHit);
            fdamagePerHit = v15;
            if ((apWeapon->GetProjectile(nullptr)) && (apWeapon->GetProjectile(nullptr)->GetExplosion())) {
                BGSProjectile* Projectile_ = apWeapon->GetProjectile(nullptr);
                double BranchDimming = Projectile_->GetExplosion()->Data.fDamage;
                v15 = static_cast<double>(apWeapon->GetNumProjectiles(HasModEffectActive_0, false, pactor)) *
                        BranchDimming +
                    fdamagePerHit;
                fdamagePerHit = v15;
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
                    if ((apWeapon->GetProjectile(nullptr)) && (apWeapon->GetProjectile(nullptr)->GetExplosion())) {
                        float* v17 = &fDamageWeaponMult.Float();
                        BGSProjectile* v18 = apWeapon->GetProjectile(nullptr);
                        fdamagePerHit = v18->GetExplosion()->Data.fDamage * *v17;
                    }

                    v15 = 1.0;
                    iroundedCondition = 10;
                    break;
                }
                default: {
                    iclipSize = apWeapon->GetFormClipRounds(bHasClipIncrease);
                    break;
                }
            }
            if (a8 == -1) {
                double v43;
                if (apActor) {
                    v43 = apActor->GetClampedActorFloatValue(apWeapon->GetWeaponSkill()) / 100.0;
                }
                else {
                    v43 = 1.0;
                }
                float* v19 = &fDamageSkillBase.Float();
                fdamagePerHit = (fDamageSkillMult.Float() * v43 + *v19) * fdamagePerHit;
            }
            else {
                float v53 = static_cast<double>(a8) / 100.0;
                float* v20 = &fDamageSkillBase.Float();
                fdamagePerHit = (fDamageSkillMult.Float() * v53 + *v20) * fdamagePerHit;
            }
            if (a4) {
                BGSEntryPoint::HandleEntryPoint(
                    BGSEntryPoint::CALCULATE_WEAPON_DAMAGE, pactor, apWeapon, pactor, &fdamagePerHit
                );
            }
            fsecondsPerClip = fAnimShotsPerSecond_2;
            if (apWeapon->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
                apWeapon->GetWeaponType() != WEAPON_ONE_HAND_MINE &&
                apWeapon->GetWeaponType() != WEAPON_ONE_HAND_THROWN) {
                if (a7) {
                    double v42 = static_cast<double>(iclipSize);
                    double v41 = GetWeaponConditionJamChance(0) * v42;
                    double AnimJamTime = apWeapon->GetAnimJamTime();
                    fsecondsPerClip = AnimJamTime * v41 + fsecondsPerClip;
                }
                else {
                    double v40 = static_cast<double>(iclipSize);
                    double v39 = GetWeaponConditionJamChance(iroundedCondition) * v40;
                    double v22 = apWeapon->GetAnimJamTime();
                    fsecondsPerClip = v22 * v39 + fsecondsPerClip;
                }
            }
            fsecondsPerClip = static_cast<double>(iclipSize) / fsecondsPerClip;
            int32_t* v23 = &iMinClipSizeToAddReloadDelay.Int();
            if (((iclipSize <= *v23 || a6) && apWeapon->GetWeaponType() != WEAPON_ONE_HAND_MELEE &&
                 apWeapon->GetWeaponType() != WEAPON_TWO_HAND_MELEE) &&
                (apWeapon->GetWeaponType()) &&
                (apWeapon->GetWeaponType() != WEAPON_ONE_HAND_GRENADE &&
                 apWeapon->GetWeaponType() != WEAPON_ONE_HAND_THROWN &&
                 apWeapon->GetWeaponType() != WEAPON_ONE_HAND_MINE)) {
                double AnimReloadTime = apWeapon->GetAnimReloadTime();
                fsecondsPerClip = AnimReloadTime + fsecondsPerClip;
            }

            if (pactor != pPlayer && apWeapon->IsRangedWeapon()) {
                if (!apWeapon->IsAutomaticWeapon()) {
                    double SemiAutomaticFireDelayMax = apWeapon->GetSemiAutomaticFireDelayMax();
                    float favgDelayTime = (apWeapon->GetSemiAutomaticFireDelayMin() + SemiAutomaticFireDelayMax) / 2.0;
                    float fshotTime = 1.0 / fAnimShotsPerSecond_2;
                    if (fshotTime < static_cast<double>(favgDelayTime)) {
                        float favgDelayTimeModified = favgDelayTime - fshotTime;
                        fsecondsPerClip = static_cast<double>(iclipSize) * favgDelayTimeModified + fsecondsPerClip;
                    }
                }
                if (apWeapon->IsAutomaticWeapon() || apWeapon->GetBurstShot()) {
                    float* v25 = &fAutomaticWeaponBurstFireTime.Float();
                    double v36 = *v25 + fAutomaticWeaponBurstCooldownTime.Float();
                    float* v26 = &fAutomaticWeaponBurstCooldownTime.Float();
                    fsecondsPerClip = v36 / *v26 * fsecondsPerClip;
                }
            }
            float v54 = 0.0;
            if (a9 != 0.0) {
                v54 = a9 * 100.0;
            }
            if (apActor) {
                v35 = apActor->GetActorFloatValue(ActorValue::CRITICAL_CHANCE);
            }
            else {
                v35 = v54;
            }
            fcritChance = apWeapon->GetCriticalChanceMult() * (v35 * 1.0);
            BSClamp(fcritChance, 0.0f, 100.0f);
            fcritChance = fcritChance * 0.009999999776482582;
            if (apWeapon->IsAutomaticWeapon() && fAnimShotsPerSecond_2 != 0.0) {
                fcritChance = fcritChance / fAnimShotsPerSecond_2;
            }
        }
        else {
            float fsecondsPerClip_2;
            if (bcreature) {
                fdamagePerHit = static_cast<float>(pactor->GetAttackDamage());
            }
            else {
                double v33;
                float v34;
                if (apActor) {
                    v34 = apActor->GetActorFloatValue(ActorValue::UNARMED_DAMAGE);
                }
                else {
                    v34 = 0.0f;
                }
                v27 = fUnarmedDamageMult.Float() * v34;
                fdamagePerHit = v27;
                if (apActor) {
                    v33 = apActor->GetClampedActorFloatValue(ActorValue::UNARMED) / 100.0;
                }
                else {
                    v33 = 0.0;
                }
                fdamagePerHit = fDamageSkillMult.Float() * v33 + fdamagePerHit;
            }
            if (bcreature) {
                fsecondsPerClip_2 = fUnarmedCreatureDPSMult.Float();
            }
            else {
                fsecondsPerClip_2 = fUnarmedNPCDPSMult.Float();
            }
            fsecondsPerClip = fsecondsPerClip_2;
        }
        float fconditionMod = CombatFormulas::ApplyWeaponConditionScaling(afCondition);
        fdamagePerHit = fdamagePerHit * fconditionMod;
        if (apWeapon) {
            fCriticalDamage = static_cast<float>(apWeapon->GetCriticalDamage());
        }
        else {
            fCriticalDamage = 0.0f;
        }
        float v55 = fCriticalDamage;
        if (a10 != 0.0 && fCriticalDamage != 0.0) {
            v55 = fCriticalDamage * a10;
        }
        if (a11) {
            v30 = 2.0f;
        }
        else {
            v30 = 1.0f;
        }
        if (a12) {
            v29 = 1.25f;
        }
        else {
            v29 = 1.0f;
        }
        float fdamagePerClip = static_cast<double>(iclipSize) * ((fcritChance * v55 + fdamagePerHit) * (v30 * v29));
        if (fsecondsPerClip != 0.0) {
            return fdamagePerClip / fsecondsPerClip;
        }
        return fresult;
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
    NiPoint3 v12;
    NiPoint3 v13;
    NiPoint3 v14;
    NiPoint3 v15;
    if (!apTarget) {
        return 0.0f;
    }
    float fDistance = apAttacker->GetDistanceFromReference(apTarget, false, false);
    if (fDistance == 3.4028234663852886e+38) {
        return fDistance;
    }
    if ((apAttacker->IsActor() && apTarget->IsActor()) && (apAttacker)) {
        double fHeightDelta;
        float fHeightDiff;
        bool bAreSwimming =
            static_cast<Actor*>(apAttacker)->IsSwimming() && static_cast<Actor*>(apTarget)->IsSwimming();
        NiPoint3 kActorPos = apAttacker->GetLocationOnReference();
        NiPoint3 kTargetPos = apTarget->GetLocationOnReference();
        if (bAreSwimming ||
            (abMeleeCombat &&
             (fHeightDiff = kActorPos.z - kTargetPos.z,
              (fHeightDelta = std::fabsf(fHeightDiff), fAICombatSlopeDifference.Float() <= fHeightDelta)))) {
            kActorPos.z = 0.0;
            kTargetPos.z = 0.0;
            NiPoint3* pActorBoundMax = &(v15 = apAttacker->GetBoundMax());
            float fActorZMax = pActorBoundMax->z + kActorPos.z;
            NiPoint3* pActorBoundMin = &(v14 = apAttacker->GetBoundMin());
            float fActorZMin = pActorBoundMin->z + kActorPos.z;
            NiPoint3* pTargetBoundMax = &(v13 = apTarget->GetBoundMax());
            float fTargetZMax = pTargetBoundMax->z + kTargetPos.z;
            NiPoint3* pTargetBoundMin = &(v12 = apTarget->GetBoundMin());
            float fTargetZMin = pTargetBoundMin->z + kTargetPos.z;
            if ((fTargetZMax >= static_cast<double>(fActorZMax) && fTargetZMin <= static_cast<double>(fActorZMax)) ||
                (fTargetZMax >= static_cast<double>(fActorZMin) && fTargetZMin <= static_cast<double>(fActorZMin))) {
                NiPoint3 result;
                result = kActorPos - kTargetPos;
                fDistance = result.Length();
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
    NiPoint3 rot;
    CombatUtilities::GetAngleToProjectedPoint(&rot, apAttacker, arTargetLocation);
    if (afArcAngle < 6.2831854820251465) {
        float fheading = apAttacker->GetActualHeading();
        rot.z = ClampAngle(rot.z);
        float fdir = 0.0f;
        float a1 = GetAngleDelta(fheading, rot.z, fdir);
        float fdelta = std::fabs(a1);
        if (fdelta > afArcAngle * 0.5) {
            return false;
        }
    }
    if (!(afLookingAngle < 6.2831854820251465)) {
        return true;
    }
    float flooking = apAttacker->GetLooking();
    rot.x = ClampAngle(rot.x);
    float fdir_2 = 0.0f;
    float a1a = GetAngleDelta(flooking, rot.x, fdir_2);
    float fdelta_2 = std::fabs(a1a);
    return !(fdelta_2 > afLookingAngle * 0.5);
}

float CombatUtilities::GetAttackerTargetExtentSum(TESObjectREFR* apAttacker, TESObjectREFR* apTarget) {
    NiPoint3 v8;
    NiPoint3 v9;
    float fHeight = 0.0;
    if (apAttacker->IsActor()) {
        Actor* v11 = static_cast<Actor*>(apAttacker);
        fHeight = static_cast<Actor*>(apAttacker)->GetForwardLength();
    }
    else {
        NiPoint3* v2 = &(v9 = apAttacker->GetBoundMax());
        fHeight = apAttacker->GetScale() * v2->y;
    }
    if (apTarget) {
        if (apTarget->IsActor()) {
            Actor* v10 = static_cast<Actor*>(apTarget);
            double ForwardLength = static_cast<Actor*>(apTarget)->GetForwardLength();
            return ForwardLength + fHeight;
        }

        NiPoint3* v4 = &(v8 = apTarget->GetBoundMax());
        double Scale = apTarget->GetScale();
        return Scale * v4->y + fHeight;
    }
    return fHeight;
}

float CombatUtilities::GetWeaponReach(Actor* apAttacker, TESObjectWEAP* apWeapon) {
    float maxFarNearRatio;
    float scale = apAttacker->GetScale();
    if (apWeapon) {
        if (apWeapon->IsMeleeWeapon()) {
            maxFarNearRatio = apWeapon->GetReach();
            return (CombatFormulas::CalcWeaponReach(maxFarNearRatio) * scale);
        }

        return (float)(scale * 128.0);
    }

    return (apAttacker->GetReach() * scale);
}

// afAttackerReach is compared with the double -1.0 (a NaN keeps it) and halved with the double 0.5.
float CombatUtilities::ComputeFaceTargetDistance(Actor* apAttacker, TESObjectREFR* apTarget, float afAttackerReach) {
    float fExtentSum = GetAttackerTargetExtentSum(apAttacker, apTarget);
    if (afAttackerReach == -1.0) {
        afAttackerReach = GetWeaponReach(apAttacker, apAttacker->GetCurrentWeapon());
    }
    float fDistance = float(afAttackerReach * 0.5);
    fDistance = fDistance + fExtentSum;
    return BSMAX(fDistance, 0.0f);
}

NiPoint3 CombatUtilities::GetTargetFiringLocation(TESObjectREFR* apTarget, ACTOR_SEGMENT_INVIEW aeTargetSegmentInView) {
    NiPoint3 result;
    NiPoint3 kActorPos = *apTarget->QLocationOnReference();
    kActorPos.z = CombatUtilities::GetTargetFiringZOffset(apTarget, aeTargetSegmentInView, false) + kActorPos.z;
    result = kActorPos;
    return result;
}

NiPoint3 CombatUtilities::GetTargetVelocity(TESObjectREFR* apTarget) {
    NiPoint3 result;
    NiPoint3 velocity = NiPoint3_ZERO;
    if (apTarget->IsActor()) {
        bhkCharacterController* pcon = static_cast<MobileObject*>(apTarget)->MobileObject::GetCharController();
        if (pcon) {
            pcon->GetLinearVelocity(velocity);
        }
    }
    result = velocity;
    return result;
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
        return NiPoint3_ZERO;
    }
    NiPoint3 kVelocity = GetTargetVelocity(apTarget);
    NiPoint3 kTargetLocation = GetTargetFiringLocation(apTarget, aeTargetSegmentInView);
    NiPoint3 kProjectedPoint =
        GetProjectedPointForTarget(aStartLocation, kTargetLocation, kVelocity, afSpeed, afGravity, false);
    return GetAngleToProjectedPoint(aStartLocation, kProjectedPoint);
}

NiPoint3* CombatUtilities::GetAngleToProjectedTarget(
    NiPoint3* apResult,
    TESObjectREFR* apAttacker,
    TESObjectREFR* apTarget,
    float afSpeed,
    float afGravity,
    ACTOR_SEGMENT_INVIEW aeTargetSegmentInView
) {
    NiPoint3 shooterLocation = CombatUtilities::GetAttackerFiringLocation(apAttacker);
    NiPoint3 angle = CombatUtilities::GetAngleToProjectedTarget(
        shooterLocation, apTarget, afSpeed, afGravity, aeTargetSegmentInView
    );
    angle = angle - apAttacker->GetAngleOnReference();
    angle.y = 0.0f;
    if (!(angle.x <= -3.1415927410125732)) {
        if (angle.x > 3.1415927410125732) {
            angle.x = angle.x - 6.2831854820251465;
        }
    }
    else {
        angle.x = angle.x + 6.2831854820251465;
    }
    if (!(angle.z <= -3.1415927410125732)) {
        if (angle.z > 3.1415927410125732) {
            angle.z = angle.z - 6.2831854820251465;
        }
    }
    else {
        angle.z = angle.z + 6.2831854820251465;
    }
    *apResult = angle;
    return apResult;
}

NiPoint3 CombatUtilities::GetProjectedPointForTarget(
    NiPoint3 aStartLocation,
    NiPoint3 aTargetLocation,
    NiPoint3 aTargetVelocity,
    float afProjectileSpeed,
    float afProjectileGravity,
    bool abHighArc
) {
    NiPoint3 result;
    int32_t iAimingNumIterations = ::iAimingNumIterations.Int();
    if (aTargetVelocity.x == 0.0 && aTargetVelocity.y == 0.0 && aTargetVelocity.z == 0.0) {
        iAimingNumIterations = 1;
    }
    float fScalar = 0.0;
    NiPoint3 v24 = aTargetLocation;
    if (afProjectileGravity == 0.0) {
        float v23 = 0.0f;
        for (int32_t i = 0; i < iAimingNumIterations; ++i) {
            NiPoint3 resulta = v24 - aStartLocation;
            v23 = resulta.Length();
            if (v23 < 1.0 || v23 > 8192.0) {
                break;
            }
            fScalar = v23 / afProjectileSpeed;
            v24 = aTargetLocation + aTargetVelocity * fScalar;
        }
    }
    else {
        float afDeltaXY = 0.0f;
        float afDeltaZ = 0.0f;
        float AngleOfProjectile = 0.0f;
        for (int32_t j = 0; j < iAimingNumIterations; ++j) {
            NiPoint3 v15 = v24 - aStartLocation;
            afDeltaZ = v15.z;
            v15.z = 0.0f;
            afDeltaXY = v15.Length();
            if (afDeltaZ > 8192.0 || (afDeltaXY < 1.0 || afDeltaXY > 8192.0)) {
                break;
            }
            AngleOfProjectile = CombatUtilities::FindAngleOfProjectile(
                afDeltaXY, afDeltaZ, afProjectileSpeed, afProjectileGravity, abHighArc
            );
            fScalar = CombatUtilities::ComputeProjectileTime(afDeltaXY, AngleOfProjectile, afProjectileSpeed);
            v24 = aTargetLocation + aTargetVelocity * fScalar;
        }
        NiPoint3 v20 = v24 - aStartLocation;
        v20.z = 0.0f;
        afDeltaXY = v20.Length();
        double v9 = std::tan(AngleOfProjectile);
        v24.z = v9 * afDeltaXY + aStartLocation.z;
    }
    result = v24;
    return result;
}

NiPoint3 CombatUtilities::GetProjectedPointForTarget(
    TESObjectREFR* apAttacker,
    TESObjectREFR* apTarget,
    NiPoint3* apReleasePoint,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    bool abHighArc
) {
    NiPoint3 result;
    ACTOR_SEGMENT_INVIEW etargetSegmentInView = ACTOR_SEGMENT_INVIEW_MIDDLE;
    if (apAttacker->IsActor()) {
        CombatController* pcombatController = static_cast<Actor*>(apAttacker)->GetCombatController();
        if (pcombatController) {
            etargetSegmentInView = pcombatController->GetTargetSegmentInView();
        }
    }
    if (!abHighArc || apProjectile->GetGravity() == 0.0) {
        double grenadeSpeed_1;
        BGSExplosion* pexplosion = apProjectile->GetExplosion();
        bool bfastProjectile = false;
        if (apProjectile->GetHitScan() ||
            (grenadeSpeed_1 = apProjectile->GetSpeed(), fCombatSplashDamageMaxSpeed.Float() < grenadeSpeed_1)) {
            bfastProjectile = true;
        }
        if (pexplosion) {
            double explosionRadius = pexplosion->GetRadiusBSUnits();
            if (fCombatSplashDamageMinRadius.Float() <= explosionRadius && !bfastProjectile) {
                double explosionDamage = pexplosion->Data.fDamage;
                if (fCombatSplashDamageMinDamage.Float() < explosionDamage) {
                    etargetSegmentInView = ACTOR_SEGMENT_INVIEW_GROUND;
                }
            }
        }
    }
    else {
        etargetSegmentInView = ACTOR_SEGMENT_INVIEW_TOP;
    }
    NiPoint3 targetLoc = CombatUtilities::GetTargetFiringLocation(apTarget, etargetSegmentInView);
    NiPoint3 targetVelocity = CombatUtilities::GetTargetVelocity(apTarget);
    CombatUtilities::GetProjectedPointForTarget(
        &result, static_cast<Actor*>(apAttacker), targetLoc, targetVelocity, apReleasePoint, apProjectile,
        afProjectileSpeed, abHighArc
    );
    return result;
}

NiPoint3 CombatUtilities::GetAngleToProjectedPoint(NiPoint3 aStartLocation, NiPoint3 aProjectedPoint) {
    NiPoint3 result;
    NiPoint3 aDirection;
    aDirection = aProjectedPoint - aStartLocation;
    float v7 = aDirection.z;
    float Length = aDirection.UnitizeGetLength();
    float v4 = v7 / Length;
    float v5 = NiASin(v4);
    float x = -v5;
    if (!(x <= -3.1415927410125732)) {
        if (x > 3.1415927410125732) {
            x = x - 6.2831854820251465;
        }
    }
    else {
        x = x + 6.2831854820251465;
    }
    float UnclampedZAngleFromVector = GetUnclampedZAngleFromVector(aDirection);
    float z = UnclampedZAngleFromVector;
    if (!(UnclampedZAngleFromVector <= -3.1415927410125732)) {
        if (UnclampedZAngleFromVector > 3.1415927410125732) {
            z = UnclampedZAngleFromVector - 6.2831854820251465;
        }
    }
    else {
        z = UnclampedZAngleFromVector + 6.2831854820251465;
    }
    ::new (&result) NiPoint3(x, static_cast<float>(0.0), z);
    return result;
}

NiPoint3*
CombatUtilities::GetAngleToProjectedPoint(NiPoint3* apResult, TESObjectREFR* apAttacker, NiPoint3 aProjectedPoint) {
    NiPoint3 apResulta;
    NiPoint3 attackerFiringLocation = CombatUtilities::GetAttackerFiringLocation(apAttacker);
    float v5 = 0.0f;
    NiPoint3 v7 = *CombatUtilities::GetAttackerFiringLocationSimple(&apResulta, apAttacker);
    CombatUtilities::GetAngleToProjectedPoint(apResult, attackerFiringLocation, v7, 0.0f, aProjectedPoint);
    return apResult;
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
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
    kFilter.SetLayer(L_PROJECTILE);
    // GetCollisionFilter writes the actor's filter word into kActorFilter and returns it; the group is then read
    // through CFilter::GetGroup on that same local.
    CFilter kActorFilter;
    apActor->GetCollisionFilter(&kActorFilter.iFilter);
    kFilter.SetGroup(kActorFilter.GetGroup());
    double dGravity;
    if (apProjectile) {
        dGravity = apProjectile->GetGravity() * CombatUtilities::fWorldGravity;
    }
    else {
        dGravity = 0.0;
    }
    float fGravity = static_cast<float>(dGravity);
    NiAVObject* pPicked = nullptr;
    NiAVObject* pHitObj = nullptr;
    // Raw stack storage the collector is built in, because a local of the class would run a C++ constructor and
    // destructor.
    alignas(16) uint8_t acCollector[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = reinterpret_cast<hkpAllRayHitCollector*>(acCollector);
    ::new (pCollector) hkpAllRayHitCollector;
    alignas(16) bhkPickData kPick;
    kPick.~bhkPickData();
    ::new (&kPick) bhkPickData;
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
        pPicked = apActor->GetParentCell()->Pick(kPick);
        pHitObj = Projectile::ProcessTargetPick(&kPick, nullptr, apProjectile, &fHitFraction, false, 3.4028235e+38f);
        if (bDebugCombatProjectileLOS.Bool() &&
            (!Main::pDebugTextReference || Main::pDebugTextReference == static_cast<TESObjectREFR*>(pPlayer) ||
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
            kTo.x = aStartLocation.x;
            kTo.y = aStartLocation.y;
            kTo.z = aStartLocation.z;
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
            pPicked = apActor->GetParentCell()->Pick(kPick);
            pHitObj =
                Projectile::ProcessTargetPick(&kPick, nullptr, apProjectile, &fHitFraction, false, 3.4028235e+38f);
            if (bDebugCombatProjectileLOS.Bool() &&
                (!Main::pDebugTextReference || Main::pDebugTextReference == static_cast<TESObjectREFR*>(pPlayer) ||
                 Main::pDebugTextReference == apActor)) {
                AddLOSDebugGeomtry(kFrom, kTo, pHitObj, fHitFraction, nullptr, nullptr);
            }
            if (pHitObj) {
                break;
            }
            kFrom = kTo;
        }
    }
    (void)pPicked;
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
        NiPoint3 kProjected =
            GetProjectedPointForTarget(aStartLocation, aTargetLocation, aTargetVelocity, fSpeed, fGravity, bHighArc);
        bool bHit = CalculateProjectileLOS(
            apAttacker, apProjectile, 0.0f, aStartLocation, kProjected, nullptr, &pRefHit, &fDistanceFraction
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
        float fSpeedMultiplier;
        if (arbHighArc) {
            fSpeedMultiplier = gs_fGrenadeHighArcSpeedPercentage.Float();
        }
        else {
            fSpeedMultiplier = 1.0f;
        }
        float fSpeed = apProjectile->GetSpeed() * fSpeedMultiplier;
        NiPoint3 kProjected =
            GetProjectedPointForTarget(aStartLocation, aTargetLocation, aTargetVelocity, fSpeed, fGravity, arbHighArc);
        bool bHit = CalculateProjectileLOS(
            apAttacker, apProjectile, fSpeed, aStartLocation, kProjected, nullptr, &pRefHit, &fDistanceFraction
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
        bool bIsPlayer = (apAttacker == pPlayer);
        CombatController* pCombatController = pActor->GetCombatController();
        TESObjectWEAP* pWeapon;
        if (pCombatController) {
            pWeapon = pCombatController->GetCurrentWeapon();
        }
        else {
            pWeapon = pActor->GetCurrentWeapon();
        }
        if (!pWeapon || pWeapon->IsRangedWeapon()) {
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            if (apAttacker && pProcess) {
                if (!pWeapon || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
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
    float fSpeedValue = fSpeed;
    // Divides at double precision and rounds to float only on the store, as the game does.
    float fMaxRange = (float)((double)fSpeedValue * fSpeedValue / fGravity);
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
        float* pfAbsoluteMult = &fCombatAbsoluteMaxRangeMult.Float();
        arfAbsoluteMaxRange = arfMaxRange * *pfAbsoluteMult;
        return;
    }
    arfMinRange = apEquippedWeapon->GetMinRange();
    arfMaxRange = apEquippedWeapon->GetMaxRange();
    if (!apEquippedWeapon->GetIsRangeFixed()) {
        arfMinRange = pCombatStyle->GetWeaponMinRangeMult() * arfMinRange;
        arfMaxRange = pCombatStyle->GetWeaponMaxRangeMult() * arfMaxRange;
    }
    float* pfAbsoluteMult = &fCombatAbsoluteMaxRangeMult.Float();
    arfAbsoluteMaxRange = arfMaxRange * *pfAbsoluteMult;
    BGSProjectile* pProjectile = apEquippedWeapon->GetProjectile(apCombatController->GetAttacker());
    if (!pProjectile) {
        return;
    }
    float fRange = GetProjectileMaximumRange(pProjectile, 0.0f);
    float* pfOptimalMult = &fCombatProjectileMaxRangeOptimalMult.Float();
    float fOptimalRange = fRange * *pfOptimalMult;
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
    NiColorA kDefaultHitColor;
    const NiColorA* pHitColor =
        apHitColor ? apHitColor : FNVD_RECONSTRUCT(NiColorA)(&kDefaultHitColor, 1.0f, 0.0f, 0.0f, 1.0f);
    NiColorA kHitColor = *pHitColor;
    NiColorA kDefaultLOSColor;
    const NiColorA* pLOSColor =
        apLOSColor ? apLOSColor : FNVD_RECONSTRUCT(NiColorA)(&kDefaultLOSColor, 1.0f, 1.0f, 0.0f, 1.0f);
    NiColorA kLOSColor = *pLOSColor;
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
    CombatState* pcombatState = apController->pCombatState;
    float fattackDPS = pcombatState->GetMaxDPSForWeaponType(aeType);
    if (fattackDPS == 0.0) {
        return 3.4028235e+38f;
    }

    float fmaxAttackDPS = pcombatState->fMaxModifiedCombatWeaponDPS;
    return fmaxAttackDPS - fattackDPS;
}

float CombatUtilities::GetTargetFiringZOffset(
    TESObjectREFR* apTarget,
    ACTOR_SEGMENT_INVIEW aeTargetSegmentInView,
    bool abIgnoreCrouch
) {
    double v3;
    if (apTarget->IsActor()) {
        float fheight;
        Actor* ptargetActor = static_cast<Actor*>(apTarget);
        bool bIsSneaking = static_cast<Actor*>(apTarget)->Actor::IsSneaking() && !abIgnoreCrouch;
        if (bIsSneaking) {
            fheight = ptargetActor->GetSneakHeight(false);
        }
        else {
            fheight = ptargetActor->GetHeight();
        }
        if (ptargetActor->IsKnockedOut() || (ptargetActor->IsSleeping() || ptargetActor->IsDead(false))) {
            return fheight * 0.10000000149011612;
        }

        switch (aeTargetSegmentInView) {
            case ACTOR_SEGMENT_INVIEW_BOTTOM: {
                float v9 = fheight * 0.25;
                v3 = v9;
                break;
            }
            case ACTOR_SEGMENT_INVIEW_MIDDLE:
            case ACTOR_SEGMENT_INVIEW_NONE: {
                float fResult = fheight * 0.5;
                v3 = fResult;
                break;
            }
            case ACTOR_SEGMENT_INVIEW_TOP: {
                double v4;
                if (bIsSneaking) {
                    v4 = fheight * 0.8999999761581421;
                }
                else {
                    v4 = fheight * 0.75;
                }
                float v11 = v4;
                v3 = v11;
                break;
            }
            default: {
                v3 = 0.0;
                break;
            }
        }
    }
    else {
        NiPoint3 kBoundMax = apTarget->GetBoundMax();
        double v8 = kBoundMax.z - apTarget->GetBoundMin().z;
        return apTarget->GetScale() * v8 * 0.5;
    }
    return v3;
}

void CombatUtilities::CalculateCoverPositions(
    CombatController* apController,
    PathingCoverLocation* apCoverLocation,
    CoverReservationInfo* apCoverData,
    NiPoint3* apCoverPosition,
    NiPoint3* apFirePosition
) {
    Actor* Attacker = apController->GetAttacker();
    float afActorRadius = Attacker->GetRadius();
    CombatUtilities::CalculateCoverPositions_1(
        afActorRadius, apCoverLocation, apCoverData, apCoverPosition, apFirePosition
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
    TESObjectREFR* phitRef;
    uint32_t v9;
    CFilter icolFilter;
    int32_t v22;
    int32_t* v17 = &v22;
    CombatAI_LOS.Start();
    COL_LAYER ecollilsionLayer = L_LOS;
    if (aiCollisionLayer != -1) {
        ecollilsionLayer = static_cast<COL_LAYER>(aiCollisionLayer);
    }
    NiPoint3 attackerLocation = arAttackerLocation;
    attackerLocation.z = CombatUtilities::GetAttackerFiringZOffset(apAttacker, true) + attackerLocation.z;
    alignas(16) NiPoint3 targetLocation = arTargetLocation;
    if (apTarget) {
        targetLocation.z =
            CombatUtilities::GetTargetFiringZOffset(apTarget, ACTOR_SEGMENT_INVIEW_TOP, true) + targetLocation.z;
    }
    bhkPickData pickData;
    apAttacker->GetCollisionFilter(&icolFilter.iFilter);
    if ((apTarget) && (apTarget->IsMobileObject())) {
        MobileObject* pmobileTarget = static_cast<MobileObject*>(apTarget);
        uint32_t uiTargetFilter;
        icolFilter.iFilter = *pmobileTarget->MobileObject::GetCollisionFilter(&uiTargetFilter);
    }

    SpecificItemCollector collector(ecollilsionLayer, icolFilter);
    pickData.SetClosestCollector(&collector);
    icolFilter.iFilter = *apAttacker->GetCollisionFilter(&v9);
    icolFilter.SetLayer(ecollilsionLayer);
    pickData.SetFilter(icolFilter);
    pickData.NiSetFrom(attackerLocation);
    pickData.NiSetTo(targetLocation);
    NiAVObject* pobjHit = apAttacker->GetParentCell()->Pick(pickData);
    if (!pobjHit || ((phitRef = TESObjectREFR::FindReferenceFor3D(pobjHit), apTarget) && phitRef == apTarget)) {
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
    return (afDeltaXY / (NiCos(afAngle) * afSpeed));
}

bool CombatUtilities::CalculateAttackerCombatPositions(Actor* apTarget, Actor* apAttacker) {
    bool bresult = false;
    if (!apTarget) {
        return bresult;
    }
    CombatTargetData* pattackerArray = pCombatManager->pTargetMap->GetTargetAttackerArray(apTarget);
    if (pattackerArray) {
        MeleeAttackerData attackerData[64];
        NiPoint3 targetLocation = *apTarget->QLocationOnReference();
        rt::eh_vector_constructor_iterator(
            attackerData, sizeof(MeleeAttackerData), 64, &fnvd::constructElement<WORLD_LOCATION>
        );
        ourMemset(attackerData, 0, sizeof(attackerData));
        uint32_t imeleeAttackers = 0;
        uint32_t imeleeAttackersInMelee = 0;
        uint32_t ireservedSlots = 0;
        uint32_t inumAttackers = pattackerArray->Attackers.iSize;
        uint32_t inumAttackers_2 = BSMIN(inumAttackers, 64u);
        for (uint32_t i = 0; i < inumAttackers_2; ++i) {
            CombatController* pcombatController;
            Actor* pattacker = pattackerArray->Attackers.operator[](i);
            if (pattacker) {
                pcombatController = pattacker->GetCombatController();
            }
            else {
                pcombatController = nullptr;
            }
            CombatController* pcombatController_2 = pcombatController;
            if ((pcombatController) &&
                (pcombatController_2->CombatPlanContainsAction(7) ||
                 pcombatController_2->CombatPlanContainsAction(8))) {
                attackerData[imeleeAttackers].pCombatController = pcombatController_2;
                NiPoint3* v4 = pattacker->QLocationOnReference();
                attackerData[imeleeAttackers].Location = *v4;
                double ZAngleFromVector = GetZAngleFromVector(attackerData[imeleeAttackers].Location - targetLocation);
                attackerData[imeleeAttackers].fAngle = ZAngleFromVector * 57.2957763671875;
                char SlotFromAngle = GetSlotFromAngle(attackerData[imeleeAttackers].fAngle);
                attackerData[imeleeAttackers].cSlot = SlotFromAngle;
                if (pcombatController_2->pMovementProcedure) {
                    CombatProcedure* pCombatProcedure = pcombatController_2->pMovementProcedure;
                    if (pCombatProcedure->GetType() == CombatProcedure::ATTACK_MELEE) {
                        attackerData[imeleeAttackers].bInMelee = true;
                        ++imeleeAttackersInMelee;
                    }
                }
                ++imeleeAttackers;
                if (pattacker == apAttacker) {
                    bresult = true;
                }
            }
        }
        if (imeleeAttackers) {
            if (imeleeAttackersInMelee) {
                for (uint32_t i_2 = 0; i_2 < imeleeAttackers; ++i_2) {
                    if (attackerData[i_2].bInMelee) {
                        char cfoundSlot = FindSlot(ireservedSlots, attackerData[i_2].cSlot);
                        if (cfoundSlot != -1) {
                            ireservedSlots = ReserveSlot(ireservedSlots, cfoundSlot);
                            float fangle = attackerData[i_2].fAngle;
                            if (cfoundSlot != attackerData[i_2].cSlot) {
                                fangle = GetAngleFromSlot(cfoundSlot);
                            }
                            attackerData[i_2].pCombatController->SetCombatPosition(fangle);
                        }
                    }
                }
            }
            for (uint32_t i_3 = 0; i_3 < imeleeAttackers; ++i_3) {
                if (!attackerData[i_3].bInMelee) {
                    char cfoundSlot_2 = FindSlot(ireservedSlots, attackerData[i_3].cSlot);
                    if (cfoundSlot_2 != -1) {
                        ireservedSlots = ReserveSlot(ireservedSlots, cfoundSlot_2);
                        float fangle_2 = attackerData[i_3].fAngle;
                        if (cfoundSlot_2 != attackerData[i_3].cSlot) {
                            fangle_2 = GetAngleFromSlot(cfoundSlot_2);
                        }
                        attackerData[i_3].pCombatController->SetCombatPosition(fangle_2);
                    }
                }
            }
        }
    }
    return bresult;
}

float CombatUtilities::GetAngleForMeleeAttack(const ANIM_GROUP_ENUM eAttack) {
    double v1;
    if (eAttack < ANIM_GROUP_ATTACK_NORMAL_POWER || eAttack > ANIM_GROUP_POWER_ATTACK_STOP) {
        return fCombatHitConeAngle.Float();
    }

    switch (eAttack) {
        case ANIM_GROUP_ATTACK_NORMAL_POWER:
        case ANIM_GROUP_ATTACK_FORWARD_POWER: {
            v1 = fCombatOverheadHitConeAngle.Float();
            break;
        }
        case ANIM_GROUP_ATTACK_BACK_POWER:
        case ANIM_GROUP_ATTACK_LEFT_POWER:
        case ANIM_GROUP_ATTACK_RIGHT_POWER: {
            v1 = fCombatSweepHitConeAngle.Float();
            break;
        }
        case ANIM_GROUP_ATTACK_CUSTOM_1_POWER: {
            v1 = fCombatUppercutHitConeAngle.Float();
            break;
        }
        default: {
            return fCombatHitConeAngle.Float();
        }
    }

    return v1;
}

char CombatUtilities::CalculateGrenadeThrow_(
    Actor* apActor,
    Actor* apTarget,
    BGSProjectile* apProjectile,
    NiPoint3* a4,
    bool* isHighArc,
    NiPoint3* targetPointOut,
    float grenadeThrowHitFractionThreshold
) {
    NiPoint3 result;
    NiPoint3 ecx0;
    NiPoint3 v11;
    if (!apActor || !apTarget) {
        return 0;
    }
    if (a4) {
        ecx0 = *a4;
    }
    else {
        NiPoint3 v8;
        ecx0 = (v8 = CombatUtilities::GetAttackerFiringLocation(apActor));
    }
    result = CombatUtilities::GetTargetFiringLocation(apTarget, ACTOR_SEGMENT_INVIEW_TOP);
    v11 = CombatUtilities::GetTargetVelocity(apTarget);
    return CombatUtilities::CheckProjectileArc(
        apActor, apTarget, apProjectile, ecx0, result, v11, *isHighArc, *targetPointOut,
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
    float fgravity = apProjectile->GetProjectileBase()->GetGravity() * CombatUtilities::fWorldGravity;
    return CombatUtilities::FindAngleOfProjectile(fHDistance, fVDistance, afSpeed, fgravity, abHighArc);
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
    NiPoint3 result;
    result = aStartPoint - aTargetedPoint;
    float fSpeed = afSpeed;
    if (afSpeed == 0.0) {
        fSpeed = apProjectile->GetSpeed();
    }
    if (apProjectile->GetGravity() == 0.0) {
        double v7 = result.Length();
        return static_cast<float>(v7 / afSpeed);
    }

    if (abHighArc && afSpeed == 0.0) {
        float* v5 = &gs_fGrenadeHighArcSpeedPercentage.Float();
        afSpeed = afSpeed * *v5;
    }
    float afDeltaZ = result.z;
    float fDeltaXY = result.XYLength();
    float afGravity = apProjectile->GetGravity() * CombatUtilities::fWorldGravity;
    float fAngle = CombatUtilities::FindAngleOfProjectile(fDeltaXY, afDeltaZ, afSpeed, afGravity, false);
    return CombatUtilities::ComputeProjectileTime(fDeltaXY, fAngle, fSpeed);
}

NiPoint3* CombatUtilities::GetAttackerFiringLocationSimple(NiPoint3* apResult, TESObjectREFR* apAttacker) {
    NiPoint3 v3 = apAttacker->GetLocationOnReference();
    v3.z = CombatUtilities::GetAttackerFiringZOffset(apAttacker, false) + v3.z;
    *apResult = v3;
    return apResult;
}

NiPoint3* CombatUtilities::GetBackProjectedAttackerFiringLocation(NiPoint3* apResult, TESObjectREFR* apAttacker) {
    NiPoint3 result;
    result = CombatUtilities::GetAttackerFiringLocation(apAttacker);
    if (apAttacker->IsActor()) {
        MobileObject* v23 = static_cast<MobileObject*>(apAttacker);
        NiPoint3 v12 = apAttacker->GetLocationOnReference();
        float v8 = v23->GetHeading(false);
        float sinOut = 0.0;
        float cosOut = 0.0;
        float v7 = v8 + 1.5707963705062866;
        NiSinCos(v7, sinOut, cosOut);
        NiPoint2 pt(sinOut, cosOut);
        float y = result.y - v12.y;
        float x = result.x - v12.x;
        NiPoint2 v15(x, y);
        float v22 = v15.Dot(pt);
        float ecx0 = pt.x * v22 + v12.x;
        float v19 = pt.y * v22 + v12.y;
        float Looking = v23->GetLooking();
        float fSin = 0.0;
        float fCos = 0.0;
        NiSinCos(Looking, fSin, fCos);
        float v4 = result.y - v19;
        float v3 = result.x - ecx0;
        NiPoint2 v20(v3, v4);
        float v9 = v20.Length();
        float v14 = v9 * fSin / fCos;
        result.x = ecx0;
        result.y = v19;
        result.z = result.z + v14;
    }
    *apResult = result;
    return apResult;
}

NiPoint3* CombatUtilities::GetAngleToProjectedPoint(
    NiPoint3* apResult,
    NiPoint3 aStartLocation,
    NiPoint3 aPivotLocation,
    float afPivotAngle,
    NiPoint3 aProjectedPoint
) {
    if (aPivotLocation.x == aStartLocation.x && aPivotLocation.y == aStartLocation.y) {
        ::new (apResult) NiPoint3(CombatUtilities::GetAngleToProjectedPoint(aStartLocation, aProjectedPoint));
        return apResult;
    }

    NiPoint3 result = aProjectedPoint - aPivotLocation;
    float fValue = result.SqrLength();
    if (fValue == 0.0) {
        ::new (apResult) NiPoint3(CombatUtilities::GetAngleToProjectedPoint(aStartLocation, aProjectedPoint));
        return apResult;
    }

    if (fValue < f1024_0 * f1024_0) {
        float v10 = NiSqrt(fValue);
        float v7 = f1024_0 / v10;
        aProjectedPoint = aPivotLocation + result * v7;
    }
    ::new (apResult) NiPoint3(CombatUtilities::GetAngleToProjectedPoint(aStartLocation, aProjectedPoint));
    return apResult;
}

void CombatUtilities::CalculateCoverPositions(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation,
    NiPoint3* apCoverPosition,
    NiPoint3* apFirePosition
) {
    CombatUtilities::CalculateCoverPositions(
        apCombatController, &apCombatCoverLocation->CoverLocation, &apCombatCoverLocation->ReservationInfo,
        apCoverPosition, apFirePosition
    );
}

void CombatUtilities::CalculateCoverPositions_1(
    float afActorRadius,
    PathingCoverLocation* apCoverLocation,
    CoverReservationInfo* apReservationInfo,
    NiPoint3* apCoverPosition,
    NiPoint3* apFirePosition
) {
    NiPoint3 v16;
    NiPoint3 v17;
    NiPoint3 v18;
    NiPoint3 v19;
    NiPoint3 v20;
    NiPoint3 v21;
    NiPoint3 v22;
    NiPoint3 v23;
    NiPoint3 v24;
    if (apCoverPosition || apFirePosition) {
        NiPoint3 v27;
        NiPoint3 result;
        NiPoint3 v31;
        float v29 = afActorRadius + 16.0;
        float v30 = 64.0f;
        float v25 = 128.0f;
        float fScalar = -v29;
        result = apCoverLocation->EdgeDirection * fScalar;
        v31 = apCoverLocation->EdgeDirection.Cross(NiPoint3_UNIT_Z);
        float Offset = apReservationInfo->GetOffset();
        NiPoint3* v5 = &(v24 = v31 * Offset);
        NiPoint3* v6 = &(v23 = apCoverLocation->LeftVertex + *v5);
        v27 = *v6 + result;
        if (apCoverLocation->IsLedgeCover()) {
            if (apFirePosition) {
                *apFirePosition = v27;
            }
            if (apCoverPosition) {
                NiPoint3 v15;
                float fScalara = -v25;
                NiPoint3* v12 = &(v16 = apCoverLocation->EdgeDirection * fScalara);
                *apCoverPosition = (v15 = v27 + *v12);
            }
        }
        else {
            if (apCoverPosition) {
                *apCoverPosition = v27;
            }
            if (apFirePosition) {
                if (apCoverLocation->IsStandCover() || apReservationInfo->bHideCover) {
                    NiPoint3* v9;
                    if (apReservationInfo->bLeftEdge) {
                        NiPoint3* v7 = &(v22 = v31 * v30);
                        NiPoint3* v8 = &(v21 = apCoverLocation->LeftVertex - *v7);
                        v9 = &(v20 = *v8 + result);
                    }
                    else {
                        NiPoint3* v10 = &(v19 = v31 * v30);
                        NiPoint3* v11 = &(v18 = apCoverLocation->RightVertex + *v10);
                        v9 = &(v17 = *v11 + result);
                    }
                    *apFirePosition = *v9;
                }
                else {
                    *apFirePosition = v27;
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
    float afBaseExplosionDamage = apExplosion->GetDamage();
    if (fDangerousProjectileExplosionDamage.Float() > static_cast<double>(afBaseExplosionDamage)) {
        return false;
    }
    float afExplosionRadius = apExplosion->GetRadiusBSUnits();
    float* v2 = &fDangerousProjectileExplosionDamage.Float();
    float v5 = CombatFormulas::CalcExplosionDamageDistance(afExplosionRadius, afBaseExplosionDamage, *v2);
    return !(fDangerousProjectileExplosionRadius.Float() > static_cast<double>(v5));
}

NiPoint3* CombatUtilities::GetProjectedPointForTarget(
    NiPoint3* arResult,
    Actor* apSource,
    const NiPoint3& akTargetLocation,
    NiPoint3* arSourcePosition,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    bool abHighArc
) {
    CombatUtilities::GetProjectedPointForTarget(
        arResult, apSource, akTargetLocation, NiPoint3_ZERO, arSourcePosition, apProjectile, afProjectileSpeed,
        abHighArc
    );
    return arResult;
}

NiPoint3* CombatUtilities::GetProjectedPointForTarget(
    NiPoint3* arResult,
    TESObjectREFR* apSource,
    NiPoint3 akTargetLocation,
    NiPoint3 akTargetVelocity,
    NiPoint3* arSourcePosition,
    BGSProjectile* apProjectile,
    float afProjectileSpeed,
    bool abHighArc
) {
    if (apProjectile->GetHitScan()) {
        *arResult = akTargetLocation;
        return arResult;
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
    ::new (arResult) NiPoint3(
        CombatUtilities::GetProjectedPointForTarget(
            releasePoint, akTargetLocation, akTargetVelocity, afProjectileSpeed, fGravity, abHighArc
        )
    );
    return arResult;
}

// The Havok world's gravity, or zero without a world. The world is looked up before the read mark.
const hkVector4& bhkWorld::GetGravity() {
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    MarkForRead();
    const hkVector4& kGravity = pw ? pw->getGravity() : hkVector4::getZero();
    UnMarkForRead();
    return kGravity;
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
