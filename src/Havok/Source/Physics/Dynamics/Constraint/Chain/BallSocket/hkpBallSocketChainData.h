#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Reconstruction.hpp"

void finishLoadedObjecthkpBallSocketChainData(void* p, int32_t finishing);

// The data of a chain of ball and socket constraints: a pivot in each body per link and the solver's tuning.
class hkpBallSocketChainData : public hkpConstraintChainData {
public:
	// The runtime memory of one instance: no members of its own; it is the chain's solver results, three per link
	// (getRuntimeInfo 0xD53CF0's size, 8 times 3 times the link count).
	struct Runtime {};

	// The pivots of one link, in the bodies' spaces.
	struct ConstraintInfo {
		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkVector4 m_pivotInA;
		hkVector4 m_pivotInB;
	};

	// Constructs the chain data in caller-provided storage.
	hkpBallSocketChainData();
	hkpBallSocketChainData(hkFinishLoadedObjectFlag flag);

	~hkpBallSocketChainData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	int32_t getNumConstraintInfos() override { return m_infos.m_size; }
	void buildJacobian(const hkpConstraintQueryIn& inChain, hkpConstraintQueryOut& out) override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;

	void addConstraintInfoInBodySpace(const hkVector4& pivotInA, const hkVector4& pivotInB);

	hkpBridgeAtoms m_atoms;
	hkArray<ConstraintInfo> m_infos;
	float m_tau;
	float m_damping;
	float m_cfm;
	float m_maxErrorDistance;
};

inline constexpr fnvd::Vtable hkpBallSocketChainData_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpBallSocketChainDataConstraintInfoTypeInfo;
extern hkClass hkpBallSocketChainDataConstraintInfoClass;
