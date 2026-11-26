#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

PathingRequestLOS::PathingRequestLOS()
  : PathingRequest(), spLOSGridMap(nullptr), IgnoreLocations(), fIgnoreLocationRadius(500.0f),
    fSearchCenterRadius(0.0f) {}

PathingRequestLOS::PathingRequestLOS(const PathingRequestLOS& aRequest)
  : PathingRequest(aRequest), spLOSGridMap(aRequest.spLOSGridMap), IgnoreLocations(),
    fIgnoreLocationRadius(aRequest.fIgnoreLocationRadius) {
    aRequest.IgnoreLocations.CopyTo(&IgnoreLocations);
    SetSearchCenter(aRequest.GetSearchCenter(), aRequest.fSearchCenterRadius);
}

PathingRequestLOS::~PathingRequestLOS() {}

// The target is a PathingRequestLOS of the same type; its fields are written without a cast check.
void PathingRequestLOS::CopyTo(PathingRequest* apDest) const {
    PathingRequestLOS* pTarget = static_cast<PathingRequestLOS*>(apDest);
    PathingRequest::CopyTo(pTarget);
    pTarget->spLOSGridMap = spLOSGridMap;
    IgnoreLocations.CopyTo(&pTarget->IgnoreLocations);
    pTarget->fIgnoreLocationRadius = fIgnoreLocationRadius;
    pTarget->SetSearchCenter(SearchCenter, fSearchCenterRadius);
}

uint32_t PathingRequestLOS::GetType() const {
    return PATHING_REQUEST_LOS;
}

void PathingRequestLOS::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount;
    PathingRequest::SaveGame(apSaveGameBuffer);
    // The address &IgnoreLocations is never null.
    isaveGameCount = IgnoreLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        apSaveGameBuffer->SaveDataEndian(&IgnoreLocations[i], sizeof(IgnoreLocations[i]), 0);
    }
    apSaveGameBuffer->SaveDataEndian(&fIgnoreLocationRadius, sizeof(fIgnoreLocationRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&SearchCenter, sizeof(SearchCenter), 0);
    apSaveGameBuffer->SaveDataEndian(&fSearchCenterRadius, sizeof(fSearchCenterRadius), 0);
}

void PathingRequestLOS::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PathingRequest::LoadGame(apLoadGameBuffer);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    IgnoreLocations.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        apLoadGameBuffer->LoadData(&IgnoreLocations[i], sizeof(IgnoreLocations[i]));
    }
    apLoadGameBuffer->LoadData(&fIgnoreLocationRadius, sizeof(fIgnoreLocationRadius));
    apLoadGameBuffer->LoadData(&SearchCenter, sizeof(SearchCenter));
    apLoadGameBuffer->LoadData(&fSearchCenterRadius, sizeof(fSearchCenterRadius));
}

void PathingRequestLOS::SetSearchCenter(NiPoint3 aCenter, float afRadius) {
    SearchCenter = aCenter;
    fSearchCenterRadius = afRadius;
}

void PathingRequestLOS::SetIgnoreLocationRadius(float afIgnoreRadius) {
    fIgnoreLocationRadius = afIgnoreRadius;
}
