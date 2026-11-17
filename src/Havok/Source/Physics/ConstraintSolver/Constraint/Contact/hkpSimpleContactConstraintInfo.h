#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"

class hkClass;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpContactPointProperties;
struct hkpSimpleContactConstraintAtom;
class hkTypeInfo;

// The per-contact-constraint state the simple contact constraint keeps beside its atom (the SDK's
// HK_ENABLE_ROLLING_FRICITON_CODE is off: seven plain words after the flags and the index).
class alignas(16) hkpSimpleContactConstraintDataInfo {
public:
	enum {
		HK_FLAG_OK = 0x0,
		HK_FLAG_POINT_REMOVED = 0x1,
		HK_FLAG_AREA_CHANGED = 0x4,
	};

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint16_t m_flags;
	uint16_t m_index;
	uint32_t m_data[7];
};

// Builds the Jacobians of a simple contact constraint's points; writeHeaderSchemas says whether it writes the header
// schema first.
void hkSimpleContactConstraintDataBuildJacobian(hkpSimpleContactConstraintAtom* atom, const hkpConstraintQueryIn& in,
	hkBool writeHeaderSchemas, hkpConstraintQueryOut& out);
// Reads m_internalSolverData of arProperties and reads and writes its m_internalDataA; the result is returned as a
// float.
float calculateRhs(hkpContactPointProperties& arProperties, float afDistance, float afDeltaTime);
// Zeroes three of the info's words and returns the pointer it was given.
hkpSimpleContactConstraintDataInfo* clearContactInfo(hkpSimpleContactConstraintDataInfo* apInfo);

// The solver's view of an hkpSimpleContactConstraintDataInfo's words (the SDK casts the info to it; the rolling
// friction arm is off): the flags, the friction's binormal axis, the contact radius and the three friction
// results. Members and layout from Havok's release PDBs (sizeof 0x20: +0, +2, +4, +8).
class alignas(16) hkpSimpleContactConstraintDataInfoInternal {
public:
	enum {
		HK_FLAG_OK = 0x0,
		HK_FLAG_POINT_REMOVED = 0x1,
		HK_FLAG_AREA_CHANGED = 0x4,
	};

	uint16_t m_flags;
	uint16_t m_biNormalAxis;
	float m_contactRadius;
	alignas(8) hkpSolverResults m_frictionResults[3];
};

// The time scale of the linear error recovery.
inline float HK_CONTACT_LINEAR_ERROR_RECOVERY_VELOCITY = 1.0f;

// The share of the penetration recovered per step.
inline float HK_CONTACT_EXPONENTIAL_ERROR_RECOVERY_VELOCITY = 0.05f;

// The info zeroing is declared once, in ConvexHull/Unplaced.h: `hkpSimpleContactConstraintDataInfo*
// clearContactInfo(hkpSimpleContactConstraintDataInfo*)`, which returns the argument pointer.

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSimpleContactConstraintDataInfoTypeInfo;
