#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpToiCountViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <cmath>
#include <xmmintrin.h>

hkpToiCountViewer::hkpToiCountViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpContactListener(), hkpWorldPostSimulationListener(), hkpEntityListener(),
    m_toiCounts() {}

// Removes the viewer from the context's worlds; the counts map goes with its member.
hkpToiCountViewer::~hkpToiCountViewer() {
    for (int32_t i = 0; m_context && i < m_context->m_worlds.m_size; ++i) {
        hkpToiCountViewer::worldRemovedCallback(m_context->m_worlds.m_data[i]);
    }
}

// Stops counting the bodies of the world (collected first, then unhooked and dropped from the map) and stops
// listening to the world.
void hkpToiCountViewer::worldRemovedCallback(hkpWorld* w) {
    hkArray<hkpEntity*> entitiesToRemove;
    for (int32_t it = m_toiCounts.getIterator(); m_toiCounts.isValid(it); it = m_toiCounts.getNext(it)) {
        hkpEntity* entity = m_toiCounts.getKey(it);
        if (entity->m_world == w) {
            // EXE-INLINED 0x6299C0
            hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
            if (entitiesToRemove.m_size ==
                (entitiesToRemove.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(heap, &entitiesToRemove, sizeof(hkpEntity*));
            }
            entitiesToRemove.m_data[entitiesToRemove.m_size] = entity;
            ++entitiesToRemove.m_size;
        }
    }
    for (int32_t i = 0; i < entitiesToRemove.m_size; ++i) {
        entitiesToRemove.m_data[i]->removeEntityListener(
            this == nullptr ? nullptr : static_cast<hkpEntityListener*>(this)
        );
        m_toiCounts.remove(static_cast<hkpRigidBody*>(entitiesToRemove.m_data[i]));
    }
    w->removeContactListener(this == nullptr ? nullptr : static_cast<hkpContactListener*>(this));
    w->removeWorldPostSimulationListener(
        this == nullptr ? nullptr : static_cast<hkpWorldPostSimulationListener*>(this)
    );
}

void hkpToiCountViewer::worldAddedCallback(hkpWorld* w) {
    hkpWorldPostSimulationListener* v4;
    hkpToiCountViewer* v3 = this;
    if (this == nullptr) {
        v4 = nullptr;
    }
    else {
        v4 = static_cast<hkpWorldPostSimulationListener*>(this);
    }
    w->addWorldPostSimulationListener(v4);
    if (v3) {
        w->addContactListener(this);
    }
    else {
        w->addContactListener(nullptr);
    }
}

int32_t hkpToiCountViewer::getProcessTag() {
    return s_tag;
}

void hkpToiCountViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    for (int32_t i = 0; m_context; ++i) {
        if (i >= m_context->m_worlds.m_size) {
            break;
        }
        worldAddedCallback(m_context->m_worlds.m_data[i]);
        m_context = this->m_context;
    }
}

// Counts the time-of-impact events of both bodies: the low 24 bits of a body's value are its count, the high byte
// its events since displayCountForBody last showed them; each saturates. A body seen for the first time is added
// with one of each and the viewer listens for its removal.
void hkpToiCountViewer::contactPointCallback(const hkpContactPointEvent& event) {
    if (event.m_type <= hkpContactPointEvent::TYPE_TOI) {
        for (int32_t i = 0; i < 2; ++i) {
            hkpRigidBody* body = event.m_bodies[i];
            int32_t it = m_toiCounts.findKey(body);
            if (m_toiCounts.isValid(it)) {
                uintptr_t& value = m_toiCounts.m_map.m_elem[it].val;
                uint32_t numRecent = (value >> 24) + 1;
                if (numRecent >= 0xFF) {
                    numRecent = 0xFF;
                }
                uint32_t numToi = (value & 0xFFFFFF) + 1;
                if (numToi >= 0xFFFFFF) {
                    numToi = 0xFFFFFF;
                }
                value = numRecent << 24 | numToi;
            }
            else {
                m_toiCounts.insert(body, 0x1000001);
                body->addEntityListener(this == nullptr ? nullptr : static_cast<hkpEntityListener*>(this));
            }
        }
    }
}

