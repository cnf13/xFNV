#include "Fallout/AI/Combat/Projectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionBox.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyT.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSBlastNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/BeamProjectile.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/ContinuousBeamProjectile.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/FlameProjectile.h"
#include "Fallout/AI/Combat/GrenadeProjectile.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Fallout/AI/Combat/MuzzleFlash.h"
#include "Fallout/AI/Combat/ProjectileListener.h"
#include "Fallout/AI/Combat/ProjectileTarget.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MiddleLowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"

#include <cmath>
#include <cstring>
#include <limits>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bProjectileDebug("bProjectileDebug:Combat", false);
SettingT<INISettingCollection> fProjectileDebugDuration("fProjectileDebugDuration:Combat", 5.0f);
SettingT<INISettingCollection> bVATSProjectileDebug("bVATSProjectileDebug:Combat", false);

// kNormal is deliberately left unset: it is compared with NiPoint3::ZERO in place of being stored.
Projectile::ImpactData::ImpactData()
  : pCollidee(nullptr), kLocation(NiPoint3::ZERO), pTargetWorldObject(nullptr), eMaterial(BHK_MATERIAL_MAX),
    eDamageLimb(BGSBodyPart::LIMB_NONE), bProcessed(false), bDoDamage(true), sTargetWorldObjectCount(0),
    sTargetWorldObjectIndex(0) {}

Projectile::ImpactData::~ImpactData() {
    if (pTargetWorldObject) {
        pTargetWorldObject->removeReference();
    }
    pTargetWorldObject = nullptr;
}

void Projectile::ImpactData::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (sTargetWorldObjectCount && sTargetWorldObjectIndex != -1 && pCollidee) {
        int16_t sCurrentWorldObjectCount = GetHavokWorldObjectCountInSceneGraph(pCollidee->Get3D());
        if (sTargetWorldObjectCount == sCurrentWorldObjectCount) {
            if (sTargetWorldObjectIndex != -1) {
                int32_t iTargetWorldObjectIndex = sTargetWorldObjectIndex;
                NiNode* p3D = pCollidee->Get3D();
                pTargetWorldObject = GetHavokWorldObjectByIndexInSceneGraph(p3D, iTargetWorldObjectIndex);
                if (pTargetWorldObject) {
                    pTargetWorldObject->addReference();
                }
            }
        }
        else {
            int32_t iTargetWorldObjectCount = sTargetWorldObjectCount;
            int32_t iFormID = pCollidee->iFormID;
            SaveGameWarning(
                "AI: World object count has changed on object %s %08 from %i to %i",
                pCollidee->GetFormEditorID(),
                iFormID,
                iTargetWorldObjectCount,
                sCurrentWorldObjectCount
            );
        }
    }
}

void Projectile::ImpactData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&kLocation, sizeof(kLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&kNormal, sizeof(kNormal), 0);
    apSaveGameBuffer->SaveDataEndian(&eMaterial, sizeof(eMaterial), 0);
    apSaveGameBuffer->SaveDataEndian(&eDamageLimb, sizeof(eDamageLimb), 0);
    apSaveGameBuffer->SaveDataEndian(&bProcessed, sizeof(bProcessed), 0);
    sTargetWorldObjectCount = 0;
    sTargetWorldObjectIndex = 0;
    if (pCollidee && pTargetWorldObject) {
        sTargetWorldObjectCount = GetHavokWorldObjectCountInSceneGraph(pCollidee->Get3D());
        hkpWorldObject* pWorldObject = pTargetWorldObject;
        NiNode* p3D = pCollidee->Get3D();
        sTargetWorldObjectIndex = GetHavokWorldObjectIndexInSceneGraph(p3D, pWorldObject);
    }

    apSaveGameBuffer->SaveDataEndian(&sTargetWorldObjectCount, sizeof(sTargetWorldObjectCount), 0);
    apSaveGameBuffer->SaveDataEndian(&sTargetWorldObjectIndex, sizeof(sTargetWorldObjectIndex), 0);
    apSaveGameBuffer->SaveFormID(pCollidee, 0);
}

void Projectile::ImpactData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&kLocation, sizeof(kLocation));
    apLoadGameBuffer->LoadData(&kNormal, sizeof(kNormal));
    apLoadGameBuffer->LoadData(&eMaterial, sizeof(eMaterial));
    apLoadGameBuffer->LoadData(&eDamageLimb, sizeof(eDamageLimb));
    apLoadGameBuffer->LoadData(&bProcessed, sizeof(bProcessed));
    if (apLoadGameBuffer->GetVersion() >= 17u) {
        apLoadGameBuffer->LoadData(&sTargetWorldObjectCount, sizeof(sTargetWorldObjectCount));
        apLoadGameBuffer->LoadData(&sTargetWorldObjectIndex, sizeof(sTargetWorldObjectIndex));
    }
    apLoadGameBuffer->LoadFormID(pCollidee);
}

void Projectile::ImpactData::InitLoadGame([[maybe_unused]] BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef;
    if (pCollidee) {
        // The collidee field holds the reference's form ID until it is resolved here (LoadGame).
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCollidee)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    else {
        pRef = nullptr;
    }
    pCollidee = pRef;
    if (pCollidee) {
        pCollidee->SetTargeted(true);
    }
}

void Projectile::ImpactData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

Projectile::Projectile(
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle
)
  : bImpactProcessed(false), fPower(1.0f), fSpeedMult(1.0f), fRange(0.0f), fDamage(0.0f), fAlpha(1.0f),
    fExplosionTimer(std::numeric_limits<float>::max()), fBlinkTimer(0.0f), fWeaponCondition(1.0f),
    pWeaponSource(nullptr), pShooter(apShooter), pDesiredTarget(nullptr), spLight(nullptr), spPickCache(nullptr),
    spActorCause(nullptr), pTarget(nullptr), pSecondaryItemChange(nullptr), bSoundFired(false), fSoundDist(10000.0f) {
    pCurrentProcess = nullptr;
    SetLocationOnReference(aOrigin);
    SetAngleOnReferenceZ(afZAngle);
    SetAngleOnReferenceX(afXAngle);
    SetAngleOnReferenceY(0.0f);
    SetObjectReference(apProjectileBase);
    if (apFromWeapon) {
        ItemChange* pWeaponItem = nullptr;
        bool bSplitBeam = false;
        pWeaponSource = apFromWeapon;
        fDamage = GetWeaponSource()->GetAttackDamage();
        Actor* pActor = nullptr;
        if (GetShooter() && GetShooter()->IsActor()) {
            pActor = static_cast<Actor*>(GetShooter());
            if (pActor->GetCurrentAIProcess()) {
                pWeaponItem = pActor->GetCurrentAIProcess()->GetCurrentWeapon();
                if (pWeaponItem) {
                    double dItemHealth = pWeaponItem->GetItemHealth(false);
                    fWeaponCondition = dItemHealth /
                        static_cast<double>(TESHealthForm::GetFormHealth(pWeaponItem->GetContainerObject()));
                    fDamage = pWeaponItem->GetModifiedAttackDamage(static_cast<ActorValueOwner*>(pActor), 1.0f, true);
                    bSplitBeam = pWeaponItem->HasModEffectActive(WEAPON_MOD_SPLIT_BEAM);
                }
            }
        }
        uint8_t cNumProjectiles = GetWeaponSource()->GetNumProjectiles(bSplitBeam, false, pActor);
        fDamage = fDamage / cNumProjectiles;
        if (bSplitBeam) {
            fDamage = fDamage * 1.3;
        }
    }
    if (GetShooter() == pPlayer && GetWeaponSource() && GetWeaponSource()->IsRockItLauncher()) {
        ItemChange* pRockItItem = pPlayer->GetRandomRockItAmmoItem();
        if (pRockItItem) {
            SetSecondaryItemChange(pRockItItem);
            if (pRockItItem->GetContainerObject()) {
                bool bIsHardcore = pPlayer->IsHardcore();
                float fWeight = TESWeightForm::GetFormWeight(pRockItItem->GetContainerObject(), bIsHardcore);
                float fBonus = BSMAX(fRockitDamageBonusWeightMin.Float(), fWeight) *
                    fRockitDamageBonusWeightMult.Float();
                fDamage = BSMAX(0.0f, fBonus) + fDamage;
            }
        }
    }
    iFlags = 0;
    fAge = 0.0f;
    fDistanceMoved = 0.0f;
    MovementDirection = NiPoint3::ZERO;
    spLight = nullptr;
    fDecalSize = 0.0f;
    kFollowOffset.MakeIdentity();
    if (pShooter && pShooter->IsActor() && static_cast<Actor*>(pShooter)->IsInCombat()) {
        SetFlag(SHOOTER_IN_COMBAT, true);
    }
    SetInitialized(false);
}

// The constructor ignores abAdd.
Projectile::Projectile([[maybe_unused]] bool abAdd)
  : bImpactProcessed(false), iFlags(0), fPower(1.0f), fSpeedMult(1.0f), fRange(0.0f), fAge(0.0f), fDamage(0.0f),
    fAlpha(1.0f), fExplosionTimer(std::numeric_limits<float>::max()), fBlinkTimer(0.0f), fSpreadHeading(0.0f),
    fSpreadLooking(0.0f), fWeaponCondition(1.0f), pWeaponSource(nullptr), pShooter(nullptr), pDesiredTarget(nullptr),
    MovementDirection(NiPoint3::ZERO), fDistanceMoved(0.0f), spLight(nullptr), bSendAlarm(false), spPickCache(nullptr),
    spActorCause(nullptr), fDecalSize(0.0f), pTarget(nullptr), pSecondaryItemChange(nullptr), bSoundFired(false),
    fSoundDist(10000.0f) {
    pCurrentProcess = nullptr;
    kFollowOffset.MakeIdentity();
    SetInitialized(false);
}

bool Projectile::Load(TESFile* apFile) {
    return TESObjectREFR::Load(apFile);
}

void Projectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    bool bHasSecondaryItemChange;
    int32_t iActorCauseID;
    MobileObject::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&iFlags, sizeof(iFlags), 0);
    apSaveGameBuffer->SaveDataEndian(&fPower, sizeof(fPower), 0);
    apSaveGameBuffer->SaveDataEndian(&fRange, sizeof(fRange), 0);
    apSaveGameBuffer->SaveDataEndian(&fSpeedMult, sizeof(fSpeedMult), 0);
    apSaveGameBuffer->SaveDataEndian(&fAge, sizeof(fAge), 0);
    apSaveGameBuffer->SaveDataEndian(&fDamage, sizeof(fDamage), 0);
    apSaveGameBuffer->SaveDataEndian(&fAlpha, sizeof(fAlpha), 0);
    apSaveGameBuffer->SaveDataEndian(&fExplosionTimer, sizeof(fExplosionTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&fBlinkTimer, sizeof(fBlinkTimer), 0);
    apSaveGameBuffer->SaveFormID(pWeaponSource, 0);
    apSaveGameBuffer->SaveFormID(pShooter, 0);
    apSaveGameBuffer->SaveFormID(pDesiredTarget, 0);
    apSaveGameBuffer->SaveDataEndian(&MovementDirection, sizeof(MovementDirection), 0);
    apSaveGameBuffer->SaveDataEndian(&fDistanceMoved, sizeof(fDistanceMoved), 0);
    apSaveGameBuffer->SaveDataEndian(&bSendAlarm, sizeof(bSendAlarm), 0);
    apSaveGameBuffer->SaveDataEndian(&fDecalSize, sizeof(fDecalSize), 0);
    apSaveGameBuffer->SaveMatrixAsQuaternion(&kFollowOffset.m_Rotate, 36, 0);
    apSaveGameBuffer->SaveDataEndian(&kFollowOffset.m_Translate, sizeof(kFollowOffset.m_Translate), 0);
    apSaveGameBuffer->SaveDataEndian(&kFollowOffset.m_fScale, sizeof(kFollowOffset.m_fScale), 0);
    apSaveGameBuffer->SaveDataEndian(&fSpreadHeading, sizeof(fSpreadHeading), 0);
    apSaveGameBuffer->SaveDataEndian(&fSpreadLooking, sizeof(fSpreadLooking), 0);
    apSaveGameBuffer->SaveDataEndian(&fWeaponCondition, sizeof(fWeaponCondition), 0);
    if (spActorCause.m_pObject) {
        iActorCauseID = spActorCause->iActorCauseID;
    }
    else {
        iActorCauseID = -1;
    }
    apSaveGameBuffer->SaveDataEndian(&iActorCauseID, sizeof(iActorCauseID), 0);
    bHasSecondaryItemChange = pSecondaryItemChange != nullptr;
    apSaveGameBuffer->SaveDataEndian(&bHasSecondaryItemChange, sizeof(bHasSecondaryItemChange), 0);
    if (bHasSecondaryItemChange) {
        pSecondaryItemChange->SaveGame(apSaveGameBuffer);
    }
    uint32_t uiSaveGameCount = 0;
    uint32_t uiSaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (Projectile::ImpactData* pImpact : ImpactList) {
        if (pImpact) {
            pImpact->SaveGame(apSaveGameBuffer);
            ++uiSaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount, uiSaveGameBufferPos);
    apSaveGameBuffer->SaveDataEndian(&bImpactProcessed, sizeof(bImpactProcessed), 0);
}

void Projectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool bHasSecondaryItemChange;
    int32_t iActorCauseID;
    MobileObject::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&iFlags, sizeof(iFlags));
    apLoadGameBuffer->LoadData(&fPower, sizeof(fPower));
    apLoadGameBuffer->LoadData(&fRange, sizeof(fRange));
    apLoadGameBuffer->LoadData(&fSpeedMult, sizeof(fSpeedMult));
    apLoadGameBuffer->LoadData(&fAge, sizeof(fAge));
    apLoadGameBuffer->LoadData(&fDamage, sizeof(fDamage));
    apLoadGameBuffer->LoadData(&fAlpha, sizeof(fAlpha));
    apLoadGameBuffer->LoadData(&fExplosionTimer, sizeof(fExplosionTimer));
    apLoadGameBuffer->LoadData(&fBlinkTimer, sizeof(fBlinkTimer));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pWeaponSource = dynamic_cast<TESObjectWEAP*>(pForm);
    apLoadGameBuffer->LoadFormID(pShooter);
    apLoadGameBuffer->LoadFormID(pDesiredTarget);
    apLoadGameBuffer->LoadData(&MovementDirection, sizeof(MovementDirection));
    apLoadGameBuffer->LoadData(&fDistanceMoved, sizeof(fDistanceMoved));
    apLoadGameBuffer->LoadData(&bSendAlarm, sizeof(bSendAlarm));
    apLoadGameBuffer->LoadData(&fDecalSize, sizeof(fDecalSize));
    apLoadGameBuffer->LoadMatrixAsQuaternion(&kFollowOffset.m_Rotate, 36);
    if (apLoadGameBuffer->GetVersion() >= 13u) {
        apLoadGameBuffer->LoadData(&kFollowOffset.m_Translate, sizeof(kFollowOffset.m_Translate));
        apLoadGameBuffer->LoadData(&kFollowOffset.m_fScale, sizeof(kFollowOffset.m_fScale));
    }
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        apLoadGameBuffer->LoadData(&fSpreadHeading, sizeof(fSpreadHeading));
        apLoadGameBuffer->LoadData(&fSpreadLooking, sizeof(fSpreadLooking));
    }
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        apLoadGameBuffer->LoadData(&fWeaponCondition, sizeof(fWeaponCondition));
    }
    iActorCauseID = -1;
    apLoadGameBuffer->LoadData(&iActorCauseID, sizeof(iActorCauseID));
    spActorCause = ActorCause::GetActorCauseByID(iActorCauseID);
    if (apLoadGameBuffer->GetVersion() >= 3u) {
        bHasSecondaryItemChange = false;
        apLoadGameBuffer->LoadData(&bHasSecondaryItemChange, sizeof(bHasSecondaryItemChange));
        if (bHasSecondaryItemChange) {
            ItemChange* pItemChange;
            pItemChange = new ItemChange;
            pSecondaryItemChange = pItemChange;
            pSecondaryItemChange->LoadGame(apLoadGameBuffer);
            if (!pSecondaryItemChange->pContainerObj) {
                pSecondaryItemChange->DeleteAllExtra();
                if (pSecondaryItemChange) {
                    delete pSecondaryItemChange;
                }
                pSecondaryItemChange = nullptr;
            }
        }
    }
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            Projectile::ImpactData* pImpact = new Projectile::ImpactData;
            pImpact->LoadGame(apLoadGameBuffer);
            if (!Get3D()) {
                pImpact->bProcessed = false;
            }
            ImpactList.AddTail(pImpact);
        }
    }
    if (apLoadGameBuffer->GetVersion() >= 16u) {
        apLoadGameBuffer->LoadData(&bImpactProcessed, sizeof(bImpactProcessed));
    }
}

void Projectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::InitLoadGame(apLoadGameBuffer);
    TESObjectREFR* pShooterRef = nullptr;
    if (pShooter) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pShooter)));
        pShooterRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pShooter = pShooterRef;
    TESObjectREFR* pDesiredTargetRef = nullptr;
    if (pDesiredTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget))
        );
        pDesiredTargetRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pDesiredTarget = pDesiredTargetRef;
    if (pSecondaryItemChange) {
        pSecondaryItemChange->InitLoadGame(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        for (Projectile::ImpactData* pImpact : ImpactList) {
            if (pImpact) {
                pImpact->InitLoadGame(apLoadGameBuffer);
            }
        }
    }
}

void Projectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::Revert(apLoadGameBuffer);
    while (!ImpactList.IsEmpty()) {
        Projectile::ImpactData* pImpact = ImpactList.m_item;
        if (pImpact) {
            delete pImpact;
        }
        ImpactList.RemoveHead();
    }
    iFlags = 0;
    fPower = 1.0f;
    fSpeedMult = 1.0f;
    fRange = 0.0f;
    fAge = 0.0f;
    fDamage = 0.0f;
    fAlpha = 1.0f;
    fDistanceMoved = 0.0f;
    MovementDirection = NiPoint3::ZERO;
    pWeaponSource = nullptr;
    pShooter = nullptr;
    pDesiredTarget = nullptr;
    spLight = static_cast<NiLight*>(nullptr);
    spPickCache = static_cast<ProjectilePickCache*>(nullptr);
    spActorCause = static_cast<ActorCause*>(nullptr);
    bSendAlarm = false;
    fDecalSize = 0.0f;
    fExplosionTimer = std::numeric_limits<float>::max();
    fBlinkTimer = 0.0f;
    fWeaponCondition = 1.0f;
    bImpactProcessed = false;
    kFollowOffset.MakeIdentity();
    pTarget = nullptr;
    if (pSecondaryItemChange) {
        delete pSecondaryItemChange;
    }
    pSecondaryItemChange = nullptr;
    if (apLoadGameBuffer->GetRevertOnly()) {
        Initialize();
    }
}

// From save version 14 on, finishes loading each impact.
void Projectile::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::FinishLoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 14) {
        for (BSSimpleList<ImpactData*>* pIter = &ImpactList; pIter;) {
            ImpactData* pImpact = pIter->GetItem();
            if (pImpact) {
                pImpact->FinishLoadGame(apLoadGameBuffer);
            }
            pIter = pIter->GetNext();
        }
    }
}

ActorCause* Projectile::GetActorCause() {
    return spActorCause.m_pObject;
}

void Projectile::InitHavok() {
    NiNode* p3d = Get3D();
    if (!p3d || !pCurrentProcess || pCurrentProcess->eLevel) {
        return;
    }
    TESObjectCELL* pcell = GetParentCell();
    bhkWorld* pw = pcell ? pcell->GetbhkWorld() : nullptr;
    if (!pw) {
        return;
    }
    if (IsFlagSet(Projectile::NO_COLLISION)) {
        bhkWorld::EnableCollision(p3d, false, true, false);
        bhkWorld::SetMotion(p3d, hkpMotion::MOTION_KEYFRAMED, true, false, true);
        return;
    }
    bhkNiCollisionObject* pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(p3d);
    bhkEntity* pRigid = nullptr;
    hkpRigidBody* hkRigid = nullptr;
    if (IsFlagSet(Projectile::NO_CHARCONTROLLER)) {
        // Get3D is called again here.
        bhkWorld::SetMotion(Get3D(), hkpMotion::MOTION_DYNAMIC, true, false, true);
        if (fGrenadeRestitution.Float() != 0.0) {
            if (GetProjectileBase()->Data.fBounceMultiplier > 0.0) {
                // The product is rounded to a float.
                float fRestitution = GetProjectileBase()->Data.fBounceMultiplier * fGrenadeRestitution.Float();
                bhkWorld::SetRestitution(p3d, fRestitution, true, false);
            }
            else {
                bhkWorld::SetRestitution(p3d, fGrenadeRestitution.Float(), true, false);
            }
        }
        if (fGrenadeFriction.Float() != 0.0) {
            bhkWorld::SetFriction(p3d, fGrenadeFriction.Float(), true, false);
        }
        bhkWorld::ChangeLayer(p3d, L_PROJECTILE, L_UNIDENTIFIED, true, false);
        if (GetShooter()) {
            TESObjectREFR* pShooterRef = GetShooter();
            if (pShooterRef->IsActor()) {
                MobileObject* pactor = static_cast<MobileObject*>(GetShooter());
                // GetCollisionFilter fills kFilter and returns its address, which GetGroup is called on.
                CFilter kFilter;
                pactor->GetCollisionFilter(&kFilter.iFilter);
                uint32_t uiGroup = kFilter.GetGroup();
                bhkWorld::SetGroup(p3d, uiGroup, true, false);
            }
            else {
                TESObjectREFR* pShooter3DRef = GetShooter();
                bhkCollisionObject* pShooterCol = FindFirstCollisionObject(pShooter3DRef->Get3D());
                bhkRigidBody* pobject = pShooterCol ? pShooterCol->GetRigidBody() : nullptr;
                if (pobject) {
                    CFilter kFilter;
                    uint32_t uiGroup = pobject->GetCollisionFilter(&kFilter)->GetGroup();
                    bhkWorld::SetGroup(p3d, uiGroup, true, false);
                }
            }
        }
        pw->AddObjects(p3d, true, true, 0, false);
        if (pcol) {
            pRigid = bhkEntity::Getbhk(pcol->GetWorldObject()->Gethk());
        }
        else if (p3d->IsFadeNode()) {
            BSFadeNode* pFade = static_cast<BSFadeNode*>(p3d);
            if (pFade->GetChildCount()) {
                for (uint32_t i = 0; i < pFade->GetChildCount(); ++i) {
                    NiAVObject* pchild = pFade->GetAt(i);
                    pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(pchild);
                    if (pcol) {
                        pRigid = bhkEntity::Getbhk(pcol->GetWorldObject()->Gethk());
                        break;
                    }
                }
            }
        }
        if (pRigid && GetProjectileBase()->GetRotates()) {
            hkRigid = static_cast<hkpRigidBody*>(pRigid->Gethk());
            // Reads the rotation's z, then y, then x.
            float fRotationZ = GetProjectileBase()->Data.fRotationZ;
            float fRotationY = GetProjectileBase()->Data.fRotationY;
            float fRotationX = GetProjectileBase()->Data.fRotationX;
            alignas(16) NiPoint3 pIn(fRotationX, fRotationY, fRotationZ);
            NiPoint3 pOut;
            hkVector4 extraTorque;
            NiMatrix3 mRot = Get3D()->GetWorldTransform().m_Rotate;
            alignas(16) NiPoint3 kTranslate;
            NiMatrix3::TransformVertices(mRot, *::new (&kTranslate) NiPoint3(0.0f, 0.0f, 0.0f), 1, &pIn, &pOut);
            extraTorque.set(pOut.x, pOut.y, pOut.z, 0.0f);
            hkRigid->setAngularVelocity(extraTorque);
        }
    }
    else {
        bhkCharacterController* pcon = GetCharController();
        if (!pcon) {
            bhkCharacterControllerCinfo info(true);
            info.SetWorld(pw);
            info.SetAcrobatics(1.0f);
            info.pObject = p3d;
            info.SetPosition(GetLocationOnReference());
            if (GetProjectileBase()->GetRotates()) {
                bhkSphereShape* pShape = new bhkSphereShape(1.0f, true);
                info.SetShape(pShape);
            }
            else {
                bhkSphereShape* pShape = new bhkSphereShape(0.1f, true);
                info.SetShape(pShape);
            }
            info.SetCollisionBox(false);
            info.SetAvoidBox(false);
            float fGravity = GetProjectileBase()->GetGravity();
            if (pShooter && pShooter->IsActor()) {
                Actor* pactor = static_cast<Actor*>(pShooter);
                // GetCollisionFilter fills kFilter and returns its address, which GetGroup is called on.
                CFilter kFilter;
                pactor->GetCollisionFilter(&kFilter.iFilter);
                uint32_t uiGroup = kFilter.GetGroup();
                info.SetColFilter(L_PROJECTILE, uiGroup);
                if (pactor->GetActorValue(ActorValue::TURBO) > 0) {
                    fGravity = 1.0 / BSTimer::QGlobalTimeMultiplier() * fGravity;
                }
            }
            info.SetGravity(fGravity);
            info.eStartState = HK_CHARACTER_USER_STATE_1;
            pcon = new ProjectileListener(&info, this);
            pcon->SetQuickSimulate(Main::GetSceneIsComplex());
            pcon->SetIsProjectile(true);
            hkVector4 startVelocity(-std::numeric_limits<float>::max(), 0.0f, 0.0f, 0.0f);
            pcon->SetLinearVelocity(startVelocity);
            // The NiPointer argument is built from pcon in the outgoing argument slot.
            pCurrentProcess->SetCharController(pcon);
            pcon->SetMOBObject(this);
            bhkWorld::EnableCollision(p3d, false, true, false);
        }
        if (pcon) {
            if (pcell) {
                pcon->SetWaterHeight(pcell->GetWaterHeight());
            }
            else {
                pcon->SetWaterHeight(-std::numeric_limits<float>::max());
            }
            bhkWorld* poldW = pcon->GetWorld();
            pcon->SetWorld(pw);
            NiAVObject* poldAV = pcon->GetNiObject();
            // This flag is set on every path, so the objects below are always added.
            bool bAdd = true;
            if (poldAV != p3d) {
                pcon->SetNiObject(p3d);
                if (pw && pw->QDebugDisplay()) {
                    pcon->BuildDisplayGeometry(nullptr);
                    BSShaderUtil::RecursiveSetPropertyWireframe(pcon->GetNiObject(), true);
                }
                bhkWorld::SetMotion(p3d, hkpMotion::MOTION_KEYFRAMED, true, false, true);
                bAdd = true;
            }
            NiPoint3 pos = GetLocationOnReference();
            if (poldW != pw) {
                if (poldW) {
                    bhkWorld::RemoveObjects(p3d, true, false);
                }
                pos.z = pos.z + 8.0;
                bAdd = true;
            }
            if (pw) {
                pcon->SetPosition(pos);
                if (bAdd) {
                    CFilter kFilter;
                    pcon->GetCollisionFilter(&kFilter);
                    uint32_t uiGroup = kFilter.GetGroup();
                    pw->AddObjects(p3d, true, false, uiGroup, false);
                }
                else {
                    bhkWorld::UpdatePosition(p3d, true, false);
                }
            }
        }
    }
}

void Projectile::MoveHavok(bool abForceRec) {
    bhkCharacterController* pcon = GetCharController();
    if (pcon || IsFlagSet(Projectile::NO_CHARCONTROLLER)) {
        bhkWorld* poldW;
        NiAVObject* pav = TESObjectREFR::Get3D();
        TESObjectCELL* pcell = GetParentCell();
        bhkWorld* pw = pcell ? pcell->GetbhkWorld() : nullptr;
        if (pcon) {
            poldW = pcon->GetWorld();
            if (pw != poldW) {
                float fwaterHeight;
                pcon->SetWantState(HK_CHARACTER_ON_GROUND);
                if (pcell) {
                    fwaterHeight = pcell->GetWaterHeight();
                }
                else {
                    fwaterHeight = 0.0f;
                }
                pcon->SetWaterHeight(fwaterHeight);
                pcon->SetPosition(*QLocationOnReference());
            }
            if (pcon->GetNiObject() != pav) {
                pcon->SetNiObject(pav);
            }
        }
        else {
            bhkCollisionObject* pcoll = FindFirstCollisionObject(Get3D());
            bhkWorldObject* pworldObject = pcoll ? pcoll->GetRigidBody() : nullptr;
            if (pworldObject) {
                poldW = bhkWorld::Getbhk(pworldObject->GethkWorld());
            }
            else {
                poldW = nullptr;
            }
        }
        bool bAdd = false;
        if (poldW != pw) {
            if (poldW) {
                bhkWorld::RemoveObjects(pav, true, false);
            }
            bAdd = true;
        }
        if (pw) {
            if (bAdd) {
                pw->AddObjects(pav, true, true, 0, false);
            }
            else {
                bhkWorld::UpdatePosition(pav, true, false);
            }
        }
    }
}

bool Projectile::IsProjectile() {
    return true;
}

bool Projectile::MovetoHigh() {
    HighProcess* pProcess;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    }
    pProcess = new HighProcess;
    pProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    MoveHavok(false);
    pCurrentProcess->SetLastSeenLocation();
    return true;
}

bool Projectile::MovetoLow() {
    LowProcess* pProcess;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    }
    pProcess = new LowProcess;
    pProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    return true;
}

bool Projectile::MovetoMiddleLow() {
    MiddleLowProcess* pProcess;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    }
    pProcess = new MiddleLowProcess;
    pProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_LOW, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    return true;
}

bool Projectile::MovetoMiddleHigh() {
    MiddleHighProcess* pProcess;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    }
    pProcess = new MiddleHighProcess;
    pProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    return true;
}

bool Projectile::ChangeProcessLevel() {
    int32_t iProcessLevel;
    if (pCurrentProcess) {
        iProcessLevel = pCurrentProcess->eLevel;
    }
    else {
        iProcessLevel = 3;
    }
    int32_t iNewProcessLevel = 3;
    bool bPermanent = IsPermanent();
    TESObjectCELL* pCell = GetParentCell();
    if (Get3D() || pModelLoader->IsReferenceQueued(this)) {
        if (TES::pInstance->IsCellLoaded(pCell, true)) {
            iNewProcessLevel = 0;
        }
        else if (bPermanent && TES::pInstance->IsCellLoaded(pCell, false)) {
            iNewProcessLevel = 1;
        }
    }
    if (iProcessLevel != iNewProcessLevel) {
        if (iNewProcessLevel) {
            if (iNewProcessLevel == 1) {
                MovetoMiddleHigh();
            }
            else {
                if (pDataHandler->IsFormIDCreated(iFormID)) {
                    if (bPermanent) {
                        pSaveLoadGame->UnloadForm(this, false);
                    }
                    SetDelete(true);
                    return false;
                }

                PROCESS_TYPE eLevel = pCurrentProcess->eLevel;
                ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                ReferenceProcessLists.RemoveReference(this, eLevel);
                if (pCurrentProcess) {
                    delete pCurrentProcess;
                }
                pCurrentProcess = nullptr;
                ReferenceProcessLists.kUpdateLock.Unlock();
                DetachCharController();
                CleanUpPointersOnDisable();
            }
        }
        else {
            MovetoHigh();
        }
    }
    return true;
}

bool Projectile::IsTransient() {
    uint32_t uiFormID = TESForm::GetFormID();
    return pDataHandler->IsFormIDCreated(uiFormID);
}

