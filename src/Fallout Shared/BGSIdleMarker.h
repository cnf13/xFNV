#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/BGSIdleCollection.h"
#include "Fallout Shared/TESObject.h"

// An idle marker (IDLM): a placeable object with the idles an actor can play at it. Its members are not declared
// yet.
class BGSIdleMarker : public TESBoundObject, public BGSIdleCollection {
public:
    BGSIdleMarker();
    ~BGSIdleMarker() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    void UnClone3D(TESObjectREFR*) override {}
    NiAVObject* Clone3D(TESObjectREFR* apRequester) override;
    using TESBoundObject::Clone3D;
    using TESForm::Save;

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};
