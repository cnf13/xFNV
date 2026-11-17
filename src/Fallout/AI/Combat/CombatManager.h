#pragma once

#include "BSMain/Setting.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

class Actor;
class BGSExplosion;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class CombatController;
class CombatCoverMap;
class CombatGroup;
class CombatTargetMap;
class CombatThreatMap;
class NiNode;
class NiPoint3;
template <class TKEY, class TVAL> class NiTMap;
class TESObjectREFR;
class TESPackage;

// The combat bookkeeping shared by every actor in combat.
class CombatManager {
public:
	CombatManager();
	~CombatManager();

	// False (thiscall, ret 8). The PDB names CombatManager::CheckShouldFlee(apAttacker, apTarget); the spectator update
	// (0x8DB810) calls it.
	bool CheckShouldFlee(Actor* apAttacker, Actor* apTarget);
	// The exe calls the shared one-field getter 0x9611E0 (LockFreeMap<int64_t, NiPointer<BSTask<int64_t>>>::GetCount),
	// a trivial accessor: written as the field.
	CombatCoverMap* GetCoverMap() { return pCoverMap; }
	// The exe calls that shared one-field getter (pThreatMap).
	CombatThreatMap* GetThreatMap() { return pThreatMap; }
	// Empty in the exe (thiscall, ret 8).
	void NotifyCombatGroupRemovedMember(CombatGroup* apGroup, Actor* apMember) {}
	// Rebuilds the local actor targeting map before a load.
	void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// The combat update.
	void Update(bool abTimeStopped);
	// Runs every combat group member's combat update; a member count that changes mid-loop restarts at the same index.
	void UpdateCombatants();
	// The main-thread part of the combat update.
	void UpdateMainThread(bool abTimeStopped);
	CombatGroup* CreateCombatGroup();
	// Splits a combat group off the player's for apMember.
	CombatGroup* SplitPlayerCombatGroup(Actor* apMember);
	// Whether apActor wants to help apAttacker's target.
	bool GetWantToHelpTarget(Actor* apActor, Actor* apAttacker);
	void TargetLocalActor(Actor* apActor, CombatGroup* apGroup);
	void UntargetLocalActor(Actor* apActor, CombatGroup* apGroup);
	uint32_t GetCombatantCount(Actor* apActor, uint32_t* apNumLost);
	void AddLocalActor(Actor* apActor);
	void NotifyCombatGroupAddedTarget(CombatGroup* apGroup, Actor* apTarget);
	void NotifyCombatGroupRemovedTarget(CombatGroup* apGroup, Actor* apTarget);
	// Adds apActor to the local actor targeting map (thiscall, ret 8, 0x992700). IDA name; not
	// in the PDB.
	void NotifyCombatGroupAddedMember(CombatGroup* apGroup, Actor* apActor);
	// Removes apActor from the local actor targeting map. The map's PDB type is not known: the exe keys it by the
	// actor while src types pLocalActorTargetingMap as a CombatCoverMap (uint32_t keys), so the body waits on the
	// field's real type (thiscall, ret 4, 0x992A30).
	void RemoveLocalActor(Actor* apActor);
	// Drops apRef from every combat group, the local actor maps and the threat map; nothing while a save loads.
	void ClearReferencePointers(TESObjectREFR* apRef);
	// The number of combat group slots.
	uint32_t GetCombatGroupCount();
	// The combat group with the ID aiGroupID.
	CombatGroup* GetCombatGroup(uint32_t aiGroupID);
	// The combat group in slot auiIndex.
	CombatGroup* GetCombatGroupByIndex(uint32_t aiIndex);
	// Whether an explosion at arTargetedPoint is a combat attack on apAttacker's targets.
	bool CheckExplosionAttack(Actor* apAttacker, BGSExplosion* apExplosion, const NiPoint3& arTargetedPoint,
		float afTime);
	// Finishes loading the combat save data.
	void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Reverts the combat save data.
	void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
	bool GetWanttoHelpGroup(Actor* apActor, Actor* apActorToHelp);
	void AddSpectatorForCombatGroup(Actor* apSpectator, Actor* apCombatant);
	// Inferred type: apActor: Actor* from 0x8a0126 (this of Actor::StartCombat); apPackage: TESPackage* from 0x8a0126
	// (parameter 7 of Actor::StartCombat)
	CombatController* CreateCombatController(Actor* apActor, TESPackage* apPackage);
	void InitializeData();
	void ClearData(bool abNotifyGroupMembers);
	void UpdateDebugGeometry();
	CombatController* AddCombatant(Actor* apActor, Actor* apTarget, uint32_t aiPriority, bool abHighPriority);
	CombatController* AddGroupMember(Actor* apActor, CombatGroup* apGroup);
	uint32_t FindLocalActorsInRadius(NiPoint3 aLocation, float afRadius, float afTime, Actor* apAttackingActor,
		uint32_t* apOutNumSelf, uint32_t* apOutNumGroupMembers, uint32_t* apOutNumTargets,
		uint32_t* apOutNumNonTargets);
	bool ShouldPlayCombatMusic();
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void Destroy();
	static void Create();

	NiTPrimitiveArray<CombatGroup*> CombatGroups;
	// Each local actor's targeting bitfield: InitializeData (0x9911C0) constructs a NiTMap<Actor*, int64_t> (0x10
	// bytes, 37 buckets) here; IDA's CombatCoverMap type is wrong.
	NiTMap<Actor*, int64_t>* pLocalActorTargetingMap;
	CombatThreatMap* pThreatMap;
	CombatCoverMap* pCoverMap;
	CombatTargetMap* pTargetMap;
	CombatTimeStamp fCombatMusicStartedTimeStamp;
	CombatTimer CombatMusicStopTimer;
	CombatTimer CombatMusicUpdateTimer;
	uint32_t iNextGroupID;
	// A NiNode (CombatManager::UpdateDebugGeometry 0x991770 creates a 0xAC-byte NiNode for it).
	NiPointer<NiNode> spDebugGeometry;
};

// The combat manager.
inline CombatManager* pCombatManager;

// The game settings CombatManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDebugCombatDetectionEvents;
