#include "Havok/Source/Physics/Internal/Collide/BroadPhase/3AxisSweep/hkp3AxisSweep.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkRadixSort.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The 3-axis sweep keeps 16-bit coordinates.
hkpBroadPhase::BroadPhaseType hkp3AxisSweep::getType() const {
    return BROADPHASE_16BIT;
}

void hkp3AxisSweep::getExtents(hkVector4& worldMinOut, hkVector4& worldMaxOut) const {
    hkVector4 v5;
    hkVector4 v6;
    worldMinOut.m_quad = _mm_xor_ps(m_offsetLow.m_quad, hkQuadRealNegateMask);
    __m128 v3 = _mm_shuffle_ps(m_scale.m_quad, _mm_unpackhi_ps(m_scale.m_quad, hkQuadReal1111), 196);
    __m128 v4 = _mm_rcp_ps(v3);
    v5.m_quad = _mm_sub_ps(_mm_add_ps(v4, v4), _mm_mul_ps(_mm_mul_ps(v4, v3), v4));
    worldMaxOut.m_quad = v5.m_quad;
    v6.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x477FFC00u)), _mm_set_ss(std::bit_cast<float>(0x477FFC00u)), 0),
        v5.m_quad
    );
    worldMaxOut.m_quad = v6.m_quad;
    worldMaxOut.m_quad = _mm_add_ps(worldMinOut.m_quad, v6.m_quad);
}

void hkp3AxisSweep::shiftBroadPhase(
    const hkVector4& shiftDistance,
    hkVector4& effectiveShiftDistanceOut,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& newCollisionPairs
) {
    hkVector4 v11;
    hkVector4 v12;
    v11.m_quad = _mm_xor_ps(shiftDistance.m_quad, hkQuadRealNegateMask);
    this->shiftAllObjects(v11, v12, newCollisionPairs);
    __m128 m_quad = v12.m_quad;
    m_offsetLow.m_quad = _mm_add_ps(m_offsetLow.m_quad, v12.m_quad);
    __m128 v7 = _mm_shuffle_ps(m_scale.m_quad, _mm_unpackhi_ps(m_scale.m_quad, hkQuadReal1111), 196);
    __m128 v8 = _mm_rcp_ps(v7);
    m_offsetHigh.m_quad =
        _mm_add_ps(_mm_sub_ps(_mm_add_ps(v8, v8), _mm_mul_ps(_mm_mul_ps(v8, v7), v8)), m_offsetLow.m_quad);
    m_offsetLow32bit.m_quad = _mm_add_ps(m_offsetLow32bit.m_quad, m_quad);
    __m128 v9 = _mm_shuffle_ps(m_scale32bit.m_quad, _mm_unpackhi_ps(m_scale32bit.m_quad, hkQuadReal1111), 196);
    __m128 v10 = _mm_rcp_ps(v9);
    m_offsetHigh32bit.m_quad =
        _mm_add_ps(_mm_sub_ps(_mm_add_ps(v10, v10), _mm_mul_ps(_mm_mul_ps(v10, v9), v10)), m_offsetLow32bit.m_quad);
    effectiveShiftDistanceOut.m_quad = _mm_xor_ps(hkQuadRealNegateMask, m_quad);
}

void sub_D10E80() {
    ;
}

// The determinism check is compiled out of this build.
void hkp3AxisSweep::checkDeterminism() {}

void hkp3AxisSweep::set32BitOffsetAndScale(
    const hkVector4& offsetLow,
    const hkVector4& offsetHigh,
    const hkVector4& scale
) {
    hkVector4 v4;
    uint32_t v6;
    uint32_t v7;
    uint32_t v8;
    v4.m_quad = m_aabb.m_min.m_quad;
    m_offsetLow32bit.m_quad = offsetLow.m_quad;
    m_offsetHigh32bit.m_quad = offsetHigh.m_quad;
    m_scale32bit.m_quad = scale.m_quad;
    __m128 v9 = _mm_sub_ps(m_aabb.m_max.m_quad, v4.m_quad);
    __m128 m_quad = offsetLow.m_quad;
    m_offsetLow.m_quad = offsetLow.m_quad;
    m_offsetHigh.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                _mm_set_ss(std::bit_cast<float>(0x37800200u)), _mm_set_ss(std::bit_cast<float>(0x37800200u)), 0
            ),
            v9
        ),
        m_quad
    );
    *reinterpret_cast<float*>(&v6) = 1.0 / v9[0];
    *reinterpret_cast<float*>(&v7) = 1.0 / v9[1];
    *reinterpret_cast<float*>(&v8) = 1.0 / v9[2];
    m_scale.m_quad = _mm_mul_ps(
        _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v6)), _mm_set_ss(std::bit_cast<float>(v8))),
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(v7)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
            )
        ),
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x477FFC00u)), _mm_set_ss(std::bit_cast<float>(0x477FFC00u)), 0)
    );
    m_scale.m_quad = _mm_shuffle_ps(m_scale.m_quad, _mm_unpackhi_ps(m_scale.m_quad, _mm_setzero_ps()), 196);
    m_offsetLow.m_quad = _mm_shuffle_ps(m_offsetLow.m_quad, _mm_unpackhi_ps(m_offsetLow.m_quad, _mm_setzero_ps()), 196);
    m_offsetHigh.m_quad =
        _mm_shuffle_ps(m_offsetHigh.m_quad, _mm_unpackhi_ps(m_offsetHigh.m_quad, _mm_setzero_ps()), 196);
}

// Two nodes overlap when every interval end of one lies on the right side of the other's: the 16-bit differences
// of the x ends, then the y and z ends read as one 32-bit word each, must all be non-negative.
bool hkp3AxisSweep::areAabbsOverlapping(const hkpBroadPhaseHandle* bhA, const hkpBroadPhaseHandle* bhB) const {
    const hkpBpNode& a = m_nodes.m_data[bhA->m_id];
    const hkpBpNode& b = m_nodes.m_data[bhB->m_id];
    uint16_t xyDiff = static_cast<uint16_t>(b.max_x - a.min_x) | static_cast<uint16_t>(a.max_x - b.min_x) |
        static_cast<uint16_t>(a.max_y - b.min_y) | static_cast<uint16_t>(b.max_y - a.min_y);
    if (xyDiff & 0x8000) {
        return false;
    }
    uint32_t aMin = a.min_y | static_cast<uint32_t>(a.min_z) << 16;
    uint32_t aMax = a.max_y | static_cast<uint32_t>(a.max_z) << 16;
    uint32_t bMin = b.min_y | static_cast<uint32_t>(b.min_z) << 16;
    uint32_t bMax = b.max_y | static_cast<uint32_t>(b.max_z) << 16;
    return !(((aMax - bMin) | (bMax - aMin)) & 0x80008000);
}

// The box is quantized with the 32-bit offsets and scale, then added through the hkAabbUint32 overload, a virtual call.
void hkp3AxisSweep::addObject(
    hkpBroadPhaseHandle* object,
    const hkAabb& aabb,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& pairsOut
) {
    hkAabbUint32 aabbUint32;
    hkVector4Util::convertAabbToUint32(aabb, m_offsetLow32bit, m_offsetHigh32bit, m_scale32bit, aabbUint32);
    addObject(object, aabbUint32, pairsOut);
}

// The first node is the sentinel.
int32_t hkp3AxisSweep::getNumObjects() {
    return m_nodes.m_size - 1;
}

// 24 bytes per node that isn't a marker, plus a 36-byte header.
int32_t hkp3AxisSweep::getAabbCacheSize() const {
    return 24 * (m_nodes.m_size - m_numMarkers) + 36;
}

void hkp3AxisSweep::getAabb(const hkpBroadPhaseHandle* object, hkAabb& aabb) const {
    getAabbFromNode(m_nodes.m_data[object->m_id], aabb);
}

// Reports the node array, the three axes' end points, the marker array and each marker's overlap list. An array
// that doesn't own its storage is reported only when the collector asks for unmanaged memory.
void hkp3AxisSweep::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t iCapacity = m_nodes.m_capacityAndFlags;
    if (iCapacity >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t iAllocated = hkStatisticsCollector::getAllocatedSize(
            m_nodes.m_data,
            (iCapacity & hkArrayBase<hkpBpNode>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpNode))
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "Nodes", m_nodes.m_data,
            static_cast<int32_t>(m_nodes.m_size * sizeof(hkpBpNode)), iAllocated
        );
    }
    for (const hkpBpAxis& rAxis : m_axis) {
        const hkArray<hkpBpEndPoint>& kEndPoints = rAxis.m_endPoints;
        iCapacity = kEndPoints.m_capacityAndFlags;
        if (iCapacity >= 0 || collector->m_includeUnmanagedArrayMemory) {
            int32_t iAllocated = hkStatisticsCollector::getAllocatedSize(
                kEndPoints.m_data,
                (iCapacity & hkArrayBase<hkpBpEndPoint>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpEndPoint))
            );
            collector->addChunk(
                hkStatisticsCollector::MEM_ARRAY, "Axis", kEndPoints.m_data,
                static_cast<int32_t>(kEndPoints.m_size * sizeof(hkpBpEndPoint)), iAllocated
            );
        }
    }
    if (m_numMarkers != 0) {
        collector->addNormalChunk("Markers", m_markers, m_numMarkers * static_cast<int32_t>(sizeof(hkpBpMarker)), 0);
    }
    for (int32_t i = 0; i < m_numMarkers; ++i) {
        const hkArray<uint16_t>& kOverlaps = m_markers[i].m_overlappingObjects;
        iCapacity = kOverlaps.m_capacityAndFlags;
        if (iCapacity >= 0 || collector->m_includeUnmanagedArrayMemory) {
            int32_t iAllocated = hkStatisticsCollector::getAllocatedSize(
                kOverlaps.m_data,
                (iCapacity & hkArrayBase<uint16_t>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(uint16_t))
            );
            collector->addChunk(
                hkStatisticsCollector::MEM_ARRAY, "Markers", kOverlaps.m_data,
                static_cast<int32_t>(kOverlaps.m_size * sizeof(uint16_t)), iAllocated
            );
        }
    }
}

// Sizes allAabbs to the objects (the nodes that are not markers) and writes each one's AABB, in node order.
void hkp3AxisSweep::getAllAabbs(hkArray<hkAabb, hkContainerHeapAllocator>& allAabbs) {
    int32_t numAabbs = m_nodes.m_size - m_numMarkers;
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    if (static_cast<int32_t>(allAabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::CAPACITY_MASK) < numAabbs) {
        hkArrayUtil::_reserve(*pRouter->m_heap, &allAabbs, numAabbs, static_cast<int32_t>(sizeof(hkAabb)));
    }
    allAabbs.m_size = m_nodes.m_size - m_numMarkers;
    int32_t iAabb = 0;
    for (const hkp3AxisSweep::hkpBpNode& node : m_nodes) {
        if (!node.isMarker()) {
            getAabbFromNode(node, allAabbs.m_data[iAabb++]);
        }
    }
}

// Quantizes each object's new AABB (min ends even, max ends odd) and moves its six end points along their axes one
// neighbour at a time: each end point it passes updates that node's index, and a passed end of the other kind
// begins or ends an overlap when the nodes also overlap on the other two axes. On x the overlap goes through the
// marker-aware calls. Among equal values, end points keep node order (the sentinel node 0 stops a max end moving up).
void hkp3AxisSweep::updateAabbs(
    hkpBroadPhaseHandle** objects,
    const hkAabb* aabbs,
    int32_t numObjects,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& addedPairsOut,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& removedPairsOut
) {
    // The object pointer is compared with objects + numObjects as unsigned addresses, not as pointers: as pointers
    // clang folds the first compare to a test of the count.
    const uintptr_t objectsEnd = reinterpret_cast<uintptr_t>(objects) +
        static_cast<uintptr_t>(static_cast<uint32_t>(numObjects)) * sizeof(hkpBroadPhaseHandle*);
    __m128 zero = _mm_setzero_ps();
    for (hkpBroadPhaseHandle** ppObject = objects; reinterpret_cast<uintptr_t>(ppObject) < objectsEnd;
         ++ppObject, ++aabbs) {
        __m128 quantMin = _mm_add_ps(
            _mm_max_ps(
                _mm_min_ps(
                    _mm_mul_ps(_mm_add_ps(aabbs->m_min.m_quad, m_offsetLow.m_quad), m_scale.m_quad),
                    hkp3AxisSweep_quantizationMax
                ),
                zero
            ),
            convertToUint16WithClipMagicQuad
        );
        __m128 quantMax = _mm_add_ps(
            _mm_max_ps(
                _mm_min_ps(
                    _mm_mul_ps(_mm_add_ps(aabbs->m_max.m_quad, m_offsetHigh.m_quad), m_scale.m_quad),
                    hkp3AxisSweep_quantizationMax
                ),
                zero
            ),
            convertToUint16WithClipMagicQuad
        );
        // The quantized value is the float's bits shifted right by 7, cut to 16 bits; a min end clears the low bit,
        // a max end sets it.
        uint32_t minX = std::bit_cast<uint32_t>(quantMin[0]) >> 7 & 0xFFFE;
        uint32_t minY = std::bit_cast<uint32_t>(quantMin[1]) >> 7 & 0xFFFE;
        uint32_t minZ = std::bit_cast<uint32_t>(quantMin[2]) >> 7 & 0xFFFE;
        uint32_t maxX = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[0]) >> 7) | 1;
        uint32_t maxY = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[1]) >> 7) | 1;
        uint32_t maxZ = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[2]) >> 7) | 1;
        hkpBpNode* nodes = m_nodes.m_data;
        uint32_t id = (*ppObject)->m_id;
        hkpBpNode* node = &nodes[id];

        // x: the min end moving down.
        uint32_t pos = node->min_x;
        hkpBpEndPoint* ep = &m_axis[0].m_endPoints.m_data[pos];
        uint32_t value = ep[-1].m_value;
        if (minX < value) {
            do {
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                *ep = ep[-1];
                --ep;
                if (value & 1) {
                    other->max_x = static_cast<uint16_t>(pos);
                    if (!node->yzDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlapCheckMarker(
                            m_markers, *node, static_cast<int32_t>(id), *other, addedPairsOut
                        );
                    }
                }
                else {
                    other->min_x = static_cast<uint16_t>(pos);
                }
                --pos;
                value = ep[-1].m_value;
            }
            while (minX < value);
        }
        while (minX == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            --ep;
            nodes[otherId].min_x = static_cast<uint16_t>(pos);
            --pos;
        }
        ep->m_value = static_cast<uint16_t>(minX);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        node->min_x = static_cast<uint16_t>(pos);

        // x: the max end moving up.
        pos = node->max_x;
        ep = &m_axis[0].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; maxX > value; value = ep[1].m_value) {
            *ep = ep[1];
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_x;
            }
            else {
                --other->min_x;
                if (!node->yzDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlapCheckMarker(
                        m_markers, *node, static_cast<int32_t>(id), *other, addedPairsOut
                    );
                }
            }
        }
        while (maxX == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            if (otherId == 0) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].max_x;
            ++pos;
        }
        // x: the max end moving down.
        for (value = ep[-1].m_value; maxX < value; value = ep[-1].m_value) {
            *ep = ep[-1];
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            --ep;
            --pos;
            if (value & 1) {
                ++other->max_x;
            }
            else {
                ++other->min_x;
                if (!node->yzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlapCheckMarker(
                        m_markers, *node, static_cast<int32_t>(id), *other, removedPairsOut
                    );
                }
            }
        }
        while (maxX == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            ++nodes[otherId].max_x;
            --ep;
            --pos;
        }
        node->max_x = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(maxX);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // x: the min end moving up.
        pos = node->min_x;
        ep = &m_axis[0].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; minX > value; value = ep[1].m_value) {
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            *ep = ep[1];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_x;
                if (!node->yzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlapCheckMarker(
                        m_markers, *node, static_cast<int32_t>(id), *other, removedPairsOut
                    );
                }
            }
            else {
                --other->min_x;
            }
        }
        while (minX == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].min_x;
            ++pos;
        }
        node->min_x = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(minX);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // y: the min end moving down.
        pos = node->min_y;
        ep = &m_axis[1].m_endPoints.m_data[pos];
        for (value = ep[-1].m_value; minY < value; value = ep[-1].m_value) {
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            *ep = ep[-1];
            --ep;
            if (value & 1) {
                other->max_y = static_cast<uint16_t>(pos);
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
            else {
                other->min_y = static_cast<uint16_t>(pos);
            }
            --pos;
        }
        while (minY == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            --ep;
            nodes[otherId].min_y = static_cast<uint16_t>(pos);
            --pos;
        }
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minY);
        node->min_y = static_cast<uint16_t>(pos);

        // y: the max end moving up.
        pos = node->max_y;
        ep = &m_axis[1].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; maxY > value; value = ep[1].m_value) {
            *ep = ep[1];
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_y;
            }
            else {
                --other->min_y;
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
        }
        while (maxY == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            if (otherId == 0) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].max_y;
            ++pos;
        }
        // y: the max end moving down.
        for (value = ep[-1].m_value; maxY < value; value = ep[-1].m_value) {
            *ep = ep[-1];
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            --ep;
            --pos;
            if (value & 1) {
                ++other->max_y;
            }
            else {
                ++other->min_y;
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
        }
        while (maxY == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            ++nodes[otherId].max_y;
            --ep;
            --pos;
        }
        node->max_y = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(maxY);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // y: the min end moving up.
        pos = node->min_y;
        ep = &m_axis[1].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; minY > value; value = ep[1].m_value) {
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            *ep = ep[1];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_y;
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
            else {
                --other->min_y;
            }
        }
        while (minY == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].min_y;
            ++pos;
        }
        node->min_y = static_cast<uint16_t>(pos);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minY);

        // z: the min end moving down.
        pos = node->min_z;
        ep = &m_axis[2].m_endPoints.m_data[pos];
        for (value = ep[-1].m_value; minZ < value; value = ep[-1].m_value) {
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            *ep = ep[-1];
            --ep;
            if (value & 1) {
                other->max_z = static_cast<uint16_t>(pos);
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
            else {
                other->min_z = static_cast<uint16_t>(pos);
            }
            --pos;
        }
        while (minZ == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            --ep;
            nodes[otherId].min_z = static_cast<uint16_t>(pos);
            --pos;
        }
        ep->m_value = static_cast<uint16_t>(minZ);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        node->min_z = static_cast<uint16_t>(pos);

        // z: the max end moving up.
        pos = node->max_z;
        ep = &m_axis[2].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; maxZ > value; value = ep[1].m_value) {
            *ep = ep[1];
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_z;
            }
            else {
                --other->min_z;
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
        }
        while (maxZ == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            if (otherId == 0) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].max_z;
            ++pos;
        }
        // z: the max end moving down.
        for (value = ep[-1].m_value; maxZ < value; value = ep[-1].m_value) {
            *ep = ep[-1];
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            --ep;
            --pos;
            if (value & 1) {
                ++other->max_z;
            }
            else {
                ++other->min_z;
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
        }
        while (maxZ == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            ++nodes[otherId].max_z;
            --ep;
            --pos;
        }
        node->max_z = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(maxZ);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // z: the min end moving up.
        pos = node->min_z;
        ep = &m_axis[2].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; minZ > value; value = ep[1].m_value) {
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            *ep = ep[1];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_z;
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
            else {
                --other->min_z;
            }
        }
        while (minZ == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].min_z;
            ++pos;
        }
        node->min_z = static_cast<uint16_t>(pos);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minZ);
    }
}

// updateAabbs on AABBs already in integer space: each end's quantized value is its integer shifted right by 15, a min
// end with the low bit cleared, a max end rounded up (unless already 0xFFFF) with the low bit set, neither cut to 16
// bits before the compares. The end points then move as in updateAabbs.
void hkp3AxisSweep::updateAabbsUint32(
    hkpBroadPhaseHandle** objects,
    const hkAabbUint32* aabbs,
    int32_t numObjects,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& addedPairsOut,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& removedPairsOut
) {
    // The end compare as in updateAabbs: as addresses, unsigned with 32-bit wrap.
    const uintptr_t objectsEnd = reinterpret_cast<uintptr_t>(objects) +
        static_cast<uintptr_t>(static_cast<uint32_t>(numObjects)) * sizeof(hkpBroadPhaseHandle*);
    for (hkpBroadPhaseHandle** ppObject = objects; reinterpret_cast<uintptr_t>(ppObject) < objectsEnd;
         ++ppObject, ++aabbs) {
        uint32_t maxX = aabbs->m_max[0] >> 15;
        if (maxX != 0xFFFF) {
            ++maxX;
        }
        maxX |= 1;
        uint32_t maxY = aabbs->m_max[1] >> 15;
        if (maxY != 0xFFFF) {
            ++maxY;
        }
        maxY |= 1;
        uint32_t maxZ = aabbs->m_max[2] >> 15;
        if (maxZ != 0xFFFF) {
            ++maxZ;
        }
        maxZ |= 1;
        uint32_t minX = aabbs->m_min[0] >> 15 & 0xFFFE;
        uint32_t minY = aabbs->m_min[1] >> 15 & 0xFFFE;
        uint32_t minZ = aabbs->m_min[2] >> 15 & 0xFFFE;
        hkpBpNode* nodes = m_nodes.m_data;
        uint32_t id = (*ppObject)->m_id;
        hkpBpNode* node = &nodes[id];

        // x: the min end moving down.
        uint32_t pos = node->min_x;
        hkpBpEndPoint* ep = &m_axis[0].m_endPoints.m_data[pos];
        uint32_t value = ep[-1].m_value;
        if (minX < value) {
            do {
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                *ep = ep[-1];
                --ep;
                if (value & 1) {
                    other->max_x = static_cast<uint16_t>(pos);
                    if (!node->yzDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlapCheckMarker(
                            m_markers, *node, static_cast<int32_t>(id), *other, addedPairsOut
                        );
                    }
                }
                else {
                    other->min_x = static_cast<uint16_t>(pos);
                }
                --pos;
                value = ep[-1].m_value;
            }
            while (minX < value);
        }
        while (minX == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            --ep;
            nodes[otherId].min_x = static_cast<uint16_t>(pos);
            --pos;
        }
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minX);
        node->min_x = static_cast<uint16_t>(pos);

        // x: the max end moving up.
        pos = node->max_x;
        ep = &m_axis[0].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; maxX > value; value = ep[1].m_value) {
            *ep = ep[1];
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_x;
            }
            else {
                --other->min_x;
                if (!node->yzDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlapCheckMarker(
                        m_markers, *node, static_cast<int32_t>(id), *other, addedPairsOut
                    );
                }
            }
        }
        while (maxX == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            if (otherId == 0) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].max_x;
            ++pos;
        }
        // x: the max end moving down.
        for (value = ep[-1].m_value; maxX < value; value = ep[-1].m_value) {
            *ep = ep[-1];
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            --ep;
            --pos;
            if (value & 1) {
                ++other->max_x;
            }
            else {
                ++other->min_x;
                if (!node->yzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlapCheckMarker(
                        m_markers, *node, static_cast<int32_t>(id), *other, removedPairsOut
                    );
                }
            }
        }
        while (maxX == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            ++nodes[otherId].max_x;
            --ep;
            --pos;
        }
        node->max_x = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(maxX);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // x: the min end moving up.
        pos = node->min_x;
        ep = &m_axis[0].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; minX > value; value = ep[1].m_value) {
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            *ep = ep[1];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_x;
                if (!node->yzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlapCheckMarker(
                        m_markers, *node, static_cast<int32_t>(id), *other, removedPairsOut
                    );
                }
            }
            else {
                --other->min_x;
            }
        }
        while (minX == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].min_x;
            ++pos;
        }
        node->min_x = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(minX);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // y: the min end moving down.
        pos = node->min_y;
        ep = &m_axis[1].m_endPoints.m_data[pos];
        for (value = ep[-1].m_value; minY < value; value = ep[-1].m_value) {
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            *ep = ep[-1];
            --ep;
            if (value & 1) {
                other->max_y = static_cast<uint16_t>(pos);
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
            else {
                other->min_y = static_cast<uint16_t>(pos);
            }
            --pos;
        }
        while (minY == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            --ep;
            nodes[otherId].min_y = static_cast<uint16_t>(pos);
            --pos;
        }
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minY);
        node->min_y = static_cast<uint16_t>(pos);

        // y: the max end moving up.
        pos = node->max_y;
        ep = &m_axis[1].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; maxY > value; value = ep[1].m_value) {
            *ep = ep[1];
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_y;
            }
            else {
                --other->min_y;
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
        }
        while (maxY == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            if (otherId == 0) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].max_y;
            ++pos;
        }
        // y: the max end moving down.
        for (value = ep[-1].m_value; maxY < value; value = ep[-1].m_value) {
            *ep = ep[-1];
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            --ep;
            --pos;
            if (value & 1) {
                ++other->max_y;
            }
            else {
                ++other->min_y;
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
        }
        while (maxY == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            ++nodes[otherId].max_y;
            --ep;
            --pos;
        }
        node->max_y = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(maxY);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // y: the min end moving up.
        pos = node->min_y;
        ep = &m_axis[1].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; minY > value; value = ep[1].m_value) {
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            *ep = ep[1];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_y;
                if (!node->xzDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
            else {
                --other->min_y;
            }
        }
        while (minY == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].min_y;
            ++pos;
        }
        node->min_y = static_cast<uint16_t>(pos);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minY);

        // z: the min end moving down.
        pos = node->min_z;
        ep = &m_axis[2].m_endPoints.m_data[pos];
        for (value = ep[-1].m_value; minZ < value; value = ep[-1].m_value) {
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            *ep = ep[-1];
            --ep;
            if (value & 1) {
                other->max_z = static_cast<uint16_t>(pos);
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
            else {
                other->min_z = static_cast<uint16_t>(pos);
            }
            --pos;
        }
        while (minZ == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            --ep;
            nodes[otherId].min_z = static_cast<uint16_t>(pos);
            --pos;
        }
        ep->m_value = static_cast<uint16_t>(minZ);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        node->min_z = static_cast<uint16_t>(pos);

        // z: the max end moving up.
        pos = node->max_z;
        ep = &m_axis[2].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; maxZ > value; value = ep[1].m_value) {
            *ep = ep[1];
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_z;
            }
            else {
                --other->min_z;
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::beginOverlap(*node, *other, addedPairsOut);
                }
            }
        }
        while (maxZ == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            if (otherId == 0) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].max_z;
            ++pos;
        }
        // z: the max end moving down.
        for (value = ep[-1].m_value; maxZ < value; value = ep[-1].m_value) {
            *ep = ep[-1];
            hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
            --ep;
            --pos;
            if (value & 1) {
                ++other->max_z;
            }
            else {
                ++other->min_z;
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
        }
        while (maxZ == ep[-1].m_value) {
            uint32_t otherId = ep[-1].m_nodeIndex;
            if (id >= otherId) {
                break;
            }
            *ep = ep[-1];
            ++nodes[otherId].max_z;
            --ep;
            --pos;
        }
        node->max_z = static_cast<uint16_t>(pos);
        ep->m_value = static_cast<uint16_t>(maxZ);
        ep->m_nodeIndex = static_cast<uint16_t>(id);

        // z: the min end moving up.
        pos = node->min_z;
        ep = &m_axis[2].m_endPoints.m_data[pos];
        for (value = ep[1].m_value; minZ > value; value = ep[1].m_value) {
            hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
            *ep = ep[1];
            ++ep;
            ++pos;
            if (value & 1) {
                --other->max_z;
                if (!node->xyDisjoint(*other)) {
                    hkp3AxisSweep::endOverlap(*node, *other, removedPairsOut);
                }
            }
            else {
                --other->min_z;
            }
        }
        while (minZ == ep[1].m_value) {
            uint32_t otherId = ep[1].m_nodeIndex;
            if (id <= otherId) {
                break;
            }
            *ep = ep[1];
            ++ep;
            --nodes[otherId].min_z;
            ++pos;
        }
        node->min_z = static_cast<uint16_t>(pos);
        ep->m_nodeIndex = static_cast<uint16_t>(id);
        ep->m_value = static_cast<uint16_t>(minZ);
    }
}

