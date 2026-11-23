#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"

// A constraint built from generic constraint data. Only its virtual table is modelled.
class bhkGenericConstraint : public bhkConstraint {
public:
    bhkGenericConstraint();
    ~bhkGenericConstraint() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    static NiRTTI ms_RTTI;
    // Number of live instances; the constructors count them.
    static inline uint32_t iCount;
};

// The creation info of a generic constraint. It adds no fields to hkConstraintCinfo. LoadBinary and SaveBinary use
// hkConstraintCinfo's.
class hkGenericConstraintCinfo : public hkConstraintCinfo {
public:
    hkGenericConstraintCinfo();
    ~hkGenericConstraintCinfo() override;

    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    uint32_t CreateData() override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};
