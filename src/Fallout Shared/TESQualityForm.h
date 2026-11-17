#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

// The quality of a form. The exe has no vtable of this class; only its RTTI descriptor (0x1186BDC), which
// TESForm::__SaveData and TESForm::LoadData cast to.
class TESQualityForm : public BaseFormComponent {
public:
	// The exe reads it as a byte (the shared byte getter 0x4F1540, from TESForm::__SaveData) and writes it as one (the
	// shared byte setter 0x4F15A0, from TESForm::LoadData); the record stores it as a float.
	uint8_t cQuality;
	uint8_t padding05[3];
};
