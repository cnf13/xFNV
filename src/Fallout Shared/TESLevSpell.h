#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"

class Actor;
class SpellItem;

int32_t retn_minusOne();

// The leveled spell form (LVSP): its leveled list picks the spell it stands for.
class TESLevSpell : public TESBoundObject, public TESLeveledList {
public:
	TESLevSpell();
	~TESLevSpell() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESBoundObject::Save;

	// Picks the form the list gives at asLevel for aeSchool into *apOutForm and its count
	// into *asOutCount.
	void CalculateCurrentForm(MagicSystem::School aeSchool, uint16_t asLevel, TESForm*& apOutForm, uint16_t& asOutCount,
		bool abRecurse);
	// The spell the list gives at asLevel for aeSchool, or null.
	SpellItem* GetLeveledSpell(MagicSystem::School aeSchool, int16_t asLevel);
	BSSimpleList<SpellItem*>* GetLeveledSpells(MagicSystem::School aeSchool, int16_t asLevel);
	// The spells of every school at apActor's levels, merged into a new list.
	BSSimpleList<SpellItem*>* GetLeveledSpells_(Actor* apActor);
};
