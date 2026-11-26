#include "Fallout/AI/Combat/CombatCoverMap.h"

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/XINISetting.h"
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

CoverReservationInfo::CoverReservationInfo()
  : cSegmentLength(0), cSegmentCount(0), cActorWidth(0), cSegment(0), bHideCover(false), bLeftEdge(false) {}

uint32_t CoverReservationInfo::GetCoverMask() {
    return static_cast<uint32_t>(((2 << (static_cast<int8_t>(cActorWidth) - 1)) - 1) << cSegment);
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
    cActorWidth = 0;
    cSegment = 0;
    cSegmentLength = 0;
    cSegmentCount = 0;
    bHideCover = false;
    bLeftEdge = false;
}

void CoverReservationInfo::CalculateSegmentInfo(PathingCoverLocation* apCoverLocation) {
    float fsegmentLength;
    float fsegmentCount;
    NiPoint3 kSpan;
    kSpan = apCoverLocation->LeftVertex - apCoverLocation->RightVertex;
    float fWidth = kSpan.Length();
    if (!(fWidth <= 256.0)) {
        fsegmentLength = static_cast<float>(fWidth / 32.0);
        if (!(fsegmentLength > 255.0)) {
            cSegmentLength = static_cast<uint8_t>(static_cast<int32_t>(FloatFloor(fsegmentLength)));
        }
        else {
            cSegmentLength = static_cast<uint8_t>(-1);
        }
        cSegmentCount = 32;
    }
    else {
        fsegmentCount = static_cast<float>(fWidth / 8.0);
        cSegmentCount = static_cast<uint8_t>(static_cast<int32_t>(FloatFloor(fsegmentCount)));
        fsegmentLength = static_cast<float>(fWidth / static_cast<double>(cSegmentCount));
        cSegmentLength = static_cast<uint8_t>(static_cast<int32_t>(FloatFloor(fsegmentLength)));
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
            int32_t ioffset = 0;
            int32_t icenter = (static_cast<int32_t>(cSegmentCount) >> 1) - cActorWidth / 2;
            char cdone = 0;
            while (!cdone) {
                for (int32_t k = 0; k < (ioffset > 0) + 1; ++k) {
                    int32_t isegment = k ? ioffset + icenter : icenter - ioffset;
                    if (isegment < 0 || isegment > cSegmentCount - cActorWidth) {
                        cdone = 1;
                    }
                    else if ((icoverMask << isegment & ireservations) == 0) {
                        cSegment = isegment;
                        return true;
                    }
                }
                ++ioffset;
            }
        }
    }
    else {
        uint8_t cshift = bLeftEdge ? 0 : cSegmentCount - cActorWidth;
        if ((icoverMask << cshift & ireservations) == 0) {
            cSegment = cshift;
            return true;
        }
    }
    Clear();
    return false;
}

void CoverReservationInfo::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&cSegmentLength, sizeof(cSegmentLength), 0);
    apSaveGameBuffer->SaveDataEndian(&cSegmentCount, sizeof(cSegmentCount), 0);
    apSaveGameBuffer->SaveDataEndian(&cActorWidth, sizeof(cActorWidth), 0);
    apSaveGameBuffer->SaveDataEndian(&cSegment, sizeof(cSegment), 0);
    apSaveGameBuffer->SaveDataEndian(&bHideCover, sizeof(bHideCover), 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftEdge, sizeof(bLeftEdge), 0);
}

void CoverReservationInfo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&cSegmentLength, sizeof(cSegmentLength));
    apLoadGameBuffer->LoadData(&cSegmentCount, sizeof(cSegmentCount));
    apLoadGameBuffer->LoadData(&cActorWidth, sizeof(cActorWidth));
    apLoadGameBuffer->LoadData(&cSegment, sizeof(cSegment));
    apLoadGameBuffer->LoadData(&bHideCover, sizeof(bHideCover));
    apLoadGameBuffer->LoadData(&bLeftEdge, sizeof(bLeftEdge));
}

