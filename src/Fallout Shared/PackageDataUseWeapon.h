#pragma once

#include "Fallout/AI/ActorPackageData.h"
#include "Reconstruction.hpp"

class TESObjectREFR;

// The per-actor data of a use weapon package: the reference the actor attacks.
class UseWeaponActorPackageData : public ActorPackageData {
public:
	UseWeaponActorPackageData();
	~UseWeaponActorPackageData() override;

	void HandleDeletedReference(TESObjectREFR* pDeletedObj) override;
	PACKAGE_TYPE GetPackageType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override {}
	// The exe's LoadGame and InitLoadGame entries are one function (0x67DA20).
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// Returns pAttackTarget; the exe's shared one-field getter of +4 (thiscall, ret 0,
	// 0x726070).
	TESObjectREFR* GetAttackTarget();

	// The exe's deleting destructor (0x67D990) frees through ourFree.
	static void operator delete(void* apMemory);

	TESObjectREFR* pAttackTarget;
};
inline constexpr fnvd::Vtable UseWeaponActorPackageData_vftable{};
