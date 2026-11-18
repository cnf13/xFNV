#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/Multithreaded/hkpCharacterProxyJobs.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyListener.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpSurfaceConstraintInfo.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpSurfaceInfo.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <x86intrin.h>
#include <xmmintrin.h>

// When the plane at index r_eax of r_edi faces up by more than 0.01 but by less than maxSlopeCos, appends a copy of
// that constraint whose plane is turned vertical (its part along up removed, normalized) and returns true.
bool addMaxSlopePlane(
    float maxSlopeCos,
    const hkVector4& up,
    int32_t r_eax,
    hkArray<hkpSurfaceConstraintInfo, hkContainerHeapAllocator>& r_edi
) {
    // EXE-INLINED 0x62B0A0
    // hkVector4::dot3: src's builds its result through the out-of-line hkSimdReal::Construct, which the exe's Havok
    // code does not call.
    const hkQuadReal product = _mm_mul_ps(r_edi.m_data[r_eax].m_plane.m_quad, up.m_quad);
    const hkQuadReal xySum = _mm_add_ss(_mm_shuffle_ps(product, product, _MM_SHUFFLE(1, 1, 1, 1)), product);
    const hkQuadReal xyzSum = _mm_add_ss(_mm_shuffle_ps(product, product, _MM_SHUFFLE(2, 2, 2, 2)), xySum);
    float surfaceVerticalComponent;
    _mm_store_ss(&surfaceVerticalComponent, xyzSum);
    if (!(surfaceVerticalComponent > 0.01f && surfaceVerticalComponent < maxSlopeCos)) {
        return false;
    }
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    // EXE-INLINED none: hkArray<hkpSurfaceConstraintInfo, hkContainerHeapAllocator>::expandOne
    if (r_edi.m_size == (r_edi.m_capacityAndFlags & hkArrayBase<hkpSurfaceConstraintInfo>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*pRouter->m_heap, &r_edi, sizeof(hkpSurfaceConstraintInfo));
    }
    hkpSurfaceConstraintInfo& newConstraint = r_edi.m_data[r_edi.m_size++];
    const hkpSurfaceConstraintInfo& original = r_edi.m_data[r_eax];
    // EXE-INLINED 0xC6D670
    // The implicit copy assignment, member by member: the exe leaves the 12 bytes after m_priority unwritten.
    newConstraint.m_plane = original.m_plane;
    newConstraint.m_velocity = original.m_velocity;
    newConstraint.m_staticFriction = original.m_staticFriction;
    newConstraint.m_extraUpStaticFriction = original.m_extraUpStaticFriction;
    newConstraint.m_extraDownStaticFriction = original.m_extraDownStaticFriction;
    newConstraint.m_dynamicFriction = original.m_dynamicFriction;
    newConstraint.m_priority = original.m_priority;
    // newConstraint.m_plane.addMul4(-surfaceVerticalComponent, arUp), with the negation rounded to a float.
    const float negVertical = -surfaceVerticalComponent;
    const hkQuadReal plane = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negVertical), up.m_quad), newConstraint.m_plane.m_quad);
    // EXE-INLINED 0x62AEA0
    // hkVector4::normalize3 (lengthInverse3 and broadcast inlined), written out for the same reason as dot3 above.
    const hkQuadReal square = _mm_mul_ps(plane, plane);
    const hkQuadReal len2 = _mm_add_ss(
        _mm_shuffle_ps(square, square, _MM_SHUFFLE(2, 2, 2, 2)),
        _mm_add_ss(_mm_shuffle_ps(square, square, _MM_SHUFFLE(1, 1, 1, 1)), square)
    );
    const hkQuadReal approx = _mm_rsqrt_ss(len2);
    const hkQuadReal len2EqualsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
    const hkQuadReal refined = _mm_mul_ss(
        _mm_mul_ss(_mm_set_ss(0.5f), approx), _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
    );
    const hkQuadReal lenInv = _mm_andnot_ps(len2EqualsZero, refined);
    newConstraint.m_plane.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, _MM_SHUFFLE(0, 0, 0, 0)), plane);
    return true;
}

// No shape phantom and empty arrays, then the proxy is filled from arInfo.
hkpCharacterProxy::hkpCharacterProxy(const hkpCharacterProxyCinfo& arInfo) : m_shapePhantom(nullptr) {
    updateFromCinfo(arInfo);
}

// The proxy stops listening to the bodies and phantoms it knows, drops its property from the shape phantom and its
// reference to it. The four arrays' storage goes with their members.
hkpCharacterProxy::~hkpCharacterProxy() {
    for (int32_t i = 0; i < m_bodies.m_size; i++) {
        m_bodies.m_data[i]->removeEntityListener(static_cast<hkpEntityListener*>(this));
    }
    m_bodies.m_size = 0;
    for (int32_t i = 0; i < m_phantoms.m_size; i++) {
        m_phantoms.m_data[i]->removePhantomListener(static_cast<hkpPhantomListener*>(this));
    }
    m_phantoms.m_size = 0;
    m_shapePhantom->removeProperty(PROPERTY_KEY_CHARACTER_PROXY);
    m_shapePhantom->removeReference();
}

void hkpCharacterProxy::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags;
    int32_t v5;
    int32_t v7;
    int32_t v9;
    int32_t v11;
    m_capacityAndFlags = (int32_t)m_manifold.m_capacityAndFlags;
    if ((m_capacityAndFlags >= 0) || collector->m_includeUnmanagedArrayMemory) {
        v5 = hkStatisticsCollector::getAllocatedSize(
            m_manifold.m_data, static_cast<int32_t>(sizeof(*m_manifold.m_data)) * (m_capacityAndFlags & 1073741823)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "Manifold", m_manifold.m_data,
            (static_cast<int32_t>(sizeof(*m_manifold.m_data)) * m_manifold.m_size), v5
        );
    }
    int32_t v6 = (int32_t)m_bodies.m_capacityAndFlags;
    if ((v6 >= 0) || collector->m_includeUnmanagedArrayMemory) {
        v7 = hkStatisticsCollector::getAllocatedSize(
            m_bodies.m_data, static_cast<int32_t>(sizeof(*m_bodies.m_data)) * v6
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "OverlapsPntr", m_bodies.m_data,
            (static_cast<int32_t>(sizeof(*m_bodies.m_data)) * m_bodies.m_size), v7
        );
    }
    int32_t v8 = (int32_t)m_phantoms.m_capacityAndFlags;
    if ((v8 >= 0) || collector->m_includeUnmanagedArrayMemory) {
        v9 = hkStatisticsCollector::getAllocatedSize(
            m_phantoms.m_data, static_cast<int32_t>(sizeof(*m_phantoms.m_data)) * v8
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "Manifold", m_phantoms.m_data,
            (static_cast<int32_t>(sizeof(*m_phantoms.m_data)) * m_phantoms.m_size), v9
        );
    }
    int32_t v10 = (int32_t)m_listeners.m_capacityAndFlags;
    if ((v10 >= 0) || collector->m_includeUnmanagedArrayMemory) {
        v11 = hkStatisticsCollector::getAllocatedSize(
            m_listeners.m_data, static_cast<int32_t>(sizeof(*m_listeners.m_data)) * v10
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "ListnrPntr", m_listeners.m_data,
            (static_cast<int32_t>(sizeof(*m_listeners.m_data)) * m_listeners.m_size), v11
        );
    }
}

// Removes this proxy from the entity's listeners, every manifold point whose second root collidable is the entity
// (the last point moved into its slot) and the entity from m_bodies.
void hkpCharacterProxy::entityRemovedCallback(hkpEntity* entity) {
    entity->removeEntityListener(this);
    for (int32_t i = m_manifold.m_size - 1; i >= 0; --i) {
        const hkpCollidable* pRoot = m_manifold.m_data[i].m_rootCollidableB;
        // EXE-INLINED 0x4B59F0
        hkpEntity* pManifoldEntity = pRoot->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
            ? static_cast<hkpEntity*>(static_cast<hkpWorldObject*>(pRoot->getOwner()))
            : nullptr;
        if (pManifoldEntity == entity) {
            // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the points' empty
            // destruct there)
            m_manifold.m_size--;
            if (m_manifold.m_size != i) {
                hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                    m_manifold.m_data + i, m_manifold.m_data + m_manifold.m_size
                );
            }
        }
    }
    // EXE-INLINED 0x629F10
    int32_t index = -1;
    for (int32_t i = 0; i < m_bodies.m_size; ++i) {
        if (m_bodies.m_data[i] == entity) {
            index = i;
            break;
        }
    }
    m_bodies.removeAt(index);
}

// Removes this proxy from the phantom's listeners, every manifold point whose second root collidable is the phantom
// (the last point moved into its slot) and the phantom from m_phantoms.
void hkpCharacterProxy::phantomRemovedCallback(hkpPhantom* phantom) {
    phantom->removePhantomListener(this);
    for (int32_t i = m_manifold.m_size - 1; i >= 0; --i) {
        const hkpCollidable* pRoot = m_manifold.m_data[i].m_rootCollidableB;
        // EXE-INLINED 0x4B5950
        hkpPhantom* pManifoldPhantom = pRoot->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM
            ? static_cast<hkpPhantom*>(static_cast<hkpWorldObject*>(pRoot->getOwner()))
            : nullptr;
        if (pManifoldPhantom == phantom) {
            // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the points' empty
            // destruct there)
            m_manifold.m_size--;
            if (m_manifold.m_size != i) {
                hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                    m_manifold.m_data + i, m_manifold.m_data + m_manifold.m_size
                );
            }
        }
    }
    // EXE-INLINED 0x629F10
    int32_t index = -1;
    for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
        if (m_phantoms.m_data[i] == phantom) {
            index = i;
            break;
        }
    }
    m_phantoms.removeAt(index);
}

