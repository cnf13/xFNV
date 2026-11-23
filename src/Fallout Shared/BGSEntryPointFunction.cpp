#include "Fallout Shared/BGSEntryPointFunction.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPointFunctionData.h"
#include "Fallout Shared/BGSEntryPointFunctionDataActivateChoice.h"
#include "Fallout Shared/BGSEntryPointFunctionDataLeveledList.h"
#include "Fallout Shared/BGSEntryPointFunctionDataOneValue.h"
#include "Fallout Shared/BGSEntryPointFunctionDataTwoValue.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cmath>

// Static local of BGSEntryPointFunctionDataActivateChoice::Load: the script variable the SLSD/SCVR chunks being read
// belong to.
static ScriptVariable* pNewVariable;

BGSEntryPointFunctionData* CreateEntryPointFunctionObject(int32_t aiDataType) {
    switch (aiDataType) {
        case 1: {
            return new BGSEntryPointFunctionDataOneValue;
        }
        case 2: {
            return new BGSEntryPointFunctionDataTwoValue;
        }
        case 3: {
            return new BGSEntryPointFunctionDataLeveledList;
        }
        case 4: {
            return new BGSEntryPointFunctionDataActivateChoice;
        }
        default: return nullptr;
    }
}

void BGSEntryPointFunction::EntryPointFunction::AddValue(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    BGSEntryPointFunctionDataOneValue* pData = static_cast<BGSEntryPointFunctionDataOneValue*>(apData);
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::VALUE_FUNCTION,
            1,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            pData
        )) {
        float* pfValue = static_cast<float*>(*apFunctionArgument);
        *pfValue = pData->GetValue() + *pfValue;
    }
}

void BGSEntryPointFunction::EntryPointFunction::MultiplyValue(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    BGSEntryPointFunctionDataOneValue* pData = static_cast<BGSEntryPointFunctionDataOneValue*>(apData);
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::VALUE_FUNCTION,
            1,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            pData
        )) {
        float* pfValue = static_cast<float*>(*apFunctionArgument);
        *pfValue = pData->GetValue() * *pfValue;
    }
}

void BGSEntryPointFunction::EntryPointFunction::AddRangeToValue(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    BGSEntryPointFunctionDataTwoValue* pData = static_cast<BGSEntryPointFunctionDataTwoValue*>(apData);
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::VALUE_FUNCTION,
            2,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            pData
        )) {
        float* pfValue = static_cast<float*>(*apFunctionArgument);
        // Reads the two values the second one first.
        float fMax = pData->fValue2;
        float fMin = pData->fValue1;
        *pfValue = RandomFloat(fMin, fMax) + *pfValue;
    }
}

void BGSEntryPointFunction::EntryPointFunction::AddActorValueMult(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    BGSEntryPointFunctionDataTwoValue* pData = static_cast<BGSEntryPointFunctionDataTwoValue*>(apData);
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::VALUE_FUNCTION,
            2,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            pData
        )) {
        float* pfValue = static_cast<float*>(*apFunctionArgument);
        // The perk owner is an actor; its ActorValueOwner is used with no null test.
        ActorValueOwner& rOwner = *static_cast<Actor*>(apPerkOwner);
        float fMult = pData->fValue1;
        // The second value is converted to a 64-bit integer and only its low 32 bits are kept.
        int32_t iWhich = static_cast<int32_t>(static_cast<int64_t>(pData->fValue2));
        int32_t iValue = rOwner.GetActorValue(static_cast<ActorValue::Index>(iWhich));
        *pfValue = static_cast<double>(iValue) * fMult + *pfValue;
    }
}

void BGSEntryPointFunction::EntryPointFunction::AbsoluteValue(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::VALUE_FUNCTION,
            0,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            apData
        )) {
        float* pfValue = static_cast<float*>(*apFunctionArgument);
        *pfValue = std::fabs(*pfValue);
    }
}

void BGSEntryPointFunction::EntryPointFunction::NegativeAbsoluteValue(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::VALUE_FUNCTION,
            0,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            apData
        )) {
        float* pfValue = static_cast<float*>(*apFunctionArgument);
        *pfValue = -std::fabs(*pfValue);
    }
}

