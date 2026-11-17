#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;

// A combat countdown: the time it was started at and the time it runs out at.
class CombatTimer {
public:
	CombatTimer();

	void Reset(float afTargetTime);
	bool Check();
	// Loads the timer and makes a finite start time absolute again.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Saves the timer with a finite start time relative to the combat timer.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	void ForceTrigger();
	bool IsSet();
	bool IsForced();
	// The timer's time since fStartTime (the PDB's CombatTimer::ElapsedTime; CombatController's update 0x97E4B0 calls
	// it on kCombatPlanChangeTimer). The exe calls it out of line.
	float ElapsedTime();
	// Name not in the PDB (the field at +0x4); folded accessor, IDA: ::GetFlt004.
	float GetTargetTime();
	float GetStartTime() { return fStartTime; }
	void SetStartTime(float a1);
	void Clear();

	static float GetTimer();
	static void UpdateTimer(float afElapsedTime);

	float fStartTime;
	float fTargetTime;

	static inline float fTimer;
};
