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
#include "BSHavok/bhkRagdollController.h"
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

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bProjectileDebug("bProjectileDebug:Combat", false);
SettingT<INISettingCollection> fProjectileDebugDuration("fProjectileDebugDuration:Combat", 5.0f);
SettingT<INISettingCollection> bVATSProjectileDebug("bVATSProjectileDebug:Combat", false);

// kNormal is deliberately left unset, as the game does: it is compared with NiPoint3_ZERO in place of being stored.
Projectile::ImpactData::ImpactData()
  : pCollidee(nullptr), kLocation(NiPoint3_ZERO), pTargetWorldObject(nullptr), eMaterial(BHK_MATERIAL_MAX),
    eDamageLimb(BGSBodyPart::LIMB_NONE), bProcessed(false), bDoDamage(true), sTargetWorldObjectCount(0),
    sTargetWorldObjectIndex(0) {}

Projectile::ImpactData::~ImpactData() {
    if (pTargetWorldObject) {
        pTargetWorldObject->removeReference();
    }
    pTargetWorldObject = nullptr;
}

void Projectile::ImpactData::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if ((this->sTargetWorldObjectCount) && (this->sTargetWorldObjectIndex != -1) && (pCollidee)) {
        NiNode* v2 = pCollidee->Get3D();
        int16_t scurrentWorldObjectCount = GetHavokWorldObjectCountInSceneGraph(v2);
        if (this->sTargetWorldObjectCount == scurrentWorldObjectCount) {
            if (this->sTargetWorldObjectIndex != -1) {
                int32_t sTargetWorldObjectIndex = this->sTargetWorldObjectIndex;
                NiNode* v3 = pCollidee->Get3D();
                pTargetWorldObject = GetHavokWorldObjectByIndexInSceneGraph(v3, sTargetWorldObjectIndex);
                if (pTargetWorldObject) {
                    pTargetWorldObject->addReference();
                }
            }
        }
        else {
            int32_t sTargetWorldObjectCount = this->sTargetWorldObjectCount;
            int32_t CastingType = pCollidee->iFormID;
            SaveGameWarning(
                "AI: World object count has changed on object %s %08 from %i to %i", pCollidee->GetFormEditorID(),
                CastingType, sTargetWorldObjectCount, scurrentWorldObjectCount
            );
        }
    }
}

void Projectile::ImpactData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&kLocation, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&kNormal, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&eMaterial, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&eDamageLimb, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bProcessed, 1u, 0);
    sTargetWorldObjectCount = 0;
    sTargetWorldObjectIndex = 0;
    if ((pCollidee) && (this->pTargetWorldObject)) {
        NiNode* v2 = pCollidee->Get3D();
        sTargetWorldObjectCount = GetHavokWorldObjectCountInSceneGraph(v2);
        hkpWorldObject* pTargetWorldObject = this->pTargetWorldObject;
        NiNode* v3 = pCollidee->Get3D();
        sTargetWorldObjectIndex = GetHavokWorldObjectIndexInSceneGraph(v3, pTargetWorldObject);
    }

    apSaveGameBuffer->SaveDataEndian(&sTargetWorldObjectCount, 2u, 0);
    apSaveGameBuffer->SaveDataEndian(&sTargetWorldObjectIndex, 2u, 0);
    apSaveGameBuffer->SaveFormID(pCollidee, 0);
}

void Projectile::ImpactData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&kLocation, 12u);
    apLoadGameBuffer->LoadData(&kNormal, 12u);
    apLoadGameBuffer->LoadData(&eMaterial, 4u);
    apLoadGameBuffer->LoadData(&eDamageLimb, 4u);
    apLoadGameBuffer->LoadData(&bProcessed, 1u);
    if (apLoadGameBuffer->GetVersion() >= 17u) {
        apLoadGameBuffer->LoadData(&sTargetWorldObjectCount, 2u);
        apLoadGameBuffer->LoadData(&sTargetWorldObjectIndex, 2u);
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
    fExplosionTimer(3.4028235e+38f), fBlinkTimer(0.0f), fWeaponCondition(1.0f), pWeaponSource(nullptr),
    pShooter(apShooter), pDesiredTarget(nullptr), spLight(nullptr), spPickCache(nullptr), spActorCause(nullptr),
    pTarget(nullptr), pSecondaryItemChange(nullptr), bSoundFired(false), fSoundDist(10000.0f) {
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
                    fWeaponCondition =
                        dItemHealth / double(TESHealthForm::GetFormHealth(pWeaponItem->GetContainerObject()));
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
                float* pfWeightMult = &fRockitDamageBonusWeightMult.Float();
                bool bIsHardcore = pPlayer->IsHardcore();
                float fWeight = TESWeightForm::GetFormWeight(pRockItItem->GetContainerObject(), bIsHardcore);
                float fBonus = BSMAX(fRockitDamageBonusWeightMin.Float(), fWeight) * *pfWeightMult;
                fDamage = BSMAX(0.0f, fBonus) + fDamage;
            }
        }
    }
    iFlags = 0;
    fAge = 0.0f;
    fDistanceMoved = 0.0f;
    MovementDirection = NiPoint3_ZERO;
    spLight = nullptr;
    fDecalSize = 0.0f;
    kFollowOffset.MakeIdentity();
    if (pShooter && pShooter->IsActor() && static_cast<Actor*>(pShooter)->IsInCombat()) {
        SetFlag(SHOOTER_IN_COMBAT, true);
    }
    SetInitialized(false);
}

// The constructor ignores abAdd, as the game does.
Projectile::Projectile([[maybe_unused]] bool abAdd)
  : bImpactProcessed(false), iFlags(0), fPower(1.0f), fSpeedMult(1.0f), fRange(0.0f), fAge(0.0f), fDamage(0.0f),
    fAlpha(1.0f), fExplosionTimer(3.4028235e+38f), fBlinkTimer(0.0f), fSpreadHeading(0.0f), fSpreadLooking(0.0f),
    fWeaponCondition(1.0f), pWeaponSource(nullptr), pShooter(nullptr), pDesiredTarget(nullptr),
    MovementDirection(NiPoint3_ZERO), fDistanceMoved(0.0f), spLight(nullptr), bSendAlarm(false), spPickCache(nullptr),
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
    int32_t iactorCauseID;
    bool bhasSecondaryItemChange;
    int32_t apData;
    MobileObject::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&iFlags, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fPower, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fRange, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fSpeedMult, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAge, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fDamage, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAlpha, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fExplosionTimer, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fBlinkTimer, 4u, 0);
    apSaveGameBuffer->SaveFormID(pWeaponSource, 0);
    apSaveGameBuffer->SaveFormID(pShooter, 0);
    apSaveGameBuffer->SaveFormID(pDesiredTarget, 0);
    apSaveGameBuffer->SaveDataEndian(&MovementDirection, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&fDistanceMoved, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bSendAlarm, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&fDecalSize, 4u, 0);
    apSaveGameBuffer->SaveMatrixAsQuaternion(&kFollowOffset.m_Rotate, 36, 0);
    apSaveGameBuffer->SaveDataEndian(&kFollowOffset.m_Translate, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&kFollowOffset.m_fScale, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fSpreadHeading, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fSpreadLooking, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fWeaponCondition, 4u, 0);
    if (spActorCause.m_pObject) {
        iactorCauseID = spActorCause.m_pObject->iActorCauseID;
    }
    else {
        iactorCauseID = -1;
    }
    apData = iactorCauseID;
    apSaveGameBuffer->SaveDataEndian(&apData, 4u, 0);
    bhasSecondaryItemChange = pSecondaryItemChange != nullptr;
    apSaveGameBuffer->SaveDataEndian(&bhasSecondaryItemChange, 1u, 0);
    if (bhasSecondaryItemChange) {
        pSecondaryItemChange->SaveGame(apSaveGameBuffer);
    }
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (Projectile::ImpactData* pimpact : ImpactList) {
        if (pimpact) {
            pimpact->SaveGame(apSaveGameBuffer);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    apSaveGameBuffer->SaveDataEndian(&bImpactProcessed, 1u, 0);
}

void Projectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool bhasSecondaryItemChange;
    int32_t iactorCauseID;
    MobileObject::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&iFlags, 4u);
    apLoadGameBuffer->LoadData(&fPower, 4u);
    apLoadGameBuffer->LoadData(&fRange, 4u);
    apLoadGameBuffer->LoadData(&fSpeedMult, 4u);
    apLoadGameBuffer->LoadData(&fAge, 4u);
    apLoadGameBuffer->LoadData(&fDamage, 4u);
    apLoadGameBuffer->LoadData(&fAlpha, 4u);
    apLoadGameBuffer->LoadData(&fExplosionTimer, 4u);
    apLoadGameBuffer->LoadData(&fBlinkTimer, 4u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pWeaponSource = dynamic_cast<TESObjectWEAP*>(FormByNumericID);
    apLoadGameBuffer->LoadFormID(pShooter);
    apLoadGameBuffer->LoadFormID(pDesiredTarget);
    apLoadGameBuffer->LoadData(&MovementDirection, 12u);
    apLoadGameBuffer->LoadData(&fDistanceMoved, 4u);
    apLoadGameBuffer->LoadData(&bSendAlarm, 1u);
    apLoadGameBuffer->LoadData(&fDecalSize, 4u);
    apLoadGameBuffer->LoadMatrixAsQuaternion(&kFollowOffset.m_Rotate, 36);
    if (apLoadGameBuffer->GetVersion() >= 13u) {
        apLoadGameBuffer->LoadData(&kFollowOffset.m_Translate, 12u);
        apLoadGameBuffer->LoadData(&kFollowOffset.m_fScale, 4u);
    }
    if (apLoadGameBuffer->GetVersion() >= 8u) {
        apLoadGameBuffer->LoadData(&fSpreadHeading, 4u);
        apLoadGameBuffer->LoadData(&fSpreadLooking, 4u);
    }
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        apLoadGameBuffer->LoadData(&fWeaponCondition, 4u);
    }
    iactorCauseID = -1;
    apLoadGameBuffer->LoadData(&iactorCauseID, 4u);
    ActorCause* ActorCauseByID = ActorCause::GetActorCauseByID(iactorCauseID);
    spActorCause.operator=(ActorCauseByID);
    if (apLoadGameBuffer->GetVersion() >= 3u) {
        bhasSecondaryItemChange = false;
        apLoadGameBuffer->LoadData(&bhasSecondaryItemChange, 1u);
        if (bhasSecondaryItemChange) {
            ItemChange* v6;
            ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (apMemory) {
                v6 = ::new (apMemory) ItemChange;
            }
            else {
                v6 = nullptr;
            }
            pSecondaryItemChange = v6;
            pSecondaryItemChange->LoadGame(apLoadGameBuffer);
            if (!pSecondaryItemChange->pContainerObj) {
                pSecondaryItemChange->DeleteAllExtra();
                ItemChange* apThis = pSecondaryItemChange;
                if (apThis) {
                    delete apThis;
                }
                pSecondaryItemChange = nullptr;
            }
        }
    }
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t i = 0; i < isaveGameCount; ++i) {
            Projectile::ImpactData* pimpact;
            Projectile::ImpactData* v8 =
                static_cast<Projectile::ImpactData*>(ourMalloc(sizeof(Projectile::ImpactData)));
            if (v8) {
                pimpact = ::new (v8) Projectile::ImpactData;
            }
            else {
                pimpact = nullptr;
            }
            pimpact->LoadGame(apLoadGameBuffer);
            if (!Get3D()) {
                pimpact->bProcessed = false;
            }
            ImpactList.AddTail(pimpact);
        }
    }
    if (apLoadGameBuffer->GetVersion() >= 16u) {
        apLoadGameBuffer->LoadData(&bImpactProcessed, 1u);
    }
}

void Projectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v4;
    TESObjectREFR* v5;
    MobileObject::InitLoadGame(apLoadGameBuffer);
    if (pShooter) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pShooter)));
        v5 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v5 = nullptr;
    }
    pShooter = v5;
    if (pDesiredTarget) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDesiredTarget)));
        v4 = dynamic_cast<TESObjectREFR*>(v3);
    }
    else {
        v4 = nullptr;
    }
    pDesiredTarget = v4;
    if (pSecondaryItemChange) {
        pSecondaryItemChange->InitLoadGame(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        for (Projectile::ImpactData* pimpact : ImpactList) {
            if (pimpact) {
                pimpact->InitLoadGame(apLoadGameBuffer);
            }
        }
    }
}

void Projectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::Revert(apLoadGameBuffer);
    while (!ImpactList.IsEmpty()) {
        Projectile::ImpactData* apThis = ImpactList.m_item;
        if (apThis) {
            delete apThis;
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
    MovementDirection = NiPoint3_ZERO;
    pWeaponSource = nullptr;
    pShooter = nullptr;
    pDesiredTarget = nullptr;
    spLight.operator=(static_cast<NiLight*>(nullptr));
    spPickCache.operator=(static_cast<ProjectilePickCache*>(nullptr));
    spActorCause.operator=(static_cast<ActorCause*>(nullptr));
    bSendAlarm = false;
    fDecalSize = 0.0f;
    fExplosionTimer = 3.4028235e+38f;
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
            ImpactData* pImpact;
            pImpact = pIter->GetItem();
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
        // Get3D is called again here, as the game does.
        bhkWorld::SetMotion(Get3D(), hkpMotion::MOTION_DYNAMIC, true, false, true);
        if (fGrenadeRestitution.Float() != 0.0) {
            if (GetProjectileBase()->Data.fBounceMultiplier > 0.0) {
                // Reads the setting before the bounce multiplier and rounds the product to a float.
                float* pfRestitution = &fGrenadeRestitution.Float();
                float fRestitution = GetProjectileBase()->Data.fBounceMultiplier * *pfRestitution;
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
                bhkSphereShape* pMemory =
                    static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
                bhkSphereShape* pShape = pMemory ? ::new (pMemory) bhkSphereShape(1.0f, true) : nullptr;
                info.SetShape(pShape);
            }
            else {
                bhkSphereShape* pMemory =
                    static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
                bhkSphereShape* pShape = pMemory ? ::new (pMemory) bhkSphereShape(0.1f, true) : nullptr;
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
            ProjectileListener* pMemory =
                static_cast<ProjectileListener*>(bhkRagdollController::operator new(sizeof(ProjectileListener)));
            ProjectileListener* pListener = pMemory ? ::new (pMemory) ProjectileListener(&info, this) : nullptr;
            pcon = pListener;
            pcon->SetQuickSimulate(Main::GetSceneIsComplex());
            pcon->SetIsProjectile(true);
            hkVector4 startVelocity(-3.4028235e+38f, 0.0f, 0.0f, 0.0f);
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
                pcon->SetWaterHeight(-3.4028235e+38f);
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
        bhkWorld* pw;
        bhkWorld* poldW;
        NiAVObject* pav = TESObjectREFR::Get3D();
        TESObjectCELL* pcell = GetParentCell();
        if (pcell) {
            pw = pcell->GetbhkWorld();
        }
        else {
            pw = nullptr;
        }
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
            bhkWorld* poldW_2;
            bhkWorldObject* pworldObject;
            NiNode* v2 = Get3D();
            bhkCollisionObject* pcoll = FindFirstCollisionObject(v2);
            if (pcoll) {
                pworldObject = pcoll->GetRigidBody();
            }
            else {
                pworldObject = nullptr;
            }
            if (pworldObject) {
                const hkpWorld* v3 = pworldObject->GethkWorld();
                poldW_2 = bhkWorld::Getbhk(v3);
            }
            else {
                poldW_2 = nullptr;
            }
            poldW = poldW_2;
        }
        bool badd = false;
        if (poldW != pw) {
            if (poldW) {
                bhkWorld::RemoveObjects(pav, true, false);
            }
            badd = true;
        }
        if (pw) {
            if (badd) {
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
    HighProcess* v3;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
        ReferenceProcessLists.RemoveReference(this, ProcessLevel);
    }
    HighProcess* apMemory = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
    if (apMemory) {
        v3 = ::new (apMemory) HighProcess;
    }
    else {
        v3 = nullptr;
    }
    v3->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = v3;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    MoveHavok(false);
    pCurrentProcess->SetLastSeenLocation();
    return true;
}

bool Projectile::MovetoLow() {
    LowProcess* v3;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
        ReferenceProcessLists.RemoveReference(this, ProcessLevel);
    }
    LowProcess* apMemory = static_cast<LowProcess*>(ourMalloc(sizeof(LowProcess)));
    if (apMemory) {
        v3 = ::new (apMemory) LowProcess;
    }
    else {
        v3 = nullptr;
    }
    v3->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = v3;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    return true;
}

bool Projectile::MovetoMiddleLow() {
    MiddleLowProcess* v3;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
        ReferenceProcessLists.RemoveReference(this, ProcessLevel);
    }
    MiddleLowProcess* apMemory = static_cast<MiddleLowProcess*>(ourMalloc(sizeof(MiddleLowProcess)));
    if (apMemory) {
        v3 = ::new (apMemory) MiddleLowProcess;
    }
    else {
        v3 = nullptr;
    }
    v3->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = v3;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_LOW, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    return true;
}

bool Projectile::MovetoMiddleHigh() {
    MiddleHighProcess* v3;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (pCurrentProcess) {
        PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
        ReferenceProcessLists.RemoveReference(this, ProcessLevel);
    }
    MiddleHighProcess* apMemory = static_cast<MiddleHighProcess*>(ourMalloc(sizeof(MiddleHighProcess)));
    if (apMemory) {
        v3 = ::new (apMemory) MiddleHighProcess;
    }
    else {
        v3 = nullptr;
    }
    v3->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = v3;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    return true;
}

bool Projectile::ChangeProcessLevel() {
    int32_t ProcessLevel;
    bool v7 = true;
    if (pCurrentProcess) {
        ProcessLevel = pCurrentProcess->eLevel;
    }
    else {
        ProcessLevel = 3;
    }
    int32_t v8 = 3;
    bool v9 = IsPermanent();
    TESObjectCELL* a1 = GetParentCell();
    if (Get3D() || pModelLoader->IsReferenceQueued(this)) {
        if (TES::pInstance->IsCellLoaded(a1, true)) {
            v8 = 0;
        }
        else {
            if (v9 && TES::pInstance->IsCellLoaded(a1, false)) {
                v8 = 1;
            }
        }
    }
    if (ProcessLevel != v8) {
        if (v8) {
            if (v8 == 1) {
                MovetoMiddleHigh();
            }
            else {
                uint32_t CastingType = iFormID;
                if (pDataHandler->IsFormIDCreated(CastingType)) {
                    if (v9) {
                        pSaveLoadGame->UnloadForm(this, false);
                    }
                    SetDelete(true);
                    return false;
                }

                PROCESS_TYPE aWhichList = pCurrentProcess->eLevel;
                ReferenceProcessLists.kUpdateLock.Lock(nullptr);
                ReferenceProcessLists.RemoveReference(this, aWhichList);
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
    return v7;
}

bool Projectile::IsTransient() {
    uint32_t v1 = TESForm::GetFormID();
    return (bool)((uint8_t)pDataHandler->IsFormIDCreated(v1));
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
                    alignas(16) hkSimdReal kScale;
                    kScale.~hkSimdReal();
                    ::new (&kScale) hkSimdReal(2.0f);
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
                NiColorA kColor;
                kColor.~NiColorA();
                ::new (&kColor) NiColorA(0.0f, 0.0f, 1.0f, 0.6f);
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
        Projectile::ImpactData* apThis = ImpactList.m_item;
        if (apThis) {
            delete apThis;
        }
        ImpactList.RemoveHead();
    }
    spActorCause.operator=(static_cast<ActorCause*>(nullptr));
    if (pTarget) {
        ProjectileTarget* CurrentFurniture = pTarget;
        if (CurrentFurniture) {
            delete CurrentFurniture;
        }
        pTarget = nullptr;
    }
}

Projectile::PROJECTILE_TYPE Projectile::GetProjectileType() {
    return PROJECTILE_TYPE_UNKNOWN;
}

void Projectile::Initialize() {
    bool v11 =
        pSaveLoadGame->GetSaveGameLoading() || (IsStillLoading() || IsFlagSet(Projectile::PROJECTILE_INITIALIZED));
    if (!pWeaponSource) {
        pWeaponSource = GetProjectileBase()->GetDefaultWeaponSource();
    }
    if (BGSDestructibleObjectForm::IsDestructible(GetObjectReference())) {
        SetDestructible(true);
    }
    SetInitialized(true);
    SetFlag(Projectile::PROJECTILE_INITIALIZED, true);
    if ((GetShooter()) && (!v11)) {
        TESObjectREFR* Shooter = GetShooter();
        SetParentCell(Shooter->GetParentCell());
    }

    bSendAlarm = false;
    bool bcreateProcess = pCurrentProcess == nullptr;
    if (bcreateProcess) {
        HighProcess* v10;
        void* apMemory = ourMalloc(sizeof(HighProcess));
        if (apMemory) {
            v10 = ::new (apMemory) HighProcess;
        }
        else {
            v10 = nullptr;
        }
        pCurrentProcess = (BaseProcess*)v10;
    }
    if ((!IsFlagSet(Projectile::NO_3D)) && (pCurrentProcess) && (pCurrentProcess->eLevel <= PROCESS_TYPE_MIDDLE_HIGH)) {
        if (!Get3D()) {
            Init3D();
        }
        InitHavok();
        if (Get3D()) {
            NiNode* v9 = Get3D();
            if (v9->IsFadeNode()) {
                NiNode* v8 = Get3D();
                BSFadeNode* v2 = v8->IsFadeNode();
                v2->TurnFadeNodeOn();
            }
            else {
                NiNode* v7 = Get3D();
                NiNode* v3 = v7->IsNode();
                BSShaderUtil::RecursiveSetPropertyFadeAlpha(v3, 1.0f);
            }
        }
    }

    if (GetParentCell()) {
        GetParentCell()->AddReference(this, false);
    }
    AttachLight();
    if (!v11) {
        SpawnMuzzleFlash();
    }
    if (bcreateProcess) {
        ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    }
    if (IsFlagSet(Projectile::POST_PROCESS)) {
        // Tests the list's address for null, which it never is, as the game does.
        BSSimpleList<void*>* plist = &ReferenceProcessLists.ProjectilePostProcessList;
        if (plist) {
            void* a2 = this;
            plist->AddHead(a2);
        }
    }
    fRange = GetProjectileBase()->Data.fRange;
    if ((!v11) && (pShooter) && (pShooter->IsActor())) {
        Actor* pshooterActor = static_cast<Actor*>(pShooter);
        if ((pshooterActor->IsPc() && pshooterActor->GetIronSights()) && (pWeaponSource) &&
            (pWeaponSource->HasScope())) {
            fRange = fRange * fCombatIronSightsRangeMult.Float();
        }
    }

    bool bbehindWall = false;
    if (!v11 && IsFlagSet(Projectile::CHECK_BEHIND_WALL)) {
        bbehindWall = CheckShotBehindWall();
    }
    if (!static_cast<GrenadeProjectile*>(this)->GrenadeProjectile::IsMine()) {
        fExplosionTimer = GetProjectileBase()->GetAltTimer();
    }
    if (!bbehindWall && !v11) {
        TestProjectileInTriggerZone();
        if (GetShooter() == pPlayer) {
            AimProjectile();
        }
        AddInitialVelocity();
        if (IsFlagSet(Projectile::HITSCAN_MODE)) {
            UpdateInstant();
        }
    }
    BGSProjectile* proj_2 = static_cast<BGSProjectile*>(GetObjectReference());
    if (proj_2->GetMotionType() == BGSProjectile::MOTION_BEAM) {
        this->SetConsistencyRecurse(Get3D());
    }
    else {
        if (proj_2->GetMotionType() == BGSProjectile::MOTION_CONTINUOUS_BEAM) {
            this->SetConsistencyRecurse(Get3D());
        }
    }
    Process3D();
    if (Get3D()) {
        NiUpdateData UpdateData(fCellAppTime, true, false);
        NiNode* v6 = Get3D();
        v6->UpdateSelectedDownwardPass(UpdateData, 0);
    }
    spPickCache.operator=(static_cast<ProjectilePickCache*>(nullptr));
}

bool Projectile::ProcessImpacts() {
    bool bprocessed = false;
    bool btargetWasAlive = false;
    if (!bImpactProcessed && !ImpactList.IsEmpty()) {
        BSSimpleList<Projectile::ImpactData*>* impactList = &ImpactList;
        do {
            if (!impactList) {
                break;
            }
            if (impactList->IsEmpty()) {
                break;
            }
            Projectile::ImpactData* pdata = impactList->m_item;
            impactList = impactList->m_pkNext;
            bool b3dSwitched = false;
            TESObjectREFR* pcollidee = pdata->pCollidee;
            if ((pcollidee) && (fDamage > 0.0 && pdata->bDoDamage)) {
                TESObjectREFR* v1;
                TESObjectREFR* v12;
                NiAVObject* pobj3D = pdata->pCollidee->TESObjectREFR::Get3D();
                if (GetShooter() &&
                    ((v12 = GetShooter(), v12->IsActor()) &&
                     (pcollidee->IsActor() && (v1 = GetShooter(), !v1->GetDisabledOrDeleted())))) {
                    btargetWasAlive = !pcollidee->IsDead(false);
                    CombatHit(static_cast<Actor*>(pcollidee), false);
                }
                else {
                    Actor* pshooterActor;
                    TESObjectREFR* Shooter;
                    bhkRigidBody* pbody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pdata->pTargetWorldObject));
                    if ((pbody) && (pbody->GetMotionType() < 4)) {
                        ActorCause* v2 = GetActorCause();
                        pcollidee->SetActorCause(v2);
                    }

                    if (GetShooter() && (Shooter = GetShooter(), Shooter->IsActor())) {
                        pshooterActor = static_cast<Actor*>(GetShooter());
                    }
                    else {
                        pshooterActor = nullptr;
                    }
                    Actor* v20 = pshooterActor;
                    float fextraDamage = 0.0f;
                    if ((pshooterActor) && (v20->IsPc() && pVATS.IsInVats()) && (pWeaponSource)) {
                        fextraDamage = static_cast<float>(pWeaponSource->GetCriticalDamage());
                    }

                    double RunSpeedMult = fDamage;
                    float fdamage = RunSpeedMult + fextraDamage;
                    if (pcollidee->IsActor()) {
                        HitData* v9;
                        Actor* ptargetActor = static_cast<Actor*>(pcollidee);
                        void* apMemory = ourMalloc(sizeof(HitData));
                        if (apMemory) {
                            v9 = ::new (apMemory) HitData;
                        }
                        else {
                            v9 = nullptr;
                        }
                        HitData* apObject = v9;
                        NiPointer<HitData> spdata(v9);
                        HitData::InitializeHitData(spdata.m_pObject, nullptr, ptargetActor, nullptr, this);
                        btargetWasAlive = !ptargetActor->IsDead(false);
                        HitData* afTime = spdata.m_pObject;
                        ptargetActor->HitMe(afTime, 0);
                    }
                    else {
                        pdata->pCollidee->DamageObject(fdamage, false);
                    }
                    if (pcollidee->GetDestroyed() && GetShooter() == pPlayer) {
                        pPlayer->AddPlayerAction(PLAYER_ACTION_DESTROY_OBJECT, 8.0f, nullptr);
                    }
                }
                NiNode* v4 = pcollidee->TESObjectREFR::Get3D();
                if (pobj3D != v4) {
                    b3dSwitched = true;
                }
            }

            if ((!pcollidee || (pcollidee->iFormFlags & 32) == 0) &&
                (!GetProjectileBase()->GetHasExplosion() ||
                 (GetProjectileBase()->GetHasExplosionAltTrigger() ||
                  GetProjectileBase()->GetHasExplosionDetonates())) &&
                !b3dSwitched) {
                const hkpCollidable* ptargetCollidable;
                if ((VATS::pStranger) && (VATS::pStranger == GetShooter()) && (pcollidee) && (pcollidee->IsActor())) {
                    PlaySound_MUSMysteriousStrangerA02();
                }

                if ((VATS::pMissFortune) && (VATS::pMissFortune == GetShooter()) && (pcollidee) &&
                    (pcollidee->IsActor() && pcollidee->IsDead(false) && btargetWasAlive)) {
                    PlaySound_MUSMysteriousStrangerA02();
                }

                if (pdata->pTargetWorldObject) {
                    ptargetCollidable = pdata->pTargetWorldObject->getCollidable();
                }
                else {
                    ptargetCollidable = nullptr;
                }
                const hkpCollidable* _330 = ptargetCollidable;
                SpawnCollisionEffects(pcollidee, pdata->kLocation, pdata->kNormal, ptargetCollidable, pdata->eMaterial);
                const hkpCollidable* v7 = _330;
                NiPoint3 v32 = GetMovementDirection();
                ApplyImpactForce(pdata->kLocation, v32, v7);
            }
            bprocessed = true;
        }
        while (!GetDisabledOrDeleted());
        bImpactProcessed = true;
    }
    return bprocessed;
}

bool Projectile::IsMine() {
    return false;
}

bool Projectile::TurnOff(Actor* apActionActor, bool abSilent) {
    FNVD_INLINED_LOCAL(BSSoundHandle, soundHandle, std::nullopt, std::nullopt);
    bool bresult = false;
    if (GetProjectileBase()->GetCanTurnOff() && !IsFlagSet(Projectile::TURNED_OFF)) {
        StopCountdownSound();
        if (!abSilent) {
            TESSound* pdeactivateSound = GetProjectileBase()->Data.pDeactivateSound;
            if (pdeactivateSound) {
                const char* Filename = pdeactivateSound->GetFilename();
                BSAudio* v3 = &BSAudio::QInstance();
                v3->GetSoundHandleByFilename(
                    &soundHandle, Filename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pdeactivateSound
                );
                const NiPoint3* v4 = &GetLocationOnReference();
                soundHandle.SetPosition(*v4);
                soundHandle.Play(false);
            }
        }
        SetFlag(Projectile::TURNED_OFF, true);
        Actor* pshooterActor = nullptr;
        TESFaction* pownerFaction = nullptr;
        if (pShooter && pShooter->IsActor()) {
            pshooterActor = static_cast<Actor*>(pShooter);
        }
        else {
            TESForm* powner = GetOwner();
            if ((powner) && (powner->GetFormType() == FACT_ID)) {
                pownerFaction = static_cast<TESFaction*>(powner);
            }
        }
        if (!abSilent &&
            (fExplosionTimer != 3.4028234663852886e+38 ||
             GetMineReactsToTarget(apActionActor, pshooterActor, pownerFaction))) {
            MiscStatManager::Increment(MiscStatManager::MS_MINES_DISARMED);
            if (iMineDisarmExperience.Int() > 0) {
                int32_t v8 = iMineDisarmExperience.Int();
                pPlayer->RewardExperience(v8);
            }
        }
        return true;
    }
    return bresult;
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
    kPickData.~bhkPickData();
    ::new (&kPickData) bhkPickData;
    NiPoint3 kPosition;
    kPosition.~NiPoint3();
    ::new (&kPosition) NiPoint3;
    NiPoint3 kForward;
    kForward.~NiPoint3();
    ::new (&kForward) NiPoint3;
    kPosition = GetLocationOnReference();
    NiMatrix3 kRotation;
    kRotation.~NiMatrix3();
    ::new (&kRotation) NiMatrix3;
    kRotation.MakeZRotation(GetHeading(false));
    NiMatrix3 kLooking;
    kLooking.~NiMatrix3();
    ::new (&kLooking) NiMatrix3;
    kLooking.MakeXRotation(GetLooking());
    kRotation = kRotation * kLooking;
    kRotation.GetCol(1, kForward);
    kForward.Unitize();
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
    if (GetCharController()) {
        CFilter kControllerFilter;
        kFilter.iFilter =
            static_cast<bhkCharacterController*>(GetCharController())->GetCollisionFilter(&kControllerFilter)->iFilter;
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
    if (spPickCache.operator->() && spPickCache.operator->()->GetCache()) {
        kPickData.SetCache(spPickCache.operator->()->GetCache());
    }
    float fLength = GetRange() - GetDistanceMoved();
    NiPoint3 kLength = kForward * fLength;
    NiPoint3 kOrigin;
    kOrigin.~NiPoint3();
    ::new (&kOrigin) NiPoint3;
    pWorld->GetOrigin(kOrigin);
    ahkpWorld* pHavokWorld = pWorld->GetahkpWorld();
    NiPoint3 kMax;
    kMax.~NiPoint3();
    ::new (&kMax) NiPoint3;
    HK2NI(kMax, pHavokWorld->m_broadPhaseExtents[1]);
    NiPoint3 kMin;
    kMin.~NiPoint3();
    ::new (&kMin) NiPoint3;
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
    float fWaterHitRatio = 3.4028235e+38f;
    bHit = bHit | PickAutoWater(&kPickData, fWaterHitRatio);
    if (bHit) {
        bHit = ProcessTargetPick(&kPickData, this, GetProjectileBase(), nullptr, true, fWaterHitRatio) != nullptr;
    }
    if (bProjectileDebug.Bool()) {
        NiColorA kMiss;
        NiColorA kReach;
        NiColorA* pColor;
        if (kPickData.GetPickFailed()) {
            pColor = FNVD_RECONSTRUCT(NiColorA)(&kMiss, 1.0f, 0.0f, 0.0f, 1.0f);
        }
        else {
            pColor = FNVD_RECONSTRUCT(NiColorA)(&kReach, 1.0f, 1.0f, 0.0f, 1.0f);
        }
        NiColorA kColor = *pColor;
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
    Projectile::ImpactData* pdata;
    NiPoint3 v38;
    CFilter v43;
    CFilter v44;
    CFilter v45;
    Projectile::ImpactData* pImpactData =
        static_cast<Projectile::ImpactData*>(ourMalloc(sizeof(Projectile::ImpactData)));
    if (pImpactData) {
        pdata = ::new (pImpactData) Projectile::ImpactData;
    }
    else {
        pdata = nullptr;
    }
    Projectile::ImpactData* pdata_2 = pdata;
    if (IsFlagSet(Projectile::ALWAYS_HIT_TARGET) && (pDesiredTarget && pdata_2->pCollidee != pDesiredTarget)) {
        pdata_2->pCollidee = pDesiredTarget;
        pdata_2->pTargetWorldObject = nullptr;
        pdata_2->kLocation = pDesiredTarget->GetLocationOnReference();
        pdata_2->kNormal = -MovementDirection;
        pdata_2->kNormal.Unitize();
        pdata_2->eDamageLimb = BGSBodyPart::LIMB_TORSO;
        pdata_2->eMaterial = BHK_MATERIAL_STONE;
        if (pDesiredTarget->IsActor()) {
            pdata_2->eMaterial = BHK_MATERIAL_ORGANIC;
            Actor* pactor = static_cast<Actor*>(pDesiredTarget);
            NiPoint3 fA = pdata_2->kLocation;
            NiPoint3 fR = pdata_2->kLocation - MovementDirection;
            NiNode* pclosestBone = pactor->GetClosestBone(fR, fA);
            if (pclosestBone) {
                hkpRigidBody* v35;
                BGSBodyPart::LIMB_ENUM DamageLocation = pactor->GetDamageLocation(pclosestBone);
                pdata_2->eDamageLimb = DamageLocation;
                bhkCollisionObject* pcollObj = static_cast<bhkCollisionObject*>(pclosestBone->GetCollisionObject());
                if (pcollObj->GetRigidBody()) {
                    v35 = pcollObj->GetRigidBody()->Gethk();
                }
                else {
                    v35 = nullptr;
                }
                pdata_2->pTargetWorldObject = (hkpWorldObject*)v35;
            }
            if (pdata_2->pTargetWorldObject) {
                pdata_2->pTargetWorldObject->addReference();
            }
        }
    }
    else {
        hkpWorldObject* Owner;
        pdata_2->pCollidee = apCollidee;
        pdata_2->kLocation = aContactPoint;
        pdata_2->kNormal = aContactNormal;
        if (apTargetCollidable) {
            Owner = static_cast<hkpWorldObject*>(apTargetCollidable->getOwner());
        }
        else {
            Owner = nullptr;
        }
        pdata_2->pTargetWorldObject = Owner;
        if (pdata_2->pTargetWorldObject) {
            pdata_2->pTargetWorldObject->addReference();
            if ((apCollidee) && (apCollidee->IsActor() && apTargetCollidable->getType() == 1)) {
                uint32_t afSize = apTargetCollidable->getCollisionFilterInfo();
                if (FNVD_RECONSTRUCT(CFilter)(&v45, afSize)->QBipLayer()) {
                    NiAVObject* phitAVObject = GetAVObjectForCollidable(apTargetCollidable);
                    Actor* pactor_2 = static_cast<Actor*>(apCollidee);
                    NiNode* pclosestBone_2 = phitAVObject->IsNode();
                    BGSBodyPart::LIMB_ENUM v10 = pactor_2->GetDamageLocation(pclosestBone_2);
                    pdata_2->eDamageLimb = v10;
                }
            }
        }
        HavokMaterialType emat = aeForceMaterial;
        if ((apTargetCollidable) && (aeForceMaterial == BHK_MATERIAL_MAX)) {
            uint32_t afSizea = apTargetCollidable->getCollisionFilterInfo();
            uint32_t Group = FNVD_RECONSTRUCT(CFilter)(&v44, afSizea)->GetGroup();
            bool bhitLandHeightField = Group == 1;
            if (Group == 1) {
                emat = TES::pInstance->GetLandMaterialType(aContactPoint);
            }
            else {
                bhkShape* pshape = bhkShape::Getbhk(apTargetCollidable->m_shape);
                if (pshape) {
                    emat = pshape->GetSubshapeMaterial(aiKey);
                }
            }
            uint32_t afSizeb = apTargetCollidable->getCollisionFilterInfo();
            if (FNVD_RECONSTRUCT(CFilter)(&v43, afSizeb)->GetLayer() == L_WATER) {
                emat = BHK_MATERIAL_WATER;
            }
        }

        pdata_2->eMaterial = emat;
        if (emat == BHK_MATERIAL_WATER) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                Instance->QueueAddRipple(0.01f, aContactPoint);
            }
            else {
                TES::pInstance->GetWaterSystem()->AddRipple(aContactPoint, 0.01f);
            }
        }
    }
    bool bvatsSuccess = true;
    if ((bVATSProjectileDebug.Bool()) && (pTarget)) {
        const char* v25;
        const char* v26;
        int32_t CastingType;
        const char* HandleString;
        const char* FullName;
        const char* v31;
        NiAVObject* ptargetObject;
        NiAVObject* phitAVObject_2 = GetAVObjectForCollidable(apTargetCollidable);
        bhkCollisionObject* ptargetCol = FindFirstCollisionObject(pTarget->spPart.m_pObject);
        if (ptargetCol) {
            ptargetObject = ptargetCol->m_pkSceneObject;
        }
        else {
            ptargetObject = nullptr;
        }
        TESObjectREFR* ptarget = pTarget->pReference;
        bool v32 = apCollidee == ptarget && phitAVObject_2 == ptargetObject;
        bvatsSuccess = v32;
        if (v32) {
            v31 = "SUCCESS";
        }
        else {
            v31 = "FAILURE";
        }
        InitializationFlow("Targetted VATS Projectile Result: %s", v31);
        if (ptarget->GetFormEditorIDLength()) {
            FullName = ptarget->GetFormEditorID();
        }
        else {
            FullName = ptarget->GetFullName();
        }
        InitializationFlow("Target Ref ID:\t%s %08X", FullName, ptarget->iFormID);
        if (ptargetObject) {
            HandleString = (&ptargetObject->m_kName)->GetHandleString();
        }
        else {
            HandleString = "";
        }
        InitializationFlow("Target Part:\t%s", HandleString);
        if (apCollidee) {
            CastingType = apCollidee->iFormID;
        }
        else {
            CastingType = 0;
        }
        if (apCollidee) {
            const char* v27;
            if (apCollidee->GetFormEditorIDLength()) {
                v27 = apCollidee->GetFormEditorID();
            }
            else {
                v27 = apCollidee->GetFullName();
            }
            v26 = v27;
        }
        else {
            v26 = "Land";
        }
        InitializationFlow("Hit Ref ID:\t\t%s %08X", v26, CastingType);
        if (phitAVObject_2) {
            v25 = (&phitAVObject_2->m_kName)->GetHandleString();
        }
        else {
            v25 = "";
        }
        InitializationFlow("Hit Part:\t\t%s\n", v25);
    }

    if (pdata_2->pCollidee) {
        pdata_2->pCollidee->SetTargeted(true);
    }
    if (GetProjectileType() == Projectile::PROJECTILE_TYPE_CONTINUOUS_BEAM) {
        ContinuousBeamProjectile* pcbeam = static_cast<ContinuousBeamProjectile*>(this);
        if (this) {
            int32_t shotsFired = pcbeam->iShotsFired;
            double Age = GetAge();
            double v23 = static_cast<double>(shotsFired);
            if (!(v23 / pWeaponSource->data.fShotsPerSec < Age)) {
                pdata_2->bDoDamage = false;
            }
            else {
                Actor* pactor_3 = static_cast<Actor*>(GetShooter());
                if (pactor_3) {
                    pactor_3->UseAmmo(1);
                }
                pcbeam->SetShotsFired(shotsFired + 1);
                pdata_2->bDoDamage = pdata_2->pCollidee && pdata_2->pCollidee->IsActor();
                if (pactor_3->GetCurrentAmmoClipCount() < 1) {
                    pcbeam->SetKill(pactor_3);
                }
            }
        }
    }
    if (bProjectileDebug.Bool() || bVATSProjectileDebug.Bool()) {
        NiColorA color(0.0f, 0.0f, 1.0f, 0.1f);
        if ((pdata_2->pCollidee) && (pdata_2->pCollidee->IsActor())) {
            NiColorA kActorColor(0.0f, 1.0f, 0.0f, 0.1f);
            color = kActorColor;
        }

        if (!bvatsSuccess) {
            NiColorA kFailureColor(1.0f, 0.0f, 0.0f, 0.65f);
            color = kFailureColor;
        }
        if (pdata_2->bDoDamage) {
            NiColorA kDamageColor(0.5f, 0.0f, 0.0f, 0.65f);
            color = kDamageColor;
        }
        NiAVObject* pdebug = (NiAVObject*)MakeTriPoint(5.0f, color, true);
        pdebug->UpdateProperties();
        pdebug->SetTranslate(pdata_2->kLocation);
        float* v15 = &fProjectileDebugDuration.Float();
        TES::pInstance->AddTempDebugObject(pdebug, *v15);
        NiPoint3* v17 = ::new (&v38) NiPoint3(pdata_2->kLocation + pdata_2->kNormal * 24.0f);
        pdebug = (NiAVObject*)MakeLine(pdata_2->kLocation, color, *v17, color, true);
        float* v18 = &fProjectileDebugDuration.Float();
        TES::pInstance->AddTempDebugObject(pdebug, *v18);
    }
    ImpactList.AddHead(pdata_2);
    return true;
}

Projectile::~Projectile() {
    if (pSecondaryItemChange) {
        delete pSecondaryItemChange;
    }
    pSecondaryItemChange = nullptr;
    if ((TESObjectREFR::Get3D() && TESObjectREFR::Get3D()->IsNode()) &&
        (TESObjectREFR::HasAddonFlags(TESObjectREFR::Get3D()->IsNode()))) {
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
        void* item = this;
        ReferenceProcessLists.ProjectilePostProcessList.Remove(item);
    }
    if (IsFlagSet(Projectile::ACTIVE_IN_INVENTORY) && GetDesiredTarget()) {
        Actor* pActorTarget = static_cast<Actor*>(GetDesiredTarget());
        if (pActorTarget->IsActor() && pActorTarget->pCurrentProcess) {
            pActorTarget->pCurrentProcess->SetPlantedExplosive(false);
        }
        GetDesiredTarget()->RemoveItem(
            GetProjectileBase(), nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false
        );
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
    const NiPoint3* kPos;
    NiPoint3* movementDirection;
    NiPoint3 result;
    float rfYAngle;
    float rfZAngle;
    NiMatrix3 akRot;
    float rfXAngle;
    NiPoint3 kAngle;
    NiMatrix3 ecx0;
    NiNode* pRootNode = Get3D();
    if (pRootNode) {
        kPos = &GetLocationOnReference();
        pRootNode->SetTranslate(*kPos);
        if (abSyncRotation) {
            kAngle = TESObjectREFR::GetAngleOnReference();
            ecx0;
            (&ecx0)->FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
            pRootNode->SetRotate(ecx0);
        }
        if (abDoArcReorientation && IsFlagSet(Projectile::REORIENT)) {
            __builtin_memcpy(&akRot, &pRootNode->GetLocalTransform(), sizeof(akRot));
            (&akRot)->ToEulerAnglesZXY(rfZAngle, rfXAngle, rfYAngle);
            movementDirection = &(result = GetMovementDirection());
            rfXAngle = -NiASin(movementDirection->z);
            (&akRot)->FromEulerAnglesZXY(rfZAngle, rfXAngle, rfYAngle);
            pRootNode->SetRotate(akRot);
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
    if ((bVATSProjectileDebug.Bool() && ImpactList.IsEmpty()) && (pTarget)) {
        const char* HandleString;
        const char* FullName;
        NiAVObject* ptargetObject;
        NiAVObject* v2 = pTarget->spPart.m_pObject;
        bhkCollisionObject* ptargetCol = FindFirstCollisionObject(v2);
        if (ptargetCol) {
            ptargetObject = ptargetCol->m_pkSceneObject;
        }
        else {
            ptargetObject = nullptr;
        }
        TESObjectREFR* ptarget = pTarget->pReference;
        InitializationFlow("Targetted VATS Projectile Result: %s", "FAILURE");
        if (ptarget->GetFormEditorIDLength()) {
            FullName = ptarget->GetFormEditorID();
        }
        else {
            FullName = ptarget->GetFullName();
        }
        InitializationFlow("Target Ref ID:\t%s %08X", FullName, ptarget->iFormID);
        if (ptargetObject) {
            HandleString = (&ptargetObject->m_kName)->GetHandleString();
        }
        else {
            HandleString = "";
        }
        InitializationFlow("Target Part:\t%s", HandleString);
    }

    if (GetFormType() == PGRE_ID) {
        VATSMenu::RemoveTarget(this);
    }
    uint32_t CastingType = iFormID;
    if (pDataHandler->IsFormIDCreated(CastingType)) {
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
        if (fExplosionTimer != 3.4028234663852886e+38 || GetProjectileBase()->GetExplosionProximity() == 0.0 ||
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
                        float* pFast = &fMinesBlinkFast.Float();
                        double fRatio = fExplosionTimer / fMinesBlinkMax.Float();
                        float* pSlow = &fMinesBlinkSlow.Float();
                        fBlinkTimer = (*pSlow - fMinesBlinkFast.Float()) * fRatio + *pFast;
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
        // This random turn is built and never used, as the game does.
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
            pRotate = &NiMatrix3_IDENTITY;
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
    Explosion* pExplosion =
        Explosion::SpawnExplosion(GetProjectileBase()->GetExplosion(), pOwner, nullptr, pCell, location, rotation);
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
    if (GetProjectileBase()->GetExplosion()->GetDamage() > 0.0 && IsFlagSet(ACTIVE_IN_INVENTORY) &&
        GetDesiredTarget() && GetDesiredTarget()->IsActor()) {
        Actor* pTargetActor = static_cast<Actor*>(GetDesiredTarget());
        Actor* pShooterActor;
        if (GetShooter() && GetShooter()->IsActor()) {
            pShooterActor = static_cast<Actor*>(GetShooter());
        }
        else {
            pShooterActor = nullptr;
        }
        // IsPc is called on the shooter even when it is null, as the game does.
        if (pShooterActor->IsPc()) {
            MiscStatManager::Increment(MiscStatManager::MS_PANTS_EXPLODED);
            if (pTargetActor->pCurrentProcess) {
                pTargetActor->pCurrentProcess->ModTrackedDamage(pPlayer, 10000.0f);
            }
        }
        pTargetActor->Kill(pShooterActor, 0.0f);
        BaseProcess* pPlayerProcess = pPlayer->pCurrentProcess;
        int32_t iSoundLevel = GetProjectileBase()->GetExplosion()->GetSoundLevelValue();
        pPlayerProcess->SetActorsDetectionEvent(
            pPlayer, pTargetActor->GetLocationOnReference(), iSoundLevel, 2, pTargetActor
        );
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
    return ((((int32_t)aeFlag) & ((int32_t)iFlags)) != 0);
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
    if (spLight.operator->() && IsFlagSet(FADING)) {
        float fDimmer = BSMAX(0.0f, BSMIN(1.0f, fAlpha));
        spLight.operator->()->SetDimmer(fDimmer);
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
    kImpulse.~hkVector4();
    ::new (&kImpulse) hkVector4;
    NI2HK(kImpulse, aDirection);
    alignas(16) hkVector4 kPoint;
    kPoint.~hkVector4();
    ::new (&kPoint) hkVector4;
    NI2HK(kPoint, aContactPoint);
    if (static_cast<float>(kImpulse.length3()) > 0.0) {
        kImpulse.normalize3();
    }
    alignas(16) hkSimdReal kForce;
    kForce.~hkSimdReal();
    ::new (&kForce) hkSimdReal(fForce);
    kImpulse.mul4(kForce);
    pWorld->markForWrite();
    pRigidBody->operator*()->applyPointImpulse(kImpulse, kPoint);
    pWorld->unmarkForWrite();
}

bool Projectile::PickAutoWater(bhkPickData* apPickData, float& afWaterHitRatio) {
    NiPoint3 startPoint;
    NiPoint3 endPoint;
    TESObjectCELL* pcell = GetParentCell();
    if (!pcell) {
        return false;
    }
    if (!pcell->HasWater()) {
        return false;
    }
    float fwaterHeight = pcell->GetWaterHeight();
    apPickData->GetFrom(startPoint);
    apPickData->GetTo(endPoint);
    if (!(fwaterHeight < static_cast<double>(startPoint.z)) || !(fwaterHeight >= static_cast<double>(endPoint.z))) {
        return false;
    }
    NiPoint3 length = endPoint - startPoint;
    float ftoWater = fwaterHeight - startPoint.z;
    afWaterHitRatio = ftoWater / length.z;
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
    kPickData.~bhkPickData();
    ::new (&kPickData) bhkPickData;
    NiPoint3 kEye;
    kEye.~NiPoint3();
    ::new (&kEye) NiPoint3;
    NiPoint3 kProjectilePos;
    kProjectilePos.~NiPoint3();
    ::new (&kProjectilePos) NiPoint3;
    NiPoint3 kLength;
    kLength.~NiPoint3();
    ::new (&kLength) NiPoint3;
    kProjectilePos = GetLocationOnReference();
    kEye = static_cast<TESObjectREFR*>(pPlayer)->GetLocationOnReference();
    // Keeps the eye height as a double and adds the float z to the double product, as the game does.
    double dEyeHeight = pPlayer->GetEyeHeight();
    kEye.z = static_cast<float>(pPlayer->GetScale() * dEyeHeight + kEye.z);
    kLength = kProjectilePos - kEye;
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
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
    float fWaterHitRatio = 3.4028235e+38f;
    bHit = bHit | PickAutoWater(&kPickData, fWaterHitRatio);
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

void Projectile::SetAngularMomentumZ(float a2) {
    fSpreadHeading = a2;
}

void Projectile::SetAngularMomentumX(float a2) {
    fSpreadLooking = a2;
}

void Projectile::StopCountdownSound() {
    if ((&sndCountdown)->IsValid()) {
        (&sndCountdown)->Stop();
        if ((&sndCountdown)->IsValid()) {
            (&sndCountdown)->Release();
        }
    }
}

void Projectile::CombatHit(Actor* apCollidee, bool abPowerAttack) {
    Actor* pSourceRef;
    uint8_t v5;
    if ((pShooter) && (pShooter->IsActor())) {
        v5 = 1;
        pSourceRef = static_cast<Actor*>(pShooter);
        if ((!pSourceRef->IsPc()) && IsFlagSet(Projectile::SHOOTER_IN_COMBAT)) {
            v5 = (uint8_t)pSourceRef->IsInCombatWithActor(apCollidee);
        }
        if (v5) {
            pSourceRef->CombatHit(apCollidee, abPowerAttack, this, 0);
        }
    }
}

// Tests the object for NiGeometry (IsKindOf) and else walks it as a node, so it takes any NiAVObject: Initialize passes
// the projectile's 3D node.
void Projectile::SetConsistencyRecurse(NiAVObject* apObject) {
    uint32_t a1;
    NiNode* v5;
    NiGeometryData* modelData;
    if (apObject) {
        if (NiObject::IsKindOf_0(&NiGeometry::ms_RTTI, apObject)) {
            modelData = static_cast<NiGeometry*>(apObject)->GetModelData();
            if (modelData) {
                modelData->SetConsistency(NiGeometryData::MUTABLE);
            }
        }
        else {
            v5 = apObject->IsNode();
            if (v5) {
                for (a1 = 0; a1 < v5->GetArrayCount(); ++a1) {
                    SetConsistencyRecurse(v5->GetAt(a1));
                }
            }
        }
    }
}

void Projectile::DetachLight() {
    ShadowSceneNode* shadowSceneNode;
    NiLight* v2;
    if ((&spLight)->m_pObject) {
        v2 = (&spLight)->m_pObject;
        shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        shadowSceneNode->RemoveLight(v2);
        (&spLight)->operator=(nullptr);
    }
}

void Projectile::AimAtPoint(NiPoint3 aTargetLocation) {
    float v4;
    float v5;
    NiPoint3 result;
    const NiPoint3* v2 = &GetLocationOnReference();
    (result = (&aTargetLocation)->operator-(*v2));
    NiPoint3 arVector = result;
    (&arVector)->Unitize();
    float v7 = GetHeading(false);
    float looking = MobileObject::GetLooking();
    BGSProjectile* actorBase = GetProjectileBase();
    float gravity = actorBase->GetGravity();
    if (gravity == 0.0) {
        v5 = (float)(arVector.z / (1.0 - (arVector.z * arVector.z)));
        looking = -std::atanf(v5);
    }
    float zAngleFromVector = GetZAngleFromVector(arVector);
    if ((gravity == 0.0) || ((v4 = zAngleFromVector - GetHeading(false)), (std::fabs(v4) <= 2.5))) {
        SetHeading(zAngleFromVector);
    }
    MobileObject::SetLooking(looking);
}

Projectile::ImpactData* Projectile::GetImpactData() {
    return ImpactList.m_item;
}

void Projectile::AimProjectile() {
    bool v5 = true;
    if (IsFlagSet(Projectile::NO_CHARCONTROLLER) && !IsFlagSet(Projectile::NO_COLLISION)) {
        v5 = pVATS.GetMode() == VATS::VATS_PLAYBACK;
    }
    if (v5) {
        NiNode* apReleaseObj = static_cast<NiNode*>(pPlayer->GetFireNode());
        if (!apReleaseObj) {
            apReleaseObj = Get3D();
        }
        if (((IsFlagSet(Projectile::WHOLE_BODY_TARGETED) || !IsFlagSet(Projectile::NO_CHARCONTROLLER)) &&
             pVATS.GetMode() == VATS::VATS_PLAYBACK) &&
            (pVATS.GetCurrentAction()) && (pVATS.GetCurrentAction()->bSuccess)) {
            ProjectileTarget* v1;
            ProjectileTarget* apMemory = static_cast<ProjectileTarget*>(ourMalloc(sizeof(ProjectileTarget)));
            if (apMemory) {
                v1 = ::new (apMemory) ProjectileTarget;
            }
            else {
                v1 = nullptr;
            }
            SetProjectileTarget(v1);
        }

        if (!spPickCache.m_pObject) {
            pPlayer->ApplyAutoAimActor(
                this, apReleaseObj, data.kRotation.z, data.kRotation.x, &data.kPosition, 1.1754944e-38f, 1.1754944e-38f
            );
        }
        Sync3DWithReference(true, false);
    }
}

void Projectile::Init3D() {
    Load3D(false);
    NiNode* a1 = Get3D();
    if (a1) {
        a1->SetScale(1.0);
        Sync3DWithReference(true, false);
        if (a1->IsNode()) {
            NiNode* v1 = a1->IsNode();
            if (TESObjectREFR::HasAddonFlags(v1)) {
                NiNode* v2 = a1->IsNode();
                TESObjectREFR::AddMasterParticleAddonNodes(v2);
            }
        }
        NiTimeController::StartAnimations(a1);
    }
}

void Projectile::AddInitialVelocity() {
    if (!IsFlagSet(Projectile::NO_COLLISION) && IsFlagSet(Projectile::NO_CHARCONTROLLER)) {
        uint32_t CastingType = iFormID;
        if (pDataHandler->IsFormIDCreated(CastingType)) {
            NiNode* abReset = Get3D();
            if (abReset) {
                NiPoint3 a3;
                bhkWorld::Enable(abReset, false, true, true);
                NiUpdateData apMemory(0.0f, false, false);
                abReset->UpdateWorldData(apMemory);
                bhkWorld::Enable(abReset, true, true, true);
                (&abReset->GetWorldTransform())->m_Rotate.GetCol(1u, a3);
                float a2 = GetSpeed();
                a3.operator*=(a2);
                TESHavokUtilities::AddVelocity(abReset, a3, true);
                AddChange(4);
            }
        }
    }
}

void Projectile::UpdateIfNotInArrowsList(float a2) {
    if (!IsFlagSet(Projectile::POST_PROCESS)) {
        Update(a2);
    }
}

void Projectile::HandleInventoryGrenadeDialogue() {
    if (IsFlagSet(Projectile::ACTIVE_IN_INVENTORY) &&
        !GetProjectileBase()->IsFlagSet(BGSProjectile::EXPLOSION_DETONATES)) {
        double Age = GetAge();
        if (fProjectileInventoryGrenadeFreakoutTime.Float() < Age && !IsFlagSet(Projectile::INVENTORY_FREAKOUT)) {
            SetFlag(Projectile::INVENTORY_FREAKOUT, true);
            pCombatManager->pThreatMap->AddProjectile(this);
            if (GetDesiredTarget()) {
                TESObjectREFR* pGrenadeTargetRef = GetDesiredTarget();
                if (pGrenadeTargetRef->IsActor()) {
                    Actor* pActor = static_cast<Actor*>(GetDesiredTarget());
                    if (pActor->GetCurrentAIProcess()) {
                        pActor->GetCurrentAIProcess()->SetPlantedExplosive(true);
                        TESTopic* Topic = TESTopic::GetTopic(DT_COMBAT, 5);
                        pActor->GetCurrentAIProcess()->ProcessGreet(pActor, Topic, false, false, false, false);
                        pActor->GetCurrentAIProcess()->SetupSpecialIdle(
                            pActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, true, true
                        );
                    }
                }
            }
        }
    }
}

bool Projectile::TestProjectileInTriggerZone() {
    CFilter v11;
    CFilter v12;
    NiPoint3 v25;
    NiPoint3 v27;
    bool btriggerFound = false;
    if (GetProjectileBase()->GetHasMuzzleFlash() &&
        (&this->GetProjectileBase()->GetMuzzleFlashModel())->GetModelLength()) {
        hkpBroadPhase* pbroadphase;
        bhkWorld* pworld;
        if (this->GetParentCell()) {
            pworld = this->GetParentCell()->GetbhkWorld();
        }
        else {
            pworld = nullptr;
        }
        if (!pworld) {
            return btriggerFound;
        }
        hkpWorld* phkworld = (hkpWorld*)pworld->operator*();
        if (phkworld) {
            pbroadphase = phkworld->m_broadPhase;
        }
        else {
            pbroadphase = nullptr;
        }
        NiPoint3* v9 = ::new (&v27) NiPoint3(1.0f * NiPoint3_UNIT_ALL);
        const NiPoint3* v2 = &this->GetLocationOnReference();
        NiPoint3 min = *v2 - *v9;
        NiPoint3* v10 = ::new (&v25) NiPoint3(1.0f * NiPoint3_UNIT_ALL);
        const NiPoint3* v3 = &this->GetLocationOnReference();
        alignas(16) NiPoint3 max = *v3 + *v10;
        hkAabb aabb;
        NI2HK(aabb.m_min, min);
        NI2HK(aabb.m_max, max);
        using hits_t = hkInplaceArray<hkpBroadPhaseHandlePair, 128, hkContainerHeapAllocator>;
        alignas(16) hits_t hits(0);
        pbroadphase->querySingleAabb(aabb, hits);
        for (int32_t i = 0; !btriggerFound; ++i) {
            bhkPhantom* pbp;
            int32_t FormHealth_0 = hits.m_size;
            if (i >= FormHealth_0) {
                break;
            }
            hkpBroadPhaseHandlePair* ppair = &hits.operator[](i);
            hkpTypedBroadPhaseHandle* tp = static_cast<hkpTypedBroadPhaseHandle*>(ppair->m_b);
            const hkpCollidable* collB = static_cast<const hkpCollidable*>(tp->getOwner());
            NiAVObject* pav = GetAVObjectForCollidable(collB);
            TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
            hkpPhantom* pphantom = hkpGetPhantom(collB);
            if (pphantom) {
                pbp = bhkPhantom::Getbhk(pphantom);
            }
            else {
                pbp = nullptr;
            }
            bhkPhantom* pbp_2 = pbp;
            if ((pbp) && (pref)) {
                CFilter* v6;
                CFilter* CollisionFilter = pbp_2->GetCollisionFilter(&v12);
                if (CollisionFilter->GetLayer() == L_PROJECTILEZONE ||
                    (v6 = pbp_2->GetCollisionFilter(&v11), v6->GetLayer() == L_GASTRAP)) {
                    btriggerFound = true;
                    ExtraDataList* scale = &pref->m_Extra;
                    Script::SetActionFlag(this, scale, SCRIPT_ACTION_ONHIT);
                    ExtraDataList* scalea = &pref->m_Extra;
                    Script::SetActionFlag(pGasTrapDummyWeap, scalea, SCRIPT_ACTION_ONHITWITH);
                }
            }
        }
    }
    return btriggerFound;
}

void Projectile::SpawnCollisionEffects(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    HavokMaterialType aeMaterial
) {
    NiCamera* v7;
    double v32;
    NiColor kVertexColor;
    hkpPropertyValue arResult;
    CFilter v39;
    FNVD_INLINED_LOCAL(BSSoundHandle, soundHandle_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, soundHandle, std::nullopt, std::nullopt);
    bool bcreateParticles = true;
    NiCamera* v6 = Main::WorldRootCamera();
    NiPoint3 distance = aContactPoint - v6->GetWorldTranslate();
    if (Main::GetSceneIsComplex() ||
        ((v32 = distance.Length(), fGunParticleCameraDistance.Float() < v32) ||
         (v7 = Main::WorldRootCamera(), !PointInFrustum(aContactPoint, v7, 32.0f)))) {
        bcreateParticles = false;
    }
    if (!apCollidee || !apCollidee->IsActor()) {
        if (apCollidee) {
            Script::SetActionFlag(this, &apCollidee->m_Extra, SCRIPT_ACTION_ONHIT);
            if (pWeaponSource) {
                Script::SetActionFlag(pWeaponSource, &apCollidee->m_Extra, SCRIPT_ACTION_ONHITWITH);
            }
        }
        BGSImpactData* pimpactData = pWeaponSource ? pWeaponSource->GetImpactDataForMaterial(aeMaterial) : nullptr;
        if (pimpactData) {
            float afLifetime;
            const char* v15;
            BGSImpactData::ORIENTATION Interior;
            NiPoint3 impactEffectVector_2;
            const char* pfilename;
            NiPoint3 arg4;
            NiPoint3 v80;
            BGSTextureSet* pset = pimpactData->pDecalTextureSet;
            if (pset) {
                NiPoint3 dir = aContactNormal;
                dir.Unitize();
                NiPoint3 origin = aContactPoint;
                if (fDecalSize == 0.0) {
                    float a1 = pimpactData->GetDecalMaxWidth();
                    float y = pimpactData->GetMinWidth();
                    fDecalSize = RandomFloat(y, a1);
                }
                DecalCaster* pcaster = DecalCaster::GetInstance(fDecalSize, true);
                pcaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
                pcaster->SetCollision(L_BIPED, false);
                pcaster->SetCollision(L_DEADBIP, false);
                TESObjectCELL* pcell = GetParentCell();
                bhkWorld* pw = pcell ? pcell->GetbhkWorld() : nullptr;
                if (pw) {
                    pcaster->Add(pw);
                    hkpAllCdBodyPairCollector collector;
                    pcaster->GetPenetrations(aContactPoint, collector);
                    int32_t ihits = (&collector.m_hits)->m_size;
                    float frotation = RandomFloatTwoPi();
                    float a1a = RandomFloat(0.0f, 4.0f);
                    uint8_t csubtex = static_cast<int32_t>(std::floor(a1a));
                    const hkpCollidable* plastCol = nullptr;
                    for (int32_t I = 0;; ++I) {
                        if (I >= ihits) {
                            if (ihits > 0) {
                                fDecalSize = 0.0f;
                            }
                            pcaster = nullptr;
                            break;
                        }
                        const hkpCollidable* pcol =
                            (&(&collector.m_hits)->operator[]<hkpRootCdBodyPair>(I))->m_rootCollidableB;
                        if (pcol) {
                            HavokMaterialType ematerialB = aeMaterial;
                            uint32_t ekey = (&(&collector.m_hits)->operator[]<hkpRootCdBodyPair>(I))->m_shapeKeyB;
                            if (ekey == -1) {
                                bhkShape* pshape_2 = bhkShape::Getbhk(pcol->m_shape);
                                if (!pshape_2) {
                                    continue;
                                }
                                ematerialB = pshape_2->GetMaterial();
                            }
                            else {
                                bhkShape* pshape = bhkShape::Getbhk(pcol->m_shape);
                                if (!pshape) {
                                    continue;
                                }
                                bhkShapeCollection* pshapeCol = pshape->FindShapeCollection();
                                if (pshapeCol) {
                                    ematerialB = pshapeCol->QSubMaterial(ekey);
                                }
                            }
                            if (ematerialB == aeMaterial && pcol != plastCol) {
                                plastCol = pcol;
                                uint32_t a1b = pcol->getCollisionFilterInfo();
                                uint32_t Group = FNVD_RECONSTRUCT(CFilter)(&v39, a1b)->GetGroup();
                                bool bcasterHitLandHeightField = Group == 1;
                                NiAVObject* pavobj = nullptr;
                                if (Group == 1) {
                                    hkpRigidBody* prb = hkpGetRigidBody(pcol);
                                    if (prb) {
                                        // The property is keyed by NiTriStrips' RTTI address; its value is the scene
                                        // object's pointer.
                                        hkpPropertyValue* Property = bhkWorldObject::GetProperty(
                                            &arResult, prb, bhkPropertyKey(NiTriStrips::ms_RTTI)
                                        );
                                        pavobj = static_cast<NiAVObject*>(Property->getPtr());
                                        if ((pavobj) && (pavobj->m_pkParent)) {
                                            pavobj = (NiAVObject*)pavobj->m_pkParent;
                                        }
                                    }
                                }
                                else {
                                    pavobj = bhkUtilFunctions::GetNiAVObject(pcol);
                                }
                                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pavobj);
                                if (bcasterHitLandHeightField ||
                                    (pref &&
                                     BGSDecalManager::IsValidDecalImpact(pref, BGSDecalManager::DECAL_TYPE_SIMPLE))) {
                                    TESObjectCELL* pcell_2 = TES::pInstance->GetCellForPoint(aContactPoint);
                                    DECAL_CREATION_DATA data;
                                    data.pOrigin = origin;
                                    data.pDirection = dir;
                                    data.fDecalWidth = fDecalSize;
                                    data.fDecalHeight = fDecalSize;
                                    data.fDecalDepth = pimpactData->GetDecalDepth();
                                    data.pAVObj = pavobj;
                                    data.pTexSet = pset;
                                    data.fRotation = frotation;
                                    data.cSubTex = csubtex;
                                    data.bAlphaBlending = pimpactData->GetIsAlphaBlend();
                                    data.bAlphaTesting = pimpactData->GetIsAlphaTest();
                                    data.bUseParallaxOcclusion = pimpactData->GetIsParallax();
                                    data.fParallaxOcclusionScale = pimpactData->GetDecalParallaxScale();
                                    data.ucParallaxOcclusionMaxPasses = pimpactData->GetParallaxPasses();
                                    data.fSpecular = pimpactData->GetDecalShininess();
                                    data.fDecalEpsilon = pimpactData->Data.fAngleThreshold;
                                    float v29 =
                                        static_cast<double>(static_cast<uint8_t>(pimpactData->GetDecalColor() >> 16)) /
                                        255.0;
                                    float v28 = static_cast<double>(static_cast<uint8_t>(
                                                    static_cast<uint16_t>(pimpactData->GetDecalColor()) >> 8
                                                )) /
                                        255.0;
                                    float v27 =
                                        static_cast<double>(static_cast<uint8_t>(pimpactData->GetDecalColor())) / 255.0;
                                    data.fVertexColor = *FNVD_RECONSTRUCT(NiColor)(&kVertexColor, v27, v28, v29);
                                    if (ematerialB == BHK_MATERIAL_GLASS) {
                                        data.bTwoSided = true;
                                    }
                                    pcell_2->AddDecal(data, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
                                }
                            }
                        }
                    }
                }
            }
            if (!bcreateParticles) {
                goto LABEL_63;
            }
            pfilename = pimpactData->GetModel();
            if (!pfilename || !*pfilename) {
                goto LABEL_63;
            }
            Interior = pimpactData->Data.eOrient;
            if (Interior) {
                if (Interior == BGSImpactData::ORIENT_PROJ_VECTOR) {
                    impactEffectVector_2 = -MovementDirection;
                    impactEffectVector_2.Unitize();
                    goto LABEL_62;
                }
                if (Interior == BGSImpactData::ORIENT_PROJ_REFLECT) {
                    NiPoint3 projectileVec = -MovementDirection;
                    projectileVec.Unitize();
                    NiPoint3 normalVec = aContactNormal;
                    normalVec.Unitize();
                    float v25 = normalVec.Dot(projectileVec) * 2.0;
                    impactEffectVector_2 = v25 * normalVec - projectileVec;
                    goto LABEL_62;
                }
            }
            impactEffectVector_2 = aContactNormal;
            impactEffectVector_2.Unitize();
        LABEL_62:;
            v80 = aContactPoint;
            arg4 = impactEffectVector_2;
            v15 = pfilename;
            afLifetime = pimpactData->Data.fEffectDuration;
            BSTempEffectParticle::Spawn(GetParentCell(), afLifetime, v15, arg4, v80, 1.0f, 7u, nullptr);
        LABEL_63:;
            ActorCause* pcause = GetActorCause();
            if ((pcause) && (pcause->pActor)) {
                Actor* pact = pcause->pActor;
                if (pact->pCurrentProcess) {
                    BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
                    int32_t SoundLevelValue = pimpactData->GetSoundLevelValue();
                    CurrentAIProcess->SetActorsDetectionEvent(pact, aContactPoint, SoundLevelValue, 0, nullptr);
                }
            }

            TESSound* pimpactSound1 = pimpactData->pSound1;
            if (pimpactSound1) {
                const char* x = pimpactSound1->GetFilename();
                BSAudio* v12 = &BSAudio::QInstance();
                v12->GetSoundHandleByFilename(
                    &soundHandle, x,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pimpactSound1
                );
                soundHandle.SetPosition(aContactPoint.x, aContactPoint.y, aContactPoint.z);
                soundHandle.Play(false);
            }
            TESSound* pimpactSound2 = pimpactData->pSound2;
            if (pimpactSound2) {
                const char* xa = pimpactSound2->GetFilename();
                BSAudio* v13 = &BSAudio::QInstance();
                v13->GetSoundHandleByFilename(
                    &soundHandle_2, xa,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pimpactSound2
                );
                soundHandle_2.SetPosition(aContactPoint.x, aContactPoint.y, aContactPoint.z);
                soundHandle_2.Play(false);
            }
        }
    }
}

void Projectile::SpawnMuzzleFlash() {
    if ((GetProjectileBase()->GetHasMuzzleFlash()) &&
        ((&GetProjectileBase()->GetMuzzleFlashModel())->GetModelLength())) {
        Actor* pactor;
        TESObjectREFR* pref = GetShooter();
        if (pref && pref->IsActor()) {
            pactor = static_cast<Actor*>(pref);
        }
        else {
            pactor = nullptr;
        }
        if ((pref) && (!pref->IsDead(false) && !pref->IsKnockedOut() && (!pactor || !pactor->IsReloading()))) {
            MuzzleFlash::SpawnMuzzleFlash(pref, this);
        }
    }
}

bool Projectile::GetMineReactsToTarget(Actor* apTarget, Actor* apShooterActor, TESFaction* apOwnerFaction) {
    bool bresult = false;
    bool bconsiderShooter = iProjectileMineShooterCanTrigger.Int() > 0;
    bool bcrimeCaused = false;
    if ((bconsiderShooter || apTarget != GetShooter()) && !apTarget->IsDead(false) &&
        (!apOwnerFaction || apTarget->GetFactionRank(apOwnerFaction, true) == -1)) {
        FIGHT_REACTION efactionFight;
        if (apShooterActor) {
            efactionFight = apShooterActor->GetFactionFightReaction(apTarget, bcrimeCaused);
        }
        else {
            efactionFight = FIGHT_REACTION_NEUTRAL;
        }
        switch (efactionFight) {
            case FIGHT_REACTION_NEUTRAL:
            case FIGHT_REACTION_ENEMY: return true;
            default: break;
        }
    }
    return bresult;
}

bool Projectile::CheckExplosionProximity() {
    float ftriggerChance;
    float fproximity = GetProjectileBase()->Data.fExplosionProximity;
    if ((GetParentCell()) && (!GetParentCell()->GetInterior())) {
        fproximity = fproximity * fMineExteriorRadiusMult.Float();
    }

    float fsqrProximity_2 = fproximity * fproximity;
    MobileObject* pmob = nullptr;
    bool bretVal = false;
    Actor* ptestFightActor = nullptr;
    TESFaction* pownerFaction = nullptr;
    if (pShooter && pShooter->IsActor()) {
        ptestFightActor = static_cast<Actor*>(pShooter);
    }
    else {
        TESForm* powner = GetOwner();
        if (powner) {
            ENUM_FORM_ID eownerType = powner->GetFormType();
            if (eownerType == FACT_ID) {
                pownerFaction = static_cast<TESFaction*>(powner);
            }
        }
    }
    for (uint32_t i = 0;; ++i) {
        Actor* v12;
        uint32_t ProcessLevelTail = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
        if (i >= ProcessLevelTail) {
            break;
        }
        pmob = (&ReferenceProcessLists.MobProcessArray)->GetItem(i);
        if (pmob && pmob->IsActor()) {
            v12 = static_cast<Actor*>(pmob);
        }
        else {
            v12 = nullptr;
        }
        Actor* pactor = v12;
        if ((v12) && (!pactor->DoesFly() && !pactor->IsChild(false) && !pactor->GetGhost())) {
            const NiPoint3* v9 = &GetLocationOnReference();
            const NiPoint3* v2 = &pmob->GetLocationOnReference();
            float fsqrDistance = (*v2 - *v9).SqrLength();
            if (fsqrProximity_2 > static_cast<double>(fsqrDistance) &&
                GetMineReactsToTarget(pactor, ptestFightActor, pownerFaction)) {
                bretVal = true;
                break;
            }
        }
    }
    if (!IsFlagSet(Projectile::NO_PLAYER_PROXIMITY) && !bretVal) {
        pmob = (MobileObject*)pPlayer;
        const NiPoint3* v10 = &GetLocationOnReference();
        const NiPoint3* v4 = &pmob->GetLocationOnReference();
        float fsqrDistance_2 = (*v4 - *v10).SqrLength();
        if (fsqrProximity_2 > static_cast<double>(fsqrDistance_2) &&
            GetMineReactsToTarget(pPlayer, ptestFightActor, pownerFaction)) {
            ftriggerChance = 100.0f;
            BGSProjectile* ProjectileBase = GetProjectileBase();
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::CALCULATE_MINE_EXPLODE_CHANCE, pPlayer, ProjectileBase, &ftriggerChance
            );
            if (Random() % 100 >= static_cast<uint32_t>(static_cast<int64_t>(ftriggerChance))) {
                SetFlag(Projectile::NO_PLAYER_PROXIMITY, true);
            }
            else {
                bretVal = true;
            }
        }
    }
    if ((bretVal) && (pmob)) {
        if (pmob->IsNpc() || pmob == pPlayer) {
            float* v6 = &fMinesDelayMin.Float();
            double AltTimer = GetProjectileBase()->GetAltTimer();
            // Only actors pass the test above, so pmob is treated as an Actor to reach its ActorValueOwner.
            ActorValueOwner* pvalueOwner = static_cast<Actor*>(pmob);
            fExplosionTimer =
                static_cast<double>(pvalueOwner->GetClampedActorValue(ActorValue::EXPLOSIVES)) * AltTimer / 100.0 + *v6;
        }
        else {
            fExplosionTimer = fMinesDelayMin.Float();
        }
    }

    return bretVal;
}

void Projectile::AttachLight() {
    if ((Get3D()) && (GetProjectileBase()->Data.pLight) && (!GetDelete())) {
        NiNode* v2 = Get3D();
        TESObjectLIGH* v1 = GetProjectileBase()->Data.pLight;
        spLight.operator=(v1->GenDynamic(this, v2, false));
    }
}

Projectile::ImpactData* Projectile::GetImpactData(TESObjectREFR* apRef) {
    Projectile::ImpactData* presult = nullptr;
    BSSimpleList<Projectile::ImpactData*>* plist = &ImpactList;
    while (!presult && (plist && !plist->IsEmpty())) {
        Projectile::ImpactData* pdata = plist->m_item;
        plist = plist->m_pkNext;
        if (pdata->pCollidee == apRef) {
            presult = pdata;
        }
    }
    return presult;
}

void Projectile::CheckRefDeleted(TESObjectREFR* apDeletedRef) {
    if (GetDesiredTarget() == apDeletedRef) {
        pDesiredTarget = nullptr;
        if (IsFlagSet(Projectile::ACTIVE_IN_INVENTORY)) {
            Kill();
        }
    }
    if (!ImpactList.IsEmpty()) {
        bool bfound = false;
        BSSimpleList<Projectile::ImpactData*>* impactList = &ImpactList;
        while (!bfound && (impactList && !impactList->IsEmpty())) {
            Projectile::ImpactData* pdata = impactList->m_item;
            impactList = impactList->m_pkNext;
            if (pdata->pCollidee == apDeletedRef) {
                Kill();
                bfound = true;
            }
        }
    }
    if (GetShooter() == apDeletedRef) {
        Kill();
    }
    if ((pTarget) && (pTarget->pReference == apDeletedRef)) {
        ProjectileTarget* apThis = pTarget;
        if (apThis) {
            delete apThis;
        }
        pTarget = nullptr;
    }
}

void Projectile::StartCountdownSound() {
    FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
    if ((!sndCountdown.IsValid()) && (GetProjectileBase()->Data.pCountdownSound)) {
        int32_t refID = GetProjectileBase()->Data.pCountdownSound->iFormID;
        BSSoundHandle* arHandle = PlaySound(
            &a2, refID, false,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
        sndCountdown.operator=(*arHandle);
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
        float v24 = static_cast<double>(i) / static_cast<double>(aiCount) * 6.2831854820251465 + afRotation;
        double v8 = std::sin(v24);
        float v21 = v8 * afConeAngle + afLooking;
        double v9 = std::cos(v24);
        float v17 = v9 * afConeAngle + afHeading;
        float v22 = std::cos(v21);
        float v19 = std::sin(v21);
        float v18 = std::cos(v17);
        float v23 = std::sin(v17);
        float z = -v19;
        float v14 = v18 * v22;
        float v13 = v23 * v22;
        NiPoint3 dst(v13, v14, z);
        apPoints[i] = aOrigin + dst * afRange;
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
    NiPoint3 v50;
    NiPoint3 result;
    NiPoint3 dst;
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    float fEntrySpeed;
    Projectile* pprojectile = nullptr;
    bool binitialize = true;
    if (apProjectileBase) {
        BGSProjectile::BGSProjectileFlags MotionType = apProjectileBase->GetMotionType();
        if (MotionType > BGSProjectile::MOTION_BEAM) {
            if (MotionType == BGSProjectile::MOTION_FLAMETHROWER) {
                pprojectile = Projectile::FindExistingProjectile(apShooter, apProjectileBase);
                if (pprojectile) {
                    FlameProjectile* pflame = static_cast<FlameProjectile*>(pprojectile);
                    pflame->ResetExpirationTimer();
                    binitialize = false;
                }
                else {
                    Projectile* pprojectile_5;
                    FlameProjectile* v57 = static_cast<FlameProjectile*>(ourMalloc(sizeof(FlameProjectile)));
                    if (v57) {
                        pprojectile_5 = ::new (v57)
                            FlameProjectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle);
                    }
                    else {
                        pprojectile_5 = nullptr;
                    }
                    pprojectile = pprojectile_5;
                }
            }
            else {
                if (MotionType == BGSProjectile::MOTION_CONTINUOUS_BEAM) {
                    Projectile* pprojectile_4;
                    ContinuousBeamProjectile* v59 =
                        static_cast<ContinuousBeamProjectile*>(ourMalloc(sizeof(ContinuousBeamProjectile)));
                    if (v59) {
                        pprojectile_4 = ::new (v59) ContinuousBeamProjectile(
                            apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle
                        );
                    }
                    else {
                        pprojectile_4 = nullptr;
                    }
                    pprojectile = pprojectile_4;
                }
            }
        }
        else {
            switch (MotionType) {
                case BGSProjectile::MOTION_BEAM: {
                    Projectile* pprojectile_3;
                    BeamProjectile* v61 = static_cast<BeamProjectile*>(ourMalloc(sizeof(BeamProjectile)));
                    if (v61) {
                        pprojectile_3 = ::new (v61)
                            BeamProjectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle);
                    }
                    else {
                        pprojectile_3 = nullptr;
                    }
                    pprojectile = pprojectile_3;
                    break;
                }
                case BGSProjectile::MOTION_MISSILE: {
                    Projectile* pprojectile_1;
                    MissileProjectile* v65 = static_cast<MissileProjectile*>(ourMalloc(sizeof(MissileProjectile)));
                    if (v65) {
                        pprojectile_1 = ::new (v65)
                            MissileProjectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle);
                    }
                    else {
                        pprojectile_1 = nullptr;
                    }
                    pprojectile = pprojectile_1;
                    break;
                }
                case BGSProjectile::MOTION_GRENADE: {
                    Projectile* pprojectile_2;
                    GrenadeProjectile* v63 = static_cast<GrenadeProjectile*>(ourMalloc(sizeof(GrenadeProjectile)));
                    if (v63) {
                        pprojectile_2 = ::new (v63)
                            GrenadeProjectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle);
                    }
                    else {
                        pprojectile_2 = nullptr;
                    }
                    pprojectile = pprojectile_2;
                    break;
                }
            }
        }
    }
    if ((apShooter == pPlayer) && (apFromWeapon)) {
        BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
        ItemChange* pitem = CurrentAIProcess->GetCurrentWeapon();
        if ((pitem) && (pitem->HasModEffectActive(WEAPON_MOD_PROJECTILE_SPEED))) {
            double Time = pprojectile->fSpeedMult;
            float v41 = apFromWeapon->GetModEffectValue(WEAPON_MOD_PROJECTILE_SPEED, 0) * Time;
            pprojectile->SetSpeedMult(v41);
        }

        if (apFromWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) {
            int32_t iStrReq = apFromWeapon->GetStrRequired();
            int32_t playStr = pPlayer->GetActorValue(ActorValue::STRENGTH);
            float fStrengthMult = fThrowingStrengthPenalty.Float() * static_cast<double>(playStr - iStrReq) + 1.0;
            float v40 = pprojectile->fSpeedMult * fStrengthMult;
            pprojectile->SetSpeedMult(v40);
        }
    }

    if ((pprojectile) && (apShooter) && (apShooter->IsActor())) {
        Actor* actor = static_cast<Actor*>(apShooter);
        fEntrySpeed = 1.0;
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::THROWING_VELOCITY_MODIFIER, static_cast<Actor*>(apShooter), apFromWeapon, &fEntrySpeed
        );
        if (fEntrySpeed != 1.0) {
            double v14 = pprojectile->fSpeedMult;
            float v39 = v14 * fEntrySpeed;
            pprojectile->SetSpeedMult(v39);
        }
        if (actor->GetActorValue(ActorValue::TURBO) > 0) {
            double v38 = pprojectile->fSpeedMult;
            float v37 = 1.0 / BSTimer::QGlobalTimeMultiplier() * v38;
            pprojectile->SetSpeedMult(v37);
        }
    }

    if (pprojectile) {
        pprojectile->SetAngularMomentumZ(afSpreadHeading);
        pprojectile->SetAngularMomentumX(afSpreadLooking);
    }
    if (pprojectile) {
        pprojectile->SetFlag(Projectile::FAKE_SHOT, abDoNoDamage);
    }
    if ((pprojectile) && (binitialize)) {
        if ((apShooter) && (apShooter->IsActor())) {
            ActorCause* v36;
            ActorCause* v55 = static_cast<ActorCause*>(ourMalloc(sizeof(ActorCause)));
            if (v55) {
                v36 = ::new (v55) ActorCause(static_cast<Actor*>(apShooter));
            }
            else {
                v36 = nullptr;
            }
            pprojectile->SetActorCause(v36);
        }

        pprojectile->SetPickCache(apPickCache);
        if (apShooter == pPlayer) {
            TESFaction* pfaction =
                static_cast<TESFaction*>(BGSDefaultObjectManager::GetDefaultObject(DO_PLAYER_FACTION));
            if (pfaction) {
                pprojectile->SetOwner(pfaction);
            }
            if (apFromWeapon->IsThrownWeapon()) {
                float* v15 = &fArrowBowTimerBase.Float();
                float* v16 = &fArrowBowTimerMult.Float();
                float v35 = pPlayer->GetTimeGrenadeHeld() * *v16 + *v15;
                float y = BSMIN(1.0f, v35);
                pprojectile->SetPower(y);
            }
        }
        if (apShooterCombatController) {
            apShooterCombatController->FiredProjectile(pprojectile);
        }
        if (apHomingTarget) {
            pprojectile->pDesiredTarget = apHomingTarget;
        }
        if (abAlwaysHit) {
            pprojectile->SetFlag(Projectile::ALWAYS_HIT_TARGET, true);
        }
        if (apParentCell) {
            pprojectile->SetParentCell(apParentCell);
        }
        pprojectile->Initialize();
    }

    if (pprojectile && (apProjectileBase && apProjectileBase->Data.pActiveSoundLoop)) {
        if (!apProjectileBase->GetSupersonic() &&
            (!apProjectileBase->GetHasExplosionAltTrigger() || apProjectileBase->Data.fExplosionProximity <= 0.0)) {
            TESSound* x = apProjectileBase->Data.pActiveSoundLoop;
            const char* afScale = apProjectileBase->Data.pActiveSoundLoop->GetFilename();
            BSAudio* v17 = &BSAudio::QInstance();
            BSSoundHandle* arHandle = v17->GetSoundHandleByFilename(&out, afScale, 0x40000012u, x);
            pprojectile->sndHandle.operator=(*arHandle);
            const NiPoint3* v18 = &apShooter->GetLocationOnReference();
            pprojectile->sndHandle.SetPosition(*v18);
            NiNode* v19 = pprojectile->Get3D();
            pprojectile->sndHandle.SetObjectToFollow(v19);
            uint8_t MaxDistance = apProjectileBase->Data.pActiveSoundLoop->GetMaxDistance();
            pprojectile->fSoundDist = static_cast<float>(100 * MaxDistance);
        }
        if (pprojectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
            pprojectile->sndHandle.Play(true);
            pprojectile->bSoundFired = true;
        }
    }
    else {
        if ((pprojectile) && (pprojectile->IsFlagSet(Projectile::HITSCAN_MODE))) {
            pprojectile->bSoundFired = true;
        }
    }
    if ((bIsProjectileDebug) && (pprojectile) && (pprojectile->Get3D())) {
        NiMatrix3 rot;
        NiColorA color(1.0f, 0.0f, 0.0f, 0.6f);
        NiAVObject* pdebug = (NiAVObject*)MakeTriPoint(4.0f, color, true);
        pdebug->UpdateProperties();
        const NiPoint3* xa = &pprojectile->Get3D()->GetWorldTranslate();
        pdebug->SetTranslate(*xa);
        float* v21 = &fProjectileDebugDuration.Float();
        TES::pInstance->AddTempDebugObject(pdebug, *v21);
        float angle = pprojectile->GetHeading(false);
        rot.MakeZRotation(angle);
        NiColorA kLineColor(1.0f, 1.0f, 0.0f, 0.6f);
        color = kLineColor;
        NiPoint3* v22 = ::new (&dst) NiPoint3(0.0f, 1000.0f, 0.0f);
        NiPoint3* fR = ::new (&result) NiPoint3(rot * *v22);
        NiNode* v23 = pprojectile->Get3D();
        NiPoint3* fRa = ::new (&v50) NiPoint3(v23->GetWorldTranslate() + *fR);
        NiNode* v24 = pprojectile->Get3D();
        pdebug = (NiAVObject*)MakeLine(v24->GetWorldTranslate(), color, *fRa, color, true);
        float* v25 = &fProjectileDebugDuration.Float();
        TES::pInstance->AddTempDebugObject(pdebug, *v25);
    }

    if (pprojectile) {
        pCombatManager->pThreatMap->AddProjectile(pprojectile);
    }
    return pprojectile;
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
            if (pShape && (pWorldObject->GetCollisionFilter(&kFilter), kFilter.QBipLayer()) &&
                (!abIgnoreHead ||
                 (pWorldObject->GetCollisionFilter(&kHeadFilter), kHeadFilter.GetBipedPart()) != P_HEAD)) {
                alignas(16) hkTransform kTransform;
                kTransform.~hkTransform();
                ::new (&kTransform) hkTransform;
                bhkRigidBody* pBody = static_cast<bhkRigidBody*>(pWorldObject);
                if (NiObject::IsKindOf_0(&bhkBlendCollisionObject::ms_RTTI, pCollisionObject)) {
                    bhkBlendCollisionObject* pBlend = static_cast<bhkBlendCollisionObject*>(pCollisionObject);
                    if (!pWorldObject->IsAddedToWorld() || pBlend->GetHeirGain() == 1.0) {
                        NI2HK(kTransform, apObject->GetWorldTransform());
                        if (NiObject::IsKindOf_0(&bhkRigidBodyT::ms_RTTI, pWorldObject)) {
                            bhkRigidBodyT* pBodyT = static_cast<bhkRigidBodyT*>(pWorldObject);
                            alignas(16) hkTransform kLocal;
                            kLocal.~hkTransform();
                            ::new (&kLocal) hkTransform;
                            alignas(16) hkTransform kWorld;
                            kWorld.~hkTransform();
                            ::new (&kWorld) hkTransform(kTransform);
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
                kInput.~hkpShapeRayCastInput();
                ::new (&kInput) hkpShapeRayCastInput;
                hkpShapeRayCastOutput kOutput;
                kOutput.~hkpShapeRayCastOutput();
                ::new (&kOutput) hkpShapeRayCastOutput;
                kInput.m_from.setTransformedInversePos(kTransform, *apPickData->GethkFrom());
                kInput.m_to.setTransformedInversePos(kTransform, apPickData->GethkTo());
                pShape->operator*()->castRay(kInput, kOutput);
                if (kOutput.hasHit()) {
                    hkpRayHitCollector* pCollector = apPickData->GetAllCollector();
                    const hkpCollidable* pCollidable = pWorldObject->operator*()->getCollidable();
                    pCollector->addRayHit(*pCollidable, kOutput);
                    bHit = true;
                }
            }
        }
        NiNode* pNode = apObject->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                bHit = bHit | PickSceneGraph(apPickData, pChild, abIgnoreHead, false);
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
    bhkPhantom* pbp;
    hkpPhantom* pph;
    CFilter v13;
    CFilter v14;
    CFilter apResult;
    CFilter a2;
    if (apTargetCollidable) {
        pph = hkpGetPhantom(apTargetCollidable);
    }
    else {
        pph = nullptr;
    }
    if (pph) {
        pbp = bhkPhantom::Getbhk(pph);
    }
    else {
        pbp = nullptr;
    }
    if (pbp) {
        bhkRigidBody* pfirstbody;
        NiNode* pav;
        if (NiObject::IsKindOf_0(&bhkCollisionBox::ms_RTTI, pbp)) {
            return false;
        }
        CFilter* CollisionFilter = pbp->GetCollisionFilter(&a2);
        if (CollisionFilter->GetLayer() != L_CHARCONTROLLER) {
            return false;
        }
        if (apCollidee) {
            pav = apCollidee->Get3D();
        }
        else {
            pav = nullptr;
        }
        NiAVObject* pav_2 = (NiAVObject*)pav;
        if (apCollidee == pPlayer) {
            pav_2 = pPlayer->Get3D(false);
        }
        bhkCollisionObject* pcol = FindFirstCollisionObject(pav_2);
        if (pcol) {
            pfirstbody = pcol->GetRigidBody();
        }
        else {
            pfirstbody = nullptr;
        }
        if (pfirstbody) {
            CFilter* v6 = pfirstbody->GetCollisionFilter(&apResult);
            if ((!v6->GetNoCollision()) && (pfirstbody->GethkWorld())) {
                return false;
            }
        }
    }
    bool result = true;
    if ((apProjectileBase) && (apTargetCollidable)) {
        uint32_t v8;
        uint32_t CollisionFilterInfo = apTargetCollidable->getCollisionFilterInfo();
        if (FNVD_RECONSTRUCT(CFilter)(&v14, CollisionFilterInfo)->GetLayer() == L_TRANSPARENT_SMALL ||
            (v8 = apTargetCollidable->getCollisionFilterInfo(),
             FNVD_RECONSTRUCT(CFilter)(&v13, v8)->GetLayer() == L_TRANSPARENT_SMALL_ANIM)) {
            BGSProjectile::BGSProjectileFlags emotion = apProjectileBase->GetMotionType();
            if (emotion != BGSProjectile::MOTION_GRENADE &&
                (emotion != BGSProjectile::MOTION_MISSILE ||
                 (apProjectileBase->GetHitScan() || apProjectileBase->GetPassThroughSmTransp()))) {
                return false;
            }
        }
    }

    return result;
}

NiAVObject* Projectile::ProcessTargetPick(
    bhkPickData* apPickData,
    Projectile* apProjectile,
    BGSProjectile* apProjectileBase,
    float* apHitFraction,
    bool bUseCharacterController,
    float afWaterHitRatio
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, sound_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, sound, std::nullopt, std::nullopt);
    CFilter kPickFilter;
    CFilter v65;
    CFilter v69;
    alignas(16) CFilter v70;
    CFilter v71;
    if ((!apProjectileBase) && (apProjectile)) {
        apProjectileBase = apProjectile->GetProjectileBase();
    }

    bool v85 = false;
    NiAVObject* phitObject = nullptr;
    if ((apPickData) && (apPickData->pAllCollector)) {
        NiPoint3 pickLength;
        NiPoint3 pickOrigin;
        apPickData->GetFrom(pickOrigin);
        apPickData->GetLength(pickLength);
        char v81 = 0;
        TESObjectREFR* ptargetRef = nullptr;
        for (int32_t i = 0; !v85; ++i) {
            int32_t FormHealth_0 = (&apPickData->pAllCollector->getHits())->m_size;
            if (i >= FormHealth_0) {
                break;
            }
            const hkpWorldRayCastOutput* v7 =
                &(&apPickData->pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i);
            hkpWorldRayCastOutput point(*v7);
            if (!(afWaterHitRatio < static_cast<double>(point.m_hitFraction))) {
                NiAVObject* pav = GetAVObjectForCollidable(point.m_rootCollidable);
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                if (pav) {
                    bhkPhantom* pbp;
                    char v75 = 1;
                    hkpPhantom* pphantom = hkpGetPhantom(point.m_rootCollidable);
                    if (pphantom) {
                        pbp = bhkPhantom::Getbhk(pphantom);
                    }
                    else {
                        pbp = nullptr;
                    }
                    bhkPhantom* pbp_2 = pbp;
                    if (pbp) {
                        CFilter* v10;
                        v75 = 0;
                        if ((apProjectile) && (pref)) {
                            CFilter* v9;
                            CFilter* CollisionFilter = pbp_2->GetCollisionFilter(&v71);
                            if (CollisionFilter->GetLayer() == L_PROJECTILEZONE ||
                                (v9 = pbp_2->GetCollisionFilter(&v70), v9->GetLayer() == L_GASTRAP)) {
                                Script::SetActionFlag(apProjectile, &pref->m_Extra, SCRIPT_ACTION_ONHIT);
                                if (apProjectile->pWeaponSource) {
                                    Script::SetActionFlag(
                                        apProjectile->pWeaponSource, &pref->m_Extra, SCRIPT_ACTION_ONHITWITH
                                    );
                                }
                            }
                        }

                        if (NiObject::IsKindOf_0(&bhkCollisionBox::ms_RTTI, pbp_2) ||
                            (bUseCharacterController &&
                             (v10 = pbp_2->GetCollisionFilter(&v69), v10->GetLayer() == L_CHARCONTROLLER))) {
                            bhkRigidBody* pfirstbody;
                            v81 = 1;
                            ptargetRef = pref;
                            bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
                            if (pcol) {
                                pfirstbody = pcol->GetRigidBody();
                            }
                            else {
                                pfirstbody = nullptr;
                            }
                            bhkRigidBody* pfirstbody_2 = pfirstbody;
                            if (pfirstbody) {
                                CFilter* v11 = pfirstbody_2->GetCollisionFilter(&v65);
                                if (v11->GetNoCollision() || !pfirstbody_2->GethkWorld()) {
                                    NiPoint3 length;
                                    NiPoint3 from;
                                    bhkPickData pd;
                                    apPickData->GetFrom(from);
                                    apPickData->GetLength(length);
                                    pickLength.operator*=(2.0f);
                                    length.operator*=(2.0f);
                                    CFilter* v12 = apPickData->GetFilter(&kPickFilter);
                                    pd.SetFilter(*v12);
                                    pd.NiSetFrom(from);
                                    pd.NiSetLength(length);
                                    hkpAllRayHitCollector collector;
                                    pd.SetAllCollector(&collector);
                                    if (Projectile::PickSceneGraph(&pd, pav, false, true) &&
                                        (&pd.pAllCollector->getHits())->m_size > 0) {
                                        const hkpWorldRayCastOutput* v13 =
                                            &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(0);
                                        point.operator=(*v13);
                                        v75 = 1;
                                    }
                                }
                            }
                        }
                    }
                    if (!Projectile::IsValidImpact(pref, point.m_rootCollidable, apProjectile, apProjectileBase)) {
                        v75 = 0;
                    }
                    if (v75) {
                        if (apProjectile) {
                            NiPoint3 normal;
                            alignas(16) NiPoint3 length_2 = pickLength * point.m_hitFraction;
                            NiPoint3 contact = pickOrigin + length_2;
                            HK2NINS(normal, point.m_normal);
                            if (apProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
                                float arPos_8 = length_2.Length();
                                apProjectile->SetRange(arPos_8);
                                apProjectile->SetMovementDelta(length_2, false);
                            }
                            uint32_t outputShapeKey = -1;
                            for (int32_t j = 0; j < 8 && point.m_shapeKeys[j] != -1; ++j) {
                                outputShapeKey = point.m_shapeKeys[j];
                            }
                            v85 = apProjectile->AddImpact(
                                pref, contact, normal, point.m_rootCollidable, outputShapeKey, BHK_MATERIAL_MAX
                            );
                            if (v85) {
                                phitObject = pav;
                            }
                        }
                        else {
                            v85 = true;
                            phitObject = pav;
                        }
                        if ((v85) && (apHitFraction)) {
                            *apHitFraction = point.m_hitFraction;
                        }
                    }
                }
            }
        }
        if (apProjectile) {
            if (!v85 && afWaterHitRatio != 3.4028234663852886e+38) {
                NiPoint3 length_3 = pickLength * afWaterHitRatio;
                NiPoint3 contact_2 = pickOrigin + length_3;
                if (apProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
                    float arPos_8a = length_3.Length();
                    apProjectile->SetRange(arPos_8a);
                    apProjectile->SetMovementDelta(length_3, false);
                }
                apProjectile->AddImpact(
                    nullptr, contact_2, NiPoint3_UNIT_Z, nullptr, HK_INVALID_SHAPE_KEY, BHK_MATERIAL_WATER
                );
            }
            if (v85 || !v81) {
                if ((apProjectile->GetProjectileBase()->Data.pActiveSoundLoop) &&
                    (((v85 && ptargetRef != pPlayer) || !v85) && apProjectile->GetShooter() != pPlayer) &&
                    ((&BSAudio::QInstance())->pListener)) {
                    alignas(16) NiPoint3 toPt;
                    NiPoint3 fromPt;
                    BSAudioListener* v41 = (&BSAudio::QInstance())->pListener;
                    NiPoint3 listenerPos = v41->GetPosition();
                    if (ptargetRef) {
                        toPt = ptargetRef->GetLocationOnReference();
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
                        uint32_t uiFlags_2 = 258;
                        TESSound* arPos_8c = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop;
                        const char* arPosa = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop->GetFilename();
                        BSAudio* v19 = &BSAudio::QInstance();
                        v19->GetSoundHandleByFilename(&sound_2, arPosa, uiFlags_2, arPos_8c);
                        sound_2.SetPosition(pointOnLine.x, pointOnLine.y, pointOnLine.z);
                        sound_2.Play(false);
                        if (bIsProjectileDebug) {
                            NiColorA color_2(0.0f, 1.0f, 1.0f, 0.3f);
                            NiAVObject* pdebug_2 = (NiAVObject*)MakeTriPoint(5.0f, color_2, true);
                            pdebug_2->UpdateProperties();
                            pdebug_2->SetTranslate(pointOnLine);
                            float* v20 = &fProjectileDebugDuration.Float();
                            TES::pInstance->AddTempDebugObject(pdebug_2, *v20);
                        }
                    }
                }
            }
            else {
                if ((ptargetRef) && (ptargetRef == pPlayer)) {
                    NiPoint3 hitPt;
                    apPickData->NiCalcHitPoint(hitPt);
                    if ((apProjectile->GetProjectileBase()) &&
                        (apProjectile->GetProjectileBase()->Data.pActiveSoundLoop)) {
                        uint32_t uiFlags = 258;
                        TESSound* arPos_8b = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop;
                        const char* arPos = apProjectile->GetProjectileBase()->Data.pActiveSoundLoop->GetFilename();
                        BSAudio* v14 = &BSAudio::QInstance();
                        v14->GetSoundHandleByFilename(&sound, arPos, uiFlags, arPos_8b);
                        sound.SetPosition(hitPt.x, hitPt.y, hitPt.z);
                        sound.Play(false);
                        if ((bIsProjectileDebug) && (apProjectile->Get3D())) {
                            NiColorA color(0.0f, 1.0f, 1.0f, 0.3f);
                            NiAVObject* pdebug = (NiAVObject*)MakeTriPoint(5.0f, color, true);
                            pdebug->UpdateProperties();
                            NiNode* v15 = apProjectile->Get3D();
                            pdebug->SetTranslate(v15->GetWorldTranslate());
                            float* v16 = &fProjectileDebugDuration.Float();
                            TES::pInstance->AddTempDebugObject(pdebug, *v16);
                        }
                    }
                }
            }
        }
    }

    return phitObject;
}

float Projectile::ComputeSpeed(float afSpeed, float afPower, float afSpeedMult) {
    return (float)((((&fArrowWeakSpeed)->Float() * (1.0 - afPower)) + (afSpeed * afPower)) * afSpeedMult);
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

ProjectileTarget::ProjectileTarget() : pReference(nullptr), spPart(nullptr), kOffset(NiPoint3_ZERO) {}

ProjectileTarget::~ProjectileTarget() {}

NiPoint3* ProjectileTarget::GetWorldLocation(NiPoint3* apResult) {
    NiPoint3 v7 = NiPoint3_ZERO;
    if (spPart.m_pObject) {
        v7 = spPart.m_pObject->GetWorldTransform() * kOffset;
    }
    else {
        if (pReference) {
            const NiPoint3* Center;
            if (pReference->Get3D()) {
                NiNode* v2 = pReference->Get3D();
                Center = &(&v2->GetWorldBound())->m_kCenter;
            }
            else {
                Center = &pReference->GetLocationOnReference();
            }
            v7 = *Center;
        }
    }
    *apResult = v7;
    return apResult;
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
    m_normal.operator=(arOther.m_normal);
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
