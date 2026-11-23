#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkStridedVertices;

// The item lists and the vertex and triangle items of a hkgpAbstractMesh, and the topology check's result.
struct hkgpAbstractMeshDefinitions {
    // A list of items of T (derived from BASE) taken from ALLOCATOR.
    template <typename T, typename BASE, typename ALLOCATOR>
    struct List {
        // The base of every item: the links of the doubly linked list of items (T is derived from the item).
        struct Item : BASE {
            T* m_next;
            T* m_prev;
        };

        // An empty list (m_uid is deliberately left uninitialised); destroying it hands every item
        // back (reset), then the allocator frees its pools.
        List() : m_allocator(), m_used(nullptr), m_numUsed(0) {}
        ~List() {
            reset();
        }

        // Frees the allocator's pools and empties the list.
        void reset();
        // Takes a new item from the allocator, copies item into it and adds it at the head of the list.
        T* allocate(const T& value);
        // Unlinks item from the list and hands it back to the allocator.
        void release(T* item);

        ALLOCATOR m_allocator;
        T* m_used;
        int32_t m_numUsed;
        uint32_t m_uid;
    };

    // A vertex item: only the list links.
    template <typename VERTEX, typename BASE>
    struct Vertex
      : List<VERTEX, BASE, hkGeometryProcessing::PoolAllocator<VERTEX, 32, hkContainerHeapAllocator>>::Item {
        // The base the mesh's vertex list is instantiated with.
        using tItemBase = BASE;

        // The SDK's isOrdered: a orders before b (its compare below 0) or equals it (0), that is b does not order
        // before a by VERTEX's operator<.
        static bool isOrdered(const VERTEX* a, const VERTEX* b) {
            return !(*b < *a);
        }
    };

    // A triangle item: the list links, its three vertices and the packed links to its three neighbours.
    template <typename TRIANGLE, typename BASE, typename VERTEX>
    struct Triangle
      : List<TRIANGLE, BASE, hkGeometryProcessing::PoolAllocator<TRIANGLE, 32, hkContainerHeapAllocator>>::Item {
        // The base the mesh's triangle list is instantiated with.
        using tItemBase = BASE;

        VERTEX m_vertices[3];
        uintptr_t m_links[3];
    };

    // The result of hkgpAbstractMesh's topology check.
    enum Error : int32_t {
        VALID,
        ORPHAN_VERTEX,
        ZOMBIE_VERTEX,
        ZOMBIE_TRIANGLE,
        VOID_TRIANGLE,
        INVALID_BIND,
        SELF_EDGE,
        NULL_EDGE,
    };
};

// The half-edge mesh's topology types.
struct hkgpTopology {
    // A directed edge: the triangle it belongs to and which of the triangle's three edges it is.
    template <typename EDGE, typename VERTEX, typename TRIANGLE, typename VERTEX_REF>
    struct Edge {
        // The SDK's inline queries the generic EdgeIterator walks with: the start and end vertices, a naked edge (its
        // link holds no triangle; the edge index sits in the link's low two bits) and the master of an edge pair (the
        // edge whose vertices are ordered, or a naked one).
        VERTEX_REF start() const {
            return m_triangle->m_vertices[m_index];
        }
        VERTEX_REF end() const {
            return m_triangle->m_vertices[(m_index + 1) % 3];
        }
        bool isNaked() const {
            return (m_triangle->m_links[m_index] & ~static_cast<uintptr_t>(3)) == 0;
        }
        bool isMaster() const {
            return VERTEX::isOrdered(start(), end()) || isNaked();
        }

        static uint32_t ip1mod3(uint32_t i) {
            return (9 >> (i << 1)) & 3;
        }
        static uint32_t ip2mod3(uint32_t i) {
            return (18 >> (i << 1)) & 3;
        }

        // The edge of no triangle.
        static const EDGE& null() {
            static const EDGE e{};
            return e;
        }

