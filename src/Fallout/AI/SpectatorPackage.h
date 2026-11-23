#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Package.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

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
    Actor* GetSource() const {
        return pSource;
    }
    void SetupThreatFromActorToCombatTarget(Actor& aSource);
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    NiPoint3 GetLastKnownPosSource() const;
    NiPoint3 GetLastKnownPosTarget() const;

protected:
    // The place to keep away from: a cylinder from the source to the target, or a sphere around whichever is known,
    // with the threat's radius and a cost of 100. nodeOut stays as it was when neither is known.
    void GetAvoidNode(PathingAvoidNode& nodeOut) const;

public:
    void GetAvoidNode(PathingAvoidNode* apPathingAvoidNode) const;
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    static void operator delete(void* apMemory);
    // The threat info's copy assignment: copies the fields up to bHasTarget, then spDebugGeometry through
    // NiPointer::operator=; returns apThis. SpectatorPackage::Copy calls it. It is not operator=, so `a = b` sites keep
    // the implicit copy assignment.
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
    void HandleDeletedActor(void* apActor);
    void RemoveActorAsThreatSourceOrTarget(Actor& aActor);
    PathingAvoidNodeArray* CreateAvoidNodeArray();

    static void operator delete(void* apMemory);

    uint32_t iLastPathRequestTime;
    float fTimer;
    float fConversationTimer;
    float fEventRadius;
    uint8_t bFlagCompleted;
    NiPoint3 kEventCenter;
    BSSimpleArray<SpectatorThreatInfo> kThreatInfos;
    // The debug geometry of the watched threat.
    NiPointer<NiAVObject> spDebugGeometry;
};

extern SettingT<INISettingCollection> bDebugSpectatorThreats;
