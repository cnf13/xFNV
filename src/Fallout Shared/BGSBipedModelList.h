#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class BGSListForm;
class TESForm;

// The list of biped models a form can use.
class BGSBipedModelList : public BaseFormComponent {
public:
	BGSBipedModelList();
	~BGSBipedModelList();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The list form of biped models.
	BGSListForm* GetBipedModelList();
	void Save();
	// Stores pListForm.
	void SetBipedModelList(BGSListForm* apList);
	void InitItem(TESForm* apOwner);

	// Returns the form's biped model list component, or null for forms without one.
	static BGSBipedModelList* GetFormAsBipedModelList(TESForm* apForm);

	BGSListForm* pListForm;
};
