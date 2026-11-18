// The generic bodies of hkgpTriangulatorType's members. The helpers (Edge::link, next, prev, master, the constraint
// bits, bind, flip, orientation, incircle) are written out: the next index is 9 >> 2i & 3, the previous 18 >> 2i & 3,
// and a link is hkgpTopology's Uid, the neighbour triangle's address with the shared edge's index in its low two bits.
// Included by hkgpAbstractMesh.h.
#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"

#include <cmath>
#include <new>

// The implicit copy assignment: the tag list copied as an hkArray, its storage grown on the heap when other's size
// exceeds the capacity.
template <typename ALLOCATOR>
hkgpTriangulatorBase::DefaultEdgeData<ALLOCATOR>&
hkgpTriangulatorBase::DefaultEdgeData<ALLOCATOR>::operator=(const DefaultEdgeData& other) {
    if ((m_tags.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK) < other.m_tags.m_size) {
        hkArrayUtil::_reserve(
            *hkMemoryRouter::getInstance().m_heap, &m_tags, other.m_tags.m_size, static_cast<int32_t>(sizeof(hkUlong))
        );
    }
    for (int32_t i = 0; i < other.m_tags.m_size; ++i) {
        m_tags.m_data[i] = other.m_tags.m_data[i];
    }
    m_tags.m_size = other.m_tags.m_size;
    return *this;
}

#define XFNV_TRIANGULATOR_HDR                                                                                          \
    template <typename VERTEX_BASE, typename TRIANGLE_BASE, typename EDGE_DATA, typename ALLOCATOR, int32_t N>
#define XFNV_TRIANGULATOR_TYP hkgpTriangulatorType<VERTEX_BASE, TRIANGLE_BASE, EDGE_DATA, ALLOCATOR, N>

// The SDK's setDomain: the margin, then the local-to-world frame (the rectangle dmin..dmax over the quantized extent,
// rotated by angle) and its inverse; the mesh reset to the domain's two triangles.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::setDomain(const hkVector4& dmin, const hkVector4& dmax, float angle, bool margin) {
    const int32_t iMargin = margin ? 1 : 0;
    m_margin = iMargin;
    const float fExt = static_cast<float>(32767.0 - (static_cast<double>(iMargin) + iMargin));
    const float fS = std::sin(angle);
    const float fC = std::cos(angle);
    const float fSclX = static_cast<float>((static_cast<double>(dmax.m_quad[0]) - dmin.m_quad[0]) / fExt);
    const float fSclY = static_cast<float>((static_cast<double>(dmax.m_quad[1]) - dmin.m_quad[1]) / fExt);
    m_localToWorld = hkMatrix4Identity;
    m_localToWorld.m_col0.m_quad[0] = fC * fSclX;
    m_localToWorld.m_col1.m_quad[0] = fS * fSclY;
    m_localToWorld.m_col3.m_quad[0] = dmin.m_quad[0];
    m_localToWorld.m_col0.m_quad[1] = -fS * fSclX;
    m_localToWorld.m_col1.m_quad[1] = fC * fSclY;
    m_localToWorld.m_col3.m_quad[1] = dmin.m_quad[1];
    m_worldToLocal.setInverse(m_localToWorld, 0.0f);
    reset(true);
}

// The SDK's getEdgeData: for a valid edge and an existing table, the data of the edge's box, else null.
XFNV_TRIANGULATOR_HDR
const EDGE_DATA* XFNV_TRIANGULATOR_TYP::getEdgeData(const Edge& edge) const {
    if (!edge.m_triangle || !m_edgeData) {
        return nullptr;
    }
    // EdgeDataBox(edge) and HashTable::find (EdgeDataBox::getHash: the makeHash of the two vertices' packed
    // coordinates, unsigned modulo the bucket count).
    const Vertex* pV0 = edge.m_triangle->m_vertices[edge.m_index];
    const Vertex* pV1 = edge.m_triangle->m_vertices[(9 >> (2 * edge.m_index)) & 3];
    const uint32_t uiHash = ((static_cast<uint32_t>(pV0->m_x) << 15) + static_cast<uint32_t>(pV0->m_y)) * 0x3442A5u ^
        ((static_cast<uint32_t>(pV1->m_x) << 15) + static_cast<uint32_t>(pV1->m_y)) * 0x1958E9u;
    const hkArray<EdgeDataBox, ALLOCATOR>& rBucket =
        m_edgeData->m_buckets.m_data[uiHash % static_cast<uint32_t>(m_edgeData->m_buckets.m_size)];
    for (int32_t i = 0; i < rBucket.m_size; ++i) {
        if (rBucket.m_data[i].m_vertices[0] == pV0 && rBucket.m_data[i].m_vertices[1] == pV1) {
            return rBucket.m_data[i].m_data;
        }
    }
    return nullptr;
}

// The SDK's getEdgeDataOrientation: over the edges of the triangles of region bit 0, whether more edges have data of
// their own than through their twin (an open edge without data counts for the twin); the twin's data is read only
// when the edge has none.
XFNV_TRIANGULATOR_HDR
int32_t XFNV_TRIANGULATOR_TYP::getEdgeDataOrientation() const {
    Edge edge;
    Edge link;
    int32_t iCountIn = 0;
    int32_t iCountOut = 0;
    for (Triangle* pTriangle = m_mesh.m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        if (pTriangle->m_region & 1) {
            edge.m_triangle = pTriangle;
            for (uint32_t i = 0; i < 3; ++i) {
                edge.m_index = i;
                const uintptr_t uiLink = pTriangle->m_links[i];
                if (uiLink & ~uintptr_t(3)) {
                    link.m_index = static_cast<uint32_t>(uiLink & 3);
                    link.m_triangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
                    if (getEdgeData(edge)) {
                        ++iCountIn;
                    }
                    else if (getEdgeData(link)) {
                        ++iCountOut;
                    }
                }
                else if (getEdgeData(edge)) {
                    ++iCountIn;
                }
                else {
                    ++iCountOut;
                }
            }
        }
    }
    return iCountIn > iCountOut;
}

