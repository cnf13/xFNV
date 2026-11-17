#pragma once

#include "Fallout Shared/Package.h"

class Actor;

// The package of an actor that backs up.
class BackUpPackage : public TESPackage {
public:
	BackUpPackage();
	~BackUpPackage() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	uint16_t GetSaveSize() override;
	void SaveGame() override;
	void LoadGame() override;
	void InitLoadGame() override;
	using TESPackage::InitLoadGame;
	using TESPackage::LoadGame;
	using TESPackage::SaveGame;

	void InitializeBackUpPackage(Actor* apActor, TESObjectCELL* apcell, TESObjectREFR* apRef);
	NiPoint3* GetBackUpPoint(NiPoint3* apResult);

	// The point 300 units from apActor straight away from aptarget, moved onto the navmesh within 200 units when
	// one is found there; zero without either reference.
	static NiPoint3 CalculateBackUpPointExterior(Actor* apActor, TESObjectREFR* aptarget);
	// The point 300 units from apActor straight away from apToAvoid, moved onto the navmesh within 200 units when one
	// is found there; zero without either reference.
	static NiPoint3 CalculateBackUpPointInterior(Actor* apActor, TESObjectREFR* aptarget);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// Saved inline as 12 bytes.
	NiPoint3 pBackUpPoint;
};
