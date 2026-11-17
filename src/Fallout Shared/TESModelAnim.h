#pragma once

#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

// The animation model of an idle form.
class TESModelAnim : public TESModel {
public:
	TESModelAnim();
	~TESModelAnim() override;
};
inline constexpr fnvd::Vtable TESModelAnim_vftable{};
