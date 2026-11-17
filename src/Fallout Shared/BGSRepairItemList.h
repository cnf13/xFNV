#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class BGSListForm;
class TESForm;

// The list of items that can repair a form.
class BGSRepairItemList : public BaseFormComponent {
public:
	BGSRepairItemList();
	~BGSRepairItemList();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	void Save();
	void InitItem(TESForm* apOwner);
	bool HasRepairItem(TESForm* apForm);

	static BGSRepairItemList* GetFormAsRepairItemList(TESForm* apForm);
	static bool CanItemRepairForm(TESForm* apRepairItem, TESForm* apForm);

	BGSListForm* pListForm;
};
inline constexpr fnvd::Vtable BGSRepairItemList_vftable{};
