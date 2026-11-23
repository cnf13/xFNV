#pragma once

#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Actor;

// The player's mover: moves the player by the control input instead of following paths.
class PlayerMover : public ActorMover {
public:
    PlayerMover(Actor* apActor);
    ~PlayerMover() override;

    uint32_t GetPreferredMoveMode() override {
        return MoveMode;
    }
    // Returns the same value as GetPreferredMoveMode: the player's move mode is the one the controls set, never
    // ActorMover's cached path mode.
    uint32_t GetMoveMode() override {
        return MoveMode;
    }
    void SetMoveModePreference(uint32_t auiPreferredMoveMode) override {
        SetMoveMode(auiPreferredMoveMode, true);
    }
    void ClearMoveModePreference(uint32_t auiPreferredMoveMode) override {
        ClearMoveMode(auiPreferredMoveMode);
    }
    void ForceMoveMode(uint32_t aiMoveMode) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void UpdateTurnTimer(float fdeltaT) override;
    void Update(float fdeltaT) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;

    void SetMoveMode(uint32_t auiMoveMode, bool abValue);
    void ClearMoveMode(uint32_t auiMoveMode);
    void SetSideVector(const NiPoint3& aSideVec);

    NiPoint3 SideVec;
    uint32_t MoveMode;
    uint32_t LastTurnDir;
    float fMinTurnTime;
};
