#pragma once

#include "Fallout/AI/Combat/CombatProcedure.h"

// The combat procedure that moves out of the way of a threat.
class CombatProcedureEvade : public CombatProcedure {
public:
    enum EVADE_STATE : int32_t {
        EVADE_STATE_INITIALIZING = 0x0,
        EVADE_STATE_EVADING = 0x1,
    };

    CombatProcedureEvade(int32_t aiLocation);
    ~CombatProcedureEvade() override;

    void Update() override;
    void Shutdown() override;
    CombatProcedure::COMBAT_PROCEDURES GetType() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    EVADE_STATE eState;
    int32_t iLocation;
};
