#pragma once

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Stopwatch/hkStopwatch.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkGeometry;
class hkgpMesh;
class hkPseudoRandomGenerator;

void cleanupLoadedObjecthkpSetLocalTransformsConstraintAtom(void* p);
void cleanupLoadedObjecthkpAngConstraintAtom(void* p);
void cleanupLoadedObjecthkpTwistLimitConstraintAtom(void* p);
void cleanupLoadedObjecthkpRagdollMotorConstraintAtom(void* p);
void cleanupLoadedObjecthkpMassChangerModifierConstraintAtom(void* p);
void cleanupLoadedObjecthkpCenterOfMassChangerModifierConstraintAtom(void* p);
void finishLoadedObjecthkpStiffSpringConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpSetLocalTranslationsConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpLinSoftConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpAngMotorConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpLinMotorConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpPulleyConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpMassChangerModifierConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpViscousSurfaceModifierConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpIgnoreModifierConstraintAtom(void* p, int32_t finishing);
void cleanupLoadedObjecthkpSetLocalRotationsConstraintAtom(void* p);
void cleanupLoadedObjecthkpConeLimitConstraintAtom(void* p);
void cleanupLoadedObjecthkpLinMotorConstraintAtom(void* p);
void cleanupLoadedObjecthkpOverwritePivotConstraintAtom(void* p);
void finishLoadedObjecthkpLinLimitConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpAngFrictionConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpRagdollMotorConstraintAtom(void* p, int32_t finishing);
void cleanupLoadedObjecthkpPulleyConstraintAtom(void* p);
void cleanupLoadedObjecthkpSoftContactModifierConstraintAtom(void* p);
void cleanupLoadedObjecthkpViscousSurfaceModifierConstraintAtom(void* p);
void finishLoadedObjecthkpSetLocalRotationsConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpConeLimitConstraintAtom(void* p, int32_t finishing);
void cleanupLoadedObjecthkpLinSoftConstraintAtom(void* p);
void cleanupLoadedObjecthkpLinLimitConstraintAtom(void* p);
void cleanupLoadedObjecthkpMovingSurfaceModifierConstraintAtom(void* p);
void finishLoadedObjecthkpConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpAngLimitConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpModifierConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpMovingSurfaceModifierConstraintAtom(void* p, int32_t finishing);
void sub_DE9B00();
void cleanupLoadedObjecthkpBallSocketConstraintAtom(void* p);
void cleanupLoadedObjecthkpAngMotorConstraintAtom(void* p);
void cleanupLoadedObjecthkpSimpleContactConstraintAtom(void* p);
// The finish function of hkpSimpleContactConstraintAtom's type info.
void finishLoadedObjecthkpSimpleContactConstraintAtom(void* p, int32_t finishing);
void cleanupLoadedObjecthkpStiffSpringConstraintAtom(void* p);
void cleanupLoadedObjecthkpSetLocalTranslationsConstraintAtom(void* p);
void cleanupLoadedObjecthkpLinConstraintAtom(void* p);
void cleanupLoadedObjecthkp2dAngConstraintAtom(void* p);
void cleanupLoadedObjecthkpAngLimitConstraintAtom(void* p);
void cleanupLoadedObjecthkpAngFrictionConstraintAtom(void* p);
void cleanupLoadedObjecthkpLinFrictionConstraintAtom(void* p);
void cleanupLoadedObjecthkpModifierConstraintAtom(void* p);
void cleanupLoadedObjecthkpIgnoreModifierConstraintAtom(void* p);
void finishLoadedObjecthkpBridgeConstraintAtom(void* p, int32_t finishing);
// Returns nothing, as finishLoadedObjecthkpBridgeConstraintAtom.
void finishLoadedObjecthkpBridgeAtoms(void* p, int32_t finishing);
void finishLoadedObjecthkpBallSocketConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpOverwritePivotConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpLinConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkp2dAngConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpAngConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpTwistLimitConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpLinFrictionConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpSoftContactModifierConstraintAtom(void* p, int32_t finishing);
void finishLoadedObjecthkpCenterOfMassChangerModifierConstraintAtom(void* p, int32_t finishing);
char intersectRayPlane(__m128* a1, __m128* a2, __m128* a3, float* a4);
double squaredDistanceToSegment(__m128* a1, __m128* a2, __m128* a3, float* a4);
__m128* normalToOctahedron(float* a1, __m128* a2);
__m128* computeMidpoint(__m128* a1, __m128* a2, __m128* a3);
__m128* computeTriangleCentroid(__m128* a1, __m128* a2, __m128* a3, __m128* a4);
__m128* computeQuadCentroid(__m128* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5);
double squaredDistanceToEdge(__m128* a1, __m128* a2, __m128* a3, __m128* a4, float* a5);
double squaredDistanceToTriangle(hkVector4* a1, __m128* a2, __m128* a3, __m128* a4, __m128* a5, __m128* a6);
double nearestDistanceBetweenLines(__m128* a1, __m128* a2, __m128* a3, __m128* a4, float* a5, float* a6);
bool isPointInsideTriangle(__m128* a1, __m128* a2, __m128* a3, __m128* a4, float a5);

