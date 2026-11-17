#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class CombatCoverLocation;

// The combat procedure that hides from the target: it moves to a hide position (or cover location) and stays there
// for a number of seconds, looking at a position.
class CombatProcedureHideFromTarget : public CombatProcedure {
public:
	enum HIDE_STATE : int32_t {
		HIDE_STATE_INITIALIZING = 0x0,
		HIDE_STATE_MOVING_TO_COVER = 0x1,
		HIDE_STATE_HIDING = 0x2,
	};

	CombatProcedureHideFromTarget(CombatCoverLocation* apCombatCoverLocation, float afNumSeconds, bool abIgnoreLOS,
		bool abClearCoverWhenFinished);
	~CombatProcedureHideFromTarget() override;

	void Update() override;
	void Shutdown() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	HIDE_STATE eState;
	float fNumSecondsToHide;
	CombatTimer HidingTimer;
	NiPoint3 HidePosition;
	NiPoint3 LookAtPosition;
	CombatCoverLocation* pCoverLocation;
	bool bIgnoreLOS;
	bool bClearCoverWhenFinished;
	uint8_t padding3E[2];
};
