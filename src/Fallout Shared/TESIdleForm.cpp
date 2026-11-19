#include "Fallout Shared/TESIdleForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/IDLE_ANIM_ROOT.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelAnim.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

void IDLE_DATA::Endian() {
    ENDIAN(sReplayDelay, false);
}

void IDLE_PARENT_IDS::Endian() {
    ENDIAN(iparentID, false);
    ENDIAN(iprevID, false);
}

TESIdleForm::TESIdleForm() : TESForm(), TESModelAnim(), Conditions(), pChildIdles(nullptr), strEditorID() {
    cFormType = IDLE_ID;
    TESIdleForm::InitializeData();
}

// Unlinks the idle from its parent or from the idle manager's lists (unless the data handler is clearing all data),
// then deletes its children.
TESIdleForm::~TESIdleForm() {
    TESIdleForm::ClearData();
    if (!pDataHandler->GetClearingData()) {
        if (GetParentIdle()) {
            GetParentIdle()->RemoveChild(this);
        }
        else {
            NiFormArray* parray = &pIdleManager->GetIdleAnimRoot(this)->IdleArray;
            if (parray) {
                uint32_t iindex = parray->GetIndexForForm(this);
                if (iindex != -1) {
                    TESIdleForm* pnextidle = static_cast<TESIdleForm*>(parray->GetFormAt(iindex + 1));
                    if (pnextidle) {
                        pnextidle->pPrevIdle = pPrevIdle;
                    }
                    parray->RemoveAt(iindex);
                    parray->Compact();
                }
            }
            NiFormArray* parray_2 = pIdleManager->GetLooseIdleArray(this);
            if (parray_2) {
                uint32_t iindex_2 = parray_2->GetIndexForForm(this);
                if (iindex_2 != -1) {
                    parray_2->RemoveAt(iindex_2);
                    parray_2->Compact();
                }
            }
        }
    }
    if (pChildIdles) {
        uint32_t icount = GetChildCount();
        for (uint32_t i = 0; i < icount; ++i) {
            TESIdleForm* pchildidle = GetChildByIndex(i);
            pchildidle->pParentIdle = nullptr;
            delete pchildidle;
        }
        delete pChildIdles;
    }
}

void TESIdleForm::InitializeData() {
    data.cAnimData = 7;
    data.cLoopMin = 0;
    data.cLoopMax = 0;
    data.sReplayDelay = 0;
    data.cFlagsEx = 0;
    pParentIdle = nullptr;
    pPrevIdle = nullptr;
}

void TESIdleForm::ClearData() {
    if ((pDataHandler->GetSaveLoad() && (!pDataHandler->GetClearingData())) && (pIdleManager->GetInitialized()) &&
        (TESForm::GetInitialized())) {
        RemoveFromManager();
    }

    TESCondition* mBRoomAccumList = TESIdleForm::GetConditions();
    mBRoomAccumList->ClearAllConditionItems();
}

bool TESIdleForm::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != IDLE_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // The buffer is sized by the chunk, but the read is capped at 0x200 bytes.
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            case DATA_ID:
                // LoadData reads the chunk as raw bytes.
                LoadData(apFile, reinterpret_cast<char*>(&data), 8);
                break;
            case CIDA_ID: GetConditions()->Load(apFile); break;
            case ANAM_ID: {
                IDLE_PARENT_IDS kIDs;
                ourMemset(&kIDs, 0, sizeof(kIDs));
                apFile->GetChunkData(&kIDs, 8);
                if (apFile->QEndian()) {
                    kIDs.Endian();
                }
                // The form IDs stay in the pointer fields until InitItem resolves them.
                pParentIdle = reinterpret_cast<TESIdleForm*>(static_cast<uintptr_t>(kIDs.iparentID));
                pPrevIdle = reinterpret_cast<TESIdleForm*>(static_cast<uintptr_t>(kIDs.iprevID));
                pIdleManager->AddLoadedIdle(this);
                break;
            }
            default: break;
        }
        if (eChunk == DATA_ID && apFile->QEndian()) {
            data.Endian();
        }
    }
    while (apFile->NextChunk());
    return true;
}

