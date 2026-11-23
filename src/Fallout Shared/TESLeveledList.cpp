#include "Fallout Shared/TESLeveledList.h"

#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/LEVELED_OBJECT_FILE.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESLevCharacter.h"
#include "Fallout Shared/TESLevCreature.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESLevSpell.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

// The next unsigned random number of the BSRandom singleton.
uint32_t Random() {
    return BSRandom::GetSingleton()->UnsignedInt();
}

LEVELED_OBJECT::LEVELED_OBJECT() : pForm(nullptr), sCount(1), sLevel(1), pItemExtra(nullptr) {}

LEVELED_OBJECT::~LEVELED_OBJECT() {
    ourFree(pItemExtra);
}

void LEVELED_OBJECT::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESLeveledList::TESLeveledList() : BaseFormComponent(), leveledList(), ScriptAddedList() {
    TESLeveledList::InitializeDataComponent();
}

TESLeveledList::~TESLeveledList() {
    TESLeveledList::ClearDataComponent();
}

void TESLeveledList::InitializeDataComponent() {
    cChanceNone = 0;
    cLLFlags = 0;
    pChanceGlobal = nullptr;
}

void TESLeveledList::ClearDataComponent() {
    FreeLeveledList();
}

void TESLeveledList::CopyComponent(BaseFormComponent* apCopyFC) {
    TESLeveledList* pCopy = dynamic_cast<TESLeveledList*>(apCopyFC);
    if (pCopy) {
        FreeLeveledList();
        BSSimpleList<LEVELED_OBJECT*>* pCopyList = &pCopy->leveledList;
        BSSimpleList<LEVELED_OBJECT*>* pList = &leveledList;
        while (pCopyList) {
            LEVELED_OBJECT* pdata;
            LEVELED_OBJECT* pcopydata = pCopyList->m_item;
            if (!pcopydata) {
                break;
            }
            pdata = new LEVELED_OBJECT;
            pdata->pForm = pcopydata->pForm;
            pdata->sLevel = pcopydata->sLevel;
            pdata->sCount = pcopydata->sCount;
            if (pcopydata->pItemExtra && !pcopydata->pItemExtra->IsDefault()) {
                ContainerItemExtra* pExtra = new ContainerItemExtra;
                pdata->pItemExtra = pExtra;
                pdata->pItemExtra->Copy(pcopydata->pItemExtra);
            }

            if (leveledList.IsEmpty()) {
                pList->AddHead(pdata);
            }
            else {
                pList->AddTail(pdata);
                pList = pList->m_pkNext;
            }
            pCopyList = pCopyList->m_pkNext;
        }
        pChanceGlobal = pCopy->pChanceGlobal;
        cChanceNone = pCopy->GetChanceNone();
        bool bCalcAllBelow = pCopy->GetCalcAllBelow();
        SetCalcAllBelow(bCalcAllBelow);
        bool bMultCalc = pCopy->GetMultCalc();
        SetMultCalc(bMultCalc);
        bool bUseAll = pCopy->GetUseAll();
        SetUseAll(bUseAll);
    }
}