// Every node overlapping object's node on x (setBitsBasedOnXInterval's bit field, an hkLocalBuffer of
// (numNodes >> 5) + 8 words) and on y and z, markers left out, makes a pair (object, handle) in pairsOut.
void hkp3AxisSweep::reQuerySingleObject(
    const hkpBroadPhaseHandle* object,
    hkArray<hkpBroadPhaseHandlePair>& pairs_out
) const {
    int32_t numNodes = m_nodes.m_size;
    int32_t numWords = (numNodes >> 5) + 8;
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint32_t* pBitField;
    int32_t capacity;
    if (numWords == 0) {
        pBitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        int32_t numBytes = numWords * sizeof(uint32_t);
        pBitField = static_cast<uint32_t*>(pTemp->bufAlloc(numBytes));
        capacity = numBytes / 4;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    const hkpBpNode& queryNode = m_nodes.m_data[object->m_id];
    setBitsBasedOnXInterval(
        numNodes, m_axis[0].m_endPoints.m_data[queryNode.min_x].m_value, queryNode, object->m_id, pBitField
    );

    // m_nodes' data and size are read again after the call.
    const uint32_t* pWordEnd = pBitField + (m_nodes.m_size >> 5) + 1;
    const hkpBpNode* pWordNodes = m_nodes.m_data;
    for (const uint32_t* pWord = pBitField; pWord < pWordEnd; pWord++, pWordNodes += 32) {
        uint32_t bits = *pWord;
        const hkpBpNode* pOther = pWordNodes;
        while (bits) {
            if (!(bits & 0xFF)) {
                pOther += 8;
                bits >>= 8;
                continue;
            }
            if ((bits & 1) && !pOther->yzDisjoint(queryNode)) {
                hkpBroadPhaseHandle* pHandle = pOther->m_handle;
                // A marker's handle word has bit 0 set.
                if (!(reinterpret_cast<uintptr_t>(pHandle) & 1)) {
                    // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
                    hkMemoryRouter& router = hkMemoryRouter::getInstance();
                    if (pairs_out.m_size ==
                        (pairs_out.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*router.m_heap, &pairs_out, sizeof(hkpBroadPhaseHandlePair));
                    }
                    hkpBroadPhaseHandlePair* pPair = pairs_out.m_data + pairs_out.m_size;
                    if (pPair) {
                        // The SDK's pair holds non-const handles; the query object goes in as it is.
                        pPair->m_a = const_cast<hkpBroadPhaseHandle*>(object);
                        pPair->m_b = pHandle;
                    }
                    pairs_out.m_size++;
                }
            }
            pOther++;
            bits >>= 1;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(pBitField, (capacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t));
    }
}

// The new node goes at the end of m_nodes: its quantized ends (min even, max odd; a max below 0xFFFF moves up one) are
// inserted on the three axes, the other nodes are fixed up, then every node it overlaps on x (from
// setBitsBasedOnXInterval's bit field, an hkLocalBuffer of (numNodes >> 5) + 8 words) and on y and z either gets the
// new node in its marker's overlap list (a marker's handle word is its byte offset into m_markers, plus 1) or makes a
// pair (object, handle) in pairsOut. object->m_id is set to nodeIndex.
void hkp3AxisSweep::addObject(
    hkpBroadPhaseHandle* object,
    const hkAabbUint32& aabb,
    hkArray<hkpBroadPhaseHandlePair>& pairsOut
) {
    uint32_t minX = aabb.m_min[0] >> 15;
    uint32_t minY = aabb.m_min[1] >> 15;
    uint32_t minZ = aabb.m_min[2] >> 15;
    uint32_t maxX = aabb.m_max[0] >> 15;
    if (maxX != 0xFFFF) {
        ++maxX;
    }
    maxX |= 1;
    uint32_t maxY = aabb.m_max[1] >> 15;
    if (maxY != 0xFFFF) {
        ++maxY;
    }
    maxY |= 1;
    uint32_t maxZ = aabb.m_max[2] >> 15;
    if (maxZ != 0xFFFF) {
        ++maxZ;
    }
    maxZ |= 1;
    uint32_t nodeIndex = m_nodes.m_size;
    minZ &= 0xFFFE;
    minX &= 0xFFFE;
    minY &= 0xFFFE;

    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode>::expandOne
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (m_nodes.m_size == (m_nodes.m_capacityAndFlags & hkArrayBase<hkpBpNode>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*router.m_heap, &m_nodes, sizeof(hkpBpNode));
    }
    hkpBpNode& node = m_nodes.m_data[m_nodes.m_size];
    m_nodes.m_size++;
    hkpBpNode* pNodes = m_nodes.m_data;
    m_axis[0].insert(pNodes, nodeIndex, minX, maxX, node.min_x, node.max_x);
    m_axis[1].insert(pNodes, nodeIndex, minY, maxY, node.min_y, node.max_y);
    m_axis[2].insert(pNodes, nodeIndex, minZ, maxZ, node.min_z, node.max_z);
    updateNodesAfterInsert(pNodes, nodeIndex, node);
    node.m_handle = object;
    object->m_id = nodeIndex;

    int32_t numNodes = m_nodes.m_size;
    int32_t numWords = (numNodes >> 5) + 8;
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint32_t* pBitField;
    int32_t capacity;
    if (numWords == 0) {
        pBitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        int32_t numBytes = numWords * sizeof(uint32_t);
        pBitField = static_cast<uint32_t*>(pTemp->bufAlloc(numBytes));
        capacity = numBytes / 4;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    setBitsBasedOnXInterval(static_cast<uint32_t>(numNodes), minX, node, nodeIndex, pBitField);

    // m_nodes' data and size are read again after the call.
    hkpBpNode* pWordNodes = m_nodes.m_data;
    const uint32_t* pWordEnd = pBitField + (m_nodes.m_size >> 5) + 1;
    for (const uint32_t* pWord = pBitField; pWord < pWordEnd; pWord++, pWordNodes += 32) {
        uint32_t bits = *pWord;
        hkpBpNode* pOther = pWordNodes;
        while (bits) {
            if (!(bits & 0xFF)) {
                pOther += 8;
                bits >>= 8;
                continue;
            }
            if (bits & 1) {
                uint32_t otherMin = pOther->min_y | static_cast<uint32_t>(pOther->min_z) << 16;
                uint32_t otherMax = pOther->max_y | static_cast<uint32_t>(pOther->max_z) << 16;
                uint32_t newMin = node.min_y | static_cast<uint32_t>(node.min_z) << 16;
                uint32_t newMax = node.max_y | static_cast<uint32_t>(node.max_z) << 16;
                if (!(((otherMax - newMin) | (newMax - otherMin)) & 0x80008000)) {
                    // A handle word is untyped here: with bit 0 set it is the SDK's hkAddByteOffset into m_markers.
                    uintptr_t handle = reinterpret_cast<uintptr_t>(pOther->m_handle);
                    if (handle & 1) {
                        hkpBpMarker* pMarker = reinterpret_cast<hkpBpMarker*>(
                            reinterpret_cast<char*>(m_markers) + (handle & ~uintptr_t(1))
                        );
                        hkArray<uint16_t>& overlaps = pMarker->m_overlappingObjects;
                        overlaps.pushBack(static_cast<uint16_t>(nodeIndex));
                    }
                    else {
                        // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
                        hkMemoryRouter& pairRouter = hkMemoryRouter::getInstance();
                        if (pairsOut.m_size == (pairsOut.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*pairRouter.m_heap, &pairsOut, sizeof(hkpBroadPhaseHandlePair));
                        }
                        hkpBroadPhaseHandlePair* pPair = pairsOut.m_data + pairsOut.m_size;
                        if (pPair) {
                            pPair->m_a = object;
                            pPair->m_b = pOther->m_handle;
                        }
                        pairsOut.m_size++;
                    }
                }
            }
            pOther++;
            bits >>= 1;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(pBitField, (capacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t));
    }
}

// Every node overlapping object's node on x (setBitsBasedOnXInterval's bit field, an hkLocalBuffer of
// (numNodes >> 5) + 8 words) and on y and z ends its overlap: a marker drops the node's index from its overlap list,
// any other node makes a pair (object, handle) in pairsOut. The node's ends leave the three axes, then the last node
// moves into its slot (its end points, its marker or handle, and the markers listing it take the new index) and
// m_nodes shrinks by one.
void hkp3AxisSweep::removeObject(hkpBroadPhaseHandle* object, hkArray<hkpBroadPhaseHandlePair>& delPairsOut) {
    uint32_t nodeIndex = object->m_id;
    hkpBpNode& node = m_nodes.m_data[nodeIndex];
    int32_t numNodes = m_nodes.m_size;
    // The size is shifted unsigned here.
    int32_t numWords = static_cast<int32_t>((static_cast<uint32_t>(numNodes) >> 5) + 8);
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint32_t* pBitField;
    int32_t capacity;
    if (numWords == 0) {
        pBitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        int32_t numBytes = numWords * sizeof(uint32_t);
        pBitField = static_cast<uint32_t*>(pTemp->bufAlloc(numBytes));
        capacity = numBytes / 4;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    setBitsBasedOnXInterval(numNodes, m_axis[0].m_endPoints.m_data[node.min_x].m_value, node, nodeIndex, pBitField);

    // m_nodes' data and size are read again after the call.
    const hkpBpNode* pWordNodes = m_nodes.m_data;
    const uint32_t* pWordEnd = pBitField + (m_nodes.m_size >> 5) + 1;
    for (const uint32_t* pWord = pBitField; pWord < pWordEnd; pWord++, pWordNodes += 32) {
        uint32_t bits = *pWord;
        const hkpBpNode* pOther = pWordNodes;
        while (bits) {
            if (!(bits & 0xFF)) {
                pOther += 8;
                bits >>= 8;
                continue;
            }
            if ((bits & 1) && !pOther->yzDisjoint(node)) {
                hkpBroadPhaseHandle* pHandle = pOther->m_handle;
                // A handle word is untyped here: with bit 0 set it is the SDK's hkAddByteOffset into m_markers.
                uintptr_t handle = reinterpret_cast<uintptr_t>(pHandle);
                if (handle & 1) {
                    hkpBpMarker* pMarker =
                        reinterpret_cast<hkpBpMarker*>(reinterpret_cast<char*>(m_markers) + (handle & ~uintptr_t(1)));
                    hkArray<uint16_t>& overlaps = pMarker->m_overlappingObjects;
                    int32_t index = overlaps.indexOf(static_cast<uint16_t>(nodeIndex));
                    overlaps.removeAt(index);
                }
                else {
                    // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
                    hkMemoryRouter& router = hkMemoryRouter::getInstance();
                    if (delPairsOut.m_size ==
                        (delPairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*router.m_heap, &delPairsOut, sizeof(hkpBroadPhaseHandlePair));
                    }
                    hkpBroadPhaseHandlePair* pPair = delPairsOut.m_data + delPairsOut.m_size;
                    if (pPair) {
                        pPair->m_a = object;
                        pPair->m_b = pHandle;
                    }
                    delPairsOut.m_size++;
                }
            }
            pOther++;
            bits >>= 1;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(pBitField, (capacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t));
    }

    hkpBpNode* pNodes = m_nodes.m_data;
    m_axis[0].remove(node.min_x, node.max_x);
    m_axis[1].remove(node.min_y, node.max_y);
    m_axis[2].remove(node.min_z, node.max_z);
    updateNodesAfterDelete(pNodes, numNodes, node);

    if (nodeIndex < static_cast<uint32_t>(numNodes - 1)) {
        node = m_nodes.m_data[numNodes - 1];
        m_axis[0].m_endPoints.m_data[node.min_x].m_nodeIndex = static_cast<uint16_t>(nodeIndex);
        m_axis[0].m_endPoints.m_data[node.max_x].m_nodeIndex = static_cast<uint16_t>(nodeIndex);
        uintptr_t handle = reinterpret_cast<uintptr_t>(node.m_handle);
        if (handle & 1) {
            // A marker's handle word is its byte offset into m_markers, plus 1.
            hkpBpMarker* pMarker =
                reinterpret_cast<hkpBpMarker*>(reinterpret_cast<char*>(m_markers) + (handle & ~uintptr_t(1)));
            pMarker->m_nodeIndex = static_cast<uint16_t>(nodeIndex);
        }
        else {
            m_axis[1].m_endPoints.m_data[node.min_y].m_nodeIndex = static_cast<uint16_t>(nodeIndex);
            m_axis[1].m_endPoints.m_data[node.max_y].m_nodeIndex = static_cast<uint16_t>(nodeIndex);
            m_axis[2].m_endPoints.m_data[node.min_z].m_nodeIndex = static_cast<uint16_t>(nodeIndex);
            m_axis[2].m_endPoints.m_data[node.max_z].m_nodeIndex = static_cast<uint16_t>(nodeIndex);
            node.m_handle->m_id = nodeIndex;
        }

        // The markers whose x range the moved node spans list it under its old index (numNodes - 1).
        if (m_numMarkers && !node.isMarker()) {
            uint16_t minX = node.min_x;
            const hkpBpEndPoint* pXPoints = m_axis[0].m_endPoints.m_data;
            // Only the low 16 bits of the shift are used.
            uint32_t shift = static_cast<uint16_t>(16 - m_ld2NumMarkers);
            int32_t startMarker = static_cast<int32_t>(static_cast<uint32_t>(pXPoints[minX].m_value) >> shift);
            if (startMarker > 0 && m_nodes.m_data[m_markers[startMarker - 1].m_nodeIndex].max_x > minX) {
                startMarker--;
            }
            int32_t endMarker = static_cast<int32_t>(static_cast<uint32_t>(pXPoints[node.max_x].m_value) >> shift) - 1;
            uint16_t oldIndex = static_cast<uint16_t>(numNodes - 1);
            for (int32_t m = startMarker; m <= endMarker; m++) {
                hkArray<uint16_t>& overlaps = m_markers[m].m_overlappingObjects;
                int32_t index = overlaps.indexOf(oldIndex);
                overlaps.m_data[index] = static_cast<uint16_t>(nodeIndex);
            }
        }
        _fixDeterministicOrderAfterNodeIdWasDecreased(static_cast<int32_t>(nodeIndex));
    }

    m_nodes.setSize(numNodes - 1);
}

// The two open lists are hkInplaceArrays of 256 node indices. The overlap test in y and z reads each node's packed
// (min_y, min_z) and (max_y, max_z) pairs as one word each and subtracts them, a borrow sign in either half meaning no
// overlap. A marker node's m_handle is no handle but its byte offset into m_markers with the low bit set, added to
// m_markers as raw memory. The overlap handling is written out twice, as the game does.
void hkp3AxisSweep::queryBatchAabbSub(
    const uint32_t* bitFieldOfQueryNodes,
    hkArray<hkpBroadPhaseHandlePair>& pairsOut,
    hkBool addMode
) {
    hkInplaceArray<uint16_t, 256> openQueryNodes;
    hkInplaceArray<uint16_t, 256> openOtherNodes;

    const hkpBpEndPoint* ep = m_axis[0].m_endPoints.m_data + 1;
    const hkpBpEndPoint* epEnd = m_axis[0].m_endPoints.m_data + m_axis[0].m_endPoints.m_size - 1;
    for (; ep < epEnd; ++ep) {
        const int32_t nodeIndex = ep->m_nodeIndex;
        const uint32_t isQuery = bitFieldOfQueryNodes[nodeIndex >> 5] & (1u << (nodeIndex & 0x1F));
        if (ep->m_value & 1) {
            // A max end point closes the node's interval.
            hkArray<uint16_t>& open = isQuery ? openQueryNodes : openOtherNodes;
            const int32_t size = open.m_size;
            int32_t index = 0;
            for (; index < size; ++index) {
                if (open.m_data[index] == static_cast<uint16_t>(nodeIndex)) {
                    break;
                }
            }
            if (index >= size) {
                index = -1;
            }
            open.m_size = static_cast<uint32_t>(size - 1);
            if (size - 1 != index) {
                open.m_data[index] = open.m_data[size - 1];
            }
            continue;
        }

        const hkpBpNode* node = &m_nodes.m_data[nodeIndex];
        for (uint16_t otherIndex : openQueryNodes) {
            const hkpBpNode* nodes = m_nodes.m_data;
            const hkpBpNode* other = &nodes[otherIndex];
            uint32_t nodeMin;
            uint32_t nodeMax;
            uint32_t otherMin;
            uint32_t otherMax;
            __builtin_memcpy(&nodeMin, &node->min_y, sizeof(uint32_t));
            __builtin_memcpy(&nodeMax, &node->max_y, sizeof(uint32_t));
            __builtin_memcpy(&otherMin, &other->min_y, sizeof(uint32_t));
            __builtin_memcpy(&otherMax, &other->max_y, sizeof(uint32_t));
            if (((otherMax - nodeMin) | (nodeMax - otherMin)) & 0x80008000u) {
                continue;
            }
            const hkpBpNode* markerNode;
            const hkpBpNode* objectNode;
            if (reinterpret_cast<uintptr_t>(node->m_handle) & 1) {
                markerNode = node;
                objectNode = other;
            }
            else if (reinterpret_cast<uintptr_t>(other->m_handle) & 1) {
                markerNode = other;
                objectNode = node;
            }
            else {
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (pairsOut.m_size ==
                    (pairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(heap, &pairsOut, sizeof(hkpBroadPhaseHandlePair));
                }
                hkpBroadPhaseHandlePair& pair = pairsOut.m_data[pairsOut.m_size];
                ++pairsOut.m_size;
                pair.m_a = other->m_handle;
                pair.m_b = node->m_handle;
                continue;
            }
            const int32_t objectIndex = static_cast<int32_t>(objectNode - nodes);
            hkpBpMarker* marker = reinterpret_cast<hkpBpMarker*>(
                reinterpret_cast<uint8_t*>(m_markers) +
                (reinterpret_cast<uintptr_t>(markerNode->m_handle) & 0xFFFFFFFEu)
            );
            hkArray<uint16_t>& overlaps = marker->m_overlappingObjects;
            if (addMode.m_bool) {
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (overlaps.m_size == (overlaps.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(heap, &overlaps, sizeof(uint16_t));
                }
                overlaps.m_data[overlaps.m_size] = static_cast<uint16_t>(objectIndex);
                ++overlaps.m_size;
            }
            else {
                const int32_t size = overlaps.m_size;
                int32_t index = 0;
                for (; index < size; ++index) {
                    if (overlaps.m_data[index] == static_cast<uint16_t>(objectIndex)) {
                        break;
                    }
                }
                if (index >= size) {
                    index = -1;
                }
                --overlaps.m_size;
                const int32_t last = overlaps.m_size;
                if (last != index) {
                    overlaps.m_data[index] = overlaps.m_data[last];
                }
            }
        }

        if (!isQuery) {
            hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
            if (openOtherNodes.m_size == (openOtherNodes.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(heap, &openOtherNodes, sizeof(uint16_t));
            }
            openOtherNodes.m_data[openOtherNodes.m_size] = static_cast<uint16_t>(nodeIndex);
            ++openOtherNodes.m_size;
            continue;
        }

        for (uint16_t otherIndex : openOtherNodes) {
            const hkpBpNode* nodes = m_nodes.m_data;
            const hkpBpNode* other = &nodes[otherIndex];
            uint32_t nodeMin;
            uint32_t nodeMax;
            uint32_t otherMin;
            uint32_t otherMax;
            __builtin_memcpy(&nodeMin, &node->min_y, sizeof(uint32_t));
            __builtin_memcpy(&nodeMax, &node->max_y, sizeof(uint32_t));
            __builtin_memcpy(&otherMin, &other->min_y, sizeof(uint32_t));
            __builtin_memcpy(&otherMax, &other->max_y, sizeof(uint32_t));
            if (((otherMax - nodeMin) | (nodeMax - otherMin)) & 0x80008000u) {
                continue;
            }
            const hkpBpNode* markerNode;
            const hkpBpNode* objectNode;
            if (reinterpret_cast<uintptr_t>(node->m_handle) & 1) {
                markerNode = node;
                objectNode = other;
            }
            else if (reinterpret_cast<uintptr_t>(other->m_handle) & 1) {
                markerNode = other;
                objectNode = node;
            }
            else {
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (pairsOut.m_size ==
                    (pairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(heap, &pairsOut, sizeof(hkpBroadPhaseHandlePair));
                }
                hkpBroadPhaseHandlePair& pair = pairsOut.m_data[pairsOut.m_size];
                ++pairsOut.m_size;
                pair.m_a = other->m_handle;
                pair.m_b = node->m_handle;
                continue;
            }
            const int32_t objectIndex = static_cast<int32_t>(objectNode - nodes);
            hkpBpMarker* marker = reinterpret_cast<hkpBpMarker*>(
                reinterpret_cast<uint8_t*>(m_markers) +
                (reinterpret_cast<uintptr_t>(markerNode->m_handle) & 0xFFFFFFFEu)
            );
            hkArray<uint16_t>& overlaps = marker->m_overlappingObjects;
            if (addMode.m_bool) {
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (overlaps.m_size == (overlaps.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(heap, &overlaps, sizeof(uint16_t));
                }
                overlaps.m_data[overlaps.m_size] = static_cast<uint16_t>(objectIndex);
                ++overlaps.m_size;
            }
            else {
                const int32_t size = overlaps.m_size;
                int32_t index = 0;
                for (; index < size; ++index) {
                    if (overlaps.m_data[index] == static_cast<uint16_t>(objectIndex)) {
                        break;
                    }
                }
                if (index >= size) {
                    index = -1;
                }
                --overlaps.m_size;
                const int32_t last = overlaps.m_size;
                if (last != index) {
                    overlaps.m_data[index] = overlaps.m_data[last];
                }
            }
        }

        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        if (openQueryNodes.m_size == (openQueryNodes.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(heap, &openQueryNodes, sizeof(uint16_t));
        }
        openQueryNodes.m_data[openQueryNodes.m_size] = static_cast<uint16_t>(nodeIndex);
        ++openQueryNodes.m_size;
    }
}

// The query box is quantized like a node; the nodes whose x interval overlaps it are flagged in a bit field (one bit
// per node, from a marker's overlap list when the box starts past a marker), its ends are looked up on the y and z
// axes, and every flagged node that is not a marker and overlaps it on y and z too goes into pairsOut with a null m_a.
// The bit field is an hkLocalBuffer of (numNodes >> 5) + 8 words from the temp allocator, cleared 16 bytes at a time.
// The quantized y and z ends are stored as 16-bit halves and read back as 32-bit words.
void hkp3AxisSweep::querySingleAabb(const hkAabb& aabbIn, hkArray<hkpBroadPhaseHandlePair>& pairs_out) const {
    int32_t numNodes = m_nodes.m_size;
    int32_t numWords = (numNodes >> 5) + 8;
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint32_t* pBitField;
    int32_t capacity;
    if (numWords == 0) {
        pBitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        // The size that bufAlloc updates is 16-aligned.
        alignas(16) int32_t numBytes = numWords * sizeof(uint32_t);
        pBitField = static_cast<uint32_t*>(pTemp->bufAlloc(numBytes));
        capacity = numBytes / 4;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    __m128 zero = _mm_setzero_ps();
    uint32_t* pClear = pBitField;
    for (int32_t i = numNodes >> 7; i >= 0; i--) {
        __builtin_memset(pClear, 0, sizeof(uint32_t) * 4);
        pClear += 4;
    }

    // EXE-INLINED none: hkp3AxisSweep::convertAabbToInt
    __m128 scale = _mm_load_ps(&reinterpret_cast<const float*>(&m_scale.m_quad)[0]);
    __m128 quantMax = _mm_load_ps(&reinterpret_cast<float*>(&hkp3AxisSweep_quantizationMax)[0]);
    __m128 quantOffset = convertToUint16WithClipMagicQuad;
    alignas(16) float minQuant[4];
    alignas(16) float maxQuant[4];
    __m128 lo = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&m_offsetLow.m_quad)[0]),
        _mm_load_ps(&reinterpret_cast<const float*>(&aabbIn.m_min.m_quad)[0])
    );
    lo = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(lo, scale), quantMax), zero), quantOffset);
    _mm_store_ps(minQuant, lo);
    __m128 hi = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&m_offsetHigh.m_quad)[0]),
        _mm_load_ps(&reinterpret_cast<const float*>(&aabbIn.m_max.m_quad)[0])
    );
    hi = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(hi, scale), quantMax), zero), quantOffset);
    _mm_store_ps(maxQuant, hi);
    // The floats' bits, shifted right by 7: min ends are even, max ends odd.
    uint32_t minX = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[0]) >> 7) & 0xFFFE;
    uint32_t minY = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[1]) >> 7) & 0xFFFE;
    uint32_t minZ = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[2]) >> 7) & 0xFFFE;
    uint32_t maxX = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[0]) >> 7) | 1;
    uint32_t maxY = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[1]) >> 7) | 1;
    uint32_t maxZ = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[2]) >> 7) | 1;

    // The x axis: past the sentinel, or from the marker before minX, toggle every node that starts before minX
    // (its end toggles it back), then flag every node that starts inside [minX, maxX).
    const hkpBpEndPoint* pEp = m_axis[0].m_endPoints.m_data + 1;
    if (m_numMarkers != 0) {
        int32_t markerIndex = static_cast<int32_t>(minX) >> (16 - m_ld2NumMarkers);
        if (markerIndex > 0) {
            const hkpBpMarker& marker = m_markers[markerIndex - 1];
            pBitField[marker.m_nodeIndex >> 5] ^= 1u << (marker.m_nodeIndex & 0x1F);
            const uint16_t* pOverlap = marker.m_overlappingObjects.m_data;
            for (int32_t i = marker.m_overlappingObjects.m_size - 1; i >= 0; i--) {
                uint16_t nodeIndex = *pOverlap++;
                pBitField[nodeIndex >> 5] ^= 1u << (nodeIndex & 0x1F);
            }
            // The marker's own interval: the nodes that start inside it are not in its list.
            const hkpBpNode& markerNode = m_nodes.m_data[marker.m_nodeIndex];
            const hkpBpEndPoint* pEnd = m_axis[0].m_endPoints.m_data + markerNode.max_x;
            for (const hkpBpEndPoint* p = m_axis[0].m_endPoints.m_data + markerNode.min_x + 1; p < pEnd; p++) {
                if (!(p->m_value & 1)) {
                    pBitField[p->m_nodeIndex >> 5] &= ~(1u << (p->m_nodeIndex & 0x1F));
                }
            }
            pEp = m_axis[0].m_endPoints.m_data + markerNode.min_x + 1;
        }
    }
    while (pEp->m_value < minX) {
        pBitField[pEp->m_nodeIndex >> 5] ^= 1u << (pEp->m_nodeIndex & 0x1F);
        pEp++;
    }
    while (pEp->m_value < maxX) {
        if (!(pEp->m_value & 1)) {
            pBitField[pEp->m_nodeIndex >> 5] ^= 1u << (pEp->m_nodeIndex & 0x1F);
        }
        pEp++;
    }

    // The y and z axes: the query's end point indices, min in the low half, max in the high half of each word.
    const hkpBpAxis& axisY = m_axis[1];
    const hkpBpEndPoint* pYBegin = axisY.m_endPoints.m_data;
    const hkpBpEndPoint* pYLast = pYBegin + axisY.m_endPoints.m_size - 2;
    uint16_t queryMinY = static_cast<uint16_t>(axisY.find(pYBegin + 1, pYLast, static_cast<uint16_t>(minY)) - pYBegin);
    uint16_t queryMaxY =
        static_cast<uint16_t>(axisY.find(pYBegin + 1, pYLast, static_cast<uint16_t>(maxY)) - pYBegin - 1);
    const hkpBpAxis& axisZ = m_axis[2];
    const hkpBpEndPoint* pZBegin = axisZ.m_endPoints.m_data;
    const hkpBpEndPoint* pZLast = pZBegin + axisZ.m_endPoints.m_size - 2;
    uint16_t queryMinZ = static_cast<uint16_t>(axisZ.find(pZBegin + 1, pZLast, static_cast<uint16_t>(minZ)) - pZBegin);
    uint16_t queryMaxZ =
        static_cast<uint16_t>(axisZ.find(pZBegin + 1, pZLast, static_cast<uint16_t>(maxZ)) - pZBegin - 1);
    uint32_t queryMin = queryMinY | static_cast<uint32_t>(queryMinZ) << 16;
    uint32_t queryMax = queryMaxY | static_cast<uint32_t>(queryMaxZ) << 16;

    const uint32_t* pWordEnd = pBitField + (m_nodes.m_size >> 5) + 1;
    const hkpBpNode* pWordNodes = m_nodes.m_data;
    for (const uint32_t* pWord = pBitField; pWord < pWordEnd; pWord++, pWordNodes += 32) {
        uint32_t bits = *pWord;
        const hkpBpNode* pNodes = pWordNodes;
        while (bits) {
            if (bits & 0xF) {
                for (int32_t j = 0; j < 4; j++) {
                    if (!(bits & (1u << j))) {
                        continue;
                    }
                    const hkpBpNode& node = pNodes[j];
                    uint32_t nodeMin = node.min_y | static_cast<uint32_t>(node.min_z) << 16;
                    uint32_t nodeMax = node.max_y | static_cast<uint32_t>(node.max_z) << 16;
                    if (((queryMax - nodeMin) | (nodeMax - queryMin)) & 0x80008000) {
                        continue;
                    }
                    hkpBroadPhaseHandle* pHandle = node.m_handle;
                    // A marker's handle word has bit 0 set.
                    if (reinterpret_cast<uintptr_t>(pHandle) & 1) {
                        continue;
                    }
                    // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
                    hkMemoryRouter& router = hkMemoryRouter::getInstance();
                    if (pairs_out.m_size == (pairs_out.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*router.m_heap, &pairs_out, sizeof(hkpBroadPhaseHandlePair));
                    }
                    hkpBroadPhaseHandlePair* pPair = pairs_out.m_data + pairs_out.m_size;
                    if (pPair) {
                        pPair->m_a = nullptr;
                        pPair->m_b = pHandle;
                    }
                    pairs_out.m_size++;
                }
            }
            bits >>= 4;
            pNodes += 4;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(pBitField, (capacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t));
    }
}