// The ANAM chunk holds the parent's form ID and that of the nearest previous idle that is not deleted; a deleted
// idle saves no previous idle. Both records are swapped around their writes when saving byte-swapped.
void TESIdleForm::Save() {
    StartForm();
    TESModel::Save(MODL_ID, MODT_ID);
    GetConditions()->Save();
    IDLE_PARENT_IDS kParentIDs;
    ourMemset(&kParentIDs, 0, sizeof(kParentIDs));
    if (GetParentIdle()) {
        kParentIDs.iparentID = GetParentIdle()->GetFormID();
    }
    if (!GetDelete()) {
        TESIdleForm* pPrev = GetPrevIdle();
        while (pPrev && pPrev->GetDelete()) {
            pPrev = pPrev->GetPrevIdle();
        }
        if (pPrev) {
            kParentIDs.iprevID = pPrev->GetFormID();
        }
    }
    if (QEndianSwapOnSave()) {
        kParentIDs.Endian();
    }
    __AddChunkData(ANAM_ID, &kParentIDs, sizeof(kParentIDs));
    if (QEndianSwapOnSave()) {
        kParentIDs.Endian();
    }
    if (QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, sizeof(data));
    if (QEndianSwapOnSave()) {
        data.Endian();
    }
    CloseForm();
}

TESForm* TESIdleForm::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    TESForm* a2[2];
    TESForm* DuplicateForm = TESForm::CreateDuplicateForm(abCreateEditorID, apCopyMap);
    TESIdleForm* pdupidle = dynamic_cast<TESIdleForm*>(DuplicateForm);
    a2[0] = nullptr;
    if (!apCopyMap->GetAt(pParentIdle, *a2) || !a2[0]) {
        a2[0] = pParentIdle;
    }
    pdupidle->SetParentIdle(static_cast<TESIdleForm*>(a2[0]));
    a2[0] = nullptr;
    if (!apCopyMap->GetAt(pPrevIdle, *a2) || !a2[0]) {
        a2[0] = pPrevIdle;
    }
    a2[1] = a2[0];
    pdupidle->SetPrevIdle(static_cast<TESIdleForm*>(a2[0]));
    if (pChildIdles) {
        pChildIdles->Compact();
        uint32_t icount = pChildIdles->m_uiSize;
        NiFormArray* pduparray = pdupidle->CreateChildArray();
        pduparray->SetSize(icount);
        TESIdleForm* pdupchild = nullptr;
        for (uint32_t i = 0; i < icount; ++i) {
            TESIdleForm* pchildidle = static_cast<TESIdleForm*>(pChildIdles->GetAt(i));
            TESIdleForm* pdupchild_2 =
                static_cast<TESIdleForm*>(pchildidle->CreateDuplicateForm(abCreateEditorID, apCopyMap));
            TESIdleForm* pdupchild_3 = pdupchild_2;
            pduparray->SetAt(i, pdupchild_3);
            pdupchild = pdupchild_2;
        }
    }
    return pdupidle;
}

void TESIdleForm::InitItem() {
    if (GetInitialized()) {
        return;
    }
    GetConditions()->InitItem(this);
    int32_t iSection = GetIdleAnimGroupSection();
    if (iSection >= 8 && (iSection < 20 || iSection > 21)) {
        uint32_t iFormID = GetFormID();
        const char* pEditorID = GetFormEditorID();
        Warning(
            "MASTERFILE: Invalid anim group section encountered on idle '%s' (%08X). Resetting to Special Idle.",
            pEditorID, iFormID
        );
        SetIdleAnimGroupSection(7);
    }
    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pParentIdle));
    if (iID) {
        TESForm::AddCompileIndex(iID, GetFile(-1));
        TESIdleForm* pParent = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(iID));
        if (!pParent) {
            BSStringT<char> kString;
            GetFormDetailedString(kString);
            Warning("MASTERFILE: Could not find parent idle (%08X) for %s.", iID, kString.String());
        }
        SetParentIdle(pParent);
    }
    iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrevIdle));
    if (iID) {
        TESForm::AddCompileIndex(iID, GetFile(-1));
        TESIdleForm* pPrev = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(iID));
        if (!pPrev) {
            BSStringT<char> kString;
            GetFormDetailedString(kString);
            Warning("MASTERFILE: Could not find previous idle (%08X) for %s.", iID, kString.String());
        }
        SetPrevIdle(pPrev);
    }
    SetInitialized(true);
}

