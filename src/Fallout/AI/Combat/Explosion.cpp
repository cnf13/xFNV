#include "Fallout/AI/Combat/Explosion.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkFilteredCdBodyCollector.h"
#include "BSHavok/bhkLiquidAction.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSValueNode.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CameraShake.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/FOHUDEffectManager.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicCaster.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace hkAlgorithm {} // namespace hkAlgorithm

// The SDK's hkAlgorithm::quickSort for the root contact points.
// KEEP: src cannot express it exactly. The exe passes its comparator's byte on to quickSortRecursive (movzx; push at
// 0x9b2eac), whose parameter is the empty hkAlgorithm::less<hkpRootCdPoint>: clang writes no bytes of an empty class
// argument, so src's slot holds other bytes.
void quickSortRootCdPoints(hkpRootCdPoint* apArray, int32_t aiSize, uint8_t aiLess) {
    if (aiSize > 1) {
        hkAlgorithm::quickSortRecursive<hkpRootCdPoint, hkAlgorithm::less<hkpRootCdPoint>>(
            apArray,
            0,
            aiSize - 1,
            hkAlgorithm::less<hkpRootCdPoint>()
        );
    }
}

Explosion::ExplosionTarget::ExplosionTarget()
  : pRef(nullptr), bHitWeakPoint(false), bMobile(false), bProcessed(false) {}

Explosion::ExplosionTarget::~ExplosionTarget() {
    limbList.RemoveAll();
}

Explosion::Explosion()
  : fAge(0.0f), fLifetime(0.0f), fBuildTime(0.0f), fRadius(0.0f), fImageSpaceRadius(0.0f), fWeaponCondition(1.0f),
    spPhantom(nullptr), spLight(nullptr), pOwner(nullptr), pExplodedRef(nullptr), spActorCause(nullptr),
    bDecalsPlaced(false), bTargetsFound(false), bTargetsProcessed(false), bForcesApplied(false),
    bIgnoreImageSpaceSwap(false), bUnderWater(false), pCaster(nullptr), pWeaponSource(nullptr), iFrameCount(0),
    ClosestPoint(NiPoint3::ZERO), ClosestPointNormal(NiPoint3::ZERO), fCalculatedDamage(-1.0f) {
    pCurrentProcess = nullptr;
}

// The base reference's construction, the explosion's vtables and members, the reference's placement, its base form's
// radius, image-space radius and flags (the orientation is reset to identity for one flag), and the starting state.
Explosion::Explosion(
    BGSExplosion* apExplosionBase,
    TESObjectREFR* apOwnerRef,
    TESObjectCELL* apCell,
    NiPoint3 aOrigin,
    NiMatrix3 aRotation
)
  : fAge(0.0f), fLifetime(3.0f), fBuildTime(0.0f), fWeaponCondition(1.0f), spPhantom(nullptr), spLight(nullptr),
    pOwner(apOwnerRef), spActorCause(nullptr), bDecalsPlaced(false), bTargetsFound(false), bTargetsProcessed(false),
    bForcesApplied(false), bIgnoreImageSpaceSwap(false), bUnderWater(false), pWeaponSource(nullptr), iFrameCount(0),
    ClosestPoint(NiPoint3::ZERO), ClosestPointNormal(NiPoint3::ZERO), fCalculatedDamage(-1.0f) {
    pCurrentProcess = nullptr;
    MobileObject::SetParentCell(apCell);
    SetLocationOnReference(aOrigin);
    SetOrientation(aRotation);
    if (apExplosionBase) {
        SetObjectReference(apExplosionBase);
        fRadius = GetExplosionBase()->GetRadiusBSUnits();
        fImageSpaceRadius = GetExplosionBase()->GetISRadius();
        bIgnoreImageSpaceSwap = GetExplosionBase()->GetFlag(BGSExplosion::FLAG_IGNORE_IMAGE_SPACE_SWAP);
        if (GetExplosionBase()->GetFlag(BGSExplosion::FLAG_ALWAYS_USES_WORLD_ORIENTATION)) {
            SetOrientation(NiMatrix3::IDENTITY);
        }
    }
    if (!apOwnerRef || !apOwnerRef->IsActor()) {
        pCaster = new NonActorMagicCaster(this);
    }
    else {
        pCaster = nullptr;
    }
}

Explosion::~Explosion() {
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D && p3D->IsNode() && TESObjectREFR::HasAddonFlags(p3D->IsNode())) {
        TESObjectREFR::RemoveMasterParticleAddonNodes(p3D->IsNode());
    }
    if (GetParentCell() && GetParentCell()->GetbhkWorld()) {
        bhkWorld::RemoveObjects(TESObjectREFR::Get3D(), true, false);
    }
    DetachLight();
    if (spPhantom.m_pObject) {
        spPhantom->Remove();
    }
    spPhantom = nullptr;
    while (!TargetList.IsEmpty()) {
        delete TargetList.m_item;
        TargetList.RemoveHead();
    }
    if (!pDataHandler->GetClearingData()) {
        CleanUpOtherMobileObjects(false);
    }
    StopSounds();
    spActorCause = nullptr;
    delete pCaster;
    // Removes this explosion's node from the water group's explosion list.
    if (gs_bReflectExplosions_Water.Bool()) {
        NiTListItem<TESObjectREFR*>* pPos = PlaceableWaterGroup::ExplosionsList.m_pkHead;
        NiTListItem<TESObjectREFR*>* pFoundPos = nullptr;
        while (pPos && !pFoundPos) {
            NiTListItem<TESObjectREFR*>* pNextPos = PlaceableWaterGroup::ExplosionsList.GetNextPos(pPos);
            if (PlaceableWaterGroup::ExplosionsList.Get(pPos) == this) {
                pFoundPos = pPos;
            }
            pPos = pNextPos;
        }
        PlaceableWaterGroup::ExplosionsList.RemovePos(pFoundPos);
    }
    TESObjectCELL* pCell = GetParentCell();
    if (pCell) {
        BSPortalGraph* pGraph = pCell->GetPortalGraph();
        if (pGraph) {
            pGraph->DetachAlwaysRenderChild(TESObjectREFR::Get3D());
        }
    }
}

void Explosion::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    MobileObject::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&fAge, sizeof(fAge), 0);
    apSaveGameBuffer->SaveDataEndian(&fLifetime, sizeof(fLifetime), 0);
    apSaveGameBuffer->SaveDataEndian(&fRadius, sizeof(fRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&fImageSpaceRadius, sizeof(fImageSpaceRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&bDecalsPlaced, sizeof(bDecalsPlaced), 0);
    apSaveGameBuffer->SaveDataEndian(&iFrameCount, sizeof(iFrameCount), 0);
    apSaveGameBuffer->SaveDataEndian(&fBuildTime, sizeof(fBuildTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fWeaponCondition, sizeof(fWeaponCondition), 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreImageSpaceSwap, sizeof(bIgnoreImageSpaceSwap), 0);
    apSaveGameBuffer->SaveDataEndian(&bUnderWater, sizeof(bUnderWater), 0);
    apSaveGameBuffer->SaveDataEndian(&ClosestPoint, sizeof(ClosestPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&ClosestPointNormal, sizeof(ClosestPointNormal), 0);
    apSaveGameBuffer->SaveDataEndian(&fCalculatedDamage, sizeof(fCalculatedDamage), 0);
    apSaveGameBuffer->SaveFormID(pOwner, 0);
    apSaveGameBuffer->SaveFormID(pExplodedRef, 0);
    apSaveGameBuffer->SaveFormID(pWeaponSource, 0);
    int32_t iActorCauseID = spActorCause.m_pObject ? spActorCause->iActorCauseID : -1;
    apSaveGameBuffer->SaveDataEndian(&iActorCauseID, sizeof(iActorCauseID), 0);
    uint8_t bHasCaster = pCaster != nullptr;
    apSaveGameBuffer->SaveDataEndian(&bHasCaster, sizeof(bHasCaster), 0);
    if (bHasCaster) {
        uint32_t uiSpellFormID = pCaster->pCurrentSpell ? pCaster->pCurrentSpell->GetMagicItemFormID() : 0;
        apSaveGameBuffer->SaveFormID(uiSpellFormID, 0);
        uint32_t uiTargetFormID = pCaster->pDesiredTarget ? pCaster->pDesiredTarget->GetMagicTargetFormID() : 0;
        apSaveGameBuffer->SaveFormID(uiTargetFormID, 0);
        apSaveGameBuffer->SaveFormID(pCaster->pCasterObject, 0);
    }
}

void Explosion::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fAge, sizeof(fAge));
    apLoadGameBuffer->LoadData(&fLifetime, sizeof(fLifetime));
    apLoadGameBuffer->LoadData(&fRadius, sizeof(fRadius));
    apLoadGameBuffer->LoadData(&fImageSpaceRadius, sizeof(fImageSpaceRadius));
    apLoadGameBuffer->LoadData(&bDecalsPlaced, sizeof(bDecalsPlaced));
    apLoadGameBuffer->LoadData(&iFrameCount, sizeof(iFrameCount));
    apLoadGameBuffer->LoadData(&fBuildTime, sizeof(fBuildTime));
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        apLoadGameBuffer->LoadData(&fWeaponCondition, sizeof(fWeaponCondition));
        apLoadGameBuffer->LoadData(&bIgnoreImageSpaceSwap, sizeof(bIgnoreImageSpaceSwap));
        apLoadGameBuffer->LoadData(&bUnderWater, sizeof(bUnderWater));
        apLoadGameBuffer->LoadData(&ClosestPoint, sizeof(ClosestPoint));
        apLoadGameBuffer->LoadData(&ClosestPointNormal, sizeof(ClosestPointNormal));
    }
    if (apLoadGameBuffer->GetVersion() >= 20u) {
        apLoadGameBuffer->LoadData(&fCalculatedDamage, sizeof(fCalculatedDamage));
    }
    apLoadGameBuffer->LoadFormID(pOwner);
    apLoadGameBuffer->LoadFormID(pExplodedRef);
    uint32_t uiWeaponFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pWeaponForm = TESForm::GetFormByNumericID(uiWeaponFormID);
    pWeaponSource = dynamic_cast<TESObjectWEAP*>(pWeaponForm);
    int32_t iActorCauseID = -1;
    apLoadGameBuffer->LoadData(&iActorCauseID, sizeof(iActorCauseID));
    spActorCause = ActorCause::GetActorCauseByID(iActorCauseID);
    bool bHasCaster = true;
    if (apLoadGameBuffer->GetVersion() >= 7u) {
        apLoadGameBuffer->LoadData(&bHasCaster, sizeof(bHasCaster));
    }
    if (bHasCaster) {
        if (!pCaster) {
            NonActorMagicCaster* pNewCaster = new NonActorMagicCaster(this);
            pCaster = pNewCaster;
        }
        apLoadGameBuffer->LoadFormID(pCaster->pCurrentSpell);
        apLoadGameBuffer->LoadFormID(pCaster->pDesiredTarget);
        apLoadGameBuffer->LoadFormID(pCaster->pCasterObject);
    }
    if (iFrameCount > 0) {
        bTargetsFound = true;
        bTargetsProcessed = true;
        bForcesApplied = true;
    }
}