// Unless multithreaded, the proxy stops listening to the bodies and phantoms it touched; the start point hits are
// copied into a stack-allocated local array, noting the smallest distance. Each manifold point, from the last down,
// takes the closest of those hits (surfaceDistance under 1.1; the hit then leaves the array) or is dropped. Of the hits
// left, those at the smallest distance and the cast collector's first hit are added where findSurface matches no
// manifold point and replace the matched point otherwise. Near duplicates (surfaceDistance under 0.1) are dropped from
// the end down, and the proxy listens to every body and phantom the manifold touches.
void hkpCharacterProxy::updateManifold(
    const hkpAllCdPointCollector& startPointCollector,
    const hkpAllCdPointCollector& castCollector,
    hkBool isMultithreaded
) {
    if (!isMultithreaded.m_bool) {
        for (int32_t i = 0; i < m_bodies.m_size; ++i) {
            m_bodies.m_data[i]->removeEntityListener(this);
        }
        for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
            m_phantoms.m_data[i]->removePhantomListener(this);
        }
    }
    float minimumPenetration = 3.40282e+38f;
    // EXE-INLINED none: hkArray<hkpRigidBody*>::clear
    m_bodies.m_size = 0;
    // EXE-INLINED none: hkArray<hkpPhantom*>::clear
    m_phantoms.m_size = 0;

    // EXE-INLINED none: hkLocalArray<hkpRootCdPoint>::hkLocalArray(numStartHits) (hkAllocateStack)
    const int32_t numStartHits = startPointCollector.m_hits.m_size;
    XFNV_INLINED_LOCAL(hkLocalArray<hkpRootCdPoint>, startPointHits, std::nullopt, std::nullopt);
    hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
    void* pBlock = stack.m_next.m_storage;
    stack.m_next.m_storage =
        static_cast<char*>(pBlock) + ((numStartHits * static_cast<int32_t>(sizeof(hkpRootCdPoint)) + 127) & ~127);
    hkpRootCdPoint* pLocal = static_cast<hkpRootCdPoint*>(pBlock);
    startPointHits.m_data = pLocal;
    startPointHits.m_localMemory = pLocal;
    startPointHits.m_capacityAndFlags =
        static_cast<int32_t>(static_cast<uint32_t>(numStartHits) | hkArrayBase<hkpRootCdPoint>::DONT_DEALLOCATE_FLAG);
    startPointHits.m_size = startPointCollector.m_hits.m_size;
    for (int32_t i = 0; i < startPointHits.m_size; ++i) {
        startPointHits.m_data[i].Assign(startPointCollector.m_hits.m_data[i]);
        if (minimumPenetration > startPointHits.m_data[i].m_contact.m_separatingNormal.m_quad[3]) {
            minimumPenetration = startPointHits.m_data[i].m_contact.m_separatingNormal.m_quad[3];
        }
    }

    // Each manifold point takes the closest start hit, or is dropped.
    for (int32_t i = m_manifold.m_size - 1; i >= 0; --i) {
        hkpRootCdPoint& current = m_manifold.m_data[i];
        float bestDistance = 1.1f;
        int32_t bestIndex = -1;
        for (int32_t j = 0; j < startPointHits.m_size; ++j) {
            const float distance = surfaceDistance(startPointHits.m_data[j], current);
            if (bestDistance > distance) {
                bestDistance = distance;
                bestIndex = j;
            }
        }
        if (bestIndex >= 0) {
            hkpRootCdPoint& hit = startPointHits.m_data[bestIndex];
            if (hit.m_rootCollidableB != current.m_rootCollidableB) {
                fireContactRemoved(current);
                fireContactAdded(hit);
            }
            current.Assign(hit);
            // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the points' empty
            // destruct there)
            startPointHits.m_size--;
            if (startPointHits.m_size != bestIndex) {
                hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                    startPointHits.m_data + bestIndex, startPointHits.m_data + startPointHits.m_size
                );
            }
        }
        else {
            fireContactRemoved(m_manifold.m_data[i]);
            // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the points' empty
            // destruct there)
            m_manifold.m_size--;
            if (m_manifold.m_size != i) {
                hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                    m_manifold.m_data + i, m_manifold.m_data + m_manifold.m_size
                );
            }
        }
    }

    // The start hits left at the smallest distance replace the manifold point they match, or are added.
    for (int32_t j = 0; j < startPointHits.m_size; ++j) {
        const hkpRootCdPoint& hit = startPointHits.m_data[j];
        if (minimumPenetration == hit.m_contact.m_separatingNormal.m_quad[3]) {
            const int32_t index = findSurface(hit);
            if (index >= 0) {
                hkpRootCdPoint& surface = m_manifold.m_data[index];
                if (hit.m_rootCollidableB != surface.m_rootCollidableB) {
                    fireContactRemoved(surface);
                    fireContactAdded(hit);
                }
                surface.Assign(hit);
            }
            else {
                fireContactAdded(hit);
                // EXE-INLINED none: hkArray<hkpRootCdPoint>::pushBack
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                if (m_manifold.m_size == (m_manifold.m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*pHeap, &m_manifold, static_cast<int32_t>(sizeof(hkpRootCdPoint)));
                }
                hkpRootCdPoint* pNew = &m_manifold.m_data[m_manifold.m_size];
                if (pNew != nullptr) {
                    *pNew = hit;
                }
                m_manifold.m_size++;
            }
        }
    }

    // The cast's first hit is added when it matches no manifold point.
    if (castCollector.m_hits.m_size > 0) {
        const hkpRootCdPoint& hit = castCollector.m_hits.m_data[0];
        if (findSurface(hit) == -1) {
            fireContactAdded(hit);
            // EXE-INLINED none: hkArray<hkpRootCdPoint>::pushBack
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            if (m_manifold.m_size == (m_manifold.m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*pHeap, &m_manifold, static_cast<int32_t>(sizeof(hkpRootCdPoint)));
            }
            hkpRootCdPoint* pNew = &m_manifold.m_data[m_manifold.m_size];
            if (pNew != nullptr) {
                *pNew = hit;
            }
            m_manifold.m_size++;
        }
    }

    // A point within 0.1 of an earlier one is dropped.
    for (int32_t i = m_manifold.m_size - 1; i > 0; --i) {
        for (int32_t j = i - 1; j >= 0; --j) {
            if (surfaceDistance(m_manifold.m_data[i], m_manifold.m_data[j]) < 0.10000000149011612) {
                fireContactRemoved(m_manifold.m_data[i]);
                // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the points' empty
                // destruct there)
                m_manifold.m_size--;
                if (m_manifold.m_size != i) {
                    hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                        m_manifold.m_data + i, m_manifold.m_data + m_manifold.m_size
                    );
                }
                break;
            }
        }
    }

    // The proxy listens to every body and phantom the manifold touches.
    for (int32_t i = 0; i < m_manifold.m_size; ++i) {
        const hkpCollidable* pRoot = m_manifold.m_data[i].m_rootCollidableB;
        // EXE-INLINED 0x4B59F0
        if (pRoot->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
            hkpRigidBody* pBody = static_cast<hkpRigidBody*>(static_cast<hkpWorldObject*>(pRoot->getOwner()));
            if (pBody != nullptr) {
                // EXE-INLINED 0x629F10
                int32_t index = -1;
                for (int32_t k = 0; k < m_bodies.m_size; ++k) {
                    if (m_bodies.m_data[k] == pBody) {
                        index = k;
                        break;
                    }
                }
                if (index == -1) {
                    if (!isMultithreaded.m_bool) {
                        pBody->addEntityListener(this);
                    }
                    // EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    if (m_bodies.m_size == (m_bodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*pHeap, &m_bodies, static_cast<int32_t>(sizeof(hkpRigidBody*)));
                    }
                    m_bodies.m_data[m_bodies.m_size] = pBody;
                    m_bodies.m_size++;
                }
            }
        }
        const hkpCollidable* pRootB = m_manifold.m_data[i].m_rootCollidableB;
        // EXE-INLINED 0x4B5950
        if (pRootB->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
            hkpPhantom* pPhantom = static_cast<hkpPhantom*>(static_cast<hkpWorldObject*>(pRootB->getOwner()));
            if (pPhantom != nullptr) {
                // EXE-INLINED 0x629F10
                int32_t index = -1;
                for (int32_t k = 0; k < m_phantoms.m_size; ++k) {
                    if (m_phantoms.m_data[k] == pPhantom) {
                        index = k;
                        break;
                    }
                }
                if (index == -1) {
                    if (!isMultithreaded.m_bool) {
                        pPhantom->addPhantomListener(this);
                    }
                    // EXE-INLINED none: hkArray<hkpPhantom*>::pushBack
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    if (m_phantoms.m_size ==
                        (m_phantoms.m_capacityAndFlags & hkArrayBase<hkpPhantom*>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*pHeap, &m_phantoms, static_cast<int32_t>(sizeof(hkpPhantom*)));
                    }
                    m_phantoms.m_data[m_phantoms.m_size] = pPhantom;
                    m_phantoms.m_size++;
                }
            }
        }
    }

    // startPointHits is raw storage that C++ never destroys, so the destructor is called here: without it the stack
    // block is never given back and the thread's Havok stack runs past its end.
    startPointHits.~hkLocalArray<hkpRootCdPoint>();
}

