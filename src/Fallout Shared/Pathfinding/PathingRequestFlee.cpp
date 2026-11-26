#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"

#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

PathingRequestFlee::PathingRequestFlee() : PathingRequest(), fMinFleeDistance(0x1p+7f) {}

PathingRequestFlee::PathingRequestFlee(const PathingRequestFlee& aRequest)
  : PathingRequest(aRequest), fMinFleeDistance(aRequest.fMinFleeDistance) {}

PathingRequestFlee::~PathingRequestFlee() {}

void PathingRequestFlee::CopyTo(PathingRequest* apDest) const {
    PathingRequest::CopyTo(apDest);
    static_cast<PathingRequestFlee*>(apDest)->fMinFleeDistance = fMinFleeDistance;
}

uint32_t PathingRequestFlee::GetType() const {
    return PATHING_REQUEST_FLEE;
}

void PathingRequestFlee::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    PathingRequest::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fMinFleeDistance)), sizeof(fMinFleeDistance), 0);
}

void PathingRequestFlee::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PathingRequest::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fMinFleeDistance), sizeof(fMinFleeDistance));
}