// The frame time, scaled in VATS playback for the player's and the target's shots, drives the fade, the turbo push, the
// movement, the age and the light; the alarm is sent once and the flight sound starts once the player is within
// fSoundDist. afDelta is never read: the delta comes from the timer.
void Projectile::Update([[maybe_unused]] float afDelta) {
    float fDelta = appTimer.GetSeconds();
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && !IsFlagSet(NO_CHARCONTROLLER)) {
        if (GetShooter() == pPlayer) {
            fDelta = pVATS.GetPlayerUpdateMult() * fDelta;
        }
        else if (GetShooter() == pVATSTarget) {
            fDelta = pVATS.GetTargetUpdateMult() * fDelta;
        }
    }
    if (IsFlagSet(FADING)) {
        float fFadeDuration = GetProjectileBase()->GetFadeDuration();
        fAlpha = fAlpha - fDelta / fFadeDuration;
        if (fAlpha <= 0.0) {
            fAlpha = 0.0f;
        }
        if (Get3D()) {
            float fFadeAlpha = fAlpha;
            NiNode* p3D = Get3D();
            BSShaderUtil::RecursiveSetPropertyFadeAlpha(p3D, fFadeAlpha);
        }
    }
    if (GetShooter()) {
        TESObjectREFR* pShooterRef = GetShooter();
        if (pShooterRef->IsActor() && IsFlagSet(NO_CHARCONTROLLER)) {
            Actor* pActor = static_cast<Actor*>(GetShooter());
            if (static_cast<ActorValueOwner*>(pActor)->GetActorValue(ActorValue::TURBO) > 0 && Get3D()) {
                bhkNiCollisionObject* pCollisionObject = bhkNiCollisionObject::GetbhkNiCollisionObject(Get3D());
                bhkEntity* pEntity;
                if (pCollisionObject) {
                    pEntity = bhkEntity::Getbhk(
                        static_cast<hkpWorldObject*>(pCollisionObject->GetWorldObject()->GethkObject())
                    );
                }
                else {
                    pEntity = nullptr;
                }
                if (pEntity) {
                    hkpRigidBody* pBody = static_cast<hkpRigidBody*>(pEntity->GethkObject());
                    bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
                    alignas(16) hkVector4 kForce(0.0f, 0.0f, -30.0f, 0.0f);
                    kForce = pWorld->GetGravity();
                    alignas(16) hkSimdReal kScale(2.0f);
                    kForce.mul4(kScale);
                    pBody->applyForce(fDelta, kForce);
                }
            }
        }
    }
    if (GetProjectileType() != PROJECTILE_TYPE_CONTINUOUS_BEAM) {
        UpdateProjectile(fDelta);
    }
    fAge = fAge + fDelta;
    UpdateLight();
    if (!bSendAlarm) {
        TESObjectREFR* pAlarmShooter = nullptr;
        if (GetShooter()) {
            TESObjectREFR* pRef = GetShooter();
            if (pRef->IsActor()) {
                pAlarmShooter = GetShooter();
            }
        }
        ReferenceProcessLists.SendGrenadeMineAlert(pAlarmShooter, this);
        bSendAlarm = true;
    }
    if (!bSoundFired && !IsFlagSet(HITSCAN_MODE) && Get3D() && pPlayer) {
        const NiPoint3& rProjectilePos = Get3D()->GetWorldTranslate();
        const NiPoint3& rPlayerPos = static_cast<TESObjectREFR*>(pPlayer)->Get3D()->GetWorldTranslate();
        NiPoint3 kDiff = rPlayerPos - rProjectilePos;
        float fDistance = std::fabs(kDiff.Length());
        if (fSoundDist >= fDistance) {
            sndHandle.Play(true);
            bSoundFired = true;
            if (bIsProjectileDebug) {
                NiColorA kColor(0.0f, 0.0f, 1.0f, 0.6f);
                NiTriShape* pPrism = MakeTriPoint(5.0f, kColor, true);
                pPrism->UpdateProperties();
                pPrism->SetTranslate(Get3D()->GetWorldTranslate());
                float fDuration = fProjectileDebugDuration.Float();
                TES::pInstance->AddTempDebugObject(pPrism, fDuration);
            }
        }
    }
}

void Projectile::CleanUpPointersOnDisable() {
    while (!ImpactList.IsEmpty()) {
        Projectile::ImpactData* pImpact = ImpactList.m_item;
        if (pImpact) {
            delete pImpact;
        }
        ImpactList.RemoveHead();
    }
    spActorCause = static_cast<ActorCause*>(nullptr);
    if (pTarget) {
        delete pTarget;
        pTarget = nullptr;
    }
}

Projectile::PROJECTILE_TYPE Projectile::GetProjectileType() {
    return PROJECTILE_TYPE_UNKNOWN;
}

void Projectile::Initialize() {
    bool bLoading = pSaveLoadGame->GetSaveGameLoading() ||
        IsStillLoading() ||
        IsFlagSet(Projectile::PROJECTILE_INITIALIZED);
    if (!pWeaponSource) {
        pWeaponSource = GetProjectileBase()->GetDefaultWeaponSource();
    }
    if (BGSDestructibleObjectForm::IsDestructible(GetObjectReference())) {
        SetDestructible(true);
    }
    SetInitialized(true);
    SetFlag(Projectile::PROJECTILE_INITIALIZED, true);
    if (GetShooter() && !bLoading) {
        TESObjectREFR* pShooterRef = GetShooter();
        SetParentCell(pShooterRef->GetParentCell());
    }

    bSendAlarm = false;
    bool bCreateProcess = pCurrentProcess == nullptr;
    if (bCreateProcess) {
        HighProcess* pProcess;
        pProcess = new HighProcess;
        pCurrentProcess = pProcess;
    }
    if (!IsFlagSet(Projectile::NO_3D) && pCurrentProcess && pCurrentProcess->eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
        if (!Get3D()) {
            Init3D();
        }
        InitHavok();
        if (Get3D()) {
            if (Get3D()->IsFadeNode()) {
                Get3D()->IsFadeNode()->TurnFadeNodeOn();
            }
            else {
                BSShaderUtil::RecursiveSetPropertyFadeAlpha(Get3D()->IsNode(), 1.0f);
            }
        }
    }

    if (GetParentCell()) {
        GetParentCell()->AddReference(this, false);
    }
    AttachLight();
    if (!bLoading) {
        SpawnMuzzleFlash();
    }
    if (bCreateProcess) {
        ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    }
    if (IsFlagSet(Projectile::POST_PROCESS)) {
        // Tests the list's address for null, which it never is.
        BSSimpleList<void*>* pList = &ReferenceProcessLists.ProjectilePostProcessList;
        if (pList) {
            pList->AddHead(this);
        }
    }
    fRange = GetProjectileBase()->Data.fRange;
    if (!bLoading && pShooter && pShooter->IsActor()) {
        Actor* pShooterActor = static_cast<Actor*>(pShooter);
        if (pShooterActor->IsPc() && pShooterActor->GetIronSights() && pWeaponSource && pWeaponSource->HasScope()) {
            fRange = fRange * fCombatIronSightsRangeMult.Float();
        }
    }

    bool bBehindWall = false;
    if (!bLoading && IsFlagSet(Projectile::CHECK_BEHIND_WALL)) {
        bBehindWall = CheckShotBehindWall();
    }
    if (!static_cast<GrenadeProjectile*>(this)->GrenadeProjectile::IsMine()) {
        fExplosionTimer = GetProjectileBase()->GetAltTimer();
    }
    if (!bBehindWall && !bLoading) {
        TestProjectileInTriggerZone();
        if (GetShooter() == pPlayer) {
            AimProjectile();
        }
        AddInitialVelocity();
        if (IsFlagSet(Projectile::HITSCAN_MODE)) {
            UpdateInstant();
        }
    }
    BGSProjectile* pProjectile = static_cast<BGSProjectile*>(GetObjectReference());
    if (pProjectile->GetMotionType() == BGSProjectile::MOTION_BEAM) {
        this->SetConsistencyRecurse(Get3D());
    }
    else if (pProjectile->GetMotionType() == BGSProjectile::MOTION_CONTINUOUS_BEAM) {
        this->SetConsistencyRecurse(Get3D());
    }
    Process3D();
    if (Get3D()) {
        NiUpdateData kUpdateData(fCellAppTime, true, false);
        Get3D()->UpdateSelectedDownwardPass(kUpdateData, 0);
    }
    spPickCache = static_cast<ProjectilePickCache*>(nullptr);
}

bool Projectile::ProcessImpacts() {
    bool bProcessed = false;
    bool bTargetWasAlive = false;
    if (!bImpactProcessed && !ImpactList.IsEmpty()) {
        BSSimpleList<Projectile::ImpactData*>* pImpactList = &ImpactList;
        do {
            if (!pImpactList) {
                break;
            }
            if (pImpactList->IsEmpty()) {
                break;
            }
            Projectile::ImpactData* pData = pImpactList->m_item;
            pImpactList = pImpactList->m_pkNext;
            bool b3dSwitched = false;
            TESObjectREFR* pCollidee = pData->pCollidee;
            if (pCollidee && fDamage > 0.0 && pData->bDoDamage) {
                NiAVObject* pObj3D = pData->pCollidee->TESObjectREFR::Get3D();
                if (GetShooter() &&
                    GetShooter()->IsActor() &&
                    pCollidee->IsActor() &&
                    !GetShooter()->GetDisabledOrDeleted()) {
                    bTargetWasAlive = !pCollidee->IsDead(false);
                    CombatHit(static_cast<Actor*>(pCollidee), false);
                }
                else {
                    bhkRigidBody* pBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pData->pTargetWorldObject));
                    if (pBody && pBody->GetMotionType() < 4) {
                        ActorCause* pActorCause = GetActorCause();
                        pCollidee->SetActorCause(pActorCause);
                    }

                    Actor* pShooterActor = nullptr;
                    if (GetShooter() && GetShooter()->IsActor()) {
                        pShooterActor = static_cast<Actor*>(GetShooter());
                    }
                    float fExtraDamage = 0.0f;
                    if (pShooterActor && pShooterActor->IsPc() && pVATS.IsInVats() && pWeaponSource) {
                        fExtraDamage = static_cast<float>(pWeaponSource->GetCriticalDamage());
                    }

                    double dBaseDamage = fDamage;
                    float fTotalDamage = dBaseDamage + fExtraDamage;
                    if (pCollidee->IsActor()) {
                        HitData* pHitData;
                        Actor* pTargetActor = static_cast<Actor*>(pCollidee);
                        pHitData = new HitData;
                        NiPointer<HitData> spData(pHitData);
                        HitData::InitializeHitData(spData.m_pObject, nullptr, pTargetActor, nullptr, this);
                        bTargetWasAlive = !pTargetActor->IsDead(false);
                        pTargetActor->HitMe(spData.m_pObject, 0);
                    }
                    else {
                        pData->pCollidee->DamageObject(fTotalDamage, false);
                    }
                    if (pCollidee->GetDestroyed() && GetShooter() == pPlayer) {
                        pPlayer->AddPlayerAction(PLAYER_ACTION_DESTROY_OBJECT, 8.0f, nullptr);
                    }
                }
                if (pObj3D != pCollidee->TESObjectREFR::Get3D()) {
                    b3dSwitched = true;
                }
            }

            if ((!pCollidee || (pCollidee->iFormFlags & TESForm::kFormFlag_Deleted) == 0) &&
                (!GetProjectileBase()->GetHasExplosion() ||
                 GetProjectileBase()->GetHasExplosionAltTrigger() ||
                 GetProjectileBase()->GetHasExplosionDetonates()) &&
                !b3dSwitched) {
                if (VATS::pStranger && VATS::pStranger == GetShooter() && pCollidee && pCollidee->IsActor()) {
                    PlaySound_MUSMysteriousStrangerA02();
                }

                if (VATS::pMissFortune &&
                    VATS::pMissFortune == GetShooter() &&
                    pCollidee &&
                    pCollidee->IsActor() &&
                    pCollidee->IsDead(false) &&
                    bTargetWasAlive) {
                    PlaySound_MUSMysteriousStrangerA02();
                }

                const hkpCollidable* pTargetCollidable;
                if (pData->pTargetWorldObject) {
                    pTargetCollidable = pData->pTargetWorldObject->getCollidable();
                }
                else {
                    pTargetCollidable = nullptr;
                }
                SpawnCollisionEffects(pCollidee, pData->kLocation, pData->kNormal, pTargetCollidable, pData->eMaterial);
                NiPoint3 kDirection = GetMovementDirection();
                ApplyImpactForce(pData->kLocation, kDirection, pTargetCollidable);
            }
            bProcessed = true;
        }
        while (!GetDisabledOrDeleted());
        bImpactProcessed = true;
    }
    return bProcessed;
}

bool Projectile::IsMine() {
    return false;
}

bool Projectile::TurnOff(Actor* apActionActor, bool abSilent) {
    if (GetProjectileBase()->GetCanTurnOff() && !IsFlagSet(Projectile::TURNED_OFF)) {
        StopCountdownSound();
        if (!abSilent) {
            TESSound* pDeactivateSound = GetProjectileBase()->Data.pDeactivateSound;
            if (pDeactivateSound) {
                const char* pFilename = pDeactivateSound->GetFilename();
                BSSoundHandle soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                    pFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pDeactivateSound
                );
                soundHandle.SetPosition(GetLocationOnReference());
                soundHandle.Play(false);
            }
        }
        SetFlag(Projectile::TURNED_OFF, true);
        Actor* pShooterActor = nullptr;
        TESFaction* pOwnerFaction = nullptr;
        if (pShooter && pShooter->IsActor()) {
            pShooterActor = static_cast<Actor*>(pShooter);
        }
        else {
            TESForm* pOwner = GetOwner();
            if (pOwner && pOwner->GetFormType() == FACT_ID) {
                pOwnerFaction = static_cast<TESFaction*>(pOwner);
            }
        }
        if (!abSilent &&
            (fExplosionTimer != 3.4028234663852886e+38 ||
             GetMineReactsToTarget(apActionActor, pShooterActor, pOwnerFaction))) {
            MiscStatManager::Increment(MiscStatManager::MS_MINES_DISARMED);
            if (iMineDisarmExperience.Int() > 0) {
                int32_t iExperience = iMineDisarmExperience.Int();
                pPlayer->RewardExperience(iExperience);
            }
        }
        return true;
    }
    return false;
}

bool Projectile::IsPermanent() {
    return IsEditorCreated();
}

