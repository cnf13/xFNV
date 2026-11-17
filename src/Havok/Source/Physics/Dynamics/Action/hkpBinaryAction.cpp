#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// The reflection data of hkpBinaryAction's class record, as the Havok SDK's class file has it: per entity member a link
// and a user-interface attribute, the two-attribute list naming both and the custom attributes record over the list,
// then the member table.
using hkpBinaryActionAttributeArray = const hkCustomAttributes::Attribute[2];
static hkLinkAttribute hkpBinaryAction_entityA_Link{{hkLinkAttribute::PARENT_NAME}};
static hkUiAttribute hkpBinaryAction_entityA_Ui{false, {hkUiAttribute::NONE}, "", "Attach to", "", false, false, false};
static hkpBinaryActionAttributeArray hkpBinaryAction_entityA_AttributeList{
    {"hk.Link", {&hkpBinaryAction_entityA_Link, &hkLinkAttributeClass}},
    {"hk.Ui", {&hkpBinaryAction_entityA_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpBinaryAction_entityA_Attributes{hkpBinaryAction_entityA_AttributeList, 2};
static hkLinkAttribute hkpBinaryAction_entityB_Link{{hkLinkAttribute::MESH}};
static hkUiAttribute hkpBinaryAction_entityB_Ui{true, {hkUiAttribute::NONE}, "Other Body", "", "", true, false, false};
static hkpBinaryActionAttributeArray hkpBinaryAction_entityB_AttributeList{
    {"hk.Link", {&hkpBinaryAction_entityB_Link, &hkLinkAttributeClass}},
    {"hk.Ui", {&hkpBinaryAction_entityB_Ui, &hkUiAttributeClass}}
};
static const hkCustomAttributes hkpBinaryAction_entityB_Attributes{hkpBinaryAction_entityB_AttributeList, 2};
using hkpBinaryActionClass_MembersArray = const hkClassMember[2];
static hkpBinaryActionClass_MembersArray hkpBinaryActionClass_Members{
    {"entityA",
     &hkpEntityClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     &hkpBinaryAction_entityA_Attributes},
    {"entityB",
     &hkpEntityClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x1C,
     &hkpBinaryAction_entityB_Attributes}
};

hkClass hkpBinaryActionClass(
    "hkpBinaryAction",
    &hkpActionClass,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkpBinaryActionClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

hkpBinaryAction::hkpBinaryAction(hkpEntity* entityA, hkpEntity* entityB, hkUlong userData)
  : hkpAction(userData), m_entityA(entityA), m_entityB(entityB) {
    if (entityB || entityA) {
        _referenceBodies();
    }
}

// Releases both entities; the name goes with its member.
hkpBinaryAction::~hkpBinaryAction() {
    if (m_entityA) {
        m_entityA->removeReference();
        m_entityA = nullptr;
    }
    if (m_entityB) {
        m_entityB->removeReference();
        m_entityB = nullptr;
    }
}

void hkpBinaryAction::getEntities(hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (entitiesOut.m_size == (entitiesOut.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &entitiesOut, sizeof(hkpEntity*));
    }
    entitiesOut.m_data[entitiesOut.m_size++] = this->getEntityA();
    hkMemoryRouter* v4 = &hkMemoryRouter::getInstance();
    if (entitiesOut.m_size == (entitiesOut.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v4->m_heap, &entitiesOut, sizeof(hkpEntity*));
    }
    entitiesOut.m_data[entitiesOut.m_size++] = this->getEntityB();
}

void hkpBinaryAction::entityRemovedCallback(hkpEntity* entity) {
    hkpWorld* m_world;
    m_world = this->m_world;
    if (m_world) {
        m_world->removeActionImmediately(this);
    }
}

void hkpBinaryAction::setEntityA(hkpEntity* entityA) {
    entityA->addReference();
    if (this->getEntityA()) {
        hkpWorld* m_world = this->m_world;
        if (m_world) {
            m_world->detachActionFromEntity(this, this->getEntityA());
        }
        this->getEntityA()->removeReference();
        m_entityA = nullptr;
    }
    hkpWorld* pWorld = this->m_world;
    m_entityA = entityA;
    if (pWorld) {
        pWorld->attachActionToEntity(this, entityA);
    }
}

void hkpBinaryAction::setEntityB(hkpEntity* entityB) {
    entityB->addReference();
    if (this->getEntityB()) {
        hkpWorld* m_world = this->m_world;
        if (m_world) {
            m_world->detachActionFromEntity(this, this->getEntityB());
        }
        this->getEntityB()->removeReference();
        m_entityB = nullptr;
    }
    hkpWorld* v4 = this->m_world;
    m_entityB = entityB;
    if (v4) {
        v4->attachActionToEntity(this, entityB);
    }
}

void hkpBinaryAction::_referenceBodies() {
    hkpEntity* m_entityA;
    hkpEntity* m_entityB;
    m_entityA = this->m_entityA;
    if (m_entityA) {
        m_entityA->hkReferencedObject::addReference();
    }
    m_entityB = this->m_entityB;
    if (m_entityB) {
        m_entityB->hkReferencedObject::addReference();
    }
}