// The geometry processing library's shared helpers. Only part is declared: the pool allocator of the meshes, the
// transposed vectors and the function objects.
struct hkGeometryProcessing {
    // The hash tables' bucket counts: the first prime above each power of two.
    enum Primes : int32_t {
        NEXT_PRIME_AFTER_8 = 11,
        NEXT_PRIME_AFTER_16 = 17,
        NEXT_PRIME_AFTER_32 = 37,
        NEXT_PRIME_AFTER_64 = 67,
        NEXT_PRIME_AFTER_128 = 131,
        NEXT_PRIME_AFTER_256 = 257,
        NEXT_PRIME_AFTER_512 = 521,
        NEXT_PRIME_AFTER_1024 = 1031,
        NEXT_PRIME_AFTER_2048 = 2053,
        NEXT_PRIME_AFTER_4096 = 4099,
        NEXT_PRIME_AFTER_8192 = 8209
    };

    // A hash table of T: the buckets, each an array of T, and the number of items.
    template <typename T, typename ALLOCATOR>
    class HashTable {
    public:
        // DeletingDestruct's free: the Havok heap's block of the table's size.
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        hkArray<hkArray<T, ALLOCATOR>, ALLOCATOR> m_buckets;
        int32_t m_numItems;
    };

    // Hands out items of T from pools of N.
    template <typename T, int32_t N, typename ALLOCATOR>
    struct PoolAllocator {
        struct Pool;
        struct Item;

        ~PoolAllocator();

        // Allocates a pool of N free items, adds it to m_pools and swaps it to the front; returns it.
        Pool* allocateNewPool();
        // Returns item to the pool it came from, freeing the pool once it holds no used item; reports an item of no
        // pool.
        void release(T* object);

        hkArray<Pool*> m_pools;
    };

    // Four points stored by component: m_x holds the four x, and so on.
    struct TransposedVectors {
        hkVector4 m_x;
        hkVector4 m_y;
        hkVector4 m_z;
    };

    // A function from IN_TYPE to OUT_TYPE.
    template <typename IN_TYPE, typename OUT_TYPE>
    struct IFunction {
        using typeIn = IN_TYPE;
        using typeOut = OUT_TYPE;

        virtual ~IFunction();

        virtual typeOut evaluate(const typeIn& x) const = 0;

        // DeletingDestruct's free (ourFree).
        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
    };

    // A FUNCTION_TYPE that is m_value everywhere.
    template <typename FUNCTION_TYPE>
    struct ConstFunction : FUNCTION_TYPE {
        ConstFunction(const FUNCTION_TYPE::typeOut& value) : m_value(value) {}

        FUNCTION_TYPE::typeOut evaluate(const FUNCTION_TYPE::typeIn&) const override {
            return m_value;
        }

        FUNCTION_TYPE::typeOut m_value;
    };

    using IFunction_4_1 = IFunction<hkVector4, float>;

    // The interval [m_bounds[0], m_bounds[1]] a set of vertices covers along a direction.
    template <typename T>
    struct Span {
        using type = T;

        T m_bounds[2];
    };

    // A vertex of an iso-contour: the triangle edge it lies on and its parameter along that edge.
    struct IsoVertex {
        // Havok SDK signature (hkInternal.lib, tied by bytes).
        static int32_t
        solveTriangle(const float* const values, hkGeometryProcessing::IsoVertex* const verticesOut, float orientation);

        int32_t m_edge;
        float m_solution;
    };

    // Times its scope with m_stopWatch and reports the time under m_name unless m_silent.
    struct PerformanceScope {
        hkStopwatch m_stopWatch;
        const char* m_name;
        bool m_silent;
    };

    // Times its scope with m_stopWatch and adds the time and one count to *m_storage.
    struct AccumulatorScope {
        // The time accumulated and the number of scopes counted.
        struct Item {
            float m_time;
            int32_t m_count;
        };

