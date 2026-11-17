#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Reconstruction.hpp"

class BGSProjectile;
class TESAmmoEffect;

// The ammo record's DATA: the projectile speed and the ammo flags.
struct AMMO_DATA {
	// Byte-swaps the speed dword.
	void Endian();

	float fSpeed;
	uint8_t cFlags;
	uint8_t padding05[3];
};

// The ammo record's New Vegas data: projectiles per round, the projectile and weight overrides, and the object a
// spent round leaves (with its chance).
struct AMMO_DATA_NV {
	// Byte-swaps the two float fields read from a big-endian file (same code as AIDATA::Endian, thiscall, 0x503240).
	void Endian();

	uint8_t cProjectilePerAmmo;
	uint8_t padding01[3];
	BGSProjectile* pProjectileOverride;
	float fWeightOverride;
	TESForm* pAmmoConsumedObject;
	float fAmmoConsumedChance;
};

// The base form of ammunition. Of its non-virtual methods, only those the reconstructed code uses are declared.
class TESAmmo : public TESBoundObject,
				public TESFullName,
				public TESModelTextureSwap,
				public TESIcon,
				public BGSMessageIcon,
				public TESValueForm,
				public BGSClipRoundsForm,
				public BGSDestructibleObjectForm,
				public BGSPickupPutdownSounds,
				public TESScriptableForm {
public:
	TESAmmo();
	~TESAmmo() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	// also override TESValueForm's slots of the same signatures
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;

	using TESBoundObject::Clone3D;
	using TESForm::Save;

	// The short name, the string at +0xC4.
	const char* GetShortName();
	// PDB name by its parameters (IDA GetEffectNames). Writes the names of the ammo's effects
	// into astrResult (auiResultLen bytes) (thiscall, ret 8, 0x503A70).
	void BuildMenuString(char* astrResult, uint32_t auiResultLen);
	const char* GetAbbrev();
	// NVdata.pProjectileOverride, through the exe's shared field accessor (thiscall, ret 0,
	// 0x8D80E0)
	BGSProjectile* GetProjectile();
	// NVdata.pProjectileOverride. A trivial setter the exe calls out of line (thiscall, ret 4).
	void SetProjectile(BGSProjectile* apProjectile) { NVdata.pProjectileOverride = apProjectile; }
	// The projectiles each round fires (NVdata.cProjectilePerAmmo).
	uint8_t GetNumProjectiles();
	// Whether the non-playable flag of data.cFlags is clear.
	bool GetIsPlayable();
	float GetWeight() { return NVdata.fWeightOverride; }
	// Out of line: the exe calls it (thiscall, ret 0, 0x923000).
	BSSimpleList<TESAmmoEffect*>* GetAmmoEffectList();
	// NVdata.pAmmoConsumedObject. The exe calls it out of line (thiscall, ret 0).
	TESForm* GetAmmoConsumedForm() { return NVdata.pAmmoConsumedObject; }
	// NVdata.fAmmoConsumedChance. The exe calls it out of line (thiscall, ret 0, ST0 float).
	float GetAmmoConsumedPercent() { return NVdata.fAmmoConsumedChance; }
	// Sets the object a spent round leaves, NVdata.pAmmoConsumedObject.
	void SetAmmoConsumedForm(TESForm* apAmmoConsumed);

	AMMO_DATA data;
	AMMO_DATA_NV NVdata;
	// The short name and the abbreviation the HUD shows next to the ammo count.
	BSStringT<char> cDesc;
	BSStringT<char> cAbbrev;
	BSSimpleList<TESAmmoEffect*> pAmmoEffectList;
};
inline constexpr fnvd::Vtable TESAmmo_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESAmmo_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESAmmo_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESAmmo_vftable_BGSClipRoundsForm{};
inline constexpr fnvd::Vtable TESAmmo_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESAmmo_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESAmmo_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESAmmo_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESAmmo_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESAmmo_vftable{};

// The game settings TESAmmo.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> iMaxArrowsInQuiver;
