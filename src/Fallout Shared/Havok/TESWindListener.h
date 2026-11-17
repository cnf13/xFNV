#pragma once

#include "BSHavok/bhkWindListener.h"

// The game's wind listener: the bhkWindListener that draws its random push through its own GetRandom.
class TESWindListener : public bhkWindListener {
public:
	TESWindListener();
	~TESWindListener() override;

	float GetRandom(float afSize) override;

	static void operator delete(void* apMemory);
};