void Explosion::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::InitLoadGame(apLoadGameBuffer);
    TESObjectREFR* pOwnerRef = nullptr;
    if (pOwner) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner)));
        pOwnerRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pOwner = pOwnerRef;
    TESObjectREFR* pExploded = nullptr;
    if (pExplodedRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pExplodedRef)));
        pExploded = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pExplodedRef = pExploded;
    if (pCaster) {
        MagicItem* pSpell = nullptr;
        if (pCaster->pCurrentSpell) {
            pSpell = MagicItem::GetMagicItemByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster->pCurrentSpell))
            );
        }
        pCaster->pCurrentSpell = pSpell;
        MagicTarget* pTarget = nullptr;
        if (pCaster->pDesiredTarget) {
            pTarget = MagicTarget::GetMagicTargetByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster->pDesiredTarget))
            );
        }
        pCaster->pDesiredTarget = pTarget;
        TESObjectREFR* pCasterRef = nullptr;
        if (pCaster->pCasterObject) {
            TESForm* pForm = TESForm::GetFormByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster->pCasterObject))
            );
            pCasterRef = dynamic_cast<TESObjectREFR*>(pForm);
        }
        pCaster->pCasterObject = pCasterRef;
    }
    if (IsDistant()) {
        Initialize();
    }
}

void Explosion::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::Revert(apLoadGameBuffer);
    bDecalsPlaced = false;
    bTargetsFound = false;
    bTargetsProcessed = false;
    bForcesApplied = false;
    spActorCause = static_cast<ActorCause*>(nullptr);
    fWeaponCondition = 1.0f;
    bIgnoreImageSpaceSwap = false;
    bUnderWater = false;
    ClosestPoint = NiPoint3::ZERO;
    ClosestPointNormal = NiPoint3::ZERO;
    while (!TargetList.IsEmpty()) {
        Explosion::ExplosionTarget* pTarget = TargetList.m_item;
        if (pTarget) {
            delete pTarget;
        }
        TargetList.RemoveHead();
    }
}

// TESForm's buffer overload; it only calls MobileObject's.
void Explosion::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    MobileObject::FinishLoadGame(apLoadGameBuffer);
}

void Explosion::SetActorCause(ActorCause* apActorCause) {
    spActorCause = apActorCause;
}

ActorCause* Explosion::GetActorCause() {
    return spActorCause;
}

// The caster's MagicCaster part, or null when the explosion has no caster.
MagicCaster* Explosion::GetMagicCaster() {
    NonActorMagicCaster* pExplosionCaster = pCaster;
    if (pExplosionCaster) {
        return pExplosionCaster;
    }
    return nullptr;
}

void Explosion::InitHavok() {
    if (spPhantom == static_cast<bhkSimpleShapePhantom*>(nullptr) &&
        this->GetParentCell() &&
        this->GetParentCell()->GetbhkWorld()) {
        alignas(16) hkSimpleShapePhantomCinfo info;
        bhkSphereShape* pShape = new bhkSphereShape(this->fRadius, true);
        info.m_shape = **pShape;
        uint32_t uiSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        info.m_collisionFilterInfo = CFilter(L_SPELLEXPLOSION, uiSystemGroup, P_OTHER).iFilter;
        if (this->Get3D()) {
            info.m_transform.NI2HK(this->Get3D()->GetWorldTransform());
        }
        else {
            info.m_transform.setIdentity();
        }
        this->spPhantom = new bhkSimpleShapePhantom(info);
        this->spPhantom->Add(this->GetParentCell()->GetbhkWorld());
    }
}

bool Explosion::IsExplosion() {
    return true;
}

bool Explosion::ChangeProcessLevel() {
    bool bChange = true;
    PROCESS_TYPE eOldLevel = pCurrentProcess->eLevel;
    PROCESS_TYPE eNewLevel = PROCESS_TYPE_LOW;
    TESObjectCELL* pCell = GetParentCell();
    if ((Get3D() || pModelLoader->IsReferenceQueued(this)) && TES::pInstance->IsCellLoaded(pCell, true)) {
        eNewLevel = PROCESS_TYPE_HIGH;
    }
    if (eOldLevel != eNewLevel && eNewLevel) {
        if (this) {
            delete this;
        }
        return false;
    }

    return bChange;
}

bool Explosion::IsTransient() {
    return true;
}

void Explosion::Update(float afDelta) {
    if (static_cast<double>(fBuildTime) <= fAge && !bTargetsFound) {
        FindTargets();
    }
    ++iFrameCount;
    if (!IsDistant()) {
        UpdateRadiation();
        UpdateLight();
    }
    double dAge = fAge;
    if (fLifetime < dAge) {
        dAge = fAge;
        if (GetExplosionBase()->GetDissipationTime() < dAge) {
            Kill();
        }
    }
}

void Explosion::Initialize() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TES_REFERENCES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\Explosion.cpp",
        376
    );
    bool bLoadingSaveGame = pSaveLoadGame->GetSaveGameLoading();
    bool bCreateProcess = pCurrentProcess == nullptr;
    if (bCreateProcess) {
        HighProcess* pProcess = new HighProcess;
        pCurrentProcess = pProcess;
    }
    // Takes the explosion's location before the player's.
    const NiPoint3& rLocation = GetLocationOnReference();
    float fDistance = (pPlayer->GetLocationOnReference() - rLocation).Length();
    float fPower = CombatFormulas::CalcExplosionPower(GetRadius(), fDistance);
    if (fExplosionImageSpaceSwapPower.Float() < fPower && !bIgnoreImageSpaceSwap) {
        ImageSpaceModifierInstanceForm::Trigger(TESImageSpaceModifier::GetExplosionInFace(), 1.0f, nullptr);
    }
    TestExplosionUnderWater();
    Init3D();
    if (IsDistant()) {
        AttachDistant();
    }
    else {
        if (!bLoadingSaveGame && GetExplosionBase()->pImpactPlacedObject) {
            // The arguments are evaluated right to left.
            TESWorldSpace* pWorld = GetWorldSpace();
            TESObjectCELL* pCell = GetParentCell();
            const NiPoint3& rAngle = GetAngleOnReference();
            const NiPoint3& rPosition = GetLocationOnReference();
            pDataHandler->CreateReferenceAtLocation(
                GetExplosionBase()->pImpactPlacedObject,
                rPosition,
                rAngle,
                pCell,
                pWorld,
                nullptr,
                nullptr,
                nullptr
            );
        }
        if (!bLoadingSaveGame &&
            GetExplosionBase()->GetRADRadius() > 0.0 &&
            GetExplosionBase()->Data.fRadiationRads > 0.0) {
            UpdateRadiation();
            m_Extra.SetRadius(GetExplosionBase()->GetRADRadius());
            AddChange(CHANGE_REFR_EXTRA_CREATED_ONLY);
        }
        AttachLight();
        InitHavok();
        TESObjectCELL* pCell = GetParentCell();
        if (pCell) {
            if (!bLoadingSaveGame) {
                pCell->RemoveReference(this);
            }
            pCell->AddReference(this, false);
        }
        NiAVObject* p3D = Get3D();
        if (p3D) {
            p3D->SetAppCulled(fExplosionImageSpaceSwapPower.Float() < fPower && !bIgnoreImageSpaceSwap);
        }
        if (p3D && pCell->GetDynamicNode()) {
            static_cast<NiNode*>(pCell->GetDynamicNode())->AttachChild(p3D, true);
        }
        NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
        BSFadeNode* pFade = pNode ? pNode->IsFadeNode() : nullptr;
        if (pFade) {
            pFade->TurnFadeNodeOn();
        }
        fBuildTime = FindTimeForNoteKey(p3D, "SpecialIdle_AreaEffect", "Hit");
    }
    if (!bLoadingSaveGame) {
        ActorCause* pCause = GetActorCause();
        if (pCause && pCause->pActor && pCause->pActor->pCurrentProcess) {
            Actor* pActor = pCause->pActor;
            BaseProcess* pProcess = pActor->pCurrentProcess;
            // Takes the sound level before the location.
            int32_t iSoundLevel = GetExplosionBase()->GetSoundLevelValue();
            pProcess->SetActorsDetectionEvent(pActor, GetLocationOnReference(), iSoundLevel, 2, pExplodedRef);
        }
        StartSounds();
        if (Sound1.IsValid() || Sound2.IsValid()) {
            const NiPoint3& rShakeLocation = GetLocationOnReference();
            float fShake = fRadius * fCameraShakeExplosionDistMult.Float();
            ShakeCamera(fShake, rShakeLocation);
        }
        if (GetParentCell() && bUnderWater) {
            NiPoint3 splashLocation = GetLocationOnReference();
            splashLocation.z = GetParentCell()->GetWaterHeight();
            double dRadius = GetRadius();
            float fScale = dRadius / fExplosionSplashRadius.Float();
            NiMatrix3 rotation;
            rotation.MakeZRotation(RandomFloatTwoPi());
            const char* pParticles = sExplosionSplashParticles.String();
            BSTempEffectParticle::Spawn(
                pPlayer->GetParentCell(),
                1.0f,
                pParticles,
                rotation,
                splashLocation,
                fScale,
                7,
                nullptr
            );
            BSSoundHandle underWaterSound;
            underWaterSound = BSAudio::QInstance().GetSoundHandleByName(
                "CWaterExplosionSplash",
                static_cast<uint32_t>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT)
            );
            if (underWaterSound.IsValid()) {
                underWaterSound.SetPosition(splashLocation.x, splashLocation.y, splashLocation.z);
                underWaterSound.Play(false);
            }
        }
    }
    if (iExplosionDebug.Int() > 0) {
        NiColorA white(1.0f, 1.0f, 1.0f, 0.65f);
        NiAVObject* pDebug = MakeTriPoint(2.5f, white, true);
        pDebug->UpdateProperties();
        pDebug->SetTranslate(GetLocationOnReference());
        TES::pInstance->AddTempDebugObject(pDebug, 10.0f);
    }
    if (bCreateProcess) {
        ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    }
    if (gs_bReflectExplosions_Water.Bool()) {
        TESObjectREFR* pRef = this;
        PlaceableWaterGroup::ExplosionsList.AddHead(pRef);
    }
}

