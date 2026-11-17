#include "Fallout/AI/CharacterProgression.h"

#include "Fallout/AI/PlayerCharacter.h"

CharacterProgressionData::CharacterProgressionData() {
	Initialize();
}

CharacterProgression::CharacterProgression() {
	AllocateData();
}

void CharacterProgression::InitializeData() {
	if (pData) {
		pData->Initialize();
	}
}

char CharacterProgression::GetReadyToLevelUp() {
	return (int8_t)((uint8_t)pData->bReadyToLevelUp);
}

// The old-format save, the counterpart of LoadGame(uint32_t, uint32_t); the flags argument is ignored.
void CharacterProgression::SaveGame(uint32_t aiFlags) {
	CharacterProgressionData* Data = pData;
	pPlayer->SaveGameDataOLD(Data, 1);
	int32_t* p_iNextThreshold = &pData->iNextThreshold;
	pPlayer->SaveGameDataOLD(p_iNextThreshold, 4);
}

// The old-format save's counterpart of SaveGame(uint32_t); the flags are ignored.
void CharacterProgression::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	CharacterProgressionData* v3 = GetData();
	pPlayer->LoadGameDataOLD(static_cast<double*>(static_cast<void*>(v3)), 1);
	CharacterProgressionData* v4 = GetData();
	pPlayer->LoadGameDataOLD(static_cast<double*>(static_cast<void*>(&v4->iNextThreshold)), 4);
}
