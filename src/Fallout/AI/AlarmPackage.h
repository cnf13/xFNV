#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"

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

    // The crimes the alarm is raised for.
    BSSimpleList<Crime*> Crimes;
};

class Crime {
public:
    Crime();
    Crime(
        uint32_t aeCrime,
        TESObjectREFR* apTarget,
        Actor* apActor,
        TESBoundObject* apObject,
        int32_t aiNumber,
        TESForm* apOwnership
    );
    ~Crime();

    // Resolves the crime's form IDs after a load.
    void InitItem(BGSLoadGameBuffer* apLoadGameBuffer);
    PackageNS::CRIME_TYPE GetCrimeType() {
        return ecrime;
    }
    TESForm* GetOwnershipCrime() {
        return pOwnership;
    }
    bool DoesActorKnowaboutCrime(Actor* apActor);
    bool ShouldCrimeBeRemoved();
    void ClearActorKnowList();
    CombatTimeStamp GetTimeStamp();
    void AddtoActorKnowList(Actor* apActor);
    void RemoveActorKnowList(Actor* apActor);
    int32_t GetRefCount();
    void AddEnemyFactions(TESFaction* apFaction);
    uint16_t GetSaveSize();
    void SaveGame();
    void Save(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // Compared signed, in Crime::ShouldCrimeBeRemoved.
    int32_t iRefCount;
    PackageNS::CRIME_TYPE ecrime;
    TESObjectREFR* pcrimetarget;
    Actor* pcriminal;
    bool breported;
    TESBoundObject* pObjectStolen;
    uint32_t inumberstolen;
    BSSimpleList<Actor*> pActorsknowofCrime;
    TESForm* pOwnership;
    uint32_t iCrimeNumber;
    // Whether a guard has responded to the crime. Crime::SaveGame, Crime::Save and the load each save and load this one
    // byte, so the member is followed by padding.
    bool bguardrespond;
    // Packed date (year << 13 | month << 9 | day); it is signed because Crime::ShouldCrimeBeRemoved compares the
    // difference signed.
    int32_t iCrimeStamp;
    CombatTimeStamp fCrimeTimeStamp;
    // Allocated on first use by Crime::AddEnemyFactions; the destructor reads TESFactions from it.
    BSSimpleList<TESFaction*>* pListofEnemyFactions;
};