        hkStopwatch m_stopWatch;
        Item* m_storage;
    };

    // The point where the planes p0, p1 and p2 (normal in xyz, offset in w) meet, written to intersection; false
    // when the normals' matrix does not invert.
    static bool intersectPlanes(const hkVector4& p0, const hkVector4& p1, const hkVector4& p2, hkVector4& intersection);
    // The unit normal of the octahedron map coordinates uv (x and y in [0, 1]), written to normal with w zero.
    static void octahedronToNormal(const hkVector4& uv, hkVector4& normal);
    // The point of vertices furthest along direction, written to vertexOut with its index in w (the index bits or
    // 0x3F000000).
    static void getSupportingVertex(
        const hkArray<TransposedVectors>& transposedVertices,
        const hkVector4& direction,
        hkVector4& vertexOut
    );
    // The triangle's quality: 2 sqrt(3) times its doubled area over the sum of its squared edge lengths (1 for an
    // equilateral triangle), or 0 when that sum is not above 1.1920929e-07f.
    static float computeTriangleQuality(const hkVector4& p0, const hkVector4& p1, const hkVector4& p2);
    // Appends vertices to output four by four, transposed; the last block repeats its first point in the lanes past
    // the end.
    static void buildTransposedArray(const hkArray<hkVector4>& vertices, hkArray<TransposedVectors>& output);
    // Whether the triangles of the three points at triangleA and the three at triangleB intersect, by separating axes.
    static int32_t checkTrianglesIntersection(const hkVector4* triangleA, const hkVector4* triangleB);
    // Calls generateUniformDistribution on the geometry's points, passing seed on as its maxIterations, with seed 0 and
    // no density function.
    static bool
    generateRandomDistribution(const hkGeometry& geometry, int32_t numPoints, hkArray<hkVector4>& points, int32_t seed);
    // Scatters numPoints points over the geometry's surface mesh and relaxes them maxIterations times towards an even
    // spread weighted by densityFunction (none: 1 everywhere), appending them to points with the density in w; false
    // when the mesh yields no random points.
    static bool generateUniformDistribution(
        const hkGeometry& geometry,
        int32_t numPoints,
        hkArray<hkVector4>& points,
        int32_t maxIterations,
        int32_t seed,
        const IFunction_4_1* densityFunction
    );
};

template <typename IN_TYPE, typename OUT_TYPE>
hkGeometryProcessing::IFunction<IN_TYPE, OUT_TYPE>::~IFunction() {}

// A pool item: while free, its first word links the next free item; while used, it holds a T.
template <typename T, int32_t N, typename ALLOCATOR>
struct hkGeometryProcessing::PoolAllocator<T, N, ALLOCATOR>::Item {
    union {
        Item* m_nextFree;
        uint8_t m_data[sizeof(T)];
    };
};

// N items, the first free one and the number in use.
template <typename T, int32_t N, typename ALLOCATOR>
struct hkGeometryProcessing::PoolAllocator<T, N, ALLOCATOR>::Pool {
    Item m_items[N];
    Item* m_free;
    int32_t m_used;
};

// Frees each pool as a block of sizeof(Pool); the member frees the pool array.
template <typename T, int32_t N, typename ALLOCATOR>
hkGeometryProcessing::PoolAllocator<T, N, ALLOCATOR>::~PoolAllocator() {
    for (int32_t i = 0; i < m_pools.m_size; ++i) {
        hkMemoryRouter::getInstance().m_heap->blockFree(m_pools.m_data[i], static_cast<int32_t>(sizeof(Pool)));
    }
}

// The SDK's allocateNewPool (hkGeometryProcessing.inl): a pool of N items from the heap, chained free from the first
// item on, appended to m_pools and swapped to the front.
template <typename T, int32_t N, typename ALLOCATOR>
hkGeometryProcessing::PoolAllocator<T, N, ALLOCATOR>::Pool*
hkGeometryProcessing::PoolAllocator<T, N, ALLOCATOR>::allocateNewPool() {
    Pool* pPool =
        static_cast<Pool*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(Pool))));
    Item* pPrev = nullptr;
    for (int32_t i = N - 1; i >= 0; --i) {
        pPool->m_items[i].m_nextFree = pPrev;
        pPrev = &pPool->m_items[i];
    }
    pPool->m_used = 0;
    pPool->m_free = pPrev;
    m_pools.pushBack(pPool);
    if (m_pools.m_size > 1) {
        Pool* pFirst = m_pools.m_data[0];
        m_pools.m_data[0] = m_pools.m_data[m_pools.m_size - 1];
        m_pools.m_data[m_pools.m_size - 1] = pFirst;
    }
    return pPool;
}

