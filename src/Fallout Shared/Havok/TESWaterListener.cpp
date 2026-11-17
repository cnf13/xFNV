#include "Fallout Shared/Havok/TESWaterListener.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkTTNode.h"
#include "BSHavok/bhkWaterListener.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/BGSWaterCollisionManager.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// TESWaterListener.cpp's own copies of the float constants ProcessRigidBody reads.
static float f2_0 = 2.0f;
static float fminus0_3 = -0.3f;
static float f0_5 = 0.5f;
static float f0_3 = 0.3f;
static float f0_75 = 0.75f;
static float f1_2 = 1.2f;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bAutoWaterUsesHavokShapes("bAutoWaterUsesHavokShapes:HAVOK", false);
NiTMapHkpRigidBodyBool TESWaterListener::RigidBodyUnderwaterMap(37);

float NiSign(float fValue) {
    if (fValue > 0.0) {
        return (float)1.0;
    }
    else {
        if (!(fValue < 0.0)) {
            return (float)0.0;
        }
        else {
            return (float)(-0x1p+0f);
        }
    }
}

WaterZone::WaterZone(TESObjectREFR* apRef, const hkpCollidable* apCol)
  : ZoneEntry(apRef, apCol), fWaterHeight(apRef ? apRef->GetLocationOnReference().z : 0.0f), pGroup(nullptr) {}

TargetEntry* WaterZone::AddTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags) {
    WaterTargetEntry* ptarget;
    if (pTargets) {
        ptarget = static_cast<WaterTargetEntry*>(GetTargetEntry(apTarget));
    }
    else {
        ptarget = nullptr;
    }
    if (ptarget) {
        if (ptarget->CollidableList.Find(
                apCol, 0, &BSSimpleArray<const hkpCollidable*>::DefaultEqualFunctionByAddress
            ) == -1) {
            ptarget->CollidableList.Add(apCol);
        }
    }
    else {
        WaterTargetEntry* pmemory =
            static_cast<WaterTargetEntry*>(bhkRagdollController::operator new(sizeof(WaterTargetEntry)));
        if (pmemory) {
            ptarget = ::new (pmemory) WaterTargetEntry(apTarget, apCol);
        }
        else {
            ptarget = nullptr;
        }
        ptarget->SetFlags(aiFlags, true);
        ptarget->pNext = pTargets;
        pTargets = ptarget;
    }
    return ptarget;
}

void WaterZone::RemoveCollidable(TESObjectREFR* apTarget, const hkpCollidable* apCol) {
    WaterTargetEntry* ptarget;
    if (pTargets) {
        ptarget = static_cast<WaterTargetEntry*>(GetTargetEntry(apTarget));
    }
    else {
        ptarget = nullptr;
    }
    if (ptarget) {
        int32_t iindex =
            ptarget->CollidableList.Find(apCol, 0, &BSSimpleArray<const hkpCollidable*>::DefaultEqualFunctionByAddress);
        if (iindex != -1) {
            ptarget->CollidableList.RemoveFast(iindex, 1);
        }
        if (!ptarget->CollidableList.iSize) {
            RemoveTargetEntry(ptarget);
        }
    }
}

WaterTargetEntry::WaterTargetEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) : TargetEntry(apRef, nullptr) {
    if (CollidableList.Find(apCol, 0, &BSSimpleArray<const hkpCollidable*>::DefaultEqualFunctionByAddress) == -1) {
        CollidableList.Add(apCol);
    }
}

WaterTargetEntry* WaterTargetEntry::GetNextEntry() {
    bhkTTNode<TESObjectREFR>* pnext = pNext;
    if (pnext) {
        // The next node is the next entry's node base: step back from it to its entry.
        return static_cast<WaterTargetEntry*>(pnext);
    }
    else {
        return 0;
    }
}

TESWaterListener::TESWaterListener() : fSplashTimer(1.0f), uiCellArrayWidth(uGridsToLoad.Int()) {
    pfWaterLevels = static_cast<float*>(ourMalloc(sizeof(float) * uiCellArrayWidth * uiCellArrayWidth));
}

TESWaterListener::TESWaterListener(float afWaterLevel)
  : fSplashTimer(1.0f), pfWaterLevels(static_cast<float*>(ourMalloc(sizeof(float)))), uiCellArrayWidth(1),
    iBaseCellNumberX(0), iBaseCellNumberY(0) {
    SetWaterHeight(afWaterLevel, 0, 0);
}

TESWaterListener::~TESWaterListener() {
    HandleQueuedRemovals(nullptr, false, false);
    ourFree(pfWaterLevels);
}

void TESWaterListener::entityRemovedCallback(hkpEntity* apEntity) {
    bhkEntity* a2 = bhkEntity::Getbhk(apEntity);
    bhkEntityListener::RemoveEntity(a2);
}

