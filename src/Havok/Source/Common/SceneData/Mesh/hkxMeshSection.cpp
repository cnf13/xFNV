#include "Havok/Source/Common/SceneData/Mesh/hkxMeshSection.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialClass.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxIndexBuffer.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexBuffer.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexDescription.h"

static void cleanupLoadedObjecthkxMeshSection(void* p);

// The reflected members of hkxMeshSection.
using hkxMeshSectionClass_MembersArray = const hkClassMember[4];
static hkxMeshSectionClass_MembersArray hkxMeshSectionClass_Members{{"vertexBuffer", &hkxVertexBufferClass, nullptr,
																		hkClassMember::TYPE_POINTER,
																		hkClassMember::TYPE_STRUCT, 0, 0, 0x8, nullptr},
	{"indexBuffers", &hkxIndexBufferClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER, 0, 0, 0xC,
		nullptr},
	{"material", &hkxMaterialClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, 0x18,
		nullptr},
	{"userChannels", &hkReferencedObjectClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER, 0, 0,
		0x1C, nullptr}};
hkClass hkxMeshSectionClass("hkxMeshSection", &hkReferencedObjectClass, 0x28, nullptr, 0, nullptr, 0,
	hkxMeshSectionClass_Members, 4, nullptr, nullptr, 0, 1);
hkTypeInfo hkxMeshSectionTypeInfo = {"hkxMeshSection", finishLoadedObjecthkxMeshSection,
	cleanupLoadedObjecthkxMeshSection, getVtablehkxMeshSection(), &typeid(hkxMeshSection), sizeof(hkxMeshSection)};

static void cleanupLoadedObjecthkxMeshSection(void* p) {
	static_cast<hkxMeshSection*>(p)->~hkxMeshSection();
}

// A reference count of 1 and empty members (the base's and members' constructors), then everything from other.
hkxMeshSection::hkxMeshSection(const hkxMeshSection& other) : hkReferencedObject() {
	operator=(other);
}

// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
hkxMeshSection::hkxMeshSection(hkFinishLoadedObjectFlag aFlag)
	: hkReferencedObject(aFlag), m_vertexBuffer(aFlag), m_indexBuffers(aFlag), m_material(aFlag),
	  m_userChannels(aFlag) {}

// The members' destructors release the user channels, the material, the index buffers and the vertex buffer and
// free both arrays.
hkxMeshSection::~hkxMeshSection() {}

// Appends the positions, then a triangle per list triple or strip index, with materialIndex; a strip's later
// triangles take the last appended triangle's c and b. Another index type is warned about and skipped.
void hkxMeshSection::appendGeometry(hkGeometry& geometryInOut, int32_t materialIndex) {
	collectVertexPositions(geometryInOut.m_vertices);
	for (int32_t i = 0; i < m_indexBuffers.m_size; ++i) {
		hkxIndexBuffer* pBuffer = m_indexBuffers.m_data[i].val();
		int32_t numIndices = pBuffer->m_indices16.m_size | pBuffer->m_indices32.m_size;
		bool bSmallIndices = pBuffer->m_indices16.m_size > 0;
		int32_t j = 0;
		while (j < numIndices) {
			int32_t a;
			int32_t b;
			int32_t c;
			if (pBuffer->m_indexType.m_storage == hkxIndexBuffer::INDEX_TYPE_TRI_LIST) {
				if (bSmallIndices) {
					a = pBuffer->m_indices16.m_data[j];
					b = pBuffer->m_indices16.m_data[j + 1];
					c = pBuffer->m_indices16.m_data[j + 2];
				} else {
					a = pBuffer->m_indices32.m_data[j];
					b = pBuffer->m_indices32.m_data[j + 1];
					c = pBuffer->m_indices32.m_data[j + 2];
				}
				j += 3;
			} else if (pBuffer->m_indexType.m_storage == hkxIndexBuffer::INDEX_TYPE_TRI_STRIP) {
				if (j < 2) {
					++j;
					continue;
				}
				if (j == 2) {
					if (bSmallIndices) {
						a = pBuffer->m_indices16.m_data[0];
						b = pBuffer->m_indices16.m_data[1];
						c = pBuffer->m_indices16.m_data[2];
					} else {
						a = pBuffer->m_indices32.m_data[0];
						b = pBuffer->m_indices32.m_data[1];
						c = pBuffer->m_indices32.m_data[2];
					}
					j = 3;
				} else {
					const hkGeometry::Triangle& last =
						geometryInOut.m_triangles.m_data[geometryInOut.m_triangles.m_size - 1];
					a = last.m_c;
					b = last.m_b;
					if (bSmallIndices) {
						c = pBuffer->m_indices16.m_data[j];
					} else {
						c = pBuffer->m_indices32.m_data[j];
					}
					++j;
				}
			} else {
				char acBuffer[512];
				hkErrStream kStream(acBuffer, 512);
				kStream << "Unsupported index buffer type - Ignoring";
				hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAA883), acBuffer,
					".\\Mesh\\hkxMeshSection.cpp", 141);
				break;
			}
			// EXE-INLINED none: hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>::expandOne
			hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>& triangles = geometryInOut.m_triangles;
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (triangles.m_size == (triangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &triangles, sizeof(hkGeometry::Triangle));
			}
			hkGeometry::Triangle& triangle = triangles.m_data[triangles.m_size];
			triangle.m_a = a;
			triangle.m_b = b;
			triangle.m_c = c;
			triangle.m_material = materialIndex;
			++triangles.m_size;
		}
	}
}