void BGSEntryPointFunction::EntryPointFunction::AddLeveledList(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    BGSEntryPointFunctionDataLeveledList* pData = static_cast<BGSEntryPointFunctionDataLeveledList*>(apData);
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::LEVELED_LIST_FUNCTION,
            3,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            pData
        )) {
        TESObjectREFR* pTarget = static_cast<TESObjectREFR*>(*apFunctionArgument);
        TESContainer kContainer;
        // pLeveledList is used without a null test.
        pData->pLeveledList->TESLeveledList::CalculateCurrentFormListForRef(apPerkOwner, &kContainer);
        InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pTarget);
        BSSimpleList<ContainerObject*>* pIter = &kContainer.objectList;
        if (pChanges) {
            while (pIter && !pIter->IsEmpty() && pIter->m_item) {
                ContainerObject* pEntry = pIter->m_item;
                TESBoundObject* pObj = nullptr;
                if (pEntry->pObj && pEntry->pObj->IsBoundObject()) {
                    pObj = pEntry->pObj;
                }

                if (pObj) {
                    Script* pScript = TESScriptableForm::GetFormScript(pEntry->pObj);
                    if (pScript) {
                        ExtraDataList* pLastExtra = nullptr;
                        for (int32_t i = 0; i < pEntry->i_count; ++i) {
                            ExtraDataList* pExtra;
                            pExtra = new ExtraDataList;
                            pLastExtra = pExtra;
                            pExtra->SetCount(1);
                            if (pExtra && !pExtra->GetScript()) {
                                pExtra->SetScript(pScript);
                                Script* pExtraScript = pExtra->GetScript();
                                pExtra->SetScriptLocals(pExtraScript->CloneLocals());
                            }
                        }
                        pTarget->AddObjecttoContainer(pObj, pLastExtra, 1);
                    }
                    else {
                        pTarget->AddObjecttoContainer(pObj, nullptr, pEntry->i_count);
                    }
                }
                pIter = pIter->m_pkNext;
            }
        }
    }
}

void BGSEntryPointFunction::EntryPointFunction::AddActivateChoice(
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (BGSEntryPointFunction::CheckFunctionArguments(
            BGSEntryPointFunction::ACTIVATE_FUNCTION,
            4,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            apData
        )) {
        BSSimpleList<void*>* pChoiceList = static_cast<BSSimpleList<void*>*>(*apFunctionArgument);
        TESObjectREFR* pOwnerObject = static_cast<TESObjectREFR*>(apFunctionArgument[1]);
        BGSEntryPointFunctionDataActivateChoice* pChoice = static_cast<BGSEntryPointFunctionDataActivateChoice*>(
            apData
        );
        if (pChoiceList && pChoice) {
            if (static_cast<uint8_t>(pChoice->GetRunImmediately())) {
                Script* pScript = &pChoice->ActivateChoiceScript;
                if (pScript) {
                    pScript->Run(pOwnerObject, nullptr, nullptr, false);
                }
            }
            else {
                pChoiceList->AddHead(*reinterpret_cast<void**>(&pChoice));
            }
        }
    }
}

// Whether the function may run: the function type and the data kind are the expected ones, the argument count is the
// type's own and every argument is non-null.
bool BGSEntryPointFunction::CheckFunctionArguments(
    ENTRY_POINT_FUNCTION_TYPE aeExpectedFunctionType,
    uint8_t acExpectedFunctionData,
    TESObjectREFR* apPerkOwner,
    ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (aeExpectedFunctionType != aeFunctionType) {
        Warning(
            "DEFAULT: Invalid function type %i passed to "
            "BGSEntryPointFunction::CheckFunctionArguments.  Expected %i.",
            aeFunctionType,
            aeExpectedFunctionType
        );
        return false;
    }
    if (EntryPointFunctionTypeArgumentCount[aeExpectedFunctionType] != acArgumentCount) {
        Warning(
            "DEFAULT: Invalid argument count %i passed to "
            "BGSEntryPointFunction::CheckFunctionArguments.  Expected %i.",
            acArgumentCount,
            EntryPointFunctionTypeArgumentCount[aeExpectedFunctionType]
        );
        return false;
    }
    // The index is compared unsigned against the zero-extended argument count.
    for (uint32_t i = 0; i < static_cast<uint32_t>(acArgumentCount); ++i) {
        if (!apFunctionArgument[i]) {
            Warning(
                "DEFAULT: Invalid function argument %i passed to "
                "BGSEntryPointFunction::CheckFunctionArguments.  Function arguments cannot be zero.",
                i
            );
            return false;
        }
    }
    BGSEntryPointFunctionData* pData = static_cast<BGSEntryPointFunctionData*>(apData);
    if (pData && pData->GetType() != acExpectedFunctionData) {
        Warning(
            "DEFAULT: Invalid function data type %i passed to "
            "BGSEntryPointFunction::CheckFunctionArguments.  Expected %i.",
            pData->GetType(),
            static_cast<uint32_t>(acExpectedFunctionData)
        );
        return false;
    }
    return true;
}