bool TESLeveledList::CompareComponent(BaseFormComponent* apCompareFC) {
    TESLeveledList* pCompare = dynamic_cast<TESLeveledList*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    int32_t iCalcAllBelow = GetCalcAllBelow();
    if (iCalcAllBelow != pCompare->GetCalcAllBelow()) {
        return true;
    }
    int32_t iMultCalc = GetMultCalc();
    if (iMultCalc != pCompare->GetMultCalc()) {
        return true;
    }
    int32_t iChanceNone = GetChanceNone();
    if (iChanceNone != pCompare->GetChanceNone()) {
        return true;
    }
    int32_t iUseAll = GetUseAll();
    if (iUseAll != pCompare->GetUseAll()) {
        return true;
    }
    if (pChanceGlobal != pCompare->pChanceGlobal) {
        return true;
    }
    BSSimpleList<LEVELED_OBJECT*>* pList = &leveledList;
    BSSimpleList<LEVELED_OBJECT*>* pCompareList = &pCompare->leveledList;
    int32_t iListEmpty = pList->IsEmpty();
    if (iListEmpty != pCompareList->IsEmpty()) {
        return true;
    }
    if (pList->IsEmpty()) {
        if (!pCompareList->IsEmpty()) {
            return true;
        }
    }
    else {
        while (pList && pCompareList) {
            LEVELED_OBJECT* pLevO = pList->m_item;
            LEVELED_OBJECT* pCompareLevO = pCompareList->m_item;
            if (pLevO && pCompareLevO) {
                if (pLevO->pForm != pCompareLevO->pForm) {
                    return true;
                }
                if (pLevO->sLevel != pCompareLevO->sLevel) {
                    return true;
                }
                if (pLevO->sCount != pCompareLevO->sCount) {
                    return true;
                }
                if (pLevO->pItemExtra && (!pCompareLevO->pItemExtra && !pLevO->pItemExtra->IsDefault())) {
                    return true;
                }
                if (!pLevO->pItemExtra && (pCompareLevO->pItemExtra && !pCompareLevO->pItemExtra->IsDefault())) {
                    return true;
                }
                if (pLevO->pItemExtra &&
                    (pCompareLevO->pItemExtra && pLevO->pItemExtra->Compare(pCompareLevO->pItemExtra))) {
                    return true;
                }
            }
            pList = pList->m_pkNext;
            pCompareList = pCompareList->m_pkNext;
        }
        if (pList || pCompareList) {
            return true;
        }
    }
    return false;
}

uint8_t TESLeveledList::GetChanceNone() {
    if (pChanceGlobal) {
        return static_cast<uint8_t>(static_cast<int32_t>(pChanceGlobal->GetValue()));
    }
    return cChanceNone;
}

bool TESLeveledList::GetMultCalc() {
    return (cLLFlags & FLAG_MULT_CALC) != 0;
}

void TESLeveledList::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags kChangeFlags;
    kChangeFlags = apLoadGameBuffer->GetOldChangeFlags();
    if (kChangeFlags.CheckFlags(CHANGE_LEVELED_LIST_ADDED_OBJECT)) {
        RemoveAllScriptAddedLeveledObjects();
    }
}

// The LVLO chunk is the 12-byte LEVELED_OBJECT_FILE local, of which only sLevel, iFormID and sCount are written;
// __AddChunkData copies its uninitialised padding bytes into the record.
void TESLeveledList::Save() {
    LEVELED_OBJECT_FILE kData;
    TESForm::AddChunk(LVLD_ID, cChanceNone);
    TESForm::AddChunk(LVLF_ID, cLLFlags);
    if (pChanceGlobal) {
        uint32_t uiGlobalID = pChanceGlobal->iFormID;
        TESForm::AddChunk(LVLG_ID, uiGlobalID);
    }
    for (BSSimpleList<LEVELED_OBJECT*>* pIter = GetLeveledList(); pIter; pIter = pIter->GetNext()) {
        LEVELED_OBJECT* pLeveledObject = pIter->GetItem();
        if (!pLeveledObject) {
            break;
        }
        kData.iFormID = pLeveledObject->pForm->iFormID;
        kData.sLevel = pLeveledObject->sLevel;
        kData.sCount = pLeveledObject->sCount;
        if (TESForm::QEndianSwapOnSave()) {
            kData.Endian();
        }
        TESForm::__AddChunkData(LVLO_ID, &kData, sizeof(LEVELED_OBJECT_FILE));
        if (TESForm::QEndianSwapOnSave()) {
            kData.Endian();
        }
        if (pLeveledObject->pItemExtra) {
            pLeveledObject->pItemExtra->Save();
        }
    }
}

void TESLeveledList::SetChanceNone(uint8_t acChance) {
    cChanceNone = acChance;
}

bool TESLeveledList::GetCalcAllBelow() {
    return (cLLFlags & FLAG_CALC_ALL_BELOW) != 0;
}

bool TESLeveledList::GetUseAll() {
    return (cLLFlags & FLAG_USE_ALL) != 0;
}

BSSimpleList<LEVELED_OBJECT*>* TESLeveledList::GetLeveledList() {
    return &leveledList;
}

