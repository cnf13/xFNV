#pragma once

#include "Havok/Source/Common/Base/Container/Queue/hkQueue.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"

class hkpRigidBody;
class hkpRigidBodyCinfo;
class hkpShape;

const void* getVtablehkpBallGun();
void finishLoadedObjecthkpBallGun(void* p, int32_t finishing);
void cleanupLoadedObjecthkpBallGun(void* p);

// The demo framework's gun that fires balls: each shot adds a rigid sphere to the world, and the oldest bullets are
// removed once m_maxBulletsInWorld are out. It overrides the destructor, fireGun and reset.
class hkpBallGun : public hkpFirstPersonGun {
public:
    hkpBallGun(hkFinishLoadedObjectFlag flag);
    hkpBallGun(int32_t numBullets);
    // Slot 0.
    ~hkpBallGun() override;

    // Slot 4.
    void fireGun(hkpWorld* world, const hkTransform& viewTransform) override;
    // Slot 8: removes every bullet still queued, telling the listeners first and taking each out
    // of world when it is in it.
    void reset(hkpWorld* world) override;

protected:
    // Computes the volume mass properties of apShape at mass afMass, then sets arInfo's inertia tensor, centre of mass
    // and mass from them.
    static hkpRigidBodyCinfo* computeMassProperties(const hkpShape* apShape, float afMass, hkpRigidBodyCinfo& arInfo);

public:
    // Havok SDK signature (hkpUtilities.lib, tied by target).
    static const hkClass& staticClass();

    float m_bulletRadius;
    float m_bulletVelocity;
    float m_bulletMass;
    float m_damageMultiplier;
    int32_t m_maxBulletsInWorld;
    hkVector4 m_bulletOffsetFromCenter;
    // The bullets in the world, oldest first.
    hkQueue<hkpRigidBody*>* m_addedBodies;
};

// The class's reflection record.
extern hkClass hkpBallGunClass;
extern hkTypeInfo hkpBallGunTypeInfo;
