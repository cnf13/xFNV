#pragma once

#include "Fallout Shared/BaseFormComponent.h"

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
    void SetFormImageSpaceModifying(TESImageSpaceModifier* apMod);
    void Save();

    TESImageSpaceModifier* pFormImageSpaceModifying;
};

// The image space modifiers a form applies, N of them (a weather keeps one per time of day).
template <uint32_t N>
class TESImageSpaceModifiableCountForm : public BaseFormComponent {
public:
    TESImageSpaceModifiableCountForm() : BaseFormComponent() {
        TESImageSpaceModifiableCountForm::InitializeDataComponent();
    }

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // The modifier for time of day auiIndex.
    TESImageSpaceModifier* GetFormImageSpaceModifying(uint32_t auiIndex) {
        return pFormImageSpaceModifying[auiIndex];
    }

    // Sets the modifier for time of day auiIndex.
    void SetFormImageSpaceModifying(uint32_t auiIndex, TESImageSpaceModifier* apMod) {
        pFormImageSpaceModifying[auiIndex] = apMod;
    }

    // Resolves the modifier form IDs, read from apOwner's file.
    void InitItem(TESForm* apOwner);
    // Saves the modifiers' form IDs as the owner's chunk.
    void Save();

    TESImageSpaceModifier* pFormImageSpaceModifying[N];
};

// Clears every modifier slot.
template <uint32_t N>
void TESImageSpaceModifiableCountForm<N>::InitializeDataComponent() {
    for (uint32_t i = 0; i < N; ++i) {
        pFormImageSpaceModifying[i] = nullptr;
    }
}

// Copies every modifier slot of apCopyFC when it is the same component.
template <uint32_t N>
void TESImageSpaceModifiableCountForm<N>::CopyComponent(BaseFormComponent* apCopyFC) {
    TESImageSpaceModifiableCountForm* pCopy = dynamic_cast<TESImageSpaceModifiableCountForm*>(apCopyFC);
    if (pCopy) {
        for (uint32_t i = 0; i < N; ++i) {
            SetFormImageSpaceModifying(i, pCopy->GetFormImageSpaceModifying(i));
        }
    }
}

// True when the components differ: apCompareFC is not this component, or a slot holds another modifier.
template <uint32_t N>
bool TESImageSpaceModifiableCountForm<N>::CompareComponent(BaseFormComponent* apCompareFC) {
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