// One ray from the projectile's position along its heading and looking for its remaining range, with the character
// controller's filter or, without one, the shooter's group (or a new one) on the projectile layer. Inside the world's
// broadphase the ray is shortened to end inside it; a projectile outside it only warns. The pick runs with the pick
// count override on, and the water pick counts too; a failed pick kills the projectile. The additions and divisions use
// double.
bool Projectile::RunTargetPick() {
    if (!GetParentCell() || !GetParentCell()->GetbhkWorld()) {
        return false;
    }
    bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
    bhkPickData kPickData;
    NiPoint3 kPosition;
    NiPoint3 kForward;
    kPosition = GetLocationOnReference();
    NiMatrix3 kRotation;
    kRotation.MakeZRotation(GetHeading(false));
    NiMatrix3 kLooking;
    kLooking.MakeXRotation(GetLooking());
    kRotation = kRotation * kLooking;
    kRotation.GetCol(1, kForward);
    kForward.Unitize();
    CFilter kFilter(0);
    if (GetCharController()) {
        CFilter kControllerFilter;
        kFilter.iFilter = static_cast<bhkCharacterController*>(GetCharController())
                              ->GetCollisionFilter(&kControllerFilter)
                              ->iFilter;
    }
    else {
        kFilter.SetLayer(L_PROJECTILE);
        if (GetShooter() && GetShooter()->IsActor()) {
            // GetGroup is called on the returned filter word, which is this local.
            CFilter kShooterFilter;
            static_cast<MobileObject*>(GetShooter())->GetCollisionFilter(&kShooterFilter.iFilter);
            kFilter.SetGroup(kShooterFilter.GetGroup());
        }
        else {
            kFilter.SetGroup(bhkCollisionFilter::GetNewSystemGroup());
        }
    }
    if (spPickCache && spPickCache->GetCache()) {
        kPickData.SetCache(spPickCache->GetCache());
    }
    float fLength = GetRange() - GetDistanceMoved();
    NiPoint3 kLength = kForward * fLength;
    NiPoint3 kOrigin;
    pWorld->GetOrigin(kOrigin);
    ahkpWorld* pHavokWorld = pWorld->GetahkpWorld();
    NiPoint3 kMax;
    HK2NI(kMax, pHavokWorld->m_broadPhaseExtents[1]);
    NiPoint3 kMin;
    HK2NI(kMin, pHavokWorld->m_broadPhaseExtents[0]);
    NiPoint3 kHalfExtent = kMax - kMin;
    kHalfExtent /= 2.0f;
    // The largest of the start's distances from the origin per axis, in half extents (z, y, then x).
    NiPoint3 kStart = kPosition - kOrigin;
    float fRatioZ = kStart.z / kHalfExtent.z;
    float fAbsZ = std::fabs(fRatioZ);
    float fRatioY = kStart.y / kHalfExtent.y;
    float fAbsY = std::fabs(fRatioY);
    float fMaxYZ = BSMAX(fAbsY, fAbsZ);
    float fRatioX = kStart.x / kHalfExtent.x;
    float fAbsX = std::fabs(fRatioX);
    float fStartRatio = BSMAX(fAbsX, fMaxYZ);
    if (fStartRatio > 1.0) {
        const char* pName;
        if (GetProjectileBase()->GetFormEditorIDLength()) {
            pName = GetProjectileBase()->GetFormEditorID();
        }
        else {
            pName = static_cast<TESFullName*>(GetProjectileBase())->GetFullName();
        }
        Warning("COMBAT: Projectile %s is spawning outside of the broadphase.", pName);
    }
    else {
        NiPoint3 kEnd = kPosition + kLength;
        NiPoint3 kEndOffset = kEnd - kOrigin;
        float fEndZ = kEndOffset.z / kHalfExtent.z;
        float fEndAbsZ = std::fabs(fEndZ);
        float fEndY = kEndOffset.y / kHalfExtent.y;
        float fEndAbsY = std::fabs(fEndY);
        float fEndMaxYZ = BSMAX(fEndAbsY, fEndAbsZ);
        float fEndX = kEndOffset.x / kHalfExtent.x;
        float fEndAbsX = std::fabs(fEndX);
        float fEndRatio = BSMAX(fEndAbsX, fEndMaxYZ);
        if (fEndRatio > 1.0) {
            // Each axis that ends past the extents scales the ray back to it; NaN compares scale nothing.
            if (kMax.x < static_cast<double>(kPosition.x) + kLength.x) {
                float fScale = (kMax.x - static_cast<double>(kPosition.x)) / kLength.x;
                kLength *= fScale;
            }
            if (kMax.y < static_cast<double>(kPosition.y) + kLength.y) {
                float fScale = (kMax.y - static_cast<double>(kPosition.y)) / kLength.y;
                kLength *= fScale;
            }
            if (kMax.z < static_cast<double>(kPosition.z) + kLength.z) {
                float fScale = (kMax.z - static_cast<double>(kPosition.z)) / kLength.z;
                kLength *= fScale;
            }
            if (kMin.x > static_cast<double>(kPosition.x) + kLength.x) {
                float fScale = (kMin.x - static_cast<double>(kPosition.x)) / kLength.x;
                kLength *= fScale;
            }
            if (kMin.y > static_cast<double>(kPosition.y) + kLength.y) {
                float fScale = (kMin.y - static_cast<double>(kPosition.y)) / kLength.y;
                kLength *= fScale;
            }
            if (kMin.z > static_cast<double>(kPosition.z) + kLength.z) {
                float fScale = (kMin.z - static_cast<double>(kPosition.z)) / kLength.z;
                kLength *= fScale;
            }
        }
    }
    kPickData.SetFilter(kFilter);
    kPickData.NiSetFrom(kPosition);
    kPickData.NiSetLength(kLength);
    alignas(16) uint8_t acCollector[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = ::new (acCollector) hkpAllRayHitCollector;
    kPickData.SetAllCollector(pCollector);
    uint8_t bPickCountOverride = bhkWorld::QPickCountOverride();
    bhkWorld::SetPickCountOverride(true);
    bhkWorld* pPlayerWorld = pPlayer->GetParentCell()->GetbhkWorld();
    bool bHit = pPlayerWorld->PickObject(kPickData);
    bhkWorld::SetPickCountOverride(bPickCountOverride);
    float fWaterHitRatio = std::numeric_limits<float>::max();
    bHit |= PickAutoWater(&kPickData, fWaterHitRatio);
    if (bHit) {
        bHit = ProcessTargetPick(&kPickData, this, GetProjectileBase(), nullptr, true, fWaterHitRatio) != nullptr;
    }
    if (bProjectileDebug.Bool()) {
        NiColorA kColor = kPickData.GetPickFailed() ? NiColorA(1.0f, 0.0f, 0.0f, 1.0f)
                                                    : NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
        NiPoint3 kLineEnd = kPosition + kLength;
        NiLines* pLine = MakeLine(kPosition, kColor, kLineEnd, kColor, true);
        TES::pInstance->AddTempDebugObject(pLine, fProjectileDebugDuration.Float());
    }
    if (kPickData.GetPickFailed()) {
        Kill();
    }
    bool bResult = bHit;
    pCollector->~hkpAllRayHitCollector();
    return bResult;
}

bool Projectile::AddImpact(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    uint32_t aiKey,
    HavokMaterialType aeForceMaterial
) {
    Projectile::ImpactData* pData;
    NiPoint3 kLineEnd;
    pData = new Projectile::ImpactData;
    if (IsFlagSet(Projectile::ALWAYS_HIT_TARGET) && pDesiredTarget && pData->pCollidee != pDesiredTarget) {
        pData->pCollidee = pDesiredTarget;
        pData->pTargetWorldObject = nullptr;
        pData->kLocation = pDesiredTarget->GetLocationOnReference();
        pData->kNormal = -MovementDirection;
        pData->kNormal.Unitize();
        pData->eDamageLimb = BGSBodyPart::LIMB_TORSO;
        pData->eMaterial = BHK_MATERIAL_STONE;
        if (pDesiredTarget->IsActor()) {
            pData->eMaterial = BHK_MATERIAL_ORGANIC;
            Actor* pActor = static_cast<Actor*>(pDesiredTarget);
            NiPoint3 kImpactLocation = pData->kLocation;
            NiPoint3 kStartingLocation = pData->kLocation - MovementDirection;
            NiNode* pClosestBone = pActor->GetClosestBone(kStartingLocation, kImpactLocation);
            if (pClosestBone) {
                hkpRigidBody* pRigidBody;
                pData->eDamageLimb = pActor->GetDamageLocation(pClosestBone);
                bhkCollisionObject* pCollObj = static_cast<bhkCollisionObject*>(pClosestBone->GetCollisionObject());
                if (pCollObj->GetRigidBody()) {
                    pRigidBody = pCollObj->GetRigidBody()->Gethk();
                }
                else {
                    pRigidBody = nullptr;
                }
                pData->pTargetWorldObject = pRigidBody;
            }
            if (pData->pTargetWorldObject) {
                pData->pTargetWorldObject->addReference();
            }
        }
    }
    else {
        hkpWorldObject* pOwner;
        pData->pCollidee = apCollidee;
        pData->kLocation = aContactPoint;
        pData->kNormal = aContactNormal;
        if (apTargetCollidable) {
            pOwner = static_cast<hkpWorldObject*>(apTargetCollidable->getOwner());
        }
        else {
            pOwner = nullptr;
        }
        pData->pTargetWorldObject = pOwner;
        if (pData->pTargetWorldObject) {
            pData->pTargetWorldObject->addReference();
            if (apCollidee && apCollidee->IsActor() && apTargetCollidable->getType() == 1) {
                uint32_t uiFilterInfo = apTargetCollidable->getCollisionFilterInfo();
                if (CFilter(uiFilterInfo).QBipLayer()) {
                    NiAVObject* pHitAVObject = GetAVObjectForCollidable(apTargetCollidable);
                    Actor* pActor = static_cast<Actor*>(apCollidee);
                    NiNode* pClosestBone = pHitAVObject->IsNode();
                    pData->eDamageLimb = pActor->GetDamageLocation(pClosestBone);
                }
            }
        }
        HavokMaterialType eMaterial = aeForceMaterial;
        if (apTargetCollidable && aeForceMaterial == BHK_MATERIAL_MAX) {
            uint32_t uiFilterInfo = apTargetCollidable->getCollisionFilterInfo();
            uint32_t uiGroup = CFilter(uiFilterInfo).GetGroup();
            if (uiGroup == 1) {
                eMaterial = TES::pInstance->GetLandMaterialType(aContactPoint);
            }
            else {
                bhkShape* pShape = bhkShape::Getbhk(apTargetCollidable->m_shape);
                if (pShape) {
                    eMaterial = pShape->GetSubshapeMaterial(aiKey);
                }
            }
            uiFilterInfo = apTargetCollidable->getCollisionFilterInfo();
            if (CFilter(uiFilterInfo).GetLayer() == L_WATER) {
                eMaterial = BHK_MATERIAL_WATER;
            }
        }

        pData->eMaterial = eMaterial;
        if (eMaterial == BHK_MATERIAL_WATER) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueAddRipple(0.01f, aContactPoint);
            }
            else {
                TES::pInstance->GetWaterSystem()->AddRipple(aContactPoint, 0.01f);
            }
        }
    }
    bool bVATSSuccess = true;
    if (bVATSProjectileDebug.Bool() && pTarget) {
        NiAVObject* pHitAVObject = GetAVObjectForCollidable(apTargetCollidable);
        bhkCollisionObject* pTargetCol = FindFirstCollisionObject(pTarget->spPart.m_pObject);
        NiAVObject* pTargetObject = pTargetCol ? pTargetCol->m_pkSceneObject : nullptr;
        TESObjectREFR* pTargetRef = pTarget->pReference;
        bVATSSuccess = apCollidee == pTargetRef && pHitAVObject == pTargetObject;
        const char* pOutcome = bVATSSuccess ? "SUCCESS" : "FAILURE";
        InitializationFlow("Targetted VATS Projectile Result: %s", pOutcome);
        const char* pTargetName;
        if (pTargetRef->GetFormEditorIDLength()) {
            pTargetName = pTargetRef->GetFormEditorID();
        }
        else {
            pTargetName = pTargetRef->GetFullName();
        }
        InitializationFlow("Target Ref ID:\t%s %08X", pTargetName, pTargetRef->iFormID);
        const char* pTargetPart;
        if (pTargetObject) {
            pTargetPart = pTargetObject->m_kName.GetHandleString();
        }
        else {
            pTargetPart = "";
        }
        InitializationFlow("Target Part:\t%s", pTargetPart);
        int32_t iHitFormID;
        if (apCollidee) {
            iHitFormID = apCollidee->iFormID;
        }
        else {
            iHitFormID = 0;
        }
        const char* pHitName;
        if (apCollidee) {
            if (apCollidee->GetFormEditorIDLength()) {
                pHitName = apCollidee->GetFormEditorID();
            }
            else {
                pHitName = apCollidee->GetFullName();
            }
        }
        else {
            pHitName = "Land";
        }
        InitializationFlow("Hit Ref ID:\t\t%s %08X", pHitName, iHitFormID);
        const char* pHitPart;
        if (pHitAVObject) {
            pHitPart = pHitAVObject->m_kName.GetHandleString();
        }
        else {
            pHitPart = "";
        }
        InitializationFlow("Hit Part:\t\t%s\n", pHitPart);
    }

    if (pData->pCollidee) {
        pData->pCollidee->SetTargeted(true);
    }
    if (GetProjectileType() == Projectile::PROJECTILE_TYPE_CONTINUOUS_BEAM) {
        ContinuousBeamProjectile* pcbeam = static_cast<ContinuousBeamProjectile*>(this);
        if (this) {
            int32_t iShotsFired = pcbeam->iShotsFired;
            double dAge = GetAge();
            double dShotsFired = static_cast<double>(iShotsFired);
            if (!(dShotsFired / pWeaponSource->data.fShotsPerSec < dAge)) {
                pData->bDoDamage = false;
            }
            else {
                Actor* pShooterActor = static_cast<Actor*>(GetShooter());
                if (pShooterActor) {
                    pShooterActor->UseAmmo(1);
                }
                pcbeam->SetShotsFired(iShotsFired + 1);
                pData->bDoDamage = pData->pCollidee && pData->pCollidee->IsActor();
                if (pShooterActor->GetCurrentAmmoClipCount() < 1) {
                    pcbeam->SetKill(pShooterActor);
                }
            }
        }
    }
    if (bProjectileDebug.Bool() || bVATSProjectileDebug.Bool()) {
        NiColorA color(0.0f, 0.0f, 1.0f, 0.1f);
        if (pData->pCollidee && pData->pCollidee->IsActor()) {
            NiColorA kActorColor(0.0f, 1.0f, 0.0f, 0.1f);
            color = kActorColor;
        }

        if (!bVATSSuccess) {
            NiColorA kFailureColor(1.0f, 0.0f, 0.0f, 0.65f);
            color = kFailureColor;
        }
        if (pData->bDoDamage) {
            NiColorA kDamageColor(0.5f, 0.0f, 0.0f, 0.65f);
            color = kDamageColor;
        }
        NiAVObject* pDebug = MakeTriPoint(5.0f, color, true);
        pDebug->UpdateProperties();
        pDebug->SetTranslate(pData->kLocation);
        TES::pInstance->AddTempDebugObject(pDebug, fProjectileDebugDuration.Float());
        NiPoint3* pLineEnd = ::new (&kLineEnd) NiPoint3(pData->kLocation + pData->kNormal * 24.0f);
        pDebug = MakeLine(pData->kLocation, color, *pLineEnd, color, true);
        TES::pInstance->AddTempDebugObject(pDebug, fProjectileDebugDuration.Float());
    }
    ImpactList.AddHead(pData);
    return true;
}

Projectile::~Projectile() {
    if (pSecondaryItemChange) {
        delete pSecondaryItemChange;
    }
    pSecondaryItemChange = nullptr;
    if (TESObjectREFR::Get3D() &&
        TESObjectREFR::Get3D()->IsNode() &&
        TESObjectREFR::HasAddonFlags(TESObjectREFR::Get3D()->IsNode())) {
        TESObjectREFR::RemoveMasterParticleAddonNodes(TESObjectREFR::Get3D()->IsNode());
    }

    TESObjectREFR::Set3D(nullptr, true);
    StopCountdownSound();
    if (GetProjectileBase() && !GetProjectileBase()->GetSupersonic()) {
        sndHandle.FadeOutAndRelease(1000u);
    }
    if (pVATS.pFirstProjectile == this) {
        pVATS.SetFirstProjectile(nullptr);
    }
    if (pVATS.pLastProjectile == this) {
        pVATS.SetLastProjectile(nullptr);
    }
    if (IsFlagSet(Projectile::POST_PROCESS)) {
        ReferenceProcessLists.ProjectilePostProcessList.Remove(this);
    }
    if (IsFlagSet(Projectile::ACTIVE_IN_INVENTORY) && GetDesiredTarget()) {
        Actor* pActorTarget = static_cast<Actor*>(GetDesiredTarget());
        if (pActorTarget->IsActor() && pActorTarget->pCurrentProcess) {
            pActorTarget->pCurrentProcess->SetPlantedExplosive(false);
        }
        GetDesiredTarget()
            ->RemoveItem(GetProjectileBase(), nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false);
    }
    DetachLight();
    while (!ImpactList.IsEmpty()) {
        Projectile::ImpactData* pData = ImpactList.m_item;
        delete pData;
        ImpactList.RemoveHead();
    }
    if (!pDataHandler->GetClearingData()) {
        CleanUpOtherMobileObjects(false);
        pCombatManager->pThreatMap->RemoveThreat(this);
    }
    if (pTarget) {
        delete pTarget;
        pTarget = nullptr;
    }
    spPickCache = nullptr;
    spActorCause = nullptr;
}

void Projectile::Sync3DWithReference(bool abSyncRotation, bool abDoArcReorientation) {
    NiPoint3 kDirection;
    float fYAngle;
    float fZAngle;
    NiMatrix3 kRot;
    float fXAngle;
    NiPoint3 kAngle;
    NiMatrix3 kRotate;
    NiNode* pRootNode = Get3D();
    if (pRootNode) {
        pRootNode->SetTranslate(GetLocationOnReference());
        if (abSyncRotation) {
            kAngle = TESObjectREFR::GetAngleOnReference();
            kRotate.FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
            pRootNode->SetRotate(kRotate);
        }
        if (abDoArcReorientation && IsFlagSet(Projectile::REORIENT)) {
            std::memcpy(&kRot, &pRootNode->GetLocalTransform(), sizeof(kRot));
            kRot.ToEulerAnglesZXY(fZAngle, fXAngle, fYAngle);
            kDirection = GetMovementDirection();
            fXAngle = -NiASin(kDirection.z);
            kRot.FromEulerAnglesZXY(fZAngle, fXAngle, fYAngle);
            pRootNode->SetRotate(kRot);
        }
    }
}

// A shooter in turbo moves its projectiles at real time, not at the slowed global time.
void Projectile::Move(float afDeltaTime, NiPoint3 aDeltaMove) {
    NiPoint3 kOldPosition = GetLocationOnReference();
    if (pShooter && pShooter->IsActor()) {
        Actor* pActor = static_cast<Actor*>(pShooter);
        if (pActor->GetActorValue(ActorValue::TURBO) > 0) {
            afDeltaTime = 1.0 / BSTimer::QGlobalTimeMultiplier() * afDeltaTime;
        }
    }
    MobileObject::Move(afDeltaTime, aDeltaMove, 15);
    NiPoint3 kNewPosition = GetLocationOnReference();
    SetMovementDelta(kNewPosition - kOldPosition, true);
}

// Rounds GetSpeed() * afDeltaTime to float before building the move.
void Projectile::Move(float afDeltaTime) {
    float fDistance = GetSpeed() * afDeltaTime;
    NiPoint3 kDeltaMove(0.0f, fDistance, 0.0f);
    Move(afDeltaTime, kDeltaMove);
}

void Projectile::Kill() {
    if (bVATSProjectileDebug.Bool() && ImpactList.IsEmpty() && pTarget) {
        bhkCollisionObject* pTargetCol = FindFirstCollisionObject(pTarget->spPart.m_pObject);
        NiAVObject* pTargetObject = pTargetCol ? pTargetCol->m_pkSceneObject : nullptr;
        TESObjectREFR* pTargetRef = pTarget->pReference;
        InitializationFlow("Targetted VATS Projectile Result: %s", "FAILURE");
        const char* pTargetName;
        if (pTargetRef->GetFormEditorIDLength()) {
            pTargetName = pTargetRef->GetFormEditorID();
        }
        else {
            pTargetName = pTargetRef->GetFullName();
        }
        InitializationFlow("Target Ref ID:\t%s %08X", pTargetName, pTargetRef->iFormID);
        const char* pTargetPart;
        if (pTargetObject) {
            pTargetPart = pTargetObject->m_kName.GetHandleString();
        }
        else {
            pTargetPart = "";
        }
        InitializationFlow("Target Part:\t%s", pTargetPart);
    }

    if (GetFormType() == PGRE_ID) {
        VATSMenu::RemoveTarget(this);
    }
    if (pDataHandler->IsFormIDCreated(iFormID)) {
        SetDelete(true);
    }
    else {
        Disable();
    }
}

