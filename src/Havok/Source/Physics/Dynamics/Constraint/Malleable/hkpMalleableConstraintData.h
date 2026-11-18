#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <stdint.h>

void cleanupLoadedObjecthkpMalleableConstraintData(void* p);
// The type info's finish-loaded-object callback.
void finishLoadedObjecthkpMalleableConstraintData(void* p, int32_t finishing);

// A constraint that wraps another constraint's data and scales how strongly it holds (m_strength). It overrides the
// destructor, isValid, getType, getRuntimeInfo, buildJacobian and getConstraintInfo.
class hkpMalleableConstraintData : public hkpConstraintData {
public:
    // The reflection's member table (hkpMalleableConstraintData::Members).
    using MembersArray = const hkClassMember[3];

    hkpMalleableConstraintData();
    hkpMalleableConstraintData(hkFinishLoadedObjectFlag aFlag);
    hkpMalleableConstraintData(hkpConstraintData* constraintData);
    ~hkpMalleableConstraintData() override;

    hkBool isValid() const override;
    int32_t getType() const override;
    void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
    void buildJacobian(const hkpConstraintQueryIn& arInfo, hkpConstraintQueryOut& out) override;
    void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;

    // The constraint data this one wraps.
    hkpConstraintData* getWrappedConstraintData();
    // The same, for a const constraint.
    const hkpConstraintData* getWrappedConstraintData() const;
    void setStrength(const float a1);
    float getStrength() const;
    void SetWrappedConstraintData(hkpConstraintData* apData);

    static const hkClass& staticClass();

    hkpConstraintData* m_constraintData;
    hkpBridgeAtoms m_atoms;
    float m_strength;

    static MembersArray Members;
};

extern hkClass hkpMalleableConstraintDataClass;
extern hkTypeInfo hkpMalleableConstraintDataTypeInfo;
