#pragma once

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpBodyOperation.h"
#include "Reconstruction.hpp"

class hkAabb;
class hkpAction;
class hkpConstraintCollisionFilter;
class hkpConstraintInstance;
class hkpEntity;
// Defined in hkpWorld.h, which includes this header.
enum hkpEntityActivation : int32_t;
class hkpPhantom;
class hkpRigidBody;
class hkpShape;
enum hkpUpdateCollectionFilterMode : int32_t;
enum hkpUpdateCollisionFilterOnEntityMode : int32_t;
enum hkpUpdateCollisionFilterOnWorldMode : int32_t;
class hkpWorld;
class hkpWorldObject;

// The deferred world operations the queue stores. They carry no vtable.
namespace hkWorldOperation {
// Whether an operation ran at once or was queued until the world's critical operations are unlocked: the
// PDB's enum (underlying int); hkpRigidBody::setShape returns 0 or 1 in eax (0xCA84B0).
enum Result : int32_t {
	POSTPONED = 0,
	DONE = 1,
};

// The operation types src uses; the values are the exe's (activateAsCriticalOperation stores 0x21,
// deactivateAsCriticalOperation 0x22, queueCallback 0x24). The rest follow the PDB's enum order after
// OPERATION_ID_INVALID, whose zero ENTITY_ADD follows: ENTITY_REMOVE is 2, CONSTRAINT_ADD 8, PHANTOM_ADD 13 and
// PHANTOM_REMOVE 14.
enum Type : uint8_t {
	// BiggestOperation's constructor stores 0 (hkpWorldOperationQueue::queueOperation 0xCF7690).
	OPERATION_ID_INVALID = 0x0,
	// The SDK's ENTITY_ADD (hkpWorld::addEntity stores 1).
	ENTITY_ADD = 0x1,
	ENTITY_REMOVE = 0x2,
	ENTITY_UPDATE_BROAD_PHASE = 0x3,
	// hkpRigidBody::setMotionType stores 4.
	RIGIDBODY_SET_MOTION_TYPE = 0x4,
	// hkpRigidBody::setShape stores 5.
	WORLD_OBJECT_SET_SHAPE = 0x5,
	ENTITY_BATCH_ADD = 0x6,
	ENTITY_BATCH_REMOVE = 0x7,
	CONSTRAINT_ADD = 0x8,
	CONSTRAINT_REMOVE = 0x9,
	ACTION_ADD = 0xA,
	ACTION_REMOVE = 0xB,
	// hkpWorldOperationUtil::mergeIslands stores 0xC.
	ISLAND_MERGE = 0xC,
	PHANTOM_ADD = 0xD,
	PHANTOM_REMOVE = 0xE,
	PHANTOM_BATCH_ADD = 0xF,
	PHANTOM_BATCH_REMOVE = 0x10,
	// hkpPhantom::updateBroadPhase stores 0x11.
	PHANTOM_UPDATE_BROAD_PHASE = 0x11,
	UPDATE_FILTER_ENTITY = 0x12,
	UPDATE_FILTER_ENTITY_PAIR = 0x13,
	UPDATE_FILTER_PHANTOM = 0x14,
	UPDATE_FILTER_WORLD = 0x15,
	// hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody stores 0x16.
	UPDATE_MOVED_BODY_INFO = 0x16,
	ENTITY_BATCH_REINTEGRATE_AND_RECOLLIDE = 0x17,
	RIGIDBODY_SET_POSITION_AND_ROTATION = 0x18,
	RIGIDBODY_SET_LINEAR_VELOCITY = 0x19,
	// hkpRigidBody::setAngularVelocityAsCriticalOperation stores 0x1A.
	RIGIDBODY_SET_ANGULAR_VELOCITY = 0x1A,
	RIGIDBODY_APPLY_LINEAR_IMPULSE = 0x1B,
	RIGIDBODY_APPLY_POINT_IMPULSE = 0x1C,
	RIGIDBODY_APPLY_ANGULAR_IMPULSE = 0x1D,
	WORLD_OBJECT_ADD_REFERENCE = 0x1E,
	WORLD_OBJECT_REMOVE_REFERENCE = 0x1F,
	ACTIVATE_REGION = 0x20,
	ACTIVATE_ENTITY = 0x21,
	DEACTIVATE_ENTITY = 0x22,
	CONSTRAINT_FILTER_BROKEN_EVENT = 0x23,
	USER_CALLBACK = 0x24,
};

struct BaseOperation {
	hkEnum<Type, uint8_t> m_type;
};

struct AddEntity : BaseOperation {
	hkpEntity* m_entity;
	hkpEntityActivation m_activation;
};

struct MergeIslands : BaseOperation {
	hkpEntity* m_entities[2];
};

struct ActivateEntity : BaseOperation {
	hkpEntity* m_entity;
};

struct DeactivateEntity : BaseOperation {
	hkpEntity* m_entity;
};

struct RemoveEntity : BaseOperation {
	hkpEntity* m_entity;
};

struct AddPhantom : BaseOperation {
	hkpPhantom* m_phantom;
};

struct RemovePhantom : BaseOperation {
	hkpPhantom* m_phantom;
};

struct AddConstraint : BaseOperation {
	hkpConstraintInstance* m_constraint;
};

// What hkpWorld::reenableCollisionBetweenEntityPair queues (UPDATE_FILTER_ENTITY_PAIR).
struct UpdateFilterOnEntityPair : BaseOperation {
	hkpEntity* m_entityA;
	hkpEntity* m_entityB;
};

struct SetWorldObjectShape : BaseOperation {
	hkpWorldObject* m_worldObject;
	const hkpShape* m_shape;
};

// The motion type change hkpRigidBody::setMotionType queues (0xCA8420 stores the three enums as bytes).
struct SetRigidBodyMotionType : BaseOperation {
	hkpRigidBody* m_rigidBody;
	uint8_t m_motionType;
	uint8_t m_activation;
	uint8_t m_collisionFilterUpdateMode;
};

// The angular velocity hkpRigidBody::setAngularVelocityAsCriticalOperation queues (0xCA8A30: three floats at 8).
struct SetRigidBodyAngularVelocity : BaseOperation {
	hkpRigidBody* m_rigidBody;
	float m_angularVelocity[3];
};

// The operations below are the Havok SDK's; each queuing function stores its Type in m_type and the members at
// the offsets pinned (hkpWorld::addEntityBatch 0xC94BD0: the count as a word at +8 and the activation byte at
// +0xA; hkpWorld::reintegrateAndRecollideEntities 0xC91470: the mode byte at +0xA; hkpWorld::
// updateCollisionFilterOnWorld 0xC96FF0: bytes at +1 and +2; hkpRigidBody::applyLinearImpulseAsCriticalOperation
// 0xCA8A80: three floats at +8; hkpConstraintCollisionFilter::constraintBreakingCallback 0xD87F00: the filter at
// +4, the constraint at +8, the bool at +0xC).
struct AddEntityBatch : BaseOperation {
	hkpEntity** m_entities;
	// SDK: hkObjectIndex (hkUint16).
	uint16_t m_numEntities;
	hkEnum<hkpEntityActivation, uint8_t> m_activation;
};

struct RemoveEntityBatch : BaseOperation {
	hkpEntity** m_entities;
	// SDK: hkObjectIndex (hkUint16).
	uint16_t m_numEntities;
};

struct RemoveConstraint : BaseOperation {
	hkpConstraintInstance* m_constraint;
};

struct AddAction : BaseOperation {
	hkpAction* m_action;
};

struct RemoveAction : BaseOperation {
	hkpAction* m_action;
};

struct AddPhantomBatch : BaseOperation {
	hkpPhantom** m_phantoms;
	// SDK: hkObjectIndex (hkUint16).
	uint16_t m_numPhantoms;
};

struct RemovePhantomBatch : BaseOperation {
	hkpPhantom** m_phantoms;
	// SDK: hkObjectIndex (hkUint16).
	uint16_t m_numPhantoms;
};

struct UpdateEntityBP : BaseOperation {
	hkpEntity* m_entity;
};

struct UpdatePhantomBP : BaseOperation {
	hkpPhantom* m_phantom;
	hkAabb* m_aabb;
};

struct UpdateFilterOnEntity : BaseOperation {
	hkpEntity* m_entity;
	hkEnum<hkpUpdateCollisionFilterOnEntityMode, uint8_t> m_collisionFilterUpdateMode;
	hkEnum<hkpUpdateCollectionFilterMode, uint8_t> m_updateShapeCollections;
};

struct UpdateFilterOnPhantom : BaseOperation {
	hkpPhantom* m_phantom;
	hkEnum<hkpUpdateCollectionFilterMode, uint8_t> m_updateShapeCollections;
};

struct UpdateFilterOnWorld : BaseOperation {
	hkEnum<hkpUpdateCollisionFilterOnWorldMode, uint8_t> m_collisionFilterUpdateMode;
	hkEnum<hkpUpdateCollectionFilterMode, uint8_t> m_updateShapeCollections;
};

struct ReintegrateAndRecollideEntityBatch : BaseOperation {
	hkpEntity** m_entities;
	// SDK: hkObjectIndex (hkUint16).
	uint16_t m_numEntities;
	uint8_t m_mode;
	hkEnum<hkpEntityActivation, uint8_t> m_activation;
};

struct UpdateMovedBodyInfo : BaseOperation {
	hkpEntity* m_entity;
};

// m_positionAndRotation points at the position and the rotation, two hkVector4s.
struct SetRigidBodyPositionAndRotation : BaseOperation {
	hkpRigidBody* m_rigidBody;
	hkVector4* m_positionAndRotation;
};

struct SetRigidBodyLinearVelocity : BaseOperation {
	hkpRigidBody* m_rigidBody;
	float m_linearVelocity[3];
};

struct ApplyRigidBodyLinearImpulse : BaseOperation {
	hkpRigidBody* m_rigidBody;
	float m_linearImpulse[3];
};

// m_pointAndImpulse points at the point and the impulse, two hkVector4s.
struct ApplyRigidBodyPointImpulse : BaseOperation {
	hkpRigidBody* m_rigidBody;
	hkVector4* m_pointAndImpulse;
};

struct ApplyRigidBodyAngularImpulse : BaseOperation {
	hkpRigidBody* m_rigidBody;
	float m_angularImpulse[3];
};

struct AddReference : BaseOperation {
	hkpWorldObject* m_worldObject;
};

struct RemoveReference : BaseOperation {
	hkpWorldObject* m_worldObject;
};

// hkpWorld::activateRegion copies the 0x20-byte box to a heap block it points m_aabb at.
struct ActivateRegion : BaseOperation {
	hkAabb* m_aabb;
};

struct ConstraintCollisionFilterConstraintBroken : BaseOperation {
	hkpConstraintCollisionFilter* m_filter;
	hkpConstraintInstance* m_constraintInstance;
	hkBool m_constraintBroken;
};

// What the world calls back once its critical operations are unlocked (hkpWorld::queueCallback). It has no exe
// vtable of its own (abstract, no RTTI): worldOperationUserCallback, slot 3, is pure (PC PDB).
class UserCallback : public hkReferencedObject {
public:
	~UserCallback() override;

