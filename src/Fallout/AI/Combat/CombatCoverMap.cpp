#include "Fallout/AI/Combat/CombatCoverMap.h"

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bDebugCombatCoverReservations("bDebugCombatCoverReservations:Combat", false);
SettingT<INISettingCollection> bDebugCombatCoverReservationsText("bDebugCombatCoverReservationsText:Combat", false);
SettingT<INISettingCollection> bDebugCombatCoverSearch("bDebugCombatCoverSearch:Combat", false);
SettingT<INISettingCollection> bDebugCombatCoverSearchText("bDebugCombatCoverSearchText:Combat", false);
SettingT<INISettingCollection> bDebugCombatCoverSearch2("bDebugCombatCoverSearch2:Combat", false);

uint8_t BSMIN(uint8_t a1, uint8_t a2) {
    if (((int32_t)a1) >= ((int32_t)a2)) {
        return a2;
    }

    return a1;
}

CoverReservationInfo::CoverReservationInfo()
  : cSegmentLength(0), cSegmentCount(0), cActorWidth(0), cSegment(0), bHideCover(false), bLeftEdge(false) {}

uint32_t CoverReservationInfo::GetCoverMask() {
    return (uint32_t)(((2 << (((int8_t)cActorWidth) - 1)) - 1) << cSegment);
}

// The radius is scaled by the double 1.5 and stored back as a float; a width below one segment length (an ordered less)
// takes one segment, else the ceiling of the width in segment lengths, truncated to a byte.
char CoverReservationInfo::CalculateActorWidth(float afActorRadius) {
    afActorRadius = afActorRadius * 1.5;
    if (afActorRadius < static_cast<float>(cSegmentLength)) {
        return 1;
    }
    float fSegments = afActorRadius / static_cast<float>(cSegmentLength);
    return static_cast<char>(static_cast<int32_t>(FloatCeil(fSegments)));
}

void CoverReservationInfo::Clear() {
    cActorWidth = (uint8_t)0;
    cSegment = (uint8_t)0;
    cSegmentLength = (uint8_t)0;
    cSegmentCount = (uint8_t)0;
    bHideCover = false;
    bLeftEdge = false;
}

void CoverReservationInfo::CalculateSegmentInfo(PathingCoverLocation* apCoverLocation) {
    float v2;
    float v3;
    float v5;
    NiPoint3 result;
    (result = apCoverLocation->LeftVertex - apCoverLocation->RightVertex);
    float fWidth = (&result)->Length();
    if (!(fWidth <= 256.0)) {
        v5 = (float)(fWidth / 32.0);
        if (!(v5 > 255.0)) {
            cSegmentLength = (uint8_t)((int32_t)FloatFloor(v5));
        }
        else {
            cSegmentLength = (uint8_t)(-1);
        }
        cSegmentCount = (uint8_t)32;
    }
    else {
        v3 = (float)(fWidth / 8.0);
        cSegmentCount = (uint8_t)((int32_t)FloatFloor(v3));
        v2 = (float)(fWidth / ((double)cSegmentCount));
        cSegmentLength = (uint8_t)((int32_t)FloatFloor(v2));
    }
}

