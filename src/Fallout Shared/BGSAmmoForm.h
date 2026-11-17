#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class BGSListForm;
class TESAmmo;
class TESForm;

// The ammunition form a weapon fires (an ammo item or a list of them).
class BGSAmmoForm : public BaseFormComponent {
public:
	BGSAmmoForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	TESAmmo* GetAmmoHelper();
	// The ammo form when it is an ammo item (AMMO_ID), else null.
	TESAmmo* GetAmmoForm();
	// The ammo form when it is a form list (FLST_ID), else null.
	BGSListForm* GetAmmoFormList();
	// True when the ammo is the Rock-It Launcher's.
	bool IsRockItLauncher();
	void Save();
	void InitItem(TESForm* apOwner);

	TESForm* pFormAmmo;
};