        // A link (hkgpTopology's Uid) packs the neighbour triangle's address with the shared edge's index in its low
        // two bits.
        uintptr_t uid() const {
            return reinterpret_cast<uintptr_t>(m_triangle) + m_index;
        }
        bool isValid() const {
            return m_triangle != nullptr;
        }
        const EDGE& self() const {
            return static_cast<const EDGE&>(*this);
        }
        EDGE next() const {
            EDGE e;
            e.m_triangle = m_triangle;
            e.m_index = ip1mod3(m_index);
            return e;
        }
        EDGE prev() const {
            EDGE e;
            e.m_triangle = m_triangle;
            e.m_index = ip2mod3(m_index);
            return e;
        }
        EDGE link() const {
            const uintptr_t id = m_triangle->m_links[m_index];
            EDGE e;
            e.m_index = static_cast<uint32_t>(id & 3);
            e.m_triangle = reinterpret_cast<TRIANGLE*>(id & ~static_cast<uintptr_t>(3));
            return e;
        }
        EDGE nextCw() const {
            return link().prev();
        }
        // The edge of the pair whose start orders before its end, or this one when it is naked. The end is read
        // through ip1mod3, unlike isMaster's.
        EDGE master() const {
            if (VERTEX::isOrdered(start(), next().start()) || isNaked()) {
                return self();
            }
            return link();
        }

        void bind(const Edge& edge) const {
            m_triangle->m_links[m_index] = edge.uid();
            if (edge.isValid()) {
                edge.m_triangle->m_links[edge.m_index] = uid();
            }
        }
        void unbind() const {
            if (!isNaked()) {
                const EDGE l = link();
                l.m_triangle->m_links[l.m_index] = 0;
            }
            m_triangle->m_links[m_index] = 0;
        }
        // Turns the edge within the quad of its two triangles; returns the previous edge, the flipped one.
        EDGE flip() const {
            const EDGE lk = link();
            const EDGE ep = prev();
            const EDGE lp = lk.prev();
            m_triangle->m_vertices[m_index] = lp.start();
            lk.m_triangle->m_vertices[lk.m_index] = ep.start();
            lk.bind(ep.link());
            bind(lp.link());
            ep.bind(lp);
            return prev();
        }

        TRIANGLE* m_triangle;
        uint32_t m_index;
    };
};

// A half-edge mesh of VERTEX and TRIANGLE items (the SDK's hkgpAbstractMesh): the vertex and triangle lists, each
// with its pool allocator. The element types must be complete where an instance is used as a member.
template <typename EDGE, typename VERTEX, typename TRIANGLE, typename ALLOCATOR>
class hkgpAbstractMesh : public hkReferencedObject {
public:
    using VertexPoolAllocator = hkGeometryProcessing::PoolAllocator<VERTEX, 32, ALLOCATOR>;
    using TrianglePoolAllocator = hkGeometryProcessing::PoolAllocator<TRIANGLE, 32, ALLOCATOR>;
    using VertexList = hkgpAbstractMeshDefinitions::List<VERTEX, typename VERTEX::tItemBase, VertexPoolAllocator>;
    using TriangleList = hkgpAbstractMeshDefinitions::
        List<TRIANGLE, typename TRIANGLE::tItemBase, TrianglePoolAllocator>;

    // Walks the mesh's edges, each one once; only m_edge is declared.
    struct EdgeIterator {
        EdgeIterator() = default;
        // Private constructor: stores edge and, when it is not reported, moves on with next.
        EdgeIterator(EDGE edge);

        // Turns round the edge's triangles to the next reported edge.
        void next();

        EDGE m_edge;
    };

    ~hkgpAbstractMesh() override;

    // Empties the vertex and triangle lists.
    virtual void reset() {
        m_vertices.reset();
        m_triangles.reset();
    }

    // Returns the edge from apStart to apEnd (the null edge when none) through apResult.
    EDGE* findEdgeOrdered(EDGE* apResult, const VERTEX* apStart, const VERTEX* apEnd) const;