void TESWaterListener::Update(float afTime) {
    if (bEnabled) {
        hkVector4 positionPlayer;
        fSplashTimer = fSplashTimer - afTime;
        float fdistanceLODSquared = fSubmergedLODDistance.Float();
        fdistanceLODSquared = fdistanceLODSquared * fdistanceLODSquared;
        fdistanceLODSquared = NI2HK(fdistanceLODSquared);
        float fdistanceWater = NI2HK(fSubmergedMaxWaterDistance.Float());
        NI2HK(positionPlayer, pPlayer->GetLocationOnReference());
        BSSimpleList<NiPointer<bhkEntity>>* plist = &Entities;
        float fwaterHeight = 0.0f;
        TESWaterListener::RigidBodyUnderwaterMap.RemoveAll();
        while (!bAutoWaterUsesHavokShapes.Bool() && plist && !plist->IsEmpty()) {
            bhkEntity* pentity = plist->m_item.m_pObject;
            hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pentity->Gethk());
            int32_t iresult = 0;
            if (ShouldProcessRigidBody(phkBody, positionPlayer, fdistanceLODSquared, nullptr, fwaterHeight)) {
                iresult = ProcessRigidBody(phkBody, fwaterHeight, afTime, false);
            }
            if (iresult) {
                TESObjectREFR* ptarget = TESHavokUtilities::FindCollidableRef(pentity->GetCollidable());
                if (ptarget) {
                    fwaterHeight = HK2NI(fwaterHeight);
                    if (iresult == 1) {
                        if (!ptarget->QUnderwater()) {
                            if (AILinearTaskThreadManager::ShouldQueue3DTask() && TaskQueueInterface::GetInstance()) {
                                TaskQueueInterface::GetInstance()->QueueWaterObjectForAdd(
                                    nullptr, ptarget, fwaterHeight
                                );
                            }
                            else {
                                TES::pInstance->GetWaterSystem()->AddTESObjectToWaterGroup(
                                    nullptr, ptarget, fwaterHeight
                                );
                            }
                        }
                        ptarget->SetUnderwater(true);
                    }
                    else if (iresult == 2) {
                        ptarget->SetUnderwater(false);
                        if (!ptarget->QUnderwater()) {
                            if (AILinearTaskThreadManager::ShouldQueue3DTask() && TaskQueueInterface::GetInstance()) {
                                TaskQueueInterface::GetInstance()->QueueWaterObjectForRemove(
                                    nullptr, ptarget, fwaterHeight
                                );
                            }
                            else {
                                TES::pInstance->GetWaterSystem()->RemoveTESObjectFromWaterGroup(
                                    nullptr, ptarget, fwaterHeight
                                );
                            }
                        }
                    }
                }
            }
            plist = plist->m_pkNext;
        }
        WaterZone* pzone = static_cast<WaterZone*>(pZones);
        while (pzone) {
            WaterTargetEntry* ptarg = static_cast<WaterTargetEntry*>(pzone->pTargets);
            while (ptarg) {
                bool bprocessed = false;
                for (uint32_t i = 0; i < ptarg->CollidableList.iSize; ++i) {
                    hkpRigidBody* pbody = hkpGetRigidBody(ptarg->CollidableList[i]);
                    int32_t iresult = 0;
                    if (!TESWaterListener::RigidBodyUnderwaterMap.GetAt(pbody, bprocessed) &&
                        ShouldProcessRigidBody(pbody, positionPlayer, fdistanceLODSquared, pzone, fwaterHeight)) {
                        iresult = ProcessRigidBody(pbody, fwaterHeight, afTime, true);
                    }
                    if (iresult == 1) {
                        ptarg->pRef->SetUnderwater(true);
                        ptarg->pRef->SetInZone(pzone, true);
                    }
                }
                // EXE-INLINED 0x61FEF0
                bhkTTNode<TESObjectREFR>* pnextTarget = ptarg->pNext;
                ptarg = pnextTarget ? static_cast<WaterTargetEntry*>(pnextTarget) : nullptr;
            }
            // EXE-INLINED 0x620180
            bhkTTNode<TESObjectREFR>* pnextZone = pzone->pNext;
            pzone = pnextZone ? static_cast<WaterZone*>(pnextZone) : nullptr;
        }
        HandleQueuedRemovals(nullptr, true, false);
    }
    if (Main::WorldRootCamera()) {
        if (pPlayer->GetParentCell()) {
            const NiPoint3& rcameraPos = Main::WorldRootCamera()->GetWorldTranslate();
            if (rcameraPos.z < pPlayer->GetParentCell()->GetWaterHeight()) {
                TESWaterSystem::SetCurrentWaterType(pPlayer->GetParentCell()->GetWaterType());
            }
        }
    }
}

