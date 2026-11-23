#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpConvexRadiusViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpConvexRadiusBuilder.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

hkpConvexRadiusViewer::hkpConvexRadiusViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpEntityListener(), hkpWorldPostSimulationListener(), m_worldEntities(),
    m_fixedObjectColor(s_convexRadiusFixedObjectColor), m_movableObjectColor(s_convexRadiusMovableObjectColor) {}

// Removes every world, last first; the world list goes with its member.
hkpConvexRadiusViewer::~hkpConvexRadiusViewer() {
    for (int32_t i = m_worldEntities.m_size - 1; i >= 0; --i) {
        removeWorld(i);
    }
}

void hkpConvexRadiusViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            addWorld(m_context->m_worlds.m_data[i]);
            m_context = this->m_context;
        }
    }
}

// Builds the display geometries of the entity's shape, drops the convex ones that have no geometry (with a report),
// adds the rest to the display handler under an id made from the collidable's address and colours them by motion type.
void hkpConvexRadiusViewer::entityAddedCallback(hkpEntity* entity) {
    if (!entity->m_collidable.m_shape) {
        return;
    }
    int32_t index = findWorld(entity->m_world);
    if (index < 0) {
        return;
    }
    WorldToEntityData* data = m_worldEntities.m_data[index];
    hkArray<hkDisplayGeometry*> displayGeometries;
    hkpConvexRadiusBuilder::hkpConvexRadiusBuilderEnvironment env;
    // The builder is constructed on raw stack storage and deliberately never destroyed.
    alignas(hkpConvexRadiusBuilder) uint8_t aBuilderMemory[sizeof(hkpConvexRadiusBuilder)];
    hkpConvexRadiusBuilder* pBuilder = ::new (aBuilderMemory) hkpConvexRadiusBuilder(env);
    pBuilder->buildDisplayGeometries(entity->m_collidable.m_shape, displayGeometries);

    for (int32_t i = displayGeometries.m_size - 1; i >= 0; i--) {
        hkDisplayGeometry* geometry = displayGeometries.m_data[i];
        if (geometry->m_type == HK_DISPLAY_CONVEX && !geometry->m_geometry) {
            {
                char buf[0x200];
                hkErrStream errStream(buf, 0x200);
                errStream << "Unable to build display geometry from hkpShape geometry data";
                hkError::s_instance->message(
                    hkError::MESSAGE_REPORT,
                    -1,
                    buf,
                    ".\\VisualDebugger\\Viewer\\Collide\\hkpConvexRadiusViewer.cpp",
                    0xA7
                );
            }
            displayGeometries.removeAt(i);
        }
    }

    if (displayGeometries.m_size > 0) {
        hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable) + 3;
        hkArray<hkUlong>& created = data->entitiesCreated;
        created.pushBack(id);
        m_displayHandler->addGeometry(displayGeometries, entity->m_motion.m_motionState.m_transform, id, m_tag, 0);
        if (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
            m_displayHandler->setGeometryColor(static_cast<int32_t>(m_fixedObjectColor), id, m_tag);
        }
        else {
            m_displayHandler->setGeometryColor(static_cast<int32_t>(m_movableObjectColor), id, m_tag);
        }
    }

    for (hkDisplayGeometry* geometry : displayGeometries) {
        if (geometry) {
            delete geometry;
        }
    }
}

// Removes the display object of an entity with a shape from the display handler and its id (the collidable's
// address plus 3, taken as a number) from the world's list.
void hkpConvexRadiusViewer::entityRemovedCallback(hkpEntity* entity) {
    if (!entity->m_collidable.m_shape) {
        return;
    }
    int32_t index = findWorld(entity->m_world);
    if (index < 0) {
        return;
    }
    WorldToEntityData* data = m_worldEntities.m_data[index];
    hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable) + 3;
    m_displayHandler->removeGeometry(id, m_tag, 0);
    hkArray<hkUlong>& created = data->entitiesCreated;
    int32_t found = created.indexOf(id);
    if (found >= 0) {
        created.removeAt(found);
    }
}

// Moves the display object of every entity of the active islands to the entity's transform at the world's current
// time, under the viewer's timer. The ids are the collidables' addresses plus 3, taken as numbers.
void hkpConvexRadiusViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpConvexRadiusViewer", this);
    for (hkpSimulationIsland* island : world->m_activeSimulationIslands) {
        const hkArrayBase<hkpEntity*>& entities = island->m_entities;
        for (hkpEntity* entity : entities) {
            alignas(16) hkTransform transform;
            float time = world->getCurrentTime();
            entity->m_motion.approxTransformAt(time, transform);
            m_displayHandler->updateGeometry(transform, reinterpret_cast<hkUlong>(&entity->m_collidable) + 3, m_tag);
        }
    }
    HK_TIMER_END();
}

void hkpConvexRadiusViewer::worldAddedCallback(hkpWorld* world) {
    addWorld(world);
}

void hkpConvexRadiusViewer::worldRemovedCallback(hkpWorld* world) {
    hkpConvexRadiusViewer* v2 = this;
    int32_t v3 = findWorld(world);
    if (v3 >= 0) {
        v2->removeWorld(v3);
    }
}