// Equivalent to locateVertex(Edge(getRoot(x, y), 0), x, y). From the root of the point's grid cell, turn
// counter-clockwise until the point is not right of the edge, then step towards it at most once per triangle,
// restarting from each triangle's first edge in turn; a walk that never ends is reported as a cycle.
XFNV_TRIANGULATOR_HDR
auto XFNV_TRIANGULATOR_TYP::locateVertex(Location* apResult, int32_t x, int32_t y) const -> Location* {
    // orientation(const Edge&, int, int): the edge's start-to-end vector crossed with start to (x, y), in wrapping
    // 32-bit arithmetic.
    auto orientation = [x, y](const Triangle* pTriangle, uint32_t uiIndex) -> int32_t {
        const Vertex* pStart = pTriangle->m_vertices[uiIndex];
        const Vertex* pEnd = pTriangle->m_vertices[(9 >> (2 * uiIndex)) & 3];
        return static_cast<int32_t>(
            (static_cast<uint32_t>(pEnd->m_x) - static_cast<uint32_t>(pStart->m_x)) *
                (static_cast<uint32_t>(y) - static_cast<uint32_t>(pStart->m_y)) -
            (static_cast<uint32_t>(pEnd->m_y) - static_cast<uint32_t>(pStart->m_y)) *
                (static_cast<uint32_t>(x) - static_cast<uint32_t>(pStart->m_x))
        );
    };
    // getRoot (rootIndex: the point's cell of the 16 x 16 grid).
    Triangle* pTriangle = m_roots[(y >> 11) * 16 + (x >> 11)];
    if (!pTriangle) {
        pTriangle = m_mesh.m_triangles.m_used;
    }
    uint32_t uiIndex = 0;
    for (Triangle* pBase = m_mesh.m_triangles.m_used; pBase; pBase = pBase->m_next) {
        int32_t iLast;
        while ((iLast = orientation(pTriangle, uiIndex)) < 0) {
            // Edge::turnCcwInPlace: the linked edge's next.
            const uintptr_t uiLink = pTriangle->m_links[uiIndex];
            pTriangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
            uiIndex = (9 >> (2 * static_cast<uint32_t>(uiLink & 3))) & 3;
        }
        for (int32_t n = m_mesh.m_triangles.m_numUsed; n > 0; --n) {
            const uint32_t auiEdges[2] = {(9u >> (2 * uiIndex)) & 3, (0x12u >> (2 * uiIndex)) & 3};
            const int32_t aiDir[2] = {orientation(pTriangle, auiEdges[0]), orientation(pTriangle, auiEdges[1])};
            const int32_t iMoi = aiDir[0] < aiDir[1] ? 0 : 1;
            const int32_t iMov = aiDir[iMoi];
            if (iMov >= 0) {
                const int32_t iType = (iLast == 0 ? 1 : 0) + (aiDir[0] == 0 ? 2 : 0) + (aiDir[1] == 0 ? 4 : 0);
                apResult->m_edge.m_triangle = pTriangle;
                switch (iType) {
                    case 0:
                        apResult->m_type.m_storage = Location::IN_TRIANGLE;
                        apResult->m_edge.m_index = uiIndex;
                        break;
                    case 1:
                        apResult->m_type.m_storage = Location::ON_EDGE;
                        apResult->m_edge.m_index = uiIndex;
                        break;
                    case 2:
                        apResult->m_type.m_storage = Location::ON_EDGE;
                        apResult->m_edge.m_index = auiEdges[0];
                        break;
                    case 4:
                        apResult->m_type.m_storage = Location::ON_EDGE;
                        apResult->m_edge.m_index = auiEdges[1];
                        break;
                    case 3:
                        apResult->m_type.m_storage = Location::ON_VERTEX;
                        apResult->m_edge.m_index = auiEdges[0];
                        break;
                    case 6:
                        apResult->m_type.m_storage = Location::ON_VERTEX;
                        apResult->m_edge.m_index = auiEdges[1];
                        break;
                    case 5:
                        apResult->m_type.m_storage = Location::ON_VERTEX;
                        apResult->m_edge.m_index = uiIndex;
                        break;
                    default:
                        // Edge::null().
                        apResult->m_type.m_storage = Location::NOT_FOUND;
                        apResult->m_edge.m_triangle = nullptr;
                        apResult->m_edge.m_index = 0;
                        break;
                }
                return apResult;
            }
            iLast = -iMov;
            // Edge::link.
            const uintptr_t uiLink = pTriangle->m_links[auiEdges[iMoi]];
            pTriangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
            uiIndex = static_cast<uint32_t>(uiLink & 3);
        }
        pTriangle = pBase;
        uiIndex = 0;
    }
    char buf[512];
    hkErrStream errStream(buf, 512);
    errStream << "Cycle detected during point location";
    if (hkError::s_instance->message(
            hkError::MESSAGE_ERROR, static_cast<int32_t>(0xB8C66B5F), buf,
            "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/GeometryProcessing/Triangulator/"
            "hkgpTriangulator.inl",
            847
        )) {
        __builtin_debugtrap();
    }
    // Edge::null().
    apResult->m_type.m_storage = Location::CYCLE;
    apResult->m_edge.m_triangle = nullptr;
    apResult->m_edge.m_index = 0;
    return apResult;
}

// Returns root when it starts at v, else the edge of the location locateVertex(root, v->m_x, v->m_y) finds (written out
// as in locateVertex above; a cycle or no location gives the null edge).
XFNV_TRIANGULATOR_HDR
auto XFNV_TRIANGULATOR_TYP::locateExistingVertex(Edge* apResult, Edge root, const Vertex* v) const -> Edge* {
    if (root.m_triangle->m_vertices[root.m_index] == v) {
        *apResult = root;
        return apResult;
    }
    const int32_t x = v->m_x;
    const int32_t y = v->m_y;
    auto orientation = [x, y](const Triangle* pTriangle, uint32_t uiIndex) -> int32_t {
        const Vertex* pStart = pTriangle->m_vertices[uiIndex];
        const Vertex* pEnd = pTriangle->m_vertices[(9 >> (2 * uiIndex)) & 3];
        return static_cast<int32_t>(
            (static_cast<uint32_t>(pEnd->m_x) - static_cast<uint32_t>(pStart->m_x)) *
                (static_cast<uint32_t>(y) - static_cast<uint32_t>(pStart->m_y)) -
            (static_cast<uint32_t>(pEnd->m_y) - static_cast<uint32_t>(pStart->m_y)) *
                (static_cast<uint32_t>(x) - static_cast<uint32_t>(pStart->m_x))
        );
    };
    Triangle* pTriangle = root.m_triangle;
    uint32_t uiIndex = root.m_index;
    for (Triangle* pBase = m_mesh.m_triangles.m_used; pBase; pBase = pBase->m_next) {
        int32_t iLast;
        while ((iLast = orientation(pTriangle, uiIndex)) < 0) {
            const uintptr_t uiLink = pTriangle->m_links[uiIndex];
            pTriangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
            uiIndex = (9 >> (2 * static_cast<uint32_t>(uiLink & 3))) & 3;
        }
        for (int32_t n = m_mesh.m_triangles.m_numUsed; n > 0; --n) {
            const uint32_t auiEdges[2] = {(9u >> (2 * uiIndex)) & 3, (0x12u >> (2 * uiIndex)) & 3};
            const int32_t aiDir[2] = {orientation(pTriangle, auiEdges[0]), orientation(pTriangle, auiEdges[1])};
            const int32_t iMoi = aiDir[0] < aiDir[1] ? 0 : 1;
            const int32_t iMov = aiDir[iMoi];
            if (iMov >= 0) {
                const int32_t iType = (iLast == 0 ? 1 : 0) + (aiDir[0] == 0 ? 2 : 0) + (aiDir[1] == 0 ? 4 : 0);
                switch (iType) {
                    case 0:
                    case 1:
                    case 5: break;
                    case 2:
                    case 3: uiIndex = auiEdges[0]; break;
                    case 4:
                    case 6: uiIndex = auiEdges[1]; break;
                    default:
                        // Edge::null().
                        apResult->m_triangle = nullptr;
                        apResult->m_index = 0;
                        return apResult;
                }
                apResult->m_index = uiIndex;
                apResult->m_triangle = pTriangle;
                return apResult;
            }
            iLast = -iMov;
            const uintptr_t uiLink = pTriangle->m_links[auiEdges[iMoi]];
            pTriangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
            uiIndex = static_cast<uint32_t>(uiLink & 3);
        }
        pTriangle = pBase;
        uiIndex = 0;
    }
    char buf[512];
    hkErrStream errStream(buf, 512);
    errStream << "Cycle detected during point location";
    if (hkError::s_instance->message(
            hkError::MESSAGE_ERROR, static_cast<int32_t>(0xB8C66B5F), buf,
            "d:\\build\\20091222_000100_AiAnBeClDePh\\Source\\Common/Internal/GeometryProcessing/Triangulator/"
            "hkgpTriangulator.inl",
            847
        )) {
        __builtin_debugtrap();
    }
    // Edge::null().
    apResult->m_triangle = nullptr;
    apResult->m_index = 0;
    return apResult;
}

