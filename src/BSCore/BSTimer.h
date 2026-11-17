#pragma once

#include <cstdint>

// Frame timer: tracks the time between updates, scaled by the global time multiplier.
class BSTimer {
public:
	BSTimer();

	// The time step of the last update.
	float GetSeconds() const;
	// Sets the global time multiplier, at once when abNow is set.
	void SetGlobalTimeMultiplier(float afMult, bool abNow);

	// The time step of the last update with the global time multiplier divided out.
	float GetRealTimeSeconds() const;

	// The tick count recorded by the last update.
	uint32_t GetTime() const;

	// protected in the engine; public here so the replacement table can name it
	void Init(uint32_t aiTime);

	// Drops one disable level; the last one shifts the recorded times by the time spent disabled.
	void Enable();
	// Adds one disable level; the first one records the time the timer stopped.
	void Disable();
	// Advances the timer to the tick count aiTime.
	void Update(uint32_t aiTime);
	// Advances the timer to the current tick count (BSGetTickCount).
	void Update();
	void SetUseGlobalTimeMultiplierTarget(bool abValue);
	// Returns bZeroDelta.
	bool GetSecondsIsZero() const;
	void ClampAt(uint32_t aiClamp);
	void RestoreGlobalTimeMultiplierCache();

	static float QGlobalTimeMultiplier();
	static void UpdateGlobalTimeMultiplier();
	static double GetGlobalTimeMultiplierTarget();
	static void SaveGlobalTimeMultiplier();

	uint8_t cDisableCounter;
	uint8_t padding01[3];
	float fClamp;
	float fClampRemainder;
	float fDelta;
	float fDeltaUnmodified;
	uint32_t iLastTime;
	uint32_t iFirstTime;
	bool bUseGlobalTimeMultiplierTarget;
	bool bZeroDelta;
	uint8_t padding1E[2];

	static inline float fGlobalTimeMultiplier = 1.0f;
	static inline float fGlobalTimeMultiplierTarget = 1.0f;
	static inline float fPreviousGlobalTimeMultiplier = 1.0f;
};

// The game's frame timer.
extern BSTimer appTimer;

inline float fTimerGlobalMultChangeSpeed = 0.1f;
