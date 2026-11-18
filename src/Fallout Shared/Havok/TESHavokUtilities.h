#pragma once

#include <cstdint>

class Actor;
class bhkRigidBody;
class HitData;
class hkpCollidable;
class hkVector4;
class NiAVObject;
class NiPoint3;
class TESObjectREFR;

enum HavokMaterialType : int32_t;

double GetObjectImpactDamage(float a1, float a2);

// Static Havok helpers over references' 3D. Only some of the entry points are declared.
class TESHavokUtilities {
public:
    // The explosion impulse multiplier per biped part, indexed with bits 8-12 of the collision filter info.
    using ExplosionMultArray = float[32];

    // The reference that owns apCol, or null.
    static TESObjectREFR* FindCollidableRef(const hkpCollidable* apCol);
    // Adds aVelocity, in game units, to the rigid bodies under apObject, replacing their velocity when abReset.
    static void AddVelocity(NiAVObject* apObject, const NiPoint3& aVelocity, bool abReset);
    // The Havok-unit form of AddVelocity.
    static void AddVelocity(NiAVObject* apObject, const hkVector4& aVelocity, bool abReset);
    // Adds the angular velocity aVelocity, in game units, to the rigid bodies under apObject.
    static void AddAngularVelocity(NiAVObject* apObject, const NiPoint3& aVelocity, bool abReset);
    // Adds the Havok angular velocity aVelocity to the rigid bodies under apObject, replacing it when abReset.
    static void AddAngularVelocity(NiAVObject* apObject, const hkVector4& aVelocity, bool abReset);
    // Merges the simulation islands of pActor's ragdoll.
    static void ConsolidateSimIslands(Actor* pActor);
    static float ScaleGameplayImpulseForce(float afBaseForce, bhkRigidBody* apPushedBody, bool abUseMinMass);
    // Detaches the collision objects of apObject and everything below it.
    static void RemoveHavokFromSceneGraph(NiAVObject* apObject);
    // The buoyancy setting of the material, 0 for the materials the switch leaves out.
    static float GetMaterialBuoyancy(HavokMaterialType aeMaterial);
    // The summed buoyancy of apBody's materials.
    static float GetTotalBuoyancy(bhkRigidBody* apBody);
    // Applies an explosion impulse of afMagnitude from the Havok-unit point aLocation to the rigid bodies under
    // apObject, scaled per biped part and by the limb apHitData hit.
    static void AddExplosionImpulse(NiAVObject* apObject, hkVector4& aLocation, float afMagnitude, HitData* apHitData);
    // Whether apRef's 3D ap3D is simulated as a ragdoll.
    static bool Should3DRagDoll(TESObjectREFR* apRef, NiAVObject* ap3D);
    static void
    InspectHavokObjects(NiAVObject* apObject, int32_t& aiConvexVerticesCount, int32_t& aiShapeCollectionTriCount);
    static NiPoint3 GetLocalPointWithinCollision(NiAVObject* apObject);
    static TESObjectREFR* FindCollidableRef_009C69E0(const hkpCollidable* apCol);

    static inline ExplosionMultArray pfExplosionMult{1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.75f, 0.5f,
                                                     0.75f, 0.5f,  0.25f, 1.0f,  0.75f, 0.5f,  0.75f, 0.5f,
                                                     0.25f, 0.75f, 0.75f, 0.75f, 0.75f, 0.75f, 0.75f};
};
