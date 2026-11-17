#pragma once

#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

// The angular limits of a hinge, used as a link of a constraint chain. The exe's vtable has 16 slots; it overrides
// isValid, getType, getRuntimeInfo and getConstraintInfo; the destructor is hkBaseObject's.
class hkpHingeLimitsData : public hkpConstraintData {
public:
	// The runtime memory of one instance (getRuntimeInfo 0xD52F30's size, 28).
	struct Runtime {
		hkpSolverResults m_solverResults[3];
		float m_lastAngle;
	};

	// The atoms the constraint is made of (the PDB's members; the exe's hkClass hkpHingeLimitsDataAtomsClass has
	// m_angLimit at 0x70, m_2dAng at 0x80 and an object size of 0x90).
	struct Atoms {
		// Values from the Havok SDK.
		enum Axis : int32_t {
			AXIS_AXLE = 0x0,
			AXIS_PERP_TO_AXLE_1 = 0x1,
			AXIS_PERP_TO_AXLE_2 = 0x2,
		};

		Atoms() = default;
		// The SDK's finish constructor: the loaded atoms keep their fields.
		Atoms(hkFinishLoadedObjectFlag) {}

		// The reflection class hkpHingeLimitsDataAtomsClass.
		static const hkClass& staticClass();

		hkpSetLocalRotationsConstraintAtom m_rotations;
		hkpAngLimitConstraintAtom m_angLimit;
		hkp2dAngConstraintAtom m_2dAng;
	};

	// The PDB's constructor: the vtable, a reference count of 1, the atoms' types (+0x10: 4,
	// +0x80: 14, +0x90: 12), identity rotations at +0x20 and +0x50, the angle limits -pi..pi with strength 1.0
	// (thiscall, ret 0, 0xD52FC0; callers hkHingeLimitsConstraintCinfo::CreateData 0xCC86C0 and CloneData
	// 0xCC85B0).
	hkpHingeLimitsData();
	hkpHingeLimitsData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {}

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// The PDB's setInBodySpace: rotation A from axisA (+0x20) and axisAPerp (+0x30), rotation
	// B from axisB (+0x50) and axisBPerp (+0x60), each normalised, the third columns their cross products
	// (thiscall, ret 0x10, 0xD52CE0; parameter names from the SDK).
	void setInBodySpace(const hkVector4& axisA, const hkVector4& axisB, const hkVector4& axisAPerp,
		const hkVector4& axisBPerp);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary (the exe's hkClass hkpHingeLimitsDataClass has m_atoms at 0x10 and an
	// object size of 0xA0), so the 4 bytes after the base class are the compiler's alignment padding.
	alignas(16) Atoms m_atoms;
};

inline constexpr fnvd::Vtable hkpHingeLimitsData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpHingeLimitsDataAtomsTypeInfo;
extern hkClass hkpHingeLimitsDataAtomsClass;
extern hkClass hkpHingeLimitsDataClass;
extern hkTypeInfo hkpHingeLimitsDataTypeInfo;
