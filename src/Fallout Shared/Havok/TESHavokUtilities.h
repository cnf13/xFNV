#pragma once

#include "Reconstruction.hpp"

class Actor;
enum HavokMaterialType : int32_t;
class HitData;
class NiAVObject;
class NiPoint3;
class TESObjectREFR;
class bhkRigidBody;
class hkVector4;
class hkpCollidable;

// Inferred type: a1: float from 0x8b9142 (result of bhkRigidBody::GetMass); a2: float from 0x6239a5 (result of NiAbs)
double GetObjectImpactDamage(float a1, float a2);

// Static Havok helpers over references' 3D. Only the entry points the reconstructed code calls are declared.
class TESHavokUtilities {
public:
	// The explosion impulse multiplier per biped part; the exe indexes it with bits 8-12 of the collision filter info.
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
	// The buoyancy setting of the material, 0 for the materials the exe's switch leaves out (cdecl, 0x62B1E0).
	static float GetMaterialBuoyancy(HavokMaterialType aeMaterial);
	// The summed buoyancy of apBody's materials, returned in ST0 (cdecl, ret 0, 0x62B310).
	static float GetTotalBuoyancy(bhkRigidBody* apBody);
	// Applies an explosion impulse of afMagnitude from the Havok-unit point aLocation to the rigid bodies under
	// apObject, scaled per biped part and by the limb apHitData hit.
	static void AddExplosionImpulse(NiAVObject* apObject, hkVector4& aLocation, float afMagnitude, HitData* apHitData);
	// Whether apRef's 3D ap3D is simulated as a ragdoll.
	static bool Should3DRagDoll(TESObjectREFR* apRef, NiAVObject* ap3D);
	static void InspectHavokObjects(NiAVObject* apObject, int32_t& aiConvexVerticesCount,
		int32_t& aiShapeCollectionTriCount);
	static NiPoint3* GetLocalPointWithinCollision(NiPoint3* apResult, NiAVObject* apObject);
	// Inferred type: return: TESObjectREFR* from 0x9c6a0e (result of TESObjectREFR::FindReferenceFor3D), its members
	// used at 0x9c6655; apCol: hkpCollidable* from 0x8d1265 (hkpRootCdPoint::m_rootCollidableB (result of
	// hkArrayBase<hkpRootCdPoint>::operator[]<hkpRootCdPoint>))
	static TESObjectREFR* FindCollidableRef_009C69E0(const hkpCollidable* apCol);

	static inline ExplosionMultArray pfExplosionMult{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.75f, 0.5f, 0.75f, 0.5f,
		0.25f, 1.0f, 0.75f, 0.5f, 0.75f, 0.5f, 0.25f, 0.75f, 0.75f, 0.75f, 0.75f, 0.75f, 0.75f};
};