    VertexList m_vertices;
    TriangleList m_triangles;
};

// The lists reset and free their pools themselves, triangles first.
template <typename EDGE, typename VERTEX, typename TRIANGLE, typename ALLOCATOR>
hkgpAbstractMesh<EDGE, VERTEX, TRIANGLE, ALLOCATOR>::~hkgpAbstractMesh() {}

// The pools go back to the Havok heap, then the list is emptied.
template <typename T, typename BASE, typename ALLOCATOR>
void hkgpAbstractMeshDefinitions::List<T, BASE, ALLOCATOR>::reset() {
    m_allocator.clear();
    m_used = nullptr;
    m_numUsed = 0;
}

// The first free item of the front pool (a new pool when there is none or it is full) becomes a copy of item, linked at
// the head of the list.
template <typename T, typename BASE, typename ALLOCATOR>
T* hkgpAbstractMeshDefinitions::List<T, BASE, ALLOCATOR>::allocate(const T& value) {
    typename ALLOCATOR::Pool* pPool;
    if (!m_allocator.m_pools.m_size || !(pPool = m_allocator.m_pools.m_data[0])->m_free) {
        pPool = m_allocator.allocateNewPool();
    }
    typename ALLOCATOR::Item* pItem = pPool->m_free;
    pPool->m_free = pItem->m_nextFree;
    ++pPool->m_used;
    T* pResult = ::new (pItem->m_data) T(value);
    pResult->m_prev = nullptr;
    pResult->m_next = m_used;
    if (m_used) {
        m_used->m_prev = pResult;
    }
    ++m_numUsed;
    m_used = pResult;
    return pResult;
}

template <typename T, typename BASE, typename ALLOCATOR>
void hkgpAbstractMeshDefinitions::List<T, BASE, ALLOCATOR>::release(T* item) {
    T* pNext = item->m_next;
    T* pPrev = item->m_prev;
    if (pNext) {
        pNext->m_prev = pPrev;
    }
    if (pPrev) {
        pPrev->m_next = pNext;
    }
    else {
        m_used = pNext;
    }
    --m_numUsed;
    m_allocator.release(item);
}

// The SDK's EdgeIterator(Edge first): an edge that is not its pair's master moves on to the next reported one.
template <typename EDGE, typename VERTEX, typename TRIANGLE, typename ALLOCATOR>
hkgpAbstractMesh<EDGE, VERTEX, TRIANGLE, ALLOCATOR>::EdgeIterator::EdgeIterator(EDGE edge) : m_edge(edge) {
    if (m_edge.m_triangle && !m_edge.isMaster()) {
        next();
    }
}

// The SDK's EdgeIterator::next: turns round the edge's triangle to its next master edge; past the triangle's last
// edge, moves on to the next triangle's edge 0 until that one is a master.
template <typename EDGE, typename VERTEX, typename TRIANGLE, typename ALLOCATOR>
void hkgpAbstractMesh<EDGE, VERTEX, TRIANGLE, ALLOCATOR>::EdgeIterator::next() {
    while (m_edge.m_triangle) {
        do {
            m_edge.m_index = (m_edge.m_index + 1) % 3;
        }
        while (m_edge.m_index && !m_edge.isMaster());
        if (m_edge.m_index != 0 && m_edge.isMaster()) {
            return;
        }
        m_edge.m_triangle = m_edge.m_triangle->m_next;
        m_edge.m_index = 0;
        if (m_edge.m_triangle && m_edge.isMaster()) {
            return;
        }
    }
}

