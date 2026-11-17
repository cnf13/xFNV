#pragma once

#include "Fallout/AI/Movement/ActorPathHandler.h"

class VirtualActorPathHandler : public ActorPathHandler {
public:
	VirtualActorPathHandler(Actor* a1, ActorMover* a2, NiPointer<PathingSolution>& a3, PathingRequest* a4);
	~VirtualActorPathHandler() override;

	ActorPathHandler::PATH_HANDLER_RESULT Init(float afPercentTraveled, float afDeltaT) override;
	ActorPathHandler::PATH_HANDLER_RESULT Update(float afDeltaT) override;
	ActorPathHandler::PATH_HANDLER_RESULT UpdateWhilePlayerWaits(float afDeltaT) override;
	float GetDistanceTraveled() override { return fDistanceTraveled; }
	float GetPercentTraveled() override;
	NiPoint3 GetNextNodeLocation() override;
	bool GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) override;
	float GetDistanceRemaining() override;
	NiPoint3 ComputeApproximateLocation(float afDeltaT) override;
	uint32_t GetCurrentNodeIndex() override { return iCurrentNodeIndex; }
	bool IsDetailedPathHandler() override;
	uint32_t GetMoveMode() override;
	bool GetMoveDelta(NiPoint3& aDelta) override;
	void PrintCurrentStatus(IDebugText* apDebugText, DEBUGTEXT_LOCATION aLoc, DEBUGTEXT_JUSTIFY aJust) override;
	void ClearMovement() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override {}

	int32_t UseDoorAtNode(NiPoint3& aNewLoc);

	uint32_t iCurrentNodeIndex;
	float fDistanceTraveled;
	float fPathLength;
};
inline constexpr fnvd::Vtable VirtualActorPathHandler_vftable{};
