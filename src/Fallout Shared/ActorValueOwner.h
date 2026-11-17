#pragma once

#include "Fallout Shared/ActorValue.h"

// Interface of things that have actor values (Actor, TESActorBase, ...); a base with only a vtable pointer and no
// virtual destructor. aiWhich is an ActorValue::Index.
class ActorValueOwner {
public:
    ActorValueOwner();

    virtual int32_t GetBaseActorValue(ActorValue::Index) {
        return 0;
    }
    virtual float GetBaseActorFloatValue(ActorValue::Index) {
        return 0.0f;
    }
    virtual int32_t GetActorValue(ActorValue::Index) {
        return 0;
    }
    virtual float GetActorFloatValue(ActorValue::Index) {
        return 0.0f;
    }
    virtual float GetTemporaryModifier(ActorValue::Index) {
        return 0.0f;
    }
    virtual float GetDamageModifier(ActorValue::Index) {
        return 0.0f;
    }
    virtual float GetPermanentModifier(ActorValue::Index) {
        return 0.0f;
    }
    // the permanent float value rounded down
    virtual int32_t GetPermanentActorValue(ActorValue::Index aiWhich);
    // the base value plus the permanent modifier
    virtual float GetPermanentActorFloatValue(ActorValue::Index aiWhich);
    virtual TESForm* GetAsForm() {
        return nullptr;
    }
    virtual uint16_t GetActorLevel() {
        return 0;
    }

    // The actor value clamped to its range.
    int32_t GetClampedActorValue(ActorValue::Index aiWhich);
    // The float actor value clamped to its range.
    float GetClampedActorFloatValue(ActorValue::Index aiWhich);

    // The fly speed scaled by afFlyInventoryWeight.
    static float CalculateFlySpeed(ActorValueOwner* apOwner, float afFlyInventoryWeight);
    static char ShouldDamageKnockdownTarget(ActorValueOwner* a1, int32_t a2);
};
