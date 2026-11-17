#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

class hkpRigidBody;
class hkpWorld;
class hkTransform;

const void* getVtablehkpFirstPersonGun();
void finishLoadedObjecthkpFirstPersonGun(void* p, int32_t finishing);
void cleanupLoadedObjecthkpFirstPersonGun(void* p);

// What a gun tells about its bullets: each one created and deleted, and each shot's end points. The bodies are the
// listeners' own, so they stay empty here.
class hkpFirstPersonGunBulletListener : public hkReferencedObject {
public:
    virtual void bulletCreatedCallback(hkpRigidBody* apBullet) {}
    virtual void bulletDeletedCallback(hkpRigidBody* apBullet) {}
    virtual void gunFiredCallback(const hkVector4* apFrom, const hkVector4* apTo) {}
};

// The base of the demo framework's first person guns: a named gun bound to a key that steps with its holder.
class hkpFirstPersonGun : public hkReferencedObject {
public:
    // The kinds of gun (m_type).
    enum Type {
        WEAPON_TYPE_INVALID = 0x0,
        WEAPON_TYPE_BALLGUN = 0x1,
        WEAPON_TYPE_GRENADEGUN = 0x2,
        WEAPON_TYPE_GRAVITYGUN = 0x3,
        WEAPON_TYPE_MOUNTEDBALLGUN = 0x4,
        WEAPON_TYPE_TWEAKERGUN = 0x5,
        WEAPON_TYPE_MISSILEGUN = 0x6,
        WEAPON_TYPE_RAYCASTGUN = 0x7,
        WEAPON_TYPE_SPHEREGUN = 0x8,
        WEAPON_TYPE_STICKYGUN = 0x9,
        WEAPON_TYPE_NUM_TYPES = 0xA,
    };

    // The keys a gun is bound to (m_keyboardKey).
    enum KeyboardKey {
        KEY_F1 = 0x70,
        KEY_F2 = 0x71,
        KEY_F3 = 0x72,
        KEY_F4 = 0x73,
        KEY_F5 = 0x74,
        KEY_F6 = 0x75,
        KEY_F7 = 0x76,
        KEY_F8 = 0x77,
        KEY_F9 = 0x78,
        KEY_F10 = 0x79,
        KEY_F11 = 0x7A,
        KEY_F12 = 0x7B,
    };

    // What sweepSphere finds: the contact point, the shape key and the body it hit.
    struct SweepSphereOut {
        hkContactPoint m_contactPoint;
        uint32_t m_shapeKey;
        hkpRigidBody* m_body;
    };

    hkpFirstPersonGun();
    hkpFirstPersonGun(hkFinishLoadedObjectFlag aFlag);

    ~hkpFirstPersonGun() override;

    virtual void initGun(hkpWorld* world) {}
    virtual void fireGun(hkpWorld* world, const hkTransform& viewTransform) {}

    virtual void stepGun(
        float timeStep,
        hkpWorld* world,
        const hkpRigidBody* characterBody,
        const hkTransform& viewTransform,
        bool fire,
        bool fireRmb
    ) {}

    virtual void quitGun(hkpWorld* world);
    // The gun's name, without the string pointer's ownership flag.
    virtual const char* getName() const;
    virtual void reset(hkpWorld* world) {}

    // Sweeps a sphere of radius from sweepStart to sweepEnd through world and fills out with the contact point, the
    // shape key and the body it hit; the result says whether it hit.
    static hkResult sweepSphere(
        const hkpWorld* world,
        const hkVector4& sweepStart,
        float radius,
        const hkVector4& sweepEnd,
        SweepSphereOut& out
    );

    // The velocity that sends a projectile from position at speed toward target, lifted against gravity for the
    // flight time; the lift is capped at speed.
    static void calcVelocityToTarget(
        const hkVector4& position,
        const hkVector4& target,
        const hkVector4& gravity,
        float speed,
        hkVector4& velocity
    );
    // Havok SDK signature (hkpUtilities.lib, tied by target).
    static const hkClass& staticClass();

    hkEnum<Type, uint8_t> m_type;
    uint8_t padding09[3];
    hkStringPtr m_name;
    hkEnum<KeyboardKey, uint8_t> m_keyboardKey;
    uint8_t padding11[3];
    hkArray<hkpFirstPersonGunBulletListener*> m_listeners;
};

extern hkClass hkpFirstPersonGunClass;

extern hkTypeInfo hkpFirstPersonGunTypeInfo;