// Finds the edge from v0 to v1, starting from locateExistingVertex's edge at v0: on the domain's boundary it is first
// turned counter-clockwise to the naked edge, then turned clockwise until it ends at v1.
XFNV_TRIANGULATOR_HDR
auto XFNV_TRIANGULATOR_TYP::locateExistingEdge(Edge* apResult, Edge root, const Vertex* v0, const Vertex* v1) const
    -> Edge* {
    Edge located;
    locateExistingVertex(&located, root, v0);
    Triangle* pTriangle = located.m_triangle;
    uint32_t uiIndex = located.m_index;
    if (pTriangle->m_vertices[(9 >> (2 * uiIndex)) & 3] != v1) {
        // Vertex::isBoundary: a coordinate on the domain's edge, 0 or the SDK's cfgBoundary 0x7FFF.
        const Vertex* pStart = pTriangle->m_vertices[uiIndex];
        if (pStart->m_x == 0 || pStart->m_x == 0x7FFF || pStart->m_y == 0 || pStart->m_y == 0x7FFF) {
            // Edge::isNaked and Edge::turnCcwInPlace (the linked edge's next).
            while (pTriangle->m_links[uiIndex] & ~uintptr_t(3)) {
                const uintptr_t uiLink = pTriangle->m_links[uiIndex];
                pTriangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
                uiIndex = (9 >> (2 * static_cast<uint32_t>(uiLink & 3))) & 3;
            }
        }
        while (pTriangle->m_vertices[(9 >> (2 * uiIndex)) & 3] != v1) {
            // Edge::turnCwInPlace: the previous edge's link.
            const uintptr_t uiLink = pTriangle->m_links[(0x12 >> (2 * uiIndex)) & 3];
            uiIndex = static_cast<uint32_t>(uiLink & 3);
            pTriangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
        }
    }
    apResult->m_index = uiIndex;
    apResult->m_triangle = pTriangle;
    return apResult;
}

// When both points are vertices, returns the edge between them (locateExistingEdge from the first location's edge),
// else the null edge.
XFNV_TRIANGULATOR_HDR
auto XFNV_TRIANGULATOR_TYP::locateEdge(Edge* apResult, int32_t a1, int32_t a2, int32_t a3, int32_t a4) const -> Edge* {
    Location location0;
    Location location1;
    locateVertex(&location0, a1, a2);
    locateVertex(&location1, a3, a4);
    if (location0.m_type.m_storage == Location::ON_VERTEX && location1.m_type.m_storage == Location::ON_VERTEX) {
        locateExistingEdge(
            apResult, location0.m_edge, location0.m_edge.m_triangle->m_vertices[location0.m_edge.m_index],
            location1.m_edge.m_triangle->m_vertices[location1.m_edge.m_index]
        );
        return apResult;
    }
    // Edge::null().
    apResult->m_triangle = nullptr;
    apResult->m_index = 0;
    return apResult;
}

// The SDK's partition: every triangle's region set to the null region (0xFFF), then the regions flooded from the
// triangle at (0, 0): a region's triangles are reached over unconstrained edges (the current stack), its neighbours
// over constrained ones start the next region (the next stack); m_stack and a local 64-edge inplace stack take turns.
// Returns the number of regions.
XFNV_TRIANGULATOR_HDR
int32_t XFNV_TRIANGULATOR_TYP::partition() {
    using tEdgeStack = hkInplaceArray<Edge, 64, ALLOCATOR>;
    const uint32_t uiNullRegion = (1 << 12) - 1;
    for (Triangle* pTriangle = m_mesh.m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
        pTriangle->m_region = uiNullRegion;
    }
    // hkArray<Edge>::pushBack.
    const auto pushBack = [](tEdgeStack* pStack, Triangle* pTriangle, uint32_t uiIndex) {
        if (pStack->m_size == (pStack->m_capacityAndFlags & hkArrayBase<Edge>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                *hkMemoryRouter::getInstance().m_heap, pStack, static_cast<int32_t>(sizeof(Edge))
            );
        }
        Edge* pSlot = pStack->m_data + pStack->m_size;
        pSlot->m_triangle = pTriangle;
        pSlot->m_index = uiIndex;
        ++pStack->m_size;
    };
    tEdgeStack stack2;
    tEdgeStack* pCurrent = &m_stack;
    tEdgeStack* pNext = &stack2;
    int32_t iRegion = -1;
    Location location;
    const Edge& rStart = locateVertex(&location, 0, 0)->m_edge;
    pushBack(pNext, rStart.m_triangle, rStart.m_index);
    do {
        // hkAlgorithm::swap and hkArray::clear.
        tEdgeStack* pSwap = pNext;
        pNext = pCurrent;
        pCurrent = pSwap;
        pNext->m_size = 0;
        ++iRegion;
        do {
            // hkArray::back and popBack.
            Triangle* pTriangle = pCurrent->m_data[pCurrent->m_size - 1].m_triangle;
            --pCurrent->m_size;
            if (pTriangle->m_region == uiNullRegion) {
                pTriangle->m_region = iRegion;
                for (uint32_t i = 0; i < 3; ++i) {
                    // Edge::link and Edge::isValid.
                    const uintptr_t uiLink = pTriangle->m_links[i];
                    Triangle* pNeighbour = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
                    const uint32_t uiNeighbourIndex = static_cast<uint32_t>(uiLink & 3);
                    if (pNeighbour && pNeighbour->m_region == uiNullRegion) {
                        // Edge::isConstrained: the constraint bit sits on the edge's master, its link when its start
                        // orders after its end and it is not naked.
                        Triangle* pMaster = pNeighbour;
                        uint32_t uiMasterIndex = uiNeighbourIndex;
                        const Vertex* pStartVertex = pNeighbour->m_vertices[uiNeighbourIndex];
                        const Vertex* pEndVertex = pNeighbour->m_vertices[(9 >> (2 * uiNeighbourIndex)) & 3];
                        if (pStartVertex->m_x > pEndVertex->m_x ||
                            (pStartVertex->m_x == pEndVertex->m_x && pStartVertex->m_y > pEndVertex->m_y)) {
                            const uintptr_t uiMasterLink = pNeighbour->m_links[uiNeighbourIndex];
                            if (uiMasterLink & ~uintptr_t(3)) {
                                pMaster = reinterpret_cast<Triangle*>(uiMasterLink & ~uintptr_t(3));
                                uiMasterIndex = static_cast<uint32_t>(uiMasterLink & 3);
                            }
                        }
                        if (static_cast<uint8_t>(pMaster->m_constraints & (1u << uiMasterIndex))) {
                            pushBack(pNext, pNeighbour, uiNeighbourIndex);
                        }
                        else {
                            pushBack(pCurrent, pNeighbour, uiNeighbourIndex);
                        }
                    }
                }
            }
        }
        while (pCurrent->m_size > 0);
    }
    while (pNext->m_size > 0);
    return iRegion + 1;
}