// Builds the surface from a contact: the plane pushed out by the keep distance, the proxy's frictions, and for a rigid
// body the surface's velocity at the contact (the body's own getPointVelocity for keyframed and fixed bodies, else from
// its swept transform), with the plane moved by the distance the surface travels in afTimeTravelled. A plane already
// penetrated gets the recovery velocity along the normal.
void hkpCharacterProxy::extractSurfaceConstraintInfo(
    const hkpRootCdPoint& hit,
    hkpSurfaceConstraintInfo& surface,
    float timeTravelled
) {
    surface.m_plane = hit.m_contact.m_separatingNormal;
    surface.m_plane.m_quad[3] = surface.m_plane.m_quad[3] - m_keepDistance;
    surface.m_staticFriction = m_staticFriction;
    surface.m_dynamicFriction = m_dynamicFriction;
    surface.m_extraUpStaticFriction = m_extraUpStaticFriction;
    surface.m_velocity.m_quad = _mm_setzero_ps();
    surface.m_extraDownStaticFriction = m_extraDownStaticFriction;
    surface.m_priority = 0;
    const hkpCollidable* pRoot = hit.m_rootCollidableB;
    // EXE-INLINED 0x4B59F0
    if (pRoot->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(static_cast<hkpWorldObject*>(pRoot->getOwner()));
        if (pBody) {
            uint8_t motionType = pBody->m_motion.m_type.m_storage;
            if (motionType == hkpMotion::MOTION_FIXED || motionType == hkpMotion::MOTION_KEYFRAMED) {
                pBody->getPointVelocity(hit.m_contact.m_position, surface.m_velocity);
            }
            else {
                const hkSweptTransform& swept = pBody->m_motion.m_motionState.m_sweptTransform;
                float invDeltaTime = swept.m_centerOfMass1.m_quad[3];
                __m128 linVel = _mm_mul_ps(
                    _mm_sub_ps(swept.m_centerOfMass1.m_quad, swept.m_centerOfMass0.m_quad),
                    _mm_shuffle_ps(_mm_set_ss(invDeltaTime), _mm_set_ss(invDeltaTime), 0)
                );
                float invDeltaTime2 = swept.m_centerOfMass1.m_quad[3];
                __m128 angVel = _mm_mul_ps(
                    _mm_shuffle_ps(_mm_set_ss(invDeltaTime2), _mm_set_ss(invDeltaTime2), 0),
                    pBody->m_motion.m_motionState.m_deltaAngle.m_quad
                );
                __m128 arm = _mm_sub_ps(hit.m_contact.m_position.m_quad, swept.m_centerOfMass1.m_quad);
                surface.m_velocity.m_quad = _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(angVel, angVel, 0xC9), _mm_shuffle_ps(arm, arm, 0xD2)),
                    _mm_mul_ps(_mm_shuffle_ps(angVel, angVel, 0xD2), _mm_shuffle_ps(arm, arm, 0xC9))
                );
                surface.m_velocity.m_quad = _mm_add_ps(surface.m_velocity.m_quad, linVel);
            }
            // EXE-INLINED none: hkVector4::dot3
            __m128 prod = _mm_mul_ps(surface.m_velocity.m_quad, surface.m_plane.m_quad);
            float velDot = _mm_cvtss_f32(
                _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
            );
            surface.m_plane.m_quad[3] = surface.m_plane.m_quad[3] - velDot * timeTravelled;
            if (pBody->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                surface.m_priority = 2;
            }
            if (pBody->m_motion.m_type.m_storage == hkpMotion::MOTION_KEYFRAMED) {
                surface.m_priority = 1;
            }
        }
    }
    if (surface.m_plane.m_quad[3] < -1.1920929e-07f) {
        float recoverySpeed = m_penetrationRecoverySpeed * -surface.m_plane.m_quad[3];
        surface.m_velocity.m_quad = _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(recoverySpeed), _mm_set_ss(recoverySpeed), 0),
                hit.m_contact.m_separatingNormal.m_quad
            ),
            surface.m_velocity.m_quad
        );
        surface.m_plane.m_quad[3] = 0.0f;
    }
}

// Distance between two contact points, for matching them across steps: the squared difference of the surfaces'
// velocities at the points (a rigid body's getPointVelocity, else zero) times 0.1, plus one minus the cosine between
// the normals times the squared contact angle sensitivity times 10, plus the squared difference of the distances. Each
// term is stored as a float.
float hkpCharacterProxy::surfaceDistance(const hkpRootCdPoint& p1, const hkpRootCdPoint& p2) const {
    // EXE-INLINED none: hkVector4::dot3
    const __m128 normalProduct =
        _mm_mul_ps(p1.m_contact.m_separatingNormal.m_quad, p2.m_contact.m_separatingNormal.m_quad);
    const float cosAngle = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(normalProduct, normalProduct, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(normalProduct, normalProduct, 0x55), normalProduct)
    ));
    const float oneMinusCos = 1.0f - cosAngle;
    const float angleDistance = m_contactAngleSensitivity * m_contactAngleSensitivity * oneMinusCos;
    const float distanceDelta = p1.m_contact.m_separatingNormal.m_quad[3] - p2.m_contact.m_separatingNormal.m_quad[3];
    const float distanceDistance = distanceDelta * distanceDelta;
    hkVector4 velocity2;
    hkVector4 velocity1;
    velocity1.m_quad = _mm_setzero_ps();
    velocity2.m_quad = _mm_setzero_ps();
    const hkpCollidable* pRoot2 = p1.m_rootCollidableB;
    // EXE-INLINED 0x4B59F0
    if (pRoot2->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        hkpRigidBody* pBody2 = static_cast<hkpRigidBody*>(static_cast<hkpWorldObject*>(pRoot2->getOwner()));
        if (pBody2 != nullptr) {
            pBody2->getPointVelocity(p1.m_contact.m_position, velocity2);
        }
    }
    const hkpCollidable* pRoot1 = p2.m_rootCollidableB;
    // EXE-INLINED 0x4B59F0
    if (pRoot1->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        hkpRigidBody* pBody1 = static_cast<hkpRigidBody*>(static_cast<hkpWorldObject*>(pRoot1->getOwner()));
        if (pBody1 != nullptr) {
            pBody1->getPointVelocity(p2.m_contact.m_position, velocity1);
        }
    }
    const __m128 velocityDelta = _mm_sub_ps(velocity2.m_quad, velocity1.m_quad);
    const __m128 velocityProduct = _mm_mul_ps(velocityDelta, velocityDelta);
    // EXE-INLINED none: hkVector4::lengthSquared3
    const float velocityDistance = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(velocityProduct, velocityProduct, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(velocityProduct, velocityProduct, 0x55), velocityProduct)
    ));
    // One x87 sum of the double-constant products, rounded to float at the return.
    return velocityDistance * 0.10000000149011612 + angleDistance * 10.0 + distanceDistance;
}

void hkpCharacterProxy::setVelocity(const hkVector4* avVelocity) {
    m_velocity = *avVelocity;
}

const hkVector4& hkpCharacterProxy::getLinearVelocity() const {
    return m_velocity;
}

// The translation of the transform the phantom's collidable is placed by.
const hkVector4& hkpCharacterProxy::getPosition() const {
    return m_shapePhantom->m_collidable.m_motion.pMotion->m_translation;
}

void hkpCharacterProxy::setPosition(const hkVector4& position) {
    m_shapePhantom->setPosition(position, m_keepContactTolerance + m_keepDistance);
}

void hkpCharacterProxy::integrateWithCollectors(
    const hkStepInfo& stepInfo,
    const hkVector4& gravity,
    hkpAllCdPointCollector& castCollector,
    hkpAllCdPointCollector& startPointCollector
) {
    integrateImplementation(stepInfo, gravity, castCollector, startPointCollector, nullptr, false);
}

