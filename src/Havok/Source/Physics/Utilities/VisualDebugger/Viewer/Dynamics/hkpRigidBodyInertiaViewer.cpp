#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpRigidBodyInertiaViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayBox.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <cmath>

hkpRigidBodyInertiaViewer::hkpRigidBodyInertiaViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts
)
  : hkpWorldViewerBase(contexts), hkpEntityListener(), hkpWorldPostSimulationListener(), m_entitiesCreated(),
    m_displayBoxes() {}

// Removes the viewer from the context's worlds (over the world count read once, as the game does); the display boxes
// and the bodies list go with their members.
hkpRigidBodyInertiaViewer::~hkpRigidBodyInertiaViewer() {
    if (m_context) {
        int32_t size = m_context->m_worlds.m_size;
        for (int32_t i = 0; i < size; ++i) {
            removeWorld(m_context->m_worlds.m_data[i]);
        }
    }
}

void hkpRigidBodyInertiaViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        int32_t m_size = m_context->m_worlds.m_size;
        for (int32_t i = 0; i < m_size; ++i) {
            addWorld(this->m_context->m_worlds.m_data[i]);
        }
    }
}

void hkpRigidBodyInertiaViewer::entityAddedCallback(hkpEntity* entity) {
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>* p_m_entitiesCreated = &m_entitiesCreated;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_entitiesCreated->m_size ==
        (p_m_entitiesCreated->m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_entitiesCreated, sizeof(hkpRigidBody*));
    }
    p_m_entitiesCreated->m_data[p_m_entitiesCreated->m_size++] = static_cast<hkpRigidBody*>(entity);
}

void hkpRigidBodyInertiaViewer::entityRemovedCallback(hkpEntity* entity) {
    int32_t m_size = m_entitiesCreated.m_size;
    int32_t v3 = 0;
    if (m_size > 0) {
        for (hkpRigidBody** i = m_entitiesCreated.begin(); *i != entity; ++i) {
            if (++v3 >= m_size) {
                return;
            }
        }
        if (v3 >= 0) {
            int32_t v5 = --m_entitiesCreated.m_size;
            if (v5 != v3) {
                m_entitiesCreated.m_data[v3] = m_entitiesCreated.m_data[v5];
            }
        }
    }
}

