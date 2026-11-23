#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpSweptTransformDisplayViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/hkpShapeDisplayBuilder.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <xmmintrin.h>

int32_t HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T0 = hkColor::rgbFromChars(200, 200, 255, 80);
int32_t HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T1 = hkColor::rgbFromChars(200, 255, 200, 150);

hkpSweptTransformDisplayViewer::hkpSweptTransformDisplayViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts
)
  : hkpWorldViewerBase(contexts) {}

// Removing each world removes its display geometries and frees its record; the array frees itself.
hkpSweptTransformDisplayViewer::~hkpSweptTransformDisplayViewer() {
    for (int32_t i = m_worldEntities.m_size - 1; i >= 0; --i) {
        removeWorld(i);
    }
}

void hkpSweptTransformDisplayViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            addWorld(m_context->m_worlds.m_data[i]);
            m_context = this->m_context;
        }
    }
}

// Builds the display geometries of the entity's shape, drops the convex ones without geometry with a report and, unless
// the entity is fixed, adds them twice: at the swept transform's start and end, under the collidable's address plus 1
// and plus 2 (the address is treated as an untyped number), each id kept in the world's list.
void hkpSweptTransformDisplayViewer::entityAddedCallback(hkpEntity* entity) {
    if (!entity->m_collidable.m_shape) {
        return;
    }
    int32_t index = findWorld(entity->m_world);
    if (index < 0) {
        return;
    }
    WorldToEntityData* data = m_worldEntities.m_data[index];
    hkArray<hkDisplayGeometry*> displayGeometries;
    // No destructor is run for either env or builder.
    hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment env;
    hkpShapeDisplayBuilder builder(env);
    builder.buildDisplayGeometries(entity->m_collidable.m_shape, displayGeometries);

    for (int32_t i = displayGeometries.m_size - 1; i >= 0; i--) {
        hkDisplayGeometry* geometry = displayGeometries.m_data[i];
        if (geometry->m_type == HK_DISPLAY_CONVEX && !geometry->m_geometry) {
            {
                alignas(16) char buf[512];
                hkErrStream errStream(buf, 512);
                errStream << "Unable to build display geometry from hkpShape geometry data";
                hkError::s_instance->message(
                    hkError::MESSAGE_REPORT,
                    -1,
                    buf,
                    ".\\VisualDebugger\\Viewer\\Dynamics\\hkpSweptTransformDisplayViewer.cpp",
                    168
                );
            }
            displayGeometries.removeAt(i);
        }
    }

    if (displayGeometries.m_size > 0 && entity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
        const hkSweptTransform& swept = entity->m_motion.m_motionState.m_sweptTransform;
        const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
        hkArray<hkUlong>& created = data->entitiesCreated;
        created.pushBack(id + 1);

        hkTransform trans0;
        hkSweptTransformUtil::calcTransAtT0(swept, trans0);

        hkTransform trans1;
        hkSweptTransformUtil::calcTransAtT1(swept, trans1);

        m_displayHandler->addGeometry(displayGeometries, trans0, id + 1, m_tag, 0);
        m_displayHandler->setGeometryColor(HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T0, id + 1, m_tag);
        created.pushBack(id + 2);
        m_displayHandler->addGeometry(displayGeometries, trans1, id + 2, m_tag, 0);
        m_displayHandler->setGeometryColor(HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T1, id + 2, m_tag);
    }

    for (hkDisplayGeometry* geometry : displayGeometries) {
        if (geometry) {
            delete geometry;
        }
    }
}

// Removes the entity's two display objects and their ids from the world's list. The ids are the collidable's address
// plus 1 and plus 2, with the address treated as an untyped number.
void hkpSweptTransformDisplayViewer::entityRemovedCallback(hkpEntity* entity) {
    if (!entity->m_collidable.m_shape) {
        return;
    }
    int32_t index = findWorld(entity->m_world);
    if (index < 0) {
        return;
    }
    WorldToEntityData* data = m_worldEntities.m_data[index];
    hkArray<hkUlong>& created = data->entitiesCreated;
    const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
    for (uint32_t step = 1; step <= 2; step++) {
        m_displayHandler->removeGeometry(id + step, m_tag, 0);
        const int32_t found = created.indexOf(id + step);
        if (found >= 0) {
            created.removeAt(found);
        }
    }
}

