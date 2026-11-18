#pragma once

#include "BSCore/BSReadWriteLock.h"
#include "BSMain/Setting.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class Actor;
class BGSExplosion;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class bhkRigidBody;
class PathingAvoidNode;
class PathingAvoidNodeArray;
class Projectile;
class TESForm;
class TESObjectREFR;

// A threat the combat AI avoids (a grenade, a mine, a moving object): the reference, its radius, when it started and
// ends, and the cached 3D, rigid body and velocity used to predict where it goes.
class CombatThreat {
public:
    // The default constructor: no threat, zero times and damage.
    CombatThreat();
    CombatThreat(TESObjectREFR* apThreat, float afRadius, float afStartTime, float afEndTime, float afDamage);
    ~CombatThreat();

    // The threat score of the threat for apActor at apLocation.
    float ComputeThreatScore(
        const NiPoint3& arLocation,
        Actor* apActor,
        float afCompareTime,
        float afAdditionalRadius,
        float afActorHealth
    );
    // The time the threat started, written to apOut.
    CombatTimeStamp* GetStartTime(CombatTimeStamp* apOut);
    void Initialize(
        TESObjectREFR* apThreat,
        float afRadius,
        float afStartTime,
        float afEndTime,
        float afDamage,
        bool abUpdatingThreat
    );
    // Whether afCompareTime lies within [fStartTime, fEndTime].
    bool IsActive(float afCompareTime);
    // Whether afCompareTime is past fEndTime.
    bool IsExpired(float afCompareTime);
    // Saves the radius, the start and end times and the damage.
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    float GetBaseThreatScore(Actor* apActor, float afActorHealth);
    NiPoint3 GetThreatLinearVelocity();
    PathingAvoidNode* GetAvoidNode(PathingAvoidNode* apResult, Actor* apActor);
    void Load(BGSLoadGameBuffer* a1);
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    TESObjectREFR* pThreat;
    float fRadius;
    CombatTimeStamp fStartTime;
    CombatTimeStamp fEndTime;
    float fDamage;
    NiPointer<NiAVObject> spDebugGeometry;
    NiAVObject* pCachedThreat3D;
    bhkRigidBody* pCachedRigidBody;
    NiPoint3 CachedLinearVelocity;
    CombatTimeStamp fCachedLinearVelocityTimeStamp;
};

// The combat threats by ID, behind a read-write lock.
class CombatThreatMap : public NiTMap<uint32_t, CombatThreat*> {
public:
    // Constructs the map in apMemory.
    CombatThreatMap();
    ~CombatThreatMap() override;

    // Ages the threats and drops the expired ones.
    void UpdateThreats();
    // Removes the threat of apForm.
    void RemoveThreat(TESForm* apForm);

    // Records a threat of apRef: its radius, the times it starts and ends at and its damage.
    void AddThreat(TESObjectREFR* apRef, float afRadius, float afStartTime, float afEndTime, float afDamage);
    // Records apRef as a threat that explodes with apExplosion at afTime.
    void AddThreat(TESObjectREFR* apRef, BGSExplosion* apExplosion, float afTime);
    // The threat at aLocation for apActor arriving after afTime seconds, counting threats within afAdditionalRadius and
    // not older than afSkipOlderThreats.
    float ComputeThreatScore(
        NiPoint3 aLocation,
        Actor* apActor,
        float afTime,
        float afAdditionalRadius,
        CombatTimeStamp afSkipOlderThreats,
        float afActorHealth
    );

    // Saves every threat: its reference's ID, then the threat, with the count in front.
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    void RemoveAll();
    PathingAvoidNodeArray* CreateAvoidNodeArray(const NiPoint3& arLocation, Actor* apActor, float afTime);
    void AddProjectile(Projectile* apProjectile);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSReadWriteLock Lock;
};

extern SettingT<INISettingCollection> bDebugCombatThreats;

extern SettingT<GameSettingCollection> gs_fCombatThreatExtrudeVelocityThreshold;
