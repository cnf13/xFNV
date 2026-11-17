#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianBallSocketChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianShiftSolverResultsPointerSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianStiffSpringChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"

class hkpConstraintInstance;
class hkpSolverResults;

// The schema that starts every constraint's Jacobians: the two bodies' accumulator indices, the solver results'
// striding and runtime, and the constraint. It takes HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA bytes of the stream: one 16-byte
// unit for 4-byte pointers, two for 8-byte pointers (the 16-byte alignment pads its 24 bytes to 32).
class alignas(16) hkpJacobianHeaderSchema {
public:
	uint8_t m_padding0;
	uint8_t m_solverResultStriding;
	uint16_t m_padding1;
	uint16_t m_bodyAIndex;
	uint16_t m_bodyBIndex;
	hkpSolverResults* m_solverResultInMainMemory;
	hkpConstraintInstance* m_constraintInstance;
};

static_assert(sizeof(hkpJacobianHeaderSchema) == HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA);

// One 16-byte unit of the Jacobian schema stream the constraint builders write (hkpConstraintQueryOut). The class has
// no data members of its own (its static getType/setType read and write the first byte); the views here are the ones
// the builders write. A schema's first byte is its type; the other bytes of the first lane belong to the schema. Before
// storing the type, HK_REAL_EPSILON is added to the float the first lane holds (whatever the buffer held there is read,
// as the game does), so the first lane is also seen as a float.
class alignas(16) hkpJacobianSchema {
public:
	// Stored as an hkEnum<SchemaType, hkInt8>; the type is written as one byte.
	enum SchemaType : uint8_t {
		SCHEMA_TYPE_END = 0,
		SCHEMA_TYPE_HEADER = 1,
		SCHEMA_TYPE_GOTO = 2,
		SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER = 3,
		SCHEMA_TYPE_SIMPLE_BEGIN = 4,
		SCHEMA_TYPE_1D_BILATERAL = 5,
		SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT = 6,
		SCHEMA_TYPE_1D_BILATERAL_USER_TAU = 7,
		SCHEMA_TYPE_1D_LINEAR_LIMITS = 8,
		SCHEMA_TYPE_1D_FRICTION = 9,
		SCHEMA_TYPE_1D_LINEAR_MOTOR = 10,
		SCHEMA_TYPE_1D_PULLEY = 11,
		SCHEMA_TYPE_1D_ANGULAR = 12,
		SCHEMA_TYPE_1D_ANGULAR_LIMITS = 13,
		SCHEMA_TYPE_1D_ANGULAR_FRICTION = 14,
		SCHEMA_TYPE_1D_ANGULAR_MOTOR = 15,
		SCHEMA_TYPE_SINGLE_CONTACT = 16,
		SCHEMA_TYPE_SINGLE_CONTACT_WITH_ACCESSOR_CHECK = 17,
		SCHEMA_TYPE_SINGLE_LIMIT_CONTACT = 18,
		SCHEMA_TYPE_SINGLE_LIMIT_CONTACT_WITH_ACCESSOR_CHECK = 19,
		SCHEMA_TYPE_PAIR_CONTACT = 20,
		SCHEMA_TYPE_PAIR_CONTACT_WITH_ACCESSOR_CHECK = 21,
		SCHEMA_TYPE_2D_FRICTION = 22,
		SCHEMA_TYPE_3D_FRICTION = 23,
		SCHEMA_TYPE_2D_ROLLING_FRICTION = 24,
		SCHEMA_TYPE_SET_MASS = 25,
		SCHEMA_TYPE_ADD_VELOCITY = 26,
		SCHEMA_TYPE_SET_CENTER_OF_MASS = 27,
		SCHEMA_TYPE_SIMPLE_END = 28,
		SCHEMA_TYPE_STIFF_SPRING_CHAIN = 29,
		SCHEMA_TYPE_BALL_SOCKET_CHAIN = 30,
		SCHEMA_TYPE_POWERED_CHAIN = 31,
		SCHEMA_TYPE_MAX = 32,
	};

