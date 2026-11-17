#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

// The number of uses of a form. The exe has no vtable of this class; only its RTTI descriptor (0x1186B88), which
// TESForm::__SaveData and TESForm::LoadData cast to.
class TESUsesForm : public BaseFormComponent {
public:
	uint8_t iUses;
	uint8_t padding05[3];
};
