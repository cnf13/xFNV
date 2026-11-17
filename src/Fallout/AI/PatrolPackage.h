#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/ActorPackageData.h"

class Actor;
class TESObjectREFR;

// The per-actor data of a patrol package. Only some of its members are declared.
class PatrolActorPackageData : public ActorPackageData {
public:
	enum PATROL_STATE : int32_t {
		PATROL_STATE_INIT = 0,
		PATROL_STATE_TRAVELING_TO_NEXT_REF = 1,
		PATROL_STATE_AT_REF = 2,
		PATROL_STATE_DEBUG_ACTOR_DELETED = 3,
		PATROL_STATE_DEBUG_DESTRUCTED = 4,
	};

	PatrolActorPackageData();

	PACKAGE_TYPE GetPackageType() override { return PACKAGE_TYPE_PATROL; }
	void HandleDeletedReference(TESObjectREFR* apDeletedObj) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	~PatrolActorPackageData() override;

	// Sets the data up for apActor running apPackage.
	bool Init(Actor* apActor, TESPackage* apPackage);
	// Picks the next patrol point.
	bool SetNextReferenceForPatrol(bool abWaitForOccupiedPoints);
	// Releases the patrol point the actor reserved.
	void ClearReservedPatrolRef();
	// The patrol point the actor is heading to.
	TESObjectREFR* GetCurrentReference();
	// Builds kReferences from the linked references of the initial reference.
	bool BuildReferenceArray();
	// Starts the patrol at the point nearest to arPoint.
	void ResumeRepeatablePatrol(const NiPoint3* arPoint);
	bool SetCurrentReference(uint32_t aiRefIndex);

	static TESObjectREFR* GetInitialReferenceForSetup(Actor* apActor, TESPackage* apPack);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	PATROL_STATE eState;
	float fCurrentTimeAtReference;
	bool bPatrolDirection;
	uint8_t padding0D[3];
	Actor* pActor;
	TESPackage* pPackage;
	const TESPatrolPackageData* pPackageData;
	BSSimpleArray<TESObjectREFR*> References;
	bool bCircularArray;
	uint8_t padding2D[3];
	uint32_t iLoopAroundIndex;
	uint32_t iCurrentReference;
};
