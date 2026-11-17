#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

enum CHUNK_ID : uint32_t;
class TESFile;
class TESForm;

// The description text of a form, stored as an offset into its file.
class TESDescription : public BaseFormComponent {
public:
	TESDescription();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The description text of apForm, read from the aeChunk record of its file.
	virtual const char* GetDescription(TESForm* apForm, CHUNK_ID aeChunk);

	// Loads the description of apForm from its file.
	void LoadDescription(TESForm* apForm, CHUNK_ID aeChunk);
	// Stores the description text read from the file (thiscall, ret 4, 0x483080); the exe returns what
	// BSStringT<char>::Set leaves in eax.
	bool SetLastDescriptionString(char* apText);
	// Stores the description's file offset; the exe calls the shared setter of the field at
	// +4 (thiscall, ret 4, 0x6ECD40).
	void SetDescriptionFileOffset(uint32_t alOffset);
	void Save();
	uint32_t GetDescriptionLength();

	// Saves the description of apDesc as chunk aiChunk.
	static void SaveDescriptionData(TESDescription* apDesc, CHUNK_ID aiChunk);
	// Points apDesc at the current record of apFile.
	static void LoadDescriptionData(TESDescription* apDesc, TESFile* apFile);

	uint32_t lFileOffset;

	// A description string shared by the class's functions.
	static BSStringT<char> strCurrentDescription;
	static inline TESDescription* pCurrentDescription;
};
inline constexpr fnvd::Vtable TESDescription_vftable{};