void hkpToiCountViewer::postSimulationCallback(hkpWorld* world) {
    hkpWorldPostSimulationListener* this_1 = static_cast<hkpWorldPostSimulationListener*>(this);
    hkpToiCountViewer::DisplayPosition DisplayPositionFromGravityVector =
        hkpToiCountViewer::getDisplayPositionFromGravityVector(world->m_gravity);
    int32_t worlda_1 = 0;
    for (int32_t worlda = 0; worlda_1 < world->m_activeSimulationIslands.m_size; worlda = worlda_1) {
        hkpSimulationIsland** m_data = world->m_activeSimulationIslands.begin();
        hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &m_data[worlda_1]->m_entities;
        int32_t m_size = 0;
        if (m_data[worlda_1]->m_entities.m_size > 0) {
            hkpToiCountViewer* v8 = this;
            do {
                v8->displayCountForBody(
                    static_cast<hkpRigidBody*>(p_m_entities->m_data[m_size++]), DisplayPositionFromGravityVector
                );
            }
            while (m_size < p_m_entities->m_size);
            this_1 = static_cast<hkpWorldPostSimulationListener*>(this);
            worlda_1 = worlda;
        }
        ++worlda_1;
    }
    hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities_1 = &world->getFixedIsland()->m_entities;
    int32_t m_size_1 = 0;
    if (world->getFixedIsland()->m_entities.m_size > 0) {
        do {
            this->displayCountForBody(
                static_cast<hkpRigidBody*>(p_m_entities_1->m_data[m_size_1++]), DisplayPositionFromGravityVector
            );
        }
        while (m_size_1 < p_m_entities_1->m_size);
    }
}

// The map's keys are the rigid bodies the viewer counts; the entity is one of them.
void hkpToiCountViewer::entityRemovedCallback(hkpEntity* entity) {
    entity->removeEntityListener(this != nullptr ? static_cast<hkpEntityListener*>(this) : nullptr);
    m_toiCounts.remove(static_cast<hkpRigidBody*>(entity));
}

// The map's keys are the rigid bodies the viewer counts; the entity is one of them.
void hkpToiCountViewer::entityDeletedCallback(hkpEntity* entity) {
    entity->removeEntityListener(this != nullptr ? static_cast<hkpEntityListener*>(this) : nullptr);
    m_toiCounts.remove(static_cast<hkpRigidBody*>(entity));
}

void hkpToiCountViewer::reset() {
    int32_t m_hashMod = m_toiCounts.m_map.m_hashMod;
    hkPointerMap<hkpRigidBody*, uint32_t, hkContainerHeapAllocator>* p_m_toiCounts = &m_toiCounts;
    int32_t v4 = 0;
    if (m_hashMod >= 0) {
        hkUlongMapBase::Pair* m_elem = p_m_toiCounts->m_map.m_elem;
        do {
            if (m_elem->key != -1) {
                break;
            }
            ++v4;
            ++m_elem;
        }
        while (v4 <= m_hashMod);
    }
    if (v4 <= m_toiCounts.m_map.m_hashMod) {
        int32_t v7;
        do {
            p_m_toiCounts->getKey(v4)->removeEntityListener(this);
            int32_t v6 = p_m_toiCounts->m_map.m_hashMod;
            v7 = v4 + 1;
            if (v4 + 1 <= v6) {
                hkUlongMapBase::Pair* v8 = &p_m_toiCounts->m_map.m_elem[v7];
                do {
                    if (v8->key != -1) {
                        break;
                    }
                    ++v7;
                    ++v8;
                }
                while (v7 <= v6);
            }
            v4 = v7;
        }
        while (v7 <= m_toiCounts.m_map.m_hashMod);
    }
    p_m_toiCounts->m_map.clear();
}