void Explosion::ApplyImpactDecals() {
    hkpPropertyValue property;
    alignas(16) CFilter filter;
    NiPoint3 location;
    if (spPhantom.m_pObject) {
        if (this->iFrameCount > 0 && this->bTargetsProcessed && !this->bForcesApplied) {
            this->ApplyForces();
        }
        if (this->bTargetsFound && !this->bDecalsPlaced) {
            NiPoint3 impactEffectVector;
            const char* pModel;
            BGSImpactData* pimpactData;
            alignas(16) NiPoint3 contactNormal;
            NiPoint3 contactPoint;
            NiAVObject* pav;
            int32_t j;
            this->bDecalsPlaced = true;
            hkLocalArray<hkpRootCdPoint> cdPointA(iNumExplosionDecalCDPoint.Int());
            alignas(16) hkpFixedBufferCdPointCollector fixedCollector(cdPointA.m_data, iNumExplosionDecalCDPoint.Int());
            hkpFixedBufferCdPointCollector::registerDefaultAddCdPointFunction();
            this->spPhantom->GetFixedClosestPoints(fixedCollector);
            if (fixedCollector.hasHit()) {
                hkAlgorithm::quickSort(cdPointA.m_data, fixedCollector.getNumHits());
            }
            if (iExplosionDebug.Int() > 1) {
                for (int32_t i = 0; i < fixedCollector.getNumHits(); ++i) {
                    NiPoint3 normal;
                    const hkpRootCdPoint* pHits = fixedCollector.getHits();
                    hkpRootCdPoint point(pHits[i]);
                    HK2NI(location, point.m_contact.getPosition());
                    HK2NINS(normal, point.m_contact.getNormal());
                    NiColorA red(1.0f, 0.0f, 0.0f, 0.65f);
                    NiAVObject* pdebug = MakeTriPoint(1.5f, red, true);
                    pdebug->UpdateProperties();
                    pdebug->SetTranslate(location);
                    TES::pInstance->AddTempDebugObject(pdebug, 10.0f);
                    pdebug = MakeLine(location, red, location + normal * 10.0f, red, true);
                    TES::pInstance->AddTempDebugObject(pdebug, 10.0f);
                }
            }
            char bClosestPointFound = 0;
            for (j = 0;; ++j) {
                const hkpRootCdPoint* pHits;
                if (bClosestPointFound || j >= fixedCollector.getNumHits()) {
                    if (this->bTargetsFound && !this->bTargetsProcessed) {
                        this->ProcessTargets();
                    }
                    return;
                }
                pHits = fixedCollector.getHits();
                hkpRootCdPoint cdPoint(pHits[j]);
                if (!cdPoint.m_rootCollidableB) {
                    continue;
                }
                pav = GetAVObjectForCollidable(cdPoint.m_rootCollidableB);
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                if (pref && pref->IsActor()) {
                    continue;
                }
                HK2NI(contactPoint, cdPoint.m_contact.getPosition());
                HK2NINS(contactNormal, cdPoint.m_contact.getNormal());
                hkpAllRayHitCollector pickCollector;
                bhkPickData pd;
                NiPoint3 origin = this->GetLocationOnReference();
                NiPoint3 pickLength = 2.0f * (contactPoint - origin);
                if (pickLength.Length() < 1.0) {
                    origin -= contactNormal * 16.0f;
                    pickLength = contactNormal * 32.0f;
                }
                pd.NiSetFrom(origin);
                pd.NiSetLength(pickLength);
                pd.SetFilter(*this->spPhantom->GetCollisionFilter(&filter));
                pd.SetAllCollector(&pickCollector);
                HavokMaterialType emat = BHK_MATERIAL_MAX;
                char bPickHit = 0;
                if (this->GetParentCell()->GetbhkWorld()) {
                    bhkWorld* pWorld = this->GetParentCell()->GetbhkWorld();
                    if (pWorld->PickObject(pd)) {
                        char bBlocked = 0;
                        for (int32_t k = 0; !bPickHit && !bBlocked && k < pd.pAllCollector->getHits().m_size; ++k) {
                            hkpWorldRayCastOutput pickPoint(pd.pAllCollector->getHits()[k]);
                            NiAVObject* ppickAV = GetAVObjectForCollidable(pickPoint.m_rootCollidable);
                            TESObjectREFR* ppickRef = TESObjectREFR::FindReferenceFor3D(ppickAV);
                            if (pav == ppickAV) {
                                bPickHit = 1;
                                uint32_t key = -1;
                                for (int32_t iKeyIndex = 0; iKeyIndex < 8 && pickPoint.m_shapeKeys[iKeyIndex] != -1;
                                     ++iKeyIndex) {
                                    key = pickPoint.m_shapeKeys[iKeyIndex];
                                }
                                uint32_t uiFilterInfo = pickPoint.m_rootCollidable->getCollisionFilterInfo();
                                uint32_t uiGroup = CFilter(uiFilterInfo).GetGroup();
                                if (uiGroup == 1) {
                                    emat = TES::pInstance->GetLandMaterialType(contactPoint);
                                }
                                else {
                                    bhkShape* pShape = bhkShape::Getbhk(pickPoint.m_rootCollidable->m_shape);
                                    if (pShape) {
                                        emat = pShape->GetSubshapeMaterial(key);
                                    }
                                }
                            }
                            else if (!ppickRef || !ppickRef->IsActor()) {
                                bBlocked = 1;
                            }
                        }
                    }
                }
                if (!bPickHit) {
                    continue;
                }
                this->ClosestPoint = contactPoint;
                if (this->ClosestPointNormal == NiPoint3::ZERO) {
                    this->ClosestPointNormal = contactNormal;
                }
                if (iExplosionDebug.Int() > 1) {
                    NiColorA green(0.0f, 1.0f, 0.0f, 0.65f);
                    NiAVObject* pdebug = MakeTriPoint(5.0f, green, true);
                    pdebug->UpdateProperties();
                    pdebug->SetTranslate(this->ClosestPoint);
                    TES::pInstance->AddTempDebugObject(pdebug, 10.0f);
                    pdebug = MakeLine(
                        this->ClosestPoint,
                        green,
                        this->ClosestPoint + this->ClosestPointNormal * 20.0f,
                        green,
                        true
                    );
                    TES::pInstance->AddTempDebugObject(pdebug, 10.0f);
                }
                bClosestPointFound = 1;
                pimpactData = this->GetExplosionBase()->GetImpactData(emat);
                if (!pimpactData) {
                    continue;
                }
                char bVisible = 1;
                NiPoint3 distance = contactPoint - Main::WorldRootCamera()->GetWorldTranslate();
                if (Main::GetSceneIsComplex()) {
                    bVisible = 0;
                }
                else {
                    double dCameraDistance = distance.Length();
                    if (fGunParticleCameraDistance.Float() < dCameraDistance ||
                        !PointInFrustum(contactPoint, Main::WorldRootCamera(), 32.0f)) {
                        bVisible = 0;
                    }
                }
                BGSTextureSet* pTextureSet = pimpactData->pDecalTextureSet;
                if (pTextureSet) {
                    float fMaxWidth = pimpactData->GetDecalMaxWidth();
                    float fMinWidth = pimpactData->GetMinWidth();
                    float fDecalSize = RandomFloat(fMinWidth, fMaxWidth);
                    DecalCaster* pcaster = DecalCaster::GetInstance(fDecalSize, true);
                    pcaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
                    pcaster->SetCollision(L_BIPED, false);
                    pcaster->SetCollision(L_DEADBIP, false);
                    TESObjectCELL* pcell = this->GetParentCell();
                    bhkWorld* pWorld = pcell ? pcell->GetbhkWorld() : nullptr;
                    if (pWorld) {
                        pcaster->Add(pWorld);
                        bhkFilteredCdBodyCollector collector;
                        pcaster->GetPenetrations(contactPoint, collector);
                        int32_t iNumHits = collector.m_hits.m_size;
                        float fRotation = RandomFloatTwoPi();
                        float fRandomSubTex = RandomFloat(0.0f, 4.0f);
                        char cSubTex = static_cast<int32_t>(std::floor(fRandomSubTex));
                        for (int32_t m = 0; m < iNumHits; ++m) {
                            const hkpCollidable* pCollidable = collector.m_hits[m].m_rootCollidableB;
                            if (pCollidable) {
                                HavokMaterialType eMaterial = emat;
                                uint32_t uiShapeKey = collector.m_hits[m].m_shapeKeyB;
                                if (uiShapeKey == -1) {
                                    bhkShape* pShape = bhkShape::Getbhk(pCollidable->m_shape);
                                    if (!pShape) {
                                        continue;
                                    }
                                    eMaterial = pShape->GetMaterial();
                                }
                                else {
                                    bhkShape* pShape = bhkShape::Getbhk(pCollidable->m_shape);
                                    if (!pShape) {
                                        continue;
                                    }
                                    bhkShapeCollection* pCollection = pShape->FindShapeCollection();
                                    if (pCollection) {
                                        eMaterial = pCollection->QSubMaterial(uiShapeKey);
                                    }
                                }
                                if (eMaterial == emat) {
                                    uint32_t uiFilterInfo = pCollidable->getCollisionFilterInfo();
                                    uint32_t uiGroup = CFilter(uiFilterInfo).GetGroup();
                                    bool bLand = uiGroup == 1;
                                    NiAVObject* pAVObject = nullptr;
                                    if (uiGroup == 1) {
                                        hkpRigidBody* pRigidBody = hkpGetRigidBody(pCollidable);
                                        if (pRigidBody) {
                                            // The property is keyed by NiTriStrips' RTTI address; its value is the
                                            // scene object's pointer.
                                            hkpPropertyValue* pProperty = bhkWorldObject::GetProperty(
                                                &property,
                                                pRigidBody,
                                                bhkPropertyKey(NiTriStrips::ms_RTTI)
                                            );
                                            pAVObject = static_cast<NiAVObject*>(pProperty->getPtr());
                                            if (pAVObject && pAVObject->m_pkParent) {
                                                pAVObject = pAVObject->m_pkParent;
                                            }
                                        }
                                    }
                                    else {
                                        pAVObject = bhkUtilFunctions::GetNiAVObject(pCollidable);
                                    }
                                    TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pAVObject);
                                    if (bLand ||
                                        (pref &&
                                         BGSDecalManager::IsValidDecalImpact(
                                             pref,
                                             BGSDecalManager::DECAL_TYPE_SIMPLE
                                         ))) {
                                        TESObjectCELL* pDecalCell = TES::pInstance->GetCellForPoint(contactPoint);
                                        alignas(16) DECAL_CREATION_DATA data;
                                        data.pOrigin = this->ClosestPoint;
                                        data.pDirection = this->ClosestPointNormal;
                                        data.fDecalWidth = fDecalSize;
                                        data.fDecalHeight = fDecalSize;
                                        data.fDecalDepth = pimpactData->GetDecalDepth();
                                        data.pAVObj = pAVObject;
                                        data.pTexSet = pTextureSet;
                                        data.fRotation = fRotation;
                                        data.cSubTex = cSubTex;
                                        data.bAlphaBlending = pimpactData->GetIsAlphaBlend();
                                        data.bAlphaTesting = pimpactData->GetIsAlphaTest();
                                        data.bUseParallaxOcclusion = pimpactData->GetIsParallax();
                                        data.fParallaxOcclusionScale = pimpactData->GetDecalParallaxScale();
                                        data.ucParallaxOcclusionMaxPasses = pimpactData->GetParallaxPasses();
                                        data.fSpecular = pimpactData->GetDecalShininess();
                                        data.fDecalEpsilon = pimpactData->Data.fAngleThreshold;
                                        int32_t iBlue = static_cast<uint8_t>(pimpactData->GetDecalColor() >> 16);
                                        float fBlue = static_cast<double>(iBlue) / 255.0;
                                        int32_t iGreen = static_cast<uint8_t>(
                                            static_cast<uint16_t>(pimpactData->GetDecalColor()) >> 8
                                        );
                                        float fGreen = static_cast<double>(iGreen) / 255.0;
                                        int32_t iRed = static_cast<uint8_t>(pimpactData->GetDecalColor());
                                        float fRed = static_cast<double>(iRed) / 255.0;
                                        data.fVertexColor = NiColor(fRed, fGreen, fBlue);
                                        pDecalCell->AddDecal(data, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
                                    }
                                }
                            }
                        }
                        pcaster = nullptr;
                    }
                }
                if (bVisible && !this->bUnderWater && (pModel = pimpactData->GetModel()) != nullptr && *pModel) {
                    BGSImpactData::ORIENTATION eOrient = pimpactData->Data.eOrient;
                    if (eOrient == BGSImpactData::ORIENT_PROJ_VECTOR) {
                        impactEffectVector = this->GetLocationOnReference() - contactPoint;
                        impactEffectVector.Unitize();
                    }
                    else if (eOrient == BGSImpactData::ORIENT_PROJ_REFLECT) {
                        alignas(16) NiPoint3 projectileVec = this->GetLocationOnReference() - contactPoint;
                        projectileVec.Unitize();
                        NiPoint3 normalVec = contactNormal;
                        normalVec.Unitize();
                        float fProjection = normalVec.Dot(projectileVec) * 2.0;
                        impactEffectVector = fProjection * normalVec - projectileVec;
                    }
                    else {
                        impactEffectVector = contactNormal;
                        impactEffectVector.Unitize();
                    }
                    NiPoint3 effectPosition = contactPoint;
                    NiPoint3 effectVector = impactEffectVector;
                    float fDuration = pimpactData->Data.fEffectDuration;
                    BSTempEffectParticle::Spawn(
                        this->GetParentCell(),
                        fDuration,
                        pModel,
                        effectVector,
                        effectPosition,
                        1.0f,
                        7u,
                        nullptr
                    );
                }
                TESSound* pimpactSound1 = pimpactData->pSound1;
                if (pimpactSound1) {
                    const char* pFilename = pimpactSound1->GetFilename();
                    BSSoundHandle soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                        pFilename,
                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                            BSGameSound::SOUND_FLAG_IMPACT,
                        pimpactSound1
                    );
                    soundHandle.SetPosition(contactPoint.x, contactPoint.y, contactPoint.z);
                    soundHandle.Play(false);
                }
                TESSound* pimpactSound2 = pimpactData->pSound2;
                if (pimpactSound2) {
                    const char* pFilename = pimpactSound2->GetFilename();
                    BSSoundHandle soundHandle = BSAudio::QInstance().GetSoundHandleByFilename(
                        pFilename,
                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                            BSGameSound::SOUND_FLAG_IMPACT,
                        pimpactSound2
                    );
                    soundHandle.SetPosition(contactPoint.x, contactPoint.y, contactPoint.z);
                    soundHandle.Play(false);
                }
            }
        }
    }
}

