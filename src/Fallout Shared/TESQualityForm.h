#pragma once

#include "Fallout Shared/BaseFormComponent.h"

// The quality of a form. TESForm::__SaveData and TESForm::LoadData cast to this class.
class TESQualityForm : public BaseFormComponent {
public:
    // Read and written as a byte, although the record stores it as a float.
    uint8_t cQuality;
};