void BGSEntryPointFunction::SetValueFunction(
    TESObjectREFR* apPerkOwner,
    ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (!CheckFunctionArguments(
            VALUE_FUNCTION,
            1,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            apData
        )) {
        return;
    }
    float* pfValue = static_cast<float*>(apFunctionArgument[0]);
    *pfValue = static_cast<BGSEntryPointFunctionDataOneValue*>(apData)->GetValue();
}

void BGSEntryPointFunction::AddActivateChoiceFunction(
    TESObjectREFR* apPerkOwner,
    ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (!CheckFunctionArguments(
            ACTIVATE_FUNCTION,
            4,
            apPerkOwner,
            aeFunctionType,
            acArgumentCount,
            apFunctionArgument,
            apData
        )) {
        return;
    }
    auto* pChoiceList = static_cast<BSSimpleList<BGSEntryPointFunctionDataActivateChoice*>*>(apFunctionArgument[0]);
    TESObjectREFR* pTarget = static_cast<TESObjectREFR*>(apFunctionArgument[1]);
    auto* pChoice = static_cast<BGSEntryPointFunctionDataActivateChoice*>(apData);
    if (!pChoiceList || !pChoice) {
        return;
    }
    if (pChoice->GetRunImmediately()) {
        Script* pScript = pChoice->GetScript();
        if (pScript) {
            pScript->Run(pTarget, nullptr, nullptr, false);
        }
    }
    else {
        pChoiceList->AddHead(pChoice);
    }
}

void BGSEntryPointFunction::ExecuteFunction(
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION aeFunction,
    TESObjectREFR* apPerkOwner,
    BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
    uint8_t acArgumentCount,
    void** apFunctionArgument,
    void* apData
) {
    if (BGSEntryPointFunction::kEntryPointFunction[aeFunction].pFunction) {
        BGSEntryPointFunction::kEntryPointFunction[aeFunction]
            .pFunction(apPerkOwner, aeFunctionType, acArgumentCount, apFunctionArgument, apData);
    }
}

char BGSEntryPointFunction::GetArgumentCount(int32_t aiFunctionType) {
    return static_cast<int8_t>(BGSEntryPointFunction::EntryPointFunctionTypeArgumentCount[aiFunctionType]);
}

BGSEntryPointFunctionData::BGSEntryPointFunctionData() {}

BGSEntryPointFunctionData::~BGSEntryPointFunctionData() {}

BGSEntryPointFunctionDataActivateChoice::BGSEntryPointFunctionDataActivateChoice()
  : BGSEntryPointFunctionData(), pActivateChoiceLabel(nullptr), ActivateChoiceScript(), cFlags(0) {
    InitializeData();
}

BGSEntryPointFunctionDataActivateChoice::~BGSEntryPointFunctionDataActivateChoice() {
    SetLabel(nullptr);
}

BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA BGSEntryPointFunctionDataActivateChoice::GetType() {
    return ACTIVATE_CHOICE;
}

BGSEntryPointFunctionData* BGSEntryPointFunctionDataActivateChoice::Duplicate() {
    BGSEntryPointFunctionDataActivateChoice* pCopy = new BGSEntryPointFunctionDataActivateChoice;
    pCopy->SetLabel(pActivateChoiceLabel);
    Script* pScript = GetScript();
    Script* pCopyScript = pCopy->GetScript();
    pCopyScript->CopyResultScript(pScript);
    pCopy->cFlags = cFlags;
    return pCopy;
}

