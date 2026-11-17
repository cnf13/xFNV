#include "Fallout Shared/TESDescription.h"

#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

BSStringT<char> TESDescription::strCurrentDescription;

TESDescription::TESDescription() : BaseFormComponent() {
	TESDescription::InitializeDataComponent();
}

void TESDescription::InitializeDataComponent() {
	(&TESDescription::strCurrentDescription)->Set(nullptr, (uint32_t)0);
	TESDescription::pCurrentDescription = 0;
	lFileOffset = (uint32_t)0;
}

void TESDescription::CopyComponent(BaseFormComponent* apCopyFC) {
	TESDescription* pCopy = dynamic_cast<TESDescription*>(apCopyFC);
	if (pCopy) {
		TESDescription::strCurrentDescription.Set(pCopy->GetDescription(0, DESC_ID), 0u);
		lFileOffset = pCopy->lFileOffset;
	}
}

bool TESDescription::CompareComponent(BaseFormComponent* apCompareFC) {
	TESDescription* v6 = dynamic_cast<TESDescription*>(apCompareFC);
	if (!v6) {
		return (bool)1;
	}
	if ((!GetDescriptionLength()) && (!v6->GetDescriptionLength())) {
		return (bool)0;
	}
	uint32_t v3 = GetDescriptionLength();
	if (v3 != v6->GetDescriptionLength()) {
		return (bool)1;
	}
	const char* v4 = v6->GetDescription(nullptr, static_cast<CHUNK_ID>(0x43534544u));
	return ((&TESDescription::strCurrentDescription)->StrCmp(v4, 0) != 0);
}

const char* TESDescription::GetDescription(TESForm* apForm, CHUNK_ID aeChunk) {
	TESForm* v6;
	if (!pDataHandler->GetLoadingFiles()) {
		v6 = apForm;
		if (!apForm) {
			v6 = dynamic_cast<TESForm*>(this);
		}
		LoadDescription(v6, aeChunk);
	}
	if ((&TESDescription::strCurrentDescription)->pString) {
		return (&TESDescription::strCurrentDescription)->pString;
	} else {
		return "";
	}
}

// Walks the file the form came from for the aeChunk record and stores its text as the current description. An empty
// chunk leaves the buffer unwritten (malformed data), so one extra byte is reserved and terminated.
void TESDescription::LoadDescription(TESForm* apForm, CHUNK_ID aeChunk) {
	if (!apForm) {
		return;
	}
	// One description string is shared by the class: reuse it while it belongs to this component and holds text.
	if (TESDescription::pCurrentDescription == this && (&TESDescription::strCurrentDescription)->GetLength()) {
		return;
	}
	TESDescription::pCurrentDescription = this;
	(&TESDescription::strCurrentDescription)->Set(nullptr, 0);
	TESFile* pFile = apForm->GetFile(-1);
	if (!pFile || !lFileOffset) {
		return;
	}
	pFile = pFile->GetThreadSafeFile();
	if (!pFile->SetOffset(lFileOffset)) {
		return;
	}
	// The record the offset points at has to be the form's own type.
	if ((uint8_t)pFile->GetTESForm() != formEnumString[apForm->GetFormType()].cFormID) {
		return;
	}
	bool bFound = false;
	do {
		uint32_t uiChunk = pFile->GetTESChunk();
		if (uiChunk == (uint32_t)aeChunk) {
			// A 16-byte aligned stack buffer of the chunk's size, read whole.
			uint32_t uiChunkSize = pFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// An empty chunk leaves the buffer unwritten, so it is terminated first.
			pBuffer[0] = '\0';
			pFile->GetChunkData(pBuffer, 0);
			SetLastDescriptionString(pBuffer);
			bFound = true;
		}
	} while (!bFound && pFile->NextChunk());
}

bool TESDescription::SetLastDescriptionString(char* apText) {
	return TESDescription::strCurrentDescription.Set(apText, 0);
}

void TESDescription::SetDescriptionFileOffset(uint32_t alOffset) {
	lFileOffset = alOffset;
}

void TESDescription::Save() {
	TESDescription::SaveDescriptionData(this, static_cast<CHUNK_ID>(1129530692));
}

uint32_t TESDescription::GetDescriptionLength() {
	return (&TESDescription::strCurrentDescription)->GetLength();
}

// Writes the description as chunk aiChunk: its text and its length plus the terminator, or a single zero byte when the
// description is empty. The length is read twice, as the game does.
void TESDescription::SaveDescriptionData(TESDescription* apDesc, CHUNK_ID aiChunk) {
	if (apDesc) {
		if (apDesc->GetDescriptionLength()) {
			uint32_t uiLength = apDesc->GetDescriptionLength() + 1;
			const char* pDescription = apDesc->GetDescription(nullptr, static_cast<CHUNK_ID>(1129530692));
			TESForm::AddChunkArray(aiChunk, const_cast<char*>(pDescription), uiLength);
		} else {
			uint8_t ucZero = 0;
			TESForm::AddChunkArray(aiChunk, reinterpret_cast<char*>(&ucZero), 1);
		}
	}
}

void TESDescription::LoadDescriptionData(TESDescription* apDesc, TESFile* apFile) {
	if (apDesc && apFile) {
		apDesc->SetDescriptionFileOffset(apFile->GetOffset());
	}
}