// Tests each triangle's edges 2, 0 and 1 in that order.
template <typename EDGE, typename VERTEX, typename TRIANGLE, typename ALLOCATOR>
EDGE* hkgpAbstractMesh<EDGE, VERTEX, TRIANGLE, ALLOCATOR>::findEdgeOrdered(
    EDGE* apResult,
    const VERTEX* apStart,
    const VERTEX* apEnd
) const {
    for (TRIANGLE* pTriangle = m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        for (uint32_t i = 0, j = 2; i < 3; j = i++) {
            if (pTriangle->m_vertices[j] == apStart && pTriangle->m_vertices[i] == apEnd) {
                apResult->m_triangle = pTriangle;
                apResult->m_index = j;
                return apResult;
            }
        }
    }
    apResult->m_triangle = nullptr;
    apResult->m_index = 0;
    return apResult;
}

// The element types of hkgpMesh's mesh: items derived from hkgpAbstractMeshDefinitions::Vertex and ::Triangle, and
// an edge derived from hkgpTopology::Edge.
struct hkgpMeshBase {
    struct BaseVertex {};
    struct BaseTriangle {};
    struct Vertex;
    struct Triangle;

    // A mesh vertex: the position, the normal, user data, the reference count and a tag.
    struct Vertex : hkgpAbstractMeshDefinitions::Vertex<Vertex, BaseVertex> {
        Vertex() = default;
        Vertex(const hkgpMeshBase::Vertex& other);

        hkVector4 m_position;
        hkVector4 m_normal;
        hkVector4 m_data;
        int32_t m_refs;
        int32_t m_tag;
    };

    // A mesh triangle: the plane, the part, plane, material and tag ids and the index of its leaf in the triangle tree.
    struct Triangle : hkgpAbstractMeshDefinitions::Triangle<Triangle, BaseTriangle, Vertex*> {
        Triangle() = default;
        Triangle(const hkgpMeshBase::Triangle& other);

        hkVector4 m_plane;
        int32_t m_partId;
        int32_t m_planeId;
        int32_t m_material;
        int32_t m_tag;
        uint32_t m_leafIdx;
    };

    // A directed edge of a triangle.
    struct Edge : hkgpTopology::Edge<Edge, Vertex, Triangle, Vertex*> {};
};

// hkgpMesh's abstract mesh.
using hkgpMeshBaseAbstractMesh =
    hkgpAbstractMesh<hkgpMeshBase::Edge, hkgpMeshBase::Vertex, hkgpMeshBase::Triangle, hkContainerHeapAllocator>;

// hkgpMesh's vertex pools.
using hkgpMeshBaseVertexPoolAllocator = hkGeometryProcessing::
    PoolAllocator<hkgpMeshBase::Vertex, 32, hkContainerHeapAllocator>;

// hkgpMesh's triangle pools.
using hkgpMeshBaseTrianglePoolAllocator = hkGeometryProcessing::
    PoolAllocator<hkgpMeshBase::Triangle, 32, hkContainerHeapAllocator>;

// The triangulators' base; it adds no virtual of its own.
class hkgpTriangulatorBase : public hkReferencedObject {
public:
    // The empty bases of the triangulator's mesh element types, like hkgpMeshBase's BaseVertex and BaseTriangle.
    // hkgpAbstractMeshDefinitions' items derive from them, so they must be complete.
    struct VertexBase {};
    struct TriangleBase {};

    // The triangulator's default per-edge data: a list of tags.
    template <typename ALLOCATOR>
    struct DefaultEdgeData {
        // The implicit copy assignment: the tag list is copied as an hkArray, the storage reallocated from the heap
        // when other's size exceeds the capacity. backupBothEdgeData calls it for `values[i].m_data = *data`.
        DefaultEdgeData& operator=(const DefaultEdgeData& other);

        hkInplaceArray<hkUlong, 4, ALLOCATOR> m_tags;
    };
};
// The triangulator: its mesh, the traversal stack, the edge data and its pools, the roots of the triangle grid, and the
// domain (its margin and the local/world matrices). Of hkgpTriangulatorBase's virtual functions, it overrides only the
// destructor.
template <typename VERTEX_BASE, typename TRIANGLE_BASE, typename EDGE_DATA, typename ALLOCATOR, int32_t N>
class hkgpTriangulatorType : public hkgpTriangulatorBase {
public:
    // A triangulator vertex: the list links, its integer coordinates, the constrained flag and user data. PC bit-fields
    // fill from bit 0.
    struct Vertex : hkgpAbstractMeshDefinitions::Vertex<Vertex, VERTEX_BASE> {
        // Orders by x, then y (signed compares).
        bool operator<(const Vertex& other) const {
            return m_x < other.m_x || (m_x == other.m_x && m_y < other.m_y);
        }