// The SDK's override: each rigid body with a mass gets the box of uniform density with its inertia tensor's diagonal,
// 1% larger, about its centre of mass, and the boxes go to the display handler as one geometry list. The extents come
// from the diagonal as Ixx = m/12 (y^2 + z^2) and so on, each square root taken only of a value that is not negative.
void hkpRigidBodyInertiaViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpRigidBodyInertiaViewer", this);
    HK_TIMER_BEGIN("getProps", this);

    {
        // EXE-INLINED none: hkArrayBase<hkDisplayBox>::_setSize
        const int32_t newSize = m_entitiesCreated.m_size;
        m_displayBoxes.reserve(newSize);
        hkDisplayBox* pDead = m_displayBoxes.m_data + newSize;
        for (int32_t n = m_displayBoxes.m_size - newSize; n > 0; --n) {
            pDead->~hkDisplayBox();
            ++pDead;
        }
        hkDisplayBox* pNew = m_displayBoxes.m_data + m_displayBoxes.m_size;
        for (int32_t n = newSize - m_displayBoxes.m_size; n > 0; --n) {
            // Placement new's null test.
            if (pNew) {
                ::new (pNew) hkDisplayBox;
            }
            ++pNew;
        }
        m_displayBoxes.m_size = newSize;
    }

    int32_t numBoxes = 0;
    for (int32_t i = 0; i < m_entitiesCreated.m_size; ++i) {
        const float mass = m_entitiesCreated.m_data[i]->getMass();
        if (mass != 0.0f) {
            hkMatrix3 inertia;
            m_entitiesCreated.m_data[i]->getInertiaLocal(inertia);

            // The squared full extents, each rounded to float.
            const float extentYSq = (inertia(0, 0) - inertia(1, 1) + inertia(2, 2)) / mass * 6.0;
            float extentY;
            if (extentYSq < 0.0f) {
                extentY = 0.0f;
            }
            else {
                extentY = std::sqrt(extentYSq);
            }
            const float extentZSq = inertia(0, 0) / mass * 12.0 - extentYSq;
            float extentZ;
            if (extentZSq < 0.0f) {
                extentZ = 0.0f;
            }
            else {
                extentZ = std::sqrt(extentZSq);
            }
            const float extentXSq = inertia(2, 2) / mass * 12.0 - extentYSq;
            float extentX;
            if (extentXSq < 0.0f) {
                extentX = 0.0f;
            }
            else {
                extentX = std::sqrt(extentXSq);
            }

            const float halfX = extentX * 1.0099999904632568 * 0.5;
            const float halfY = extentY * 1.0099999904632568 * 0.5;
            const float halfZ = extentZ * 1.0099999904632568 * 0.5;
            hkVector4 halfExtents;
            halfExtents.set(halfX, halfY, halfZ, 0.0f);

            hkTransform t = m_entitiesCreated.m_data[i]->getTransform();
            hkVector4 centerOfMass = m_entitiesCreated.m_data[i]->getCenterOfMassLocal();
            hkVector4 centerShift;
            centerShift.setRotatedDir(t.getRotation(), centerOfMass);
            t.getTranslation().add4(centerShift);
            m_displayBoxes.m_data[numBoxes++].setParameters(halfExtents, t);
        }
    }

    HK_TIMER_END();
    HK_TIMER_BEGIN("sendProps", this);

    {
        // EXE-INLINED none: hkArrayBase<hkDisplayBox>::_setSize
        const int32_t newSize = numBoxes;
        m_displayBoxes.reserve(newSize);
        hkDisplayBox* pDead = m_displayBoxes.m_data + newSize;
        for (int32_t n = m_displayBoxes.m_size - newSize; n > 0; --n) {
            pDead->~hkDisplayBox();
            ++pDead;
        }
        hkDisplayBox* pNew = m_displayBoxes.m_data + m_displayBoxes.m_size;
        for (int32_t n = newSize - m_displayBoxes.m_size; n > 0; --n) {
            // Placement new's null test.
            if (pNew) {
                ::new (pNew) hkDisplayBox;
            }
            ++pNew;
        }
        m_displayBoxes.m_size = newSize;
    }

    // EXE-INLINED none: hkArray<hkDisplayGeometry*>::hkArray()
    FNVD_INLINED_LOCAL(hkArray<hkDisplayGeometry*>, displayGeometries, std::nullopt, std::nullopt);
    displayGeometries.m_data = nullptr;
    displayGeometries.m_size = 0;
    displayGeometries.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG);
    displayGeometries.setSize(m_displayBoxes.m_size);
    for (int32_t i = m_displayBoxes.m_size - 1; i >= 0; --i) {
        displayGeometries.m_data[i] = &m_displayBoxes.m_data[i];
    }
    m_displayHandler->displayGeometry(
        displayGeometries, static_cast<int32_t>(hkColor::MAGENTA), 0, hkpRigidBodyInertiaViewer::m_tag
    );

    HK_TIMER_END();
    HK_TIMER_END();

    // The array's inline destructor: the size cleared, the heap storage freed unless flagged DONT_DEALLOCATE.
    displayGeometries.clearAndDeallocate();
}

void hkpRigidBodyInertiaViewer::worldAddedCallback(hkpWorld* world) {
    addWorld(world);
}

// Removes the world.
void hkpRigidBodyInertiaViewer::worldRemovedCallback(hkpWorld* world) {
    removeWorld(world);
}