void Explosion::UpdateNonRenderSafeMovement(float afDelta) {
    if (IsDistant() && Get3D()) {
        NiUpdateData updateData(fCellAppTime, true, false);
        Get3D()->Update(updateData);
    }
}

// The age is doubled and divided at double precision; the dimmer is rounded to float before it is clamped.
void Explosion::UpdateLight() {
    if (spLight) {
        float fDimmer = 1.0 - GetAge() * 2.0 / GetLifetime();
        float fClamped = BSMAX(0.0f, BSMIN(1.0f, fDimmer));
        spLight->SetDimmer(fClamped);
    }
}

float Explosion::GetAge() {
    return fAge;
}

float Explosion::GetLifetime() {
    return fLifetime;
}

float Explosion::GetRadius() {
    return fRadius;
}

bool Explosion::IsDistant() {
    return TESObjectREFR::GetParentCell() == nullptr;
}

TESObjectREFR* Explosion::GetOwner() {
    return pOwner;
}

void Explosion::StopSounds() {
    if (Sound1.IsValid()) {
        Sound1.Stop();
        if (Sound1.IsValid()) {
            Sound1.Release();
        }
    }
    if (Sound2.IsValid()) {
        Sound2.Stop();
        if (Sound2.IsValid()) {
            Sound2.Release();
        }
    }
}

void Explosion::SetOuterRadius(float afRadius) {
    fRadius = afRadius;
}

BGSExplosion* Explosion::GetExplosionBase() {
    return static_cast<BGSExplosion*>(GetObjectReference());
}

// The first listed target with the reference takes the weak-point and mobile flags it lacks and is returned; else a new
// target is built, filled in and put at the head of the list.
Explosion::ExplosionTarget* Explosion::AddTarget(TESObjectREFR* apTargetRef, bool abWeakPoint, bool abMobile) {
    for (BSSimpleList<ExplosionTarget*>* pList = &TargetList; pList && !pList->IsEmpty(); pList = pList->GetNext()) {
        ExplosionTarget* pTarget = pList->GetItem();
        if (pTarget && pTarget->pRef == apTargetRef) {
            if (abWeakPoint && !pTarget->bHitWeakPoint) {
                pTarget->bHitWeakPoint = abWeakPoint;
            }

            if (abMobile && !pTarget->bMobile) {
                pTarget->bMobile = abMobile;
            }

            return pTarget;
        }
    }
    ExplosionTarget* pTarget = new ExplosionTarget;
    pTarget->pRef = apTargetRef;
    pTarget->bHitWeakPoint = abWeakPoint;
    pTarget->bMobile = abMobile;
    TargetList.AddHead(pTarget);
    return pTarget;
}

void Explosion::SetWeaponSource(TESObjectWEAP* apSource) {
    pWeaponSource = apSource;
}

void Explosion::SetLifetime(float afValue) {
    fLifetime = afValue;
}

void Explosion::ProcessMovement(float afDelta) {
    float fSeconds = appTimer.GetSeconds();
    Update(fSeconds);
    fAge = fAge + fSeconds;
}

void Explosion::Kill() {
    SetDelete(true);
}

BSSimpleList<Explosion::ExplodedLimb*>* Explosion::GetAffectedLimbList(TESObjectREFR* apTarget) {
    BSSimpleList<Explosion::ExplodedLimb*>* pResult = nullptr;
    for (BSSimpleList<Explosion::ExplosionTarget*>* pTargets = &TargetList;
         !pResult && pTargets && !pTargets->IsEmpty();
         pTargets = pTargets->m_pkNext) {
        Explosion::ExplosionTarget* pTarget = pTargets->m_item;
        if (pTarget && pTarget->pRef == apTarget) {
            pResult = &pTarget->limbList;
        }
    }
    return pResult;
}

void Explosion::SetClosestPointNormal(NiPoint3 aNormal) {
    ClosestPointNormal = aNormal;
}

