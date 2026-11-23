#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/ExtraDataList.h"

class Actor;
class TESObjectREFR;

// The guards of a reference that is guarded (a door or a container the guards watch), as the form IDs of the references
// that guard it.
class ExtraGuardedRefData : public BSExtraData {
public:
    ExtraGuardedRefData();
    ~ExtraGuardedRefData() override;

    bool Compare(BSExtraData* apCompare) override;

    // Adds apActor to the guards.
    void AddGuard(Actor* apActor);
    // Tells every guard actor of this extra's reference list about the activation of apObject
    // by apActionRef.
    void NotifyGuardsAboutActivation(TESObjectREFR* apObject, TESObjectREFR* apActionRef);

    // Frees through ourFree (BSCore/MemoryManager.h).
    static void operator delete(void* apMemory);

    BSSimpleArray<uint32_t, 1024> Guards;
};
