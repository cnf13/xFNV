#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpToiContactPointViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <xmmintrin.h>

// Draws nothing unless dir's squared length passes FLT_EPSILON, else the line from from to from + dir, then two head
// lines from that tip to the point 0.85 of the way along, offset by +-0.15 of dir's length along a unit perpendicular,
// inside the timer. Same arrow as hkContactPointViewer_displayArrow in hkpContactPointViewer.cpp.
void hkToiContactPointViewer_displayArrow(
    hkDebugDisplayHandler* handler,
    const hkVector4& from,
    int32_t color,
    int32_t id,
    int32_t tag,
    const hkVector4& dir
) {
    const __m128 kDirSquares = _mm_mul_ps(dir.m_quad, dir.m_quad);
    const __m128 kDirLengthSquared = _mm_add_ss(
        _mm_shuffle_ps(kDirSquares, kDirSquares, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(kDirSquares, kDirSquares, 0x55), kDirSquares)
    );
    const float fDirLengthSquared = _mm_cvtss_f32(kDirLengthSquared);
    if (!(fDirLengthSquared > 1.1920929e-07f)) {
        return;
    }
    HK_TIMER_BEGIN("ToiDisplayArrow", nullptr);

    const __m128 kDir = dir.m_quad;
    hkVector4 to;
    to.m_quad = _mm_add_ps(from.m_quad, kDir);

    hkVector4 ort;
    hkVector4Util::calculatePerpendicularVector(dir, ort);

    ort.normalize3();
    ort.mul4(dir.length3());

    hkVector4 p;
    p.setInterpolate4(from, to, hkSimdReal(0.85f));

    hkVector4 p1;
    p1.setAddMul4(p, ort, hkSimdReal(0.14999998f));
    hkVector4 p2;
    p2.setAddMul4(p, ort, hkSimdReal(-0.14999998f));

    handler->displayLine(from, to, color, id, tag);
    handler->displayLine(to, p1, color, id, tag);
    handler->displayLine(to, p2, color, id, tag);
    HK_TIMER_END();
}

hkpToiContactPointViewer::hkpToiContactPointViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkpWorldViewerBase(contexts), hkpContactListener() {}

// Removes the viewer from the context's worlds.
hkpToiContactPointViewer::~hkpToiContactPointViewer() {
    for (int32_t i = 0; m_context && i < m_context->m_worlds.m_size; ++i) {
        hkpToiContactPointViewer::worldRemovedCallback(m_context->m_worlds.m_data[i]);
    }
}

int32_t hkpToiContactPointViewer::getProcessTag() {
    return s_tag;
}

void hkpToiContactPointViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    for (int32_t i = 0; m_context; ++i) {
        if (i >= m_context->m_worlds.m_size) {
            break;
        }
        worldAddedCallback(m_context->m_worlds.m_data[i]);
        m_context = this->m_context;
    }
}

// Draws an arrow along each time-of-impact contact's normal at its position. The display id is an int: the low 32 bits
// of the first body's collidable address.
void hkpToiContactPointViewer::contactPointCallback(const hkpContactPointEvent& event) {
    if (event.m_type <= hkpContactPointEvent::TYPE_TOI) {
        hkToiContactPointViewer_displayArrow(
            m_displayHandler, event.m_contactPoint->m_position, static_cast<int32_t>(hkColor::RED),
            static_cast<int32_t>(reinterpret_cast<hkUlong>(&event.m_bodies[0]->m_collidable)), s_tag,
            event.m_contactPoint->m_separatingNormal
        );
    }
}

void hkpToiContactPointViewer::worldAddedCallback(hkpWorld* w) {
    if (this == nullptr) {
        w->addContactListener(nullptr);
    }
    else {
        w->addContactListener(this);
    }
}

void hkpToiContactPointViewer::worldRemovedCallback(hkpWorld* w) {
    if (this == nullptr) {
        w->removeContactListener(nullptr);
    }
    else {
        w->removeContactListener(this);
    }
}

hkProcess* hkpToiContactPointViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpToiContactPointViewer* v2 =
        static_cast<hkpToiContactPointViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpToiContactPointViewer)));
    v2->m_memSizeAndFlags = sizeof(hkpToiContactPointViewer);
    hkpToiContactPointViewer* v3 = ::new (v2) hkpToiContactPointViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
    }

    return nullptr;
}

void hkpToiContactPointViewer::registerViewer() {
    hkpToiContactPointViewer::s_tag =
        hkProcessFactory::s_instance->registerProcess("Contact Points (TOI)", &hkpToiContactPointViewer::create);
}
