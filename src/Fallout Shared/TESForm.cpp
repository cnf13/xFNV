#include "Fallout Shared/TESForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/MessageHandler.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/FormComponentCollection.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQualityForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESUsesForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadStructs.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <cstdint>
#include <zlib.h>

struct z_stream_s;

thread_local bool TESForm::bIsReadingName;

// Maps a form ID from the save's load order to the current one through the save's file index table: unchanged when
// there is no table or the ID is a created form's (index 0xFF), 0 when the save's file is gone.
uint32_t TESSaveLoadGame_GetSaveFormID(TESSaveLoadGame* apSaveLoadGame, uint32_t aiID) {
    uint8_t cFileIndex = (aiID & 0xFF000000u) >> 24;
    if (!apSaveLoadGame->m_pFileIndexArray || cFileIndex == 255) {
        return aiID;
    }
    if (cFileIndex >= apSaveLoadGame->m_iSavedPluginCount) {
        return 0;
    }
    uint8_t cNewIndex = apSaveLoadGame->m_pFileIndexArray[cFileIndex];
    if (cNewIndex == 255) {
        return 0;
    }
    return (cNewIndex << 24) + (aiID & 0xFFFFFF);
}

uint16_t Min(uint16_t a1, uint16_t a2) {
    if (((int32_t)a1) >= ((int32_t)a2)) {
        return a2;
    }

    return a1;
}

TESForm::TESForm() : BaseFormComponent(), cFormType(0), iFormFlags(8), iFormID(0), pSourceFiles() {
    if (!iTotalFormCount) {
        InitializeFormDataStructures();
    }
    ++iTotalFormCount;
    // Checks the form type table once: every entry in its slot, no two with the same record code.
    if (!bEnumStringsChecked) {
        bool bFailed = false;
        MessageHandler::IncDisableWarningCount(true);
        for (uint32_t i = 0; i < 121; ++i) {
            const char* pString = formEnumString[i].pFormString;
            formEnumString[i].uiFormString = pString[3] << 24 | (pString[2] << 16 | (pString[1] << 8 | *pString));
            if (formEnumString[i].cFormID != i) {
                Warning("FORMS: formEnumString[ %d ].cFormID in TESForm.cpp is out of order.", i);
                bFailed = true;
            }
            for (uint32_t j = 0; j < 121; ++j) {
                if (i != j && formEnumString[i].uiFormString == formEnumString[j].uiFormString) {
                    Warning(
                        "FORMS: formEnumString[ %d ] and formEnumString[ %d ] have the same iFormString %s in "
                        "TESForm.cpp.",
                        i, j, formEnumString[i].pFormString
                    );
                    bFailed = true;
                }
            }
        }
        MessageHandler::IncDisableWarningCount(false);
        if (bFailed) {
            InitializationFlow("You must fix the problems in TESForm.cpp to run this game.");
        }
        bEnumStringsChecked = true;
    }
    // A form created at run time (not while the files load) takes the next form ID and the active file.
    if (pDataHandler && !pDataHandler->GetLoadingFiles()) {
        iFormID = pDataHandler->GetNextID();
        if (pDataHandler->GetActiveFile()) {
            SetFile(pDataHandler->GetActiveFile());
        }
        if (iFormID <= 2047) {
            TESForm::SetFormID(2048u, true);
        }
    }
    if (iFormID) {
        pAllForms->SetAt(iFormID, this);
    }
}

TESForm::~TESForm() {
    RemoveFromDataStructures();
    pSourceFiles.RemoveAll();
    if (!GetTemporary() && pSaveLoadGameOLD) {
        pSaveLoadGameOLD->RemoveChanges(this, false);
        pSaveLoadGameOLD->RemoveFormFromDeferredDeletionList(this);
    }
    if (!GetTemporary() && pSaveLoadGame) {
        pSaveLoadGame->RemoveChanges(this);
    }
    if (!--iTotalFormCount) {
        ReleaseFormDataStructures();
    }
}

void TESForm::InitializeDataComponent() {}

void TESForm::CopyComponent(BaseFormComponent* apCopyFC) {
    int32_t altered;
    uint8_t v3;
    TESForm* v5 = dynamic_cast<TESForm*>(apCopyFC);
    if (v5) {
        altered = (int32_t)GetAltered();
        if (((uint32_t)altered) != ((uint32_t)((uint8_t)v5->GetAltered()))) {
            v3 = (uint8_t)v5->GetAltered();
            SetAltered((bool)v3);
        }
        cFormType = v5->GetFormType();
        iFormFlags = (iFormFlags & 16384) | (v5->TESForm::GetFormFlags() & 0xFFFFBFFFu);
    }
}

bool TESForm::CompareComponent(BaseFormComponent* apCompareFC) {
    uint32_t FormEditorIDLength;
    const char* v5;
    const char* v6;
    TESForm* pCompare = dynamic_cast<TESForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    int32_t cFormType = this->cFormType;
    if (cFormType != pCompare->GetFormType()) {
        return true;
    }
    if (!GetFormEditorIDLength() && !pCompare->GetFormEditorIDLength()) {
        goto LABEL_11;
    }
    FormEditorIDLength = GetFormEditorIDLength();
    if (FormEditorIDLength != pCompare->GetFormEditorIDLength()) {
        return true;
    }
    v6 = pCompare->GetFormEditorID();
    v5 = GetFormEditorID();
    if (BSstricmp(v5, v6)) {
        return true;
    }
LABEL_11:;
    uint32_t iTempFlags = iFormFlags;
    return (iTempFlags & 0xFFFFBFF4u) != (pCompare->iFormFlags & 0xFFFFBFF4u);
}

bool TESForm::Save(TESFile* apFile) {
    if (GetTemporary()) {
        return false;
    }
    Save();
    return (apFile->AddTESForm(this) == 0);
}

bool TESForm::LoadEdit(TESFile* apFile) {
    return (bool)((uint8_t)Load(apFile));
}

bool TESForm::SaveEdit(TESFile* apFile) {
    if (GetTemporary()) {
        return false;
    }
    if (!GetDelete()) {
        return Save(apFile);
    }
    apFile->StartForm(this);
    apFile->CloseForm();
    return true;
}

bool TESForm::SavesBefore(TESForm* apForm) {
    uint32_t v5;
    uint32_t v7;
    uint32_t FormType = apForm->GetFormType();
    uint32_t v6 = GetFormType();
    switch (FormType) {
        case 103u: {
            v5 = 311;
            break;
        }
        case 108u: {
            v5 = 312;
            break;
        }
        case 115u: {
            v5 = 313;
            break;
        }
        case 116u: {
            v5 = 314;
            break;
        }
        default: {
            v5 = 10 * FormType;
            break;
        }
    }
    switch (v6) {
        case 103u: {
            v7 = 311;
            break;
        }
        case 108u: {
            v7 = 312;
            break;
        }
        case 115u: {
            v7 = 313;
            break;
        }
        case 116u: {
            v7 = 314;
            break;
        }
        default: {
            v7 = 10 * v6;
            break;
        }
    }
    return v7 < v5;
}

