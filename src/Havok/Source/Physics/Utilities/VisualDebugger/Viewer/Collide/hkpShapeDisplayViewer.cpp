#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpShapeDisplayViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayConvex.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpProperty.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/hkpShapeDisplayBuilder.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <xmmintrin.h>

const hkpShape* getUserShape(char noStreams, uint8_t* overriddenWithNone, hkpEntity* entity) {
    hkpPropertyValue shapeProperty;
    shapeProperty = entity->getProperty(0x1135u, hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE);
    const hkpShape* m_data = static_cast<const hkpShape*>(
        entity->getProperty(0x1235u, hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE).getPtr()
    );
    const hkpShape* v5 = static_cast<const hkpShape*>(
        entity->getProperty(0x1234u, hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE).getPtr()
    );
    const hkpShape* v6 = nullptr;
    *overriddenWithNone = 0;
    if (noStreams) {
        int32_t v7 = entity->m_properties.m_size;
        hkpProperty* v9 = entity->m_properties.begin();
        for (int32_t v8 = 0; v8 < v7; ++v8, ++v9) {
            if (v9->m_key == 0x1234) {
                if (!v5) {
                    *overriddenWithNone = 1;
                }
                return v5;
            }
        }
    }
    int32_t m_size = entity->m_properties.m_size;
    hkpProperty* v12 = entity->m_properties.begin();
    for (int32_t v11 = 0; v11 < m_size; ++v11, ++v12) {
        if (v12->m_key == 0x1235) {
            if (m_data) {
                return m_data;
            }

            *overriddenWithNone = 1;
            return nullptr;
        }
    }
    hkpProperty* i = entity->m_properties.begin();
    for (int32_t v13 = 0; v13 < m_size; ++v13, ++i) {
        if (i->m_key == 0x1135) {
            v6 = static_cast<const hkpShape*>(shapeProperty.getPtr());
            if (v6) {
                return v6;
            }

            *overriddenWithNone = 1;
            return nullptr;
        }
    }
    return v6;
}

hkpShapeDisplayViewer::hkpShapeDisplayViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpEntityListener(), hkpWorldPostSimulationListener(), m_worldEntities(),
    m_instancedShapeToGeomID(), m_instancedShapeToUsageCount(), m_cachedShapes(), m_enableShapeTransformUpdate(true),
    m_enableInstancing(false), m_enableDisplayCaching(false), m_enableDisplayCreation(true),
    m_autoGeometryCreation(true), m_enableAutoColor(true), m_enableAutoColorOverride(false), m_timeForDisplay(-1.0f),
    m_fixedObjectColor(s_fixedObjectColor), m_movableObjectColor(s_movableObjectColor) {
    // The first options context among the contexts sets whether the transforms are kept up to date.
    for (hkProcessContext* context : contexts) {
        if (!hkString::strCmp("ShapeDisplayViewerOptions", context->getType())) {
            ShapeDisplayViewerOptions* options = static_cast<ShapeDisplayViewerOptions*>(context);
            m_enableShapeTransformUpdate.m_bool = options->m_enableShapeTransformUpdate.m_bool;
            break;
        }
    }
}

// Removes every world, last first; the maps and the world list go with their members.
hkpShapeDisplayViewer::~hkpShapeDisplayViewer() {
    for (int32_t i = m_worldEntities.m_size - 1; i >= 0; --i) {
        removeWorld(i);
    }
}

void hkpShapeDisplayViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            addWorld(m_context->m_worlds.m_data[i]);
            m_context = this->m_context;
        }
    }
}

// Sends the display handler the transforms of the active islands' entities at the display time (the world's time
// unless one is set).
void hkpShapeDisplayViewer::synchronizeTransforms(hkDebugDisplayHandler* displayHandler, hkpWorld* world) {
    const float time = (m_timeForDisplay > 0.0f) ? m_timeForDisplay : world->getCurrentTime();
    displayHandler->lockForUpdate();
    for (int32_t i = 0; i < world->m_activeSimulationIslands.m_size; ++i) {
        hkpSimulationIsland* island = world->m_activeSimulationIslands.m_data[i];
        for (int32_t j = 0; j < island->m_entities.m_size; ++j) {
            hkpEntity* entity = island->m_entities.m_data[j];
            const hkUlong id = hkpShapeDisplayViewer::getId(entity);
            alignas(16) hkTransform transform;
            entity->m_motion.approxTransformAt(time, transform);
            displayHandler->updateGeometry(transform, id, m_tag);
        }
    }
    displayHandler->unlockForUpdate();
}