// The proxy's step: up to m_maxCastIterations times while time is left, cast the phantom along the last displacement
// for the contacts, turn the manifold into surface constraints, let the simplex solver move the character through them,
// apply the contacts to what they touch, and when the solver's move differs from the cast, cast again along it and stop
// at the first new hit. A multithreaded integrate (command given) moves the phantom's motion state itself and adds no
// listeners. The stack allocator's bytes are untyped memory: each local array keeps its elements there, and its stack
// pointer goes back as bytes. When the loop does not run (the step's time is not above HK_REAL_EPSILON, or
// m_maxCastIterations is not above 0), output.m_velocity is stored into m_velocity without anything having written it,
// as the game does: only hkSimplexSolverSolve fills it.
void hkpCharacterProxy::integrateImplementation(
    const hkStepInfo& stepInfo,
    const hkVector4& gravity,
    hkpAllCdPointCollector& castCollector,
    hkpAllCdPointCollector& startPointCollector,
    hkpCharacterProxyIntegrateCommand* command,
    int32_t maxInteractions
) {
    HK_TIMER_BEGIN_LIST("updateCharacter", "Cast");
    const bool isMultithreaded = command != nullptr;
    hkVector4 position = getPosition();
    float remainingTime = stepInfo.m_deltaTime;
    hkpLinearCastInput castInput;
    castInput.m_startPointTolerance = m_keepContactTolerance + m_keepDistance;
    castInput.m_maxExtraPenetration = 0.01f;
    hkpSimplexSolverOutput output;
    for (int32_t iter = 0; remainingTime > 1.1920929e-07f && iter < m_maxCastIterations; ++iter) {
        HK_TIMER_SPLIT_LIST("InitialCast");
        // The cast goes along the last displacement: only its start points are needed, and the guess may spare the
        // second cast.
        castInput.m_to.setAdd4(m_oldDisplacement, position);
        castCollector.reset();
        startPointCollector.reset();
        hkpShapePhantom* pPhantom = m_shapePhantom;
        if (isMultithreaded) {
            pPhantom->getMotionState()->m_transform.m_translation = position;
            linearCast(hkBool(iter != 0), castInput, castCollector, &startPointCollector);
        }
        else {
            pPhantom->setPositionAndLinearCast(position, castInput, castCollector, &startPointCollector);
        }
        if (castCollector.m_hits.m_size > 0) {
            castCollector.sortHits();
            convertFractionToDistance(castCollector.m_hits.m_data, castCollector.m_hits.m_size, m_oldDisplacement);
        }

        HK_TIMER_SPLIT_LIST("UpdateManifold");
        updateManifold(startPointCollector, castCollector, hkBool(isMultithreaded));

        // The manifold's points as plane constraints, with room for the slope planes and the user planes.
        alignas(16) hkLocalArray<hkpSurfaceConstraintInfo> surfaceConstraints;
        {
            // EXE-INLINED none: hkLocalArray<hkpSurfaceConstraintInfo>::hkLocalArray (hkAllocateStack)
            const int32_t iCapacity = m_manifold.m_size + m_userPlanes + 10;
            surfaceConstraints.m_data = nullptr;
            surfaceConstraints.m_size = 0;
            surfaceConstraints.m_capacityAndFlags = hkArrayBase<hkpSurfaceConstraintInfo>::DONT_DEALLOCATE_FLAG;
            hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
            char* pBlock = stack.m_next.m_storage;
            stack.m_next.m_storage =
                pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpSurfaceConstraintInfo)) + 127) & ~127);
            surfaceConstraints.m_data = reinterpret_cast<hkpSurfaceConstraintInfo*>(pBlock);
            surfaceConstraints.m_localMemory = surfaceConstraints.m_data;
            surfaceConstraints.m_capacityAndFlags =
                iCapacity | hkArrayBase<hkpSurfaceConstraintInfo>::DONT_DEALLOCATE_FLAG;
        }
        surfaceConstraints.setSizeUnchecked(m_manifold.m_size);
        for (int32_t i = 0; i < m_manifold.m_size; ++i) {
            extractSurfaceConstraintInfo(
                m_manifold.m_data[i], surfaceConstraints.m_data[i], stepInfo.m_deltaTime - remainingTime
            );
            addMaxSlopePlane(m_maxSlopeCosine, m_up, i, surfaceConstraints);
        }
        {
            const int32_t iUserPlanes = m_userPlanes;
            if (surfaceConstraints.getCapacity() - surfaceConstraints.m_size < iUserPlanes) {
                surfaceConstraints.reserve(surfaceConstraints.m_size + iUserPlanes);
            }
        }

        HK_TIMER_SPLIT_LIST("SlexMove");
        // The solver's output per constraint.
        alignas(16) hkLocalArray<hkpSurfaceConstraintInteraction> surfaceInteractions;
        {
            // EXE-INLINED none: hkLocalArray<hkpSurfaceConstraintInteraction>::hkLocalArray (hkAllocateStack)
            const int32_t iCapacity = surfaceConstraints.m_size + m_userPlanes;
            surfaceInteractions.m_data = nullptr;
            surfaceInteractions.m_size = 0;
            surfaceInteractions.m_capacityAndFlags = hkArrayBase<hkpSurfaceConstraintInteraction>::DONT_DEALLOCATE_FLAG;
            hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
            char* pBlock = stack.m_next.m_storage;
            stack.m_next.m_storage =
                pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpSurfaceConstraintInteraction)) + 127) & ~127);
            surfaceInteractions.m_data = reinterpret_cast<hkpSurfaceConstraintInteraction*>(pBlock);
            surfaceInteractions.m_localMemory = surfaceInteractions.m_data;
            surfaceInteractions.m_capacityAndFlags =
                iCapacity | hkArrayBase<hkpSurfaceConstraintInteraction>::DONT_DEALLOCATE_FLAG;
        }
        // The hkpSimplexSolverInput constructor's defaults (zero position, up (0,0,1,0), epsilon surface velocity) are
        // all overwritten below, so they are left out.
        hkpSimplexSolverInput input;
        input.m_constraints = surfaceConstraints.m_data;
        input.m_numConstraints = surfaceConstraints.m_size;
        input.m_velocity = m_velocity;
        input.m_deltaTime = remainingTime;
        if (static_cast<float>(m_velocity.lengthSquared3()) == 0.0f) {
            input.m_minDeltaTime = 0.0f;
        }
        else {
            const float lengthInverse = m_velocity.lengthInverse3();
            input.m_minDeltaTime = lengthInverse * m_keepDistance * 0.5;
        }
        input.m_position.setZero4();
        input.m_upVector = m_up;
        input.m_maxSurfaceVelocity.setAll(m_maxCharacterSpeedForSolver);
        fireConstraintsProcessed(m_manifold, input);
        output.m_planeInteractions = surfaceInteractions.m_data;
        hkSimplexSolverSolve(input, output);

        HK_TIMER_SPLIT_LIST("ApplySurf");
        applySurfaceInteractions(stepInfo, gravity, command, maxInteractions);

        HK_TIMER_SPLIT_LIST("CastMove");
        // Whether the solver kept to the cast's direction (within 0.001 on x, y and z): then the cast's contacts hold.
        int32_t simplexResultOk =
            (_mm_movemask_ps(_mm_cmple_ps(
                 _mm_and_ps(_mm_sub_ps(m_oldDisplacement.m_quad, output.m_position.m_quad), hkVector4_setAbs4_absMask),
                 _mm_set1_ps(0.001f)
             )) &
             7) == 7;
        bool bMovedToHit = false;
        if (!simplexResultOk) {
            // Cast again along the solver's move; the start points are known already.
            castInput.m_to.setAdd4(output.m_position, position);
            castCollector.reset();
            hkpShapePhantom* pCastPhantom = m_shapePhantom;
            if (isMultithreaded) {
                pCastPhantom->getMotionState()->m_transform.m_translation = position;
                linearCast(hkBool(true), castInput, castCollector, nullptr);
            }
            else {
                pCastPhantom->setPositionAndLinearCast(position, castInput, castCollector, nullptr);
            }
            if (castCollector.m_hits.m_size > 0) {
                castCollector.sortHits();
                convertFractionToDistance(castCollector.m_hits.m_data, castCollector.m_hits.m_size, output.m_position);
                // Hits already in the manifold are dropped; when none is left the solver's move stands.
                while (findSurface(castCollector.m_hits.m_data[0]) != -1) {
                    // EXE-INLINED none: hkArray<hkpRootCdPoint>::removeAtAndCopy(0)
                    castCollector.m_hits.m_size--;
                    hkMemUtil::memCpy<alignof(hkpRootCdPoint)>(
                        castCollector.m_hits.m_data, castCollector.m_hits.m_data + 1,
                        castCollector.m_hits.m_size * static_cast<int32_t>(sizeof(hkpRootCdPoint))
                    );
                    if (castCollector.m_hits.m_size <= 0) {
                        simplexResultOk = 1;
                        break;
                    }
                }
                if (castCollector.m_hits.m_size > 0) {
                    const hkpRootCdPoint& hit = castCollector.m_hits.m_data[0];
                    fireContactAdded(hit);
                    {
                        // EXE-INLINED none: hkArray<hkpRootCdPoint>::pushBack
                        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                        if (m_manifold.m_size ==
                            (m_manifold.m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(
                                *pHeap, &m_manifold, static_cast<int32_t>(sizeof(hkpRootCdPoint))
                            );
                        }
                        hkpRootCdPoint* pNew = &m_manifold.m_data[m_manifold.m_size];
                        if (pNew != nullptr) {
                            *pNew = hit;
                        }
                        m_manifold.m_size++;
                    }
                    if (!m_refreshManifoldInCheckSupport) {
                        const hkpCollidable* pRootB = hit.m_rootCollidableB;
                        // EXE-INLINED none: hkGetRigidBody
                        hkpRigidBody* body = pRootB->getType() == hkpWorldObject::BROAD_PHASE_ENTITY
                            ? static_cast<hkpRigidBody*>(pRootB->getOwner())
                            : nullptr;
                        if (body != nullptr) {
                            // EXE-INLINED none: hkArray<hkpRigidBody*>::indexOf
                            int32_t index = -1;
                            for (int32_t i = 0; i < m_bodies.m_size; ++i) {
                                if (m_bodies.m_data[i] == body) {
                                    index = i;
                                    break;
                                }
                            }
                            if (index == -1) {
                                if (!isMultithreaded) {
                                    body->addEntityListener(this);
                                }
                                // EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
                                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                                if (m_bodies.m_size ==
                                    (m_bodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
                                    hkArrayUtil::_reserveMore(
                                        *pHeap, &m_bodies, static_cast<int32_t>(sizeof(hkpRigidBody*))
                                    );
                                }
                                m_bodies.m_data[m_bodies.m_size] = body;
                                m_bodies.m_size++;
                            }
                        }
                        const hkpCollidable* pPhantomRootB = hit.m_rootCollidableB;
                        // EXE-INLINED none: hkGetPhantom
                        hkpPhantom* phantom = pPhantomRootB->getType() == hkpWorldObject::BROAD_PHASE_PHANTOM
                            ? static_cast<hkpPhantom*>(pPhantomRootB->getOwner())
                            : nullptr;
                        if (phantom != nullptr) {
                            // EXE-INLINED none: hkArray<hkpPhantom*>::indexOf
                            int32_t index = -1;
                            for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
                                if (m_phantoms.m_data[i] == phantom) {
                                    index = i;
                                    break;
                                }
                            }
                            if (index == -1) {
                                if (!isMultithreaded) {
                                    phantom->addPhantomListener(this);
                                }
                                // EXE-INLINED none: hkArray<hkpPhantom*>::pushBack
                                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                                if (m_phantoms.m_size ==
                                    (m_phantoms.m_capacityAndFlags & hkArrayBase<hkpPhantom*>::CAPACITY_MASK)) {
                                    hkArrayUtil::_reserveMore(
                                        *pHeap, &m_phantoms, static_cast<int32_t>(sizeof(hkpPhantom*))
                                    );
                                }
                                m_phantoms.m_data[m_phantoms.m_size] = phantom;
                                m_phantoms.m_size++;
                            }
                        }
                    }
                }
                if (!simplexResultOk) {
                    // The new hit stops the character: it moves to the hit, and the time it took is used up.
                    const double remainingBefore = remainingTime;
                    remainingTime =
                        remainingBefore - moveToLinearCastHitPosition(output, castCollector, castInput, position);
                    bMovedToHit = true;
                }
            }
        }
        if (!bMovedToHit) {
            remainingTime = remainingTime - output.m_deltaTime;
            position.setAdd4(output.m_position, position);
        }
        m_oldDisplacement = output.m_position;
    }

    m_velocity = output.m_velocity;
    if (isMultithreaded) {
        m_shapePhantom->getMotionState()->m_transform.m_translation = position;
    }
    else {
        m_shapePhantom->setPosition(position, castInput.m_startPointTolerance);
    }
    HK_TIMER_END_LIST();
}

int32_t hkpCharacterProxy::findSurface(const hkpRootCdPoint& a1) const {
    int32_t v6;
    float v9;
    float v7 = 0.10000000149011612f;
    int32_t result = (-1);
    int32_t v4 = 0;
    int32_t v8 = (-1);
    if (((int32_t)m_manifold.m_size) > 0) {
        v6 = 0;
        do {
            v9 = surfaceDistance(a1, m_manifold.m_data[v6]);
            if (v7 > ((double)v9)) {
                v7 = v9;
                v8 = v4;
            }
            ++v4;
            ++v6;
        }
        while (v4 < ((int32_t)m_manifold.m_size));
        return v8;
    }
    return result;
}

