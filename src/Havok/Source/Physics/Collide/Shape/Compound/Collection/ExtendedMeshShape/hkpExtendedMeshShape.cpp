#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"

#include <bit>
#include <xmmintrin.h>

// Not named in the PDB or IDA; name chosen here. Copies 6 quads (96 bytes) from src to dst
// with dword moves, as the SDK's hkString::memCpy16<96> does on Win32; cdecl, ret 0; its caller
// hkpExtendedMeshShape::getChildShape (0xD55CE0) copies a TrianglesSubpart with it and ignores eax.
static void copyTrianglesSubpart(void* dst, const void* src);

// The SDK's hkString::memCpy16<96> on Win32 (memCpy16NonEmpty: a quad at a time, as dwords), out of line.
static void copyTrianglesSubpart(void* dst, const void* src) {
	const uint32_t* src32 = static_cast<const uint32_t*>(src);
	uint32_t* dst32 = static_cast<uint32_t*>(dst);
	int32_t numQuads = 6;
	do {
		dst32[0] = src32[0];
		dst32[1] = src32[1];
		dst32[2] = src32[2];
		dst32[3] = src32[3];
		dst32 += 4;
		src32 += 4;
	} while (--numQuads > 0);
}

// Member by member, as the exe copies it (the padding after m_numVertices and after m_flipAlternateTriangles is not
// copied).
hkpExtendedMeshShape::TrianglesSubpart&
hkpExtendedMeshShape::TrianglesSubpart::operator=(const hkpExtendedMeshShape::TrianglesSubpart& other) {
	static_cast<hkpExtendedMeshShape::Subpart&>(*this) = static_cast<const hkpExtendedMeshShape::Subpart&>(other);
	m_numTriangleShapes = other.m_numTriangleShapes;
	m_vertexBase = other.m_vertexBase;
	m_vertexStriding = other.m_vertexStriding;
	m_numVertices = other.m_numVertices;
	m_extrusion.m_quad = other.m_extrusion.m_quad;
	m_indexBase = other.m_indexBase;
	m_indexStriding = other.m_indexStriding;
	m_stridingType = other.m_stridingType;
	m_flipAlternateTriangles = other.m_flipAlternateTriangles;
	m_triangleOffset = other.m_triangleOffset;
	m_userData = other.m_userData;
	return *this;
}

// When finishing, the transform flags in m_translation's w are recomputed: FLAG_TRANSLATE_SET for a translation
// beyond 0.001 on any axis, FLAG_ROTATE_SET for a rotation of 0.001 or more.
hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart(hkFinishLoadedObjectFlag flag) : m_childShapes(flag) {
	if (flag.m_finishing) {
		int32_t iFlags = !(__builtin_fabsf(m_translation.m_quad[0]) <= 0.001f &&
							 __builtin_fabsf(m_translation.m_quad[1]) <= 0.001f &&
							 __builtin_fabsf(m_translation.m_quad[2]) <= 0.001f)
			? FLAG_TRANSLATE_SET
			: FLAG_NONE;
		if (!(m_rotation.getAngle() < 0.001f)) {
			iFlags |= FLAG_ROTATE_SET;
		}
		m_translation.setInt24W(iFlags);
	}
}

// The SDK's constructor from the children and an offset: no rotation; FLAG_TRANSLATE_SET when the offset is beyond
// 0.001 on any axis. Each child is referenced.
hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart(const hkpConvexShape* const* a1, int32_t a2, const hkVector4& a3)
	: m_translation(a3) {
	m_type.m_storage = SUBPART_SHAPE;
	m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
	m_materialIndexStriding = 0;
	m_materialBase = nullptr;
	m_materialIndexBase = nullptr;
	m_materialClass = nullptr;
	m_materialStriding = 0;
	m_numMaterials = 1;
	m_childShapes.reserve(a2);
	m_childShapes.setSizeUnchecked(a2);
	for (int32_t i = 0; i < a2; ++i) {
		hkpConvexShape* pChild = const_cast<hkpConvexShape*>(a1[i]);
		if (pChild) {
			pChild->addReference();
		}
		m_childShapes.m_data[i] = pChild;
	}
	m_rotation.setIdentity();
	m_translation.setInt24W(!(__builtin_fabsf(a3.m_quad[0]) <= 0.001f && __builtin_fabsf(a3.m_quad[1]) <= 0.001f &&
								__builtin_fabsf(a3.m_quad[2]) <= 0.001f)
			? FLAG_TRANSLATE_SET
			: FLAG_NONE);
}

// The SDK's constructor from the children and a transform: FLAG_TRANSLATE_SET when the translation is beyond 0.001
// on any axis, FLAG_ROTATE_SET when the rotation is not the identity within 0.001. Each child is referenced.
hkpExtendedMeshShape::ShapesSubpart::ShapesSubpart(const hkpConvexShape* const* a1, int32_t a2, const hkTransform& r)
	: m_translation(r.m_translation) {
	m_materialStriding = 0;
	m_type.m_storage = SUBPART_SHAPE;
	m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
	m_materialIndexStriding = 0;
	m_numMaterials = 1;
	m_materialBase = nullptr;
	m_materialIndexBase = nullptr;
	m_materialClass = nullptr;
	m_rotation.set(r.m_rotation);
	m_childShapes.reserve(a2);
	m_childShapes.setSizeUnchecked(a2);
	for (int32_t i = 0; i < a2; ++i) {
		hkpConvexShape* pChild = const_cast<hkpConvexShape*>(a1[i]);
		if (pChild) {
			pChild->addReference();
		}
		m_childShapes.m_data[i] = pChild;
	}
	int32_t iFlags =
		!(__builtin_fabsf(m_translation.m_quad[0]) <= 0.001f && __builtin_fabsf(m_translation.m_quad[1]) <= 0.001f &&
			__builtin_fabsf(m_translation.m_quad[2]) <= 0.001f)
		? FLAG_TRANSLATE_SET
		: FLAG_NONE;
	if (!r.m_rotation.isApproximatelyEqual(hkTransformIdentity.m_rotation, 0.001f).m_bool) {
		iFlags |= FLAG_ROTATE_SET;
	}
	m_translation.setInt24W(iFlags);
}

