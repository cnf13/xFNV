#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// A request for a point that sees (or hides from) the locations of a line-of-sight map, searched around a center.
class PathingRequestLOS : public PathingRequest {
public:
    PathingRequestLOS();
    PathingRequestLOS(const PathingRequestLOS& aRequest);
    ~PathingRequestLOS() override;

    void CopyTo(PathingRequest* apDest) const override;
    uint32_t GetType() const override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    void SetSearchCenter(NiPoint3 aCenter, float afRadius);
    NiPoint3 GetSearchCenter() const;
    void SetLOSGridMap(const NiPointer<PathingLOSGridMap>& aspLOSGridMap);
    // Stores afRadius in fIgnoreLocationRadius.
    void SetIgnoreLocationRadius(float afIgnoreRadius);
    // Copies aLocations into IgnoreLocations.
    void SetIgnoreLocations(const BSSimpleArray<NiPoint3>& aLocations);

    NiPointer<PathingLOSGridMap> spLOSGridMap;
    BSSimpleArray<NiPoint3> IgnoreLocations;
    float fIgnoreLocationRadius;
    NiPoint3 SearchCenter;
    float fSearchCenterRadius;
};