// Shows the body's count, with its events since the last display in brackets when there were any, and clears
// those.
void hkpToiCountViewer::displayCountForBody(hkpRigidBody* body, hkpToiCountViewer::DisplayPosition displayPosition) {
    int32_t it = m_toiCounts.findKey(body);
    if (m_toiCounts.isValid(it)) {
        uint32_t value = m_toiCounts.m_map.m_elem[it].val;
        uint32_t numRecent = value >> 24;
        hkStringBuf str;
        uint32_t numToi = value & 0xFFFFFF;
        if (numRecent) {
            str.printf("%d(%d)", numToi, numRecent);
        }
        else {
            str.printf("%d", numToi);
        }
        hkpToiCountViewer::displayTextAtBody(
            m_displayHandler, getProcessTag(), body, displayPosition, str.m_string.begin(),
            static_cast<int32_t>(hkColor::MAROON), 0.1f, static_cast<hkBool>(true)
        );
        m_toiCounts.m_map.m_elem[it].val = numToi;
    }
}

hkProcess* hkpToiCountViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpToiCountViewer* v2 = static_cast<hkpToiCountViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpToiCountViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpToiCountViewer);
    hkpToiCountViewer* v3 = ::new (v2) hkpToiCountViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }

    return nullptr;
}

void hkpToiCountViewer::registerViewer() {
    s_tag = hkProcessFactory::s_instance->registerProcess("TOI counter", &hkpToiCountViewer::create);
}

// Draws text at the side of the body's bounding box that pos names, extraDistance beyond it, and optionally a line
// from that side to the body's centre of mass. The display id is the body's collidable address as an integer, as the
// SDK casts it.
void hkpToiCountViewer::displayTextAtBody(
    hkDebugDisplayHandler* handler,
    int32_t tag,
    hkpRigidBody* body,
    hkpToiCountViewer::DisplayPosition pos,
    const char* text,
    int32_t color,
    float extraDistance,
    hkBool drawLine
) {
    hkAabb aabb;
    body->m_collidable.m_shape->getAabbImpl(body->m_motion.m_motionState.m_transform, 0.0f, aabb);
    __m128 center = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0), _mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad)
    );
    // The axis pos % 3, positive for the first three positions and negative for the others.
    float sign = 1.0 - (static_cast<double>(pos / 3) + static_cast<double>(pos / 3));
    __m128 halfExtents = _mm_sub_ps(aabb.m_max.m_quad, center);
    __m128 dir = hkVector4Zero.m_quad;
    dir[pos % 3] = sign;
    __m128 extent = _mm_mul_ps(halfExtents, dir);
    __m128 sumXY = _mm_add_ss(_mm_shuffle_ps(extent, extent, 0x55), extent);
    __m128 sum = _mm_add_ss(_mm_shuffle_ps(extent, extent, 0xAA), sumXY);
    float distance = std::fabs(_mm_cvtss_f32(sum));
    hkVector4 surfacePoint;
    surfacePoint.m_quad =
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(distance), _mm_set_ss(distance), 0), dir), center);
    hkVector4 textPos;
    textPos.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(extraDistance), _mm_set_ss(extraDistance), 0), dir), surfacePoint.m_quad
    );
    // The display id is an int: the low 32 bits of the collidable's address.
    const int32_t id = static_cast<int32_t>(reinterpret_cast<hkUlong>(&body->m_collidable));
    handler->display3dText(text, textPos, color, id, tag);
    if (drawLine.m_bool) {
        handler->displayLine(
            surfacePoint, body->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1, color, id, tag
        );
    }
}

hkpToiCountViewer::DisplayPosition hkpToiCountViewer::getDisplayPositionFromGravityVector(const hkVector4& gravity) {
    const __m128* a1 = &gravity.m_quad;
    int32_t v2;
    __m128 v1 = _mm_and_ps(_mm_mul_ps(*a1, *a1), hkVector4_setAbs4_absMask);
    if (!(v1[1] > static_cast<double>(v1[0]))) {
        if (!(v1[2] > static_cast<double>(v1[0]))) {
            v2 = 0;
        }
        else {
            v2 = 2;
        }
    }
    else {
        if (!(v1[2] > static_cast<double>(v1[1]))) {
            v2 = 1;
        }
        else {
            v2 = 2;
        }
    }
    if (!((*a1)[v2] > 0.0f)) {
        return static_cast<hkpToiCountViewer::DisplayPosition>(v2);
    }

    return static_cast<hkpToiCountViewer::DisplayPosition>(v2 + 3);
}