// Each child's reference goes back and its slot is cleared; the array frees its own storage.
hkpExtendedMeshShape::ShapesSubpart::~ShapesSubpart() {
	for (int32_t i = 0; i < m_childShapes.m_size; ++i) {
		if (m_childShapes.m_data[i]) {
			m_childShapes.m_data[i]->removeReference();
		}
		m_childShapes.m_data[i] = nullptr;
	}
}

// The SDK's operator=: the Subpart, then m_childShapes as hkArray<hkRefPtr<hkpConvexShape>>::operator= (inlined: the
// surplus shapes released and cleared, the shared ones assigned, the rest copy-constructed in place), then the
// transform.
hkpExtendedMeshShape::ShapesSubpart&
hkpExtendedMeshShape::ShapesSubpart::operator=(const hkpExtendedMeshShape::ShapesSubpart& other) {
	static_cast<hkpExtendedMeshShape::Subpart&>(*this) = static_cast<const hkpExtendedMeshShape::Subpart&>(other);
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t oldSize = m_childShapes.m_size;
	int32_t newSize = other.m_childShapes.m_size;
	int32_t numAssigned = newSize > oldSize ? oldSize : newSize;
	int32_t capacity = m_childShapes.m_capacityAndFlags & hkArrayBase<hkpConvexShape*>::CAPACITY_MASK;
	if (capacity < newSize) {
		int32_t numReserved = 2 * capacity;
		if (newSize >= numReserved) {
			numReserved = newSize;
		}
		hkArrayUtil::_reserve(*pHeap, &m_childShapes, numReserved, sizeof(hkpConvexShape*));
	}
	// EXE-INLINED none: hkRefPtr<hkpConvexShape>::~hkRefPtr
	hkpConvexShape** pSurplus = m_childShapes.m_data + newSize;
	for (int32_t i = 0; i < oldSize - newSize; ++i) {
		if (pSurplus[i]) {
			pSurplus[i]->removeReference();
		}
		pSurplus[i] = nullptr;
	}
	// EXE-INLINED none: hkRefPtr<hkpConvexShape>::operator=
	hkpConvexShape** pDst = m_childShapes.m_data;
	hkpConvexShape* const* pSrc = other.m_childShapes.m_data;
	for (int32_t i = 0; i < numAssigned; ++i) {
		if (pSrc[i]) {
			pSrc[i]->addReference();
		}
		if (pDst[i]) {
			pDst[i]->removeReference();
		}
		pDst[i] = pSrc[i];
	}
	// EXE-INLINED none: hkRefPtr<hkpConvexShape>'s copy constructor, placed with its null test
	hkpConvexShape* const* pSrcRest = other.m_childShapes.m_data + numAssigned;
	hkpConvexShape** pDstRest = m_childShapes.m_data + numAssigned;
	for (int32_t i = 0; i < newSize - numAssigned; ++i) {
		hkpConvexShape** pSlot = &pDstRest[i];
		if (pSlot) {
			if (pSrcRest[i]) {
				pSrcRest[i]->addReference();
			}
			*pSlot = pSrcRest[i];
		}
	}
	m_childShapes.m_size = newSize;
	m_rotation = other.m_rotation;
	m_translation = other.m_translation;
	return *this;
}

// The loaded subparts stay as loaded. When finishing: every subpart without a material index striding type gets
// MATERIAL_INDICES_INT8, a single triangles subpart moves into m_embeddedTrianglesSubpart (the array points at it as
// user storage), and a cached child count of -1 becomes INT32_MIN (still to be counted).
hkpExtendedMeshShape::hkpExtendedMeshShape(hkFinishLoadedObjectFlag flag)
	: hkpShapeCollection(flag), m_trianglesSubparts(flag), m_shapesSubparts(flag), m_weldingInfo(flag) {
	if (!flag.m_finishing) {
		m_type.m_storage = HK_SHAPE_EXTENDED_MESH;
		return;
	}
	m_collectionType.m_storage = COLLECTION_EXTENDED_MESH;
	for (int32_t i = 0; i < m_trianglesSubparts.m_size; ++i) {
		if (m_trianglesSubparts.m_data[i].m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INVALID) {
			m_trianglesSubparts.m_data[i].m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
		}
	}
	for (int32_t i = 0; i < m_shapesSubparts.m_size; ++i) {
		if (m_shapesSubparts.m_data[i].m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INVALID) {
			m_shapesSubparts.m_data[i].m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
		}
	}
	if (m_trianglesSubparts.m_size == 1) {
		m_embeddedTrianglesSubpart = m_trianglesSubparts.m_data[0];
		m_trianglesSubparts.clearAndDeallocate();
		m_trianglesSubparts.m_data = &m_embeddedTrianglesSubpart;
		m_trianglesSubparts.m_size = 1;
		m_trianglesSubparts.m_capacityAndFlags = 1 | hkArrayBase<TrianglesSubpart>::DONT_DEALLOCATE_FLAG;
	}
	m_type.m_storage = HK_SHAPE_EXTENDED_MESH;
	if (m_cachedNumChildShapes == -1) {
		m_cachedNumChildShapes = INT32_MIN;
	}
}

// An empty shape: unit scaling, no welding, an empty box (half extents rdata_104E148, the exe's -HK_REAL_MAX).
hkpExtendedMeshShape::hkpExtendedMeshShape(float a1, int32_t a2)
	: hkpShapeCollection(HK_SHAPE_EXTENDED_MESH, COLLECTION_EXTENDED_MESH), m_numBitsForSubpartIndex(a2),
	  m_defaultCollisionFilterInfo(0), m_cachedNumChildShapes(0), m_triangleRadius(a1) {
	m_embeddedTrianglesSubpart.m_type.m_storage = SUBPART_TRIANGLES;
	m_embeddedTrianglesSubpart.m_materialBase = nullptr;
	m_embeddedTrianglesSubpart.m_materialIndexBase = nullptr;
	m_embeddedTrianglesSubpart.m_materialClass = nullptr;
	m_embeddedTrianglesSubpart.m_flipAlternateTriangles = 0;
	m_embeddedTrianglesSubpart.m_userData = 0;
	m_embeddedTrianglesSubpart.m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
	m_embeddedTrianglesSubpart.m_materialIndexStriding = 0;
	m_embeddedTrianglesSubpart.m_numMaterials = 1;
	m_embeddedTrianglesSubpart.m_materialStriding = 0;
	m_embeddedTrianglesSubpart.m_extrusion.m_quad = _mm_setzero_ps();
	m_scaling.setAll(1.0f);
	m_weldingType.m_storage = hkpWeldingUtility::WELDING_TYPE_NONE;
	m_aabbHalfExtents.setAll(rdata_104E148);
	m_aabbCenter.m_quad = _mm_setzero_ps();
}

