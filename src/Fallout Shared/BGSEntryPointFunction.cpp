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

#include <cmath>

// The PDB's static local of BGSEntryPointFunctionDataActivateChoice::Load: the script variable the SLSD/SCVR chunks
// being read belong to.
static ScriptVariable* pNewVariable;

// The exe's cleanup-only C++ EH frame is left out.
BGSEntryPointFunctionData* CreateEntryPointFunctionObject(int32_t a1) {
	switch (a1) {
	case 1: {
		BGSEntryPointFunctionDataOneValue* pMemory =
			static_cast<BGSEntryPointFunctionDataOneValue*>(ourMalloc(sizeof(BGSEntryPointFunctionDataOneValue)));
		if (pMemory) {
			return ::new (pMemory) BGSEntryPointFunctionDataOneValue;
		}
		return 0;
	}
	case 2: {
		BGSEntryPointFunctionDataTwoValue* pMemory =
			static_cast<BGSEntryPointFunctionDataTwoValue*>(ourMalloc(sizeof(BGSEntryPointFunctionDataTwoValue)));
		if (pMemory) {
			return ::new (pMemory) BGSEntryPointFunctionDataTwoValue;
		}
		return 0;
	}
	case 3: {
		BGSEntryPointFunctionDataLeveledList* pMemory =
			static_cast<BGSEntryPointFunctionDataLeveledList*>(ourMalloc(sizeof(BGSEntryPointFunctionDataLeveledList)));
		if (pMemory) {
			return ::new (pMemory) BGSEntryPointFunctionDataLeveledList;
		}
		return 0;
	}
	case 4: {
		BGSEntryPointFunctionDataActivateChoice* pMemory = static_cast<
			BGSEntryPointFunctionDataActivateChoice*>(ourMalloc(sizeof(BGSEntryPointFunctionDataActivateChoice)));
		if (pMemory) {
			return ::new (pMemory) BGSEntryPointFunctionDataActivateChoice;
		}
		return 0;
	}
	default:
		return 0;
	}
}

void BGSEntryPointFunction::EntryPointFunction::AddValue(TESObjectREFR* a1,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t arg8, void** a4, void* apData) {
	BGSEntryPointFunctionDataOneValue* a5 = static_cast<BGSEntryPointFunctionDataOneValue*>(apData);
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::VALUE_FUNCTION, 1, a1, aeFunctionType,
			arg8, a4, a5)) {
		float* pfValue = static_cast<float*>(*a4);
		*pfValue = a5->GetValue() + *pfValue;
	}
}

void BGSEntryPointFunction::EntryPointFunction::MultiplyValue(TESObjectREFR* a1,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t arg8, void** a4, void* apData) {
	BGSEntryPointFunctionDataOneValue* a5 = static_cast<BGSEntryPointFunctionDataOneValue*>(apData);
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::VALUE_FUNCTION, 1, a1, aeFunctionType,
			arg8, a4, a5)) {
		float* pfValue = static_cast<float*>(*a4);
		*pfValue = a5->GetValue() * *pfValue;
	}
}

void BGSEntryPointFunction::EntryPointFunction::AddRangeToValue(TESObjectREFR* a1,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t arg8, void** a4, void* apData) {
	BGSEntryPointFunctionDataTwoValue* a5 = static_cast<BGSEntryPointFunctionDataTwoValue*>(apData);
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::VALUE_FUNCTION, 2, a1, aeFunctionType,
			arg8, a4, a5)) {
		float* pfValue = static_cast<float*>(*a4);
		// The exe reads the two values through one-field getters (0x488D50, 0x6B9130), the second one first.
		float fMax = a5->fValue2;
		float fMin = a5->fValue1;
		*pfValue = RandomFloat(fMin, fMax) + *pfValue;
	}
}

void BGSEntryPointFunction::EntryPointFunction::AddActorValueMult(TESObjectREFR* a1,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t arg8, void** a4, void* apData) {
	BGSEntryPointFunctionDataTwoValue* a5 = static_cast<BGSEntryPointFunctionDataTwoValue*>(apData);
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::VALUE_FUNCTION, 2, a1, aeFunctionType,
			arg8, a4, a5)) {
		float* pfValue = static_cast<float*>(*a4);
		// The perk owner is an actor: the exe calls slot 2 of the ActorValueOwner at +0xA4 with no null test.
		ActorValueOwner& rOwner = *static_cast<Actor*>(a1);
		float fMult = a5->fValue1;
		// The exe converts the second value with fistp qword and keeps the low dword.
		int32_t iWhich = static_cast<int32_t>(static_cast<int64_t>(a5->fValue2));
		int32_t iValue = rOwner.GetActorValue(static_cast<ActorValue::Index>(iWhich));
		*pfValue = static_cast<double>(iValue) * fMult + *pfValue;
	}
}