bool TESForm::SavesBefore(FORM* apGroupFORM) {
    bool bretval = false;
    if ((apGroupFORM) && (apGroupFORM->form == formEnumString[2].uiFormString)) {
        uint32_t form = apGroupFORM->form;
        switch (apGroupFORM->iFormID) {
            case 0u: {
                uint32_t v12;
                int32_t v14;
                ENUM_FORM_ID FormTypeFromFormString = TESForm::GetFormTypeFromFormString(apGroupFORM->flags);
                ENUM_FORM_ID FormType = GetFormType();
                switch (FormTypeFromFormString) {
                    case IMOD_ID: {
                        v12 = 311;
                        break;
                    }
                    case CHIP_ID: {
                        v12 = 312;
                        break;
                    }
                    case CCRD_ID: {
                        v12 = 313;
                        break;
                    }
                    case CMNY_ID: {
                        v12 = 314;
                        break;
                    }
                    default: {
                        v12 = 10 * FormTypeFromFormString;
                        break;
                    }
                }
                switch (FormType) {
                    case IMOD_ID: {
                        v14 = 311;
                        break;
                    }
                    case CHIP_ID: {
                        v14 = 312;
                        break;
                    }
                    case CCRD_ID: {
                        v14 = 313;
                        break;
                    }
                    case CMNY_ID: {
                        v14 = 314;
                        break;
                    }
                    default: {
                        v14 = 10 * FormType;
                        break;
                    }
                }
                if (v14 < v12) {
                    bretval = true;
                }
                break;
            }
            case 1u:
            case 4u:
            case 5u: {
                uint32_t v8;
                int32_t v7 = GetFormType();
                switch (v7) {
                    case 103: {
                        v8 = 311;
                        break;
                    }
                    case 108: {
                        v8 = 312;
                        break;
                    }
                    case 115: {
                        v8 = 313;
                        break;
                    }
                    case 116: {
                        v8 = 314;
                        break;
                    }
                    default: {
                        v8 = 10 * v7;
                        break;
                    }
                }
                if (v8 < 650) {
                    bretval = true;
                }
                break;
            }
            case 2u:
            case 3u:
            case 6u: {
                uint32_t v10;
                ENUM_FORM_ID v9 = GetFormType();
                switch (v9) {
                    case IMOD_ID: {
                        v10 = 311;
                        break;
                    }
                    case CHIP_ID: {
                        v10 = 312;
                        break;
                    }
                    case CCRD_ID: {
                        v10 = 313;
                        break;
                    }
                    case CMNY_ID: {
                        v10 = 314;
                        break;
                    }
                    default: {
                        v10 = 10 * v9;
                        break;
                    }
                }
                if (v10 < 570) {
                    bretval = true;
                }
                break;
            }
            case 7u: {
                uint32_t v6;
                int32_t v5 = GetFormType();
                switch (v5) {
                    case 103: {
                        v6 = 311;
                        break;
                    }
                    case 108: {
                        v6 = 312;
                        break;
                    }
                    case 115: {
                        v6 = 313;
                        break;
                    }
                    case 116: {
                        v6 = 314;
                        break;
                    }
                    default: {
                        v6 = 10 * v5;
                        break;
                    }
                }
                if (v6 < 690) {
                    bretval = true;
                }
                break;
            }
            default: {
                return bretval;
            }
        }
    }

    return bretval;
}

TESForm* TESForm::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    uint8_t FormType = GetFormType();
    TESForm* pdupform = TESDataHandler::CreateFormOfType(FormType);
    if (pdupform) {
        BSStringT<char> tempstr(TESForm::GetFormEditorID());
        SetFormEditorID("");
        pdupform->Copy(this);
        if (tempstr.pString) {
            SetFormEditorID(tempstr.pString);
        }
    }
    if ((pdupform) && (apCopyMap)) {
        apCopyMap->SetAt(this, pdupform);
    }

    return pdupform;
}

void TESForm::AddChange(int32_t aiChangeFlags) {
    pSaveLoadGame->AddChange(this, BGSChangeFlags{aiChangeFlags}, false);
}

void TESForm::RemoveChange(int32_t aiChangeFlags) {
    pSaveLoadGame->RemoveChange(this, BGSChangeFlags{aiChangeFlags});
}

uint16_t TESForm::GetSaveSize(uint32_t aiFlags) {
    uint16_t v3 = 0;
    if ((aiFlags & 1) != 0) {
        return (uint16_t)4;
    }
    return v3;
}

// Saves the form flags when the save changed them.
void TESForm::SaveGame(uint32_t aiFlags) {
    if (aiFlags & CHANGE_FORM_FLAGS) {
        SaveGameDataOLD(&iFormFlags, 4);
    }
}

// Saves the form flags when the save changed them.
void TESForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FORM_FLAGS)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFormFlags), 4, 0);
    }
}

void TESForm::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint32_t apBuffer;
    if ((aiFlags & 1) != 0) {
        LoadGameDataOLD(&apBuffer, 4);
        if (dynamic_cast<TESObjectREFR*>(this)) {
            iFormFlags = (apBuffer & ~-0x912861) | (iFormFlags & 0xFF6ED79Fu);
        }
        else {
            iFormFlags = (apBuffer & ~-0x40000C21) | (iFormFlags & 0xBFFFF3DFu);
        }
    }
}

void TESForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags result;
    int32_t pData;
    BGSChangeFlags* v2 = &(result = apLoadGameBuffer->GetChangeFlags());
    if (v2->CheckFlags(1u)) {
        pData = 0;
        apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&pData)), 4u);
        if (IsReference()) {
            iFormFlags = (pData & 9513056) | (iFormFlags & 0xFF6ED79Fu);
        }
        else {
            iFormFlags = (pData & 1073744928) | (iFormFlags & 0xBFFFF3DFu);
        }
    }
}

void TESForm::Revert(uint32_t) {}

// Calls TESForm::GetFormEditorID directly.
void TESForm::GetFormDetailedString(BSStringT<char>& aString) {
    uint32_t uiID = GetFormID();
    const char* pEditorID = TESForm::GetFormEditorID();
    const char* pType = GetFormTypeString();
    aString.SPrintF("%s Form '%s' (%08X)", pType, pEditorID, uiID);
}

bool TESForm::GetQuestObject() {
    return (iFormFlags & 0x400) != 0;
}

bool TESForm::GetHasSpokenFlag() {
    return (iFormFlags & 0x40) != 0;
}

bool TESForm::GetHavokDeath() {
    return (iFormFlags & 0x10000) != 0;
}

bool TESForm::GetRandomAnim() {
    return (iFormFlags & FORM_FLAG_RANDOM_ANIM) != 0;
}

bool TESForm::GetNeedtoChangeProcess() {
    return (iFormFlags & 0x20000) != 0;
}

bool TESForm::GetDangerous() {
    return (iFormFlags & FORM_FLAG_DANGEROUS) != 0;
}

bool TESForm::GetHasPLSpecTex() {
    return (iFormFlags & 0x80000) != 0;
}

bool TESForm::GetObstacle() {
    return (iFormFlags & 0x2000000) != 0;
}

bool TESForm::GetContinuousBroadcast() {
    return ((iFormFlags & 1073741824) != 0);
}

bool TESForm::GetOnLocalMap() {
    return (iFormFlags & 0x200) != 0;
}

// Sets or clears form flag 0x200.
void TESForm::SetOnLocalMap(bool abVal) {
    if (abVal) {
        iFormFlags |= 0x200;
    }
    else {
        iFormFlags &= ~0x200u;
    }
}

void TESForm::SetDelete(bool abDeleted) {
    if (abDeleted) {
        iFormFlags |= kFormFlag_Deleted;
    }
    else {
        iFormFlags &= ~kFormFlag_Deleted;
    }

    AddChange(1);
}