// querySingleAabb's search with every overlapping node's handle passed to collector, as cast 0 (the returned fraction
// is dropped), in place of the pair list.
void hkp3AxisSweep::querySingleAabbWithCollector(const hkAabb& aabbIn, hkpBroadPhaseCastCollector* collector) const {
    int32_t numNodes = m_nodes.m_size;
    int32_t numWords = (numNodes >> 5) + 8;
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint32_t* pBitField;
    int32_t capacity;
    if (numWords == 0) {
        pBitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        // The size that bufAlloc updates is 16-aligned.
        alignas(16) int32_t numBytes = numWords * sizeof(uint32_t);
        pBitField = static_cast<uint32_t*>(pTemp->bufAlloc(numBytes));
        capacity = numBytes / 4;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    __m128 zero = _mm_setzero_ps();
    uint32_t* pClear = pBitField;
    for (int32_t i = numNodes >> 7; i >= 0; i--) {
        __builtin_memset(pClear, 0, sizeof(uint32_t) * 4);
        pClear += 4;
    }

    // EXE-INLINED none: hkp3AxisSweep::convertAabbToInt
    __m128 scale = _mm_load_ps(&reinterpret_cast<const float*>(&m_scale.m_quad)[0]);
    __m128 quantMax = _mm_load_ps(&reinterpret_cast<float*>(&hkp3AxisSweep_quantizationMax)[0]);
    __m128 quantOffset = convertToUint16WithClipMagicQuad;
    alignas(16) float minQuant[4];
    alignas(16) float maxQuant[4];
    __m128 lo = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&m_offsetLow.m_quad)[0]),
        _mm_load_ps(&reinterpret_cast<const float*>(&aabbIn.m_min.m_quad)[0])
    );
    lo = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(lo, scale), quantMax), zero), quantOffset);
    _mm_store_ps(minQuant, lo);
    __m128 hi = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&m_offsetHigh.m_quad)[0]),
        _mm_load_ps(&reinterpret_cast<const float*>(&aabbIn.m_max.m_quad)[0])
    );
    hi = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(hi, scale), quantMax), zero), quantOffset);
    _mm_store_ps(maxQuant, hi);
    // The floats' bits, shifted right by 7: min ends are even, max ends odd.
    uint32_t minX = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[0]) >> 7) & 0xFFFE;
    uint32_t minY = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[1]) >> 7) & 0xFFFE;
    uint32_t minZ = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[2]) >> 7) & 0xFFFE;
    uint32_t maxX = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[0]) >> 7) | 1;
    uint32_t maxY = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[1]) >> 7) | 1;
    uint32_t maxZ = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[2]) >> 7) | 1;

    // The x axis, as in querySingleAabb.
    const hkpBpEndPoint* pEp = m_axis[0].m_endPoints.m_data + 1;
    if (m_numMarkers != 0) {
        int32_t markerIndex = static_cast<int32_t>(minX) >> (16 - m_ld2NumMarkers);
        if (markerIndex > 0) {
            const hkpBpMarker& marker = m_markers[markerIndex - 1];
            pBitField[marker.m_nodeIndex >> 5] ^= 1u << (marker.m_nodeIndex & 0x1F);
            const uint16_t* pOverlap = marker.m_overlappingObjects.m_data;
            for (int32_t i = marker.m_overlappingObjects.m_size - 1; i >= 0; i--) {
                uint16_t nodeIndex = *pOverlap++;
                pBitField[nodeIndex >> 5] ^= 1u << (nodeIndex & 0x1F);
            }
            const hkpBpNode& markerNode = m_nodes.m_data[marker.m_nodeIndex];
            const hkpBpEndPoint* pEnd = m_axis[0].m_endPoints.m_data + markerNode.max_x;
            for (const hkpBpEndPoint* p = m_axis[0].m_endPoints.m_data + markerNode.min_x + 1; p < pEnd; p++) {
                if (!(p->m_value & 1)) {
                    pBitField[p->m_nodeIndex >> 5] &= ~(1u << (p->m_nodeIndex & 0x1F));
                }
            }
            pEp = m_axis[0].m_endPoints.m_data + markerNode.min_x + 1;
        }
    }
    while (pEp->m_value < minX) {
        pBitField[pEp->m_nodeIndex >> 5] ^= 1u << (pEp->m_nodeIndex & 0x1F);
        pEp++;
    }
    while (pEp->m_value < maxX) {
        if (!(pEp->m_value & 1)) {
            pBitField[pEp->m_nodeIndex >> 5] ^= 1u << (pEp->m_nodeIndex & 0x1F);
        }
        pEp++;
    }

    // The y and z axes, as in querySingleAabb.
    const hkpBpAxis& axisY = m_axis[1];
    const hkpBpEndPoint* pYBegin = axisY.m_endPoints.m_data;
    const hkpBpEndPoint* pYLast = pYBegin + axisY.m_endPoints.m_size - 2;
    uint16_t queryMinY = static_cast<uint16_t>(axisY.find(pYBegin + 1, pYLast, static_cast<uint16_t>(minY)) - pYBegin);
    uint16_t queryMaxY =
        static_cast<uint16_t>(axisY.find(pYBegin + 1, pYLast, static_cast<uint16_t>(maxY)) - pYBegin - 1);
    const hkpBpAxis& axisZ = m_axis[2];
    const hkpBpEndPoint* pZBegin = axisZ.m_endPoints.m_data;
    const hkpBpEndPoint* pZLast = pZBegin + axisZ.m_endPoints.m_size - 2;
    uint16_t queryMinZ = static_cast<uint16_t>(axisZ.find(pZBegin + 1, pZLast, static_cast<uint16_t>(minZ)) - pZBegin);
    uint16_t queryMaxZ =
        static_cast<uint16_t>(axisZ.find(pZBegin + 1, pZLast, static_cast<uint16_t>(maxZ)) - pZBegin - 1);
    uint32_t queryMin = queryMinY | static_cast<uint32_t>(queryMinZ) << 16;
    uint32_t queryMax = queryMaxY | static_cast<uint32_t>(queryMaxZ) << 16;

    const uint32_t* pWordEnd = pBitField + (m_nodes.m_size >> 5) + 1;
    const hkpBpNode* pWordNodes = m_nodes.m_data;
    for (const uint32_t* pWord = pBitField; pWord < pWordEnd; pWord++, pWordNodes += 32) {
        uint32_t bits = *pWord;
        const hkpBpNode* pNodes = pWordNodes;
        while (bits) {
            if (bits & 0xF) {
                for (int32_t j = 0; j < 4; j++) {
                    if (!(bits & (1u << j))) {
                        continue;
                    }
                    const hkpBpNode& node = pNodes[j];
                    uint32_t nodeMin = node.min_y | static_cast<uint32_t>(node.min_z) << 16;
                    uint32_t nodeMax = node.max_y | static_cast<uint32_t>(node.max_z) << 16;
                    if (((queryMax - nodeMin) | (nodeMax - queryMin)) & 0x80008000) {
                        continue;
                    }
                    hkpBroadPhaseHandle* pHandle = node.m_handle;
                    // A marker's handle word has bit 0 set.
                    if (reinterpret_cast<uintptr_t>(pHandle) & 1) {
                        continue;
                    }
                    collector->addBroadPhaseHandle(pHandle, 0);
                }
            }
            bits >>= 4;
            pNodes += 4;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(pBitField, (capacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t));
    }
}

// querySingleAabb's search collecting the overlapping nodes themselves into a local array, reserved up
// front for every node, which calcAabbCacheInternal turns into the cache.
void hkp3AxisSweep::calcAabbCache(const hkAabb& aabb, hkpBroadPhaseAabbCache* AabbCacheOut) const {
    hkArray<const hkpBpNode*, hkContainerTempAllocator> overlaps;
    overlaps.reserve(m_nodes.m_size);

    int32_t numNodes = m_nodes.m_size;
    int32_t numWords = (numNodes >> 5) + 8;
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint32_t* pBitField;
    int32_t capacity;
    if (numWords == 0) {
        pBitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        int32_t numBytes = numWords * sizeof(uint32_t);
        pBitField = static_cast<uint32_t*>(pTemp->bufAlloc(numBytes));
        capacity = numBytes / 4;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    __m128 zero = _mm_setzero_ps();
    uint32_t* pClear = pBitField;
    for (int32_t i = numNodes >> 7; i >= 0; i--) {
        __builtin_memset(pClear, 0, sizeof(uint32_t) * 4);
        pClear += 4;
    }

    // EXE-INLINED none: hkp3AxisSweep::convertAabbToInt
    __m128 scale = _mm_load_ps(&reinterpret_cast<const float*>(&m_scale.m_quad)[0]);
    __m128 quantMax = _mm_load_ps(&reinterpret_cast<float*>(&hkp3AxisSweep_quantizationMax)[0]);
    __m128 quantOffset = convertToUint16WithClipMagicQuad;
    alignas(16) float minQuant[4];
    alignas(16) float maxQuant[4];
    __m128 lo = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&m_offsetLow.m_quad)[0]),
        _mm_load_ps(&reinterpret_cast<const float*>(&aabb.m_min.m_quad)[0])
    );
    lo = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(lo, scale), quantMax), zero), quantOffset);
    _mm_store_ps(minQuant, lo);
    __m128 hi = _mm_add_ps(
        _mm_load_ps(&reinterpret_cast<const float*>(&m_offsetHigh.m_quad)[0]),
        _mm_load_ps(&reinterpret_cast<const float*>(&aabb.m_max.m_quad)[0])
    );
    hi = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(hi, scale), quantMax), zero), quantOffset);
    _mm_store_ps(maxQuant, hi);
    // The floats' bits, shifted right by 7: min ends are even, max ends odd.
    uint32_t minX = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[0]) >> 7) & 0xFFFE;
    uint32_t minY = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[1]) >> 7) & 0xFFFE;
    uint32_t minZ = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, minQuant[2]) >> 7) & 0xFFFE;
    uint32_t maxX = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[0]) >> 7) | 1;
    uint32_t maxY = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[1]) >> 7) | 1;
    uint32_t maxZ = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, maxQuant[2]) >> 7) | 1;

    // The x axis, as in querySingleAabb.
    const hkpBpEndPoint* pEp = m_axis[0].m_endPoints.m_data + 1;
    if (m_numMarkers != 0) {
        int32_t markerIndex = static_cast<int32_t>(minX) >> (16 - m_ld2NumMarkers);
        if (markerIndex > 0) {
            const hkpBpMarker& marker = m_markers[markerIndex - 1];
            pBitField[marker.m_nodeIndex >> 5] ^= 1u << (marker.m_nodeIndex & 0x1F);
            const uint16_t* pOverlap = marker.m_overlappingObjects.m_data;
            for (int32_t i = marker.m_overlappingObjects.m_size - 1; i >= 0; i--) {
                uint16_t nodeIndex = *pOverlap++;
                pBitField[nodeIndex >> 5] ^= 1u << (nodeIndex & 0x1F);
            }
            const hkpBpNode& markerNode = m_nodes.m_data[marker.m_nodeIndex];
            const hkpBpEndPoint* pEnd = m_axis[0].m_endPoints.m_data + markerNode.max_x;
            for (const hkpBpEndPoint* p = m_axis[0].m_endPoints.m_data + markerNode.min_x + 1; p < pEnd; p++) {
                if (!(p->m_value & 1)) {
                    pBitField[p->m_nodeIndex >> 5] &= ~(1u << (p->m_nodeIndex & 0x1F));
                }
            }
            pEp = m_axis[0].m_endPoints.m_data + markerNode.min_x + 1;
        }
    }
    while (pEp->m_value < minX) {
        pBitField[pEp->m_nodeIndex >> 5] ^= 1u << (pEp->m_nodeIndex & 0x1F);
        pEp++;
    }
    while (pEp->m_value < maxX) {
        if (!(pEp->m_value & 1)) {
            pBitField[pEp->m_nodeIndex >> 5] ^= 1u << (pEp->m_nodeIndex & 0x1F);
        }
        pEp++;
    }

    // The y and z axes, as in querySingleAabb.
    const hkpBpAxis& axisY = m_axis[1];
    const hkpBpEndPoint* pYBegin = axisY.m_endPoints.m_data;
    const hkpBpEndPoint* pYLast = pYBegin + axisY.m_endPoints.m_size - 2;
    uint16_t queryMinY = static_cast<uint16_t>(axisY.find(pYBegin + 1, pYLast, static_cast<uint16_t>(minY)) - pYBegin);
    uint16_t queryMaxY =
        static_cast<uint16_t>(axisY.find(pYBegin + 1, pYLast, static_cast<uint16_t>(maxY)) - pYBegin - 1);
    const hkpBpAxis& axisZ = m_axis[2];
    const hkpBpEndPoint* pZBegin = axisZ.m_endPoints.m_data;
    const hkpBpEndPoint* pZLast = pZBegin + axisZ.m_endPoints.m_size - 2;
    uint16_t queryMinZ = static_cast<uint16_t>(axisZ.find(pZBegin + 1, pZLast, static_cast<uint16_t>(minZ)) - pZBegin);
    uint16_t queryMaxZ =
        static_cast<uint16_t>(axisZ.find(pZBegin + 1, pZLast, static_cast<uint16_t>(maxZ)) - pZBegin - 1);
    uint32_t queryMin = queryMinY | static_cast<uint32_t>(queryMinZ) << 16;
    uint32_t queryMax = queryMaxY | static_cast<uint32_t>(queryMaxZ) << 16;

    const uint32_t* pWordEnd = pBitField + (m_nodes.m_size >> 5) + 1;
    const hkpBpNode* pWordNodes = m_nodes.m_data;
    for (const uint32_t* pWord = pBitField; pWord < pWordEnd; pWord++, pWordNodes += 32) {
        uint32_t bits = *pWord;
        const hkpBpNode* pNodes = pWordNodes;
        while (bits) {
            if (bits & 0xF) {
                for (int32_t j = 0; j < 4; j++) {
                    if (!(bits & (1u << j))) {
                        continue;
                    }
                    const hkpBpNode& node = pNodes[j];
                    uint32_t nodeMin = node.min_y | static_cast<uint32_t>(node.min_z) << 16;
                    uint32_t nodeMax = node.max_y | static_cast<uint32_t>(node.max_z) << 16;
                    if (((queryMax - nodeMin) | (nodeMax - queryMin)) & 0x80008000) {
                        continue;
                    }
                    // A marker's handle word has bit 0 set.
                    if (reinterpret_cast<uintptr_t>(node.m_handle) & 1) {
                        continue;
                    }
                    overlaps.pushBackUnchecked(&node);
                }
            }
            bits >>= 4;
            pNodes += 4;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(pBitField, (capacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t));
    }

    calcAabbCacheInternal(overlaps, AabbCacheOut);
}

// The overlapping collidables' nodes, in a local array on the thread's stack memory, go to calcAabbCacheInternal.
void hkp3AxisSweep::calcAabbCache(
    const hkArrayBase<hkpCollidable*>& overlappingCollidables,
    hkpBroadPhaseAabbCache* AabbCacheOut
) const {
    const int32_t numCollidables = overlappingCollidables.m_size;
    hkLocalArray<const hkpBpNode*> overlaps(numCollidables);
    for (hkpCollidable* collidable : overlappingCollidables) {
        overlaps.pushBackUnchecked(&m_nodes.m_data[collidable->m_broadPhaseHandle.m_id]);
    }
    calcAabbCacheInternal(overlaps, AabbCacheOut);
}

// Renumbers the nodes in x order of their min end points: copies the nodes, sorts (min x, node index) pairs past node
// 0, writes the nodes back in that order, and maps every node index the handles, markers' overlap lists and axis end
// points hold to the new one. The three temp arrays are hkArray<T, hkContainerTempAllocator> of a given size,
// constructed and destroyed inline.
void hkp3AxisSweep::defragment() {
    int32_t numNodes = m_nodes.m_size;

    // EXE-INLINED none: hkArray<hkpBpNode, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator& temp0 = hkMemoryRouter::getInstance().temp();
    hkpBpNode* nodes = nullptr;
    int32_t nodesCapacity = 0;
    if (numNodes) {
        nodesCapacity = numNodes;
        nodes = temp0._bufAlloc<hkpBpNode>(nodesCapacity);
    }
    if (!nodesCapacity) {
        nodesCapacity = static_cast<int32_t>(hkArrayBase<hkpBpNode>::DONT_DEALLOCATE_FLAG);
    }

    // EXE-INLINED none: hkArray<hkpBpEndPoint, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator& temp1 = hkMemoryRouter::getInstance().temp();
    hkpBpEndPoint* sortedEndPoints = nullptr;
    int32_t sortedCapacity = 0;
    if (numNodes) {
        sortedCapacity = numNodes;
        sortedEndPoints = temp1._bufAlloc<hkpBpEndPoint>(sortedCapacity);
    }
    if (!sortedCapacity) {
        sortedCapacity = static_cast<int32_t>(hkArrayBase<hkpBpEndPoint>::DONT_DEALLOCATE_FLAG);
    }

    {
        hkArray<int32_t, hkContainerTempAllocator> newIndex(numNodes);

        for (int32_t i = 0; i < m_nodes.m_size; ++i) {
            nodes[i] = m_nodes.m_data[i];
            sortedEndPoints[i].m_value = m_nodes.m_data[i].min_x;
            sortedEndPoints[i].m_nodeIndex = static_cast<uint16_t>(i);
        }

        // Node 0 keeps its place: the sort covers entries 1 to numNodes - 1.
        uint32_t uiLess = 0;
        if (numNodes - 1 > 1) {
            quickSortRecursive(sortedEndPoints + 1, 0, numNodes - 2, uiLess);
        }

        for (int32_t i = 0; i < m_nodes.m_size; ++i) {
            int32_t oldIndex = sortedEndPoints[i].m_nodeIndex;
            newIndex.m_data[oldIndex] = i;
            m_nodes.m_data[i] = nodes[oldIndex];
        }

        for (int32_t i = 1; i < m_nodes.m_size; ++i) {
            hkpBpNode& node = m_nodes.m_data[i];
            if (node.isMarker()) {
                // EXE-INLINED none: hkp3AxisSweep::hkpBpNode::getMarker
                // A marker node's handle field is no pointer: the exe treats its bits as its marker's byte offset in
                // m_markers, tagged by the low bit.
                hkpBpMarker* pMarker = reinterpret_cast<hkpBpMarker*>(
                    reinterpret_cast<uintptr_t>(m_markers) +
                    (reinterpret_cast<uintptr_t>(node.m_handle) & ~static_cast<uintptr_t>(1))
                );
                pMarker->m_nodeIndex = static_cast<uint16_t>(i);
            }
            else {
                node.m_handle->m_id = static_cast<uint32_t>(i);
            }
        }

        for (int32_t i = 0; i < m_numMarkers; ++i) {
            hkpBpMarker& marker = m_markers[i];
            for (int32_t j = marker.m_overlappingObjects.m_size - 1; j >= 0; --j) {
                uint16_t* pObject = &marker.m_overlappingObjects.m_data[j];
                *pObject = static_cast<uint16_t>(newIndex.m_data[*pObject]);
            }
        }

        for (hkpBpAxis& rAxis : m_axis) {
            hkArray<hkpBpEndPoint>& endPoints = rAxis.m_endPoints;
            for (hkpBpEndPoint& endPoint : endPoints) {
                endPoint.m_nodeIndex = static_cast<uint16_t>(newIndex.m_data[endPoint.m_nodeIndex]);
            }
        }

        for (int32_t i = 1; i < m_nodes.m_size; ++i) {
            _fixDeterministicOrderAfterNodeIdWasDecreased(i);
        }
    }
    // EXE-INLINED none: ~hkArray<hkpBpEndPoint, hkContainerTempAllocator>
    hkMemoryAllocator& temp4 = hkMemoryRouter::getInstance().temp();
    if ((sortedCapacity & hkArrayBase<hkpBpEndPoint>::DONT_DEALLOCATE_FLAG) == 0) {
        temp4._bufFree<hkpBpEndPoint>(sortedEndPoints, sortedCapacity & hkArrayBase<hkpBpEndPoint>::CAPACITY_MASK);
    }
    // EXE-INLINED none: ~hkArray<hkpBpNode, hkContainerTempAllocator>
    hkMemoryAllocator& temp5 = hkMemoryRouter::getInstance().temp();
    if ((nodesCapacity & hkArrayBase<hkpBpNode>::DONT_DEALLOCATE_FLAG) == 0) {
        temp5._bufFree<hkpBpNode>(nodes, nodesCapacity & hkArrayBase<hkpBpNode>::CAPACITY_MASK);
    }
}

