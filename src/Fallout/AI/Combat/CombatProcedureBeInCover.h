#pragma once

#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimer.h"

class CombatCoverLocation;

class CombatProcedureBeInCover : public CombatProcedure {
public:
	// Values from the Xbox PDB.
	enum COVER_STATE : int32_t {
		COVER_STATE_INITIALIZING = 0x0,
		COVER_STATE_WAITING_BEHIND_COVER = 0x1,
		COVER_STATE_MOVING_OUT = 0x2,
		COVER_STATE_WAITING_OUT_OF_COVER = 0x3,
		COVER_STATE_FIRING_OUT_OF_COVER = 0x4,
		COVER_STATE_MOVING_IN = 0x5,
		COVER_STATE_MOVING_IN_AND_ROTATE = 0x6,
		COVER_STATE_HOLDING_GROUND = 0x7,
	};

	CombatProcedureBeInCover(CombatCoverLocation* apCombatCoverLocation);
	~CombatProcedureBeInCover() override;

	void Update() override;
	void Shutdown() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	void FiredProjectile(Projectile* apProjectile) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	bool GetBehindCover();

	COVER_STATE eState;
	CombatCoverLocation* pCoverLocation;
	NiPoint3 CurrentCoverPosition;
	NiPoint3 CurrentFirePosition;
	NiPoint3 LookAtPosition;
	CombatTimer WaitBehindCoverTimer;
	CombatTimer WaitingOutOfCoverTimer;
	CombatTimer FiringOutOfCoverTimer;
	CombatTimer ReevaluateCoverTimer;
	CombatTimer HoldingGroundTimer;
	CombatTimer TargetBlockedTimer;
};
inline constexpr fnvd::Vtable CombatProcedureBeInCover_vftable{};
