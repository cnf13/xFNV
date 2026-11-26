#include "Fallout Shared/TESIdleForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
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
    BSEndian(sReplayDelay, false);
}

void IDLE_PARENT_IDS::Endian() {
    BSEndian(iparentID, false);
    BSEndian(iprevID, false);
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
            NiFormArray* pArray = &pIdleManager->GetIdleAnimRoot(this)->IdleArray;
            if (pArray) {
                uint32_t uiIndex = pArray->GetIndexForForm(this);
                if (uiIndex != -1) {
                    TESIdleForm* pNextIdle = static_cast<TESIdleForm*>(pArray->GetFormAt(uiIndex + 1));
                    if (pNextIdle) {
                        pNextIdle->pPrevIdle = pPrevIdle;
                    }
                    pArray->RemoveAt(uiIndex);
                    pArray->Compact();
                }
            }
            pArray = pIdleManager->GetLooseIdleArray(this);
            if (pArray) {
                uint32_t uiIndex = pArray->GetIndexForForm(this);
                if (uiIndex != -1) {
                    pArray->RemoveAt(uiIndex);
                    pArray->Compact();
                }
            }
        }
    }
    if (pChildIdles) {
        uint32_t uiCount = GetChildCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            TESIdleForm* pChildIdle = GetChildByIndex(i);
            pChildIdle->pParentIdle = nullptr;
            delete pChildIdle;
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
    if (pDataHandler->GetSaveLoad() &&
        !pDataHandler->GetClearingData() &&
        pIdleManager->GetInitialized() &&
        TESForm::GetInitialized()) {
        RemoveFromManager();
    }

    TESIdleForm::GetConditions()->ClearAllConditionItems();
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
                LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(data));
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
    TESForm* pDupForm = TESForm::CreateDuplicateForm(abCreateEditorID, apCopyMap);
    TESIdleForm* pDupIdle = dynamic_cast<TESIdleForm*>(pDupForm);
    TESForm* pCopy = nullptr;
    if (!apCopyMap->GetAt(pParentIdle, pCopy) || !pCopy) {
        pCopy = pParentIdle;
    }
    pDupIdle->SetParentIdle(static_cast<TESIdleForm*>(pCopy));
    pCopy = nullptr;
    if (!apCopyMap->GetAt(pPrevIdle, pCopy) || !pCopy) {
        pCopy = pPrevIdle;
    }
    pDupIdle->SetPrevIdle(static_cast<TESIdleForm*>(pCopy));
    if (pChildIdles) {
        pChildIdles->Compact();
        uint32_t uiCount = pChildIdles->m_uiSize;
        NiFormArray* pDupArray = pDupIdle->CreateChildArray();
        pDupArray->SetSize(uiCount);
        for (uint32_t i = 0; i < uiCount; ++i) {
            TESIdleForm* pChildIdle = static_cast<TESIdleForm*>(pChildIdles->GetAt(i));
            TESIdleForm* pDupChild = static_cast<TESIdleForm*>(
                pChildIdle->CreateDuplicateForm(abCreateEditorID, apCopyMap)
            );
            pDupArray->SetAt(i, pDupChild);
        }
    }
    return pDupIdle;
}

void TESIdleForm::InitItem() {
    if (GetInitialized()) {
        return;
    }
    GetConditions()->InitItem(this);
    int32_t iSection = GetIdleAnimGroupSection();
    if (iSection >= 8 && (iSection < 20 || iSection > 21)) {
        uint32_t uiFormID = GetFormID();
        const char* pEditorID = GetFormEditorID();
        Warning(
            "MASTERFILE: Invalid anim group section encountered on idle '%s' (%08X). Resetting to Special Idle.",
            pEditorID,
            uiFormID
        );
        SetIdleAnimGroupSection(7);
    }
    uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pParentIdle));
    if (uiID) {
        TESForm::AddCompileIndex(uiID, GetFile(-1));
        TESIdleForm* pParent = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(uiID));
        if (!pParent) {
            BSStringT<char> kString;
            GetFormDetailedString(kString);
            Warning("MASTERFILE: Could not find parent idle (%08X) for %s.", uiID, kString.String());
        }
        SetParentIdle(pParent);
    }
    uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrevIdle));
    if (uiID) {
        TESForm::AddCompileIndex(uiID, GetFile(-1));
        TESIdleForm* pPrev = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(uiID));
        if (!pPrev) {
            BSStringT<char> kString;
            GetFormDetailedString(kString);
            Warning("MASTERFILE: Could not find previous idle (%08X) for %s.", uiID, kString.String());
        }
        SetPrevIdle(pPrev);
    }
    SetInitialized(true);
}

