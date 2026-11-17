#pragma once

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
	operator float() const;

	float fTimeStamp;
};