        int32_t m_x;
        int32_t m_y;
        uint32_t m_constraint : 1;
        uint32_t m_data : 31;
    };

    // A triangulator triangle: the list links, vertices and neighbour links, then the constrained-edge bits, the root
    // flag, the region and the set bits. Bit-fields fill from bit 0, so partition's `|= 0xFFF0` sets m_region.
    struct Triangle : hkgpAbstractMeshDefinitions::Triangle<Triangle, TRIANGLE_BASE, Vertex*> {
        uint16_t m_constraints : 3;
        uint16_t m_root : 1;
        uint16_t m_region : 12;
        uint16_t m_sets;
    };

    // A directed edge of a triangle. Its constraint bit sits on its master's triangle.
    struct Edge : hkgpTopology::Edge<Edge, Vertex, Triangle, Vertex*> {
        using tBase = hkgpTopology::Edge<Edge, Vertex, Triangle, Vertex*>;

        bool isConstrained() const {
            const Edge m = tBase::master();
            return (m.m_triangle->m_constraints & (1u << m.m_index)) != 0;
        }
        void setConstrained() const {
            const Edge m = tBase::master();
            m.m_triangle->m_constraints |= (1u << m.m_index);
        }
        bool getAndClearConstraint() const {
            const bool constrained = isConstrained();
            if (constrained) {
                const Edge m = tBase::master();
                m.m_triangle->m_constraints &= ~(1u << m.m_index);
            }
            return constrained;
        }
        // Whether the edge is bound, not constrained, and both triangles of the flipped quad keep their orientation.
        bool canFlip() const {
            if (!tBase::isNaked() && !isConstrained()) {
                const Edge lk = tBase::link();
                const Edge ep = tBase::prev();
                const Edge lp = lk.prev();
                return orientation(ep.start(), tBase::start(), lp.start()) > 0 &&
                    orientation(lp.start(), lk.start(), ep.start()) > 0;
            }
            return false;
        }
        // The topology's flip, the constraints of the quad's two far sides kept on the same sides.
        Edge flip() const {
            const bool c0 = tBase::prev().getAndClearConstraint();
            const bool c1 = tBase::link().prev().getAndClearConstraint();
            const Edge edge = tBase::flip();
            if (c0) {
                edge.link().next().setConstrained();
            }
            if (c1) {
                edge.next().setConstrained();
            }
            return edge;
        }
    };

    // The hash table's box of an edge's data: the edge's two vertices (its key) and the data.
    struct EdgeDataBox {
        Vertex* m_vertices[2];
        EDGE_DATA* m_data;
    };

    // An edge's data and whether it has any.
    struct EdgeDataValue {
        // No data; the tag list starts empty on its inline tags.
        EdgeDataValue() : m_hasData(false), m_data() {}
        // The tag list frees its storage itself.
        ~EdgeDataValue() {}

        bool m_hasData;
        EDGE_DATA m_data;
    };

    // Where a point lies in the mesh: its kind and an edge of the triangle, edge or vertex.
    struct Location {
        enum eType { IN_TRIANGLE, ON_EDGE, ON_VERTEX, NOT_FOUND, CYCLE };

        hkEnum<eType, int32_t> m_type;
        Edge m_edge;
    };