void TESIdleForm::Copy(TESForm* apCopy) {
    TESIdleForm* pcopy = dynamic_cast<TESIdleForm*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        (&Conditions)->Copy(&pcopy->Conditions, nullptr);
        bool bloose = GetLooseIdle();
        // Copies the 8-byte record whole.
        data = pcopy->data;
        if (!GetTemporary()) {
            SetLooseIdle(bloose);
        }
    }
}

bool TESIdleForm::Compare(TESForm* apCompare) {
    TESIdleForm* pcomp = dynamic_cast<TESIdleForm*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if ((&Conditions)->Compare(&pcomp->Conditions)) {
        return true;
    }
    if (data.cAnimData != pcomp->data.cAnimData) {
        return true;
    }
    if (data.cLoopMin != pcomp->data.cLoopMin) {
        return true;
    }
    if (data.cLoopMax != pcomp->data.cLoopMax) {
        return true;
    }
    if (data.sReplayDelay == pcomp->data.sReplayDelay) {
        return data.cFlagsEx != pcomp->data.cFlagsEx;
    }
    return true;
}

// The editor ID, or the shared empty string when it has none.
const char* TESIdleForm::GetFormEditorID() {
    if (strEditorID.String()) {
        return strEditorID.String();
    }
    return "";
}

bool TESIdleForm::SetFormEditorID(const char* apEditorID) {
    uint32_t uiSize = BSstrlen(apEditorID) + 1;
    char* pEditorID = static_cast<char*>(XFNV_ALLOCA(uiSize));
    BSstrcpy(pEditorID, uiSize, apEditorID);
    if (!strEditorID.StrCmp(pEditorID, true)) {
        return false;
    }
    strEditorID.Set(pEditorID, 0);
    return true;
}

bool TESIdleForm::CheckConditions(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef, bool abRecurseUp) {
    bool v8 = false;
    TESCondition* MBRoomAccumList = GetConditions();
    if (MBRoomAccumList->IsTrue(apActionRef, apTargetRef)) {
        if (abRecurseUp && GetParentIdle()) {
            TESIdleForm* State = GetParentIdle();
            return State->CheckConditions(apActionRef, apTargetRef, abRecurseUp);
        }

        return true;
    }
    return v8;
}

void TESIdleForm::RemoveFromManager() {
    if (GetLooseIdle()) {
        NiFormArray* pLooseArray = pIdleManager->GetLooseIdleArray(this);
        uint32_t uiIndex = pLooseArray->GetIndexForForm(this);
        if (uiIndex != -1) {
            pLooseArray->RemoveAt(uiIndex);
            pLooseArray->Compact();
        }
        SetLooseIdle(false);
    }
    else {
        NiFormArray* pArray;
        if (GetParentIdle()) {
            pArray = GetParentIdle()->pChildIdles;
        }
        else {
            pArray = &pIdleManager->GetIdleAnimRoot(this)->IdleArray;
        }
        if (pArray) {
            uint32_t uiIndex = pArray->GetIndexForForm(this);
            if (uiIndex != -1) {
                pArray->RemoveAt(uiIndex);
                pArray->Compact();
                pParentIdle = nullptr;
                // The idle that followed this one now sits at its index.
                TESIdleForm* pNextIdleForm = static_cast<TESIdleForm*>(pArray->GetAt(uiIndex));
                if (pNextIdleForm) {
                    if (pNextIdleForm->GetInitialized()) {
                        pNextIdleForm->pPrevIdle = pPrevIdle;
                    }
                    else {
                        // Before InitItem the previous-idle field holds a form ID, not a pointer.
                        TESIdleForm* pPrev = pPrevIdle;
                        pNextIdleForm->pPrevIdle =
                            reinterpret_cast<TESIdleForm*>(static_cast<uintptr_t>(pPrev ? pPrev->GetFormID() : 0));
                    }
                    pPrevIdle = nullptr;
                }
            }
        }
    }
}

void TESIdleForm::RemoveChildAtIndex(uint32_t aiIndex) {
    TESIdleForm* childByIndex;
    TESIdleForm* v4;
    if (pChildIdles) {
        childByIndex = GetChildByIndex(aiIndex + 1);
        v4 = GetChildByIndex(aiIndex - 1);
        if (childByIndex) {
            childByIndex->TESIdleForm::SetPrevIdle(v4);
        }
        pChildIdles->RemoveAt(aiIndex);
        pChildIdles->Compact();
    }
}

