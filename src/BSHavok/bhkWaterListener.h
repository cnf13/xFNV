#pragma once

#include "BSHavok/bhkEntityListener.h"

// An entity listener for bodies in water.
class bhkWaterListener : public bhkEntityListener {
public:
	bhkWaterListener();
	~bhkWaterListener() override;

	void Update(float) override {}

	virtual float GetRandom(float) { return 0.0f; }
};
