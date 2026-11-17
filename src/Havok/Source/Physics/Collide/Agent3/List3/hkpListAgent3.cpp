#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

// An uninitialised read, kept as the game does: registerAgent3 copies the whole of f, so the bytes after
// m_reusePreviousEntry, never written here, land in the dispatcher's table.
void hkListAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
	alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
	f.m_isPredictive.m_bool = 0;
	f.m_ignoreSymmetricVersion.m_bool = 0;
	f.m_reusePreviousEntry.m_bool = 0;
	f.m_updateFilterFunc = nullptr;
	f.m_calcStatisticsFunc = nullptr;
	f.m_invalidateTimFunc = nullptr;
	f.m_warpTimeFunc = nullptr;
	f.m_sepNormalFunc = nullptr;
	hkListAgent3::initAgentFunc(f);
	dispatcher->registerAgent3(f, HK_SHAPE_ALL, HK_SHAPE_LIST);
}

// The list agent has no GSK manifold, so the point functions and the separating normal are null.
void hkListAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
	f.m_createFunc = &hkListAgent3::create;
	f.m_processFunc = &hkListAgent3::process;
	f.m_sepNormalFunc = nullptr;
	f.m_cleanupFunc = nullptr;
	f.m_removePointFunc = nullptr;
	f.m_commitPotentialFunc = nullptr;
	f.m_createZombieFunc = nullptr;
	f.m_updateFilterFunc = &hkListAgent3::updateFilter;
	f.m_calcStatisticsFunc = &hkListAgent3::calcStatistics;
	f.m_invalidateTimFunc = &hkListAgent3::invalidateTim;
	f.m_warpTimeFunc = &hkListAgent3::warpTime;
	f.m_destroyFunc = &hkListAgent3::destroy;
	f.m_isPredictive.m_bool = 1;
}

// The keys of the list's enabled children, ended by HK_INVALID_SHAPE_KEY, go to the track's machine with the list as
// container. The key buffer is an hkLocalBuffer<hkpShapeKey>.
hkpAgentData* hkListAgent3::process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
	hkVector4* separatingNormal, hkpProcessCollisionOutput& result) {
	HK_TIMER_BEGIN("List3", nullptr);
	hkpAgent1nTrack* agent1nTrack = static_cast<hkpAgent1nTrack*>(agentData);
	const hkpListShape* lShape = static_cast<const hkpListShape*>(input.m_bodyB->m_shape);
	int32_t size = lShape->m_childInfo.m_size;
	// The stack allocator hands out untyped bytes (m_next is a char*), cast to the keys and back.
	// EXE-INLINED none: hkLocalBuffer<hkpShapeKey>::hkLocalBuffer
	hkMemoryRouter& router = hkMemoryRouter::getInstance();
	uint32_t* hitList = reinterpret_cast<uint32_t*>(router.m_stack.m_next.m_storage);
	router.m_stack.m_next.m_storage += (sizeof(uint32_t) * (size + 1) + 127) & ~static_cast<size_t>(127);
	int32_t numHits = 0;
	for (int32_t i = 0; i < size; i++) {
		// EXE-INLINED none: hkpListShape::isChildEnabled
		if (static_cast<uint32_t>(i) >= hkpListShape::MAX_DISABLED_CHILDREN ||
			(lShape->m_enabledChildren[static_cast<uint32_t>(i) >> 5] & (1u << (i & 31))) != 0) {
			hitList[numHits++] = i;
		}
	}
	hitList[numHits] = HK_INVALID_SHAPE_KEY;
	hkAgent1nMachine_Process(*agent1nTrack, input, lShape, hitList, result);
	HK_TIMER_END();
	// EXE-INLINED none: hkLocalBuffer<hkpShapeKey>::~hkLocalBuffer
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(hitList);
	// The SDK's hkAddByteOffset(agentData, sizeof(hkpAgent1nTrack)): the track's 16-byte aligned size, as create
	// leaves it.
	return static_cast<char*>(agentData) + ((sizeof(hkpAgent1nTrack) + 15) & ~static_cast<size_t>(15));
}

void hkListAgent3::calcStatistics(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input,
	hkStatisticsCollector* collector) {
	collector->pushDir("hkListAgent3");
	hkpAgentMachineUtil::calc1nStatistics(agentData, &input, collector);
	collector->popDir();
}

// The track is built in freeMemory, with a null test; the data after it starts on the next 16-byte boundary.
hkpAgentData* hkListAgent3::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
	hkpAgent1nTrack* agentTrack = static_cast<hkpAgent1nTrack*>(agentData);
	// EXE-INLINED none: hkpAgent1nTrack::hkpAgent1nTrack
	if (agentTrack) {
		agentTrack->m_sectors.m_data = nullptr;
		agentTrack->m_sectors.m_size = 0;
		agentTrack->m_sectors.m_capacityAndFlags = hkArrayBase<hkpAgent1nSector*>::DONT_DEALLOCATE_FLAG;
	}
	hkAgent1nMachine_Create(*agentTrack);
	return static_cast<char*>(agentData) + ((sizeof(hkpAgent1nTrack) + 15) & ~static_cast<size_t>(15));
}

void hkListAgent3::updateFilter(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCdBody& bodyA,
	const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr, hkpConstraintOwner& constraintOwner) {
	hkpAgent1nMachine_VisitorInput vin;
	const hkpShapeContainer* Container = bodyB.m_shape->getContainer();
	vin.m_bodyA = &bodyA;
	vin.m_containerShapeB = Container;
	vin.m_contactMgr = mgr;
	vin.m_input = &input;
	vin.m_collectionBodyB = &bodyB;
	vin.m_constraintOwner = &constraintOwner;
	hkAgent1nMachine_UpdateShapeCollectionFilter(*static_cast<hkpAgent1nTrack*>(agentData), vin);
}