// Casts numCasts rays from input.m_from through the broad phase. A byte per node (the temp allocator's buffer) holds,
// per axis, whether the node's interval on that axis contains the start point: bit axis in the low nibble (the state at
// the start) and bit axis + 4 in the high nibble (the state as a ray walks). The start state comes from walking each
// axis's end points (of the cache's axes when there is one) from its nearer end to the start's quantized value. Every
// non-marker node containing the start (0x77) goes to every cast's collector first; each cast then walks the end points
// on the axis of the nearest next crossing, reporting each node whose high nibble becomes full, until the crossing lies
// beyond the hit fraction. Between casts the high nibbles are reset from the low ones. An uninitialised read is
// deliberately kept, as the game does: an axis whose ray extent is degenerate (start or end times
// 1.1920928955078125e-07 above the scaled direction's magnitude) gets inverse 0.0f and start -2.0f but no step, and the
// walk reads that step whenever the axis is picked (all positions at 2.0f), which a NaN hit fraction from the collector
// allows.
void hkp3AxisSweep::castRay(
    const hkpBroadPhase::hkpCastRayInput& rayInput,
    hkpBroadPhaseCastCollector* collectorBase,
    int32_t collectorStriding
) const {
    __m128 zero = _mm_setzero_ps();
    __m128 quantFrom = _mm_add_ps(
        _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(m_offsetLow.m_quad, rayInput.m_from.m_quad), m_scale.m_quad),
                hkp3AxisSweep_quantizationMax
            ),
            zero
        ),
        convertToUint16WithClipMagicQuad
    );
    // The quantized value is the float's bits shifted right by 7, cut to 16 bits.
    uint32_t fromQuant[3];
    fromQuant[0] = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantFrom[0]) >> 7);
    fromQuant[1] = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantFrom[1]) >> 7);
    fromQuant[2] = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantFrom[2]) >> 7);

    // EXE-INLINED none: hkLocalBuffer<hkUchar>::hkLocalBuffer
    int32_t numBytes = m_nodes.m_size + 16;
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint8_t* bitField;
    int32_t capacity;
    if (numBytes == 0) {
        bitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        int32_t allocBytes = numBytes;
        bitField = static_cast<uint8_t*>(pTemp->bufAlloc(allocBytes));
        capacity = allocBytes;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    // EXE-INLINED 0xD11000
    // memclear16(bitField, m_nodes.m_size): the exe clears with movaps stores of a zero register.
    uint8_t* pClear = bitField;
    for (int32_t i = m_nodes.m_size >> 4; i >= 0; i--) {
        __builtin_memset(pClear, 0, 16);
        pClear += 16;
    }

    // The cache calcAabbCache builds holds three axes' end points.
    const hkpBpAxis* axes =
        rayInput.m_aabbCacheInfo ? reinterpret_cast<const hkpBpAxis*>(rayInput.m_aabbCacheInfo) : m_axis;
    const hkpBpEndPoint* startEndPoints[3];
    uint8_t axisBits = 0x11;
    for (int32_t i = 0; i < 3; i++) {
        const hkpBpEndPoint* endPoints = axes[i].m_endPoints.m_data;
        int32_t numEndPoints = axes[i].m_endPoints.m_size;
        uint32_t value = fromQuant[i];
        const hkpBpEndPoint* ep;
        if (value < endPoints[numEndPoints >> 1].m_value) {
            // Up from the first end point after the sentinel, four at a time while the fourth is not above.
            const hkpBpEndPoint* limit = endPoints + numEndPoints - 4;
            ep = endPoints + 1;
            while (ep < limit && ep[3].m_value <= value) {
                bitField[ep[0].m_nodeIndex] ^= axisBits;
                bitField[ep[1].m_nodeIndex] ^= axisBits;
                bitField[ep[2].m_nodeIndex] ^= axisBits;
                bitField[ep[3].m_nodeIndex] ^= axisBits;
                ep += 4;
            }
            while (ep->m_value <= value) {
                bitField[ep->m_nodeIndex] ^= axisBits;
                ep++;
            }
        }
        else {
            // Down from the last end point before the sentinel, four at a time while the fourth is above.
            const hkpBpEndPoint* limit = endPoints + 4;
            ep = endPoints + numEndPoints - 2;
            while (ep >= limit && ep[-3].m_value > value) {
                bitField[ep[0].m_nodeIndex] ^= axisBits;
                bitField[ep[-1].m_nodeIndex] ^= axisBits;
                bitField[ep[-2].m_nodeIndex] ^= axisBits;
                bitField[ep[-3].m_nodeIndex] ^= axisBits;
                ep -= 4;
            }
            while (ep->m_value > value) {
                bitField[ep->m_nodeIndex] ^= axisBits;
                ep--;
            }
            ep++;
        }
        startEndPoints[i] = ep;
        axisBits <<= 1;
    }

    float* hitFractions = hkAllocateStack<float>(rayInput.m_numCasts, nullptr);
    for (int32_t i = 0; i < rayInput.m_numCasts; i++) {
        hitFractions[i] = 1.0f;
    }

    // Every node containing the start on all three axes (0x77), markers left out, goes to every cast. A word whose
    // four bytes all lack bit 3 after adding 1 (no byte is 0x?7) is skipped, up to three at a time.
    int32_t numScanNodes = m_nodes.m_size;
    const hkpBpNode* scanNodes = m_nodes.m_data;
    const uint8_t* scanEnd = bitField + 4 * (numScanNodes >> 2) + 4;
    const uint8_t* scan = bitField;
    while (scan < scanEnd) {
        // Reads the raw byte buffer four bytes at a time.
        const uint32_t* words = reinterpret_cast<const uint32_t*>(scan);
        if (((words[0] + 0x01010101u) & 0x08080808u) == 0) {
            int32_t skip;
            if (((words[1] + 0x01010101u) & 0x08080808u) != 0) {
                skip = 4;
            }
            else if (((words[2] + 0x01010101u) & 0x08080808u) != 0) {
                skip = 8;
            }
            else {
                skip = 12;
            }
            scan += skip;
            scanNodes += skip;
            continue;
        }
        for (int32_t b = 0; b < 4; b++) {
            if (scan[b] == 0x77 && !scanNodes[b].isMarker()) {
                hkpBroadPhaseCastCollector* pCollector = collectorBase;
                for (int32_t castIndex = 0; castIndex < rayInput.m_numCasts; castIndex++) {
                    float oldHit = hitFractions[castIndex];
                    float hit = pCollector->addBroadPhaseHandle(scanNodes[b].m_handle, castIndex);
                    hitFractions[castIndex] = hit > oldHit ? oldHit : hit;
                    // EXE-INLINED none: hkAddByteOffset<hkpBroadPhaseCastCollector>
                    pCollector = reinterpret_cast<hkpBroadPhaseCastCollector*>(
                        reinterpret_cast<char*>(pCollector) + collectorStriding
                    );
                }
            }
        }
        scan += 4;
        scanNodes += 4;
    }

    // The sentinel node 0 counts as outside on every axis while walking.
    bitField[0] = 0x88;
    int32_t numNodes = m_nodes.m_size;
    hkpBroadPhaseCastCollector* pCollector = collectorBase;
    for (int32_t castIndex = 0; castIndex < rayInput.m_numCasts; castIndex++) {
        const hkpBpEndPoint* ep[3] = {startEndPoints[0], startEndPoints[1], startEndPoints[2]};
        float hitFraction = hitFractions[castIndex];
        hkVector4 to = *hkAddByteOffsetConst(rayInput.m_toBase, rayInput.m_toStriding * castIndex);
        hkVector4 dir;
        dir.m_quad = _mm_sub_ps(to.m_quad, rayInput.m_from.m_quad);

        // Per axis: the step through the end points, the inverse of the scaled direction and the ray parameter's
        // offset, so that an end point's value times the inverse minus the offset is where the ray crosses it.
        int32_t step[3];
        float invDir[3];
        float startOffset[3];
        for (int32_t i = 0; i < 3; i++) {
            float scaledDir = dir.m_quad[i] * m_scale.m_quad[i];
            float absDir = std::fabs(scaledDir);
            float scaledFrom = (m_offsetLow.m_quad[i] + rayInput.m_from.m_quad[i]) * m_scale.m_quad[i];
            float scaledTo;
            if (scaledFrom * 1.1920928955078125e-07 > absDir ||
                (scaledTo = (to.m_quad[i] + m_offsetLow.m_quad[i]) * m_scale.m_quad[i],
                 scaledTo * 1.1920928955078125e-07 > absDir)) {
                invDir[i] = 0.0f;
                startOffset[i] = -2.0f;
            }
            else {
                step[i] = 1;
                float inv = 1.0 / scaledDir;
                if (scaledDir < 0.0f) {
                    step[i] = -1;
                    ep[i]--;
                }
                invDir[i] = inv;
                float fromCorrected = scaledFrom - m_intToFloatFloorCorrection;
                startOffset[i] = inv * fromCorrected;
            }
        }
        float crossing[3];
        crossing[0] = ep[0]->m_value * invDir[0] - startOffset[0];
        crossing[1] = ep[1]->m_value * invDir[1] - startOffset[1];
        crossing[2] = ep[2]->m_value * invDir[2] - startOffset[2];

        for (;;) {
            int32_t axis;
            float nearest;
            if (crossing[1] > crossing[0]) {
                if (crossing[2] > crossing[0]) {
                    axis = 0;
                    nearest = crossing[0];
                }
                else {
                    axis = 2;
                    nearest = crossing[2];
                }
            }
            else if (crossing[2] > crossing[1]) {
                axis = 1;
                nearest = crossing[1];
            }
            else {
                axis = 2;
                nearest = crossing[2];
            }
            if (hitFraction < nearest) {
                break;
            }
            uint8_t walkBit = static_cast<uint8_t>(1 << (axis + 4));
            bool reachedSentinel = false;
            const hkpBpEndPoint* cur;
            for (;;) {
                cur = ep[axis];
                uint32_t nodeIndex = cur->m_nodeIndex;
                bitField[nodeIndex] ^= walkBit;
                if (bitField[nodeIndex] >= 0x70) {
                    if (nodeIndex == 0) {
                        reachedSentinel = true;
                        break;
                    }
                    const hkpBpNode& node = m_nodes.m_data[nodeIndex];
                    if (!node.isMarker()) {
                        hkpBroadPhaseHandle* handle = node.m_handle;
                        float hit = pCollector->addBroadPhaseHandle(handle, castIndex);
                        if (!(hit > hitFraction)) {
                            hitFraction = hit;
                        }
                    }
                }
                uint16_t curValue = cur->m_value;
                cur += step[axis];
                ep[axis] = cur;
                if (curValue != cur->m_value) {
                    break;
                }
            }
            if (reachedSentinel) {
                crossing[axis] = 2.0f;
            }
            else {
                crossing[axis] = cur->m_value * invDir[axis] - startOffset[axis];
            }
        }

        // The walk state goes back to the start state for the next cast: each byte's low nibble into its high one.
        if (castIndex < rayInput.m_numCasts - 1) {
            // Rewrites the raw byte buffer two 32-bit words at a time, reading and writing up to 4 bytes past the end,
            // inside the 16 spare bytes.
            uint32_t* words = reinterpret_cast<uint32_t*>(bitField);
            uint32_t* wordsEnd = reinterpret_cast<uint32_t*>(bitField + 4 * (numNodes >> 2) + 4);
            for (; words < wordsEnd; words += 2) {
                uint32_t low0 = words[0] & 0x0F0F0F0Fu;
                uint32_t low1 = words[1] & 0x0F0F0F0Fu;
                words[0] = low0 << 4 | low0;
                words[1] = low1 << 4 | low1;
            }
        }
        // EXE-INLINED none: hkAddByteOffset<hkpBroadPhaseCastCollector>
        pCollector =
            reinterpret_cast<hkpBroadPhaseCastCollector*>(reinterpret_cast<char*>(pCollector) + collectorStriding);
    }

    hkDeallocateStack(hitFractions);
    // EXE-INLINED none: hkLocalBuffer<hkUchar>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(bitField, capacity & hkArrayBase<uint8_t>::CAPACITY_MASK);
    }
}