void TESIdleForm::Copy(TESForm* apCopy) {
    TESIdleForm* pCopy = dynamic_cast<TESIdleForm*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        Conditions.Copy(&pCopy->Conditions, nullptr);
        bool bLoose = GetLooseIdle();
        // Copies the 8-byte record whole.
        data = pCopy->data;
        if (!GetTemporary()) {
            SetLooseIdle(bLoose);
        }
    }
}

bool TESIdleForm::Compare(TESForm* apCompare) {
    TESIdleForm* pComp = dynamic_cast<TESIdleForm*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (Conditions.Compare(&pComp->Conditions)) {
        return true;
    }
    if (data.cAnimData != pComp->data.cAnimData) {
        return true;
    }
    if (data.cLoopMin != pComp->data.cLoopMin) {
        return true;
    }
    if (data.cLoopMax != pComp->data.cLoopMax) {
        return true;
    }
    if (data.sReplayDelay == pComp->data.sReplayDelay) {
        return data.cFlagsEx != pComp->data.cFlagsEx;
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
    if (GetConditions()->IsTrue(apActionRef, apTargetRef)) {
        if (abRecurseUp && GetParentIdle()) {
            return GetParentIdle()->CheckConditions(apActionRef, apTargetRef, abRecurseUp);
        }

        return true;
    }
    return false;
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
                        pNextIdleForm->pPrevIdle = reinterpret_cast<TESIdleForm*>(
                            static_cast<uintptr_t>(pPrev ? pPrev->GetFormID() : 0)
                        );
                    }
                    pPrevIdle = nullptr;
                }
            }
        }
    }
}

void TESIdleForm::RemoveChildAtIndex(uint32_t aiIndex) {
    if (pChildIdles) {
        TESIdleForm* pNextIdle = GetChildByIndex(aiIndex + 1);
        TESIdleForm* pPrev = GetChildByIndex(aiIndex - 1);
        if (pNextIdle) {
            pNextIdle->TESIdleForm::SetPrevIdle(pPrev);
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
            uint32_t uiFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning("Failed to find loose idle array for '%s' (%08X)", pEditorID, uiFormID);
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
    NiFormArray* pArray = ::new NiFormArray;
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
    if (pChildIdles) {
        TESForm* pForm = pChildIdles->GetFormAt(aiIndex);
        return dynamic_cast<TESIdleForm*>(pForm);
    }
    return nullptr;
}

uint32_t TESIdleForm::GetIndexForChild(TESIdleForm* apChild) {
    if (pChildIdles) {
        return pChildIdles->GetIndexForForm(apChild);
    }
    return static_cast<uint32_t>(-1);
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
    return (data.cAnimData & 0x80) != 0;
}

void TESIdleForm::SetLooseIdle(bool abVal) {
    if (abVal) {
        data.cAnimData |= 64;
    }
    else {
        data.cAnimData &= ~64;
    }
}

bool TESIdleForm::GetNoAttacking() {
    return (data.cFlagsEx & IDLE_DATA::FLAG_NO_ATTACKING) != 0;
}

void TESIdleForm::RemoveChild(TESIdleForm* apChild) {
    if (apChild && pChildIdles) {
        RemoveChildAtIndex(GetIndexForChild(apChild));
    }
}

TESIdleForm* TESIdleForm::GetIdleToPlay(
    TESObjectREFR* apActionRef,
    TESObjectREFR* apTargetRef,
    BSSimpleList<IDLE_REPLAY_DELAY*>* apReplayDelayList
) {
    TESIdleForm* pRetIdle = nullptr;
    if (!GetDelete() && Conditions.IsTrue(apActionRef, apTargetRef)) {
        if (CheckIdleFile()) {
            BSSimpleList<IDLE_REPLAY_DELAY*>* pDelayList = apReplayDelayList;
            bool bDelay = false;
            while (pDelayList && !bDelay && !pDelayList->IsEmpty()) {
                bDelay = pDelayList->m_item->pIdleForm == this;
                pDelayList = pDelayList->m_pkNext;
            }
            if (!bDelay) {
                pRetIdle = this;
            }
        }
        else if (GetAllowNoFile()) {
            pRetIdle = this;
        }
        uint32_t uiCount = GetChildCount();
        if (uiCount) {
            TESIdleForm* pGoodChild = nullptr;
            for (uint32_t i = 0; i < uiCount; ++i) {
                TESIdleForm* pChildIdle = GetChildByIndex(i);
                if (pChildIdle) {
                    pGoodChild = pChildIdle->GetIdleToPlay(apActionRef, apTargetRef, apReplayDelayList);
                }
                if (pGoodChild) {
                    break;
                }
            }
            if (pGoodChild) {
                return pGoodChild;
            }
        }
    }
    return pRetIdle;
}

TESModelAnim::TESModelAnim() : TESModel() {}

TESModelAnim::~TESModelAnim() {}
