#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class Actor;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class CombatController;
class CombatGoal;
class CombatGroup;
struct CombatGroupCluster;
class CombatGroupStrategy;
struct CombatMember;
struct CombatSearchLocation;
struct CombatTarget;
class PathingAvoidNodeArray;
class PathingLOSGridMap;
class TESObjectREFR;

// A place the combat group searches for a lost target, with its detection time stamp and score.
struct CombatSearchLocation {
    CombatSearchLocation();

    // The score less the time since the detection.
    float GetScore();
    float GetModifiedScore(const BGSWorldLocation& arMemberWorldLoc);
    // The search location's debug colour by score.
    NiColorA* GetColor(NiColorA* apResult);

    BGSWorldLocation WorldLoc;
    CombatTimeStamp fTimeStamp;
    uint32_t iTargetID;
    float fScore;
};

// What a combat group knows about one of its targets: where it was last noticed, detected, seen and attacked, with
// the time stamps of each and how many members see it.
struct CombatTarget {
    CombatTarget();
    CombatTarget(Actor* apTarget, uint32_t aiSearchCount);

    bool GetHasBufferedDetection();
    void CopyData(CombatTarget* apCombatTarget);
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup);
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup);

    Actor* pActor;
    int32_t iDetectionLevel;
    BGSWorldLocation LastNoticedLocation;
    BGSWorldLocation LastDetectedLocation;
    BGSWorldLocation LastSeenLocation;
    BGSWorldLocation LastAttackedMemberLocation;
    uint16_t sLastSearchNoticed;
    uint16_t sAttackerCount;
    CombatTimeStamp fLastSeenTimeStamp;
    CombatTimeStamp fLastDetectedTimeStamp;
    CombatTimeStamp fDetectionLevelUpdateTimeStamp;
    CombatTimeStamp fLastNoticedTimeStamp;
    CombatTimeStamp fDetectionEventTimeStamp;
    CombatTimeStamp fLastAttackedMemberTimeStamp;
    uint8_t cMemberLOSCount;
    uint8_t cMember360LOSCount;
};

// A cluster of group members: its centre and how many members it holds.
struct CombatGroupCluster {
    CombatGroupCluster();

    // Saves the centre and the member count; apGroup is unused.
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup);

    BGSWorldLocation Center;
    uint32_t iMemberCount;
};

// A member's entry while the group builds its clusters: the member, its location and the index of its closest
// member.
struct ClusterData {
    // Constructs only the location.
    ClusterData();

    CombatMember* pCombatMember;
    BGSWorldLocation WorldLoc;
    int8_t cClosestMember;
};

// One actor in a combat group: its group strategy assignment and the strength and damage the group counts for it.
struct CombatMember {
    // The empty default constructor: it leaves every field as it is.
    CombatMember() {}
    // Constructs the member for apActor in apMemory.
    CombatMember(Actor* apActor);

    // Saves the actor, assignment, damage and strength, then the cluster as its index in apGroup (-1 for none).
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup);
    // Reads what SaveGame writes, resolving the cluster index through apGroup.
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup);
    // Resolves the actor's saved form ID.
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup);
    void CopyData(CombatMember* apCombatMember);

    Actor* pActor;
    uint32_t iGroupStrategyAssignment;
    float fDamagePerSecond;
    float fCombatStrength;
    CombatGroupCluster* pCluster;
};

using BSSimpleArray_CombatTarget_1024 = BSSimpleArray<CombatTarget, 1024>;
using CombatGroupBoundsCornerArray = uint8_t[8][3];

class CombatGroup {
public:
    CombatGroup(uint32_t aiGroupID);
    ~CombatGroup();

