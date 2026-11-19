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

namespace hkAlgorithm {} // namespace hkAlgorithm

// The SDK's hkAlgorithm::quickSort for the root contact points.
// KEEP: src cannot express it exactly. The exe passes its comparator's byte on to quickSortRecursive (movzx; push at
// 0x9b2eac), whose parameter is the empty hkAlgorithm::less<hkpRootCdPoint>: clang writes no bytes of an empty class
// argument, so src's slot holds other bytes.
void quickSortRootCdPoints(hkpRootCdPoint* a1, int32_t a2, uint8_t aiLess) {
    if (a2 > 1) {
        hkAlgorithm::quickSortRecursive<hkpRootCdPoint, hkAlgorithm::less<hkpRootCdPoint>>(
            a1, 0, a2 - 1, hkAlgorithm::less<hkpRootCdPoint>()
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
    ClosestPoint(NiPoint3_ZERO), ClosestPointNormal(NiPoint3_ZERO), fCalculatedDamage(-1.0f) {
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
    ClosestPoint(NiPoint3_ZERO), ClosestPointNormal(NiPoint3_ZERO), fCalculatedDamage(-1.0f) {
    pCurrentProcess = nullptr;
    MobileObject::SetParentCell(apCell);
    SetLocationOnReference(aOrigin);
    SetOrientation(aRotation);
    if (apExplosionBase) {
        SetObjectReference(apExplosionBase);
        fRadius = GetExplosionBase()->GetRadiusBSUnits();
        fImageSpaceRadius = GetExplosionBase()->GetISRadius();
        bIgnoreImageSpaceSwap = GetExplosionBase()->GetFlag(0x40);
        if (GetExplosionBase()->GetFlag(2)) {
            SetOrientation(NiMatrix3_IDENTITY);
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
        spPhantom.m_pObject->Remove();
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
    int32_t iActorCauseID;
    uint8_t bHasCaster;
    MobileObject::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&fAge, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fLifetime, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fRadius, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fImageSpaceRadius, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bDecalsPlaced, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iFrameCount, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fBuildTime, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fWeaponCondition, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreImageSpaceSwap, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bUnderWater, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&ClosestPoint, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&ClosestPointNormal, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&fCalculatedDamage, 4u, 0);
    apSaveGameBuffer->SaveFormID(pOwner, 0);
    apSaveGameBuffer->SaveFormID(pExplodedRef, 0);
    apSaveGameBuffer->SaveFormID(pWeaponSource, 0);
    if (spActorCause.m_pObject) {
        iActorCauseID = spActorCause.m_pObject->iActorCauseID;
    }
    else {
        iActorCauseID = -1;
    }
    apSaveGameBuffer->SaveDataEndian(&iActorCauseID, 4u, 0);
    bHasCaster = pCaster != nullptr;
    apSaveGameBuffer->SaveDataEndian(&bHasCaster, 1u, 0);
    if (bHasCaster) {
        uint32_t MagicTargetFormID;
        uint32_t aiFormID;
        if (pCaster->pCurrentSpell) {
            aiFormID = pCaster->pCurrentSpell->GetMagicItemFormID();
        }
        else {
            aiFormID = 0;
        }
        apSaveGameBuffer->SaveFormID(aiFormID, 0);
        if (pCaster->pDesiredTarget) {
            MagicTargetFormID = pCaster->pDesiredTarget->GetMagicTargetFormID();
        }
        else {
            MagicTargetFormID = 0;
        }
        apSaveGameBuffer->SaveFormID(MagicTargetFormID, 0);
        apSaveGameBuffer->SaveFormID(pCaster->pCasterObject, 0);
    }
}

void Explosion::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t iactorCauseID;
    bool bhasCaster;
    MobileObject::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fAge, 4u);
    apLoadGameBuffer->LoadData(&fLifetime, 4u);
    apLoadGameBuffer->LoadData(&fRadius, 4u);
    apLoadGameBuffer->LoadData(&fImageSpaceRadius, 4u);
    apLoadGameBuffer->LoadData(&bDecalsPlaced, 1u);
    apLoadGameBuffer->LoadData(&iFrameCount, 4u);
    apLoadGameBuffer->LoadData(&fBuildTime, 4u);
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        apLoadGameBuffer->LoadData(&fWeaponCondition, 4u);
        apLoadGameBuffer->LoadData(&bIgnoreImageSpaceSwap, 1u);
        apLoadGameBuffer->LoadData(&bUnderWater, 1u);
        apLoadGameBuffer->LoadData(&ClosestPoint, 12u);
        apLoadGameBuffer->LoadData(&ClosestPointNormal, 12u);
    }
    if (apLoadGameBuffer->GetVersion() >= 20u) {
        apLoadGameBuffer->LoadData(&fCalculatedDamage, 4u);
    }
    apLoadGameBuffer->LoadFormID(pOwner);
    apLoadGameBuffer->LoadFormID(pExplodedRef);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pWeaponSource = dynamic_cast<TESObjectWEAP*>(FormByNumericID);
    iactorCauseID = -1;
    apLoadGameBuffer->LoadData(&iactorCauseID, 4u);
    ActorCause* ActorCauseByID = ActorCause::GetActorCauseByID(iactorCauseID);
    spActorCause = ActorCauseByID;
    bhasCaster = true;
    if (apLoadGameBuffer->GetVersion() >= 7u) {
        apLoadGameBuffer->LoadData(&bhasCaster, 1u);
    }
    if (bhasCaster) {
        if (!pCaster) {
            NonActorMagicCaster* v5;
            NonActorMagicCaster* apMemory = static_cast<NonActorMagicCaster*>(ourMalloc(sizeof(NonActorMagicCaster)));
            if (apMemory) {
                v5 = ::new (apMemory) NonActorMagicCaster(this);
            }
            else {
                v5 = nullptr;
            }
            pCaster = v5;
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
    TESObjectREFR* v8;
    TESObjectREFR* v9;
    MobileObject::InitLoadGame(apLoadGameBuffer);
    if (pOwner) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner)));
        v9 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v9 = nullptr;
    }
    pOwner = v9;
    if (pExplodedRef) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pExplodedRef)));
        v8 = dynamic_cast<TESObjectREFR*>(v3);
    }
    else {
        v8 = nullptr;
    }
    pExplodedRef = v8;
    if (pCaster) {
        TESObjectREFR* v5;
        MagicTarget* MagicTargetByNumericID;
        MagicItem* MagicItemByNumericID;
        if (pCaster->pCurrentSpell) {
            MagicItemByNumericID = MagicItem::GetMagicItemByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster->pCurrentSpell))
            );
        }
        else {
            MagicItemByNumericID = nullptr;
        }
        pCaster->pCurrentSpell = MagicItemByNumericID;
        if (pCaster->pDesiredTarget) {
            MagicTargetByNumericID = MagicTarget::GetMagicTargetByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster->pDesiredTarget))
            );
        }
        else {
            MagicTargetByNumericID = nullptr;
        }
        pCaster->pDesiredTarget = MagicTargetByNumericID;
        if (pCaster->pCasterObject) {
            TESForm* v4 =
                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster->pCasterObject)));
            v5 = dynamic_cast<TESObjectREFR*>(v4);
        }
        else {
            v5 = nullptr;
        }
        pCaster->pCasterObject = v5;
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
    ClosestPoint = NiPoint3_ZERO;
    ClosestPointNormal = NiPoint3_ZERO;
    while (!TargetList.IsEmpty()) {
        Explosion::ExplosionTarget* apThis = TargetList.m_item;
        if (apThis) {
            delete apThis;
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
    if ((spPhantom == static_cast<bhkSimpleShapePhantom*>(nullptr)) && (this->GetParentCell()) &&
        (this->GetParentCell()->GetbhkWorld())) {
        bhkSimpleShapePhantom* v6;
        bhkSphereShape* pshape;
        alignas(16) hkSimpleShapePhantomCinfo info;
        bhkSphereShape* v12 = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
        if (v12) {
            float afRadius = this->fRadius;
            pshape = ::new (v12) bhkSphereShape(afRadius, true);
        }
        else {
            pshape = nullptr;
        }
        info.m_shape = **pshape;
        uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        info.m_collisionFilterInfo = CFilter(L_SPELLEXPLOSION, NewSystemGroup, P_OTHER).iFilter;
        if (this->Get3D()) {
            NiNode* v2 = this->Get3D();
            info.m_transform.NI2HK(v2->GetWorldTransform());
        }
        else {
            info.m_transform.setIdentity();
        }
        bhkSimpleShapePhantom* v7 =
            static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
        if (v7) {
            v6 = ::new (v7) bhkSimpleShapePhantom(info);
        }
        else {
            v6 = nullptr;
        }
        this->spPhantom = v6;
        bhkSimpleShapePhantom* v5 = this->spPhantom.m_pObject;
        bhkWorld* v3 = this->GetParentCell()->GetbhkWorld();
        v5->Add(v3);
    }
}

bool Explosion::IsExplosion() {
    return true;
}

bool Explosion::ChangeProcessLevel() {
    bool bchange = true;
    PROCESS_TYPE eoldLevel = pCurrentProcess->eLevel;
    PROCESS_TYPE enewLevel = PROCESS_TYPE_LOW;
    TESObjectCELL* pcell = GetParentCell();
    if ((Get3D() || pModelLoader->IsReferenceQueued(this)) && TES::pInstance->IsCellLoaded(pcell, true)) {
        enewLevel = PROCESS_TYPE_HIGH;
    }
    if ((eoldLevel != enewLevel) && (enewLevel)) {
        if (this) {
            delete this;
        }
        return false;
    }

    return bchange;
}

bool Explosion::IsTransient() {
    return true;
}

void Explosion::Update(float afDelta) {
    if (!iFrameCount) {
        TESObjectREFR* pObjectHit = nullptr;
        if (!TargetList.IsEmpty()) {
            pObjectHit = TargetList.m_item->pRef;
        }
    }
    if (static_cast<double>(fBuildTime) <= fAge && !bTargetsFound) {
        FindTargets();
    }
    ++iFrameCount;
    if (!IsDistant()) {
        UpdateRadiation();
        UpdateLight();
    }
    double RustleSpeed = fAge;
    if (fLifetime < RustleSpeed) {
        double v2 = fAge;
        if (GetExplosionBase()->GetDissipationTime() < v2) {
            Kill();
        }
    }
}

void Explosion::Initialize() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TES_REFERENCES, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\Explosion.cpp",
        376
    );
    bool bLoadingSaveGame = pSaveLoadGame->GetSaveGameLoading();
    bool bCreateProcess = pCurrentProcess == nullptr;
    if (bCreateProcess) {
        HighProcess* pMemory = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
        HighProcess* pProcess;
        if (pMemory) {
            pProcess = ::new (pMemory) HighProcess;
        }
        else {
            pProcess = nullptr;
        }
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
            // The arguments are evaluated right to left, as the game does.
            TESWorldSpace* pWorld = GetWorldSpace();
            TESObjectCELL* pCell = GetParentCell();
            const NiPoint3& rAngle = GetAngleOnReference();
            const NiPoint3& rPosition = GetLocationOnReference();
            pDataHandler->CreateReferenceAtLocation(
                GetExplosionBase()->pImpactPlacedObject, rPosition, rAngle, pCell, pWorld, nullptr, nullptr, nullptr
            );
        }
        if (!bLoadingSaveGame && GetExplosionBase()->GetRADRadius() > 0.0 &&
            GetExplosionBase()->Data.fRadiationRads > 0.0) {
            UpdateRadiation();
            m_Extra.SetRadius(GetExplosionBase()->GetRADRadius());
            AddChange(0x40000000);
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
                pPlayer->GetParentCell(), 1.0f, pParticles, rotation, splashLocation, fScale, 7, nullptr
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
    hkpPropertyValue v64;
    NiPoint3 v90;
    alignas(16) CFilter v107;
    alignas(16) NiPoint3 v109;
    hkpRootCdPoint point_2;
    NiPoint3 v122;
    NiPoint3 location;
    int32_t v139;
    int32_t* v134 = &v139;
    if (spPhantom.m_pObject) {
        if (this->iFrameCount > 0 && this->bTargetsProcessed && !this->bForcesApplied) {
            this->ApplyForces();
        }
        if (this->bTargetsFound && !this->bDecalsPlaced) {
            NiPoint3* p_ClosestPoint;
            float* v19;
            NiCamera* v20;
            NiPoint3 impactEffectVector;
            const char* v53;
            double v86;
            BGSImpactData* pimpactData;
            alignas(16) NiPoint3 contactNormal;
            NiPoint3 contactPoint;
            NiAVObject* pav;
            int32_t j;
            this->bDecalsPlaced = true;
            hkLocalArray<hkpRootCdPoint> cdPointA(iNumExplosionDecalCDPoint.Int());
            alignas(16) hkpFixedBufferCdPointCollector fixedCollector(cdPointA.m_data, iNumExplosionDecalCDPoint.Int());
            hkpFixedBufferCdPointCollector::registerDefaultAddCdPointFunction();
            this->spPhantom.m_pObject->GetFixedClosestPoints(fixedCollector);
            hkBool v130 = fixedCollector.hasHit();
            if (v130.operator bool()) {
                int32_t NumHits = fixedCollector.getNumHits();
                hkAlgorithm::quickSort(cdPointA.m_data, NumHits);
            }
            if (iExplosionDebug.Int() > 1) {
                for (int32_t i = 0;; ++i) {
                    NiPoint3 normal;
                    int32_t v3 = fixedCollector.getNumHits();
                    if (i >= v3) {
                        break;
                    }
                    const hkpRootCdPoint* Hits = fixedCollector.getHits();
                    hkpRootCdPoint point(Hits[i]);
                    HK2NI(location, point.m_contact.getPosition());
                    HK2NINS(normal, point.m_contact.getNormal());
                    NiColorA red(1.0f, 0.0f, 0.0f, 0.65f);
                    NiAVObject* pdebug = (NiAVObject*)MakeTriPoint(1.5f, red, true);
                    pdebug->UpdateProperties();
                    pdebug->SetTranslate(location);
                    TES::pInstance->AddTempDebugObject(pdebug, 10.0f);
                    NiPoint3* v6 = ::new (&v122) NiPoint3(location + normal * 10.0f);
                    pdebug = (NiAVObject*)MakeLine(location, red, *v6, red, true);
                    TES::pInstance->AddTempDebugObject(pdebug, 10.0f);
                }
            }
            char v121 = 0;
            for (j = 0;; ++j) {
                int32_t v7;
                const hkpRootCdPoint* v8;
                if (v121 || (v7 = fixedCollector.getNumHits(), j >= v7)) {
                    if (this->bTargetsFound && !this->bTargetsProcessed) {
                        this->ProcessTargets();
                    }
                    return;
                }
                v8 = fixedCollector.getHits();
                point_2.~hkpRootCdPoint();
                ::new (&point_2) hkpRootCdPoint(v8[j]);
                if (point_2.m_rootCollidableB) {
                    pav = GetAVObjectForCollidable(point_2.m_rootCollidableB);
                    TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                    if (!pref || !pref->IsActor()) {
                        break;
                    }
                }
            LABEL_101:;
                ;
            }
            HK2NI(contactPoint, point_2.m_contact.getPosition());
            HK2NINS(contactNormal, point_2.m_contact.getNormal());
            hkpAllRayHitCollector pickCollector;
            bhkPickData pd;
            NiPoint3 origin = this->GetLocationOnReference();
            NiPoint3 pickLength = 2.0f * (contactPoint - origin);
            if (pickLength.Length() < 1.0) {
                NiPoint3* v10 = ::new (&v109) NiPoint3(contactNormal * 16.0f);
                origin -= *v10;
                pickLength = contactNormal * 32.0f;
            }
            pd.NiSetFrom(origin);
            pd.NiSetLength(pickLength);
            CFilter* CollisionFilter = this->spPhantom.m_pObject->GetCollisionFilter(&v107);
            pd.SetFilter(*CollisionFilter);
            pd.SetAllCollector(&pickCollector);
            HavokMaterialType emat = BHK_MATERIAL_MAX;
            char v105 = 0;
            if (this->GetParentCell()->GetbhkWorld()) {
                bhkWorld* pw_2 = this->GetParentCell()->GetbhkWorld();
                if (pw_2->PickObject(pd)) {
                    char v103 = 0;
                    for (int32_t k = 0; !v105; ++k) {
                        if (v103) {
                            break;
                        }
                        int32_t FormHealth_0 = (&pd.pAllCollector->getHits())->m_size;
                        if (k >= FormHealth_0) {
                            break;
                        }
                        const hkpWorldRayCastOutput* v13 =
                            &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(k);
                        hkpWorldRayCastOutput pickPoint(*v13);
                        NiAVObject* ppickAV = GetAVObjectForCollidable(pickPoint.m_rootCollidable);
                        TESObjectREFR* ppickRef = TESObjectREFR::FindReferenceFor3D(ppickAV);
                        if (pav == ppickAV) {
                            v105 = 1;
                            uint32_t key = -1;
                            for (int32_t j_2 = 0; j_2 < 8 && pickPoint.m_shapeKeys[j_2] != -1; ++j_2) {
                                key = pickPoint.m_shapeKeys[j_2];
                            }
                            uint32_t iNumExplosionDecalCDPoint_ = pickPoint.m_rootCollidable->getCollisionFilterInfo();
                            uint32_t Group = CFilter(iNumExplosionDecalCDPoint_).GetGroup();
                            bool v95 = Group == 1;
                            if (Group == 1) {
                                emat = TES::pInstance->GetLandMaterialType(contactPoint);
                            }
                            else {
                                bhkShape* pshape = bhkShape::Getbhk(pickPoint.m_rootCollidable->m_shape);
                                if (pshape) {
                                    emat = pshape->GetSubshapeMaterial(key);
                                }
                            }
                        }
                        else {
                            if (!ppickRef || !ppickRef->IsActor()) {
                                v103 = 1;
                            }
                        }
                    }
                }
            }
            if (!v105) {
                goto LABEL_100;
            }
            p_ClosestPoint = &this->ClosestPoint;
            this->ClosestPoint.x = contactPoint.x;
            p_ClosestPoint->y = contactPoint.y;
            p_ClosestPoint->z = contactPoint.z;
            if (this->ClosestPointNormal == NiPoint3_ZERO) {
                this->ClosestPointNormal = contactNormal;
            }
            if (iExplosionDebug.Int() > 1) {
                NiColorA green(0.0f, 1.0f, 0.0f, 0.65f);
                NiAVObject* pdebug_2 = (NiAVObject*)MakeTriPoint(5.0f, green, true);
                pdebug_2->UpdateProperties();
                pdebug_2->SetTranslate(this->ClosestPoint);
                TES::pInstance->AddTempDebugObject(pdebug_2, 10.0f);
                NiPoint3* v17 = ::new (&v90) NiPoint3(this->ClosestPoint + this->ClosestPointNormal * 20.0f);
                pdebug_2 = (NiAVObject*)MakeLine(this->ClosestPoint, green, *v17, green, true);
                TES::pInstance->AddTempDebugObject(pdebug_2, 10.0f);
            }
            v121 = 1;
            pimpactData = this->GetExplosionBase()->GetImpactData(emat);
            if (!pimpactData) {
            LABEL_100:;
                goto LABEL_101;
            }
            char v88 = 1;
            NiCamera* v18 = Main::WorldRootCamera();
            NiPoint3 distance = contactPoint - v18->GetWorldTranslate();
            if (Main::GetSceneIsComplex() ||
                ((v86 = distance.Length(), (v19 = &fGunParticleCameraDistance.Float(), *v19 < v86)) ||
                 (v20 = Main::WorldRootCamera(), !PointInFrustum(contactPoint, v20, 32.0f)))) {
                v88 = 0;
            }
            BGSTextureSet* DecalTextureSet = pimpactData->pDecalTextureSet;
            if (DecalTextureSet) {
                float iNumExplosionDecalCDPoint_a = pimpactData->GetDecalMaxWidth();
                float x_4 = pimpactData->GetMinWidth();
                float fDecalSize = RandomFloat(x_4, iNumExplosionDecalCDPoint_a);
                DecalCaster* pcaster = DecalCaster::GetInstance(fDecalSize, true);
                pcaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
                pcaster->SetCollision(L_BIPED, false);
                pcaster->SetCollision(L_DEADBIP, false);
                TESObjectCELL* pcell = this->GetParentCell();
                bhkWorld* pw = pcell ? pcell->GetbhkWorld() : nullptr;
                if (pw) {
                    pcaster->Add(pw);
                    bhkFilteredCdBodyCollector v79;
                    pcaster->GetPenetrations(contactPoint, v79);
                    int32_t v78 = (&v79.m_hits)->m_size;
                    float v77 = RandomFloatTwoPi();
                    float iNumExplosionDecalCDPoint_b = RandomFloat(0.0f, 4.0f);
                    char v76 = static_cast<int32_t>(std::floor(iNumExplosionDecalCDPoint_b));
                    for (int32_t m = 0;; ++m) {
                        if (m >= v78) {
                            pcaster = nullptr;
                            break;
                        }
                        const hkpCollidable* m_rootCollidableB =
                            (&(&v79.m_hits)->operator[]<hkpRootCdBodyPair>(m))->m_rootCollidableB;
                        if (m_rootCollidableB) {
                            HavokMaterialType Material = emat;
                            uint32_t m_shapeKeyB = (&(&v79.m_hits)->operator[]<hkpRootCdBodyPair>(m))->m_shapeKeyB;
                            if (m_shapeKeyB == -1) {
                                bhkShape* v69 = bhkShape::Getbhk(m_rootCollidableB->m_shape);
                                if (!v69) {
                                    continue;
                                }
                                Material = v69->GetMaterial();
                            }
                            else {
                                bhkShape* v71 = bhkShape::Getbhk(m_rootCollidableB->m_shape);
                                if (!v71) {
                                    continue;
                                }
                                bhkShapeCollection* v70 = v71->FindShapeCollection();
                                if (v70) {
                                    Material = v70->QSubMaterial(m_shapeKeyB);
                                }
                            }
                            if (Material == emat) {
                                uint32_t iNumExplosionDecalCDPoint_c = m_rootCollidableB->getCollisionFilterInfo();
                                uint32_t v21 = CFilter(iNumExplosionDecalCDPoint_c).GetGroup();
                                bool v67 = v21 == 1;
                                NiAVObject* Count = nullptr;
                                if (v21 == 1) {
                                    hkpRigidBody* RigidBody = hkpGetRigidBody(m_rootCollidableB);
                                    if (RigidBody) {
                                        // The property is keyed by NiTriStrips' RTTI address; its value is the scene
                                        // object's pointer.
                                        hkpPropertyValue* Property = bhkWorldObject::GetProperty(
                                            &v64, RigidBody, bhkPropertyKey(NiTriStrips::ms_RTTI)
                                        );
                                        Count = static_cast<NiAVObject*>(Property->getPtr());
                                        if ((Count) && (Count->m_pkParent)) {
                                            Count = (NiAVObject*)Count->m_pkParent;
                                        }
                                    }
                                }
                                else {
                                    Count = bhkUtilFunctions::GetNiAVObject(m_rootCollidableB);
                                }
                                TESObjectREFR* pref_2 = TESObjectREFR::FindReferenceFor3D(Count);
                                if (v67 ||
                                    (pref_2 &&
                                     BGSDecalManager::IsValidDecalImpact(pref_2, BGSDecalManager::DECAL_TYPE_SIMPLE))) {
                                    TESObjectCELL* pcell_2 = TES::pInstance->GetCellForPoint(contactPoint);
                                    alignas(16) DECAL_CREATION_DATA data;
                                    data.pOrigin.x = this->ClosestPoint.x;
                                    data.pOrigin.y = this->ClosestPoint.y;
                                    data.pOrigin.z = this->ClosestPoint.z;
                                    data.pDirection = this->ClosestPointNormal;
                                    data.fDecalWidth = fDecalSize;
                                    data.fDecalHeight = fDecalSize;
                                    data.fDecalDepth = pimpactData->GetDecalDepth();
                                    data.pAVObj = Count;
                                    data.pTexSet = DecalTextureSet;
                                    data.fRotation = v77;
                                    data.cSubTex = v76;
                                    data.bAlphaBlending = pimpactData->GetIsAlphaBlend();
                                    data.bAlphaTesting = pimpactData->GetIsAlphaTest();
                                    data.bUseParallaxOcclusion = pimpactData->GetIsParallax();
                                    data.fParallaxOcclusionScale = pimpactData->GetDecalParallaxScale();
                                    data.ucParallaxOcclusionMaxPasses = pimpactData->GetParallaxPasses();
                                    data.fSpecular = pimpactData->GetDecalShininess();
                                    data.fDecalEpsilon = pimpactData->Data.fAngleThreshold;
                                    int32_t pavobj = static_cast<uint8_t>(pimpactData->GetDecalColor() >> 16);
                                    float v59 = static_cast<double>(pavobj) / 255.0;
                                    float iNumExplosionDecalCDPoint_d = v59;
                                    int32_t v58 =
                                        static_cast<uint8_t>(static_cast<uint16_t>(pimpactData->GetDecalColor()) >> 8);
                                    float v57 = static_cast<double>(v58) / 255.0;
                                    float x_4a = v57;
                                    int32_t DecalColor = static_cast<uint8_t>(pimpactData->GetDecalColor());
                                    float v55 = static_cast<double>(DecalColor) / 255.0;
                                    data.fVertexColor = NiColor(v55, x_4a, iNumExplosionDecalCDPoint_d);
                                    pcell_2->AddDecal(data, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
                                }
                            }
                        }
                    }
                }
            }
            if (!v88 || (this->bUnderWater || ((v53 = pimpactData->GetModel()) == nullptr || !*v53))) {
            LABEL_96:;
                TESSound* pimpactSound1 = pimpactData->pSound1;
                if (pimpactSound1) {
                    const char* x = pimpactSound1->GetFilename();
                    BSAudio* v26 = &BSAudio::QInstance();
                    BSSoundHandle soundHandle = v26->GetSoundHandleByFilename(
                        x,
                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                            BSGameSound::SOUND_FLAG_IMPACT,
                        pimpactSound1
                    );
                    soundHandle.SetPosition(contactPoint.x, contactPoint.y, contactPoint.z);
                    soundHandle.Play(false);
                }
                TESSound* pimpactSound2 = pimpactData->pSound2;
                if (pimpactSound2) {
                    const char* xa = pimpactSound2->GetFilename();
                    BSAudio* v27 = &BSAudio::QInstance();
                    BSSoundHandle soundHandle_2 = v27->GetSoundHandleByFilename(
                        xa,
                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                            BSGameSound::SOUND_FLAG_IMPACT,
                        pimpactSound2
                    );
                    soundHandle_2.SetPosition(contactPoint.x, contactPoint.y, contactPoint.z);
                    soundHandle_2.Play(false);
                }
                goto LABEL_100;
            }
            BGSImpactData::ORIENTATION Interior = pimpactData->Data.eOrient;
            if (Interior) {
                if (Interior == BGSImpactData::ORIENT_PROJ_VECTOR) {
                    const NiPoint3* v23 = &this->GetLocationOnReference();
                    impactEffectVector = *v23 - contactPoint;
                    impactEffectVector.Unitize();
                    goto LABEL_95;
                }
                if (Interior == BGSImpactData::ORIENT_PROJ_REFLECT) {
                    const NiPoint3* v24 = &this->GetLocationOnReference();
                    alignas(16) NiPoint3 projectileVec = *v24 - contactPoint;
                    projectileVec.Unitize();
                    NiPoint3 normalVec = contactNormal;
                    normalVec.Unitize();
                    float v46 = normalVec.Dot(projectileVec) * 2.0;
                    impactEffectVector = v46 * normalVec - projectileVec;
                    goto LABEL_95;
                }
            }
            impactEffectVector = contactNormal;
            impactEffectVector.Unitize();
        LABEL_95:;
            NiPoint3 v142 = contactPoint;
            NiPoint3 v141 = impactEffectVector;
            float afLifetime = pimpactData->Data.fEffectDuration;
            BSTempEffectParticle::Spawn(this->GetParentCell(), afLifetime, v53, v141, v142, 1.0f, 7u, nullptr);
            goto LABEL_96;
        }
    }
}

void Explosion::UpdateNonRenderSafeMovement(float afDelta) {
    if ((IsDistant()) && (Get3D())) {
        NiUpdateData a2(fCellAppTime, true, false);
        NiNode* v2 = Get3D();
        v2->Update(a2);
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
    return (TESObjectREFR::GetParentCell() == nullptr);
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

void Explosion::SetOuterRadius(float a2) {
    fRadius = a2;
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
            if ((abWeakPoint) && (!pTarget->bHitWeakPoint)) {
                pTarget->bHitWeakPoint = abWeakPoint;
            }

            if ((abMobile) && (!pTarget->bMobile)) {
                pTarget->bMobile = abMobile;
            }

            return pTarget;
        }
    }
    void* pMemory = ourMalloc(sizeof(ExplosionTarget));
    ExplosionTarget* pTarget;
    if (pMemory) {
        pTarget = ::new (pMemory) ExplosionTarget;
    }
    else {
        pTarget = nullptr;
    }
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

void Explosion::ProcessMovement(float a2) {
    float seconds = (&appTimer)->GetSeconds();
    Update(seconds);
    fAge = (fAge + seconds);
}

void Explosion::Kill() {
    SetDelete(true);
}

BSSimpleList<Explosion::ExplodedLimb*>* Explosion::GetAffectedLimbList(TESObjectREFR* apTarget) {
    BSSimpleList<Explosion::ExplodedLimb*>* presult = nullptr;
    for (BSSimpleList<Explosion::ExplosionTarget*>* ptargets = &TargetList;
         !presult && (ptargets && !ptargets->IsEmpty()); ptargets = ptargets->m_pkNext) {
        Explosion::ExplosionTarget* ptarget = ptargets->m_item;
        if ((ptarget) && (ptarget->pRef == apTarget)) {
            presult = &ptarget->limbList;
        }
    }
    return presult;
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
    bhkRigidBodyCinfo cinfo;
    Load3D(false);
    NiNode* pav = this->Get3D();
    if (pav) {
        NiFixedString* v12;
        NiControllerSequence* pseqArea;
        alignas(16) NiMatrix3 rot;
        float fscale = 1.0f;
        if (this->IsDistant()) {
            BSFadeNode* pfade;
            NiNode* pnode = pav->IsNode();
            if (pnode) {
                pfade = pnode->IsFadeNode();
            }
            else {
                pfade = nullptr;
            }
            if (pfade) {
                pfade->SetRange(3.4028235e+38f, 3.4028235e+38f);
            }
        }
        if (!iMolotovTest.Int()) {
            NiAVObject* pradiusNode = BSUtilities::GetObjectByName(pav, "BlastRadiusNode");
            if (pradiusNode) {
                double Radius = this->fRadius;
                fscale = Radius / (&pradiusNode->GetTranslate())->Length();
            }
            else {
                const char* FullName;
                if (this->GetExplosionBase()->GetFormEditorIDLength()) {
                    BGSExplosion* ExplosionBase = this->GetExplosionBase();
                    FullName = ExplosionBase->GetFormEditorID();
                }
                else {
                    FullName = this->GetExplosionBase()->GetFullName();
                }
                Warning(
                    "COMBAT: Explosion does not have a BlastRadiusNode: %08X %s ", this->GetExplosionBase()->iFormID,
                    FullName
                );
                fscale = 1.0f;
            }
        }
        pav->SetScale(fscale);
        const NiPoint3* v1 = &this->GetLocationOnReference();
        pav->SetTranslate(*v1);
        NiPoint3 angle = this->GetAngleOnReference();
        rot.FromEulerAnglesZYX(angle.z, angle.y, angle.x);
        pav->SetRotate(rot);
        if (iMolotovTest.Int() > 1) {
            this->GetExplosionBase()->SetForce(f1_0_11A5810);
            this->fLifetime = f15_0_11A580C;
            NiPoint3 v60 = this->GetLocationOnReference();
            const NiFixedString* v2 = &BSXFlags::Tag();
            BSXFlags* pbsx = static_cast<BSXFlags*>(pav->GetExtraData(*v2));
            if (pbsx) {
                NiExtraData* pextraData = (NiExtraData*)pbsx;
                pbsx->SetFlags(16u, true);
            }
            for (int32_t i = 0;; ++i) {
                bhkLiquidAction* pliquid;
                BSValueNode* paddon;
                bhkRigidBody* pbody;
                bhkSphereShape* pshape;
                bhkCollisionObject* pobj;
                NiNode* pthing;
                int32_t* v3 = &iMolotovTest.Int();
                if (i >= *v3) {
                    break;
                }
                BSCloningProcess process(1.0f);
                NiNode* v55 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
                if (v55) {
                    pthing = ::new (v55) NiNode(0);
                }
                else {
                    pthing = nullptr;
                }
                NiNode* v53 = pthing;
                NiNode* pthing_2 = pthing;
                BSRandom* Singleton = BSRandom::GetSingleton();
                float v51 = Singleton->FloatNeg1To1() * 10.0 + 32.0;
                float v14 = v51;
                BSRandom* v5 = BSRandom::GetSingleton();
                float v50 = v5->FloatNeg1To1() * 10.0;
                float v13 = v50;
                BSRandom* v6 = BSRandom::GetSingleton();
                float v49 = v6->FloatNeg1To1() * 10.0;
                pthing_2->SetTranslate(v49, v13, v14);
                bhkCollisionObject* v48 =
                    static_cast<bhkCollisionObject*>(NiMemObject::operator new(sizeof(bhkCollisionObject)));
                if (v48) {
                    pobj = ::new (v48) bhkCollisionObject(pthing_2);
                }
                else {
                    pobj = nullptr;
                }
                bhkCollisionObject* v46 = pobj;
                bhkCollisionObject* pobj_2 = pobj;
                bhkSphereShape* v44 = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
                if (v44) {
                    pshape = ::new (v44) bhkSphereShape(8.0f, true);
                }
                else {
                    pshape = nullptr;
                }
                bhkSphereShape* v42 = pshape;
                bhkSphereShape* pshape_2 = pshape;
                cinfo.~bhkRigidBodyCinfo();
                ::new (&cinfo) bhkRigidBodyCinfo;
                cinfo.SetMass(5.0f);
                cinfo.SetRestitution(0.0f);
                cinfo.SetMotionType(hkpMotion::MOTION_DYNAMIC);
                cinfo.SetFriction(1.0f);
                uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
                cinfo.SetCollisionFilter(CFilter(L_CLUTTER, NewSystemGroup, P_OTHER).iFilter);
                cinfo.SetShape(pshape_2);
                bhkRigidBody* v38 = static_cast<bhkRigidBody*>(NiMemObject::operator new(sizeof(bhkRigidBody)));
                if (v38) {
                    pbody = ::new (v38) bhkRigidBody(cinfo);
                }
                else {
                    pbody = nullptr;
                }
                bhkRigidBody* v36 = pbody;
                bhkRigidBody* pbody_2 = pbody;
                pobj_2->SetWorldObject(pbody);
                pav->AttachChild(pthing_2, 1);
                BSValueNode* v34 = static_cast<BSValueNode*>(NiMemObject::operator new(sizeof(BSValueNode)));
                if (v34) {
                    paddon = ::new (v34) BSValueNode;
                }
                else {
                    paddon = nullptr;
                }
                BSValueNode* v32 = paddon;
                BSValueNode* paddon_2 = paddon;
                paddon->iValue = 14;
                pthing_2->AttachChild(paddon_2, 1);
                hkLiquidActionCinfo info;
                info.m_body = **pbody_2;
                info.fInitialStickForce = f5000_0;
                info.fStickStrength = f15_0_;
                info.fNeighborDistance = f64_0;
                info.fNeighborStrength = f15_0;
                bhkLiquidAction* v29 =
                    static_cast<bhkLiquidAction*>(NiMemObject::operator new(sizeof(bhkLiquidAction)));
                if (v29) {
                    pliquid = ::new (v29) bhkLiquidAction(info);
                }
                else {
                    pliquid = nullptr;
                }
                bhkLiquidAction* v27 = pliquid;
                bhkLiquidAction* v26 = pliquid;
                pbody_2->AddAction(pliquid);
            }
            if ((this->GetParentCell()) && (this->GetParentCell()->GetbhkWorld())) {
                this->GetParentCell()->GetbhkWorld();
                bhkWorld::UpdatePosition(pav, true, true);
                NiUpdateData update(0.0f, false, false);
                pav->Update(update);
                bhkWorld* v24 = this->GetParentCell()->GetbhkWorld();
                v24->AddObjects(pav, true, true, 0, true);
            }
        }
        if ((this->GetParentCell()) && (this->GetParentCell()->GetbhkWorld())) {
            this->GetParentCell()->GetbhkWorld();
            bhkWorld::UpdatePosition(pav, true, true);
            NiUpdateData update_2(0.0f, false, false);
            pav->Update(update_2);
            bhkWorld* v22 = this->GetParentCell()->GetbhkWorld();
            v22->AddObjects(pav, true, true, 0, true);
        }

        if (pav->IsNode()) {
            NiNode* v9 = pav->IsNode();
            if (TESObjectREFR::HasAddonFlags(v9)) {
                NiNode* v10 = pav->IsNode();
                TESObjectREFR::AddMasterParticleAddonNodes(v10);
            }
        }
        if ((this->GetParentCell()) && (this->GetParentCell()->GetbhkWorld())) {
            bhkWorld* v11 = this->GetParentCell()->GetbhkWorld();
            if (v11->QDebugDisplay()) {
                bhkWorld* v21 = this->GetParentCell()->GetbhkWorld();
                v21->SetDebugDisplay(pav, true);
                PrepareObjectWithNoLightingRecurse(pav);
            }
        }

        NiTimeController* po = pav->GetControllers();
        if (!po) {
            NiNode* pn = pav->IsNode();
            if ((pn) && (pn->GetAtChecked(0))) {
                po = pn->GetAt(0)->GetControllers();
            }
        }
        NiControllerManager* pmanager = nullptr;
        while (po) {
            if (NiObject::IsKindOf_0(&NiControllerManager::ms_RTTI, po)) {
                pmanager = static_cast<NiControllerManager*>(po);
                break;
            }
            po = po->GetNext();
        }
        if (pmanager &&
            (v12 = &FixedStrings::QSpecialIdle_AreaEffect(),
             (pseqArea = pmanager->GetSequenceByName(*v12)) != nullptr)) {
            pmanager->DeactivateAll(0.0f);
            pmanager->ActivateSequence(pseqArea, 0, false, 1.0f, 0.0f, nullptr);
            pmanager->SetActive(true);
            pseqArea->ResetSequence();
            double Lifetime = this->fLifetime;
            if (pseqArea->m_fEndKeyTime > Lifetime) {
                float EndKeyTime = pseqArea->m_fEndKeyTime;
                SetLifetime(EndKeyTime);
            }
        }
        else {
            NiTimeController::StartAnimations(pav);
        }
    }
}

void Explosion::ApplyForces() {
    using Detail = hkpSimpleShapePhantom::CollisionDetail;
    CFilter apResult;
    if (spPhantom.m_pObject) {
        bool bonlyAffectExploded = GetExplosionBase()->GetFlag(32);
        for (int32_t i = 0;; ++i) {
            COL_LAYER elayer;
            hkArray<Detail, hkContainerHeapAllocator>* pDetails = &spPhantom.m_pObject->GetCollisionDetails();
            if (i >= pDetails->m_size) {
                break;
            }
            pDetails = &spPhantom.m_pObject->GetCollisionDetails();
            hkpRigidBody* phk = hkpGetRigidBody(pDetails->operator[]<Detail>(i).m_collidable);
            bhkRigidBody* prb = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(phk));
            if (prb) {
                CFilter* CollisionFilter = prb->GetCollisionFilter(&apResult);
                elayer = CollisionFilter->GetLayer();
            }
            else {
                elayer = L_UNIDENTIFIED;
            }
            COL_LAYER v16 = elayer;
            if (bonlyAffectExploded || v16 == L_DEADBIP) {
                bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(phk);
                NiAVObject* pav = pcol ? pcol->m_pkSceneObject : nullptr;
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                if (bonlyAffectExploded && pref != pExplodedRef) {
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
            if ((prb) && (prb->GetMotionType() < 4 || v16 == L_DEBRIS_LARGE)) {
                PushRigidBody(prb);
            }
        }
    }
    bForcesApplied = true;
}

void Explosion::ProcessTargets() {
    NiPoint2 v33;
    NiPoint2 apResult;
    bTargetsProcessed = true;
    BSSimpleList<Explosion::ExplosionTarget*>* plist = &TargetList;
    while (plist && !plist->IsEmpty()) {
        Explosion::ExplosionTarget* ptarget = plist->m_item;
        plist = plist->m_pkNext;
        if ((ptarget) && (ptarget->pRef) &&
            (!ptarget->bProcessed && !ptarget->pRef->GetDestroyed() && !ptarget->pRef->GetDisabled())) {
            float fimagespacepower;
            const NiPoint3* afDistance = &GetLocationOnReference();
            const NiPoint3* v1 = &ptarget->pRef->GetLocationOnReference();
            float fdistance = (*v1 - *afDistance).Length();
            float fdistance_2 = fdistance;
            float afStrength = fRadius;
            float fpower = CombatFormulas::CalcExplosionPower(afStrength, fdistance_2);
            float fimagespaceradius = fImageSpaceRadius;
            if (!(fimagespaceradius > 0.0)) {
                fimagespacepower = 1.0f;
            }
            else {
                fimagespacepower = CombatFormulas::CalcExplosionPower(fimagespaceradius, fdistance);
            }
            float fimagespacepower_2 = fimagespacepower;
            if (fimagespacepower > 0.0 && ptarget->pRef->IsActor()) {
                PlayerCharacter* pactor_2 = static_cast<PlayerCharacter*>(ptarget->pRef);
                if (pactor_2 == pPlayer) {
                    BGSExplosion* pbase = GetExplosionBase();
                    TESImageSpaceModifier* pmodifier = pbase->pFormImageSpaceModifying;
                    if (pmodifier) {
                        ImageSpaceModifierInstanceForm::Trigger(pmodifier, fimagespacepower_2, nullptr);
                    }
                }
            }
            if (fpower > 0.0) {
                Actor* a1;
                Actor* pactor;
                if (ptarget->bMobile) {
                    ActorCause* v3 = GetActorCause();
                    ptarget->pRef->SetActorCause(v3);
                }
                if (ptarget->pRef->IsActor()) {
                    pactor = static_cast<Actor*>(ptarget->pRef);
                }
                else {
                    pactor = nullptr;
                }
                Actor* pactor_3 = pactor;
                if (pactor) {
                    pactor_3->ClearLastHitData();
                }
                if (GetActorCause()) {
                    a1 = GetActorCause()->pActor;
                }
                else {
                    a1 = nullptr;
                }
                Script::SetActionFlag(a1, &ptarget->pRef->m_Extra, SCRIPT_ACTION_ONHIT);
                if (GetSourceWeapon()) {
                    ExtraDataList* afStrengtha = &ptarget->pRef->m_Extra;
                    TESObjectWEAP* SourceWeapon = GetSourceWeapon();
                    Script::SetActionFlag(SourceWeapon, afStrengtha, SCRIPT_ACTION_ONHITWITH);
                }
                else {
                    Script::SetActionFlag(GetExplosionBase(), &ptarget->pRef->m_Extra, SCRIPT_ACTION_ONHITWITH);
                }
                double Damage = GetDamage();
                float fdamage = Damage * fpower;
                if (ptarget->pRef->GetDestroyed()) {
                    PlayerCharacter* Owner = static_cast<PlayerCharacter*>(GetOwner());
                    if (Owner == pPlayer) {
                        pPlayer->AddPlayerAction(PLAYER_ACTION_DESTROY_OBJECT, 8.0f, nullptr);
                    }
                }
                if (pactor_3) {
                    Actor* afDistancea = pactor_3;
                    NiPoint3 afScale = GetLocationOnReference();
                    if (RunLOSPick(afScale, afDistancea)) {
                        HitData data;
                        if (pOwner && pOwner->IsActor()) {
                            HitData::InitializeExplosionData(
                                &data, static_cast<Actor*>(pOwner), pactor_3, this, fdamage
                            );
                        }
                        else {
                            HitData::InitializeExplosionData(&data, nullptr, pactor_3, this, fdamage);
                        }
                        pactor_3->HitMe(&data, 0);
                        if (pactor_3 == pPlayer) {
                            float* v7 = &fExplosionShudderPowerMin.Float();
                            if (*v7 < static_cast<double>(fpower)) {
                                float afDistanceb = fExplosionShudderDurationY.Float();
                                float* v8 = &fExplosionShudderDurationX.Float();
                                NiPoint2 kduration(*v8, afDistanceb);
                                float afDistancec = fExplosionShudderIntensityY.Float();
                                float* v9 = &fExplosionShudderIntensityX.Float();
                                NiPoint2 kintensity(*v9, afDistancec);
                                float afDistanced = fExplosionShudderFrequencyY.Float();
                                float* v10 = &fExplosionShudderFrequencyX.Float();
                                NiPoint2 kfrequency(*v10, afDistanced);
                                NiPoint2* afStrengthb = NiPoint2::Scale_(&apResult, fpower, &kintensity);
                                NiPoint2* afTime = NiPoint2::Scale_(&v33, fpower, &kduration);
                                FOHUDEffectManager* HUDEffects = Interface::GetHUDEffects();
                                HUDEffects->TriggerShudder_007F78F0(*afTime, *afStrengthb, kfrequency);
                            }
                        }
                        EnchantmentItem* FormEnchanting = GetExplosionBase()->pFormEnchanting;
                        if (FormEnchanting) {
                            TESObjectREFR* v28;
                            if ((GetOwner() && (v28 = GetOwner(), v28->IsActor())) || pCaster) {
                                if (pCaster) {
                                    MagicItem* v26;
                                    MagicTarget* v27;
                                    if (pactor_3) {
                                        v27 = static_cast<MagicTarget*>(pactor_3);
                                    }
                                    else {
                                        v27 = nullptr;
                                    }
                                    if (FormEnchanting) {
                                        v26 = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*FormEnchanting));
                                    }
                                    else {
                                        v26 = nullptr;
                                    }
                                    pCaster->CastSpellImmediate(v26, false, v27, 1.0f, false);
                                }
                                else {
                                    MagicItem* v24;
                                    MagicTarget* v25;
                                    if (pactor_3) {
                                        v25 = static_cast<MagicTarget*>(pactor_3);
                                    }
                                    else {
                                        v25 = nullptr;
                                    }
                                    if (FormEnchanting) {
                                        v24 = &static_cast<MagicItem&>(static_cast<MagicItemForm&>(*FormEnchanting));
                                    }
                                    else {
                                        v24 = nullptr;
                                    }
                                    // The owner is an actor here (tested above); the call goes through its
                                    // MagicCaster with no null test.
                                    static_cast<Actor*>(GetOwner())->CastSpellImmediate(v24, false, v25, 1.0f, false);
                                }
                            }
                        }
                        if (!pactor_3->IsDead(false)) {
                            char ShouldDamageKnockdownTarget;
                            if (GetExplosionBase()->GetFlag(4) ||
                                (GetExplosionBase()->GetFlag(8) &&
                                 (!pactor_3
                                      ? ShouldDamageKnockdownTarget = ActorValueOwner::ShouldDamageKnockdownTarget(
                                            nullptr, static_cast<int32_t>(data.fHealthDamage)
                                        )
                                      : ShouldDamageKnockdownTarget = ActorValueOwner::ShouldDamageKnockdownTarget(
                                            pactor_3, static_cast<int32_t>(data.fHealthDamage)
                                        ),
                                  ShouldDamageKnockdownTarget))) {
                                PushActor(pactor_3, fpower);
                            }
                        }
                        if (pactor_3->IsDead(false)) {
                            NiAVObject* apObject;
                            if (pactor_3 == pPlayer) {
                                apObject = pPlayer->Get3D(false);
                            }
                            else {
                                apObject = (NiAVObject*)pactor_3->Get3D();
                            }
                            bhkWorld::SetMotion(apObject, hkpMotion::MOTION_DYNAMIC, true, false, true);
                            NiUpdateData UpdateData(0.0f, false, false);
                            apObject->Update(UpdateData);
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
    BaseProcess* CurrentAIProcess;
    if ((apTargetActor) && (apTargetActor->pCurrentProcess) &&
        (fExplosionForceKnockdownMinimum.Float() < static_cast<double>(afExplosionPower))) {
        CurrentAIProcess = apTargetActor->pCurrentProcess;
        NiPoint3 v5 = GetLocationOnReference();
        // The magnitude is 0.
        CurrentAIProcess->KnockExplosion(apTargetActor, v5, 0.0f);
    }
}

void Explosion::StartSounds() {
    StopSounds();
    if (GetExplosionBase()->Data.pSound1) {
        int32_t refID = GetExplosionBase()->Data.pSound1->iFormID;
        Sound1 = PlaySound(
            refID, false,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_BATTLE_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT |
                BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
    if (GetExplosionBase()->Data.pSound2) {
        int32_t uiFormID = GetExplosionBase()->Data.pSound2->iFormID;
        Sound2 = PlaySound(
            uiFormID, false,
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_BATTLE_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT |
                BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
            true
        );
    }
}

void Explosion::AttachLight() {
    if ((Get3D()) && (GetExplosionBase()->Data.pLight)) {
        NiNode* v2 = Get3D();
        TESObjectLIGH* WaterType = GetExplosionBase()->Data.pLight;
        spLight = static_cast<NiPointLight*>(WaterType->GenDynamic(this, v2, false));
    }
}

void Explosion::UpdateRadiation() {
    float Speed = GetExplosionBase()->Data.fRadiationRads;
    float DissipationTime = GetExplosionBase()->GetDissipationTime();
    if (Speed > 0.0 && DissipationTime > 0.0) {
        float frads = (1.0 - fAge / DissipationTime) * Speed;
        if (frads <= 0.0) {
            frads = 0.01f;
        }
        (&m_Extra)->SetRadiation(frads);
        AddChange(CHANGE_REFR_EXTRA_CREATED_ONLY);
    }
}

bool Explosion::RunLOSPick(NiPoint3 aLocation, TESObjectREFR* apTarget) {
    NiPoint3 v47;
    CFilter fltr;
    NiPoint3 targetLocation[7];
    bhkWorld* pw;
    NiNode* ptarget3D;
    int32_t v66;
    int32_t* v61 = &v66;
    BGSExplosion* pExplode = GetExplosionBase();
    if (pExplode->GetFlag(16)) {
        return true;
    }
    if (apTarget) {
        ptarget3D = apTarget->Get3D();
    }
    else {
        ptarget3D = nullptr;
    }
    NiNode* ptarget3D_2 = ptarget3D;
    if (apTarget == pPlayer) {
        ptarget3D_2 = static_cast<NiNode*>(pPlayer->Get3D(false));
    }
    if (!ptarget3D_2 || !ptarget3D_2->IsVisualObject()) {
        return false;
    }
    if (apTarget->GetParentCell()) {
        pw = apTarget->GetParentCell()->GetbhkWorld();
    }
    else {
        pw = nullptr;
    }
    if (!pw) {
        return false;
    }
    int32_t v54 = 0x26206;
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, 0x26206u);
    rt::eh_vector_constructor_iterator(targetLocation, sizeof(NiPoint3), 7, &emptyPointConstructor);
    targetLocation[0] = (&ptarget3D_2->GetWorldBound())->m_kCenter;
    char bInFurniture = 0;
    if ((apTarget->IsActor()) && (apTarget->GetSitSleepState())) {
        bInFurniture = 1;
        targetLocation[1] = targetLocation[0];
        const NiBound* WorldBound = &ptarget3D_2->GetWorldBound();
        double Seconds = WorldBound->m_fRadius;
        targetLocation[1].z = Seconds + Seconds + targetLocation[1].z;
        targetLocation[2] = targetLocation[0];
        const NiBound* v6 = &ptarget3D_2->GetWorldBound();
        double v7 = v6->m_fRadius;
        targetLocation[2].z = targetLocation[2].z - (v7 + v7);
        targetLocation[3] = targetLocation[0];
        const NiBound* v8 = &ptarget3D_2->GetWorldBound();
        double v9 = v8->m_fRadius;
        targetLocation[3].y = v9 + v9 + targetLocation[3].y;
        targetLocation[4] = targetLocation[0];
        const NiBound* v10 = &ptarget3D_2->GetWorldBound();
        double v11 = v10->m_fRadius;
        targetLocation[4].y = targetLocation[4].y - (v11 + v11);
        targetLocation[5] = targetLocation[0];
        const NiBound* v12 = &ptarget3D_2->GetWorldBound();
        double v13 = v12->m_fRadius;
        targetLocation[5].x = v13 + v13 + targetLocation[5].x;
        targetLocation[6] = targetLocation[0];
        const NiBound* v14 = &ptarget3D_2->GetWorldBound();
        double v15 = v14->m_fRadius;
        targetLocation[6].x = targetLocation[6].x - (v15 + v15);
    }

    bhkPickData pd;
    fltr.iFilter = 0;
    fltr.SetLayer(L_CUSTOMPICK1);
    bool v16 = apTarget->IsActor();
    Actor* pactorTarget = v16 ? static_cast<Actor*>(apTarget) : nullptr;
    if (pactorTarget && pactorTarget->GetCharController()) {
        bhkCharacterController* CharController = pactorTarget->GetCharController();
        uint32_t FilterGroup = CharController->GetFilterGroup();
        fltr.SetGroup(FilterGroup);
    }
    else {
        uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        fltr.SetGroup(NewSystemGroup);
    }
    float* v20 = &fExplosionLOSBufferDistance.Float();
    aLocation += *::new (&v47) NiPoint3(this->ClosestPointNormal * *v20);
    pd.SetFilter(fltr);
    pd.NiSetFrom(aLocation);
    pd.NiSetTo(*targetLocation);
    hkpAllRayHitCollector collector;
    pd.SetAllCollector(&collector);
    char bIsBlocked = 0;
    pw->PickObject(pd);
    for (int32_t i = 0; !bIsBlocked; ++i) {
        int32_t FormHealth_0 = (&pd.pAllCollector->getHits())->m_size;
        if (i >= FormHealth_0) {
            break;
        }
        const hkpWorldRayCastOutput* v22 = &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i);
        hkpWorldRayCastOutput point(*v22);
        NiPoint3 length = *targetLocation - aLocation;
        NiPoint3 toHit = point.m_hitFraction * length;
        if (this->ClosestPointNormal[2] < -0.98) {
            double v40 = toHit.Length();
            if (fExplosionLOSBuffer.Float() > v40) {
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
        NiColorA v35 = bIsBlocked ? NiColorA(1.0f, 0.0f, 0.0f, 1.0f) : NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
        NiAVObject* pdebug = (NiAVObject*)MakeLine(aLocation, v35, *targetLocation, v35, true);
        TES::pInstance->AddTempDebugObject(pdebug, 25.0f);
    }
    if ((bInFurniture) && (bIsBlocked)) {
        for (int32_t i_2 = 1; bIsBlocked && i_2 < 7; ++i_2) {
            bIsBlocked = 0;
            hkpAllRayHitCollector collector_2;
            pd.SetAllCollector(&collector_2);
            pd.NiSetTo(targetLocation[i_2]);
            pw->PickObject(pd);
            for (int32_t i_3 = 0; !bIsBlocked; ++i_3) {
                int32_t v24 = (&pd.pAllCollector->getHits())->m_size;
                if (i_3 >= v24) {
                    break;
                }
                const hkpWorldRayCastOutput* v25 =
                    &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i_3);
                hkpWorldRayCastOutput point_2(*v25);
                NiPoint3 length_2 = targetLocation[i_3] - aLocation;
                NiPoint3 toHit_2 = point_2.m_hitFraction * length_2;
                if (this->ClosestPointNormal[2] < -0.98) {
                    double v27 = toHit_2.Length();
                    if (fExplosionLOSBuffer.Float() > v27) {
                        continue;
                    }
                }
                bIsBlocked = 1;
            }
        }
    }

    bool v26 = bIsBlocked == 0;
    return v26;
}

bool Explosion::TestExplosionUnderWater() {
    NiPoint3 v18;
    NiPoint3 v20;
    int32_t v37;
    void* retaddr;
    int32_t* v32 = &v37;
    bool binWater = false;
    float fExplosionRadiusRatio = fExplosionWaterRadiusRatio.Float();
    TESObjectCELL* pcell = this->GetParentCell();
    if ((!this->IsDistant()) && (pcell)) {
        const NiPoint3* v1 = &this->GetLocationOnReference();
        double v27 = this->fRadius * fExplosionRadiusRatio + v1->z;
        if (!(pcell->GetWaterHeight() > v27)) {
            hkpBroadPhase* pbroadphase;
            hkpWorld* phkworld;
            bhkWorld* pworld = pcell->GetbhkWorld();
            if (pworld) {
                phkworld = **pworld;
            }
            else {
                phkworld = nullptr;
            }
            if (phkworld) {
                pbroadphase = phkworld->m_broadPhase;
            }
            else {
                pbroadphase = nullptr;
            }
            if (pbroadphase) {
                NiPoint3* v7 = &(v20 = 1.0f * NiPoint3_UNIT_ALL);
                const NiPoint3* v2 = &this->GetLocationOnReference();
                alignas(16) NiPoint3 min = *v2 - *v7;
                NiPoint3* v8 = &(v18 = 1.0f * NiPoint3_UNIT_ALL);
                const NiPoint3* v3 = &this->GetLocationOnReference();
                NiPoint3 max = *v3 + *v8;
                hkAabb aabb;
                NI2HK(aabb.m_min, min);
                NI2HK(aabb.m_max, max);
                using hits_t = hkInplaceArray<hkpBroadPhaseHandlePair, 128, hkContainerHeapAllocator>;
                alignas(16) hits_t hits(0);
                pbroadphase->querySingleAabb(aabb, hits);
                for (int32_t i = 0; !binWater && i < hits.m_size; ++i) {
                    hkpBroadPhaseHandlePair* v13 = &hits[i];
                    const hkpCollidable* collB = static_cast<const hkpCollidable*>(
                        static_cast<hkpTypedBroadPhaseHandle*>(v13->m_b)->hkpTypedBroadPhaseHandle::getOwner()
                    );
                    NiAVObject* pav = GetAVObjectForCollidable(collB);
                    TESObjectREFR* pwaterRef = TESObjectREFR::FindReferenceFor3D(pav);
                    if ((pwaterRef) && (pwaterRef->GetObjectReference()->GetFormType() == PWAT_ID)) {
                        const NiPoint3* v4 = &this->GetLocationOnReference();
                        double Radius = this->fRadius;
                        double v9 = Radius * fExplosionRadiusRatio + v4->z;
                        if ((&pwaterRef->GetLocationOnReference())->z > v9) {
                            binWater = true;
                        }
                    }
                }
            }
        }
        else {
            binWater = true;
        }
    }

    this->bUnderWater |= binWater;
    return binWater;
}

void Explosion::FindTargets() {
    using Detail = hkpSimpleShapePhantom::CollisionDetail;
    Explosion::ExplosionTarget* pplayerTargetEntry = AddTarget(pPlayer, false, true);
    if (spPhantom.m_pObject) {
        for (int32_t uiIndex = 0;; ++uiIndex) {
            hkArray<Detail, hkContainerHeapAllocator>* pDetails = &spPhantom.m_pObject->GetCollisionDetails();
            if (uiIndex >= pDetails->m_size) {
                break;
            }
            pDetails = &spPhantom.m_pObject->GetCollisionDetails();
            NiAVObject* pav = GetAVObjectForCollidable(pDetails->operator[]<Detail>(uiIndex).m_collidable);
            Actor* pactor = static_cast<Actor*>(TESObjectREFR::FindReferenceFor3D(pav));
            if ((pactor) && (!pactor->IsActor() || !pactor->GetGhost())) {
                bool bdynamic;
                pDetails = &spPhantom.m_pObject->GetCollisionDetails();
                hkpRigidBody* pbody = hkpGetRigidBody(pDetails->operator[]<Detail>(uiIndex).m_collidable);
                if (pbody) {
                    bdynamic = pbody->getMotionType() < hkpMotion::MOTION_KEYFRAMED;
                }
                else {
                    bdynamic = false;
                }
                Explosion::ExplosionTarget* ptargetEntry = nullptr;
                if (pactor == pPlayer || pactor == pOwner) {
                    if (pactor == pPlayer) {
                        ptargetEntry = pplayerTargetEntry;
                    }
                }
                else {
                    ptargetEntry = AddTarget(pactor, false, bdynamic);
                }
                if ((ptargetEntry) && (pactor->IsActor() && GetDamage() > 0.0)) {
                    const NiPoint3* v7 = &GetLocationOnReference();
                    float afDistanceb = (pav->GetWorldTranslate() - *v7).Length();
                    CheckLimbTarget(ptargetEntry, pav, afDistanceb);
                }
            }
        }
    }
    bTargetsFound = true;
}

void Explosion::PushRigidBody(bhkRigidBody* apRigidBody) {
    if (apRigidBody) {
        float fforce = this->GetExplosionBase()->Data.fForce;
        if (fforce > 0.0) {
            hkVector4 hkCenter;
            NiPoint3 rbLoc;
            HK2NI(rbLoc, apRigidBody->GetCenterOfMassInWorld(hkCenter));
            NiPoint3 direction = rbLoc - this->GetLocationOnReference();
            direction.Unitize();
            CFilter kFilter;
            if (apRigidBody->GetCollisionFilter(&kFilter)->QBipLayer()) {
                float* pfZ = &direction[2];
                *pfZ = *pfZ + fExplosionForceClutterUpBias.Float();
            }
            uint8_t bSourceRef = 0;
            if (this->pExplodedRef) {
                bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(**apRigidBody);
                NiAVObject* pav;
                if (pcol) {
                    pav = pcol->m_pkSceneObject;
                }
                else {
                    pav = nullptr;
                }
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                if ((pref) && (pref == this->pExplodedRef)) {
                    CFilter kLayerFilter;
                    if (apRigidBody->GetCollisionFilter(&kLayerFilter)->GetLayer() == L_DEBRIS_LARGE) {
                        apRigidBody->SetMotionType(hkpMotion::MOTION_DYNAMIC);
                    }
                    fforce = fforce * fExplosionSourceRefMult.Float();
                    bSourceRef = 1;
                }
            }
            fforce = TESHavokUtilities::ScaleGameplayImpulseForce(fforce, apRigidBody, true);
            if (!bSourceRef) {
                fforce = BSMIN(fforce, fExplosionMaxImpulse.Float());
            }
            float z = fExplosionForceMultLinear.Float();
            alignas(16) NiPoint3 kLinearForce = direction * fforce;
            apRigidBody->ApplyLinearImpulseNi(kLinearForce * z);
            float za = RandomFloat(-1.0f, 1.0f);
            float y = RandomFloat(-1.0f, 1.0f);
            float x = RandomFloat(-1.0f, 1.0f);
            NiPoint3 kangularForce(x, y, za);
            float zb = fExplosionForceMultAngular.Float();
            apRigidBody->ApplyAngularImpulse(kangularForce * fforce * zb);
        }
    }
}

float Explosion::GetDamage() {
    if (!(fCalculatedDamage >= 0.0)) {
        fCalculatedDamage = CombatFormulas::CalcExplosionDamage(GetExplosionBase());
        if (GetOwner()) {
            Actor* pSource = static_cast<Actor*>(GetOwner());
            if ((pSource->IsActor()) && (GetSourceWeapon())) {
                Actor* Owner = static_cast<Actor*>(GetOwner());
                TESObjectWEAP* SourceWeapon = GetSourceWeapon();
                ActorValue::Index WeaponSkill = SourceWeapon->GetWeaponSkill();
                float fActorValue = Owner->GetActorFloatValue(WeaponSkill);
                float* v4 = &fDamageSkillBase.Float();
                float fDamageMult = fActorValue * 0.009999999776482582 * fDamageSkillMult.Float() + *v4;
                fCalculatedDamage = fCalculatedDamage * fDamageMult;
            }
        }
        return fCalculatedDamage;
    }

    return fCalculatedDamage;
}

void Explosion::DetachLight() {
    if (spLight.m_pObject) {
        NiLight* v2 = spLight.m_pObject;
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
        ShadowSceneNode_->RemoveLight(v2);
        spLight = static_cast<NiPointLight*>(nullptr);
    }
}

void Explosion::CheckLimbTarget(Explosion::ExplosionTarget* apTarget, NiAVObject* apTargetObject, float afDistance) {
    if ((apTarget) && (apTarget->pRef) && (apTarget->pRef->IsActor())) {
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
        if (icount < 2 || (!pfarthestLimb || afDistance < static_cast<double>(pfarthestLimb->fDistance))) {
            NiNode* v4 = apTargetObject->IsNode();
            BGSBodyPart::LIMB_ENUM elimb = ptargetActor->GetDamageLocation(v4);
            if (elimb != BGSBodyPart::LIMB_NONE && elimb != BGSBodyPart::LIMB_WEAPON) {
                AddLimbTarget(apTarget, elimb, afDistance);
            }
        }
    }
}

void Explosion::AddLimbTarget(Explosion::ExplosionTarget* apTarget, BGSBodyPart::LIMB_ENUM aeLimb, float afDistance) {
    int32_t v5 = 0;
    Explosion::ExplodedLimb* a2 = nullptr;
    BSSimpleList<Explosion::ExplodedLimb*>* ecx0 = &apTarget->limbList;
    while (ecx0 && (!ecx0->IsEmpty() && (!a2 || a2->eLimb != aeLimb))) {
        ++v5;
        Explosion::ExplodedLimb* v4 = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        if (!a2 || (v4->fDistance < static_cast<double>(a2->fDistance) || a2->eLimb == aeLimb)) {
            a2 = v4;
        }
    }
    if (a2 && v5 >= 2) {
        a2->eLimb = aeLimb;
        a2->fDistance = afDistance;
    }
    else {
        a2 = static_cast<Explosion::ExplodedLimb*>(ourMalloc(sizeof(Explosion::ExplodedLimb)));
        a2->eLimb = aeLimb;
        a2->fDistance = afDistance;
        apTarget->limbList.AddHead(a2);
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
        if ((ptarget) && (ptarget->pRef == apDeletedRef)) {
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
                    BGSEntryPoint::ADJUST_EXPLOSION_RADIUS, pPlayer, pweapon, &fRadiusAdjustment
                );
            }
        }
        else if (apOwnerRef && apOwnerRef == pPlayer) {
            ItemChange* pweaponChange =
                pPlayer->GetCurrentAIProcess() ? pPlayer->GetCurrentAIProcess()->GetCurrentWeapon() : nullptr;
            TESBoundObject* pweapon = pweaponChange ? pweaponChange->pContainerObj : nullptr;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::ADJUST_EXPLOSION_RADIUS, pPlayer, pweapon, &fRadiusAdjustment
            );
        }
        void* pmemory = ourMalloc(sizeof(Explosion));
        pexplosion =
            pmemory ? ::new (pmemory) Explosion(apExplosionBase, prealOwner, apCell, aOrigin, aRotation) : nullptr;
        float fRadius = pexplosion->GetRadius() * fRadiusAdjustment;
        pexplosion->SetOuterRadius(fRadius);
    }
    if (pexplosion) {
        if (pexplosion->GetOwner()) {
            ActorCause* pcause = nullptr;
            if (pexplosion->GetOwner()->IsActor()) {
                void* pmemory = ourMalloc(sizeof(ActorCause));
                pcause = pmemory ? ::new (pmemory) ActorCause(static_cast<Actor*>(pexplosion->GetOwner())) : nullptr;
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
void* hkpTypedBroadPhaseHandle::getOwner() {
    return reinterpret_cast<char*>(this) + m_ownerOffset;
}

hkpFixedBufferCdPointCollector::hkpFixedBufferCdPointCollector(hkpRootCdPoint* a1, int32_t a2)
  : m_pointsArrayBase{a1}, m_nextFreePoint{a1}, m_capacity{a2}, m_numPoints{0} {}

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
    return m_numPoints.operator int32_t() > 0;
}

void hkpFixedBufferCdPointCollector::operator delete(void* p) {
    if (p) {
        hkMemoryRouter* pInstance = &hkMemoryRouter::getInstance();
        pInstance->m_heap->blockFree(p, sizeof(hkpFixedBufferCdPointCollector));
    }
}

// Each column goes through one NiPoint3.
bhkMatrix3& bhkMatrix3::NI2HKNS(const NiMatrix3& arSrc) {
    NiPoint3 kCol;
    kCol.~NiPoint3();
    ::new (&kCol) NiPoint3;
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
void* hkAllocateStackRootCdPoints(int32_t a1, char* a2) {
    hkMemoryRouter* ecx0 = &hkMemoryRouter::getInstance();
    return ecx0->m_stack.allocateStack((112 * a1 + 127) & ~127, a2);
}