LEVELED_OBJECT*
TESLeveledList::AddLeveledObject(uint16_t asLevel, uint16_t asCount, TESForm* apForm, ContainerItemExtra* apExtra) {
    LEVELED_OBJECT* pObject = nullptr;
    BSSimpleList<LEVELED_OBJECT*>* pIter = GetLeveledList();
    BSSimpleList<LEVELED_OBJECT*>* pPrev = nullptr;
    while (pIter) {
        LEVELED_OBJECT* pCurrent = pIter->GetItem();
        if (pCurrent) {
            if (pCurrent->sLevel > asLevel) {
                break;
            }
            pPrev = pIter;
        }
        pIter = pIter->GetNext();
    }

    pObject = new LEVELED_OBJECT;
    pObject->pForm = apForm;
    pObject->sLevel = asLevel;
    pObject->sCount = asCount;
    pObject->pItemExtra = apExtra;
    if (pIter) {
        pIter->AddHead(pObject);
    }
    else if (pPrev) {
        pPrev->AddTail(pObject);
    }
    else {
        GetLeveledList()->AddTail(pObject);
    }
    return pObject;
}

void TESLeveledList::CalculateCurrentForm(
    uint16_t asLevel,
    TESForm*& apOutForm,
    uint16_t& asOutCount,
    ContainerItemExtra*& apOutExtra,
    bool abRecurse,
    int32_t aiAllBelowForce
) {
    apOutForm = nullptr;
    asOutCount = 0;
    apOutExtra = nullptr;
    uint8_t cChanceNone = GetChanceNone();
    if (cChanceNone > 0) {
        int32_t iRoll = Random() % 100;
        if (iRoll < cChanceNone) {
            return;
        }
    }

    int32_t iMaxLevel = asLevel;
    if ((GetCalcAllBelow() && aiAllBelowForce == 0) || aiAllBelowForce > 0) {
        uint32_t uiLevelDiff = GetMaxLevelDifference();
        if (uiLevelDiff) {
            iMaxLevel = asLevel - uiLevelDiff;
        }
        else {
            iMaxLevel = -1;
        }
    }

    uint32_t uiCount = 1;
    int32_t iBestLevel = 0;
    BSSimpleList<LEVELED_OBJECT*>* pBest = nullptr;
    BSSimpleList<LEVELED_OBJECT*>* pIter = GetLeveledList();
    bool bMaxLevelAchieved = false;
    while (pIter) {
        LEVELED_OBJECT* pLeveledObject = pIter->GetItem();
        if (pLeveledObject) {
            int32_t iItemLevel = pLeveledObject->sLevel;
            if (iItemLevel > asLevel) {
                break;
            }
            if (iItemLevel > iBestLevel && (!iBestLevel || !bMaxLevelAchieved)) {
                if (iItemLevel >= iMaxLevel) {
                    bMaxLevelAchieved = true;
                }
                uiCount = 1;
                pBest = pIter;
                iBestLevel = iItemLevel;
            }
            else {
                ++uiCount;
            }
        }
        pIter = pIter->GetNext();
    }

    if (!pBest) {
        return;
    }
    uint32_t uiSkip = Random() % uiCount;
    pIter = pBest;
    while (uiSkip) {
        pIter = pIter->GetNext();
        --uiSkip;
        if (!pIter) {
            return;
        }
    }
    LEVELED_OBJECT* pLeveledObject = pIter->GetItem();
    TESForm* pForm = pLeveledObject->pForm;
    uint16_t usCount = pLeveledObject->sCount;
    ContainerItemExtra* pItemExtra = pLeveledObject->pItemExtra;
    TESLeveledList* pLeveledList = GetFormAsLeveledList(pLeveledObject->pForm);
    if (pLeveledList && abRecurse) {
        // The callee writes the three results before reading them.
        TESForm* pOutForm;
        uint16_t usOutCount;
        ContainerItemExtra* pOutExtra;
        pOutForm = nullptr;
        usOutCount = 0;
        pOutExtra = nullptr;
        pLeveledList->CalculateCurrentForm(asLevel, pOutForm, usOutCount, pOutExtra, abRecurse, aiAllBelowForce);
        pForm = pOutForm;
        usCount = static_cast<uint16_t>(usCount * usOutCount);
        if (pOutExtra) {
            pItemExtra = pOutExtra;
        }
    }
    apOutForm = pForm;
    asOutCount = usCount;
    apOutExtra = pItemExtra;
}