    // The detection level the group's attacker apAttacker has of apTarget, with the time it was
    // last detected and whether the target is in line of sight.
    void GetTargetDetectionLevel(
        Actor* apAttacker,
        Actor* apTarget,
        int32_t& ariDetectionLevel,
        CombatTimeStamp& arfLastTargetDetectedTimeStamp,
        bool& arbTargetLineOfSight,
        bool& arbTarget360LineOfSight
    );
    // Removes apActor from the targets.
    void RemoveTarget(Actor* apActor);
    // Removes the target at aiIndex.
    void RemoveTarget(uint32_t aiIndex);
    // The group strategy assignment of apMember.
    uint32_t GetMemberGroupStrategyAssignment(Actor* apMember);
    // The group strategy assignment of the member aiIndex.
    uint32_t GetMemberGroupStrategyAssignment(uint32_t aiIndex);
    CombatMember* GetCombatMember(uint32_t aiIndex);
    // Recounts how many members attack each target.
    void UpdateAttackerCounts();
    // Updates the group.
    void Update();
    // Drops apRef from every member's combat controller, and from the targets when it is an actor.
    void ClearReferencePointers(TESObjectREFR* apRef);
    // Records that apLocation has been searched.
    void InvestigatedSearchLocation(const BGSWorldLocation& arLocation);
    // The first target within afRadius of arWorldLoc.
    Actor* FindTargetNearLocation(const BGSWorldLocation& arWorldLoc, float afRadius, bool abDetectedOnly);
    // Grows the eight box corners apCorners (each coordinate a minimum or a maximum, per BoundsCornerIsMax)
    // around every target's location; unless abAlreadyInitialized, first sets each corner coordinate to -FLT_MAX for a
    // maximum, FLT_MAX for a minimum.
    void ComputeTargetBoundingBox(NiPoint3* apCorners, bool abAlreadyInitialized);
    uint32_t GetTargetCount();
    // Whether every member is fleeing: no member is not fleeing and some member is.
    bool IsFleeing();
    CombatSearchDoor* GetCombatSearchDoor(TESObjectREFR* apDoor);
    CombatTarget* GetCombatTargetForActor(Actor* apActor);
    bool AddMember(Actor* apActor);
    CombatMember* GetCombatMemberForActor(Actor* apActor);
    bool AddTarget(Actor* apActor);
    Actor* GetTarget(uint32_t aiIndex);
    // Tells the combat manager each member is removed, then empties the member array.
    void RemoveAllMembers();
    // The search location aiIndex.
    CombatSearchLocation& GetSearchLocation(uint32_t aiIndex);
    // Clears apCombatController's attacker's strategy assignment when it is the one apGoal was planned for.
    void NotifyMemberPlanFinished(CombatController* apCombatController, CombatGoal* apGoal);
    // Stamps the time and place apAttacker, a target, last attacked a member.
    void NotifyMemberHit(Actor* apMember, Actor* apAttacker);
    // The first member whose strategy assignment is aiAssigment.
    Actor* FindMemberWithStrategyAssignment(uint32_t aiAssigment);
    // Recounts the attackers and drops every member, other than the player, whose combat controller is not in this
    // group.
    void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    // The centre of the group's search.
    BGSWorldLocation GetSearchCenter();
    // The group's search line-of-sight map, through spPathingLOSGridMap's operator->.
    PathingLOSGridMap* GetSearchLOSGridMap();
    bool IsSearching();
    CombatTimer* GetDetectionDialogueTimer(uint32_t aiIndex);
    bool IsTarget(Actor* apActor);
    bool IsMember(Actor* apActor);
    bool CanAddMember(Actor* apActor);
    bool IsEmpty();
    void IncrementAttackerCount(Actor* apTarget);
    void DecrementAttackerCount(Actor* apTarget);
    char GetCombatMusicState();
    void InvestigatedSearchDoor(TESObjectREFR* apDoor);
    void ReserveSearchDoor(TESObjectREFR* apDoor);
    void UnreserveSearchDoor(TESObjectREFR* apDoor);
    CombatGroupCluster* GetClusterForMember(Actor* apMember);
    // The index of apCluster in the cluster array.
    char GetClusterIndex(CombatGroupCluster* apCluster);
    // The cluster at acIndex, or null for a negative index. The
    // caller pushes the index zero-extended.
    CombatGroupCluster* GetCluster(char acIndex);
    void SetSearchingMember(Actor* apMember);
    Actor* GetSearchingMember();
    void SetMemberGroupStrategyAssignment(Actor* apMember, uint32_t auiAssignment);
    // Sets the strategy assignment for member aiIndex.
    void* SetGroupStrategyAssignment(int32_t aiIndex, uint32_t auiAssignment);
    // Removes apActor from the group.
    void RemoveMember(Actor* apActor);
    // Finds a target by form ID.
    CombatTarget* GetCombatTargetForNumericID(uint32_t aiTargetID);
    // Finds a member by form ID.
    CombatMember* GetCombatMemberForNumericID(uint32_t auiRefID);
    // The member at auiIndex.
    Actor* GetMember(uint32_t aiIndex);
    // Whether the members and the targets overlap along the line between their centres.
    bool CheckMemberTargetIntersection();
    // The number of group members.
    uint32_t GetMemberCount();
    // The combat target at auiIndex.
    CombatTarget* GetCombatTarget(uint32_t aiIndex);
    // Finds a member within afRadius of apLocation.
    Actor* FindMemberNearLocation(const BGSWorldLocation& arWorldLoc, float afRadius, Actor* apIgnoreMember);
    // Finds the closest member to apLocation within afRadius.
    Actor* FindClosestMemberToLocation(const BGSWorldLocation& arWorldLoc, Actor* apIgnoreMember, float afMaxDistance);
    bool CanAddTarget(Actor* apActor);
    Actor* FindMemberNearActor(Actor* apActor, float afRadius);
    Actor* FindClosestMemberToActor(Actor* apActor, float afMaxDistance);
    void FailedToInvestigateSearchDoor(TESObjectREFR* apDoor);
    // Clears every member's group strategy assignment.
    void ClearGroupStrategyAssignments();
    // Adds a search location at arLocation, stamped now, for target aiTargetID with score afScore.
    void AddSearchLocation(const BGSWorldLocation& arLocation, float afScore, uint32_t aiTargetID);
    // How much of the search area has been seen, or -1 without a search grid map.
    float GetSearchPercent();
    uint32_t GetSearchCount() {
        return iSearchCount;
    }
    void UpdateCounts();
    bool ShouldPlayCombatMusic();
    char GetClusterCount();
    uint32_t GetNumSearchLocations();
    void FillTargetArray(BSSimpleArray<Actor*, 1024>* apActorArray);
    Actor* GetBestTarget(Actor* apMember);
    void FillMemberArray(BSSimpleArray<Actor*, 1024>* apActorArray);
    void RemoveFromMembers();
    uint8_t GetTargetMemberLOSCount(Actor* apTarget);
    void ComputeSearchArea();
    void BuildSearchTeleportDoorArray();
    void UpdateSearchLocations();
    bool UpdateSearchArea();
    void UpdateGroupStrategy();
    PathingAvoidNodeArray* CreateMovementAvoidNodeArray(
        Actor* apActor,
        const NiPoint3& arDestination,
        float afExtraRadius,
        PathingAvoidNodeArray* apExistingAvoidNodeArray,
        Actor* apIgnoreActor
    );
    PathingAvoidNodeArray* CreateFleeAvoidNodeArray(Actor* apActor, PathingAvoidNodeArray* apExistingAvoidNodeArray);
    double GetMemberCombatStrength(Actor* apMember);
    void UpdateClusters();
    void UpdateClusterCenters();
    void ForceGroupStrategy(CombatGroupStrategy* apStrategy);
    void CopyTargets(CombatGroup* apGroup);
    bool CheckRemoveCombatTarget(CombatTarget* apCombatTarget);
    bool MergeGroup(CombatGroup* apCombatGroup);
    void ClearClusterArray();
    int32_t GetDetectionValue(Actor* apTarget);
    CombatTimeStamp* GetTargetLastDetectedTimeStamp(CombatTimeStamp* apResult, Actor* apTarget);
    // Copies the target's last seen location (16 bytes from CombatTarget+0x28) to arRetLastSeenLocation when it was
    // seen within afMaxTime.
    bool GetTargetLastSeenLocation(uint32_t aiIndex, BGSWorldLocation& arRetLastSeenLocation, float afMaxTime);
    PathingAvoidNodeArray* CreateFlankingAvoidNodeArray(
        Actor* apActor,
        const NiPoint3& arDestination,
        float afExtraRadius,
        PathingAvoidNodeArray* apExistingAvoidNodeArray
    );
    float ComputeMemberBoundingSphere(NiPoint3& arCenter);
    float ComputeTargetBoundingSphere(NiPoint3& arCenter);
    void ComputeMemberBoundingBox(NiPoint3* arPoints, bool abAlreadyInitialized);
    bool CheckGroupStrategyChosenTimeStamp(uint32_t aiIndex, float afMinimumTimePassed);
    float ComputeMemberHealthPercentage();
    void UpdateSearch();
    void StartSearch();
    void GetMinMaxSearchRadius(BGSWorldLocation& arSearchLocation, float& arfMinRadius, float& arfMaxRadius);
    void ClearSearch();
    CombatSearchLocation*
    FindSearchLocation(Actor* apMember, uint8_t acMinScore, BGSWorldLocation* apCurrentSearchLocation);
    void CalculateTargetSearchLocationScores(
        CombatTarget* apCombatTarget,
        float& arfDetectedScore,
        float& arfNoticedScore,
        float& arfAttackedScore
    );
    TESObjectREFR* FindSearchDoor(Actor* apMember, float afMaxDistance, bool abCheckExist);
    void AddSearchDoor(TESObjectREFR* apDoor);
    bool CalculateSearchIgnoreLocations(Actor* apSearcher, BSSimpleArray<NiPoint3, 1024>* apIgnoreLocations);
    void UpdateCombatStrength();
    double GetAverageMemberDamagePerSecond(Actor* apMember);
    Actor* FindClusterPartner(Actor* apMember, Actor* apCurrentPartner, float afMaxDistance);
    void UpdateTargets();
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    // Returns a copy of fSearchStartedTimeStamp.
    CombatTimeStamp GetSearchStartedTimeStamp();
    bool GetTargetLastDetectedLocation(Actor* apTarget, BGSWorldLocation& arLastDetectedLocation);
    void CreateDebugGeometry(bool abClear);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static CombatGroupStrategy* SelectGroupStrategy(CombatGroup* apGroup);