// The mesh shape's welding, scaling and radius are taken over and each of its subparts becomes a triangles subpart
// (a single one in m_embeddedTrianglesSubpart); then the box and the child count are computed.
hkpExtendedMeshShape::hkpExtendedMeshShape(const hkpMeshShape* apMeshShape)
	: hkpShapeCollection(HK_SHAPE_EXTENDED_MESH, COLLECTION_EXTENDED_MESH), m_scaling(apMeshShape->m_scaling),
	  m_numBitsForSubpartIndex(apMeshShape->m_numBitsForSubpartIndex), m_defaultCollisionFilterInfo(0),
	  m_triangleRadius(apMeshShape->m_radius) {
	m_embeddedTrianglesSubpart.m_type.m_storage = SUBPART_TRIANGLES;
	m_embeddedTrianglesSubpart.m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
	m_embeddedTrianglesSubpart.m_materialIndexStriding = 0;
	m_embeddedTrianglesSubpart.m_materialBase = nullptr;
	m_embeddedTrianglesSubpart.m_materialIndexBase = nullptr;
	m_embeddedTrianglesSubpart.m_materialClass = nullptr;
	m_embeddedTrianglesSubpart.m_flipAlternateTriangles = 0;
	m_embeddedTrianglesSubpart.m_userData = 0;
	m_embeddedTrianglesSubpart.m_materialStriding = 0;
	m_embeddedTrianglesSubpart.m_extrusion.m_quad = _mm_setzero_ps();
	m_embeddedTrianglesSubpart.m_numMaterials = 1;
	m_weldingType.m_storage = apMeshShape->m_weldingType.m_storage;

	const int32_t iNumWelding = apMeshShape->m_weldingInfo.m_size;
	m_weldingInfo.reserveExactly(iNumWelding);
	m_weldingInfo.setSizeUnchecked(iNumWelding);
	for (int32_t i = 0; i < iNumWelding; ++i) {
		m_weldingInfo.m_data[i] = apMeshShape->m_weldingInfo.m_data[i];
	}

	const int32_t iNumSubparts = apMeshShape->m_subparts.m_size;
	if (iNumSubparts == 1) {
		m_trianglesSubparts.m_data = &m_embeddedTrianglesSubpart;
		m_trianglesSubparts.m_size = 1;
		m_trianglesSubparts.m_capacityAndFlags = 1 | hkArrayBase<TrianglesSubpart>::DONT_DEALLOCATE_FLAG;
	} else {
		m_trianglesSubparts.reserve(iNumSubparts);
		m_trianglesSubparts.setSizeUnchecked(iNumSubparts);
		for (int32_t i = 0; i < iNumSubparts; ++i) {
			TrianglesSubpart& rPart = m_trianglesSubparts.m_data[i];
			rPart.m_type.m_storage = SUBPART_TRIANGLES;
			rPart.m_materialIndexStridingType.m_storage = MATERIAL_INDICES_INT8;
			rPart.m_materialStriding = 0;
			rPart.m_materialIndexBase = nullptr;
			rPart.m_materialIndexStriding = 0;
			rPart.m_numMaterials = 1;
			rPart.m_materialBase = nullptr;
			rPart.m_materialClass = nullptr;
			rPart.m_extrusion.m_quad = _mm_setzero_ps();
			rPart.m_flipAlternateTriangles = 0;
			rPart.m_userData = 0;
		}
	}
	for (int32_t i = 0; i < iNumSubparts; ++i) {
		TrianglesSubpart& rPart = m_trianglesSubparts.m_data[i];
		const hkpMeshShape::Subpart& rMeshPart = apMeshShape->m_subparts.m_data[i];
		rPart.m_numTriangleShapes = rMeshPart.m_numTriangles;
		rPart.m_materialIndexBase = rMeshPart.m_materialIndexBase;
		rPart.m_materialIndexStriding = rMeshPart.m_materialIndexStriding;
		rPart.m_materialIndexStridingType.m_storage = rMeshPart.m_materialIndexStridingType.m_storage;
		rPart.m_materialBase = rMeshPart.m_materialBase;
		rPart.m_materialStriding = rMeshPart.m_materialStriding;
		rPart.m_numMaterials = rMeshPart.m_numMaterials;
		rPart.m_vertexBase = rMeshPart.m_vertexBase;
		rPart.m_vertexStriding = rMeshPart.m_vertexStriding;
		rPart.m_numVertices = rMeshPart.m_numVertices;
		rPart.m_indexBase = rMeshPart.m_indexBase;
		rPart.m_stridingType.m_storage = rMeshPart.m_stridingType.m_storage;
		rPart.m_indexStriding = rMeshPart.m_indexStriding;
		rPart.m_triangleOffset = rMeshPart.m_triangleOffset;
		rPart.m_extrusion.m_quad = _mm_setzero_ps();
		rPart.m_flipAlternateTriangles = rMeshPart.m_flipAlternateTriangles;
	}
	recalcAabbExtents();
	m_cachedNumChildShapes = hkpShapeContainer::getNumChildShapes();
}

// The three arrays free their own storage, m_shapesSubparts after each subpart's destructor.
hkpExtendedMeshShape::~hkpExtendedMeshShape() {}

int32_t hkpExtendedMeshShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& a1, int32_t a2) const {
	hkpExtendedMeshShape::ShapesSubpart* v4;
	int32_t v5;
	int32_t i;
	m_shapesSubparts.m_size;
	int32_t v11 = 0;
	if (m_shapesSubparts.m_size <= 0) {
		return 240;
	}
	for (i = 0;; ++i) {
		v4 = &m_shapesSubparts.m_data[i];
		v5 = 0;
		if (v4->m_childShapes.m_size > 0) {
			break;
		}
	LABEL_10:;
		if (++v11 >= m_shapesSubparts.m_size) {
			return 240;
		}
	}
	while (1) {
		hkpShape* v6 = v4->m_childShapes.m_data[v5];
		int32_t v7 = v6->calcSizeForSpu(a1, 256);
		int32_t v8 = (std::bit_cast<int32_t>(v4->m_translation.m_quad[3]) & 0xC0FFFFFFu) != 0
			? (std::bit_cast<int32_t>(v4->m_translation.m_quad[3]) & 0xC0FFFFFFu) != 1 ? 416 : 464
			: 512;
		if (v7 < 0 || v7 > v8) {
			return -1;
		}
		if (++v5 >= v4->m_childShapes.m_size) {
			goto LABEL_10;
		}
	}
}

void hkpExtendedMeshShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	hkAabbUtil::calcAabb(localToWorld, m_aabbHalfExtents, m_aabbCenter, tolerance, out);
}

void hkpExtendedMeshShape::initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) {
	m_weldingType.m_storage = static_cast<uint8_t>(weldingType);
	if (weldingType == hkpWeldingUtility::WELDING_TYPE_NONE) {
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t m_capacityAndFlags = m_weldingInfo.m_capacityAndFlags;
		m_weldingInfo.m_size = 0;
		if (m_capacityAndFlags >= 0) {
			m_heap->bufFree(m_weldingInfo.begin(), sizeof(uint16_t) * (m_capacityAndFlags & 0x3FFFFFFF));
		}
		m_weldingInfo.m_data = 0;
		m_weldingInfo.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
	} else {
		int32_t v3 = 0;
		int32_t v4 = 0;
		if (m_trianglesSubparts.m_size > 0) {
			hkpExtendedMeshShape::TrianglesSubpart* m_data = m_trianglesSubparts.begin();
			int32_t v6 = 0;
			do {
				m_data[v6].m_triangleOffset = v3;
				m_data = m_trianglesSubparts.begin();
				v3 += m_data[v6].m_numTriangleShapes;
				++v4;
				++v6;
			} while (v4 < m_trianglesSubparts.m_size);
		}
		hkArray<uint16_t, hkContainerHeapAllocator>* p_m_weldingInfo = &m_weldingInfo;
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		if ((p_m_weldingInfo->m_capacityAndFlags & 0x3FFFFFFF) < v3) {
			hkArrayUtil::_reserve(Instance->m_heap, p_m_weldingInfo, v3, sizeof(uint16_t));
		}
		hkMemoryAllocator* v9 = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t v10 = p_m_weldingInfo->m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK;
		if (v10 < v3) {
			int32_t v11 = 2 * v10;
			if (v3 >= v11) {
				v11 = v3;
			}
			hkArrayUtil::_reserve(v9, p_m_weldingInfo, v11, sizeof(uint16_t));
		}
		p_m_weldingInfo->m_size = v3;
	}
}

void hkpExtendedMeshShape::setWeldingInfo(uint32_t key, int16_t weldingInfo) {
	if (!m_weldingInfo.m_size) {
		int32_t v4 = 0;
		int32_t v5 = 0;
		if (m_trianglesSubparts.m_size > 0) {
			hkpExtendedMeshShape::TrianglesSubpart* m_data = m_trianglesSubparts.begin();
			int32_t v7 = 0;
			do {
				m_data[v7].m_triangleOffset = v5;
				m_data = m_trianglesSubparts.begin();
				v5 += m_data[v7].m_numTriangleShapes;
				++v4;
				++v7;
			} while (v4 < m_trianglesSubparts.m_size);
		}
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		if ((m_weldingInfo.m_capacityAndFlags & 0x3FFFFFFF) < v5) {
			hkArrayUtil::_reserve(Instance->m_heap, &m_weldingInfo, v5, sizeof(uint16_t));
		}
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t v10 = m_weldingInfo.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK;
		if (v10 < v5) {
			int32_t v11 = 2 * v10;
			if (v5 >= v11) {
				v11 = v5;
			}
			hkArrayUtil::_reserve(m_heap, &m_weldingInfo, v11, sizeof(uint16_t));
		}
		m_weldingInfo.m_size = v5;
	}
	if ((key & 0x80000000u) == 0) {
		m_weldingInfo
			.m_data[m_trianglesSubparts.m_data[(key & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex)].m_triangleOffset +
				(key & 0xFFFFFFFFu >> m_numBitsForSubpartIndex)] = weldingInfo;
	}
}

void hkpExtendedMeshShape::addTrianglesSubpart(const hkpExtendedMeshShape::TrianglesSubpart& part) {
	hkVector4 v4;
	hkVector4 v5;
	FNVD_INLINED_LOCAL(hkAabb, arOut, std::nullopt, std::nullopt);
	hkpExtendedMeshShape::TrianglesSubpart* v3 = expandOneTriangleSubparts();
	*v3 = part;
	v4.m_quad = m_aabbCenter.m_quad;
	v5.m_quad = m_aabbHalfExtents.m_quad;
	__m128 v10 = _mm_sub_ps(v4.m_quad, v5.m_quad);
	__m128 v11 = _mm_add_ps(v5.m_quad, v4.m_quad);
	calcAabbExtents(*v3, m_scaling, arOut);
	__m128 v6 = _mm_shuffle_ps(_mm_set_ss(m_triangleRadius), _mm_set_ss(m_triangleRadius), 0);
	__m128 v7 = _mm_min_ps(v10, _mm_sub_ps(arOut.m_min.m_quad, v6));
	__m128 v8 = _mm_max_ps(v11, _mm_add_ps(arOut.m_max.m_quad, v6));
	int32_t v9 = m_trianglesSubparts.m_size - 1;
	m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(v8, v7), hkQuadRealHalf);
	m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(v8, v7), hkQuadRealHalf);
	m_cachedNumChildShapes += _getNumChildShapesInTrianglesSubpart(part, v9);
}

// The SDK's addShapesSubpart: a new subpart is appended and assigned from aSubpart, the shape's AABB grown by the
// subpart's, and the subpart's child shapes added to the cached count. Returns the new subpart's index.
int32_t hkpExtendedMeshShape::addShapesSubpart(const hkpExtendedMeshShape::ShapesSubpart& part) {
	hkpExtendedMeshShape::ShapesSubpart* pPart = expandOneShapesSubparts();
	*pPart = part;
	const hkQuadReal kOldMin = _mm_sub_ps(m_aabbCenter.m_quad, m_aabbHalfExtents.m_quad);
	const hkQuadReal kOldMax = _mm_add_ps(m_aabbHalfExtents.m_quad, m_aabbCenter.m_quad);
	FNVD_INLINED_LOCAL(hkAabb, kPartAabb, std::nullopt, std::nullopt);
	calcAabbExtents(pPart, &kPartAabb);
	const hkQuadReal kMin = _mm_min_ps(kOldMin, kPartAabb.m_min.m_quad);
	const hkQuadReal kMax = _mm_max_ps(kOldMax, kPartAabb.m_max.m_quad);
	m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(kMax, kMin), hkQuadRealHalf);
	m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(kMax, kMin), hkQuadRealHalf);
	m_cachedNumChildShapes += _getNumChildShapesInShapesSubpart(part);
	return m_shapesSubparts.m_size - 1;
}

// The count is cached in the mutable m_cachedNumChildShapes on first use.
int32_t hkpExtendedMeshShape::getNumChildShapes() const {
	if (m_cachedNumChildShapes < 0) {
		m_cachedNumChildShapes = static_cast<const hkpShapeContainer*>(this)->hkpShapeContainer::getNumChildShapes();
	}
	return m_cachedNumChildShapes;
}

// The first child's key, or the next one when that child is a degenerate triangle.
FNVD_NO_STACK_COOKIE uint32_t hkpExtendedMeshShape::getFirstKey() const {
	const hkpShapeContainer* pContainer = this;
	int32_t numTrianglesSubparts = m_trianglesSubparts.m_size;
	if (!(numTrianglesSubparts + m_shapesSubparts.m_size)) {
		return 0xFFFFFFFFu;
	}
	uint32_t key = numTrianglesSubparts != 0 ? 0 : 0x80000000u;
	alignas(16) char buffer[512];
	const hkpShape* pChild = pContainer->getChildShape(key, buffer);
	if (pChild->m_type.m_storage != HK_SHAPE_TRIANGLE) {
		return key;
	}
	const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pChild);
	hkBool result;
	if (!hkpTriangleUtil::isDegenerate(&result, pTriangle->getVertex(0), pTriangle->getVertex(1),
			pTriangle->getVertex(2), hkDefaultTriangleDegeneracyTolerance)
			->m_bool) {
		return key;
	}
	return pContainer->getNextKey(key);
}

// Steps to the next child, past the end of each subpart into the next and from the triangles subparts into the
// shapes subparts, skipping degenerate triangles.
FNVD_NO_STACK_COOKIE uint32_t hkpExtendedMeshShape::getNextKey(uint32_t initialKey) const {
	const hkpShapeContainer* pContainer = this;
	uint32_t subpartIndex = (initialKey & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex);
	int32_t terminalIndex = initialKey & (~0u >> m_numBitsForSubpartIndex);
	uint32_t shapesBit = initialKey & 0x80000000u;
	alignas(16) char buffer[512];
	hkBool result;
	while (true) {
		++terminalIndex;
		if (!shapesBit) {
			if (terminalIndex >= m_trianglesSubparts.m_data[subpartIndex].m_numTriangleShapes) {
				++subpartIndex;
				terminalIndex = 0;
				if (subpartIndex >= static_cast<uint32_t>(m_trianglesSubparts.m_size)) {
					if (m_shapesSubparts.m_size == 0) {
						return 0xFFFFFFFFu;
					}
					subpartIndex = 0;
					shapesBit = 0x80000000u;
					terminalIndex = -1;
					continue;
				}
			}
		} else if (terminalIndex >= m_shapesSubparts.m_data[subpartIndex].m_childShapes.m_size) {
			++subpartIndex;
			if (subpartIndex >= static_cast<uint32_t>(m_shapesSubparts.m_size)) {
				return 0xFFFFFFFFu;
			}
			terminalIndex = 0;
		}
		uint32_t key = (subpartIndex << (32 - m_numBitsForSubpartIndex)) | shapesBit | terminalIndex;
		const hkpShape* pChild = pContainer->getChildShape(key, buffer);
		if (pChild->m_type.m_storage != HK_SHAPE_TRIANGLE) {
			return key;
		}
		const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pChild);
		if (!hkpTriangleUtil::isDegenerate(&result, pTriangle->getVertex(0), pTriangle->getVertex(1),
				pTriangle->getVertex(2), hkDefaultTriangleDegeneracyTolerance)
				->m_bool) {
			return key;
		}
	}
}

uint32_t hkpExtendedMeshShape::getCollisionFilterInfo(uint32_t key) const {
	const hkpMeshMaterial* MeshMaterial = getMeshMaterial(key);
	if (MeshMaterial) {
		return MeshMaterial->m_filterInfo;
	} else {
		return m_defaultCollisionFilterInfo;
	}
}

