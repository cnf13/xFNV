#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Reconstruction.hpp"

// The data of a chain of stiff springs: a pivot in each body and the spring's rest length per link, and the solver's
// tuning.
class hkpStiffSpringChainData : public hkpConstraintChainData {
public:
	// The pivots and rest length of one link; the pivots are in the bodies' spaces.
	struct ConstraintInfo {
		hkVector4 m_pivotInA;
		hkVector4 m_pivotInB;
		float m_springLength;
	};

	// What the solver keeps per constraint: its results. The structure itself holds nothing.
	struct Runtime {
		const hkpSolverResults* getSolverResults() { return reinterpret_cast<hkpSolverResults*>(this); }
	};

	hkpStiffSpringChainData();
	// The finish constructor of loaded chain data: sets the vtable, then finishes the atoms and
	// the link array.
	hkpStiffSpringChainData(hkFinishLoadedObjectFlag aFlag);
	~hkpStiffSpringChainData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	int32_t getNumConstraintInfos() override { return m_infos.m_size; }
	void buildJacobian(const hkpConstraintQueryIn& inNotValid, hkpConstraintQueryOut& out) override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;

	// Appends a link.
	void addConstraintInfoInBodySpace(const hkVector4& pivotInA, const hkVector4& pivotInB, float springLength);
	const Runtime* getRuntime(void* runtime) { return reinterpret_cast<Runtime*>(runtime); }

	hkpBridgeAtoms m_atoms;
	hkArray<ConstraintInfo> m_infos;
	float m_tau;
	float m_damping;
	float m_cfm;
};

inline constexpr fnvd::Vtable hkpStiffSpringChainData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpStiffSpringChainDataConstraintInfoTypeInfo;