// Appends the vertex buffer's positions (its first position element; the vertex data is untyped bytes, and a vector is
// read from them at each stride).
void hkxMeshSection::collectVertexPositions(hkArray<hkVector4, hkContainerHeapAllocator>& verticesInOut) const {
	hkxVertexBuffer* pVertexBuffer = m_vertexBuffer.val();
	if (!pVertexBuffer) {
		return;
	}
	hkxVertexDescription::ElementDecl* pDecl =
		pVertexBuffer->m_desc.getElementDecl(hkxVertexDescription::HKX_DU_POSITION, 0);
	int32_t numVerts = static_cast<int32_t>(pVertexBuffer->m_data.m_numVerts);
	if (!pDecl || numVerts <= 0) {
		return;
	}
	const char* pSrc = static_cast<const char*>(pVertexBuffer->getVertexDataPtr(*pDecl));
	uint32_t stride = pDecl->m_byteStride;
	int32_t first = verticesInOut.m_size;
	int32_t newSize = first + numVerts;
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::expandBy
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t capacity = verticesInOut.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
	if (capacity < newSize) {
		int32_t newCapacity = 2 * capacity;
		if (newSize >= newCapacity) {
			newCapacity = newSize;
		}
		hkArrayUtil::_reserve(*pHeap, &verticesInOut, newCapacity, sizeof(hkVector4));
	}
	verticesInOut.m_size = newSize;
	for (int32_t i = 0; i < numVerts; ++i) {
		verticesInOut.m_data[first + i].m_quad = reinterpret_cast<const hkVector4*>(pSrc)->m_quad;
		pSrc += stride;
	}
}

uint32_t hkxMeshSection::getNumTriangles() {
	int32_t v4;
	uint32_t v2 = 0;
	for (int32_t i = 0; i < m_indexBuffers.m_size; v2 += v4) {
		v4 = m_indexBuffers.m_data[i++].val()->getNumTriangles();
	}
	return v2;
}

void hkxMeshSection::getTriangleIndices(uint32_t triIndex, uint32_t& indexAOut, uint32_t& indexBOut,
	uint32_t& indexCOut) {
	int32_t v6 = 0;
	uint32_t v7 = 0;
	if (m_indexBuffers.m_size > 0) {
		hkxIndexBuffer* m_pntr;
		while (1) {
			m_pntr = m_indexBuffers.m_data[v6].val();
			uint32_t v9 = v7 + m_pntr->getNumTriangles();
			if (triIndex < v9) {
				break;
			}
			++v6;
			v7 = v9;
			if (v6 >= m_indexBuffers.m_size) {
				return;
			}
		}
		m_pntr->getTriangleIndicesPtr(triIndex - v7, &indexAOut, &indexBOut, &indexCOut);
	}
}

