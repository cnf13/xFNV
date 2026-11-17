#pragma once

#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"

#include <stdint.h>

class bhkWorld;
class NiAVObject;
class NiTransform;

void GetTransformRecursive(NiAVObject* apObject, NiAVObject* apRoot, NiTransform& aWorldTransform);

// The blend collision object of a weapon: the node it moves, and the rigid bodies of its chain.
class WeaponObject : public bhkBlendCollisionObject {
public:
    WeaponObject();
    WeaponObject(
        CFilter akFilter,
        NiAVObject* apOwner3d,
        NiAVObject* apObject,
        bhkCollisionObject* apReplaceObject,
        bool abDefaultKeyframed
    );
    ~WeaponObject() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess&) override;
    void UpdateWorldData(NiUpdateData&) override;
    // Returns false: a weapon is not part of the skeleton, unlike bhkNiCollisionObject's (true).
    bool QIncludedInSkeleton() const override;

    // Clears the hidden flag of the scene graph object of every gathered rigid body but the collision object's own.
    void UpdateChildRB3D();
    void Init(
        CFilter aOwnerFilter,
        NiAVObject* apOwner3d,
        NiAVObject* apObject,
        bhkCollisionObject* apReplaceObject,
        bool abDefaultKeyframed
    );
    void Kill();

protected:
    // Copies the members into apDest when cloning.
    void CopyMembers(WeaponObject* apDest, NiCloningProcess& arCloneData);

public:
    void UpdateConstraints(bool abEnable, CFilter aOwnerFilter, NiAVObject* apRoot3d, bool abDefaultKeyframed);

    // A new weapon object over apObject, or null when apObject has no collision object.
    static WeaponObject*
    CreateWeaponObject(CFilter aOwnerFilter, NiAVObject* apOwner3d, NiAVObject* apObject, bool abDefaultKeyframed);
    // Updates apWeaponObj, then makes the rigid bodies of the nodes beside the "Chain01" node dynamic. apWorld is not
    // read.
    static void UpdateChain(NiAVObject* apWeaponObj, bhkWorld* apWorld);
    // Moves the weapon's rigid body to the parent's rigid body transform composed with the weapon object's transform
    // below apParent.
    static void UpdateRigidBodyTransforms(NiAVObject* apObject, NiAVObject* apParent);
    static void
    Constrain(bool abEnable, CFilter aOwnerFilter, NiAVObject* apFrom, NiAVObject* apTo, bool abDefaultKeyframed);

    static NiRTTI ms_RTTI;
};
