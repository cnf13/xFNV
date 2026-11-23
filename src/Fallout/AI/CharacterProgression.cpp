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
    return pData->bReadyToLevelUp;
}

// The old-format save, the counterpart of LoadGame(uint32_t, uint32_t); the flags argument is ignored.
void CharacterProgression::SaveGame(uint32_t aiFlags) {
    pPlayer->SaveGameDataOLD(pData, 1);
    pPlayer->SaveGameDataOLD(&pData->iNextThreshold, sizeof(pData->iNextThreshold));
}

// The old-format save's counterpart of SaveGame(uint32_t); the flags are ignored.
void CharacterProgression::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    CharacterProgressionData* pProgressionData = GetData();
    pPlayer->LoadGameDataOLD(static_cast<double*>(static_cast<void*>(pProgressionData)), 1);
    pProgressionData = GetData();
    pPlayer->LoadGameDataOLD(
        static_cast<double*>(static_cast<void*>(&pProgressionData->iNextThreshold)),
        sizeof(pProgressionData->iNextThreshold)
    );
}
