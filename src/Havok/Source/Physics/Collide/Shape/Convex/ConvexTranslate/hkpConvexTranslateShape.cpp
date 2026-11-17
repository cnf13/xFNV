#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

#include <xmmintrin.h>

static void finishLoadedObjecthkpConvexTranslateShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpConvexTranslateShape();
static void cleanupLoadedObjecthkpConvexTranslateShape(void* p);

// The class record's member table (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
using hkpConvexTranslateShapeClassMemberArray = const hkClassMember[1];
static hkpConvexTranslateShapeClassMemberArray hkpConvexTranslateShapeClass_Members{
	{"translation", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr}};
hkClass hkpConvexTranslateShapeClass("hkpConvexTranslateShape", &hkpConvexTransformShapeBaseClass, 0x30, nullptr, 0,
	nullptr, 0, hkpConvexTranslateShapeClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpConvexTranslateShapeTypeInfo{"hkpConvexTranslateShape", finishLoadedObjecthkpConvexTranslateShape,
	cleanupLoadedObjecthkpConvexTranslateShape, getVtablehkpConvexTranslateShape(), &typeid(hkpConvexTranslateShape),
	sizeof(hkpConvexTranslateShape)};

static const void* getVtablehkpConvexTranslateShape() {
	return hkGetVtableOf<hkpConvexTranslateShape>();
}

static void cleanupLoadedObjecthkpConvexTranslateShape(void* p) {
	static_cast<hkpConvexTranslateShape*>(p)->~hkpConvexTranslateShape();
}

// The SDK's finish constructor placed on p (inlined in the exe).
static void finishLoadedObjecthkpConvexTranslateShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpConvexTranslateShape(flag);
	}
}

void hkpConvexTranslateShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	collector->addReferencedObject("Child", m_childShape.m_childShape, 0);
	hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

float hkpConvexTranslateShape::getMaximumProjection(const hkVector4& direction) const {
	float v6 = m_childShape.m_childShape->getMaximumProjection(direction);
	__m128 v3 = _mm_mul_ps(m_translation.m_quad, direction.m_quad);
	return static_cast<float>(_mm_shuffle_ps(v3, v3, 170)[0] +
			   static_cast<float>(_mm_shuffle_ps(v3, v3, 85)[0] + v3[0])) +
		v6;
}

const hkpShapeContainer* hkpConvexTranslateShape::getContainer() const {
	return &m_childShape;
}

// The child's size, given the buffer left after this shape. A child stored right after this shape counts in the
// returned size and caches 0; any other child is fetched separately, so its size is cached and only this shape's
// size is returned.
int32_t hkpConvexTranslateShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input,
	int32_t spuBufferSizeLeft) const {
	const int32_t maxChildSize = spuBufferSizeLeft - static_cast<int32_t>(sizeof(*this));
	const int32_t childSize = m_childShape.m_childShape->calcSizeForSpu(input, maxChildSize);
	if (childSize < 0 || childSize > maxChildSize) {
		return -1;
	}
	if (m_childShape.m_childShape == this + 1) {
		m_childShapeSize = 0;
		return childSize + static_cast<int32_t>(sizeof(*this));
	}
	m_childShapeSize = childSize;
	return static_cast<int32_t>(sizeof(*this));
}

// The child's AABB under localToWorld, moved by the translation rotated into world space. The exe rotates with
// shufps/mulps/addps: (t.y * col1 + t.x * col0) + t.z * col2 on all four lanes, then adds that to both corners.
void hkpConvexTranslateShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	const hkpShape* pChild = m_childShape.m_childShape;
	pChild->getAabbImpl(localToWorld, tolerance, out);

	const hkQuadReal kT = m_translation.m_quad;
	const hkQuadReal kOffset =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kT, kT, 0x55), localToWorld.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(kT, kT, 0x00), localToWorld.m_rotation.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(kT, kT, 0xAA), localToWorld.m_rotation.m_col2.m_quad));
	out.m_min.m_quad = _mm_add_ps(out.m_min.m_quad, kOffset);
	out.m_max.m_quad = _mm_add_ps(out.m_max.m_quad, kOffset);
}

