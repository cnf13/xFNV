#pragma once

#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObject.h"

// A magic item that is also a bound object in the world (alchemy items and ingredients).
class MagicItemObject : public TESBoundObject, public MagicItem {
public:
    MagicItemObject();
    ~MagicItemObject() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void SetAltered(bool abValue) override;
    bool IsMagicItem() override {
        return true;
    }
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    // pure here: both TESObject's and MagicItem's slots
    bool IsAutoCalc() override = 0;
    void SetAutoCalc(bool abValue) override = 0;

protected:
    // swaps the byte order of the item's data
    virtual void Endian() = 0;

public:
    using TESForm::Save;

    // The bases' operator deletes are ambiguous: the same free (ourFree) as theirs.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};