CoverReservation::CoverReservation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation)
  : pCombatController(apCombatController), pCombatCoverLocation(apCombatCoverLocation) {}

ReservedCoverLocation::ReservedCoverLocation()
  : iReservations(0), cSegmentLength(0), cSegmentCount(0), cCoverHeight(0), spDebugGeometry(nullptr) {}

ReservedCoverLocation::~ReservedCoverLocation() {
    if (spDebugGeometry.m_pObject && spDebugGeometry->GetParent()) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueNodeDetach(spDebugGeometry.m_pObject);
        }
        else {
            spDebugGeometry->GetParent()->DetachChild(spDebugGeometry.m_pObject);
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
    for (uint32_t i = 0; i < Reservations.iSize; ++i) {
        CoverReservation* coverReservation = &Reservations[i];
        if (coverReservation->pCombatController == apCombatController) {
            return;
        }
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
    for (uint32_t i = 0; i < Reservations.iSize; ++i) {
        CoverReservation* coverReservation = &Reservations[i];
        if (coverReservation->pCombatController == apCombatController) {
            return false;
        }
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
        if (Reservations[i].pCombatController == apCombatController) {
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
    if (spCoverSearchDebugGeometry.m_pObject && spCoverSearchDebugGeometry->m_pkParent) {
        spCoverSearchDebugGeometry->m_pkParent->DetachChild(spCoverSearchDebugGeometry.m_pObject);
    }
}

bool CombatCoverMap::IsCurrentThreadWriting() {
    return Lock.IsWritingThread();
}

void CombatCoverMap::UnlockForWrite() {
    Lock.UnlockWrite();
}

void CombatCoverMap::LockForWrite() {
    Lock.LockForWrite();
}

void CombatCoverMap::ReserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    Lock.LockForWrite();
    ReservedCoverLocation* preservedLocation = nullptr;
    if (!GetAt(apCombatCoverLocation->iCoverLocationKey, preservedLocation)) {
        preservedLocation = new ReservedCoverLocation;
        preservedLocation->SetCoverHeight(apCombatCoverLocation->cCoverHeight);
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
        preservedLocation = new ReservedCoverLocation;
        preservedLocation->SetCoverHeight(apCombatCoverLocation->cCoverHeight);
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
    uint8_t ccanReserve = 1;
    Lock.LockForRead();
    ReservedCoverLocation* pReservedLocation = nullptr;
    bool bblocked = NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>::
                        GetAt(aiKey, pReservedLocation) &&
        !pReservedLocation->CanReserveCoverLocation(apCombatController);
    if (bblocked) {
        ccanReserve = 0;
    }
    if (ccanReserve) {
        ccanReserve = apReservationInfo->FindCoverReservation(
            apCoverLocation,
            apCombatController,
            afActorRadius,
            pReservedLocation,
            abHideCover,
            abLeftEdge
        );
    }
    Lock.UnlockRead();
    return ccanReserve;
}

void CombatCoverMap::UnreserveCoverLocation(
    CombatController* apCombatController,
    CombatCoverLocation* apCombatCoverLocation
) {
    ReservedCoverLocation* pLocation;
    if (apCombatCoverLocation->iCoverLocationKey) {
        Lock.LockForWrite();
        pLocation = nullptr;
        bool
            bfound = NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>::
                GetAt(apCombatCoverLocation->iCoverLocationKey, pLocation);
        if (bfound) {
            if (pLocation->UnreserveCoverLocation(apCombatController, apCombatCoverLocation)) {
                if (pLocation) {
                    delete pLocation;
                }
                NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>::
                    RemoveAt(apCombatCoverLocation->iCoverLocationKey);
            }
            apCombatCoverLocation->SetReserved(false);
        }
        Lock.UnlockWrite();
    }
}

void CombatCoverLocation::SetReserved(bool abValue) {
    bReserved = abValue;
}

bool CombatCoverLocation::IsLedgeCover() {
    return CoverLocation.IsLedgeCover();
}
