#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout/AI/ActorPackageData.h"

class Actor;

// The per-actor data of a running escort package: the escorting actor, its package and the actors it escorts.
class EscortActorPackageData : public ActorPackageData {
public:
    EscortActorPackageData();
    ~EscortActorPackageData() override;

    void HandleDeletedReference(TESObjectREFR* apDeletedObj) override;
    PACKAGE_TYPE GetPackageType() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    // Removes apActor from the escorted actors.
    void RemoveEscortedActor(Actor* apActor);
    // Adds apActor to the escorted actors unless it is there already.
    void AddEscortedActor(Actor* apActor);
    int32_t GetNumEscortedActorsOfType(Actor* apEscorter, TESBoundObject* apType);

    Actor* pActor;
    TESPackage* pPackage;
    const TESEscortPackageData* pPackageData;
    BSSimpleList<Actor*> EscortedActors;
};
