#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESForm.h"

// A magic item that is a plain form (spells and enchantments).
class MagicItemForm : public TESForm, public MagicItem {
public:
    MagicItemForm();
    ~MagicItemForm() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void SetAltered(bool abValue) override;
    bool IsMagicItem() override {
        return true;
    }
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

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