// KEEP: a triangle key's vertices are each loaded into a stack vector by x, y and z only (the SDK's load3), and the
// whole vector, its w never written in this function, is multiplied by m_scaling into the triangle's m_vertexA,
// m_vertexB and m_vertexC (0xD55EC3-0xD55F44): an uninitialised read that reaches the result.
const hkpShape* hkpExtendedMeshShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
	const uint32_t subpartIndex = (key & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex);
	const uint32_t terminalIndex = key & (0xFFFFFFFFu >> m_numBitsForSubpartIndex);
	// The key's top bit is the subpart type: set for SUBPART_SHAPE.
	if ((key & 0x80000000u) != 0) {
		// The exe copies the subpart to an aligned local first (the SDK's memCpy16 of its 64 bytes, out of line).
		// Natively the subpart is read in place: a byte copy would be destroyed as an owner of the child shapes.
		const ShapesSubpart& part = m_shapesSubparts.m_data[subpartIndex];
		hkpConvexShape* childShape = part.m_childShapes.m_data[terminalIndex];
		const int32_t flags = part.m_translation.getInt24W();
		if (flags == ShapesSubpart::FLAG_NONE) {
			return childShape;
		}
		if (flags == ShapesSubpart::FLAG_TRANSLATE_SET) {
			if (!&buffer) {
				return nullptr;
			}
			// The SDK's placement new of the translate shape in the buffer: raw memory the exe constructs in.
			return ::new (&buffer[0])
				hkpConvexTranslateShape(childShape, part.m_translation, hkpShapeContainer::REFERENCE_POLICY_IGNORE);
		}
		// The exe copies the translation with dword moves, then sets the rotation out of line.
		hkTransform transform;
		transform.m_translation.m_quad = part.m_translation.m_quad;
		transform.m_rotation.set(part.m_rotation);
		if (!&buffer) {
			return nullptr;
		}
		return ::new (&buffer[0])
			hkpConvexTransformShape(childShape, transform, hkpShapeContainer::REFERENCE_POLICY_IGNORE);
	}

	// A single triangles subpart is the embedded one; any other is copied to an aligned local first (the SDK's
	// memCpy16 of its 96 bytes, out of line).
	const TrianglesSubpart* part;
	TrianglesSubpart partCopy;
	if (m_trianglesSubparts.m_size == 1) {
		part = &m_embeddedTrianglesSubpart;
	} else {
		copyTrianglesSubpart(&partCopy, &m_trianglesSubparts.m_data[subpartIndex]);
		part = &partCopy;
	}
	// The index buffer is untyped strided memory (the SDK's hkAddByteOffsetConst), read at the striding's width.
	const uint8_t* triangle = static_cast<const uint8_t*>(part->m_indexBase) + terminalIndex * part->m_indexStriding;
	// Every other triangle of a strip is flipped: its second and third indices swap.
	const int32_t flip = static_cast<int32_t>(terminalIndex) & part->m_flipAlternateTriangles;
	const int16_t b = static_cast<int16_t>(flip + 1);
	const int16_t c = static_cast<int16_t>((flip ^ 1) + 1);
	int32_t index0;
	int32_t index1;
	int32_t index2;
	switch (part->m_stridingType.m_storage) {
	case INDICES_INT8:
		index0 = triangle[0];
		index1 = triangle[b];
		index2 = triangle[c];
		break;
	case INDICES_INT16: {
		const uint16_t* indices = reinterpret_cast<const uint16_t*>(triangle);
		index0 = indices[0];
		index1 = indices[b];
		index2 = indices[c];
		break;
	}
	case INDICES_INT32: {
		const int32_t* indices = reinterpret_cast<const int32_t*>(triangle);
		index0 = indices[0];
		index1 = indices[b];
		index2 = indices[c];
		break;
	}
	default:
		index0 = 0;
		index1 = 0;
		index2 = 0;
		break;
	}
	// The vertex buffer is untyped strided memory as well (three floats at each vertexStriding bytes).
	const char* vertexBase = reinterpret_cast<const char*>(part->m_vertexBase);
	const int32_t vertexStriding = part->m_vertexStriding;

	// The SDK's placement new of the triangle shape in the buffer (default radius, no welding): raw memory the exe
	// constructs in.
	hkpTriangleShape* pTriangle = nullptr;
	if (&buffer) {
		pTriangle =
			::new (&buffer[0]) hkpTriangleShape(hkConvexShapeDefaultRadius, 0, hkpWeldingUtility::WELDING_TYPE_NONE);
	}
	pTriangle->m_weldingType.m_storage = m_weldingType.m_storage;
	pTriangle->m_radius = m_triangleRadius;
	// The extrusion's squared length, summed x + y, then + z, as the exe's addss do.
	const hkQuadReal extrusion = part->m_extrusion.m_quad;
	const hkQuadReal squares = _mm_mul_ps(extrusion, extrusion);
	const float xy = squares[1] + squares[0];
	const float lengthSquared = squares[2] + xy;
	pTriangle->m_type.m_storage = HK_SHAPE_TRIANGLE;
	pTriangle->m_isExtruded = lengthSquared > 0.0f ? 1 : 0;
	pTriangle->m_extrusion.m_quad = extrusion;
	pTriangle->m_weldingInfo =
		m_weldingInfo.m_size == 0 ? 0 : m_weldingInfo.m_data[part->m_triangleOffset + terminalIndex];
	hkVector4 vertex;
	vertex.load3(reinterpret_cast<const float*>(vertexBase + vertexStriding * index0));
	pTriangle->m_vertexA.setMul4(vertex, m_scaling);
	vertex.load3(reinterpret_cast<const float*>(vertexBase + vertexStriding * index1));
	pTriangle->m_vertexB.setMul4(vertex, m_scaling);
	vertex.load3(reinterpret_cast<const float*>(vertexBase + vertexStriding * index2));
	pTriangle->m_vertexC.setMul4(vertex, m_scaling);
	return pTriangle;
}

// The SDK's count of a triangles subpart's child shapes: its triangles that are not degenerate. Each triangle is
// fetched by its shape key into a 512-byte buffer.
int32_t
hkpExtendedMeshShape::_getNumChildShapesInTrianglesSubpart(const hkpExtendedMeshShape::TrianglesSubpart& subpart,
	int32_t subpartIndex) const {
	int32_t iNumChildShapes = 0;
	for (int32_t i = 0; i < subpart.m_numTriangleShapes; ++i) {
		alignas(16) char buffer[512];
		const uint32_t uiKey =
			static_cast<uint32_t>(subpartIndex) << (32 - m_numBitsForSubpartIndex) | static_cast<uint32_t>(i);
		const hkpTriangleShape* pTriangle =
			static_cast<const hkpTriangleShape*>(hkpExtendedMeshShape::getChildShape(uiKey, buffer));
		hkBool bDegenerate;
		if (!hkpTriangleUtil::isDegenerate(&bDegenerate, pTriangle->getVertex(0), pTriangle->getVertex(1),
				pTriangle->getVertex(2), hkDefaultTriangleDegeneracyTolerance)
				->m_bool) {
			++iNumChildShapes;
		}
	}
	return iNumChildShapes;
}

