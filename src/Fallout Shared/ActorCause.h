#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class Actor;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
// DetectionState is declared in Fallout/AI/HighProcess.h.
class DetectionState;

// The list of detections.
class DetectionData_tList : public BSSimpleList<DetectionState*> {};

// The actor a deed is blamed on (the shooter of a projectile, the one who set off an explosion), kept by ID so it
// survives saving and the actor's removal. Counted by hand through uiRefCount; every cause is also in kCauseList.
class ActorCause {
public:
    ActorCause();
    ActorCause(Actor* apActor);
    ~ActorCause();

    // Takes a reference: an atomic increment of iRefCount.
    void IncRefCount();
    // Releases a reference; the last one deletes the cause.
    void DecRefCount();
    // Reads the cause from a save.
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    // Saves the origin, the ID, the actor, then every witness, with their count in front.
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    // pActor.
    Actor* GetActor() {
        return pActor;
    }
    // Whether the cause still has its actor (pActor not null).
    bool IsValid();
    // Returns &kOrigin.
    NiPoint3* FreeDetectionList();
    void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

    // Walks kCauseList on a load: initialises each cause that still has references and frees the others.
    static void InitLoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer);
    // Empties kCauseList on a revert.
    static void RevertActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void LoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer);
    static void ClearActor(Actor* apActor);
    static ActorCause* GetActorCauseByID(int32_t aiActorCauseID);
    static void SaveActorCausesList(BGSSaveGameBuffer* apSaveGameBuffer);

    Actor* pActor;
    BSSimpleList<DetectionState*>* pWitnesses;
    NiPoint3 kOrigin;
    uint32_t iActorCauseID;
    uint32_t iRefCount;

    // The next ID ActorCause hands out.
    static inline uint32_t iActorCauseIDCounter;
    // Every cause that exists (the list's head node is inline).
    static BSSimpleList<ActorCause*> kCauseList;
};

// NiPointer of a cause takes and releases references through the cause's own IncRefCount and DecRefCount.
inline void NiIncRefCount(ActorCause* apCause) {
    apCause->IncRefCount();
}

inline void NiDecRefCount(ActorCause* apCause) {
    apCause->DecRefCount();
}