// Copies the base's two members, then the references and arrays from other: hkRefPtr's and hkRefVariant's assignments
// and hkArray's copy (reserve, release the excess, assign the common part, copy-construct the rest).
hkxMeshSection& hkxMeshSection::operator=(const hkxMeshSection& other) {
	m_memSizeAndFlags = other.m_memSizeAndFlags;
	m_referenceCount = other.m_referenceCount;
	// EXE-INLINED none: hkRefPtr<hkxVertexBuffer>::operator=
	if (other.m_vertexBuffer.m_pntr) {
		other.m_vertexBuffer.m_pntr->addReference();
	}
	if (m_vertexBuffer.m_pntr) {
		m_vertexBuffer.m_pntr->removeReference();
	}
	m_vertexBuffer.m_pntr = other.m_vertexBuffer.m_pntr;
	// EXE-INLINED none: hkArray<hkRefPtr<hkxIndexBuffer>, hkContainerHeapAllocator>::operator=
	{
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		int32_t oldSize = m_indexBuffers.m_size;
		int32_t newSize = other.m_indexBuffers.m_size;
		int32_t copiedSize = newSize > oldSize ? oldSize : newSize;
		int32_t capacity = m_indexBuffers.m_capacityAndFlags & hkArrayBase<hkRefPtr<hkxIndexBuffer>>::CAPACITY_MASK;
		if (capacity < newSize) {
			int32_t newCapacity = 2 * capacity;
			if (newSize >= newCapacity) {
				newCapacity = newSize;
			}
			hkArrayUtil::_reserve(*pHeap, &m_indexBuffers, newCapacity, sizeof(hkRefPtr<hkxIndexBuffer>));
		}
		hkRefPtr<hkxIndexBuffer>* pExcess = m_indexBuffers.m_data + newSize;
		for (int32_t i = 0; i < oldSize - newSize; ++i) {
			if (pExcess[i].m_pntr) {
				pExcess[i].m_pntr->removeReference();
			}
			pExcess[i].m_pntr = nullptr;
		}
		hkRefPtr<hkxIndexBuffer>* pDst = m_indexBuffers.m_data;
		const hkRefPtr<hkxIndexBuffer>* pSrc = other.m_indexBuffers.m_data;
		for (int32_t i = 0; i < copiedSize; ++i) {
			if (pSrc[i].m_pntr) {
				pSrc[i].m_pntr->addReference();
			}
			if (pDst[i].m_pntr) {
				pDst[i].m_pntr->removeReference();
			}
			pDst[i].m_pntr = pSrc[i].m_pntr;
		}
		pDst = m_indexBuffers.m_data + copiedSize;
		pSrc = other.m_indexBuffers.m_data + copiedSize;
		for (int32_t i = 0; i < newSize - copiedSize; ++i) {
			hkRefPtr<hkxIndexBuffer>* pNew = &pDst[i];
			if (pNew) {
				if (pSrc[i].m_pntr) {
					pSrc[i].m_pntr->addReference();
				}
				pNew->m_pntr = pSrc[i].m_pntr;
			}
		}
		m_indexBuffers.m_size = newSize;
	}
	// EXE-INLINED none: hkRefPtr<hkxMaterial>::operator=
	if (other.m_material.m_pntr) {
		other.m_material.m_pntr->addReference();
	}
	if (m_material.m_pntr) {
		m_material.m_pntr->removeReference();
	}
	m_material.m_pntr = other.m_material.m_pntr;
	// EXE-INLINED none: hkArray<hkRefVariant, hkContainerHeapAllocator>::operator=
	{
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		int32_t oldSize = m_userChannels.m_size;
		int32_t newSize = other.m_userChannels.m_size;
		int32_t copiedSize = newSize > oldSize ? oldSize : newSize;
		int32_t capacity = m_userChannels.m_capacityAndFlags & hkArrayBase<hkRefVariant>::CAPACITY_MASK;
		if (capacity < newSize) {
			int32_t newCapacity = 2 * capacity;
			if (newSize >= newCapacity) {
				newCapacity = newSize;
			}
			hkArrayUtil::_reserve(*pHeap, &m_userChannels, newCapacity, sizeof(hkRefVariant));
		}
		hkRefVariant* pExcess = m_userChannels.m_data + newSize;
		for (int32_t i = 0; i < oldSize - newSize; ++i) {
			if (pExcess[i].m_pntr) {
				pExcess[i].m_pntr->removeReference();
			}
			pExcess[i].m_pntr = nullptr;
		}
		hkRefVariant* pDst = m_userChannels.m_data;
		const hkRefVariant* pSrc = other.m_userChannels.m_data;
		// hkRefVariant's assignment skips a self-assignment.
		for (int32_t i = 0; i < copiedSize; ++i) {
			if (&pDst[i] != &pSrc[i]) {
				hkReferencedObject* pObject = pSrc[i].m_pntr;
				if (pObject) {
					pObject->addReference();
				}
				if (pDst[i].m_pntr) {
					pDst[i].m_pntr->removeReference();
				}
				pDst[i].m_pntr = pObject;
			}
		}
		pDst = m_userChannels.m_data + copiedSize;
		pSrc = other.m_userChannels.m_data + copiedSize;
		for (int32_t i = 0; i < newSize - copiedSize; ++i) {
			hkRefVariant* pNew = &pDst[i];
			if (pNew) {
				if (pSrc[i].m_pntr) {
					pSrc[i].m_pntr->addReference();
				}
				pNew->m_pntr = pSrc[i].m_pntr;
			}
		}
		m_userChannels.m_size = newSize;
	}
	return *this;
}