void TESLeveledList::CalculateCurrentFormList(
    uint16_t asLevel,
    uint16_t asCount,
    TESContainer* apOutCont,
    int32_t aiAllBelowForce
) {
    if (asLevel < 1) {
        asLevel = 1;
    }
    if (!apOutCont || asCount == 0) {
        return;
    }

    uint32_t uiLevel = asLevel;
    if ((GetCalcAllBelow() && aiAllBelowForce == 0) || aiAllBelowForce > 0) {
        uint16_t usHighestLevel = 0;
        for (BSSimpleList<LEVELED_OBJECT*>* pIter = GetLeveledList(); pIter && !pIter->IsEmpty();
             pIter = pIter->GetNext()) {
            uint16_t usItemLevel = pIter->GetItem()->sLevel;
            if (usItemLevel > usHighestLevel) {
                usHighestLevel = usItemLevel;
            }
        }
        if (usHighestLevel < uiLevel) {
            uiLevel = usHighestLevel;
        }
    }

    alignas(TESContainer) uint8_t kTempContainerMemory[sizeof(TESContainer)];
    // Uses the stack container's address, not the constructor's result.
    TESContainer* pTempContainer = reinterpret_cast<TESContainer*>(kTempContainerMemory);
    ::new (kTempContainerMemory) TESContainer;
    if (GetMultCalc()) {
        for (int32_t i = 0; i < asCount; ++i) {
            TESForm* pOutForm = nullptr;
            uint16_t usOutCount = 0;
            ContainerItemExtra* pOutExtra = nullptr;
            CalculateCurrentForm(
                static_cast<uint16_t>(uiLevel),
                pOutForm,
                usOutCount,
                pOutExtra,
                false,
                aiAllBelowForce
            );
            if (pOutForm && usOutCount) {
                TESLeveledList* pLeveledList = GetFormAsLeveledList(pOutForm);
                if (pLeveledList) {
                    pLeveledList->CalculateCurrentFormList(asLevel, usOutCount, pTempContainer, aiAllBelowForce);
                }
                else if (pOutForm->IsBoundObject()) {
                    TESBoundObject* pObject = static_cast<TESBoundObject*>(pOutForm);
                    if (pObject) {
                        pTempContainer->AddObject(pObject, usOutCount, pOutExtra);
                    }
                }
            }
        }
    }
    else {
        TESForm* pForm = nullptr;
        uint16_t usItemCount = 0;
        ContainerItemExtra* pItemExtra = nullptr;
        if (GetUseAll()) {
            uint8_t cChanceNone = GetChanceNone();
            int32_t iRoll = Random() % 100;
            if (iRoll >= cChanceNone) {
                BSSimpleList<LEVELED_OBJECT*>* pIter = GetLeveledList();
                while (pIter && !pIter->IsEmpty()) {
                    LEVELED_OBJECT* pLeveledObject = pIter->GetItem();
                    pIter = pIter->GetNext();
                    pForm = pLeveledObject->pForm;
                    usItemCount = pLeveledObject->sCount;
                    pItemExtra = pLeveledObject->pItemExtra;
                    if (pForm && usItemCount) {
                        TESLeveledList* pLeveledList = GetFormAsLeveledList(pForm);
                        if (pLeveledList) {
                            pLeveledList
                                ->CalculateCurrentFormList(asLevel, usItemCount, pTempContainer, aiAllBelowForce);
                        }
                        else if (pForm->IsBoundObject()) {
                            TESBoundObject* pObject = static_cast<TESBoundObject*>(pForm);
                            if (pObject) {
                                pTempContainer->AddObject(pObject, usItemCount, pItemExtra);
                            }
                        }
                    }
                }
            }
        }
        else {
            CalculateCurrentForm(
                static_cast<uint16_t>(uiLevel),
                pForm,
                usItemCount,
                pItemExtra,
                false,
                aiAllBelowForce
            );
            if (pForm && usItemCount) {
                TESLeveledList* pLeveledList = GetFormAsLeveledList(pForm);
                if (pLeveledList) {
                    pLeveledList->CalculateCurrentFormList(asLevel, usItemCount, pTempContainer, aiAllBelowForce);
                }
                else if (pForm->IsBoundObject()) {
                    TESBoundObject* pObject = static_cast<TESBoundObject*>(pForm);
                    if (pObject) {
                        pTempContainer->AddObject(pObject, usItemCount, pItemExtra);
                    }
                }
            }
        }
        pTempContainer->MultiplyContainerCounts(asCount);
    }
    apOutCont->AddContainerContents(pTempContainer);
    pTempContainer->~TESContainer();
}