void hkpShapeDisplayViewer::synchronizeTransforms(hkpWorld* world) {
    synchronizeTransforms(m_displayHandler, world);
}

// Adds the world.
void hkpShapeDisplayViewer::worldAddedCallback(hkpWorld* world) {
    addWorld(world);
}

void hkpShapeDisplayViewer::worldRemovedCallback(hkpWorld* world) {
    hkpShapeDisplayViewer* v2 = this;
    int32_t v3 = findWorld(world);
    if (v3 >= 0) {
        v2->removeWorld(v3);
    }
}

// Gives the entity a display object: the shape getUserShape picks (else the collidable's), as an instance of a shape
// already shown, or as display geometries built from the shape or from an override geometry property, coloured by
// its colour property or the automatic colours.
void hkpShapeDisplayViewer::entityAddedCallback(hkpEntity* entity) {
    const char noStreams = (m_inStream == nullptr && m_outStream == nullptr) ? 1 : 0;
    // Set by getUserShape where the display shape is overridden with none; then reused for whether the display
    // geometries own their hkGeometry.
    uint8_t flag;
    const hkpShape* userShape = getUserShape(noStreams, &flag, entity);
    if (flag) {
        return;
    }
    const int32_t worldIndex = findWorld(entity->m_world);
    if (worldIndex < 0) {
        return;
    }
    hkpShapeDisplayViewer::WorldToEntityData* data = m_worldEntities.m_data[worldIndex];
    const hkpShape* shape = userShape;
    if (!shape) {
        shape = entity->m_collidable.m_shape;
        if (!shape) {
            return;
        }
    }
    bool isCached = false;
    if (m_enableDisplayCaching.m_bool) {
        const uint32_t cached = m_cachedShapes.getWithDefault(shape, 0);
        isCached = cached != 0;
        if (!cached) {
            m_cachedShapes.insert(shape, 1);
        }
    }
    hkUlong geomId = 0;
    if (m_enableInstancing.m_bool) {
        geomId = m_instancedShapeToGeomID.getWithDefault(shape, 0);
    }
    hkInplaceArray<hkDisplayGeometry*, 8> displayGeometries;
    flag = 1;
    if (!geomId && !isCached && m_enableDisplayCreation.m_bool && m_autoGeometryCreation.m_bool) {
        hkGeometry* noDeleteGeometry = static_cast<hkGeometry*>(
            entity->getProperty(HK_PROPERTY_OVERRIDE_DEBUG_DISPLAY_GEOMETRY_NO_DELETE).getPtr()
        );
        hkGeometry* overrideGeometry = static_cast<hkGeometry*>(
            entity->getProperty(HK_PROPERTY_OVERRIDE_DEBUG_DISPLAY_GEOMETRY).getPtr()
        );
        if (!noDeleteGeometry && !overrideGeometry) {
            alignas(16) hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment env;
            hkpShapeDisplayBuilder builder(env);
            builder.buildDisplayGeometries(shape, displayGeometries);
            for (int32_t i = displayGeometries.m_size - 1; i >= 0; --i) {
                hkDisplayGeometry* geometry = displayGeometries.m_data[i];
                if (geometry->m_type == HK_DISPLAY_CONVEX && !geometry->m_geometry) {
                    {
                        char buf[512];
                        hkErrStream errStream(buf, 512);
                        errStream << "Unable to build display geometry from hkpShape geometry data";
                        hkError::s_instance->message(
                            hkError::MESSAGE_REPORT,
                            -1,
                            buf,
                            ".\\VisualDebugger\\Viewer\\Collide\\hkpShapeDisplayViewer.cpp",
                            316
                        );
                    }
                    displayGeometries.removeAt(i);
                }
            }
            // A display shape the entity was given for this shape is consumed here.
            hkReferencedObject* displayShape = static_cast<hkReferencedObject*>(
                entity->getProperty(HK_PROPERTY_DISPLAY_SHAPE).getPtr()
            );
            if (displayShape && displayShape == userShape) {
                displayShape->removeReference();
                entity->removeProperty(HK_PROPERTY_DISPLAY_SHAPE);
            }
        }
        else {
            flag = noDeleteGeometry == nullptr;
            if (overrideGeometry) {
                entity->removeProperty(HK_PROPERTY_OVERRIDE_DEBUG_DISPLAY_GEOMETRY);
            }
            hkDisplayConvex* convex = new hkDisplayConvex(noDeleteGeometry ? noDeleteGeometry : overrideGeometry);
            convex->m_transform.setIdentity();
            displayGeometries.pushBackUnchecked(convex);
        }
    }
    const hkUlong id = hkpShapeDisplayViewer::getId(entity);
    data->entitiesCreated.pushBack(id);
    // The SDK's shape id hint is the shape's address (hkUlong).
    const hkUlong shapeId = reinterpret_cast<hkUlong>(shape);
    if (geomId) {
        const uint32_t usageCount = m_instancedShapeToUsageCount.getWithDefault(shape, 1) + 1;
        m_instancedShapeToUsageCount.insert(shape, usageCount);
        m_displayHandler->addGeometryInstance(geomId, entity->m_motion.m_motionState.m_transform, id, m_tag, shapeId);
    }
    else if (displayGeometries.m_size > 0 || isCached || !m_autoGeometryCreation.m_bool) {
        m_displayHandler
            ->addGeometry(displayGeometries, entity->m_motion.m_motionState.m_transform, id, m_tag, shapeId);
        if (m_enableInstancing.m_bool) {
            m_instancedShapeToGeomID.insert(shape, id);
            m_instancedShapeToUsageCount.insert(shape, 1);
        }
        // The colour is the low 32 bits of the property's data.
        uint32_t color = static_cast<uint32_t>(entity->getProperty(HK_PROPERTY_DEBUG_DISPLAY_COLOR).m_data);
        if (m_enableAutoColor.m_bool && (m_enableAutoColorOverride.m_bool || !color)) {
            color = (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) ? m_fixedObjectColor
                                                                                   : m_movableObjectColor;
        }
        if (color) {
            m_displayHandler->setGeometryColor(static_cast<int32_t>(color), id, m_tag);
        }
    }
    // Geometries given by a no-delete property are not the display geometries' to free.
    for (hkDisplayGeometry* geometry : displayGeometries) {
        if (!flag) {
            geometry->m_geometry = nullptr;
        }
        if (geometry) {
            delete geometry;
        }
    }
}