	// Static tables, indexed by the schema type: each simple schema's size in bytes and the number of hkpSolverElemTemp
	// it uses (0xFF for the types that have none).
	using SchemaTypeTable = uint8_t[SCHEMA_TYPE_MAX];

	// The header schema's view (type 1) of the stream at this unit: the stream is raw memory the builders lay out by
	// byte offsets, hence the casts.
	hkpJacobianHeaderSchema& getHeader() { return *reinterpret_cast<hkpJacobianHeaderSchema*>(this); }

	const hkpJacobianHeaderSchema& getHeader() const { return *reinterpret_cast<const hkpJacobianHeaderSchema*>(this); }

	union {
		hkQuadReal m_quad;

		struct {
			uint8_t m_type;
			// The rest of the 16-byte unit is the schema's own payload (the views below; the pulley, limit, motor,
			// friction and chain schemas are wider than one unit and have their own headers in this folder).
			uint8_t unk01[15];
		} m_bytes;

		// The header schema (type 1) is no view of the unit: it is wider than one unit for 8-byte pointers
		// (getHeader).
		// The shift schema's view (type 3).
		hkpJacobianShiftSolverResultsPointerSchema m_shiftSolverResults;
		// The chain schemas' heads (types 30 and 29); their first word is the type byte's lane.
		hkpJacobianBallSocketChainSchema m_ballSocketChain;
		hkpJacobianStiffSpringChainSchema m_stiffSpringChain;
	};

	// Each simple schema's size in bytes, by schema type (0xFF: a type with no fixed size). The header (type 1), the 3D
	// friction (23) and the 2D rolling friction (24) schemas are wider for 8-byte pointers (hkpSolve.h).
	static inline SchemaTypeTable s_schemaSize{0x10, HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA, 0x10, 0x10, 0xFF, 0x30, 0x30,
		0x40, 0x40, 0x40, 0x50, 0x40, 0x20, 0x30, 0x30, 0x40, 0x30, 0x30, 0x30, 0x30, 0x70, 0x70, 0x70,
		HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA, HK_SIZE_OF_JACOBIAN_2D_ROLLING_FRICTION_SCHEMA, 0x20, 0x20, 0x60, 0xFF,
		0xFF, 0xFF, 0xFF};
	// The number of hkpSolverElemTemp each simple schema uses, by schema type (0xFF: a type with none).
	static inline SchemaTypeTable s_schemaNumSolverElemTemps{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x02,
		0x03, 0x01, 0x01, 0x01, 0x02, 0x03, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x03, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF};
};

// The stream's unit: every builder and solver steps through the schemas by it, whatever the pointer size.
static_assert(sizeof(hkpJacobianSchema) == 16);

// The schemas below have no header of their own in the SDK's library (its debug info names none); members from
// hkpconstraintsolver.pdb, kept beside the schema stream they are part of.

// Jumps the solver m_offset bytes on in the schema stream.
class hkpJacobianGotoSchema {
public:
	uint32_t m_padding0;
	uint32_t m_offset;
	uint32_t m_padding1[2];
};

// Adds m_deltaVel to the velocity of body m_bodyIndex.
class hkpJacobianAddVelocitySchema {
public:
	hkVector4 m_deltaVel;
	int32_t m_bodyIndex;
};

// Overrides the two bodies' inverse masses (and inertias) for the constraints that follow.
class hkpJacobianSetMassSchema {
public:
	hkVector4 m_invMass0;
	hkVector4 m_invMass1;
};

// Maps the two bodies' angular velocities to the linear velocity of a shifted center of mass.
class hkpJacobianSetCenterOfMassSchema {
public:
	hkMatrix3 m_angToLinVelA;
	hkMatrix3 m_angToLinVelB;
};