// Sweeps the AABB of half extents input.m_halfExtents from input.m_from to input.m_to through the broad phase. A byte
// per node (the temp allocator's buffer) has bit axis set while the node's interval on that axis overlaps the swept
// AABB. The start state comes from walking each axis's end points (of the cache's axes when there is one) from its
// nearer end to the AABB's quantized min, then on to its max. Every non-marker node overlapping on all three axes (7)
// goes to the collector first. Then the leading and the trailing face of the AABB walk the end points in the order
// the sweep crosses them, the leading face reporting each node that comes to overlap on all three axes, until the
// next crossing lies beyond the hit fraction.
void hkp3AxisSweep::castAabb(
    const hkpBroadPhase::hkpCastAabbInput& input,
    hkpBroadPhaseCastCollector& collector
) const {
    hkVector4 aabbMin;
    aabbMin.m_quad = _mm_sub_ps(input.m_from.m_quad, input.m_halfExtents.m_quad);
    hkVector4 aabbMax;
    aabbMax.m_quad = _mm_add_ps(input.m_from.m_quad, input.m_halfExtents.m_quad);
    __m128 zero = _mm_setzero_ps();
    __m128 quantMin = _mm_add_ps(
        _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(m_offsetLow.m_quad, aabbMin.m_quad), m_scale.m_quad),
                hkp3AxisSweep_quantizationMax
            ),
            zero
        ),
        convertToUint16WithClipMagicQuad
    );
    __m128 quantMax = _mm_add_ps(
        _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(m_offsetLow.m_quad, aabbMax.m_quad), m_scale.m_quad),
                hkp3AxisSweep_quantizationMax
            ),
            zero
        ),
        convertToUint16WithClipMagicQuad
    );
    // The quantized value is the float's bits shifted right by 7, cut to 16 bits.
    uint32_t minQuant[3];
    uint32_t maxQuant[3];
    for (int32_t i = 0; i < 3; i++) {
        minQuant[i] = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMin[i]) >> 7);
        maxQuant[i] = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[i]) >> 7);
    }

    // EXE-INLINED none: hkLocalBuffer<hkUchar>::hkLocalBuffer
    int32_t numBytes = m_nodes.m_size + 16;
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    uint8_t* bitField;
    int32_t capacity;
    if (numBytes == 0) {
        bitField = nullptr;
        capacity = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
    }
    else {
        int32_t allocBytes = numBytes;
        bitField = static_cast<uint8_t*>(pTemp->bufAlloc(allocBytes));
        capacity = allocBytes;
        if (capacity == 0) {
            capacity = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    // EXE-INLINED 0xD11000
    // memclear16(bitField, m_nodes.m_size): the exe clears with movaps stores of a zero register.
    uint8_t* pClear = bitField;
    for (int32_t i = m_nodes.m_size >> 4; i >= 0; i--) {
        __builtin_memset(pClear, 0, 16);
        pClear += 16;
    }

    // The cache calcAabbCache builds is raw memory holding three axes' end points.
    const hkpBpAxis* axes = input.m_aabbCacheInfo ? reinterpret_cast<const hkpBpAxis*>(input.m_aabbCacheInfo) : m_axis;
    // Per axis, the first end point above the min (the trailing face's) and above the max (the leading face's);
    // the setup below swaps them for a sweep towards lower values.
    const hkpBpEndPoint* trailEp[3];
    const hkpBpEndPoint* leadEp[3];
    uint8_t axisBit = 1;
    for (int32_t i = 0; i < 3; i++) {
        const hkpBpEndPoint* endPoints = axes[i].m_endPoints.m_data;
        int32_t numEndPoints = axes[i].m_endPoints.m_size;
        uint32_t minValue = minQuant[i];
        const hkpBpEndPoint* ep;
        if (minValue < endPoints[numEndPoints >> 1].m_value) {
            // Up from the first end point after the sentinel, four at a time while the fourth is not above the min.
            const hkpBpEndPoint* limit = endPoints + numEndPoints - 4;
            ep = endPoints + 1;
            while (ep < limit && ep[3].m_value <= minValue) {
                bitField[ep[0].m_nodeIndex] ^= axisBit;
                bitField[ep[1].m_nodeIndex] ^= axisBit;
                bitField[ep[2].m_nodeIndex] ^= axisBit;
                bitField[ep[3].m_nodeIndex] ^= axisBit;
                ep += 4;
            }
            while (ep->m_value <= minValue) {
                bitField[ep->m_nodeIndex] ^= axisBit;
                ep++;
            }
            uint32_t maxValue = maxQuant[i];
            trailEp[i] = ep;
            // On up to the max: only a node's min end point (even value) flips its bit.
            while (ep->m_value <= maxValue) {
                bitField[ep->m_nodeIndex] ^= axisBit & ((ep->m_value & 1) - 1);
                ep++;
            }
            leadEp[i] = ep;
        }
        else {
            // Down from the last end point before the sentinel, four at a time while the fourth is above the max.
            const hkpBpEndPoint* limit = endPoints + 4;
            ep = endPoints + numEndPoints - 2;
            while (ep >= limit && ep[-3].m_value > maxQuant[i]) {
                bitField[ep[0].m_nodeIndex] ^= axisBit;
                bitField[ep[-1].m_nodeIndex] ^= axisBit;
                bitField[ep[-2].m_nodeIndex] ^= axisBit;
                bitField[ep[-3].m_nodeIndex] ^= axisBit;
                ep -= 4;
            }
            uint32_t maxValue = maxQuant[i];
            while (ep->m_value > maxValue) {
                bitField[ep->m_nodeIndex] ^= axisBit;
                ep--;
            }
            leadEp[i] = ep + 1;
            // On down to the min: only a node's max end point (odd value) flips its bit.
            while (ep->m_value > minValue) {
                bitField[ep->m_nodeIndex] ^= axisBit & -(ep->m_value & 1);
                ep--;
            }
            trailEp[i] = ep + 1;
        }
        axisBit <<= 1;
    }

    // Every node overlapping on all three axes (7), markers left out, goes to the collector. The raw byte buffer is
    // read four bytes at a time: adding 1 sets bit 3 of a byte only for a 7.
    float hitFraction = 1.0f;
    const hkpBpNode* scanNodes = m_nodes.m_data;
    const uint8_t* scanEnd = bitField + 4 * (m_nodes.m_size >> 2) + 4;
    for (const uint8_t* scan = bitField; scan < scanEnd; scan += 4, scanNodes += 4) {
        if (((*reinterpret_cast<const uint32_t*>(scan) + 0x01010101u) & 0x08080808u) != 0) {
            for (int32_t b = 0; b < 4; b++) {
                if (scan[b] == 7 && !scanNodes[b].isMarker()) {
                    float hit = collector.addBroadPhaseHandle(scanNodes[b].m_handle, 0);
                    if (!(hit > hitFraction)) {
                        hitFraction = hit;
                    }
                }
            }
        }
    }

    // Per axis: the step through the end points, the inverse of the scaled direction and the sweep parameter's
    // offsets of both faces, so that an end point's value times the inverse minus a face's offset is where that face
    // crosses it. A face's flip says which end points (min: 1, max: 0) turn its bit on when the face crosses them.
    hkVector4 dir;
    dir.m_quad = _mm_sub_ps(input.m_to.m_quad, input.m_from.m_quad);
    int32_t step[3];
    uint8_t trailFlip[3];
    int32_t leadFlip[3];
    float invDir[3];
    float trailOffset[3];
    float leadOffset[3];
    for (int32_t i = 0; i < 3; i++) {
        float scaledDir = dir.m_quad[i] * m_scale.m_quad[i];
        float absDir = std::fabs(scaledDir);
        if (scaledDir > 0.0f) {
            step[i] = 1;
            trailFlip[i] = 0;
            leadFlip[i] = 1;
        }
        else {
            // Towards lower values the min face leads: the faces swap, each starting at the end point below.
            step[i] = -1;
            const hkpBpEndPoint* oldTrail = trailEp[i];
            float oldMin = aabbMin.m_quad[i];
            trailEp[i] = leadEp[i] - 1;
            aabbMin.m_quad[i] = aabbMax.m_quad[i];
            leadEp[i] = oldTrail - 1;
            aabbMax.m_quad[i] = oldMin;
            trailFlip[i] = 1;
            leadFlip[i] = 0;
        }
        float scaledMin = (aabbMin.m_quad[i] + m_offsetLow.m_quad[i]) * m_scale.m_quad[i];
        float scaledTo;
        if (scaledMin * 1.1920928955078125e-07 > absDir ||
            (scaledTo = (m_offsetLow.m_quad[i] + input.m_to.m_quad[i]) * m_scale.m_quad[i],
             scaledTo * 1.1920928955078125e-07 > absDir)) {
            invDir[i] = 0.0f;
            trailOffset[i] = -2.0f;
            leadOffset[i] = -2.0f;
        }
        else {
            float inv = 1.0 / scaledDir;
            invDir[i] = inv;
            float minCorrected = scaledMin - m_intToFloatFloorCorrection;
            trailOffset[i] = minCorrected * inv;
            float scaledMax = (aabbMax.m_quad[i] + m_offsetLow.m_quad[i]) * m_scale.m_quad[i];
            float maxCorrected = scaledMax - m_intToFloatFloorCorrection;
            leadOffset[i] = inv * maxCorrected;
        }
    }
    alignas(16) float trailCross[3];
    alignas(16) float leadCross[3];
    for (int32_t i = 0; i < 3; i++) {
        trailCross[i] = trailEp[i]->m_value * invDir[i] - trailOffset[i];
    }
    for (int32_t i = 0; i < 3; i++) {
        leadCross[i] = leadEp[i]->m_value * invDir[i] - leadOffset[i];
    }
    // The sentinel node 0 counts as outside on every axis while walking: either face reaching it takes its byte
    // above 7.
    bitField[0] = 8;

    // getMinComponent's copy of the smallest crossing, which nothing reads here.
    float minCross;
    int32_t trailAxis = getMinComponent(trailCross, &minCross);
    int32_t leadAxis = getMinComponent(leadCross, &minCross);
    for (;;) {
        if (trailCross[trailAxis] > leadCross[leadAxis]) {
            int32_t axis = leadAxis;
            if (hitFraction < leadCross[axis]) {
                break;
            }
            bool reachedSentinel = false;
            const hkpBpEndPoint* cur;
            for (;;) {
                cur = leadEp[axis];
                uint32_t nodeIndex = cur->m_nodeIndex;
                int32_t bit = ((cur->m_value & 1) ^ leadFlip[axis]) << axis;
                bitField[nodeIndex] ^= bit;
                if (bitField[nodeIndex] >= 7) {
                    if (nodeIndex == 0) {
                        reachedSentinel = true;
                        break;
                    }
                    // A node whose last bit just came on overlaps on all three axes now.
                    if (bit != 0) {
                        const hkpBpNode& node = m_nodes.m_data[nodeIndex];
                        if (!node.isMarker()) {
                            float hit = collector.addBroadPhaseHandle(node.m_handle, 0);
                            if (!(hit > hitFraction)) {
                                hitFraction = hit;
                            }
                        }
                    }
                }
                uint16_t curValue = cur->m_value;
                cur += step[axis];
                leadEp[axis] = cur;
                if (curValue != cur->m_value) {
                    break;
                }
            }
            if (reachedSentinel) {
                leadCross[axis] = 2.0f;
            }
            else {
                leadCross[axis] = cur->m_value * invDir[axis] - leadOffset[axis];
            }
            leadAxis = getMinComponent(leadCross, &minCross);
        }
        else {
            int32_t axis = trailAxis;
            if (hitFraction < trailCross[axis]) {
                break;
            }
            uint8_t flip = trailFlip[axis];
            bool reachedSentinel = false;
            const hkpBpEndPoint* cur;
            for (;;) {
                cur = trailEp[axis];
                uint32_t nodeIndex = cur->m_nodeIndex;
                bitField[nodeIndex] ^= static_cast<uint8_t>(((cur->m_value & 1) ^ flip) << axis);
                if (bitField[nodeIndex] > 8) {
                    reachedSentinel = true;
                    break;
                }
                uint16_t curValue = cur->m_value;
                cur += step[axis];
                trailEp[axis] = cur;
                if (curValue != cur->m_value) {
                    break;
                }
            }
            if (reachedSentinel) {
                trailCross[axis] = 2.0f;
            }
            else {
                trailCross[axis] = cur->m_value * invDir[axis] - trailOffset[axis];
            }
            trailAxis = getMinComponent(trailCross, &minCross);
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUchar>::~hkLocalBuffer
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    if (capacity >= 0) {
        pTempFree->bufFree(bitField, capacity & hkArrayBase<uint8_t>::CAPACITY_MASK);
    }
}

// Moves every end point by the shift quantized to an even step on its axis, walking each axis against the shift so that
// no end point overtakes one not yet moved; the border's sentinel values (0, 1 and from 0xFFFC up) stay. An end point
// clamped to the border (0 or 0xFFFD) breaks the order, so its node's six end points are sorted again with their
// current values as updateAabbs does: the overlaps begun go to newCollisionPairs, the ones ended to a local array that
// is dropped. effectiveShiftDistanceOut gets the quantized shift back in world units.
void hkp3AxisSweep::shiftAllObjects(
    const hkVector4& shiftDistance,
    hkVector4& effectiveShiftDistanceOut,
    hkArray<hkpBroadPhaseHandlePair>& newCollisionPairs
) {
    hkVector4 shift = shiftDistance;
    int64_t intShift[3];
    for (int32_t i = 0; i < 3; i++) {
        // __ftol2 truncates the product to 64 bits; the low bit is cleared so that min ends stay even.
        intShift[i] = static_cast<int64_t>(m_scale(i) * shift(i)) & ~static_cast<int64_t>(1);
        shift(i) = static_cast<float>(static_cast<double>(intShift[i]) / m_scale(i));
    }
    effectiveShiftDistanceOut = shift;

    alignas(16) hkArray<hkpBroadPhaseHandlePair> delPairs;
    delPairs.m_data = nullptr;
    delPairs.m_size = 0;
    delPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
    for (int32_t axisIndex = 0; axisIndex < 3; axisIndex++) {
        hkpBpAxis& axis = m_axis[axisIndex];
        int32_t sh = static_cast<int32_t>(intShift[axisIndex]);
        int32_t start;
        int32_t end;
        int32_t step;
        if (sh >= 0) {
            start = axis.m_endPoints.m_size - 2;
            end = 0;
            step = -1;
        }
        else {
            start = 1;
            end = axis.m_endPoints.m_size - 1;
            step = 1;
        }
        for (int32_t epIndex = start; epIndex != end; epIndex += step) {
            hkpBpEndPoint& movedEp = axis.m_endPoints.m_data[epIndex];
            int32_t oldValue = movedEp.m_value;
            if (static_cast<uint32_t>(oldValue - 2) > 0xFFF9u) {
                continue;
            }
            hkpBpNode& movedNode = m_nodes.m_data[movedEp.m_nodeIndex];
            int32_t isMax = oldValue & 1;
            int32_t newValue = ((oldValue + sh) & ~1) | isMax;
            if (newValue < 0) {
                newValue = isMax;
            }
            else if (newValue >= AABB_MAX_VALUE) {
                newValue = isMax | AABB_MAX_VALUE;
            }
            movedEp.m_value = static_cast<uint16_t>(newValue);
            if (newValue != AABB_MIN_VALUE && newValue != (AABB_MAX_VALUE | 1)) {
                continue;
            }

            uint32_t minX = m_axis[0].m_endPoints.m_data[movedNode.min_x].m_value;
            uint32_t minY = m_axis[1].m_endPoints.m_data[movedNode.min_y].m_value;
            uint32_t minZ = m_axis[2].m_endPoints.m_data[movedNode.min_z].m_value;
            uint32_t maxX = m_axis[0].m_endPoints.m_data[movedNode.max_x].m_value;
            uint32_t maxY = m_axis[1].m_endPoints.m_data[movedNode.max_y].m_value;
            uint32_t maxZ = m_axis[2].m_endPoints.m_data[movedNode.max_z].m_value;
            hkpBpNode* nodes = m_nodes.m_data;
            uint32_t id = movedNode.m_handle->m_id;
            hkpBpNode* node = &nodes[id];

            // x: the min end moving down.
            uint32_t pos = node->min_x;
            hkpBpEndPoint* ep = &m_axis[0].m_endPoints.m_data[pos];
            uint32_t value;
            for (value = ep[-1].m_value; minX < value; value = ep[-1].m_value) {
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                *ep = ep[-1];
                --ep;
                if (value & 1) {
                    other->max_x = static_cast<uint16_t>(pos);
                    if (!node->yzDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlapCheckMarker(
                            m_markers, *node, static_cast<int32_t>(id), *other, newCollisionPairs
                        );
                    }
                }
                else {
                    other->min_x = static_cast<uint16_t>(pos);
                }
                --pos;
            }
            while (minX == ep[-1].m_value) {
                uint32_t otherId = ep[-1].m_nodeIndex;
                if (id >= otherId) {
                    break;
                }
                *ep = ep[-1];
                --ep;
                nodes[otherId].min_x = static_cast<uint16_t>(pos);
                --pos;
            }
            ep->m_nodeIndex = static_cast<uint16_t>(id);
            ep->m_value = static_cast<uint16_t>(minX);
            node->min_x = static_cast<uint16_t>(pos);

            // x: the max end moving up.
            pos = node->max_x;
            ep = &m_axis[0].m_endPoints.m_data[pos];
            for (value = ep[1].m_value; maxX > value; value = ep[1].m_value) {
                *ep = ep[1];
                hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
                ++ep;
                ++pos;
                if (value & 1) {
                    --other->max_x;
                }
                else {
                    --other->min_x;
                    if (!node->yzDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlapCheckMarker(
                            m_markers, *node, static_cast<int32_t>(id), *other, newCollisionPairs
                        );
                    }
                }
            }
            while (maxX == ep[1].m_value) {
                uint32_t otherId = ep[1].m_nodeIndex;
                if (id <= otherId) {
                    break;
                }
                if (otherId == 0) {
                    break;
                }
                *ep = ep[1];
                ++ep;
                --nodes[otherId].max_x;
                ++pos;
            }
            // x: the max end moving down.
            for (value = ep[-1].m_value; maxX < value; value = ep[-1].m_value) {
                *ep = ep[-1];
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                --ep;
                --pos;
                if (value & 1) {
                    ++other->max_x;
                }
                else {
                    ++other->min_x;
                    if (!node->yzDisjoint(*other)) {
                        hkp3AxisSweep::endOverlapCheckMarker(
                            m_markers, *node, static_cast<int32_t>(id), *other, delPairs
                        );
                    }
                }
            }
            while (maxX == ep[-1].m_value) {
                uint32_t otherId = ep[-1].m_nodeIndex;
                if (id >= otherId) {
                    break;
                }
                *ep = ep[-1];
                ++nodes[otherId].max_x;
                --ep;
                --pos;
            }
            node->max_x = static_cast<uint16_t>(pos);
            ep->m_value = static_cast<uint16_t>(maxX);
            ep->m_nodeIndex = static_cast<uint16_t>(id);

            // x: the min end moving up.
            pos = node->min_x;
            ep = &m_axis[0].m_endPoints.m_data[pos];
            for (value = ep[1].m_value; minX > value; value = ep[1].m_value) {
                hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
                *ep = ep[1];
                ++ep;
                ++pos;
                if (value & 1) {
                    --other->max_x;
                    if (!node->yzDisjoint(*other)) {
                        hkp3AxisSweep::endOverlapCheckMarker(
                            m_markers, *node, static_cast<int32_t>(id), *other, delPairs
                        );
                    }
                }
                else {
                    --other->min_x;
                }
            }
            while (minX == ep[1].m_value) {
                uint32_t otherId = ep[1].m_nodeIndex;
                if (id <= otherId) {
                    break;
                }
                *ep = ep[1];
                ++ep;
                --nodes[otherId].min_x;
                ++pos;
            }
            node->min_x = static_cast<uint16_t>(pos);
            ep->m_nodeIndex = static_cast<uint16_t>(id);
            ep->m_value = static_cast<uint16_t>(minX);

            // y: the min end moving down.
            pos = node->min_y;
            ep = &m_axis[1].m_endPoints.m_data[pos];
            for (value = ep[-1].m_value; minY < value; value = ep[-1].m_value) {
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                *ep = ep[-1];
                --ep;
                if (value & 1) {
                    other->max_y = static_cast<uint16_t>(pos);
                    if (!node->xzDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlap(*node, *other, newCollisionPairs);
                    }
                }
                else {
                    other->min_y = static_cast<uint16_t>(pos);
                }
                --pos;
            }
            while (minY == ep[-1].m_value) {
                uint32_t otherId = ep[-1].m_nodeIndex;
                if (id >= otherId) {
                    break;
                }
                *ep = ep[-1];
                --ep;
                nodes[otherId].min_y = static_cast<uint16_t>(pos);
                --pos;
            }
            ep->m_nodeIndex = static_cast<uint16_t>(id);
            ep->m_value = static_cast<uint16_t>(minY);
            node->min_y = static_cast<uint16_t>(pos);

            // y: the max end moving up.
            pos = node->max_y;
            ep = &m_axis[1].m_endPoints.m_data[pos];
            for (value = ep[1].m_value; maxY > value; value = ep[1].m_value) {
                *ep = ep[1];
                hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
                ++ep;
                ++pos;
                if (value & 1) {
                    --other->max_y;
                }
                else {
                    --other->min_y;
                    if (!node->xzDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlap(*node, *other, newCollisionPairs);
                    }
                }
            }
            while (maxY == ep[1].m_value) {
                uint32_t otherId = ep[1].m_nodeIndex;
                if (id <= otherId) {
                    break;
                }
                if (otherId == 0) {
                    break;
                }
                *ep = ep[1];
                ++ep;
                --nodes[otherId].max_y;
                ++pos;
            }
            // y: the max end moving down.
            for (value = ep[-1].m_value; maxY < value; value = ep[-1].m_value) {
                *ep = ep[-1];
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                --ep;
                --pos;
                if (value & 1) {
                    ++other->max_y;
                }
                else {
                    ++other->min_y;
                    if (!node->xzDisjoint(*other)) {
                        hkp3AxisSweep::endOverlap(*node, *other, delPairs);
                    }
                }
            }
            while (maxY == ep[-1].m_value) {
                uint32_t otherId = ep[-1].m_nodeIndex;
                if (id >= otherId) {
                    break;
                }
                *ep = ep[-1];
                ++nodes[otherId].max_y;
                --ep;
                --pos;
            }
            node->max_y = static_cast<uint16_t>(pos);
            ep->m_value = static_cast<uint16_t>(maxY);
            ep->m_nodeIndex = static_cast<uint16_t>(id);

            // y: the min end moving up.
            pos = node->min_y;
            ep = &m_axis[1].m_endPoints.m_data[pos];
            for (value = ep[1].m_value; minY > value; value = ep[1].m_value) {
                hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
                *ep = ep[1];
                ++ep;
                ++pos;
                if (value & 1) {
                    --other->max_y;
                    if (!node->xzDisjoint(*other)) {
                        hkp3AxisSweep::endOverlap(*node, *other, delPairs);
                    }
                }
                else {
                    --other->min_y;
                }
            }
            while (minY == ep[1].m_value) {
                uint32_t otherId = ep[1].m_nodeIndex;
                if (id <= otherId) {
                    break;
                }
                *ep = ep[1];
                ++ep;
                --nodes[otherId].min_y;
                ++pos;
            }
            node->min_y = static_cast<uint16_t>(pos);
            ep->m_nodeIndex = static_cast<uint16_t>(id);
            ep->m_value = static_cast<uint16_t>(minY);

            // z: the min end moving down.
            pos = node->min_z;
            ep = &m_axis[2].m_endPoints.m_data[pos];
            for (value = ep[-1].m_value; minZ < value; value = ep[-1].m_value) {
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                *ep = ep[-1];
                --ep;
                if (value & 1) {
                    other->max_z = static_cast<uint16_t>(pos);
                    if (!node->xyDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlap(*node, *other, newCollisionPairs);
                    }
                }
                else {
                    other->min_z = static_cast<uint16_t>(pos);
                }
                --pos;
            }
            while (minZ == ep[-1].m_value) {
                uint32_t otherId = ep[-1].m_nodeIndex;
                if (id >= otherId) {
                    break;
                }
                *ep = ep[-1];
                --ep;
                nodes[otherId].min_z = static_cast<uint16_t>(pos);
                --pos;
            }
            ep->m_nodeIndex = static_cast<uint16_t>(id);
            ep->m_value = static_cast<uint16_t>(minZ);
            node->min_z = static_cast<uint16_t>(pos);

            // z: the max end moving up.
            pos = node->max_z;
            ep = &m_axis[2].m_endPoints.m_data[pos];
            for (value = ep[1].m_value; maxZ > value; value = ep[1].m_value) {
                *ep = ep[1];
                hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
                ++ep;
                ++pos;
                if (value & 1) {
                    --other->max_z;
                }
                else {
                    --other->min_z;
                    if (!node->xyDisjoint(*other)) {
                        hkp3AxisSweep::beginOverlap(*node, *other, newCollisionPairs);
                    }
                }
            }
            while (maxZ == ep[1].m_value) {
                uint32_t otherId = ep[1].m_nodeIndex;
                if (id <= otherId) {
                    break;
                }
                if (otherId == 0) {
                    break;
                }
                *ep = ep[1];
                ++ep;
                --nodes[otherId].max_z;
                ++pos;
            }
            // z: the max end moving down.
            for (value = ep[-1].m_value; maxZ < value; value = ep[-1].m_value) {
                *ep = ep[-1];
                hkpBpNode* other = &nodes[ep[-1].m_nodeIndex];
                --ep;
                --pos;
                if (value & 1) {
                    ++other->max_z;
                }
                else {
                    ++other->min_z;
                    if (!node->xyDisjoint(*other)) {
                        hkp3AxisSweep::endOverlap(*node, *other, delPairs);
                    }
                }
            }
            while (maxZ == ep[-1].m_value) {
                uint32_t otherId = ep[-1].m_nodeIndex;
                if (id >= otherId) {
                    break;
                }
                *ep = ep[-1];
                ++nodes[otherId].max_z;
                --ep;
                --pos;
            }
            node->max_z = static_cast<uint16_t>(pos);
            ep->m_value = static_cast<uint16_t>(maxZ);
            ep->m_nodeIndex = static_cast<uint16_t>(id);

            // z: the min end moving up.
            pos = node->min_z;
            ep = &m_axis[2].m_endPoints.m_data[pos];
            for (value = ep[1].m_value; minZ > value; value = ep[1].m_value) {
                hkpBpNode* other = &nodes[ep[1].m_nodeIndex];
                *ep = ep[1];
                ++ep;
                ++pos;
                if (value & 1) {
                    --other->max_z;
                    if (!node->xyDisjoint(*other)) {
                        hkp3AxisSweep::endOverlap(*node, *other, delPairs);
                    }
                }
                else {
                    --other->min_z;
                }
            }
            while (minZ == ep[1].m_value) {
                uint32_t otherId = ep[1].m_nodeIndex;
                if (id <= otherId) {
                    break;
                }
                *ep = ep[1];
                ++ep;
                --nodes[otherId].min_z;
                ++pos;
            }
            node->min_z = static_cast<uint16_t>(pos);
            ep->m_nodeIndex = static_cast<uint16_t>(id);
            ep->m_value = static_cast<uint16_t>(minZ);
        }
    }

    // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::~hkArray
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    delPairs.m_size = 0;
    if (delPairs.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            delPairs.m_data,
            static_cast<int32_t>(sizeof(hkpBroadPhaseHandlePair)) *
                static_cast<int32_t>(delPairs.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)
        );
    }
    delPairs.m_data = nullptr;
    delPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
    // Fetches the router once more and drops the result, as the game does.
    hkMemoryRouter::getInstance();
}

// Adds the objects of addObjectList, their AABBs in addAabbList, in one batch. Each new node's quantized end points
// (min even, max odd) go to three per-axis arrays, which are sorted, appended to the axes (insertTail) and merged into
// place. Before that, the old nodes overlapping the batch's total AABB are collected: a bit field of the old nodes is
// swept along x up to the total AABB's max (from the marker below its min when there are markers, as
// setBitsBasedOnXInterval does), and the nodes set there that overlap it on y and z, markers left out, are kept. The
// new nodes, sorted by their min x end point, are then collided among themselves and with those, sorted the same way,
// into newPairs. The temp arrays are hkArray<T, hkContainerTempAllocator> of a given size.
void hkp3AxisSweep::addObjectBatch(
    const hkArrayBase<hkpBroadPhaseHandle*>& addObjectList,
    const hkArrayBase<hkAabb>& addAabbList,
    hkArray<hkpBroadPhaseHandlePair>& newPairs
) {
    if (addAabbList.m_size < 1) {
        return;
    }
    const int32_t numA = addObjectList.m_size;
    const int32_t oldNumNodes = m_nodes.m_size;
    const int32_t numNewEndPoints = 2 * numA;
    const int32_t kDontDeallocate = static_cast<int32_t>(hkArrayBase<hkpBpEndPoint>::DONT_DEALLOCATE_FLAG);

    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpEndPoint, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator* pTemp0 = hkMemoryRouter::getInstance().m_temp;
    hkpBpEndPoint* newEndPoints0 = nullptr;
    int32_t capacity0 = 0;
    if (numNewEndPoints) {
        alignas(16) int32_t numBytes0 = numNewEndPoints * static_cast<int32_t>(sizeof(hkpBpEndPoint));
        newEndPoints0 = static_cast<hkpBpEndPoint*>(pTemp0->bufAlloc(numBytes0));
        capacity0 = numBytes0 / static_cast<int32_t>(sizeof(hkpBpEndPoint));
    }
    if (!capacity0) {
        capacity0 = kDontDeallocate;
    }
    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpEndPoint, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator* pTemp1 = hkMemoryRouter::getInstance().m_temp;
    hkpBpEndPoint* newEndPoints1 = nullptr;
    int32_t capacity1 = 0;
    if (numNewEndPoints) {
        int32_t numBytes1 = numNewEndPoints * static_cast<int32_t>(sizeof(hkpBpEndPoint));
        newEndPoints1 = static_cast<hkpBpEndPoint*>(pTemp1->bufAlloc(numBytes1));
        capacity1 = numBytes1 / static_cast<int32_t>(sizeof(hkpBpEndPoint));
    }
    if (!capacity1) {
        capacity1 = kDontDeallocate;
    }
    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpEndPoint, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator* pTemp2 = hkMemoryRouter::getInstance().m_temp;
    hkpBpEndPoint* newEndPoints2 = nullptr;
    int32_t capacity2 = 0;
    if (numNewEndPoints) {
        alignas(16) int32_t numBytes2 = numNewEndPoints * static_cast<int32_t>(sizeof(hkpBpEndPoint));
        newEndPoints2 = static_cast<hkpBpEndPoint*>(pTemp2->bufAlloc(numBytes2));
        capacity2 = numBytes2 / static_cast<int32_t>(sizeof(hkpBpEndPoint));
    }
    if (!capacity2) {
        capacity2 = kDontDeallocate;
    }

    // The old nodes overlapping the batch's total AABB.
    hkArray<const hkpBpNode*, hkContainerTempAllocator> overlaps;
    overlaps.reserve(oldNumNodes);

    // The new nodes take their handles; their end point indices come with the merge below.
    {
        // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode>::expandBy
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        int32_t newSize = m_nodes.m_size + numA;
        int32_t capacity = m_nodes.m_capacityAndFlags & hkArrayBase<hkpBpNode>::CAPACITY_MASK;
        if (capacity < newSize) {
            int32_t reqElem = 2 * capacity;
            if (newSize >= reqElem) {
                reqElem = newSize;
            }
            hkArrayUtil::_reserve(pHeap, &m_nodes, reqElem, sizeof(hkpBpNode));
        }
        m_nodes.m_size += numA;
    }

    // The batch's total AABB starts empty (+HK_REAL_MAX min, -HK_REAL_MAX max).
    __m128 totalMin = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
    __m128 totalMax = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
    hkpBpEndPoint* newEndPoints[3] = {newEndPoints0, newEndPoints1, newEndPoints2};
    const __m128 zero = _mm_setzero_ps();
    for (int32_t i = 0; i < numA; i++) {
        const hkAabb& aabb = addAabbList.m_data[i];
        totalMin = _mm_min_ps(totalMin, aabb.m_min.m_quad);
        totalMax = _mm_max_ps(totalMax, aabb.m_max.m_quad);
        __m128 quantMin = _mm_add_ps(
            _mm_max_ps(
                _mm_min_ps(
                    _mm_mul_ps(_mm_add_ps(aabb.m_min.m_quad, m_offsetLow.m_quad), m_scale.m_quad),
                    hkp3AxisSweep_quantizationMax
                ),
                zero
            ),
            convertToUint16WithClipMagicQuad
        );
        __m128 quantMax = _mm_add_ps(
            _mm_max_ps(
                _mm_min_ps(
                    _mm_mul_ps(_mm_add_ps(aabb.m_max.m_quad, m_offsetHigh.m_quad), m_scale.m_quad),
                    hkp3AxisSweep_quantizationMax
                ),
                zero
            ),
            convertToUint16WithClipMagicQuad
        );
        // The quantized value is the float's bits shifted right by 7, cut to 16 bits; a min end clears the low bit,
        // a max end sets it.
        uint16_t minX = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMin[0]) >> 7 & 0xFFFE);
        uint16_t minY = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMin[1]) >> 7 & 0xFFFE);
        uint16_t minZ = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMin[2]) >> 7 & 0xFFFE);
        uint16_t maxX = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[0]) >> 7) | 1;
        uint16_t maxY = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[1]) >> 7) | 1;
        uint16_t maxZ = static_cast<uint16_t>(std::bit_cast<uint32_t>(quantMax[2]) >> 7) | 1;

        hkpBroadPhaseHandle* handle = addObjectList.m_data[i];
        int32_t nodeIndex = oldNumNodes + i;
        m_nodes.m_data[nodeIndex].m_handle = handle;
        handle->m_id = static_cast<uint32_t>(nodeIndex);
        uint16_t index = static_cast<uint16_t>(nodeIndex);
        newEndPoints0[2 * i].m_value = minX;
        newEndPoints0[2 * i].m_nodeIndex = index;
        newEndPoints0[2 * i + 1].m_value = maxX;
        newEndPoints0[2 * i + 1].m_nodeIndex = index;
        newEndPoints1[2 * i].m_value = minY;
        newEndPoints1[2 * i].m_nodeIndex = index;
        newEndPoints1[2 * i + 1].m_value = maxY;
        newEndPoints1[2 * i + 1].m_nodeIndex = index;
        newEndPoints2[2 * i].m_value = minZ;
        newEndPoints2[2 * i].m_nodeIndex = index;
        newEndPoints2[2 * i + 1].m_value = maxZ;
        newEndPoints2[2 * i + 1].m_nodeIndex = index;
    }
    m_nodes.m_size = oldNumNodes;

    // A bit per old node: (oldNumNodes >> 5) + 8 words.
    // EXE-INLINED none: hkLocalBuffer<hkUint32>::hkLocalBuffer
    hkMemoryAllocator* pTempBits = hkMemoryRouter::getInstance().m_temp;
    int32_t numWords = (oldNumNodes >> 5) + 8;
    uint32_t* bitField;
    int32_t bitFieldCapacity;
    if (numWords == 0) {
        bitField = nullptr;
        bitFieldCapacity = kDontDeallocate;
    }
    else {
        alignas(16) int32_t numBytes = numWords * static_cast<int32_t>(sizeof(uint32_t));
        bitField = static_cast<uint32_t*>(pTempBits->bufAlloc(numBytes));
        bitFieldCapacity = numBytes / static_cast<int32_t>(sizeof(uint32_t));
        if (bitFieldCapacity == 0) {
            bitFieldCapacity = kDontDeallocate;
        }
    }
    // EXE-INLINED 0xD11000
    // memclear16(bitField, oldNumNodes >> 3): the exe clears with movaps stores of a zero register, 16 bytes (four
    // words) a store.
    uint32_t* pClear = bitField;
    for (int32_t i = oldNumNodes >> 7; i >= 0; i--) {
        __builtin_memset(pClear, 0, sizeof(uint32_t) * 4);
        pClear += 4;
    }

    __m128 totalQuantMin = _mm_add_ps(
        _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(m_offsetLow.m_quad, totalMin), m_scale.m_quad), hkp3AxisSweep_quantizationMax
            ),
            zero
        ),
        convertToUint16WithClipMagicQuad
    );
    __m128 totalQuantMax = _mm_add_ps(
        _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(m_offsetHigh.m_quad, totalMax), m_scale.m_quad), hkp3AxisSweep_quantizationMax
            ),
            zero
        ),
        convertToUint16WithClipMagicQuad
    );
    uint32_t totalMinX = std::bit_cast<uint32_t>(totalQuantMin[0]) >> 7 & 0xFFFE;
    uint32_t totalMinY = std::bit_cast<uint32_t>(totalQuantMin[1]) >> 7 & 0xFFFE;
    uint32_t totalMinZ = std::bit_cast<uint32_t>(totalQuantMin[2]) >> 7 & 0xFFFE;
    uint32_t totalMaxX = static_cast<uint16_t>(std::bit_cast<uint32_t>(totalQuantMax[0]) >> 7) | 1;
    uint32_t totalMaxY = static_cast<uint16_t>(std::bit_cast<uint32_t>(totalQuantMax[1]) >> 7) | 1;
    uint32_t totalMaxZ = static_cast<uint16_t>(std::bit_cast<uint32_t>(totalQuantMax[2]) >> 7) | 1;

    // The sweep along x: from the marker below the total AABB's min, its node and its overlaps set and the nodes
    // it contains cleared, or else from the first end point; every end point below the min flips its node, and
    // every min end point below the max sets its node.
    const hkpBpEndPoint* ep = m_axis[0].m_endPoints.m_data + 1;
    if (m_numMarkers) {
        int32_t markerIndex = static_cast<int32_t>(totalMinX) >> (16 - m_ld2NumMarkers);
        if (markerIndex > 0) {
            const hkpBpMarker& marker = m_markers[markerIndex - 1];
            // EXE-INLINED 0xD10AD0
            bitField[marker.m_nodeIndex >> 5] ^= 1u << (marker.m_nodeIndex & 31);
            const uint16_t* pOverlap = marker.m_overlappingObjects.m_data;
            for (int32_t j = marker.m_overlappingObjects.m_size - 1; j >= 0; j--) {
                int32_t overlap = *pOverlap++;
                // EXE-INLINED 0xD10AD0
                bitField[overlap >> 5] ^= 1u << (overlap & 31);
            }
            const hkpBpNode& markerNode = m_nodes.m_data[marker.m_nodeIndex];
            const hkpBpEndPoint* pEndPoints = m_axis[0].m_endPoints.m_data;
            const hkpBpEndPoint* pEnd = &pEndPoints[markerNode.max_x];
            for (const hkpBpEndPoint* p = &pEndPoints[markerNode.min_x + 1]; p < pEnd; p++) {
                if (!p->isMaxPoint()) {
                    // EXE-INLINED 0xD10AF0
                    bitField[p->m_nodeIndex >> 5] &= ~(1u << (p->m_nodeIndex & 31));
                }
            }
            ep = &m_axis[0].m_endPoints.m_data[markerNode.min_x + 1];
        }
    }
    while (ep->m_value < totalMinX) {
        // EXE-INLINED 0xD10AD0
        bitField[ep->m_nodeIndex >> 5] ^= 1u << (ep->m_nodeIndex & 31);
        ep++;
    }
    while (ep->m_value < totalMaxX) {
        if (!ep->isMaxPoint()) {
            // EXE-INLINED 0xD10AD0
            bitField[ep->m_nodeIndex >> 5] ^= 1u << (ep->m_nodeIndex & 31);
        }
        ep++;
    }

    // The total AABB as a node of y and z end point positions: the min's position is the first end point not below
    // it, the max's the last one below it.
    hkpBpNode queryNode;
    const hkpBpEndPoint* pEndPoints1 = m_axis[1].m_endPoints.m_data;
    const hkpBpEndPoint* pEnd1 = &pEndPoints1[m_axis[1].m_endPoints.m_size - 2];
    queryNode.min_y =
        static_cast<uint16_t>(m_axis[1].find(pEndPoints1 + 1, pEnd1, static_cast<uint16_t>(totalMinY)) - pEndPoints1);
    queryNode.max_y = static_cast<uint16_t>(
        m_axis[1].find(pEndPoints1 + 1, pEnd1, static_cast<uint16_t>(totalMaxY)) - pEndPoints1 - 1
    );
    const hkpBpEndPoint* pEndPoints2 = m_axis[2].m_endPoints.m_data;
    const hkpBpEndPoint* pEnd2 = &pEndPoints2[m_axis[2].m_endPoints.m_size - 2];
    queryNode.min_z =
        static_cast<uint16_t>(m_axis[2].find(pEndPoints2 + 1, pEnd2, static_cast<uint16_t>(totalMinZ)) - pEndPoints2);
    queryNode.max_z = static_cast<uint16_t>(
        m_axis[2].find(pEndPoints2 + 1, pEnd2, static_cast<uint16_t>(totalMaxZ)) - pEndPoints2 - 1
    );

    // The set nodes overlapping the total AABB on y and z, markers left out, four bits at a time.
    const hkpBpNode* wordNodes = m_nodes.m_data;
    const uint32_t* wordEnd = bitField + (m_nodes.m_size >> 5) + 1;
    for (const uint32_t* pWord = bitField; pWord < wordEnd; pWord++, wordNodes += 32) {
        uint32_t bits = *pWord;
        const hkpBpNode* pNodes = wordNodes;
        while (bits) {
            if (bits & 0xF) {
                for (int32_t k = 0; k < 4; k++) {
                    const hkpBpNode* pNode = &pNodes[k];
                    if ((bits & (1u << k)) && !pNode->yzDisjoint(queryNode) && !pNode->isMarker()) {
                        overlaps.pushBackUnchecked(pNode);
                    }
                }
            }
            bits >>= 4;
            pNodes += 4;
        }
    }

    // EXE-INLINED none: hkLocalBuffer<hkUint32>::~hkLocalBuffer
    hkMemoryAllocator* pTempBitsFree = hkMemoryRouter::getInstance().m_temp;
    if (bitFieldCapacity >= 0) {
        pTempBitsFree->bufFree(
            bitField, (bitFieldCapacity & hkArrayBase<uint32_t>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(uint32_t))
        );
    }

    {
        // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode>::expandBy
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        int32_t newSize = m_nodes.m_size + numA;
        int32_t capacity = m_nodes.m_capacityAndFlags & hkArrayBase<hkpBpNode>::CAPACITY_MASK;
        if (capacity < newSize) {
            int32_t reqElem = 2 * capacity;
            if (newSize >= reqElem) {
                reqElem = newSize;
            }
            hkArrayUtil::_reserve(pHeap, &m_nodes, reqElem, sizeof(hkpBpNode));
        }
        m_nodes.m_size += numA;
    }
    hkpBpNode* nodes = m_nodes.m_data;

    for (int32_t k = 0; k < 3; k++) {
        // EXE-INLINED none: hkAlgorithm::quickSort<hkp3AxisSweep::hkpBpEndPoint>
        if (numNewEndPoints > 1) {
            hkAlgorithm::quickSortRecursive<hkpBpEndPoint, hkAlgorithm::less<hkpBpEndPoint>>(
                newEndPoints[k], 0, numNewEndPoints - 1, hkAlgorithm::less<hkpBpEndPoint>()
            );
        }
    }
    int32_t insertIndex[3];
    for (int32_t k = 0; k < 3; k++) {
        insertIndex[k] = m_axis[k].insertTail(nodes, k, newEndPoints[k], numNewEndPoints);
    }
    m_axis[0].mergeBatchX(nodes, insertIndex[0], newEndPoints0, numNewEndPoints);
    m_axis[1].mergeBatchY(nodes, insertIndex[1], newEndPoints1, numNewEndPoints);
    m_axis[2].mergeBatchZ(nodes, insertIndex[2], newEndPoints2, numNewEndPoints);

    // The markers' max ends on y and z lie above the new end points.
    if (m_numMarkers) {
        for (int32_t i = 0; i < m_numMarkers; i++) {
            hkpBpNode& markerNode = m_nodes.m_data[m_markers[i].m_nodeIndex];
            markerNode.max_y = static_cast<uint16_t>(markerNode.max_y + numNewEndPoints);
            markerNode.max_z = static_cast<uint16_t>(markerNode.max_z + numNewEndPoints);
        }
    }

    // The new nodes in min x order, a node of min x 0xFFFF after them.
    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator* pTempA = hkMemoryRouter::getInstance().m_temp;
    hkpBpNode* nodesA = nullptr;
    int32_t capacityA = 0;
    if (numA + 1) {
        int32_t numBytesA = (numA + 1) * static_cast<int32_t>(sizeof(hkpBpNode));
        nodesA = static_cast<hkpBpNode*>(pTempA->bufAlloc(numBytesA));
        capacityA = numBytesA / static_cast<int32_t>(sizeof(hkpBpNode));
    }
    if (!capacityA) {
        capacityA = kDontDeallocate;
    }
    {
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator* pTempSort = hkMemoryRouter::getInstance().m_temp;
        hkRadixSort::SortData16* sortData = nullptr;
        int32_t sortDataCapacity = 0;
        if (numA + 4) {
            alignas(16) int32_t numBytes = (numA + 4) * static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
            sortData = static_cast<hkRadixSort::SortData16*>(pTempSort->bufAlloc(numBytes));
            sortDataCapacity = numBytes / static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
        }
        if (!sortDataCapacity) {
            sortDataCapacity = kDontDeallocate;
        }
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator* pTempBuffer = hkMemoryRouter::getInstance().m_temp;
        hkRadixSort::SortData16* buffer = nullptr;
        int32_t bufferCapacity = 0;
        if (numA + 4) {
            int32_t numBytes = (numA + 4) * static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
            buffer = static_cast<hkRadixSort::SortData16*>(pTempBuffer->bufAlloc(numBytes));
            bufferCapacity = numBytes / static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
        }
        if (!bufferCapacity) {
            bufferCapacity = kDontDeallocate;
        }
        for (int32_t i = 0; i < numA; i++) {
            sortData[i].m_userData = static_cast<uint16_t>(oldNumNodes + i);
            sortData[i].m_key = m_nodes.m_data[oldNumNodes + i].min_x;
        }
        // Keys above every end point's pad the sort to a multiple of four.
        sortData[numA + 1].m_key = 0xFFFF;
        sortData[numA].m_key = 0xFFFF;
        sortData[numA + 2].m_key = 0xFFFF;
        hkRadixSort::sort16(sortData, (numA + 3) & ~3, buffer);
        for (int32_t i = 0; i < numA; i++) {
            nodesA[i] = m_nodes.m_data[sortData[i].m_userData];
        }
        nodesA[numA].min_x = 0xFFFF;
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator* pTempBufferFree = hkMemoryRouter::getInstance().m_temp;
        if ((bufferCapacity & hkArrayBase<hkRadixSort::SortData16>::DONT_DEALLOCATE_FLAG) == 0) {
            pTempBufferFree->bufFree(
                buffer,
                (bufferCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkRadixSort::SortData16))
            );
        }
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator* pTempSortFree = hkMemoryRouter::getInstance().m_temp;
        if (sortDataCapacity >= 0) {
            pTempSortFree->bufFree(
                sortData,
                (sortDataCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkRadixSort::SortData16))
            );
        }
    }

    // The overlapping old nodes in min x order, a node of min x 0xFFFF after them.
    const int32_t numB = overlaps.m_size;
    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator* pTempB = hkMemoryRouter::getInstance().m_temp;
    hkpBpNode* nodesB = nullptr;
    int32_t capacityB = 0;
    if (numB + 1) {
        int32_t numBytesB = (numB + 1) * static_cast<int32_t>(sizeof(hkpBpNode));
        nodesB = static_cast<hkpBpNode*>(pTempB->bufAlloc(numBytesB));
        capacityB = numBytesB / static_cast<int32_t>(sizeof(hkpBpNode));
    }
    if (!capacityB) {
        capacityB = kDontDeallocate;
    }
    {
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator* pTempSort = hkMemoryRouter::getInstance().m_temp;
        hkRadixSort::SortData16* sortData = nullptr;
        int32_t sortDataCapacity = 0;
        if (numB + 4) {
            int32_t numBytes = (numB + 4) * static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
            sortData = static_cast<hkRadixSort::SortData16*>(pTempSort->bufAlloc(numBytes));
            sortDataCapacity = numBytes / static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
        }
        if (!sortDataCapacity) {
            sortDataCapacity = kDontDeallocate;
        }
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator* pTempBuffer = hkMemoryRouter::getInstance().m_temp;
        hkRadixSort::SortData16* buffer = nullptr;
        int32_t bufferCapacity = 0;
        if (numB + 4) {
            alignas(16) int32_t numBytes = (numB + 4) * static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
            buffer = static_cast<hkRadixSort::SortData16*>(pTempBuffer->bufAlloc(numBytes));
            bufferCapacity = numBytes / static_cast<int32_t>(sizeof(hkRadixSort::SortData16));
        }
        if (!bufferCapacity) {
            bufferCapacity = kDontDeallocate;
        }
        const hkpBpNode* nodesBase = m_nodes.m_data;
        for (int32_t i = 0; i < numB; i++) {
            const hkpBpNode* pNode = overlaps.m_data[i];
            sortData[i].m_userData = static_cast<uint16_t>(pNode - nodesBase);
            sortData[i].m_key = pNode->min_x;
        }
        // Keys above every end point's pad the sort to a multiple of four.
        sortData[numB + 1].m_key = 0xFFFF;
        sortData[numB].m_key = 0xFFFF;
        sortData[numB + 2].m_key = 0xFFFF;
        hkRadixSort::sort16(sortData, (numB + 3) & ~3, buffer);
        for (int32_t i = 0; i < numB; i++) {
            nodesB[i] = m_nodes.m_data[sortData[i].m_userData];
        }
        nodesB[numB].min_x = 0xFFFF;
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator* pTempBufferFree = hkMemoryRouter::getInstance().m_temp;
        if ((bufferCapacity & hkArrayBase<hkRadixSort::SortData16>::DONT_DEALLOCATE_FLAG) == 0) {
            pTempBufferFree->bufFree(
                buffer,
                (bufferCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkRadixSort::SortData16))
            );
        }
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator* pTempSortFree = hkMemoryRouter::getInstance().m_temp;
        if ((sortDataCapacity & hkArrayBase<hkRadixSort::SortData16>::DONT_DEALLOCATE_FLAG) == 0) {
            pTempSortFree->bufFree(
                sortData,
                (sortDataCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkRadixSort::SortData16))
            );
        }
    }

    collide1Axis(nodesA, numA, newPairs);
    collide1Axis(nodesA, numA, nodesB, numB, MARKERS_ADD_NEW_OVERLAPS, newPairs);

    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>
    hkMemoryAllocator* pTempBFree = hkMemoryRouter::getInstance().m_temp;
    if (capacityB >= 0) {
        pTempBFree->bufFree(
            nodesB, (capacityB & hkArrayBase<hkpBpNode>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpNode))
        );
    }
    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>
    hkMemoryAllocator* pTempAFree = hkMemoryRouter::getInstance().m_temp;
    if (capacityA >= 0) {
        pTempAFree->bufFree(
            nodesA, (capacityA & hkArrayBase<hkpBpNode>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpNode))
        );
    }

    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpEndPoint, hkContainerTempAllocator>
    hkMemoryAllocator* pTemp2Free = hkMemoryRouter::getInstance().m_temp;
    if (capacity2 >= 0) {
        pTemp2Free->bufFree(
            newEndPoints2,
            (capacity2 & hkArrayBase<hkpBpEndPoint>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpEndPoint))
        );
    }
    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpEndPoint, hkContainerTempAllocator>
    hkMemoryAllocator* pTemp1Free = hkMemoryRouter::getInstance().m_temp;
    if (capacity1 >= 0) {
        pTemp1Free->bufFree(
            newEndPoints1,
            (capacity1 & hkArrayBase<hkpBpEndPoint>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpEndPoint))
        );
    }
    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpEndPoint, hkContainerTempAllocator>
    hkMemoryAllocator* pTemp0Free = hkMemoryRouter::getInstance().m_temp;
    if (capacity0 >= 0) {
        pTemp0Free->bufFree(
            newEndPoints0,
            (capacity0 & hkArrayBase<hkpBpEndPoint>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(hkpBpEndPoint))
        );
    }
}

