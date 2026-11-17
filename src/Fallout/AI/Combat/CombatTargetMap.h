#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class Actor;

// What the combat targets map keeps for one actor: the actors attacking it and a node of debug geometry.
class CombatTargetData {
public:
    CombatTargetData();
    ~CombatTargetData();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSSimpleArray<Actor*> Attackers;
    NiPointer<NiNode> spDebugGeometry;
};

// The combat targets by actor.
class CombatTargetMap : public NiTMap<Actor*, CombatTargetData*> {
public:
    CombatTargetMap();
    ~CombatTargetMap() override;

    // Deletes apTarget's target data and removes it from the map.
    void RemoveTarget(Actor* apTarget);
    void AddTargetAttacker(Actor* apTarget, Actor* apAttacker);
    void RemoveTargetAttacker(Actor* apTarget, Actor* apAttacker);
    // apTarget's attacker data, or null.
    CombatTargetData* GetTargetAttackerArray(Actor* apTarget);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

// The game settings CombatTargetMap.cpp defines.
extern SettingT<INISettingCollection> bDebugCombatTargets;
