#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class TESForm;
class TESImageSpaceModifier;

// The image space modifier a form applies.
class TESImageSpaceModifiableForm : public BaseFormComponent {
public:
	TESImageSpaceModifiableForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Resolves the saved image space form ID of apOwner.
	void InitItem(TESForm* apOwner);
	// Sets pFormImageSpaceModifying; the exe's shared one-field setter (thiscall, ret 4,
	// 0x6ECD40; TESScriptableForm::SetFormScript calls the same address).
	void SetFormImageSpaceModifying(TESImageSpaceModifier* apMod);
	void Save();

	TESImageSpaceModifier* pFormImageSpaceModifying;
};

// The image space modifiers a form applies, N of them (a weather keeps one per time of day).
template <uint32_t N> class TESImageSpaceModifiableCountForm : public BaseFormComponent {
public:
	TESImageSpaceModifiableCountForm() : BaseFormComponent() {
		TESImageSpaceModifiableCountForm::InitializeDataComponent();
	}

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The hybrid forwards these four to the exe's copies for N = 6, which TESImageSpaceModifiableCountForm<6>
	// registers; natively they are the generic bodies (InitItem's and Save's in TESImageSpaceModifiableForm.cpp).
	// The modifier for time of day auiIndex (thiscall, ret 4, 0x5822A0 for N = 6).
	TESImageSpaceModifier* GetFormImageSpaceModifying(uint32_t auiIndex) { return pFormImageSpaceModifying[auiIndex]; }

	// Sets the modifier for time of day auiIndex (thiscall, ret 8, 0x581FB0 for N = 6).
	void SetFormImageSpaceModifying(uint32_t auiIndex, TESImageSpaceModifier* apMod) {
		pFormImageSpaceModifying[auiIndex] = apMod;
	}

	// Resolves the modifier form IDs, read from apOwner's file (thiscall, ret 4, 0x5820A0 for
	// N = 6).
	void InitItem(TESForm* apOwner);
	// Saves the modifiers' form IDs as the owner's chunk (thiscall, ret 0, 0x582040 for N = 6).
	void Save();

	TESImageSpaceModifier* pFormImageSpaceModifying[N];
};

inline constexpr fnvd::Vtable TESImageSpaceModifiableForm_vftable{};
inline constexpr fnvd::Vtable TESImageSpaceModifiableCountForm_6_vftable{};

// Clears every modifier slot (the exe's N = 6 copy, 0x582000).
template <uint32_t N> void TESImageSpaceModifiableCountForm<N>::InitializeDataComponent() {
	for (uint32_t i = 0; i < N; ++i) {
		pFormImageSpaceModifying[i] = nullptr;
	}
}

// Copies every modifier slot of apCopyFC when it is the same component (the exe's N = 6 copy, 0x5821B0).
template <uint32_t N> void TESImageSpaceModifiableCountForm<N>::CopyComponent(BaseFormComponent* apCopyFC) {
	TESImageSpaceModifiableCountForm* pCopy = dynamic_cast<TESImageSpaceModifiableCountForm*>(apCopyFC);
	if (pCopy) {
		for (uint32_t i = 0; i < N; ++i) {
			SetFormImageSpaceModifying(i, pCopy->GetFormImageSpaceModifying(i));
		}
	}
}

// True when the components differ: apCompareFC is not this component, or a slot holds another modifier (the exe's
// N = 6 copy, 0x582220).
template <uint32_t N> bool TESImageSpaceModifiableCountForm<N>::CompareComponent(BaseFormComponent* apCompareFC) {
	TESImageSpaceModifiableCountForm* pCompare = dynamic_cast<TESImageSpaceModifiableCountForm*>(apCompareFC);
	if (!pCompare) {
		return true;
	}
	for (uint32_t i = 0; i < N; ++i) {
		if (GetFormImageSpaceModifying(i) != pCompare->GetFormImageSpaceModifying(i)) {
			return true;
		}
	}
	return false;
}
