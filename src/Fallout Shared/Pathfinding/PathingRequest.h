#pragma once

#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

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
	// What the exe's EXTERIOR_DATA constructor (0x540720), folded with this class's, does to PathingRequest's
	// LockData: all three fields zeroed.
	ActorLockData() : pActorBase(nullptr), pInventory(nullptr), bIsAlarmed(false) {}
	// The lock data of apActor, built in apMemory.
	ActorLockData(Actor* apActor);

	// Copies the three fields.
	ActorLockData& operator=(const ActorLockData& aData);

	TESActorBase* pActorBase;
	InventoryChanges* pInventory;
	bool bIsAlarmed;
};

// The kind of pathing request, one value per subclass: PathingRequest::CreatePathingRequest (0x6E2F80) switches on
// it to allocate the subclass (case 0 standard, 1 cover, 2 close point, 3 flee, 4 hide, 5 LOS, 7 optimal location,
// 8 covered move; 6 is the safe straight line, whose class Pathing::BuildPath 0x6D0900 builds under its case 6).
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
	// The default constructor.
	PathingRequest();
	PathingRequest(const PathingRequest& aRequest);
	~PathingRequest() override;

	// The exe's slots after NiRefObject's two, in vtable order.
	// Copies the request's fields into apTarget. The PDB's CopyTo is const in every class of the family; the copy
	// constructors (0x6E4D20 and the others) call it on their const source.
	// thiscall, ret 4, 0x6E2750.
	virtual void CopyTo(PathingRequest* apDest) const;
	virtual void CheckValid() const {}
	// The kind of request, one value per subclass: a PATHING_REQUEST_TYPE. PDB: const, as every override.
	virtual uint32_t GetType() const;
	// Prints the request's fields, one line each.
	virtual void PrintDebugText(IDebugText* apDebugText);
	// Writes the request to a save game (IDA name Write).
	// thiscall, ret 4, 0x6E3230.
	virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Reads the request back from a save game (IDA name Read).
	// thiscall, ret 4, 0x6E34F0.
	virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Sets bWaitIfPlayerWaits from the form's package after a load (IDA name Unk_08/Func0008).
	virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	// Sets the start of the path to aOrigin (thiscall, ret 4, 0x6D3AC0). No caller reads the eax the exe leaves
	// (PathingLocation::operator='s result).
	void SetOrigin(const PathingLocation& aOrigin);
	void SetAllowIncompletePath(bool abAllow);
	void SetRetryCount(int32_t aiRetryCount);
	// Fills the request in for apActor: its location as the origin, its radius, lock data,
	// whether it swims or flies, whether it can open doors and its heading.
	void Initialize(Actor* apActor);
	void SetNoDestination() { Destination.SetUnused(); }
	void SetActorRadius(float aRadius);
	// Origin; the exe's body is a shared "this + 0xC" getter (thiscall, 0x48D150).
	PathingLocation& GetOrigin();
	// The PDB's const overload; the same exe getter (0x48D150).
	const PathingLocation& GetOrigin() const;
	// Destination; the exe's body is a shared "this + 0x34" getter (thiscall, 0x6A9540).
	PathingLocation& GetDestination();
	// The PDB's const overload; the same exe getter (0x6A9540).
	const PathingLocation& GetDestination() const;
	// fActorRadius; the exe's body is a shared float getter (thiscall, ST0, 0x69EF80).
	float GetActorRadius();
	// Not named in the PDB or IDA; name chosen here. fTargetRadius (+0x74); the exe calls that shared float getter
	// (ST0).
	float GetTargetRadius() { return fTargetRadius; }
	// fTargetRadius; the exe's shared float setter at +0x74 it folds with
	// TESObjectTREE::SetMaximumLeafAngle (thiscall, ret 4, 0x6D3B00).
	void SetTargetRadius(float afTargetRadius);
	// fActorHeight (+0x6C); the exe calls that shared float setter.
	void SetActorHeight(float aHeight);
	// fGoalZDelta (+0x70); the exe calls that shared float setter.
	void SetGoalZDelta(float aZDelta);
	// fSmoothingRandomness (+0x88); the exe calls that shared float setter.
	void SetSmoothingRandomness(float afRandomness);
	// Not named in the PDB or IDA; name chosen here. bAllowIncompletePath (+0x9B); the exe calls that shared byte
	// getter.
	uint8_t GetAllowIncompletePath() { return bAllowIncompletePath; }
	void SetGoalAngleToFace(float aTargetAngle);
	// The exe calls that shared getter of fGoalAngle (+0x8C).
	float GetGoalAngleToFace() { return fGoalAngle; }
	// The exe calls that shared getter of bFaceTargetAtGoal (+0x99).
	bool GetFaceTargetAtGoal() { return bFaceTargetAtGoal; }
	// The exe calls that shared getter of bCantOpenDoors (+0x98).
	bool GetCantOpenDoors() { return bCantOpenDoors; }
	// bCantOpenDoors; the exe's shared bool setter at +0x98 it folds with
	// bhkCharacterControllerCinfo::SetRoll (thiscall, ret 4, 0x6E2B50).
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
	// IDA name; not in the PDB. The exe's body only reads Center (0x6A7910), and NavMeshSearch::GetNodeConnections
	// calls it on its const request.
	NiPoint3* GetTargetPoint(NiPoint3* a2) const;
	// bFirstTangentUsesHeading (thiscall, ret 0, 0x6CA4C0: mov al, [ecx+0xA0]).
	bool GetFirstTangentUsesHeading() const;
	// fInitialPathHeading (+0x90); the exe calls the shared float getter (thiscall, ST0, 0x6CA4E0).
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
	// From Construct_006E3FB0.
	PathingRequestClosePoint(const PathingRequestClosePoint& aRequest);
	~PathingRequestClosePoint() override;

	void CopyTo(PathingRequest* apDest) const override;
	uint32_t GetType() const override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// The exe's shared float setters at +0xB0 and +0xB4 (thiscall, ret 4, 0x507610 and
	// 0x6E5EE0).
	void SetMinDistance(float afDistance);
	void SetMaxDistance(float afDistance);
	// fMinDistance; the exe's body is a float getter (thiscall, ST0, 0x6D2C20).
	float GetMinDistance() const;
	// fMaxDistance; the exe's body is a shared float getter (thiscall, ST0, 0x6D2C40).
	float GetMaxDistance() const;

	float fMinDistance;
	float fMaxDistance;
};

inline constexpr fnvd::Vtable PathingRequest_vftable{};
inline constexpr fnvd::Vtable PathingRequestClosePoint_vftable{};