bool BGSEntryPointFunctionDataActivateChoice::Compare(BGSEntryPointFunctionData* apCompare) {
    BGSEntryPointFunctionDataActivateChoice* pActivateChoice = dynamic_cast<BGSEntryPointFunctionDataActivateChoice*>(
        apCompare
    );
    if (!pActivateChoice) {
        return true;
    }
    if ((pActivateChoiceLabel && !pActivateChoice->pActivateChoiceLabel) ||
        (!pActivateChoiceLabel && pActivateChoice->pActivateChoiceLabel)) {
        return true;
    }
    if (pActivateChoiceLabel &&
        pActivateChoice->pActivateChoiceLabel &&
        BSstrcmp(pActivateChoiceLabel, pActivateChoice->pActivateChoiceLabel)) {
        return true;
    }
    if (ActivateChoiceScript.CompareResultScripts(&pActivateChoice->ActivateChoiceScript)) {
        return true;
    }
    return cFlags != pActivateChoice->cFlags;
}

void BGSEntryPointFunctionDataActivateChoice::Save() {
    BGSEntryPointFunctionData::Save();
    if (pActivateChoiceLabel && std::strlen(pActivateChoiceLabel)) {
        uint32_t uiLength = BSstrlen(pActivateChoiceLabel);
        TESForm::AddChunkArray(EPF2_ID, pActivateChoiceLabel, uiLength + 1);
    }

    TESForm::AddChunk(EPF3_ID, cFlags);
    ActivateChoiceScript.SaveResultScript();
}

// One chunk of the choice: the label, the flags, or a piece of the choice script. The SLSD/SCVR case re-reads the
// chunk. The SCRO and SCRV chunks are read into locals that are stored into the new reference; a chunk shorter than
// four bytes leaves them unwritten (malformed data), so they are initialised to 0.
bool BGSEntryPointFunctionDataActivateChoice::Load(TESFile* apFile) {
    switch (apFile->GetTESChunk()) {
        case SCHR_ID:
            apFile->GetChunkData(ActivateChoiceScript.GetHeader(), 0);
            if (apFile->QEndian()) {
                ActivateChoiceScript.GetHeader()->Endian();
            }
            break;
        case SLSD_ID:
        case SCVR_ID:
            if (apFile->GetTESChunk() == SLSD_ID) {
                pNewVariable = new ScriptVariable;
                pNewVariable->Load(apFile);
                ActivateChoiceScript.GetVariableList()->AddTail(pNewVariable);
            }
            else if (pNewVariable) {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                pNewVariable->cName.Set(pBuffer, 0);
            }
            break;
        case SCDA_ID: {
            uint32_t uiChunkSize = apFile->GetChunkSize();
            // A 16-byte aligned buffer of the chunk's size.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
            ourMemset(pBuffer, 0, uiChunkSize);
            apFile->GetChunkData(pBuffer, 0);
            ActivateChoiceScript.SetCompileData(uiChunkSize, pBuffer);
            break;
        }
        case SCRO_ID: {
            Script::RefVariable* pReferenced = new Script::RefVariable;
            uint32_t uiFormID = 0;
            apFile->GetChunkData(uiFormID);
            pReferenced->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
            ActivateChoiceScript.GetReferencedObjectList()->AddTail(pReferenced);
            break;
        }
        case SCRV_ID: {
            Script::RefVariable* pReferenced = new Script::RefVariable;
            uint32_t uiVariableID = 0;
            apFile->GetChunkData(uiVariableID);
            pReferenced->varIdx = uiVariableID;
            ActivateChoiceScript.GetReferencedObjectList()->AddTail(pReferenced);
            break;
        }
        case EPF2_ID: {
            // A 16-byte aligned buffer of the chunk's size.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0);
            SetLabel(pBuffer);
            break;
        }
        case EPF3_ID: apFile->LoadWord(reinterpret_cast<int16_t*>(&cFlags)); break;
        default: break;
    }
    return true;
}