	virtual void worldOperationUserCallback(hkUlong userData) = 0;
};

struct UserCallbackOperation : BaseOperation {
	UserCallback* m_userCallback;
	hkUlong m_userData;
};

// The largest operation, which the queue keeps any operation in: BaseOperation's type byte, then the SDK's
// dummy of four hkUlongs (sizeof 20 in the exe, which reserves 20 bytes an entry, queueOperation 0xCF7690; 40 on
// x64, where the operations' pointers are 8 bytes wide and MergeIslands alone is 24).
struct BiggestOperation : BaseOperation {
	hkUlong dummy[4];
};

// Every operation is copied into a BiggestOperation slot, on both widths.
static_assert(sizeof(MergeIslands) <= sizeof(BiggestOperation));
static_assert(sizeof(ConstraintCollisionFilterConstraintBroken) <= sizeof(BiggestOperation));
static_assert(sizeof(SetRigidBodyAngularVelocity) <= sizeof(BiggestOperation));
static_assert(sizeof(UserCallbackOperation) <= sizeof(BiggestOperation));
}
namespace hkAlgorithm {
// The PDB's quickSortRecursive<hkWorldOperation::BiggestOperation, hkBool (__cdecl*)(const
// hkWorldOperation::BiggestOperation&, const hkWorldOperation::BiggestOperation&)> as the exe compiled it (cdecl,
// ret 0, 0xCF8510): sorts the pending island merges apArr[aiLow..aiHigh] in place (inclusive bounds). The comparator
// (islandLess) returns its hkBool through the hidden result pointer. eax is left holding the low bound of the last
// pass, or the last recursive call's eax.
int32_t quickSortRecursive(hkWorldOperation::BiggestOperation* apArr, int32_t aiLow, int32_t aiHigh,
	hkBool* (*cmpLess)(hkBool*, const hkWorldOperation::BiggestOperation&, const hkWorldOperation::BiggestOperation&));
} // namespace hkAlgorithm

