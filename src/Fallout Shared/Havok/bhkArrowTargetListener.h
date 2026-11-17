#pragma once

#include "BSHavok/bhkEntityListener.h"

// The Havok side of the arrow targets: an entity listener that does not implement Update itself (TESArrowTargetListener
// does).
class bhkArrowTargetListener : public bhkEntityListener {
public:
	bhkArrowTargetListener();
	~bhkArrowTargetListener() override;

	static void operator delete(void* apMemory);
};