bool TESWaterListener::contactPointAddedCallback(hkpRootCdPoint& CPoint) {
    PlayerCharacter* apActor =
        static_cast<PlayerCharacter*>(TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableB));
    if (apActor) {
        TESObjectREFR* arg0a = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        if (arg0a) {
            TESBoundObject* ObjectReference = arg0a->GetObjectReference();
            BGSPlaceableWater* v15 = dynamic_cast<BGSPlaceableWater*>(ObjectReference);
            if (bAutoWaterUsesHavokShapes.Bool() || !v15->IsAutomaticWater()) {
                if (apActor == pPlayer) {
                    TESWaterForm* v8 = v15->GetWaterType();
                    TESWaterSystem::SetCurrentWaterType(v8);
                }
                WaterZone* apZone = static_cast<WaterZone*>(AddZoneTargetPair(CPoint, arg0a, apActor, 0, 0));
                apActor->SetInZone(apZone, 1);
                if (AILinearTaskThreadManager::ShouldQueue3DTask() && TaskQueueInterface::GetInstance()) {
                    TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                    Instance->QueueWaterObjectForAdd(arg0a, apActor, 0.0);
                }
                else {
                    TESWaterSystem* WaterSystem = TES::pInstance->GetWaterSystem();
                    WaterSystem->AddTESObjectToWaterGroup(arg0a, apActor, 0.0);
                }
                HandleQueuedRemovals(apActor, 0, 1);
                return 1;
            }
            else {
                return 0;
            }
        }
        else {
            return 0;
        }
    }
    else {
        bhkWorldObject* CameraRigidBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
        if (CPoint.m_rootCollidableB == CameraRigidBody->GetCollidable()) {
            TESObjectREFR* CollidableRef_0 = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
            if (CollidableRef_0) {
                TESBoundObject* v4 = CollidableRef_0->GetObjectReference();
                BGSPlaceableWater* pPlaceableWater = dynamic_cast<BGSPlaceableWater*>(v4);
                if (bAutoWaterUsesHavokShapes.Bool() || !pPlaceableWater->IsAutomaticWater()) {
                    const NiPoint3* v5 = &CollidableRef_0->GetLocationOnReference();
                    TESWaterSystem::SetCameraRelevantWaterHeight(v5->z);
                    TESWaterSystem::SetPlayerForceUnderwater(1);
                    TESWaterSystem::SetCameraInPlaceableWater(1);
                    TESWaterForm* v6 = pPlaceableWater->GetWaterType();
                    TESWaterSystem::SetCurrentWaterType(v6);
                    return 1;
                }
                else {
                    return 0;
                }
            }
            else {
                return 0;
            }
        }
        else {
            return 0;
        }
    }
}

bool TESWaterListener::contactPointRemovedCallback(hkpRootCdPoint& CPoint) {
    QueuedRemoval queuedRemoval;
    TESObjectREFR* ptarget = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableB);
    TESObjectREFR* pwaterRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
    if (!pwaterRef) {
        return false;
    }
    BGSPlaceableWater* pwater = dynamic_cast<BGSPlaceableWater*>(pwaterRef->GetObjectReference());
    if (!bAutoWaterUsesHavokShapes.Bool() && pwater->IsAutomaticWater()) {
        return false;
    }
    ZoneEntry* pzone;
    if (pZones) {
        pzone = pZones->GetZoneEntry(CPoint.m_rootCollidableA);
    }
    else {
        pzone = nullptr;
    }
    WaterZone* pwaterZone = static_cast<WaterZone*>(pzone);
    if (ptarget || !pPlayer->GetParentCell()) {
        if (!ptarget) {
            return false;
        }
        if (ptarget->GetDelete()) {
            return false;
        }
        bhkRigidBody* prb = nullptr;
        NiCollisionObject* pnco = ptarget->Get3D()->GetCollisionObject();
        if (pnco) {
            bhkNiCollisionObject* pbnco = pnco->IsBhkNiCollisionObject();
            if (pbnco) {
                bhkWorldObject* pbwo = pbnco->GetWorldObject();
                if (pbwo) {
                    prb = pbwo->IsBhkRigidBody();
                }
            }
        }
        if (prb) {
            NiTListItem<QueuedRemoval*>* ppos = QueuedRemovals.m_pkHead;
            bool bfound = false;
            while (ppos) {
                QueuedRemoval* pqr = QueuedRemovals.Get(ppos);
                ppos = QueuedRemovals.GetNextPos(ppos);
                if (pqr->pRB == prb) {
                    bfound = true;
                    break;
                }
            }
            if (!bfound) {
                QueuedRemoval* pqueuedRemoval = static_cast<QueuedRemoval*>(ourMalloc(sizeof(QueuedRemoval)));
                if (pqueuedRemoval) {
                    std::memset(pqueuedRemoval, 0, sizeof(QueuedRemoval));
                }
                else {
                    pqueuedRemoval = nullptr;
                }
                pqueuedRemoval->pTarget = ptarget;
                pqueuedRemoval->pRB = prb;
                pqueuedRemoval->pWaterRef = pwaterRef;
                pqueuedRemoval->pWZone = pwaterZone;
                pqueuedRemoval->pCollidable = CPoint.m_rootCollidableB;
                QueuedRemovals.AddHead(pqueuedRemoval);
            }
        }
        else {
            queuedRemoval.pTarget = ptarget;
            queuedRemoval.pRB = nullptr;
            queuedRemoval.pWaterRef = pwaterRef;
            queuedRemoval.pWZone = pwaterZone;
            queuedRemoval.pCollidable = CPoint.m_rootCollidableB;
            RemoveReferenceFromZone(&queuedRemoval);
        }
        return true;
    }
    bhkWorldObject* pcameraBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
    if (CPoint.m_rootCollidableB != pcameraBody->GetCollidable()) {
        return false;
    }
    TESWaterSystem::SetPlayerForceUnderwater(false);
    TESWaterSystem::SetCameraRelevantWaterHeight(pPlayer->GetParentCell()->GetWaterHeight());
    TESWaterSystem::SetCameraInPlaceableWater(false);
    TESWaterForm* pwaterType;
    WaterZone* pplayerZone = pPlayer->GetExtra()->GetFirstWaterZone();
    if (pplayerZone) {
        BGSPlaceableWater* pzoneWater = static_cast<BGSPlaceableWater*>(pplayerZone->pRef->GetObjectReference());
        pwaterType = pzoneWater->GetWaterType();
    }
    else {
        pwaterType = pPlayer->GetParentCell()->GetWaterType();
    }
    TESWaterSystem::SetCurrentWaterType(pwaterType);
    return true;
}

