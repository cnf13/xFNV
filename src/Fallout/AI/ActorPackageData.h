#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Package.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;

// The per-actor data of a running package, in PC vtable order.
class ActorPackageData {
public:
    ActorPackageData();
    virtual ~ActorPackageData();

    virtual void HandleDeletedReference(TESObjectREFR* apDeletedObj) = 0;
    virtual PACKAGE_TYPE GetPackageType() = 0;
    virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) = 0;
    virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) = 0;
    virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) = 0;

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // The factory: the package data of aPackageType.
    static ActorPackageData* CreateActorPackageData(PACKAGE_TYPE aPackageType);
};
