#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimer.h"

class BGSExplosion;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

class CombatProcedureAttackGrenade : public CombatProcedure {
public:
    enum ATTACK_STATE : int32_t {
        ATTACK_STATE_IDLE = 0x0,
        ATTACK_STATE_ATTACKING = 0x1,
    };

    CombatProcedureAttackGrenade();
    CombatProcedureAttackGrenade(const NiPoint3& arTargetLocation, bool abHighArc);
    ~CombatProcedureAttackGrenade() override;

    void Update() override;
    void Shutdown() override;
    void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
    bool GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) override;
    void FiredProjectile(Projectile* apProjectile) override;
    CombatProcedure::COMBAT_PROCEDURES GetType() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    bool CheckExplosion(BGSExplosion* apExplosion, const NiPoint3& arLocation, float afTime);
    NiPoint3 CalculateTargetedPoint();

    ATTACK_STATE eState;
    bool bTargetLocation;
    uint8_t padding15[3];
    NiPoint3 TargetLocation;
    NiPoint3 TargetedPoint;
    bool bHighArc;
    uint8_t padding31[3];
    CombatTimer ExplosiveProjectileBlockedTimer;
};
