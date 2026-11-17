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
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->addWorldPostSimulationListener(this);
        }
    }
}

// Removes the viewer from the context's worlds; the display geometries go with their member.
hkpBroadphaseViewer::~hkpBroadphaseViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->removeWorldPostSimulationListener(this);
        }
    }
}

void hkpBroadphaseViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpBroadphaseViewer", this);

    // The SDK's hkArray<hkAabb> allAabbs(1024): heap storage for 1024 boxes, the size set to it; a zero-sized
    // buffer leaves the array flagged DONT_DEALLOCATE.
    // Raw storage: the body frees the storage itself, so no C++ destructor may free it again.
    alignas(16) FNVD_INLINED_LOCAL(hkArray<hkAabb>, allAabbs, std::nullopt, std::nullopt);
    allAabbs.m_data = nullptr;
    allAabbs.m_size = 0;
    allAabbs.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG);
    int32_t numAabbs = 1024;
    hkAabb* pAabbs = hkMemoryRouter::getInstance().heap()._bufAlloc<hkAabb>(numAabbs);
    allAabbs.m_data = pAabbs;
    allAabbs.m_capacityAndFlags = numAabbs ? numAabbs : static_cast<int32_t>(hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG);
    allAabbs.m_size = 1024;
    world->getBroadPhase()->getAllAabbs(allAabbs);

    if (allAabbs.m_size > m_broadPhaseDisplayGeometries.m_size) {
        // The SDK's m_broadPhaseDisplayGeometries.setSize(allAabbs.getSize()): reserve, destroy the elements past the
        // new size through the virtual destructor, construct the new ones in place.
        // EXE-INLINED none: hkArrayBase<hkDisplayAABB>::_setSize
        const int32_t newSize = allAabbs.m_size;
        m_broadPhaseDisplayGeometries.reserve(newSize);
        hkDisplayAABB* pDead = m_broadPhaseDisplayGeometries.m_data + newSize;
        for (int32_t n = m_broadPhaseDisplayGeometries.m_size - newSize; n > 0; --n) {
            pDead->~hkDisplayAABB();
            ++pDead;
        }
        hkDisplayAABB* pNew = m_broadPhaseDisplayGeometries.m_data + m_broadPhaseDisplayGeometries.m_size;
        for (int32_t n = newSize - m_broadPhaseDisplayGeometries.m_size; n > 0; --n) {
            // Placement new's null test.
            if (pNew) {
                ::new (pNew) hkDisplayAABB;
            }
            ++pNew;
        }
        m_broadPhaseDisplayGeometries.m_size = newSize;
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
    m_displayHandler->displayGeometry(
        displayGeometries, static_cast<int32_t>(hkColor::RED), 0, hkpBroadphaseViewer::m_tag
    );

    HK_TIMER_END();

    // The arrays' inline destructors: the size cleared, the heap storage freed unless flagged DONT_DEALLOCATE.
    displayGeometries.clearAndDeallocate();
    // hkAabb has no destructor to run, so its array's clear only zeroes the size.
    // EXE-INLINED none: hkArray<hkAabb>::~hkArray
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    allAabbs.m_size = 0;
    if ((allAabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG) == 0) {
        heap._bufFree<hkAabb>(allAabbs.m_data, allAabbs.getCapacity());
    }
}

void hkpBroadphaseViewer::worldAddedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->addWorldPostSimulationListener(0);
    }
    else {
        world->addWorldPostSimulationListener(this);
    }
}

void hkpBroadphaseViewer::worldRemovedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->removeWorldPostSimulationListener(0);
    }
    else {
        world->removeWorldPostSimulationListener(this);
    }
}

hkProcess* hkpBroadphaseViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpBroadphaseViewer* v2 =
        static_cast<hkpBroadphaseViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpBroadphaseViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpBroadphaseViewer);
    hkpBroadphaseViewer* v3 = ::new (v2) hkpBroadphaseViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }
    else {
        return 0;
    }
}

void hkpBroadphaseViewer::registerViewer() {
    hkpBroadphaseViewer::m_tag =
        hkProcessFactory::s_instance->registerProcess("Broadphase", &hkpBroadphaseViewer::create);
}
