#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpSimulationIslandViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <xmmintrin.h>

hkpSimulationIslandViewer::hkpSimulationIslandViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts
)
  : hkpWorldViewerBase(contexts), m_showActiveIslands(true), m_showInactiveIslands(true) {
    if (m_context) {
        for (hkpWorld* world : m_context->m_worlds) {
            world->addWorldPostSimulationListener(this);
        }
    }
}

hkpSimulationIslandViewer::~hkpSimulationIslandViewer() {
    if (m_context) {
        for (hkpWorld* world : m_context->m_worlds) {
            world->removeWorldPostSimulationListener(this);
        }
    }
}

// Draws a box around each active island (blue) and each inactive one (green), when shown: the union of its entities'
// broad phase boxes, starting from an empty box. The display geometries grow to the island count as needed. The active
// pass also fills a temporary array of empty boxes that nothing reads.
void hkpSimulationIslandViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpSimulationIslandViewer", this);
    if (m_showActiveIslands) {
        int32_t numIslands = world->m_activeSimulationIslands.m_size;
        if (numIslands > m_activeIslandDisplayGeometries.m_size) {
            m_activeIslandDisplayGeometries.setSize(numIslands);
        }

        hkArray<hkDisplayGeometry*, hkContainerTempAllocator> displayGeometries;
        displayGeometries.setSize(world->m_activeSimulationIslands.m_size);

        hkArray<hkAabb, hkContainerTempAllocator> islandAabbs;
        islandAabbs.setSize(world->m_activeSimulationIslands.m_size);

        for (int32_t i = 0; i < world->m_activeSimulationIslands.m_size; i++) {
            hkpSimulationIsland* island = world->m_activeSimulationIslands.m_data[i];
            islandAabbs.m_data[i].m_max.m_quad = _mm_set1_ps(-HK_REAL_MAX);
            islandAabbs.m_data[i].m_min.m_quad = _mm_set1_ps(HK_REAL_MAX);
            hkVector4 islandMax;
            islandMax.m_quad = _mm_set1_ps(-HK_REAL_MAX);
            hkVector4 islandMin;
            islandMin.m_quad = _mm_set1_ps(HK_REAL_MAX);
            hkInplaceArray<hkpEntity*, 1>& entities = island->m_entities;
            for (hkpEntity* entity : entities) {
                hkpCollidable* collidable = &entity->m_collidable;
                if (collidable->m_shape != nullptr) {
                    hkAabb entityAabb;
                    world->m_broadPhase->getAabb(collidable->getBroadPhaseHandle(), entityAabb);
                    islandMax.setMax4(islandMax, entityAabb.m_max);
                    islandMin.setMin4(islandMin, entityAabb.m_min);
                }
            }
            m_activeIslandDisplayGeometries.m_data[i].setExtents(islandMin, islandMax);
            displayGeometries.m_data[i] = &m_activeIslandDisplayGeometries.m_data[i];
        }
        m_displayHandler->displayGeometry(displayGeometries, static_cast<int32_t>(hkColor::BLUE), 0, m_tag);
    }
    if (m_showInactiveIslands) {
        int32_t numIslands = world->m_inactiveSimulationIslands.m_size;
        if (numIslands > m_inactiveIslandDisplayGeometries.m_size) {
            m_inactiveIslandDisplayGeometries.setSize(numIslands);
        }

        hkArray<hkDisplayGeometry*> displayGeometries;
        displayGeometries.setSize(world->m_inactiveSimulationIslands.m_size);

        for (int32_t i = 0; i < world->m_inactiveSimulationIslands.m_size; i++) {
            hkpSimulationIsland* island = world->m_inactiveSimulationIslands.m_data[i];
            hkVector4 islandMax;
            islandMax.m_quad = _mm_set1_ps(-HK_REAL_MAX);
            hkVector4 islandMin;
            islandMin.m_quad = _mm_set1_ps(HK_REAL_MAX);
            hkInplaceArray<hkpEntity*, 1>& entities = island->m_entities;
            for (hkpEntity* entity : entities) {
                hkpCollidable* collidable = &entity->m_collidable;
                if (collidable->m_shape != nullptr) {
                    hkAabb entityAabb;
                    world->m_broadPhase->getAabb(collidable->getBroadPhaseHandle(), entityAabb);
                    islandMax.setMax4(islandMax, entityAabb.m_max);
                    islandMin.setMin4(islandMin, entityAabb.m_min);
                }
            }
            m_inactiveIslandDisplayGeometries.m_data[i].setExtents(islandMin, islandMax);
            displayGeometries.m_data[i] = &m_inactiveIslandDisplayGeometries.m_data[i];
        }
        m_displayHandler->displayGeometry(displayGeometries, static_cast<int32_t>(hkColor::GREEN), 0, m_tag);
    }
    HK_TIMER_END();
}

void hkpSimulationIslandViewer::worldAddedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->addWorldPostSimulationListener(nullptr);
    }
    else {
        world->addWorldPostSimulationListener(this);
    }
}

void hkpSimulationIslandViewer::worldRemovedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->removeWorldPostSimulationListener(nullptr);
    }
    else {
        world->removeWorldPostSimulationListener(this);
    }
}

hkProcess* hkpSimulationIslandViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpSimulationIslandViewer* pViewer = new hkpSimulationIslandViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpSimulationIslandViewer::registerViewer() {
    hkpSimulationIslandViewer::m_tag = hkProcessFactory::s_instance->registerProcess(
        "Simulation Islands",
        &hkpSimulationIslandViewer::create
    );
}