bool TESIdleForm::InitParents() {
    bool bResult = true;
    TESIdleForm* pParent = pParentIdle;
    TESIdleForm* pPrev = pPrevIdle;
    if (GetLooseIdle()) {
        NiFormArray* pLooseArray = pIdleManager->GetLooseIdleArray(this);
        if (!pLooseArray) {
            pIdleManager->GetRootIdleArray(this);
            pLooseArray = pIdleManager->GetLooseIdleArray(this);
        }
        if (pLooseArray) {
            pLooseArray->InsertFormAt(pLooseArray->GetSize(), this);
        }
        else {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning("Failed to find loose idle array for '%s' (%08X)", pEditorID, iFormID);
            bResult = false;
        }
        return bResult;
    }
    uint32_t uiIndex = 0;
    NiFormArray* pArray = nullptr;
    if (pParent) {
        pArray = pParent->CreateChildArray();
    }
    else {
        pArray = pIdleManager->GetRootIdleArray(this);
    }
    if (pArray) {
        if (pPrev) {
            uint32_t uiPrevIndex = pArray->GetIndexForForm(pPrev);
            if (uiPrevIndex != 0xFFFFFFFF) {
                ++uiPrevIndex;
            }
            uiIndex = uiPrevIndex;
        }
        if (uiIndex == 0xFFFFFFFF || GetDelete()) {
            uiIndex = pArray->GetSize() + 1;
        }
        pArray->InsertFormAt(uiIndex, this);
        FixNextIdles(pArray);
    }
    else {
        BSStringT<char> kString;
        GetFormDetailedString(kString);
        Warning("MASTERFILE: Could not find parent array for %s.", kString.String());
        bResult = false;
    }
    return bResult;
}

void TESIdleForm::FixNextIdles(NiFormArray* apArray) {
    if (!apArray) {
        return;
    }
    uint32_t uiEmpty = 0;
    uint32_t uiThisIndex = 0xFFFFFFFF;
    uint32_t uiSize = apArray->GetSize();
    uint32_t uiNextIndex = 0xFFFFFFFF;
    TESIdleForm* pNext = nullptr;
    for (uint32_t i = 0; i < uiSize; ++i) {
        TESIdleForm* pIdle = dynamic_cast<TESIdleForm*>(apArray->GetAt(i));
        if (!pIdle) {
            ++uiEmpty;
        }
        else if (pIdle == this) {
            uiThisIndex = i;
        }
        else if (pIdle->GetPrevIdle() == this) {
            if (!pNext) {
                pNext = pIdle;
                uiNextIndex = i;
            }
            else {
                BSStringT<char> kString;
                GetFormDetailedString(kString);
                Warning("MASTERFILE: Multiple next idles for %s.", kString.String());
            }
        }
    }
    if (uiThisIndex == 0xFFFFFFFF || uiNextIndex == 0xFFFFFFFF || uiNextIndex == uiThisIndex + 1) {
        return;
    }
    TESForm* pNull = nullptr;
    apArray->SetAt(uiNextIndex, pNull);
    apArray->Compact();
    if (uiNextIndex < uiThisIndex) {
        ++uiEmpty;
    }
    uiThisIndex -= uiEmpty;
    apArray->InsertFormAt(uiThisIndex + 1, pNext);
    apArray->Compact();
    pNext->FixNextIdles(apArray);
}

TESCondition* TESIdleForm::GetConditions() {
    return &Conditions;
}

void TESIdleForm::SetParentIdle(TESIdleForm* apIdle) {
    pParentIdle = apIdle;
}

TESIdleForm* TESIdleForm::GetParentIdle() {
    return pParentIdle;
}

TESIdleForm* TESIdleForm::GetPrevIdle() {
    return pPrevIdle;
}

void TESIdleForm::SetPrevIdle(TESIdleForm* apIdle) {
    pPrevIdle = apIdle;
}

NiFormArray* TESIdleForm::CreateChildArray() {
    if (pChildIdles) {
        return pChildIdles;
    }
    void* pMemory = ourMalloc(sizeof(NiFormArray));
    NiFormArray* pArray = pMemory ? ::new (pMemory) NiFormArray : nullptr;
    pChildIdles = pArray;
    return pArray;
}

ANIM_GROUP_SECTION TESIdleForm::GetIdleAnimGroupSection() {
    uint8_t cSection = data.cAnimData;
    cSection &= 0x3F;
    return static_cast<ANIM_GROUP_SECTION>(cSection);
}

