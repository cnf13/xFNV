#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <new>
#include <xmmintrin.h>

hkpConvexListShape::hkpConvexListShape(const hkpConvexShape* const* apChildShapes, int32_t aiNumChildShapes)
	: hkpConvexShape(HK_SHAPE_CONVEX_LIST, 0.0f), m_minDistanceToUseConvexHullForGetClosestPoints(1.0f) {
	setShapesAndRadius(apChildShapes, aiNumChildShapes);
	setUseCachedAabb(true);
}

hkpConvexListShape::~hkpConvexListShape() {
	for (int32_t i = 0; i < m_childShapes.m_size; i++) {
		m_childShapes.m_data[i]->removeReference();
	}
}

// The child pointer array is reported as one array chunk, then every child as a referenced object; the base then
// accounts for the shape itself. The flag test is the array's own DONT_DEALLOCATE bit.
void hkpConvexListShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	// EXE-INLINED none: hkStatisticsCollector::addArray
	const int32_t iCapacityAndFlags = m_childShapes.getCapacityAndFlags();
	if (iCapacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
		const int32_t iNumBytesAllocated = hkStatisticsCollector::getAllocatedSize(m_childShapes.m_data,
			(iCapacityAndFlags & hkArrayBase<const hkpConvexShape*>::CAPACITY_MASK) * 4);
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "ChildPtrs", m_childShapes.m_data,
			m_childShapes.m_size * 4, iNumBytesAllocated);
	}
	for (int32_t i = 0; i < m_childShapes.m_size; ++i) {
		collector->addReferencedObject(nullptr, m_childShapes.m_data[i], 0);
	}
	hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

const hkpShapeContainer* hkpConvexListShape::getContainer() const {
	if (this) {
		return static_cast<const hkpShapeContainer*>(this);
	} else {
		return 0;
	}
}

void hkpConvexListShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	if (m_useCachedAabb) {
		float extraRadius = m_radius + tolerance;
		hkAabbUtil::calcAabb(localToWorld, m_aabbHalfExtents, m_aabbCenter, extraRadius, out);
	} else {
		m_childShapes.m_data[0]->getAabbImpl(localToWorld, tolerance, out);
		hkAabb childAabb;
		for (int32_t i = 1; i < m_childShapes.m_size; ++i) {
			m_childShapes.m_data[i]->getAabbImpl(localToWorld, tolerance, childAabb);
			out.m_min.m_quad = _mm_min_ps(out.m_min.m_quad, childAabb.m_min.m_quad);
			out.m_max.m_quad = _mm_max_ps(out.m_max.m_quad, childAabb.m_max.m_quad);
		}
	}
}

hkBool hkpConvexListShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
	HK_TIMER_BEGIN("rcCxList", nullptr);
	++results.m_shapeKeyIndex;
	const hkpShapeContainer* pContainer = static_cast<const hkpShapeContainer*>(this);
	uint32_t hitKey = HK_INVALID_SHAPE_KEY;
	for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
		alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
		const hkpShape* pChildShape = pContainer->getChildShape(key, shapeBuffer);
		hkBool childHit = pChildShape->castRayImpl(input, results);
		if (childHit.m_bool) {
			hitKey = key;
		}
	}
	--results.m_shapeKeyIndex;
	if (hitKey != HK_INVALID_SHAPE_KEY) {
		results.m_shapeKeys[results.m_shapeKeyIndex] = hitKey;
	}
	hkBool result;
	result.m_bool = hitKey != HK_INVALID_SHAPE_KEY;
	HK_TIMER_END();
	return result;
}

void hkpConvexListShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	HK_TIMER_BEGIN("rcShpCollect", nullptr);
	const hkpShapeContainer* pContainer = static_cast<const hkpShapeContainer*>(this);
	for (uint32_t key = pContainer->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = pContainer->getNextKey(key)) {
		alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
		const hkpShape* pChildShape = pContainer->getChildShape(key, shapeBuffer);
		// Child body of cdBody, equivalent to hkpCdBody(&cdBody, ...).
		hkpCdBody childBody;
		childBody.m_motion.pState = cdBody.m_motion.pState;
		childBody.m_parent = &cdBody;
		childBody.m_shape = pChildShape;
		childBody.m_shapeKey = key;
		pChildShape->castRayWithCollectorImpl(input, childBody, collector);
	}
	HK_TIMER_END();
}

