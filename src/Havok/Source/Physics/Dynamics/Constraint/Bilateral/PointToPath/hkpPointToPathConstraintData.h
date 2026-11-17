#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

#include <stdint.h>

void cleanupLoadedObjecthkpPointToPathConstraintData(void* p);
// The type info's finish-loaded-object callback for hkpPointToPathConstraintData.
void finishLoadedObjecthkpPointToPathConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpPointToPathConstraintData();

// A constraint that keeps a pivot of one body on a curve (the path) and, as chosen, its orientation to the path. The
// exe's vtable has 16 slots; it overrides the destructor, isValid, getType, getRuntimeInfo, buildJacobian and
// getConstraintInfo.
class hkpPointToPathConstraintData : public hkpConstraintData {
public:
	// Values from the Xbox PDB.
	enum OrientationConstraintType : int32_t {
		CONSTRAIN_ORIENTATION_INVALID = 0x0,
		CONSTRAIN_ORIENTATION_NONE = 0x1,
		CONSTRAIN_ORIENTATION_ALLOW_SPIN = 0x2,
		CONSTRAIN_ORIENTATION_TO_PATH = 0x3,
		CONSTRAIN_ORIENTATION_MAX_ID = 0x4,
	};

	// The runtime memory of one instance (getRuntimeInfo 0xDEE720's size, 60).
	struct Runtime {
		hkpSolverResults m_solverResults[7];
		float m_parametricPosition;
	};

	// The reflected members (the PDB's hkpPointToPathConstraintData::Members). The exe keeps them in .data: its
	// initialiser 0xFC5220 stores the OrientationConstraintType enum into m_angularConstrainedDOF's row.
	using MembersArray = const hkClassMember[5];

	hkpPointToPathConstraintData();

	// The SDK's finish constructor: when finishing, the bridge atom is initialised again with the data it holds.
	hkpPointToPathConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {
		if (aFlag.m_finishing) {
			m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
		}
	}

	~hkpPointToPathConstraintData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;

	// Takes a reference on apPath and releases the old path.
	void setPath(hkpParametricCurve* path);
	void setInWorldSpace(const hkTransform& ws_T_Aos, const hkTransform& ws_T_Ros, const hkVector4& pivotWs,
		hkpParametricCurve* path, const hkTransform& os_T_Rks);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, hkpParametricCurve* path);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	void calcPivot(const hkTransform& transformBodyA, hkVector4& pivotOut) const;

	static const hkClass& staticClass();

	hkpBridgeAtoms m_atoms;
	// Reference counted.
	hkpParametricCurve* m_path;
	float m_maxFrictionForce;
	hkEnum<OrientationConstraintType, int8_t> m_angularConstrainedDOF;
	// The pivot transforms of the two bodies, from the bodies' object space to the constraint's key space. They
	// start on a 16-byte boundary (the PDB has m_transform_OS_KS at 0x30, hkTransform's hkVector4 gives it that
	// alignment and the constructor 0xDEE750 stores them with movaps), so the 15 bytes after
	// m_angularConstrainedDOF are the compiler's padding.
	hkTransform m_transform_OS_KS[2];

	static MembersArray Members;
};

extern hkClass hkpPointToPathConstraintDataClass;
extern hkTypeInfo hkpPointToPathConstraintDataTypeInfo;
inline constexpr fnvd::Vtable hkpPointToPathConstraintData_vftable{};
