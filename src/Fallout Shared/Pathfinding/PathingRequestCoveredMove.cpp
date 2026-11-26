#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/AI/Combat/CombatCoverSearch.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

PathingRequestCoveredMove::PathingRequestCoveredMove()
  : PathingRequest(), fAttackerRadius(0.0f), fAttackerSpeed(1.0f), fAttackerHeight(0.0f), fAttackerSneakHeight(0.0f),
    fAttackerHealth(1.0f), iLocationIndex(0), CoverLocation(NiPoint3::ZERO), TargetLocation(NiPoint3::ZERO),
    bHasTargetLocation(false), bFinished(false), UnreachableCoverLocations(), TargetLocations(),
    pCombatCoverLocation(nullptr), pCombatCoverSearchInfo(nullptr) {}

// The base's copy, empty arrays, then aRequest's CopyTo onto this, called directly.
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
    TargetLocations.CopyTo(pTarget->GetTargetLocations());
}

uint32_t PathingRequestCoveredMove::GetType() const {
    return PATHING_REQUEST_COVERED_MOVE;
}

void PathingRequestCoveredMove::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    PathingRequest::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&fAttackerRadius, sizeof(fAttackerRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerSpeed, sizeof(fAttackerSpeed), 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerHeight, sizeof(fAttackerHeight), 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerSneakHeight, sizeof(fAttackerSneakHeight), 0);
    apSaveGameBuffer->SaveDataEndian(&fAttackerHealth, sizeof(fAttackerHealth), 0);
    apSaveGameBuffer->SaveDataEndian(&iLocationIndex, sizeof(iLocationIndex), 0);
    apSaveGameBuffer->SaveDataEndian(&CoverLocation, sizeof(CoverLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&TargetLocation, sizeof(TargetLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&bHasTargetLocation, sizeof(bHasTargetLocation), 0);
    uint32_t uiSaveGameCount = UnreachableCoverLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        apSaveGameBuffer->SaveDataEndian(&UnreachableCoverLocations[i], sizeof(UnreachableCoverLocations[i]), 0);
    }
    uiSaveGameCount = TargetLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        apSaveGameBuffer->SaveDataEndian(&TargetLocations[i], sizeof(TargetLocations[i]), 0);
    }
}

void PathingRequestCoveredMove::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PathingRequest::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fAttackerRadius, sizeof(fAttackerRadius));
    apLoadGameBuffer->LoadData(&fAttackerSpeed, sizeof(fAttackerSpeed));
    apLoadGameBuffer->LoadData(&fAttackerHeight, sizeof(fAttackerHeight));
    apLoadGameBuffer->LoadData(&fAttackerSneakHeight, sizeof(fAttackerSneakHeight));
    apLoadGameBuffer->LoadData(&fAttackerHealth, sizeof(fAttackerHealth));
    apLoadGameBuffer->LoadData(&iLocationIndex, sizeof(iLocationIndex));
    apLoadGameBuffer->LoadData(&CoverLocation, sizeof(CoverLocation));
    apLoadGameBuffer->LoadData(&TargetLocation, sizeof(TargetLocation));
    apLoadGameBuffer->LoadData(&bHasTargetLocation, sizeof(bHasTargetLocation));
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    UnreachableCoverLocations.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        apLoadGameBuffer->LoadData(&UnreachableCoverLocations[i], sizeof(UnreachableCoverLocations[i]));
    }
    uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    TargetLocations.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        apLoadGameBuffer->LoadData(&TargetLocations[i], sizeof(TargetLocations[i]));
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

void PathingRequestCoveredMove::SetActorSneakHeight(float afSneakHeight) {
    fAttackerSneakHeight = afSneakHeight;
}

void PathingRequestCoveredMove::SetTargetLocation(const NiPoint3& arTargetLocation) {
    TargetLocation = arTargetLocation;
    bHasTargetLocation = true;
}

void PathingRequestCoveredMove::ClearTargetLocation() {
    TargetLocation = NiPoint3::ZERO;
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
