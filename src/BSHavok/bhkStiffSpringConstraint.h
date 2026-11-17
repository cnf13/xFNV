#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"

// The creation info of a stiff spring constraint. It adds no fields to hkConstraintCinfo.
class hkStiffSpringConstraintCinfo : public hkConstraintCinfo {
public:
    hkStiffSpringConstraintCinfo();
    ~hkStiffSpringConstraintCinfo() override;

    // hkConstraintCinfo overrides.
    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    uint32_t CreateData() override;

    // The info is freed through MemoryManager::Deallocate.
    static void operator delete(void* apMemory);
};

// A constraint that keeps two bodies at a fixed distance. Only its RTTI is modelled; it adds no members that are read.
class bhkStiffSpringConstraint : public bhkConstraint {
public:
    bhkStiffSpringConstraint();
    bhkStiffSpringConstraint(const hkStiffSpringConstraintCinfo* apInfo);
    ~bhkStiffSpringConstraint() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // Allocates the constraint, builds its base and its creation info pointer, and counts it.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Number of live instances; CreateObject and CreateClone count them.
    static inline uint32_t iCount;
};