// Adds the form to pAlteredForms (once) or removes it, then sets or clears the altered flag. A temporary form is never
// added.
void TESForm::SetAltered(bool abAltered) {
    if (abAltered) {
        if (!GetAltered() && !GetTemporary() && pAlteredForms) {
            bool bAdd = true;
            for (uint32_t i = 0; i < pAlteredForms->GetSize(); ++i) {
                // SWEEP-INLINE 0x877A30
                if (pAlteredForms->GetAt(i) == this) {
                    bAdd = false;
                    break;
                }
            }
            if (bAdd) {
                TESForm* pForm = this;
                pAlteredForms->AddFirstEmpty(pForm);
            }
        }
        uint32_t& ruiFlags = iFormFlags;
        ruiFlags = ruiFlags | kFormFlag_Altered;
    }
    else {
        if (GetAltered() && pAlteredForms) {
            for (uint32_t i = 0; i < pAlteredForms->GetSize(); ++i) {
                // SWEEP-INLINE 0x877A30
                if (pAlteredForms->GetAt(i) == this) {
                    pAlteredForms->RemoveAt(i);
                    break;
                }
            }
        }
        uint32_t& ruiFlags = iFormFlags;
        ruiFlags = ruiFlags & ~kFormFlag_Altered;
    }
}

void TESForm::SetQuestObject(bool abQuest) {
    uint32_t v2;
    if (abQuest) {
        v2 = iFormFlags | 1024;
    }
    else {
        v2 = iFormFlags & (~0x400u);
    }
    iFormFlags = v2;
    AddChange(1u);
}

void TESForm::SetHasSpokenFlag(bool abSpoke) {
    uint32_t v2;
    if (abSpoke) {
        v2 = iFormFlags | 64;
    }
    else {
        v2 = iFormFlags & 0xFFFFFFBFu;
    }
    iFormFlags = v2;
    AddChange(1);
}

void TESForm::SetHavokDeath(bool abDeath) {
    uint32_t v2;
    if (abDeath) {
        v2 = iFormFlags | 65536;
    }
    else {
        v2 = iFormFlags & (~0x10000u);
    }
    iFormFlags = v2;
    AddChange(1u);
}

void TESForm::SetNeedToChangeProcess(bool abChange) {
    if (abChange) {
        iFormFlags |= 0x20000;
    }
    else {
        iFormFlags &= ~0x20000u;
    }
}

void TESForm::Copy(TESForm* apCopy) {
    const char* pFormString = apCopy->GetFormTypeString();
    const char* v2 = apCopy->GetFormEditorID();
    Warning("FORMS: Trying to copy form \"%s\".\r\nNo copy function for %s form types.\r\n", v2, pFormString);
}

bool TESForm::Compare(TESForm* apCopy) {
    const char* pFormString = apCopy->GetFormTypeString();
    const char* v2 = apCopy->GetFormEditorID();
    Warning("FORMS: Trying to compare form \"%s\".\r\nNo compare function for %s form types.\r\n", v2, pFormString);
    return false;
}

// Whether apGroupFORM is the top-level group of this form's type: a 'GRUP' header of type FGT_TYPE whose label is the
// type's record code. The two flags are not read.
bool TESForm::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
    bool bBelongs = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[GRUP_ID].uiFormString && apGroupFORM->iFormID == FGT_TYPE &&
        apGroupFORM->flags == formEnumString[GetFormType()].uiFormString) {
        bBelongs = true;
    }
    return bBelongs;
}

void TESForm::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
    if ((apOutGroupFORM) && (!apParentGroup)) {
        apOutGroupFORM->form = formEnumString[GRUP_ID].uiFormString;
        apOutGroupFORM->iFormID = 0;
        apOutGroupFORM->flags = formEnumString[GetFormType()].uiFormString;
        apOutGroupFORM->length = 0;
        apOutGroupFORM->iVersionControl = 0;
        apOutGroupFORM->sFormVersion = 0;
        apOutGroupFORM->sVCVersion = 0;
    }
}

void TESForm::SetFormID(uint32_t aiID, bool abUpdateFile) {
    if (aiID == iFormID) {
        return;
    }

    if (!GetTemporary()) {
        if (iFormID) {
            pAllForms->RemoveAt(iFormID);
        }

        if (abUpdateFile && iFormID) {
            pDataHandler->RemoveIDFromDataHandler(iFormID);
        }

        if (aiID) {
            pAllForms->SetAt(aiID, this);
        }
    }

    iFormID = aiID;
}

const char* TESForm::GetFormEditorID() {
    return "";
}

void TESForm::LoadGameDataOLD(void* apData, int32_t aiSize) {
    pSaveLoadGameOLD->LoadGameDataOLD(apData, aiSize);
}

void TESForm::SetVATSTargetOverride(bool abVal) {
    if (abVal) {
        iFormFlags |= 0x4000000;
    }
    else {
        iFormFlags &= ~0x4000000u;
    }
    AddChange(1);
}

ENUM_FORM_ID TESForm::GetFormType() const {
    return static_cast<ENUM_FORM_ID>(cFormType);
}

void TESForm::SetFormType(uint8_t aType) {
    cFormType = aType;
}

// The file at `aiIndex` among the files that define or override this form, or the last one for -1. GetFileList is
// called on a const form, so the const is cast away for it.
TESFile* TESForm::GetFile(int32_t aiIndex) const {
    BSSimpleList<TESFile*>* pNode = const_cast<TESForm*>(this)->GetFileList();
    int32_t count = 0;
    TESFile* file = nullptr;
    while (pNode) {
        TESFile* pItem = pNode->GetItem();
        pNode = pNode->GetNext();
        if (pItem) {
            file = pItem;
            if (aiIndex != -1 && ++count > aiIndex) {
                break;
            }
        }
    }
    return file;
}

BSSimpleList<TESFile*>* TESForm::GetFileList() {
    return &pSourceFiles;
}

uint32_t TESForm::GetFormFlags() const {
    return iFormFlags;
}

void TESForm::SetFormFlags(uint32_t aiFlags) {
    iFormFlags = aiFlags;
}

bool TESForm::GetMaster() const {
    return (iFormFlags & kFormFlag_Master) != 0;
}

void TESForm::SetMaster(bool abMaster) {
    if (abMaster) {
        iFormFlags |= kFormFlag_Master;
    }
    else {
        iFormFlags &= ~kFormFlag_Master;
    }
}

bool TESForm::GetAltered() const {
    return (iFormFlags & kFormFlag_Altered) != 0;
}

bool TESForm::GetInitialized() const {
    return (iFormFlags & kFormFlag_Initialized) != 0;
}

void TESForm::SetInitialized(bool abInit) {
    if (abInit) {
        iFormFlags |= kFormFlag_Initialized;
    }
    else {
        iFormFlags &= ~kFormFlag_Initialized;
    }
}

bool TESForm::GetDelete() const {
    return (iFormFlags & kFormFlag_Deleted) != 0;
}

bool TESForm::GetDisabled() const {
    return (iFormFlags & kFormFlag_Disabled) != 0;
}

bool TESForm::GetDisabledOrDeleted() const {
    return (iFormFlags & (kFormFlag_Disabled | kFormFlag_Deleted)) != 0;
}

bool TESForm::GetTemporary() const {
    return (iFormFlags & kFormFlag_Temporary) != 0;
}

bool TESForm::GetMustBeVisibleDistant() const {
    return (iFormFlags & kFormFlag_MustBeVisibleDistant) != 0;
}