// Moves the display object of an entity that moved while inactive to the entity's transform. Its id is the collidable's
// address plus 3 taken as a number, as entityAddedCallback made it.
void hkpConvexRadiusViewer::inactiveEntityMovedCallback(hkpEntity* entity) {
    m_displayHandler->updateGeometry(
        entity->m_motion.m_motionState.m_transform,
        reinterpret_cast<hkUlong>(&entity->m_collidable) + 3,
        m_tag
    );
}

// The full object as this: init calls it per world.
void hkpConvexRadiusViewer::addWorld(hkpWorld* world) {
    hkpEntityListener* v4;
    hkpWorldPostSimulationListener* v6;
    hkpConvexRadiusViewer* v2 = this;
    if (this) {
        v4 = static_cast<hkpEntityListener*>(this);
    }
    else {
        v4 = nullptr;
    }
    world->addEntityListener(v4);
    if (v2) {
        v6 = static_cast<hkpWorldPostSimulationListener*>(v2);
    }
    else {
        v6 = nullptr;
    }
    world->addWorldPostSimulationListener(v6);
    hkpConvexRadiusViewer::WorldToEntityData* v3 = new hkpConvexRadiusViewer::WorldToEntityData;
    v3->world = world;
    v2->m_worldEntities.pushBack(v3);
    int32_t v10 = 0;
    for (int32_t i = 0; v10 < world->m_activeSimulationIslands.m_size; i = v10) {
        hkpSimulationIsland** m_data = world->m_activeSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &m_data[v10]->m_entities;
        int32_t v13 = 0;
        if (m_data[v10]->m_entities.m_size > 0) {
            hkpEntityListener* v14 = static_cast<hkpEntityListener*>(v2);
            do {
                v14->entityAddedCallback(p_m_entities->m_data[v13++]);
            }
            while (v13 < p_m_entities->m_size);
            v2 = this;
            v10 = i;
        }
        ++v10;
    }
    int32_t v15 = 0;
    for (int32_t j = 0; v15 < world->m_inactiveSimulationIslands.m_size; j = v15) {
        hkpSimulationIsland** v16 = world->m_inactiveSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* v17 = &v16[v15]->m_entities;
        int32_t v18 = 0;
        if (v16[v15]->m_entities.m_size > 0) {
            hkpEntityListener* v19 = static_cast<hkpEntityListener*>(v2);
            do {
                v19->entityAddedCallback(v17->m_data[v18++]);
            }
            while (v18 < v17->m_size);
            v15 = j;
            v2 = this;
        }
        ++v15;
    }
    hkpSimulationIsland* m_fixedIsland = world->getFixedIsland();
    if (m_fixedIsland) {
        for (int32_t k = 0; k < m_fixedIsland->m_entities.m_size; ++k) {
            v2->entityAddedCallback(m_fixedIsland->m_entities.m_data[k]);
        }
    }
}

// The index of world's entry in m_worldEntities, or -1.
int32_t hkpConvexRadiusViewer::findWorld(hkpWorld* world) {
    int32_t m_size = m_worldEntities.m_size;
    int32_t result = 0;
    if (m_size <= 0) {
        return -1;
    }
    for (hkpConvexRadiusViewer::WorldToEntityData** i =
             static_cast<hkArrayBase<hkpConvexRadiusViewer::WorldToEntityData*>&>(m_worldEntities).begin();
         (*i)->world != world;
         ++i) {
        if (++result >= m_size) {
            return -1;
        }
    }
    return result;
}

void hkpConvexRadiusViewer::removeWorld(int32_t i) {
    hkpEntityListener* v3;
    if (this) {
        v3 = static_cast<hkpEntityListener*>(this);
    }
    else {
        v3 = nullptr;
    }
    m_worldEntities.m_data[i]->world->removeEntityListener(v3);
    m_worldEntities.m_data[i]->world->removeWorldPostSimulationListener(this);
    removeAllGeometries(i);
    delete m_worldEntities.m_data[i];
    int32_t v8 = --m_worldEntities.m_size;
    if (v8 != i) {
        m_worldEntities.m_data[i] = m_worldEntities.m_data[v8];
    }
}

void hkpConvexRadiusViewer::removeAllGeometries(int32_t worldIndex) {
    hkpConvexRadiusViewer::WorldToEntityData* v3 = m_worldEntities.m_data[worldIndex];
    for (hkUlong id : v3->entitiesCreated) {
        m_displayHandler->removeGeometry(id, hkpConvexRadiusViewer::m_tag, 0);
    }
    v3->entitiesCreated.m_size = 0;
}

hkProcess* hkpConvexRadiusViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpConvexRadiusViewer* pViewer = new hkpConvexRadiusViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpConvexRadiusViewer::registerViewer() {
    hkpConvexRadiusViewer::m_tag = hkProcessFactory::s_instance->registerProcess(
        "Convex Radius",
        &hkpConvexRadiusViewer::create
    );
}