// Removes the entity's display object, drops its shape's instance count, and forgets its id.
void hkpShapeDisplayViewer::entityRemovedCallback(hkpEntity* entity) {
    // Whether the viewer has no streams; getUserShape tests only the byte.
    const char noStreams = (m_inStream == nullptr && m_outStream == nullptr) ? 1 : 0;
    // The flag getUserShape writes; nothing here reads it.
    uint8_t shapeFlag;
    const hkpShape* shape = getUserShape(noStreams, &shapeFlag, entity);
    const int32_t worldIndex = findWorld(entity->m_world);
    if (worldIndex < 0) {
        return;
    }
    hkpShapeDisplayViewer::WorldToEntityData* data = m_worldEntities.m_data[worldIndex];
    if (!shape) {
        shape = entity->m_collidable.m_shape;
    }
    const hkUlong id = hkpShapeDisplayViewer::getId(entity);
    // The SDK's shape id hint is the shape's address (hkUlong).
    m_displayHandler->removeGeometry(id, m_tag, reinterpret_cast<hkUlong>(shape));
    if (m_enableInstancing.m_bool && shape) {
        const uint32_t usageCount = m_instancedShapeToUsageCount.getWithDefault(shape, 0) - 1;
        if (usageCount == 0) {
            m_instancedShapeToGeomID.remove(shape);
            m_instancedShapeToUsageCount.remove(shape);
        }
        else {
            m_instancedShapeToUsageCount.insert(shape, usageCount);
        }
    }
    int32_t index = data->entitiesCreated.indexOf(id);
    if (index >= 0) {
        data->entitiesCreated.removeAt(index);
    }
}

// Keeps the display transforms up to date after each step, unless that is switched off.
void hkpShapeDisplayViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpShapeDisplayViewer", this);
    if (m_enableShapeTransformUpdate.m_bool) {
        synchronizeTransforms(m_displayHandler, world);
    }
    HK_TIMER_END();
}

void hkpShapeDisplayViewer::inactiveEntityMovedCallback(hkpEntity* entity) {
    const hkUlong id = hkpShapeDisplayViewer::getId(entity);
    m_displayHandler->updateGeometry(entity->m_motion.m_motionState.m_transform, id, m_tag);
}

// Stores the flag's byte as it comes, in the byte the setters of m_enableInstancing, m_enableDisplayCaching,
// m_autoGeometryCreation and m_enableAutoColor sit next to.
void hkpShapeDisplayViewer::setDisplayBodyCreateEnabled(bool on) {
    m_enableDisplayCreation.m_bool = on;
}

void hkpShapeDisplayViewer::setInstancingEnabled(bool on) {
    m_enableInstancing.m_bool = on;
}

