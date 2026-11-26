#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"

// Bethesda's hkpMalleableConstraintData: the same layout and vtable, with the engine's own constructor and setup
// functions.
class ahkpMalleableConstraintData : public hkpMalleableConstraintData {
public:
    ahkpMalleableConstraintData();

    void SetChild(hkpConstraintData* apData);
    hkpConstraintData* GetChild() {
        return m_constraintData;
    }
};

// The creation info of a malleable constraint: hkConstraintCinfo's four members and its own overrides of the five
// serialization functions (the destructor is the base's).
class hkMalleableConstraintCinfo : public hkConstraintCinfo {
public:
    hkMalleableConstraintCinfo();

    void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    // Creates the creation info's malleable constraint data when it has none, under the Havok heap context, and
    // returns the data's size.
    uint32_t CreateData() override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    void CA9E50(hkpConstraintData* apData);
    // Stores afStrength in the malleable data when there is data.
    void SetStrength(float afStrength);
    // The malleable data's strength, or 0 without data.
    float GetStrength() {
        ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
        return pd ? pd->m_strength : 0.0f;
    }
    // The constraint data the malleable data wraps, or null without data.
    hkpConstraintData* GetWrappedConstraintData() {
        ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
        return pd ? pd->GetChild() : nullptr;
    }

    // Scales the data apInfo's malleable data wraps by afScale.
    static void ApplyScale(float afScale, hkpConstraintData* apInfo);
};

// The wrapper of an hkpMalleableConstraintData, a constraint whose strength is scaled. Only its RTTI and the
// virtuals it overrides are modelled.
class bhkMalleableConstraint : public bhkConstraint {
public:
    // The constructor from a creation info.
    bhkMalleableConstraint();
    bhkMalleableConstraint(const hkMalleableConstraintCinfo& arInfo);
    ~bhkMalleableConstraint() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // The constraint's creation info.
    hkMalleableConstraintCinfo& GetCinfo();

    // Creates an uninitialised instance for the stream's factory.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Number of live instances; the constructors count them.
    static inline uint32_t iCount;
};

inline hkMalleableConstraintCinfo& bhkMalleableConstraint::GetCinfo() {
    bool bc;
    hkSerializableCinfo* pCinfo = &CreateCinfo(bc);
    return *static_cast<hkMalleableConstraintCinfo*>(pCinfo);
}
