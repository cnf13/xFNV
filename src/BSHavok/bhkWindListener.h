#pragma once

#include "BSHavok/bhkEntityListener.h"
#include "Reconstruction.hpp"

// Pushes Havok bodies along the wind. Only the members the reconstruction calls are defined.
class bhkWindListener : public bhkEntityListener {
public:
	bhkWindListener();
	~bhkWindListener() override;

	void Update(float afTime) override;
	virtual float GetRandom(float afValue);

	// Sets the wind every listener applies.
	static void SetWind(float afSpeed, float afAngle);
	// The exe's deleting destructor frees through MemoryManager::Deallocate.
	static void operator delete(void* apMemory) { MemoryManager_Singleton.Deallocate(apMemory); }

	// The wind speed and direction (radians) every listener applies; SetWind writes them.
	static inline float fWindSpeed;
	static inline float fWindDir;
	// The cap on the randomised wind speed.
	static inline float fMaxWind = 250.0f;
};
inline constexpr fnvd::Vtable bhkWindListener_vftable{};