// The SDK's release: item's T destroyed, the item looked up in each pool's item range, an emptied pool freed
// (releasePool) and swapped out of m_pools (removeAt); an item of no pool is reported.
template <typename T, int32_t N, typename ALLOCATOR>
void hkGeometryProcessing::PoolAllocator<T, N, ALLOCATOR>::release(T* object) {
    if (object) {
        object->~T();
        const void* pItem = object;
        for (int32_t i = 0; i < m_pools.m_size; ++i) {
            if (pItem >= static_cast<const void*>(m_pools.m_data[i]->m_items) &&
                pItem <= static_cast<const void*>(&m_pools.m_data[i]->m_items[N - 1])) {
                if (--m_pools.m_data[i]->m_used == 0) {
                    hkMemoryRouter::getInstance().m_heap->blockFree(
                        m_pools.m_data[i], static_cast<int32_t>(sizeof(Pool))
                    );
                    m_pools.removeAt(i);
                }
                return;
            }
        }
        char buf[512];
        hkErrStream errStream(buf, 512);
        errStream << "Item does not belong to the pool allocator";
        if (hkError::s_instance->message(
                hkError::MESSAGE_ERROR, static_cast<int32_t>(0x90355365), buf,
                "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/GeometryProcessing/"
                "hkGeometryProcessing.inl",
                175
            )) {
            __builtin_debugtrap();
        }
    }
}

// hkGeometryProcessing with the helper types of its operations (hkGeometryProcessing.cpp); it adds no members.
struct hkGeometryProcessingInternals : public hkGeometryProcessing {
    // Keeps the leaf nearest the query point: a leaf whose box centre is closer than m_distanceSquared replaces m_leaf;
    // addLeaf returns 1. The deleting destructor frees the object from the Havok heap.
    struct NearestFeaturePolicy
      : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::ClosestPointCollector {
        ~NearestFeaturePolicy() override;

        float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf) override;

        // Frees the Havok heap's block of the policy's size.
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* m_leaf;
    };

    // Appends numPoints random points of the mesh's bounding box that lie inside the mesh to points (w 1), giving up
    // after 8 * numPoints misses in a row; returns whether all were found.
    static bool generateRandomDistribution(
        const hkgpMesh& mesh,
        int32_t numPoints,
        hkArray<hkVector4>& points,
        hkPseudoRandomGenerator& rndGenerator
    );
};

extern hkClass hkpSimpleContactConstraintAtomClass;
extern hkClass hkpBallSocketConstraintAtomClass;
extern hkClass hkpStiffSpringConstraintAtomClass;
extern hkClass hkpSetLocalTransformsConstraintAtomClass;
extern hkClass hkpSetLocalTranslationsConstraintAtomClass;
extern hkClass hkpSetLocalRotationsConstraintAtomClass;
extern hkClass hkpOverwritePivotConstraintAtomClass;
extern hkClass hkpLinConstraintAtomClass;
extern hkClass hkpLinSoftConstraintAtomClass;
extern hkClass hkpLinLimitConstraintAtomClass;
extern hkClass hkp2dAngConstraintAtomClass;
extern hkClass hkpAngConstraintAtomClass;
extern hkClass hkpAngLimitConstraintAtomClass;
extern hkClass hkpTwistLimitConstraintAtomClass;
extern hkClass hkpConeLimitConstraintAtomClass;
extern hkClass hkpAngFrictionConstraintAtomClass;
extern hkClass hkpAngMotorConstraintAtomClass;
extern hkClass hkpRagdollMotorConstraintAtomClass;
extern hkClass hkpLinFrictionConstraintAtomClass;
extern hkClass hkpLinMotorConstraintAtomClass;
extern hkClass hkpPulleyConstraintAtomClass;
extern hkClass hkpModifierConstraintAtomClass;
extern hkClass hkpSoftContactModifierConstraintAtomClass;
extern hkClass hkpMassChangerModifierConstraintAtomClass;
extern hkClass hkpCenterOfMassChangerModifierConstraintAtomClass;
extern hkClass hkpViscousSurfaceModifierConstraintAtomClass;
extern hkClass hkpMovingSurfaceModifierConstraintAtomClass;
extern hkClass hkpIgnoreModifierConstraintAtomClass;
inline float f0_25 = 0.25f;
