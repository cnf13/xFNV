#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

void cleanupLoadedObjecthkpSphereMotion(void* p);
// The type info's finish-loaded-object callback for hkpSphereMotion.
void finishLoadedObjecthkpSphereMotion(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpSphereMotion();

// A rigid motion whose inertia is a sphere's: one value on the diagonal, kept inverted in m_inertiaAndMassInv's x, y
// and z.
class hkpSphereMotion : public hkpMotion {
public:
    hkpSphereMotion(const hkVector4& position, const hkQuaternion& rotation);
    // The finish constructor: finishing is the hkFinishLoadedObjectFlag's m_finishing.
    explicit hkpSphereMotion(int32_t finishing);
    // The SDK's finish constructor, which getVtablehkpSphereMotion builds: the one above with the flag's m_finishing.
    hkpSphereMotion(hkFinishLoadedObjectFlag flag) : hkpSphereMotion(flag.m_finishing) {}
    ~hkpSphereMotion() override;

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
    void applyForce(float deltaTime, const hkVector4& force, const hkVector4& p) override;
    void applyForce(float deltaTime, const hkVector4& force) override;
    void applyTorque(float deltaTime, const hkVector4& torque) override;

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();
};

extern hkClass hkpSphereMotionClass;
extern hkTypeInfo hkpSphereMotionTypeInfo;
