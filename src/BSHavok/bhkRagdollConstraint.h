#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"

// The creation info of a ragdoll constraint. It adds no fields to hkConstraintCinfo.
class hkRagdollConstraintCinfo : public hkConstraintCinfo {
public:
    hkRagdollConstraintCinfo();
    ~hkRagdollConstraintCinfo() override;

    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    uint32_t CreateData() override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};

// The Gamebryo side of a Havok ragdoll constraint: the cone, plane and twist motors between two bodies. Only some of
// its members are declared here.
class bhkRagdollConstraint : public bhkConstraint {
public:
    bhkRagdollConstraint();
    bhkRagdollConstraint(const hkRagdollConstraintCinfo& arInfo);

    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    const NiRTTI* GetRTTI() const override;
    // Releases the creation info through its virtual deleting destructor when this wrapper made it.
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    ~bhkRagdollConstraint() override;

    // Allocates the constraint, builds its base and its creation info pointer, and counts it.
    static NiObject* CreateObject();

    // Number of live instances; CreateObject counts them.
    static inline uint32_t iCount;
};