// One deferred operation on an entity.
struct hkpBodyOperationEntry {
	hkpEntity* m_entity;
	hkpBodyOperation* m_operation;
	uint32_t m_priority;
	hkpBodyOperation::ExecutionState m_hint;
};

bool hkpWorldBodyOperationQueueSortCmp(const hkpBodyOperationEntry& a, const hkpBodyOperationEntry& b);

// The PDB's quickSortRecursive<hkpBodyOperationEntry, bool (__cdecl*)(const hkpBodyOperationEntry&, const
// hkpBodyOperationEntry&)> (cdecl, 0xCF83C0), forwarded from hkpWorldOperationQueue.cpp.
using hkpBodyOperationEntryLess = bool (*)(const hkpBodyOperationEntry&, const hkpBodyOperationEntry&);
namespace hkAlgorithm {}

// The world's queue of deferred operations: the operations waiting to run, the pending island merges and the
// pending body operations. The element types are laid out above.
class hkpWorldOperationQueue {
public:
	hkpWorldOperationQueue(hkpWorld* a1);

	// Defers one operation on an entity: the entry takes the four words in order and the operation keeps two
	// references.
	void queueBodyOperation(hkpEntity* entity, hkpBodyOperation* operation, int32_t priority,
		hkpBodyOperation::ExecutionState hint);
	// Runs the deferred body operations, in priority order, until none are queued.
	void executeAllPendingBodyOperations();
	// Runs the operations queued while the world was locked, and what they queue in turn.
	void executeAllPending();
	void queueOperation(const hkWorldOperation::BaseOperation& operation);

	hkArray<hkWorldOperation::BiggestOperation> m_pending;
	hkpWorld* m_world;
	hkArray<hkWorldOperation::BiggestOperation> m_islandMerges;
	hkArray<hkpBodyOperationEntry> m_pendingBodyOperations;
};

// A link of the world's chain of pending operation queues (hkpWorld::m_pendingOperationQueues): the queue's
// operations, the next one to run and its neighbours. Members from the Havok SDK and the PDB;
// hkpWorld::calcContentStatistics (0xC92DB0) walks the chain through m_nextQueue (+8) and reports 16 bytes a link.
struct hkpDebugInfoOnPendingOperationQueues {
	hkArray<hkWorldOperation::BiggestOperation>* m_pending;
	int32_t m_nextPendingOperationIndex;
	hkpDebugInfoOnPendingOperationQueues* m_nextQueue;
	hkpDebugInfoOnPendingOperationQueues* m_prevQueue;
};