TESObjectWEAP* Explosion::GetSourceWeapon() {
    return pWeaponSource;
}

void Explosion::AttachDistant() {
    NiNode* p3D = Get3D();
    if (p3D) {
        NiNode* pLODRoot = TES::pInstance->pWorldSpace->GetTerrainManager()->GetLODRoot();
        if (pLODRoot) {
            pLODRoot->AttachChild(p3D, 1);
        }
    }
}

void Explosion::Init3D() {
    Load3D(false);
    NiNode* pav = this->Get3D();
    if (pav) {
        NiControllerSequence* pSeqArea;
        alignas(16) NiMatrix3 rot;
        float fScale = 1.0f;
        if (this->IsDistant()) {
            NiNode* pNode = pav->IsNode();
            BSFadeNode* pFade = pNode ? pNode->IsFadeNode() : nullptr;
            if (pFade) {
                pFade->SetRange(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            }
        }
        if (!iMolotovTest.Int()) {
            NiAVObject* pRadiusNode = BSUtilities::GetObjectByName(pav, "BlastRadiusNode");
            if (pRadiusNode) {
                double dRadius = this->fRadius;
                fScale = dRadius / pRadiusNode->GetTranslate().Length();
            }
            else {
                const char* pName;
                if (this->GetExplosionBase()->GetFormEditorIDLength()) {
                    pName = this->GetExplosionBase()->GetFormEditorID();
                }
                else {
                    pName = this->GetExplosionBase()->GetFullName();
                }
                Warning(
                    "COMBAT: Explosion does not have a BlastRadiusNode: %08X %s ",
                    this->GetExplosionBase()->iFormID,
                    pName
                );
                fScale = 1.0f;
            }
        }
        pav->SetScale(fScale);
        pav->SetTranslate(this->GetLocationOnReference());
        NiPoint3 angle = this->GetAngleOnReference();
        rot.FromEulerAnglesZYX(angle.z, angle.y, angle.x);
        pav->SetRotate(rot);
        if (iMolotovTest.Int() > 1) {
            this->GetExplosionBase()->SetForce(fMolotovTestForce);
            this->fLifetime = fMolotovTestLifetime;
            BSXFlags* pbsx = static_cast<BSXFlags*>(pav->GetExtraData(BSXFlags::Tag()));
            if (pbsx) {
                pbsx->SetFlags(BSXFlags::ADDON, true);
            }
            for (int32_t i = 0; i < iMolotovTest.Int(); ++i) {
                BSCloningProcess process(1.0f);
                NiNode* pthing = new NiNode(0);
                float fZ = BSRandom::GetSingleton()->FloatNeg1To1() * 10.0 + 32.0;
                float fY = BSRandom::GetSingleton()->FloatNeg1To1() * 10.0;
                float fX = BSRandom::GetSingleton()->FloatNeg1To1() * 10.0;
                pthing->SetTranslate(fX, fY, fZ);
                bhkCollisionObject* pobj = new bhkCollisionObject(pthing);
                bhkSphereShape* pshape = new bhkSphereShape(8.0f, true);
                bhkRigidBodyCinfo cinfo;
                cinfo.SetMass(5.0f);
                cinfo.SetRestitution(0.0f);
                cinfo.SetMotionType(hkpMotion::MOTION_DYNAMIC);
                cinfo.SetFriction(1.0f);
                uint32_t uiSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
                cinfo.SetCollisionFilter(CFilter(L_CLUTTER, uiSystemGroup, P_OTHER).iFilter);
                cinfo.SetShape(pshape);
                bhkRigidBody* pbody = new bhkRigidBody(cinfo);
                pobj->SetWorldObject(pbody);
                pav->AttachChild(pthing, 1);
                BSValueNode* paddon = new BSValueNode;
                paddon->iValue = 14;
                pthing->AttachChild(paddon, 1);
                hkLiquidActionCinfo info;
                info.m_body = **pbody;
                info.fInitialStickForce = fMolotovTestInitialStickForce;
                info.fStickStrength = fMolotovTestStickStrength;
                info.fNeighborDistance = fMolotovTestNeighborDistance;
                info.fNeighborStrength = fMolotovTestNeighborStrength;
                bhkLiquidAction* pliquid = new bhkLiquidAction(info);
                pbody->AddAction(pliquid);
            }
            if (this->GetParentCell() && this->GetParentCell()->GetbhkWorld()) {
                this->GetParentCell()->GetbhkWorld();
                bhkWorld::UpdatePosition(pav, true, true);
                NiUpdateData update(0.0f, false, false);
                pav->Update(update);
                this->GetParentCell()->GetbhkWorld()->AddObjects(pav, true, true, 0, true);
            }
        }
        if (this->GetParentCell() && this->GetParentCell()->GetbhkWorld()) {
            this->GetParentCell()->GetbhkWorld();
            bhkWorld::UpdatePosition(pav, true, true);
            NiUpdateData update(0.0f, false, false);
            pav->Update(update);
            this->GetParentCell()->GetbhkWorld()->AddObjects(pav, true, true, 0, true);
        }

        if (pav->IsNode()) {
            if (TESObjectREFR::HasAddonFlags(pav->IsNode())) {
                TESObjectREFR::AddMasterParticleAddonNodes(pav->IsNode());
            }
        }
        if (this->GetParentCell() && this->GetParentCell()->GetbhkWorld()) {
            if (this->GetParentCell()->GetbhkWorld()->QDebugDisplay()) {
                this->GetParentCell()->GetbhkWorld()->SetDebugDisplay(pav, true);
                PrepareObjectWithNoLightingRecurse(pav);
            }
        }

        NiTimeController* po = pav->GetControllers();
        if (!po) {
            NiNode* pn = pav->IsNode();
            if (pn && pn->GetAtChecked(0)) {
                po = pn->GetAt(0)->GetControllers();
            }
        }
        NiControllerManager* pManager = nullptr;
        while (po) {
            if (NiObject::IsKindOf_0(&NiControllerManager::ms_RTTI, po)) {
                pManager = static_cast<NiControllerManager*>(po);
                break;
            }
            po = po->GetNext();
        }
        if (pManager && (pSeqArea = pManager->GetSequenceByName(FixedStrings::QSpecialIdle_AreaEffect())) != nullptr) {
            pManager->DeactivateAll(0.0f);
            pManager->ActivateSequence(pSeqArea, 0, false, 1.0f, 0.0f, nullptr);
            pManager->SetActive(true);
            pSeqArea->ResetSequence();
            double dLifetime = this->fLifetime;
            if (pSeqArea->m_fEndKeyTime > dLifetime) {
                SetLifetime(pSeqArea->m_fEndKeyTime);
            }
        }
        else {
            NiTimeController::StartAnimations(pav);
        }
    }
}

void Explosion::ApplyForces() {
    using Detail = hkpSimpleShapePhantom::CollisionDetail;
    CFilter filter;
    if (spPhantom.m_pObject) {
        bool bOnlyAffectExploded = GetExplosionBase()->GetFlag(BGSExplosion::FLAG_PUSH_EXPLOSION_SOURCE_REF_ONLY);
        for (int32_t i = 0; i < spPhantom->GetCollisionDetails().m_size; ++i) {
            COL_LAYER eLayer;
            hkArray<Detail, hkContainerHeapAllocator>* pDetails = &spPhantom->GetCollisionDetails();
            hkpRigidBody* phk = hkpGetRigidBody((*pDetails)[i].m_collidable);
            bhkRigidBody* prb = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(phk));
            if (prb) {
                eLayer = prb->GetCollisionFilter(&filter)->GetLayer();
            }
            else {
                eLayer = L_UNIDENTIFIED;
            }
            if (bOnlyAffectExploded || eLayer == L_DEADBIP) {
                bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(phk);
                NiAVObject* pav = pcol ? pcol->m_pkSceneObject : nullptr;
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                if (bOnlyAffectExploded && pref != pExplodedRef) {
                    continue;
                }
                if (pref) {
                    Actor* pactor = pref->IsActor() ? static_cast<Actor*>(pref) : nullptr;
                    // Actor::GetCriticalStage is a trivial accessor.
                    if (pactor && pactor->eCriticalStage == ACS_GOO_START) {
                        continue;
                    }
                }
            }
            if (prb && (prb->GetMotionType() < 4 || eLayer == L_DEBRIS_LARGE)) {
                PushRigidBody(prb);
            }
        }
    }
    bForcesApplied = true;
}

