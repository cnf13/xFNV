#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/Default/hkpDefaultToiResourceMgr.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/hkpToiResourceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/hkpToiResources.h"

hkClass hkpDefaultToiResourceMgrClass("hkpDefaultToiResourceMgr", &hkReferencedObjectClass, 16, nullptr, 0, nullptr, 0,
	nullptr, 0, nullptr, nullptr, 0, 0);

hkpDefaultToiResourceMgr::hkpDefaultToiResourceMgr() : m_defaultScratchpadSize(131072) {}

hkpDefaultToiResourceMgr::~hkpDefaultToiResourceMgr() {}

// The scratch pad's size goes to the solver allocator through a local the allocator may round up; the rounded size
// is the capacity endToiAndFreeResources frees.
hkResult hkpDefaultToiResourceMgr::beginToiAndSetupResources(const hkpToiEvent& event,
	const hkArray<hkpToiEvent, hkContainerHeapAllocator>& otherEvents, hkpToiResources& resourcesOut) {
	hkBool shouldHandle;
	if (!shouldHandleGivenToi(&shouldHandle, event)->m_bool) {
		return HK_FAILURE;
	}
	resourcesOut.m_scratchpadSize = m_defaultScratchpadSize;
	m_scratchPadCapacity = m_defaultScratchpadSize;
	hkMemoryAllocator* solverAllocator = hkMemoryRouter::getInstance().m_solver;
	int32_t scratchPadCapacity = m_scratchPadCapacity;
	void* scratchpad = solverAllocator->bufAlloc(scratchPadCapacity);
	m_scratchPadCapacity = scratchPadCapacity;
	resourcesOut.m_scratchpad = static_cast<char*>(scratchpad);
	resourcesOut.m_maxNumActiveEntities = 1000;
	resourcesOut.m_maxNumConstraints = 1000;
	resourcesOut.m_maxNumEntities = 1000;
	resourcesOut.m_numToiSolverIterations = 3;
	resourcesOut.m_numForcedToiFinalSolverIterations = 4;
	resourcesOut.m_minPriorityToProcess = hkpConstraintInstance::PRIORITY_TOI;
	return HK_SUCCESS;
}

hkpToiResourceMgrResponse hkpDefaultToiResourceMgr::cannotSolve(hkArray<ConstraintViolationInfo>& violatedConstraints) {
	return HK_TOI_RESOURCE_MGR_RESPONSE_CONTINUE;
}

hkpToiResourceMgrResponse hkpDefaultToiResourceMgr::resourcesDepleted() {
	return HK_TOI_RESOURCE_MGR_RESPONSE_DO_NOT_EXPAND_AND_CONTINUE;
}

void hkpDefaultToiResourceMgr::endToiAndFreeResources(const hkpToiEvent& event,
	const hkArray<hkpToiEvent, hkContainerHeapAllocator>& otherEvents, const hkpToiResources& resources) {
	int32_t m_scratchPadCapacity = this->m_scratchPadCapacity;
	char* m_scratchpad = resources.m_scratchpad;
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	Instance->m_solver->bufFree(m_scratchpad, m_scratchPadCapacity);
}

const hkClass* hkpDefaultToiResourceMgr::getClassType() const {
	return &hkpDefaultToiResourceMgrClass;
}

void hkpDefaultToiResourceMgr::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	collector->pushDir("hkpDefaultToiResourceMgr");
	collector->popDir();
}

hkBool* hkpDefaultToiResourceMgr::shouldHandleGivenToi(hkBool* apResult, const hkpToiEvent& event) {
	apResult->m_bool = true;
	return apResult;
}