// Copies the creation info into the proxy: a reference is taken on its shape phantom before the old one is dropped, the
// settings are copied, the up vector is normalized, the phantom is moved when it is in a world, the known bodies and
// phantoms lose their listeners, and the proxy binds itself to the phantom under its own property key.
void hkpCharacterProxy::updateFromCinfo(const hkpCharacterProxyCinfo& info) {
    info.m_shapePhantom->addReference();
    if (m_shapePhantom != nullptr) {
        m_shapePhantom->removeReference();
    }
    m_velocity = info.m_velocity;
    m_dynamicFriction = info.m_dynamicFriction;
    m_staticFriction = info.m_staticFriction;
    m_keepContactTolerance = info.m_keepContactTolerance;
    m_extraUpStaticFriction = info.m_extraUpStaticFriction;
    m_extraDownStaticFriction = info.m_extraDownStaticFriction;
    m_keepDistance = info.m_keepDistance;
    m_shapePhantom = info.m_shapePhantom;
    m_contactAngleSensitivity = info.m_contactAngleSensitivity;
    m_userPlanes = static_cast<int32_t>(info.m_userPlanes);
    m_maxCharacterSpeedForSolver = info.m_maxCharacterSpeedForSolver;
    m_characterStrength = info.m_characterStrength;
    m_characterMass = info.m_characterMass;
    float fMaxSlopeCosine = std::cos(info.m_maxSlope);
    m_maxSlopeCosine = fMaxSlopeCosine;
    m_penetrationRecoverySpeed = info.m_penetrationRecoverySpeed;
    m_maxCastIterations = info.m_maxCastIterations;
    m_refreshManifoldInCheckSupport = info.m_refreshManifoldInCheckSupport;
    m_up = info.m_up;
    m_up.normalize3();
    if (m_shapePhantom->m_world != nullptr) {
        setPosition(info.m_position);
    }
    m_oldDisplacement.m_quad[0] = 0.0f;
    m_oldDisplacement.m_quad[1] = 0.0f;
    m_oldDisplacement.m_quad[2] = 0.0f;
    m_oldDisplacement.m_quad[3] = 0.0f;
    for (int32_t i = 0; i < m_bodies.m_size; ++i) {
        m_bodies.m_data[i]->removeEntityListener(static_cast<hkpEntityListener*>(this));
    }
    m_bodies.m_size = 0;
    for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
        m_phantoms.m_data[i]->removePhantomListener(static_cast<hkpPhantomListener*>(this));
    }
    m_phantoms.m_size = 0;
    m_manifold.m_size = 0;
    // Scans the phantom's properties for the key and only adds the property when the phantom lacks it.
    hkpShapePhantom* pPhantom = m_shapePhantom;
    const int32_t numProperties = pPhantom->m_properties.m_size;
    bool bHasProperty = false;
    if (numProperties > 0) {
        const hkpProperty* pProperty = pPhantom->m_properties.m_data;
        for (int32_t i = 0; i < numProperties; ++i, ++pProperty) {
            if (pProperty->m_key == PROPERTY_KEY_CHARACTER_PROXY) {
                bHasProperty = true;
                break;
            }
        }
    }
    if (!bHasProperty) {
        hkpPropertyValue kValue;
        kValue.m_data = reinterpret_cast<uintptr_t>(this);
        pPhantom->addProperty(PROPERTY_KEY_CHARACTER_PROXY, kValue);
    }
}

// Fills arInfo from the proxy.
void hkpCharacterProxy::getCinfo(hkpCharacterProxyCinfo& info) const {
    double v6;
    info.m_position.m_quad = (&getPosition())->m_quad;
    info.m_velocity = m_velocity;
    info.m_shapePhantom = m_shapePhantom;
    info.m_dynamicFriction = m_dynamicFriction;
    info.m_staticFriction = m_staticFriction;
    info.m_keepContactTolerance = m_keepContactTolerance;
    info.m_up = m_up;
    info.m_extraUpStaticFriction = m_extraUpStaticFriction;
    info.m_extraDownStaticFriction = m_extraDownStaticFriction;
    info.m_keepDistance = m_keepDistance;
    info.m_contactAngleSensitivity = m_contactAngleSensitivity;
    info.m_userPlanes = m_userPlanes;
    info.m_maxCharacterSpeedForSolver = m_maxCharacterSpeedForSolver;
    info.m_characterStrength = m_characterStrength;
    info.m_characterMass = m_characterMass;
    double m_maxSlopeCosine = this->m_maxSlopeCosine;
    float v7 = std::fabs(m_maxSlopeCosine);
    if (!(v7 >= 1.0f)) {
        float v9 = std::acos(m_maxSlopeCosine);
        v6 = v9;
    }
    else {
        bool v4 = m_maxSlopeCosine > 0.0;
        double v5 = 0.0;
        if (!v4) {
            v5 = 3.1415927f;
        }
        float v8 = v5;
        v6 = v8;
    }
    float v10 = v6;
    info.m_maxSlope = v10;
    info.m_penetrationRecoverySpeed = m_penetrationRecoverySpeed;
    info.m_maxCastIterations = m_maxCastIterations;
    info.m_refreshManifoldInCheckSupport = m_refreshManifoldInCheckSupport;
}

// Runs integrateImplementation single-threaded, with a cast and a start point collector built on the stack.
void hkpCharacterProxy::integrate(const hkStepInfo& stepInfo, const hkVector4& worldGravity) {
    hkpAllCdPointCollector castCollector;
    hkpAllCdPointCollector startPointCollector;
    integrateImplementation(stepInfo, worldGravity, castCollector, startPointCollector, nullptr, 0);
}

// integrate with a multithreading command: integrateImplementation with a cast and a start point collector built on the
// stack.
void hkpCharacterProxy::integrateMultithreaded(
    const hkStepInfo& stepInfo,
    const hkVector4& gravity,
    hkpCharacterProxyIntegrateCommand* command,
    int32_t maxInteractions
) {
    hkpAllCdPointCollector castCollector;
    hkpAllCdPointCollector startPointCollector;
    integrateImplementation(stepInfo, gravity, castCollector, startPointCollector, command, maxInteractions);
}

// The support the proxy stands on, found through a collector built on the stack.
void hkpCharacterProxy::checkSupport(const hkVector4& direction, hkpSurfaceInfo& ground) {
    hkpAllCdPointCollector collector;
    checkSupportWithCollector(direction, ground, collector);
}

// Finds the support the manifold gives against direction: runs the simplex solver on the manifold's surface constraints
// with their velocities removed and fills ground with the supporting state, the averaged normal of the touched planes
// facing direction and their averaged velocity. The stack allocator's bytes are untyped memory: each local array keeps
// its elements there, and its stack pointer goes back as bytes.
void hkpCharacterProxy::checkSupportWithCollector(
    const hkVector4& direction,
    hkpSurfaceInfo& ground,
    hkpAllCdPointCollector& startPointCollector
) {
    HK_TIMER_BEGIN("checkSupport", nullptr);
    if (m_refreshManifoldInCheckSupport) {
        refreshManifold(startPointCollector);
    }
    alignas(16) hkLocalArray<hkpSurfaceConstraintInfo> constraints;
    {
        // EXE-INLINED none: hkLocalArray<hkpSurfaceConstraintInfo>::hkLocalArray (hkAllocateStack)
        const int32_t iCapacity = m_manifold.m_size + m_userPlanes + 10;
        constraints.m_data = nullptr;
        constraints.m_size = 0;
        constraints.m_capacityAndFlags = hkArrayBase<hkpSurfaceConstraintInfo>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpSurfaceConstraintInfo)) + 127) & ~127);
        constraints.m_data = reinterpret_cast<hkpSurfaceConstraintInfo*>(pBlock);
        constraints.m_localMemory = constraints.m_data;
        constraints.m_capacityAndFlags = iCapacity | hkArrayBase<hkpSurfaceConstraintInfo>::DONT_DEALLOCATE_FLAG;
    }
    constraints.setSizeUnchecked(m_manifold.m_size);
    for (int32_t i = 0; i < m_manifold.m_size; ++i) {
        extractSurfaceConstraintInfo(m_manifold.m_data[i], constraints.m_data[i], 0.0f);
        addMaxSlopePlane(m_maxSlopeCosine, m_up, i, constraints);
    }
    {
        // The SDK's room for the user planes: reserved only when the free capacity falls short.
        const int32_t iUserPlanes = m_userPlanes;
        if (constraints.getCapacity() - constraints.m_size < iUserPlanes) {
            constraints.reserve(constraints.m_size + iUserPlanes);
        }
    }
    // The solver's output per constraint.
    alignas(16) hkLocalArray<hkpSurfaceConstraintInteraction> interactions;
    {
        // EXE-INLINED none: hkLocalArray<hkpSurfaceConstraintInteraction>::hkLocalArray (hkAllocateStack)
        const int32_t iCapacity = constraints.m_size + m_userPlanes;
        interactions.m_data = nullptr;
        interactions.m_size = 0;
        interactions.m_capacityAndFlags = hkArrayBase<hkpSurfaceConstraintInteraction>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpSurfaceConstraintInteraction)) + 127) & ~127);
        interactions.m_capacityAndFlags =
            iCapacity | hkArrayBase<hkpSurfaceConstraintInteraction>::DONT_DEALLOCATE_FLAG;
        interactions.m_data = reinterpret_cast<hkpSurfaceConstraintInteraction*>(pBlock);
        interactions.m_localMemory = interactions.m_data;
    }
    // The constraints' velocities, kept to give the surface velocity of the supporting planes.
    alignas(16) hkLocalArray<hkVector4> storedVelocities;
    {
        // EXE-INLINED none: hkLocalArray<hkVector4>::hkLocalArray (hkAllocateStack)
        const int32_t iCapacity = constraints.m_size + m_userPlanes;
        storedVelocities.m_data = nullptr;
        storedVelocities.m_size = 0;
        storedVelocities.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkVector4)) + 127) & ~127);
        storedVelocities.m_data = reinterpret_cast<hkVector4*>(pBlock);
        storedVelocities.m_localMemory = storedVelocities.m_data;
        storedVelocities.m_capacityAndFlags = iCapacity | hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
    }
    hkpSimplexSolverInput input;
    input.m_deltaTime = 0.016666668f;
    input.m_minDeltaTime = 0.016666668f;
    input.m_position.m_quad = _mm_setzero_ps();
    input.m_velocity.m_quad = direction.m_quad;
    input.m_upVector = m_up;
    input.m_constraints = constraints.m_data;
    input.m_numConstraints = constraints.m_size;
    input.m_maxSurfaceVelocity.m_quad = _mm_set1_ps(m_maxCharacterSpeedForSolver);
    hkpSimplexSolverOutput output;
    output.m_planeInteractions = interactions.m_data;
    fireConstraintsProcessed(m_manifold, input);
    // EXE-INLINED none: hkArray<T>::setSize, written as its reserve and size store (the element classes have no
    // hkArrayUtil::construct of their own).
    {
        const int32_t iNum = input.m_numConstraints;
        storedVelocities.reserve(iNum);
        storedVelocities.m_size = iNum;
    }
    {
        const int32_t iNum = input.m_numConstraints;
        interactions.reserve(iNum);
        interactions.m_size = iNum;
    }
    {
        const int32_t iNum = input.m_numConstraints;
        constraints.reserve(iNum);
        constraints.m_size = iNum;
    }
    // The query is of the static geometry: the constraints' velocities are kept aside and cleared.
    for (int32_t i = 0; i < input.m_numConstraints; ++i) {
        storedVelocities.m_data[i] = constraints.m_data[i].m_velocity;
        constraints.m_data[i].m_velocity.m_quad = _mm_setzero_ps();
    }
    hkSimplexSolverSolve(input, output);
    ground.m_surfaceVelocity.m_quad = _mm_setzero_ps();
    ground.m_surfaceNormal.m_quad = _mm_setzero_ps();
    const __m128 dir = direction.m_quad;
    const __m128 vel = output.m_velocity.m_quad;
    // EXE-INLINED 0x458900: hkVector4::equals3 (the output velocity within 0.001 of direction: no support)
    const __m128 absDiff = _mm_and_ps(_mm_sub_ps(vel, dir), hkVector4_setAbs4_absMask);
    if ((_mm_movemask_ps(_mm_cmple_ps(absDiff, _mm_set1_ps(0.001f))) & 7) == 7) {
        ground.m_supportedState = hkpSurfaceInfo::UNSUPPORTED;
    }
    else {
        // EXE-INLINED 0x630100: hkVector4::lengthSquared3
        const __m128 velSq = _mm_mul_ps(vel, vel);
        const __m128 velLenSq =
            _mm_add_ss(_mm_shuffle_ps(velSq, velSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(velSq, velSq, 0x55), velSq));
        if (_mm_cvtss_f32(velLenSq) < 0.001f) {
            ground.m_supportedState = hkpSurfaceInfo::SUPPORTED;
        }
        else {
            // EXE-INLINED 0x62AEA0: hkVector4::normalize3
            const __m128 approx = _mm_rsqrt_ss(velLenSq);
            const __m128 lenIsZero = _mm_cmpeq_ss(velLenSq, _mm_set_ss(zero));
            const __m128 refined = _mm_mul_ss(
                _mm_mul_ss(_mm_set_ss(f0_5_), approx),
                _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(velLenSq, approx), approx))
            );
            const __m128 lenInv = _mm_andnot_ps(lenIsZero, refined);
            output.m_velocity.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), vel);
            // EXE-INLINED 0x62B0A0: hkVector4::dot3
            const __m128 dotProd = _mm_mul_ps(dir, output.m_velocity.m_quad);
            const float fAngleSin = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(dotProd, dotProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(dotProd, dotProd, 0x55), dotProd)
            ));
            // Stores 1 - sin^2 as a float and compares it with the product of the slope cosine.
            const float fCosSqr = 1.0f - fAngleSin * fAngleSin;
            if (fCosSqr < m_maxSlopeCosine * m_maxSlopeCosine) {
                ground.m_supportedState = hkpSurfaceInfo::SLIDING;
            }
            else {
                ground.m_supportedState = hkpSurfaceInfo::SUPPORTED;
            }
        }
        // The touched planes that face direction are the supporting surfaces.
        int32_t iNumTouching = 0;
        for (int32_t i = 0; i < input.m_numConstraints; ++i) {
            if (interactions.m_data[i].m_touched) {
                const __m128 plane = constraints.m_data[i].m_plane.m_quad;
                // EXE-INLINED 0x62B0A0: hkVector4::dot3
                const __m128 planeDot = _mm_mul_ps(direction.m_quad, plane);
                const float fDot = _mm_cvtss_f32(_mm_add_ss(
                    _mm_shuffle_ps(planeDot, planeDot, 0xAA),
                    _mm_add_ss(_mm_shuffle_ps(planeDot, planeDot, 0x55), planeDot)
                ));
                if (fDot < -0.08f) {
                    ++iNumTouching;
                    ground.m_surfaceNormal.m_quad = _mm_add_ps(ground.m_surfaceNormal.m_quad, plane);
                    ground.m_surfaceVelocity.m_quad =
                        _mm_add_ps(ground.m_surfaceVelocity.m_quad, storedVelocities.m_data[i].m_quad);
                }
            }
        }
        if (iNumTouching > 0) {
            const float fInvNumTouching = 1.0f / iNumTouching;
            // EXE-INLINED 0x62AEA0: hkVector4::normalize3
            const __m128 normal = ground.m_surfaceNormal.m_quad;
            const __m128 normalSq = _mm_mul_ps(normal, normal);
            const __m128 normalLenSq = _mm_add_ss(
                _mm_shuffle_ps(normalSq, normalSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(normalSq, normalSq, 0x55), normalSq)
            );
            const __m128 approx = _mm_rsqrt_ss(normalLenSq);
            const __m128 lenIsZero = _mm_cmpeq_ss(normalLenSq, _mm_set_ss(zero));
            const __m128 refined = _mm_mul_ss(
                _mm_mul_ss(_mm_set_ss(f0_5_), approx),
                _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(normalLenSq, approx), approx))
            );
            const __m128 lenInv = _mm_andnot_ps(lenIsZero, refined);
            ground.m_surfaceNormal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), normal);
            ground.m_surfaceVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fInvNumTouching), ground.m_surfaceVelocity.m_quad);
        }
        else {
            ground.m_supportedState = hkpSurfaceInfo::UNSUPPORTED;
        }
    }
    ground.m_surfaceDistanceExcess = 0.0f;
    HK_TIMER_END();
}

