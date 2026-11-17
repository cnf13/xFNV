#pragma once

#include "Fallout Shared/BaseFormComponent.h"

// The number of rounds in a clip.
class BGSClipRoundsForm : public BaseFormComponent {
public:
	BGSClipRoundsForm();
	~BGSClipRoundsForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The rounds per clip.
	int32_t GetFormClipRounds();

	uint8_t cClipRounds;
	uint8_t padding05[3];
};