int32_t hkpConvexListShape::getNumCollisionSpheresImpl() const {
	const hkpConvexShape* v4;
	int32_t v2 = 0;
	for (int32_t i = 0; i < m_childShapes.m_size; v2 += v4->getNumCollisionSpheresImpl()) {
		v4 = m_childShapes.m_data[i++];
	}
	return v2;
}

const hkSphere* hkpConvexListShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
	int32_t v4 = 0;
	hkSphere* v5 = sphereBuffer;
	if (m_childShapes.m_size > 0) {
		do {
			const hkpConvexShape* v6 = m_childShapes.m_data[v4];
			v6->getCollisionSpheresImpl(v5);
			++v4;
			v5 += v6->getNumCollisionSpheresImpl();
		} while (v4 < m_childShapes.m_size);
		return sphereBuffer;
	}
	return sphereBuffer;
}

void hkpConvexListShape::getFirstVertex(hkVector4& v) const {
	m_childShapes.m_data[0]->getFirstVertex(v);
}

// The dot product is the sum z + (y + x), stored as a float and then compared with maxDot.
void hkpConvexListShape::getSupportingVertexImpl(const hkVector4& dir, hkpCdVertex& supportingVertexOut) const {
	float maxDot = -3.40282e+38f;
	int32_t subShape = 0;
	for (int32_t i = 0; i < m_childShapes.m_size; ++i) {
		hkpCdVertex support;
		m_childShapes.m_data[i]->getSupportingVertexImpl(dir, support);
		__m128 product = _mm_mul_ps(dir.m_quad, support.m_quad);
		float dot = _mm_shuffle_ps(product, product, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(product, product, 85)[0] + product[0]);
		if (maxDot < dot) {
			maxDot = dot;
			supportingVertexOut = support;
			subShape = i;
		}
	}
	supportingVertexOut.setInt24W(supportingVertexOut.getInt24W() + (subShape << 8));
}

void hkpConvexListShape::convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds,
	hkpCdVertex* verticesOut) const {
	int32_t v4 = numIds;
	if (numIds > 0) {
		hkpCdVertex* v6 = verticesOut;
		do {
			uint32_t v7 = *ids;
			// The child's vertex id, the id's low byte.
			uint16_t usChildVertexId = static_cast<uint8_t>(*ids);
			v7 >>= 8;
			const hkpConvexShape* v8 = m_childShapes.m_data[v7];
			v8->convertVertexIdsToVerticesImpl(&usChildVertexId, 1, v6);
			v6->m_quad[3] = std::bit_cast<float>(static_cast<int32_t>(((v7 << 8) +
					(std::bit_cast<int32_t>(v6->m_quad[3]) & 0xC0FFFFFFu) |
				0x3F000000)));
			++ids;
			++v6;
			--v4;
		} while (v4);
	}
}

int32_t hkpConvexListShape::getNumChildShapes() const {
	return m_childShapes.m_size;
}

uint32_t hkpConvexListShape::getFirstKey() const {
	return (uint32_t)0;
}

uint32_t hkpConvexListShape::getNextKey(uint32_t oldKey) const {
	// The SDK compares the next key with the child count as an int.
	if (static_cast<int32_t>(oldKey + 1) < m_childShapes.m_size) {
		return oldKey + 1;
	}
	return HK_INVALID_SHAPE_KEY;
}

uint32_t hkpConvexListShape::getCollisionFilterInfo(uint32_t a1) const {
	return (uint32_t)0;
}

const hkpShape* hkpConvexListShape::getChildShape(uint32_t a1, char (&a2)[512]) const {
	return m_childShapes.m_data[a1];
}

bool hkpConvexListShape::getUseCachedAabb() {
	return (bool)((uint8_t)m_useCachedAabb);
}

