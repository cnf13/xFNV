#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpRigidBodyCentreOfMassViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
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

hkpRigidBodyCentreOfMassViewer::hkpRigidBodyCentreOfMassViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts
)
  : hkpWorldViewerBase(contexts), hkpEntityListener(), hkpWorldPostSimulationListener(), m_entitiesCreated() {}

// Removes the viewer from the context's worlds; the bodies list goes with its member.
hkpRigidBodyCentreOfMassViewer::~hkpRigidBodyCentreOfMassViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            removeWorld(m_context->m_worlds.m_data[i]);
        }
    }
}

void hkpRigidBodyCentreOfMassViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            addWorld(m_context->m_worlds.m_data[i]);
            m_context = this->m_context;
        }
    }
}

void hkpRigidBodyCentreOfMassViewer::entityAddedCallback(hkpEntity* entity) {
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>* p_m_entitiesCreated = &m_entitiesCreated;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_entitiesCreated->m_size ==
        (p_m_entitiesCreated->m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_entitiesCreated, sizeof(hkpRigidBody*));
    }
    p_m_entitiesCreated->m_data[p_m_entitiesCreated->m_size++] = static_cast<hkpRigidBody*>(entity);
}

void hkpRigidBodyCentreOfMassViewer::entityRemovedCallback(hkpEntity* entity) {
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

// For each body with a mass, draws three lines from its centre of mass along the columns of its rotation, m_scale long,
// in red, green and blue.
void hkpRigidBodyCentreOfMassViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpRigidBodyCentreOfMassViewer", this);
    for (int32_t i = 0; i < m_entitiesCreated.m_size; ++i) {
        if (m_entitiesCreated.m_data[i]->getMass() != 0.0f) {
            HK_TIMER_BEGIN("getMassAndLines", this);
            hkVector4 centreOfMass = m_entitiesCreated.m_data[i]->getCenterOfMassInWorld();
            const __m128 scale = _mm_shuffle_ps(_mm_load_ss(&m_scale), _mm_load_ss(&m_scale), 0);
            hkVector4 xAxis;
            xAxis.m_quad = _mm_add_ps(
                _mm_mul_ps(scale, m_entitiesCreated.m_data[i]->getTransform().getColumn(0).m_quad), centreOfMass.m_quad
            );
            hkVector4 yAxis;
            yAxis.m_quad = _mm_add_ps(
                _mm_mul_ps(scale, m_entitiesCreated.m_data[i]->getTransform().getColumn(1).m_quad), centreOfMass.m_quad
            );
            hkVector4 zAxis;
            zAxis.m_quad = _mm_add_ps(
                _mm_mul_ps(scale, m_entitiesCreated.m_data[i]->getTransform().getColumn(2).m_quad), centreOfMass.m_quad
            );
            HK_TIMER_END();
            HK_TIMER_BEGIN("display3lines", this);
            // The id is an int display id: the low 32 bits of the collidable's address.
            const int32_t id =
                static_cast<int32_t>(reinterpret_cast<hkUlong>(m_entitiesCreated.m_data[i]->getCollidable()));
            m_displayHandler->displayLine(centreOfMass, xAxis, static_cast<int32_t>(hkColor::RED), id, m_tag);
            m_displayHandler->displayLine(centreOfMass, yAxis, static_cast<int32_t>(hkColor::GREEN), id, m_tag);
            m_displayHandler->displayLine(centreOfMass, zAxis, static_cast<int32_t>(hkColor::BLUE), id, m_tag);
            HK_TIMER_END();
        }
    }
    HK_TIMER_END();
}

void hkpRigidBodyCentreOfMassViewer::worldAddedCallback(hkpWorld* world) {
    addWorld(world);
}

void hkpRigidBodyCentreOfMassViewer::worldRemovedCallback(hkpWorld* world) {
    removeWorld(world);
}

// The full object as this: init calls it per world.
void hkpRigidBodyCentreOfMassViewer::addWorld(hkpWorld* world) {
    hkpEntityListener* v3;
    hkpWorldPostSimulationListener* v5;
    hkpRigidBodyCentreOfMassViewer* v2 = this;
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
void hkpRigidBodyCentreOfMassViewer::removeWorld(hkpWorld* world) {
    hkpEntityListener* v3;
    hkpWorldPostSimulationListener* v5;
    hkpRigidBodyCentreOfMassViewer* v2 = this;
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

hkProcess*
hkpRigidBodyCentreOfMassViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpRigidBodyCentreOfMassViewer* v2 = static_cast<hkpRigidBodyCentreOfMassViewer*>(
        Instance->m_heap->blockAlloc(sizeof(hkpRigidBodyCentreOfMassViewer))
    );
    v2->m_memSizeAndFlags = sizeof(hkpRigidBodyCentreOfMassViewer);
    hkpRigidBodyCentreOfMassViewer* v3 = ::new (v2) hkpRigidBodyCentreOfMassViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }
    else {
        return 0;
    }
}

void hkpRigidBodyCentreOfMassViewer::registerViewer() {
    hkpRigidBodyCentreOfMassViewer::m_tag =
        hkProcessFactory::s_instance->registerProcess("Center of Mass", &hkpRigidBodyCentreOfMassViewer::create);
}
