#pragma once

#include "Fallout Shared/TESTexture1024.h"
#include "Reconstruction.hpp"

// The icon texture of a tree.
class TESIconTree : public TESTexture1024 {
public:
	TESIconTree();
	~TESIconTree();

	const char* GetDefaultPath() const override;
};
inline constexpr fnvd::Vtable TESIconTree_vftable{};