void Explosion::ProcessTargets() {
    NiPoint2 kScaledDuration;
    NiPoint2 kScaledIntensity;
    bTargetsProcessed = true;
    BSSimpleList<Explosion::ExplosionTarget*>* plist = &TargetList;
    while (plist && !plist->IsEmpty()) {
        Explosion::ExplosionTarget* ptarget = plist->m_item;
        plist = plist->m_pkNext;
        if (ptarget &&
            ptarget->pRef &&
            !ptarget->bProcessed &&
            !ptarget->pRef->GetDestroyed() &&
            !ptarget->pRef->GetDisabled()) {
            const NiPoint3& kLocation = GetLocationOnReference();
            const NiPoint3& kTargetLocation = ptarget->pRef->GetLocationOnReference();
            float fdistance = (kTargetLocation - kLocation).Length();
            float fpower = CombatFormulas::CalcExplosionPower(fRadius, fdistance);
            float fISRadius = fImageSpaceRadius;
            float fISPower;
            if (fISRadius > 0.0) {
                fISPower = CombatFormulas::CalcExplosionPower(fISRadius, fdistance);
            }
            else {
                fISPower = 1.0f;
            }
            if (fISPower > 0.0 && ptarget->pRef->IsActor()) {
                PlayerCharacter* pTargetPlayer = static_cast<PlayerCharacter*>(ptarget->pRef);
                if (pTargetPlayer == pPlayer) {
                    BGSExplosion* pBase = GetExplosionBase();
                    TESImageSpaceModifier* pModifier = pBase->pFormImageSpaceModifying;
                    if (pModifier) {
                        ImageSpaceModifierInstanceForm::Trigger(pModifier, fISPower, nullptr);
                    }
                }
            }
            if (fpower > 0.0) {
                if (ptarget->bMobile) {
                    ActorCause* pCause = GetActorCause();
                    ptarget->pRef->SetActorCause(pCause);
                }
                Actor* pActor = ptarget->pRef->IsActor() ? static_cast<Actor*>(ptarget->pRef) : nullptr;
                if (pActor) {
                    pActor->ClearLastHitData();
                }
                Actor* pCauseActor = GetActorCause() ? GetActorCause()->pActor : nullptr;
                Script::SetActionFlag(pCauseActor, &ptarget->pRef->m_Extra, SCRIPT_ACTION_ONHIT);
                if (GetSourceWeapon()) {
                    ExtraDataList* pExtra = &ptarget->pRef->m_Extra;
                    TESObjectWEAP* pWeapon = GetSourceWeapon();
                    Script::SetActionFlag(pWeapon, pExtra, SCRIPT_ACTION_ONHITWITH);
                }
                else {
                    Script::SetActionFlag(GetExplosionBase(), &ptarget->pRef->m_Extra, SCRIPT_ACTION_ONHITWITH);
                }
                double dDamage = GetDamage();
                float fdamage = dDamage * fpower;
                if (ptarget->pRef->GetDestroyed()) {
                    PlayerCharacter* pOwnerPlayer = static_cast<PlayerCharacter*>(GetOwner());
                    if (pOwnerPlayer == pPlayer) {
                        pPlayer->AddPlayerAction(PLAYER_ACTION_DESTROY_OBJECT, 8.0f, nullptr);
                    }
                }
                if (pActor) {
                    if (RunLOSPick(GetLocationOnReference(), pActor)) {
                        HitData data;
                        if (pOwner && pOwner->IsActor()) {
                            HitData::InitializeExplosionData(&data, static_cast<Actor*>(pOwner), pActor, this, fdamage);
                        }
                        else {
                            HitData::InitializeExplosionData(&data, nullptr, pActor, this, fdamage);
                        }
                        pActor->HitMe(&data, 0);
                        if (pActor == pPlayer) {
                            if (fExplosionShudderPowerMin.Float() < static_cast<double>(fpower)) {
                                float fDurationY = fExplosionShudderDurationY.Float();
                                NiPoint2 kDuration(fExplosionShudderDurationX.Float(), fDurationY);
                                float fIntensityY = fExplosionShudderIntensityY.Float();
                                NiPoint2 kIntensity(fExplosionShudderIntensityX.Float(), fIntensityY);
                                float fFrequencyY = fExplosionShudderFrequencyY.Float();
                                NiPoint2 kFrequency(fExplosionShudderFrequencyX.Float(), fFrequencyY);
                                NiPoint2* pIntensity = NiPoint2::Scale_(&kScaledIntensity, fpower, &kIntensity);
                                NiPoint2* pDuration = NiPoint2::Scale_(&kScaledDuration, fpower, &kDuration);
                                Interface::GetHUDEffects()->TriggerShudder(*pDuration, *pIntensity, kFrequency);
                            }
                        }
                        EnchantmentItem* pEnchantment = GetExplosionBase()->pFormEnchanting;
                        if (pEnchantment) {
                            if ((GetOwner() && GetOwner()->IsActor()) || pCaster) {
                                if (pCaster) {
                                    MagicTarget* pMagicTarget = static_cast<MagicTarget*>(pActor);
                                    MagicItem* pSpell = static_cast<MagicItem*>(pEnchantment);
                                    pCaster->CastSpellImmediate(pSpell, false, pMagicTarget, 1.0f, false);
                                }
                                else {
                                    MagicTarget* pMagicTarget = static_cast<MagicTarget*>(pActor);
                                    MagicItem* pSpell = static_cast<MagicItem*>(pEnchantment);
                                    // The owner is an actor here (tested above); the call goes through its
                                    // MagicCaster with no null test.
                                    static_cast<Actor*>(GetOwner())
                                        ->CastSpellImmediate(pSpell, false, pMagicTarget, 1.0f, false);
                                }
                            }
                        }
                        if (!pActor->IsDead(false)) {
                            if (GetExplosionBase()->GetFlag(BGSExplosion::FLAG_KNOCK_DOWN_ALWAYS) ||
                                (GetExplosionBase()->GetFlag(BGSExplosion::FLAG_KNOCK_DOWN_BY_FORMULA) &&
                                 ActorValueOwner::ShouldDamageKnockdownTarget(
                                     pActor,
                                     static_cast<int32_t>(data.fHealthDamage)
                                 ))) {
                                PushActor(pActor, fpower);
                            }
                        }
                        if (pActor->IsDead(false)) {
                            NiAVObject* p3D;
                            if (pActor == pPlayer) {
                                p3D = pPlayer->Get3D(false);
                            }
                            else {
                                p3D = pActor->Get3D();
                            }
                            bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
                            NiUpdateData updateData(0.0f, false, false);
                            p3D->Update(updateData);
                        }
                    }
                }
                else {
                    ptarget->pRef->DamageObject(fdamage, false);
                }
            }
        }
    }
}

void Explosion::PushActor(Actor* apTargetActor, float afExplosionPower) {
    if (apTargetActor &&
        apTargetActor->pCurrentProcess &&
        fExplosionForceKnockdownMinimum.Float() < static_cast<double>(afExplosionPower)) {
        BaseProcess* pProcess = apTargetActor->pCurrentProcess;
        NiPoint3 location = GetLocationOnReference();
        // The magnitude is 0.
        pProcess->KnockExplosion(apTargetActor, location, 0.0f);
    }
}

