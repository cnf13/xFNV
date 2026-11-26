#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"

#include <bit>
#include <stdint.h>
#include <xmmintrin.h>

// The sweep-and-prune broad phase: each object's AABB, quantized, is kept as a node whose ends are sorted on three
// axes.
class hkp3AxisSweep : public hkpBroadPhase {
public:
    class hkpBpMarker;

    // What collide1Axis does with the overlaps of a marker. Values from Havok's release hkpcollide.pdb.
    enum MarkerHandling : int32_t {
        MARKERS_ADD_NEW_OVERLAPS = 0x0,
        MARKERS_IGNORE = 0x1,
        MARKERS_REMOVE_NEW_OVERLAPS = 0x2,
    };

    // Whether a query may use the markers. Values from Havok's release hkpcollide.pdb.
    enum hkpBpMarkerUse : int32_t {
        HK_BP_NO_MARKER = 0x0,
        HK_BP_USE_MARKER = 0x1,
    };

    // What a single-AABB query reports: handle pairs, nodes, or hits to a collector. Values from Havok's release
    // hkpcollide.pdb.
    enum hkpBpQuerySingleType : int32_t {
        HK_BP_REPORT_HANDLES = 0x0,
        HK_BP_REPORT_NODES = 0x1,
        HK_BP_COLLECTOR = 0x2,
    };

    // The quantized coordinate range: end point values run from AABB_MIN_VALUE to AABB_MAX_VALUE, the end markers
    // included.
    enum {
        AABB_MIN_VALUE = 0,
        AABB_MAX_VALUE = 65532,
        AABB_MAX_FVALUE = 65532,
        HK_BP_NUM_VALUE_BITS = 16,
    };

    // An object's quantized AABB: the indices of its end points on each axis, and its handle.
    class hkpBpNode {
    public:
        // The byte offset in a node of each axis's min and max end point index, indexed 2 * axis + isMaxPoint: min_x,
        // max_x, min_y, max_y, min_z, max_z.
        using MemberOffsets = const int32_t[6];

