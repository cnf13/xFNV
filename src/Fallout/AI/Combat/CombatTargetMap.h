#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

class Actor;

// What the combat targets map keeps for one actor: the actors attacking it and a node of debug geometry.
class CombatTargetData {
public:
	CombatTargetData();
	~CombatTargetData();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSimpleArray<Actor*> Attackers;
	NiPointer<NiNode> spDebugGeometry;
};

// The combat targets by actor. IDA sees only its map (an NiTMap<int, int>); RTTI gives the base used here.
class CombatTargetMap : public NiTMap<Actor*, CombatTargetData*> {
public:
	// Runs the exe's constructor (thiscall, ret 0, 0x9A3E90) on apMemory. Returns apMemory.
	CombatTargetMap();
	~CombatTargetMap() override;

	// Deletes apTarget's target data and removes it from the map.
	void RemoveTarget(Actor* apTarget);
	void AddTargetAttacker(Actor* apTarget, Actor* apAttacker);
	void RemoveTargetAttacker(Actor* apTarget, Actor* apAttacker);
	// apTarget's attacker data, or null.
	CombatTargetData* GetTargetAttackerArray(Actor* apTarget);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
};
inline constexpr fnvd::Vtable CombatTargetMap_vftable{};

// The game settings CombatTargetMap.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDebugCombatTargets;