void hkpExtendedMeshShape::recalcAabbExtents() {
	FNVD_INLINED_LOCAL(hkAabb, aabb, std::nullopt, std::nullopt);
	__m128 vMin = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
	__m128 vMax = _mm_xor_ps(hkQuadRealNegateMask, vMin);
	__m128 vRadius = _mm_shuffle_ps(_mm_set_ss(m_triangleRadius), _mm_set_ss(m_triangleRadius), 0);
	for (int32_t i = 0; i < m_trianglesSubparts.m_size; ++i) {
		calcAabbExtents(m_trianglesSubparts.m_data[i], m_scaling, aabb);
		vMin = _mm_min_ps(vMin, _mm_sub_ps(aabb.m_min.m_quad, vRadius));
		vMax = _mm_max_ps(vMax, _mm_add_ps(aabb.m_max.m_quad, vRadius));
	}
	for (int32_t i = 0; i < m_shapesSubparts.m_size; ++i) {
		calcAabbExtents(&m_shapesSubparts.m_data[i], &aabb);
		vMin = _mm_min_ps(vMin, aabb.m_min.m_quad);
		vMax = _mm_max_ps(vMax, aabb.m_max.m_quad);
	}
	m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(vMax, vMin), hkQuadRealHalf);
	m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(vMax, vMin), hkQuadRealHalf);
}

hkpExtendedMeshShape::TrianglesSubpart* hkpExtendedMeshShape::expandOneTriangleSubparts() {
	if (m_trianglesSubparts.m_size) {
		hkArray<hkpExtendedMeshShape::TrianglesSubpart, hkContainerHeapAllocator>* p_m_trianglesSubparts =
			&m_trianglesSubparts;
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t m_size = p_m_trianglesSubparts->m_size;
		int32_t v5 = p_m_trianglesSubparts->m_capacityAndFlags &
			hkArrayBase<hkpExtendedMeshShape::TrianglesSubpart>::CAPACITY_MASK;
		if (v5 < m_size + 1) {
			int32_t v6 = 2 * v5;
			if (m_size + 1 >= v6) {
				v6 = m_size + 1;
			}
			hkArrayUtil::_reserve(*m_heap, p_m_trianglesSubparts, v6, sizeof(hkpExtendedMeshShape::TrianglesSubpart));
		}
		hkpExtendedMeshShape::TrianglesSubpart* v7 = &p_m_trianglesSubparts->m_data[p_m_trianglesSubparts->m_size];
		if (v7) {
			v7->m_materialIndexStriding = 0;
			v7->m_materialStriding = 0;
			v7->m_type.m_storage = 0;
			v7->m_materialIndexStridingType.m_storage = 1;
			v7->m_numMaterials = 1;
			v7->m_materialBase = 0;
			v7->m_materialIndexBase = 0;
			v7->m_materialClass = 0;
			v7->m_flipAlternateTriangles = 0;
			v7->m_extrusion.m_quad = _mm_setzero_ps();
			v7->m_userData = 0;
		}
		++p_m_trianglesSubparts->m_size;
		return &p_m_trianglesSubparts->m_data[m_size];
	} else {
		m_trianglesSubparts.m_data = &m_embeddedTrianglesSubpart;
		m_trianglesSubparts.m_size = 1;
		m_trianglesSubparts.m_capacityAndFlags = -0x7FFFFFFF;
		return &m_embeddedTrianglesSubpart;
	}
}

hkpExtendedMeshShape::ShapesSubpart* hkpExtendedMeshShape::expandOneShapesSubparts() {
	hkArray<hkpExtendedMeshShape::ShapesSubpart, hkContainerHeapAllocator>* p_m_shapesSubparts = &m_shapesSubparts;
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_size = p_m_shapesSubparts->m_size;
	int32_t v4 =
		p_m_shapesSubparts->m_capacityAndFlags & hkArrayBase<hkpExtendedMeshShape::ShapesSubpart>::CAPACITY_MASK;
	if (v4 < m_size + 1) {
		int32_t v5 = 2 * v4;
		if (m_size + 1 >= v5) {
			v5 = m_size + 1;
		}
		hkArrayUtil::_reserve(*m_heap, p_m_shapesSubparts, v5, sizeof(hkpExtendedMeshShape::ShapesSubpart));
	}
	hkpExtendedMeshShape::ShapesSubpart* v6 = &p_m_shapesSubparts->m_data[p_m_shapesSubparts->m_size];
	if (v6) {
		v6->m_materialIndexStriding = 0;
		v6->m_materialStriding = 0;
		v6->m_numMaterials = 1;
		v6->m_type.m_storage = 1;
		v6->m_materialIndexStridingType.m_storage = 1;
		v6->m_materialBase = 0;
		v6->m_materialIndexBase = 0;
		v6->m_materialClass = 0;
		v6->m_childShapes.m_data = 0;
		v6->m_childShapes.m_size = 0;
		v6->m_childShapes.m_capacityAndFlags = hkArrayBase<hkpConvexShape*>::DONT_DEALLOCATE_FLAG;
	}
	++p_m_shapesSubparts->m_size;
	return &p_m_shapesSubparts->m_data[m_size];
}

void hkpExtendedMeshShape::setScaling(const hkVector4& scaling) {
	m_scaling.m_quad = scaling.m_quad;
}

int32_t
hkpExtendedMeshShape::_getNumChildShapesInShapesSubpart(const hkpExtendedMeshShape::ShapesSubpart& subpart) const {
	return subpart.m_childShapes.m_size;
}

// The SDK's getSubPart, getSubPartIndex and getTerminalIndexInSubPart, inlined; a single triangles subpart is the
// embedded one.
const hkpMeshMaterial* hkpExtendedMeshShape::getMeshMaterial(uint32_t key) const {
	uint32_t subpartIndex = (key & 0x7FFFFFFF) >> (32 - m_numBitsForSubpartIndex);
	int32_t terminalIndex = key & (~0u >> m_numBitsForSubpartIndex);
	const Subpart* pPart;
	if (key & 0x80000000u) {
		pPart = &m_shapesSubparts.m_data[subpartIndex];
	} else {
		uint32_t numTrianglesSubparts = m_trianglesSubparts.m_size;
		if (numTrianglesSubparts == 1) {
			pPart = &m_embeddedTrianglesSubpart;
		} else {
			if (subpartIndex >= numTrianglesSubparts) {
				return nullptr;
			}
			pPart = &m_trianglesSubparts.m_data[subpartIndex];
		}
	}
	const uint8_t* pIndexBase = static_cast<const uint8_t*>(pPart->m_materialIndexBase);
	int32_t materialStriding = pPart->m_materialStriding;
	if (!pIndexBase) {
		return nullptr;
	}
	const uint8_t* pIndex = pIndexBase + terminalIndex * pPart->m_materialIndexStriding;
	int32_t materialOffset;
	if (pPart->m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
		materialOffset = materialStriding * *pIndex;
	} else {
		// The material index buffer is the user's untyped memory: 16-bit indices here.
		materialOffset = materialStriding * *reinterpret_cast<const uint16_t*>(pIndex);
	}
	// The SDK's hkAddByteOffsetConst over the user's untyped material buffer: materials are m_materialStriding
	// bytes apart.
	return reinterpret_cast<const hkpMeshMaterial*>(reinterpret_cast<const char*>(pPart->m_materialBase) +
		materialOffset);
}