bhkRigidBody* TESWaterListener::GetRB(hkpEntity* apEntity) {
    CFilter a2;
    bhkEntity* pe = bhkEntity::Getbhk(apEntity);
    bhkRigidBody* pb = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pe));
    if (pb->GetMotionType() == 5) {
        return 0;
    }
    if (!pb) {
        return 0;
    }
    CFilter* CollisionFilter = pb->GetCollisionFilter(&a2);
    switch (CollisionFilter->GetLayer()) {
        case L_CLUTTER:
        case L_WEAPON:
        case L_PROJECTILE:
        case L_BIPED:
        case L_PROPS:
        case L_TRIGGER:
        case L_TERRAIN:
        case L_TRAP:
        case L_DEADBIP: {
            break;
        }
        default: {
            return 0;
        }
    }
    return pb;
}

float TESWaterListener::GetRandom(float afSize) {
    float a1 = -afSize;
    return RandomFloat(a1, afSize);
}

// A's hkpRigidBody itself, not its bhkEntity, is put in the entity list through a temporary NiPointer, as the game
// does: hkpGetRigidBody returns hkpRigidBody*, and the original source cast that result straight to
// NiPointer<bhkEntity> between the unrelated types.
bool TESWaterListener::collidableEnterCallback(const hkpCollidable* collidableA, const hkpCollidable* collidableB) {
    bool bres = false;
    NiPointer<bhkEntity> spentity = reinterpret_cast<bhkEntity*>(hkpGetRigidBody(collidableA));
    Entities.AddHead(spentity);
    return bres;
}

// As collidableEnterCallback, the list item is A's hkpRigidBody.
bool TESWaterListener::collidableLeaveCallback(const hkpCollidable* collidableA, const hkpCollidable* collidableB) {
    NiPointer<bhkEntity> spentity = reinterpret_cast<bhkEntity*>(hkpGetRigidBody(collidableA));
    Entities.Remove(spentity);
    return true;
}

ZoneEntry* TESWaterListener::CreateZoneEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) {
    WaterZone* v5;
    WaterZone* apMemory = static_cast<WaterZone*>(ourMalloc(sizeof(WaterZone)));
    if (apMemory) {
        v5 = ::new (apMemory) WaterZone(apRef, apCol);
    }
    else {
        v5 = 0;
    }
    TESWaterSystem* WaterSystem = TES::pInstance->GetWaterSystem();
    v5->pGroup = WaterSystem->FindWaterGroup(apRef, 0, 0.0);
    return v5;
}

// A queued removal whose reference has lost its 3D always goes. Otherwise it goes when it matches apSpecificReference
// (or that is null), or with abCheckBound when the body's bounds lie wholly above the water (it is kept on equal or
// NaN); abCancel drops it without leaving the zone.
void TESWaterListener::HandleQueuedRemovals(TESObjectREFR* apSpecificReference, bool abCheckBound, bool abCancel) {
    NiTListItem<QueuedRemoval*>* pPos = QueuedRemovals.GetHeadPos();
    NiTListItem<QueuedRemoval*>* pNext = nullptr;
    while (pPos) {
        pNext = QueuedRemovals.GetNextPos(pPos);
        if (pPos) {
            QueuedRemoval* pRemoval = QueuedRemovals.Get(pPos);
            if (!pRemoval->pTarget->Get3D()) {
                RemoveReferenceFromZone(pRemoval);
                QueuedRemovals.RemovePos(pPos);
                ourFree(pRemoval);
            }
            else {
                bool bRemove = !apSpecificReference || pRemoval->pTarget == apSpecificReference ||
                    pRemoval->pWaterRef == apSpecificReference;
                if (abCheckBound) {
                    alignas(16) hkAabb kBound;
                    pRemoval->pRB->GetAabb(kBound);
                    float* pMinZ = &kBound.m_min(2);
                    float fWaterZ = NI2HK(pRemoval->pWaterRef->GetLocationOnReference().z);
                    bRemove = *pMinZ > fWaterZ;
                }
                if (bRemove) {
                    if (!abCancel) {
                        RemoveReferenceFromZone(pRemoval);
                    }
                    QueuedRemovals.RemovePos(pPos);
                    ourFree(pRemoval);
                }
            }
        }
        pPos = pNext;
    }
}