// The SDK's conformEdges: passes over every triangle's edges, flipping each non-conform edge that can flip, until a
// pass flips none. The determinants wrap as 32- and 64-bit integer arithmetic does.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::conformEdges() {
    const auto linkOf = [](const Edge& e) {
        const uintptr_t uid = e.m_triangle->m_links[e.m_index];
        Edge l;
        l.m_triangle = reinterpret_cast<Triangle*>(uid & ~static_cast<uintptr_t>(3));
        l.m_index = static_cast<uint32_t>(uid & 3);
        return l;
    };
    const auto makeEdge = [](Triangle* t, uint32_t i) {
        Edge e;
        e.m_triangle = t;
        e.m_index = i;
        return e;
    };
    const auto nextIndex = [](uint32_t i) { return static_cast<uint32_t>((9 >> (2 * i)) & 3); };
    const auto prevIndex = [](uint32_t i) { return static_cast<uint32_t>((18 >> (2 * i)) & 3); };
    // The triangulator Edge's master: the edge or its link, whichever holds the constraint bit.
    const auto masterOf = [&](const Edge& e) {
        const Vertex* s = e.m_triangle->m_vertices[e.m_index];
        const Vertex* n = e.m_triangle->m_vertices[nextIndex(e.m_index)];
        if (s->m_x > n->m_x || (s->m_x == n->m_x && s->m_y > n->m_y)) {
            const Edge l = linkOf(e);
            if (l.m_triangle) {
                return l;
            }
        }
        return e;
    };
    const auto isConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        return (m.m_triangle->m_constraints & (1u << m.m_index)) != 0;
    };
    const auto setConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        m.m_triangle->m_constraints = m.m_triangle->m_constraints | (1u << m.m_index);
    };
    const auto getAndClearConstraint = [&](const Edge& e) {
        const bool constrained = isConstrained(e);
        if (constrained) {
            const Edge m = masterOf(e);
            m.m_triangle->m_constraints = m.m_triangle->m_constraints & ~(1u << m.m_index);
        }
        return constrained;
    };
    const auto bind = [](const Edge& a, const Edge& b) {
        a.m_triangle->m_links[a.m_index] = reinterpret_cast<uintptr_t>(b.m_triangle) + b.m_index;
        if (b.m_triangle) {
            b.m_triangle->m_links[b.m_index] = reinterpret_cast<uintptr_t>(a.m_triangle) + a.m_index;
        }
    };
    const auto orientation = [](const Vertex* a, const Vertex* b, const Vertex* c) {
        const uint32_t bx = static_cast<uint32_t>(b->m_x) - static_cast<uint32_t>(a->m_x);
        const uint32_t by = static_cast<uint32_t>(b->m_y) - static_cast<uint32_t>(a->m_y);
        const uint32_t cx = static_cast<uint32_t>(c->m_x) - static_cast<uint32_t>(a->m_x);
        const uint32_t cy = static_cast<uint32_t>(c->m_y) - static_cast<uint32_t>(a->m_y);
        return static_cast<int32_t>(cy * bx - cx * by);
    };
    // incircle: the 2x2 determinants and squared lengths in 32 bits, widened to 64 for the sum.
    const auto incircle = [](const Vertex* a, const Vertex* b, const Vertex* c, const Vertex* d) {
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
    };

    bool bFound;
    do {
        bFound = false;
        for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
            for (uint32_t i = 0; i < 3; ++i) {
                const Edge edge = makeEdge(t, i);
                // isConform: naked, constrained, or the link's apex outside.
                if (linkOf(edge).m_triangle == nullptr || isConstrained(edge)) {
                    continue;
                }
                const Edge lk = linkOf(edge);
                if (!incircle(
                        t->m_vertices[prevIndex(i)], t->m_vertices[i], t->m_vertices[nextIndex(i)],
                        lk.m_triangle->m_vertices[prevIndex(lk.m_index)]
                    )) {
                    continue;
                }
                // Edge::canFlip (the constraint tested again).
                if (isConstrained(edge)) {
                    continue;
                }
                const Edge ep = makeEdge(t, prevIndex(i));
                const Edge lp = makeEdge(lk.m_triangle, prevIndex(lk.m_index));
                const Vertex* epStart = ep.m_triangle->m_vertices[ep.m_index];
                const Vertex* lpStart = lp.m_triangle->m_vertices[lp.m_index];
                if (orientation(epStart, t->m_vertices[i], lpStart) <= 0 ||
                    orientation(lpStart, lk.m_triangle->m_vertices[lk.m_index], epStart) <= 0) {
                    continue;
                }
                // Edge::flip, carrying the constraints of the two outer edges over.
                const bool c0 = getAndClearConstraint(ep);
                const Edge clk = linkOf(edge);
                const bool c1 = getAndClearConstraint(makeEdge(clk.m_triangle, prevIndex(clk.m_index)));
                const Edge flk = linkOf(edge);
                const Edge fep = makeEdge(t, prevIndex(i));
                const Edge flp = makeEdge(flk.m_triangle, prevIndex(flk.m_index));
                t->m_vertices[i] = flp.m_triangle->m_vertices[flp.m_index];
                flk.m_triangle->m_vertices[flk.m_index] = fep.m_triangle->m_vertices[fep.m_index];
                bind(flk, linkOf(fep));
                bind(edge, linkOf(flp));
                bind(fep, flp);
                if (c0) {
                    const Edge l = linkOf(fep);
                    setConstrained(makeEdge(l.m_triangle, nextIndex(l.m_index)));
                }
                if (c1) {
                    setConstrained(makeEdge(fep.m_triangle, nextIndex(fep.m_index)));
                }
                bFound = true;
            }
        }
    }
    while (bFound);
}

// The SDK's createEdgeDataHashTable: the table constructed on a heap block with cfgEdgeDataHashSize (1031) empty
// buckets; null when the block is.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::createEdgeDataTable() {
    using tTable = hkGeometryProcessing::HashTable<EdgeDataBox, ALLOCATOR>;
    using tBucket = hkArray<EdgeDataBox, ALLOCATOR>;
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(tTable)));
    tTable* pTable = nullptr;
    if (pMemory) {
        pTable = ::new (pMemory) tTable();
        // HashTable(int) and setBucketsCount: the bucket array reserved and filled with empty buckets.
        hkArrayUtil::_reserve(
            *hkMemoryRouter::getInstance().m_heap, &pTable->m_buckets, hkGeometryProcessing::NEXT_PRIME_AFTER_1024,
            static_cast<int32_t>(sizeof(tBucket))
        );
        for (int32_t i = 0; i < hkGeometryProcessing::NEXT_PRIME_AFTER_1024; ++i) {
            ::new (pTable->m_buckets.m_data + i) tBucket();
        }
        pTable->m_buckets.m_size = hkGeometryProcessing::NEXT_PRIME_AFTER_1024;
    }
    m_edgeData = pTable;
}