// Resolves the choice script's form references, then reports what its InitItem warned about, tagged with the owning
// form's editor ID and form ID.
void BGSEntryPointFunctionDataActivateChoice::InitItem(TESForm* apOwner) {
    int32_t iOldWarningCount = MessageHandler::QWarningCount();
    MessageHandler::IncDisableWarningCount(true);
    MessageHandler::SetWarningCount(0);
    ActivateChoiceScript.InitItem(apOwner);
    MessageHandler::IncDisableWarningCount(false);
    int32_t iWarningCount = MessageHandler::QWarningCount();
    MessageHandler::SetWarningCount(iOldWarningCount);
    if (iWarningCount) {
        // Reads the form ID before the editor ID.
        uint32_t uiFormID = apOwner->GetFormID();
        const char* pcEditorID = apOwner->GetFormEditorID();
        Warning(
            "MASTERFILE: Errors were encountered during InitItem for result script for perk '%s' (%08X)\n"
            "\n"
            "See Warnings file for more information.",
            pcEditorID,
            uiFormID
        );
    }
}

void BGSEntryPointFunctionDataActivateChoice::InitializeData() {
    cFlags = 0;
    ActivateChoiceScript.TESForm::SetInitialized(false);
    ActivateChoiceScript.TESForm::SetTemporary();
}

void BGSEntryPointFunctionDataActivateChoice::SetLabel(const char* apLabel) {
    ourFree(pActivateChoiceLabel);
    pActivateChoiceLabel = nullptr;
    if (apLabel) {
        uint32_t uiLength = BSstrlen(apLabel);
        pActivateChoiceLabel = static_cast<char*>(ourMalloc(uiLength + 1));
        ourStrcpy(pActivateChoiceLabel, apLabel);
    }
}

Script* BGSEntryPointFunctionDataActivateChoice::GetScript() {
    return &ActivateChoiceScript;
}

uint32_t BGSEntryPointFunctionDataActivateChoice::GetRunImmediately() {
    return cFlags & FLAG_RUN_IMMEDIATELY;
}

BGSEntryPointFunctionDataLeveledList::BGSEntryPointFunctionDataLeveledList()
  : BGSEntryPointFunctionData(), pLeveledList(nullptr) {}

BGSEntryPointFunctionDataLeveledList::~BGSEntryPointFunctionDataLeveledList() {}

BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA BGSEntryPointFunctionDataLeveledList::GetType() {
    return LEVELED_LIST;
}

BGSEntryPointFunctionData* BGSEntryPointFunctionDataLeveledList::Duplicate() {
    BGSEntryPointFunctionDataLeveledList* pCopy;
    pCopy = new BGSEntryPointFunctionDataLeveledList;
    pCopy->pLeveledList = pLeveledList;
    return pCopy;
}

bool BGSEntryPointFunctionDataLeveledList::Compare(BGSEntryPointFunctionData* apCompare) {
    BGSEntryPointFunctionDataLeveledList* pCompare = dynamic_cast<BGSEntryPointFunctionDataLeveledList*>(apCompare);
    return !pCompare || pLeveledList != pCompare->pLeveledList;
}

void BGSEntryPointFunctionDataLeveledList::Save() {
    uint32_t uiFormID;
    BGSEntryPointFunctionData::Save();
    if (pLeveledList) {
        uiFormID = pLeveledList->TESForm::GetFormID();
        TESForm::AddChunk(EPFD_ID, uiFormID);
    }
}

// Reads the leveled list's form ID into pLeveledList; InitItem resolves it. Any other chunk size fails.
bool BGSEntryPointFunctionDataLeveledList::Load(TESFile* apFile) {
    if (apFile->GetChunkSize() != 4) {
        return false;
    }
    uint32_t iFormID = 0;
    apFile->GetChunkData(&iFormID, 0);
    if (apFile->QEndian()) {
        ENDIAN(iFormID, false);
    }
    pLeveledList = reinterpret_cast<TESLevItem*>(static_cast<uintptr_t>(iFormID));
    return true;
}