    // The result of an insertion: its outcome and the edge it made or stopped at.
    struct Insertion {
        enum eType {
            SUCCESS,
            ALREADY_PRESENT,
            ON_CONSTRAINED_EDGE,
            CROSSING_CONSTRAINED_EDGE,
            CROSSING_CONSTRAINED_VERTEX,
            CANNOT_RESOLVE_CROSSING,
            CANNOT_LOCATE_ORIGIN,
            CANNOT_LOCATE_DESTINATION,
            INVALID_ORIENTATION,
            EDGE_IS_VERTEX,
            FAILED
        };

        Insertion(eType type, const Edge& edge) : m_edge(edge) {
            m_type.m_storage = type;
        }

        hkEnum<eType, int32_t> m_type;
        Edge m_edge;
    };

    // The 2D cross product of b - a and c - a, in wrapping 32-bit arithmetic.
    static int32_t orientation(const Vertex* a, const Vertex* b, const Vertex* c) {
        const uint32_t bx = static_cast<uint32_t>(b->m_x) - static_cast<uint32_t>(a->m_x);
        const uint32_t by = static_cast<uint32_t>(b->m_y) - static_cast<uint32_t>(a->m_y);
        const uint32_t cx = static_cast<uint32_t>(c->m_x) - static_cast<uint32_t>(a->m_x);
        const uint32_t cy = static_cast<uint32_t>(c->m_y) - static_cast<uint32_t>(a->m_y);
        return static_cast<int32_t>(cy * bx - cx * by);
    }
    // Whether d is inside the circumcircle of a, b and c: the 2x2 determinants and squared lengths wrap in 32 bits
    // and are summed in 64.
    static bool incircle(const Vertex* a, const Vertex* b, const Vertex* c, const Vertex* d) {
        const uint32_t ax = static_cast<uint32_t>(a->m_x) - static_cast<uint32_t>(d->m_x);
        const uint32_t ay = static_cast<uint32_t>(a->m_y) - static_cast<uint32_t>(d->m_y);
        const uint32_t bx = static_cast<uint32_t>(b->m_x) - static_cast<uint32_t>(d->m_x);
        const uint32_t by = static_cast<uint32_t>(b->m_y) - static_cast<uint32_t>(d->m_y);
        const uint32_t cx = static_cast<uint32_t>(c->m_x) - static_cast<uint32_t>(d->m_x);
        const uint32_t cy = static_cast<uint32_t>(c->m_y) - static_cast<uint32_t>(d->m_y);
        const uint64_t d0 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(ax * by - bx * ay)));
        const uint64_t d1 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(bx * cy - cx * by)));
        const uint64_t d2 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(cx * ay - ax * cy)));
        const uint64_t l0 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(ax * ax + ay * ay)));
        const uint64_t l1 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(bx * bx + by * by)));
        const uint64_t l2 = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(cx * cx + cy * cy)));
        return static_cast<int64_t>(0 - (l0 * d1 + l1 * d2 + l2 * d0)) < 0;
    }
    // Whether the edge needs no flip: naked, constrained, or its link's apex outside its triangle's circumcircle.
    static bool isConform(const Edge& edge) {
        if (edge.isNaked() || edge.isConstrained()) {
            return true;
        }
        const Edge lk = edge.link();
        return !incircle(
            edge.m_triangle->m_vertices[Edge::ip2mod3(edge.m_index)],
            edge.start(),
            edge.m_triangle->m_vertices[Edge::ip1mod3(edge.m_index)],
            lk.m_triangle->m_vertices[Edge::ip2mod3(lk.m_index)]
        );
    }
    // Rounds half away from zero, the half added in double.
    static int32_t round(float x) {
        return static_cast<int32_t>(x < 0.0f ? static_cast<double>(x) - 0.5 : static_cast<double>(x) + 0.5);
    }
    static int32_t clamp(int32_t x, int32_t lo, int32_t hi) {
        return x < lo ? lo : (x > hi ? hi : x);
    }
    // The point in the quantized frame, before rounding.
    hkVector4 transformUnquantized(const hkVector4& kWorld) const {
        hkVector4 kLocal;
        m_worldToLocal.transformPosition(kWorld, kLocal);
        return kLocal;
    }
    // The point's x and y rounded and clamped to the margin; the vertex's other members are left unset.
    Vertex quantize(const hkVector4& kLocal) const {
        const int32_t iLow = m_margin;
        const int32_t iHigh = 0x7FFF - m_margin;
        Vertex kVertex;
        kVertex.m_x = clamp(round(kLocal(0)), iLow, iHigh);
        kVertex.m_y = clamp(round(kLocal(1)), iLow, iHigh);
        return kVertex;
    }
    Vertex transform(const hkVector4& kWorld) const {
        return quantize(transformUnquantized(kWorld));
    }

    // Does not call setDomain; callers call it after construction. Leaves the mesh, stack and pools empty and no edge
    // data table.
    hkgpTriangulatorType() : m_edgeData(nullptr) {}
    ~hkgpTriangulatorType() override;

    // Sets the domain's quantized frame from the rectangle dmin..dmax rotated by angle, then resets the mesh.
    void setDomain(const hkVector4& dmin, const hkVector4& dmax, float angle, bool margin);
    // Sets the domain's quantized frame from the bounds of the points transformed by worldToXYPlane: dmax gives the
    // frame a margin.
    void
    setDomainFromPoints(const hkStridedVertices& points, const hkMatrix4& worldToXYPlane, bool dmax, bool keepRatio);
    // The SDK's setDomainFromPlanarPoints: setDomainFromPoints with the rotation taking planeNormal to +z.
    void setDomainFromPlanarPoints(
        const hkStridedVertices& points,
        const hkVector4& planeNormal,
        bool margin,
        bool keepRatio
    );
    // Empties the mesh; setDomain passes true.
    void reset(bool createDomainTriangulation);

    // Returns the data of edge from the edge data table (m_edgeData, hashed on its two vertices' coordinates), or
    // null when the edge is invalid or the table or the edge's box is missing.
    const EDGE_DATA* getEdgeData(const Edge& edge) const;
    // Over the edges of the triangles of region bit 0, returns whether more edges have data of their own than through
    // their twin.
    int32_t getEdgeDataOrientation() const;
    // Sets every triangle's region to the null region, then floods the regions from locateVertex(0, 0) over m_stack and
    // a 64-edge inplace stack; returns the region count.
    int32_t partition();
    // Passes over the triangles' edges until one pass flips none.
    void conformEdges();
    // Flips the non-conform edges of m_stack until it is empty.
    void flushConform();
    // Returns the edge between the vertices at (x0, y0) and (x1, y1), or an empty edge when either point is no vertex.
    Edge* locateEdge(Edge* apResult, int32_t x0, int32_t y0, int32_t x1, int32_t y1) const;
    void createEdgeDataTable();
    // Sets the data of edge to data, creating its entry and its edge data item when it has none.
    void appendOrReplaceEdgeData(const Edge& edge, const EDGE_DATA& data);
    // Puts the data of edge and of its twin edge back from data.
    void setEdgeDataValues(Edge edge, const EdgeDataValue* const data);
    // Empties and frees the edge data table.
    void releaseEdgeData();
    // Removes the data of edge, destroying the table once it holds no edge.
    void removeEdgeData(const Edge& edge);
    // Takes the data of edge and of its twin edge out of the table into data, clearing both edges.
    void extractEdgeDataValues(Edge edge, EdgeDataValue* const data);
    // The location of the point (x, y), walked from the root of its grid cell (m_roots) or the first triangle.
    Location* locateVertex(Location* apResult, int32_t x, int32_t y) const;
    // Returns root, or the edge locateVertex finds from it for v's coordinates when root does not start at v.
    Edge* locateExistingVertex(Edge* apResult, Edge root, const Vertex* v) const;
    // The edge from v0 to v1, turning round v0 from locateExistingVertex's edge.
    Edge* locateExistingEdge(Edge* apResult, Edge root, const Vertex* v0, const Vertex* v1) const;
    // insertEdge returns EDGE_IS_VERTEX when both ends are the same point.
    Insertion insertEdge(int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool conform, const EDGE_DATA* data);
    // Switches on the location's type and returns FAILED for an unknown one.
    Insertion insertVertex(int32_t x, int32_t y, bool conform, const Location& location, bool edgeSplit);

    // hkReferencedObject (vtable and the reference count) fills 0x0-0x8.
    hkgpAbstractMesh<Edge, Vertex, Triangle, ALLOCATOR> m_mesh;
    // The mesh's traversal stack, the edge data table and its pools, and the triangle grid's roots.
    hkInplaceArray<Edge, 64, ALLOCATOR> m_stack;
    hkGeometryProcessing::HashTable<EdgeDataBox, ALLOCATOR>* m_edgeData;
    hkGeometryProcessing::PoolAllocator<EDGE_DATA, 32, ALLOCATOR> m_edgeDataAllocator;
    Triangle* m_roots[256];
    int32_t m_margin;
    hkMatrix4 m_localToWorld;
    hkMatrix4 m_worldToLocal;
};