bool TESForm::IsStillLoading() const {
    return (iFormFlags & kFormFlag_StillLoading) != 0;
}

void TESForm::SetStillLoading(bool abVal) {
    if (abVal) {
        iFormFlags |= kFormFlag_StillLoading;
    }
    else {
        iFormFlags &= ~kFormFlag_StillLoading;
    }
}

bool TESForm::GetDestroyed() const {
    return (iFormFlags & kFormFlag_Destroyed) != 0;
}

bool TESForm::GetNoCollision() const {
    return (iFormFlags & 0x10) != 0;
}

void TESForm::SetNoCollision(bool abNoCollision) {
    if (abNoCollision) {
        iFormFlags = iFormFlags | 0x10;
    }
    else {
        iFormFlags = iFormFlags & ~0x10u;
    }
}

bool TESForm::GetLODTree() const {
    return (iFormFlags & 0x40) != 0;
}

bool TESForm::GetDestructible() const {
    return (iFormFlags & 0x1000000) != 0;
}

void TESForm::SetResetDestruct(bool abVal) {
    uint32_t v2;
    if (abVal) {
        v2 = iFormFlags | 8192;
    }
    else {
        v2 = iFormFlags & (~0x2000u);
    }
    iFormFlags = v2;
}

void TESForm::SetEmpty(bool abEmpty) {
    if (abEmpty) {
        if (!pSaveLoadGameOLD->GetLoading()) {
            AddChange(CHANGE_OBJECT_EMPTY);
        }
        iFormFlags |= 0x2000;
    }
    else {
        if (!pSaveLoadGameOLD->GetLoading()) {
            RemoveChange(CHANGE_OBJECT_EMPTY);
        }
        iFormFlags &= ~0x2000u;
    }
}

bool TESForm::GetDisableFade() const {
    return (iFormFlags & 0x8000000) != 0;
}

bool TESForm::GetFireOff() const {
    return ((iFormFlags & 0x80u) != 0);
}

void TESForm::SetHasTemp3D(bool abVal) {
    uint32_t iFlags = GetFormFlags();
    if (abVal) {
        iFlags |= kFormFlag_HasTemp3D;
    }
    else {
        iFlags &= ~kFormFlag_HasTemp3D;
    }
    SetFormFlags(iFlags);
}

// Change flag 1 (the form flags) has no name in CHANGE_TYPE.
void TESForm::SetDestroyed(bool abVal) {
    if (abVal) {
        iFormFlags |= kFormFlag_Destroyed;
    }
    else {
        iFormFlags &= ~kFormFlag_Destroyed;
    }
    AddChange(1);
}

void TESForm::SetDestructible(bool abVal) {
    if (abVal) {
        iFormFlags |= 0x1000000;
    }
    else {
        iFormFlags &= ~0x1000000u;
    }
}

void TESForm::SetDisabled(bool abDisabled) {
    if (abDisabled) {
        iFormFlags |= kFormFlag_Disabled;
    }
    else {
        iFormFlags &= ~kFormFlag_Disabled;
    }

    AddChange(1);
}

// GetFileList is not const, so the const is cast away to call it on a const form.
uint32_t TESForm::GetFileCount() const {
    return const_cast<TESForm*>(this)->GetFileList()->ItemsInList();
}

// The last master file among the files that define or override this form. GetFileList is called on a const form, so the
// const is cast away for it.
TESFile* TESForm::GetOwnerMaster() const {
    BSSimpleList<TESFile*>* pNode = const_cast<TESForm*>(this)->GetFileList();
    TESFile* owner = nullptr;
    while (pNode && !pNode->IsEmpty()) {
        TESFile* file = pNode->GetItem();
        pNode = pNode->GetNext();
        if (file->GetMaster()) {
            owner = file;
        }
    }
    return owner;
}

void TESForm::LoadForm(TESFile* apFile) {
    apFile->LoadForm(this);
}

// Copies the DATA chunk into apData (at most asSize bytes), then reads the values of the form's components that the
// record stores after the first asSize bytes, each only while the chunk has bytes left.
// KEEP: an uninitialised read. Each value is read whole once its first byte is inside the chunk, so a chunk that ends
// inside a value (or one GetChunkData leaves unfilled) has the exe read the alloca's unwritten bytes (checkbody:
// SetFormValue's argument differs, exe and src each reading their own stack garbage).
void TESForm::LoadData(TESFile* apFile, void* apData, uint16_t asSize) {
    uint16_t sChunkSize = apFile->GetChunkSize();
    uint16_t sOffset = asSize;
    // A 16-byte aligned stack buffer of the chunk's size, holding raw bytes.
    uint8_t* pBuffer = static_cast<uint8_t*>(XFNV_ALLOCA16(sChunkSize));
    apFile->GetChunkData(pBuffer, sChunkSize);
    ourMemcpy(apData, pBuffer, Min(sChunkSize, asSize));
    TESUsesForm* pUses = dynamic_cast<TESUsesForm*>(this);
    TESValueForm* pValue = dynamic_cast<TESValueForm*>(this);
    TESHealthForm* pHealth = dynamic_cast<TESHealthForm*>(this);
    TESWeightForm* pWeight = dynamic_cast<TESWeightForm*>(this);
    TESQualityForm* pQuality = dynamic_cast<TESQualityForm*>(this);
    TESAttackDamageForm* pAttack = dynamic_cast<TESAttackDamageForm*>(this);
    TESAttributes* pAttributes = dynamic_cast<TESAttributes*>(this);
    BGSClipRoundsForm* pClipRounds = dynamic_cast<BGSClipRoundsForm*>(this);
    if ((pUses) && (sOffset < sChunkSize)) {
        pUses->iUses = pBuffer[sOffset];
        ++sOffset;
    }

    if ((pValue) && (sOffset < sChunkSize)) {
        int32_t iValue = *reinterpret_cast<int32_t*>(&pBuffer[sOffset]);
        sOffset += 4;
        if (apFile->QEndian()) {
            ENDIAN(iValue, false);
        }
        pValue->SetFormValue(iValue);
    }

    if ((pHealth) && (sOffset < sChunkSize)) {
        uint32_t iHealth = *reinterpret_cast<uint32_t*>(&pBuffer[sOffset]);
        sOffset += 4;
        if (apFile->QEndian()) {
            ENDIAN(iHealth, false);
        }
        pHealth->iHealth = iHealth;
    }

    if ((pWeight) && (sOffset < sChunkSize)) {
        float fRead;
        ourMemcpy(&fRead, &pBuffer[sOffset], sizeof(fRead));
        float fWeight = fRead;
        sOffset += 4;
        if (apFile->QEndian()) {
            ENDIAN(fWeight, false);
        }
        pWeight->SetFormWeight(fWeight);
    }

    if ((pQuality) && (sOffset < sChunkSize)) {
        float fRead;
        ourMemcpy(&fRead, &pBuffer[sOffset], sizeof(fRead));
        float fQuality = fRead;
        sOffset += 4;
        if (apFile->QEndian()) {
            ENDIAN(fQuality, false);
        }
        // Truncates the float to an int and stores its low byte.
        pQuality->cQuality = static_cast<uint8_t>(static_cast<int32_t>(fQuality));
    }

    if ((pAttack) && (sOffset < sChunkSize)) {
        uint16_t sDamage = *reinterpret_cast<uint16_t*>(&pBuffer[sOffset]);
        sOffset += 2;
        if (apFile->QEndian()) {
            ENDIAN(sDamage, false);
        }
        pAttack->SetDamage(sDamage);
    }

    if (pAttributes) {
        sOffset += pAttributes->Load(&pBuffer[sOffset]);
    }
    if ((pClipRounds) && (sOffset < sChunkSize)) {
        pClipRounds->cClipRounds = pBuffer[sOffset];
        ++sOffset;
    }
}

