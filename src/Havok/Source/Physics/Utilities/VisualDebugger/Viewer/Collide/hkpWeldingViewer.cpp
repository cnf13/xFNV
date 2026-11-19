#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpWeldingViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpMeshWeldingUtility.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <xmmintrin.h>

// Adds the viewer to the context's worlds as a post-simulation listener.
hkpWeldingViewer::hkpWeldingViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpWorldPostSimulationListener() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->addWorldPostSimulationListener(this);
        }
    }
}

// Removes the viewer from the context's worlds.
hkpWeldingViewer::~hkpWeldingViewer() {
    if (m_context) {
        for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
            m_context->m_worlds.m_data[i]->removeWorldPostSimulationListener(this);
        }
    }
}

// Draws each mopp body's triangles in red, and for each welded edge its welding direction, the normal and the centre
// from a point 0.48 along the edge, with the arc from the direction to the normal. The timer's name is copied from
// hkpActiveContactPointViewer. normalize3 uses the constants 0.0, 3.0 and 0.5. The welding type (0 or 4) indexes the
// table entry's floats, so the clockwise type reads m_cosAccept1 and m_sinAccept1. The negated normal of a clockwise
// triangle is kept for its later edges, as the game does.
void hkpWeldingViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpActiveContactPointViewer::postSimulationCallback", nullptr);
    world->lock();
    hkpPhysicsSystem* system = world->getWorldAsOneSystem();
    for (int32_t i = 0; i < system->m_rigidBodies.m_size; i++) {
        hkpRigidBody* body = system->m_rigidBodies.m_data[i];
        const hkpShape* shape = body->m_collidable.m_shape;
        // The id is an int display id: the low 32 bits of the collidable's address.
        const int32_t id = static_cast<int32_t>(reinterpret_cast<hkUlong>(&body->m_collidable));
        if (shape->m_type.m_storage != HK_SHAPE_MOPP) {
            continue;
        }
        // EXE-INLINED none: hkpMoppBvTreeShape::getShapeCollection
        const hkpShapeCollection* collection =
            static_cast<const hkpShapeCollection*>(static_cast<const hkpMoppBvTreeShape*>(shape)->m_child.m_childShape);
        alignas(16) char shapeBuffer[512];
        for (uint32_t key = collection->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = collection->getNextKey(key)) {
            const hkpShape* child = collection->getChildShape(key, shapeBuffer);
            if (child->m_type.m_storage != HK_SHAPE_TRIANGLE) {
                continue;
            }
            const hkpTriangleShape* triangle = static_cast<const hkpTriangleShape*>(child);
            const uint8_t weldingType = triangle->m_weldingType.m_storage;
            if (weldingType == hkpWeldingUtility::WELDING_TYPE_TWO_SIDED ||
                weldingType == hkpWeldingUtility::WELDING_TYPE_NONE) {
                continue;
            }

            const hkTransform& transform = body->m_motion.m_motionState.m_transform;
            const uint16_t weldingInfo = triangle->m_weldingInfo;
            hkVector4 vertices[3];
            for (int32_t k = 0; k < 3; k++) {
                vertices[k].setTransformedPos(transform, triangle->getVertex(k));
            }
            m_displayHandler->displayLine(vertices[0], vertices[1], static_cast<int32_t>(hkColor::RED), id, m_tag);
            m_displayHandler->displayLine(vertices[1], vertices[2], static_cast<int32_t>(hkColor::RED), id, m_tag);
            m_displayHandler->displayLine(vertices[2], vertices[0], static_cast<int32_t>(hkColor::RED), id, m_tag);

            hkVector4 centre;
            centre.m_quad = _mm_mul_ps(
                _mm_add_ps(_mm_add_ps(vertices[1].m_quad, vertices[0].m_quad), vertices[2].m_quad),
                _mm_set1_ps(0.33333334f)
            );
            hkVector4 normal;
            hkpMeshWeldingUtility::calcAntiClockwiseTriangleNormal(vertices[0], vertices[1], vertices[2], normal);

            for (int32_t e = 0; e < 3; e++) {
                const hkVector4& v0 = vertices[e];
                const hkVector4& v1 = vertices[(e + 1) % 3];

                hkVector4 edge;
                edge.m_quad = _mm_sub_ps(v1.m_quad, v0.m_quad);
                edge.normalize3();

                const int32_t sector = getSectorFromWeldingInfo(weldingInfo, static_cast<char>(e));

                hkVector4 start;
                start.setInterpolate4(v0, v1, 0.48f);

                const hkpWeldingUtility::SinCosTableEntry& entry = hkpWeldingUtility::m_sinCosTable[sector];
                const uint8_t type = triangle->m_weldingType.m_storage;
                const float cosAccept = (&entry.m_cosAccept0)[type];
                const float sinAccept = (&entry.m_sinAccept0)[type];
                hkVector4 perp;
                perp.setCross(edge, normal);
                hkVector4 dir;
                dir.m_quad = _mm_add_ps(
                    _mm_mul_ps(perp.m_quad, _mm_set1_ps(sinAccept)), _mm_mul_ps(_mm_set1_ps(cosAccept), normal.m_quad)
                );
                dir.normalize3();
                if (type == hkpWeldingUtility::WELDING_TYPE_CLOCKWISE) {
                    normal.setNeg3(normal);
                }

                const hkQuadReal dn = _mm_mul_ps(dir.m_quad, normal.m_quad);
                const float fDot = _mm_cvtss_f32(
                    _mm_add_ss(_mm_shuffle_ps(dn, dn, 0xAA), _mm_add_ss(_mm_shuffle_ps(dn, dn, 0x55), dn))
                );
                // Compares against -0.98f widened to double; a NaN draws.
                if (fDot < -0.9800000190734863) {
                    continue;
                }

                hkVector4 dirEnd;
                dirEnd.setAdd4(dir, start);
                hkVector4 normalEnd;
                normalEnd.setAdd4(normal, start);
                m_displayHandler->displayLine(start, dirEnd, static_cast<int32_t>(hkColor::ORANGE), id, m_tag);
                m_displayHandler->displayLine(start, normalEnd, static_cast<int32_t>(hkColor::ORANGE), id, m_tag);
                m_displayHandler->displayLine(start, centre, static_cast<int32_t>(hkColor::GREEN), id, m_tag);

                hkVector4 prev = dir;
                for (int32_t j = 0; j < 10; j++) {
                    // Divides by the double 10.0 and stores the step as a float.
                    const float fT = static_cast<double>(j + 1) / 10.0;
                    hkVector4 cur;
                    cur.setInterpolate4(dir, normal, fT);
                    cur.normalize3();
                    hkVector4 lineStart;
                    lineStart.setAdd4(start, prev);
                    hkVector4 lineEnd;
                    lineEnd.setAdd4(cur, start);
                    m_displayHandler->displayLine(lineStart, lineEnd, static_cast<int32_t>(hkColor::ORANGE), id, m_tag);
                    prev = cur;
                }
            }
        }
    }
    system->removeReference();
    world->unlock();
    HK_TIMER_END();
}

void hkpWeldingViewer::worldAddedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->addWorldPostSimulationListener(nullptr);
    }
    else {
        world->addWorldPostSimulationListener(this);
    }
}

void hkpWeldingViewer::worldRemovedCallback(hkpWorld* world) {
    if (this == nullptr) {
        world->removeWorldPostSimulationListener(nullptr);
    }
    else {
        world->removeWorldPostSimulationListener(this);
    }
}

hkProcess* hkpWeldingViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpWeldingViewer* v2 = static_cast<hkpWeldingViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpWeldingViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpWeldingViewer);
    hkpWeldingViewer* v3 = ::new (v2) hkpWeldingViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }

    return nullptr;
}

// Registers create under getName() and stores the tag in m_tag.
void hkpWeldingViewer::registerViewer() {
    m_tag = hkProcessFactory::s_instance->registerProcess(getName(), &hkpWeldingViewer::create);
}
