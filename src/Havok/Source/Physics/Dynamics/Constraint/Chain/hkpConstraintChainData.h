#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// The base of the constraint data of a chain of constraints: the data adds how many constraints the chain holds.
class hkpConstraintChainData : public hkpConstraintData {
public:
	hkpConstraintChainData() = default;
	hkpConstraintChainData(hkFinishLoadedObjectFlag flag) : hkpConstraintData(flag) {}

	virtual int32_t getNumConstraintInfos() = 0;
};

// The class's reflection record.
extern hkClass hkpConstraintChainDataClass;
