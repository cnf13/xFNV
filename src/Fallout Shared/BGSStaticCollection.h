#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"

// A static collection form (SCOL). Its own members are not declared yet.
class BGSStaticCollection : public TESBoundObject, public TESModelTextureSwap {
public:
    BGSStaticCollection();
    ~BGSStaticCollection() override;

    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::Save;

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};
