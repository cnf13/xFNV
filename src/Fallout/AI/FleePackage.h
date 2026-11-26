#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Package.h"

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
    float GetFleeTimer();
    TESObjectREFR* GetRefFleeTo();
    // Stores pActorToFleeTo.
    void SetRefFleeTo(TESObjectREFR* apref) {
        pRefFleeTo = apref;
    }
    void SetFleeSucceeded(bool abflag);
    // Rebuilds the list of avoided refs from the process.
    void UpdateAvoidedRefList(BaseProcess* apProcess);
    // The avoided ref closest to apFromRef, or null.
    TESObjectREFR* GetClosestAvoidedRef(TESObjectREFR* apFromRef);
    // Finds a teleport door away from apThreat.
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

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static bool EvaluateReferenceforTeleportDoor(TESObjectREFR* apRef, void* aiFilterData);

    bool bEvaluatePoint;
    bool bCombatMode;
    NiPoint3 kFleePoint;
    float fFleeTimer;
    uint8_t bFleeSucceeded;
    // The references to avoid.
    BSSimpleList<TESObjectREFR*> kAvoidRefs;
    TESObjectREFR* pTeleportDoor;
    // SetRefFleeTo stores a reference here, and loading casts the form to TESObjectREFR.
    TESObjectREFR* pRefFleeTo;
    bool bknowstarget;
    bool busehorse;
};

inline TESObjectREFR* pcombatActor;
