#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"

#include <stdint.h>

class hkpConstraintChainInstance;

// The action a constraint chain instance adds to the world, so that the chain hears when its entities are removed.
class hkpConstraintChainInstanceAction : public hkpAction {
public:
	hkpConstraintChainInstanceAction(hkpConstraintChainInstance* constraintInstance)
		: hkpAction(0), m_constraintInstance(constraintInstance) {}

	// The SDK's finish constructor of a loaded object.
	hkpConstraintChainInstanceAction(hkFinishLoadedObjectFlag aFlag) : hkpAction(aFlag) {}

	// Does nothing.
	void applyAction(const hkStepInfo& stepInfo) override;
	// Appends the chain's entities.
	void getEntities(hkArray<hkpEntity*>& entitiesOut) override;
	// Passes the removal on to the chain instance.
	void entityRemovedCallback(hkpEntity* entity) override;
	// A chain's action is not cloned: returns null.
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	static const hkClass& staticClass();

	hkpConstraintChainInstance* m_constraintInstance;
};

extern hkClass hkpConstraintChainInstanceActionClass;
extern hkTypeInfo hkpConstraintChainInstanceActionTypeInfo;
