#pragma once

#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout Shared/ActorValue.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
char ToggleSelection(ListBox<ActorValue::Index>* apThis, [[maybe_unused]] void* apEdx, Tile* a2);

class CharGenMenu : public Menu {
public:
	// What the menu assigns points to (eItemType). Values from the Xbox PDB.
	enum ITEM_TYPE : int32_t {
		IT_ATTRIBUTE = 0,
		IT_TAG_SKILL = 1,
		NUM_TYPES = 2,
	};

	CharGenMenu();
	~CharGenMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;
	bool DoGamepadUpEvent(int32_t, float) override;

	char HasTiles();
	uint32_t SetupSpecialPoints();
	void SetupSkillPoints();
	void SetDistributePointsString();
	// Inferred type: th_s: Tile* from 0x753ae2 (result of ListBox<ActorValue::Index>::AddItem)
	void SetDisplayString(Tile* th_s);

	// Creates the menu (cdecl, four arguments, 0x753420); Create_ is the exe's wrapper that first checks that the
	// interface manager exists and has been initialised.
	static Tile* Create(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills);
	// IDA name; not in the PDB. Creates the menu once the interface manager is initialised (cdecl, four arguments,
	// 0x7062A0); it returns Create's tile, else null (eax is 0 on both other paths).
	static Tile* Create_(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills);
	// The menu's id, eCharGenMenu.
	static int32_t GetClassStatic();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// PDB name (IDA callback_Tile_T): the attribute list's ListBox::Traverse visitor that DoClick's reset button
	// (id 5, IT_ATTRIBUTE) runs; gives the tile's added points back. aeIndex is not read (cdecl, 0x754330).
	static void ResetAttributesFunc(Tile* apTile, ActorValue::Index aeIndex);
	static void Close();
	// Inferred type: aiSkill: ActorValue::Index: passed to TESClass::IsTagSkill 0 at 0x7543e9
	static void ResetSkillsFunc(Tile* a1, ActorValue::Index aiSkill);
	// Inferred type: a2: ActorValue::Index: passed to PlayerCharacter::SetActorBaseValueF 0 at 0x754518
	static void SetAttributesFunc(Tile* th_s, ActorValue::Index a2);
	// Inferred type: a2: ActorValue::Index: passed to TESClass::SetMajorSkill 1 at 0x75456f
	static void TagSkillsFunc(Tile* th_s, ActorValue::Index a2);

	ITEM_TYPE eItemType;
	Tile* pTiles[7];
	int32_t iCurrPoints;
	int32_t iMaxPoints;
	float fPrevHighlightIndex;
	uint8_t bOnlyAddValues;
	uint8_t bShowTaggedSkills;
	uint8_t padding56[2];
	// PC listboxes are 0x30 bytes, rather than the Xbox PDB's 0x38 bytes.
	ListBox<ActorValue::Index> lbItemList;

	static inline CharGenMenu* pMe;
	static inline int32_t iUT_AddedValue;
};
inline constexpr fnvd::Vtable CharGenMenu_vftable{};

// IDA names; not in the PDB.
inline int32_t g_CharGenMenuTrait_BaseValue;
inline int32_t g_CharGenMenuTrait_ExtraValue;
inline int32_t aiIndex;
inline int32_t g_CharGenMenuTrait_CurrPoints;
inline int32_t g_CharGenMenuTrait_MaxPoints;
inline int32_t g_CharGenMenuTrait_selected;
