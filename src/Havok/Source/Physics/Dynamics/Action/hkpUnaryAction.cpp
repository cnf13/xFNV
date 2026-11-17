#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// The reflection data of hkpUnaryAction's class record, as the Havok SDK's class file has it: the entity member's link
// attribute, the one-attribute list and the custom attributes record over it, then the member table.
static hkLinkAttribute hkpUnaryAction_entity_Link{{hkLinkAttribute::PARENT_NAME}};
using hkpUnaryActionAttributeArray = const hkCustomAttributes::Attribute[1];
static hkpUnaryActionAttributeArray hkpUnaryAction_entity_AttributeList{
	{"hk.Link", {&hkpUnaryAction_entity_Link, &hkLinkAttributeClass}}};
static const hkCustomAttributes hkpUnaryAction_entity_Attributes{hkpUnaryAction_entity_AttributeList, 1};
using hkpUnaryActionClass_MembersArray = const hkClassMember[1];
static hkpUnaryActionClass_MembersArray hkpUnaryActionClass_Members{{"entity", &hkpEntityClass, nullptr,
	{hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, {0}, 0x18, &hkpUnaryAction_entity_Attributes}};

hkClass hkpUnaryActionClass("hkpUnaryAction", &hkpActionClass, 28, nullptr, 0, nullptr, 0, hkpUnaryActionClass_Members,
	1, nullptr, nullptr, 0, 0);

hkpUnaryAction::hkpUnaryAction(hkpEntity* apEntity, hkUlong auiUserData) : hkpAction(auiUserData), m_entity(apEntity) {
	if (apEntity) {
		apEntity->addReference();
	}
}

// The loaded entity stays.
hkpUnaryAction::hkpUnaryAction(int32_t aiFinishing) : hkpAction(hkFinishLoadedObjectFlag{aiFinishing}) {}

// Releases the entity; the name goes with its member.
hkpUnaryAction::~hkpUnaryAction() {
	if (m_entity) {
		m_entity->removeReference();
		m_entity = nullptr;
	}
}

const hkClass* hkpUnaryAction::getClassType() const {
	return &hkpUnaryActionClass;
}

void hkpUnaryAction::getEntities(hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	if (entitiesOut.m_size == (entitiesOut.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*Instance->m_heap, &entitiesOut, sizeof(hkpEntity*));
	}
	entitiesOut.m_data[entitiesOut.m_size++] = m_entity;
}

void hkpUnaryAction::entityRemovedCallback(hkpEntity* entity) {
	hkpWorld* m_world;
	m_world = this->m_world;
	if (m_world) {
		m_world->removeActionImmediately(this);
	}
}

// The Havok SDK's setEntity: off the world's action lists and released, then the new entity held and put on them.
void hkpUnaryAction::setEntity(hkpEntity* entity) {
	if (m_entity) {
		if (m_world) {
			m_world->detachActionFromEntity(this, m_entity);
		}
		m_entity->removeReference();
		m_entity = nullptr;
	}
	m_entity = entity;
	entity->addReference();
	if (m_world) {
		m_world->attachActionToEntity(this, m_entity);
	}
}
