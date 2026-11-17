#pragma once

#include "Fallout Shared/BaseFormComponent.h"

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
	void SetPickupSound(TESSound* apSound);
	void SetPutdownSound(TESSound* apSound);
	TESSound* GetPutdownSound();
	TESSound* GetPickupSound();

	TESSound* pPickupSound;
	TESSound* pPutdownSound;
};
