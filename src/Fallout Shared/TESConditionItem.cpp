#include "Fallout Shared/TESConditionItem.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESParameters.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

namespace {
// The condition data chunk's id: the chunk's four characters ('C', 'T', 'D', 'A')
// read as a little-endian dword.
constexpr uint32_t kCTDAChunkID = 0x41445443;

// Bits of ConditionItemData::ucType. The two run-on flags are the old format's way of
// naming the reference the condition runs on; loading replaces them with the run-on field.
constexpr uint8_t kTypeFlagOr = 0x1;
constexpr uint8_t kTypeFlagRunOnTarget = 0x2;
constexpr uint8_t kTypeFlagGlobalComparisonValue = 0x4;
constexpr uint8_t kTypeFlagRunOnReference = 0x8;
constexpr uint8_t kTypeComparisonMask = 0xE0;
constexpr uint8_t kTypeFunctionMask = 0x1F;
constexpr int32_t kTypeComparisonShift = 5;
} // namespace

void TESConditionItem::Load(TESFile* apFile) {
	if (apFile != nullptr && apFile->GetTESChunk() == kCTDAChunkID) {
		apFile->GetChunkData(&Data, sizeof(Data));
#if FNVD_64BIT
		// The file's record is 0x1C bytes (or a shorter older one); the comparison value, both function
		// parameters and the reference are 4 bytes there, a form ID or a number.
		TESFile::WidenFormIDRecords(&Data, 1, apFile->GetChunkSize() < 0x1C ? apFile->GetChunkSize() : 0x1C,
			sizeof(Data),
			{{4, offsetof(ConditionItemData, uComparisonValue)},
				{0xC, offsetof(ConditionItemData, kFunctionData.uParam[0])},
				{0x10, offsetof(ConditionItemData, kFunctionData.uParam[1])},
				{0x18, offsetof(ConditionItemData, pReference)}});
#endif
		if (apFile->QEndian())
			Data.Endian();
	}
	if ((Data.ucType & kTypeFlagRunOnTarget) != 0) {
		Data.uiRunOn = CIO_TARGET;
		Data.ucType = static_cast<uint8_t>(Data.ucType & ~kTypeFlagRunOnTarget);
	}
	if ((Data.ucType & kTypeFlagRunOnReference) != 0) {
		Data.uiRunOn = CIO_REF;
		Data.ucType = static_cast<uint8_t>(Data.ucType & ~kTypeFlagRunOnReference);
	}
}

// KEEP, not registered (0x681840): the exe runs its loop to the function's parameter count, not to the two operands
// uParam holds, so for a count above 4 it reads and writes its own locals at [ebp-8] and [ebp-4] (the FunctionData
// pointer and the count itself) through uParam[4] and uParam[5]; src's locals are laid out otherwise.
void TESConditionItem::Save() {
	// The chunk is written from a copy whose form pointers are replaced by their form IDs.
	ConditionItemData kData;
	ourMemcpy(&kData, &Data, sizeof(kData));
	FunctionData* pFuncData = &Data.kFunctionData;
	int32_t iParamCount = static_cast<int32_t>(TESParameters::GetParamCount(pFuncData));
	for (int32_t i = 0; i < iParamCount; ++i) {
		if (TESParameters::GetParamIsTESForm(pFuncData, static_cast<uint32_t>(i))) {
			TESForm* pForm = static_cast<TESForm*>(kData.kFunctionData.uParam[i].pPointer);
			if (pForm) {
				kData.kFunctionData.uParam[i].uiNumber = pForm->iFormID;
			}
		}
	}
	if (GetValueIsGlobal() && Data.uComparisonValue.pGlobal) {
		kData.uComparisonValue.pGlobal =
			reinterpret_cast<TESGlobal*>(static_cast<uintptr_t>(Data.uComparisonValue.pGlobal->iFormID));
	}
	if (Data.pReference) {
		kData.pReference = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(Data.pReference->iFormID));
	}
	if (TESForm::QEndianSwapOnSave()) {
		kData.Endian();
	}
#if FNVD_64BIT
	// The file's record is Load's 0x1C bytes: the comparison value, both function parameters and the reference are
	// 4 bytes there, a form ID or a number.
	TESFile::NarrowFormIDRecords(&kData, 1, 0x1C, sizeof(kData),
		{{4, offsetof(ConditionItemData, uComparisonValue)},
			{0xC, offsetof(ConditionItemData, kFunctionData.uParam[0])},
			{0x10, offsetof(ConditionItemData, kFunctionData.uParam[1])},
			{0x18, offsetof(ConditionItemData, pReference)}});
	TESForm::__AddChunkData(CIDA_ID, &kData, 0x1C);