// The full object as this: init calls it per world.
void hkpRigidBodyInertiaViewer::addWorld(hkpWorld* world) {
    hkpEntityListener* v3;
    hkpWorldPostSimulationListener* v5;
    hkpRigidBodyInertiaViewer* v2 = this;
    if (this) {
        v3 = static_cast<hkpEntityListener*>(this);
    }
    else {
        v3 = 0;
    }
    world->addEntityListener(v3);
    if (v2) {
        v5 = static_cast<hkpWorldPostSimulationListener*>(v2);
    }
    else {
        v5 = 0;
    }
    world->addWorldPostSimulationListener(v5);
    int32_t v6 = 0;
    for (int32_t i = 0; v6 < world->m_activeSimulationIslands.m_size; i = v6) {
        hkpSimulationIsland** m_data = world->m_activeSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &m_data[v6]->m_entities;
        int32_t v9 = 0;
        if (m_data[v6]->m_entities.m_size > 0) {
            hkpEntityListener* v10 = static_cast<hkpEntityListener*>(v2);
            do {
                v10->entityAddedCallback(p_m_entities->m_data[v9++]);
            }
            while (v9 < p_m_entities->m_size);
            v6 = i;
            v2 = this;
        }
        ++v6;
    }
    int32_t v11 = 0;
    for (int32_t j = 0; v11 < world->m_inactiveSimulationIslands.m_size; j = v11) {
        hkpSimulationIsland** v12 = world->m_inactiveSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* v13 = &v12[v11]->m_entities;
        int32_t v14 = 0;
        if (v12[v11]->m_entities.m_size > 0) {
            hkpEntityListener* v15 = static_cast<hkpEntityListener*>(v2);
            do {
                v15->entityAddedCallback(v13->m_data[v14++]);
            }
            while (v14 < v13->m_size);
            v11 = j;
            v2 = this;
        }
        ++v11;
    }
    hkpSimulationIsland* m_fixedIsland = world->getFixedIsland();
    if (m_fixedIsland) {
        for (int32_t k = 0; k < m_fixedIsland->m_entities.m_size; ++k) {
            v2->entityAddedCallback(m_fixedIsland->m_entities.m_data[k]);
        }
    }
}

// The full object as this: the destructor calls it per world.
void hkpRigidBodyInertiaViewer::removeWorld(hkpWorld* world) {
    hkpEntityListener* v3;
    hkpWorldPostSimulationListener* v5;
    hkpRigidBodyInertiaViewer* v2 = this;
    if (this) {
        v3 = static_cast<hkpEntityListener*>(this);
    }
    else {
        v3 = 0;
    }
    world->removeEntityListener(v3);
    if (v2) {
        v5 = static_cast<hkpWorldPostSimulationListener*>(v2);
    }
    else {
        v5 = 0;
    }
    world->removeWorldPostSimulationListener(v5);
    int32_t v6 = 0;
    for (int32_t worlda = 0; v6 < world->m_activeSimulationIslands.m_size; worlda = v6) {
        hkpSimulationIsland** m_data = world->m_activeSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &m_data[v6]->m_entities;
        int32_t v9 = 0;
        if (m_data[v6]->m_entities.m_size > 0) {
            hkpEntityListener* v10 = static_cast<hkpEntityListener*>(v2);
            do {
                v10->entityRemovedCallback(p_m_entities->m_data[v9++]);
            }
            while (v9 < p_m_entities->m_size);
            v6 = worlda;
            v2 = this;
        }
        ++v6;
    }
    int32_t v11 = 0;
    for (int32_t worldb = 0; v11 < world->m_inactiveSimulationIslands.m_size; worldb = v11) {
        hkpSimulationIsland** v12 = world->m_inactiveSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* v13 = &v12[v11]->m_entities;
        int32_t v14 = 0;
        if (v12[v11]->m_entities.m_size > 0) {
            hkpEntityListener* v15 = static_cast<hkpEntityListener*>(v2);
            do {
                v15->entityRemovedCallback(v13->m_data[v14++]);
            }
            while (v14 < v13->m_size);
            v11 = worldb;
            v2 = this;
        }
        ++v11;
    }
    hkpSimulationIsland* m_fixedIsland = world->getFixedIsland();
    if (m_fixedIsland) {
        for (int32_t i = 0; i < m_fixedIsland->m_entities.m_size; ++i) {
            v2->entityRemovedCallback(m_fixedIsland->m_entities.m_data[i]);
        }
    }
}

hkProcess* hkpRigidBodyInertiaViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpRigidBodyInertiaViewer* v2 =
        static_cast<hkpRigidBodyInertiaViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpRigidBodyInertiaViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpRigidBodyInertiaViewer);
    hkpRigidBodyInertiaViewer* v3 = ::new (v2) hkpRigidBodyInertiaViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }
    else {
        return 0;
    }
}

void hkpRigidBodyInertiaViewer::registerViewer() {
    hkpRigidBodyInertiaViewer::m_tag =
        hkProcessFactory::s_instance->registerProcess("Inertia Tensors", &hkpRigidBodyInertiaViewer::create);
}
