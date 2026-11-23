#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstanceAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
static const void* getVtablehkpConstraintChainInstance();
static void cleanupLoadedObjecthkpConstraintChainInstance(void* p);

using hkpConstraintChainInstanceClass_MembersArray = const hkClassMember[2];
static hkpConstraintChainInstanceClass_MembersArray hkpConstraintChainInstanceClass_Members{
    {"chainedEntities",
     &hkpEntityClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x38,
     nullptr},
    {"action",
     &hkpConstraintChainInstanceActionClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x44,
     nullptr}
};
hkClass hkpConstraintChainInstanceClass(
    "hkpConstraintChainInstance",
    &hkpConstraintInstanceClass,
    72,
    nullptr,
    0,
    nullptr,
    0,
    hkpConstraintChainInstanceClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpConstraintChainInstanceTypeInfo = {
    "hkpConstraintChainInstance",
    finishLoadedObjecthkpConstraintChainInstance,
    cleanupLoadedObjecthkpConstraintChainInstance,
    getVtablehkpConstraintChainInstance(),
    &typeid(hkpConstraintChainInstance),
    sizeof(hkpConstraintChainInstance)
};

static const void* getVtablehkpConstraintChainInstance() {
    return hkGetVtableOf<hkpConstraintChainInstance>();
}

static void cleanupLoadedObjecthkpConstraintChainInstance(void* p) {
    static_cast<hkpConstraintChainInstance*>(p)->~hkpConstraintChainInstance();
}

hkpConstraintChainInstance::hkpConstraintChainInstance(hkpConstraintChainData* apData)
  : hkpConstraintInstance(hkpConstraintInstance::PRIORITY_PSI) {
    m_data = apData;
    apData->addReference();
    m_entities[0] = nullptr;
    m_entities[1] = nullptr;
    m_action = new hkpConstraintChainInstanceAction(this);
}

// Each chained entity and the action released; the compiler frees the entity array and runs the base's destructor.
hkpConstraintChainInstance::~hkpConstraintChainInstance() {
    for (int32_t i = 0; i < m_chainedEntities.m_size; ++i) {
        m_chainedEntities.m_data[i]->removeReference();
    }
    m_action->removeReference();
}

void hkpConstraintChainInstance::entityRemovedCallback(hkpEntity* entity) {
    hkpWorld* m_world = entity->m_world;
    m_world->lockCriticalOperations();
    m_world->removeActionImmediately(m_action);
    hkpWorldOperationUtil::removeConstraintFromCriticalLockedIsland(m_world, this);
    m_world->unlockAndAttemptToExecutePendingOperations();
}

hkpConstraintInstance::InstanceType hkpConstraintChainInstance::getType() const {
    return TYPE_CHAIN;
}

void hkpConstraintChainInstance::addEntity(hkpEntity* entity) {
    int32_t iNumEntities = m_chainedEntities.m_size;
    if (iNumEntities < 2 && !m_entities[iNumEntities]) {
        m_entities[iNumEntities] = entity;
        entity->addReference();
    }
    m_chainedEntities.pushBack(entity);
    entity->addReference();
}

// The entity is inserted first in the chain, then referenced.
void hkpConstraintChainInstance::insertEntityAtFront(hkpEntity* entity) {
    m_chainedEntities._insertAt(hkMemoryRouter::getInstance().heap(), 0, entity);
    entity->addReference();
}

const hkClass& hkpConstraintChainInstance::staticClass() {
    return hkpConstraintChainInstanceClass;
}
