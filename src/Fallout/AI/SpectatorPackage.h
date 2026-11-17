#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/Package.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class BGSLoadFormBuffer;
class BGSSaveGameBuffer;
class PathingAvoidNode;
class PathingAvoidNodeArray;

// What a spectator remembers about a threat it watches: the source and target actors, how dangerous it is and where
// each was last known to be.
class SpectatorThreatInfo {
public:
	// Values from the Xbox PDB.
	enum THREAT_LEVEL : int32_t {
		THREAT_LEVEL_NONE = 0x0,
		THREAT_LEVEL_MELEE_WEAPONS = 0x1,
		THREAT_LEVEL_PROJECTILE_WEAPONS = 0x2,
		THREAT_LEVEL_EXPLOSION = 0x3,
		THREAT_LEVEL_INSANE = 0x4,
	};

	SpectatorThreatInfo();
	~SpectatorThreatInfo();

	// Refreshes the perceived time and last known positions, and raises the threat level and radius by the source's
	// weapon type.
	void Update();
	// The exe reads pSource through its shared one-field getter (thiscall, 0x559450).
	Actor* GetSource() const { return pSource; }
	void SetupThreatFromActorToCombatTarget(Actor& aSource);
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// LastKnownPosSource, out of line (thiscall, ret 4, hidden result, 0x9F7050).
	NiPoint3 GetLastKnownPosSource() const;
	// LastKnownPosTarget, out of line (thiscall, ret 4, hidden result, 0x9F7080).
	NiPoint3 GetLastKnownPosTarget() const;

protected:
	// The place to keep away from: a cylinder from the source to the target, or a sphere around whichever is known,
	// with the threat's radius and a cost of 100. nodeOut stays as it was when neither is known.
	void GetAvoidNode(PathingAvoidNode& nodeOut) const;

public:
	void GetAvoidNode(PathingAvoidNode* apPathingAvoidNode) const;
	// Inferred type: a2: BGSSaveGameBuffer* from 0x9f88e4 (parameter 0 of SpectatorPackage::SaveGame)
	void SaveGame(BGSSaveGameBuffer* a2);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	static void operator delete(void* apMemory);
	// The threat info's implicit copy assignment, out of line (the PDB's operator=(__that)):
	// the fields up to bHasTarget, then spDebugGeometry through NiPointer::operator= 0x6E5CC0; returns apThis
	// (thiscall, ret 4, 0x9F7630). SpectatorPackage::Copy (0x9F7500) calls it. Not operator=: src's `a = b` sites
	// keep the implicit copy assignment.
	// Inferred type: apThis: SpectatorThreatInfo* from 0x9f761d (result of BSSimpleArray<SpectatorThreatInfo,
	// 1024>::operator[]), its members used at 0x9f7630; arOther: SpectatorThreatInfo* from 0x9f761d (result of
	// BSSimpleArray<SpectatorThreatInfo, 1024>::operator[]), its members used at 0x9f763d
	static SpectatorThreatInfo* Assign(SpectatorThreatInfo* apThis, const SpectatorThreatInfo& arOther);

	Actor* pSource;
	Actor* pTarget;
	THREAT_LEVEL eThreatLevel;
	uint32_t iLastPerceivedTime;
	float fRadius;
	NiPoint3 LastKnownPosSource;
	NiPoint3 LastKnownPosTarget;
	bool bHasSource;
	bool bHasTarget;
	uint8_t padding2E[2];
	NiPointer<NiAVObject> spDebugGeometry;
};

// The package of an actor that watches a fight.
class SpectatorPackage : public TESPackage {
public:
	SpectatorPackage();
	~SpectatorPackage() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Copy(TESForm* apCopy) override;
	uint16_t GetSaveSize() override;
	void SaveGame() override;
	void LoadGame() override;
	// the exe shares this body with BackUpPackage::InitLoadGame (0x9F87B0)
	void InitLoadGame() override;
	using TESPackage::InitLoadGame;
	using TESPackage::LoadGame;
	using TESPackage::SaveGame;

	// Adds a threat from aSourceActor to the combat target.
	void AddThreatFromActorToCombatTarget(Actor& aSourceActor);
	// The point the spectators watch.
	NiPoint3 GetEventCenter();
	// Empties apRetThreats, reserves room for two entries per threat, then appends each threat's source actor.
	void CreateThreatArray(BSSimpleArray<Actor*>* apRetThreats);
	bool IsComplete() const;
	void ComputeCenterPoint();
	void MakeCombatComment(Actor* apActor, bool bflag);
	void LookAtCombatants(Actor* apActor, bool bflag);
	void Update(Actor* pOwner);
	void HandleDeletedActor(void* a2);
	void RemoveActorAsThreatSourceOrTarget(Actor& aActor);
	PathingAvoidNodeArray* CreateAvoidNodeArray();

	static void operator delete(void* apMemory);

	uint32_t iLastPathRequestTime;
	float fTimer;
	float fConversationTimer;
	float fEventRadius;
	uint8_t bFlagCompleted;
	uint8_t padding91[0x3];
	NiPoint3 kEventCenter;
	BSSimpleArray<SpectatorThreatInfo> kThreatInfos;
	// The debug geometry of the watched threat (the PDB's name; IDA: spObjectB0).
	NiPointer<NiAVObject> spDebugGeometry;
};
inline constexpr fnvd::Vtable SpectatorPackage_vftable{};

// The game settings SpectatorPackage.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDebugSpectatorThreats;