#else
	TESForm::__AddChunkData(CIDA_ID, &kData, sizeof(kData));
#endif
	if (TESForm::QEndianSwapOnSave()) {
		kData.Endian();
	}
}

void FunctionData::Endian() {
	ENDIAN(usFunction, false);
	ENDIAN(uParam[0].uiNumber, false);
	ENDIAN(uParam[1].uiNumber, false);
}

void ConditionItemData::Endian() {
	ENDIAN(uComparisonValue.fValue, false);
	ENDIAN(pReference, false);
	ENDIAN(uiRunOn, false);
	kFunctionData.Endian();
}

TESConditionItem::TESConditionItem() {
	ourMemset(&Data, 0, sizeof(Data));
	Data.pReference = nullptr;
	SetDefaultDataForFunction(FUNCTION_GET_IS_ID);
}

SCRIPT_OUTPUT TESConditionItem::GetConditionFunction() {
	return static_cast<SCRIPT_OUTPUT>(Data.kFunctionData.usFunction);
}

bool TESConditionItem::GetOr() {
	return (static_cast<int8_t>(Data.ucType) & kTypeFlagOr) != 0;
}

bool TESConditionItem::GetValueIsGlobal() {
	return (static_cast<int8_t>(Data.ucType) & kTypeFlagGlobalComparisonValue) != 0;
}

ENUM_COMPARISON_CONDITION TESConditionItem::GetComparisonCondition() {
	return static_cast<ENUM_COMPARISON_CONDITION>((static_cast<int8_t>(Data.ucType) & kTypeComparisonMask) >>
		kTypeComparisonShift);
}

void TESConditionItem::SetComparisonValueFloat(float afValue) {
	if (!GetValueIsGlobal())
		Data.uComparisonValue.fValue = afValue;
}

void TESConditionItem::SetConditionFunction(uint16_t aiFunc) {
	ourMemset(&Data.kFunctionData, 0, sizeof(Data.kFunctionData));
	Data.kFunctionData.usFunction = aiFunc;
}

void TESConditionItem::SetComparisonCondition(ENUM_COMPARISON_CONDITION aiCond) {
	int32_t condition = aiCond;
	if (condition < CC_EQUAL || condition >= CC_COUNT)
		condition = CC_EQUAL;
	// Only the operator bits of the type byte are replaced.
	Data.ucType = static_cast<uint8_t>((Data.ucType & kTypeFunctionMask) | (condition << kTypeComparisonShift));
}

void TESConditionItem::SetDefaultDataForFunction(SCRIPT_OUTPUT aiFunc) {
	// The function's index in the function table; aiFunc may be given as an index or as an opcode, and is made
	// an opcode here.
	int32_t iFunc = aiFunc;
	if (aiFunc >= START_OF_FUNCTION_SECTION && aiFunc < END_OF_FUNCTION_SECTION) {
		iFunc = aiFunc - START_OF_FUNCTION_SECTION;
	} else if (aiFunc >= 0 && aiFunc < FUNCTION_COUNT) {
		aiFunc = static_cast<SCRIPT_OUTPUT>(aiFunc + START_OF_FUNCTION_SECTION);
	}
	if (iFunc < 0 || iFunc >= FUNCTION_COUNT) {
		return;
	}
	// The exe pushes the index's whole dword; the callee stores its low word.
	SetConditionFunction(static_cast<uint16_t>(iFunc));
	switch (aiFunc) {
	case FUNCTION_GET_DISTANCE:
		SetComparisonCondition(CC_LESS_THAN);
		SetComparisonValueFloat(100.0f);
		break;
	case FUNCTION_IS_RAINING:
	case FUNCTION_GET_IS_CREATURE:
	case FUNCTION_GET_IN_CELL:
	case FUNCTION_GET_IS_CLASS:
	case FUNCTION_GET_IS_RACE:
	case FUNCTION_GET_IS_SEX:
	case FUNCTION_GET_IN_FACTION:
	case FUNCTION_GET_IS_ID:
	case FUNCTION_GET_ALERT:
	case FUNCTION_IS_WEAPON_OUT:
	case FUNCTION_IS_TORCH_OUT:
	case FUNCTION_IS_SHIELD_OUT:
	case FUNCTION_IS_FACING_UP:
	case FUNCTION_IS_WAITING:
	case FUNCTION_IS_IDLE_PLAYING:
	case FUNCTION_GET_IN_WORLDSPACE:
		SetComparisonCondition(CC_EQUAL);
		SetComparisonValueFloat(1.0f);
		break;
	case FUNCTION_GET_KNOCKED_STATE:
		SetComparisonCondition(CC_NOT_EQUAL);
		SetComparisonValueFloat(0.0f);
		break;
	case FUNCTION_GET_IS_USED_ITEM_TYPE:
		Data.kFunctionData.uParam[0].uiNumber = 40;
		SetComparisonCondition(CC_EQUAL);
		SetComparisonValueFloat(1.0f);
		break;
	default:
		break;
	}
}