// Each vertex is scaled into floats (fld / fmul / fstp dword), packed as
// (x, y, z, 0) (unpcklps), transformed by the identity transform out of line, and added to the box with minps/maxps
// (the box first, as the exe's operands are ordered). The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void calcAabbExtents(const hkpExtendedMeshShape::TrianglesSubpart& part, hkVector4& scaling,
	hkAabb& out) {
	// HK_REAL_MAX (0x104DA48, 0x7F7FFFEE); the maximum is its sign flipped through the mask at 0x10C44D0.
	__m128 kMax = _mm_set1_ps(3.40282e+38f);
	_mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0], kMax);
	_mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0], _mm_xor_ps(kMax, _mm_set1_ps(-0.0f)));
	// The vertex and index buffers are untyped strided memory in the exe: the casts below address them by bytes.
	for (int32_t i = 0; i < part.m_numTriangleShapes; ++i) {
		const char* pIndex = static_cast<const char*>(part.m_indexBase) + part.m_indexStriding * i;
		uint32_t uiA;
		uint32_t uiB;
		uint32_t uiC;
		if (part.m_stridingType.m_storage == hkpExtendedMeshShape::INDICES_INT8) {
			const uint8_t* pIndices = reinterpret_cast<const uint8_t*>(pIndex);
			uiA = pIndices[0];
			uiB = pIndices[1];
			uiC = pIndices[2];
		} else if (part.m_stridingType.m_storage == hkpExtendedMeshShape::INDICES_INT16) {
			const uint16_t* pIndices = reinterpret_cast<const uint16_t*>(pIndex);
			uiA = pIndices[0];
			uiB = pIndices[1];
			uiC = pIndices[2];
		} else {
			const uint32_t* pIndices = reinterpret_cast<const uint32_t*>(pIndex);
			uiA = pIndices[0];
			uiB = pIndices[1];
			uiC = pIndices[2];
		}
		const char* pVertices = reinterpret_cast<const char*>(part.m_vertexBase);
		int32_t iStriding = part.m_vertexStriding;
		const float* apTriangle[3] = {
			reinterpret_cast<const float*>(pVertices + uiA * iStriding),
			reinterpret_cast<const float*>(pVertices + uiB * iStriding),
			reinterpret_cast<const float*>(pVertices + uiC * iStriding),
		};
		// The exe gives each vertex its own pair of frame slots, the scaled vertex followed by the transformed point
		// ([esp+0x20] to [esp+0x70]), so no vertex reuses the slot the previous call wrote.
		alignas(16) hkVector4 akFrame[6];
		for (int32_t j = 0; j < 3; ++j) {
			const float* pVertex = apTriangle[j];
			float fX = pVertex[0] * scaling.m_quad[0];
			float fY = pVertex[1] * scaling.m_quad[1];
			float fZ = pVertex[2] * scaling.m_quad[2];
			hkVector4& kScaled = akFrame[2 * j];
			kScaled.m_quad[0] = fX;
			kScaled.m_quad[1] = fY;
			kScaled.m_quad[2] = fZ;
			kScaled.m_quad[3] = 0.0f;
			hkVector4& kPoint = akFrame[2 * j + 1];
			kPoint.setTransformedPos(hkTransformIdentity, kScaled);
			__m128 kP = _mm_load_ps(&reinterpret_cast<float*>(&kPoint.m_quad)[0]);
			_mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0],
				_mm_min_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0]), kP));
			_mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0],
				_mm_max_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0]), kP));
		}
	}
}

// The vertex at eax scaled by the vector at ecx (each product an x87 one rounded to a float), w 0, moved by the
// transform and added to the box at esi.
void includeScaledTransformedPoint(const float* a1, const hkTransform* pTransform, const float* r_eax, hkAabb* r_esi) {
	const float x = a1[0] * r_eax[0];
	const float y = a1[1] * r_eax[1];
	const float z = a1[2] * r_eax[2];
	hkVector4 point;
	point.m_quad = _mm_setr_ps(x, y, z, 0.0f);
	hkVector4 transformed;
	transformed.setTransformedPos(*pTransform, point);
	r_esi->includePoint(transformed);
}

void sub_D55500() {
	;
}

void sub_D55510() {
	;
}

// As above, hkString::memCpy16<64>.
void copy64Bytes(void* dst, const void* src) {
	const uint32_t* src32 = static_cast<const uint32_t*>(src);
	uint32_t* dst32 = static_cast<uint32_t*>(dst);
	int32_t numQuads = 4;
	do {
		dst32[0] = src32[0];
		dst32[1] = src32[1];
		dst32[2] = src32[2];
		dst32[3] = src32[3];
		dst32 += 4;
		src32 += 4;
	} while (--numQuads > 0);
}

// The AABB of a shapes subpart in the mesh's space: apAabbOut starts empty (+/- HK_REAL_MAX), then takes in each
// child shape's AABB (tolerance 0) under the subpart's rotation and translation.
void calcAabbExtents(const hkpExtendedMeshShape::ShapesSubpart* part, hkAabb* scaling) {
	scaling->m_min.setAll(rdata_104DA48);
	scaling->m_max.setNeg4(scaling->m_min);
	hkTransform transform;
	transform.getTranslation() = part->m_translation;
	transform.getRotation().set(part->m_rotation);
	FNVD_INLINED_LOCAL(hkAabb, childAabb, std::nullopt, std::nullopt);
	for (int32_t i = 0; i < part->m_childShapes.m_size; i++) {
		part->m_childShapes.m_data[i]->getAabb(transform, 0.0f, childAabb);
		scaling->m_min.setMin4(scaling->m_min, childAabb.m_min);
		scaling->m_max.setMax4(scaling->m_max, childAabb.m_max);
	}
}