void Explosion::StartSounds() {
    StopSounds();
    if (GetExplosionBase()->Data.pSound1) {
        int32_t iSoundID = GetExplosionBase()->Data.pSound1->iFormID;
        Sound1 = PlaySound(
            iSoundID,
            false,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_BATTLE_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT |
                BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
    if (GetExplosionBase()->Data.pSound2) {
        int32_t iSoundID = GetExplosionBase()->Data.pSound2->iFormID;
        Sound2 = PlaySound(
            iSoundID,
            false,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_BATTLE_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT |
                BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
}

void Explosion::AttachLight() {
    if (Get3D() && GetExplosionBase()->Data.pLight) {
        NiNode* p3D = Get3D();
        TESObjectLIGH* pLight = GetExplosionBase()->Data.pLight;
        spLight = static_cast<NiPointLight*>(pLight->GenDynamic(this, p3D, false));
    }
}

void Explosion::UpdateRadiation() {
    float fRadiation = GetExplosionBase()->Data.fRadiationRads;
    float fDissipationTime = GetExplosionBase()->GetDissipationTime();
    if (fRadiation > 0.0 && fDissipationTime > 0.0) {
        float fRads = (1.0 - fAge / fDissipationTime) * fRadiation;
        if (fRads <= 0.0) {
            fRads = 0.01f;
        }
        m_Extra.SetRadiation(fRads);
        AddChange(CHANGE_REFR_EXTRA_CREATED_ONLY);
    }
}

bool Explosion::RunLOSPick(NiPoint3 aLocation, TESObjectREFR* apTarget) {
    CFilter fltr;
    BGSExplosion* pExplode = GetExplosionBase();
    if (pExplode->GetFlag(BGSExplosion::FLAG_IGNORE_LOS_CHECK)) {
        return true;
    }
    NiNode* pTarget3D = apTarget ? apTarget->Get3D() : nullptr;
    if (apTarget == pPlayer) {
        pTarget3D = static_cast<NiNode*>(pPlayer->Get3D(false));
    }
    if (!pTarget3D || !pTarget3D->IsVisualObject()) {
        return false;
    }
    bhkWorld* pw = apTarget->GetParentCell() ? apTarget->GetParentCell()->GetbhkWorld() : nullptr;
    if (!pw) {
        return false;
    }
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, 0x26206u);
    NiPoint3 targetLocation[7];
    targetLocation[0] = pTarget3D->GetWorldBound().m_kCenter;
    char bInFurniture = 0;
    if (apTarget->IsActor() && apTarget->GetSitSleepState()) {
        bInFurniture = 1;
        targetLocation[1] = targetLocation[0];
        double dRadius = pTarget3D->GetWorldBound().m_fRadius;
        targetLocation[1].z = dRadius + dRadius + targetLocation[1].z;
        targetLocation[2] = targetLocation[0];
        dRadius = pTarget3D->GetWorldBound().m_fRadius;
        targetLocation[2].z = targetLocation[2].z - (dRadius + dRadius);
        targetLocation[3] = targetLocation[0];
        dRadius = pTarget3D->GetWorldBound().m_fRadius;
        targetLocation[3].y = dRadius + dRadius + targetLocation[3].y;
        targetLocation[4] = targetLocation[0];
        dRadius = pTarget3D->GetWorldBound().m_fRadius;
        targetLocation[4].y = targetLocation[4].y - (dRadius + dRadius);
        targetLocation[5] = targetLocation[0];
        dRadius = pTarget3D->GetWorldBound().m_fRadius;
        targetLocation[5].x = dRadius + dRadius + targetLocation[5].x;
        targetLocation[6] = targetLocation[0];
        dRadius = pTarget3D->GetWorldBound().m_fRadius;
        targetLocation[6].x = targetLocation[6].x - (dRadius + dRadius);
    }

    bhkPickData pd;
    fltr.iFilter = 0;
    fltr.SetLayer(L_CUSTOMPICK1);
    Actor* pactorTarget = apTarget->IsActor() ? static_cast<Actor*>(apTarget) : nullptr;
    if (pactorTarget && pactorTarget->GetCharController()) {
        fltr.SetGroup(pactorTarget->GetCharController()->GetFilterGroup());
    }
    else {
        fltr.SetGroup(bhkCollisionFilter::GetNewSystemGroup());
    }
    aLocation += this->ClosestPointNormal * fExplosionLOSBufferDistance.Float();
    pd.SetFilter(fltr);
    pd.NiSetFrom(aLocation);
    pd.NiSetTo(*targetLocation);
    hkpAllRayHitCollector collector;
    pd.SetAllCollector(&collector);
    char bIsBlocked = 0;
    pw->PickObject(pd);
    for (int32_t i = 0; !bIsBlocked && i < pd.pAllCollector->getHits().m_size; ++i) {
        hkpWorldRayCastOutput point(pd.pAllCollector->getHits()[i]);
        NiPoint3 length = *targetLocation - aLocation;
        NiPoint3 toHit = point.m_hitFraction * length;
        if (this->ClosestPointNormal[2] < -0.98) {
            double dHitDistance = toHit.Length();
            if (fExplosionLOSBuffer.Float() > dHitDistance) {
                continue;
            }
        }
        if (!this->GetOwner() || !this->GetOwner()->GetDestructible()) {
            if (iExplosionDebug.Int() > 0) {
                Warning("EXPLOSION BLOCKED");
            }
            bIsBlocked = 1;
        }
    }
    if (iExplosionDebug.Int() > 0) {
        NiColorA color = bIsBlocked ? NiColorA(1.0f, 0.0f, 0.0f, 1.0f) : NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
        NiAVObject* pdebug = MakeLine(aLocation, color, *targetLocation, color, true);
        TES::pInstance->AddTempDebugObject(pdebug, 25.0f);
    }
    if (bInFurniture && bIsBlocked) {
        for (int32_t i = 1; bIsBlocked && i < 7; ++i) {
            bIsBlocked = 0;
            hkpAllRayHitCollector furnitureCollector;
            pd.SetAllCollector(&furnitureCollector);
            pd.NiSetTo(targetLocation[i]);
            pw->PickObject(pd);
            for (int32_t j = 0; !bIsBlocked && j < pd.pAllCollector->getHits().m_size; ++j) {
                hkpWorldRayCastOutput point(pd.pAllCollector->getHits()[j]);
                NiPoint3 length = targetLocation[j] - aLocation;
                NiPoint3 toHit = point.m_hitFraction * length;
                if (this->ClosestPointNormal[2] < -0.98) {
                    double dHitDistance = toHit.Length();
                    if (fExplosionLOSBuffer.Float() > dHitDistance) {
                        continue;
                    }
                }
                bIsBlocked = 1;
            }
        }
    }

    return bIsBlocked == 0;
}

bool Explosion::TestExplosionUnderWater() {
    bool binWater = false;
    float fExplosionRadiusRatio = fExplosionWaterRadiusRatio.Float();
    TESObjectCELL* pcell = this->GetParentCell();
    if (!this->IsDistant() && pcell) {
        const NiPoint3& kLocation = this->GetLocationOnReference();
        double dExplosionTop = this->fRadius * fExplosionRadiusRatio + kLocation.z;
        if (pcell->GetWaterHeight() > dExplosionTop) {
            binWater = true;
        }
        else {
            bhkWorld* pworld = pcell->GetbhkWorld();
            hkpWorld* phkworld = pworld ? **pworld : nullptr;
            hkpBroadPhase* pbroadphase = phkworld ? phkworld->m_broadPhase : nullptr;
            if (pbroadphase) {
                NiPoint3 kExtent = 1.0f * NiPoint3::UNIT_ALL;
                alignas(16) NiPoint3 min = this->GetLocationOnReference() - kExtent;
                kExtent = 1.0f * NiPoint3::UNIT_ALL;
                NiPoint3 max = this->GetLocationOnReference() + kExtent;
                hkAabb aabb;
                NI2HK(aabb.m_min, min);
                NI2HK(aabb.m_max, max);
                using hits_t = hkInplaceArray<hkpBroadPhaseHandlePair, 128, hkContainerHeapAllocator>;
                alignas(16) hits_t hits(0);
                pbroadphase->querySingleAabb(aabb, hits);
                for (int32_t i = 0; !binWater && i < hits.m_size; ++i) {
                    const hkpCollidable* collB = static_cast<const hkpCollidable*>(
                        static_cast<hkpTypedBroadPhaseHandle*>(hits[i].m_b)->getOwner()
                    );
                    NiAVObject* pav = GetAVObjectForCollidable(collB);
                    TESObjectREFR* pwaterRef = TESObjectREFR::FindReferenceFor3D(pav);
                    if (pwaterRef && pwaterRef->GetObjectReference()->GetFormType() == PWAT_ID) {
                        const NiPoint3& kCurrentLocation = this->GetLocationOnReference();
                        double dRadius = this->fRadius;
                        dExplosionTop = dRadius * fExplosionRadiusRatio + kCurrentLocation.z;
                        if (pwaterRef->GetLocationOnReference().z > dExplosionTop) {
                            binWater = true;
                        }
                    }
                }
            }
        }
    }

    this->bUnderWater |= binWater;
    return binWater;
}

void Explosion::FindTargets() {
    using Detail = hkpSimpleShapePhantom::CollisionDetail;
    Explosion::ExplosionTarget* pplayerTargetEntry = AddTarget(pPlayer, false, true);
    if (spPhantom.m_pObject) {
        for (int32_t iIndex = 0; iIndex < spPhantom->GetCollisionDetails().m_size; ++iIndex) {
            hkArray<Detail, hkContainerHeapAllocator>* pDetails = &spPhantom->GetCollisionDetails();
            NiAVObject* pav = GetAVObjectForCollidable((*pDetails)[iIndex].m_collidable);
            Actor* pactor = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(pav));
            if (pactor && (!pactor->IsActor() || !pactor->GetGhost())) {
                bool bDynamic;
                pDetails = &spPhantom->GetCollisionDetails();
                hkpRigidBody* pbody = hkpGetRigidBody((*pDetails)[iIndex].m_collidable);
                if (pbody) {
                    bDynamic = pbody->getMotionType() < hkpMotion::MOTION_KEYFRAMED;
                }
                else {
                    bDynamic = false;
                }
                Explosion::ExplosionTarget* ptargetEntry = nullptr;
                if (pactor == pPlayer || pactor == pOwner) {
                    if (pactor == pPlayer) {
                        ptargetEntry = pplayerTargetEntry;
                    }
                }
                else {
                    ptargetEntry = AddTarget(pactor, false, bDynamic);
                }
                if (ptargetEntry && pactor->IsActor() && GetDamage() > 0.0) {
                    const NiPoint3& kLocation = GetLocationOnReference();
                    float fDistance = (pav->GetWorldTranslate() - kLocation).Length();
                    CheckLimbTarget(ptargetEntry, pav, fDistance);
                }
            }
        }
    }
    bTargetsFound = true;
}

void Explosion::PushRigidBody(bhkRigidBody* apRigidBody) {
    if (apRigidBody) {
        float fForce = this->GetExplosionBase()->Data.fForce;
        if (fForce > 0.0) {
            hkVector4 hkCenter;
            NiPoint3 rbLoc;
            HK2NI(rbLoc, apRigidBody->GetCenterOfMassInWorld(hkCenter));
            NiPoint3 direction = rbLoc - this->GetLocationOnReference();
            direction.Unitize();
            CFilter kFilter;
            if (apRigidBody->GetCollisionFilter(&kFilter)->QBipLayer()) {
                direction[2] = direction[2] + fExplosionForceClutterUpBias.Float();
            }
            uint8_t bSourceRef = 0;
            if (this->pExplodedRef) {
                bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(**apRigidBody);
                NiAVObject* pav = pcol ? pcol->m_pkSceneObject : nullptr;
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                if (pref && pref == this->pExplodedRef) {
                    CFilter kLayerFilter;
                    if (apRigidBody->GetCollisionFilter(&kLayerFilter)->GetLayer() == L_DEBRIS_LARGE) {
                        apRigidBody->SetMotionType(hkpMotion::MOTION_DYNAMIC);
                    }
                    fForce = fForce * fExplosionSourceRefMult.Float();
                    bSourceRef = 1;
                }
            }
            fForce = TESHavokUtilities::ScaleGameplayImpulseForce(fForce, apRigidBody, true);
            if (!bSourceRef) {
                fForce = BSMIN(fForce, fExplosionMaxImpulse.Float());
            }
            float fLinearMult = fExplosionForceMultLinear.Float();
            alignas(16) NiPoint3 kLinearForce = direction * fForce;
            apRigidBody->ApplyLinearImpulseNi(kLinearForce * fLinearMult);
            float fZ = RandomFloat(-1.0f, 1.0f);
            float fY = RandomFloat(-1.0f, 1.0f);
            float fX = RandomFloat(-1.0f, 1.0f);
            NiPoint3 kAngularForce(fX, fY, fZ);
            float fAngularMult = fExplosionForceMultAngular.Float();
            apRigidBody->ApplyAngularImpulse(kAngularForce * fForce * fAngularMult);
        }
    }
}

float Explosion::GetDamage() {
    if (fCalculatedDamage >= 0.0) {
        return fCalculatedDamage;
    }
    fCalculatedDamage = CombatFormulas::CalcExplosionDamage(GetExplosionBase());
    if (GetOwner()) {
        Actor* pSource = static_cast<Actor*>(GetOwner());
        if (pSource->IsActor() && GetSourceWeapon()) {
            Actor* pOwnerActor = static_cast<Actor*>(GetOwner());
            ActorValue::Index eWeaponSkill = GetSourceWeapon()->GetWeaponSkill();
            float fActorValue = pOwnerActor->GetActorFloatValue(eWeaponSkill);
            float fDamageMult = fActorValue * 0.009999999776482582 * fDamageSkillMult.Float() +
                fDamageSkillBase.Float();
            fCalculatedDamage = fCalculatedDamage * fDamageMult;
        }
    }
    return fCalculatedDamage;
}

void Explosion::DetachLight() {
    if (spLight.m_pObject) {
        NiLight* pLight = spLight.m_pObject;
        BSShaderManager::GetShadowSceneNode(0)->RemoveLight(pLight);
        spLight = static_cast<NiPointLight*>(nullptr);
    }
}

void Explosion::CheckLimbTarget(Explosion::ExplosionTarget* apTarget, NiAVObject* apTargetObject, float afDistance) {
    if (apTarget && apTarget->pRef && apTarget->pRef->IsActor()) {
        Actor* ptargetActor = static_cast<Actor*>(apTarget->pRef);
        int32_t icount = 0;
        Explosion::ExplodedLimb* pfarthestLimb = nullptr;
        BSSimpleList<Explosion::ExplodedLimb*>* plist = &apTarget->limbList;
        while (plist && !plist->IsEmpty()) {
            ++icount;
            Explosion::ExplodedLimb* plimb = plist->m_item;
            plist = plist->m_pkNext;
            if (!pfarthestLimb || plimb->fDistance < static_cast<double>(pfarthestLimb->fDistance)) {
                pfarthestLimb = plimb;
            }
        }
        if (icount < 2 || !pfarthestLimb || afDistance < static_cast<double>(pfarthestLimb->fDistance)) {
            BGSBodyPart::LIMB_ENUM elimb = ptargetActor->GetDamageLocation(apTargetObject->IsNode());
            if (elimb != BGSBodyPart::LIMB_NONE && elimb != BGSBodyPart::LIMB_WEAPON) {
                AddLimbTarget(apTarget, elimb, afDistance);
            }
        }
    }
}

void Explosion::AddLimbTarget(Explosion::ExplosionTarget* apTarget, BGSBodyPart::LIMB_ENUM aeLimb, float afDistance) {
    int32_t icount = 0;
    Explosion::ExplodedLimb* plimb = nullptr;
    BSSimpleList<Explosion::ExplodedLimb*>* plist = &apTarget->limbList;
    while (plist && !plist->IsEmpty() && (!plimb || plimb->eLimb != aeLimb)) {
        ++icount;
        Explosion::ExplodedLimb* pentry = plist->m_item;
        plist = plist->m_pkNext;
        if (!plimb || pentry->fDistance < static_cast<double>(plimb->fDistance) || plimb->eLimb == aeLimb) {
            plimb = pentry;
        }
    }
    if (plimb && icount >= 2) {
        plimb->eLimb = aeLimb;
        plimb->fDistance = afDistance;
    }
    else {
        plimb = static_cast<Explosion::ExplodedLimb*>(ourMalloc(sizeof(Explosion::ExplodedLimb)));
        plimb->eLimb = aeLimb;
        plimb->fDistance = afDistance;
        apTarget->limbList.AddHead(plimb);
    }
}

void Explosion::CheckRefDeleted(TESObjectREFR* apDeletedRef) {
    BSSimpleList<Explosion::ExplosionTarget*>* pnext;
    if (GetOwner() == apDeletedRef) {
        pOwner = nullptr;
    }
    if (pExplodedRef == apDeletedRef) {
        pExplodedRef = nullptr;
    }
    for (BSSimpleList<Explosion::ExplosionTarget*>* plist = &TargetList; plist && !plist->IsEmpty(); plist = pnext) {
        pnext = plist->m_pkNext;
        Explosion::ExplosionTarget* ptarget = plist->m_item;
        if (ptarget && ptarget->pRef == apDeletedRef) {
            ptarget->bProcessed = true;
            ptarget->pRef = nullptr;
        }
    }
}

// The player's Adjust Explosion Radius perks scale the radius, with the projectile's weapon, or the player's current
// weapon when the player caused the explosion directly; a projectile owner hands its shooter on as the owner. The
// explosion gets the owner's actor cause (a new one for an actor owner), is marked as having moved its exploded
// reference, and is initialised now or queued to the background threads.
Explosion* Explosion::SpawnExplosion(
    BGSExplosion* apExplosionBase,
    TESObjectREFR* apOwnerRef,
    TESObjectREFR* apExplodedRef,
    TESObjectCELL* apCell,
    NiPoint3 aOrigin,
    NiMatrix3 aRotation
) {
    Explosion* pexplosion = nullptr;
    float fRadiusAdjustment = 1.0f;
    if (apExplosionBase) {
        TESObjectREFR* prealOwner = apOwnerRef;
        if (apOwnerRef && apOwnerRef->IsProjectile()) {
            prealOwner = static_cast<Projectile*>(apOwnerRef)->GetShooter();
            if (prealOwner == pPlayer) {
                Projectile* pprojectile = static_cast<Projectile*>(apOwnerRef);
                TESObjectWEAP* pweapon = pprojectile ? pprojectile->GetWeaponSource() : nullptr;
                BGSEntryPoint::HandleEntryPoint(
                    BGSEntryPoint::ADJUST_EXPLOSION_RADIUS,
                    pPlayer,
                    pweapon,
                    &fRadiusAdjustment
                );
            }
        }
        else if (apOwnerRef && apOwnerRef == pPlayer) {
            ItemChange* pweaponChange = pPlayer->GetCurrentAIProcess()
                ? pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()
                : nullptr;
            TESBoundObject* pweapon = pweaponChange ? pweaponChange->pContainerObj : nullptr;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::ADJUST_EXPLOSION_RADIUS,
                pPlayer,
                pweapon,
                &fRadiusAdjustment
            );
        }
        pexplosion = new Explosion(apExplosionBase, prealOwner, apCell, aOrigin, aRotation);
        float fRadius = pexplosion->GetRadius() * fRadiusAdjustment;
        pexplosion->SetOuterRadius(fRadius);
    }
    if (pexplosion) {
        if (pexplosion->GetOwner()) {
            ActorCause* pcause = nullptr;
            if (pexplosion->GetOwner()->IsActor()) {
                pcause = new ActorCause(static_cast<Actor*>(pexplosion->GetOwner()));
            }
            else {
                pcause = pexplosion->GetOwner()->GetActorCause();
            }
            pexplosion->SetActorCause(pcause);
        }
        pexplosion->pExplodedRef = apExplodedRef;
        if (apExplodedRef && apExplodedRef->CanBeMoved()) {
            apExplodedRef->AddChange(CHANGE_REFR_HAVOK_MOVE);
        }
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueMobileObjectInitialize(pexplosion);
        }
        else {
            pexplosion->Initialize();
        }
    }
    return pexplosion;
}

