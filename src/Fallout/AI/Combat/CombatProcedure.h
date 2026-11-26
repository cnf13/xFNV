#pragma once

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Actor;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class CombatController;
class Projectile;
class TESBoundObject;
class TESObjectREFR;

class CombatProcedure {
public:
    enum COMBAT_PROCEDURES : uint32_t;

    // Where the procedure stands.
    enum COMBAT_PROCEDURE_STATUS : uint32_t {
        ACTIVE = 0,
        FAILED = 1,
        FINISHED = 2,
        COMBAT_PROCEDURE_STATUS_COUNT = 3,
    };

    CombatProcedure();
    virtual ~CombatProcedure();

    virtual void Update() {}
    virtual void Initialize(CombatController* apCombatController);
    virtual void Shutdown() {}
    virtual void ClearReferencePointers(TESObjectREFR* apRef) {}

    virtual void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) {
    }

    virtual bool GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint);
    virtual void Disarmed(Actor* apOpponent) {}
    virtual void FiredProjectile(Projectile* apProjectile) {}
    virtual void TargetChanged() {}

    virtual void
    ActivatedReference(TESObjectREFR* apRef, TESBoundObject* apObject, int32_t aiCount, bool abActivatedScriptOnly) {}

    virtual void StartedPlanning() {}
    virtual void FinishedPlanning() {}
    virtual CombatProcedure::COMBAT_PROCEDURES GetType() = 0;
    virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {}

    // The controller's attacker.
    Actor* GetAttacker();
    void ClearLookAtTarget();
    void ClearMoveMode();
    void SetFailed(const char* apReason);
    Actor* GetTarget();
    bool GetFailed();
    bool GetFinished();
    void SetMoveMode(int32_t aiCombatMoveMode);
    bool WithinAttackRange(float afDistance, float afReach, int32_t aiAttackWithinReach);
    void SetLookAtTarget(const NiPoint3& arLookAtPoint, uint8_t acPriority);
    void SetFinished();
    ANIM_GROUP_ENUM SelectPowerAttack(
        float afDistance,
        float afReach,
        bool abTargetAttacking,
        bool abTargetPostAttack,
        bool abForwardOnly
    );

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Creates the procedure of the kind the save data names.
    static CombatProcedure* CreateProcedure(uint32_t aiType);

    CombatController* pCombatController;
    COMBAT_PROCEDURE_STATUS eStatus;
    char* pFailureReason;
};

// The procedure kinds GetType returns (CombatProcedureSearch::GetType returns SEARCH = 10).
enum CombatProcedure::COMBAT_PROCEDURES : uint32_t {
    ATTACK_RANGED = 0,
    ATTACK_MELEE = 1,
    ATTACK_GRENADE = 2,
    ATTACK_LOW = 3,
    EVADE = 4,
    SWITCH_WEAPON = 5,
    MOVE_TO_LOCATION = 6,
    BE_IN_COVER = 7,
    ACTIVATE_OBJECT = 8,
    HIDE_FROM_TARGET = 9,
    SEARCH = 10,
    USE_COMBAT_ITEM = 11,
    ENGAGE_TARGET = 12,
};

// The power attack animation group (ANIM_GROUP_ENUM values) of each
// preferred direction 0-4; SelectPowerAttack indexes it, 5 meaning none.
using ePowerAttacks_t = int32_t[5];
inline ePowerAttacks_t ePowerAttacks{92, 93, 94, 95, 96};