void TESForm::ForceChange(int32_t aiChangeFlags) {
    pSaveLoadGame->AddChange(this, BGSChangeFlags{aiChangeFlags}, true);
}

uint32_t TESForm::GetFormID() const {
    return iFormID;
}

// the form ID without its load-order byte
uint32_t TESForm::GetFormIDWithoutIndex() const {
    return iFormID & 0xFFFFFF;
}

void TESForm::SetTemporary() {
    RemoveFromDataStructures();
    if (pSaveLoadGameOLD) {
        pSaveLoadGameOLD->RemoveChanges(this, false);
    }
    if (pSaveLoadGame) {
        pSaveLoadGame->RemoveChanges(this);
    }
    iFormFlags |= kFormFlag_Temporary;
}

void TESForm::RemoveFromDataStructures() {
    if (GetTemporary()) {
        return;
    }

    if (pAllForms) {
        pAllForms->RemoveAt(iFormID);
    }

    for (uint32_t i = 0; pAlteredForms && i < pAlteredForms->GetSize(); ++i) {
        if (pAlteredForms->GetAt(i) == this) {
            pAlteredForms->RemoveAt(i);
            break;
        }
    }

    if (pDataHandler) {
        pDataHandler->RemoveIDFromDataHandler(iFormID);
    }
}

void TESForm::ClearDataAllComponents() {
    switch (GetFormType()) {
        case SCPT_ID:
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case LAND_ID:
        case NAVM_ID:
        case PACK_ID:
        case PCBE_ID: TESForm::ClearDataComponent(); break;
        case CELL_ID: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(this);
            pCell->TESFullName::ClearDataComponent();
            TESForm::ClearDataComponent();
            break;
        }
        default: {
            alignas(FormComponentCollection) uint8_t collectionMemory[sizeof(FormComponentCollection)];
            FormComponentCollection* pCollection = ::new (collectionMemory) FormComponentCollection;
            pCollection->InitFormComponentArray(this);
            pCollection->ClearDataAllComponents();
            pCollection->Destruct();
            break;
        }
    }
}

void TESForm::InitializeDataAllComponents() {
    switch (GetFormType()) {
        case SCPT_ID:
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case LAND_ID:
        case NAVM_ID:
        case PACK_ID:
        case PCBE_ID: TESForm::InitializeDataComponent(); break;
        case CELL_ID: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(this);
            pCell->TESFullName::InitializeDataComponent();
            TESForm::InitializeDataComponent();
            break;
        }
        default: {
            alignas(FormComponentCollection) uint8_t collectionMemory[sizeof(FormComponentCollection)];
            FormComponentCollection* pCollection = ::new (collectionMemory) FormComponentCollection;
            pCollection->InitFormComponentArray(this);
            pCollection->InitializeDataAllComponents();
            pCollection->Destruct();
            break;
        }
    }
}

// Makes apFile (or its thread-safe parent) the last file of the form. A null file drops the last one; a master
// first drops every non-master file.
void TESForm::SetFile(TESFile* apFile) {
    TESFile* pParent = apFile ? apFile->GetThreadSafeParent() : nullptr;
    if (pParent) {
        apFile = pParent;
    }

    if (!apFile) {
        BSSimpleList<TESFile*>* pNode = GetFileList();
        BSSimpleList<TESFile*>* pLast = nullptr;
        while (pNode) {
            if (pNode->GetItem()) {
                pLast = pNode;
            }
            pNode = pNode->GetNext();
        }
        if (pLast) {
            pLast->RemoveHead();
        }
        else {
            GetFileList()->RemoveHead();
        }
    }
    else if (apFile->GetMaster()) {
        BSSimpleList<TESFile*>* pNode = GetFileList();
        BSSimpleList<TESFile*>* pPrev = nullptr;
        while (pNode && !pNode->IsEmpty()) {
            TESFile* pFile = pNode->GetItem();
            if (pFile->GetMaster()) {
                pPrev = pNode;
                pNode = pNode->GetNext();
            }
            else if (pPrev) {
                pPrev->Remove(pFile);
                pNode = pPrev->GetNext();
            }
            else {
                pNode->RemoveHead();
            }
        }
        GetFileList()->AddTail(apFile);
    }
    else {
        BSSimpleList<TESFile*>* pNode = GetFileList();
        BSSimpleList<TESFile*>* pLast = nullptr;
        while (pNode) {
            TESFile* pFile = pNode->GetItem();
            if (pFile) {
                pLast = pNode;
                if (pFile == apFile) {
                    return;
                }
            }
            pNode = pNode->GetNext();
        }
        if (pLast) {
            pLast->AddTail(apFile);
        }
        else {
            GetFileList()->AddHead(apFile);
        }
    }
}

uint32_t TESForm::GetDataSizeAdded() {
    TESUsesForm* pUses = dynamic_cast<TESUsesForm*>(this);
    TESValueForm* pValue = dynamic_cast<TESValueForm*>(this);
    TESHealthForm* pHealth = dynamic_cast<TESHealthForm*>(this);
    TESWeightForm* pWeight = dynamic_cast<TESWeightForm*>(this);
    TESQualityForm* pQuality = dynamic_cast<TESQualityForm*>(this);
    TESAttackDamageForm* pAttackDamage = dynamic_cast<TESAttackDamageForm*>(this);
    TESAttributes* pAttributes = dynamic_cast<TESAttributes*>(this);
    BGSClipRoundsForm* pClipRounds = dynamic_cast<BGSClipRoundsForm*>(this);

    uint32_t iSize = 0;
    if (pUses) {
        iSize += 1;
    }
    if (pValue) {
        iSize += 4;
    }
    if (pHealth) {
        iSize += 4;
    }
    if (pWeight) {
        iSize += 4;
    }
    if (pQuality) {
        iSize += 4;
    }
    if (pAttackDamage) {
        iSize += 2;
    }
    if (pAttributes) {
        iSize += pAttributes->GetAttributeDataSize();
    }
    if (pClipRounds) {
        iSize += 1;
    }
    return iSize;
}

const char* TESForm::GetFormTypeString() const {
    return formEnumString[cFormType].pFormString;
}

bool TESForm::IsDefaultForm() {
    return IsDefaultForm(GetFormID());
}

