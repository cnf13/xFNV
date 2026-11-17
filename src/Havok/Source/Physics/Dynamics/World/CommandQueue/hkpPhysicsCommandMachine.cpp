#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommandMachine.h"

#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommand.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"

// Only the low byte of callbackRequest is ored into the byte-wide m_callbackRequest.
void addConstraintToCriticalLockedIsland(hkpWorld* world, hkpConstraintInstance* constraint, int32_t callbackRequest) {
	hkpWorldOperationUtil::addConstraintToCriticalLockedIsland(world, constraint);
	constraint->m_internal->m_callbackRequest |= static_cast<uint8_t>(callbackRequest);
}

// The command block is raw queue memory: each command takes its size rounded up to 16 bytes (the queue's addCommand,
// HK_NEXT_MULTIPLE_OF(16, sizeof(command))), and each command's fields are read from the bytes after its type. The size
// is added to the start and the addresses are compared unsigned: the bound is computed as an integer, since a pointer
// compare lets clang test the size's sign in place of the addresses.
void hkPhysicsCommandMachineProcess(hkpWorld* world, hkpPhysicsCommand* begin, int32_t size) {
	const uintptr_t end = reinterpret_cast<uintptr_t>(begin) + static_cast<uintptr_t>(size);
	for (uint8_t* command = reinterpret_cast<uint8_t*>(begin); reinterpret_cast<uintptr_t>(command) < end;) {
		switch (reinterpret_cast<hkpPhysicsCommand*>(command)->m_type.m_storage) {
		case hkpPhysicsCommand::TYPE_ADD_CONSTRAINT_TO_LOCKED_ISLAND: {
			hkpAddConstraintToCriticalLockedIslandPhysicsCommand* com =
				reinterpret_cast<hkpAddConstraintToCriticalLockedIslandPhysicsCommand*>(command);
			addConstraintToCriticalLockedIsland(world, com->m_object0, com->m_object1);
			command += (sizeof(hkpAddConstraintToCriticalLockedIslandPhysicsCommand) + 15) & ~size_t{15};
			break;
		}
		case hkpPhysicsCommand::TYPE_REMOVE_CONSTRAINT_FROM_LOCKED_ISLAND: {
			hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand* com =
				reinterpret_cast<hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand*>(command);
			hkpWorldOperationUtil::removeConstraintFromCriticalLockedIsland(world, com->m_object);
			command += (sizeof(hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand) + 15) & ~size_t{15};
			break;
		}
		default:
			return;
		}
	}
}
