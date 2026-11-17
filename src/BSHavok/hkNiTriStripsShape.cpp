#include "BSHavok/hkNiTriStripsShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiTriStripsShape.h"
#include "BSHavok/hkNormalTriangleShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

// Each sub part is appended as the array's Add does: SetSize grows the buffer by m_uiGrowBy when it is full, then SetAt
// stores the part.
hkNiTriStripsShape::hkNiTriStripsShape(const hkNiTriStripsShapeCinfo& arInfo)
	: hkpShapeCollection(HK_SHAPE_USER0, hkpShapeCollection::COLLECTION_USER), m_radius(arInfo.m_radius) {
	// The SDK's inline set, lane by lane from the creation info's bhkVector4.
	Scale.set(arInfo.Scale.fX, arInfo.Scale.fY, arInfo.Scale.fZ, arInfo.Scale.fW);
	for (int32_t i = 0; i < static_cast<int32_t>(arInfo.pShapesA.m_uiSize); ++i) {
		const hkNiTriStripsData* pPart = &arInfo.pShapesA.m_pBase[i];
		uint32_t uiIndex = pShapesA.m_uiSize;
		if (uiIndex >= pShapesA.m_uiMaxSize) {
			pShapesA.SetSize(uiIndex + pShapesA.m_uiGrowBy);
		}
		pShapesA.SetAt(uiIndex, *pPart);
	}
	// No world box yet: the SDK's inline set, w 0.
	WorldAabb.m_min.set(3.40282e+38f, 3.40282e+38f, 3.40282e+38f, 0.0f);
	WorldAabb.m_max.set(-3.40282e+38f, -3.40282e+38f, -3.40282e+38f, 0.0f);
	m_disableWelding.m_bool = 1;
}

// The destructor only frees pShapesA's buffer, which the array's own destructor does.
hkNiTriStripsShape::~hkNiTriStripsShape() {}

// The box of the sub parts' vertices, each scaled by the global factor and the shape's scale and transformed by
// localToWorld, widened by the radius plus tolerance. The cached box (CalcWorldAabb's) is reused when it is set and its
// transform is within 0.001 of localToWorld. This function only reads the cache.
void hkNiTriStripsShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	// Compares against 3.40282e+38f; NaN goes on to the transform test.
	// EXE-INLINED 0x5609F0
	if (WorldAabb.m_min.m_quad[0] != 3.40282e+38f &&
		AabbTransform.isApproximatelyEqual(localToWorld, 0.001f).m_bool != 0) {
		_mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0],
			_mm_load_ps(&reinterpret_cast<const float*>(&WorldAabb.m_min.m_quad)[0]));
		_mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0],
			_mm_load_ps(&reinterpret_cast<const float*>(&WorldAabb.m_max.m_quad)[0]));
	} else {
		_mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0],
			_mm_setr_ps(3.40282e+38f, 3.40282e+38f, 3.40282e+38f, 0.0f));
		_mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0],
			_mm_setr_ps(-3.40282e+38f, -3.40282e+38f, -3.40282e+38f, 0.0f));
		uint32_t uiCount = pShapesA.m_uiSize;
		for (uint32_t i = 0; i < uiCount; ++i) {
			NiTriStripsData* pData = pShapesA.m_pBase[i].spGeom.m_pObject;
			const NiPoint3* pVertex = pData->m_pkVertex;
			uint32_t uiVertices = pData->m_usVertices;
			for (; uiVertices != 0; --uiVertices, ++pVertex) {
				alignas(16) hkVector4 kLocal;
				// Reads the factor once per vertex.
				const float fScale = bhkConvert::fBS2HkScaleSC;
				kLocal.m_quad[0] = pVertex->x * fScale;
				kLocal.m_quad[1] = pVertex->y * fScale;
				kLocal.m_quad[2] = fScale * pVertex->z;
				kLocal.m_quad[3] = 0.0f;
				__m128 kScaled = _mm_mul_ps(_mm_load_ps(&reinterpret_cast<const float*>(&Scale.m_quad)[0]),
					_mm_load_ps(&reinterpret_cast<float*>(&kLocal.m_quad)[0]));
				_mm_store_ps(&reinterpret_cast<float*>(&kLocal.m_quad)[0], kScaled);
				alignas(16) hkVector4 kWorld;
				kWorld.setTransformedPos(localToWorld, kLocal);
				__m128 kPoint = _mm_load_ps(&reinterpret_cast<float*>(&kWorld.m_quad)[0]);
				_mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0],
					_mm_min_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0]), kPoint));
				_mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0],
					_mm_max_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0]), kPoint));
			}
		}
	}
	float fRadius = m_radius + tolerance;
	__m128 kRadius = _mm_set1_ps(fRadius);
	_mm_store_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0],
		_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_min.m_quad)[0]), kRadius));
	_mm_store_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0],
		_mm_add_ps(_mm_load_ps(&reinterpret_cast<float*>(&out.m_max.m_quad)[0]), kRadius));
}