void TESLeveledList::CalculateCurrentFormListForRef(TESObjectREFR* apRef, TESContainer* apOutCont) {
    if (!apRef || !apOutCont) {
        return;
    }
    float fLevelMult = 1.0f;
    int32_t eOutAllBelowForce = 0;
    apRef->GetExtra()->GetLevCreaMultiplier(fLevelMult, eOutAllBelowForce);
    uint32_t uiLevel = 1;
    uiLevel = apRef->GetCalcLevel(true);
    // The level is converted exactly and multiplied in double before the one rounding to float; the double product is
    // exact for every level below 2^29.
    float fLevel = static_cast<float>(static_cast<double>(uiLevel) * fLevelMult);
    int32_t iLevel = static_cast<int32_t>(std::floor(fLevel));
    if (iLevel < 1) {
        iLevel = 1;
    }
    CalculateCurrentFormList(static_cast<uint16_t>(iLevel), 1, apOutCont, eOutAllBelowForce);
}

void TESLeveledList::InitItem(TESForm* apOwner) {
    TESFile* pownerFile;
    BSSimpleList<LEVELED_OBJECT*>* pList = &leveledList;
    LEVELED_OBJECT* pdata = nullptr;
    if (apOwner) {
        pownerFile = apOwner->GetFile(-1);
    }
    else {
        pownerFile = nullptr;
    }
    while (pList) {
        pdata = pList->m_item;
        if (!pdata) {
            break;
        }
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pdata->pForm));
        TESForm::AddCompileIndex(tempint, pownerFile);
        pdata->pForm = TESForm::GetFormByNumericID(tempint);
        if (pdata->pForm) {
            pList = pList->m_pkNext;
        }
        else {
            pList->RemoveHead();
            if (pdata) {
                delete pdata;
            }
            pdata = nullptr;
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pcEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Leveled Object Form (%08X) for owner object \"%s\".",
                        tempint,
                        pcEditorID
                    );
                }
                else {
                    int32_t iOwnerFormID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Leveled Object Form (%08X) for owner object (%08X).",
                        tempint,
                        iOwnerFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find Leveled Object Form (%08X) for unknown owner.", tempint);
            }
        }
        if (pdata && pdata->pItemExtra) {
            pdata->pItemExtra->InitItem(apOwner);
        }
    }
    if (pChanceGlobal) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pChanceGlobal));
        TESForm::AddCompileIndex(tempint, pownerFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        pChanceGlobal = dynamic_cast<TESGlobal*>(pForm);
        if (!pChanceGlobal) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pcEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Leveled Chance Global (%08X) for owner object \"%s\".",
                        tempint,
                        pcEditorID
                    );
                }
                else {
                    int32_t iOwnerFormID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Leveled Chance Global (%08X) for owner object (%08X).",
                        tempint,
                        iOwnerFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find Leveled Chance Global (%08X) for unknown owner.", tempint);
            }
        }
    }
}

void TESLeveledList::FreeLeveledList() {
    LEVELED_OBJECT* pdata;
    BSSimpleList<LEVELED_OBJECT*>* pList = TESLeveledList::GetLeveledList();
    while (pList) {
        pdata = pList->m_item;
        if (pdata) {
            delete pdata;
            pList->RemoveHead();
        }
        else {
            pList = pList->m_pkNext;
        }
    }
    ScriptAddedList.RemoveAll();
}

void TESLeveledList::SaveGame(BGSSaveFormBuffer* apSaveFormBuffer) {
    if (apSaveFormBuffer->GetChangeFlags().CheckFlags(CHANGE_LEVELED_LIST_ADDED_OBJECT)) {
        uint32_t uiCount = 0;
        uint32_t uiPosition = apSaveFormBuffer->StartVariableSizedValue();
        for (BSSimpleList<LEVELED_OBJECT*>* pIter = &ScriptAddedList; pIter; pIter = pIter->GetNext()) {
            LEVELED_OBJECT* pLeveledObject = pIter->GetItem();
            if (pLeveledObject) {
                apSaveFormBuffer->SaveFormID(pLeveledObject->pForm, 0);
                apSaveFormBuffer->SaveDataEndian(&pLeveledObject->sLevel, sizeof(pLeveledObject->sLevel), 0);
                apSaveFormBuffer->SaveDataEndian(&pLeveledObject->sCount, sizeof(pLeveledObject->sCount), 0);
                float fHealthPercent = -1.0f;
                if (pLeveledObject->pItemExtra) {
                    fHealthPercent = pLeveledObject->pItemExtra->fHealthMult;
                }
                apSaveFormBuffer->SaveDataEndian(&fHealthPercent, sizeof(fHealthPercent), 0);
                ++uiCount;
            }
        }
        apSaveFormBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    }
}

