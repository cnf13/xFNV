#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMeshSection.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

static void cleanupLoadedObjecthkxMeshUserChannelInfo(void* p);

// The reflected members of hkxMesh::UserChannelInfo.
using hkxMesh_UserChannelInfoClass_MembersArray = const hkClassMember[2];
static hkxMesh_UserChannelInfoClass_MembersArray hkxMesh_UserChannelInfoClass_Members{
    {"name", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x14, nullptr},
    {"className", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x18, nullptr}
};
hkClass hkxMeshUserChannelInfoClass(
    "hkxMeshUserChannelInfo",
    &hkxAttributeHolderClass,
    0x1C,
    nullptr,
    0,
    nullptr,
    0,
    hkxMesh_UserChannelInfoClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkxMeshUserChannelInfoTypeInfo = {"hkxMeshUserChannelInfo",
                                             finishLoadedObjecthkxMeshUserChannelInfo,
                                             cleanupLoadedObjecthkxMeshUserChannelInfo,
                                             getVtablehkxMeshUserChannelInfo(),
                                             &typeid(hkxMesh::UserChannelInfo),
                                             sizeof(hkxMesh::UserChannelInfo)};

using hkxMeshClass_MembersArray = const hkClassMember[2];
static hkxMeshClass_MembersArray hkxMeshClass_Members{
    {"sections", &hkxMeshSectionClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER, 0, 0, 0x8,
     nullptr},
    {"userChannelInfos", &hkxMeshUserChannelInfoClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER,
     0, 0, 0x14, nullptr}
};
hkClass hkxMeshClass(
    "hkxMesh",
    &hkReferencedObjectClass,
    0x20,
    nullptr,
    0,
    nullptr,
    0,
    hkxMeshClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxMeshTypeInfo = {"hkxMesh",          finishLoadedObjecthkxMesh, cleanupLoadedObjecthkxMesh,
                              getVtablehkxMesh(), &typeid(hkxMesh),          sizeof(hkxMesh)};

static void cleanupLoadedObjecthkxMeshUserChannelInfo(void* p) {
    static_cast<hkxMesh::UserChannelInfo*>(p)->~UserChannelInfo();
}

// The members' destructors release the strings; the base's destroys the attribute groups.
hkxMesh::UserChannelInfo::~UserChannelInfo() {}

const hkClass& hkxMesh::UserChannelInfo::staticClass() {
    return hkxMeshUserChannelInfoClass;
}

// The loaded arrays are kept as they are.
hkxMesh::hkxMesh(hkFinishLoadedObjectFlag aFlag)
  : hkReferencedObject(aFlag), m_sections(aFlag), m_userChannelInfos(aFlag) {}

// The members' destructors release the user channel infos and the sections and free both arrays.
hkxMesh::~hkxMesh() {}

void hkxMesh::collectVertexPositions(hkArray<hkVector4, hkContainerHeapAllocator>& verticesInOut) const {
    for (const hkRefPtr<hkxMeshSection>& section : m_sections) {
        section.val()->collectVertexPositions(verticesInOut);
    }
}

// Each section's geometry, built in a local and appended with its triangle indices offset past the vertices already
// there; with apMaterialsInOut, the section's material is looked up there (added when missing) and its index given to
// the section's triangles; without it, every triangle gets -1.
void hkxMesh::appendGeometry(
    hkGeometry& geometryInOut,
    hkArray<hkxMaterial*, hkContainerHeapAllocator>* materialsInOut
) const {
    int32_t materialIndex = -1;
    for (const hkRefPtr<hkxMeshSection>& section : m_sections) {
        int32_t indexOffset = geometryInOut.m_vertices.m_size;
        if (materialsInOut) {
            hkxMaterial* pMaterial = section.val()->m_material.val();
            materialIndex = materialsInOut->indexOf(pMaterial);
            if (materialIndex == -1) {
                materialIndex = materialsInOut->m_size;
                materialsInOut->pushBack(pMaterial);
            }
        }
        hkGeometry geometry;
        section.val()->appendGeometry(geometry, materialIndex);
        for (hkGeometry::Triangle& triangle : geometry.m_triangles) {
            triangle.m_a += indexOffset;
            triangle.m_b += indexOffset;
            triangle.m_c += indexOffset;
        }
        int32_t numVertices = geometry.m_vertices.m_size;
        hkVector4* pVertices = geometry.m_vertices.m_data;
        geometryInOut.m_vertices._insertAt(*hkMemoryRouter::getInstance().m_heap, indexOffset, pVertices, numVertices);
        int32_t numTriangles = geometry.m_triangles.m_size;
        int32_t triangleIndex = geometryInOut.m_triangles.m_size;
        hkGeometry::Triangle* pTriangles = geometry.m_triangles.m_data;
        geometryInOut.m_triangles._insertAt(
            *hkMemoryRouter::getInstance().m_heap, triangleIndex, pTriangles, numTriangles
        );
    }
}

const hkClass& hkxMesh::staticClass() {
    return hkxMeshClass;
}
