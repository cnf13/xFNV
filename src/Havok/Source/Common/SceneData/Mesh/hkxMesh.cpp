#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMeshSection.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

static void cleanupLoadedObjecthkxMeshUserChannelInfo(void* p);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkxMesh::UserChannelInfo, as the exe's .rdata holds them.
using hkxMesh_UserChannelInfoClass_MembersArray = const hkClassMember[2];
static hkxMesh_UserChannelInfoClass_MembersArray hkxMesh_UserChannelInfoClass_Members{
	{"name", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x14, nullptr},
	{"className", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x18, nullptr}};
hkClass hkxMeshUserChannelInfoClass("hkxMeshUserChannelInfo", &hkxAttributeHolderClass, 0x1C, nullptr, 0, nullptr, 0,
	hkxMesh_UserChannelInfoClass_Members, 2, nullptr, nullptr, 0, 0);
hkTypeInfo hkxMeshUserChannelInfoTypeInfo = {"hkxMeshUserChannelInfo", finishLoadedObjecthkxMeshUserChannelInfo,
	cleanupLoadedObjecthkxMeshUserChannelInfo, getVtablehkxMeshUserChannelInfo(), &typeid(hkxMesh::UserChannelInfo),
	sizeof(hkxMesh::UserChannelInfo)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of hkxMesh, as
// the exe's .rdata holds them.
using hkxMeshClass_MembersArray = const hkClassMember[2];
static hkxMeshClass_MembersArray hkxMeshClass_Members{{"sections", &hkxMeshSectionClass, nullptr,
														  hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER, 0, 0,
														  0x8, nullptr},
	{"userChannelInfos", &hkxMeshUserChannelInfoClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER,
		0, 0, 0x14, nullptr}};
hkClass hkxMeshClass("hkxMesh", &hkReferencedObjectClass, 0x20, nullptr, 0, nullptr, 0, hkxMeshClass_Members, 2,
	nullptr, nullptr, 0, 1);
hkTypeInfo hkxMeshTypeInfo = {"hkxMesh", finishLoadedObjecthkxMesh, cleanupLoadedObjecthkxMesh, getVtablehkxMesh(),
	&typeid(hkxMesh), sizeof(hkxMesh)};

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
	for (int32_t i = 0; i < m_sections.m_size; ++i) {
		m_sections.m_data[i].val()->collectVertexPositions(verticesInOut);
	}
}

// Each section's geometry, built in a local and appended with its triangle indices offset past the vertices already
// there; with apMaterialsInOut, the section's material is looked up there (added when missing) and its index given to
// the section's triangles; without it, every triangle gets -1.
void hkxMesh::appendGeometry(hkGeometry& geometryInOut,
	hkArray<hkxMaterial*, hkContainerHeapAllocator>* materialsInOut) const {
	int32_t materialIndex = -1;
	for (int32_t i = 0; i < m_sections.m_size; ++i) {
		int32_t indexOffset = geometryInOut.m_vertices.m_size;
		if (materialsInOut) {
			hkxMaterial* pMaterial = m_sections.m_data[i].val()->m_material.val();
			// EXE-INLINED none: hkArray<hkxMaterial*, hkContainerHeapAllocator>::indexOf
			int32_t found = -1;
			for (int32_t j = 0; j < materialsInOut->m_size; ++j) {
				if (materialsInOut->m_data[j] == pMaterial) {
					found = j;
					break;
				}
			}
			materialIndex = found;
			if (found == -1) {
				materialIndex = materialsInOut->m_size;
				// EXE-INLINED none: hkArray<hkxMaterial*, hkContainerHeapAllocator>::pushBack
				hkMemoryRouter& router = hkMemoryRouter::getInstance();
				if (materialsInOut->m_size ==
					(materialsInOut->m_capacityAndFlags & hkArrayBase<hkxMaterial*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*router.m_heap, materialsInOut, sizeof(hkxMaterial*));
				}
				materialsInOut->m_data[materialsInOut->m_size++] = pMaterial;
			}
		}
		FNVD_INLINED_LOCAL(hkGeometry, geometry, std::nullopt, std::nullopt);
		// EXE-INLINED none: hkGeometry::hkGeometry
		geometry.m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		geometry.m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
		geometry.m_vertices.m_data = nullptr;
		geometry.m_vertices.m_size = 0;
		geometry.m_triangles.m_data = nullptr;
		geometry.m_triangles.m_size = 0;
		m_sections.m_data[i].val()->appendGeometry(geometry, materialIndex);
		for (int32_t t = 0; t < geometry.m_triangles.m_size; ++t) {
			geometry.m_triangles.m_data[t].m_a += indexOffset;
			geometry.m_triangles.m_data[t].m_b += indexOffset;
			geometry.m_triangles.m_data[t].m_c += indexOffset;
		}
		int32_t numVertices = geometry.m_vertices.m_size;
		hkVector4* pVertices = geometry.m_vertices.m_data;
		geometryInOut.m_vertices._insertAt(*hkMemoryRouter::getInstance().m_heap, indexOffset, pVertices, numVertices);
		int32_t numTriangles = geometry.m_triangles.m_size;
		int32_t triangleIndex = geometryInOut.m_triangles.m_size;
		hkGeometry::Triangle* pTriangles = geometry.m_triangles.m_data;
		geometryInOut.m_triangles._insertAt(*hkMemoryRouter::getInstance().m_heap, triangleIndex, pTriangles,
			numTriangles);
		geometry.~hkGeometry();
	}
}

const hkClass& hkxMesh::staticClass() {
	return hkxMeshClass;
}
