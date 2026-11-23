#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpBroadphaseViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// Adds the viewer to the context's worlds as a post-simulation listener.
hkpBroadphaseViewer::hkpBroadphaseViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpWorldPostSimulationListener(), m_broadPhaseDisplayGeometries() {
    if (m_context) {
        for (hkpWorld* world : m_context->m_worlds) {
            world->addWorldPostSimulationListener(this);
        }
    }
}

// Removes the viewer from the context's worlds; the display geometries go with their member.
hkpBroadphaseViewer::~hkpBroadphaseViewer() {
    if (m_context) {
        for (hkpWorld* world : m_context->m_worlds) {
            world->removeWorldPostSimulationListener(this);
        }
    }
}

void hkpBroadphaseViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpBroadphaseViewer", this);

    alignas(16) hkArray<hkAabb> allAabbs(1024);
    world->getBroadPhase()->getAllAabbs(allAabbs);

    if (allAabbs.m_size > m_broadPhaseDisplayGeometries.m_size) {
        m_broadPhaseDisplayGeometries.setSize(allAabbs.m_size);
    }

    alignas(16) hkArray<hkDisplayGeometry*> displayGeometries;
    displayGeometries.m_data = nullptr;
    displayGeometries.m_size = 0;
    displayGeometries.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG);
    displayGeometries.setSize(allAabbs.m_size);
    for (int32_t i = allAabbs.m_size - 1; i >= 0; --i) {
        m_broadPhaseDisplayGeometries.m_data[i].setExtents(allAabbs.m_data[i].m_min, allAabbs.m_data[i].m_max);
        displayGeometries.m_data[i] = &m_broadPhaseDisplayGeometries.m_data[i];
    }
    m_displayHandler
        ->displayGeometry(displayGeometries, static_cast<int32_t>(hkColor::RED), 0, hkpBroadphaseViewer::m_tag);

    HK_TIMER_END();

    // The array's inline destructor: the size cleared, the heap storage freed unless flagged DONT_DEALLOCATE.
    displayGeometries.clearAndDeallocate();
}

void hkpBroadphaseViewer::worldAddedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->addWorldPostSimulationListener(nullptr);
    }
    else {
        world->addWorldPostSimulationListener(this);
    }
}

void hkpBroadphaseViewer::worldRemovedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->removeWorldPostSimulationListener(nullptr);
    }
    else {
        world->removeWorldPostSimulationListener(this);
    }
}

hkProcess* hkpBroadphaseViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpBroadphaseViewer* pViewer = new hkpBroadphaseViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpBroadphaseViewer::registerViewer() {
    hkpBroadphaseViewer::m_tag = hkProcessFactory::s_instance->registerProcess(
        "Broadphase",
        &hkpBroadphaseViewer::create
    );
}