// Adds the DATA chunk, then extends it with the values of the form's components that the record stores after it.
void TESForm::__SaveData(void* apData, uint32_t aiSize) {
    float ftemp_3;
    float ftemp_2;
    if (aiSize <= 0xFFFF) {
        uint16_t sSize = 0;
        uint32_t iBufferSize = GetFormBufferSize();
        TESForm::__AddChunkData(DATA_ID, apData, aiSize);
        // The form buffer is raw bytes: the chunk just added starts at the old buffer size.
        CHUNK* pChunk = reinterpret_cast<CHUNK*>(static_cast<char*>(GetFormBuffer()) + iBufferSize);
        TESUsesForm* pUses = dynamic_cast<TESUsesForm*>(this);
        TESValueForm* pValue = dynamic_cast<TESValueForm*>(this);
        TESHealthForm* pHealth = dynamic_cast<TESHealthForm*>(this);
        TESWeightForm* pWeight = dynamic_cast<TESWeightForm*>(this);
        TESQualityForm* pQuality = dynamic_cast<TESQualityForm*>(this);
        TESAttackDamageForm* pAttack = dynamic_cast<TESAttackDamageForm*>(this);
        TESAttributes* pAttributes = dynamic_cast<TESAttributes*>(this);
        BGSClipRoundsForm* pClipRounds = dynamic_cast<BGSClipRoundsForm*>(this);
        if (pUses) {
            ++sSize;
        }
        if (pValue) {
            sSize += 4;
        }
        if (pHealth) {
            sSize += 4;
        }
        if (pWeight) {
            sSize += 4;
        }
        if (pQuality) {
            sSize += 4;
        }
        if (pAttack) {
            sSize += 2;
        }
        if (pAttributes) {
            sSize += pAttributes->GetAttributeDataSize();
        }
        if (pClipRounds) {
            ++sSize;
        }
        if (sSize) {
            iBufferSize = GetFormBufferSize();
            if (ExpandChunk(pChunk, sSize)) {
                // The chunk's data is raw bytes; the values are written unaligned.
                char* pData = static_cast<char*>(GetFormBuffer()) + iBufferSize;
                if (pUses) {
                    *pData++ = pUses->iUses;
                }
                if (pValue) {
                    *reinterpret_cast<uint32_t*>(pData) = pValue->iValue;
                    if (TESForm::QEndianSwapOnSave()) {
                        ENDIAN(*reinterpret_cast<uint32_t*>(pData), false);
                    }
                    pData += 4;
                }
                if (pHealth) {
                    *reinterpret_cast<uint32_t*>(pData) = pHealth->iHealth;
                    if (TESForm::QEndianSwapOnSave()) {
                        ENDIAN(*reinterpret_cast<uint32_t*>(pData), false);
                    }
                    pData += 4;
                }
                if (pWeight) {
                    ftemp_2 = pWeight->GetWeight();
                    if (TESForm::QEndianSwapOnSave()) {
                        ENDIAN<float>(ftemp_2, false);
                    }
                    ourMemcpy(pData, &ftemp_2, 4u);
                    pData += 4;
                }
                if (pQuality) {
                    ftemp_3 = static_cast<float>(pQuality->cQuality);
                    if (TESForm::QEndianSwapOnSave()) {
                        ENDIAN<float>(ftemp_3, false);
                    }
                    ourMemcpy(pData, &ftemp_3, 4u);
                    pData += 4;
                }
                if (pAttack) {
                    *reinterpret_cast<uint16_t*>(pData) = pAttack->GetAttackDamage();
                    if (TESForm::QEndianSwapOnSave()) {
                        ENDIAN(*reinterpret_cast<uint16_t*>(pData), false);
                    }
                    pData += 2;
                }
                if (pAttributes) {
                    pData += pAttributes->Save(pData);
                }
                if (pClipRounds) {
                    *pData++ = pClipRounds->cClipRounds;
                }
            }
        }
    }
    else {
        Warning("MASTERFILE: Trying to SaveData that's too large.");
    }
}

void TESForm::SaveNumericID(uint32_t* apNumericID, int32_t aiSize) {
    pSaveLoadGameOLD->SaveNumericID(apNumericID, aiSize);
}

void TESForm::LoadNumericID(uint32_t* apNumericID, int32_t aiSize) {
    pSaveLoadGameOLD->LoadNumericID(apNumericID, aiSize);
}

void TESForm::SaveData() {
    __SaveData(nullptr, 0);
}

void TESForm::StartForm() {
    if (!GetTemporary()) {
        TESForm::iBufferSize = 24;
        TESForm::pFormBuffer = static_cast<FORM*>(ourMalloc(sizeof(FORM)));
        FORM* pForm = TESForm::pFormBuffer;
        pForm->flags = iFormFlags;
        if (GetFormType() != TES4_ID) {
            pForm->flags &= 0x30032FE0;
        }
        pForm->form = formEnumString[GetFormType()].uiFormString;
        pForm->iFormID = iFormID;
        pForm->length = 0;
        pForm->sFormVersion = TESFile::GetFileFormVersion();
        pForm->iVersionControl = 0;
        pForm->sVCVersion = 0;
        SaveObjectBound();
    }
}

void TESForm::CopyAllComponents(TESForm* apCopy) {
    FormComponentCollection th_s;
    FormComponentCollection a2;
    th_s.InitFormComponentArray(this);
    a2.InitFormComponentArray(apCopy);
    th_s.CopyAllComponents(&a2);
    a2.Destruct();
    th_s.Destruct();
}

// Whether every component of this form equals the same component of apCompare; true when there is none to compare.
bool TESForm::CompareAllComponents(TESForm* apCompare) {
    if (!apCompare) {
        return true;
    }
    alignas(FormComponentCollection) uint8_t thisMemory[sizeof(FormComponentCollection)];
    FormComponentCollection* pThisFCC = ::new (thisMemory) FormComponentCollection;
    alignas(FormComponentCollection) uint8_t compareMemory[sizeof(FormComponentCollection)];
    FormComponentCollection* pCompareFCC = ::new (compareMemory) FormComponentCollection;
    pThisFCC->InitFormComponentArray(this);
    pCompareFCC->InitFormComponentArray(apCompare);
    bool bResult = pThisFCC->CompareAllComponents(pCompareFCC);
    pCompareFCC->Destruct();
    pThisFCC->Destruct();
    return bResult;
}

void TESForm::SaveGameDataOLD(const void* apData, int32_t aiSize) {
    pSaveLoadGameOLD->SaveGameDataOLD(apData, aiSize);
}

void TESForm::CloseForm() {
    if (!GetTemporary()) {
        FORM* pForm = TESForm::pFormBuffer;
        TESForm::pFormBuffer->length = TESForm::iBufferSize - 24;
        if (TESForm::QEndianSwapOnSave()) {
            pForm->Endian();
        }
    }
}

void TESForm::SetFireOff(bool abVal) {
    uint32_t v2;
    if (abVal) {
        v2 = iFormFlags | 128;
    }
    else {
        v2 = iFormFlags & (~0x80u);
    }
    iFormFlags = v2;
}

bool TESForm::FormIDMatchesWithoutIndex(uint32_t aiID) const {
    return (bool)((uint8_t)((aiID & 16777215) == GetFormIDWithoutIndex()));
}

bool TESForm::IsRecentPCDrop() const {
    return ((iFormFlags & 4194304) != 0);
}

bool TESForm::GetVATSTargetOverride() const {
    return ((iFormFlags & 67108864) != 0);
}

void* TESForm::GetFormBuffer() {
    return TESForm::pFormBuffer;
}

uint32_t TESForm::GetFormBufferSize() {
    return TESForm::iBufferSize;
}

void TESForm::FreeFormBuffer() {
    ourFree(TESForm::pFormBuffer);
    TESForm::pFormBuffer = nullptr;
}

int32_t TESForm::GetLODFormID() {
    int32_t uiRelativeFormID = (int32_t)GetFormIDWithoutIndex();
    TESFile* pFile = GetFile(0);
    BSSimpleList<char*>* pListMasters = pFile->GetMasterList();
    return uiRelativeFormID | (((uint8_t)(!pListMasters->IsEmpty())) << 24);
}

void TESForm::SetDisableFade(bool abVal) {
    uint32_t iFormFlags;
    uint32_t v3;
    if (abVal) {
        this->iFormFlags |= 134217728;
        iFormFlags = this->iFormFlags;
    }
    else {
        this->iFormFlags &= (~0x8000000u);
        v3 = this->iFormFlags;
    }
    uint32_t v4 = this->iFormFlags;
}