float TESConditionItem::GetComparisonValue() {
	float v3 = (float)0.0;
	if (GetValueIsGlobal()) {
		if (Data.uComparisonValue.pGlobal) {
			return (float)Data.uComparisonValue.pGlobal->TESGlobal::GetValue();
		}
	} else {
		return Data.uComparisonValue.fValue;
	}
	return v3;
}

void TESConditionItem::Copy(TESConditionItem* apCopy, TESForm* apOwner) {
	TESConditionItem* v3 = apCopy;
	ourMemcpy(this, v3, sizeof(TESConditionItem));
}

// Resolves the reference and the comparison global from their file-local form IDs, and warns about an item whose
// function is no condition function. The exe's cleanup-only C++ EH frame is left out.
void TESConditionItem::InitItem(TESForm* apOwner) {
	FNVD_INLINED_LOCAL(BSStringT<char>, kGlobalInfoText, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStringT<char>, kInfoText, std::nullopt, std::nullopt);
	TESParameters::InitItem(apOwner, &Data.kFunctionData);
	uint32_t uiRefID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pReference));
	if (uiRefID) {
		TESForm::AddCompileIndex(uiRefID, apOwner->GetFile(-1));
		TESForm* pRefForm = TESForm::GetFormByNumericID(uiRefID);
		Data.pReference = dynamic_cast<TESObjectREFR*>(pRefForm);
		if (!Data.pReference) {
			if (apOwner) {
				uint32_t uiOwnerID = apOwner->iFormID;
				const char* pOwnerEditorID = apOwner->GetFormEditorID();
				const char* pOwnerType = apOwner->GetFormTypeString();
				Warning("MASTERFILE: Could not find reference (%08X) for condition item on owner form %s '%s' (%08X).",
					uiRefID, pOwnerType, pOwnerEditorID, uiOwnerID);
			} else {
				Warning("MASTERFILE: Could not find reference (%08X) for condition item on UNKNOWN owner.", uiRefID);
			}
		}
	}
	if (!scriptFunctions[GetConditionFunction()].pConditionFunction) {
		char cFormText[1040];
		if (apOwner->GetFormType() == INFO_ID) {
			TESTopicInfo* pInfo = dynamic_cast<TESTopicInfo*>(apOwner);
			TESTopic* pTopic = TESTopic::GetOwnerTopic(pInfo);
			TESQuest* pQuest = nullptr;
			if (pTopic) {
				pQuest = pTopic->GetOwnerQuest(pInfo);
			}
			::new (&kInfoText) BSStringT<char>;
			pInfo->GetInfoDisplayText(kInfoText, false);
			uint32_t uiTopicID = pTopic ? pTopic->iFormID : 0;
			const char* pTopicEditorID = pTopic ? pTopic->GetFormEditorID() : "";
			uint32_t uiQuestID = pQuest ? pQuest->iFormID : 0;
			const char* pQuestEditorID = pQuest ? pQuest->GetEditorName() : "";
			char* pText = kInfoText.pString;
			std::sprintf(cFormText, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", pInfo->iFormID, pText,
				pQuestEditorID, uiQuestID, pTopicEditorID, uiTopicID);
			kInfoText.~BSStringT<char>();
		} else {
			uint32_t uiOwnerID = apOwner->iFormID;
			const char* pOwnerEditorID = apOwner->GetFormEditorID();
			std::sprintf(cFormText, "Form %s %s (%08X)", apOwner->GetFormTypeString(), pOwnerEditorID, uiOwnerID);
		}
		const char* pFunctionName = scriptFunctions[GetConditionFunction()].pFunctionName;
		const char* pFilename = apOwner->GetFile(-1)->GetFilename();
		Warning("MASTERFILE: %s in file %s contains bad condition item data. Function \"%s\" is not a condition "
				"function.",
			cFormText, pFilename, pFunctionName);
	}
	if (GetValueIsGlobal() && Data.uComparisonValue.pGlobal) {
		uint32_t uiGlobalID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.uComparisonValue.pGlobal));
		TESForm::AddCompileIndex(uiGlobalID, apOwner->GetFile(-1));
		TESForm* pGlobalForm = TESForm::GetFormByNumericID(uiGlobalID);
		Data.uComparisonValue.pGlobal = dynamic_cast<TESGlobal*>(pGlobalForm);
		if (!Data.uComparisonValue.pGlobal) {
			char cFormText[1040];
			if (apOwner->GetFormType() == INFO_ID) {
				TESTopicInfo* pInfo = dynamic_cast<TESTopicInfo*>(apOwner);
				TESTopic* pTopic = TESTopic::GetOwnerTopic(pInfo);
				TESQuest* pQuest = nullptr;
				if (pTopic) {
					pQuest = pTopic->GetOwnerQuest(pInfo);
				}
				::new (&kGlobalInfoText) BSStringT<char>;
				pInfo->GetInfoDisplayText(kGlobalInfoText, false);
				uint32_t uiTopicID = pTopic ? pTopic->iFormID : 0;
				const char* pTopicEditorID = pTopic ? pTopic->GetFormEditorID() : "";
				uint32_t uiQuestID = pQuest ? pQuest->iFormID : 0;
				const char* pQuestEditorID = pQuest ? pQuest->GetEditorName() : "";
				char* pText = kGlobalInfoText.pString;
				_snprintf(cFormText, 1040, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", pInfo->iFormID,
					pText, pQuestEditorID, uiQuestID, pTopicEditorID, uiTopicID);
				kGlobalInfoText.~BSStringT<char>();
			} else {
				uint32_t uiOwnerID = apOwner->iFormID;
				const char* pOwnerEditorID = apOwner->GetFormEditorID();
				_snprintf(cFormText, 1040, "Form %s %s (%08X)", apOwner->GetFormTypeString(), pOwnerEditorID,
					uiOwnerID);
			}
			Warning("MASTERFILE: Unable to find Value TESGlobal (%08X) in TESConditionItem Init for form:\n\n%s.",
				uiGlobalID, cFormText);
		}
	}
}

