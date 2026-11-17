#include "Fallout Shared/Havok/BGSAcousticSpaceListener.h"

#include "BSCore/MemoryManager.h"
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
            const hkpCollidable* Collidable = apEntity->getCollidable();
            bhkWorldObject* CameraRigidBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
            if (Collidable == CameraRigidBody->GetCollidable()) {
                FalloutAudio::ClearAcousticSpace(nullptr);
            }
        }
        else {
            if (TESHavokUtilities::FindCollidableRef(apEntity->getCollidable()) == pPlayer) {
                FalloutAudio::ClearAcousticSpace(nullptr);
            }
        }
    }
}

// Every queued removal counts down by one; then each that reached zero is dropped and its acoustic space cleared,
// the walk starting over after each drop.
void BGSAcousticSpaceListener::Update(float afTime) {
    NiTMapItem<TESObjectREFR*, int32_t>* iter = RemovalQueueMap.GetFirstPos();
    while (iter) {
        TESObjectREFR* pref;
        int32_t icount;
        RemovalQueueMap.GetNext(iter, pref, icount);
        --icount;
        RemovalQueueMap.SetAt(pref, icount);
    }
    bool bloop = true;
    while (bloop) {
        bloop = false;
        NiTMapItem<TESObjectREFR*, int32_t>* iter2 = RemovalQueueMap.GetFirstPos();
        while (iter2) {
            TESObjectREFR* pref;
            int32_t icount;
            RemovalQueueMap.GetNext(iter2, pref, icount);
            if (!icount) {
                RemovalQueueMap.RemoveAt(pref);
                FalloutAudio::ClearAcousticSpace(static_cast<BGSAcousticSpace*>(pref->GetObjectReference()));
                bloop = true;
                break;
            }
        }
    }
}

bool BGSAcousticSpaceListener::contactPointAddedCallback(hkpRootCdPoint& CPoint) {
    TESObjectREFR* apRef = nullptr;
    if (pPlayer->Is3rdPerson()) {
        bhkWorldObject* CameraRigidBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
        if (CPoint.m_rootCollidableB == CameraRigidBody->GetCollidable()) {
            apRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        }
    }
    else {
        if (TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableB) == pPlayer) {
            apRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        }
    }
    if (!apRef) {
        return false;
    }
    if (!bBufferRemovalOperations || !DequeueRemoval(apRef, false)) {
        BGSAcousticSpace* ObjectReference = static_cast<BGSAcousticSpace*>(apRef->GetObjectReference());
        FalloutAudio::SetAcousticSpace(ObjectReference);
    }
    return true;
}

bool BGSAcousticSpaceListener::contactPointRemovedCallback(hkpRootCdPoint& CPoint) {
    TESObjectREFR* apRef = nullptr;
    if (pPlayer->Is3rdPerson()) {
        bhkWorldObject* CameraRigidBody = static_cast<bhkWorldObject*>(pPlayer->GetCameraRigidBody());
        if (CPoint.m_rootCollidableB == CameraRigidBody->GetCollidable()) {
            apRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        }
    }
    else {
        if (TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableB) == pPlayer) {
            apRef = TESHavokUtilities::FindCollidableRef(CPoint.m_rootCollidableA);
        }
    }
    if (apRef) {
        if (bBufferRemovalOperations) {
            QueueRemoval(apRef);
        }
        else {
            BGSAcousticSpace* ObjectReference = static_cast<BGSAcousticSpace*>(apRef->GetObjectReference());
            FalloutAudio::ClearAcousticSpace(ObjectReference);
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
    (&RemovalQueueMap)->SetAt(apRef, 5);
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
    BGSAcousticSpace* objectReference;
    int32_t a4;
    TESObjectREFR* a3;
    NiTMapItem<TESObjectREFR*, int32_t>* a2;
    a2 = (&RemovalQueueMap)->GetFirstPos();
    while (a2) {
        (&RemovalQueueMap)->GetNext(a2, a3, a4);
        objectReference = static_cast<BGSAcousticSpace*>(a3->TESObjectREFR::GetObjectReference());
        FalloutAudio::ClearAcousticSpace(objectReference);
    }
    (&RemovalQueueMap)->RemoveAll();
}

void BGSAcousticSpaceListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkpEntityListener::hkpEntityListener() {}

hkpEntityListener::~hkpEntityListener() {}

void hkpEntityListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
