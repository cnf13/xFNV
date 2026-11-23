#pragma once

#include "Fallout/AI/Actor.h"

// An animal, robot, mutant or other non-humanoid actor.
class Creature : public Actor {
public:
    Creature();
    explicit Creature(bool abadd);
    ~Creature() override;

    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    void Set3D(NiAVObject* apObject, bool abTestProcessLevel) override;
    bool IsCreature() override {
        return true;
    }

    int32_t GetActorValue(ActorValue::Index aiWhich) override;
    float GetActorFloatValue(ActorValue::Index aiWhich) override;

    float GetTurningSpeed() override;
    float GetReach() override;
    void SetRagDoll(bool abFound) override {
        bRagDoll = abFound;
    }
    bool HasRagDoll() override {
        return bRagDoll;
    }
    int32_t GetActorType() override {
        return 0;
    }
    void SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) override;
    void SetActorValue(ActorValue::Index aiWhich, float afValue) override;
    void TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) override;
    void TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) override;
    void PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) override;
    void PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) override;
    void DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) override;
    void DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) override;
    void ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) override;
    void ModActorBaseValue(ActorValue::Index aiWhich, float afValue) override;
    void ResetArmorRating() override;
    float GetArmorRating() override;
    float GetArmorThreshold() override;
    int32_t GetAttackDamage() override;
    float CalculateWalkSpeed() override;
    float CalculateRunSpeed() override;

    float GetWeaponSkill();
    // Creatures have no weapon skills: every combat skill is read as Unarmed.
    ActorValue::Index MapActorValue(ActorValue::Index aiWhich);

    bool FindRagDoll(NiAVObject* apObject);
    // Detaches the limbs the creature is missing from its 3D.
    void CheckMissingLimbs();
    float GetMovementAnimationSpeed(ANIM_GROUP_ENUM aeMoveMode);

    bool bRagDoll;
    float fArmorRating;
    float fArmorThreshold;
};