// The SDK's setEdgeData: the table created when missing; the edge's box looked up, else inserted with a new edge data
// item from the pools; then each tag of data the item's list lacks appended (DefaultEdgeData::appendOrReplace).
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::appendOrReplaceEdgeData(const Edge& edge, const EDGE_DATA& data) {
    using tBucket = hkArray<EdgeDataBox, ALLOCATOR>;
    using tEdgeDataPools = hkGeometryProcessing::PoolAllocator<EDGE_DATA, 32, ALLOCATOR>;
    if (!m_edgeData) {
        createEdgeDataTable();
    }
    // EdgeDataBox(edge) and HashTable::find, as in getEdgeData.
    Vertex* pV0 = edge.m_triangle->m_vertices[edge.m_index];
    Vertex* pV1 = edge.m_triangle->m_vertices[(9 >> (2 * edge.m_index)) & 3];
    const uint32_t uiHash = ((static_cast<uint32_t>(pV0->m_x) << 15) + static_cast<uint32_t>(pV0->m_y)) * 0x3442A5u ^
        ((static_cast<uint32_t>(pV1->m_x) << 15) + static_cast<uint32_t>(pV1->m_y)) * 0x1958E9u;
    tBucket& rBucket = m_edgeData->m_buckets.m_data[uiHash % static_cast<uint32_t>(m_edgeData->m_buckets.m_size)];
    EdgeDataBox* pBox = nullptr;
    for (int32_t i = 0; i < rBucket.m_size; ++i) {
        if (rBucket.m_data[i].m_vertices[0] == pV0 && rBucket.m_data[i].m_vertices[1] == pV1) {
            pBox = &rBucket.m_data[i];
            break;
        }
    }
    if (!pBox) {
        // HashTable::insert: the box appended to its bucket.
        ++m_edgeData->m_numItems;
        if (rBucket.m_size == (rBucket.m_capacityAndFlags & hkArrayBase<EdgeDataBox>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                *hkMemoryRouter::getInstance().m_heap, &rBucket, static_cast<int32_t>(sizeof(EdgeDataBox))
            );
        }
        pBox = rBucket.m_data + rBucket.m_size;
        ++rBucket.m_size;
        pBox->m_vertices[0] = pV0;
        pBox->m_vertices[1] = pV1;
        // PoolAllocator::allocate: allocateItem, then the edge data's default constructor.
        typename tEdgeDataPools::Pool* pPool;
        if (!m_edgeDataAllocator.m_pools.m_size || !(pPool = m_edgeDataAllocator.m_pools.m_data[0])->m_free) {
            pPool = m_edgeDataAllocator.allocateNewPool();
        }
        typename tEdgeDataPools::Item* pItem = pPool->m_free;
        pPool->m_free = pItem->m_nextFree;
        ++pPool->m_used;
        pBox->m_data = ::new (pItem->m_data) EDGE_DATA();
    }
    // DefaultEdgeData::appendOrReplace: each tag of data not in the list yet appended.
    EDGE_DATA* pTarget = pBox->m_data;
    for (int32_t i = 0; i < data.m_tags.m_size; ++i) {
        int32_t iIndex = -1;
        for (int32_t j = 0; j < pTarget->m_tags.m_size; ++j) {
            if (pTarget->m_tags.m_data[j] == data.m_tags.m_data[i]) {
                iIndex = j;
                break;
            }
        }
        if (iIndex == -1) {
            if (pTarget->m_tags.m_size == (pTarget->m_tags.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(
                    *hkMemoryRouter::getInstance().m_heap, &pTarget->m_tags, static_cast<int32_t>(sizeof(hkUlong))
                );
            }
            pTarget->m_tags.m_data[pTarget->m_tags.m_size] = data.m_tags.m_data[i];
            ++pTarget->m_tags.m_size;
        }
    }
}

// The SDK's restoreBothEdgeData: for the edge and then its twin, the value's data set back where it has any.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::setEdgeDataValues(Edge edge, const EdgeDataValue* const data) {
    for (int32_t i = 0; i < 2; ++i) {
        if (data[i].m_hasData) {
            appendOrReplaceEdgeData(edge, data[i].m_data);
        }
        // Edge::isBound and Edge::link.
        const uintptr_t uiLink = edge.m_triangle->m_links[edge.m_index];
        if ((uiLink & ~uintptr_t(3)) == 0) {
            break;
        }
        edge.m_triangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
        edge.m_index = static_cast<uint32_t>(uiLink & 3);
    }
}

// The SDK's destroyEdgeDataHashTable: the table destroyed and its heap block freed.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::releaseEdgeData() {
    if (m_edgeData) {
        delete m_edgeData;
        m_edgeData = nullptr;
    }
}

// The SDK's clearEdgeData: the edge's box looked up, its data handed back to the edge data pools and the box removed
// (swapped with its bucket's last); the table destroyed once it holds no edge.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::removeEdgeData(const Edge& edge) {
    if (!m_edgeData) {
        return;
    }
    const Vertex* pV0 = edge.m_triangle->m_vertices[edge.m_index];
    const Vertex* pV1 = edge.m_triangle->m_vertices[(9 >> (2 * edge.m_index)) & 3];
    const uint32_t uiHash = ((static_cast<uint32_t>(pV0->m_x) << 15) + static_cast<uint32_t>(pV0->m_y)) * 0x3442A5u ^
        ((static_cast<uint32_t>(pV1->m_x) << 15) + static_cast<uint32_t>(pV1->m_y)) * 0x1958E9u;
    hkArray<EdgeDataBox, ALLOCATOR>& rBucket =
        m_edgeData->m_buckets.m_data[uiHash % static_cast<uint32_t>(m_edgeData->m_buckets.m_size)];
    for (int32_t i = 0; i < rBucket.m_size; ++i) {
        if (rBucket.m_data[i].m_vertices[0] == pV0 && rBucket.m_data[i].m_vertices[1] == pV1) {
            m_edgeDataAllocator.release(rBucket.m_data[i].m_data);
            --m_edgeData->m_numItems;
            const int32_t iLast = --rBucket.m_size;
            if (i != iLast) {
                rBucket.m_data[i] = rBucket.m_data[iLast];
            }
            break;
        }
    }
    if (!m_edgeData->m_numItems) {
        releaseEdgeData();
    }
}

// The SDK's backupBothEdgeData: both values emptied, then for the edge and its twin, the edge's data copied into its
// value and cleared from the table.
XFNV_TRIANGULATOR_HDR
void XFNV_TRIANGULATOR_TYP::extractEdgeDataValues(Edge edge, EdgeDataValue* const data) {
    data[0].m_hasData = false;
    data[1].m_hasData = false;
    for (int32_t i = 0; i < 2; ++i) {
        const EDGE_DATA* pData = getEdgeData(edge);
        if (pData) {
            data[i].m_data = *pData;
            data[i].m_hasData = true;
            removeEdgeData(edge);
        }
        const uintptr_t uiLink = edge.m_triangle->m_links[edge.m_index];
        if ((uiLink & ~uintptr_t(3)) == 0) {
            break;
        }
        edge.m_triangle = reinterpret_cast<Triangle*>(uiLink & ~uintptr_t(3));
        edge.m_index = static_cast<uint32_t>(uiLink & 3);
    }
}

