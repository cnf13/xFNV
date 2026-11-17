#pragma once

#include "BSHavok/bhkEntityListener.h"
#include "Reconstruction.hpp"

// The Havok side of the arrow targets: an entity listener that does not implement Update itself (TESArrowTargetListener
// does). The PDB's source module is TESArrowTargetListener.cpp.
class bhkArrowTargetListener : public bhkEntityListener {
public:
	bhkArrowTargetListener();
	~bhkArrowTargetListener() override;

	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable bhkArrowTargetListener_vftable{};
