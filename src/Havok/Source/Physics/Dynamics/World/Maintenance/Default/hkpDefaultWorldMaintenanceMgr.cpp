#include "Havok/Source/Physics/Dynamics/World/Maintenance/Default/hkpDefaultWorldMaintenanceMgr.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

hkpDefaultWorldMaintenanceMgr::hkpDefaultWorldMaintenanceMgr()
  : m_minAllowedTimeValue(33.0f), m_maxAllowedTimeValue(63.0f) {}

void hkpDefaultWorldMaintenanceMgr::init(hkpWorld* world) {
    hkpSimulation* m_simulation = world->m_simulation;
    m_simulation->m_currentTime = m_minAllowedTimeValue;
    m_simulation->m_currentPsiTime = m_minAllowedTimeValue;
}

void hkpDefaultWorldMaintenanceMgr::performMaintenance(hkpWorld* world, hkStepInfo& stepInfo) {
    HK_TIMER_BEGIN_LIST("Maintenance", "Split");
    hkpWorldOperationUtil::splitSimulationIslands(world);
    if (m_maxAllowedTimeValue <= stepInfo.m_startTime) {
        HK_TIMER_SPLIT_LIST("ResetTime");
        resetWorldTime(world, stepInfo);
    }
    HK_TIMER_END_LIST();
}

void hkpDefaultWorldMaintenanceMgr::performMaintenanceNoSplit(hkpWorld* world, hkStepInfo& stepInfo) {
    HK_TIMER_BEGIN_LIST("Maintenance", "ResetTime");
    if (m_maxAllowedTimeValue <= stepInfo.m_startTime) {
        resetWorldTime(world, stepInfo);
    }
    HK_TIMER_END_LIST();
}

void hkpDefaultWorldMaintenanceMgr::resetWorldTime(hkpWorld* world, hkStepInfo& stepInfo) {
    float stepInfob = stepInfo.m_endTime - stepInfo.m_startTime;
    world->m_dynamicsStepInfo.m_stepInfo.m_startTime = m_minAllowedTimeValue;
    world->m_dynamicsStepInfo.m_stepInfo.m_endTime = stepInfob + m_minAllowedTimeValue;
    world->m_dynamicsStepInfo.m_stepInfo.m_deltaTime = stepInfob;
    world->m_dynamicsStepInfo.m_stepInfo.m_invDeltaTime = 1.0 / stepInfob;
    hkpSimulation* m_simulation = world->m_simulation;
    float stepInfoa = world->m_dynamicsStepInfo.m_stepInfo.m_startTime - stepInfo.m_startTime;
    double v5 = stepInfoa;
    m_simulation->m_currentTime = stepInfoa + m_simulation->m_currentTime;
    m_simulation->m_currentPsiTime = world->m_dynamicsStepInfo.m_stepInfo.m_startTime;
    if (-1.0f != m_simulation->m_simulateUntilTime) {
        world->m_simulation->m_simulateUntilTime = world->m_simulation->m_simulateUntilTime + v5;
    }
    for (hkpSimulationIsland* pIsland : world->m_activeSimulationIslands) {
        hkpEntity* v9;
        for (int32_t j = 0; j < pIsland->m_entities.m_size;
             v9->m_motion.m_motionState.m_sweptTransform.m_centerOfMass0.m_quad[3] =
                 v5 + v9->m_motion.m_motionState.m_sweptTransform.m_centerOfMass0.m_quad[3]) {
            v9 = pIsland->m_entities.m_data[j++];
        }
        hkpWorldAgentUtil::warpTime(
            pIsland, stepInfo.m_endTime, world->m_dynamicsStepInfo.m_stepInfo.m_endTime, *world->m_collisionInput
        );
        v5 = stepInfoa;
    }
    world->m_simulation->warpTime(v5);
    stepInfo = world->m_dynamicsStepInfo.m_stepInfo;
    world->m_collisionInput->m_stepInfo = world->m_dynamicsStepInfo.m_stepInfo;
}
