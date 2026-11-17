#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMenu/IDebugText.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class Actor;
class ActorMover;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class PathingRequest;
class PathingSolution;
class TESObjectREFR;

class ActorPathHandler {
public:
	// What a handler's update reports: no path (-1), moving on it, a failure, or the path done. The exe's bodies
	// return it in eax: DetailedActorPathHandler::CheckGoalReached (0x9E8000) returns 0 (xor eax,eax at 0x9E8221)
	// and 5 (mov eax,5 at 0x9E822E), and ActorMover's caller compares the result with 5 and 6.
	enum PATH_HANDLER_RESULT : int32_t {
		PATH_HANDLER_RESULT_NONE = -1,
		PATH_HANDLER_RESULT_OK = 0,
		PATH_HANDLER_RESULT_CANT_MOVE = 1,
		PATH_HANDLER_RESULT_CANT_REACH_Z = 2,
		PATH_HANDLER_RESULT_CANT_USE_DOOR = 3,
		PATH_HANDLER_RESULT_FELL_OFF_PATH = 4,
		PATH_HANDLER_RESULT_DONE = 5,
		PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL = 6,
	};

	ActorPathHandler(Actor* apActor, ActorMover* apActorMover);
	ActorPathHandler(Actor* a1, ActorMover* a2, NiPointer<PathingSolution>& a3, PathingRequest* a4);
	virtual ~ActorPathHandler();

	virtual ActorPathHandler::PATH_HANDLER_RESULT Init(float afPercentTraveled, float afDeltaT) = 0;
	virtual bool InitRotationOnly(float fTargetAngle, bool abForceTurn);
	virtual ActorPathHandler::PATH_HANDLER_RESULT Update(float afDeltaT) = 0;
	virtual ActorPathHandler::PATH_HANDLER_RESULT NoPathUpdate(float afDeltaT);
	virtual ActorPathHandler::PATH_HANDLER_RESULT UpdateWhilePlayerWaits(float afDeltaT) = 0;
	virtual float GetDistanceTraveled() = 0;
	virtual float GetPercentTraveled() = 0;
	virtual NiPoint3 GetNextNodeLocation() = 0;
	virtual bool GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) = 0;
	virtual float GetDistanceRemaining() = 0;
	virtual NiPoint3 ComputeApproximateLocation(float afDeltaT) = 0;
	virtual uint32_t GetCurrentNodeIndex() = 0;
	virtual bool IsDetailedPathHandler() = 0;
	virtual void SetWaitForUpdatedPath(PathingRequest* apRequest) {}
	virtual bool IsWaitingForAvoidance();
	virtual bool IsMovementBlocked();
	virtual bool ComputePathPassesThroughPlayersArea();
	virtual uint32_t GetMoveMode() = 0;
	virtual bool GetMoveDelta(NiPoint3& aDelta) = 0;
	virtual bool GetNextTetherPoint(NiPoint3& arPoint);
	virtual void ForceMoveMode(uint32_t aiMoveMode) {}
	virtual void HandlePreviousPathTransition(ActorPathHandler* apOldHander) {}
	virtual void HandleDeletedReference(TESObjectREFR* apRef) {}
	virtual void PrintCurrentStatus(IDebugText* apDebugText, DEBUGTEXT_LOCATION aLoc, DEBUGTEXT_JUSTIFY aJust) = 0;
	virtual void UpdateDebugGeometry() {}
	virtual void SetLookAtTarget(NiPoint3 aPoint) {}
	virtual void ClearLookAtTarget() {}
	virtual void ClearMovement() = 0;
	virtual bool WarpToNextNode();
	virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) = 0;
	virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) = 0;
	virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) = 0;

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	Actor* pActor;
	ActorMover* pActorMover;
	NiPointer<PathingSolution> spPath;
	NiPointer<PathingRequest> spRequest;
};

inline constexpr fnvd::Vtable ActorPathHandler_vftable{};
