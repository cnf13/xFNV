#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

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

	// The list form of biped models (the shared one-field getter 0x726070).
	BGSListForm* GetBipedModelList();
	void Save();
	// Stores pListForm; the exe's shared one-field setter (thiscall, ret 4, 0x6ECD40).
	void SetBipedModelList(BGSListForm* apList);
	void InitItem(TESForm* apOwner);

	// cdecl; the form's biped model list component, or null for forms without one.
	static BGSBipedModelList* GetFormAsBipedModelList(TESForm* apForm);

	BGSListForm* pListForm;
};
inline constexpr fnvd::Vtable BGSBipedModelList_vftable{};