bool CoverReservationInfo::FindCoverReservation(
    PathingCoverLocation* apCoverLocation,
    CombatController* apCombatController,
    float afActorRadius,
    ReservedCoverLocation* apReservedCoverLocation,
    bool abHideCover,
    bool abLeftEdge
) {
    uint32_t ireservations = 0;
    if (apReservedCoverLocation) {
        cSegmentCount = apReservedCoverLocation->cSegmentCount;
        cSegmentLength = apReservedCoverLocation->cSegmentLength;
        ireservations = apReservedCoverLocation->iReservations;
    }
    else {
        CalculateSegmentInfo(apCoverLocation);
    }
    if (!cSegmentLength || !cSegmentCount) {
        return false;
    }
    if (!cActorWidth) {
        cActorWidth = CalculateActorWidth(afActorRadius);
    }
    if (static_cast<uint8_t>(static_cast<int32_t>(static_cast<double>(cActorWidth) / 1.5)) >
        static_cast<int32_t>(cSegmentCount)) {
        return false;
    }
    cActorWidth = BSMIN(cActorWidth, cSegmentCount);
    if (cSegment) {
        bHideCover = abHideCover;
        if ((ireservations & GetCoverMask()) == 0 && bLeftEdge == abLeftEdge) {
            return true;
        }
    }
    bHideCover = abHideCover;
    bLeftEdge = abLeftEdge;
    cSegment = 0;
    uint32_t icoverMask = GetCoverMask();
    if (!apCoverLocation->IsStandCover() || bHideCover) {
        if (cSegmentCount / static_cast<int32_t>(cActorWidth) == 2) {
            if (apCoverLocation->bLeftEdgeOpen) {
                for (int32_t i = cSegmentCount - cActorWidth; i >= 0; --i) {
                    if ((icoverMask << i & ireservations) == 0) {
                        cSegment = i;
                        return true;
                    }
                }
            }
            else {
                for (int32_t j = 0; j <= cSegmentCount - cActorWidth; ++j) {
                    if ((icoverMask << j & ireservations) == 0) {
                        cSegment = j;
                        return true;
                    }
                }
            }
        }
        else {
            int32_t v13 = 0;
            int32_t v14 = (static_cast<int32_t>(cSegmentCount) >> 1) - cActorWidth / 2;
            char v15 = 0;
            while (!v15) {
                for (int32_t k = 0; k < (v13 > 0) + 1; ++k) {
                    int32_t v8;
                    if (k) {
                        v8 = v13 + v14;
                    }
                    else {
                        v8 = v14 - v13;
                    }
                    if (v8 < 0 || v8 > cSegmentCount - cActorWidth) {
                        v15 = 1;
                    }
                    else {
                        if ((icoverMask << v8 & ireservations) == 0) {
                            cSegment = v8;
                            return true;
                        }
                    }
                }
                ++v13;
            }
        }
    }
    else {
        uint8_t cshift;
        if (bLeftEdge) {
            cshift = 0;
        }
        else {
            cshift = cSegmentCount - cActorWidth;
        }
        if ((icoverMask << cshift & ireservations) == 0) {
            cSegment = cshift;
            return true;
        }
    }
    Clear();
    return false;
}

void CoverReservationInfo::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(this, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cSegmentCount, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cActorWidth, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cSegment, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bHideCover, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftEdge, 1u, 0);
}

void CoverReservationInfo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 1u);
    apLoadGameBuffer->LoadData(&cSegmentCount, 1u);
    apLoadGameBuffer->LoadData(&cActorWidth, 1u);
    apLoadGameBuffer->LoadData(&cSegment, 1u);
    apLoadGameBuffer->LoadData(&bHideCover, 1u);
    apLoadGameBuffer->LoadData(&bLeftEdge, 1u);
}

CoverReservation::CoverReservation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation)
  : pCombatController(apCombatController), pCombatCoverLocation(apCombatCoverLocation) {}

ReservedCoverLocation::ReservedCoverLocation()
  : iReservations(0), cSegmentLength(0), cSegmentCount(0), cCoverHeight(0), spDebugGeometry(nullptr) {}

ReservedCoverLocation::~ReservedCoverLocation() {
    if (spDebugGeometry.m_pObject && spDebugGeometry.m_pObject->GetParent()) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueNodeDetach(spDebugGeometry.m_pObject);
        }
        else {
            spDebugGeometry.m_pObject->GetParent()->DetachChild(spDebugGeometry.m_pObject);
        }
    }
}

void ReservedCoverLocation::CopySegmentInfo(CoverReservationInfo& arReservationInfo) {
    cSegmentLength = arReservationInfo.cSegmentLength;
    cSegmentCount = arReservationInfo.cSegmentCount;
}

