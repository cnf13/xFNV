#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <stdint.h>

class hkpConstraintChainData;
class hkpConstraintChainInstanceAction;
class hkpEntity;

void cleanupLoadedObjecthkpPoweredChainMapperTarget(void* p);
void cleanupLoadedObjecthkpPoweredChainMapperLinkInfo(void* p);
void cleanupLoadedObjecthkpPoweredChainMapper(void* p);

// Owns the entities and action participating in a constraint-chain instance.
class hkpConstraintChainInstance : public hkpConstraintInstance {
public:
	// The constructor, on apMemory (the caller sets m_memSizeAndFlags): a PSI-priority instance over apData, with no
	// entities yet and the action that tells the chain when its entities leave the world.
	hkpConstraintChainInstance(hkpConstraintChainData* apData);

	// The SDK's finish constructor of a loaded object.
	hkpConstraintChainInstance(hkFinishLoadedObjectFlag aFlag)
		: hkpConstraintInstance(aFlag), m_chainedEntities(aFlag) {}

	~hkpConstraintChainInstance() override;

	void entityRemovedCallback(hkpEntity* entity) override;
	hkpConstraintInstance::InstanceType getType() const override;

	// Appends apEntity to the chain; the first two also become the instance's own entities A and B when those are
	// still empty.
	void addEntity(hkpEntity* entity);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes).
	void insertEntityAtFront(hkpEntity* entity);

	static const hkClass& staticClass();

	hkArray<hkpEntity*, hkContainerHeapAllocator> m_chainedEntities;
	hkpConstraintChainInstanceAction* m_action;
};

extern hkClass hkpConstraintChainInstanceClass;
extern hkTypeInfo hkpConstraintChainInstanceTypeInfo;
extern hkClass hkpPoweredChainMapperClass;
extern hkTypeInfo hkpPoweredChainMapperTypeInfo;
extern hkClass hkpPoweredChainMapperLinkInfoClass;
extern hkClass hkpPoweredChainMapperTargetClass;