// The comparator is a zeroed hkAlgorithm::less<hkpRootCdPoint>, passed as its byte (quickSortRootCdPoints's aiLess).
void hkAlgorithm::quickSort(hkpRootCdPoint* pArr, int32_t iSize) {
    quickSortRootCdPoints(pArr, iSize, 0);
}

void bhkRigidBody::ApplyLinearImpulse(const hkVector4& arImpulse) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->applyLinearImpulse(arImpulse);
        UnMarkWrite();
    }
}

void bhkRigidBody::ApplyAngularImpulse(const hkVector4& arImpulse) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->applyAngularImpulse(arImpulse);
        UnMarkWrite();
    }
}

void bhkRigidBody::ApplyAngularImpulse(const NiPoint3& arImpulse) {
    hkVector4 hkImpulse;
    this->ApplyAngularImpulse(NI2HK(hkImpulse, arImpulse));
}

hkArray<hkpSimpleShapePhantom::CollisionDetail, hkContainerHeapAllocator>&
bhkSimpleShapePhantom::GetCollisionDetails() {
    return static_cast<hkpSimpleShapePhantom*>(**this)->getCollisionDetails();
}

bhkTransform& bhkTransform::NI2HK(const NiTransform& arNiTran) {
    Rot.NI2HKNS(arNiTran.m_Rotate);
    Trans.NI2HK(arNiTran.m_Translate);
    return *this;
}

bhkVector4& bhkVector4::NI2HKNS(const NiPoint3& arSrc) {
    fX = arSrc.x;
    fY = arSrc.y;
    fZ = arSrc.z;
    fW = 0.0f;
    return *this;
}

void hkMemoryAllocator::freeHandlePairBuffer(void* p, int32_t numElem) {
    bufFree(p, sizeof(hkpBroadPhaseHandlePair) * numElem);
}

// The SDK's form: raw byte arithmetic on the handle's address, where the owner sits m_ownerOffset bytes away.
void* hkpTypedBroadPhaseHandle::getOwner() const {
    return const_cast<char*>(reinterpret_cast<const char*>(this) + m_ownerOffset);
}

hkpFixedBufferCdPointCollector::hkpFixedBufferCdPointCollector(hkpRootCdPoint* buffer, int32_t capacity)
  : m_pointsArrayBase{buffer}, m_nextFreePoint{buffer}, m_capacity{capacity}, m_numPoints{0} {}

hkpFixedBufferCdPointCollector::~hkpFixedBufferCdPointCollector() {}

void hkpFixedBufferCdPointCollector::addCdPoint(const hkpCdPoint& event) {
    g_FixedBufferCdPointCollectorAddCdPointCallbackFunc(event, this);
}

const hkpRootCdPoint* hkpFixedBufferCdPointCollector::getHits() const {
    return m_pointsArrayBase;
}

int32_t hkpFixedBufferCdPointCollector::getNumHits() const {
    return m_numPoints;
}

hkBool hkpFixedBufferCdPointCollector::hasHit() {
    return m_numPoints > 0;
}

void hkpFixedBufferCdPointCollector::operator delete(void* p) {
    if (p) {
        hkMemoryRouter::getInstance().m_heap->blockFree(p, sizeof(hkpFixedBufferCdPointCollector));
    }
}

// Each column goes through one NiPoint3.
bhkMatrix3& bhkMatrix3::NI2HKNS(const NiMatrix3& arSrc) {
    NiPoint3 kCol;
    arSrc.GetCol(0, kCol);
    Col0.NI2HKNS(kCol);
    arSrc.GetCol(1, kCol);
    Col1.NI2HKNS(kCol);
    arSrc.GetCol(2, kCol);
    Col2.NI2HKNS(kCol);
    return *this;
}

// hkAllocateStack<hkpRootCdPoint>: HK_NEXT_MULTIPLE_OF(128, n * sizeof(hkpRootCdPoint)), in int arithmetic as the
// SDK's.
void* hkAllocateStackRootCdPoints(int32_t n, char* what) {
    return hkMemoryRouter::getInstance().m_stack.allocateStack((112 * n + 127) & ~127, what);
}
