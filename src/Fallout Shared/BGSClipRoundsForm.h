#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

// The number of rounds in a clip.
class BGSClipRoundsForm : public BaseFormComponent {
public:
	BGSClipRoundsForm();
	~BGSClipRoundsForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The rounds per clip; the exe's shared byte getter at +4, the same function as
	// TESForm::GetFormType (thiscall, ret 0, 0x401170).
	int32_t GetFormClipRounds();

	uint8_t cClipRounds;
	uint8_t padding05[3];
};
inline constexpr fnvd::Vtable BGSClipRoundsForm_vftable{};
