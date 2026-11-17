#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <xmmintrin.h>

// hkpConvexShape's reflection records: WeldResult's values, the enum and the member.
using hkpConvexShapeWeldResultEnumItemsArray = const hkClassEnum::Item[3];
static hkpConvexShapeWeldResultEnumItemsArray hkpConvexShapeWeldResultEnumItems{{0, "WELD_RESULT_REJECT_CONTACT_POINT"},
	{1, "WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED"}, {2, "WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED"}};
using hkpConvexShapeEnumsArray = const hkClassEnum[1];
static hkpConvexShapeEnumsArray hkpConvexShapeEnums{{"WeldResult", hkpConvexShapeWeldResultEnumItems, 3, nullptr, {0}}};
using hkpConvexShapeClass_MembersArray = const hkClassMember[1];
static hkpConvexShapeClass_MembersArray hkpConvexShapeClass_Members{
	{"radius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr}};
hkClass hkpConvexShapeClass("hkpConvexShape", &hkpSphereRepShapeClass, 0x14, nullptr, 0, hkpConvexShapeEnums, 1,
	hkpConvexShapeClass_Members, 1, nullptr, nullptr, 0, 0);

float hkpConvexShape::getMaximumProjection(const hkVector4& direction) const {
	hkpCdVertex v7;
	getSupportingVertexImpl(direction, v7);
	__m128 v3 = _mm_mul_ps(direction.m_quad, v7.m_quad);
	return m_radius +
		static_cast<float>(_mm_shuffle_ps(v3, v3, 170)[0] + static_cast<float>(_mm_shuffle_ps(v3, v3, 85)[0] + v3[0]));
}

// KEEP (partcopy:): the output handed to collector.addRayHit has m_pad (bytes 0x18-0x1F) and m_shapeKeys[1..7]
// written by nothing here nor by castRayImpl, and the collectors copy the whole collector output, m_pad included
// (hkpAllRayHitCollector::addRayHit, the inlined 0x9C1B00): stack garbage reaches the collector's hits, as in
// hkpPlaneShape's and hkpMultiSphereShape's castRayWithCollectorImpl (0xD54F90, 0xD2ABE0).
// The SDK's body: the output's constructor inlined with the collector's early-out fraction as the hit fraction (copied
// with fld/fstp, 0xC9E2AD), castRayImpl through the vtable, the normal turned into world space (the inlined 0xC7F700)
// by the body's rotation, then the hit reported.
void hkpConvexShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& inputLocal, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	hkpShapeRayCastOutput output;
	// EXE-INLINED 0x4A3D20
	output.m_hitFraction = collector.m_earlyOutHitFraction;
	output.m_extraInfo = -1;
	output.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
	output.m_shapeKeyIndex = 0;
	if (castRayImpl(inputLocal, output).m_bool) {
		// EXE-INLINED 0xC7F700
		const hkRotation& rRotation = cdBody.m_motion.pState->m_transform.m_rotation;
		const __m128 kNormal = output.m_normal.m_quad;
		output.m_normal.m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x55), rRotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x00), rRotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xAA), rRotation.m_col2.m_quad));
		collector.addRayHit(cdBody, output);
	}
}

// The base shape does no welding: the contact point stands as it is.
int32_t hkpConvexShape::weldContactPointImpl(uint16_t*, uint8_t&, hkVector4&, const hkTransform*, const hkpConvexShape*,
	const hkTransform*, hkVector4&) const {
	return WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
}

void hkpConvexShape::getCentreImpl(hkVector4& centreOut) const {
	hkVector4 v2;
	hkAabb v3;
	getAabbImpl(hkTransformIdentity, 0.0f, v3);
	v2.m_quad = _mm_add_ps(v3.m_max.m_quad, v3.m_min.m_quad);
	centreOut.m_quad = v2.m_quad;
	centreOut.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0), v2.m_quad);
}

// hk4dGskCollideCalcToi's closest feature step, as that body writes it.
const hkpConvexShape* getClosestPointAndNormal(const hkpConvexShape* a1, const hkpConvexShape* a2, hkVector4* a3,
	hkVector4* a4, hkpGsk* r_edi, const hkTransform* r_esi, const hkTransform* r_eax) {
	hkTransform aTb;
	aTb.setMulInverseMul(*r_esi, *r_eax);
	hkVector4 sepOut;
	r_edi->getClosestFeature(a1, a2, aTb, sepOut);
	// EXE-INLINED 0xC7F700
	const hkRotation& rWa = r_esi->getRotation();
	const hkQuadReal s = sepOut.m_quad;
	a3->m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), rWa.m_col1.m_quad),
								_mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), rWa.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), rWa.m_col2.m_quad));
	// The point on A of the simplex: A's vertex, else B's vertex moved back by the distance, else the closest point
	// of the two edges.
	hkVector4 pointOnA;
	if (r_edi->m_dimA.m_storage == 1) {
		pointOnA = r_edi->m_verticesA[0];
	} else if (r_edi->m_dimB.m_storage == 1) {
		const hkQuadReal d = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
		pointOnA.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, _MM_SHUFFLE(0, 0, 0, 0)), s), r_edi->m_verticesBinA[0].m_quad);
	} else {
		pointOnA = r_edi->m_closestLineLineResult.m_closestPointA;
	}
	// EXE-INLINED 0xC7F690
	const hkQuadReal w = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
	hkVector4 pointBinA;
	pointBinA.m_quad =
		_mm_sub_ps(_mm_sub_ps(pointOnA.m_quad, _mm_mul_ps(_mm_shuffle_ps(w, w, _MM_SHUFFLE(0, 0, 0, 0)), s)),
			aTb.m_translation.m_quad);
	a4->_setRotatedInverseDir(aTb.getRotation(), pointBinA);
	a3->m_quad[3] = sepOut.m_quad[3] - a1->m_radius - a2->m_radius;
	return a1;
}