        // Whether this node is a marker: Havok tags a marker's handle field by its low bit, so this tests the pointer's
        // bits.
        int32_t isMarker() const {
            return static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_handle) & 1);
        }

        // A marker node's handle field holds its marker's byte offset in markers, tagged by the low bit.
        hkpBpMarker& getMarker(hkpBpMarker* markers) const {
            return *reinterpret_cast<hkpBpMarker*>(
                reinterpret_cast<uintptr_t>(markers) +
                (reinterpret_cast<uintptr_t>(m_handle) & ~static_cast<uintptr_t>(1))
            );
        }

        // Nonzero when this node and other are apart on y or z: the sign bits of the differences of the end points,
        // each node's y and z words packed into one 32-bit value.
        uint32_t yzDisjoint(const hkpBpNode& other) const {
            uint32_t maxA = max_y | static_cast<uint32_t>(max_z) << 16;
            uint32_t minA = min_y | static_cast<uint32_t>(min_z) << 16;
            uint32_t maxB = other.max_y | static_cast<uint32_t>(other.max_z) << 16;
            uint32_t minB = other.min_y | static_cast<uint32_t>(other.min_z) << 16;
            return ((maxA - minB) | (maxB - minA)) & 0x80008000u;
        }

        // Nonzero when this node and other are apart on x or y: the sign bit of the 16-bit differences of the end
        // points (Havok's inline).
        int32_t xyDisjoint(const hkpBpNode& other) const {
            return ((max_x - other.min_x) | (other.max_x - min_x) | (max_y - other.min_y) | (other.max_y - min_y)) &
                0x8000;
        }

        // Nonzero when this node and other are apart on x or z, as xyDisjoint (Havok's inline).
        int32_t xzDisjoint(const hkpBpNode& other) const {
            return ((max_x - other.min_x) | (other.max_x - min_x) | (max_z - other.min_z) | (other.max_z - min_z)) &
                0x8000;
        }

        uint16_t min_y;
        uint16_t min_z;
        uint16_t max_y;
        uint16_t max_z;
        uint16_t min_x;
        uint16_t max_x;
        hkpBroadPhaseHandle* m_handle;

        static inline MemberOffsets s_memberOffsets{8, 10, 0, 4, 2, 6};
    };

    // One end of a node's interval on an axis: the quantized coordinate and the node it belongs to.
    class hkpBpEndPoint {
    public:
        // Orders by coordinate, then by node.
        hkBool operator<(const hkpBpEndPoint& b) const {
            return hkBool(m_value < b.m_value || (m_value == b.m_value && m_nodeIndex < b.m_nodeIndex));
        }

        // Whether this is a node's max end: the value's low bit (Havok's inline).
        int32_t isMaxPoint() const {
            return m_value & 1;
        }

        uint16_t m_value;
        uint16_t m_nodeIndex;
    };

    // One axis: its end points, sorted.
    class hkpBpAxis {
    public:
        hkpBpAxis() = default;
        hkpBpAxis(hkpBpEndPoint* endPoints, int32_t numEndPoints) : m_endPoints(endPoints, 0, numEndPoints) {}

        // The end point from start up to end (both inside m_endPoints) where an end of the given quantized value
        // would go.
        const hkpBpEndPoint* find(const hkpBpEndPoint* start, const hkpBpEndPoint* end, uint16_t value) const;

        // Inserts node nodeIndex's min and max end points on this axis and writes their positions to the out words.
        void insert(
            hkpBpNode* nodes,
            int32_t nodeIndex,
            uint16_t minPosition,
            uint16_t maxPosition,
            uint16_t& minInsertPositionOut,
            uint16_t& maxInsertPositionOut
        );
        // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc). Takes out the end points at minPosition and
        // maxPosition.
        void remove(int32_t minPosition, int32_t maxPosition);
        // The three axes' merges of a batch of sorted new end points into the gap insertTail left below insertIndex
        // (axis 0, 1, 2, each with its axis's s_memberOffsets entries built in): from the top down, each slot takes the
        // greater of the next old end point below the gap and the next new one, the node's index on the axis following
        // it. They stop after the lowest new one and return the new end point below it. nodes and newEndPoints are the
        // pointers addObjectBatch holds.
        hkpBpEndPoint*
        mergeBatchX(hkpBpNode* nodes, int32_t insertIndex, hkpBpEndPoint* newEndPoints, int32_t numNewEndPoints);
        hkpBpEndPoint*
        mergeBatchY(hkpBpNode* nodes, int32_t insertIndex, hkpBpEndPoint* newEndPoints, int32_t numNewEndPoints);
        hkpBpEndPoint*
        mergeBatchZ(hkpBpNode* nodes, int32_t insertIndex, hkpBpEndPoint* newEndPoints, int32_t numNewEndPoints);
        // Appends numNewEndPoints end points to the axis: the old end points above the last new one's value (the
        // last old one, the sentinel, always) move up by numNewEndPoints, their nodes' indices on this axis
        // following them. Returns the index just above the first free slot.
        int32_t insertTail(hkpBpNode* nodes, int32_t axis, hkpBpEndPoint* newNodes, int32_t numNewEndPoints);
        // Drops the end points of removed nodes, whose newIndices entry is negative, and renumbers the rest: each kept
        // end point takes its node's new index, and that node's word for this axis's end (s_memberOffsets) its new
        // position. One instantiation per axis.
        template <int32_t axis>
        void removeBatch(hkpBpNode* nodes, const hkArrayBase<int32_t>& nodeRelocations);
        // Merges newNum sorted new end points, stored at indexOfNewEps, with the old ones below them (copied to
        // scratch first) from slot 1 up; each placed end point's node takes its new position on the axis.
        void mergeBatch(
            hkp3AxisSweep::hkpBpNode* nodes,
            int32_t indexOfNewEps,
            int32_t newNum,
            int32_t axis,
            hkp3AxisSweep::hkpBpEndPoint* scratch
        );

        hkArray<hkpBpEndPoint> m_endPoints;
    };

    // A marker node: its node index, its coordinate, and the objects overlapping it.
    class hkpBpMarker {
    public:
        uint16_t m_nodeIndex;
        uint16_t m_value;
        hkArray<uint16_t> m_overlappingObjects;
    };

    // The static holds a 1 in each lane.
    using OneMaskArray = uint32_t[4];

    hkp3AxisSweep(const hkVector4& worldMin, const hkVector4& worldMax, int32_t numMarkers);
    ~hkp3AxisSweep() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    hkpBroadPhase::BroadPhaseType getType() const override;
    void addObject(
        hkpBroadPhaseHandle* object,
        const hkAabbUint32& aabb,
        hkArray<hkpBroadPhaseHandlePair>& pairsOut
    ) override;
    void
    addObject(hkpBroadPhaseHandle* object, const hkAabb& aabb, hkArray<hkpBroadPhaseHandlePair>& pairsOut) override;
    void addObjectBatch(
        const hkArrayBase<hkpBroadPhaseHandle*>& addObjectList,
        const hkArrayBase<hkAabb>& addAabbList,
        hkArray<hkpBroadPhaseHandlePair>& newPairs
    ) override;
    void removeObject(hkpBroadPhaseHandle* object, hkArray<hkpBroadPhaseHandlePair>& delPairsOut) override;
    void removeObjectBatch(
        const hkArrayBase<hkpBroadPhaseHandle*>& removeObjectList,
        hkArray<hkpBroadPhaseHandlePair>& delPairsOut
    ) override;
    int32_t getNumObjects() override;
    void updateAabbs(
        hkpBroadPhaseHandle* objects[],
        const hkAabb* aabbs,
        int32_t numObjects,
        hkArray<hkpBroadPhaseHandlePair>& addedPairsOut,
        hkArray<hkpBroadPhaseHandlePair>& removedPairsOut
    ) override;
    void updateAabbsUint32(
        hkpBroadPhaseHandle* objects[],
        const hkAabbUint32* aabbs,
        int32_t numObjects,
        hkArray<hkpBroadPhaseHandlePair>& addedPairsOut,
        hkArray<hkpBroadPhaseHandlePair>& removedPairsOut
    ) override;
    void defragment() override;
    void checkDeterminism() override;
    void getAllAabbs(hkArray<hkAabb>& allAabbs) override;
    void getAabb(const hkpBroadPhaseHandle* object, hkAabb& aabb) const override;
    void getExtents(hkVector4& worldMinOut, hkVector4& worldMaxOut) const override;
    void querySingleAabb(const hkAabb& aabbIn, hkArray<hkpBroadPhaseHandlePair>& pairs_out) const override;
    void
    reQuerySingleObject(const hkpBroadPhaseHandle* object, hkArray<hkpBroadPhaseHandlePair>& pairs_out) const override;
    bool areAabbsOverlapping(const hkpBroadPhaseHandle* bhA, const hkpBroadPhaseHandle* bhB) const override;
    void shiftAllObjects(
        const hkVector4& shiftDistance,
        hkVector4& effectiveShiftDistanceOut,
        hkArray<hkpBroadPhaseHandlePair>& newCollisionPairs
    ) override;
    void shiftBroadPhase(
        const hkVector4& shiftDistance,
        hkVector4& effectiveShiftDistanceOut,
        hkArray<hkpBroadPhaseHandlePair>& newCollisionPairs
    ) override;
    void getOffsetLowHigh32bit(hkVector4& offsetLow32bit, hkVector4& offsetHigh32bit) const override;
    void castRay(
        const hkpBroadPhase::hkpCastRayInput& rayInput,
        hkpBroadPhaseCastCollector* collectorBase,
        int32_t collectorStriding
    ) const override;
    int32_t getAabbCacheSize() const override;
    void calcAabbCache(const hkAabb& aabb, hkpBroadPhaseAabbCache* AabbCacheOut) const override;
    void calcAabbCache(
        const hkArrayBase<hkpCollidable*>& overlappingCollidables,
        hkpBroadPhaseAabbCache* AabbCacheOut
    ) const override;
    void castAabb(const hkpBroadPhase::hkpCastAabbInput& input, hkpBroadPhaseCastCollector& collector) const override;
    void
    set32BitOffsetAndScale(const hkVector4& offsetLow, const hkVector4& offsetHigh, const hkVector4& scale) override;
    virtual void querySingleAabbWithCollector(const hkAabb& aabbIn, hkpBroadPhaseCastCollector* collector) const;

    // Appends every overlapping pair of the numA nodes at pa and the numB nodes at pb to pairsOut; a pair with a marker
    // goes to or leaves the marker's overlap list as markerHandling says.
    void collide1Axis(
        const hkpBpNode* pa,
        int32_t numA,
        const hkpBpNode* pb,
        int32_t numB,
        MarkerHandling markerHandling,
        hkArray<hkpBroadPhaseHandlePair>& pairsOut
    );
    // Appends every overlapping pair of the numA nodes at pa whose later node is no marker.
    void collide1Axis(const hkpBpNode* pa, int32_t numA, hkArray<hkpBroadPhaseHandlePair>& pairsOut);

