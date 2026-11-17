#pragma once

#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;

// The game time a combat event happened at.
class CombatTimeStamp {
public:
	CombatTimeStamp();
	CombatTimeStamp(float afValue);

	// Loads the time stamp and makes it absolute again.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

	// Saves the time stamp relative to the combat timer; an infinite stamp is saved as is.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	float ElapsedTime();
	float DisplayElapsedTime();
	// fTimeStamp; the exe's copy is folded with DecalData::GetDecalMinWidth (thiscall, ret 0,
	// ST0, 0x6A7F50).
	operator float() const;

	float fTimeStamp;
};
