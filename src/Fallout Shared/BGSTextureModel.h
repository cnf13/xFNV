#pragma once

#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

// A model used as a texture.
class BGSTextureModel : public TESModel {
public:
	~BGSTextureModel() override;
};
inline constexpr fnvd::Vtable BGSTextureModel_vftable{};