    uint32_t iGroupNum;
    uint32_t iGroupID;
    BSSimpleArray<CombatTarget> TargetArray;
    BSSimpleArray<CombatMember> MemberArray;
    CombatGroupStrategy* pGroupStrategy;
    bool bStrategyForced;
    CombatTimer ChooseStrategyTimer;
    CombatTimer UpdateStrategyTimer;
    uint32_t iLastGroupStrategyChosenIndex;
    CombatTimeStamp fLastGroupStrategyChosenTimeStamp;
    CombatTimer AvoidThreatDialogueTimer;
    CombatTimer CombatStrengthTimer;
    CombatTimer DetectionDialogueTimers[10];
    CombatTimer TargetUpdateTimer;
    CombatTimer ClusterUpdateTimer;
    CombatTimer CombatMusicUpdateTimer;
    float fMemberCombatStrength;
    float fAverageMemberDamagePerSecond;
    float fTargetCombatStrength;
    float fAverageTargetCombatStrength;
    uint32_t iSearchCount;
    NiPointer<PathingLOSGridMap> spPathingLOSGridMap;
    CombatTimer SearchUpdateTimer;
    CombatTimer SearchAreaUpdateTimer;
    CombatTimeStamp fSearchStartedTimeStamp;
    Actor* pSearchingMember;
    BGSWorldLocation SearchCenter;
    NiPoint3 SearchFocalPoint;
    float fSearchRadius;
    BSSimpleArray<CombatSearchLocation> SearchLocations;
    BSSimpleArray<CombatSearchDoor> SearchTeleportDoors;
    NiPointer<NiRefObject> spSearchDebugGeometry;
    bool bUpdateSearchDebugGeometry;
    BSSimpleArray<CombatGroupCluster*> ClusterArray;
    uint32_t iInitializedMemberCount;
    uint32_t iFleeingMemberCount;
    uint32_t iNonFleeingMemberCount;
    uint8_t cCombatMusicState;
    NiPointer<NiNode> spDebugGeometry;

