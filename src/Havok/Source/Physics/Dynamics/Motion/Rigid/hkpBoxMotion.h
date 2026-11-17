#pragma once

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

void finishLoadedObjecthkpBoxMotion(void* p, int32_t finishing);

// Adds no fields to hkpMotion.
class hkpBoxMotion : public hkpMotion {
public:
    // The finish constructor: a1 is the hkFinishLoadedObjectFlag's m_finishing.
    explicit hkpBoxMotion(int32_t a1);
    // The SDK's finish constructor, which getVtablehkpBoxMotion builds: the one above with the flag's m_finishing.
    hkpBoxMotion(hkFinishLoadedObjectFlag flag) : hkpBoxMotion(flag.m_finishing) {}
    hkpBoxMotion(const hkVector4& position, const hkQuaternion& rotation);
    ~hkpBoxMotion() override;

    void setMass(float mass) override;
    void getInertiaLocal(hkMatrix3& inertia) const override;
    void getInertiaWorld(hkMatrix3& inertia) const override;
    void setInertiaLocal(const hkMatrix3& inertia) override;
    void setInertiaInvLocal(const hkMatrix3& inertiaInv) override;
    void getInertiaInvLocal(hkMatrix3& inertiaInv) const override;
    void getInertiaInvWorld(hkMatrix3& inertiaInv) const override;
    void getProjectedPointVelocity(
        const hkVector4& pos,
        const hkVector4& normal,
        float& velOut,
        float& invVirtMassOut
    ) const override;
    void applyPointImpulse(const hkVector4& imp, const hkVector4& p) override;
    void applyAngularImpulse(const hkVector4& imp) override;
    void applyForce(float deltaTime, const hkVector4& force) override;
    void applyForce(float deltaTime, const hkVector4& force, const hkVector4& p) override;
    void applyTorque(float deltaTime, const hkVector4& torque) override;

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();
};

extern hkClass hkpBoxMotionClass;
extern hkTypeInfo hkpBoxMotionTypeInfo;
