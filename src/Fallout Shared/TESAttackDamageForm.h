#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class TESForm;

// The base attack damage of a form.
class TESAttackDamageForm : public BaseFormComponent {
public:
    TESAttackDamageForm();
    ~TESAttackDamageForm();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    virtual uint16_t GetAttackDamage() const;

    void SetDamage(uint16_t asDamage);

    static uint16_t GetAttackDamage_0(TESForm* apForm);

    uint16_t sAttackDamage;
};
