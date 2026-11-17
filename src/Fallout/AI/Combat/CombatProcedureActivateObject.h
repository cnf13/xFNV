#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout/AI/Combat/CombatProcedure.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

// The combat procedure that walks to an object and activates it (a combat item or a door in the way).
class CombatProcedureActivateObject : public CombatProcedure {
public:
	CombatProcedureActivateObject(TESObjectREFR* apTargetRef, TESBoundObject* apBoundObject);
	~CombatProcedureActivateObject() override;

	void Update() override;
	void ClearReferencePointers(TESObjectREFR* apRef) override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	TESObjectREFR* pTargetRef;
	TESBoundObject* pBoundObject;
	uint8_t cAnimating;
	uint8_t padding19[3];
};