protected:
    void getAabbFromNode(const hkpBpNode& node, hkAabb& aabb) const;
    // Sweeps the x axis once for the nodes whose bits are set in bitFieldOfQueryNodes (the query nodes): each pair of a
    // query node and any other node overlapping in y and z goes to pairsOut, or, when one of them is a marker, the
    // other's index is added to the marker's overlaps (addMode) or removed from them.
    void
    queryBatchAabbSub(const uint32_t* bitFieldOfQueryNodes, hkArray<hkpBroadPhaseHandlePair>& pairsOut, hkBool addMode);
    // Writes the cache of the given overlapping nodes into AabbCacheOut.
    void
    calcAabbCacheInternal(const hkArrayBase<const hkpBpNode*>& overlaps, hkpBroadPhaseAabbCache* AabbCacheOut) const;
    // Shifts the other nodes' end point indices past the new node's.
    void updateNodesAfterInsert(hkpBpNode* nodes, int32_t numNodes, hkpBpNode& newNode);
    // Sets the bits of the nodes whose x interval overlaps queryNode's.
    void setBitsBasedOnXInterval(
        int32_t numNodes,
        int32_t x_value,
        const hkpBpNode& queryNode,
        uint16_t queryNodeIndex,
        uint32_t* bitField
    ) const;

