#include "Fallout Shared/Havok/BGSAcousticSpaceListener.h"

#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

BGSAcousticSpaceListener::BGSAcousticSpaceListener()
  : bhkEntityListener(0), bBufferRemovalOperations(true), RemovalQueueMap(37u) {}

BGSAcousticSpaceListener::~BGSAcousticSpaceListener() {}

void BGSAcousticSpaceListener::entityRemovedCallback(hkpEntity* apEntity) {
    if (pPlayer) {
        if (pPlayer->Is3rdPerson()) {
            const hkpCollidable* pCollidable = apEntity->getCollidable();
            bhkWorldObject* pCameraBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
            if (pCollidable == pCameraBody->GetCollidable()) {
                FalloutAudio::ClearAcousticSpace(nullptr);
            }
        }
        else if (TESHavokUtilities::FindCollidableRef(apEntity->getCollidable()) == pPlayer) {
            FalloutAudio::ClearAcousticSpace(nullptr);
        }
    }
}

// Every queued removal counts down by one; then each that reached zero is dropped and its acoustic space cleared,
// the walk starting over after each drop.
void BGSAcousticSpaceListener::Update(float afTime) {
    NiTMapItem<TESObjectREFR*, int32_t>* iter = RemovalQueueMap.GetFirstPos();
    while (iter) {
        TESObjectREFR* pRef;
        int32_t iCount;
        RemovalQueueMap.GetNext(iter, pRef, iCount);
        --iCount;
        RemovalQueueMap.SetAt(pRef, iCount);
    }
    bool bLoop = true;
    while (bLoop) {
        bLoop = false;
        iter = RemovalQueueMap.GetFirstPos();
        while (iter) {
            TESObjectREFR* pRef;
            int32_t iCount;
            RemovalQueueMap.GetNext(iter, pRef, iCount);
            if (!iCount) {
                RemovalQueueMap.RemoveAt(pRef);
                FalloutAudio::ClearAcousticSpace(static_cast<BGSAcousticSpace*>(pRef->GetObjectReference()));
                bLoop = true;
                break;
            }
        }
    }
}

bool BGSAcousticSpaceListener::contactPointAddedCallback(hkpRootCdPoint& CPoint) {
    TESObjectREFR* pRef = nullptr;
    if (pPlayer->Is3rdPerson()) {
        bhkWorldObject* pCameraBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
        if (CPoint.m_rootCollidableB == pCameraBody->GetCollidable()) {
            pRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        }
    }
    else if (TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableB) == pPlayer) {
        pRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
    }
    if (!pRef) {
        return false;
    }
    if (!bBufferRemovalOperations || !DequeueRemoval(pRef, false)) {
        BGSAcousticSpace* pAcousticSpace = static_cast<BGSAcousticSpace*>(pRef->GetObjectReference());
        FalloutAudio::SetAcousticSpace(pAcousticSpace);
    }
    return true;
}

bool BGSAcousticSpaceListener::contactPointRemovedCallback(hkpRootCdPoint& CPoint) {
    TESObjectREFR* pRef = nullptr;
    if (pPlayer->Is3rdPerson()) {
        bhkWorldObject* pCameraBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
        if (CPoint.m_rootCollidableB == pCameraBody->GetCollidable()) {
            pRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        }
    }
    else if (TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableB) == pPlayer) {
        pRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
    }
    if (pRef) {
        if (bBufferRemovalOperations) {
            QueueRemoval(pRef);
        }
        else {
            BGSAcousticSpace* pAcousticSpace = static_cast<BGSAcousticSpace*>(pRef->GetObjectReference());
            FalloutAudio::ClearAcousticSpace(pAcousticSpace);
        }
    }
    return false;
}

bool BGSAcousticSpaceListener::collidableEnterCallback(hkpCollidable* collidableA, hkpCollidable* collidableB) {
    return false;
}

bool BGSAcousticSpaceListener::collidableLeaveCallback(hkpCollidable* collidableA, hkpCollidable* collidableB) {
    return false;
}

void BGSAcousticSpaceListener::QueueRemoval(TESObjectREFR* apRef) {
    RemovalQueueMap.SetAt(apRef, 5);
}

// The removal is dropped from the queue; when it was queued and abDoRemoval, the space is cleared now.
bool BGSAcousticSpaceListener::DequeueRemoval(TESObjectREFR* apRef, bool abDoRemoval) {
    bool bRemoved = RemovalQueueMap.RemoveAt(apRef);
    if (bRemoved && abDoRemoval) {
        FalloutAudio::ClearAcousticSpace(static_cast<BGSAcousticSpace*>(apRef->GetObjectReference()));
    }
    return bRemoved;
}

void BGSAcousticSpaceListener::DequeueAllRemovals(bool abDoRemoval) {
    int32_t iCount;
    TESObjectREFR* pRef;
    NiTMapItem<TESObjectREFR*, int32_t>* iter = RemovalQueueMap.GetFirstPos();
    while (iter) {
        RemovalQueueMap.GetNext(iter, pRef, iCount);
        BGSAcousticSpace* pAcousticSpace = static_cast<BGSAcousticSpace*>(pRef->TESObjectREFR::GetObjectReference());
        FalloutAudio::ClearAcousticSpace(pAcousticSpace);
    }
    RemovalQueueMap.RemoveAll();
}

void BGSAcousticSpaceListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