// Refreshes the start point collector with the closest points of the phantom, at the world's tolerance widened by the
// keep distance and the keep contact tolerance, then updates the manifold from it with an empty cast collector.
void hkpCharacterProxy::refreshManifold(hkpAllCdPointCollector& startPointCollector) {
    hkpCollisionInput input;
    startPointCollector.reset();
    hkpProcessCollisionInput* pCollisionInput = m_shapePhantom->m_world->m_collisionInput;
    // Copies the hkpCollisionInput part of the world's input member by member, padding left out.
    input.m_dispatcher = pCollisionInput->m_dispatcher;
    input.m_weldClosestPoints.m_storage = pCollisionInput->m_weldClosestPoints.m_storage;
    input.m_tolerance = pCollisionInput->m_tolerance;
    input.m_filter = pCollisionInput->m_filter;
    input.m_convexListFilter = pCollisionInput->m_convexListFilter;
    input.m_createPredictiveAgents.m_storage = pCollisionInput->m_createPredictiveAgents.m_storage;
    input.m_aabb32Info.m_bitOffsetLow = pCollisionInput->m_aabb32Info.m_bitOffsetLow;
    input.m_aabb32Info.m_bitOffsetHigh = pCollisionInput->m_aabb32Info.m_bitOffsetHigh;
    input.m_aabb32Info.m_bitScale = pCollisionInput->m_aabb32Info.m_bitScale;
    // One x87 sum, rounded to float only at the store.
    input.m_tolerance = m_keepContactTolerance + m_keepDistance + input.m_tolerance;
    m_shapePhantom->getClosestPoints(startPointCollector, &input);
    hkpAllCdPointCollector castCollector;
    updateManifold(startPointCollector, castCollector, hkBool(false));
}

hkpShapePhantom* hkpCharacterProxy::getShapePhantom() {
    return m_shapePhantom;
}