// Tests the player against a flag the function sets false and never changes, so the water type update below never runs;
// the flag is kept deliberately, as the game does.
void TESWaterListener::RemoveReferenceFromZone(QueuedRemoval* apQR) {
    bool bupdatePlayerWater = false;
    PlaceableWaterGroup* pgroup;
    if (apQR->pWZone) {
        pgroup = apQR->pWZone->pGroup;
    }
    else {
        pgroup = nullptr;
    }
    if (apQR->pTarget == pPlayer && bupdatePlayerWater && pPlayer->GetParentCell()) {
        TESWaterSystem::SetCurrentWaterType(pPlayer->GetParentCell()->GetWaterType());
    }
    if (apQR->pWZone) {
        apQR->pWZone->RemoveCollidable(apQR->pTarget, apQR->pCollidable);
    }
    if (apQR->pRB) {
        PlaySplashEffects(apQR->pRB, apQR->pWaterRef->GetLocationOnReference().z);
        apQR->pRB->SetFlags(0x80, false);
    }
    if (apQR->pWZone) {
        apQR->pTarget->SetInZone(apQR->pWZone, false);
    }
    apQR->pTarget->SetUnderwater(false);
    bool bremove = true;
    if (!apQR->pTarget->GetExtra()->GetFirstWaterZone()) {
        NiTMap<WaterZone*, int32_t>* pmap = apQR->pTarget->GetExtra()->QWaterZoneMap();
        if (pmap) {
            NiTMapItem<WaterZone*, int32_t>* ppos = pmap->GetFirstPos();
            while (ppos) {
                int32_t icount = 0;
                WaterZone* ptestzone = nullptr;
                pmap->GetNext(ppos, ptestzone, icount);
                if (pgroup && ptestzone->pGroup == pgroup) {
                    bremove = false;
                    break;
                }
            }
        }
    }
    if (bremove) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask() && TaskQueueInterface::GetInstance()) {
            TaskQueueInterface::GetInstance()->QueueWaterObjectForRemove(apQR->pWaterRef, apQR->pTarget, 0.0f);
        }
        else {
            TES::pInstance->GetWaterSystem()->RemoveTESObjectFromWaterGroup(apQR->pWaterRef, apQR->pTarget, 0.0f);
        }
    }
}

// Stores the cell's water height, converted to Havok units.
void TESWaterListener::SetWaterHeight(float afWaterHeight, uint32_t auX, uint32_t auY) {
    uint32_t uiIndex = auX + uiCellArrayWidth * auY;
    pfWaterLevels[uiIndex] = NI2HK(afWaterHeight);
}

double TESWaterListener::GetWaterHeightForCoord(int32_t aiX, int32_t aiY) {
    int32_t v3;
    uint32_t v7 = (uint32_t)(aiX + (((int32_t)uiCellArrayWidth) * aiY));
    if (v7 >= ((uint32_t)(((int32_t)uiCellArrayWidth) * ((int32_t)uiCellArrayWidth)))) {
        return (float)(-0x1.fffffe0000000p+127f);
    }
    else {
        v3 = aiX + (((int32_t)uiCellArrayWidth) * aiY);
        return pfWaterLevels[v7];
    }
}