int32_t getMinComponent(float* a1, float* a2) {
    if (!(a1[1] > ((double)(*a1)))) {
        if (a1[2] > ((double)a1[1])) {
            *a2 = a1[1];
            return 1;
        }
    }
    else {
        if (a1[2] > ((double)(*a1))) {
            *a2 = (*a1);
            return 0;
        }
    }
    *a2 = a1[2];
    return 2;
}

int32_t max2Uint16(uint16_t a1, uint16_t a2) {
    int32_t result = (int32_t)a1;
    if (((uint32_t)((int32_t)a1)) <= ((uint32_t)((int32_t)a2))) {
        return (int32_t)a2;
    }
    return result;
}

// Removes the objects of removeObjectList in one batch. Their nodes, sorted by min x end point, are copied out
// (with their total AABB) and marked by a local handle's address in their handle field; the node array is then
// compacted, the last kept node moving into each removed node's place, and the kept nodes overlapping the total AABB
// are collected and sorted the same way. The pairs among the removed nodes and between them and those go to
// delPairsOut, the axes drop the removed end points and renumber the rest (removeBatch), and the markers' overlap
// lists follow the new node indices. The temp arrays are the SDK's hkArray<T, hkContainerTempAllocator> of a given
// size (or empty, for movedNodes), constructed and destroyed inline.
// The removed nodes the compaction leaves above the new size keep the local handle's address, a stack address of
// this frame.
void hkp3AxisSweep::removeObjectBatch(
    const hkArrayBase<hkpBroadPhaseHandle*>& removeObjectList,
    hkArray<hkpBroadPhaseHandlePair>& delPairsOut
) {
    const int32_t numNodes = m_nodes.m_size;
    const int32_t numA = removeObjectList.m_size;
    const int32_t kDontDeallocate = static_cast<int32_t>(hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);
    // The handle whose address marks a removed node.
    hkpBroadPhaseHandle removedHandle;
    removedHandle.m_id = 0;

    // The removed nodes in min x order, a node of min x 0xFFFF after them.
    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator& tempA = hkMemoryRouter::getInstance().temp();
    int32_t capacityA = numA + 1;
    hkpBpNode* nodesA = capacityA ? tempA._bufAlloc<hkpBpNode>(capacityA) : nullptr;
    if (!capacityA) {
        capacityA = kDontDeallocate;
    }

    // The removed nodes' total AABB, started from the first one's node.
    hkpBpNode total = m_nodes.m_data[removeObjectList.m_data[0]->m_id];
    {
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator& tempSort = hkMemoryRouter::getInstance().temp();
        int32_t sortDataCapacity = numA + 4;
        hkRadixSort::SortData16* sortData =
            sortDataCapacity ? tempSort._bufAlloc<hkRadixSort::SortData16>(sortDataCapacity) : nullptr;
        if (!sortDataCapacity) {
            sortDataCapacity = kDontDeallocate;
        }
        for (int32_t i = 0; i < numA; i++) {
            const uint32_t id = removeObjectList.m_data[i]->m_id;
            const hkpBpNode& node = m_nodes.m_data[id];
            sortData[i].m_userData = static_cast<uint16_t>(id);
            sortData[i].m_key = node.min_x;
            if (node.min_x <= total.min_x) {
                total.min_x = node.min_x;
            }
            if (node.min_y <= total.min_y) {
                total.min_y = node.min_y;
            }
            if (node.min_z <= total.min_z) {
                total.min_z = node.min_z;
            }
            if (node.max_x >= total.max_x) {
                total.max_x = node.max_x;
            }
            if (node.max_y >= total.max_y) {
                total.max_y = node.max_y;
            }
            if (node.max_z >= total.max_z) {
                total.max_z = node.max_z;
            }
        }
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator& tempBuffer = hkMemoryRouter::getInstance().temp();
        int32_t bufferCapacity = numA + 4;
        hkRadixSort::SortData16* buffer =
            bufferCapacity ? tempBuffer._bufAlloc<hkRadixSort::SortData16>(bufferCapacity) : nullptr;
        if (!bufferCapacity) {
            bufferCapacity = kDontDeallocate;
        }
        // Keys above every end point's pad the sort to a multiple of four.
        sortData[numA + 1].m_key = 0xFFFF;
        sortData[numA].m_key = 0xFFFF;
        sortData[numA + 2].m_key = 0xFFFF;
        hkRadixSort::sort16(sortData, (numA + 3) & ~3, buffer);
        for (int32_t i = 0; i < numA; i++) {
            hkpBpNode& node = m_nodes.m_data[sortData[i].m_userData];
            nodesA[i] = node;
            node.m_handle->m_id = 0;
            node.m_handle = &removedHandle;
        }
        nodesA[numA].min_x = 0xFFFF;
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator& tempBufferFree = hkMemoryRouter::getInstance().temp();
        if (bufferCapacity >= 0) {
            tempBufferFree._bufFree<hkRadixSort::SortData16>(
                buffer, bufferCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK
            );
        }
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator& tempSortFree = hkMemoryRouter::getInstance().temp();
        if (sortDataCapacity >= 0) {
            tempSortFree._bufFree<hkRadixSort::SortData16>(
                sortData, sortDataCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK
            );
        }
    }

    // The kept nodes moved into a removed node's place, for the deterministic order fix at the end.
    using TempIntArray = hkArray<int32_t, hkContainerTempAllocator>;
    TempIntArray movedNodes;
    movedNodes.reserve(m_numMarkers + numA);

    // Each old node's new index, -1 for a removed one.
    TempIntArray newIndices(numNodes);
    int32_t* newIndex = newIndices.m_data;

    // The kept nodes overlapping the total AABB, keyed by min x.
    int32_t numB = 0;
    // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator& tempSortB = hkMemoryRouter::getInstance().temp();
    int32_t sortDataBCapacity = numNodes - numA + 4;
    hkRadixSort::SortData16* sortDataB =
        sortDataBCapacity ? tempSortB._bufAlloc<hkRadixSort::SortData16>(sortDataBCapacity) : nullptr;
    if (!sortDataBCapacity) {
        sortDataBCapacity = kDontDeallocate;
    }

    // Node 0 keeps its place. Removed nodes at the top are dropped; a removed node below gets the top node.
    int32_t last = numNodes - 1;
    newIndex[0] = 0;
    int32_t i = 1;
    while (i <= last) {
        hkpBpNode* nodes = m_nodes.m_data;
        if (nodes[last].m_handle == &removedHandle) {
            newIndex[last] = -1;
            last--;
            continue;
        }
        hkpBpNode& node = nodes[i];
        if (node.m_handle == &removedHandle) {
            node = nodes[last];
            newIndex[last] = i;
            last--;
            newIndex[i] = -1;
            movedNodes.pushBackUnchecked(i);
            if (node.isMarker()) {
                // EXE-INLINED none: hkp3AxisSweep::hkpBpNode::getMarker
                // A marker node's handle field is no pointer: the exe treats its bits as its marker's byte offset in
                // m_markers, tagged by the low bit. A moved marker is not collected.
                hkpBpMarker* pMarker = reinterpret_cast<hkpBpMarker*>(
                    reinterpret_cast<uintptr_t>(m_markers) +
                    (reinterpret_cast<uintptr_t>(node.m_handle) & ~static_cast<uintptr_t>(1))
                );
                pMarker->m_nodeIndex = static_cast<uint16_t>(i);
                i++;
                continue;
            }
            node.m_handle->m_id = static_cast<uint32_t>(i);
        }
        else {
            newIndex[i] = i;
        }
        if (!node.yzDisjoint(total) && !node.xyDisjoint(total)) {
            sortDataB[numB].m_key = node.min_x;
            sortDataB[numB].m_userData = static_cast<uint16_t>(i);
            numB++;
        }
        i++;
    }
    m_nodes.setSize(last + 1);

    // The collected nodes in min x order, a node of min x 0xFFFF after them.
    // EXE-INLINED none: hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>::hkArray(int)
    hkMemoryAllocator& tempB = hkMemoryRouter::getInstance().temp();
    int32_t capacityB = numB + 1;
    hkpBpNode* nodesB = capacityB ? tempB._bufAlloc<hkpBpNode>(capacityB) : nullptr;
    if (!capacityB) {
        capacityB = kDontDeallocate;
    }
    {
        // EXE-INLINED none: hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>::hkArray(int)
        hkMemoryAllocator& tempBuffer = hkMemoryRouter::getInstance().temp();
        int32_t bufferCapacity = numB + 4;
        hkRadixSort::SortData16* buffer =
            bufferCapacity ? tempBuffer._bufAlloc<hkRadixSort::SortData16>(bufferCapacity) : nullptr;
        if (!bufferCapacity) {
            bufferCapacity = kDontDeallocate;
        }
        // Keys above every end point's pad the sort to a multiple of four.
        sortDataB[numB].m_key = 0xFFFF;
        sortDataB[numB + 2].m_key = 0xFFFF;
        sortDataB[numB + 1].m_key = 0xFFFF;
        hkRadixSort::sort16(sortDataB, (numB + 3) & ~3, buffer);
        for (int32_t j = 0; j < numB; j++) {
            nodesB[j] = m_nodes.m_data[sortDataB[j].m_userData];
        }
        nodesB[numB].min_x = 0xFFFF;
        // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
        hkMemoryAllocator& tempBufferFree = hkMemoryRouter::getInstance().temp();
        if (bufferCapacity >= 0) {
            tempBufferFree._bufFree<hkRadixSort::SortData16>(
                buffer, bufferCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK
            );
        }
    }

    collide1Axis(nodesA, numA, delPairsOut);
    collide1Axis(nodesA, numA, nodesB, numB, MARKERS_IGNORE, delPairsOut);
    m_axis[0].removeBatch<0>(m_nodes.m_data, newIndices);
    m_axis[1].removeBatch<1>(m_nodes.m_data, newIndices);
    m_axis[2].removeBatch<2>(m_nodes.m_data, newIndices);

    // Each marker's max end points move down by the removed end points; its overlap list keeps the kept nodes,
    // renumbered.
    if (m_numMarkers) {
        const int32_t numRemovedEndPoints = 2 * numA;
        for (int32_t m = 0; m < m_numMarkers; m++) {
            hkpBpMarker& marker = m_markers[m];
            hkpBpNode& markerNode = m_nodes.m_data[marker.m_nodeIndex];
            markerNode.max_y = static_cast<uint16_t>(markerNode.max_y - numRemovedEndPoints);
            markerNode.max_z = static_cast<uint16_t>(markerNode.max_z - numRemovedEndPoints);
            int32_t numKept = 0;
            for (int32_t j = 0; j < marker.m_overlappingObjects.m_size; j++) {
                const int32_t objectIndex = newIndex[marker.m_overlappingObjects.m_data[j]];
                if (objectIndex >= 0) {
                    marker.m_overlappingObjects.m_data[numKept++] = static_cast<uint16_t>(objectIndex);
                }
            }
            marker.m_overlappingObjects.setSize(numKept);
        }
    }

    for (int32_t nodeIndex : movedNodes) {
        _fixDeterministicOrderAfterNodeIdWasDecreased(nodeIndex);
    }

    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>
    hkMemoryAllocator& tempBFree = hkMemoryRouter::getInstance().temp();
    if (capacityB >= 0) {
        tempBFree._bufFree<hkpBpNode>(nodesB, capacityB & hkArrayBase<hkpBpNode>::CAPACITY_MASK);
    }
    // EXE-INLINED none: ~hkArray<hkRadixSort::SortData16, hkContainerTempAllocator>
    hkMemoryAllocator& tempSortBFree = hkMemoryRouter::getInstance().temp();
    if (sortDataBCapacity >= 0) {
        tempSortBFree._bufFree<hkRadixSort::SortData16>(
            sortDataB, sortDataBCapacity & hkArrayBase<hkRadixSort::SortData16>::CAPACITY_MASK
        );
    }
    // EXE-INLINED none: ~hkArray<hkp3AxisSweep::hkpBpNode, hkContainerTempAllocator>
    hkMemoryAllocator& tempAFree = hkMemoryRouter::getInstance().temp();
    if (capacityA >= 0) {
        tempAFree._bufFree<hkpBpNode>(nodesA, capacityA & hkArrayBase<hkpBpNode>::CAPACITY_MASK);
    }
}

// Hoare's partition on the end points: the pivot is the middle element's coordinate, the two sides walk to it with
// unsigned 16-bit compares, the pair found is swapped whole, and only the coordinate decides the order. The left part
// recurses, the right part is looped over.
int32_t quickSortRecursive(hkp3AxisSweep::hkpBpEndPoint* a1, int32_t a2, int32_t a3, uint32_t a4) {
    int32_t lo = a2;
    int32_t last = a3;
    // The result is whatever the last iteration left: the recursion's result when it ran, the right index otherwise.
    int32_t result = a3;
    for (;;) {
        uint16_t pivot = a1[(lo + a3) >> 1].m_value;
        int32_t left = lo;
        last = a3;
        do {
            while (a1[left].m_value < pivot) {
                ++left;
            }
            while (pivot < a1[last].m_value) {
                --last;
            }
            if (last < left) {
                break;
            }
            if (last != left) {
                hkp3AxisSweep::hkpBpEndPoint tmp = a1[last];
                a1[last] = a1[left];
                a1[left] = tmp;
            }
            --last;
            ++left;
        }
        while (left <= last);
        result = last;
        if (lo < last) {
            result = quickSortRecursive(a1, lo, last, a4);
        }
        if (left >= a3) {
            break;
        }
        lo = left;
    }
    return result;
}

// Copies out the two 32-bit offset vectors.
void hkp3AxisSweep::getOffsetLowHigh32bit(hkVector4& offsetLow32bit, hkVector4& offsetHigh32bit) const {
    offsetLow32bit = m_offsetLow32bit;
    offsetHigh32bit = m_offsetHigh32bit;
}

int32_t toggleBit(int32_t a1, uint32_t* a2) {
    a2[a1 >> 5] ^= 1 << (a1 & 31);
    return a1 >> 5;
}

int32_t clearBit(int32_t a1, uint32_t* a2) {
    a2[a1 >> 5] &= ~(1 << (a1 & 31));
    return a1 >> 5;
}

