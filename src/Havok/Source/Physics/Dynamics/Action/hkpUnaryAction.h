#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"

class hkpEntity;

// An action that works on a single entity. It supplies the entity list and the removal callback; the derived action
// supplies applyAction, getPhantoms and clone.
class hkpUnaryAction : public hkpAction {
public:
	hkpUnaryAction(hkpEntity* apEntity, hkUlong auiUserData);
	hkpUnaryAction(int32_t aiFinishing);
	~hkpUnaryAction() override;

	const hkClass* getClassType() const override;
	void getEntities(hkArray<hkpEntity*>& entitiesOut) override;
	void entityRemovedCallback(hkpEntity* entity) override;

	// Releases the current entity (taking it off the world's action lists first), then holds entity and puts it
	// there.
	void setEntity(hkpEntity* entity);

	hkpEntity* m_entity;
};

// The class's reflection record.
extern hkClass hkpUnaryActionClass;
