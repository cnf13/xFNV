#pragma once

#include "Fallout/AI/ActorPackageData.h"

class TESObjectREFR;

// The per-actor data of a use weapon package: the reference the actor attacks.
class UseWeaponActorPackageData : public ActorPackageData {
public:
	UseWeaponActorPackageData();
	~UseWeaponActorPackageData() override;

	void HandleDeletedReference(TESObjectREFR* pDeletedObj) override;
	PACKAGE_TYPE GetPackageType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override {}
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	TESObjectREFR* GetAttackTarget();

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	TESObjectREFR* pAttackTarget;
};