void hkpConvexListShape::setUseCachedAabb(bool useCachedAabb) {
	m_useCachedAabb = useCachedAabb;
	if (useCachedAabb) {
		__m128 v3 = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
		int32_t v4 = 0;
		__m128 v5 = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
		__m128 v7 = v3;
		for (__m128 i = v5; v4 < m_childShapes.m_size; i = v5) {
			hkAabb v9;
			const hkpConvexShape* v6 = m_childShapes.m_data[v4];
			v6->getAabbImpl(hkTransformIdentity, 0.0f, v9);
			v3 = _mm_min_ps(v7, v9.m_min.m_quad);
			++v4;
			v5 = _mm_max_ps(i, v9.m_max.m_quad);
			v7 = v3;
		}
		m_aabbCenter.m_quad = _mm_mul_ps(_mm_add_ps(v5, v3), hkQuadRealHalf);
		m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_sub_ps(v5, v3), hkQuadRealHalf);
	}
}

void hkpConvexListShape::setShapesAndRadius(const hkpConvexShape* const* shapeArray, int32_t numShapes) {
	hkArray<const hkpConvexShape*, hkContainerHeapAllocator>* p_m_childShapes = &m_childShapes;
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v6 = p_m_childShapes->m_capacityAndFlags & hkArrayBase<const hkpConvexShape*>::CAPACITY_MASK;
	if (v6 < numShapes) {
		int32_t v7 = 2 * v6;
		if (numShapes >= v7) {
			v7 = numShapes;
		}
		hkArrayUtil::_reserve(m_heap, p_m_childShapes, v7, sizeof(const hkpConvexShape*));
	}
	p_m_childShapes->m_size = numShapes;
	m_radius = (*shapeArray)->m_radius;
	for (int32_t i = 0; i < numShapes; ++i) {
		p_m_childShapes->m_data[i] = shapeArray[i];
		shapeArray[i]->addReference();
	}
}

const hkClass& hkpConvexListShape::staticClass() {
	return hkpConvexListShapeClass;
}

hkpConvexTransformShape::hkpConvexTransformShape(hkpConvexShape* apChildShape, const hkTransform& arTransform,
	hkpShapeContainer::ReferencePolicy aeRef)
	: hkpConvexTransformShapeBase(HK_SHAPE_CONVEX_TRANSFORM, apChildShape->m_radius, apChildShape, aeRef),
	  m_transform(arTransform) {}

hkpConvexTransformShape::~hkpConvexTransformShape() {}

// The single child shape is reported as a referenced object, then the base accounts for the shape itself.
void hkpConvexTransformShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	collector->addReferencedObject("Child", m_childShape.m_childShape, 0);
	hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

float hkpConvexTransformShape::getMaximumProjection(const hkVector4& direction) const {
	hkVector4 v3;
	hkQuaternion v4;
	hkVector4 v5;
	uint32_t v13;
	hkVector4 v14;
	v3.m_quad = m_transform.m_rotation.m_col0.m_quad;
	v4.m_vec.m_quad = m_transform.m_rotation.m_col1.m_quad;
	v5.m_quad = m_transform.m_rotation.m_col2.m_quad;
	const hkpShape* m_childShape = this->m_childShape.m_childShape;
	__m128 v7 = _mm_unpacklo_ps(v3.m_quad, v4.m_vec.m_quad);
	__m128 v8 = _mm_movelh_ps(v7, v5.m_quad);
	v14.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 85),
										   _mm_shuffle_ps(_mm_movehl_ps(v8, v7), v5.m_quad, 212)),
								_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 0), v8)),
		_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 170),
			_mm_shuffle_ps(_mm_unpackhi_ps(v3.m_quad, v4.m_vec.m_quad), v5.m_quad, 228)));
	*reinterpret_cast<float*>(&v13) = m_childShape->getMaximumProjection(v14);
	__m128 v9 =
		_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v13)), _mm_set_ss(std::bit_cast<float>(v13)), 0),
			v14.m_quad);
	__m128 v10 =
		_mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v9, v9, 85), m_transform.m_rotation.m_col1.m_quad),
								  _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), m_transform.m_rotation.m_col0.m_quad)),
					   _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v9, v9, 170), m_transform.m_rotation.m_col2.m_quad),
						   m_transform.m_translation.m_quad)),
			direction.m_quad);
	return _mm_shuffle_ps(v10, v10, 170)[0] + static_cast<float>(_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]);
}

