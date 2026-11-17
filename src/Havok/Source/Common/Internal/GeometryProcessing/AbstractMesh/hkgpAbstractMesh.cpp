#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

// The SDK's reset(createDomainTriangulation): the mesh reset through its vtable, the roots cleared, the edge data
// table destroyed (destroyEdgeDataHashTable) and the edge data pools freed; for createDomainTriangulation, the domain's
// four corners and its two triangles (newVertex, newTriangle with setRoot), bound along their diagonal.
// KEEP: partcopy: the exe hands one frame slot nothing wrote ([esp+0x14], 0x24 bytes) to the vertex and triangle
// lists' allocate (0xE247C0, 0xE24850) as the item to copy, and their copies carry its bytes into the new items on
// the heap. no_builtin keeps clang from turning the exe's inline clearing of the roots (rep stosd) into a call of the
// DLL's memset.
template <>
__attribute__((no_builtin("memset"))) void hkgpDefaultTriangulatorType::reset(bool createDomainTriangulation) {
    hkgpTriangulatorAbstractMesh* pMesh = &m_mesh;
    pMesh->reset();
    for (int32_t i = 0; i < 256; ++i) {
        m_roots[i] = nullptr;
    }
    // EXE-INLINED 0xE27400
    if (m_edgeData) {
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        m_edgeData->~hkgpDefaultTriangulatorEdgeDataTable();
        pHeap->blockFree(m_edgeData, static_cast<int32_t>(sizeof(hkgpDefaultTriangulatorEdgeDataTable)));
        m_edgeData = nullptr;
    }
    // EXE-INLINED none: hkGeometryProcessing::PoolAllocator::clear
    for (int32_t i = 0; i < m_edgeDataAllocator.m_pools.m_size; ++i) {
        hkMemoryRouter::getInstance().m_heap->blockFree(
            m_edgeDataAllocator.m_pools.m_data[i],
            static_cast<int32_t>(sizeof(
                hkGeometryProcessing::PoolAllocator<
                    hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>, 32, hkContainerHeapAllocator>::Pool
            ))
        );
    }
    m_edgeDataAllocator.m_pools.m_size = 0;
    if (createDomainTriangulation) {
        // The items allocate() copies: the same uninitialised item is passed to every allocate, as the game does.
        Vertex vertexItem;
        Triangle triangleItem;
        // EXE-INLINED none: newVertex (the domain's corners; the SDK's cfgBoundary is 0x7FFF)
        Vertex* pC00 = m_mesh.m_vertices.allocate(vertexItem);
        pC00->m_x = 0;
        pC00->m_y = 0;
        pC00->m_constraint = 0;
        pC00->m_data = 0x7FFFFFFF;
        Vertex* pC01 = m_mesh.m_vertices.allocate(vertexItem);
        pC01->m_x = 0;
        pC01->m_y = 0x7FFF;
        pC01->m_constraint = 0;
        pC01->m_data = 0x7FFFFFFF;
        Vertex* pC10 = m_mesh.m_vertices.allocate(vertexItem);
        pC10->m_x = 0x7FFF;
        pC10->m_y = 0;
        pC10->m_constraint = 0;
        pC10->m_data = 0x7FFFFFFF;
        Vertex* pC11 = m_mesh.m_vertices.allocate(vertexItem);
        pC11->m_x = 0x7FFF;
        pC11->m_y = 0x7FFF;
        pC11->m_constraint = 0;
        pC11->m_data = 0x7FFFFFFF;
        // EXE-INLINED none: newTriangle and setRoot (the clockwise domain: t0 = c00 c10 c11, t1 = c00 c11 c01)
        Triangle* pT0 = m_mesh.m_triangles.allocate(triangleItem);
        pT0->m_constraints = 0;
        pT0->m_root = 0;
        pT0->m_region = 0;
        pT0->m_vertices[1] = pC10;
        pT0->m_vertices[0] = pC00;
        pT0->m_vertices[2] = pC11;
        pT0->m_sets = 0;
        int32_t iCy = (pT0->m_vertices[0]->m_y + pT0->m_vertices[1]->m_y + pT0->m_vertices[2]->m_y) / 3;
        int32_t iCx = (pT0->m_vertices[0]->m_x + pT0->m_vertices[1]->m_x + pT0->m_vertices[2]->m_x) / 3;
        m_roots[(iCy >> 11) * 16 + (iCx >> 11)] = pT0;
        pT0->m_root = 1;
        Triangle* pT1 = m_mesh.m_triangles.allocate(triangleItem);
        pT1->m_vertices[2] = pC01;
        pT1->m_sets = 0;
        pT1->m_vertices[0] = pC00;
        pT1->m_vertices[1] = pC11;
        pT1->m_constraints = 0;
        pT1->m_root = 0;
        pT1->m_region = 0;
        iCy = (pT1->m_vertices[0]->m_y + pT1->m_vertices[1]->m_y + pT1->m_vertices[2]->m_y) / 3;
        iCx = (pT1->m_vertices[0]->m_x + pT1->m_vertices[1]->m_x + pT1->m_vertices[2]->m_x) / 3;
        m_roots[(iCy >> 11) * 16 + (iCx >> 11)] = pT1;
        pT1->m_root = 1;
        pT0->m_links[1] = 0;
        pT0->m_links[0] = 0;
        pT1->m_links[2] = 0;
        pT1->m_links[1] = 0;
        // EXE-INLINED none: Edge::bind (Edge(t0, 2) with Edge(t1, 0)): each link is hkgpTopology's Uid, the
        // neighbour's address with the shared edge's index in its low two bits.
        pT0->m_links[2] = reinterpret_cast<uintptr_t>(pT1);
        pT1->m_links[0] = reinterpret_cast<uintptr_t>(pT0) + 2;
    }
}
using hkgpTriangulatorType_lhkgpTriangulatorBase_nVertexBase_chkgpTriangulatorBase_nTriangleBase_chkgpTriangulatorBase_nDefaultEdgeData_lhkContainerHeapAllocator_g_chkContainerHeapAllocator_c_x2D1 =
    hkgpTriangulatorType<
        hkgpTriangulatorBase::VertexBase,
        hkgpTriangulatorBase::TriangleBase,
        hkgpTriangulatorBase::DefaultEdgeData<hkContainerHeapAllocator>,
        hkContainerHeapAllocator,
        -1>;
// The edge array pointers' _append: the SDK's inline body (hkArray.h).
using hkArrayBase_lhkArray_lhkgpMeshBase_nEdge_chkContainerHeapAllocator_g_p =
    hkArrayBase<hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>*>;
// The list links (m_next, m_prev) are not copied: the list sets them.
hkgpMeshBase::Vertex::Vertex(const hkgpMeshBase::Vertex& a1)
  : m_position(a1.m_position), m_normal(a1.m_normal), m_data(a1.m_data), m_refs(a1.m_refs), m_tag(a1.m_tag) {}