// The SDK's insertEdge(int, int, int, int, bool, const EdgeData*): the origin located as a vertex, the fan around it
// turned towards (x1, y1), then the destination walked to across the triangles, refusing constrained crossings; the
// edge forced in by flips, constrained, given data, conformed and set as its triangle's root.
XFNV_TRIANGULATOR_HDR
auto XFNV_TRIANGULATOR_TYP::insertEdge(
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    bool conform,
    const EDGE_DATA* data
) -> Insertion {
    // The SDK's cfgBoundary: the domain's far coordinate.
    const int32_t iBoundary = 0x7FFF;
    const auto linkOf = [](const Edge& e) {
        const uintptr_t uid = e.m_triangle->m_links[e.m_index];
        Edge l;
        l.m_triangle = reinterpret_cast<Triangle*>(uid & ~static_cast<uintptr_t>(3));
        l.m_index = static_cast<uint32_t>(uid & 3);
        return l;
    };
    const auto makeEdge = [](Triangle* t, uint32_t i) {
        Edge e;
        e.m_triangle = t;
        e.m_index = i;
        return e;
    };
    const auto nextIndex = [](uint32_t i) { return static_cast<uint32_t>((9 >> (2 * i)) & 3); };
    const auto prevIndex = [](uint32_t i) { return static_cast<uint32_t>((18 >> (2 * i)) & 3); };
    const auto startOf = [](const Edge& e) { return e.m_triangle->m_vertices[e.m_index]; };
    const auto endOf = [&](const Edge& e) { return e.m_triangle->m_vertices[nextIndex(e.m_index)]; };
    const auto apexOf = [&](const Edge& e) { return e.m_triangle->m_vertices[prevIndex(e.m_index)]; };
    const auto masterOf = [&](const Edge& e) {
        const Vertex* s = e.m_triangle->m_vertices[e.m_index];
        const Vertex* n = e.m_triangle->m_vertices[nextIndex(e.m_index)];
        if (s->m_x > n->m_x || (s->m_x == n->m_x && s->m_y > n->m_y)) {
            const Edge l = linkOf(e);
            if (l.m_triangle) {
                return l;
            }
        }
        return e;
    };
    const auto isConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        return (m.m_triangle->m_constraints & (1u << m.m_index)) != 0;
    };
    const auto setConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        m.m_triangle->m_constraints = m.m_triangle->m_constraints | (1u << m.m_index);
    };
    const auto getAndClearConstraint = [&](const Edge& e) {
        const bool constrained = isConstrained(e);
        if (constrained) {
            const Edge m = masterOf(e);
            m.m_triangle->m_constraints = m.m_triangle->m_constraints & ~(1u << m.m_index);
        }
        return constrained;
    };
    const auto bind = [](const Edge& a, const Edge& b) {
        a.m_triangle->m_links[a.m_index] = reinterpret_cast<uintptr_t>(b.m_triangle) + b.m_index;
        if (b.m_triangle) {
            b.m_triangle->m_links[b.m_index] = reinterpret_cast<uintptr_t>(a.m_triangle) + a.m_index;
        }
    };
    // orientation (ORIENTATION -1): the 2D cross product of b - a and c - a, in wrapping 32-bit arithmetic.
    const auto orientation = [](int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t cx, int32_t cy) {
        const uint32_t dbx = static_cast<uint32_t>(bx) - static_cast<uint32_t>(ax);
        const uint32_t dby = static_cast<uint32_t>(by) - static_cast<uint32_t>(ay);
        const uint32_t dcx = static_cast<uint32_t>(cx) - static_cast<uint32_t>(ax);
        const uint32_t dcy = static_cast<uint32_t>(cy) - static_cast<uint32_t>(ay);
        return static_cast<int32_t>(dcy * dbx - dcx * dby);
    };
    const auto orientationV = [&](const Vertex* a, const Vertex* b, const Vertex* c) {
        return orientation(a->m_x, a->m_y, b->m_x, b->m_y, c->m_x, c->m_y);
    };
    const auto orientationE = [&](const Edge& e, int32_t x, int32_t y) {
        return orientation(startOf(e)->m_x, startOf(e)->m_y, endOf(e)->m_x, endOf(e)->m_y, x, y);
    };
    // hkArray<Edge>::pushBack on m_stack.
    const auto push = [this](const Edge& e) {
        if (m_stack.m_size == (m_stack.m_capacityAndFlags & hkArrayBase<Edge>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                *hkMemoryRouter::getInstance().m_heap, &m_stack, static_cast<int32_t>(sizeof(Edge))
            );
        }
        m_stack.m_data[m_stack.m_size] = e;
        ++m_stack.m_size;
    };
    // conformEdge: the edge's neighbours pushed, flushConform, the edge found again between its two vertices.
    const auto conformEdge = [&](const Edge& e) {
        Vertex* pV0 = startOf(e);
        Vertex* pV1 = endOf(e);
        push(makeEdge(e.m_triangle, nextIndex(e.m_index)));
        push(makeEdge(e.m_triangle, prevIndex(e.m_index)));
        const Edge l = linkOf(e);
        if (l.m_triangle) {
            push(makeEdge(l.m_triangle, nextIndex(l.m_index)));
            const Edge l2 = linkOf(e);
            push(makeEdge(l2.m_triangle, prevIndex(l2.m_index)));
        }
        flushConform();
        Edge result;
        locateExistingEdge(&result, e, pV0, pV1);
        return result;
    };
    const auto setRoot = [this](Triangle* t) {
        const int32_t cy = (t->m_vertices[0]->m_y + t->m_vertices[1]->m_y + t->m_vertices[2]->m_y) / 3;
        const int32_t cx = (t->m_vertices[0]->m_x + t->m_vertices[1]->m_x + t->m_vertices[2]->m_x) / 3;
        m_roots[(cy >> 11) * 16 + (cx >> 11)] = t;
        t->m_root = 1;
    };
    // Insertion(eType, const Edge&).
    const auto result = [](Insertion::eType eType, const Edge& e) { return Insertion(eType, e); };
    // Insertion(eType, Edge::null()).
    const auto nullResult = [&](Insertion::eType eType) { return result(eType, makeEdge(nullptr, 0)); };

    if (x0 == x1 && y0 == y1) {
        return nullResult(Insertion::EDGE_IS_VERTEX);
    }
    Location location0;
    locateVertex(&location0, x0, y0);
    if (location0.m_type.m_storage != Location::ON_VERTEX) {
        return nullResult(Insertion::CANNOT_LOCATE_ORIGIN);
    }
    Edge edge = location0.m_edge;
    const Vertex* v0 = startOf(edge);
    // Edge::turnCcwInPlace (link().next()).
    while (orientationE(edge, x1, y1) < 0) {
        const Edge l = linkOf(edge);
        edge = makeEdge(l.m_triangle, nextIndex(l.m_index));
    }
    // Edge::turnCwInPlace (prev().link()).
    while (edge.m_triangle && orientationE(makeEdge(edge.m_triangle, prevIndex(edge.m_index)), x1, y1) <= 0) {
        edge = linkOf(makeEdge(edge.m_triangle, prevIndex(edge.m_index)));
    }
    if (!edge.m_triangle) {
        return nullResult(Insertion::INVALID_ORIENTATION);
    }
    Edge focal = makeEdge(edge.m_triangle, nextIndex(edge.m_index));

    // Vertex::isEqual.
    if (endOf(edge)->m_x == x1 && endOf(edge)->m_y == y1) {
        if (isConstrained(edge)) {
            return result(Insertion::ALREADY_PRESENT, edge);
        }
        setConstrained(edge);
        if (data) {
            appendOrReplaceEdgeData(edge, *data);
        }
        if (conform) {
            edge = conformEdge(edge);
        }
        setRoot(edge.m_triangle);
        return result(Insertion::SUCCESS, edge);
    }

    // checkOrientation (cfgIsCW: the ends swapped).
    if (x1 == x0) {
        if (x1 == 0 && y1 < y0) {
            return nullResult(Insertion::INVALID_ORIENTATION);
        }
        if (x1 == iBoundary && y1 > y0) {
            return nullResult(Insertion::INVALID_ORIENTATION);
        }
    }
    if (y1 == y0) {
        if (y1 == 0 && x1 > x0) {
            return nullResult(Insertion::INVALID_ORIENTATION);
        }
        if (y1 == iBoundary && x1 < x0) {
            return nullResult(Insertion::INVALID_ORIENTATION);
        }
    }

    while (orientationE(focal, x1, y1) < 0) {
        if (isConstrained(focal)) {
            if (orientation(startOf(focal)->m_x, startOf(focal)->m_y, x0, y0, x1, y1) == 0) {
                return result(Insertion::CROSSING_CONSTRAINED_VERTEX, focal);
            }
            if (orientation(endOf(focal)->m_x, endOf(focal)->m_y, x0, y0, x1, y1) == 0) {
                return result(
                    Insertion::CROSSING_CONSTRAINED_VERTEX, makeEdge(focal.m_triangle, nextIndex(focal.m_index))
                );
            }

            return result(Insertion::CROSSING_CONSTRAINED_EDGE, focal);
        }
        focal = linkOf(focal);
        const Vertex* pApex = apexOf(focal);
        const int32_t o = orientation(v0->m_x, v0->m_y, pApex->m_x, pApex->m_y, x1, y1);
        if (o < 0) {
            focal = makeEdge(focal.m_triangle, nextIndex(focal.m_index));
        }
        else if (o > 0) {
            focal = makeEdge(focal.m_triangle, prevIndex(focal.m_index));
        }
        else {
            if (!(pApex->m_x == x1 && pApex->m_y == y1)) {
                return result(
                    Insertion::CROSSING_CONSTRAINED_VERTEX, makeEdge(focal.m_triangle, prevIndex(focal.m_index))
                );
            }
            focal = makeEdge(focal.m_triangle, prevIndex(focal.m_index));
        }
    }
    const Vertex* v1 = startOf(focal);
    if (v0 == v1) {
        return nullResult(Insertion::EDGE_IS_VERTEX);
    }
    if (orientationE(focal, x1, y1) != 0) {
        return nullResult(Insertion::CANNOT_LOCATE_DESTINATION);
    }
    if (!(v1->m_x == x1 && v1->m_y == y1)) {
        return result(Insertion::CROSSING_CONSTRAINED_VERTEX, focal);
    }
    // sameBoundary.
    if ((v0->m_x == v1->m_x && (v0->m_x == 0 || v0->m_x == iBoundary)) ||
        (v0->m_y == v1->m_y && (v0->m_y == 0 || v0->m_y == iBoundary))) {
        return result(Insertion::INVALID_ORIENTATION, focal);
    }
    if (orientationE(edge, v1->m_x, v1->m_y) == 0) {
        return result(Insertion::CROSSING_CONSTRAINED_VERTEX, makeEdge(edge.m_triangle, nextIndex(edge.m_index)));
    }
    if (orientationE(edge, v1->m_x, v1->m_y) < 0) {
        return result(Insertion::CROSSING_CONSTRAINED_VERTEX, focal);
    }

    int32_t iSkippedA = 0;
    while (endOf(edge) != v1) {
        const Edge root = edge;
        edge = makeEdge(edge.m_triangle, nextIndex(edge.m_index));
        while (orientationV(v0, v1, endOf(edge)) > 0) {
            const Edge l = linkOf(edge);
            edge = makeEdge(l.m_triangle, nextIndex(l.m_index));
        }
        if (orientationV(startOf(root), startOf(edge), endOf(edge)) > 0) {
            const Vertex* q = endOf(edge);
            int32_t iSkippedB = 0;
            edge = makeEdge(root.m_triangle, nextIndex(root.m_index));
            while (endOf(edge) != q) {
                // Edge::canFlip.
                bool bCanFlip = false;
                if (linkOf(edge).m_triangle && !isConstrained(edge)) {
                    const Edge lk = linkOf(edge);
                    const Edge ep = makeEdge(edge.m_triangle, prevIndex(edge.m_index));
                    const Edge lp = makeEdge(lk.m_triangle, prevIndex(lk.m_index));
                    bCanFlip = orientationV(startOf(ep), startOf(edge), startOf(lp)) > 0 &&
                        orientationV(startOf(lp), startOf(lk), startOf(ep)) > 0;
                }
                if (bCanFlip) {
                    // Edge::flip, carrying the constraints of the two outer edges over.
                    const bool c0 = getAndClearConstraint(makeEdge(edge.m_triangle, prevIndex(edge.m_index)));
                    const Edge clk = linkOf(edge);
                    const bool c1 = getAndClearConstraint(makeEdge(clk.m_triangle, prevIndex(clk.m_index)));
                    Triangle* t = edge.m_triangle;
                    const uint32_t i = edge.m_index;
                    const Edge flk = linkOf(edge);
                    const Edge fep = makeEdge(t, prevIndex(i));
                    const Edge flp = makeEdge(flk.m_triangle, prevIndex(flk.m_index));
                    t->m_vertices[i] = flp.m_triangle->m_vertices[flp.m_index];
                    flk.m_triangle->m_vertices[flk.m_index] = fep.m_triangle->m_vertices[fep.m_index];
                    bind(flk, linkOf(fep));
                    bind(edge, linkOf(flp));
                    bind(fep, flp);
                    const Edge flipped = fep;
                    if (c0) {
                        const Edge l = linkOf(flipped);
                        setConstrained(makeEdge(l.m_triangle, nextIndex(l.m_index)));
                    }
                    if (c1) {
                        setConstrained(makeEdge(flipped.m_triangle, nextIndex(flipped.m_index)));
                    }
                    const Edge fl = linkOf(flipped);
                    if (iSkippedB) {
                        // Edge::nextCcw().link().
                        edge = linkOf(makeEdge(fl.m_triangle, nextIndex(fl.m_index)));
                        --iSkippedB;
                    }
                    else {
                        // Edge::nextCw.
                        edge = makeEdge(fl.m_triangle, prevIndex(fl.m_index));
                    }
                }
                else {
                    ++iSkippedB;
                    const Edge l = linkOf(edge);
                    edge = makeEdge(l.m_triangle, nextIndex(l.m_index));
                }
            }
            // Edge::prevCcw (next().link()).
            edge = linkOf(makeEdge(edge.m_triangle, nextIndex(edge.m_index)));
            if (iSkippedA) {
                while (orientationV(v0, v1, apexOf(edge)) > 0) {
                    edge = linkOf(makeEdge(edge.m_triangle, prevIndex(edge.m_index)));
                }
                edge = makeEdge(edge.m_triangle, prevIndex(edge.m_index));
                --iSkippedA;
            }
        }
        else {
            ++iSkippedA;
        }
    }

    setConstrained(edge);
    if (data) {
        appendOrReplaceEdgeData(edge, *data);
    }
    if (conform) {
        edge = conformEdge(edge);
    }
    setRoot(edge.m_triangle);
    return result(Insertion::SUCCESS, edge);
}