const hkpShapeContainer* hkpConvexTransformShape::getContainer() const {
	return &m_childShape;
}

// The child's size within the buffer left after this shape's own; a child stored right after this shape is counted
// with it (child size 0), any other is uploaded on its own.
int32_t hkpConvexTransformShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input,
	int32_t spuBufferSizeLeft) const {
	int32_t maxAvailableBufferSizeForChild = spuBufferSizeLeft - static_cast<int32_t>(sizeof(*this));
	int32_t childSize = m_childShape.m_childShape->calcSizeForSpu(input, maxAvailableBufferSizeForChild);
	if (childSize < 0 || childSize > maxAvailableBufferSizeForChild) {
		return -1;
	}
	if (m_childShape.m_childShape == this + 1) {
		m_childShapeSize = 0;
		return static_cast<int32_t>(sizeof(*this)) + childSize;
	}
	m_childShapeSize = childSize;
	return static_cast<int32_t>(sizeof(*this));
}

// The child's box under localToWorld times this shape's transform.
void hkpConvexTransformShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	hkTransform childToWorld;
	childToWorld.setMul(localToWorld, m_transform);
	getChildShape()->getAabbImpl(childToWorld, tolerance, out);
}

// The ray moved into the child's space by the inverse transform (the rotation's transpose), cast against the child
// one shape key level down; a hit's normal is rotated back into this shape's space.
hkBool hkpConvexTransformShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
	HK_TIMER_BEGIN("rcConvTransl", nullptr);
	hkVector4 v8;
	hkVector4 v9;
	hkVector4 v16;
	hkVector4 v17;
	hkVector4 v18;
	hkVector4 v20;
	v8.m_quad = m_transform.m_rotation.m_col1.m_quad;
	v9.m_quad = m_transform.m_rotation.m_col2.m_quad;
	__m128 v10 = _mm_sub_ps(input.m_from.m_quad, m_transform.m_translation.m_quad);
	__m128 v11 = _mm_unpacklo_ps(m_transform.m_rotation.m_col0.m_quad, v8.m_quad);
	__m128 v12 = _mm_unpackhi_ps(m_transform.m_rotation.m_col0.m_quad, v8.m_quad);
	hkpShapeRayCastInput localInput = input;
	__m128 v13 = _mm_movelh_ps(v11, v9.m_quad);
	__m128 v14 =
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v10, v10, 85), _mm_shuffle_ps(_mm_movehl_ps(v13, v11), v9.m_quad, 212)),
			_mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), v13));
	__m128 v15 = _mm_shuffle_ps(v12, v9.m_quad, 228);
	v16.m_quad = m_transform.m_rotation.m_col2.m_quad;
	v17.m_quad = _mm_add_ps(v14, _mm_mul_ps(v15, _mm_shuffle_ps(v10, v10, 170)));
	v18.m_quad = m_transform.m_rotation.m_col1.m_quad;
	__m128 v19 = _mm_sub_ps(input.m_to.m_quad, m_transform.m_translation.m_quad);
	v20.m_quad = m_transform.m_rotation.m_col0.m_quad;
	++results.m_shapeKeyIndex;
	const hkpShape* childShape = m_childShape.m_childShape;
	localInput.m_from.m_quad = v17.m_quad;
	__m128 v22 = _mm_unpacklo_ps(v20.m_quad, v18.m_quad);
	__m128 v23 = _mm_movelh_ps(v22, v16.m_quad);
	localInput.m_to.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v19, v19, 85),
													   _mm_shuffle_ps(_mm_movehl_ps(v23, v22), v16.m_quad, 212)),
											_mm_mul_ps(_mm_shuffle_ps(v19, v19, 0), v23)),
		_mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(v20.m_quad, v18.m_quad), v16.m_quad, 228),
			_mm_shuffle_ps(v19, v19, 170)));
	hkBool result = childShape->castRayImpl(localInput, results);
	--results.m_shapeKeyIndex;
	if (result.m_bool) {
		// Copies the normal to a local and rotates that copy into the output.
		hkVector4 normal = results.m_normal;
		results.m_normal.setRotatedDir(m_transform.m_rotation, normal);
		results.m_shapeKeys[results.m_shapeKeyIndex] = 0;
	}
	HK_TIMER_END();
	return result;
}

