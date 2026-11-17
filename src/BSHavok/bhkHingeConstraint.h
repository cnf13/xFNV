#pragma once

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"

// The Gamebryo side of a hinge constraint. It adds no fields to bhkConstraint.
class bhkHingeConstraint : public bhkConstraint {
public:
    bhkHingeConstraint();
    bhkHingeConstraint(hkConstraintCinfo* apInfo);
    ~bhkHingeConstraint() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // Makes a hinge constraint with the default constructor.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};

// The creation info of a hinge constraint. It adds no fields to hkConstraintCinfo.
class hkHingeConstraintCinfo : public hkConstraintCinfo {
public:
    hkHingeConstraintCinfo();
    ~hkHingeConstraintCinfo() override;

    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    uint32_t CreateData() override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    void SetInWorldSpace(
        const hkTransform& arBodyATrans,
        const hkTransform& arBodyBTrans,
        const hkVector4& arPivot,
        const hkVector4& arAxis
    );

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Scales the pivots of the hinge data apData by afScale.
    static void ApplyScale(float afScale, hkpConstraintData* apInfo);
};