// The ray moved into the child's space, cast against the child one level down the shape key hierarchy. A hit's key
// at this level is 0, the single child's.
hkBool hkpConvexTranslateShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
	HK_TIMER_BEGIN("rcConvTransl", nullptr);
	hkpShapeRayCastInput subInput = input;
	subInput.m_from.m_quad = _mm_sub_ps(input.m_from.m_quad, m_translation.m_quad);
	subInput.m_to.m_quad = _mm_sub_ps(input.m_to.m_quad, m_translation.m_quad);
	++results.m_shapeKeyIndex;
	hkBool hit = m_childShape.m_childShape->castRayImpl(subInput, results);
	--results.m_shapeKeyIndex;
	if (hit.m_bool) {
		results.m_shapeKeys[results.m_shapeKeyIndex] = 0;
	}
	HK_TIMER_END();
	return hit;
}

// The ray moved into the child's space, cast against the child as a body under body whose transform is body's
// with this shape's translation applied first.
void hkpConvexTranslateShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	hkpShapeRayCastInput subInput = input;
	subInput.m_from.m_quad = _mm_sub_ps(input.m_from.m_quad, m_translation.m_quad);
	subInput.m_to.m_quad = _mm_sub_ps(input.m_to.m_quad, m_translation.m_quad);
	// The exe builds the identity rotation inline: each column zeroed, then its diagonal 1 stored from fld1.
	hkTransform translation;
	translation.m_rotation.m_col0.m_quad = _mm_setzero_ps();
	translation.m_rotation.m_col0.m_quad[0] = 1.0f;
	translation.m_rotation.m_col1.m_quad = _mm_setzero_ps();
	translation.m_rotation.m_col1.m_quad[1] = 1.0f;
	translation.m_rotation.m_col2.m_quad = _mm_setzero_ps();
	translation.m_rotation.m_col2.m_quad[2] = 1.0f;
	translation.m_translation.m_quad = m_translation.m_quad;
	hkTransform childToWorld;
	childToWorld.setMul(*cdBody.m_motion.pMotion, translation);
	const hkpShape* pChild = m_childShape.m_childShape;
	hkpCdBody childBody;
	childBody.m_motion.pMotion = &childToWorld;
	childBody.m_parent = &cdBody;
	childBody.m_shape = pChild;
	childBody.m_shapeKey = 0;
	pChild->castRayWithCollectorImpl(subInput, childBody, collector);
}

int32_t hkpConvexTranslateShape::getNumCollisionSpheresImpl() const {
	return getChildShape()->getNumCollisionSpheresImpl();
}

// The child's spheres, moved by the translation into sphereBuffer. The exe asks for the spheres first, then for
// their count.
const hkSphere* hkpConvexTranslateShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
	const hkSphere* pChildSpheres = getChildShape()->getCollisionSpheresImpl(sphereBuffer);
	int32_t numSpheres = getChildShape()->getNumCollisionSpheresImpl();
	hkSphere* pOut = sphereBuffer;
	for (; numSpheres > 0; --numSpheres) {
		pOut->m_pos.m_quad = _mm_add_ps(pChildSpheres->m_pos.m_quad, m_translation.m_quad);
		++pChildSpheres;
		++pOut;
	}
	return sphereBuffer;
}

void hkpConvexTranslateShape::getFirstVertex(hkVector4& v) const {
	getChildShape()->getFirstVertex(v);
	v.m_quad = _mm_add_ps(m_translation.m_quad, v.m_quad);
}

void hkpConvexTranslateShape::getSupportingVertexImpl(const hkVector4& direction,
	hkpCdVertex& supportingVertexOut) const {
	getChildShape()->getSupportingVertexImpl(direction, supportingVertexOut);
	supportingVertexOut.m_quad = _mm_add_ps(m_translation.m_quad, supportingVertexOut.m_quad);
}

void hkpConvexTranslateShape::convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds,
	hkpCdVertex* verticesOut) const {
	getChildShape()->convertVertexIdsToVerticesImpl(ids, numIds, verticesOut);
	if (numIds > 0) {
		hkpCdVertex* v5 = verticesOut;
		int32_t v6 = numIds;
		do {
			v5->m_quad = _mm_add_ps(m_translation.m_quad, v5->m_quad);
			++v5;
			--v6;
		} while (v6);
	}
}

void hkpConvexTranslateShape::getCentreImpl(hkVector4& centreOut) const {
	getChildShape()->getCentre(centreOut);
	centreOut.m_quad = _mm_add_ps(m_translation.m_quad, centreOut.m_quad);
}

const hkClass& hkpConvexTranslateShape::staticClass() {
	return hkpConvexTranslateShapeClass;
}
