#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/Combat/CombatTimer.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

class CombatProcedureSearch : public CombatProcedure {
public:
	// Values from the Xbox PDB.
	enum SEARCH_STATE : int32_t {
		SEARCH_STATE_INITIALIZING = 0x0,
		SEARCH_STATE_INVESTIGATING = 0x1,
		SEARCH_STATE_SEARCHING_DOOR = 0x2,
		SEARCH_STATE_SEARCHING = 0x3,
		SEARCH_STATE_LOOKING_DOOR_FADE = 0x4,
		SEARCH_STATE_LOOKING_DOOR = 0x5,
		SEARCH_STATE_LOOKING = 0x6,
		SEARCH_STATE_LOOKING_AROUND_NEW_SPACE = 0x7,
		SEARCH_STATE_MOVING_TO_SEARCH_CENTER = 0x8,
	};

	CombatProcedureSearch();
	~CombatProcedureSearch() override;

	void Update() override;
	void Shutdown() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	void ActivatedReference(TESObjectREFR* apRef, TESBoundObject* apObject, int32_t aiCount,
		bool abActivatedScriptOnly) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	SEARCH_STATE eState;
	CombatTimer CheckDestinationTimer;
	CombatTimer LookingTimer;
	CombatTimer CheckSearchDoorTimer;
	BGSWorldLocation InvestigateLocation;
	TESObjectREFR* pSearchDoor;
};
inline constexpr fnvd::Vtable CombatProcedureSearch_vftable{};
