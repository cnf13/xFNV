#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpContactPointViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

static void displayArrow(
    hkDebugDisplayHandler* handler,
    const hkVector4& from,
    int32_t color,
    int32_t id,
    int32_t tag,
    const hkVector4& dir
);

// The arrow as hkDebugDisplay::displayArrow draws it but with one head plane: the line from from to from + dir, then
// two head lines from that tip to the point 0.85 of the way along, offset by +-0.15 of dir's length along a unit
// perpendicular. Both paths end the timer.
static void displayArrow(
    hkDebugDisplayHandler* handler,
    const hkVector4& from,
    int32_t color,
    int32_t id,
    int32_t tag,
    const hkVector4& dir
) {
    HK_TIMER_BEGIN("hkContactPointViewer_displayArrow", nullptr);
    const __m128 kDir = dir.m_quad;
    const __m128 kDirSquares = _mm_mul_ps(kDir, kDir);
    const __m128 kDirLengthSquared = _mm_add_ss(
        _mm_shuffle_ps(kDirSquares, kDirSquares, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(kDirSquares, kDirSquares, 0x55), kDirSquares)
    );
    if (_mm_cvtss_f32(kDirLengthSquared) < 1.1920929e-07f) {
        HK_TIMER_END();
        return;
    }

    hkVector4 to;
    to.m_quad = _mm_add_ps(from.m_quad, kDir);

    // EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
    float fMin = __builtin_fabsf(dir(0));
    const float fAbsY = __builtin_fabsf(dir(1));
    const float fAbsZ = __builtin_fabsf(dir(2));
    int32_t iMin = 0;
    int32_t iFirst = 1;
    int32_t iSecond = 2;
    if (fMin > fAbsY) {
        fMin = fAbsY;
        iFirst = 0;
        iMin = 1;
    }
    if (fMin > fAbsZ) {
        iSecond = iMin;
    }
    hkVector4 ort;
    ort.m_quad = _mm_setzero_ps();
    ort(iFirst) = dir(iSecond);
    ort(iSecond) = -dir(iFirst);

    // EXE-INLINED 0x62AEA0
    const __m128 kOrt = ort.m_quad;
    const __m128 kOrtSquares = _mm_mul_ps(kOrt, kOrt);
    const __m128 kOrtLengthSquared = _mm_add_ss(
        _mm_shuffle_ps(kOrtSquares, kOrtSquares, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(kOrtSquares, kOrtSquares, 0x55), kOrtSquares)
    );
    const __m128 kEstimate = _mm_rsqrt_ss(kOrtLengthSquared);
    const __m128 kZeroMask = _mm_cmpeq_ss(kOrtLengthSquared, _mm_set_ss(0.0f));
    const __m128 kStep = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kOrtLengthSquared, kEstimate), kEstimate));
    const __m128 kScale = _mm_andnot_ps(kZeroMask, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kEstimate), kStep));
    // EXE-INLINED 0x630B40
    const __m128 kDirLength = _mm_sqrt_ss(kDirLengthSquared);
    // EXE-INLINED 0x627920
    const __m128 kOrtScaled =
        _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), kOrt), _mm_shuffle_ps(kDirLength, kDirLength, 0));

    // EXE-INLINED 0x5DBF70
    const __m128 kC = _mm_set1_ps(0.85f);
    const __m128 kP = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, kC), from.m_quad), _mm_mul_ps(kC, to.m_quad));

    // EXE-INLINED none: hkVector4::setAddMul4
    hkVector4 p1;
    p1.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.14999998f), kOrtScaled), kP);
    hkVector4 p2;
    p2.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(-0.14999998f), kOrtScaled), kP);

    handler->displayLine(from, to, color, id, tag);
    handler->displayLine(to, p1, color, id, tag);
    handler->displayLine(to, p2, color, id, tag);
    HK_TIMER_END();
}

hkpContactPointViewer::hkpContactPointViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts,
    int32_t color
)
  : hkpWorldViewerBase(contexts), hkpWorldPostSimulationListener(), m_limit(0.0f), m_color(color) {}

// Removes the viewer from the context's worlds.
hkpContactPointViewer::~hkpContactPointViewer() {
    for (int32_t i = 0; m_context && i < m_context->m_worlds.m_size; ++i) {
        hkpContactPointViewer::worldRemovedCallback(m_context->m_worlds.m_data[i]);
    }
}

void hkpContactPointViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    for (int32_t i = 0; m_context; ++i) {
        if (i >= m_context->m_worlds.m_size) {
            break;
        }
        worldAddedCallback(m_context->m_worlds.m_data[i]);
        m_context = this->m_context;
    }
}