// The SDK's insertVertex(int, int, bool, const Location&, bool), by the location's kind: IN_TRIANGLE splits the
// triangle into three around the new vertex; ON_EDGE (refused on a constrained edge unless edgeSplit) splits the
// edge's triangle and its twin into two each, edgeSplit carrying the edge's constraint and data over to both halves;
// ON_VERTEX marks the vertex there constrained; any other kind is FAILED with Edge::null().
XFNV_TRIANGULATOR_HDR
auto XFNV_TRIANGULATOR_TYP::insertVertex(int32_t x, int32_t y, bool conform, const Location& location, bool edgeSplit)
    -> Insertion {
    const auto linkOf = [](const Edge& e) {
        const uintptr_t uid = e.m_triangle->m_links[e.m_index];
        Edge l;
        l.m_triangle = reinterpret_cast<Triangle*>(uid & ~static_cast<uintptr_t>(3));
        l.m_index = static_cast<uint32_t>(uid & 3);
        return l;
    };
    const auto makeEdge = [](Triangle* t, uint32_t i) {
        Edge e;
        e.m_triangle = t;
        e.m_index = i;
        return e;
    };
    const auto nextIndex = [](uint32_t i) { return static_cast<uint32_t>((9 >> (2 * i)) & 3); };
    const auto prevIndex = [](uint32_t i) { return static_cast<uint32_t>((18 >> (2 * i)) & 3); };
    const auto nextOf = [&](const Edge& e) { return makeEdge(e.m_triangle, nextIndex(e.m_index)); };
    const auto prevOf = [&](const Edge& e) { return makeEdge(e.m_triangle, prevIndex(e.m_index)); };
    const auto startOf = [](const Edge& e) { return e.m_triangle->m_vertices[e.m_index]; };
    const auto endOf = [&](const Edge& e) { return e.m_triangle->m_vertices[nextIndex(e.m_index)]; };
    const auto apexOf = [&](const Edge& e) { return e.m_triangle->m_vertices[prevIndex(e.m_index)]; };
    const auto masterOf = [&](const Edge& e) {
        const Vertex* s = e.m_triangle->m_vertices[e.m_index];
        const Vertex* n = e.m_triangle->m_vertices[nextIndex(e.m_index)];
        if (s->m_x > n->m_x || (s->m_x == n->m_x && s->m_y > n->m_y)) {
            const Edge l = linkOf(e);
            if (l.m_triangle) {
                return l;
            }
        }
        return e;
    };
    const auto isConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        return (m.m_triangle->m_constraints & (1u << m.m_index)) != 0;
    };
    const auto setConstrained = [&](const Edge& e) {
        const Edge m = masterOf(e);
        m.m_triangle->m_constraints = m.m_triangle->m_constraints | (1u << m.m_index);
    };
    const auto getAndClearConstraint = [&](const Edge& e) {
        const bool constrained = isConstrained(e);
        if (constrained) {
            const Edge m = masterOf(e);
            m.m_triangle->m_constraints = m.m_triangle->m_constraints & ~(1u << m.m_index);
        }
        return constrained;
    };
    const auto bind = [](const Edge& a, const Edge& b) {
        a.m_triangle->m_links[a.m_index] = reinterpret_cast<uintptr_t>(b.m_triangle) + b.m_index;
        if (b.m_triangle) {
            b.m_triangle->m_links[b.m_index] = reinterpret_cast<uintptr_t>(a.m_triangle) + a.m_index;
        }
    };
    const auto push = [this](const Edge& e) {
        if (m_stack.m_size == (m_stack.m_capacityAndFlags & hkArrayBase<Edge>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                *hkMemoryRouter::getInstance().m_heap, &m_stack, static_cast<int32_t>(sizeof(Edge))
            );
        }
        m_stack.m_data[m_stack.m_size] = e;
        ++m_stack.m_size;
    };
    const auto setRoot = [this](Triangle* t) {
        const int32_t cy = (t->m_vertices[0]->m_y + t->m_vertices[1]->m_y + t->m_vertices[2]->m_y) / 3;
        const int32_t cx = (t->m_vertices[0]->m_x + t->m_vertices[1]->m_x + t->m_vertices[2]->m_x) / 3;
        m_roots[(cy >> 11) * 16 + (cx >> 11)] = t;
        t->m_root = 1;
    };
    // The items the SDK's allocate() copies; every field of the new item is set after the copy but the list links,
    // which allocate sets itself.
    const Vertex vertexItem = Vertex();
    const Triangle triangleItem = Triangle();
    // newVertex: constrained, its data all ones.
    const auto newVertex = [&] {
        Vertex* k = m_mesh.m_vertices.allocate(vertexItem);
        k->m_x = x;
        k->m_y = y;
        k->m_constraint = 1;
        k->m_data = 0x7FFFFFFF;
        return k;
    };
    const auto newTriangle = [&](Vertex* a, Vertex* b, Vertex* c) {
        Triangle* t = m_mesh.m_triangles.allocate(triangleItem);
        t->m_vertices[0] = a;
        t->m_vertices[1] = b;
        t->m_vertices[2] = c;
        t->m_constraints = 0;
        t->m_root = 0;
        t->m_region = 0;
        t->m_sets = 0;
        setRoot(t);
        return t;
    };
    const auto copyDataset = [](const Triangle* from, Triangle* to) {
        to->m_region = from->m_region;
        to->m_sets = from->m_sets;
    };
    // Insertion(eType, const Edge&).
    const auto result = [](Insertion::eType eType, const Edge& e) { return Insertion(eType, e); };

    Edge edge = location.m_edge;
    switch (location.m_type.m_storage) {
        case Location::IN_TRIANGLE: {
            Vertex* k = newVertex();
            const bool c1 = getAndClearConstraint(nextOf(edge));
            const bool c2 = getAndClearConstraint(prevOf(edge));
            Triangle* t0 = newTriangle(endOf(edge), apexOf(edge), k);
            Triangle* t1 = newTriangle(apexOf(edge), startOf(edge), k);
            edge.m_triangle->m_vertices[prevIndex(edge.m_index)] = k;
            copyDataset(edge.m_triangle, t0);
            copyDataset(edge.m_triangle, t1);
            bind(makeEdge(t0, 0), linkOf(nextOf(edge)));
            bind(makeEdge(t1, 0), linkOf(prevOf(edge)));
            bind(makeEdge(t0, 1), makeEdge(t1, 2));
            bind(makeEdge(t0, 2), nextOf(edge));
            bind(makeEdge(t1, 1), prevOf(edge));
            if (c1) {
                setConstrained(makeEdge(t0, 0));
            }
            if (c2) {
                setConstrained(makeEdge(t1, 0));
            }
            Edge e = makeEdge(t0, 2);
            const Vertex* v = startOf(e);
            if (conform) {
                push(makeEdge(t0, 0));
                push(makeEdge(t1, 0));
                push(edge);
                flushConform();
                Edge located;
                locateExistingVertex(&located, e, v);
                e = located;
            }
            return result(Insertion::SUCCESS, e);
        }
        case Location::ON_EDGE: {
            if (!edgeSplit && isConstrained(edge)) {
                return result(Insertion::ON_CONSTRAINED_EDGE, edge);
            }
            Vertex* k = newVertex();
            const bool ce = getAndClearConstraint(edge);
            EdgeDataValue edv[2];
            if (ce) {
                extractEdgeDataValues(edge, edv);
            }
            const Edge link = linkOf(edge);
            if (link.m_triangle) {
                const bool c0 = getAndClearConstraint(prevOf(edge));
                const bool c1 = getAndClearConstraint(prevOf(link));
                Triangle* t0 = newTriangle(apexOf(edge), startOf(edge), k);
                Triangle* t1 = newTriangle(apexOf(link), startOf(link), k);
                copyDataset(edge.m_triangle, t0);
                copyDataset(link.m_triangle, t1);
                link.m_triangle->m_vertices[link.m_index] = k;
                edge.m_triangle->m_vertices[edge.m_index] = k;
                bind(makeEdge(t0, 0), linkOf(prevOf(edge)));
                bind(makeEdge(t1, 0), linkOf(prevOf(link)));
                bind(makeEdge(t0, 1), link);
                bind(makeEdge(t1, 1), edge);
                bind(makeEdge(t0, 2), prevOf(edge));
                bind(makeEdge(t1, 2), prevOf(link));
                if (c0) {
                    setConstrained(makeEdge(t0, 0));
                }
                if (c1) {
                    setConstrained(makeEdge(t1, 0));
                }
                if (edgeSplit && ce) {
                    setConstrained(makeEdge(t0, 1));
                    setConstrained(makeEdge(t1, 1));
                    setEdgeDataValues(makeEdge(t0, 1), edv);
                    setEdgeDataValues(linkOf(makeEdge(t1, 1)), edv);
                }
                if (conform) {
                    push(nextOf(edge));
                    push(nextOf(link));
                    push(makeEdge(t0, 0));
                    push(makeEdge(t1, 0));
                }
            }
            else {
                const bool c0 = getAndClearConstraint(prevOf(edge));
                Triangle* t0 = newTriangle(apexOf(edge), startOf(edge), k);
                copyDataset(edge.m_triangle, t0);
                edge.m_triangle->m_vertices[edge.m_index] = k;
                bind(makeEdge(t0, 0), linkOf(prevOf(edge)));
                bind(makeEdge(t0, 1), makeEdge(nullptr, 0));
                bind(makeEdge(t0, 2), prevOf(edge));
                if (c0) {
                    setConstrained(makeEdge(t0, 0));
                }
                if (edgeSplit && ce) {
                    setConstrained(makeEdge(t0, 1));
                    setConstrained(nextOf(linkOf(makeEdge(t0, 2))));
                    setEdgeDataValues(makeEdge(t0, 1), edv);
                    setEdgeDataValues(nextOf(linkOf(makeEdge(t0, 2))), edv);
                }
                if (conform) {
                    push(nextOf(edge));
                    push(makeEdge(t0, 0));
                }
            }
            if (conform) {
                const Vertex* v = startOf(edge);
                flushConform();
                Edge located;
                locateExistingVertex(&located, edge, v);
                edge = located;
            }
            return result(Insertion::SUCCESS, edge);
        }
        case Location::ON_VERTEX: {
            Vertex* s = startOf(edge);
            if (!s->m_constraint) {
                s->m_constraint = 1;
                return result(Insertion::SUCCESS, edge);
            }
            return result(Insertion::ALREADY_PRESENT, edge);
        }
        default: break;
    }
    // Edge::null().
    return result(Insertion::FAILED, makeEdge(nullptr, 0));
}

#undef XFNV_TRIANGULATOR_TYP
#undef XFNV_TRIANGULATOR_HDR
