#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Package.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BaseProcess;
class PathingAvoidNodeArray;
class TESObjectREFR;

// The package of an actor that flees.
class FleePackage : public TESPackage {
public:
	FleePackage(TESObjectREFR* apActor, TESObjectCELL* pcell, TESObjectREFR* apRef);
	~FleePackage() override;

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

	void ModFleeTimer(float aftime);
	void SetKnowsTarget(bool abflag);
	// fFleeTimer; the exe's float getter at +0x90, registered in src as
	// BGSProjectile::GetFadeDuration (thiscall, ST0, 0x6CA4E0).
	float GetFleeTimer();
	// pRefFleeTo (+0xA4); the exe's shared pointer getter it folds with
	// InterfaceManager::GetDebugTextRoot (thiscall, 0x5E3FC0).
	TESObjectREFR* GetRefFleeTo();
	// Stores pActorToFleeTo.
	void SetRefFleeTo(TESObjectREFR* apref) { pRefFleeTo = apref; }
	// bFleeSucceeded; the exe's shared bool setter at +0x94 it folds with
	// bhkCharacterControllerCinfo::SetAvoidBox (thiscall, ret 4, 0x810530).
	void SetFleeSucceeded(bool abflag);
	// Rebuilds the list of avoided refs from the process.
	void UpdateAvoidedRefList(BaseProcess* apProcess);
	// The avoided ref closest to apFromRef, or null.
	TESObjectREFR* GetClosestAvoidedRef(TESObjectREFR* apFromRef);
	// Finds a teleport door away from apThreat; result al (thiscall, ret 8, 0x9F1140).
	bool FindTeleportDoor(Actor* apActor, TESObjectREFR* apThreat);
	// The avoided refs as a pathing avoid-node array.
	PathingAvoidNodeArray* CreateAvoidNodeArray();
	char FleeSucceeded();
	bool IsFleeListEmpty();
	void RemoveAvoidedRef(TESObjectREFR* apRef);
	bool ShouldShutDown();
	// Stores the caller's byte as it is into the bool field.
	void SetEvaluatePoint(bool abEvaluatePoint);
	void AddAvoidedRef(TESObjectREFR* apRef);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static bool EvaluateReferenceforTeleportDoor(TESObjectREFR* apRef, void* aiFilterData);

	bool bEvaluatePoint;
	bool bCombatMode;
	uint8_t padding82[0x2];
	NiPoint3 kFleePoint;
	float fFleeTimer;
	uint8_t bFleeSucceeded;
	uint8_t padding95[0x3];
	// The references to avoid.
	BSSimpleList<TESObjectREFR*> kAvoidRefs;
	TESObjectREFR* pTeleportDoor;
	// The PDB's TESObjectREFR* pRefFleeTo: SetRefFleeTo stores a reference here and the load (0x9F25F0) casts the
	// form to TESObjectREFR.
	TESObjectREFR* pRefFleeTo;
	bool bknowstarget;
	bool busehorse;
	uint8_t paddingAA[0x2];
};

inline TESObjectREFR* pcombatActor;
inline constexpr fnvd::Vtable FleePackage_vftable{};