TESObjectREFR* Projectile::GetShooter() {
    return pShooter;
}

BGSProjectile* Projectile::GetProjectileBase() {
    return static_cast<BGSProjectile*>(GetObjectReference());
}

void Projectile::CheckExplosion(float afDelta) {
    if (!GetProjectileBase()->GetHasExplosion() || !GetProjectileBase()->GetExplosion() || IsFlagSet(TURNED_OFF)) {
        return;
    }
    bool bSpawn = false;
    if (GetProjectileBase()->GetHasExplosionAltTrigger()) {
        // The timer counts down when it is no longer FLT_MAX, when the base form has no trigger proximity, or when
        // CheckExplosionProximity finds something in range; otherwise the countdown sound stops.
        if (fExplosionTimer != 3.4028234663852886e+38 ||
            GetProjectileBase()->GetExplosionProximity() == 0.0 ||
            CheckExplosionProximity()) {
            fExplosionTimer = fExplosionTimer - afDelta;
            if (!(fExplosionTimer <= 0.0)) {
                fBlinkTimer = fBlinkTimer - afDelta;
                if (fBlinkTimer <= 0.0) {
                    NiAVObject* pLight = BSUtilities::GetObjectByName(Get3D(), "##LightOn");
                    if (pLight) {
                        bool bCulled = pLight->GetAppCulled();
                        SetLight(Get3D(), bCulled);
                    }
                    if (!(fMinesBlinkMax.Float() < static_cast<double>(fExplosionTimer))) {
                        // The blink speeds up from fMinesBlinkSlow to fMinesBlinkFast as the timer runs down.
                        double dRatio = fExplosionTimer / fMinesBlinkMax.Float();
                        fBlinkTimer = (fMinesBlinkSlow.Float() - fMinesBlinkFast.Float()) * dRatio +
                            fMinesBlinkFast.Float();
                    }
                    else {
                        fBlinkTimer = fMinesBlinkSlow.Float();
                    }
                    StartCountdownSound();
                }
            }
            else {
                bSpawn = true;
            }
        }
        else {
            StopCountdownSound();
        }
    }
    else if (IsFlagSet(DETONATE)) {
        bSpawn = true;
    }
    else if (!GetProjectileBase()->GetHasExplosionDetonates() && !ImpactList.IsEmpty()) {
        bSpawn = true;
    }
    if (!bSpawn) {
        return;
    }
    NiPoint3 location;
    NiMatrix3 rotation;
    if ((iMolotovTest.Int() > 0 || IsFlagSet(SPAWN_EXPLOSION_AT_IMPACT)) && !ImpactList.IsEmpty()) {
        // At the first impact, turned to its normal.
        Projectile::ImpactData* pData = ImpactList.GetItem();
        float fRand = RandomFloatTwoPi();
        // This random turn is built and never used.
        NiMatrix3 kRandRot;
        kRandRot.MakeZRotation(fRand);
        rotation = BSUtilities::GetRotationZToVectorTranspose(pData->kNormal);
        location = pData->kLocation;
    }
    else {
        location = GetLocationOnReference();
        const NiMatrix3* pRotate;
        if (Get3D()) {
            pRotate = &Get3D()->GetWorldTransform().m_Rotate;
        }
        else {
            pRotate = &NiMatrix3::IDENTITY;
        }
        rotation = *pRotate;
        FindBlastNodes(Get3D(), location);
    }
    TESObjectCELL* pCell = GetParentCell();
    TESObjectREFR* pOwner;
    if (IsFlagSet(ACTIVE_IN_INVENTORY)) {
        pOwner = nullptr;
    }
    else {
        pOwner = GetShooter();
    }
    Explosion* pExplosion = Explosion::SpawnExplosion(
        GetProjectileBase()->GetExplosion(),
        pOwner,
        nullptr,
        pCell,
        location,
        rotation
    );
    if (pExplosion && !ImpactList.IsEmpty() && GetProjectileBase()->GetMotionType() == BGSProjectile::MOTION_MISSILE) {
        pExplosion->SetClosestPointNormal(ImpactList.GetItem()->kNormal);
    }
    if (pExplosion && pWeaponSource) {
        pExplosion->SetWeaponSource(pWeaponSource);
    }
    if (IsFlagSet(KILL_ON_EXPLOSION)) {
        if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
            ProcessImpacts();
        }
        Projectile::Kill();
    }
    if (GetProjectileBase()->GetExplosion()->GetDamage() > 0.0 &&
        IsFlagSet(ACTIVE_IN_INVENTORY) &&
        GetDesiredTarget() &&
        GetDesiredTarget()->IsActor()) {
        Actor* pTargetActor = static_cast<Actor*>(GetDesiredTarget());
        Actor* pShooterActor;
        if (GetShooter() && GetShooter()->IsActor()) {
            pShooterActor = static_cast<Actor*>(GetShooter());
        }
        else {
            pShooterActor = nullptr;
        }
        // IsPc is called on the shooter even when it is null.
        if (pShooterActor->IsPc()) {
            MiscStatManager::Increment(MiscStatManager::MS_PANTS_EXPLODED);
            if (pTargetActor->pCurrentProcess) {
                pTargetActor->pCurrentProcess->ModTrackedDamage(pPlayer, 10000.0f);
            }
        }
        pTargetActor->Kill(pShooterActor, 0.0f);
        BaseProcess* pPlayerProcess = pPlayer->pCurrentProcess;
        int32_t iSoundLevel = GetProjectileBase()->GetExplosion()->GetSoundLevelValue();
        pPlayerProcess
            ->SetActorsDetectionEvent(pPlayer, pTargetActor->GetLocationOnReference(), iSoundLevel, 2, pTargetActor);
    }
}

void Projectile::SetRotation(const NiMatrix3* apRotation) {
    float fZAngle;
    float fXAngle;
    float fYAngle;
    apRotation->ToEulerAnglesZXY(fZAngle, fXAngle, fYAngle);
    SetHeading(fZAngle);
    SetLooking(fXAngle);
}

void Projectile::SetPickCache(ProjectilePickCache* apCache) {
    spPickCache = apCache;
}

void Projectile::SetPower(float afValue) {
    fPower = afValue;
}

void Projectile::SetMovementDelta(NiPoint3 aDelta, bool abAddDistance) {
    MovementDirection = aDelta;
    if (abAddDistance) {
        fDistanceMoved = MovementDirection.UnitizeGetLength() + fDistanceMoved;
    }
    else {
        MovementDirection.Unitize();
    }
}

bool Projectile::IsFlagSet(ProjectileFlags aeFlag) {
    return (aeFlag & iFlags) != 0;
}

// A post-processing projectile updates once more after the process lists' pass; it never asks to be removed.
bool Projectile::PostProcess(float afDelta) {
    if (Projectile::IsFlagSet(Projectile::POST_PROCESS)) {
        Update(afDelta);
    }
    return false;
}

float Projectile::GetAlpha() {
    return fAlpha;
}

void Projectile::UpdateLight() {
    if (spLight && IsFlagSet(FADING)) {
        float fDimmer = BSMAX(0.0f, BSMIN(1.0f, fAlpha));
        spLight->SetDimmer(fDimmer);
    }
}

float Projectile::GetRange() {
    return fRange;
}

// A body that is not keyframed or fixed is pushed at aContactPoint along the unit aDirection with the base form's
// force, scaled for gameplay, inside the world's write marks.
void Projectile::ApplyImpactForce(
    NiPoint3 aContactPoint,
    NiPoint3 aDirection,
    const hkpCollidable* apTargetCollidable
) {
    if (!apTargetCollidable) {
        return;
    }
    bhkEntity* pEntity = bhkEntity::Getbhk(hkpGetRigidBody(apTargetCollidable));
    ahkpWorld* pWorld;
    if (pEntity) {
        pWorld = static_cast<ahkpWorld*>(pEntity->GethkWorldRw());
    }
    else {
        pWorld = nullptr;
    }
    if (!pWorld) {
        return;
    }
    // Keyframed, fixed and the motion types after them are not pushed.
    if (pEntity->GetMotionType() >= hkpMotion::MOTION_KEYFRAMED) {
        return;
    }
    bhkRigidBody* pRigidBody = static_cast<bhkRigidBody*>(pEntity);
    float fForce = GetProjectileBase()->GetForce();
    fForce = TESHavokUtilities::ScaleGameplayImpulseForce(fForce, pRigidBody, true);
    if (fForce == 0.0) {
        return;
    }
    alignas(16) hkVector4 kImpulse;
    NI2HK(kImpulse, aDirection);
    alignas(16) hkVector4 kPoint;
    NI2HK(kPoint, aContactPoint);
    if (static_cast<float>(kImpulse.length3()) > 0.0) {
        kImpulse.normalize3();
    }
    alignas(16) hkSimdReal kForce(fForce);
    kImpulse.mul4(kForce);
    pWorld->markForWrite();
    (**pRigidBody)->applyPointImpulse(kImpulse, kPoint);
    pWorld->unmarkForWrite();
}

bool Projectile::PickAutoWater(bhkPickData* apPickData, float& afWaterHitRatio) {
    NiPoint3 startPoint;
    NiPoint3 endPoint;
    TESObjectCELL* pCell = GetParentCell();
    if (!pCell) {
        return false;
    }
    if (!pCell->HasWater()) {
        return false;
    }
    float fWaterHeight = pCell->GetWaterHeight();
    apPickData->GetFrom(startPoint);
    apPickData->GetTo(endPoint);
    if (!(fWaterHeight < static_cast<double>(startPoint.z)) || !(fWaterHeight >= static_cast<double>(endPoint.z))) {
        return false;
    }
    NiPoint3 length = endPoint - startPoint;
    float fToWater = fWaterHeight - startPoint.z;
    afWaterHitRatio = fToWater / length.z;
    return true;
}

// The ray runs from the player's eye (the player's position raised by the scaled eye height) to the projectile, with
// the player's (or the projectile's character controller's) filter on the projectile layer; the world pick and the
// water pick both count.
bool Projectile::CheckShotBehindWall() {
    if (pShooter != pPlayer) {
        return false;
    }
    bhkPickData kPickData;
    NiPoint3 kEye;
    NiPoint3 kProjectilePos;
    NiPoint3 kLength;
    kProjectilePos = GetLocationOnReference();
    kEye = static_cast<TESObjectREFR*>(pPlayer)->GetLocationOnReference();
    // Keeps the eye height as a double and adds the float z to the double product.
    double dEyeHeight = pPlayer->GetEyeHeight();
    kEye.z = static_cast<float>(pPlayer->GetScale() * dEyeHeight + kEye.z);
    kLength = kProjectilePos - kEye;
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    CFilter kFilter(0);
    uint32_t uiFilter;
    CFilter kControllerFilter;
    if (pController) {
        kFilter.iFilter = pController->GetCollisionFilter(&kControllerFilter)->iFilter;
    }
    else {
        kFilter.iFilter = *pPlayer->GetCollisionFilter(&uiFilter);
    }
    kFilter.SetLayer(L_PROJECTILE);
    kPickData.SetFilter(kFilter);
    kPickData.NiSetFrom(kEye);
    kPickData.NiSetLength(kLength);
    alignas(16) uint8_t acCollector[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = ::new (acCollector) hkpAllRayHitCollector;
    kPickData.SetAllCollector(pCollector);
    bhkWorld* pWorld = pPlayer->GetParentCell()->GetbhkWorld();
    bool bHit = pWorld->PickObject(kPickData);
    float fWaterHitRatio = std::numeric_limits<float>::max();
    bHit |= PickAutoWater(&kPickData, fWaterHitRatio);
    if (bHit) {
        bHit = ProcessTargetPick(&kPickData, this, GetProjectileBase(), nullptr, true, fWaterHitRatio) != nullptr;
    }
    if (bHit) {
        SetRange(0.0f);
    }
    bool bResult = bHit;
    pCollector->~hkpAllRayHitCollector();
    return bResult;
}

TESObjectREFR* Projectile::GetDesiredTarget() {
    return pDesiredTarget;
}

void Projectile::SetExplosionTimer(float afExplosionTimer) {
    fExplosionTimer = afExplosionTimer;
}

void Projectile::UpdateInstant() {
    if (IsFlagSet(Projectile::HITSCAN_MODE)) {
        RunTargetPick();
    }
}

ItemChange* Projectile::GetSecondaryItemChange() {
    return pSecondaryItemChange;
}

void Projectile::SetAngularMomentumZ(float afMomentum) {
    fSpreadHeading = afMomentum;
}

void Projectile::SetAngularMomentumX(float afMomentum) {
    fSpreadLooking = afMomentum;
}

void Projectile::StopCountdownSound() {
    if (sndCountdown.IsValid()) {
        sndCountdown.Stop();
        if (sndCountdown.IsValid()) {
            sndCountdown.Release();
        }
    }
}

void Projectile::CombatHit(Actor* apCollidee, bool abPowerAttack) {
    if (pShooter && pShooter->IsActor()) {
        uint8_t cCanHit = 1;
        Actor* pSourceRef = static_cast<Actor*>(pShooter);
        if (!pSourceRef->IsPc() && IsFlagSet(Projectile::SHOOTER_IN_COMBAT)) {
            cCanHit = pSourceRef->IsInCombatWithActor(apCollidee);
        }
        if (cCanHit) {
            pSourceRef->CombatHit(apCollidee, abPowerAttack, this, 0);
        }
    }
}

// Tests the object for NiGeometry (IsKindOf) and else walks it as a node, so it takes any NiAVObject: Initialize passes
// the projectile's 3D node.
void Projectile::SetConsistencyRecurse(NiAVObject* apObject) {
    if (apObject) {
        if (NiObject::IsKindOf_0(&NiGeometry::ms_RTTI, apObject)) {
            NiGeometryData* pModelData = static_cast<NiGeometry*>(apObject)->GetModelData();
            if (pModelData) {
                pModelData->SetConsistency(NiGeometryData::MUTABLE);
            }
        }
        else {
            NiNode* pNode = apObject->IsNode();
            if (pNode) {
                for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                    SetConsistencyRecurse(pNode->GetAt(i));
                }
            }
        }
    }
}

void Projectile::DetachLight() {
    if (spLight.m_pObject) {
        NiLight* pLight = spLight.m_pObject;
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        pShadowSceneNode->RemoveLight(pLight);
        spLight = nullptr;
    }
}

void Projectile::AimAtPoint(NiPoint3 aTargetLocation) {
    NiPoint3 kVector = aTargetLocation - GetLocationOnReference();
    kVector.Unitize();
    float fLooking = MobileObject::GetLooking();
    float fGravity = GetProjectileBase()->GetGravity();
    if (fGravity == 0.0) {
        float fSlope = static_cast<float>(kVector.z / (1.0 - kVector.z * kVector.z));
        fLooking = -std::atanf(fSlope);
    }
    float fZAngle = GetZAngleFromVector(kVector);
    if (fGravity == 0.0 || std::fabs(fZAngle - GetHeading(false)) <= 2.5) {
        SetHeading(fZAngle);
    }
    MobileObject::SetLooking(fLooking);
}

Projectile::ImpactData* Projectile::GetImpactData() {
    return ImpactList.m_item;
}

void Projectile::AimProjectile() {
    bool bAim = true;
    if (IsFlagSet(Projectile::NO_CHARCONTROLLER) && !IsFlagSet(Projectile::NO_COLLISION)) {
        bAim = pVATS.GetMode() == VATS::VATS_PLAYBACK;
    }
    if (bAim) {
        NiNode* pReleaseObj = static_cast<NiNode*>(pPlayer->GetFireNode());
        if (!pReleaseObj) {
            pReleaseObj = Get3D();
        }
        if ((IsFlagSet(Projectile::WHOLE_BODY_TARGETED) || !IsFlagSet(Projectile::NO_CHARCONTROLLER)) &&
            pVATS.GetMode() == VATS::VATS_PLAYBACK &&
            pVATS.GetCurrentAction() &&
            pVATS.GetCurrentAction()->bSuccess) {
            ProjectileTarget* pProjectileTarget;
            pProjectileTarget = new ProjectileTarget;
            SetProjectileTarget(pProjectileTarget);
        }

        if (!spPickCache.m_pObject) {
            pPlayer->ApplyAutoAimActor(
                this,
                pReleaseObj,
                data.kRotation.z,
                data.kRotation.x,
                &data.kPosition,
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min()
            );
        }
        Sync3DWithReference(true, false);
    }
}

