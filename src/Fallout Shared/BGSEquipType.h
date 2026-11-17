#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSMenuIcon;
class TESForm;

// How an item is equipped.
class BGSEquipType : public BaseFormComponent {
public:
	// The menu icon form of each equip type.
	using MenuIconArray = BGSMenuIcon* [14];
	using pEquipTypeNames_t = char* [14];

	enum EQUIP_TYPE : int32_t {
		ET_NONE = -1,
		ET_BIG_GUN = 0,
		ET_ENERGY_WEAPON = 1,
		ET_SMALL_GUN = 2,
		ET_MELEE_WEAPON = 3,
		ET_UNARMED_WEAPON = 4,
		ET_THROWN_WEAPON = 5,
		ET_MINE = 6,
		ET_BODY_WEAR = 7,
		ET_HEAD_WEAR = 8,
		ET_HAND_WEAR = 9,
		ET_CHEMS = 10,
		ET_STIMPACK = 11,
		ET_FOOD = 12,
		ET_ALCOHOL = 13,
		ET_COUNT = 14,
	};

	BGSEquipType();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	void Save();
	// Stores eEquipType; the exe's shared one-field setter (thiscall, ret 4, 0x6ECD40).
	void SetEquipType(EQUIP_TYPE aeType);

	// Creates the default menu icon forms; cdecl at 0x479570.
	static void GenerateDefaultMenuIcons();

	// The equip type of apForm, or ET_NONE.
	static EQUIP_TYPE GetEquipType(TESForm* apForm);

	// The menu icon form of equip type aeType, or null.
	static BGSMenuIcon* GetEquipTypeIcon(EQUIP_TYPE aeType);
	// The icon file of equip type aeType's menu icon, or the shared empty string without one.
	static const char* GetEquipTypeIconFilename(EQUIP_TYPE aeType);

	EQUIP_TYPE eEquipType;

	static inline MenuIconArray pEquipTypeMenuIcons{};
	static inline pEquipTypeNames_t pEquipTypeNames{const_cast<char*>("BigGuns"), const_cast<char*>("EnergyWeapons"),
		const_cast<char*>("SmallGuns"), const_cast<char*>("MeleeWeapons"), const_cast<char*>("UnarmedWeapons"),
		const_cast<char*>("ThrowWeapons"), const_cast<char*>("Mine"), const_cast<char*>("BodyWear"),
		const_cast<char*>("HeadWear"), const_cast<char*>("HandWear"), const_cast<char*>("Chems"),
		const_cast<char*>("Stimpack"), const_cast<char*>("Food"), const_cast<char*>("Alcohol")};
};
inline constexpr fnvd::Vtable BGSEquipType_vftable{};
