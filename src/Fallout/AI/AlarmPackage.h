#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Reconstruction.hpp"

class Actor;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class Crime;
class TESBoundObject;
class TESFaction;
class TESObjectREFR;

// The package of an actor that raises the alarm.
class AlarmPackage : public TESPackage {
public:
	AlarmPackage();
	AlarmPackage(Crime* apCrime);
	~AlarmPackage() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Copy(TESForm* apCopy) override;
	uint16_t GetSaveSize() override;
	void SaveGame() override;
	void LoadGame() override;
	using TESPackage::InitLoadGame;
	using TESPackage::LoadGame;
	using TESPackage::SaveGame;

	void RemoveCrime(Crime* apcrime, Actor* apActor);
	bool IsCrimeInList(Crime* apCrime);
	void AddCrime(Crime* apcrime);

	// The crimes the alarm is raised for; the exe empties it with the shared pointer-list RemoveAll (0x470470).
	BSSimpleList<Crime*> Crimes;
};

class Crime {
public:
	Crime();
	Crime(uint32_t aeCrime, TESObjectREFR* apTarget, Actor* apActor, TESBoundObject* apObject, int32_t aiNumber,
		TESForm* apOwnership);
	~Crime();

	// Resolves the crime's form IDs after a load (thiscall, ret 4, 0x9EC400). IDA name; not in the PDB.
	void InitItem(BGSLoadGameBuffer* apLoadGameBuffer);
	// ecrime. The exe calls it out of line.
	PackageNS::CRIME_TYPE GetCrimeType() { return ecrime; }
	// pOwnership. The exe calls it out of line.
	TESForm* GetOwnershipCrime() { return pOwnership; }
	bool DoesActorKnowaboutCrime(Actor* apActor);
	bool ShouldCrimeBeRemoved();
	void ClearActorKnowList();
	// Returns fCrimeTimeStamp through the hidden result pointer.
	CombatTimeStamp* GetTimeStamp(CombatTimeStamp* apResult);
	void AddtoActorKnowList(Actor* apActor);
	void RemoveActorKnowList(Actor* apActor);
	int32_t GetRefCount();
	void AddEnemyFactions(TESFaction* apFaction);
	uint16_t GetSaveSize();
	void SaveGame();
	// Inferred type: a2: BGSSaveGameBuffer* from 0x9758f6 (parameter 0 of ProcessLists::SaveGame)
	void Save(BGSSaveGameBuffer* a2);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// The exe compares it signed (jge 0x9EB904, Crime::ShouldCrimeBeRemoved); the PDB has int.
	int32_t iRefCount;
	PackageNS::CRIME_TYPE ecrime;
	TESObjectREFR* pcrimetarget;
	Actor* pcriminal;
	bool breported;
	uint8_t padding11[3];
	TESBoundObject* pObjectStolen;
	uint32_t inumberstolen;
	// Witness list element type is not yet reconstructed.
	BSSimpleList<Actor*> pActorsknowofCrime;
	TESForm* pOwnership;
	uint32_t iCrimeNumber;
	// Whether a guard has responded to the crime. The exe saves and loads this one byte: Crime::SaveGame (0x9EBCA0),
	// Crime::Save (0x9EBFE0) and the load (0x9EC1A0) all pass a size of 1, so the word IDA sees here is 1 + padding.
	bool bguardrespond;
	uint8_t padding2D[3];
	// Packed date (year << 13 | month << 9 | day); the PDB has int, and Crime::ShouldCrimeBeRemoved compares the
	// difference signed (jl 0x9EB8F3).
	int32_t iCrimeStamp;
	CombatTimeStamp fCrimeTimeStamp;
	// Allocated on first use (Crime::AddEnemyFactions 0x9EBAE0); the destructor (0x9EB660) reads TESFactions from it.
	BSSimpleList<TESFaction*>* pListofEnemyFactions;
};

inline constexpr fnvd::Vtable AlarmPackage_vftable{};