void Projectile::Init3D() {
    Load3D(false);
    NiNode* p3D = Get3D();
    if (p3D) {
        p3D->SetScale(1.0);
        Sync3DWithReference(true, false);
        if (p3D->IsNode() && TESObjectREFR::HasAddonFlags(p3D->IsNode())) {
            TESObjectREFR::AddMasterParticleAddonNodes(p3D->IsNode());
        }
        NiTimeController::StartAnimations(p3D);
    }
}

void Projectile::AddInitialVelocity() {
    if (!IsFlagSet(Projectile::NO_COLLISION) && IsFlagSet(Projectile::NO_CHARCONTROLLER)) {
        if (pDataHandler->IsFormIDCreated(iFormID)) {
            NiNode* p3D = Get3D();
            if (p3D) {
                NiPoint3 kVelocity;
                bhkWorld::Enable(p3D, false, true, true);
                NiUpdateData kUpdateData(0.0f, false, false);
                p3D->UpdateWorldData(kUpdateData);
                bhkWorld::Enable(p3D, true, true, true);
                p3D->GetWorldTransform().m_Rotate.GetCol(1u, kVelocity);
                kVelocity *= GetSpeed();
                TESHavokUtilities::AddVelocity(p3D, kVelocity, true);
                AddChange(CHANGE_REFR_HAVOK_MOVE);
            }
        }
    }
}

void Projectile::UpdateIfNotInArrowsList(float afDelta) {
    if (!IsFlagSet(Projectile::POST_PROCESS)) {
        Update(afDelta);
    }
}

void Projectile::HandleInventoryGrenadeDialogue() {
    if (IsFlagSet(Projectile::ACTIVE_IN_INVENTORY) &&
        !GetProjectileBase()->IsFlagSet(BGSProjectile::EXPLOSION_DETONATES)) {
        double dAge = GetAge();
        if (fProjectileInventoryGrenadeFreakoutTime.Float() < dAge && !IsFlagSet(Projectile::INVENTORY_FREAKOUT)) {
            SetFlag(Projectile::INVENTORY_FREAKOUT, true);
            pCombatManager->pThreatMap->AddProjectile(this);
            if (GetDesiredTarget()) {
                TESObjectREFR* pGrenadeTargetRef = GetDesiredTarget();
                if (pGrenadeTargetRef->IsActor()) {
                    Actor* pActor = static_cast<Actor*>(GetDesiredTarget());
                    if (pActor->GetCurrentAIProcess()) {
                        pActor->GetCurrentAIProcess()->SetPlantedExplosive(true);
                        TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 5);
                        pActor->GetCurrentAIProcess()->ProcessGreet(pActor, pTopic, false, false, false, false);
                        pActor->GetCurrentAIProcess()
                            ->SetupSpecialIdle(pActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, true, true);
                    }
                }
            }
        }
    }
}

bool Projectile::TestProjectileInTriggerZone() {
    CFilter kGasTrapFilter;
    CFilter kZoneFilter;
    NiPoint3 kMaxOffset;
    NiPoint3 kMinOffset;
    bool bTriggerFound = false;
    if (GetProjectileBase()->GetHasMuzzleFlash() && this->GetProjectileBase()->GetMuzzleFlashModel().GetModelLength()) {
        bhkWorld* pWorld = this->GetParentCell() ? this->GetParentCell()->GetbhkWorld() : nullptr;
        if (!pWorld) {
            return bTriggerFound;
        }
        hkpWorld* pHkWorld = **pWorld;
        hkpBroadPhase* pBroadPhase = pHkWorld ? pHkWorld->m_broadPhase : nullptr;
        ::new (&kMinOffset) NiPoint3(1.0f * NiPoint3::UNIT_ALL);
        NiPoint3 min = this->GetLocationOnReference() - kMinOffset;
        ::new (&kMaxOffset) NiPoint3(1.0f * NiPoint3::UNIT_ALL);
        alignas(16) NiPoint3 max = this->GetLocationOnReference() + kMaxOffset;
        hkAabb aabb;
        NI2HK(aabb.m_min, min);
        NI2HK(aabb.m_max, max);
        using hits_t = hkInplaceArray<hkpBroadPhaseHandlePair, 128, hkContainerHeapAllocator>;
        alignas(16) hits_t hits(0);
        pBroadPhase->querySingleAabb(aabb, hits);
        for (int32_t i = 0; !bTriggerFound && i < hits.m_size; ++i) {
            hkpBroadPhaseHandlePair* pPair = &hits[i];
            hkpTypedBroadPhaseHandle* tp = static_cast<hkpTypedBroadPhaseHandle*>(pPair->m_b);
            const hkpCollidable* collB = static_cast<const hkpCollidable*>(tp->getOwner());
            NiAVObject* pav = GetAVObjectForCollidable(collB);
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pav);
            hkpPhantom* pPhantom = hkpGetPhantom(collB);
            bhkPhantom* pBP = pPhantom ? bhkPhantom::Getbhk(pPhantom) : nullptr;
            if (pBP && pRef) {
                if (pBP->GetCollisionFilter(&kZoneFilter)->GetLayer() == L_PROJECTILEZONE ||
                    pBP->GetCollisionFilter(&kGasTrapFilter)->GetLayer() == L_GASTRAP) {
                    bTriggerFound = true;
                    Script::SetActionFlag(this, &pRef->m_Extra, SCRIPT_ACTION_ONHIT);
                    Script::SetActionFlag(pGasTrapDummyWeap, &pRef->m_Extra, SCRIPT_ACTION_ONHITWITH);
                }
            }
        }
    }
    return bTriggerFound;
}

void Projectile::SpawnCollisionEffects(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    HavokMaterialType aeMaterial
) {
    hkpPropertyValue kPropertyValue;
    bool bCreateParticles = true;
    NiPoint3 distance = aContactPoint - Main::WorldRootCamera()->GetWorldTranslate();
    if (Main::GetSceneIsComplex()) {
        bCreateParticles = false;
    }
    else {
        double dDistance = distance.Length();
        if (fGunParticleCameraDistance.Float() < dDistance ||
            !PointInFrustum(aContactPoint, Main::WorldRootCamera(), 32.0f)) {
            bCreateParticles = false;
        }
    }
    if (!apCollidee || !apCollidee->IsActor()) {
        if (apCollidee) {
            Script::SetActionFlag(this, &apCollidee->m_Extra, SCRIPT_ACTION_ONHIT);
            if (pWeaponSource) {
                Script::SetActionFlag(pWeaponSource, &apCollidee->m_Extra, SCRIPT_ACTION_ONHITWITH);
            }
        }
        BGSImpactData* pImpactData = pWeaponSource ? pWeaponSource->GetImpactDataForMaterial(aeMaterial) : nullptr;
        if (pImpactData) {
            float fLifetime;
            BGSImpactData::ORIENTATION eOrient;
            NiPoint3 kImpactEffectVector;
            const char* pFilename;
            NiPoint3 kEffectVector;
            NiPoint3 kEffectPosition;
            BGSTextureSet* pSet = pImpactData->pDecalTextureSet;
            if (pSet) {
                NiPoint3 dir = aContactNormal;
                dir.Unitize();
                NiPoint3 origin = aContactPoint;
                if (fDecalSize == 0.0) {
                    float fMaxWidth = pImpactData->GetDecalMaxWidth();
                    float fMinWidth = pImpactData->GetMinWidth();
                    fDecalSize = RandomFloat(fMinWidth, fMaxWidth);
                }
                DecalCaster* pCaster = DecalCaster::GetInstance(fDecalSize, true);
                pCaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
                pCaster->SetCollision(L_BIPED, false);
                pCaster->SetCollision(L_DEADBIP, false);
                TESObjectCELL* pcell = GetParentCell();
                bhkWorld* pw = pcell ? pcell->GetbhkWorld() : nullptr;
                if (pw) {
                    pCaster->Add(pw);
                    hkpAllCdBodyPairCollector collector;
                    pCaster->GetPenetrations(aContactPoint, collector);
                    int32_t iHits = collector.m_hits.m_size;
                    float fRotation = RandomFloatTwoPi();
                    float fSubTex = RandomFloat(0.0f, 4.0f);
                    uint8_t cSubTex = static_cast<int32_t>(std::floor(fSubTex));
                    const hkpCollidable* pLastCol = nullptr;
                    for (int32_t I = 0; I < iHits; ++I) {
                        const hkpCollidable* pCol = collector.m_hits[I].m_rootCollidableB;
                        if (pCol) {
                            HavokMaterialType eMaterialB = aeMaterial;
                            uint32_t uiKey = collector.m_hits[I].m_shapeKeyB;
                            if (uiKey == -1) {
                                bhkShape* pShape = bhkShape::Getbhk(pCol->m_shape);
                                if (!pShape) {
                                    continue;
                                }
                                eMaterialB = pShape->GetMaterial();
                            }
                            else {
                                bhkShape* pShape = bhkShape::Getbhk(pCol->m_shape);
                                if (!pShape) {
                                    continue;
                                }
                                bhkShapeCollection* pShapeCol = pShape->FindShapeCollection();
                                if (pShapeCol) {
                                    eMaterialB = pShapeCol->QSubMaterial(uiKey);
                                }
                            }
                            if (eMaterialB == aeMaterial && pCol != pLastCol) {
                                pLastCol = pCol;
                                uint32_t uiFilterInfo = pCol->getCollisionFilterInfo();
                                uint32_t uiGroup = CFilter(uiFilterInfo).GetGroup();
                                bool bCasterHitLandHeightField = uiGroup == 1;
                                NiAVObject* pAVObj = nullptr;
                                if (uiGroup == 1) {
                                    hkpRigidBody* pRB = hkpGetRigidBody(pCol);
                                    if (pRB) {
                                        // The property is keyed by NiTriStrips' RTTI address; its value is the scene
                                        // object's pointer.
                                        hkpPropertyValue* pProperty = bhkWorldObject::GetProperty(
                                            &kPropertyValue,
                                            pRB,
                                            bhkPropertyKey(NiTriStrips::ms_RTTI)
                                        );
                                        pAVObj = static_cast<NiAVObject*>(pProperty->getPtr());
                                        if (pAVObj && pAVObj->m_pkParent) {
                                            pAVObj = pAVObj->m_pkParent;
                                        }
                                    }
                                }
                                else {
                                    pAVObj = bhkUtilFunctions::GetNiAVObject(pCol);
                                }
                                TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAVObj);
                                if (bCasterHitLandHeightField ||
                                    (pRef &&
                                     BGSDecalManager::IsValidDecalImpact(pRef, BGSDecalManager::DECAL_TYPE_SIMPLE))) {
                                    TESObjectCELL* pDecalCell = TES::pInstance->GetCellForPoint(aContactPoint);
                                    DECAL_CREATION_DATA kData;
                                    kData.pOrigin = origin;
                                    kData.pDirection = dir;
                                    kData.fDecalWidth = fDecalSize;
                                    kData.fDecalHeight = fDecalSize;
                                    kData.fDecalDepth = pImpactData->GetDecalDepth();
                                    kData.pAVObj = pAVObj;
                                    kData.pTexSet = pSet;
                                    kData.fRotation = fRotation;
                                    kData.cSubTex = cSubTex;
                                    kData.bAlphaBlending = pImpactData->GetIsAlphaBlend();
                                    kData.bAlphaTesting = pImpactData->GetIsAlphaTest();
                                    kData.bUseParallaxOcclusion = pImpactData->GetIsParallax();
                                    kData.fParallaxOcclusionScale = pImpactData->GetDecalParallaxScale();
                                    kData.ucParallaxOcclusionMaxPasses = pImpactData->GetParallaxPasses();
                                    kData.fSpecular = pImpactData->GetDecalShininess();
                                    kData.fDecalEpsilon = pImpactData->Data.fAngleThreshold;
                                    float fBlue = static_cast<double>(
                                                      static_cast<uint8_t>(pImpactData->GetDecalColor() >> 16)
                                                  ) /
                                        255.0;
                                    float fGreen = static_cast<double>(static_cast<uint8_t>(
                                                       static_cast<uint16_t>(pImpactData->GetDecalColor()) >> 8
                                                   )) /
                                        255.0;
                                    float fRed = static_cast<double>(
                                                     static_cast<uint8_t>(pImpactData->GetDecalColor())
                                                 ) /
                                        255.0;
                                    kData.fVertexColor = NiColor(fRed, fGreen, fBlue);
                                    if (eMaterialB == BHK_MATERIAL_GLASS) {
                                        kData.bTwoSided = true;
                                    }
                                    pDecalCell->AddDecal(kData, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
                                }
                            }
                        }
                    }
                    if (iHits > 0) {
                        fDecalSize = 0.0f;
                    }
                }
            }
            if (bCreateParticles) {
                pFilename = pImpactData->GetModel();
                if (pFilename && *pFilename) {
                    eOrient = pImpactData->Data.eOrient;
                    if (eOrient == BGSImpactData::ORIENT_PROJ_VECTOR) {
                        kImpactEffectVector = -MovementDirection;
                        kImpactEffectVector.Unitize();
                    }
                    else if (eOrient == BGSImpactData::ORIENT_PROJ_REFLECT) {
                        NiPoint3 projectileVec = -MovementDirection;
                        projectileVec.Unitize();
                        NiPoint3 normalVec = aContactNormal;
                        normalVec.Unitize();
                        float fDot = normalVec.Dot(projectileVec) * 2.0;
                        kImpactEffectVector = fDot * normalVec - projectileVec;
                    }
                    else {
                        kImpactEffectVector = aContactNormal;
                        kImpactEffectVector.Unitize();
                    }
                    kEffectPosition = aContactPoint;
                    kEffectVector = kImpactEffectVector;
                    fLifetime = pImpactData->Data.fEffectDuration;
                    BSTempEffectParticle::Spawn(
                        GetParentCell(),
                        fLifetime,
                        pFilename,
                        kEffectVector,
                        kEffectPosition,
                        1.0f,
                        7u,
                        nullptr
                    );
                }
            }
            ActorCause* pCause = GetActorCause();
            if (pCause && pCause->pActor) {
                Actor* pAct = pCause->pActor;
                if (pAct->pCurrentProcess) {
                    BaseProcess* pProcess = pAct->pCurrentProcess;
                    int32_t iSoundLevel = pImpactData->GetSoundLevelValue();
                    pProcess->SetActorsDetectionEvent(pAct, aContactPoint, iSoundLevel, 0, nullptr);
                }
            }

            TESSound* pImpactSound1 = pImpactData->pSound1;
            if (pImpactSound1) {
                const char* pSoundFilename = pImpactSound1->GetFilename();
                BSSoundHandle soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                    pSoundFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pImpactSound1
                );
                soundHandle.SetPosition(aContactPoint.x, aContactPoint.y, aContactPoint.z);
                soundHandle.Play(false);
            }
            TESSound* pImpactSound2 = pImpactData->pSound2;
            if (pImpactSound2) {
                const char* pSoundFilename = pImpactSound2->GetFilename();
                BSSoundHandle soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                    pSoundFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pImpactSound2
                );
                soundHandle.SetPosition(aContactPoint.x, aContactPoint.y, aContactPoint.z);
                soundHandle.Play(false);
            }
        }
    }
}

void Projectile::SpawnMuzzleFlash() {
    if (GetProjectileBase()->GetHasMuzzleFlash() && GetProjectileBase()->GetMuzzleFlashModel().GetModelLength()) {
        Actor* pActor;
        TESObjectREFR* pRef = GetShooter();
        if (pRef && pRef->IsActor()) {
            pActor = static_cast<Actor*>(pRef);
        }
        else {
            pActor = nullptr;
        }
        if (pRef && !pRef->IsDead(false) && !pRef->IsKnockedOut() && (!pActor || !pActor->IsReloading())) {
            MuzzleFlash::SpawnMuzzleFlash(pRef, this);
        }
    }
}

bool Projectile::GetMineReactsToTarget(Actor* apTarget, Actor* apShooterActor, TESFaction* apOwnerFaction) {
    bool bConsiderShooter = iProjectileMineShooterCanTrigger.Int() > 0;
    bool bCrimeCaused = false;
    if ((bConsiderShooter || apTarget != GetShooter()) &&
        !apTarget->IsDead(false) &&
        (!apOwnerFaction || apTarget->GetFactionRank(apOwnerFaction, true) == -1)) {
        FIGHT_REACTION eFactionFight;
        if (apShooterActor) {
            eFactionFight = apShooterActor->GetFactionFightReaction(apTarget, bCrimeCaused);
        }
        else {
            eFactionFight = FIGHT_REACTION_NEUTRAL;
        }
        switch (eFactionFight) {
            case FIGHT_REACTION_NEUTRAL:
            case FIGHT_REACTION_ENEMY: return true;
            default: break;
        }
    }
    return false;
}

