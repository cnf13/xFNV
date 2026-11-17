#pragma once

#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiAVObject;
class NiTransform;
class bhkWorld;

void GetTransformRecursive(NiAVObject* apObject, NiAVObject* apRoot, NiTransform& aWorldTransform);

// The blend collision object of a weapon: the node it moves, and the rigid bodies of its chain.
class WeaponObject : public bhkBlendCollisionObject {
public:
	WeaponObject();
	WeaponObject(CFilter akFilter, NiAVObject* apOwner3d, NiAVObject* apObject, bhkCollisionObject* apReplaceObject,
		bool abDefaultKeyframed);
	~WeaponObject() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess&) override;
	void UpdateWorldData(NiUpdateData&) override;
	// The exe's slot holds NiObject::StreamCanSkip's body (0xEE8990): a weapon is not part of the skeleton, unlike
	// bhkNiCollisionObject's (true).
	bool QIncludedInSkeleton() const override;

	// Clears the hidden flag of the scene graph object of every gathered rigid body but the collision object's own.
	void UpdateChildRB3D();
	void Init(CFilter aOwnerFilter, NiAVObject* apOwner3d, NiAVObject* apObject, bhkCollisionObject* apReplaceObject,
		bool abDefaultKeyframed);
	void Kill();

protected:
	// Copies the members into apDest when cloning.
	void CopyMembers(WeaponObject* apDest, NiCloningProcess& arCloneData);

public:
	void UpdateConstraints(bool abEnable, CFilter aOwnerFilter, NiAVObject* apRoot3d, bool abDefaultKeyframed);

	// A new weapon object over apObject, or null when apObject has no collision object.
	static WeaponObject* CreateWeaponObject(CFilter aOwnerFilter, NiAVObject* apOwner3d, NiAVObject* apObject,
		bool abDefaultKeyframed);
	// Updates apWeaponObj, then makes the rigid bodies of the nodes beside the "Chain01" node dynamic (cdecl, the
	// caller pops 8, 0xC88810). apWorld is pushed by the caller (0xC88FCC) and not read.
	static void UpdateChain(NiAVObject* apWeaponObj, bhkWorld* apWorld);
	// Moves the weapon's rigid body to the parent's rigid body transform composed with the weapon object's
	// transform below apParent (cdecl, ret 0, 0xC88520; a KEEP body, unregistered).
	static void UpdateRigidBodyTransforms(NiAVObject* apObject, NiAVObject* apParent);
	// The exe tests abEnable's and abDefaultKeyframed's low bytes only (0xC88BE5, 0xC88C1C).
	static void Constrain(bool abEnable, CFilter aOwnerFilter, NiAVObject* apFrom, NiAVObject* apTo,
		bool abDefaultKeyframed);

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable WeaponObject_vftable{};
