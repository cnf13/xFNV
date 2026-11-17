#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESBoundObject;

// The combat procedure of using a combat item (a stimpak, a grenade, a food item): equips it and plays the use.
class CombatProcedureUseCombatItem : public CombatProcedure {
public:
	CombatProcedureUseCombatItem(TESBoundObject* apItem);
	~CombatProcedureUseCombatItem() override;

	void Update() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	TESBoundObject* pItem;
	bool bAnimating;
	// IDA ends the struct at 0x15; the exe allocates 0x18.
	uint8_t padding15[3];
};
inline constexpr fnvd::Vtable CombatProcedureUseCombatItem_vftable{};
