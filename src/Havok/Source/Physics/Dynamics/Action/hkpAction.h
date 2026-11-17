#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpEntity;
class hkpPhantom;
class hkpSimulationIsland;
class hkpWorld;
struct hkStepInfo;

// A user action the world applies every step to the entities it names.
class hkpAction : public hkReferencedObject {
public:
	// The SDK's constructors, which the derived actions' constructors inline.
	hkpAction(hkUlong userData);
	hkpAction(hkFinishLoadedObjectFlag aFlag);

	virtual void applyAction(const hkStepInfo& stepInfo) = 0;
	// Appends the entities the action works on.
	virtual void getEntities(hkArray<hkpEntity*>& entitiesOut) = 0;
	// Appends the phantoms the action works on (the base appends none): the SDK's inline body.
	virtual void getPhantoms(hkArray<hkpPhantom*>& phantomsOut) {}
	// Called when one of the action's entities is removed from the world.
	virtual void entityRemovedCallback(hkpEntity* apEntity) = 0;
	// A copy of the action acting on the given entities and phantoms.
	virtual hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const = 0;
	~hkpAction() override;

	// The world the action was added to; null while it is not in one.
	hkpWorld* m_world;
	hkpSimulationIsland* m_island;
	// The game stores the bhkAction wrapping the action here.
	hkUlong m_userData;
	hkStringPtr m_name;
};