// The Havok locals are alignas(16). The arithmetic runs at double precision, so the mixed float/double expressions are
// written in double and rounded to float where the result is stored as a float. The exponential used is exp(float), not
// log.
int32_t
TESWaterListener::ProcessRigidBody(hkpRigidBody* phkBody, float fwaterHeight, float afTime, bool abPlaceableWater) {
    int32_t iResult = 0;
    alignas(16) hkVector4 kPosition;
    FNVD_RECONSTRUCT(hkVector4)(&kPosition, phkBody->getTranslation());
    bhkRigidBody* pBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(phkBody));
    alignas(16) hkAabb kAabb;
    pBody->GetAabb(kAabb);
    // Out of the water when the surface is below the bounds' bottom, or when the compare is unordered.
    float& rBottom = kAabb.m_min(2);
    if (!(fwaterHeight >= rBottom)) {
        uint32_t uiFlag = abPlaceableWater ? 0x80 : 8;
        if (pBody->IsFlagSet(uiFlag)) {
            PlaySplashEffects(pBody, HK2NI(fwaterHeight));
            iResult = 2;
        }
        pBody->SetFlags(uiFlag, false);
        return iResult;
    }
    hkpMotion::MotionType eMotionType = phkBody->getMotionType();
    if (bhkRigidBody::IsMotionTypeDynamic(eMotionType)) {
        ahkpWorld* pWorld = static_cast<ahkpWorld*>(phkBody->getWorld());
        if (pWorld) {
            pWorld->markForRead();
            alignas(16) hkVector4 kGravity;
            FNVD_RECONSTRUCT(hkVector4)(&kGravity, pWorld->getGravity());
            alignas(16) hkVector4 kVelocity;
            FNVD_RECONSTRUCT(hkVector4)(&kVelocity, phkBody->getLinearVelocity());
            float fMass = phkBody->getMass();
            float& rTop = kAabb.m_max(2);
            float& rMin = kAabb.m_min(2);
            float fHeight = rTop - rMin;
            double dQuarter = fHeight * 0.25;
            float fLow = static_cast<float>(kAabb.m_min(2) + dQuarter);
            double dHigh = fHeight * 0.8500000238418579;
            float fHigh = static_cast<float>(kAabb.m_min(2) + dHigh);
            float fDepth =
                static_cast<float>((static_cast<double>(fwaterHeight) - fLow) / (static_cast<double>(fHigh) - fLow));
            if (fDepth <= 0.0) {
                pWorld->unmarkForRead();
                return iResult;
            }
            float fBuoyancy = TESHavokUtilities::GetTotalBuoyancy(pBody);
            uint8_t bDampVertical = 1;
            if (f1_2 > fDepth && fBuoyancy > 0.0) {
                if (f0_75 < fDepth) {
                    fBuoyancy = f0_3;
                }
                else if (f0_5 < fDepth) {
                    fBuoyancy = fSubmergedMidBuoyancy;
                }
                else if (fSubmergedLowDepth < fDepth) {
                    fBuoyancy = fminus0_3;
                }
                if (f2_0 > std::fabs(static_cast<float>(kVelocity.getSimdAt(2)))) {
                    bDampVertical = 0;
                }
            }
            fBuoyancy = -fBuoyancy;
            alignas(16) hkVector4 kForce;
            kForce.~hkVector4();
            ::new (&kForce) hkVector4;
            float fForceScale = fBuoyancy * fMass;
            alignas(16) hkSimdReal kForceScale;
            kForceScale.~hkSimdReal();
            ::new (&kForceScale) hkSimdReal(fForceScale);
            kForce.setMul4(kForceScale, kGravity);
            alignas(16) hkVector4 kPoint;
            kPoint.~hkVector4();
            ::new (&kPoint) hkVector4;
            if (phkBody->getNumConstraints()) {
                kPoint = phkBody->getCenterOfMassInWorld();
            }
            else {
                alignas(16) hkAabb kLocalAabb;
                pBody->GetAabbLocal(kLocalAabb);
                alignas(16) hkVector4 kOffset(0.0f, 0.0f, 0.0f, 0.0f);
                alignas(16) hkVector4 kExtent;
                kExtent.~hkVector4();
                ::new (&kExtent) hkVector4;
                kExtent.setSub4(kLocalAabb.m_max, kLocalAabb.m_min);
                // The longest axis: the compare is strictly greater, and a NaN never counts as greater.
                int32_t iLongest;
                float& rX0 = kExtent(0);
                float& rY0 = kExtent(1);
                if (rY0 > rX0) {
                    float& rY1 = kExtent(1);
                    float& rZ1 = kExtent(2);
                    iLongest = rZ1 > rY1 ? 2 : 1;
                }
                else {
                    float& rX1 = kExtent(0);
                    float& rZ1 = kExtent(2);
                    iLongest = rZ1 > rX1 ? 2 : 0;
                }
                // The shortest axis (strictly less; a NaN compares as not less).
                int32_t iShortest;
                float& rX2 = kExtent(0);
                float& rY2 = kExtent(1);
                if (rY2 < rX2) {
                    float& rY3 = kExtent(1);
                    float& rZ3 = kExtent(2);
                    iShortest = rZ3 < rY3 ? 2 : 1;
                }
                else {
                    float& rX3 = kExtent(0);
                    float& rZ3 = kExtent(2);
                    iShortest = rZ3 < rX3 ? 2 : 0;
                }
                float& rLongest = kExtent(iLongest);
                float& rShortest = kExtent(iShortest);
                if (rLongest > rShortest * 1.5) {
                    float& rLong = kExtent(iLongest);
                    float& rShort = kExtent(iShortest);
                    double dRatio = static_cast<double>(rLong) / rShort * 0.10000000149011612;
                    double dOffset = NiSign(fBuoyancy) * dRatio;
                    kOffset(iShortest) = static_cast<float>(dOffset);
                    alignas(16) hkQuaternion kRotation;
                    kRotation.~hkQuaternion();
                    ::new (&kRotation) hkQuaternion;
                    pBody->GetRotation(kRotation);
                    kPoint.setRotatedDir(kRotation, kOffset);
                    kPoint.add4(phkBody->getCenterOfMassInWorld());
                }
                else {
                    kPoint = phkBody->getCenterOfMassInWorld();
                }
            }
            pWorld->unmarkForRead();
            phkBody->applyForce(afTime, kForce, kPoint);
            alignas(16) hkVector4 kGravityForce;
            kGravityForce.~hkVector4();
            ::new (&kGravityForce) hkVector4;
            float fNegMass = static_cast<float>(fMass * -1.0);
            alignas(16) hkSimdReal kNegMass;
            kNegMass.~hkSimdReal();
            ::new (&kNegMass) hkSimdReal(fNegMass);
            kGravityForce.setMul4(kNegMass, kGravity);
            phkBody->applyForce(afTime, kGravityForce);
            pWorld->markForWrite();
            alignas(16) hkVector4 kLinear;
            FNVD_RECONSTRUCT(hkVector4)(&kLinear, phkBody->getLinearVelocity());
            float fVertical = kLinear(2);
            kLinear(2) = 0.0f;
            if (bDampVertical) {
                float fDampV = static_cast<float>(fSubmergedLinearDampingV.Float() * -1.0 * afTime);
                fVertical = std::exp(fDampV) * fVertical;
            }
            double dSpeed = static_cast<float>(kLinear.length3());
            if (fSubmergedMaxSpeed.Float() < dSpeed) {
                float fDampH = static_cast<float>(fSubmergedLinearDampingH.Float() * -1.0 * afTime);
                alignas(16) hkSimdReal kDampH;
                FNVD_RECONSTRUCT(hkSimdReal)(&kDampH, std::exp(fDampH));
                kLinear.mul4(kDampH);
            }
            kLinear(2) = fVertical;
            phkBody->setLinearVelocity(kLinear);
            alignas(16) hkVector4 kAngular;
            FNVD_RECONSTRUCT(hkVector4)(&kAngular, phkBody->getAngularVelocity());
            float fDampA = static_cast<float>(fSubmergedAngularDamping.Float() * -1.0 * afTime);
            alignas(16) hkSimdReal kDampA;
            FNVD_RECONSTRUCT(hkSimdReal)(&kDampA, std::exp(fDampA));
            kAngular.mul4(kDampA);
            phkBody->setAngularVelocity(kAngular);
            pWorld->unmarkForWrite();
        }
    }
    uint32_t uiFlag = abPlaceableWater ? 0x80 : 8;
    if (!pBody->IsFlagSet(uiFlag)) {
        pBody->SetFlags(uiFlag, true);
        PlaySplashEffects(pBody, HK2NI(fwaterHeight));
        iResult = 1;
    }
    RigidBodyUnderwaterMap.SetAt(phkBody, true);
    return iResult;
}