// Moves the two display objects of every entity of the active islands to the entity's swept transform at the step's
// start and end, under the viewer's timer. Their ids are the collidable's address plus 1 and plus 2, with the address
// treated as an untyped number.
void hkpSweptTransformDisplayViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpSweptTransformDisplayViewer", this);
    for (hkpSimulationIsland* island : world->m_activeSimulationIslands) {
        const hkArrayBase<hkpEntity*>& entities = island->m_entities;
        for (hkpEntity* entity : entities) {
            const hkSweptTransform& swept = entity->m_motion.m_motionState.m_sweptTransform;
            const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
            hkTransform trans;

            hkSweptTransformUtil::calcTransAtT0(swept, trans);
            m_displayHandler->updateGeometry(trans, id + 1, m_tag);

            hkSweptTransformUtil::calcTransAtT1(swept, trans);
            m_displayHandler->updateGeometry(trans, id + 2, m_tag);
        }
    }
    HK_TIMER_END();
}

void hkpSweptTransformDisplayViewer::worldAddedCallback(hkpWorld* world) {
    addWorld(world);
}

void hkpSweptTransformDisplayViewer::worldRemovedCallback(hkpWorld* world) {
    hkpSweptTransformDisplayViewer* v2 = this;
    int32_t v3 = findWorld(world);
    if (v3 >= 0) {
        v2->removeWorld(v3);
    }
}

// Moves the two display objects of an entity that moved while inactive to its swept transform's start and end. Their
// ids are the collidable's address plus 1 and plus 2, with the address treated as an untyped number.
void hkpSweptTransformDisplayViewer::inactiveEntityMovedCallback(hkpEntity* entity) {
    const hkSweptTransform& swept = entity->m_motion.m_motionState.m_sweptTransform;
    const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
    hkTransform trans;

    hkSweptTransformUtil::calcTransAtT0(swept, trans);
    m_displayHandler->updateGeometry(trans, id + 1, m_tag);

    hkSweptTransformUtil::calcTransAtT1(swept, trans);
    m_displayHandler->updateGeometry(trans, id + 2, m_tag);
}

// The full object as this: init calls it per world.
void hkpSweptTransformDisplayViewer::addWorld(hkpWorld* world) {
    hkpEntityListener* v4;
    hkpWorldPostSimulationListener* v6;
    hkpSweptTransformDisplayViewer* v2 = this;
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
    hkpSweptTransformDisplayViewer::WorldToEntityData* v3 = new hkpSweptTransformDisplayViewer::WorldToEntityData;
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
    hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* v20 = &world->getFixedIsland()->m_entities;
    for (int32_t k = 0; k < v20->m_size; ++k) {
        v2->entityAddedCallback(v20->m_data[k]);
    }
}

int32_t hkpSweptTransformDisplayViewer::findWorld(hkpWorld* world) {
    int32_t m_size = m_worldEntities.m_size;
    int32_t result = 0;
    if (m_size <= 0) {
        return -1;
    }
    for (hkpSweptTransformDisplayViewer::WorldToEntityData** i =
             static_cast<hkArrayBase<hkpSweptTransformDisplayViewer::WorldToEntityData*>&>(m_worldEntities).begin();
         (*i)->world != world;
         ++i) {
        if (++result >= m_size) {
            return -1;
        }
    }
    return result;
}

void hkpSweptTransformDisplayViewer::removeWorld(int32_t i) {
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

void hkpSweptTransformDisplayViewer::removeAllGeometries(int32_t worldIndex) {
    hkpSweptTransformDisplayViewer::WorldToEntityData* v3 = m_worldEntities.m_data[worldIndex];
    for (hkUlong id : v3->entitiesCreated) {
        m_displayHandler->removeGeometry(id, m_tag, 0);
    }
    v3->entitiesCreated.m_size = 0;
}

hkProcess*
hkpSweptTransformDisplayViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpSweptTransformDisplayViewer* pViewer = new hkpSweptTransformDisplayViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpSweptTransformDisplayViewer::registerViewer() {
    m_tag = hkProcessFactory::s_instance->registerProcess("Swept Transforms", &hkpSweptTransformDisplayViewer::create);
}
