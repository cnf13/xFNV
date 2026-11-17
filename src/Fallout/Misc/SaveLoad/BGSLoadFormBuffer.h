#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Reconstruction.hpp"

// The buffer one form is loaded from: the form's header, the size of its data and the form once it is found. The slots
// GetVersion, GetForm, GetReference and GetActor are overridden in the exe.
class BGSLoadFormBuffer : public BGSLoadGameBuffer {
public:
	// The bits of iFlags.
	enum LOAD_FORM_BUFFER_FLAGS : uint32_t {
		LOAD_FORM_SKIP_FORM = 0x1,
		LOAD_FORM_FORM_PENDING = 0x2,
		LOAD_FORM_REVERT_ONLY = 0x4,
		LOAD_FORM_RECONSTRUCTED_FORM = 0x8,
		LOAD_FORM_CELL_CHANGED = 0x10,
	};

	BGSLoadFormBuffer();
	~BGSLoadFormBuffer();

	uint8_t GetVersion() override;
	TESForm* GetForm() override;
	TESObjectREFR* GetReference() override;
	Actor* GetActor() override;

	// The form's change flags (Header.iChangeFlags), returned through a hidden result; the exe's code is shared with
	// BGSSaveFormBuffer::GetChangeFlags (thiscall, ret 4, 0x428110).
	BGSChangeFlags GetChangeFlags();
	// The change flags before the load (iOldChangeFlags), returned through a hidden result.
	BGSChangeFlags GetOldChangeFlags();
	// Stores the form the buffer loads, or null.
	void SetForm(TESForm* apForm);
	// Sets the field GetOldChangeFlags returns.
	void SetOldChangeFlags(BGSChangeFlags aiChangeFlags);
	void SetReconstructedForm(bool abValue);
	bool GetReconstructedForm();
	bool GetRevertOnly();
	void SetRevertOnly(bool abValue);
	bool GetCellChanged();
	void SetFormPending(bool abValue);
	bool GetFormPending();
	void SetChangeFlags(BGSChangeFlags aiChangeFlags);
	void ClearBuffer();
	ENUM_FORM_ID GetFormType();
	// Reads the form's iSize bytes of data from apFile into the buffer; the bytes read, or -1 on a short read.
	uint32_t LoadDataFromFile(BGSSaveLoadFile* apFile);
	// Seeks apFile past the rest of the form's data when aiAlreadyLoadedAmount bytes of it were read.
	void SkipData(BGSSaveLoadFile* apFile, uint32_t aiAlreadyLoadedAmount);
	bool GetSkipForm();
	// Sets or clears LOAD_FORM_SKIP_FORM.
	void SetSkipForm(bool abValue);
	// IDA name; not in the PDB. Sets or clears LOAD_FORM_CELL_CHANGED (the 0x10 bit of the
	// flags at +0x28; thiscall, ret 4, 0x84F880).
	void SetCellChanged(bool abValue);
	// iFormID, through the exe's shared one-field getter (thiscall, ret 0, 0x44EDB0).
	uint32_t GetFormID();
	// Reads the form's header from apFile.
	void LoadHeader(BGSSaveLoadFile* apFile);

	static void operator delete(void* apMemory);

	// IDA names the fields uiFormID, kHeader, bufferSize, uiLoadFormFlags and pCurrentFormChange.
	uint32_t iFormID;
	BGSSaveLoadFormHeader Header;
	uint32_t iSize;
	TESForm* pForm;
	uint32_t iFlags;
	BGSChangeFlags iOldChangeFlags;
};
inline constexpr fnvd::Vtable BGSLoadFormBuffer_vftable{};