void BGSEntryPointFunction::EntryPointFunction::AbsoluteValue(TESObjectREFR* a1,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t arg8, void** a4, void* apData) {
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::VALUE_FUNCTION, 0, a1, aeFunctionType,
			arg8, a4, apData)) {
		float* pfValue = static_cast<float*>(*a4);
		*pfValue = std::fabs(*pfValue);
	}
}

void BGSEntryPointFunction::EntryPointFunction::NegativeAbsoluteValue(TESObjectREFR* a1,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t arg8, void** a4, void* apData) {
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::VALUE_FUNCTION, 0, a1, aeFunctionType,
			arg8, a4, apData)) {
		float* pfValue = static_cast<float*>(*a4);
		*pfValue = -std::fabs(*pfValue);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSEntryPointFunction::EntryPointFunction::AddLeveledList(TESObjectREFR* arg0,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t a3, void** a4, void* apData) {
	BGSEntryPointFunctionDataLeveledList* a5 = static_cast<BGSEntryPointFunctionDataLeveledList*>(apData);
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::LEVELED_LIST_FUNCTION, 3, arg0,
			aeFunctionType, a3, a4, a5)) {
		TESObjectREFR* a1 = static_cast<TESObjectREFR*>(*a4);
		FNVD_CONSTRUCTED_LOCAL(TESContainer, a2, &TESContainer::Destruct);
		// The exe reads pLeveledList through a one-field getter (0x726070) and adds 0x30 with no null test.
		a5->pLeveledList->TESLeveledList::CalculateCurrentFormListForRef(arg0, &a2);
		InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(a1);
		BSSimpleList<ContainerObject*>* ecx0 = &a2.objectList;
		if (InventoryChanges_) {
			while (ecx0 && (!ecx0->IsEmpty() && ecx0->m_item)) {
				ContainerObject* m_item = ecx0->m_item;
				TESBoundObject* pObj = 0;
				if (m_item->pObj) {
					if (m_item->pObj->IsBoundObject()) {
						pObj = m_item->pObj;
					}
				}
				if (pObj) {
					Script* apScript = TESScriptableForm::GetFormScript(m_item->pObj);
					if (apScript) {
						ExtraDataList* v10 = 0;
						for (int32_t i = 0; i < m_item->i_count; ++i) {
							ExtraDataList* v7;
							ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
							if (apMemory) {
								v7 = ::new (apMemory) ExtraDataList;
							} else {
								v7 = 0;
							}
							v10 = v7;
							v7->SetCount(1);
							if (v7) {
								if (!v7->GetScript()) {
									v7->SetScript(apScript);
									Script* Script_ = v7->GetScript();
									ScriptLocals* v6 = Script_->CloneLocals();
									v7->SetScriptLocals(v6);
								}
							}
						}
						a1->AddObjecttoContainer(pObj, v10, 1);
					} else {
						a1->AddObjecttoContainer(pObj, 0, m_item->i_count);
					}
				}
				ecx0 = ecx0->m_pkNext;
			}
		}
	}
}

void BGSEntryPointFunction::EntryPointFunction::AddActivateChoice(TESObjectREFR* apPerkOwner,
	BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t acArgumentCount, void** apFunctionArgument,
	void* apData) {
	BGSEntryPointFunctionDataActivateChoice* a2;
	if (BGSEntryPointFunction::CheckFunctionArguments(BGSEntryPointFunction::ACTIVATE_FUNCTION, 4, apPerkOwner,
			aeFunctionType, acArgumentCount, apFunctionArgument, apData)) {
		BSSimpleList<void*>* v6 = static_cast<BSSimpleList<void*>*>(*apFunctionArgument);
		TESObjectREFR* ownerObject = static_cast<TESObjectREFR*>(apFunctionArgument[1]);
		a2 = static_cast<BGSEntryPointFunctionDataActivateChoice*>(apData);
		if (v6) {
			if (a2) {
				if (static_cast<uint8_t>(a2->GetRunImmediately())) {
					Script* Script_ = &a2->ActivateChoiceScript;
					if (Script_) {
						Script_->Run(ownerObject, 0, 0, 0);
					}
				} else {
					v6->AddHead(*reinterpret_cast<void**>(&a2));
				}
			}
		}
	}
}