// The ray moved into the child's space as castRayImpl moves it, cast against the child under a child body whose
// transform is the parent body's times this shape's transform.
void hkpConvexTransformShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	HK_TIMER_BEGIN("rcCxTransform", nullptr);
	hkVector4 v8;
	hkVector4 v9;
	hkVector4 v13;
	hkVector4 v15;
	v8.m_quad = m_transform.m_rotation.m_col1.m_quad;
	v9.m_quad = m_transform.m_rotation.m_col2.m_quad;
	__m128 v10 = _mm_sub_ps(input.m_from.m_quad, m_transform.m_translation.m_quad);
	__m128 v11 = _mm_unpacklo_ps(m_transform.m_rotation.m_col0.m_quad, v8.m_quad);
	__m128 v12 = _mm_movelh_ps(v11, v9.m_quad);
	v13.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v10, v10, 85),
										   _mm_shuffle_ps(_mm_movehl_ps(v12, v11), v9.m_quad, 212)),
								_mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), v12)),
		_mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(m_transform.m_rotation.m_col0.m_quad, v8.m_quad), v9.m_quad, 228),
			_mm_shuffle_ps(v10, v10, 170)));
	__m128 v14 = _mm_sub_ps(input.m_to.m_quad, m_transform.m_translation.m_quad);
	v15.m_quad = m_transform.m_rotation.m_col0.m_quad;
	hkpShapeRayCastInput localInput = input;
	localInput.m_from.m_quad = v13.m_quad;
	__m128 v16 = _mm_unpacklo_ps(v15.m_quad, v8.m_quad);
	__m128 v17 = _mm_movelh_ps(v16, v9.m_quad);
	localInput.m_to.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v14, v14, 85),
													   _mm_shuffle_ps(_mm_movehl_ps(v17, v16), v9.m_quad, 212)),
											_mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), v17)),
		_mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(v15.m_quad, v8.m_quad), v9.m_quad, 228),
			_mm_shuffle_ps(v14, v14, 170)));
	hkTransform childToWorld;
	childToWorld.setMul(*cdBody.m_motion.pMotion, m_transform);
	hkpCdBody childBody;
	childBody.m_motion.pMotion = &childToWorld;
	const hkpShape* childShape = m_childShape.m_childShape;
	childBody.m_parent = &cdBody;
	childBody.m_shape = childShape;
	childBody.m_shapeKey = 0;
	childShape->castRayWithCollectorImpl(localInput, childBody, collector);
	HK_TIMER_END();
}

int32_t hkpConvexTransformShape::getNumCollisionSpheresImpl() const {
	return getChildShape()->getNumCollisionSpheresImpl();
}

// The child's spheres, written into aSphereBuffer moved by the transform (radius kept), from the last one down.
const hkSphere* hkpConvexTransformShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
	const hkSphere* childSpheres = getChildShape()->getCollisionSpheresImpl(sphereBuffer);
	int32_t numSpheres = getChildShape()->getNumCollisionSpheresImpl();
	hkTransform transform = m_transform;
	int32_t i = numSpheres - 1;
	do {
		const __m128 p = childSpheres[i].m_pos.m_quad;
		__m128 v7 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 85), transform.m_rotation.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(p, p, 0), transform.m_rotation.m_col0.m_quad)),
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 170), transform.m_rotation.m_col2.m_quad),
				transform.m_translation.m_quad));
		sphereBuffer[i].m_pos.m_quad = _mm_shuffle_ps(v7, _mm_unpackhi_ps(v7, p), 196);
		--i;
	} while (i >= 0);
	return sphereBuffer;
}

// The child's first vertex moved by the transform.
void hkpConvexTransformShape::getFirstVertex(hkVector4& aVertexOut) const {
	hkVector4 childVertex;
	getChildShape()->getFirstVertex(childVertex);
	const __m128 v = childVertex.m_quad;
	aVertexOut.m_quad =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 85), m_transform.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(v, v, 0), m_transform.m_rotation.m_col0.m_quad)),
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 170), m_transform.m_rotation.m_col2.m_quad),
				m_transform.m_translation.m_quad));
}

