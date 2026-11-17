#pragma once

#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

// A model whose ragdoll constraint data a biped model form uses.
class TESModelRDT : public TESModel {
public:
	TESModelRDT();
	~TESModelRDT() override;
};
inline constexpr fnvd::Vtable TESModelRDT_vftable{};
