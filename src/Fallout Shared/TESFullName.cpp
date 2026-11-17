#include "Fallout Shared/TESFullName.h"

#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

TESFullName::TESFullName() : BaseFormComponent(), cFullName() {}

void TESFullName::InitializeDataComponent() {
	(&cFullName)->Set(nullptr, (uint32_t)0);
}

void TESFullName::ClearDataComponent() {}

void TESFullName::CopyComponent(BaseFormComponent* apCopyFC) {
	const char* fullName_0;
	TESFullName* v4 = dynamic_cast<TESFullName*>(apCopyFC);
	if (v4) {
		fullName_0 = v4->TESFullName::GetFullName();
		(&cFullName)->Set(fullName_0, (uint32_t)0);
	}
}

bool TESFullName::CompareComponent(BaseFormComponent* apCompareFC) {
	TESFullName* pCompare = dynamic_cast<TESFullName*>(apCompareFC);
	if (!pCompare) {
		return 1;
	}
	if (!GetFullNameLength() && !pCompare->GetFullNameLength()) {
		return 0;
	}
	uint32_t FullNameLength = GetFullNameLength();
	if (FullNameLength != pCompare->GetFullNameLength()) {
		return 1;
	}
	return cFullName.StrCmp(pCompare->GetFullName(), 0) != 0;
}

void TESFullName::Save() {
	if (GetFullNameLength()) {
		TESForm::AddChunkArray(FULL_ID, GetFullName(), GetFullNameLength() + 1);
	}
}

uint32_t TESFullName::GetFullNameLength() {
	return cFullName.GetLength();
}

// Reads the FULL chunk of apFile into apFullName. An empty chunk clears the name.
void TESFullName::LoadFullNameChunk(TESFullName* apFullName, TESFile* apFile) {
	if (apFile && apFullName) {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == FULL_ID) {
			if (apFile->GetChunkSize()) {
				char* pBuffer = static_cast<char*>(__builtin_alloca(apFile->GetChunkSize()));
				apFile->GetChunkData(pBuffer, 0);
				apFullName->SetFullName(pBuffer);
			} else {
				apFullName->SetFullName(nullptr);
			}
		}
	}
}