using hkgpTriangulatorTypeDefault = hkgpTriangulatorType<
    hkgpTriangulatorBase::VertexBase,
    hkgpTriangulatorBase::TriangleBase,
    hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>,
    hkContainerHeapAllocator,
    -1>;

// The default triangulator's triangle pools; allocateNewPool allocates 1160 bytes for one.
using hkgpTriangulatorTypeDefaultTrianglePoolAllocator = hkGeometryProcessing::
    PoolAllocator<hkgpTriangulatorTypeDefault::Triangle, 32, hkContainerHeapAllocator>;

// The triangulator's mesh type.
using hkgpDefaultTriangulatorType = hkgpTriangulatorType<
    hkgpTriangulatorBase::VertexBase,
    hkgpTriangulatorBase::TriangleBase,
    hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>,
    hkContainerHeapAllocator,
    -1>;

// hkgpMesh's triangulator's setDomainFromPoints, defined in hkgpMesh.cpp.
template <>
void hkgpDefaultTriangulatorType::setDomainFromPoints(
    const hkStridedVertices& points,
    const hkMatrix4& worldToXYPlane,
    bool dmax,
    bool keepRatio
);
// hkgpMesh's triangulator's setDomainFromPlanarPoints, defined in hkgpMesh.cpp.
template <>
void hkgpDefaultTriangulatorType::setDomainFromPlanarPoints(
    const hkStridedVertices& points,
    const hkVector4& planeNormal,
    bool margin,
    bool keepRatio
);
// Declared because setDomainFromPoints calls reset(bool).
template <>
void hkgpDefaultTriangulatorType::reset(bool createDomainTriangulation);
// Defined in hkgpMesh.cpp; the generic insertEdge and insertVertex call it.
template <>
void hkgpDefaultTriangulatorType::flushConform();
// The triangulator's edge data table.
using hkgpDefaultTriangulatorEdgeDataTable = hkGeometryProcessing::
    HashTable<hkgpDefaultTriangulatorType::EdgeDataBox, hkContainerHeapAllocator>;

// Clears the triangulation and its edge data (reset(true) frees the edge data table); the members free the edge data
// pools, the stack and the mesh.
template <typename VERTEX_BASE, typename TRIANGLE_BASE, typename EDGE_DATA, typename ALLOCATOR, int32_t N>
hkgpTriangulatorType<VERTEX_BASE, TRIANGLE_BASE, EDGE_DATA, ALLOCATOR, N>::~hkgpTriangulatorType() {
    reset(true);
}

// The triangulator's abstract mesh.
using hkgpTriangulatorAbstractMesh = hkgpAbstractMesh<
    hkgpDefaultTriangulatorType::Edge,
    hkgpDefaultTriangulatorType::Vertex,
    hkgpDefaultTriangulatorType::Triangle,
    hkContainerHeapAllocator>;

#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpTriangulatorType.inl"
