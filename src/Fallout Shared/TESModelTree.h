#pragma once

#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

// The model of a tree.
class TESModelTree : public TESModel {
public:
	TESModelTree();
	~TESModelTree() override;
};
inline constexpr fnvd::Vtable TESModelTree_vftable{};
