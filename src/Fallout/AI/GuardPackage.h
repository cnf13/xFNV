#pragma once

#include "Fallout Shared/PackageData.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Reconstruction.hpp"

class Actor;

// The run-time data of a guard package: the guard's state, the actor it warned, the warning count and the timer.
class GuardActorPackageData : public ActorPackageData {
public:
	// Values from the Xbox PDB.
	enum GUARD_STATE : int32_t {
		GUARD_STATE_INIT = 0x0,
		GUARD_STATE_IDLE = 0x1,
		GUARD_STATE_WAIT_TO_ATTACK = 0x2,
	};

	GuardActorPackageData();
	~GuardActorPackageData() override;

	void HandleDeletedReference(TESObjectREFR* pDeletedObj) override;
	PACKAGE_TYPE GetPackageType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	void IncWarningCount();
	void ClearWarningCount();
	// Inferred type: a2: float: used as one at 0x90316d
	void UpdateWarningTimer(float a2);
	void SetTarget(Actor* a2);
	void ClearWarningTimer();
	// Stores eState.
	void SetState(GUARD_STATE aeState) { eState = aeState; }

	GUARD_STATE eState;
	Actor* pGuardWarnedActor;
	int32_t iWarningCount;
	float fGuardTimer;
};

// A guard package's type data: how long the guard waits between warnings, how many it gives, and whether its target
// comes from a linked ref. The exe never builds one (it has no RTTI, vtable or constructor of this class), so the
// shape is the PDB's and only TESPackage::GetGuardPackageData's declaration needs the type.
class GuardPackageData : public TESPackageData {
public:
	~GuardPackageData() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	float fTimeBetweenWarnings;
	int32_t iWarningMaxCount;
	bool bGuardLinkedRef;
};

inline constexpr fnvd::Vtable GuardActorPackageData_vftable{};