bool Projectile::CheckExplosionProximity() {
    float fTriggerChance;
    float fProximity = GetProjectileBase()->Data.fExplosionProximity;
    if (GetParentCell() && !GetParentCell()->GetInterior()) {
        fProximity = fProximity * fMineExteriorRadiusMult.Float();
    }

    float fSqrProximity = fProximity * fProximity;
    MobileObject* pMob = nullptr;
    bool bRetVal = false;
    Actor* pTestFightActor = nullptr;
    TESFaction* pOwnerFaction = nullptr;
    if (pShooter && pShooter->IsActor()) {
        pTestFightActor = static_cast<Actor*>(pShooter);
    }
    else {
        TESForm* pOwner = GetOwner();
        if (pOwner) {
            ENUM_FORM_ID eOwnerType = pOwner->GetFormType();
            if (eOwnerType == FACT_ID) {
                pOwnerFaction = static_cast<TESFaction*>(pOwner);
            }
        }
    }
    for (uint32_t i = 0; i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
        pMob = ReferenceProcessLists.MobProcessArray.GetItem(i);
        Actor* pActor;
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        else {
            pActor = nullptr;
        }
        if (pActor && !pActor->DoesFly() && !pActor->IsChild(false) && !pActor->GetGhost()) {
            const NiPoint3& kPos = GetLocationOnReference();
            const NiPoint3& kMobPos = pMob->GetLocationOnReference();
            float fSqrDistance = (kMobPos - kPos).SqrLength();
            if (fSqrProximity > static_cast<double>(fSqrDistance) &&
                GetMineReactsToTarget(pActor, pTestFightActor, pOwnerFaction)) {
                bRetVal = true;
                break;
            }
        }
    }
    if (!IsFlagSet(Projectile::NO_PLAYER_PROXIMITY) && !bRetVal) {
        pMob = pPlayer;
        const NiPoint3& kPos = GetLocationOnReference();
        const NiPoint3& kMobPos = pMob->GetLocationOnReference();
        float fSqrDistance = (kMobPos - kPos).SqrLength();
        if (fSqrProximity > static_cast<double>(fSqrDistance) &&
            GetMineReactsToTarget(pPlayer, pTestFightActor, pOwnerFaction)) {
            fTriggerChance = 100.0f;
            BGSProjectile* pProjectileBase = GetProjectileBase();
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::CALCULATE_MINE_EXPLODE_CHANCE,
                pPlayer,
                pProjectileBase,
                &fTriggerChance
            );
            if (Random() % 100 >= static_cast<uint32_t>(static_cast<int64_t>(fTriggerChance))) {
                SetFlag(Projectile::NO_PLAYER_PROXIMITY, true);
            }
            else {
                bRetVal = true;
            }
        }
    }
    if (bRetVal && pMob) {
        if (pMob->IsNpc() || pMob == pPlayer) {
            double dAltTimer = GetProjectileBase()->GetAltTimer();
            // Only actors pass the test above, so pMob is treated as an Actor to reach its ActorValueOwner.
            ActorValueOwner* pValueOwner = static_cast<Actor*>(pMob);
            fExplosionTimer = static_cast<double>(pValueOwner->GetClampedActorValue(ActorValue::EXPLOSIVES)) *
                    dAltTimer / 100.0 +
                fMinesDelayMin.Float();
        }
        else {
            fExplosionTimer = fMinesDelayMin.Float();
        }
    }

    return bRetVal;
}

void Projectile::AttachLight() {
    if (Get3D() && GetProjectileBase()->Data.pLight && !GetDelete()) {
        NiNode* p3D = Get3D();
        TESObjectLIGH* pLight = GetProjectileBase()->Data.pLight;
        spLight = pLight->GenDynamic(this, p3D, false);
    }
}

Projectile::ImpactData* Projectile::GetImpactData(TESObjectREFR* apRef) {
    Projectile::ImpactData* pFound = nullptr;
    BSSimpleList<Projectile::ImpactData*>* pList = &ImpactList;
    while (!pFound && pList && !pList->IsEmpty()) {
        Projectile::ImpactData* pData = pList->m_item;
        pList = pList->m_pkNext;
        if (pData->pCollidee == apRef) {
            pFound = pData;
        }
    }
    return pFound;
}

void Projectile::CheckRefDeleted(TESObjectREFR* apDeletedRef) {
    if (GetDesiredTarget() == apDeletedRef) {
        pDesiredTarget = nullptr;
        if (IsFlagSet(Projectile::ACTIVE_IN_INVENTORY)) {
            Kill();
        }
    }
    if (!ImpactList.IsEmpty()) {
        bool bFound = false;
        BSSimpleList<Projectile::ImpactData*>* pImpactList = &ImpactList;
        while (!bFound && pImpactList && !pImpactList->IsEmpty()) {
            Projectile::ImpactData* pData = pImpactList->m_item;
            pImpactList = pImpactList->m_pkNext;
            if (pData->pCollidee == apDeletedRef) {
                Kill();
                bFound = true;
            }
        }
    }
    if (GetShooter() == apDeletedRef) {
        Kill();
    }
    if (pTarget && pTarget->pReference == apDeletedRef) {
        delete pTarget;
        pTarget = nullptr;
    }
}

void Projectile::StartCountdownSound() {
    if (!sndCountdown.IsValid() && GetProjectileBase()->Data.pCountdownSound) {
        int32_t iSoundID = GetProjectileBase()->Data.pCountdownSound->iFormID;
        sndCountdown = PlaySound(
            iSoundID,
            false,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
}

bool Projectile::FindBlastNodes(NiAVObject* apRoot, NiPoint3& aBlastPosition) {
    if (!apRoot) {
        return false;
    }
    bool bFound = false;
    if (NiObject::IsKindOf_0(&BSBlastNode::ms_RTTI, apRoot)) {
        bFound = true;
        aBlastPosition = apRoot->GetWorldTranslate();
    }
    NiNode* pNode = apRoot->IsNode();
    if (pNode) {
        for (uint32_t i = 0; !bFound && i < pNode->GetArrayCount(); ++i) {
            NiAVObject* pChild = pNode->GetAt(i);
            bFound = FindBlastNodes(pChild, aBlastPosition);
        }
    }
    return bFound;
}

void Projectile::GetConeEndpoints(
    NiPoint3* apPoints,
    int32_t aiCount,
    NiPoint3 aOrigin,
    float afHeading,
    float afLooking,
    float afConeAngle,
    float afRange,
    float afRotation
) {
    for (int32_t i = 0; i < aiCount; ++i) {
        float fAngle = static_cast<double>(i) / static_cast<double>(aiCount) * 6.2831854820251465 + afRotation;
        double dSinAngle = std::sin(fAngle);
        float fLooking = dSinAngle * afConeAngle + afLooking;
        double dCosAngle = std::cos(fAngle);
        float fHeading = dCosAngle * afConeAngle + afHeading;
        float fCosLooking = std::cos(fLooking);
        float fSinLooking = std::sin(fLooking);
        float fCosHeading = std::cos(fHeading);
        float fSinHeading = std::sin(fHeading);
        float fZ = -fSinLooking;
        float fY = fCosHeading * fCosLooking;
        float fX = fSinHeading * fCosLooking;
        NiPoint3 kDirection(fX, fY, fZ);
        apPoints[i] = aOrigin + kDirection * afRange;
    }
}

// The high level of the object process array, scanned in order until a match.
Projectile* Projectile::FindExistingProjectile(TESObjectREFR* apShooter, BGSProjectile* apBase) {
    Projectile* pFound = nullptr;
    for (uint32_t i = 0; !pFound; ++i) {
        if (i >= ReferenceProcessLists.GetProcessArray()->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
            break;
        }
        MobileObject* pObject = ReferenceProcessLists.GetProcessArray()->GetItem(i);
        if (pObject && pObject->IsProjectile()) {
            Projectile* pProjectile = static_cast<Projectile*>(pObject);
            if (pProjectile->GetProjectileBase() == apBase && pProjectile->GetShooter() == apShooter) {
                pFound = pProjectile;
            }
        }
    }
    return pFound;
}

Projectile* Projectile::Launch(
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    CombatController* apShooterCombatController,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle,
    ProjectilePickCache* apPickCache,
    TESObjectREFR* apHomingTarget,
    bool abAlwaysHit,
    bool abDoNoDamage,
    float afSpreadHeading,
    float afSpreadLooking,
    TESObjectCELL* apParentCell
) {
    NiPoint3 kLineEnd;
    NiPoint3 kOffset;
    NiPoint3 kForward;
    float fEntrySpeed;
    Projectile* pProjectile = nullptr;
    bool bInitialize = true;
    if (apProjectileBase) {
        BGSProjectile::BGSProjectileFlags eMotionType = apProjectileBase->GetMotionType();
        if (eMotionType > BGSProjectile::MOTION_BEAM) {
            if (eMotionType == BGSProjectile::MOTION_FLAMETHROWER) {
                pProjectile = Projectile::FindExistingProjectile(apShooter, apProjectileBase);
                if (pProjectile) {
                    FlameProjectile* pFlame = static_cast<FlameProjectile*>(pProjectile);
                    pFlame->ResetExpirationTimer();
                    bInitialize = false;
                }
                else {
                    pProjectile = new FlameProjectile(
                        apProjectileBase,
                        apShooter,
                        apFromWeapon,
                        aOrigin,
                        afZAngle,
                        afXAngle
                    );
                }
            }
            else if (eMotionType == BGSProjectile::MOTION_CONTINUOUS_BEAM) {
                pProjectile = new ContinuousBeamProjectile(
                    apProjectileBase,
                    apShooter,
                    apFromWeapon,
                    aOrigin,
                    afZAngle,
                    afXAngle
                );
            }
        }
        else {
            switch (eMotionType) {
                case BGSProjectile::MOTION_BEAM: {
                    pProjectile = new BeamProjectile(
                        apProjectileBase,
                        apShooter,
                        apFromWeapon,
                        aOrigin,
                        afZAngle,
                        afXAngle
                    );
                    break;
                }
                case BGSProjectile::MOTION_MISSILE: {
                    pProjectile = new MissileProjectile(
                        apProjectileBase,
                        apShooter,
                        apFromWeapon,
                        aOrigin,
                        afZAngle,
                        afXAngle
                    );
                    break;
                }
                case BGSProjectile::MOTION_GRENADE: {
                    pProjectile = new GrenadeProjectile(
                        apProjectileBase,
                        apShooter,
                        apFromWeapon,
                        aOrigin,
                        afZAngle,
                        afXAngle
                    );
                    break;
                }
            }
        }
    }
    if (apShooter == pPlayer && apFromWeapon) {
        ItemChange* pItem = pPlayer->pCurrentProcess->GetCurrentWeapon();
        if (pItem && pItem->HasModEffectActive(WEAPON_MOD_PROJECTILE_SPEED)) {
            double dSpeedMult = pProjectile->fSpeedMult;
            float fNewSpeedMult = apFromWeapon->GetModEffectValue(WEAPON_MOD_PROJECTILE_SPEED, 0) * dSpeedMult;
            pProjectile->SetSpeedMult(fNewSpeedMult);
        }

        if (apFromWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) {
            int32_t iStrReq = apFromWeapon->GetStrRequired();
            int32_t iPlayerStr = pPlayer->GetActorValue(ActorValue::STRENGTH);
            float fStrengthMult = fThrowingStrengthPenalty.Float() * static_cast<double>(iPlayerStr - iStrReq) + 1.0;
            float fNewSpeedMult = pProjectile->fSpeedMult * fStrengthMult;
            pProjectile->SetSpeedMult(fNewSpeedMult);
        }
    }

    if (pProjectile && apShooter && apShooter->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apShooter);
        fEntrySpeed = 1.0;
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::THROWING_VELOCITY_MODIFIER,
            static_cast<Actor*>(apShooter),
            apFromWeapon,
            &fEntrySpeed
        );
        if (fEntrySpeed != 1.0) {
            double dSpeedMult = pProjectile->fSpeedMult;
            float fNewSpeedMult = dSpeedMult * fEntrySpeed;
            pProjectile->SetSpeedMult(fNewSpeedMult);
        }
        if (pActor->GetActorValue(ActorValue::TURBO) > 0) {
            double dSpeedMult = pProjectile->fSpeedMult;
            float fNewSpeedMult = 1.0 / BSTimer::QGlobalTimeMultiplier() * dSpeedMult;
            pProjectile->SetSpeedMult(fNewSpeedMult);
        }
    }

    if (pProjectile) {
        pProjectile->SetAngularMomentumZ(afSpreadHeading);
        pProjectile->SetAngularMomentumX(afSpreadLooking);
    }
    if (pProjectile) {
        pProjectile->SetFlag(Projectile::FAKE_SHOT, abDoNoDamage);
    }
    if (pProjectile && bInitialize) {
        if (apShooter && apShooter->IsActor()) {
            ActorCause* pActorCause;
            pActorCause = new ActorCause(static_cast<Actor*>(apShooter));
            pProjectile->SetActorCause(pActorCause);
        }

        pProjectile->SetPickCache(apPickCache);
        if (apShooter == pPlayer) {
            TESFaction* pFaction = static_cast<TESFaction*>(
                BGSDefaultObjectManager::GetDefaultObject(DO_PLAYER_FACTION)
            );
            if (pFaction) {
                pProjectile->SetOwner(pFaction);
            }
            if (apFromWeapon->IsThrownWeapon()) {
                float fPower = pPlayer->GetTimeGrenadeHeld() * fArrowBowTimerMult.Float() + fArrowBowTimerBase.Float();
                pProjectile->SetPower(BSMIN(1.0f, fPower));
            }
        }
        if (apShooterCombatController) {
            apShooterCombatController->FiredProjectile(pProjectile);
        }
        if (apHomingTarget) {
            pProjectile->pDesiredTarget = apHomingTarget;
        }
        if (abAlwaysHit) {
            pProjectile->SetFlag(Projectile::ALWAYS_HIT_TARGET, true);
        }
        if (apParentCell) {
            pProjectile->SetParentCell(apParentCell);
        }
        pProjectile->Initialize();
    }

    if (pProjectile && apProjectileBase && apProjectileBase->Data.pActiveSoundLoop) {
        if (!apProjectileBase->GetSupersonic() &&
            (!apProjectileBase->GetHasExplosionAltTrigger() || apProjectileBase->Data.fExplosionProximity <= 0.0)) {
            TESSound* pSound = apProjectileBase->Data.pActiveSoundLoop;
            const char* pFilename = apProjectileBase->Data.pActiveSoundLoop->GetFilename();
            pProjectile->sndHandle = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, 0x40000012u, pSound);
            const NiPoint3& kShooterPos = apShooter->GetLocationOnReference();
            pProjectile->sndHandle.SetPosition(kShooterPos);
            NiNode* p3D = pProjectile->Get3D();
            pProjectile->sndHandle.SetObjectToFollow(p3D);
            uint8_t ucMaxDistance = apProjectileBase->Data.pActiveSoundLoop->GetMaxDistance();
            pProjectile->fSoundDist = static_cast<float>(100 * ucMaxDistance);
        }
        if (pProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
            pProjectile->sndHandle.Play(true);
            pProjectile->bSoundFired = true;
        }
    }
    else if (pProjectile && pProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
        pProjectile->bSoundFired = true;
    }
    if (bIsProjectileDebug && pProjectile && pProjectile->Get3D()) {
        NiMatrix3 kRot;
        NiColorA color(1.0f, 0.0f, 0.0f, 0.6f);
        NiAVObject* pDebug = MakeTriPoint(4.0f, color, true);
        pDebug->UpdateProperties();
        pDebug->SetTranslate(pProjectile->Get3D()->GetWorldTranslate());
        TES::pInstance->AddTempDebugObject(pDebug, fProjectileDebugDuration.Float());
        kRot.MakeZRotation(pProjectile->GetHeading(false));
        NiColorA kLineColor(1.0f, 1.0f, 0.0f, 0.6f);
        color = kLineColor;
        ::new (&kForward) NiPoint3(0.0f, 1000.0f, 0.0f);
        ::new (&kOffset) NiPoint3(kRot * kForward);
        ::new (&kLineEnd) NiPoint3(pProjectile->Get3D()->GetWorldTranslate() + kOffset);
        pDebug = MakeLine(pProjectile->Get3D()->GetWorldTranslate(), color, kLineEnd, color, true);
        TES::pInstance->AddTempDebugObject(pDebug, fProjectileDebugDuration.Float());
    }

    if (pProjectile) {
        pCombatManager->pThreatMap->AddProjectile(pProjectile);
    }
    return pProjectile;
}

