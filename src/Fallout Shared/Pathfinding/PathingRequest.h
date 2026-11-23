#pragma once

#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class Actor;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class IDebugText;
class InventoryChanges;
class PathingAvoidNodeArray;
class TESActorBase;

// What a path's doors and locks are judged against: the actor's base, its inventory (for keys) and whether it is
// alarmed.
class ActorLockData {
public:
    ActorLockData() : pActorBase(nullptr), pInventory(nullptr), bIsAlarmed(false) {}
    // The lock data of apActor, built in apMemory.
    ActorLockData(Actor* apActor);

    // Copies the three fields.
    ActorLockData& operator=(const ActorLockData& aData);

    TESActorBase* pActorBase;
    InventoryChanges* pInventory;
    bool bIsAlarmed;
};

// The kind of pathing request, one value per subclass: PathingRequest::CreatePathingRequest switches on it to allocate
// the subclass (case 0 standard, 1 cover, 2 close point, 3 flee, 4 hide, 5 LOS, 7 optimal location, 8 covered move; 6
// is the safe straight line, whose class Pathing::BuildPath builds under its case 6).
enum PATHING_REQUEST_TYPE : int32_t {
    PATHING_REQUEST_STANDARD = 0x0,
    PATHING_REQUEST_COVER = 0x1,
    PATHING_REQUEST_CLOSE_POINT = 0x2,
    PATHING_REQUEST_FLEE = 0x3,
    PATHING_REQUEST_HIDE = 0x4,
    PATHING_REQUEST_LOS = 0x5,
    PATHING_REQUEST_STRAIGHT_LINE = 0x6,
    PATHING_REQUEST_OPTIMAL_LOCATION = 0x7,
    PATHING_REQUEST_COVERED_MOVE = 0x8,
    PATHING_REQUEST_COUNT = 0x9,
};

// The parameters of a path search for an actor: where it starts and ends, the actor's size and what it may cross.
class PathingRequest : public NiRefObject {
public:
    PathingRequest();
    PathingRequest(const PathingRequest& aRequest);
    ~PathingRequest() override;

    // Copies the request's fields into apDest. CopyTo is const in every class of the family; the copy constructors call
    // it on their const source.
    virtual void CopyTo(PathingRequest* apDest) const;
    virtual void CheckValid() const {}
    // The kind of request, one value per subclass: a PATHING_REQUEST_TYPE.
    virtual uint32_t GetType() const;
    // Prints the request's fields, one line each.
    virtual void PrintDebugText(IDebugText* apDebugText);
    virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Sets bWaitIfPlayerWaits from the form's package after a load.
    virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    void SetOrigin(const PathingLocation& aOrigin);
    void SetAllowIncompletePath(bool abAllow);
    void SetRetryCount(int32_t aiRetryCount);
    // Fills the request in for apActor: its location as the origin, its radius, lock data,
    // whether it swims or flies, whether it can open doors and its heading.
    void Initialize(Actor* apActor);
    void SetNoDestination() {
        Destination.SetUnused();
    }
    void SetActorRadius(float aRadius);
    PathingLocation& GetOrigin();
    const PathingLocation& GetOrigin() const;
    PathingLocation& GetDestination();
    const PathingLocation& GetDestination() const;
    float GetActorRadius();
    float GetTargetRadius() {
        return fTargetRadius;
    }
    void SetTargetRadius(float afTargetRadius);
    // Sets fActorHeight.
    void SetActorHeight(float aHeight);
    void SetGoalZDelta(float aZDelta);
    void SetSmoothingRandomness(float afRandomness);
    uint8_t GetAllowIncompletePath() {
        return bAllowIncompletePath;
    }
    void SetGoalAngleToFace(float aTargetAngle);
    float GetGoalAngleToFace() {
        return fGoalAngle;
    }
    bool GetFaceTargetAtGoal() {
        return bFaceTargetAtGoal;
    }
    bool GetCantOpenDoors() {
        return bCantOpenDoors;
    }
    void SetCantOpenDoors(bool abCantOpenDoors);
    PathingAvoidNodeArray* GetAvoidNodeArray() const;
    bool GetCanSwim() const;
    bool GetCanFly() const;
    bool GetTryToUseCurvedPath() const;
    bool GetIgnoreLocks() const;
    bool GetGoDirectIfPathingFails() const;
    void SetCenterRadius(NiPoint3& arCenter, float afOriginRadius);
    void SetDestination(const PathingLocation& aDestination);
    void SetAvoidNodeArray(PathingAvoidNodeArray* apAvoidNodeArray);
    void SetActorLockData(const ActorLockData& aLockData);
    void SetActorLockData(Actor* apActor);
    void SetCanSwim(bool abCanSwim);
    void SetCanFly(bool abCanFly);
    void SetInitialPathHeading(float afInitialPathHeading);
    void SetGoDirectIfPathingFails(bool abGoDirect);
    void SetWaitIfPlayerWaits(bool abAbort);
    void SetSoftGoalRadius(bool abUseSoft);
    void SetIgnoreSmoothingIfFails(bool abIgnoreSmoothingIfFails);
    bool GetIgnoreSmoothingIfFails() const;
    void ClearInitialPathHeading();
    bool GetWaitIfPlayerWaits() const;
    bool GetSoftGoalRadius() const;
    // bFailIfGoalOccupied, through the shared one-byte getter.
    bool GetFailIfGoalOccupied();
    // Only reads Center, and NavMeshSearch::GetNodeConnections calls it on its const request.
    NiPoint3 GetTargetPoint() const;
    bool GetFirstTangentUsesHeading() const;
    // Returns fInitialPathHeading.
    float GetInitialPathHeading() const;

    // The radius requests use when none is set.
    static float GetDefaultRadius();
    static PathingRequest* CreatePathingRequest(PATHING_REQUEST_TYPE aeType);

    Actor* pActor;
    PathingLocation Origin;
    PathingLocation Destination;
    ActorLockData LockData;
    float fActorRadius;
    float fActorHeight;
    float fGoalZDelta;
    float fTargetRadius;
    float fCenterRadius;
    NiPoint3 Center;
    float fSmoothingRandomness;
    float fGoalAngle;
    float fInitialPathHeading;
    NiPointer<PathingAvoidNodeArray> spAvoidNodeArray;
    bool bCantOpenDoors;
    bool bFaceTargetAtGoal;
    bool bDrawDebugGeometry;
    bool bAllowIncompletePath;
    bool bSoftGoalRadius;
    bool bCanSwim;
    bool bCanFly;
    bool bWaitIfPlayerWaits;
    bool bFirstTangentUsesHeading;
    bool bGoDirectIfPathingFails;
    bool bTryToUseCurvedPath;
    bool bIgnoreSmoothingIfFails;
    bool bIgnoreLocks;
    int32_t iRetryCount;
    bool bFailIfGoalOccupied;

    static inline float fDefaultRadius = 25.0f;
};

NiSmartPointer(PathingRequest);

// A request for a point between fMinDistance and fMaxDistance from the destination.
class PathingRequestClosePoint : public PathingRequest {
public:
    // The default constructor: the base's, then the default distances.
    PathingRequestClosePoint();
    PathingRequestClosePoint(const PathingRequestClosePoint& aRequest);
    ~PathingRequestClosePoint() override;

    void CopyTo(PathingRequest* apDest) const override;
    uint32_t GetType() const override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    void SetMinDistance(float afDistance);
    void SetMaxDistance(float afDistance);
    float GetMinDistance() const;
    float GetMaxDistance() const;

    float fMinDistance;
    float fMaxDistance;
};
