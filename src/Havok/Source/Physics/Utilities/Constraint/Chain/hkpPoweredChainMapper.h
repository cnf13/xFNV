#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkQuaternion;
class hkpConstraintChainInstance;
class hkpConstraintInstance;
class hkpConstraintMotor;
class hkpEntity;
class hkpPoweredChainData;

// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpPoweredChainMapper();

// Maps powered constraint-chain links to their target constraints.
class hkpPoweredChainMapper : public hkReferencedObject {
public:
	// One link: its run of targets in m_targets and the constraint limiting it (the PDB's members; the exe's hkClass
	// hkpPoweredChainMapperLinkInfoClass has them at 0x0, 0x4 and 0x8 and an object size of 0xC).
	struct LinkInfo {
		// The reflection class hkpPoweredChainMapperLinkInfoClass.
		static const hkClass& staticClass();

		int32_t m_firstTargetIdx;
		int32_t m_numTargets;
		hkpConstraintInstance* m_limitConstraint;
	};

	// One target of a link: a chain and the index of the link's info in it (the PDB's members; the exe's hkClass
	// hkpPoweredChainMapperTargetClass has them at 0x0 and 0x4 and an object size of 8).
	struct Target {
		// The reflection class hkpPoweredChainMapperTargetClass.
		static const hkClass& staticClass();

		hkpPoweredChainData* m_chain;
		int32_t m_infoIndex;
	};

	// buildChainMapper's options (Havok SDK; the SDK's constructor clears both).
	struct Config {
		hkBool m_createLimitConstraints;
		hkBool m_cloneMotors;
	};

	// The two ends of one chain buildChainMapper is asked for (Havok SDK).
	struct ChainEndpoints {
		hkpEntity* m_start;
		hkpEntity* m_end;
	};

	// The SDK's (private) hkpPoweredChainMapper(): no links, targets or chains, one reference.
	hkpPoweredChainMapper() {}

	hkpPoweredChainMapper(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_links(aFlag), m_targets(aFlag), m_chains(aFlag) {}

	~hkpPoweredChainMapper() override;

	// Appends to motorsOut the motorIndex-th motor of every target of link linkIndex that
	// has one (thiscall, ret 0xC, 0xE3A520). Parameter names from the Havok SDK; the PDB names only locals.
	void getMotors(int32_t linkIndex, int32_t coordinateIndex, hkArray<hkpConstraintMotor*>& motorsOut);

	// Shares the force limits out among the link's motors (thiscall, ret 0x10, 0xE3AE70). Parameter names from the
	// Havok SDK; the PDB names only locals.
	void setForceLimits(int32_t linkIndex, int32_t coordinageIndex, float minForce, float maxForce);
	void setMotors(int32_t linkIndex, int32_t coordinateIndex, hkpConstraintMotor* newMotor);
	void setTargetOrientation(int32_t linkIndex, const hkQuaternion& newTarget);

	// The reflection class hkpPoweredChainMapperClass.
	static const hkClass& staticClass();
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,string): a powered chain per pair of end bodies and the
	// map from the constraints to their links in them.
	static hkpPoweredChainMapper* buildChainMapper(hkpPoweredChainMapper::Config config,
		const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& allConstraints,
		const hkArray<hkpPoweredChainMapper::ChainEndpoints, hkContainerHeapAllocator>& pairs,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>* unusedConstraints);

	hkArray<LinkInfo, hkContainerHeapAllocator> m_links;
	hkArray<Target, hkContainerHeapAllocator> m_targets;
	hkArray<hkpConstraintChainInstance*, hkContainerHeapAllocator> m_chains;
};

inline constexpr fnvd::Vtable hkpPoweredChainMapper_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPoweredChainMapperTargetTypeInfo;
extern hkTypeInfo hkpPoweredChainMapperLinkInfoTypeInfo;