// Whether the function may run: the function type and the data kind are the expected ones, the argument count is the
// type's own and every argument is non-null.
bool BGSEntryPointFunction::CheckFunctionArguments(ENTRY_POINT_FUNCTION_TYPE aeExpectedFunctionType,
	uint8_t acExpectedFunctionData, TESObjectREFR* apPerkOwner, ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
	uint8_t acArgumentCount, void** apFunctionArgument, void* apData) {
	if (aeExpectedFunctionType != aeFunctionType) {
		Warning("DEFAULT: Invalid function type %i passed to "
				"BGSEntryPointFunction::CheckFunctionArguments.  Expected %i.",
			aeFunctionType, aeExpectedFunctionType);
		return false;
	}
	if (EntryPointFunctionTypeArgumentCount[aeExpectedFunctionType] != acArgumentCount) {
		Warning("DEFAULT: Invalid argument count %i passed to "
				"BGSEntryPointFunction::CheckFunctionArguments.  Expected %i.",
			acArgumentCount, EntryPointFunctionTypeArgumentCount[aeExpectedFunctionType]);
		return false;
	}
	// The exe compares the index against the zero-extended argument count with an unsigned branch.
	for (uint32_t i = 0; i < static_cast<uint32_t>(acArgumentCount); ++i) {
		if (!apFunctionArgument[i]) {
			Warning("DEFAULT: Invalid function argument %i passed to "
					"BGSEntryPointFunction::CheckFunctionArguments.  Function arguments cannot be zero.",
				i);
			return false;
		}
	}
	BGSEntryPointFunctionData* pData = static_cast<BGSEntryPointFunctionData*>(apData);
	if (pData && pData->GetType() != acExpectedFunctionData) {
		Warning("DEFAULT: Invalid function data type %i passed to "
				"BGSEntryPointFunction::CheckFunctionArguments.  Expected %i.",
			pData->GetType(), static_cast<uint32_t>(acExpectedFunctionData));
		return false;
	}
	return true;
}

void BGSEntryPointFunction::SetValueFunction(TESObjectREFR* apPerkOwner, ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
	uint8_t acArgumentCount, void** apFunctionArgument, void* apData) {
	if (!CheckFunctionArguments(VALUE_FUNCTION, 1, apPerkOwner, aeFunctionType, acArgumentCount, apFunctionArgument,
			apData)) {
		return;
	}
	float* pfValue = static_cast<float*>(apFunctionArgument[0]);
	*pfValue = static_cast<BGSEntryPointFunctionDataOneValue*>(apData)->GetValue();
}

// Left unregistered: the strict check (checkbody 6c65a02cdc89, and 29d4f0babe86 with 40 seeds) is UNCOVERED: the
// edge 0x5E6221->0x5E6235 (no script, skip Run) is never taken because GetScript's stub never returns null. The
// compared states agree, eax on the early return included.
void BGSEntryPointFunction::AddActivateChoiceFunction(TESObjectREFR* apPerkOwner,
	ENTRY_POINT_FUNCTION_TYPE aeFunctionType, uint8_t acArgumentCount, void** apFunctionArgument, void* apData) {
	if (!CheckFunctionArguments(ACTIVATE_FUNCTION, 4, apPerkOwner, aeFunctionType, acArgumentCount, apFunctionArgument,
			apData)) {
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
	} else {
		pChoiceList->AddHead(pChoice);
	}
}

void BGSEntryPointFunction::ExecuteFunction(BGSEntryPointFunction::ENTRY_POINT_FUNCTION aeFunction,
	TESObjectREFR* apPerkOwner, BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
	uint8_t acArgumentCount, void** apFunctionArgument, void* apData) {
	if (BGSEntryPointFunction::kEntryPointFunction[aeFunction].pFunction) {
		BGSEntryPointFunction::kEntryPointFunction[aeFunction].pFunction(apPerkOwner, aeFunctionType, acArgumentCount,
			apFunctionArgument, apData);
	}
}