public:
    // aabb quantized like a node's ends: each end moved by its offset, scaled, clamped to [0, MaxVal] and made a
    // 16-bit integer by adding QuantizationOffset and taking the float's bits shifted right by 7; min ends are made
    // even and max ends odd. Only m_min and m_max are written.
    static void _convertAabbToInt(
        const hkAabb& aabb,
        const hkVector4& offsetLow,
        const hkVector4& offsetHigh,
        const hkVector4& scale,
        hkAabbUint32& aabbOut
    ) {
        const __m128 zero = _mm_setzero_ps();
        alignas(16) float minQuant[4];
        alignas(16) float maxQuant[4];
        __m128 lo = _mm_add_ps(offsetLow.m_quad, aabb.m_min.m_quad);
        lo = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(lo, scale.m_quad), MaxVal), zero), QuantizationOffset);
        _mm_store_ps(minQuant, lo);
        __m128 hi = _mm_add_ps(offsetHigh.m_quad, aabb.m_max.m_quad);
        hi = _mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(hi, scale.m_quad), MaxVal), zero), QuantizationOffset);
        _mm_store_ps(maxQuant, hi);
        aabbOut.m_min[0] = static_cast<uint16_t>(std::bit_cast<uint32_t>(minQuant[0]) >> 7) & 0xFFFE;
        aabbOut.m_min[1] = static_cast<uint16_t>(std::bit_cast<uint32_t>(minQuant[1]) >> 7) & 0xFFFE;
        aabbOut.m_min[2] = static_cast<uint16_t>(std::bit_cast<uint32_t>(minQuant[2]) >> 7) & 0xFFFE;
        aabbOut.m_max[0] = static_cast<uint16_t>(std::bit_cast<uint32_t>(maxQuant[0]) >> 7) | 1;
        aabbOut.m_max[1] = static_cast<uint16_t>(std::bit_cast<uint32_t>(maxQuant[1]) >> 7) | 1;
        aabbOut.m_max[2] = static_cast<uint16_t>(std::bit_cast<uint32_t>(maxQuant[2]) >> 7) | 1;
    }
    // The same with the broad phase's own offsets and scale.
    void _convertAabbToInt(const hkAabb& aabb, hkAabbUint32& aabbOut) const {
        _convertAabbToInt(aabb, m_offsetLow, m_offsetHigh, m_scale, aabbOut);
    }
    // Returns m_aabb.
    const hkAabb& getOriginalAabb() const;
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    void _fixDeterministicOrderAfterNodeIdWasDecreased(int32_t nodeIndex);
    // Adds numNewEndPoints to each node's end point positions above offsets[axis] - 1.
    void updateNodesAfterBatchTailInsert(
        hkp3AxisSweep::hkpBpNode* nodes,
        int32_t numNodes,
        int32_t numNewEndPoints,
        int32_t* offsets
    );
    void updateNodesAfterDelete(hkp3AxisSweep::hkpBpNode* nodes, int32_t numNodes, hkp3AxisSweep::hkpBpNode& oldNode);

    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static void beginOverlap(
        hkp3AxisSweep::hkpBpNode& a,
        hkp3AxisSweep::hkpBpNode& b,
        hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& newPairsOut
    );
    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static void endOverlap(
        hkp3AxisSweep::hkpBpNode& a,
        hkp3AxisSweep::hkpBpNode& b,
        hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& deletedPairsOut
    );
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc). When b's handle has the marker bit, a's index joins
    // that marker's overlap list, else the pair (a, b handles) is appended to the array.
    static void beginOverlapCheckMarker(
        hkp3AxisSweep::hkpBpMarker* markers,
        hkp3AxisSweep::hkpBpNode& a,
        int32_t nodeIndexA,
        hkp3AxisSweep::hkpBpNode& b,
        hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator>& newPairsOut
    );
    // When b's handle has the marker bit, a's index leaves that marker's overlap list, else the pair (a, b handles) is
    // appended to the array.
    static void endOverlapCheckMarker(
        hkp3AxisSweep::hkpBpMarker* markers,
        hkp3AxisSweep::hkpBpNode& a,
        int32_t nodeIndexA,
        hkp3AxisSweep::hkpBpNode& b,
        hkArray<hkpBroadPhaseHandlePair>& deletedPairsOut
    );

    hkAabb m_aabb;
    hkVector4 m_offsetLow;
    hkVector4 m_offsetHigh;
    hkVector4 m_scale;
    hkVector4 m_offsetLow32bit;
    hkVector4 m_offsetHigh32bit;
    hkVector4 m_scale32bit;
    hkArray<hkpBpNode> m_nodes;
    hkpBpAxis m_axis[3];
    int32_t m_numMarkers;
    int32_t m_ld2NumMarkers;
    hkpBpMarker* m_markers;
    float m_intToFloatFloorCorrection;

    static inline OneMaskArray OneMask{1, 1, 1, 1};
    // The upper clamp of a scaled AABB end before quantization.
    static inline hkQuadReal MaxVal{65532.0f, 65532.0f, 65532.0f, 65532.0f};
    // Added to a clamped AABB end so that the float's bits hold its 16-bit integer.
    static inline hkQuadReal QuantizationOffset{65536.0f, 65536.0f, 65536.0f, 65536.0f};
};

// Returns the index of v's smallest x/y/z component and writes that component to *minOut.
int32_t getMinComponent(float* v, float* minOut);
int32_t max2Uint16(uint16_t a, uint16_t b);
// Flips bit bitIndex of the bit array bitArray and returns the word at index bitIndex >> 5.
int32_t toggleBit(int32_t bitIndex, uint32_t* bitArray);
// Clears bit bitIndex of the bit array bitArray and returns the 32-bit word at index bitIndex >> 5.
int32_t clearBit(int32_t bitIndex, uint32_t* bitArray);
hkpBroadPhase* hk3AxisSweep16CreateBroadPhase(const hkVector4& worldMin, const hkVector4& worldMax, int32_t numMarkers);
// Loads the 16-bit value at its argument.
int16_t getEpValue(const int16_t* endPoint);
// Tests bit bitIndex of the bit field at bitArray.
int32_t isBitSet(int32_t bitIndex, const uint32_t* bitArray);
// Zeroes 16-byte blocks from dest on, (bytesize >> 4) + 1 of them.
void memclear16(void* dest, int32_t bytesize);
