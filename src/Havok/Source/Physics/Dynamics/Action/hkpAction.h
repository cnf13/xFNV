#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpEntity;
class hkpPhantom;
class hkpSimulationIsland;
class hkpWorld;

// A user action the world applies every step to the entities it names. Its code stays with the exe.
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
	// Called when one of the action's entities is removed from the world. The exe's slot holds _purecall (0xEC60FA).
	virtual void entityRemovedCallback(hkpEntity* apEntity) = 0;
	// A copy of the action acting on the given entities and phantoms.
	virtual hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const = 0;
	~hkpAction() override;

	// The world the action was added to; null while it is not in one.
	hkpWorld* m_world;
	hkpSimulationIsland* m_island;
	// PDB and SDK: hkUlong. The game stores the bhkAction wrapping the action here.
	hkUlong m_userData;
	hkStringPtr m_name;
};
inline constexpr fnvd::Vtable hkpAction_vftable{};