bool TESConditionItem::Compare(TESConditionItem* apCompare) {
	TESConditionItem* v2 = apCompare;
	return std::memcmp(v2, this, sizeof(TESConditionItem)) != 0;
}

bool TESConditionItem::IsTrue(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef, bool& abDispositionFailure) {
	bool bRetVal = false;
	double fVal = 0.0;
	TESObjectREFR* pActionRef = apActionRef;
	TESObjectREFR* pTargetRef = apTargetRef;
	abDispositionFailure = false;
	switch (Data.uiRunOn) {
	case 1u:
		pActionRef = apTargetRef;
		pTargetRef = apActionRef;
		break;
	case 2u:
		pActionRef = Data.pReference;
		break;
	case 3u:
		// Run on the action reference's combat target.
		pActionRef = nullptr;
		pTargetRef = apActionRef;
		if (apActionRef && apActionRef->IsActor()) {
			pActionRef = static_cast<Actor*>(apActionRef)->GetCombatTarget();
		}
		break;
	case 4u:
		pActionRef = nullptr;
		pTargetRef = nullptr;
		if (apActionRef) {
			pActionRef = apActionRef->GetLinkedRef();
		}
		break;
	default:
		break;
	}
	FunctionData* pFuncData = &Data.kFunctionData;
	void* pParam1 = pFuncData->uParam[0].pPointer;
	if (!pParam1 && TESParameters::GetParamIsReference(pFuncData, 0)) {
		pParam1 = pTargetRef;
	}
	if (scriptFunctions[pFuncData->usFunction].pConditionFunction &&
		(!scriptFunctions[pFuncData->usFunction].bReferenceFunction || pActionRef) &&
		scriptFunctions[pFuncData->usFunction].pConditionFunction(pActionRef, pParam1, pFuncData->uParam[1].pPointer,
			fVal)) {
		float fCompVal;
		if (Data.uComparisonValue.pGlobal && GetValueIsGlobal()) {
			fCompVal = Data.uComparisonValue.pGlobal->GetValue();
		} else {
			fCompVal = Data.uComparisonValue.fValue;
		}
		float fValue = static_cast<float>(fVal);
		bRetVal = TESCondition::CheckValue(GetComparisonCondition(), fValue, fCompVal);
		if (!bRetVal) {
			// The exe compares the function's opcode, not its index.
			SCRIPT_OUTPUT eFunction = static_cast<SCRIPT_OUTPUT>(GetConditionFunction() + START_OF_FUNCTION_SECTION);
			if (eFunction == FUNCTION_GET_DISPOSITION) {
				if (GetComparisonCondition() == CC_GREATER_THAN || GetComparisonCondition() == CC_GREATER_THAN_EQUAL) {
					abDispositionFailure = true;
				}
			} else if (eFunction == FUNCTION_HASFRIENDDISPOSITION) {
				abDispositionFailure = true;
			}
		}
	}
	return bRetVal;
}

void TESConditionItem::operator delete(void* apMemory) {
	ourFree(apMemory);
}
