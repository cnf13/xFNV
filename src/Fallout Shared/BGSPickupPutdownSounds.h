#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class TESForm;
class TESSound;

// The sounds played when an item is picked up and put down.
class BGSPickupPutdownSounds : public BaseFormComponent {
public:
	BGSPickupPutdownSounds();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	void InitItem(TESForm* apOwner);
	void Save();
	// Stores pPickupSound; the exe's shared one-field setter (thiscall, ret 4, 0x6ECD40).
	void SetPickupSound(TESSound* apSound);
	// Stores pPutdownSound; the exe's shared setter of the field at +8 (thiscall, ret 4,
	// 0x403550, the same body as TESForm::SetFormFlags).
	void SetPutdownSound(TESSound* apSound);
	// pPutdownSound (+0x8); the exe's shared one-field getter 0x44DDC0 (TESForm::GetFormFlags
	// owns the fold).
	TESSound* GetPutdownSound();
	// pPickupSound (+0x4); the exe's shared one-field getter 0x726070 (TESHealthForm::
	// GetFormHealth owns the fold).
	TESSound* GetPickupSound();

	TESSound* pPickupSound;
	TESSound* pPutdownSound;
};
inline constexpr fnvd::Vtable BGSPickupPutdownSounds_vftable{};
