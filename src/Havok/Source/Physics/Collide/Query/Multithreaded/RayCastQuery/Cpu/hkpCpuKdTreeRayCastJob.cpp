#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuKdTreeRayCastJob.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

#include <xmmintrin.h>

hkKdTreeCpuBroadphaseBundleCastCollector::hkKdTreeCpuBroadphaseBundleCastCollector() : m_command(nullptr) {
	m_workInput.m_filterInfo = 0;
	m_workInput.m_rayShapeCollectionFilter = nullptr;
	for (int32_t i = 0; i < 4; i++) {
		m_numHits[i] = 0;
	}
}

hkKdTreeCpuBroadphaseBundleCastCollector::~hkKdTreeCpuBroadphaseBundleCastCollector() {}

// The primitive id is the collidable's address. Unless it has no shape or the filter rejects it for the first ray, the
// four rays' ends are taken into the collidable's space and stored transposed in m_workInput, the bundle is cast
// against the shape, and each ray's hit closer than its result so far is copied into the command's results, its normal
// turned back into world space. The collidable's transform is read before the filter call and kept across the call.
// Only the collidable is stored as each result's m_rootCollidable.
void hkKdTreeCpuBroadphaseBundleCastCollector::addMidPhaseHit(hkPrimitiveId primId, const hkVector4Comparison& mask) {
	hkpCollidable* pCollidable = reinterpret_cast<hkpCollidable*>(primId);
	const hkpShape* pShape = pCollidable->m_shape;
	if (!pShape) {
		return;
	}
	const hkTransform& rTransform = *pCollidable->m_motion.pMotion;
	if (m_filter &&
		!static_cast<const hkpRayCollidableFilter*>(m_filter)
			->isCollisionEnabled(*m_originalInput, *pCollidable)
			.m_bool) {
		return;
	}

	hkVector4 kFrom[4];
	hkVector4 kTo[4];
	for (int32_t i = 0; i < 4; ++i) {
		hkVector4 kRelative;
		// EXE-INLINED 0xC7F690
		kRelative.m_quad = _mm_sub_ps(m_originalInput[i].m_from.m_quad, rTransform.m_translation.m_quad);
		kFrom[i]._setRotatedInverseDir(rTransform.m_rotation, kRelative);
		// EXE-INLINED 0xC7F690
		kRelative.m_quad = _mm_sub_ps(m_originalInput[i].m_to.m_quad, rTransform.m_translation.m_quad);
		kTo[i]._setRotatedInverseDir(rTransform.m_rotation, kRelative);
	}
	// Coordinate c of the four ends in m_from[c] and m_to[c]; the w lanes are dropped.
	hkQuadReal kFrom0 = kFrom[0].m_quad, kFrom1 = kFrom[1].m_quad, kFrom2 = kFrom[2].m_quad, kFrom3 = kFrom[3].m_quad;
	_MM_TRANSPOSE4_PS(kFrom0, kFrom1, kFrom2, kFrom3);
	m_workInput.m_from[0].m_quad = kFrom0;
	m_workInput.m_from[1].m_quad = kFrom1;
	m_workInput.m_from[2].m_quad = kFrom2;
	hkQuadReal kTo0 = kTo[0].m_quad, kTo1 = kTo[1].m_quad, kTo2 = kTo[2].m_quad, kTo3 = kTo[3].m_quad;
	_MM_TRANSPOSE4_PS(kTo0, kTo1, kTo2, kTo3);
	m_workInput.m_to[0].m_quad = kTo0;
	m_workInput.m_to[1].m_quad = kTo1;
	m_workInput.m_to[2].m_quad = kTo2;

	// The lanes the shape hit: unused.
	hkVector4Comparison kHitLanes;
	hkpShapeRayBundleCastOutput kOutput;
	kOutput.reset();
	pShape->castRayBundleImpl(&kHitLanes, m_workInput, kOutput, mask);

	int32_t* pNumHits = m_numHits;
	for (int32_t i = 0; i < 4; ++i, ++pNumHits) {
		const hkpShapeRayCastOutput& rOutput = kOutput.m_outputs[i];
		if (rOutput.m_hitFraction < 1.0f &&
			(!*pNumHits || m_command->m_results[i].m_hitFraction > rOutput.m_hitFraction)) {
			hkpWorldRayCastOutput& rResult = m_command->m_results[i];
			// EXE-INLINED 0x9C1A70
			rResult.hkpShapeRayCastOutput::operator=(rOutput);
			m_command->m_results[i].m_rootCollidable = pCollidable;
			const hkQuadReal kNormal = rOutput.m_normal.m_quad;
			// EXE-INLINED 0xC7F700
			m_command->m_results[i].m_normal.m_quad =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x00),
										  rTransform.m_rotation.m_col0.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x55), rTransform.m_rotation.m_col1.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xAA), rTransform.m_rotation.m_col2.m_quad));
			hkKdTreeRayCastBundleCommand* pCommand = m_command;
			const float fFraction = pCommand->m_stopAfterFirstHit.m_bool ? 0.0f : pCommand->m_results[i].m_hitFraction;
			pCommand->m_results[i].m_hitFraction = fFraction;
			*pNumHits = 1;
		}
	}
}

// The four rays' hit fractions, taken from the command's results. Without a command the vector is left untouched.
void hkKdTreeCpuBroadphaseBundleCastCollector::getHitFractions(hkVector4& hitFractions) const {
	if (m_command) {
		hitFractions.m_quad[0] = m_command->m_results[0].m_hitFraction;
		hitFractions.m_quad[1] = m_command->m_results[1].m_hitFraction;
		hitFractions.m_quad[2] = m_command->m_results[2].m_hitFraction;
		hitFractions.m_quad[3] = m_command->m_results[3].m_hitFraction;
	}
}

void hkKdTreeCpuBroadphaseBundleCastCollector::operator delete(void* apMemory) {
	if (apMemory) {
		hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkKdTreeCpuBroadphaseBundleCastCollector));
	}
}
