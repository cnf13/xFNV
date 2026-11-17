#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Reconstruction.hpp"

// The buffer one form is saved into: the form's header and the form itself. The slots GetForm, GetReference and
// GetActor are overridden in the exe.
class BGSSaveFormBuffer : public BGSSaveGameBuffer {
public:
	BGSSaveFormBuffer();
	~BGSSaveFormBuffer();

	TESForm* GetForm() override { return pForm; }
	TESObjectREFR* GetReference() override;
	Actor* GetActor() override;

	// The form's type, from Header.FormInfo.
	ENUM_FORM_ID GetFormType();

	// The form's change flags (Header.iChangeFlags), returned through a hidden result; the
	// exe's code is shared with BGSLoadFormBuffer::GetChangeFlags (thiscall, ret 4, 0x428110).
	BGSChangeFlags GetChangeFlags();
	void SetHeader(uint32_t aiFormID, BGSChangeFlags aiChangeFlags, ENUM_FORM_ID aeFormType, uint8_t acVersion);
	// thiscall, ret 4.
	void RemoveChange(int32_t aiFlags);
	uint32_t GetOverhead();
	// Header.iChangeFlags = aiChangeFlags; the exe's code is shared with
	// BGSLoadFormBuffer::SetChangeFlags (thiscall, ret 4, 0x428130).
	void SetChangeFlags(BGSChangeFlags aiChangeFlags);
	// pForm = apForm; the exe's shared one-field setter of +0x20 (thiscall, ret 4, 0x50F9C0).
	void SetForm(TESForm* apForm);
	// Writes the header and the buffered data to apFile.
	void Save(BGSSaveLoadFile* apFile);
	// Inferred type: a2: NiMatrix3* from 0x9c516c (&Projectile+0x94 (this of Projectile::SaveGame))
	void SaveMatrixAsQuaternion(NiMatrix3* a2, int32_t a3, int32_t a4);

	// IDA names the fields kHeader and pForm.
	BGSSaveLoadFormHeader Header;
	TESForm* pForm;
};
inline constexpr fnvd::Vtable BGSSaveFormBuffer_vftable{};
