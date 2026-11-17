#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/AI/Combat/CombatCoverSearch.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

PathingRequestCoveredMove::PathingRequestCoveredMove()
  : PathingRequest(), fAttackerRadius(0.0f), fAttackerSpeed(1.0f), fAttackerHeight(0.0f), fAttackerSneakHeight(0.0f),
    fAttackerHealth(1.0f), iLocationIndex(0), CoverLocation(NiPoint3_ZERO), TargetLocation(NiPoint3_ZERO),
    bHasTargetLocation(false), bFinished(false), UnreachableCoverLocations(), TargetLocations(),
    pCombatCoverLocation(nullptr), pCombatCoverSearchInfo(nullptr) {}

// The base's copy, empty arrays, then aRequest's CopyTo onto this, called directly as the game does.
PathingRequestCoveredMove::PathingRequestCoveredMove(const PathingRequestCoveredMove& aRequest)
  : PathingRequest(aRequest), UnreachableCoverLocations(), TargetLocations() {
    aRequest.PathingRequestCoveredMove::CopyTo(this);
}

// The request owns its cover location and search info.
PathingRequestCoveredMove::~PathingRequestCoveredMove() {
    if (pCombatCoverLocation) {
        delete pCombatCoverLocation;
    }
    if (pCombatCoverSearchInfo) {
        delete pCombatCoverSearchInfo;
    }
}

void PathingRequestCoveredMove::CopyTo(PathingRequest* apDest) const {
    PathingRequestCoveredMove* pTarget = static_cast<PathingRequestCoveredMove*>(apDest);
    PathingRequest::CopyTo(apDest);
    pTarget->SetAttackerRadius(fAttackerRadius);
    pTarget->SetAttackerSpeed(fAttackerSpeed);
    pTarget->SetAttackerHeight(fAttackerHeight);
    pTarget->SetActorSneakHeight(fAttackerSneakHeight);
    pTarget->SetAttackerHealth(fAttackerHealth);
    pTarget->iLocationIndex = iLocationIndex;
    pTarget->SetCoverLocation(CoverLocation);
    if (bHasTargetLocation) {
        pTarget->SetTargetLocation(TargetLocation);
    }
    else {
        pTarget->ClearTargetLocation();
    }
    pTarget->bFinished = bFinished;
    UnreachableCoverLocations.CopyTo(pTarget->GetUnreachableCoverLocations());
    BSSimpleArray<NiPoint3, 1024>* pTargetLocations = pTarget->GetTargetLocations();
    TargetLocations.CopyTo(pTargetLocations);
}

uint32_t PathingRequestCoveredMove::GetType() const {
    return PATHING_REQUEST_COVERED_MOVE;
}

void PathingRequestCoveredMove::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount_2;
    uint32_t isaveGameCount;
    PathingRequest::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&fAttackerRadius, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerSpeed, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerHeight, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerSneakHeight, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerHealth, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iLocationIndex, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&CoverLocation, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&TargetLocation, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&bHasTargetLocation, 1u, 0);
    isaveGameCount = UnreachableCoverLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t auiIndex = 0; auiIndex < isaveGameCount; ++auiIndex) {
        apSaveGameBuffer->SaveDataEndian(&UnreachableCoverLocations.operator[](auiIndex), 4u, 0);
    }
    isaveGameCount_2 = TargetLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2);
    for (uint32_t i = 0; i < isaveGameCount_2; ++i) {
        apSaveGameBuffer->SaveDataEndian(&TargetLocations.operator[](i), 12u, 0);
    }
}

void PathingRequestCoveredMove::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PathingRequest::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fAttackerRadius, 4u);
    apLoadGameBuffer->LoadData(&fAttackerSpeed, 4u);
    apLoadGameBuffer->LoadData(&fAttackerHeight, 4u);
    apLoadGameBuffer->LoadData(&fAttackerSneakHeight, 4u);
    apLoadGameBuffer->LoadData(&fAttackerHealth, 4u);
    apLoadGameBuffer->LoadData(&iLocationIndex, 4u);
    apLoadGameBuffer->LoadData(&CoverLocation, 12u);
    apLoadGameBuffer->LoadData(&TargetLocation, 12u);
    apLoadGameBuffer->LoadData(&bHasTargetLocation, 1u);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    UnreachableCoverLocations.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        apLoadGameBuffer->LoadData(&UnreachableCoverLocations.operator[](i), 4u);
    }
    uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
    TargetLocations.SetSize(isaveGameCount_2, true);
    for (uint32_t i_2 = 0; i_2 < isaveGameCount_2; ++i_2) {
        apLoadGameBuffer->LoadData(&TargetLocations.operator[](i_2), 12u);
    }
}

void PathingRequestCoveredMove::SetCombatCoverSearchInfo(CombatCoverSearchInfo* apSearchInfo) {
    pCombatCoverSearchInfo = apSearchInfo;
}

void PathingRequestCoveredMove::ClearCombatCoverData() {
    pCombatCoverLocation = nullptr;
    pCombatCoverSearchInfo = nullptr;
}

void PathingRequestCoveredMove::SetCoverLocation(const NiPoint3& arCoverLocation) {
    CoverLocation = arCoverLocation;
}

void PathingRequestCoveredMove::SetActorSneakHeight(float a2) {
    fAttackerSneakHeight = a2;
}

void PathingRequestCoveredMove::SetTargetLocation(const NiPoint3& arTargetLocation) {
    TargetLocation = arTargetLocation;
    bHasTargetLocation = true;
}

void PathingRequestCoveredMove::ClearTargetLocation() {
    TargetLocation = NiPoint3_ZERO;
    bHasTargetLocation = false;
}

void PathingRequestCoveredMove::SetAttackerRadius(float afRadius) {
    fAttackerRadius = afRadius;
}

void PathingRequestCoveredMove::SetAttackerSpeed(float afSpeed) {
    fAttackerSpeed = afSpeed;
}

void PathingRequestCoveredMove::SetAttackerHeight(float afHeight) {
    fAttackerHeight = afHeight;
}

void PathingRequestCoveredMove::SetAttackerHealth(float afHealth) {
    fAttackerHealth = afHealth;
}

BSSimpleArray<uint32_t, 1024>* PathingRequestCoveredMove::GetUnreachableCoverLocations() {
    return &UnreachableCoverLocations;
}

BSSimpleArray<NiPoint3, 1024>* PathingRequestCoveredMove::GetTargetLocations() {
    return &TargetLocations;
}
