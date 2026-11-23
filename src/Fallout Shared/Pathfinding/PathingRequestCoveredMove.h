#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class CombatCoverLocation;
class CombatCoverSearchInfo;

// A request for a path to a cover location: the attacker's size, speed and health, the kind of cover wanted, the target
// it hides from and the cover locations already found unreachable. GetType, CopyTo, SaveGame and LoadGame are
// overridden.
class PathingRequestCoveredMove : public PathingRequest {
public:
    PathingRequestCoveredMove();
    PathingRequestCoveredMove(const PathingRequestCoveredMove& aRequest);
    ~PathingRequestCoveredMove() override;

    void CopyTo(PathingRequest* apDest) const override;
    uint32_t GetType() const override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    void SetCombatCoverSearchInfo(CombatCoverSearchInfo* apSearchInfo);
    void ClearCombatCoverData();
    void SetCoverLocation(const NiPoint3& arCoverLocation);
    void SetActorSneakHeight(float afSneakHeight);
    // Sets TargetLocation and bHasTargetLocation; the body only reads arLocation.
    void SetTargetLocation(const NiPoint3& arTargetLocation);
    // Clears TargetLocation and bHasTargetLocation.
    void ClearTargetLocation();
    void SetAttackerRadius(float afRadius);
    void SetAttackerSpeed(float afSpeed);
    void SetAttackerHeight(float afHeight);
    void SetAttackerHealth(float afHealth);
    // The unreachable cover locations' array.
    BSSimpleArray<uint32_t>* GetUnreachableCoverLocations();
    // The target locations' array.
    BSSimpleArray<NiPoint3>* GetTargetLocations();

    float fAttackerRadius;
    float fAttackerSpeed;
    float fAttackerHeight;
    float fAttackerSneakHeight;
    float fAttackerHealth;
    uint32_t iLocationIndex;
    NiPoint3 CoverLocation;
    NiPoint3 TargetLocation;
    bool bHasTargetLocation;
    bool bFinished;
    // The cover keys of the locations found unreachable.
    BSSimpleArray<uint32_t> UnreachableCoverLocations;
    BSSimpleArray<NiPoint3> TargetLocations;
    CombatCoverLocation* pCombatCoverLocation;
    CombatCoverSearchInfo* pCombatCoverSearchInfo;
};