// A blend collision object out of the world, or blended fully, takes its object's world transform (offset by a
// bhkRigidBodyT's local transform); any other body asks its Havok body. The ray is taken into the body's space and cast
// against its shape.
bool Projectile::PickSceneGraph(bhkPickData* apPickData, NiAVObject* apObject, bool abIgnoreHead, bool abSort) {
    bool bHit = false;
    if (apObject) {
        bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
        if (pCollisionObject) {
            bhkWorldObject* pWorldObject = pCollisionObject->GetWorldObject();
            bhkShape* pShape;
            if (pWorldObject) {
                pShape = pWorldObject->GetShape();
            }
            else {
                pShape = nullptr;
            }
            // GetCollisionFilter writes the filter word into the CFilter and returns its address, and the CFilter
            // member is called on it.
            CFilter kFilter;
            alignas(16) CFilter kHeadFilter;
            if (pShape &&
                (pWorldObject->GetCollisionFilter(&kFilter), kFilter.QBipLayer()) &&
                (!abIgnoreHead ||
                 (pWorldObject->GetCollisionFilter(&kHeadFilter), kHeadFilter.GetBipedPart()) != P_HEAD)) {
                alignas(16) hkTransform kTransform;
                bhkRigidBody* pBody = static_cast<bhkRigidBody*>(pWorldObject);
                if (NiObject::IsKindOf_0(&bhkBlendCollisionObject::ms_RTTI, pCollisionObject)) {
                    bhkBlendCollisionObject* pBlend = static_cast<bhkBlendCollisionObject*>(pCollisionObject);
                    if (!pWorldObject->IsAddedToWorld() || pBlend->GetHeirGain() == 1.0) {
                        NI2HK(kTransform, apObject->GetWorldTransform());
                        if (NiObject::IsKindOf_0(&bhkRigidBodyT::ms_RTTI, pWorldObject)) {
                            bhkRigidBodyT* pBodyT = static_cast<bhkRigidBodyT*>(pWorldObject);
                            alignas(16) hkTransform kLocal;
                            alignas(16) hkTransform kWorld(kTransform);
                            const hkVector4& rLocalPos = pBodyT->GetLocalPos();
                            const hkQuaternion& rLocalRot = pBodyT->GetLocalRot();
                            kLocal.set(rLocalRot, rLocalPos);
                            kTransform.setMul(kWorld, kLocal);
                        }
                    }
                    else {
                        pBody->GetTransform(kTransform);
                    }
                }
                else {
                    pBody->GetTransform(kTransform);
                }
                hkpShapeRayCastInput kInput;
                hkpShapeRayCastOutput kOutput;
                kInput.m_from.setTransformedInversePos(kTransform, *apPickData->GethkFrom());
                kInput.m_to.setTransformedInversePos(kTransform, apPickData->GethkTo());
                (**pShape)->castRay(kInput, kOutput);
                if (kOutput.hasHit()) {
                    hkpRayHitCollector* pCollector = apPickData->GetAllCollector();
                    const hkpCollidable* pCollidable = (**pWorldObject)->getCollidable();
                    pCollector->addRayHit(*pCollidable, kOutput);
                    bHit = true;
                }
            }
        }
        NiNode* pNode = apObject->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                bHit |= PickSceneGraph(apPickData, pChild, abIgnoreHead, false);
            }
        }
    }
    if (abSort) {
        apPickData->GetAllCollector()->sortHits();
    }
    return bHit;
}

bool Projectile::IsValidImpact(
    TESObjectREFR* apCollidee,
    const hkpCollidable* apTargetCollidable,
    Projectile* apProjectile,
    BGSProjectile* apProjectileBase
) {
    CFilter kBodyFilter;
    CFilter kPhantomFilter;
    hkpPhantom* pPhantom = apTargetCollidable ? hkpGetPhantom(apTargetCollidable) : nullptr;
    bhkPhantom* pBP = pPhantom ? bhkPhantom::Getbhk(pPhantom) : nullptr;
    if (pBP) {
        if (NiObject::IsKindOf_0(&bhkCollisionBox::ms_RTTI, pBP)) {
            return false;
        }
        if (pBP->GetCollisionFilter(&kPhantomFilter)->GetLayer() != L_CHARCONTROLLER) {
            return false;
        }
        NiAVObject* pAV;
        if (apCollidee) {
            pAV = apCollidee->Get3D();
        }
        else {
            pAV = nullptr;
        }
        if (apCollidee == pPlayer) {
            pAV = pPlayer->Get3D(false);
        }
        bhkCollisionObject* pCol = FindFirstCollisionObject(pAV);
        bhkRigidBody* pFirstBody = pCol ? pCol->GetRigidBody() : nullptr;
        if (pFirstBody) {
            if (!pFirstBody->GetCollisionFilter(&kBodyFilter)->GetNoCollision() && pFirstBody->GethkWorld()) {
                return false;
            }
        }
    }
    if (apProjectileBase && apTargetCollidable) {
        uint32_t uiFilterInfo = apTargetCollidable->getCollisionFilterInfo();
        if (CFilter(uiFilterInfo).GetLayer() == L_TRANSPARENT_SMALL ||
            CFilter(apTargetCollidable->getCollisionFilterInfo()).GetLayer() == L_TRANSPARENT_SMALL_ANIM) {
            BGSProjectile::BGSProjectileFlags eMotion = apProjectileBase->GetMotionType();
            if (eMotion != BGSProjectile::MOTION_GRENADE &&
                (eMotion != BGSProjectile::MOTION_MISSILE ||
                 apProjectileBase->GetHitScan() ||
                 apProjectileBase->GetPassThroughSmTransp())) {
                return false;
            }
        }
    }

    return true;
}

NiAVObject* Projectile::ProcessTargetPick(
    bhkPickData* apPickData,
    Projectile* apProjectile,
    BGSProjectile* apProjectileBase,
    float* apHitFraction,
    bool bUseCharacterController,
    float afWaterHitRatio
) {
    CFilter kPickFilter;
    CFilter kBodyFilter;
    CFilter kCharFilter;
    alignas(16) CFilter kGasTrapFilter;
    CFilter kZoneFilter;
    if (!apProjectileBase && apProjectile) {
        apProjectileBase = apProjectile->GetProjectileBase();
    }

    bool bHit = false;
    NiAVObject* pHitObject = nullptr;
    if (apPickData && apPickData->pAllCollector) {
        NiPoint3 pickLength;
        NiPoint3 pickOrigin;
        apPickData->GetFrom(pickOrigin);
        apPickData->GetLength(pickLength);
        char cPhantomHit = 0;
        TESObjectREFR* pTargetRef = nullptr;
        for (int32_t i = 0; !bHit && i < apPickData->pAllCollector->getHits().m_size; ++i) {
            hkpWorldRayCastOutput point(apPickData->pAllCollector->getHits()[i]);
            if (!(afWaterHitRatio < static_cast<double>(point.m_hitFraction))) {
                NiAVObject* pAV = GetAVObjectForCollidable(point.m_rootCollidable);
                TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAV);
                if (pAV) {
                    char cValid = 1;
                    hkpPhantom* pPhantom = hkpGetPhantom(point.m_rootCollidable);
                    bhkPhantom* pBP = pPhantom ? bhkPhantom::Getbhk(pPhantom) : nullptr;
                    if (pBP) {
                        cValid = 0;
                        if (apProjectile && pRef) {
                            if (pBP->GetCollisionFilter(&kZoneFilter)->GetLayer() == L_PROJECTILEZONE ||
                                pBP->GetCollisionFilter(&kGasTrapFilter)->GetLayer() == L_GASTRAP) {
                                Script::SetActionFlag(apProjectile, &pRef->m_Extra, SCRIPT_ACTION_ONHIT);
                                if (apProjectile->pWeaponSource) {
                                    Script::SetActionFlag(
                                        apProjectile->pWeaponSource,
                                        &pRef->m_Extra,
                                        SCRIPT_ACTION_ONHITWITH
                                    );
                                }
                            }
                        }

                        if (NiObject::IsKindOf_0(&bhkCollisionBox::ms_RTTI, pBP) ||
                            (bUseCharacterController &&
                             pBP->GetCollisionFilter(&kCharFilter)->GetLayer() == L_CHARCONTROLLER)) {
                            cPhantomHit = 1;
                            pTargetRef = pRef;
                            bhkCollisionObject* pCol = FindFirstCollisionObject(pAV);
                            bhkRigidBody* pFirstBody = pCol ? pCol->GetRigidBody() : nullptr;
                            if (pFirstBody) {
                                if (pFirstBody->GetCollisionFilter(&kBodyFilter)->GetNoCollision() ||
                                    !pFirstBody->GethkWorld()) {
                                    NiPoint3 length;
                                    NiPoint3 from;
                                    bhkPickData pd;
                                    apPickData->GetFrom(from);
                                    apPickData->GetLength(length);
                                    pickLength *= 2.0f;
                                    length *= 2.0f;
                                    pd.SetFilter(*apPickData->GetFilter(&kPickFilter));
                                    pd.NiSetFrom(from);
                                    pd.NiSetLength(length);
                                    hkpAllRayHitCollector collector;
                                    pd.SetAllCollector(&collector);
                                    if (Projectile::PickSceneGraph(&pd, pAV, false, true) &&
                                        pd.pAllCollector->getHits().m_size > 0) {
                                        point = pd.pAllCollector->getHits()[0];
                                        cValid = 1;
                                    }
                                }
                            }
                        }
                    }
                    if (!Projectile::IsValidImpact(pRef, point.m_rootCollidable, apProjectile, apProjectileBase)) {
                        cValid = 0;
                    }
                    if (cValid) {
                        if (apProjectile) {
                            NiPoint3 normal;
                            alignas(16) NiPoint3 kHitLength = pickLength * point.m_hitFraction;
                            NiPoint3 contact = pickOrigin + kHitLength;
                            HK2NINS(normal, point.m_normal);
                            if (apProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
                                float fRange = kHitLength.Length();
                                apProjectile->SetRange(fRange);
                                apProjectile->SetMovementDelta(kHitLength, false);
                            }
                            uint32_t outputShapeKey = -1;
                            for (int32_t j = 0; j < 8 && point.m_shapeKeys[j] != -1; ++j) {
                                outputShapeKey = point.m_shapeKeys[j];
                            }
                            bHit = apProjectile->AddImpact(
                                pRef,
                                contact,
                                normal,
                                point.m_rootCollidable,
                                outputShapeKey,
                                BHK_MATERIAL_MAX
                            );
                            if (bHit) {
                                pHitObject = pAV;
                            }
                        }
                        else {
                            bHit = true;
                            pHitObject = pAV;
                        }
                        if (bHit && apHitFraction) {
                            *apHitFraction = point.m_hitFraction;
                        }
                    }
                }
            }
        }
        if (apProjectile) {
            if (!bHit && afWaterHitRatio != 3.4028234663852886e+38) {
                NiPoint3 kWaterLength = pickLength * afWaterHitRatio;
                NiPoint3 contact = pickOrigin + kWaterLength;
                if (apProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
                    float fRange = kWaterLength.Length();
                    apProjectile->SetRange(fRange);
                    apProjectile->SetMovementDelta(kWaterLength, false);
                }
                apProjectile
                    ->AddImpact(nullptr, contact, NiPoint3::UNIT_Z, nullptr, HK_INVALID_SHAPE_KEY, BHK_MATERIAL_WATER);
            }
            if (bHit || !cPhantomHit) {
                if (apProjectile->GetProjectileBase()->Data.pActiveSoundLoop &&
                    ((bHit && pTargetRef != pPlayer) || !bHit) &&
                    apProjectile->GetShooter() != pPlayer &&
                    BSAudio::QInstance().pListener) {
                    alignas(16) NiPoint3 toPt;
                    NiPoint3 fromPt;
                    NiPoint3 listenerPos = BSAudio::QInstance().pListener->GetPosition();
                    if (pTargetRef) {
                        toPt = pTargetRef->GetLocationOnReference();
                    }
                    else {
                        apPickData->GetTo(toPt);
                    }
                    apPickData->GetFrom(fromPt);
                    NiPoint3 projPath = toPt - fromPt;
                    alignas(16) NiPoint3 kSourceDiff = listenerPos - fromPt;
                    NiPoint3 toSource = kSourceDiff;
                    float c1 = toSource.Dot(projPath);
                    float c2 = projPath.Dot(projPath);
                    float b = c1 / c2;
                    NiPoint3 pointOnLine = fromPt + b * projPath;
                    alignas(16) NiPoint3 toLine = listenerPos - pointOnLine;
                    float dist = toLine.Length();
                    if (dist < 600.0) {
                        uint32_t uiFlags = 258;
                        TESSound* pSound = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop;
                        const char* pFilename = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop->GetFilename();
                        BSSoundHandle sound = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, uiFlags, pSound);
                        sound.SetPosition(pointOnLine.x, pointOnLine.y, pointOnLine.z);
                        sound.Play(false);
                        if (bIsProjectileDebug) {
                            NiColorA color(0.0f, 1.0f, 1.0f, 0.3f);
                            NiAVObject* pDebug = MakeTriPoint(5.0f, color, true);
                            pDebug->UpdateProperties();
                            pDebug->SetTranslate(pointOnLine);
                            TES::pInstance->AddTempDebugObject(pDebug, fProjectileDebugDuration.Float());
                        }
                    }
                }
            }
            else if (pTargetRef && pTargetRef == pPlayer) {
                NiPoint3 hitPt;
                apPickData->NiCalcHitPoint(hitPt);
                if (apProjectile->GetProjectileBase() && apProjectile->GetProjectileBase()->Data.pActiveSoundLoop) {
                    uint32_t uiFlags = 258;
                    TESSound* pSound = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop;
                    const char* pFilename = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop->GetFilename();
                    BSSoundHandle sound = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, uiFlags, pSound);
                    sound.SetPosition(hitPt.x, hitPt.y, hitPt.z);
                    sound.Play(false);
                    if (bIsProjectileDebug && apProjectile->Get3D()) {
                        NiColorA color(0.0f, 1.0f, 1.0f, 0.3f);
                        NiAVObject* pDebug = MakeTriPoint(5.0f, color, true);
                        pDebug->UpdateProperties();
                        pDebug->SetTranslate(apProjectile->Get3D()->GetWorldTranslate());
                        TES::pInstance->AddTempDebugObject(pDebug, fProjectileDebugDuration.Float());
                    }
                }
            }
        }
    }

    return pHitObject;
}

float Projectile::ComputeSpeed(float afSpeed, float afPower, float afSpeedMult) {
    return static_cast<float>((fArrowWeakSpeed.Float() * (1.0 - afPower) + afSpeed * afPower) * afSpeedMult);
}

void bhkCharacterControllerCinfo::SetCollisionBox(bool abTF) {
    bCollisionBox = abTF;
}

void bhkPickData::GetLength(NiPoint3& arLength) {
    HK2NI(arLength, GethkLength());
}

// The ray's filter, built from its filter info through CFilter's one-field constructor.
CFilter* bhkPickData::GetFilter(CFilter* apResult) {
    ::new (apResult) CFilter(m_filterInfo);
    return apResult;
}

ProjectileTarget::ProjectileTarget() : pReference(nullptr), spPart(nullptr), kOffset(NiPoint3::ZERO) {}

ProjectileTarget::~ProjectileTarget() {}

NiPoint3 ProjectileTarget::GetWorldLocation() {
    NiPoint3 kLocation = NiPoint3::ZERO;
    if (spPart.m_pObject) {
        kLocation = spPart->GetWorldTransform() * kOffset;
    }
    else if (pReference) {
        const NiPoint3* pCenter;
        if (pReference->Get3D()) {
            pCenter = &pReference->Get3D()->GetWorldBound().m_kCenter;
        }
        else {
            pCenter = &pReference->GetLocationOnReference();
        }
        kLocation = *pCenter;
    }
    return kLocation;
}

void ProjectileTarget::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkpWorldRayCastOutput& hkpWorldRayCastOutput::operator=(const hkpWorldRayCastOutput& __that) {
    hkpShapeRayCastOutput::operator=(__that);
    m_rootCollidable = __that.m_rootCollidable;
    return *this;
}

hkpShapeRayCastCollectorOutput&
hkpShapeRayCastCollectorOutput::operator=(const hkpShapeRayCastCollectorOutput& arOther) {
    m_normal = arOther.m_normal;
    m_hitFraction = arOther.m_hitFraction;
    m_extraInfo = arOther.m_extraInfo;
    for (uint32_t i = 0; i < 2; ++i) {
        m_pad[i] = arOther.m_pad[i];
    }
    return *this;
}

hkpShapeRayCastOutput& hkpShapeRayCastOutput::operator=(const hkpShapeRayCastOutput& __that) {
    hkpShapeRayCastCollectorOutput::operator=(__that);
    for (uint32_t i = 0; i < MAX_HIERARCHY_DEPTH; ++i) {
        m_shapeKeys[i] = __that.m_shapeKeys[i];
    }
    m_shapeKeyIndex = __that.m_shapeKeyIndex;
    return *this;
}
