#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"

// The wrapper of an hkpBreakableConstraintData, a constraint that breaks past an impulse limit. Only its RTTI and
// the virtuals it overrides are modelled.
class bhkBreakableConstraint : public bhkConstraint {
public:
    bhkBreakableConstraint();
    ~bhkBreakableConstraint() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // The class's factory: a new default-built breakable constraint.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Number of live instances; the constructors count them.
    static inline uint32_t iCount;
};

// The creation info of a breakable constraint. It adds no fields to hkConstraintCinfo.
class hkBreakableConstraintCinfo : public hkConstraintCinfo {
public:
    hkBreakableConstraintCinfo();
    ~hkBreakableConstraintCinfo() override;

    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    uint32_t CreateData() override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};

// The game's breakable constraint data: hkpBreakableConstraintData with the game's defaults. It adds no data and
// overrides none of hkpBreakableConstraintData's virtual functions.
class ahkpBreakableConstraintData : public hkpBreakableConstraintData {
public:
    ahkpBreakableConstraintData();
};
