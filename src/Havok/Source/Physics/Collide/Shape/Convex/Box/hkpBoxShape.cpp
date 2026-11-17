#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

#include <bit>
#include <xmmintrin.h>

static void finishLoadedObjecthkpBoxShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpBoxShape();
static void cleanupLoadedObjecthkpBoxShape(void* p);

// The class record's member table.
using hkpBoxShapeClassMemberArray = const hkClassMember[1];
static hkpBoxShapeClassMemberArray hkpBoxShapeClass_Members{
	{"halfExtents", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr}};
hkClass hkpBoxShapeClass("hkpBoxShape", &hkpConvexShapeClass, 0x30, nullptr, 0, nullptr, 0, hkpBoxShapeClass_Members, 1,
	nullptr, nullptr, 0, 0);
hkTypeInfo hkpBoxShapeTypeInfo{"hkpBoxShape", finishLoadedObjecthkpBoxShape, cleanupLoadedObjecthkpBoxShape,
	getVtablehkpBoxShape(), &typeid(hkpBoxShape), sizeof(hkpBoxShape)};

// Runs the finish constructor hkpBoxShape(flag) in place.
static void finishLoadedObjecthkpBoxShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpBoxShape(kFlag);
	}
}

static const void* getVtablehkpBoxShape() {
	return hkGetVtableOf<hkpBoxShape>();
}

static void cleanupLoadedObjecthkpBoxShape(void* p) {
	static_cast<hkpBoxShape*>(p)->~hkpBoxShape();
}

// The half extents' w holds the smallest of the three.
hkpBoxShape::hkpBoxShape(const hkVector4& a1, float a2) : hkpConvexShape(HK_SHAPE_BOX, a2), m_halfExtents(a1) {
	float fMin = m_halfExtents.m_quad[1] > m_halfExtents.m_quad[0] ? m_halfExtents.m_quad[0] : m_halfExtents.m_quad[1];
	if (m_halfExtents.m_quad[2] < fMin) {
		fMin = m_halfExtents.m_quad[2];
	}
	m_halfExtents.m_quad[3] = fMin;
}

hkpBoxShape::~hkpBoxShape() {}

void hkpBoxShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	hkReferencedObject::calcContentStatistics(collector, &hkpConvexShapeClass);
}

int32_t hkpBoxShape::calcSizeForSpu(const CalcSizeForSpuInput& a1, int32_t a2) const {
	return 48;
}

void hkpBoxShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	float extraRadius = m_radius + tolerance;
	hkAabbUtil::calcAabb(localToWorld, m_halfExtents, extraRadius, out);
}

// The hit normal is zeroed only in the entering branch (an entering fraction not below the entry one). With finite ray
// ends that branch runs before a hit, but a non-finite m_from/m_to lane makes the fraction NaN, which skips it, and the
// never-written normal is then copied into output.m_normal and a hit is returned, as the game does (an uninitialised
// read). The slab test: the extents (half extents plus the radius) and their negation give per-axis outside masks for
// both ray ends. Two passes over the axes z, y, x: first the max faces (sign 1.0f), then the min faces (sign -1.0f).
// The distances, the fraction, the entry fraction and the sign are floats. An entering fraction replaces the entry one
// when not below it (NaN skips); an exiting fraction replaces the exit one unless above it (NaN replaces); the ray
// misses when the exit fraction is below the entry one (NaN goes on). The timer macros write into the stream's raw
// buffer (hence the casts), as in hkpCapsuleCapsuleAgent.
hkBool hkpBoxShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
	HK_TIMER_BEGIN("rcBox", nullptr);
	const __m128 kRadius = _mm_load_ss(&m_radius);
	const __m128 kExtents = _mm_add_ps(_mm_shuffle_ps(kRadius, kRadius, 0),
		_mm_load_ps(&reinterpret_cast<const float*>(&m_halfExtents.m_quad)[0]));
	// EXE-INLINED none: hkVector4::setNeg4
	const __m128 kNegExtents = _mm_xor_ps(kExtents, _mm_load_ps(&reinterpret_cast<float*>(&hkQuadRealNegateMask)[0]));
	alignas(16) hkVector4 negExtents;
	_mm_store_ps(&reinterpret_cast<float*>(&negExtents.m_quad)[0], kNegExtents);
	const __m128 kFrom = _mm_load_ps(&reinterpret_cast<const float*>(&input.m_from.m_quad)[0]);
	const __m128 kTo = _mm_load_ps(&reinterpret_cast<const float*>(&input.m_to.m_quad)[0]);
	const int32_t iFromMax = _mm_movemask_ps(_mm_cmple_ps(kExtents, kFrom)) & 7;
	const int32_t iFromMin = _mm_movemask_ps(_mm_cmple_ps(kFrom, kNegExtents)) & 7;
	const int32_t iToMax = _mm_movemask_ps(_mm_cmple_ps(kExtents, kTo)) & 7;
	const int32_t iToMin = _mm_movemask_ps(_mm_cmple_ps(kTo, kNegExtents)) & 7;

	// The result byte is written only after HK_TIMER_END.
	bool bHit = false;
	if ((iFromMax & iToMax) == 0 && (iFromMin & iToMin) == 0 && (iFromMin | iFromMax) != 0) {
		float fMin = 0.0f;
		float fMax = results.m_hitFraction;
		float fSign = 1.0f;
		const int32_t aiAxisBits[3] = {1, 2, 4};
		alignas(16) hkVector4 normal;
		int32_t iTested = iToMax | iFromMax;
		int32_t iEntering = iFromMax;
		bool bMiss = false;
		for (int32_t iPass = 1; iPass >= 0 && !bMiss; iPass--) {
			for (int32_t i = 2; i >= 0; i--) {
				const int32_t iBit = aiAxisBits[i];
				if ((iTested & iBit) == 0) {
					continue;
				}
				const float fNegExtent = (&reinterpret_cast<float*>(&negExtents.m_quad)[0])[i];
				const float fDistFrom =
					(&reinterpret_cast<const float*>(&input.m_from.m_quad)[0])[i] * fSign + fNegExtent;
				const float fDistTo = (&reinterpret_cast<const float*>(&input.m_to.m_quad)[0])[i] * fSign + fNegExtent;
				const float fFraction = fDistFrom / (fDistFrom - fDistTo);
				if ((iEntering & iBit) != 0) {
					if (fFraction >= fMin) {
						fMin = fFraction;
						_mm_store_ps(&reinterpret_cast<float*>(&normal.m_quad)[0], _mm_setzero_ps());
						(&reinterpret_cast<float*>(&normal.m_quad)[0])[i] = fSign;
					}
				} else if (!(fFraction > fMax)) {
					fMax = fFraction;
				}
				if (fMax < fMin) {
					bMiss = true;
					break;
				}
			}
			iTested = iToMin | iFromMin;
			iEntering = iFromMin;
			fSign = -1.0f;
		}
		if (!bMiss) {
			results.m_hitFraction = fMin;
			_mm_store_ps(&reinterpret_cast<float*>(&results.m_normal.m_quad)[0],
				_mm_load_ps(&reinterpret_cast<float*>(&normal.m_quad)[0]));
			results.m_shapeKeys[results.m_shapeKeyIndex] = 0xFFFFFFFF;
			bHit = true;
		}
	}
	HK_TIMER_END();
	hkBool result;
	result.m_bool = bHit;
	return result;
}