void hkp3AxisSweep::_fixDeterministicOrderAfterNodeIdWasDecreased(int32_t a1) {
    hkp3AxisSweep::hkpBpEndPoint* i;
    hkp3AxisSweep::hkpBpEndPoint* j;
    hkp3AxisSweep::hkpBpEndPoint* k;
    hkp3AxisSweep::hkpBpEndPoint* m;
    hkp3AxisSweep::hkpBpNode* m_data = m_nodes.begin();
    int32_t min_x = m_data[a1].min_x;
    hkp3AxisSweep::hkpBpNode* v5 = &m_data[a1];
    for (i = &m_axis[0].m_endPoints.m_data[min_x]; i->m_value == i[-1].m_value;
         min_x = static_cast<int32_t>(
             (static_cast<uint32_t>(min_x) & 0xFFFF0000u) | (static_cast<uint32_t>(static_cast<uint16_t>((min_x - 1))))
         )) {
        uint32_t m_nodeIndex = i[-1].m_nodeIndex;
        if (a1 >= m_nodeIndex) {
            break;
        }
        *i = i[-1];
        --i;
        m_data[m_nodeIndex].min_x = min_x;
    }
    i->m_nodeIndex = a1;
    v5->min_x = min_x;
    int32_t max_x = v5->max_x;
    hkp3AxisSweep::hkpBpEndPoint* v9 = this->m_axis[0].m_endPoints.begin();
    bool v10 = v9[max_x].m_value == v9[max_x - 1].m_value;
    hkp3AxisSweep::hkpBpEndPoint* v11 = &v9[max_x];
    if (v10) {
        do {
            uint32_t v12 = v11[-1].m_nodeIndex;
            if (a1 >= v12) {
                break;
            }
            *v11 = v11[-1];
            --v11;
            m_data[v12].max_x = max_x;
            max_x = static_cast<int32_t>(
                (static_cast<uint32_t>(max_x) & 0xFFFF0000u) |
                (static_cast<uint32_t>(static_cast<uint16_t>((max_x - 1))))
            );
        }
        while (v11->m_value == v11[-1].m_value);
    }
    v11->m_nodeIndex = a1;
    v5->max_x = max_x;
    int32_t min_y = v5->min_y;
    hkp3AxisSweep::hkpBpEndPoint* v14 = this->m_axis[1].m_endPoints.begin();
    v10 = v14[min_y].m_value == v14[min_y - 1].m_value;
    hkp3AxisSweep::hkpBpEndPoint* v15 = &v14[min_y];
    if (v10) {
        do {
            uint32_t v16 = v15[-1].m_nodeIndex;
            if (a1 >= v16) {
                break;
            }
            *v15 = v15[-1];
            --v15;
            m_data[v16].min_y = min_y;
            min_y = static_cast<int32_t>(
                (static_cast<uint32_t>(min_y) & 0xFFFF0000u) |
                (static_cast<uint32_t>(static_cast<uint16_t>((min_y - 1))))
            );
        }
        while (v15->m_value == v15[-1].m_value);
    }
    v15->m_nodeIndex = a1;
    v5->min_y = min_y;
    int32_t max_y = v5->max_y;
    for (j = &this->m_axis[1].m_endPoints.m_data[max_y]; j->m_value == j[-1].m_value;
         max_y = static_cast<int32_t>(
             (static_cast<uint32_t>(max_y) & 0xFFFF0000u) | (static_cast<uint32_t>(static_cast<uint16_t>((max_y - 1))))
         )) {
        uint32_t v19 = j[-1].m_nodeIndex;
        if (a1 >= v19) {
            break;
        }
        *j = j[-1];
        --j;
        m_data[v19].max_y = max_y;
    }
    j->m_nodeIndex = a1;
    v5->max_y = max_y;
    int32_t min_z = v5->min_z;
    for (k = &this->m_axis[2].m_endPoints.m_data[min_z]; k->m_value == k[-1].m_value;
         min_z = static_cast<int32_t>(
             (static_cast<uint32_t>(min_z) & 0xFFFF0000u) | (static_cast<uint32_t>(static_cast<uint16_t>((min_z - 1))))
         )) {
        uint32_t v22 = k[-1].m_nodeIndex;
        if (a1 >= v22) {
            break;
        }
        *k = k[-1];
        --k;
        m_data[v22].min_z = min_z;
    }
    k->m_nodeIndex = a1;
    v5->min_z = min_z;
    int32_t max_z = v5->max_z;
    for (m = &this->m_axis[2].m_endPoints.m_data[max_z]; m->m_value == m[-1].m_value;
         max_z = static_cast<int32_t>(
             (static_cast<uint32_t>(max_z) & 0xFFFF0000u) | (static_cast<uint32_t>(static_cast<uint16_t>((max_z - 1))))
         )) {
        uint32_t v25 = m[-1].m_nodeIndex;
        if (a1 >= v25) {
            break;
        }
        *m = m[-1];
        --m;
        m_data[v25].max_z = max_z;
    }
    m->m_nodeIndex = a1;
    v5->max_z = max_z;
}

// Takes out the end points at minPosition and maxPosition: those between them move down one place, those after
// maxPosition two.
void hkp3AxisSweep::hkpBpAxis::remove(int32_t minPosition, int32_t maxPosition) {
    hkpBpEndPoint* p = &m_endPoints.m_data[minPosition];
    hkpBpEndPoint* end = &m_endPoints.m_data[maxPosition - 1];
    for (; p < end; p++) {
        p[0] = p[1];
    }
    const int32_t newSize = m_endPoints.m_size - 2;
    end = &m_endPoints.m_data[newSize];
    m_endPoints.m_size = newSize;
    for (; p < end; p++) {
        p[0] = p[2];
    }
}

// Havok SDK: an overlap of a and b ends. When b is a marker (bit 0 of its handle set, the rest the byte offset of
// its hkpBpMarker in markers), a's index leaves the marker's overlap list; otherwise the handle pair is appended.
void hkp3AxisSweep::endOverlapCheckMarker(
    hkp3AxisSweep::hkpBpMarker* markers,
    hkp3AxisSweep::hkpBpNode& a,
    int32_t nodeIndexA,
    hkp3AxisSweep::hkpBpNode& b,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& deletedPairsOut
) {
    // A handle word is untyped here: with bit 0 set it is the SDK's hkAddByteOffset into markers.
    const uintptr_t handle = reinterpret_cast<uintptr_t>(b.m_handle);
    if (handle & 1) {
        hkpBpMarker* pMarker =
            reinterpret_cast<hkpBpMarker*>(reinterpret_cast<char*>(markers) + (handle & ~uintptr_t(1)));
        hkArray<uint16_t>& overlaps = pMarker->m_overlappingObjects;
        overlaps.removeAt(overlaps.indexOf(static_cast<uint16_t>(nodeIndexA)));
    }
    else {
        // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (deletedPairsOut.m_size ==
            (deletedPairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &deletedPairsOut, sizeof(hkpBroadPhaseHandlePair));
        }
        hkpBroadPhaseHandlePair& pair = deletedPairsOut.m_data[deletedPairsOut.m_size];
        deletedPairsOut.m_size++;
        pair.m_a = a.m_handle;
        pair.m_b = b.m_handle;
    }
}

// The nodes at pa are sorted by min_x: each is tested against the nodes after it that start inside its x interval, and
// each overlapping pair whose later node is no marker is appended.
void hkp3AxisSweep::collide1Axis(const hkpBpNode* pa, int32_t numA, hkArray<hkpBroadPhaseHandlePair>& pairsOut) {
    for (int32_t remaining = numA - 1; remaining > 0; --remaining) {
        const uint32_t maxX = pa->max_x;
        for (const hkpBpNode* other = pa + 1; other->min_x < maxX; ++other) {
            if (pa->yzDisjoint(*other) || other->isMarker()) {
                continue;
            }
            // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
            hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
            if (pairsOut.m_size ==
                (pairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(heap, &pairsOut, sizeof(hkpBroadPhaseHandlePair));
            }
            hkpBroadPhaseHandlePair& pair = pairsOut.m_data[pairsOut.m_size];
            ++pairsOut.m_size;
            pair.m_a = pa->m_handle;
            pair.m_b = other->m_handle;
        }
        ++pa;
    }
}

// The sweep over [worldMin, worldMax] with numMarkers rounded to a power of two (at least 1; m_numMarkers is one
// less). m_intToFloatFloorCorrection is found by bisecting [10, 11] 23 times: a midpoint whose value plus 1,
// quantized with offset 0 and scale 1, lands at 12 or above becomes the high end. Node 0 is the sentinel spanning
// every axis (its end points 0 and 0xFFFD on each); each marker gets a node of its own on x, its handle word the
// marker's byte offset into m_markers plus 1.
hkp3AxisSweep::hkp3AxisSweep(const hkVector4& worldMin, const hkVector4& worldMax, int32_t numMarkers)
  : m_markers(nullptr) {
    m_aabb.m_min = worldMin;
    m_aabb.m_max = worldMax;
    if (numMarkers == 0) {
        numMarkers = 1;
    }
    int32_t ld2NumMarkers = -1;
    while (numMarkers > 0) {
        numMarkers >>= 1;
        ld2NumMarkers++;
    }
    m_nodes.reserve(255);

    // The floor correction: bisect [10, 11] for the value whose quantized max end (offset 0, scale 1) reaches 12.
    float high = 11.0f;
    float low = 10.0f;
    m_offsetLow.m_quad = _mm_setzero_ps();
    m_offsetHigh.m_quad = _mm_setzero_ps();
    m_scale.m_quad = _mm_set1_ps(1.0f);
    const __m128 zero = _mm_setzero_ps();
    for (int32_t i = 0; i < 23; i++) {
        float mid = (high + low) * 0.5f;
        float value = mid + 1.0f;
        alignas(16) float quant[4];
        _mm_store_ps(
            quant,
            _mm_add_ps(
                _mm_max_ps(
                    _mm_min_ps(
                        _mm_mul_ps(_mm_add_ps(_mm_set1_ps(value), m_offsetLow.m_quad), m_scale.m_quad),
                        hkp3AxisSweep_quantizationMax
                    ),
                    zero
                ),
                convertToUint16WithClipMagicQuad
            )
        );
        if (static_cast<uint16_t>((__builtin_bit_cast(uint32_t, quant[0]) >> 7) | 1) >= 12) {
            high = mid;
        }
        else {
            low = mid;
        }
    }
    m_scale.m_quad = _mm_setzero_ps();
    m_offsetLow.m_quad = _mm_setzero_ps();
    m_offsetHigh.m_quad = _mm_setzero_ps();
    m_intToFloatFloorCorrection = (high + low) * 0.5f - 11.0f;

    // Node 0 is the sentinel spanning every axis; its max ends are set once the markers' end points are in.
    {
        hkpBpNode& rSentinel = m_nodes.expandOne();
        rSentinel.min_x = 0;
        rSentinel.min_y = 0;
        rSentinel.min_z = 0;
        rSentinel.m_handle = nullptr;
    }
    const int32_t numMarkersPow2 = 1 << ld2NumMarkers;
    m_axis[0].m_endPoints.reserve(2 * numMarkersPow2 + 510);
    m_axis[1].m_endPoints.reserve(512);
    m_axis[2].m_endPoints.reserve(512);
    hkpBpEndPoint ep;
    ep.m_value = AABB_MIN_VALUE;
    ep.m_nodeIndex = 0;
    for (hkpBpAxis& rAxis : m_axis) {
        rAxis.m_endPoints.pushBack(ep);
    }

    // Each marker gets a node of its own on x; its handle word is the marker's byte offset with bit 0 set.
    m_ld2NumMarkers = ld2NumMarkers;
    m_numMarkers = numMarkersPow2 - 1;
    if (m_numMarkers != 0) {
        m_markers = static_cast<hkpBpMarker*>(hkMemoryRouter::easyAlloc(
            *hkMemoryRouter::getInstance().m_heap, m_numMarkers * static_cast<int32_t>(sizeof(hkpBpMarker))
        ));
    }
    for (int32_t i = 0; i < m_numMarkers; i++) {
        hkpBpMarker* pMarker = new (&m_markers[i]) hkpBpMarker();
        ep.m_nodeIndex = static_cast<uint16_t>(m_nodes.m_size);
        ep.m_value = static_cast<uint16_t>((i + 1) << (16 - m_ld2NumMarkers));
        pMarker->m_nodeIndex = ep.m_nodeIndex;
        pMarker->m_value = ep.m_value;
        hkpBpNode& rNode = m_nodes.expandOne();
        hkArray<hkpBpEndPoint>& xPoints = m_axis[0].m_endPoints;
        rNode.min_x = static_cast<uint16_t>(xPoints.m_size);
        xPoints.pushBack(ep);
        ep.m_value |= 1;
        rNode.max_x = static_cast<uint16_t>(xPoints.m_size);
        xPoints.pushBack(ep);
        // The SDK's tag, not an object: the marker's byte offset with bit 0 set.
        rNode.m_handle = reinterpret_cast<hkpBroadPhaseHandle*>(static_cast<uintptr_t>(i) * sizeof(hkpBpMarker) | 1);
        rNode.min_y = 0;
        rNode.min_z = 0;
        rNode.max_y = 1;
        rNode.max_z = 1;
    }

    hkpBpNode& rSentinel = m_nodes.m_data[0];
    rSentinel.max_x = static_cast<uint16_t>(m_axis[0].m_endPoints.m_size);
    rSentinel.max_y = static_cast<uint16_t>(m_axis[1].m_endPoints.m_size);
    rSentinel.max_z = static_cast<uint16_t>(m_axis[2].m_endPoints.m_size);
    // Each axis ends with the sentinel's max end, above AABB_MAX_VALUE.
    ep.m_value = AABB_MAX_VALUE + 1;
    ep.m_nodeIndex = 0;
    for (hkpBpAxis& rAxis : m_axis) {
        rAxis.m_endPoints.pushBack(ep);
    }
}

hkpBroadPhase*
hk3AxisSweep16CreateBroadPhase(const hkVector4& worldMin, const hkVector4& worldMax, int32_t numMarkers) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkp3AxisSweep* v4 = static_cast<hkp3AxisSweep*>(Instance->m_heap->blockAlloc(sizeof(hkp3AxisSweep)));
    v4->m_memSizeAndFlags = sizeof(hkp3AxisSweep);
    return ::new (v4) hkp3AxisSweep(worldMin, worldMax, numMarkers);
}

// The SDK's destructor: each marker's overlap list, the marker block, the three axes' end points from the last axis
// down and the nodes are freed, then the base's destructor runs.
// The axes and the nodes free themselves; the markers were placed in a block of their own.
hkp3AxisSweep::~hkp3AxisSweep() {
    for (int32_t i = 0; i < m_numMarkers; i++) {
        m_markers[i].~hkpBpMarker();
    }
    hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, m_markers);
}

// The merge on axis 0. Each end point placed writes its node's min_x or max_x: the node is treated as untyped bytes
// there, addressing the word at the byte offset s_memberOffsets gives. The last value read is the word below
// newEndPoints, as the game does.
hkp3AxisSweep::hkpBpEndPoint* hkp3AxisSweep::hkpBpAxis::mergeBatchX(
    hkpBpNode* nodes,
    int32_t insertIndex,
    hkpBpEndPoint* newEndPoints,
    int32_t numNewEndPoints
) {
    hkpBpEndPoint* pData = m_endPoints.m_data;
    hkpBpEndPoint* pNew = &newEndPoints[numNewEndPoints - 1];
    hkpBpEndPoint* pOld = &pData[insertIndex - numNewEndPoints - 1];
    int32_t oldValue = pOld->m_value;
    int32_t newValue = pNew->m_value;
    int32_t position = insertIndex - 1;
    hkpBpEndPoint* pDest = &pData[position];
    do {
        while (oldValue > newValue) {
            *pDest = *pOld;
            *reinterpret_cast<uint16_t*>(
                reinterpret_cast<char*>(&nodes[pOld->m_nodeIndex]) + hkpBpNode::s_memberOffsets[pOld->isMaxPoint()]
            ) = static_cast<uint16_t>(position);
            --pOld;
            --pDest;
            --position;
            oldValue = pOld->m_value;
        }
        *pDest = *pNew;
        *reinterpret_cast<uint16_t*>(
            reinterpret_cast<char*>(&nodes[pNew->m_nodeIndex]) + hkpBpNode::s_memberOffsets[pNew->isMaxPoint()]
        ) = static_cast<uint16_t>(position);
        --pNew;
        --pDest;
        --position;
        newValue = pNew->m_value;
    }
    while (pNew >= newEndPoints);
    return pNew;
}

// The merge on axis 1, as mergeBatchX: each end point placed writes its node's min_y or max_y.
hkp3AxisSweep::hkpBpEndPoint* hkp3AxisSweep::hkpBpAxis::mergeBatchY(
    hkpBpNode* nodes,
    int32_t insertIndex,
    hkpBpEndPoint* newEndPoints,
    int32_t numNewEndPoints
) {
    hkpBpEndPoint* pData = m_endPoints.m_data;
    hkpBpEndPoint* pNew = &newEndPoints[numNewEndPoints - 1];
    hkpBpEndPoint* pOld = &pData[insertIndex - numNewEndPoints - 1];
    int32_t oldValue = pOld->m_value;
    int32_t newValue = pNew->m_value;
    int32_t position = insertIndex - 1;
    hkpBpEndPoint* pDest = &pData[position];
    do {
        while (oldValue > newValue) {
            *pDest = *pOld;
            *reinterpret_cast<uint16_t*>(
                reinterpret_cast<char*>(&nodes[pOld->m_nodeIndex]) + hkpBpNode::s_memberOffsets[2 + pOld->isMaxPoint()]
            ) = static_cast<uint16_t>(position);
            --pOld;
            --pDest;
            --position;
            oldValue = pOld->m_value;
        }
        *pDest = *pNew;
        *reinterpret_cast<uint16_t*>(
            reinterpret_cast<char*>(&nodes[pNew->m_nodeIndex]) + hkpBpNode::s_memberOffsets[2 + pNew->isMaxPoint()]
        ) = static_cast<uint16_t>(position);
        --pNew;
        --pDest;
        --position;
        newValue = pNew->m_value;
    }
    while (pNew >= newEndPoints);
    return pNew;
}

// The merge on axis 2, as mergeBatchX: each end point placed writes its node's min_z or max_z.
hkp3AxisSweep::hkpBpEndPoint* hkp3AxisSweep::hkpBpAxis::mergeBatchZ(
    hkpBpNode* nodes,
    int32_t insertIndex,
    hkpBpEndPoint* newEndPoints,
    int32_t numNewEndPoints
) {
    hkpBpEndPoint* pData = m_endPoints.m_data;
    hkpBpEndPoint* pNew = &newEndPoints[numNewEndPoints - 1];
    hkpBpEndPoint* pOld = &pData[insertIndex - numNewEndPoints - 1];
    int32_t oldValue = pOld->m_value;
    int32_t newValue = pNew->m_value;
    int32_t position = insertIndex - 1;
    hkpBpEndPoint* pDest = &pData[position];
    do {
        while (oldValue > newValue) {
            *pDest = *pOld;
            *reinterpret_cast<uint16_t*>(
                reinterpret_cast<char*>(&nodes[pOld->m_nodeIndex]) + hkpBpNode::s_memberOffsets[4 + pOld->isMaxPoint()]
            ) = static_cast<uint16_t>(position);
            --pOld;
            --pDest;
            --position;
            oldValue = pOld->m_value;
        }
        *pDest = *pNew;
        *reinterpret_cast<uint16_t*>(
            reinterpret_cast<char*>(&nodes[pNew->m_nodeIndex]) + hkpBpNode::s_memberOffsets[4 + pNew->isMaxPoint()]
        ) = static_cast<uint16_t>(position);
        --pNew;
        --pDest;
        --position;
        newValue = pNew->m_value;
    }
    while (pNew >= newEndPoints);
    return pNew;
}

void hkp3AxisSweep::hkpBpAxis::insert(
    hkp3AxisSweep::hkpBpNode* nodes,
    int32_t nodeIndex,
    uint16_t minPosition,
    uint16_t maxPosition,
    uint16_t& minInsertPositionOut,
    uint16_t& maxInsertPositionOut
) {
    bool i;
    bool j;
    int32_t v8 = m_endPoints.m_size + 2;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v10 = m_endPoints.m_capacityAndFlags & hkArrayBase<hkp3AxisSweep::hkpBpEndPoint>::CAPACITY_MASK;
    if (v10 < v8) {
        int32_t v11 = 2 * v10;
        if (v8 >= v11) {
            v11 = v8;
        }
        hkArrayUtil::_reserve(m_heap, &m_endPoints, v11, sizeof(hkp3AxisSweep::hkpBpEndPoint));
    }
    hkp3AxisSweep::hkpBpEndPoint* v12 = &m_endPoints.m_data[v8 - 3];
    m_endPoints.m_size = v8;
    v12[2] = *v12;
    hkp3AxisSweep::hkpBpEndPoint* v13 = v12 - 1;
    for (i = maxPosition == v13->m_value; maxPosition < v13->m_value; i = maxPosition == v13->m_value) {
        v13[2] = *v13;
        --v13;
    }
    if (i) {
        do {
            if (nodeIndex >= static_cast<uint32_t>(v13->m_nodeIndex)) {
                break;
            }
            v13[2] = *v13;
            --v13;
        }
        while (maxPosition == v13->m_value);
    }
    v13[2].m_value = maxPosition;
    v13[2].m_nodeIndex = nodeIndex;
    maxInsertPositionOut = v13 - m_endPoints.begin() + 2;
    for (j = minPosition == v13->m_value; minPosition < v13->m_value; j = minPosition == v13->m_value) {
        v13[1] = *v13;
        --v13;
    }
    if (j) {
        do {
            if (nodeIndex >= static_cast<uint32_t>(v13->m_nodeIndex)) {
                break;
            }
            v13[1] = *v13;
            --v13;
        }
        while (minPosition == v13->m_value);
    }
    v13[1].m_nodeIndex = nodeIndex;
    v13[1].m_value = minPosition;
    minInsertPositionOut = v13 - m_endPoints.begin() + 1;
}