char BGSEntryPointFunction::GetArgumentCount(int32_t a1) {
	return (int8_t)BGSEntryPointFunction::EntryPointFunctionTypeArgumentCount[a1];
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

// The exe's cleanup-only C++ EH frame is left out.
BGSEntryPointFunctionData* BGSEntryPointFunctionDataActivateChoice::Duplicate() {
	BGSEntryPointFunctionDataActivateChoice* v4;
	BGSEntryPointFunctionDataActivateChoice* apMemory = static_cast<
		BGSEntryPointFunctionDataActivateChoice*>(ourMalloc(sizeof(BGSEntryPointFunctionDataActivateChoice)));
	if (apMemory) {
		v4 = ::new (apMemory) BGSEntryPointFunctionDataActivateChoice;
	} else {
		v4 = 0;
	}
	v4->SetLabel(pActivateChoiceLabel);
	Script* Name = GetScript();
	Script* v1 = v4->GetScript();
	v1->CopyResultScript(Name);
	v4->cFlags = cFlags;
	return v4;
}

bool BGSEntryPointFunctionDataActivateChoice::Compare(BGSEntryPointFunctionData* apCompare) {
	BGSEntryPointFunctionDataActivateChoice* pactivateChoice =
		dynamic_cast<BGSEntryPointFunctionDataActivateChoice*>(apCompare);
	if (!pactivateChoice) {
		return 1;
	}
	if ((pActivateChoiceLabel && !pactivateChoice->pActivateChoiceLabel) ||
		(!pActivateChoiceLabel && pactivateChoice->pActivateChoiceLabel)) {
		return 1;
	}
	if (pActivateChoiceLabel &&
		(pactivateChoice->pActivateChoiceLabel &&
			BSstrcmp(pActivateChoiceLabel, pactivateChoice->pActivateChoiceLabel))) {
		return 1;
	}
	if ((&ActivateChoiceScript)->CompareResultScripts(&pactivateChoice->ActivateChoiceScript)) {
		return 1;
	}
	return cFlags != pactivateChoice->cFlags;
}

void BGSEntryPointFunctionDataActivateChoice::Save() {
	BGSEntryPointFunctionData::Save();
	if (pActivateChoiceLabel) {
		if (std::strlen(pActivateChoiceLabel)) {
			uint32_t v1 = BSstrlen(pActivateChoiceLabel);
			TESForm::AddChunkArray(EPF2_ID, pActivateChoiceLabel, v1 + 1);
		}
	}
	TESForm::AddChunk(EPF3_ID, cFlags);
	Script* Name = &ActivateChoiceScript;
	Name->SaveResultScript();
}

// One chunk of the choice: the label, the flags, or a piece of the choice script. The SLSD/SCVR case re-reads the
// chunk as the exe does. The exe's cleanup-only EH frame (over the two allocations) is left out.
// The exe reads the SCRO and SCRV chunks into stack dwords it never initialises ([ebp-0x1C], [ebp-0x24]) and stores
// them into the new reference; a chunk shorter than four bytes leaves them unwritten (malformed data), so src
// initialises them to 0.
bool BGSEntryPointFunctionDataActivateChoice::Load(TESFile* apFile) {
	switch (apFile->GetTESChunk()) {
	case SCHR_ID:
		apFile->GetChunkData(ActivateChoiceScript.GetHeader(), 0);
		if (apFile->QEndian())
			ActivateChoiceScript.GetHeader()->Endian();
		break;
	case SLSD_ID:
	case SCVR_ID:
		if (apFile->GetTESChunk() == SLSD_ID) {
			void* pMemory = ourMalloc(sizeof(ScriptVariable));
			pNewVariable = pMemory ? ::new (pMemory) ScriptVariable : nullptr;
			pNewVariable->Load(apFile);
			ActivateChoiceScript.GetVariableList()->AddTail(pNewVariable);
		} else if (pNewVariable) {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			pNewVariable->cName.Set(pBuffer, 0);
		}
		break;
	case SCDA_ID: {
		uint32_t uiChunkSize = apFile->GetChunkSize();
		// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
		ourMemset(pBuffer, 0, uiChunkSize);
		apFile->GetChunkData(pBuffer, 0);
		ActivateChoiceScript.SetCompileData(uiChunkSize, pBuffer);
		break;
	}
	case SCRO_ID: {
		void* pMemory = ourMalloc(sizeof(Script::RefVariable));
		Script::RefVariable* pReferenced = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
		uint32_t uiFormID = 0;
		apFile->GetChunkData(uiFormID);
		pReferenced->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
		ActivateChoiceScript.GetReferencedObjectList()->AddTail(pReferenced);
		break;
	}
	case SCRV_ID: {
		void* pMemory = ourMalloc(sizeof(Script::RefVariable));
		Script::RefVariable* pReferenced = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
		uint32_t uiVariableID = 0;
		apFile->GetChunkData(uiVariableID);
		pReferenced->varIdx = uiVariableID;
		ActivateChoiceScript.GetReferencedObjectList()->AddTail(pReferenced);
		break;
	}
	case EPF2_ID: {
		// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		SetLabel(pBuffer);
		break;
	}
	case EPF3_ID:
		apFile->LoadWord(reinterpret_cast<int16_t*>(&cFlags));
		break;
	default:
		break;
	}
	return true;
}

// Resolves the choice script's form references, then reports what its InitItem warned about, tagged with the owning
// form's editor ID and form ID.
void BGSEntryPointFunctionDataActivateChoice::InitItem(TESForm* apOwner) {
	int32_t iOldWarningCount = static_cast<int32_t>(MessageHandler::QWarningCount());
	MessageHandler::IncDisableWarningCount(true);
	MessageHandler::SetWarningCount(0);
	ActivateChoiceScript.InitItem(apOwner);
	MessageHandler::IncDisableWarningCount(false);
	int32_t iWarningCount = static_cast<int32_t>(MessageHandler::QWarningCount());
	MessageHandler::SetWarningCount(iOldWarningCount);
	if (iWarningCount) {
		// The exe reads the form ID before the editor ID.
		uint32_t uiFormID = apOwner->GetFormID();
		const char* pcEditorID = apOwner->GetFormEditorID();
		Warning("MASTERFILE: Errors were encountered during InitItem for result script for perk '%s' (%08X)\n"
				"\n"
				"See Warnings file for more information.",
			pcEditorID, uiFormID);
	}
}

void BGSEntryPointFunctionDataActivateChoice::InitializeData() {
	cFlags = (uint16_t)0;
	(&ActivateChoiceScript)->TESForm::SetInitialized(0);
	(&ActivateChoiceScript)->TESForm::SetTemporary();
}

void BGSEntryPointFunctionDataActivateChoice::SetLabel(const char* apLabel) {
	ourFree(pActivateChoiceLabel);
	pActivateChoiceLabel = 0;
	if (apLabel) {
		uint32_t v2 = BSstrlen(apLabel);
		pActivateChoiceLabel = static_cast<char*>(ourMalloc(v2 + 1));
		ourStrcpy(pActivateChoiceLabel, apLabel);
	}
}

Script* BGSEntryPointFunctionDataActivateChoice::GetScript() {
	return &ActivateChoiceScript;
}

uint32_t BGSEntryPointFunctionDataActivateChoice::GetRunImmediately() {
	return cFlags & 1;
}

BGSEntryPointFunctionDataLeveledList::BGSEntryPointFunctionDataLeveledList()
	: BGSEntryPointFunctionData(), pLeveledList(nullptr) {}

// The exe's vtable holds BGSEntryPointFunctionDataOneValue's deleting destructor (0x5E64E0), which runs only the
// base's destructor.
BGSEntryPointFunctionDataLeveledList::~BGSEntryPointFunctionDataLeveledList() {}

BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA BGSEntryPointFunctionDataLeveledList::GetType() {
	return LEVELED_LIST;
}

BGSEntryPointFunctionData* BGSEntryPointFunctionDataLeveledList::Duplicate() {
	BGSEntryPointFunctionDataLeveledList* v2;
	void* v4 = ourMalloc(sizeof(BGSEntryPointFunctionDataLeveledList));
	if (v4) {
		v2 = ::new (v4) BGSEntryPointFunctionDataLeveledList;
	} else {
		v2 = nullptr;
	}
	v2->pLeveledList = pLeveledList;
	return v2;
}

bool BGSEntryPointFunctionDataLeveledList::Compare(BGSEntryPointFunctionData* apCompare) {
	BGSEntryPointFunctionDataLeveledList* v4 = dynamic_cast<BGSEntryPointFunctionDataLeveledList*>(apCompare);
	return (bool)((uint8_t)((!v4) || (pLeveledList != v4->pLeveledList)));
}

void BGSEntryPointFunctionDataLeveledList::Save() {
	uint32_t v2;
	BGSEntryPointFunctionData::Save();
	if (pLeveledList) {
		v2 = pLeveledList->TESForm::GetFormID();
		TESForm::AddChunk(static_cast<CHUNK_ID>(1145458757), v2);
	}
}

// Reads the leveled list's form ID into pLeveledList; InitItem resolves it. Any other chunk size fails.
bool BGSEntryPointFunctionDataLeveledList::Load(TESFile* apFile) {
	if (apFile->GetChunkSize() != 4)
		return false;
	uint32_t iFormID = 0;
	apFile->GetChunkData(&iFormID, 0);
	if (apFile->QEndian())
		ENDIAN(iFormID, false);
	pLeveledList = reinterpret_cast<TESLevItem*>(static_cast<uintptr_t>(iFormID));
	return true;
}

void BGSEntryPointFunctionDataLeveledList::InitItem(TESForm* apOwner) {
	if (pLeveledList) {
		uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLeveledList));
		TESFile* File = apOwner->GetFile(-1);
		TESForm::AddCompileIndex(itempint, File);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
		pLeveledList = dynamic_cast<TESLevItem*>(FormByNumericID);
		if (!pLeveledList) {
			if (apOwner->GetFormEditorIDLength()) {
				const char* v4 = apOwner->GetFormEditorID();
				Warning("MASTERFILE: Unable to find leveled list (%08X) on owner object \"%s\".", itempint, v4);
			} else {
				int32_t CastingType = apOwner->iFormID;
				Warning("MASTERFILE: Unable to find leveled list (%08X) on owner object (%08X).", itempint,
					CastingType);
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
	BGSEntryPointFunctionDataOneValue* v2;
	void* v4 = ourMalloc(sizeof(BGSEntryPointFunctionDataOneValue));
	if (v4) {
		v2 = ::new (v4) BGSEntryPointFunctionDataOneValue;
	} else {
		v2 = nullptr;
	}
	v2->fValue = fValue;
	return v2;
}

bool BGSEntryPointFunctionDataOneValue::Compare(BGSEntryPointFunctionData* apCompare) {
	BGSEntryPointFunctionDataOneValue* v4 = dynamic_cast<BGSEntryPointFunctionDataOneValue*>(apCompare);
	return (bool)((uint8_t)((!v4) || (v4->fValue != fValue)));
}

// 'EPFD' (0x44465045) has no named CHUNK_ID value in src.
void BGSEntryPointFunctionDataOneValue::Save() {
	BGSEntryPointFunctionData::Save();
	TESForm::AddChunk(static_cast<CHUNK_ID>(0x44465045), fValue);
}

// Reads the one float of the chunk; any other chunk size fails.
bool BGSEntryPointFunctionDataOneValue::Load(TESFile* apFile) {
	if (apFile->GetChunkSize() != 4)
		return false;
	apFile->GetChunkData(&fValue, 0);
	if (apFile->QEndian())
		ENDIAN(fValue, false);
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

// The exe's vtable holds BGSEntryPointFunctionDataOneValue's deleting destructor (0x5E64E0), which runs only the
// base's destructor.
BGSEntryPointFunctionDataTwoValue::~BGSEntryPointFunctionDataTwoValue() {}

BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA BGSEntryPointFunctionDataTwoValue::GetType() {
	return TWO_VALUE;
}

BGSEntryPointFunctionData* BGSEntryPointFunctionDataTwoValue::Duplicate() {
	BGSEntryPointFunctionDataTwoValue* v2;
	void* v4 = ourMalloc(sizeof(BGSEntryPointFunctionDataTwoValue));
	if (v4) {
		v2 = ::new (v4) BGSEntryPointFunctionDataTwoValue;
	} else {
		v2 = nullptr;
	}
	v2->fValue1 = fValue1;
	v2->fValue2 = fValue2;
	return v2;
}

bool BGSEntryPointFunctionDataTwoValue::Compare(BGSEntryPointFunctionData* apCompare) {
	BGSEntryPointFunctionDataTwoValue* v4 = dynamic_cast<BGSEntryPointFunctionDataTwoValue*>(apCompare);
	if (!v4) {
		return (bool)1;
	}
	if (v4->fValue1 == fValue1) {
		return (bool)((uint8_t)(v4->fValue2 != fValue2));
	}
	return (bool)1;
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
	uint32_t isize = apFile->GetChunkSize();
	if (isize != 8) {
		return 0;
	}
	apFile->GetChunkData(&kData, 8u);
	if (apFile->QEndian()) {
		kData.Endian();
	}
	fValue1 = kData.fValue1;
	fValue2 = kData.fValue2;
	return 1;
}
