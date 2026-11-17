#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"

class hkpConstraintInstance;

// The header every command in a physics command queue starts with: which command it is.
struct hkpPhysicsCommand {
	enum TYPE : int32_t {
		TYPE_ADD_CONSTRAINT_TO_LOCKED_ISLAND = 0x0,
		TYPE_REMOVE_CONSTRAINT_FROM_LOCKED_ISLAND = 0x1,
		TYPE_END = 0x2,
	};

	hkEnum<TYPE, uint8_t> m_type;
};

// A command with one object.
template <hkpPhysicsCommand::TYPE COMMAND_TYPE, typename OBJECT_TYPE>
struct hkpSingleObjectPhysicsCommand : public hkpPhysicsCommand {
	OBJECT_TYPE m_object;
};

// A command with two objects.
template <hkpPhysicsCommand::TYPE COMMAND_TYPE, typename OBJECT_TYPE0, typename OBJECT_TYPE1>
struct hkpDualObjectPhysicsCommand : public hkpPhysicsCommand {
	OBJECT_TYPE0 m_object0;
	OBJECT_TYPE1 m_object1;
};

// An hkpConstraintInfo that also names its constraint.
struct hkpConstraintInfoExtended : public hkpConstraintInfo {
	hkpConstraintInstance* m_constraint;
};

using hkpAddConstraintToCriticalLockedIslandPhysicsCommand =
	hkpDualObjectPhysicsCommand<hkpPhysicsCommand::TYPE_ADD_CONSTRAINT_TO_LOCKED_ISLAND, hkpConstraintInstance*,
		int32_t>;
using hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand =
	hkpSingleObjectPhysicsCommand<hkpPhysicsCommand::TYPE_REMOVE_CONSTRAINT_FROM_LOCKED_ISLAND, hkpConstraintInstance*>;
