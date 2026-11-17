#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

#include <bit>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpTransformShape(void* p);

hkpTransformShape::MembersArray hkpTransformShape::Members{{"childShape", &hkpSingleShapeContainerClass, nullptr,
															   {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID},
															   0, {0}, 0x10, nullptr},
	{"childShapeSize", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0x18,
		nullptr},
	{"rotation", nullptr, nullptr, {hkClassMember::TYPE_QUATERNION}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
	{"transform", nullptr, nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30,
		nullptr}};
hkClass hkpTransformShapeClass("hkpTransformShape", &hkpShapeClass, 112, nullptr, 0, nullptr, 0,
	hkpTransformShape::Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkpTransformShapeTypeInfo = {"hkpTransformShape", finishLoadedObjecthkpTransformShape,
	cleanupLoadedObjecthkpTransformShape, getVtablehkpTransformShape(), &typeid(hkpTransformShape),
	sizeof(hkpTransformShape)};

static void cleanupLoadedObjecthkpTransformShape(void* p) {
	static_cast<hkpTransformShape*>(p)->~hkpTransformShape();
}

hkpTransformShape::hkpTransformShape(const hkpShape* childShape, const hkTransform& transform)
	: hkpShape(HK_SHAPE_TRANSFORM), m_childShape(childShape) {
	setTransform(transform);
}

hkpTransformShape::~hkpTransformShape() {}

// Tells the collector that the child shape is referenced, then hands the child shape on to the base with hkpShapeClass
// as the class record; the incoming class is not used. The base is called directly, so the call is qualified.
void hkpTransformShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	collector->addReferencedObject("ChildShape", m_childShape.m_childShape, 0);
	hkReferencedObject::calcContentStatistics(collector, &hkpShapeClass);
}

float hkpTransformShape::getMaximumProjection(const hkVector4& direction) const {
	hkVector4 v3;
	hkVector4 v4;
	hkVector4 v5;
	uint32_t v13;
	hkVector4 v14;
	v3.m_quad = m_transform.m_rotation.m_col0.m_quad;
	v4.m_quad = m_transform.m_rotation.m_col1.m_quad;
	v5.m_quad = m_transform.m_rotation.m_col2.m_quad;
	const hkpShape* m_childShape = this->m_childShape.m_childShape;
	__m128 v7 = _mm_unpacklo_ps(v3.m_quad, v4.m_quad);
	__m128 v8 = _mm_movelh_ps(v7, v5.m_quad);
	v14.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 85),
										   _mm_shuffle_ps(_mm_movehl_ps(v8, v7), v5.m_quad, 212)),
								_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 0), v8)),
		_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 170),
			_mm_shuffle_ps(_mm_unpackhi_ps(v3.m_quad, v4.m_quad), v5.m_quad, 228)));
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

const hkpShapeContainer* hkpTransformShape::getContainer() const {
	return &m_childShape;
}

// The SDK's form: the child gets the buffer left after this shape; a child stored right after this shape counts in
// this shape's size, else its size is kept for the SPU.
int32_t hkpTransformShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
	int32_t maxAvailableBufferSizeForChild = spuBufferSizeLeft - static_cast<int32_t>(sizeof(*this));
	int32_t childSize = m_childShape.m_childShape->calcSizeForSpu(input, maxAvailableBufferSizeForChild);
	if (childSize < 0 || childSize > maxAvailableBufferSizeForChild) {
		return -1;
	}
	if (m_childShape.m_childShape == static_cast<const hkpShape*>(this + 1)) {
		m_childShapeSize = 0;
		return static_cast<int32_t>(sizeof(*this)) + childSize;
	}
	m_childShapeSize = childSize;
	return static_cast<int32_t>(sizeof(*this));
}

void hkpTransformShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	hkTransform v5;
	v5.setMul(localToWorld, m_transform);
	m_childShape.m_childShape->getAabbImpl(v5, tolerance, out);
}

// The ray is moved into the child's space, cast one shape-key level down, and a hit's normal is rotated back on a copy
// of the normal.
hkBool hkpTransformShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
	HK_TIMER_BEGIN("rcTransform", nullptr);
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
	hkpShapeRayCastInput subInput = input;
	// EXE-INLINED 0xC7F690
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
	const hkpShape* pChild = m_childShape.m_childShape;
	subInput.m_from.m_quad = v17.m_quad;
	// EXE-INLINED 0xC7F690
	__m128 v22 = _mm_unpacklo_ps(v20.m_quad, v18.m_quad);
	__m128 v23 = _mm_movelh_ps(v22, v16.m_quad);
	subInput.m_to.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v19, v19, 85),
													 _mm_shuffle_ps(_mm_movehl_ps(v23, v22), v16.m_quad, 212)),
										  _mm_mul_ps(_mm_shuffle_ps(v19, v19, 0), v23)),
		_mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(v20.m_quad, v18.m_quad), v16.m_quad, 228),
			_mm_shuffle_ps(v19, v19, 170)));
	hkBool result = pChild->castRayImpl(subInput, results);
	--results.m_shapeKeyIndex;
	if (result.m_bool) {
		hkVector4 normal = results.m_normal;
		results.m_normal.setRotatedDir(m_transform.m_rotation, normal);
		results.m_shapeKeys[results.m_shapeKeyIndex] = 0;
	}
	HK_TIMER_END();
	return result;
}

// The ray is moved into the child's space as in castRayImpl, and the child body carries the parent's transform times
// this shape's.
void hkpTransformShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	HK_TIMER_BEGIN("rcTransform", nullptr);
	hkVector4 v8;
	hkVector4 v9;
	hkVector4 v13;
	hkVector4 v15;
	alignas(16) hkpCdBody childBody;
	hkTransform childTransform;
	v8.m_quad = m_transform.m_rotation.m_col1.m_quad;
	v9.m_quad = m_transform.m_rotation.m_col2.m_quad;
	// EXE-INLINED 0xC7F690
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
	hkpShapeRayCastInput subInput = input;
	subInput.m_from.m_quad = v13.m_quad;
	// EXE-INLINED 0xC7F690
	__m128 v16 = _mm_unpacklo_ps(v15.m_quad, v8.m_quad);
	__m128 v17 = _mm_movelh_ps(v16, v9.m_quad);
	subInput.m_to.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v14, v14, 85),
													 _mm_shuffle_ps(_mm_movehl_ps(v17, v16), v9.m_quad, 212)),
										  _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), v17)),
		_mm_mul_ps(_mm_shuffle_ps(_mm_unpackhi_ps(v15.m_quad, v8.m_quad), v9.m_quad, 228),
			_mm_shuffle_ps(v14, v14, 170)));
	childTransform.setMul(*cdBody.m_motion.pMotion, m_transform);
	childBody.m_motion.pMotion = &childTransform;
	const hkpShape* pChild = m_childShape.m_childShape;
	childBody.m_parent = &cdBody;
	childBody.m_shape = pChild;
	childBody.m_shapeKey = 0;
	pChild->castRayWithCollectorImpl(subInput, childBody, collector);
	HK_TIMER_END();
}

// The transform is copied whole and the rotation member then takes the transform's rotation matrix, which starts at the
// transform's first byte.
void hkpTransformShape::setTransform(const hkTransform& transform) {
	m_transform = transform;
	m_rotation.set(m_transform.m_rotation);
}

const hkClass& hkpTransformShape::staticClass() {
	return hkpTransformShapeClass;
}