int32_t hkpBoxShape::getNumCollisionSpheresImpl() const {
	return 8;
}

// One sphere of radius m_radius on each of the box's eight corners: the half extents with the radius in w, times each
// vertex sign.
const hkSphere* hkpBoxShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
	hkVector4 extentsAndRadius;
	extentsAndRadius.m_quad = m_halfExtents.m_quad;
	extentsAndRadius.m_quad[3] = m_radius;
	for (int32_t i = 0; i < 8; i += 2) {
		sphereBuffer[i].m_pos.m_quad = _mm_mul_ps(hkpBoxShapeVertexSigns[i], extentsAndRadius.m_quad);
		sphereBuffer[i + 1].m_pos.m_quad = _mm_mul_ps(hkpBoxShapeVertexSigns[i + 1], extentsAndRadius.m_quad);
	}
	return sphereBuffer;
}

void hkpBoxShape::getFirstVertex(hkVector4& v) const {
	v = m_halfExtents;
}

void hkpBoxShape::getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const {
	hkpCdVertex v3;
	v3.m_quad = _mm_xor_ps(_mm_and_ps(direction.m_quad, hkQuadRealNegateMask), m_halfExtents.m_quad);
	v3.m_quad[3] =
		std::bit_cast<float>(static_cast<int32_t>(((_mm_movemask_ps(_mm_cmplt_ps(v3.m_quad, _mm_setzero_ps())) & 7) |
			0x3F000000)));
	supportingVertexOut.m_quad = v3.m_quad;
}

void hkpBoxShape::convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const {
	for (int32_t i = numIds - 1; i >= 0; --i) {
		int32_t v7 = *ids | 0x3F000000;
		verticesOut->m_quad = _mm_mul_ps(hkpBoxShapeVertexSigns[*ids], m_halfExtents.m_quad);
		verticesOut->m_quad[3] = std::bit_cast<float>(static_cast<int32_t>(v7));
		++verticesOut;
		++ids;
	}
}

void hkpBoxShape::getCentreImpl(hkVector4& centreOut) const {
	centreOut.m_quad = _mm_setzero_ps();
}

void hkpBoxShape::setHalfExtents(const hkVector4& halfExtents) {
	double v2;
	m_halfExtents.m_quad = halfExtents.m_quad;
	if (!(m_halfExtents.m_quad[1] > static_cast<double>(m_halfExtents.m_quad[0]))) {
		v2 = m_halfExtents.m_quad[1];
	} else {
		v2 = m_halfExtents.m_quad[0];
	}
	float v4 = v2;
	double v3 = v4;
	m_halfExtents.m_quad[3] = v4;
	if (m_halfExtents.m_quad[2] < static_cast<double>(v4)) {
		v3 = m_halfExtents.m_quad[2];
	}
	float v5 = v3;
	m_halfExtents.m_quad[3] = v5;
}

const hkClass& hkpBoxShape::staticClass() {
	return hkpBoxShapeClass;
}
