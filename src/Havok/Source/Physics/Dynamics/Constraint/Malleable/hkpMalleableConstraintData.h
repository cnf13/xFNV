#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

#include <stdint.h>

void cleanupLoadedObjecthkpMalleableConstraintData(void* p);
// The type info's finish-loaded-object callback.
void finishLoadedObjecthkpMalleableConstraintData(void* p, int32_t finishing);

// A constraint that wraps another constraint's data and scales how strongly it holds (m_strength). The exe's vtable
// has 16 slots (the destructor, getClassType, calcContentStatistics and the constraint data interface); it
// overrides the destructor, isValid, getType, getRuntimeInfo, buildJacobian and getConstraintInfo.
class hkpMalleableConstraintData : public hkpConstraintData {
public:
	// The reflection's member table (the PDB's hkpMalleableConstraintData::Members).
	using MembersArray = const hkClassMember[3];

	hkpMalleableConstraintData();
	hkpMalleableConstraintData(hkFinishLoadedObjectFlag aFlag);
	hkpMalleableConstraintData(hkpConstraintData* constraintData);
	~hkpMalleableConstraintData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void buildJacobian(const hkpConstraintQueryIn& arInfo, hkpConstraintQueryOut& out) override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;

	// The constraint data this one wraps.
	hkpConstraintData* getWrappedConstraintData();
	// The same, for a const constraint: a second copy of the body in the exe (thiscall, ret 0, 0xCDF6D0).
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
inline constexpr fnvd::Vtable hkpMalleableConstraintData_vftable{};