void TESWaterListener::PlaySplashEffects(bhkRigidBody* apBody, float afWaterHeight) {
    CFilter a2;
    NiPoint3 velocity;
    if (apBody) {
        if (!(fSplashTimer > 0.0)) {
            CFilter* CollisionFilter = apBody->GetCollisionFilter(&a2);
            if (CollisionFilter->GetLayer() != L_BIPED) {
                float fmass = apBody->GetMass();
                apBody->GetLinearVelocityNi(&velocity);
                float fverticalSpeed = NiAbs(velocity.z);
                if (!(fverticalSpeed < 0.001)) {
                    NiPoint3 location;
                    float* v4 = &fSplashSoundVelocityMult.Float();
                    float fsize = *v4 * fverticalSpeed + fmass;
                    if (velocity.z > 0.0) {
                        float* v5 = &fSplashSoundOutMult.Float();
                        fsize = fsize * *v5;
                    }
                    apBody->GetPosition(location);
                    float flandHeight = afWaterHeight;
                    if (TES::pInstance->pInteriorCell ||
                        (!TES::pInstance->GetLandHeight(location, flandHeight) ||
                         !(afWaterHeight < static_cast<double>(flandHeight)))) {
                        location.z = afWaterHeight;
                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                            TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                            Instance->QueueWaterSplashEffects(fsize, location);
                        }
                        else {
                            BGSWaterCollisionManager::PlaySplashEffects(fsize, location);
                        }
                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                            TaskQueueInterface* v7 = TaskQueueInterface::GetInstance();
                            v7->QueueAddRipple(0.01f, location);
                        }
                        else {
                            TES::pInstance->GetWaterSystem()->AddRipple(location, 0.01f);
                        }
                        fSplashTimer = fSplashSoundTimer.Float();
                    }
                }
            }
        }
    }
}