void hkpShapeDisplayViewer::setAutoColorMode(bool on) {
    m_enableAutoColor.m_bool = on;
}

void hkpShapeDisplayViewer::setDisplayBodyCachingEnabled(bool on) {
    m_enableDisplayCaching.m_bool = on;
}

void hkpShapeDisplayViewer::clearCache() {
    m_cachedShapes.m_map.clear();
}

void hkpShapeDisplayViewer::setAutoGeometryCreation(bool on) {
    m_autoGeometryCreation.m_bool = on;
}

// Takes the full object as this: init calls it per world. The entityAddedCallback(entity) calls go through the
// hkpEntityListener subobject.
void hkpShapeDisplayViewer::addWorld(hkpWorld* world) {
    hkpEntityListener* entityListener;
    hkpWorldPostSimulationListener* postSimListener;
    if (this) {
        entityListener = static_cast<hkpEntityListener*>(this);
    }
    else {
        entityListener = nullptr;
    }
    world->addEntityListener(entityListener);
    if (this) {
        postSimListener = static_cast<hkpWorldPostSimulationListener*>(this);
    }
    else {
        postSimListener = nullptr;
    }
    world->addWorldPostSimulationListener(postSimListener);
    WorldToEntityData* wed = new WorldToEntityData;
    wed->world = world;
    m_worldEntities.pushBack(wed);
    for (int32_t i = 0; i < world->m_activeSimulationIslands.m_size; ++i) {
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>& entities = world->m_activeSimulationIslands.m_data[i]
                                                                                ->m_entities;
        for (int32_t j = 0; j < entities.m_size; ++j) {
            static_cast<hkpEntityListener*>(this)->entityAddedCallback(entities.m_data[j]);
        }
    }
    for (int32_t i = 0; i < world->m_inactiveSimulationIslands.m_size; ++i) {
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>&
            entities = world->m_inactiveSimulationIslands.m_data[i]->m_entities;
        for (int32_t j = 0; j < entities.m_size; ++j) {
            static_cast<hkpEntityListener*>(this)->entityAddedCallback(entities.m_data[j]);
        }
    }
    hkpSimulationIsland* fixedIsland = world->getFixedIsland();
    if (fixedIsland) {
        for (int32_t k = 0; k < fixedIsland->m_entities.m_size; ++k) {
            static_cast<hkpEntityListener*>(this)->entityAddedCallback(fixedIsland->m_entities.m_data[k]);
        }
    }
}

int32_t hkpShapeDisplayViewer::findWorld(hkpWorld* world) {
    int32_t m_size = m_worldEntities.m_size;
    int32_t result = 0;
    if (m_size <= 0) {
        return -1;
    }
    for (hkpShapeDisplayViewer::WorldToEntityData** i =
             static_cast<hkArrayBase<hkpShapeDisplayViewer::WorldToEntityData*>&>(m_worldEntities).begin();
         (*i)->world != world;
         ++i) {
        if (++result >= m_size) {
            return -1;
        }
    }
    return result;
}

void hkpShapeDisplayViewer::removeWorld(int32_t i) {
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

// Removes the display objects made for the world's entities, each with the shape its collidable has now.
void hkpShapeDisplayViewer::removeAllGeometries(int32_t worldIndex) {
    hkpShapeDisplayViewer::WorldToEntityData* data = m_worldEntities.m_data[worldIndex];
    for (hkUlong id : data->entitiesCreated) {
        // The id is the collidable's address: the shape is read through it, also for an id a debug display id property
        // gave.
        const hkpLinkedCollidable* collidable = reinterpret_cast<const hkpLinkedCollidable*>(id);
        m_displayHandler->removeGeometry(id, m_tag, reinterpret_cast<hkUlong>(collidable->m_shape));
    }
    data->entitiesCreated.setSize(0);
}

hkProcess* hkpShapeDisplayViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpShapeDisplayViewer* pViewer = new hkpShapeDisplayViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpShapeDisplayViewer::registerViewer() {
    m_tag = hkProcessFactory::s_instance->registerProcess("Shapes", &hkpShapeDisplayViewer::create);
}

// Returns the entity's debug display id property where it has one, else its collidable's address.
hkUlong hkpShapeDisplayViewer::getId(hkpEntity* entity) {
    if (entity->hasProperty(HK_PROPERTY_DEBUG_DISPLAY_ID)) {
        return reinterpret_cast<hkUlong>(entity->getProperty(HK_PROPERTY_DEBUG_DISPLAY_ID).getPtr());
    }
    return reinterpret_cast<hkUlong>(&entity->m_collidable);
}
