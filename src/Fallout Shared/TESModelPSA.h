#pragma once

#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

// The pose model of a body part.
class TESModelPSA : public TESModel {
public:
	TESModelPSA();
	~TESModelPSA() override;
};
inline constexpr fnvd::Vtable TESModelPSA_vftable{};
