#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESForm.h"

// A form list (FLST): an ordered list of forms, plus the number of forms scripts added to it at run time.
class BGSListForm : public TESForm {
public:
    BGSListForm();
    ~BGSListForm() override;

    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::LoadGame;
    using TESForm::Revert;
    using TESForm::Save;
    using TESForm::SaveGame;

    BSSimpleList<TESForm*>* GetFormList();
    BGSListForm* EmptyList();
    int32_t GetItemIndex(TESForm* apForm);
    // Adds apForm at the head of ListOfForms when it is not there.
    void AddForm(TESForm* apForm);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSSimpleList<TESForm*> ListOfForms;
    uint32_t iScriptAddedFormCount;
};