// The direction rotated into the child's space (the rotation's transpose), the child's supporting vertex moved back
// by the transform; w stays the child's vertex id.
void hkpConvexTransformShape::getSupportingVertexImpl(const hkVector4& direction,
	hkpCdVertex& supportingVertexOut) const {
	const __m128 c0 = m_transform.m_rotation.m_col0.m_quad;
	const __m128 c1 = m_transform.m_rotation.m_col1.m_quad;
	const __m128 c2 = m_transform.m_rotation.m_col2.m_quad;
	__m128 v8 = _mm_unpacklo_ps(c0, c1);
	__m128 v9 = _mm_movelh_ps(v8, c2);
	hkVector4 localDirection;
	localDirection.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 0), v9),
										   _mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 85),
											   _mm_shuffle_ps(_mm_movehl_ps(v9, v8), c2, 212))),
		_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 170),
			_mm_shuffle_ps(_mm_unpackhi_ps(c0, c1), c2, 228)));
	hkpCdVertex childVertex;
	getChildShape()->getSupportingVertexImpl(localDirection, childVertex);
	const __m128 v = childVertex.m_quad;
	__m128 v10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 85), m_transform.m_rotation.m_col1.m_quad),
								_mm_mul_ps(_mm_shuffle_ps(v, v, 0), m_transform.m_rotation.m_col0.m_quad)),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 170), m_transform.m_rotation.m_col2.m_quad),
			m_transform.m_translation.m_quad));
	supportingVertexOut.m_quad = _mm_shuffle_ps(v10, _mm_unpackhi_ps(v10, v), 196);
}

void hkpConvexTransformShape::convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds,
	hkpCdVertex* verticesOut) const {
	hkVector4 v10;
	static_cast<const hkpConvexShape*>(m_childShape.m_childShape)
		->convertVertexIdsToVerticesImpl(ids, numIds, verticesOut);
	hkTransform transform = m_transform;
	v10.m_quad = transform.m_translation.m_quad;
	int32_t v5 = numIds - 1;
	hkpCdVertex* v6 = &verticesOut[numIds - 1];
	do {
		__m128 v7 =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v6->m_quad, v6->m_quad, 85),
									  transform.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(v6->m_quad, v6->m_quad, 0), transform.m_rotation.m_col0.m_quad)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v6->m_quad, v6->m_quad, 170), transform.m_rotation.m_col2.m_quad),
					v10.m_quad));
		v6->m_quad = _mm_shuffle_ps(v7, _mm_unpackhi_ps(v7, v6->m_quad), 196);
		--v5;
		--v6;
	} while (v5 >= 0);
}

// The child's centre, then moved by the transform (the rotation's columns, then the translation).
void hkpConvexTransformShape::getCentreImpl(hkVector4& centreOut) const {
	// The SDK's getChildShape: the child of a convex transform shape is convex.
	static_cast<const hkpConvexShape*>(m_childShape.m_childShape)->getCentre(centreOut);
	centreOut.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(centreOut.m_quad, centreOut.m_quad, 85),
												 m_transform.m_rotation.m_col1.m_quad),
									  _mm_mul_ps(_mm_shuffle_ps(centreOut.m_quad, centreOut.m_quad, 0),
										  m_transform.m_rotation.m_col0.m_quad)),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(centreOut.m_quad, centreOut.m_quad, 170),
					   m_transform.m_rotation.m_col2.m_quad),
			m_transform.m_translation.m_quad));
}

// The SDK's inline setTransform: the four columns copied into m_transform.
void hkpConvexTransformShape::setTransform(const hkTransform& transform) {
	m_transform.m_rotation.m_col0.m_quad = transform.m_rotation.m_col0.m_quad;
	m_transform.m_rotation.m_col1.m_quad = transform.m_rotation.m_col1.m_quad;
	m_transform.m_rotation.m_col2.m_quad = transform.m_rotation.m_col2.m_quad;
	m_transform.m_translation.m_quad = transform.m_translation.m_quad;
}

const hkClass& hkpConvexTransformShape::staticClass() {
	return hkpConvexTransformShapeClass;
}