void BGSEntryPointFunctionDataLeveledList::InitItem(TESForm* apOwner) {
    if (pLeveledList) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLeveledList));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(uiTempInt, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        pLeveledList = dynamic_cast<TESLevItem*>(pForm);
        if (!pLeveledList) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* pcEditorID = apOwner->GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find leveled list (%08X) on owner object \"%s\".",
                    uiTempInt,
                    pcEditorID
                );
            }
            else {
                int32_t iOwnerFormID = apOwner->iFormID;
                Warning(
                    "MASTERFILE: Unable to find leveled list (%08X) on owner object (%08X).",
                    uiTempInt,
                    iOwnerFormID
                );
            }
        }
    }
}

BGSEntryPointFunctionDataOneValue::BGSEntryPointFunctionDataOneValue() : BGSEntryPointFunctionData(), fValue(0.0f) {}

BGSEntryPointFunctionDataOneValue::~BGSEntryPointFunctionDataOneValue() {}

BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA BGSEntryPointFunctionDataOneValue::GetType() {
    return ONE_VALUE;
}

BGSEntryPointFunctionData* BGSEntryPointFunctionDataOneValue::Duplicate() {
    BGSEntryPointFunctionDataOneValue* pCopy;
    pCopy = new BGSEntryPointFunctionDataOneValue;
    pCopy->fValue = fValue;
    return pCopy;
}

bool BGSEntryPointFunctionDataOneValue::Compare(BGSEntryPointFunctionData* apCompare) {
    BGSEntryPointFunctionDataOneValue* pCompare = dynamic_cast<BGSEntryPointFunctionDataOneValue*>(apCompare);
    return !pCompare || pCompare->fValue != fValue;
}

void BGSEntryPointFunctionDataOneValue::Save() {
    BGSEntryPointFunctionData::Save();
    TESForm::AddChunk(EPFD_ID, fValue);
}

// Reads the one float of the chunk; any other chunk size fails.
bool BGSEntryPointFunctionDataOneValue::Load(TESFile* apFile) {
    if (apFile->GetChunkSize() != 4) {
        return false;
    }
    apFile->GetChunkData(&fValue, 0);
    if (apFile->QEndian()) {
        ENDIAN(fValue, false);
    }
    return true;
}

float BGSEntryPointFunctionDataOneValue::GetValue() {
    return fValue;
}

// Both values swapped for a big-endian file.
void TwoValues::Endian() {
    ENDIAN(fValue1, false);
    ENDIAN(fValue2, false);
}

BGSEntryPointFunctionDataTwoValue::BGSEntryPointFunctionDataTwoValue()
  : BGSEntryPointFunctionData(), fValue1(0.0f), fValue2(0.0f) {}

BGSEntryPointFunctionDataTwoValue::~BGSEntryPointFunctionDataTwoValue() {}

BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA BGSEntryPointFunctionDataTwoValue::GetType() {
    return TWO_VALUE;
}

BGSEntryPointFunctionData* BGSEntryPointFunctionDataTwoValue::Duplicate() {
    BGSEntryPointFunctionDataTwoValue* pCopy;
    pCopy = new BGSEntryPointFunctionDataTwoValue;
    pCopy->fValue1 = fValue1;
    pCopy->fValue2 = fValue2;
    return pCopy;
}

bool BGSEntryPointFunctionDataTwoValue::Compare(BGSEntryPointFunctionData* apCompare) {
    BGSEntryPointFunctionDataTwoValue* pCompare = dynamic_cast<BGSEntryPointFunctionDataTwoValue*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pCompare->fValue1 == fValue1) {
        return pCompare->fValue2 != fValue2;
    }
    return true;
}

void BGSEntryPointFunctionDataTwoValue::Save() {
    TwoValues kData;
    BGSEntryPointFunctionData::Save();
    kData.fValue1 = fValue1;
    kData.fValue2 = fValue2;
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESForm::__AddChunkData(EPFD_ID, &kData, 8u);
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
}

bool BGSEntryPointFunctionDataTwoValue::Load(TESFile* apFile) {
    TwoValues kData;
    uint32_t uiSize = apFile->GetChunkSize();
    if (uiSize != 8) {
        return false;
    }
    apFile->GetChunkData(&kData, 8u);
    if (apFile->QEndian()) {
        kData.Endian();
    }
    fValue1 = kData.fValue1;
    fValue2 = kData.fValue2;
    return true;
}
