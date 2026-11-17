#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpMidphaseViewer.h"

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
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

// Adds the viewer to the context's worlds as a post-simulation listener.
hkpMidphaseViewer::hkpMidphaseViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpWorldPostSimulationListener(), m_broadPhaseDisplayGeometries() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->addWorldPostSimulationListener(this);
        }
    }
}

// Removes the viewer from the context's worlds; the display geometries go with their member.
hkpMidphaseViewer::~hkpMidphaseViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->removeWorldPostSimulationListener(this);
        }
    }
}

// Rebuilds the viewer's boxes from the child shape AABBs of every entity of the active, then the inactive islands
// (each 32-bit AABB expanded by its expansion bytes and unpacked with the collision input's bit scale and offset),
// and displays them in orange for this frame, under the viewer's timer.
void hkpMidphaseViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpMidphaseViewer", this);
    // EXE-INLINED none: hkArray<hkDisplayAABB>::clear
    {
        hkDisplayAABB* geometry = m_broadPhaseDisplayGeometries.m_data;
        for (int32_t n = m_broadPhaseDisplayGeometries.m_size; n > 0; n--) {
            geometry->~hkDisplayAABB();
            ++geometry;
        }
        m_broadPhaseDisplayGeometries.m_size = 0;
    }

    const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>* islands = &world->m_activeSimulationIslands;
    for (int32_t pass = 2; pass != 0; pass--) {
        for (int32_t i = 0; i < islands->m_size; i++) {
            const hkArrayBase<hkpEntity*>& entities = islands->m_data[i]->m_entities;
            for (int32_t j = 0; j < entities.m_size; j++) {
                const hkpCollidable::BoundingVolumeData& bvd = entities.m_data[j]->m_collidable.m_boundingVolumeData;
                const hkAabbUint32* childAabb = bvd.m_childShapeAabbs;
                if (!childAabb) {
                    continue;
                }
                for (int32_t k = 0; k < bvd.m_numChildShapeAabbs; k++, childAabb++) {
                    const hkpCollisionInput* input = world->m_collisionInput;
                    uint32_t shift = childAabb->m_expansionShift;
                    float minX = static_cast<float>(
                        childAabb->m_min[0] - (static_cast<uint32_t>(childAabb->m_expansionMin[0]) << shift)
                    );
                    float minY = static_cast<float>(
                        childAabb->m_min[1] - (static_cast<uint32_t>(childAabb->m_expansionMin[1]) << shift)
                    );
                    float minZ = static_cast<float>(
                        childAabb->m_min[2] - (static_cast<uint32_t>(childAabb->m_expansionMin[2]) << shift)
                    );
                    float maxX = static_cast<float>(
                        childAabb->m_max[0] + (static_cast<uint32_t>(childAabb->m_expansionMax[0]) << shift)
                    );
                    float maxY = static_cast<float>(
                        childAabb->m_max[1] + (static_cast<uint32_t>(childAabb->m_expansionMax[1]) << shift)
                    );
                    float maxZ = static_cast<float>(
                        childAabb->m_max[2] + (static_cast<uint32_t>(childAabb->m_expansionMax[2]) << shift)
                    );
                    alignas(16) hkVector4 minPoint;
                    minPoint.set(minX, minY, minZ, 0.0f);
                    alignas(16) hkVector4 maxPoint;
                    maxPoint.set(maxX, maxY, maxZ, 0.0f);
                    hkVector4 invScale;
                    invScale.setReciprocal3(input->m_aabb32Info.m_bitScale);
                    maxPoint.mul4(invScale);
                    minPoint.setMul4(invScale, minPoint);
                    minPoint.sub4(input->m_aabb32Info.m_bitOffsetLow);
                    maxPoint.sub4(input->m_aabb32Info.m_bitOffsetLow);

                    // EXE-INLINED none: hkArray<hkDisplayAABB>::expandOne
                    hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
                    int32_t size = m_broadPhaseDisplayGeometries.m_size;
                    int32_t required = size + 1;
                    int32_t capacity =
                        m_broadPhaseDisplayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayAABB>::CAPACITY_MASK;
                    if (capacity < required) {
                        int32_t doubled = 2 * capacity;
                        hkArrayUtil::_reserve(
                            heap, &m_broadPhaseDisplayGeometries, required < doubled ? doubled : required,
                            static_cast<int32_t>(sizeof(hkDisplayAABB))
                        );
                    }
                    hkDisplayAABB* slot = &m_broadPhaseDisplayGeometries.m_data[m_broadPhaseDisplayGeometries.m_size];
                    if (slot) {
                        ::new (slot) hkDisplayAABB;
                    }
                    m_broadPhaseDisplayGeometries.m_size++;
                    m_broadPhaseDisplayGeometries.m_data[size].setExtents(minPoint, maxPoint);
                }
            }
        }
        islands = &world->m_inactiveSimulationIslands;
    }

    int32_t numGeometries = m_broadPhaseDisplayGeometries.m_size;
    // EXE-INLINED none: hkArray<hkDisplayGeometry*>::hkArray(int)
    hkArrayBase<hkDisplayGeometry*> displayGeometries;
    displayGeometries.m_data = nullptr;
    displayGeometries.m_size = 0;
    displayGeometries.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
    hkDisplayGeometry** data = nullptr;
    int32_t count = numGeometries;
    if (numGeometries) {
        data = heap->_bufAlloc<hkDisplayGeometry*>(count);
    }
    displayGeometries.m_data = data;
    displayGeometries.m_size = count;
    displayGeometries.m_capacityAndFlags = count ? count : hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<hkDisplayGeometry*>::setSize
    hkMemoryAllocator* reserveHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t geometryCapacity = displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK;
    if (geometryCapacity < numGeometries) {
        int32_t doubled = 2 * geometryCapacity;
        hkArrayUtil::_reserve(
            *reserveHeap, &displayGeometries, numGeometries < doubled ? doubled : numGeometries,
            static_cast<int32_t>(sizeof(hkDisplayGeometry*))
        );
    }
    displayGeometries.m_size = numGeometries;
    for (int32_t i = 0; i < numGeometries; i++) {
        displayGeometries.m_data[i] = &m_broadPhaseDisplayGeometries.m_data[i];
    }
    m_displayHandler->displayGeometry(displayGeometries, static_cast<int32_t>(hkColor::ORANGE), 0, m_tag);
    HK_TIMER_END();
    // EXE-INLINED none: hkArray<hkDisplayGeometry*>::~hkArray
    hkMemoryAllocator* freeHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t capacityAndFlags = displayGeometries.m_capacityAndFlags;
    displayGeometries.m_size = 0;
    if (capacityAndFlags >= 0) {
        freeHeap->_bufFree<hkDisplayGeometry*>(
            displayGeometries.m_data, capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK
        );
    }
}

void hkpMidphaseViewer::worldAddedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->addWorldPostSimulationListener(0);
    }
    else {
        world->addWorldPostSimulationListener(this);
    }
}

void hkpMidphaseViewer::worldRemovedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->removeWorldPostSimulationListener(0);
    }
    else {
        world->removeWorldPostSimulationListener(this);
    }
}

hkProcess* hkpMidphaseViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpMidphaseViewer* v2 = static_cast<hkpMidphaseViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpMidphaseViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpMidphaseViewer);
    hkpMidphaseViewer* v3 = ::new (v2) hkpMidphaseViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }
    else {
        return 0;
    }
}

void hkpMidphaseViewer::registerViewer() {
    hkpMidphaseViewer::m_tag = hkProcessFactory::s_instance->registerProcess("Midphase", &hkpMidphaseViewer::create);
}