void TESLeveledList::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_LEVELED_LIST_ADDED_OBJECT)) {
        if (!ScriptAddedList.IsEmpty()) {
            RemoveAllScriptAddedLeveledObjects();
        }
        uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            TESForm* pForm = nullptr;
            uint16_t usCount = 0;
            uint16_t usLevel = 0;
            float fHealthPercent = 0.0f;
            uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
            pForm = TESForm::GetFormByNumericID(uiFormID);
            apLoadGameBuffer->LoadData(&usLevel, sizeof(usLevel));
            apLoadGameBuffer->LoadData(&usCount, sizeof(usCount));
            apLoadGameBuffer->LoadData(&fHealthPercent, sizeof(fHealthPercent));
            if (pForm) {
                ContainerItemExtra* pExtra = nullptr;
                if (fHealthPercent != -1.0) {
                    ContainerItemExtra* pNew = new ContainerItemExtra;
                    pExtra = pNew;
                    pExtra->AdjustHealthPercent(fHealthPercent);
                }
                TESForm* pOwner = apLoadGameBuffer->GetForm();
                AddScriptAddedLeveledObject(pOwner, usLevel, usCount, pForm, pExtra);
            }
        }
    }
}

void TESLeveledList::RemoveAllScriptAddedLeveledObjects() {
    while (!ScriptAddedList.IsEmpty()) {
        LEVELED_OBJECT* plev = ScriptAddedList.m_item;
        ScriptAddedList.RemoveHead();
        leveledList.Remove(plev);
        if (plev) {
            delete plev;
        }
    }
}

void TESLeveledList::AddScriptAddedLeveledObject(
    TESForm* apOwner,
    uint16_t asLevel,
    uint16_t asCount,
    TESForm* apForm,
    ContainerItemExtra* apExtra
) {
    LEVELED_OBJECT* pdata = AddLeveledObject(asLevel, asCount, apForm, apExtra);
    if (pdata) {
        ScriptAddedList.AddHead(pdata);
        apOwner->AddChange(CHANGE_LEVELED_LIST_ADDED_OBJECT);
    }
}

void TESLeveledList::SetCalcAllBelow(bool abAllBelow) {
    if (abAllBelow) {
        cLLFlags |= FLAG_CALC_ALL_BELOW;
    }
    else {
        cLLFlags &= ~FLAG_CALC_ALL_BELOW;
    }
}

void TESLeveledList::SetMultCalc(bool abVal) {
    if (abVal) {
        cLLFlags |= FLAG_MULT_CALC;
    }
    else {
        cLLFlags &= ~FLAG_MULT_CALC;
    }
}

void TESLeveledList::SetUseAll(bool abVal) {
    if (abVal) {
        cLLFlags |= FLAG_USE_ALL;
    }
    else {
        cLLFlags &= ~FLAG_USE_ALL;
    }
}

TESLeveledList* TESLeveledList::GetFormAsLeveledList(TESForm* apForm) {
    TESLeveledList* pList = nullptr;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case LVLI_ID: pList = static_cast<TESLevItem*>(apForm); break;
            case LVLC_ID: pList = static_cast<TESLevCreature*>(apForm); break;
            case LVLN_ID: pList = static_cast<TESLevCharacter*>(apForm); break;
            case LVSP_ID: pList = static_cast<TESLevSpell*>(apForm); break;
        }
    }
    return pList;
}

void LEVELED_OBJECT_FILE::Endian() {
    ENDIAN(sLevel, false);
    ENDIAN(iFormID, false);
    ENDIAN(sCount, false);
}

float TESGlobal::GetValue() {
    return fValue;
}