    static inline float fCombatClusterUpdateTime = 1.0f;
    static inline bool bUseSimpleCombatGroupClustering = true;
    static inline float fstaticGatherDistance = 1024.0f;
    static inline float fstaticMaxDistance = 768.0f;
    static inline float fStaticMinDistance = 384.0f;
    static inline bool bUseClustering;
    // Per box corner and coordinate, whether the corner holds the maximum (1) or the minimum (0).
    static CombatGroupBoundsCornerArray BoundsCornerIsMax;
};

inline CombatGroupBoundsCornerArray
    CombatGroup::BoundsCornerIsMax{{1, 1, 1}, {1, 1}, {1, 0, 1}, {1}, {0, 1, 1}, {0, 1}, {0, 0, 1}};

// The game settings CombatGroup.cpp defines.
extern SettingT<INISettingCollection> bDebugCombatSearch;
extern SettingT<INISettingCollection> bDebugCombatGroupClusters;
extern SettingT<INISettingCollection> bEncounterZoneTargetRestrict;
extern SettingT<INISettingCollection> bDebugCombatGroups2;
extern SettingT<INISettingCollection> bDebugCombatGroups;
inline float fClusterDistance = 1024.0f;
using pSearchRadius_t = Setting* [2][3];
extern pSearchRadius_t pSearchRadius;
inline float fFleeingPenalty = 500.0f;
inline float fDyingPenalty = 500.0f;
inline float fCantExistInTargetSpacePenalty = 2500.0f;
inline float fUnreachablePenalty = 2000.0f;
inline float fDistanceBonus = 1000.0f;
inline float fMaxDistance = 2048.0f;
inline float fweaponTypeBonus = 100.0f;
inline float fCurrentTargetLOSFalloffTime = 2.0f;
inline float fCurrentTargetBonus = 100.0f;
inline float fLOSBonus = 100.0f;
inline float f360LOSBonus = 1000.0f;
inline float fShareTargetBonus;
