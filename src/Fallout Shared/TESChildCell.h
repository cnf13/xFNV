#pragma once

#include "Reconstruction.hpp"

class TESObjectCELL;

// A mixin for forms that live in a cell; it contributes only its vtable pointer (no virtual destructor in the exe).
class TESChildCell {
public:
	TESChildCell() {}

	// the cell a save game files the form under
	virtual TESObjectCELL* GetSaveParentCell() = 0;
};
inline constexpr fnvd::Vtable TESChildCell_vftable{};