// Deprecated support query: the simplex solver is run on the manifold's surface constraints (velocities kept) for the
// proxy's velocity plus direction; ground says whether the solved velocity lost speed along direction (supported) and
// still moves across it (sliding), with the averaged normal and velocity of the touched planes that face direction. The
// stack allocator's bytes are untyped memory: each local array keeps its elements there, and its stack pointer goes
// back as bytes.
void hkpCharacterProxy::checkSupportDeprecated(const hkVector4& direction, hkpSurfaceInfoDeprecated& ground) {
    HK_TIMER_BEGIN("checkSupport", nullptr);
    XFNV_INLINED_LOCAL(hkLocalArray<hkpSurfaceConstraintInfo>, constraints, std::nullopt, std::nullopt);
    {
        // EXE-INLINED none: hkLocalArray<hkpSurfaceConstraintInfo>::hkLocalArray (hkAllocateStack)
        const int32_t iCapacity = m_manifold.m_size + m_userPlanes + 10;
        constraints.m_data = nullptr;
        constraints.m_size = 0;
        constraints.m_capacityAndFlags = hkArrayBase<hkpSurfaceConstraintInfo>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpSurfaceConstraintInfo)) + 127) & ~127);
        constraints.m_data = reinterpret_cast<hkpSurfaceConstraintInfo*>(pBlock);
        constraints.m_localMemory = constraints.m_data;
        constraints.m_capacityAndFlags = iCapacity | hkArrayBase<hkpSurfaceConstraintInfo>::DONT_DEALLOCATE_FLAG;
    }
    constraints.setSizeUnchecked(m_manifold.m_size);
    for (int32_t i = 0; i < m_manifold.m_size; ++i) {
        extractSurfaceConstraintInfo(m_manifold.m_data[i], constraints.m_data[i], 0.0f);
        addMaxSlopePlane(m_maxSlopeCosine, m_up, i, constraints);
    }
    {
        // The SDK's room for the user planes: reserved only when the free capacity falls short.
        const int32_t iUserPlanes = m_userPlanes;
        if (constraints.getCapacity() - constraints.m_size < iUserPlanes) {
            constraints.reserve(constraints.m_size + iUserPlanes);
        }
    }
    // The solver's output per constraint.
    XFNV_INLINED_LOCAL(hkLocalArray<hkpSurfaceConstraintInteraction>, interactions, std::nullopt, std::nullopt);
    {
        // EXE-INLINED none: hkLocalArray<hkpSurfaceConstraintInteraction>::hkLocalArray (hkAllocateStack)
        const int32_t iCapacity = constraints.m_size + m_userPlanes;
        interactions.m_data = nullptr;
        interactions.m_size = 0;
        interactions.m_capacityAndFlags = hkArrayBase<hkpSurfaceConstraintInteraction>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpSurfaceConstraintInteraction)) + 127) & ~127);
        interactions.m_capacityAndFlags =
            iCapacity | hkArrayBase<hkpSurfaceConstraintInteraction>::DONT_DEALLOCATE_FLAG;
        interactions.m_data = reinterpret_cast<hkpSurfaceConstraintInteraction*>(pBlock);
        interactions.m_localMemory = interactions.m_data;
    }
    {
        // EXE-INLINED none: hkArray<hkpSurfaceConstraintInteraction>::setSize, written as its reserve and size store
        // (the element class has no hkArrayUtil::construct of its own).
        const int32_t iNum = constraints.m_size + m_userPlanes;
        interactions.reserve(iNum);
        interactions.m_size = iNum;
    }
    hkpSimplexSolverInput input;
    input.m_constraints = constraints.m_data;
    input.m_numConstraints = m_manifold.m_size;
    input.m_velocity.m_quad = _mm_add_ps(m_velocity.m_quad, direction.m_quad);
    input.m_deltaTime = 0.016666668f;
    input.m_minDeltaTime = 0.016666668f;
    input.m_position.m_quad = _mm_setzero_ps();
    input.m_upVector = m_up;
    input.m_maxSurfaceVelocity.m_quad = _mm_set1_ps(m_maxCharacterSpeedForSolver);
    hkpSimplexSolverOutput output;
    output.m_planeInteractions = interactions.m_data;
    fireConstraintsProcessed(m_manifold, input);
    hkSimplexSolverSolve(input, output);
    output.m_velocity.m_quad = _mm_sub_ps(output.m_velocity.m_quad, m_velocity.m_quad);
    // EXE-INLINED 0x62B0A0: hkVector4::dot3
    const __m128 projProd = _mm_mul_ps(direction.m_quad, output.m_velocity.m_quad);
    const float fProjectedVelocity = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(projProd, projProd, 0xAA), _mm_add_ss(_mm_shuffle_ps(projProd, projProd, 0x55), projProd)
    ));
    // Compares against 0.99f widened to double.
    ground.m_isSupported.m_bool = fProjectedVelocity < 0.9900000095367432;
    // The negated projection is stored as a float.
    const float fNegProjected = -fProjectedVelocity;
    const __m128 across =
        _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fNegProjected), direction.m_quad), output.m_velocity.m_quad);
    // EXE-INLINED 0x630100: hkVector4::lengthSquared3
    const __m128 acrossSq = _mm_mul_ps(across, across);
    const float fAcrossLenSq = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(acrossSq, acrossSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(acrossSq, acrossSq, 0x55), acrossSq)
    ));
    ground.m_isSliding.m_bool = fAcrossLenSq > 0.01f;
    ground.m_surfaceVelocity.m_quad = _mm_setzero_ps();
    ground.m_surfaceNormal.m_quad = _mm_setzero_ps();
    // The touched planes that face direction are the supporting surfaces.
    int32_t iNumTouching = 0;
    for (int32_t i = 0; i < input.m_numConstraints; ++i) {
        if (interactions.m_data[i].m_touched) {
            const __m128 plane = constraints.m_data[i].m_plane.m_quad;
            // EXE-INLINED 0x62B0A0: hkVector4::dot3
            const __m128 planeDot = _mm_mul_ps(plane, direction.m_quad);
            const float fDot = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(planeDot, planeDot, 0xAA), _mm_add_ss(_mm_shuffle_ps(planeDot, planeDot, 0x55), planeDot)
            ));
            if (fDot < -0.08f) {
                ground.m_surfaceNormal.m_quad = _mm_add_ps(plane, ground.m_surfaceNormal.m_quad);
                ground.m_surfaceVelocity.m_quad =
                    _mm_add_ps(constraints.m_data[i].m_velocity.m_quad, ground.m_surfaceVelocity.m_quad);
                ++iNumTouching;
            }
        }
    }
    if (iNumTouching > 0) {
        const float fInvNumTouching = 1.0f / iNumTouching;
        // EXE-INLINED 0x62AEA0: hkVector4::normalize3
        const __m128 normal = ground.m_surfaceNormal.m_quad;
        const __m128 normalSq = _mm_mul_ps(normal, normal);
        const __m128 normalLenSq = _mm_add_ss(
            _mm_shuffle_ps(normalSq, normalSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(normalSq, normalSq, 0x55), normalSq)
        );
        const __m128 approx = _mm_rsqrt_ss(normalLenSq);
        const __m128 lenIsZero = _mm_cmpeq_ss(normalLenSq, _mm_set_ss(zero));
        const __m128 refined = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(f0_5_), approx),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(normalLenSq, approx), approx))
        );
        const __m128 lenInv = _mm_andnot_ps(lenIsZero, refined);
        ground.m_surfaceNormal.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), normal);
        ground.m_surfaceVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fInvNumTouching), ground.m_surfaceVelocity.m_quad);
    }
    else {
        ground.m_isSupported.m_bool = false;
    }
    HK_TIMER_END();
    // The local arrays are destroyed in reverse order.
    interactions.~hkLocalArray<hkpSurfaceConstraintInteraction>();
    constraints.~hkLocalArray<hkpSurfaceConstraintInfo>();
}

// Adds apListener to the proxy's listeners.
void hkpCharacterProxy::addCharacterProxyListener(hkpCharacterProxyListener* cpl) {
    hkArray<hkpCharacterProxyListener*, hkContainerHeapAllocator>* p_m_listeners = &m_listeners;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_listeners->m_size ==
        (p_m_listeners->m_capacityAndFlags & hkArrayBase<hkpCharacterProxyListener*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_listeners, sizeof(hkpCharacterProxyListener*));
    }
    p_m_listeners->m_data[p_m_listeners->m_size++] = cpl;
}

void hkpCharacterProxy::removeCharacterProxyListener(hkpCharacterProxyListener* cpl) {
    int32_t m_size = m_listeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    LABEL_5:;
        v3 = -1;
    }
    else {
        hkpCharacterProxyListener** m_data = static_cast<hkArrayBase<hkpCharacterProxyListener*>&>(m_listeners).begin();
        while (*m_data != cpl) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto LABEL_5;
            }
        }
    }
    uint32_t v5 = --m_listeners.m_size;
    if (v5 != v3) {
        m_listeners.m_data[v3] = m_listeners.m_data[v5];
    }
}

// Linear cast of the proxy's phantom: through the world when multithreaded, else against each body the phantom
// overlaps, with a collision input built from the world's (the path is the last displacement). A simple phantom casts
// through the dispatcher's linear-cast function for the two shape types, a caching phantom through each overlap's
// agent. The input's inline constructor stores (m_weldClosestPoints 0, m_maxExtraPenetration epsilon) are overwritten
// before any use and left out. The input's padding is left unwritten, as the game does; the callees take it by
// reference.
void hkpCharacterProxy::linearCast(
    hkBool doWorldCast,
    const hkpLinearCastInput& arInput,
    hkpAllCdPointCollector& castCollector,
    hkpAllCdPointCollector* startCollector
) {
    hkpShapePhantom* pPhantom = m_shapePhantom;
    hkpWorld* pWorld = pPhantom->m_world;
    const hkpCollidable* pCollidableA = &pPhantom->m_collidable;
    if (doWorldCast.m_bool) {
        pWorld->linearCast(pCollidableA, arInput, castCollector, startCollector);
        return;
    }
    alignas(16) hkpLinearCastCollisionInput input;
    const hkpProcessCollisionInput* pProcessInput = pWorld->m_collisionInput;
    // EXE-INLINED none: hkpLinearCastCollisionInput::set
    input.m_dispatcher = pProcessInput->m_dispatcher;
    input.m_weldClosestPoints = pProcessInput->m_weldClosestPoints;
    input.m_tolerance = pProcessInput->m_tolerance;
    input.m_filter = pProcessInput->m_filter;
    input.m_convexListFilter = pProcessInput->m_convexListFilter;
    input.m_createPredictiveAgents = pProcessInput->m_createPredictiveAgents;
    input.m_aabb32Info.m_bitOffsetLow = pProcessInput->m_aabb32Info.m_bitOffsetLow;
    input.m_aabb32Info.m_bitOffsetHigh = pProcessInput->m_aabb32Info.m_bitOffsetHigh;
    input.m_aabb32Info.m_bitScale = pProcessInput->m_aabb32Info.m_bitScale;
    input.m_config = pProcessInput->m_config;
    // EXE-INLINED none: hkpLinearCastCollisionInput::setPathAndTolerance
    input.m_path = m_oldDisplacement;
    __m128 sq = _mm_mul_ps(m_oldDisplacement.m_quad, m_oldDisplacement.m_quad);
    input.m_cachedPathLength = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)))
    );
    input.m_tolerance = arInput.m_startPointTolerance;
    input.m_maxExtraPenetration = arInput.m_maxExtraPenetration;

    if (pPhantom->getType() == HK_PHANTOM_SIMPLE_SHAPE) {
        hkpSimpleShapePhantom* pSimple = static_cast<hkpSimpleShapePhantom*>(m_shapePhantom);
        hkpCollisionDispatcher* pDispatcher = pWorld->m_collisionDispatcher;
        hkArray<hkpSimpleShapePhantom::CollisionDetail>& details = pSimple->m_collisionDetails;
        for (int32_t i = details.m_size - 1; i >= 0; --i) {
            const hkpCollidable* pCollidableB = details.m_data[i].m_collidable;
            hkpCollisionDispatcher::LinearCastFunc linearCastFunc = pDispatcher->getLinearCastFunc(
                static_cast<hkpShapeType>(pCollidableA->m_shape->m_type.m_storage),
                static_cast<hkpShapeType>(pCollidableB->m_shape->m_type.m_storage)
            );
            linearCastFunc(*pCollidableA, *pCollidableB, input, castCollector, startCollector);
        }
    }
    else {
        hkpCachingShapePhantom* pCaching = static_cast<hkpCachingShapePhantom*>(m_shapePhantom);
        hkArray<hkpCachingShapePhantom::hkpCollisionDetail>& details = pCaching->m_collisionDetails;
        for (int32_t i = details.m_size - 1; i >= 0; --i) {
            hkpCachingShapePhantom::hkpCollisionDetail& detail = details.m_data[i];
            detail.m_agent->linearCast(*pCollidableA, *detail.m_collidable, input, castCollector, startCollector);
        }
    }
}