bool TESForm::ExpandChunk(CHUNK* apChunk, uint16_t asExpandSize) {
    uint8_t endianSwapOnSave = (uint8_t)TESForm::QEndianSwapOnSave();
    if (endianSwapOnSave) {
        apChunk->Endian();
    }
    if ((apChunk->length || asExpandSize) && ((asExpandSize + apChunk->length) <= 65535)) {
        apChunk->length += asExpandSize;
        if (endianSwapOnSave) {
            apChunk->Endian();
        }
        TESForm::iBufferSize += asExpandSize;
        TESForm::pFormBuffer = static_cast<FORM*>(ourRealloc(TESForm::pFormBuffer, TESForm::iBufferSize));
        return true;
    }

    if (endianSwapOnSave) {
        apChunk->Endian();
    }
    return false;
}

void TESForm::SetInPlaceableWater(bool abVal) {
    uint32_t iFormFlags;
    uint32_t v3;
    if (abVal) {
        this->iFormFlags |= 0x40u;
        iFormFlags = this->iFormFlags;
    }
    else {
        this->iFormFlags &= (~0x40u);
        v3 = this->iFormFlags;
    }
    uint32_t v4 = this->iFormFlags;
    AddChange(1u);
}

const char* TESForm::GetFormTypeString(uint8_t aiFormType) {
    if (aiFormType >= 121u) {
        return "";
    }

    return formEnumString[aiFormType].pFormString;
}

bool TESForm::IsDefaultForm(uint32_t aiID) {
    return aiID && aiID <= 0x7FF;
}

TESForm* TESForm::GetFormByNumericID(uint32_t aiSearchID) {
    TESForm* form = nullptr;

    if (pAllForms && pAllForms->GetAt(aiSearchID, form)) {
        return form;
    }

    return nullptr;
}

// The form type whose record code is `aiFormString`, or 0 when none matches; the last hit is cached.
ENUM_FORM_ID TESForm::GetFormTypeFromFormString(uint32_t aiFormString) {
    if (aiFormString == iLastFormString) {
        return static_cast<ENUM_FORM_ID>(iLastFormType);
    }

    for (int32_t type = 0; type < FORM_ID_COUNT; ++type) {
        if (formEnumString[type].uiFormString == aiFormString) {
            iLastFormType = type;
            iLastFormString = aiFormString;
            return static_cast<ENUM_FORM_ID>(type);
        }
    }

    return NO_FORM;
}

TESForm* TESForm::GetFormByEditorID(const char* apSearchID) {
    TESForm* form = nullptr;

    if (apSearchID && *apSearchID && pAllFormsByEditorID && pAllFormsByEditorID->GetAt(apSearchID, form)) {
        return form;
    }

    return nullptr;
}

// Replaces the load-order byte of `*aiID` with the compile index of the file that owns the form.
void TESForm::AddCompileIndex(uint32_t& aiID, TESFile* apFile) {
    if (pSaveLoadGameOLD->GetLoading()) {
        aiID = TESSaveLoadGame_GetSaveFormID(pSaveLoadGameOLD, aiID);
        return;
    }

    if (IsDefaultForm(aiID) || !apFile) {
        return;
    }

    TESFile* indexFile = apFile->GetIndexFile((aiID >> 24) + 1);
    uint32_t compileIndex = (indexFile ? indexFile : apFile)->GetCompileIndex();
    aiID = (compileIndex << 24) | (aiID & 0xFFFFFF);
}

[[clang::disable_tail_calls]] void TESForm::AddChunkArray(CHUNK_ID aeChunkID, const char* apData, uint32_t aiSize) {
    __AddChunkData(aeChunkID, apData, aiSize);
}

bool TESForm::QEndianSwapOnSave() {
    return (bool)((int8_t)TESForm::bEndianSwapOnSave);
}

// Appends a chunk (its 6-byte header, then aiSize bytes of apData) to the form buffer; a chunk longer than 0xFFFF
// bytes is preceded by an XXXX chunk holding its size, and its own header's length is 0.
void TESForm::__AddChunkData(CHUNK_ID aeChunkID, const void* apData, uint32_t aiSize) {
    uint16_t sSize = aiSize;
    if (aiSize > 0xFFFF) {
        TESForm::AddChunk(XXXX_ID, aiSize);
        sSize = 0;
    }
    uint32_t iStartingSize = TESForm::iBufferSize;
    TESForm::iBufferSize += aiSize + sizeof(CHUNK);
    TESForm::pFormBuffer = static_cast<FORM*>(ourRealloc(TESForm::pFormBuffer, TESForm::iBufferSize));
    // The buffer is the FORM header followed by raw chunk bytes.
    CHUNK* pChunk = reinterpret_cast<CHUNK*>(reinterpret_cast<char*>(TESForm::pFormBuffer) + iStartingSize);
    char* pChunkHeader = reinterpret_cast<char*>(TESForm::pFormBuffer) + iStartingSize;
    pChunk->chunk = aeChunkID;
    pChunk->length = sSize;
    if (TESForm::QEndianSwapOnSave()) {
        pChunk->Endian();
    }
    ourMemcpy(pChunkHeader, pChunk, sizeof(CHUNK));
    ourMemcpy(reinterpret_cast<char*>(TESForm::pFormBuffer) + iStartingSize + sizeof(CHUNK), apData, aiSize);
}

[[clang::disable_tail_calls]] void TESForm::AddChunk(CHUNK_ID aeChunkID, uint8_t aiData) {
    TESForm::__AddChunkData(aeChunkID, &aiData, 1);
}

[[clang::disable_tail_calls]] void TESForm::AddChunk(CHUNK_ID aeChunkID, uint16_t asData) {
    if (TESForm::QEndianSwapOnSave()) {
        ENDIAN(asData, false);
    }

    TESForm::__AddChunkData(aeChunkID, &asData, 2);
}

[[clang::disable_tail_calls]] void TESForm::AddChunk(CHUNK_ID aeChunkID, uint32_t aiData) {
    if (TESForm::QEndianSwapOnSave()) {
        ENDIAN(aiData, false);
    }

    TESForm::__AddChunkData(aeChunkID, &aiData, 4);
}

[[clang::disable_tail_calls]] void TESForm::AddChunk(CHUNK_ID aeChunkID, float afData) {
    if (TESForm::QEndianSwapOnSave()) {
        ENDIAN(afData, false);
    }

    TESForm::__AddChunkData(aeChunkID, &afData, 4);
}

[[clang::disable_tail_calls]] void TESForm::AddChunk(CHUNK_ID aeChunkID, int32_t aiData) {
    if (TESForm::QEndianSwapOnSave()) {
        ENDIAN(aiData, false);
    }

    TESForm::__AddChunkData(aeChunkID, &aiData, 4);
}

void TESForm::AddChunk(CHUNK_ID aeChunkID) {
    TESForm::__AddChunkData(aeChunkID, nullptr, 0);
}

bool TESForm::IsInteriorCellOrWorldspace(TESForm* apForm) {
    bool v4 = false;
    if (apForm) {
        int32_t FormType = apForm->GetFormType();
        if (FormType == 57) {
            TESObjectCELL* v3 = dynamic_cast<TESObjectCELL*>(apForm);
            if (v3) {
                return v3->GetInterior();
            }
        }
        else {
            if (FormType == 65) {
                return true;
            }
        }
    }
    return v4;
}

