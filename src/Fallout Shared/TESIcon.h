#pragma once

#include "Fallout Shared/TESTexture.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

// An icon texture file name.
class TESIcon : public TESTexture {
public:
	TESIcon();
	~TESIcon();

	const char* GetDefaultPath() const override;
};
inline constexpr fnvd::Vtable TESIcon_vftable{};
