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
#include "xfnv/Compiler.h"

// The SDK's reset(createDomainTriangulation): the mesh reset through its vtable, the roots cleared, the edge data
// table destroyed (destroyEdgeDataHashTable) and the edge data pools freed; for createDomainTriangulation, the domain's
// four corners and its two triangles (newVertex, newTriangle with setRoot), bound along their diagonal.
// KEEP: partcopy: vertexItem and triangleItem are never written and are handed to the vertex and triangle lists'
// allocate as the item to copy, so the copies carry their bytes into the new items on the heap. XFNV_NO_BUILTIN keeps
// clang from turning the inline clearing of the roots into a call of the DLL's memset.
template <>
XFNV_NO_BUILTIN("memset")
void hkgpDefaultTriangulatorType::reset(bool createDomainTriangulation) {
    hkgpTriangulatorAbstractMesh* pMesh = &m_mesh;
    pMesh->reset();
    for (Triangle*& pRoot : m_roots) {
        pRoot = nullptr;
    }
    releaseEdgeData();
    m_edgeDataAllocator.clear();
    if (createDomainTriangulation) {
        // The items allocate() copies: the same uninitialised item is passed to every allocate.
        Vertex vertexItem;
        Triangle triangleItem;
        // The domain's corners.
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
        // The domain's two triangles, t0 = c00 c10 c11 and t1 = c00 c11 c01, each the root of its centroid's cell.
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
        Edge{{pT0, 2}}.bind(Edge{{pT1, 0}});
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
using hkArrayBase_lhkArray_lhkgpMeshBase_nEdge_chkContainerHeapAllocator_g_p = hkArrayBase<
    hkArray<hkgpMeshBase::Edge, hkContainerHeapAllocator>*>;
