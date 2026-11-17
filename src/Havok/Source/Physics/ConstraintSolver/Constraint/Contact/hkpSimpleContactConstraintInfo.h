#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Reconstruction.hpp"

class hkClass;
class hkTypeInfo;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpContactPointProperties;
struct hkpSimpleContactConstraintAtom;

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

// Builds the Jacobians of a simple contact constraint's points; writeHeaderSchema says
// whether it writes the header schema first (cdecl, 0xD72190; the PDB's types: the flag is an hkBool by value, whose
// byte the callee reads; its callers store the byte into the argument dword).
void hkSimpleContactConstraintDataBuildJacobian(hkpSimpleContactConstraintAtom* atom, const hkpConstraintQueryIn& in,
	hkBool writeHeaderSchemas, hkpConstraintQueryOut& out);
// The PDB's calculateRhs(ccp, solverResultsModified); IDA's name gives float __cdecl
// calculateRhs(const hkContactPoint*, hkpContactPointProperties&, float, float, int&). The exe's copy, which nothing
// calls (hkSimpleContactConstraintDataBuildJacobian inlines it, 0xD72190), is whole-program optimised: the
// properties in ecx (m_internalSolverData at +0x4 read, m_internalDataA at +0x10 read and written, 0xD720A3,
// 0xD72162), the two floats on the stack, caller-cleaned (ret 0), the result in ST0; the contact point and the int
// are gone.
float calculateRhs(hkpContactPointProperties& arProperties, float afDistance, float afDeltaTime);
// Not named in the PDB or IDA; name chosen here. Zeroes the info's words at +0xC, +0x14 and +0x1C (the exe stores
// float 0.0, the same bytes) and returns the pointer it was given in eax.
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

// The PDB's name: the time scale of the linear error recovery.
inline float HK_CONTACT_LINEAR_ERROR_RECOVERY_VELOCITY = 1.0f;

// The PDB's name: the share of the penetration recovered per step.
inline float HK_CONTACT_EXPONENTIAL_ERROR_RECOVERY_VELOCITY = 0.05f;

// The info zeroing at 0xD72090 is declared once, where its reconstructed body is: ConvexHull/Unplaced.h's
// `hkpSimpleContactConstraintDataInfo* clearContactInfo(hkpSimpleContactConstraintDataInfo*)` (the exe returns the argument
// pointer in eax).

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSimpleContactConstraintDataInfoTypeInfo;