// Draws the contact points of the islands the derived viewer picks. The timer name is deliberately the active viewer's,
// as the game does.
void hkpContactPointViewer::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("hkpActiveContactPointViewer::postSimulationCallback", this);
    const hkArray<hkpSimulationIsland*>& islands = getIslands(world);
    for (int32_t i = 0; i < islands.m_size; ++i) {
        drawAllContactPointsInIsland(islands.m_data[i]);
    }
    HK_TIMER_END();
}

void hkpContactPointViewer::worldAddedCallback(hkpWorld* w) {
    if (this == nullptr) {
        w->addWorldPostSimulationListener(0);
    }
    else {
        w->addWorldPostSimulationListener(this);
    }
}

void hkpContactPointViewer::worldRemovedCallback(hkpWorld* w) {
    if (this == nullptr) {
        w->removeWorldPostSimulationListener(0);
    }
    else {
        w->removeWorldPostSimulationListener(this);
    }
}

void hkpContactPointViewer::drawAllContactPointsInIsland(const hkpSimulationIsland* island) {
    HK_TIMER_BEGIN("hkpContactPointViewer::drawAllContactPointsInIsland", this);

    // HK_FOR_ALL_AGENT_ENTRIES_BEGIN/END, written out: every entry of each sector, the last sector ending at the bytes
    // it uses. A sector is an untyped byte buffer the entries are packed into, each entry's size stepping to the next,
    // so its bytes are cast to the entry. The entry's address is compared with the end unsigned, wrap included, also
    // before the first entry.
    const hkpAgentNnTrack& track = island->m_agentTrack;
    for (int32_t sectorIndex = 0; sectorIndex < track.m_sectors.m_size;) {
        hkpAgentNnSector* sector = track.m_sectors.m_data[sectorIndex++];
        hkpAgentNnEntry* entry = sector->getBegin();
        const uintptr_t entryEnd = (sectorIndex == track.m_sectors.m_size)
            ? reinterpret_cast<uintptr_t>(sector->m_data) + track.m_bytesUsedInLastSector
            : reinterpret_cast<uintptr_t>(sector->m_data) + sizeof(hkpAgentNnSector);
        for (; reinterpret_cast<uintptr_t>(entry) < entryEnd;
             entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size)) {
            hkpDynamicsContactMgr* manager = static_cast<hkpDynamicsContactMgr*>(entry->m_contactMgr);
            if (manager->getType() != hkpContactMgr::TYPE_SIMPLE_CONSTRAINT_CONTACT_MGR) {
                continue;
            }

            // The SDK's hkLocalArray<hkContactPointId> contactPoints(HK_MAX_CONTACT_POINT): 256 ids on the thread's
            // stack allocator, flagged DONT_DEALLOCATE. The stack allocator hands out raw memory.
            // EXE-INLINED none: hkLocalArray<uint16_t>::hkLocalArray(int)
            FNVD_INLINED_LOCAL(hkLocalArray<uint16_t>, contactPoints, std::nullopt, std::nullopt);
            contactPoints.m_data = nullptr;
            contactPoints.m_size = 0;
            contactPoints.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG);
            // EXE-INLINED none: hkAllocateStack<uint16_t>
            hkMemoryRouter* router = &hkMemoryRouter::getInstance();
            uint16_t* stackMemory = reinterpret_cast<uint16_t*>(router->m_stack.m_next.m_storage);
            router->m_stack.m_next.m_storage += (256 * sizeof(uint16_t) + 127) & ~static_cast<size_t>(127);
            contactPoints.m_data = stackMemory;
            contactPoints.m_localMemory = stackMemory;
            contactPoints.m_capacityAndFlags = static_cast<int32_t>(256 | hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG);
            manager->getAllContactPointIds(contactPoints);

            for (int32_t i = 0; i < contactPoints.m_size; ++i) {
                hkContactPoint* cp = manager->getContactPoint(contactPoints.m_data[i]);
                hkpContactPointProperties* properties = manager->getContactPointProperties(contactPoints.m_data[i]);
                // Drawn unless below the limit: an unordered compare draws too.
                if (!(m_limit > properties->m_impulseApplied)) {
                    const hkpEntity* masterEntity =
                        static_cast<hkpSimpleConstraintContactMgr*>(manager)->m_constraint.getMasterEntity();
                    // The id is an int display id: the low 32 bits of the collidable's address.
                    const int32_t id = static_cast<int32_t>(reinterpret_cast<hkUlong>(masterEntity->getCollidable()));
                    displayArrow(
                        m_displayHandler, cp->m_position, m_color, id, getProcessTag(), cp->m_separatingNormal
                    );
                }
            }

            contactPoints.~hkLocalArray<uint16_t>();
        }
    }

    HK_TIMER_END();
}
