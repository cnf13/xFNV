#pragma once

#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Reconstruction.hpp"

class hkpEntity;

// An action that works on two entities. It supplies the entity list and the removal callback; the derived action
// supplies applyAction, getPhantoms and clone.
class hkpBinaryAction : public hkpAction {
public:
	hkpBinaryAction(hkpEntity* entityA, hkpEntity* entityB, hkUlong userData);
	// The SDK's finish constructor of a loaded object (the derived actions' finish constructors call it).
	hkpBinaryAction(hkFinishLoadedObjectFlag flag) : hkpAction(flag) {}
	~hkpBinaryAction() override;

	const hkClass* getClassType() const override;
	void getEntities(hkArray<hkpEntity*>& entitiesOut) override;
	void entityRemovedCallback(hkpEntity* entity) override;
	// Still pure here (the PDB lists it in hkpBinaryAction).
	void applyAction(const hkStepInfo& stepInfo) override = 0;

	// Replaces entity A, moving the action's world registration over.
	void setEntityA(hkpEntity* entityA);
	// Replaces entity B, moving the action's world registration over.
	void setEntityB(hkpEntity* entityB);
	hkpEntity* getEntityA() { return m_entityA; }
	hkpEntity* getEntityB() { return m_entityB; }
	void _referenceBodies();

	hkpEntity* m_entityA;
	hkpEntity* m_entityB;
};
inline constexpr fnvd::Vtable hkpBinaryAction_vftable{};