hkpShapeType hkNiTriStripsShape::getType() const {
	return HK_SHAPE_TRIANGLE_COLLECTION;
}

// The first sub part's first triangle: key 0 (hkpShapeContainer's vtable at +0x10).
uint32_t hkNiTriStripsShape::getFirstKey() const {
	return 0;
}

// The key's high 12 bits pick the sub part, its low 20 bits the triangle. Both parts are compared signed; it skips
// degenerate triangles and returns HK_INVALID_SHAPE_KEY past the last sub part.
uint32_t hkNiTriStripsShape::getNextKey(uint32_t aOldKey) const {
	while (1) {
		++aOldKey;
		if (static_cast<int32_t>(aOldKey & 0xFFFFF) >=
			pShapesA.m_pBase[aOldKey >> 20].spGeom.m_pObject->m_usTriangles) {
			aOldKey = ((aOldKey >> 20) + 1) << 20;
		}
		if (static_cast<int32_t>(aOldKey >> 20) >= static_cast<int32_t>(pShapesA.m_uiSize)) {
			break;
		}
		if (!isChildShapeDegenerate(aOldKey)) {
			return aOldKey;
		}
	}
	return HK_INVALID_SHAPE_KEY;
}

uint32_t hkNiTriStripsShape::getCollisionFilterInfo(uint32_t aKey) const {
	return pShapesA.m_pBase[aKey >> 20].iFilter;
}

const hkpShape* hkNiTriStripsShape::getChildShape(uint32_t aKey, char (&arBuffer)[512]) const {
	uint32_t i;
	int32_t v11;
	hkNormalTriangleShape* ptriangleShape;
	__m128 v23;
	NiTriStripsData* m_pObject = pShapesA.m_pBase[aKey >> 20].spGeom.m_pObject;
	uint16_t m_usStrips = m_pObject->GetStripCount();
	uint16_t* v5 = m_pObject->m_pusStripLists;
	uint16_t* m_pusStripLengths = m_pObject->GetStripLengths();
	NiPoint3* m_pkVertex = m_pObject->m_pkVertex;
	uint16_t v8 = 0;
	// The key's low 20 bits index the triangle within the strips data; the high bits pick the data.
	for (i = aKey & 0xFFFFF; v8 < m_usStrips; v5 += v11) {
		uint16_t* v10 = &m_pusStripLengths[v8];
		if (i < static_cast<uint32_t>(*v10) - 2) {
			break;
		}
		v11 = *v10;
		++v8;
		i += 2 - v11;
	}
	uint16_t v12 = v5[i];
	uint16_t* v13 = &v5[i];
	uint16_t v14 = v13[1];
	uint16_t v15 = v13[2];
	if (v12 == v14 || (v12 == v15 || v14 == v15)) {
		return 0;
	}
	float* p_x = &m_pkVertex[v12].x;
	float* v17 = &m_pkVertex[v14].x;
	float* v18 = &m_pkVertex[v15].x;
	if (&arBuffer) {
		ptriangleShape = ::new (arBuffer) hkNormalTriangleShape(m_radius);
	} else {
		ptriangleShape = 0;
	}
	double v20 = bhkConvert::fBS2HkScaleSC;
	v23[0] = *p_x * bhkConvert::fBS2HkScaleSC;
	v23[1] = p_x[1] * bhkConvert::fBS2HkScaleSC;
	v23[2] = p_x[2] * bhkConvert::fBS2HkScaleSC;
	v23[3] = 0.0f;
	ptriangleShape->m_vertexA.m_quad = _mm_mul_ps(v23, Scale.m_quad);
	v23[0] = *v17 * v20;
	v23[1] = v17[1] * v20;
	v23[2] = v17[2] * v20;
	v23[3] = 0.0f;
	ptriangleShape->m_vertexB.m_quad = _mm_mul_ps(v23, Scale.m_quad);
	v23[0] = *v18 * v20;
	v23[1] = v18[1] * v20;
	v23[2] = v20 * v18[2];
	v23[3] = 0.0f;
	ptriangleShape->m_vertexC.m_quad = _mm_mul_ps(v23, Scale.m_quad);
	return ptriangleShape;
}