void ReservedCoverLocation::ReserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    uint32_t i_2 = 0;
    for (uint32_t i = Reservations.iSize; i_2 < i; i = Reservations.iSize) {
        CoverReservation* coverReservation = &Reservations[i_2];
        if (coverReservation->pCombatController == apCombatController) {
            return;
        }
        ++i_2;
    }
    if (!cSegmentCount) {
        CopySegmentInfo(apCombatCoverLocation->ReservationInfo);
    }
    uint32_t icoverSegmentMask = apCombatCoverLocation->ReservationInfo.GetCoverMask();
    iReservations |= icoverSegmentMask;
    CoverReservation kReservation(apCombatController, apCombatCoverLocation);
    Reservations.Add(kReservation);
}

bool ReservedCoverLocation::TryReserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    uint32_t i_2 = 0;
    for (uint32_t i = Reservations.iSize; i_2 < i; i = Reservations.iSize) {
        CoverReservation* coverReservation = &Reservations[i_2];
        if (coverReservation->pCombatController == apCombatController) {
            return false;
        }
        ++i_2;
    }
    if (!cSegmentCount) {
        CopySegmentInfo(apCombatCoverLocation->ReservationInfo);
    }
    uint32_t icoverSegmentMask = apCombatCoverLocation->ReservationInfo.GetCoverMask();
    if ((icoverSegmentMask & iReservations) != 0) {
        return false;
    }
    iReservations |= icoverSegmentMask;
    CoverReservation kReservation(apCombatController, apCombatCoverLocation);
    Reservations.Add(kReservation);
    return true;
}

bool ReservedCoverLocation::CanReserveCoverLocation(CombatController* apCombatController) {
    CombatGroup* pGroup;
    Actor* pActor;
    CoverReservation* pReservation;
    uint32_t i;
    if (apCombatController) {
        for (i = 0; i < Reservations.iSize; ++i) {
            pReservation = &Reservations[i];
            if (pReservation->pCombatController == apCombatController) {
                return false;
            }
            pActor = pReservation->pCombatController->GetAttacker();
            pGroup = apCombatController->CombatController::GetCombatGroup();
            if (pGroup->IsTarget(pActor)) {
                return false;
            }
        }
    }
    return true;
}

void ReservedCoverLocation::SetCoverHeight(uint8_t acCoverHeight) {
    cCoverHeight = acCoverHeight;
}

bool ReservedCoverLocation::UnreserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    uint32_t i;
    uint32_t isize = Reservations.iSize;
    for (i = 0;; ++i) {
        if (i >= isize) {
            return false;
        }
        if ((&Reservations[i])->pCombatController == apCombatController) {
            break;
        }
    }
    if (isize == 1) {
        return true;
    }
    iReservations &= ~apCombatCoverLocation->ReservationInfo.GetCoverMask();
    Reservations.RemoveFast(i, 1u);
    return false;
}

CombatCoverMap::CombatCoverMap() : NiTMap<uint32_t, ReservedCoverLocation*>(37u), spCoverSearchDebugGeometry(nullptr) {}

CombatCoverMap::~CombatCoverMap() {
    NiTMapItem<uint32_t, ReservedCoverLocation*>* it = GetFirstPos();
    while (it) {
        uint32_t ikey = 0;
        ReservedCoverLocation* preservedLocation = nullptr;
        GetNext(it, ikey, preservedLocation);
        delete preservedLocation;
    }
    RemoveAll();
    if (spCoverSearchDebugGeometry.m_pObject && spCoverSearchDebugGeometry.m_pObject->m_pkParent) {
        spCoverSearchDebugGeometry.m_pObject->m_pkParent->DetachChild(spCoverSearchDebugGeometry.m_pObject);
    }
}

bool CombatCoverMap::IsCurrentThreadWriting() {
    return (bool)((uint8_t)(&Lock)->IsWritingThread());
}

void CombatCoverMap::UnlockForWrite() {
    (&Lock)->UnlockWrite();
}

void CombatCoverMap::LockForWrite() {
    (&Lock)->LockForWrite();
}

