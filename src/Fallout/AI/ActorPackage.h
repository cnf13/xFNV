#pragma once

#include <cstdint>

class ActorPackageData;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;
class TESPackage;

// The package an actor is running: the package form, the per-actor data made for that package type, the target the
// package was started with, which procedure inside it is executing and when it started.
class ActorPackage {
public:
    // The bit indices of uiFlags SetFlag and GetFlag take.
    enum ACTOR_PACKAGE_FLAG : int32_t {
        SAVELOAD_SHARED_PACKAGE = 0,
    };

    // Resets to the empty package, which is also the state the engine reads as "nothing running".
    ActorPackage();
    ~ActorPackage();

    // Clamps the current procedure index to the package's
    // procedures; the callee ignores both arguments.
    void InitLoadGame(int32_t aiFlags, int32_t aiOldFlags);
    // Does nothing and returns 0; the argument is unused.
    int32_t Revert(int32_t);
    // Saves the package into the form buffer.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Writes the package's form ID, the target and the procedure index to the old save game; BaseProcess's save calls
    // it on CurrentPackage.
    void SaveGame(uint32_t aiFlags);
    // Clears pTarget when it is apRef (BaseProcess::HandleDeletedReference calls it on CurrentPackage).
    void HandleDeletedReference(TESObjectREFR* apRef);
    // Loads the package from the form buffer.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Resolves the saved package of the load game buffer.
    void InitLoadGame(BGSLoadFormBuffer* apBuffer);
    // Moves the package into another ActorPackage, leaving this one empty.
    void Copy(ActorPackage* apDestination);

    // Starts a package form for this actor: the old package is cleared, the new one is referenced, its per-actor data
    // is created from the package type and the target is stored.
    void InitFromPackage(TESPackage* apPackage, TESObjectREFR* apTarget);

    // Drops the package: releases the form and destroys its per-actor data, both of which need the form lists.
    void Clear();
    // Sets or clears bit aeFlag of uiFlags.
    void SetFlag(ACTOR_PACKAGE_FLAG aeFlag, bool abValue);
    // Whether bit aeFlag of uiFlags is set.
    bool GetFlag(ACTOR_PACKAGE_FLAG aeFlag);

    void SetTarget(TESObjectREFR* apTarget);
    TESObjectREFR* GetTarget() {
        return pTarget;
    }
    // Forgets pData once the per-actor data has been handed elsewhere.
    void NullInstanceData();
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    float GetPackageStartTime() {
        return fPackageStartTime;
    }
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer);
    uint16_t GetSaveSize(uint32_t aiFlags);
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags);
    void AddToCurrentProcedureIndex(int32_t aiOffset);

    // The package form itself, held referenced for as long as the actor has it.
    TESPackage* pPackage;

    // The per-actor data of the package, or null while no package is running.
    ActorPackageData* pData;

    // The reference the package target resolved to when the package started.
    TESObjectREFR* pTarget;

    // Index of the procedure inside the package that is executing; -1 for none. Signed: it is compared as a signed
    // value and set to -1.
    int32_t iCurrentProcedureIndex;

    // The hour the package started at, from the game calendar; -1 while it has not started.
    float fPackageStartTime;

    uint32_t iFlags;
};
