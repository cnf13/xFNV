#pragma once

#include "BSHavok/bhkConstraint.h"

class hkBallAndSocketConstraintCinfo;

// The Gamebryo side of a ball-and-socket constraint. Only its virtuals are declared.
class bhkBallAndSocketConstraint : public bhkConstraint {
public:
    bhkBallAndSocketConstraint();
    bhkBallAndSocketConstraint(const hkBallAndSocketConstraintCinfo& arInfo);
    ~bhkBallAndSocketConstraint() override;

    const NiRTTI* GetRTTI() const override;
    // A clone under the clone critical section, filled through bhkConstraint::CopyCloneMembers.
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void KillCinfo(bool abCreated) override;
    // The cinfo's size, 20 bytes.
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // Builds a bhkBallAndSocketConstraint from NiMemObject's allocator; the stream's create function.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Number of live instances; CreateObject counts them.
    static inline uint32_t iCount;
};

// The creation info of a ball-and-socket constraint. It adds no fields to hkConstraintCinfo (its data is
// hkpBallAndSocketConstraintData).
class hkBallAndSocketConstraintCinfo : public hkConstraintCinfo {
public:
    hkBallAndSocketConstraintCinfo();
    ~hkBallAndSocketConstraintCinfo() override;

    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    uint32_t CreateData() override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    // The pivots' scaling.
    static void ApplyScale(float afScale, hkpConstraintData* apInfo);
};