// The length of apForm's full name, 0 when it has none.
uint32_t TESForm::GetNameLen(TESForm* apForm) {
    TESFullName* pFullName = dynamic_cast<TESFullName*>(apForm);
    if (pFullName) {
        return pFullName->GetFullNameLength();
    }

    return 0;
}

void TESForm::AddChunkArray(CHUNK_ID aeChunkID, const uint32_t* apData, uint32_t aiCount) {
    TESForm::AddChunkArray32(aeChunkID, apData, aiCount);
}

void TESForm::AddChunkArray(CHUNK_ID aeChunkID, const float* apData, uint32_t aiCount) {
    TESForm::AddChunkArray32(aeChunkID, apData, aiCount);
}

void TESForm::AddChunkArray(CHUNK_ID aeChunkID, const uint16_t* apData, uint32_t aiCount) {
    TESForm::AddChunkArray16(aeChunkID, apData, aiCount);
}

void TESForm::AddChunkArray(CHUNK_ID aeChunkID, const int16_t* apData, uint32_t aiCount) {
    TESForm::AddChunkArray16(aeChunkID, apData, aiCount);
}

// Compresses the form buffer's record data with zlib: the FORM header (flagged compressed), the original size, then
// the deflated bytes.
void TESForm::CompressSaveBuffer() {
    uint32_t itempsize;
    z_stream_s zstream;
    if ((TESForm::pFormBuffer) && (TESForm::iBufferSize > 24)) {
        FORM* pFORM = TESForm::pFormBuffer;
        if (TESForm::QEndianSwapOnSave()) {
            pFORM->Endian();
        }
        if ((pFORM->flags & 0x40000) != 0) {
            if (TESForm::QEndianSwapOnSave()) {
                pFORM->Endian();
            }
        }
        else {
            uint32_t ioriginalsize = TESForm::iBufferSize - 24;
            // The record data after the FORM header is treated as raw bytes.
            uint8_t* apDest = reinterpret_cast<uint8_t*>(&TESForm::pFormBuffer[1]);
            zstream.zalloc = nullptr;
            zstream.zfree = nullptr;
            zstream.opaque = nullptr;
            zstream.state = nullptr;
            if (deflateInit(&zstream, Z_DEFAULT_COMPRESSION)) {
                Warning("SAVELOAD: Error initializing ZLib stream for deflate.");
            }
            else {
                uint32_t icompbuffsize = 2 * ioriginalsize;
                char* pcompressionbuffer = static_cast<char*>(ourMalloc(2 * ioriginalsize));
                zstream.avail_in = ioriginalsize;
                zstream.next_in = apDest;
                zstream.avail_out = icompbuffsize;
                zstream.next_out = reinterpret_cast<uint8_t*>(pcompressionbuffer);
                if (deflate(&zstream, Z_FINISH) == Z_STREAM_ERROR) {
                    Warning("SAVELOAD: Error deflating ZLib stream.");
                    ourFree(pcompressionbuffer);
                }
                else {
                    uint32_t icompressedsize = icompbuffsize - zstream.avail_out;
                    uint32_t inewbuffersize = icompbuffsize - zstream.avail_out + 28;
                    FORM* pbuffer = static_cast<FORM*>(ourMalloc(inewbuffersize));
                    pFORM->flags |= 0x40000;
                    pFORM->length = inewbuffersize - 24;
                    if (TESForm::QEndianSwapOnSave()) {
                        pFORM->Endian();
                    }
                    ourMemcpy(pbuffer, pFORM, sizeof(FORM));
                    itempsize = ioriginalsize;
                    if (TESForm::QEndianSwapOnSave()) {
                        ENDIAN<uint32_t>(itempsize, false);
                    }
                    ourMemcpy(&pbuffer[1], &itempsize, 4u);
                    ourMemcpy(&pbuffer[1].length, pcompressionbuffer, icompressedsize);
                    ourFree(TESForm::pFormBuffer);
                    TESForm::pFormBuffer = pbuffer;
                    TESForm::iBufferSize = inewbuffersize;
                    deflateEnd(&zstream);
                    ourFree(pcompressionbuffer);
                }
            }
        }
    }
}

void TESForm::AddChunkArray32(CHUNK_ID aeChunkID, const void* apData, uint32_t aiCount) {
    uint32_t iSize = aiCount * 4;
    if (QEndianSwapOnSave()) {
        uint32_t* pSwapped = static_cast<uint32_t*>(
            ourMalloc(aiCount > SIZE_MAX / sizeof(uint32_t) ? SIZE_MAX : sizeof(uint32_t) * aiCount)
        );
        ourMemcpy(pSwapped, apData, iSize);
        for (uint32_t i = 0; i < aiCount; ++i) {
            ENDIAN(pSwapped[i], false);
        }
        __AddChunkData(aeChunkID, pSwapped, iSize);
        ourFree(pSwapped);
    }
    else {
        __AddChunkData(aeChunkID, apData, iSize);
    }
}

void TESForm::AddChunkArray16(CHUNK_ID aeChunkID, const void* apData, uint32_t aiCount) {
    uint32_t iSize = aiCount * 2;
    if (QEndianSwapOnSave()) {
        uint16_t* pSwapped = static_cast<uint16_t*>(
            ourMalloc(aiCount > SIZE_MAX / sizeof(uint16_t) ? SIZE_MAX : sizeof(uint16_t) * aiCount)
        );
        ourMemcpy(pSwapped, apData, iSize);
        for (uint32_t i = 0; i < aiCount; ++i) {
            ENDIAN(pSwapped[i], false);
        }
        __AddChunkData(aeChunkID, pSwapped, iSize);
        ourFree(pSwapped);
    }
    else {
        __AddChunkData(aeChunkID, apData, iSize);
    }
}

void TESForm::InitializeFormDataStructures() {
    if (!pAllForms) {
        void* memory = ourMalloc(sizeof(TESForm::AllFormsMap));
        pAllForms = memory ? ::new (memory) TESForm::AllFormsMap(0x2008Du) : nullptr;
    }

    if (!pAllFormsByEditorID) {
        void* memory = ourMalloc(sizeof(BSTCaseInsensitiveStringMap<TESForm*>));
        pAllFormsByEditorID = memory ? ::new (memory) BSTCaseInsensitiveStringMap<TESForm*>(0xFA1u, true) : nullptr;
    }

    if (!pAlteredForms) {
        void* memory = ourMalloc(sizeof(NiTLargePrimitiveArray<TESForm*>));
        pAlteredForms = memory ? ::new (memory) NiTLargePrimitiveArray<TESForm*>(0x20u, 1) : nullptr;
    }
}

void TESForm::ReleaseFormDataStructures() {
    if (TESForm::pAllForms) {
        TESForm::pAllForms->RemoveAll();
        if (TESForm::pAllForms) {
            delete TESForm::pAllForms;
        }
        TESForm::pAllForms = nullptr;
    }
    if (TESForm::pAllFormsByEditorID) {
        TESForm::pAllFormsByEditorID->RemoveAll();
        if (TESForm::pAllFormsByEditorID) {
            delete TESForm::pAllFormsByEditorID;
        }
        TESForm::pAllFormsByEditorID = nullptr;
    }
    if (TESForm::pAlteredForms) {
        TESForm::pAlteredForms->RemoveAll();
        if (TESForm::pAlteredForms) {
            delete TESForm::pAlteredForms;
        }
        TESForm::pAlteredForms = nullptr;
    }
}