// The lists at pa and pb are each sorted by min_x and are merged by it: the node that starts first is tested against
// the nodes of the other list that start inside its x interval, then dropped. Only the pb list holds markers; a
// marker's m_handle is its byte offset into m_markers with the low bit set, added as raw memory to m_markers. The
// overlap handling is written out twice.
void hkp3AxisSweep::collide1Axis(
    const hkpBpNode* pa,
    int32_t numA,
    const hkpBpNode* pb,
    int32_t numB,
    MarkerHandling markerHandling,
    hkArray<hkpBroadPhaseHandlePair>& pairsOut
) {
    hkpBpMarker* const markers = m_markers;
    int32_t remaining;
    do {
        if (pa->min_x > pb->min_x) {
            // pb starts first: the pa nodes starting inside its interval.
            const uint32_t maxX = pb->max_x;
            for (const hkpBpNode* node = pa; node->min_x < maxX; ++node) {
                if (pb->yzDisjoint(*node)) {
                    continue;
                }
                if (!pb->isMarker()) {
                    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                    if (pairsOut.m_size ==
                        (pairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(heap, &pairsOut, sizeof(hkpBroadPhaseHandlePair));
                    }
                    hkpBroadPhaseHandlePair& pair = pairsOut.m_data[pairsOut.m_size];
                    ++pairsOut.m_size;
                    pair.m_a = node->m_handle;
                    pair.m_b = pb->m_handle;
                    continue;
                }
                if (markerHandling == MARKERS_IGNORE) {
                    continue;
                }
                const uint32_t id = node->m_handle->m_id;
                hkpBpMarker* marker = reinterpret_cast<hkpBpMarker*>(
                    reinterpret_cast<uint8_t*>(markers) + (reinterpret_cast<uintptr_t>(pb->m_handle) & 0xFFFFFFFEu)
                );
                hkArray<uint16_t>& overlaps = marker->m_overlappingObjects;
                if (markerHandling == MARKERS_ADD_NEW_OVERLAPS) {
                    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                    if (overlaps.m_size == (overlaps.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(heap, &overlaps, sizeof(uint16_t));
                    }
                    overlaps.m_data[overlaps.m_size] = static_cast<uint16_t>(id);
                    ++overlaps.m_size;
                }
                else {
                    const int32_t size = overlaps.m_size;
                    int32_t index = 0;
                    for (; index < size; ++index) {
                        if (overlaps.m_data[index] == static_cast<uint16_t>(id)) {
                            break;
                        }
                    }
                    if (index >= size) {
                        index = -1;
                    }
                    --overlaps.m_size;
                    const int32_t last = overlaps.m_size;
                    if (last != index) {
                        overlaps.m_data[index] = overlaps.m_data[last];
                    }
                }
            }
            ++pb;
            remaining = --numB;
        }
        else {
            // pa starts first: the pb nodes starting inside its interval.
            const uint32_t maxX = pa->max_x;
            for (const hkpBpNode* node = pb; node->min_x < maxX; ++node) {
                if (pa->yzDisjoint(*node)) {
                    continue;
                }
                if (!node->isMarker()) {
                    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                    if (pairsOut.m_size ==
                        (pairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(heap, &pairsOut, sizeof(hkpBroadPhaseHandlePair));
                    }
                    hkpBroadPhaseHandlePair& pair = pairsOut.m_data[pairsOut.m_size];
                    ++pairsOut.m_size;
                    pair.m_a = pa->m_handle;
                    pair.m_b = node->m_handle;
                    continue;
                }
                if (markerHandling == MARKERS_IGNORE) {
                    continue;
                }
                const uint32_t id = pa->m_handle->m_id;
                hkpBpMarker* marker = reinterpret_cast<hkpBpMarker*>(
                    reinterpret_cast<uint8_t*>(markers) + (reinterpret_cast<uintptr_t>(node->m_handle) & 0xFFFFFFFEu)
                );
                hkArray<uint16_t>& overlaps = marker->m_overlappingObjects;
                if (markerHandling == MARKERS_ADD_NEW_OVERLAPS) {
                    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                    if (overlaps.m_size == (overlaps.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(heap, &overlaps, sizeof(uint16_t));
                    }
                    overlaps.m_data[overlaps.m_size] = static_cast<uint16_t>(id);
                    ++overlaps.m_size;
                }
                else {
                    const int32_t size = overlaps.m_size;
                    int32_t index = 0;
                    for (; index < size; ++index) {
                        if (overlaps.m_data[index] == static_cast<uint16_t>(id)) {
                            break;
                        }
                    }
                    if (index >= size) {
                        index = -1;
                    }
                    --overlaps.m_size;
                    const int32_t last = overlaps.m_size;
                    if (last != index) {
                        overlaps.m_data[index] = overlaps.m_data[last];
                    }
                }
            }
            ++pa;
            remaining = --numA;
        }
    }
    while (remaining > 0);
}

// Binary search down to a span of 16 end points, then a linear scan to the first end point not below value.
const hkp3AxisSweep::hkpBpEndPoint*
hkp3AxisSweep::hkpBpAxis::find(const hkpBpEndPoint* start, const hkpBpEndPoint* end, uint16_t value) const {
    while (end - start > 16) {
        const hkpBpEndPoint* mid = start + ((end - start) >> 1);
        if (mid->m_value < value) {
            start = mid;
        }
        else {
            end = mid;
        }
    }
    while (start->m_value < value) {
        ++start;
    }
    return start;
}

// The cache is three axes followed by their end points: each axis's buffer holds a start marker, the two end points of
// each overlapping node on that axis and an end marker, the end points between the markers sorted. The axes and the end
// points are constructed in place.
// KEEP: the comparator's argument slot of each sort call (0xD19FA0) is the slot of the prologue's push ecx, of which
// only the low byte is written; its upper three bytes are this's, or a pointer the copy loop left there. src cannot
// reproduce that register garbage (as hkpAllCdPointCollector::sortHits, 0xCABAD0).
void hkp3AxisSweep::calcAabbCacheInternal(
    const hkArrayBase<const hkpBpNode*>& overlaps,
    hkpBroadPhaseAabbCache* AabbCacheOut
) const {
    // The cache is raw bytes to the broad phase (hkpBroadPhaseAabbCache is a char): the SDK lays its axes and end
    // points out in it.
    hkpBpAxis* axis = reinterpret_cast<hkpBpAxis*>(AabbCacheOut);
    hkpBpEndPoint* endPoints = reinterpret_cast<hkpBpEndPoint*>(axis + 3);
    int32_t numEndPoints = 2 * overlaps.m_size + 2;
    for (int32_t i = 0; i < 3; i++) {
        // EXE-INLINED none: hkp3AxisSweep::hkpBpAxis::hkpBpAxis(hkpBpEndPoint*, int)
        hkpBpAxis* pAxis = &axis[i];
        if (pAxis) {
            pAxis->m_endPoints.m_data = endPoints + i * numEndPoints;
            pAxis->m_endPoints.m_size = 0;
            pAxis->m_endPoints.m_capacityAndFlags =
                static_cast<int32_t>(numEndPoints | hkArrayBase<hkpBpEndPoint>::DONT_DEALLOCATE_FLAG);
        }
    }

    hkpBpEndPoint start;
    start.m_value = AABB_MIN_VALUE;
    start.m_nodeIndex = 0;
    for (int32_t i = 0; i < 3; i++) {
        axis[i].m_endPoints.pushBackUnchecked(start);
    }

    // Each node's x, y and z end points, read from this sweep's axes.
    const hkpBpNode* const* ppNode = overlaps.m_data;
    for (int32_t n = overlaps.m_size - 1; n >= 0; n--) {
        axis[0].m_endPoints.pushBackUnchecked(m_axis[0].m_endPoints.m_data[(*ppNode)->min_x]);
        axis[0].m_endPoints.pushBackUnchecked(m_axis[0].m_endPoints.m_data[(*ppNode)->max_x]);
        axis[1].m_endPoints.pushBackUnchecked(m_axis[1].m_endPoints.m_data[(*ppNode)->min_y]);
        axis[1].m_endPoints.pushBackUnchecked(m_axis[1].m_endPoints.m_data[(*ppNode)->max_y]);
        axis[2].m_endPoints.pushBackUnchecked(m_axis[2].m_endPoints.m_data[(*ppNode)->min_z]);
        axis[2].m_endPoints.pushBackUnchecked(m_axis[2].m_endPoints.m_data[(*ppNode)->max_z]);
        ppNode++;
    }

    // Sorts the end points after each axis's start marker.
    for (int32_t i = 0; i < 3; i++) {
        // EXE-INLINED none: hkAlgorithm::quickSort<hkp3AxisSweep::hkpBpEndPoint>
        int32_t numToSort = axis[i].m_endPoints.m_size - 1;
        if (numToSort > 1) {
            hkAlgorithm::quickSortRecursive<hkpBpEndPoint, hkAlgorithm::less<hkpBpEndPoint>>(
                axis[i].m_endPoints.m_data + 1, 0, numToSort - 1, hkAlgorithm::less<hkpBpEndPoint>()
            );
        }
    }

    hkpBpEndPoint end;
    end.m_value = AABB_MAX_VALUE;
    end.m_nodeIndex = 0;
    for (int32_t i = 0; i < 3; i++) {
        axis[i].m_endPoints.pushBackUnchecked(end);
    }
}

// Moves every node's end point indices past the new node's: each index (compared as a signed 16-bit value) above the
// new min - 1 goes up one, and one more above the new max - 2.
// KEEP: the exe's only caller, addObject (0xD142B0), stores object->m_id from edx after its call of this function,
// which leaves edx intact in the exe; clang does not guarantee that.
void hkp3AxisSweep::updateNodesAfterInsert(hkpBpNode* nodes, int32_t numNodes, hkpBpNode& newNode) {
    const int16_t minY = int16_t(newNode.min_y - 1);
    const int16_t minZ = int16_t(newNode.min_z - 1);
    const int16_t maxY = int16_t(newNode.max_y - 2);
    const int16_t maxZ = int16_t(newNode.max_z - 2);
    const int16_t minX = int16_t(newNode.min_x - 1);
    const int16_t maxX = int16_t(newNode.max_x - 2);
    const __m64 minYZ = _mm_set_pi16(minZ, minY, minZ, minY);
    const __m64 maxYZ = _mm_set_pi16(maxZ, maxY, maxZ, maxY);
    // Lanes 2 and 3 of these two are set to 0; their value does not matter because the mask zeroes the lanes they
    // decide.
    const __m64 minXX = _mm_set_pi16(0, 0, minX, minX);
    const __m64 maxXX = _mm_set_pi16(0, 0, maxX, maxX);
    const __m64 xMask = _mm_set_pi32(0, -1);
    hkpBpNode* end = nodes + numNodes;
    for (hkpBpNode* node = nodes; node < end; ++node) {
        // Reads and writes the node as two raw 64-bit values, not through its fields.
        __m64* quads = reinterpret_cast<__m64*>(node);
        const __m64 yz = quads[0];
        quads[0] = _mm_sub_pi16(yz, _mm_add_pi16(_mm_cmpgt_pi16(yz, minYZ), _mm_cmpgt_pi16(yz, maxYZ)));
        const __m64 x = quads[1];
        const __m64 xSteps = _mm_add_pi16(_mm_cmpgt_pi16(x, minXX), _mm_cmpgt_pi16(x, maxXX));
        quads[1] = _mm_sub_pi16(x, _mm_and_si64(xMask, xSteps));
    }
    _mm_empty();
}

// Havok SDK: clears the bit field, then sets the bits of the nodes whose x interval holds x_value. With markers,
// the sweep starts at the last marker below x_value: its own bit and its overlaps' (but the query node's) are
// toggled and the nodes starting inside the marker are cleared; every end point from there up to the query node's
// min_x toggles its node, and the nodes starting inside the query node's interval are toggled.
void hkp3AxisSweep::setBitsBasedOnXInterval(
    int32_t numNodes,
    int32_t x_value,
    const hkp3AxisSweep::hkpBpNode& queryNode,
    uint16_t queryNodeIndex,
    uint32_t* bitField
) const {
    const int32_t max_x = queryNode.max_x;
    const int32_t min_x = queryNode.min_x;
    // Clears 16 bytes per 128 nodes.
    uint32_t* pClear = bitField;
    for (int32_t i = numNodes >> 7; i >= 0; i--) {
        __builtin_memset(pClear, 0, sizeof(uint32_t) * 4);
        pClear += 4;
    }
    const hkpBpEndPoint* ep = &m_axis[0].m_endPoints.m_data[1];
    if (m_numMarkers) {
        const int32_t markerIndex = x_value >> (16 - m_ld2NumMarkers);
        if (markerIndex > 0) {
            const hkpBpMarker& marker = m_markers[markerIndex - 1];
            const int32_t markerNodeIndex = marker.m_nodeIndex;
            // EXE-INLINED 0xD10AD0
            bitField[markerNodeIndex >> 5] ^= 1u << (markerNodeIndex & 31);
            const uint16_t* pOverlap = marker.m_overlappingObjects.m_data;
            for (int32_t i = marker.m_overlappingObjects.m_size - 1; i >= 0; i--) {
                if (*pOverlap != queryNodeIndex) {
                    const int32_t overlapIndex = *pOverlap;
                    // EXE-INLINED 0xD10AD0
                    bitField[overlapIndex >> 5] ^= 1u << (overlapIndex & 31);
                }
                pOverlap++;
            }
            const hkpBpNode& markerNode = m_nodes.m_data[marker.m_nodeIndex];
            const hkpBpEndPoint* p = &m_axis[0].m_endPoints.m_data[markerNode.min_x + 1];
            const hkpBpEndPoint* end = &m_axis[0].m_endPoints.m_data[markerNode.max_x];
            for (; p < end; p++) {
                // EXE-INLINED none: hkp3AxisSweep::hkpBpEndPoint::isMaxPoint
                if (!(p->m_value & 1)) {
                    const int32_t nodeIndex = p->m_nodeIndex;
                    // EXE-INLINED 0xD10AF0
                    bitField[nodeIndex >> 5] &= ~(1u << (nodeIndex & 31));
                }
            }
            ep = &m_axis[0].m_endPoints.m_data[markerNode.min_x + 1];
        }
    }
    const hkpBpEndPoint* end = &m_axis[0].m_endPoints.m_data[min_x];
    for (; ep < end; ep++) {
        const int32_t nodeIndex = ep->m_nodeIndex;
        // EXE-INLINED 0xD10AD0
        bitField[nodeIndex >> 5] ^= 1u << (nodeIndex & 31);
    }
    ep++;
    end = &m_axis[0].m_endPoints.m_data[max_x];
    for (; ep < end; ep++) {
        // EXE-INLINED none: hkp3AxisSweep::hkpBpEndPoint::isMaxPoint
        if (!(ep->m_value & 1)) {
            const int32_t nodeIndex = ep->m_nodeIndex;
            // EXE-INLINED 0xD10AD0
            bitField[nodeIndex >> 5] ^= 1u << (nodeIndex & 31);
        }
    }
}

// A node's end point index is written at the byte offset s_memberOffsets gives.
int32_t
hkp3AxisSweep::hkpBpAxis::insertTail(hkpBpNode* nodes, int32_t axis, hkpBpEndPoint* newNodes, int32_t numNewEndPoints) {
    int32_t oldSize = m_endPoints.m_size;
    int32_t newSize = oldSize + numNewEndPoints;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t capacity = m_endPoints.m_capacityAndFlags & hkArrayBase<hkp3AxisSweep::hkpBpEndPoint>::CAPACITY_MASK;
    if (capacity < newSize) {
        int32_t reqElem = 2 * capacity;
        if (newSize >= reqElem) {
            reqElem = newSize;
        }
        hkArrayUtil::_reserve(pHeap, &m_endPoints, reqElem, static_cast<int32_t>(sizeof(hkpBpEndPoint)));
    }
    m_endPoints.m_size = newSize;
    uint16_t lastNewValue = newNodes[numNewEndPoints - 1].m_value;
    hkpBpEndPoint* pData = m_endPoints.m_data;
    hkpBpEndPoint sentinel = pData[oldSize - 1];
    // The prefetch intrinsic takes raw bytes.
    _mm_prefetch(reinterpret_cast<const char*>(&pData[newSize - 1]), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(&pData[newSize - 129]), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(&pData[oldSize - 1]), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(&pData[newSize - 129]), _MM_HINT_NTA);
    hkpBpEndPoint* pDest = &pData[newSize - 1];
    hkpBpEndPoint* pSource = &pData[oldSize - 1];
    *pDest = sentinel;
    *reinterpret_cast<uint16_t*>(
        reinterpret_cast<char*>(&nodes[pSource->m_nodeIndex]) +
        hkpBpNode::s_memberOffsets[2 * axis + pSource->isMaxPoint()]
    ) = static_cast<uint16_t>(pDest - m_endPoints.m_data);
    --pSource;
    --pDest;
    if (pSource->m_value > lastNewValue) {
        const hkpBpEndPoint* pPrefetch = pSource - 128;
        do {
            *pDest = *pSource;
            _mm_prefetch(reinterpret_cast<const char*>(pPrefetch), _MM_HINT_NTA);
            *reinterpret_cast<uint16_t*>(
                reinterpret_cast<char*>(&nodes[pSource->m_nodeIndex]) +
                hkpBpNode::s_memberOffsets[2 * axis + pSource->isMaxPoint()]
            ) = static_cast<uint16_t>(pDest - m_endPoints.m_data);
            _mm_prefetch(reinterpret_cast<const char*>(pPrefetch), _MM_HINT_NTA);
            --pSource;
            --pPrefetch;
            --pDest;
        }
        while (pSource->m_value > lastNewValue);
    }
    return static_cast<int32_t>(pDest - m_endPoints.m_data) + 1;
}

// A kept node's word for this axis's end is written at the byte offset s_memberOffsets gives. The size is set as
// hkArray::setSize does. The loop end is compared as an address (unsigned, with 32-bit wrap), because as pointers clang
// folds the first compare to a test of the size.
template <int32_t axis>
void hkp3AxisSweep::hkpBpAxis::removeBatch(hkpBpNode* nodes, const hkArrayBase<int32_t>& nodeRelocations) {
    hkpBpEndPoint* source = m_endPoints.m_data;
    uintptr_t end = reinterpret_cast<uintptr_t>(source) +
        static_cast<uintptr_t>(static_cast<uint32_t>(m_endPoints.m_size)) * sizeof(hkpBpEndPoint);
    hkpBpEndPoint* dest = source;
    int32_t numKept = 0;
    for (; reinterpret_cast<uintptr_t>(source) < end; ++source) {
        int32_t newIndex = nodeRelocations.m_data[source->m_nodeIndex];
        if (newIndex >= 0) {
            *dest = *source;
            dest->m_nodeIndex = static_cast<uint16_t>(newIndex);
            *reinterpret_cast<uint16_t*>(
                reinterpret_cast<char*>(&nodes[newIndex]) + hkpBpNode::s_memberOffsets[2 * axis + source->isMaxPoint()]
            ) = static_cast<uint16_t>(numKept);
            ++dest;
            ++numKept;
        }
    }
    m_endPoints.setSize(numKept);
}
template void hkp3AxisSweep::hkpBpAxis::removeBatch<0>(hkpBpNode* nodes, const hkArrayBase<int32_t>& newIndices);
template void hkp3AxisSweep::hkpBpAxis::removeBatch<1>(hkpBpNode* nodes, const hkArrayBase<int32_t>& newIndices);
template void hkp3AxisSweep::hkpBpAxis::removeBatch<2>(hkpBpNode* nodes, const hkArrayBase<int32_t>& newIndices);

void hkp3AxisSweep::getAabbFromNode(const hkp3AxisSweep::hkpBpNode& node, hkAabb& aabb) const {
    __m128 v3 = _mm_undefined_ps();
    hkVector4 v6;
    uint32_t v9;
    uint32_t v11;
    alignas(16) uint32_t v13;
    uint32_t v15;
    uint32_t v16;
    uint32_t v17;
    hkp3AxisSweep::hkpBpEndPoint* m_data = m_axis[0].m_endPoints.m_data;
    hkp3AxisSweep::hkpBpEndPoint* v5 = m_axis[1].m_endPoints.m_data;
    v6.m_quad = m_offsetLow.m_quad;
    v3[0] = 0.0f;
    float v10 = 1.0 / m_scale.m_quad[0];
    float v12 = 1.0 / m_scale.m_quad[1];
    float v14 = 1.0 / m_scale.m_quad[2];
    hkp3AxisSweep::hkpBpEndPoint* v7 = m_axis[2].m_endPoints.m_data;
    *reinterpret_cast<float*>(&v9) = static_cast<double>(m_data[node.min_x].m_value) * v10;
    double v8 = v12;
    *reinterpret_cast<float*>(&v16) = static_cast<double>(v5[node.min_y].m_value) * v12;
    *reinterpret_cast<float*>(&v17) = static_cast<double>(v7[node.min_z].m_value) * v14;
    *reinterpret_cast<float*>(&v13) = v10 * static_cast<double>(m_data[node.max_x].m_value);
    *reinterpret_cast<float*>(&v11) = v8 * static_cast<double>(v5[node.max_y].m_value);
    *reinterpret_cast<float*>(&v15) = v14 * static_cast<double>(v7[node.max_z].m_value);
    aabb.m_min.m_quad = _mm_sub_ps(
        _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v9)), _mm_set_ss(std::bit_cast<float>(v17))),
            _mm_unpacklo_ps(
                _mm_set_ss(std::bit_cast<float>(v16)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
            )
        ),
        v6.m_quad
    );
    aabb.m_max.m_quad = _mm_sub_ps(
        _mm_unpacklo_ps(
            _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v13)), _mm_set_ss(std::bit_cast<float>(v15))),
            _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v11)), v3)
        ),
        v6.m_quad
    );
}

int16_t getEpValue(const int16_t* r_eax) {
    return *r_eax;
}

// The old end points below the new ones go to scratch first; the merge then fills the axis from slot 1 up, the lesser
// of the next old and the next new end point (hkpBpEndPoint's order) first. The old ones below the first new value are
// skipped (they stay where they are). A placed end point's node word for this axis's end is written at the byte offset
// s_memberOffsets gives.
void hkp3AxisSweep::hkpBpAxis::mergeBatch(
    hkp3AxisSweep::hkpBpNode* nodes,
    int32_t indexOfNewEps,
    int32_t newNum,
    int32_t axis,
    hkp3AxisSweep::hkpBpEndPoint* scratch
) {
    for (int32_t i = 0; i < indexOfNewEps; ++i) {
        scratch[i] = m_endPoints.m_data[i];
    }
    hkpBpEndPoint* pOld = scratch + 1;
    hkpBpEndPoint* const pOldLast = &scratch[indexOfNewEps - 1];
    hkpBpEndPoint* pNew = &m_endPoints.m_data[indexOfNewEps];
    hkpBpEndPoint* const pNewEnd = &m_endPoints.m_data[newNum + indexOfNewEps];
    hkpBpEndPoint* pDest = m_endPoints.m_data + 1;
    const uint16_t firstNewValue = pNew->m_value;
    while (pOld->m_value < firstNewValue) {
        ++pOld;
        ++pDest;
    }
    // Copies source to pDest, writes its position into its node, and steps pDest.
    auto place = [&](const hkpBpEndPoint& source) {
        *pDest = source;
        *reinterpret_cast<uint16_t*>(
            reinterpret_cast<char*>(&nodes[pDest->m_nodeIndex]) +
            hkpBpNode::s_memberOffsets[2 * axis + pDest->isMaxPoint()]
        ) = static_cast<uint16_t>(pDest - m_endPoints.m_data);
        ++pDest;
    };
    if (pOld < pOldLast && pNew < pNewEnd) {
        for (;;) {
            if (pOld->m_value < pNew->m_value ||
                (pOld->m_value == pNew->m_value && pOld->m_nodeIndex < pNew->m_nodeIndex)) {
                place(*pOld);
                ++pOld;
                if (pOld >= pOldLast) {
                    break;
                }
            }
            else {
                place(*pNew);
                ++pNew;
                if (pNew >= pNewEnd) {
                    break;
                }
            }
        }
    }
    while (pNew < pNewEnd) {
        place(*pNew);
        ++pNew;
    }
    while (pOld <= pOldLast) {
        place(*pOld);
        ++pOld;
    }
}

int32_t isBitSet(int32_t a1, const uint32_t* a2) {
    return a2[a1 >> 5] & (1 << (a1 & 31));
}

// Each compare is a signed 16-bit one, and a position above its axis's offsets[axis] - 1 gains numNewEndPoints. The
// added 16-bit values come from numNewEndPoints * 0x10001 as a 32-bit value: its low half on min_y, max_y and min_x,
// its high half on min_z, max_z and max_x, which differ when the count does not fit in 16 bits. The handle is left
// unchanged.
void hkp3AxisSweep::updateNodesAfterBatchTailInsert(
    hkp3AxisSweep::hkpBpNode* nodes,
    int32_t numNodes,
    int32_t numNewEndPoints,
    int32_t* offsets
) {
    const int16_t lastX = static_cast<int16_t>(offsets[0] - 1);
    const int16_t lastY = static_cast<int16_t>(offsets[1] - 1);
    const int16_t lastZ = static_cast<int16_t>(offsets[2] - 1);
    const uint32_t steps = static_cast<uint32_t>(numNewEndPoints) * 0x10001u;
    const uint16_t stepEven = static_cast<uint16_t>(steps);
    const uint16_t stepOdd = static_cast<uint16_t>(steps >> 16);
    const uintptr_t nodesEnd =
        reinterpret_cast<uintptr_t>(nodes) + static_cast<uintptr_t>(numNodes) * sizeof(hkpBpNode);
    for (hkpBpNode* node = nodes; reinterpret_cast<uintptr_t>(node) < nodesEnd; ++node) {
        const int16_t minY = static_cast<int16_t>(node->min_y);
        const int16_t minZ = static_cast<int16_t>(node->min_z);
        const int16_t maxY = static_cast<int16_t>(node->max_y);
        const int16_t maxZ = static_cast<int16_t>(node->max_z);
        const int16_t minX = static_cast<int16_t>(node->min_x);
        const int16_t maxX = static_cast<int16_t>(node->max_x);
        node->min_y = static_cast<uint16_t>(minY + (minY > lastY ? stepEven : 0));
        node->min_z = static_cast<uint16_t>(minZ + (minZ > lastZ ? stepOdd : 0));
        node->max_y = static_cast<uint16_t>(maxY + (maxY > lastY ? stepEven : 0));
        node->max_z = static_cast<uint16_t>(maxZ + (maxZ > lastZ ? stepOdd : 0));
        node->min_x = static_cast<uint16_t>(minX + (minX > lastX ? stepEven : 0));
        node->max_x = static_cast<uint16_t>(maxX + (maxX > lastX ? stepOdd : 0));
    }
}

// Moves every node's end point positions down past the deleted node's end points. Each compare is a signed 16-bit one
// and each "greater" adds -1, so a position above both of the old node's end points on its axis drops by two. The old
// node's positions are read once, before the loop.
void hkp3AxisSweep::updateNodesAfterDelete(hkpBpNode* nodes, int32_t numNodes, hkpBpNode& oldNode) {
    const int16_t oldMinX = static_cast<int16_t>(oldNode.min_x);
    const int16_t oldMinY = static_cast<int16_t>(oldNode.min_y);
    const int16_t oldMinZ = static_cast<int16_t>(oldNode.min_z);
    const int16_t oldMaxY = static_cast<int16_t>(oldNode.max_y);
    const int16_t oldMaxZ = static_cast<int16_t>(oldNode.max_z);
    const int16_t oldMaxX = static_cast<int16_t>(oldNode.max_x);
    const uintptr_t nodesEnd =
        reinterpret_cast<uintptr_t>(nodes) + static_cast<uintptr_t>(numNodes) * sizeof(hkpBpNode);
    for (hkpBpNode* node = nodes; reinterpret_cast<uintptr_t>(node) != nodesEnd; ++node) {
        const int16_t minY = static_cast<int16_t>(node->min_y);
        const int16_t minZ = static_cast<int16_t>(node->min_z);
        const int16_t maxY = static_cast<int16_t>(node->max_y);
        const int16_t maxZ = static_cast<int16_t>(node->max_z);
        const int16_t minX = static_cast<int16_t>(node->min_x);
        const int16_t maxX = static_cast<int16_t>(node->max_x);
        node->min_y = static_cast<uint16_t>(minY - (minY > oldMinY) - (minY > oldMaxY));
        node->min_z = static_cast<uint16_t>(minZ - (minZ > oldMinZ) - (minZ > oldMaxZ));
        node->max_y = static_cast<uint16_t>(maxY - (maxY > oldMinY) - (maxY > oldMaxY));
        node->max_z = static_cast<uint16_t>(maxZ - (maxZ > oldMinZ) - (maxZ > oldMaxZ));
        node->min_x = static_cast<uint16_t>(minX - (minX > oldMinX) - (minX > oldMaxX));
        node->max_x = static_cast<uint16_t>(maxX - (maxX > oldMinX) - (maxX > oldMaxX));
    }
}

const hkAabb& hkp3AxisSweep::getOriginalAabb() const {
    return m_aabb;
}

// Clears (bytesize >> 4) + 1 blocks of 16 bytes.
void memclear16(void* dest, int32_t bytesize) {
    uint8_t* pClear = static_cast<uint8_t*>(dest);
    for (int32_t i = bytesize >> 4; i >= 0; i--) {
        __builtin_memset(pClear, 0, 16);
        pClear += 16;
    }
}

void hkp3AxisSweep::beginOverlap(
    hkp3AxisSweep::hkpBpNode& a1,
    hkp3AxisSweep::hkpBpNode& a2,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& newPairsOut
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (newPairsOut.m_size == (newPairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &newPairsOut, sizeof(hkpBroadPhaseHandlePair));
    }
    int32_t m_size = newPairsOut.m_size;
    hkpBroadPhaseHandlePair* v7 = &newPairsOut.m_data[m_size];
    newPairsOut.m_size = m_size + 1;
    v7->m_a = a1.m_handle;
    v7->m_b = a2.m_handle;
}

void hkp3AxisSweep::endOverlap(
    hkp3AxisSweep::hkpBpNode& a1,
    hkp3AxisSweep::hkpBpNode& a2,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& deletedPairsOut
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (deletedPairsOut.m_size ==
        (deletedPairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &deletedPairsOut, sizeof(hkpBroadPhaseHandlePair));
    }
    int32_t m_size = deletedPairsOut.m_size;
    hkpBroadPhaseHandlePair* v7 = &deletedPairsOut.m_data[m_size];
    deletedPairsOut.m_size = m_size + 1;
    v7->m_a = a1.m_handle;
    v7->m_b = a2.m_handle;
}

// Havok SDK: a new overlap of a and b. When b is a marker (bit 0 of its handle set, the rest the byte offset of its
// hkpBpMarker in markers), a's index joins the marker's overlap list; otherwise the handle pair is appended.
void hkp3AxisSweep::beginOverlapCheckMarker(
    hkp3AxisSweep::hkpBpMarker* markers,
    hkp3AxisSweep::hkpBpNode& a,
    int32_t nodeIndexA,
    hkp3AxisSweep::hkpBpNode& b,
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& newPairsOut
) {
    // A handle word is untyped here: with bit 0 set it is the SDK's hkAddByteOffset into markers.
    const uintptr_t handle = reinterpret_cast<uintptr_t>(b.m_handle);
    if (handle & 1) {
        hkpBpMarker* pMarker =
            reinterpret_cast<hkpBpMarker*>(reinterpret_cast<char*>(markers) + (handle & ~uintptr_t(1)));
        hkArray<uint16_t>& overlaps = pMarker->m_overlappingObjects;
        overlaps.pushBack(static_cast<uint16_t>(nodeIndexA));
    }
    else {
        // EXE-INLINED none: hkArray<hkpBroadPhaseHandlePair>::expandOne
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (newPairsOut.m_size ==
            (newPairsOut.m_capacityAndFlags & hkArrayBase<hkpBroadPhaseHandlePair>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &newPairsOut, sizeof(hkpBroadPhaseHandlePair));
        }
        hkpBroadPhaseHandlePair& pair = newPairsOut.m_data[newPairsOut.m_size];
        newPairsOut.m_size++;
        pair.m_a = a.m_handle;
        pair.m_b = b.m_handle;
    }
}