void CombatCoverMap::ReserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    Lock.LockForWrite();
    ReservedCoverLocation* preservedLocation = nullptr;
    if (!GetAt(apCombatCoverLocation->iCoverLocationKey, preservedLocation)) {
        ReservedCoverLocation* preservedLocation_2;
        ReservedCoverLocation* apMemory = static_cast<ReservedCoverLocation*>(ourMalloc(sizeof(ReservedCoverLocation)));
        if (apMemory) {
            preservedLocation_2 = ::new (apMemory) ReservedCoverLocation;
        }
        else {
            preservedLocation_2 = nullptr;
        }
        preservedLocation = preservedLocation_2;
        uint8_t v3 = apCombatCoverLocation->cCoverHeight;
        preservedLocation->SetCoverHeight(v3);
        SetAt(apCombatCoverLocation->iCoverLocationKey, preservedLocation);
    }
    preservedLocation->ReserveCoverLocation(apCombatController, apCombatCoverLocation);
    apCombatCoverLocation->bReserved = true;
    Lock.UnlockWrite();
}

bool CombatCoverMap::TryReserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    Lock.LockForWrite();
    ReservedCoverLocation* preservedLocation = nullptr;
    if (!GetAt(apCombatCoverLocation->iCoverLocationKey, preservedLocation)) {
        ReservedCoverLocation* preservedLocation_2;
        ReservedCoverLocation* apMemory = static_cast<ReservedCoverLocation*>(ourMalloc(sizeof(ReservedCoverLocation)));
        if (apMemory) {
            preservedLocation_2 = ::new (apMemory) ReservedCoverLocation;
        }
        else {
            preservedLocation_2 = nullptr;
        }
        preservedLocation = preservedLocation_2;
        uint8_t v3 = apCombatCoverLocation->cCoverHeight;
        preservedLocation->SetCoverHeight(v3);
        SetAt(apCombatCoverLocation->iCoverLocationKey, preservedLocation);
    }
    bool bresult = preservedLocation->TryReserveCoverLocation(apCombatController, apCombatCoverLocation);
    apCombatCoverLocation->bReserved = bresult;
    Lock.UnlockWrite();
    return bresult;
}

bool CombatCoverMap::CanReserveCoverLocation(
    uint32_t aiKey,
    PathingCoverLocation* apCoverLocation,
    CombatController* apCombatController,
    float afActorRadius,
    CoverReservationInfo* apReservationInfo,
    bool abHideCover,
    bool abLeftEdge
) {
    uint8_t v11 = 1;
    (&Lock)->LockForRead();
    ReservedCoverLocation* pReservedLocation = nullptr;
    bool vCondition1;
    vCondition1 =
        NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>::GetAt(
            aiKey, pReservedLocation
        ) &&
        (!pReservedLocation->CanReserveCoverLocation(apCombatController));
    if (vCondition1) {
        v11 = 0;
    }
    if (v11) {
        v11 = (uint8_t)apReservationInfo->FindCoverReservation(
            apCoverLocation, apCombatController, afActorRadius, pReservedLocation, abHideCover, abLeftEdge
        );
    }
    (&Lock)->UnlockRead();
    return (bool)v11;
}

void CombatCoverMap::UnreserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    ReservedCoverLocation* pLocation;
    if (apCombatCoverLocation->iCoverLocationKey) {
        (&Lock)->LockForWrite();
        pLocation = nullptr;
        bool vCondition1;
        vCondition1 =
            NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>::GetAt(
                apCombatCoverLocation->iCoverLocationKey, pLocation
            );
        if (vCondition1) {
            if (pLocation->UnreserveCoverLocation(apCombatController, apCombatCoverLocation)) {
                if (pLocation) {
                    delete pLocation;
                }
                NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>::
                    RemoveAt(apCombatCoverLocation->iCoverLocationKey);
            }
            apCombatCoverLocation->SetReserved(false);
        }
        (&Lock)->UnlockWrite();
    }
}

void CombatCoverLocation::SetReserved(bool abValue) {
    bReserved = abValue;
}

bool CombatCoverLocation::IsLedgeCover() {
    return CoverLocation.IsLedgeCover();
}
