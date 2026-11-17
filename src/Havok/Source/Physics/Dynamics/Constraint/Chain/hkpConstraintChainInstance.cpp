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
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"

// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
static const void* getVtablehkpConstraintChainInstance();
static void cleanupLoadedObjecthkpConstraintChainInstance(void* p);

using hkpConstraintChainInstanceClass_MembersArray = const hkClassMember[2];
static hkpConstraintChainInstanceClass_MembersArray hkpConstraintChainInstanceClass_Members{
	{"chainedEntities", &hkpEntityClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0},
		0x38, nullptr},
	{"action", &hkpConstraintChainInstanceActionClass, nullptr, {hkClassMember::TYPE_POINTER},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x44, nullptr}};
hkClass hkpConstraintChainInstanceClass("hkpConstraintChainInstance", &hkpConstraintInstanceClass, 72, nullptr, 0,
	nullptr, 0, hkpConstraintChainInstanceClass_Members, 2, nullptr, nullptr, 0, 0);
hkTypeInfo hkpConstraintChainInstanceTypeInfo = {"hkpConstraintChainInstance",
	finishLoadedObjecthkpConstraintChainInstance, cleanupLoadedObjecthkpConstraintChainInstance,
	getVtablehkpConstraintChainInstance(), &typeid(hkpConstraintChainInstance), sizeof(hkpConstraintChainInstance)};

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
	++m_world->m_criticalOperationsLockCount;
	m_world->removeActionImmediately(m_action);
	hkpWorldOperationUtil::removeConstraintFromCriticalLockedIsland(m_world, this);
	if (m_world->m_criticalOperationsLockCount-- == 1 && !m_world->m_blockExecutingPendingOperations) {
		if (m_world->m_pendingOperationsCount) {
			m_world->internal_executePendingOperations();
		}
		if (m_world->m_pendingOperationQueueCount == 1) {
			if (m_world->m_pendingBodyOperationsCount) {
				m_world->internal_executePendingBodyOperations();
			}
		}
	}
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
	hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
	if (m_chainedEntities.m_size == (m_chainedEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*rRouter.m_heap, &m_chainedEntities, sizeof(hkpEntity*));
	}
	m_chainedEntities.m_data[m_chainedEntities.m_size] = entity;
	++m_chainedEntities.m_size;
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