void TESIdleForm::SetIdleAnimGroupSection(uint8_t aeSection) {
    data.cAnimData &= 192;
    data.cAnimData |= aeSection;
}

uint32_t TESIdleForm::GetChildCount() {
    uint32_t uiCount = 0;
    if (pChildIdles) {
        uiCount = pChildIdles->GetSize();
    }
    return uiCount;
}

TESIdleForm* TESIdleForm::GetChildByIndex(uint32_t aiIndex) {
    TESForm* formAt;
    TESIdleForm* v4 = nullptr;
    if (pChildIdles) {
        formAt = pChildIdles->GetFormAt(aiIndex);
        return dynamic_cast<TESIdleForm*>(formAt);
    }
    return v4;
}

uint32_t TESIdleForm::GetIndexForChild(TESIdleForm* apChild) {
    uint32_t v3 = (uint32_t)(-1);
    if (pChildIdles) {
        return pChildIdles->GetIndexForForm(apChild);
    }
    return v3;
}

bool TESIdleForm::GetLooseIdle() {
    return (data.cAnimData & 0x40) != 0;
}

bool TESIdleForm::CheckIdleFile() {
    bool bResult = false;
    TESModel* pModel = this;
    const char* pExtension = std::strrchr(pModel->GetModel(), '.');
    if (pExtension && !BSstricmp(pExtension, ".kf")) {
        bResult = true;
    }
    return bResult;
}

uint16_t TESIdleForm::GetReplayDelay() {
    return data.sReplayDelay;
}

// The number of loops to play: 0 without a loop range, 255 (endless) as is, else a random count in the range less 1.
uint8_t TESIdleForm::GetLoops() {
    if (!data.cLoopMin || !data.cLoopMax) {
        return 0;
    }
    if (data.cLoopMin == 255) {
        return data.cLoopMin;
    }
    if (data.cLoopMax > data.cLoopMin) {
        return RandomInt(data.cLoopMin, data.cLoopMax) - 1;
    }
    return data.cLoopMax - 1;
}

bool TESIdleForm::GetAllowNoFile() {
    return ((data.cAnimData & 0x80) != 0);
}

void TESIdleForm::SetLooseIdle(bool abVal) {
    if (abVal) {
        data.cAnimData |= 64;
    }
    else {
        data.cAnimData &= (~64);
    }
}

bool TESIdleForm::GetNoAttacking() {
    return ((data.cFlagsEx & 1) != 0);
}

void TESIdleForm::RemoveChild(TESIdleForm* apChild) {
    uint32_t indexForChild;
    if ((apChild) && (pChildIdles)) {
        indexForChild = GetIndexForChild(apChild);
        RemoveChildAtIndex(indexForChild);
    }
}

TESIdleForm* TESIdleForm::GetIdleToPlay(
    TESObjectREFR* apActionRef,
    TESObjectREFR* apTargetRef,
    BSSimpleList<IDLE_REPLAY_DELAY*>* apReplayDelayList
) {
    TESIdleForm* pretidle = nullptr;
    if (!GetDelete() && (&Conditions)->IsTrue(apActionRef, apTargetRef)) {
        if (CheckIdleFile()) {
            BSSimpleList<IDLE_REPLAY_DELAY*>* pdelayList = apReplayDelayList;
            bool bdelay = false;
            while (pdelayList && (!bdelay && !pdelayList->IsEmpty())) {
                bdelay = pdelayList->m_item->pIdleForm == this;
                pdelayList = pdelayList->m_pkNext;
            }
            if (!bdelay) {
                pretidle = this;
            }
        }
        else {
            if (GetAllowNoFile()) {
                pretidle = this;
            }
        }
        uint32_t icount = GetChildCount();
        if (icount) {
            TESIdleForm* pgoodchild = nullptr;
            for (uint32_t i = 0; i < icount; ++i) {
                TESIdleForm* pchildidle = GetChildByIndex(i);
                if (pchildidle) {
                    pgoodchild = pchildidle->GetIdleToPlay(apActionRef, apTargetRef, apReplayDelayList);
                }
                if (pgoodchild) {
                    break;
                }
            }
            if (pgoodchild) {
                return pgoodchild;
            }
        }
    }
    return pretidle;
}

TESModelAnim::TESModelAnim() : TESModel() {}

TESModelAnim::~TESModelAnim() {}
