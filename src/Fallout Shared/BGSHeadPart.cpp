#include "Fallout Shared/BGSHeadPart.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "SDK/MSVC/rtti.h"

BGSHeadPart::BGSHeadPart() : TESForm(), TESFullName(), TESModelTextureSwap(), ExtraParts() {
	SetFormType(9u);
	BGSHeadPart::InitializeData();
}

BGSHeadPart::~BGSHeadPart() {
	BGSHeadPart::ClearData();
}

void BGSHeadPart::InitializeData() {
	cFlags = (uint8_t)0;
}

void BGSHeadPart::ClearData() {
	BSSimpleList<BGSHeadPart*>* pList = GetExtraPartList();
	pList->RemoveAll();
}

// Reads the head part's chunks. MODS holds the texture swaps, which the exe unpacks with the record's form version;
// HNAM names an extra part.
bool BGSHeadPart::Load(TESFile* apFile) {
	if (apFile->GetTESForm() != HDPT_ID) {
		return false;
	}
	this->LoadForm(apFile);
	do {
		uint32_t eChunk = apFile->GetTESChunk();
		if (eChunk == NO_CHUNK) {
			break;
		}
		switch (eChunk) {
		case DATA_ID:
			apFile->GetChunkData(&cFlags, 1u);
			break;
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200u);
			static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
			break;
		}
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case HNAM_ID: {
			// The chunk holds a form ID, which the exe passes on as the raw pointer value.
			uint32_t uiFormID = 0;
			apFile->GetChunkData<uint32_t>(uiFormID);
			this->AddExtraPart(reinterpret_cast<BGSHeadPart*>(static_cast<uintptr_t>(uiFormID)));
			break;
		}
		case MODL_ID:
		case MODD_ID:
		case MOSD_ID:
		case MODT_ID:
			TESModel::LoadModelChunk(this, apFile);
			break;
		case MODS_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
			apFile->GetChunkData(pBuffer, uiChunkSize);
			uint16_t usVersion = apFile->GetFormVersion();
			static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usVersion);
			break;
		}
		case OBND_ID:
			static_cast<TESForm*>(this)->LoadObjectBound(apFile);
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void BGSHeadPart::Save() {
	StartForm();
	TESFullName::Save();
	TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
	TESForm::AddChunk(DATA_ID, this->BGSHeadPart::cFlags);
	BSSimpleList<BGSHeadPart*>* plist = &ExtraParts;
	while (plist && !plist->IsEmpty()) {
		BGSHeadPart* ppart = plist->m_item;
		plist = plist->m_pkNext;
		TESForm::AddChunk(HNAM_ID, ppart->iFormID);
	}
	CloseForm();
}

// Before InitItem the extra parts list holds the form IDs the file stored.
void BGSHeadPart::InitItem() {
	if (!GetInitialized()) {
		TESModelTextureSwap::InitItem(this);
		BSSimpleList<BGSHeadPart*>* plist = &ExtraParts;
		BSSimpleList<BGSHeadPart*>* plastlist = 0;
		while (plist && !plist->IsEmpty()) {
			uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(itempint, File);
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
			BGSHeadPart* ppart = dynamic_cast<BGSHeadPart*>(FormByNumericID);
			if (ppart) {
				plist->SetItem(ppart);
				plastlist = plist;
				plist = plist->m_pkNext;
			} else {
				int32_t CastingType = iFormID;
				const char* v3 = GetFormEditorID();
				Warning("Unable to find extra part (%08X) for head part '%s' (%08X).", itempint, v3, CastingType);
				if (plastlist) {
					BGSHeadPart* pformid = reinterpret_cast<BGSHeadPart*>(static_cast<uintptr_t>(itempint));
					plastlist->Remove(pformid);
					plist = plastlist->m_pkNext;
				} else {
					plist->RemoveHead();
				}
			}
		}
		SetInitialized(1);
	}
}

void BGSHeadPart::Copy(TESForm* apCopy) {
	BGSHeadPart* pcopy = dynamic_cast<BGSHeadPart*>(apCopy);
	if (pcopy) {
		CopyAllComponents(apCopy);
		BGSHeadPart::cFlags = pcopy->BGSHeadPart::cFlags;
		(&ExtraParts)->RemoveAll();
		BSSimpleList<BGSHeadPart*>* plist = &pcopy->ExtraParts;
		while (plist && !plist->IsEmpty()) {
			BGSHeadPart* ppart = plist->m_item;
			plist = plist->m_pkNext;
			AddExtraPart(ppart);
		}
	}
}

bool BGSHeadPart::Compare(TESForm* apCompare) {
	BGSHeadPart* pcomp = dynamic_cast<BGSHeadPart*>(apCompare);
	if (!pcomp) {
		return 1;
	}
	if (CompareAllComponents(pcomp)) {
		return 1;
	}
	if (pcomp->BGSHeadPart::cFlags != BGSHeadPart::cFlags) {
		return 1;
	}
	BSSimpleList<BGSHeadPart*>* plist = &pcomp->ExtraParts;
	while (plist && !plist->IsEmpty()) {
		BGSHeadPart* ppart = plist->m_item;
		plist = plist->m_pkNext;
		if (!(&ExtraParts)->IsInList(ppart)) {
			return 1;
		}
	}
	return 0;
}

BSSimpleList<BGSHeadPart*>* BGSHeadPart::GetExtraPartList() {
	return &ExtraParts;
}

void BGSHeadPart::AddExtraPart(BGSHeadPart* apPart) {
	(&ExtraParts)->AddHead(apPart);
}

bool BGSHeadPart::IsPlayable() {
	return ((cFlags & 1) != 0);
}
