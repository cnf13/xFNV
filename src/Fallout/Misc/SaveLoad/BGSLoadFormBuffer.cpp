#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"

BGSLoadFormBuffer::BGSLoadFormBuffer() : iFormID(0), iSize(0), pForm(nullptr), iFlags(0) {}

BGSLoadFormBuffer::~BGSLoadFormBuffer() {}

uint8_t BGSLoadFormBuffer::GetVersion() {
	return Header.cVersion;
}

TESForm* BGSLoadFormBuffer::GetForm() {
	if (!pForm && GetFormPending()) {
		pForm = TESForm::GetFormByNumericID(iFormID);
	}
	return pForm;
}

TESObjectREFR* BGSLoadFormBuffer::GetReference() {
	TESForm* v2 = GetForm();
	if (v2 && v2->IsReference()) {
		return static_cast<TESObjectREFR*>(v2);
	} else {
		return nullptr;
	}
}

Actor* BGSLoadFormBuffer::GetActor() {
	Actor* v2 = static_cast<Actor*>(GetReference());
	if (v2 && v2->IsActor()) {
		return v2;
	} else {
		return nullptr;
	}
}

void BGSLoadFormBuffer::SetReconstructedForm(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = iFlags | 8;
	} else {
		v2 = iFlags & 0xFFFFFFF7u;
	}
	iFlags = v2;
}

bool BGSLoadFormBuffer::GetRevertOnly() {
	return ((iFlags & 4) != 0);
}

void BGSLoadFormBuffer::SetRevertOnly(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = iFlags | 4;
	} else {
		v2 = iFlags & 0xFFFFFFFBu;
	}
	iFlags = v2;
}

bool BGSLoadFormBuffer::GetCellChanged() {
	return ((iFlags & 16) != 0);
}

void BGSLoadFormBuffer::SetFormPending(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = iFlags | 2;
	} else {
		v2 = iFlags & (~2u);
	}
	iFlags = v2;
}

bool BGSLoadFormBuffer::GetFormPending() {
	return ((iFlags & 2) != 0);
}

void BGSLoadFormBuffer::SetChangeFlags(BGSChangeFlags aiChangeFlags) {
	Header.iChangeFlags = aiChangeFlags;
}

void BGSLoadFormBuffer::ClearBuffer() {
	ourFree(pBuffer);
	pBuffer = nullptr;
	iBufferSize = (uint32_t)0;
	iBufferPosition = (uint32_t)0;
}

ENUM_FORM_ID BGSLoadFormBuffer::GetFormType() {
	return static_cast<ENUM_FORM_ID>((&Header.FormInfo)->GetFormType());
}

uint32_t BGSLoadFormBuffer::LoadDataFromFile(BGSSaveLoadFile* apFile) {
	uint32_t uiRead = Load(apFile, iSize);
	if (uiRead != iSize) {
		return (uint32_t)-1;
	}
	return uiRead;
}

void BGSLoadFormBuffer::SkipData(BGSSaveLoadFile* apFile, uint32_t aiAlreadyLoadedAmount) {
	if (iSize != (uint32_t)aiAlreadyLoadedAmount) {
		apFile->SeekCurrent(iSize - aiAlreadyLoadedAmount);
	}
}

void BGSLoadFormBuffer::SetCellChanged(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = iFlags | 16;
	} else {
		v2 = iFlags & 0xFFFFFFEFu;
	}
	iFlags = v2;
}

void BGSLoadFormBuffer::LoadHeader(BGSSaveLoadFile* apFile) {
	apFile->Read(&Header, 9u);
	iFormID = Header.FormIDIndex.GetNumericID();
	SIZE_TYPE esizeType = Header.FormInfo.GetSizeType();
	LoadGameSizeFromFile(apFile, iSize, esizeType);
	iSize;
}

void BGSLoadFormBuffer::operator delete(void* apMemory) {
	ourFree(apMemory);
}

BGSSaveLoadFormInfo::BGSSaveLoadFormInfo() : cData(0) {}

SIZE_TYPE BGSSaveLoadFormInfo::GetSizeType() {
	return static_cast<SIZE_TYPE>(static_cast<int32_t>(cData) >> 6);
}

BGSSaveLoadFormHeader::BGSSaveLoadFormHeader() : FormIDIndex(), iChangeFlags(), FormInfo(), cVersion(0) {}