// Each sub part is appended with the array's Add: SetSize grows the buffer by m_uiGrowBy when it is full, then SetAt
// stores the part. The count is compared signed.
void hkNiTriStripsShape::CopyCinfo(hkNiTriStripsShapeCinfo& arInfo) {
	arInfo.m_radius = m_radius;
	arInfo.pShapesA.RemoveAll();
	arInfo.Scale = Scale;
	for (int32_t i = 0; i < static_cast<int32_t>(pShapesA.m_uiSize); ++i) {
		const hkNiTriStripsData* pPart = &pShapesA.m_pBase[i];
		// EXE-INLINED none: NiTLargeArray<hkNiTriStripsData, NiTMallocInterface<hkNiTriStripsData>>::Add
		uint32_t uiIndex = arInfo.pShapesA.m_uiSize;
		if (uiIndex >= arInfo.pShapesA.m_uiMaxSize) {
			arInfo.pShapesA.SetSize(uiIndex + arInfo.pShapesA.m_uiGrowBy);
		}
		arInfo.pShapesA.SetAt(uiIndex, *pPart);
	}
}

void hkNiTriStripsShape::CalcWorldAabb(const hkTransform& localToWorld) {
	__m128 v2 = _mm_undefined_ps();
	__m128 v3 = _mm_undefined_ps();
	__m128 v4 = _mm_undefined_ps();
	hkVector4 v9;
	AabbTransform = localToWorld;
	v2[0] = 3.40282e+38f;
	v3[0] = 3.40282e+38f;
	v4[0] = 0.0f;
	uint32_t m_uiSize = pShapesA.m_uiSize;
	uint32_t v7 = 0;
	__m128 v8 = _mm_unpacklo_ps(v3, v4);
	v9.m_quad =
		_mm_unpacklo_ps(_mm_unpacklo_ps(v2, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(rdata_104DA48)))),
			v8);
	WorldAabb.m_min.m_quad = v9.m_quad;
	v9.m_quad[0] = -3.40282e+38f;
	v8[0] = -3.40282e+38f;
	uint32_t v19 = m_uiSize;
	uint32_t v18 = 0;
	for (WorldAabb.m_max.m_quad =
			 _mm_unpacklo_ps(_mm_unpacklo_ps(v9.m_quad,
								 _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(rdata_104E148)))),
				 _mm_unpacklo_ps(v8, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))));
		v7 < m_uiSize; v18 = v7) {
		NiTriStripsData* m_pObject = pShapesA.m_pBase[v7].spGeom.m_pObject;
		NiPoint3* m_pkVertex = m_pObject->m_pkVertex;
		int32_t m_usVertices = m_pObject->m_usVertices;
		if (m_usVertices) {
			bool v16;
			float* p_z = &m_pkVertex->z;
			int32_t v17 = m_usVertices;
			do {
				hkVector4 v14;
				hkVector4 a3;
				hkVector4 v21;
				v14.m_quad = Scale.m_quad;
				a3.m_quad[0] = *(p_z - 2) * bhkConvert::fBS2HkScaleSC;
				a3.m_quad[1] = *(p_z - 1) * bhkConvert::fBS2HkScaleSC;
				a3.m_quad[2] = bhkConvert::fBS2HkScaleSC * *p_z;
				a3.m_quad[3] = 0.0f;
				a3.m_quad = _mm_mul_ps(v14.m_quad, a3.m_quad);
				v21.setTransformedPos(localToWorld, a3);
				__m128 m_quad = v21.m_quad;
				WorldAabb.m_min.m_quad = _mm_min_ps(WorldAabb.m_min.m_quad, v21.m_quad);
				p_z += 3;
				v16 = v17-- == 1;
				WorldAabb.m_max.m_quad = _mm_max_ps(WorldAabb.m_max.m_quad, m_quad);
			} while (!v16);
			v7 = v18;
			m_uiSize = v19;
		}
		++v7;
	}
}

bool hkNiTriStripsShape::isChildShapeDegenerate(uint32_t a1) const {
	uint32_t i;
	int32_t v8;
	NiTriStripsData* m_pObject = pShapesA.m_pBase[a1 >> 20].spGeom.m_pObject;
	uint16_t m_usStrips = m_pObject->GetStripCount();
	uint16_t* v4 = m_pObject->m_pusStripLists;
	uint16_t* m_pusStripLengths = m_pObject->GetStripLengths();
	uint16_t v6 = 0;
	for (i = a1 & 0xFFFFF; v6 < m_usStrips; v4 += v8) {
		if (i < static_cast<uint32_t>(m_pusStripLengths[v6]) - 2) {
			break;
		}
		v8 = m_pusStripLengths[v6++];
		i += 2 - v8;
	}
	uint16_t v9 = v4[i];
	uint16_t* v10 = &v4[i];
	uint16_t v11 = v10[1];
	uint16_t v12 = v10[2];
	return v9 == v11 || (v9 == v12 || v11 == v12);
}