// The flags tested, 0x88, have no names.
bool TESWaterListener::ShouldProcessRigidBody(
    hkpRigidBody* phkBody,
    hkVector4& positionPlayer,
    float fdistanceLODSquared,
    WaterZone* apZone,
    float& fwaterHeight
) {
    if (!phkBody) {
        return false;
    }
    bhkRigidBody* pb = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(phkBody));
    if (!pb) {
        return false;
    }
    hkVector4 position(phkBody->getTranslation());
    hkVector4 playerDistance;
    playerDistance.setSub4(position, positionPlayer);
    if (fdistanceLODSquared < playerDistance.lengthSquared3().operator float()) {
        if (pb->IsFlagSet(0x88)) {
            if (phkBody->isActive()) {
                pb->Deactivate();
            }
        }
        return false;
    }
    if (apZone) {
        fwaterHeight = NI2HK(apZone->fWaterHeight);
    }
    else if (uiCellArrayWidth <= 1) {
        fwaterHeight = GetWaterHeightForCoord(0, 0);
    }
    else {
        alignas(16) NiPoint3 positionNI;
        HK2NI(positionNI, position);
        int32_t ix = FloatToInt(positionNI.x) >> 12;
        int32_t iy = FloatToInt(positionNI.y) >> 12;
        ix -= iBaseCellNumberX;
        iy -= iBaseCellNumberY;
        fwaterHeight = GetWaterHeightForCoord(ix, iy);
    }
    float fdistanceWater = NI2HK(fSubmergedMaxWaterDistance.Float());
    double fsurface = fwaterHeight + fdistanceWater;
    if (!(position.getSimdAt(2).operator float() > fsurface)) {
        return true;
    }
    return pb->IsFlagSet(0x88);
}

void TESWaterListener::RemoveWaterZoneForReference(TESObjectREFR* apWaterRef) {
    WaterZone* pzone = TESWaterListener::FindWater(apWaterRef);
    if (pzone) {
        for (TargetEntry* pentry = pzone->pTargets; pentry;
             pentry = static_cast<WaterTargetEntry*>(pentry)->WaterTargetEntry::GetNextEntry()) {
            TESObjectREFR* pref = pentry->pRef;
            if (pref) {
                while ((&pref->m_Extra)->GetInWaterZone(pzone)) {
                    (&pref->m_Extra)->SetInWaterZone(pzone, 0);
                }
            }
        }
        RemoveZoneEntry(pzone);
    }
}

TESWaterListener* TESWaterListener::GetExteriorListener() {
    TESWaterListener* v1;
    void* v2;
    if (!g_TESWaterListener) {
        v2 = ourMalloc(sizeof(TESWaterListener));
        if (v2) {
            v1 = ::new (v2) TESWaterListener;
        }
        else {
            v1 = nullptr;
        }
        g_TESWaterListener = v1;
    }
    return g_TESWaterListener;
}

void TESWaterListener::ClearExteriorListener() {
    g_TESWaterListener = nullptr;
}

// A cell without water reads as the lowest float. The grid width is compared signed. With no placeable water
// either, a listener that found no water anywhere is disabled.
void TESWaterListener::RefreshExteriorWaterHeights() {
    TESWaterListener* pListener = GetExteriorListener();
    pListener->fHighestWaterLevel = -3.4028235e+38f;
    int32_t iWidthX = static_cast<int32_t>(pListener->uiCellArrayWidth);
    int32_t iWidthY = static_cast<int32_t>(pListener->uiCellArrayWidth);
    for (int32_t x = 0; x < iWidthX; ++x) {
        for (int32_t y = 0; y < iWidthY; ++y) {
            float fHeight = -3.4028235e+38f;
            TESObjectCELL* pCell = TES::pInstance->GetGridCell(x, y)->pCell;
            if (pCell && pCell->HasWater()) {
                fHeight = pCell->GetWaterHeight();
            }
            pListener->SetWaterHeight(fHeight, x, y);
            if (fHeight > pListener->fHighestWaterLevel) {
                pListener->fHighestWaterLevel = fHeight;
            }
        }
    }
    TESObjectCELL* pBaseCell = TES::pInstance->GetGridCell(0, 0)->pCell;
    if (pBaseCell) {
        pListener->iBaseCellNumberX = pBaseCell->GetDataX();
        pListener->iBaseCellNumberY = pBaseCell->GetDataY();
    }
    // Compares the widened float against the double constant -3.4028234663852886e+38.
    if (!TES::pInstance->QPlaceableWaterCount() &&
        static_cast<double>(pListener->fHighestWaterLevel) == -3.4028234663852886e+38) {
        pListener->SetEnabled(false);
    }
}

WaterZone* TESWaterListener::FindWater(TESObjectREFR* apWater) {
    ZoneEntry* v3 = nullptr;
    TESWaterListener* waterListener = BGSPlaceableWater::GetWaterListener(apWater);
    if (waterListener) {
        return static_cast<WaterZone*>(static_cast<BGSZoneTargetListener*>(waterListener)->GetZoneEntry(apWater));
    }
    return static_cast<WaterZone*>(v3);
}

bool TESWaterListener::GetUnderwater() {
    return TESWaterSystem::bForceUnderwater;
}

void TESWaterListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