// Applies the manifold's contacts to what they touch: another character proxy is reported to the listeners; a rigid
// body that is neither fixed nor keyframed gets the impulse that stops it pushing into the character, plus the
// character's weight, after the listeners have seen it. A multithreaded integrate (command given) records at most
// maxInteractions impulses for the caller to apply and ends the list with a null body when it has room. The only caller
// is integrateImplementation.
void hkpCharacterProxy::applySurfaceInteractions(
    const hkStepInfo& stepInfo,
    const hkVector4& worldGravity,
    hkpCharacterProxyIntegrateCommand* command,
    int32_t maxInteractions
) {
    hkpCharacterObjectInteractionEvent input;
    input.m_timestep = stepInfo.m_deltaTime;
    int32_t numInteractions = m_manifold.m_size;
    hkpCharacterProxyInteractionResults* interactions = nullptr;
    if (command != nullptr) {
        if (maxInteractions < numInteractions) {
            numInteractions = maxInteractions;
        }
        interactions = command->m_objectInteraction;
    }
    for (int32_t s = 0; s < numInteractions; ++s) {
        // Another character: the listeners hear of it.
        const hkpWorldObject* object =
            static_cast<const hkpWorldObject*>(m_manifold.m_data[s].m_rootCollidableB->getOwner());
        // EXE-INLINED none: hkpWorldObject::hasProperty
        bool bHasProxy = false;
        for (int32_t i = 0; i < object->m_properties.m_size; ++i) {
            if (object->m_properties.m_data[i].m_key == PROPERTY_KEY_CHARACTER_PROXY) {
                bHasProxy = true;
                break;
            }
        }
        if (bHasProxy) {
            hkpCharacterProxy* otherChar =
                static_cast<hkpCharacterProxy*>(object->getProperty(PROPERTY_KEY_CHARACTER_PROXY).getPtr());
            for (int32_t i = m_listeners.m_size - 1; i >= 0; --i) {
                if (m_listeners.m_data[i] != nullptr) {
                    m_listeners.m_data[i]->characterInteractionCallback(
                        this, otherChar, m_manifold.m_data[s].m_contact
                    );
                }
            }
        }

        // A rigid body that moves: the character pushes it.
        const hkpCollidable* rootCollidable = m_manifold.m_data[s].m_rootCollidableB;
        // EXE-INLINED none: hkGetRigidBody
        hkpRigidBody* body = rootCollidable->getType() == hkpWorldObject::BROAD_PHASE_ENTITY
            ? static_cast<hkpRigidBody*>(rootCollidable->getOwner())
            : nullptr;
        if (body == nullptr) {
            continue;
        }
        if (body->isFixedOrKeyframed().m_bool) {
            continue;
        }
        const hkpRootCdPoint& point = m_manifold.m_data[s];
        input.m_position = point.m_contact.m_position;
        input.m_position.m_quad[3] = point.m_contact.m_separatingNormal.m_quad[3];
        input.m_normal = point.m_contact.m_separatingNormal;
        input.m_body = body;

        // The velocity change that stops the body moving into the character, plus a recovery term while they
        // penetrate.
        float deltaVelocity;
        {
            hkVector4 pointVel;
            body->m_motion.getPointVelocity(input.m_position, pointVel);
            pointVel.setSub4(pointVel, m_velocity);
            // EXE-INLINED 0x62B0A0
            const __m128 kVelProducts = _mm_mul_ps(pointVel.m_quad, input.m_normal.m_quad);
            input.m_projectedVelocity = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(kVelProducts, kVelProducts, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kVelProducts, kVelProducts, 0x55), kVelProducts)
            ));
            deltaVelocity = -input.m_projectedVelocity * 0.8999999761581421;
            if (input.m_position.m_quad[3] < 0.0f) {
                deltaVelocity =
                    input.m_position.m_quad[3] * stepInfo.m_invDeltaTime * 0.4000000059604645 + deltaVelocity;
            }
        }

        hkpCharacterObjectInteractionResult output;
        output.m_impulsePosition = input.m_position;
        output.m_objectImpulse.setZero4();
        if (deltaVelocity < 0.0f) {
            hkMatrix3 inertiaInv;
            hkpMotion* pMotion = &body->m_motion;
            pMotion->getInertiaInvWorld(inertiaInv);
            hkVector4 r;
            r.setSub4(input.m_position, body->m_motion.getCenterOfMassInWorld());
            hkVector4 jacAng;
            jacAng.setCross(r, input.m_normal);
            hkVector4 rc;
            rc.setMul3(inertiaInv, jacAng);
            // EXE-INLINED 0x62B0A0
            const __m128 kJacProducts = _mm_mul_ps(rc.m_quad, jacAng.m_quad);
            input.m_objectMassInv = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(kJacProducts, kJacProducts, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kJacProducts, kJacProducts, 0x55), kJacProducts)
            ));
            input.m_objectMassInv = input.m_objectMassInv + body->m_motion.m_inertiaAndMassInv.m_quad[3];
            input.m_objectImpulse = deltaVelocity / input.m_objectMassInv;
            const float maxPushImpulse = -m_characterStrength * stepInfo.m_deltaTime;
            if (input.m_objectImpulse < maxPushImpulse) {
                input.m_objectImpulse = maxPushImpulse;
            }
            output.m_objectImpulse.m_quad = _mm_mul_ps(_mm_set1_ps(input.m_objectImpulse), input.m_normal.m_quad);
        }
        else {
            input.m_objectImpulse = 0.0f;
            input.m_objectMassInv = body->m_motion.m_inertiaAndMassInv.m_quad[3];
        }

        // The character's weight along the normal, while the two are not separating faster than gravity pulls.
        {
            const float deltaTime = stepInfo.m_deltaTime;
            hkVector4 charVelDown;
            charVelDown.m_quad = _mm_mul_ps(_mm_set1_ps(deltaTime), worldGravity.m_quad);
            // EXE-INLINED 0x62B0A0
            const __m128 kDownProducts = _mm_mul_ps(charVelDown.m_quad, input.m_normal.m_quad);
            float relVel = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(kDownProducts, kDownProducts, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(kDownProducts, kDownProducts, 0x55), kDownProducts)
            ));
            if (input.m_projectedVelocity < 0.0f) {
                relVel = relVel - input.m_projectedVelocity;
            }
            if (relVel < -1.1920929e-07f) {
                const float weightImpulse = relVel * m_characterMass;
                output.m_objectImpulse.m_quad = _mm_add_ps(
                    _mm_mul_ps(_mm_set1_ps(weightImpulse), input.m_normal.m_quad), output.m_objectImpulse.m_quad
                );
            }
        }

        for (int32_t i = m_listeners.m_size - 1; i >= 0; --i) {
            if (m_listeners.m_data[i] != nullptr) {
                m_listeners.m_data[i]->objectInteractionCallback(this, input, output);
            }
        }

        if (command != nullptr) {
            interactions->m_collidingBody = body;
            interactions->m_impulsePosition = output.m_impulsePosition;
            interactions->m_objectImpulse = output.m_objectImpulse;
            ++interactions;
        }
        else {
            body->activate();
            hkpMotion* pMotion = &body->m_motion;
            pMotion->applyPointImpulse(output.m_objectImpulse, output.m_impulsePosition);
        }
    }
    if ((command != nullptr) && (interactions - command->m_objectInteraction < maxInteractions)) {
        interactions->m_collidingBody = nullptr;
    }
}

// The SDK's in-place conversion: each hit's distance field holds the cast fraction; it is kept in the position's w
// and replaced by the distance along the normal (fraction times minus the normal's projection of the displacement).
// The points are written through the const pointer, as the SDK's const_cast does.
void hkpCharacterProxy::convertFractionToDistance(
    const hkpRootCdPoint* cHit,
    int32_t numHits,
    const hkVector4& displacement
) {
    hkpRootCdPoint* pHits = const_cast<hkpRootCdPoint*>(cHit);
    for (int32_t i = numHits - 1; i >= 0; --i) {
        float fraction = pHits->m_contact.m_separatingNormal.m_quad[3];
        pHits->m_contact.m_position.m_quad[3] = fraction;
        // EXE-INLINED none: hkVector4::dot3
        __m128 prod = _mm_mul_ps(pHits->m_contact.m_separatingNormal.m_quad, displacement.m_quad);
        float projLength = _mm_cvtss_f32(
            _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod))
        );
        pHits->m_contact.m_separatingNormal.m_quad[3] = fraction * -projLength;
        pHits++;
    }
}

void hkpCharacterProxy::fireContactAdded(const hkpRootCdPoint& point) {
    int32_t i;
    hkpCharacterProxyListener* v4;
    for (i = (int32_t)(m_listeners.m_size - 1); i >= 0; --i) {
        v4 = m_listeners.m_data[i];
        v4->contactPointAddedCallback(this, point);
    }
}

void hkpCharacterProxy::fireContactRemoved(const hkpRootCdPoint& point) {
    int32_t i;
    hkpCharacterProxyListener* v4;
    for (i = (int32_t)(m_listeners.m_size - 1); i >= 0; --i) {
        v4 = m_listeners.m_data[i];
        v4->contactPointRemovedCallback(this, point);
    }
}

void hkpCharacterProxy::fireConstraintsProcessed(
    const hkArray<hkpRootCdPoint>& manifold,
    hkpSimplexSolverInput& input
) {
    int32_t i;
    hkpCharacterProxyListener* v5;
    for (i = (int32_t)(m_listeners.m_size - 1); i >= 0; --i) {
        v5 = m_listeners.m_data[i];
        v5->processConstraintsCallback(this, manifold, input);
    }
}

float hkpCharacterProxy::moveToLinearCastHitPosition(
    const hkpSimplexSolverOutput& output,
    const hkpAllCdPointCollector& castCollector,
    const hkpLinearCastInput& castInput,
    hkVector4& position
) {
    uint32_t v18;
    hkpRootCdPoint* m_data = castCollector.m_hits.m_data;
    __m128 v6 = _mm_mul_ps(output.m_position.m_quad, output.m_position.m_quad);
    float v7 = _mm_shuffle_ps(v6, v6, 85)[0] + v6[0];
    float v8 = _mm_shuffle_ps(v6, v6, 170)[0];
    __m128 v9 = _mm_mul_ps(m_data->m_contact.m_separatingNormal.m_quad, output.m_position.m_quad);
    double v10 = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((v8 + v7))));
    float v19 = (_mm_shuffle_ps(v9, v9, 170)[0] + (_mm_shuffle_ps(v9, v9, 85)[0] + v9[0])) / v10;
    float v20 = m_keepDistance / -v19;
    float v16 = m_data->m_contact.m_position.m_quad[3] - v20 / v10;
    double v11 = 1.0;
    if (v16 < 1.0f) {
        v11 = v16;
    }
    float v17 = v11;
    double v12 = v17;
    if (v17 < 0.0) {
        v12 = 0.0;
    }
    *reinterpret_cast<float*>(&v18) = v12;
    __m128 v13 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v18)), _mm_set_ss(std::bit_cast<float>(v18)), 0);
    position.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v13), position.m_quad), _mm_mul_ps(castInput.m_to.m_quad, v13)
    );
    return output.m_deltaTime * *reinterpret_cast<float*>(&v18);
}
