#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGsk.h"

#include <cmath>
#include <xmmintrin.h>

// Both bodies' frames at afTime (a fraction of the step) and B's plane, the velocities and the mass centres in A's
// space.
void hk4dGskCollideInitMatrices(const hkp4dGskVertexCollidePointsInput& input, float Rt, hk4dGskCollideMatrices& out) {
	const hkStepInfo* pStepInfo = input.m_stepInfo.m_storage;
	float fTAddOn = pStepInfo->m_deltaTime * Rt;
	hkSweptTransformUtil::lerp2Ha(input.m_motionA.m_storage->m_sweptTransform, pStepInfo->m_startTime, fTAddOn,
		out.m_wTa);
	hkSweptTransformUtil::lerp2Ha(input.m_motionB.m_storage->m_sweptTransform, pStepInfo->m_startTime, fTAddOn,
		out.m_wTb);
	out.m_aTb.setMulInverseMul(out.m_wTa, out.m_wTb);
	out.m_plane.setRotatedDir(out.m_aTb.getRotation(), input.m_planeB);
	out.m_pointOnPlane.setTransformedPos(out.m_aTb, input.m_pointOnPlaneB);
	out.m_plane.m_quad[3] = -input.m_radiusSum.m_storage;
	out.m_massCenterAinA.m_quad = input.m_motionA.m_storage->m_sweptTransform.m_centerOfMassLocal.m_quad;

	// hkVector4Util::rotateInversePoints on both delta angles: A's rotation transposed (HK_TRANSPOSE3), then x * row 0
	// added to y * row 1, plus z * row 2.
	// EXE-INLINED none: hkVector4Util::rotateInversePoints
	const hkRotation& rWa = out.m_wTa.getRotation();
	hkQuadReal c0 = rWa.m_col0.m_quad;
	hkQuadReal c1 = rWa.m_col1.m_quad;
	hkQuadReal c2 = rWa.m_col2.m_quad;
	hkQuadReal lo = _mm_unpacklo_ps(c0, c1);
	hkQuadReal hi = _mm_unpackhi_ps(c0, c1);
	hkQuadReal row0 = _mm_movelh_ps(lo, c2);
	hkQuadReal row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), c2, _MM_SHUFFLE(3, 1, 1, 0));
	hkQuadReal row2 = _mm_shuffle_ps(hi, c2, _MM_SHUFFLE(3, 2, 1, 0));
	for (int32_t i = 0; i < 2; ++i) {
		hkQuadReal b = input.m_deltaAngles[i].m_quad;
		out.m_angularVel[i].m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 1, 1, 1)), row1),
						   _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 0, 0)), row0)),
				_mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 2, 2, 2)), row2));
	}

	out.m_massCenterBinA.setTransformedPos(out.m_aTb, input.m_motionB.m_storage->m_sweptTransform.m_centerOfMassLocal);
	out.m_linearVel.setRotatedInverseDir(out.m_wTa.getRotation(), input.m_linearTimInfo);
}

// Refines the time at which arPoint reaches the separation plane between afT0 (distance afDist0) and afT1
// (afDist1): up to ten regula falsi steps, the even ones with the fraction clamped to [0.1, 0.9], until the distance
// is within a quarter of the TOI accuracy of the TOI separation. Records the time and the point in arOut when it is
// earlier than arOut's, and returns arOut's time. afUnused is never read.
float hk4dGskCollidePointWithPlaneFinal(const hkp4dGskVertexCollidePointsInput& input, const hkp4dGskTolerances& tol,
	const hkVector4& vertexA, float startRt, float endRt, float distance0, float distance1, float velocity,
	hkp4dGskVertexCollidePointsOutput& out) {
	float fTime = startRt;
	int32_t i = 0;
	float fAccuracy = tol.m_toiAccuracy.m_storage * 0.25;
	do {
		alignas(16) hk4dGskCollideMatrices kMatrices;
		if (tol.m_toiSeparation.m_storage >= distance0) {
			break;
		}
		float fDeltaDist = distance1 - distance0;
		if (-fAccuracy < fDeltaDist) {
			break;
		}
		float fFraction = (tol.m_toiSeparation.m_storage - distance0) / fDeltaDist;
		if ((i & 1) == 0) {
			if (!(fFraction > 0.10000000149011612)) {
				fFraction = 0.1f;
			}
			if (!(fFraction < 0.8999999761581421)) {
				fFraction = 0.9f;
			}
		}
		fTime = fFraction * endRt + (1.0 - fFraction) * startRt;
		hk4dGskCollideInitMatrices(input, fTime, kMatrices);

		// EXE-INLINED none: hkVector4::dot4xyz1 (the plane's dot product with the point's offset, plus its w)
		hkQuadReal offset =
			_mm_mul_ps(_mm_sub_ps(vertexA.m_quad, kMatrices.m_pointOnPlane.m_quad), kMatrices.m_plane.m_quad);
		hkQuadReal zw = _mm_add_ss(_mm_shuffle_ps(offset, offset, _MM_SHUFFLE(2, 2, 2, 2)),
			_mm_shuffle_ps(kMatrices.m_plane.m_quad, kMatrices.m_plane.m_quad, _MM_SHUFFLE(3, 3, 3, 3)));
		hkQuadReal xy = _mm_add_ss(_mm_shuffle_ps(offset, offset, _MM_SHUFFLE(1, 1, 1, 1)), offset);
		float fDist = _mm_cvtss_f32(_mm_add_ps(zw, xy));

		float fError = fDist - tol.m_toiSeparation.m_storage;
		float fAbsError = std::fabs(fError);
		if (fAccuracy > fAbsError) {
			break;
		}
		if (!(tol.m_toiSeparation.m_storage > fDist)) {
			startRt = fTime;
			distance0 = fDist;
		} else {
			endRt = fTime;
			distance1 = fDist;
		}
		++i;
	} while (i < 10);
	if (!(out.m_Rtoi.m_storage > fTime)) {
		return out.m_Rtoi.m_storage;
	}
	out.m_Rtoi.m_storage = fTime;
	out.m_collidingPoint.m_quad = vertexA.m_quad;
	return out.m_Rtoi.m_storage;
}

// Sweeps A's vertices against B's plane over [m_startRt, 1]: each round sets up the frames at the round's start time,
// moves every vertex that can still reach the TOI separation within the round to the front of the array (its w set to
// its distance), records an already penetrating vertex in arOut (refined by hk4dGskCollidePointWithPlaneFinal unless
// the round starts at m_startRt) and advances the start time by the smallest safe step; the next round checks only the
// vertices moved to the front.
void hk4dGskCollidePointsWithPlane(const hkp4dGskVertexCollidePointsInput& input, const hkp4dGskTolerances& tol,
	hkp4dGskVertexCollidePointsOutput& out) {
	float fRtEnd = 1.0f;
	float fRt = input.m_startRt.m_storage;
	int32_t iNumToCheck = input.m_numVertices.m_storage;
	float fLastRt = fRt;
	if (!iNumToCheck) {
		return;
	}
	while (true) {
		alignas(16) hk4dGskCollideMatrices kMatrices;
		hk4dGskCollideInitMatrices(input, fRt, kMatrices);
		hkVector4* pVertex = input.m_verticesA.m_storage;
		hkVector4* pFront = pVertex;
		const float fDeltaRt = fRtEnd - fRt;
		int32_t i = iNumToCheck - 1;
		iNumToCheck = 0;
		const float fMaxApproach = input.m_worstCaseApproachingDelta.m_storage * fDeltaRt;
		float fBestRt = fRtEnd;
		for (; i >= 0; --i, ++pVertex) {
			const hkQuadReal v = pVertex->m_quad;
			const hkQuadReal plane = kMatrices.m_plane.m_quad;

			// EXE-INLINED none: hkVector4::dot4xyz1 (the plane's dot product with the vertex's offset, plus its w)
			const hkQuadReal offset = _mm_mul_ps(_mm_sub_ps(v, kMatrices.m_pointOnPlane.m_quad), plane);
			const hkQuadReal zw = _mm_add_ss(_mm_shuffle_ps(offset, offset, _MM_SHUFFLE(2, 2, 2, 2)),
				_mm_shuffle_ps(plane, plane, _MM_SHUFFLE(3, 3, 3, 3)));
			const hkQuadReal xy = _mm_add_ss(_mm_shuffle_ps(offset, offset, _MM_SHUFFLE(1, 1, 1, 1)), offset);
			const float fDist = _mm_cvtss_f32(_mm_add_ps(zw, xy));
			if (tol.m_toiSeparation.m_storage + fMaxApproach < fDist) {
				continue;
			}

			const hkQuadReal rA = _mm_sub_ps(v, kMatrices.m_massCenterAinA.m_quad);
			const hkQuadReal rB = _mm_sub_ps(v, kMatrices.m_massCenterBinA.m_quad);
			const float fDistToSep = fDist - tol.m_toiSeparation.m_storage;
			// The vertex's velocity relative to B, projected on the plane: angular[0] x rA - linear
			// - angular[1] x rB, dotted with the plane's normal.
			const hkQuadReal ang0 = kMatrices.m_angularVel[0].m_quad;
			const hkQuadReal ang1 = kMatrices.m_angularVel[1].m_quad;
			// EXE-INLINED 0x630DF0
			const hkQuadReal crossA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(ang0, ang0, _MM_SHUFFLE(3, 0, 2, 1)),
													 _mm_shuffle_ps(rA, rA, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(_mm_shuffle_ps(ang0, ang0, _MM_SHUFFLE(3, 1, 0, 2)),
					_mm_shuffle_ps(rA, rA, _MM_SHUFFLE(3, 0, 2, 1))));
			// EXE-INLINED 0x630DF0
			const hkQuadReal crossB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(ang1, ang1, _MM_SHUFFLE(3, 0, 2, 1)),
													 _mm_shuffle_ps(rB, rB, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(_mm_shuffle_ps(ang1, ang1, _MM_SHUFFLE(3, 1, 0, 2)),
					_mm_shuffle_ps(rB, rB, _MM_SHUFFLE(3, 0, 2, 1))));
			const hkQuadReal vel =
				_mm_mul_ps(_mm_sub_ps(_mm_sub_ps(crossA, kMatrices.m_linearVel.m_quad), crossB), plane);
			// EXE-INLINED 0x62B0A0
			const float fProjVel = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(vel, vel, _MM_SHUFFLE(2, 2, 2, 2)),
				_mm_add_ss(_mm_shuffle_ps(vel, vel, _MM_SHUFFLE(1, 1, 1, 1)), vel)));

			const float fDistAtEnd = fDistToSep + fProjVel * fDeltaRt;
			const float fMaxDrop = fDeltaRt * 0.5 * fDeltaRt * input.m_maxAccel.m_storage;
			if (fMaxDrop < fDistAtEnd) {
				continue;
			}

			if (tol.m_toiSeparation.m_storage > fDist) {
				// Already within the TOI separation at the round's start.
				if (input.m_startRt.m_storage == fRt) {
					out.m_Rtoi.m_storage = fRt;
					fRtEnd = fRt;
					out.m_collidingPoint = *pVertex;
				} else {
					fRtEnd = hk4dGskCollidePointWithPlaneFinal(input, tol, *pVertex, fLastRt, fRt, (*pVertex)(3), fDist,
						fProjVel, out);
				}
				continue;
			}

			// The vertex may reach the plane: keep it for the next round, with its distance in w.
			hkVector4 kSwap = *pVertex;
			*pVertex = *pFront;
			*pFront = kSwap;
			(*pFront)(3) = fDist;
			++iNumToCheck;
			++pFront;

			const float fDistToMin = fDist - tol.m_minSeparation.m_storage;
			const float fFreeFall = fDistToMin * 2.0 * input.m_invMaxAccel.m_storage;
			if (fFreeFall > fDeltaRt * fDeltaRt) {
				continue;
			}
			const float fSepRange = tol.m_toiSeparation.m_storage - tol.m_minSeparation.m_storage;
			const float fSepFall = 2.0 * fSepRange * input.m_invMaxAccel.m_storage;
			float fStep = std::sqrt(fFreeFall);
			if (fProjVel >= 0.0f) {
				fStep = fProjVel * input.m_invMaxAccel.m_storage + fProjVel * input.m_invMaxAccel.m_storage + fStep;
			} else {
				const float fMinStep = -fProjVel * input.m_invMaxAccel.m_storage;
				if (fStep < fMinStep) {
					fStep = fMinStep;
				}
			}
			if (fDeltaRt < fStep) {
				continue;
			}
			const float fSepStep = std::sqrt(fSepFall);
			float fSafeStep = fSepStep + fStep;
			const float fMinSafeDeltaRt = tol.m_minSafeDeltaRt.m_storage;
			if (!(fMinSafeDeltaRt < fSafeStep)) {
				fSafeStep = fMinSafeDeltaRt;
			}
			const float fCandidate = fSafeStep + fRt;
			if (!(fCandidate > fBestRt)) {
				fBestRt = fCandidate;
			}
		}

		if (fRtEnd < fRt) {
			return;
		}
		if (fBestRt == fRt && fRtEnd <= fRt) {
			return;
		}
		fLastRt = fRt;
		fRt = fBestRt;
		if (!iNumToCheck) {
			return;
		}
	}
}

// KEEP (partcopy:): the cache handed to addToi and copied whole into out.m_toi.m_gskCache shares its stack slot with
// the previous separating normal (or the zero it starts as): exitAndExportCacheImpl writes only the dimensions and
// the used vertex ids, so the exe's cache carries that vector's bytes in the unused ids, m_maxDimA/m_maxDimB,
// m_gskFlags and the four padding bytes, which the next call reads back. src's cache has none of them.
//
// The time of impact of A and B over the step from the separating normal gsk last found: conservative advancement
// (the safe step from the normal's distance and the bodies' worst approach, refined by a vertex sweep of A's
// spheres against B's plane once the step gets small) until the distance is within the TOI separation, then up to
// ten regula falsi steps between the last two times; the contact point is welded against triangles and its
// separating velocity is reported to the contact manager's addToi, which replaces out's TOI when it accepts (cdecl,
// 0xDB60C0). The x87 values the exe stores with `fstp dword` are float locals.
void hk4dGskCollideCalcToi(const hkpAgent3ProcessInput& in3, const float allowedPenetrationDepth,
	const float minSeparation, const float toiSeparation, hkpGskCache& gskCache, hkVector4& separatingNormal,
	hkpProcessCollisionOutput& out) {
	const hkpProcessCollisionInput* input = in3.m_input;
	const hkpCollisionQualityInfo* quality = input->m_collisionQualityInfo;
	hkp4dGskTolerances tol;
	tol.m_minSeparation.m_storage = minSeparation;
	tol.m_toiSeparation.m_storage = toiSeparation;
	tol.m_toiAccuracy.m_storage = quality->m_toiAccuracy * allowedPenetrationDepth;
	const float fRelSafeDeltaTime = allowedPenetrationDepth * quality->m_minSafeDeltaTime;
	const float fAbsSafeDeltaTime = quality->m_minAbsoluteSafeDeltaTime;
	const float fMinSafeDeltaTime = (fAbsSafeDeltaTime < fRelSafeDeltaTime) ? fRelSafeDeltaTime : fAbsSafeDeltaTime;
	const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape);
	const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape);
	const hkMotionState* motionA = in3.m_bodyA->m_motion.pState;
	const hkMotionState* motionB = in3.m_bodyB->m_motion.pState;
	tol.m_minSafeDeltaRt.m_storage = input->m_stepInfo.m_invDeltaTime * fMinSafeDeltaTime;
	tol.m_minToiDeltaTime.m_storage = quality->m_minToiDeltaTime;

	alignas(16) hkp4dGskVertexCollidePointsInput sweepIn;
	sweepIn.m_verticesA.m_storage = nullptr;
	const float fDeltaTime = input->m_stepInfo.m_deltaTime;
	const float fDeltaRtA = fDeltaTime * motionA->m_sweptTransform.m_centerOfMass1.m_quad[3];
	const float fDeltaRtB = fDeltaTime * motionB->m_sweptTransform.m_centerOfMass1.m_quad[3];
	sweepIn.m_deltaAngles[0].m_quad = _mm_mul_ps(_mm_set_ps1(fDeltaRtA), motionA->m_deltaAngle.m_quad);
	sweepIn.m_deltaAngles[1].m_quad = _mm_mul_ps(_mm_set_ps1(fDeltaRtB), motionB->m_deltaAngle.m_quad);

	// The SDK's inline hkpGsk init from the cache.
	alignas(16) hkpGsk gsk;
	gsk.m_dimA.m_storage = gskCache.m_dimA;
	gsk.m_dimB.m_storage = gskCache.m_dimB;
	gsk.m_maxDimA.m_storage = gskCache.m_maxDimA;
	gsk.m_maxDimB.m_storage = gskCache.m_maxDimB;
	gsk.m_doNotHandlePenetration.m_storage = 0;
	gsk.m_featureChange.m_storage = 0;
	shapeA->convertVertexIdsToVerticesImpl(&gskCache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
	shapeB->convertVertexIdsToVerticesImpl(&gskCache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage,
		&gsk.m_verticesBinB[0]);

	{
		alignas(16) hkp4dGskVertexCollidePointsOutput sweepOut;
		alignas(16) hkVector4 lastNormal;
		alignas(16) hkVector4 sepOut;
		alignas(16) hkVector4 pointOnA;
		alignas(16) hkTransform wTa;
		alignas(16) hkTransform wTb;
		alignas(16) hkTransform aTb;
		float fPrevDist = separatingNormal.m_quad[3];
		lastNormal.setZero4();
		int32_t iFeatureChange = 1;
		float fRt = 0.0f;
		float fRt0 = 0.0f;
		float fDist = separatingNormal.m_quad[3];
		const float fToiSepWithAccuracy = tol.m_toiAccuracy.m_storage + toiSeparation;
		bool bSweepDone = false;
		bool bHaveFeature = false;

		// Conservative advancement: step by the time the distance needs to fall to minSeparation at the worst
		// approach, or by the vertex sweep's time of impact once the step is small, and find the closest feature.
		if (!(fDist <= fToiSepWithAccuracy)) {
			while (true) {
				const float fDeltaRt = 1.0f - fRt;
				if (fDeltaRt <= 0.0f) {
					goto done;
				}
				hkVector4 crossA;
				crossA.setCross(sweepIn.m_deltaAngles[0], separatingNormal);
				const hkQuadReal sqA = _mm_mul_ps(crossA.m_quad, crossA.m_quad);
				// EXE-INLINED 0x630B40
				const float fLenA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqA, sqA, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(sqA, sqA, 0x55), sqA))));
				const float fAngA = motionA->m_objectRadius * fLenA;
				hkVector4 crossB;
				crossB.setCross(sweepIn.m_deltaAngles[1], separatingNormal);
				const hkQuadReal sqB = _mm_mul_ps(crossB.m_quad, crossB.m_quad);
				// EXE-INLINED 0x630B40
				const float fLenB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqB, sqB, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(sqB, sqB, 0x55), sqB))));
				const float fAngB = motionB->m_objectRadius * fLenB;
				const float fAngular = fAngA + fAngB;
				const hkQuadReal lin = _mm_mul_ps(in3.m_linearTimInfo.m_quad, separatingNormal.m_quad);
				// EXE-INLINED 0x62B0A0
				const float fLinear = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(lin, lin, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(lin, lin, 0x55), lin)));
				const float fApproach = fLinear + fAngular;
				if (fApproach <= 0.0f) {
					goto done;
				}
				const float fDistAtEnd = separatingNormal.m_quad[3] - fDeltaRt * fApproach;
				if (minSeparation < fDistAtEnd) {
					goto done;
				}
				const float fToMinSeparation = separatingNormal.m_quad[3] - minSeparation;
				float fStep = fToMinSeparation / fApproach;
				if (fStep < 0.20000000298023224 && !bSweepDone && bHaveFeature && fLinear < fAngular) {
					if (!sweepIn.m_verticesA.m_storage) {
						sweepIn.m_motionA.m_storage = in3.m_bodyA->m_motion.pState;
						sweepIn.m_motionB.m_storage = in3.m_bodyB->m_motion.pState;
						const hkpConvexShape* pShapeA = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape);
						const hkpConvexShape* pShapeB = static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape);
						sweepIn.m_radiusSum.m_storage = pShapeA->m_radius + pShapeB->m_radius;
						sweepIn.m_stepInfo.m_storage = &in3.m_input->m_stepInfo;
						sweepIn.m_linearTimInfo = in3.m_linearTimInfo;
						const int32_t iNumSpheres = pShapeA->getNumCollisionSpheresImpl();
						sweepIn.m_allocatedNumVertices.m_storage = iNumSpheres;
						sweepIn.m_numVertices.m_storage = iNumSpheres;
						// EXE-INLINED none: hkAllocateStack<hkVector4>
						hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
						void* pBlock = stack.m_next.m_storage;
						stack.m_next.m_storage = static_cast<char*>(pBlock) +
							((iNumSpheres * static_cast<int32_t>(sizeof(hkVector4)) + 127) & ~127);
						sweepIn.m_verticesA.m_storage = static_cast<hkVector4*>(pBlock);
						// The stack block is untyped memory the spheres are written to and read back as vectors.
						pShapeA->getCollisionSpheresImpl(static_cast<hkSphere*>(pBlock));
						const float fAccelA = sweepIn.m_motionA.m_storage->m_objectRadius *
							sweepIn.m_deltaAngles[0].m_quad[3] * sweepIn.m_deltaAngles[0].m_quad[3];
						const float fAccelB = sweepIn.m_motionB.m_storage->m_objectRadius *
							sweepIn.m_deltaAngles[1].m_quad[3] * sweepIn.m_deltaAngles[1].m_quad[3];
						sweepIn.m_maxAccel.m_storage = fAccelA + fAccelB;
						sweepIn.m_invMaxAccel.m_storage = 1.0 / (sweepIn.m_maxAccel.m_storage + 1.1920928955078125e-07);
					}
					if (!sweepIn.m_verticesA.m_storage) {
						bSweepDone = true;
					} else {
						sweepIn.m_startRt.m_storage = fRt;
						sweepIn.m_worstCaseApproachingDelta.m_storage = fApproach;
						sweepOut.m_Rtoi.m_storage = 1.0f;
						hk4dGskCollidePointsWithPlane(sweepIn, tol, sweepOut);
						const float fToiStep = sweepOut.m_Rtoi.m_storage - fRt;
						if (fStep + fStep > fToiStep) {
							bSweepDone = true;
						}
						if (!(fStep > fToiStep)) {
							fStep = fToiStep;
						}
					}
				}
				if (fStep + fRt >= 1.0) {
					goto done;
				}
				fRt0 = fRt;
				if (tol.m_minSafeDeltaRt.m_storage > fStep) {
					fStep = tol.m_minSafeDeltaRt.m_storage;
				}
				fRt = fRt + fStep;
				if (!(1.0f > fRt)) {
					fRt = 1.0f;
				}

				fPrevDist = separatingNormal.m_quad[3];
				const float fTAddOn = input->m_stepInfo.m_deltaTime * fRt;
				lastNormal = separatingNormal;
				hkSweptTransformUtil::lerp2Ha(motionA->m_sweptTransform, input->m_stepInfo.m_startTime, fTAddOn, wTa);
				hkSweptTransformUtil::lerp2Ha(motionB->m_sweptTransform, input->m_stepInfo.m_startTime, fTAddOn, wTb);
				aTb.setMulInverseMul(wTa, wTb);
				gsk.getClosestFeature(shapeA, shapeB, aTb, sepOut);
				// EXE-INLINED 0xC7F700
				const hkRotation& rWa = wTa.getRotation();
				const hkQuadReal s = sepOut.m_quad;
				separatingNormal.m_quad =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), rWa.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), rWa.m_col0.m_quad)),
						_mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), rWa.m_col2.m_quad));
				// The point on A of the simplex: A's vertex, else B's vertex moved back by the distance, else the
				// closest point of the two edges.
				if (gsk.m_dimA.m_storage == 1) {
					pointOnA = gsk.m_verticesA[0];
				} else if (gsk.m_dimB.m_storage == 1) {
					const hkQuadReal d = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
					pointOnA.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, _MM_SHUFFLE(0, 0, 0, 0)), s),
						gsk.m_verticesBinA[0].m_quad);
				} else {
					pointOnA = gsk.m_closestLineLineResult.m_closestPointA;
				}

				// B's plane for a vertex sweep: the point on B (A's point moved back by the distance) and the
				// normal, both in B's space.
				// EXE-INLINED 0xC7F690
				const hkQuadReal w = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
				hkVector4 pointBinA;
				pointBinA.m_quad = _mm_sub_ps(_mm_sub_ps(pointOnA.m_quad,
												  _mm_mul_ps(_mm_shuffle_ps(w, w, _MM_SHUFFLE(0, 0, 0, 0)), s)),
					aTb.m_translation.m_quad);
				sweepIn.m_pointOnPlaneB._setRotatedInverseDir(aTb.getRotation(), pointBinA);
				iFeatureChange = gsk.m_featureChange.m_storage;
				separatingNormal.m_quad[3] = sepOut.m_quad[3] - shapeA->m_radius - shapeB->m_radius;
				fDist = separatingNormal.m_quad[3];
				sweepIn.m_planeB._setRotatedInverseDir(wTb.getRotation(), separatingNormal);
				bHaveFeature = true;
				if (fDist <= fToiSepWithAccuracy) {
					break;
				}
			}
		}

		// Regula falsi between fRt0 (distance fPrevDist) and fRt (fDist), at most ten steps; from the second step on
		// the feature is kept and only B's vertices are moved once the distance is within the accuracy.
		float fT0 = fRt0;
		float fT1 = fRt;
		float fD0 = fPrevDist;
		float fD1 = fDist;
		int32_t iStep = 0;
		const float fRadiusSum = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape)->m_radius +
			static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape)->m_radius;
		const float fApproachPerRt = (fDist - fPrevDist) / (fRt - fRt0);
		const float fNegAccuracy = -tol.m_toiAccuracy.m_storage;
		float fT;
		while (true) {
			float fDeltaDist = fD1 - fD0;
			if (fNegAccuracy < fDeltaDist) {
				fT1 = fT0;
				fDeltaDist = 0.5f;
			}
			const float fError = fD1 - toiSeparation;
			const float fAbsError = std::fabs(fError);
			if (fAbsError < tol.m_toiAccuracy.m_storage) {
				bHaveFeature = true;
				iFeatureChange = 0;
				fT = fT1;
			} else {
				bHaveFeature = false;
				float fFraction = (toiSeparation - fD0) / fDeltaDist;
				if (!(fFraction > 0.10000000149011612)) {
					fFraction = 0.1f;
				}
				if (!(fFraction < 0.8999999761581421)) {
					fFraction = 0.9f;
				}
				fT = fFraction * fT1 + (1.0 - fFraction) * fT0;
			}
			const float fTAddOn = in3.m_input->m_stepInfo.m_deltaTime * fT;
			hkSweptTransformUtil::lerp2Ha(in3.m_bodyA->m_motion.pState->m_sweptTransform,
				in3.m_input->m_stepInfo.m_startTime, fTAddOn, wTa);
			hkSweptTransformUtil::lerp2Ha(in3.m_bodyB->m_motion.pState->m_sweptTransform,
				in3.m_input->m_stepInfo.m_startTime, fTAddOn, wTb);
			aTb.setMulInverseMul(wTa, wTb);
			if (iFeatureChange) {
				gsk.getClosestFeature(static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape),
					static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape), aTb, sepOut);
				// As in the advancement above.
				if (gsk.m_dimA.m_storage == 1) {
					pointOnA = gsk.m_verticesA[0];
				} else if (gsk.m_dimB.m_storage == 1) {
					const hkQuadReal s = sepOut.m_quad;
					const hkQuadReal d = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
					pointOnA.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, _MM_SHUFFLE(0, 0, 0, 0)), s),
						gsk.m_verticesBinA[0].m_quad);
				} else {
					pointOnA = gsk.m_closestLineLineResult.m_closestPointA;
				}
			} else {
				// B's vertices moved into A's space by aTb, from the last down.
				const hkQuadReal c0 = aTb.m_rotation.m_col0.m_quad;
				const hkQuadReal c1 = aTb.m_rotation.m_col1.m_quad;
				const hkQuadReal c2 = aTb.m_rotation.m_col2.m_quad;
				const hkQuadReal t = aTb.m_translation.m_quad;
				int32_t i = gsk.m_dimB.m_storage - 1;
				const hkpCdVertex* pVertexB = gsk.m_verticesBinB + i;
				hkVector4* pVertexA = gsk.m_verticesBinA + i;
				do {
					const hkQuadReal v = pVertexB->m_quad;
					// EXE-INLINED 0xC7F640
					pVertexA->m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1),
													  _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0)),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2), t));
					--i;
					--pVertexB;
					--pVertexA;
				} while (i >= 0);
				alignas(16) hkVector4 hint;
				hint._setRotatedInverseDir(wTa.getRotation(), lastNormal);
				hkGskRecalcContactInternal(gsk.m_verticesA, gsk.m_verticesBinA, gsk.m_dimA.m_storage,
					gsk.m_dimB.m_storage, &hint, &pointOnA, &sepOut);
			}
			fDist = sepOut.m_quad[3] - fRadiusSum;
			const float fNewError = fDist - toiSeparation;
			const float fNewAbsError = std::fabs(fNewError);
			if (tol.m_toiAccuracy.m_storage > fNewAbsError) {
				break;
			}
			if (bHaveFeature) {
				break;
			}
			if (fT1 == fT0) {
				break;
			}
			if (fDist < toiSeparation) {
				fT1 = fT;
				fD1 = fDist;
			} else {
				fT0 = fT;
				fD0 = fDist;
			}
			++iStep;
			if (iStep >= 10) {
				break;
			}
		}

		const hkStepInfo& stepInfo = in3.m_input->m_stepInfo;
		const float fToiDeltaTime = stepInfo.m_deltaTime * fT;
		const float fSafeToiDeltaTime =
			(tol.m_minToiDeltaTime.m_storage < fToiDeltaTime) ? fToiDeltaTime : tol.m_minToiDeltaTime.m_storage;
		const float fToi = stepInfo.m_startTime + fSafeToiDeltaTime;
		if (!(out.m_toi.m_time.m_storage > fToi)) {
			goto done;
		}
		if (!(fToi < stepInfo.m_endTime - tol.m_minToiDeltaTime.m_storage)) {
			goto done;
		}

		// The contact point in world space: on B, the normal's w the distance.
		const hkpConvexShape* pShapeA = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape);
		alignas(16) hkVector4 pointWorld;
		pointWorld.setTransformedPos(wTa, pointOnA);
		alignas(16) hkContactPoint cp;
		cp.m_separatingNormal.setRotatedDir(wTa.getRotation(), sepOut);
		const float fPositionScale = -pShapeA->m_radius - fDist;
		const hkQuadReal rotatedNormal = cp.m_separatingNormal.m_quad;
		cp.m_separatingNormal.m_quad[3] = fDist;
		alignas(16) hkVector4 normalInOut = cp.m_separatingNormal;
		cp.m_position.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set_ps1(fPositionScale), rotatedNormal), pointWorld.m_quad);

		// Welding against a triangle: B's, else A's (with the position moved back onto A and the normal flipped).
		int32_t iWeldResult = hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
		uint16_t featureIds[3];
		uint8_t numFeatures;
		const hkpConvexShape* pShapeB = static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape);
		if (pShapeB->m_type.m_storage == HK_SHAPE_TRIANGLE) {
			numFeatures = static_cast<uint8_t>(gsk.m_dimB.m_storage);
			featureIds[0] = static_cast<uint16_t>(gsk.m_verticesBinB[0].getId());
			featureIds[1] = (numFeatures >= 2) ? static_cast<uint16_t>(gsk.m_verticesBinB[1].getId()) : 0xFFFF;
			featureIds[2] = (numFeatures >= 3) ? static_cast<uint16_t>(gsk.m_verticesBinB[2].getId()) : 0xFFFF;
			iWeldResult =
				pShapeB->weldContactPointImpl(featureIds, numFeatures, cp.m_position, &wTb, pShapeA, &wTa, normalInOut);
			cp.m_separatingNormal = normalInOut;
		} else if (pShapeA->m_type.m_storage == HK_SHAPE_TRIANGLE) {
			numFeatures = static_cast<uint8_t>(gsk.m_dimA.m_storage);
			featureIds[0] = static_cast<uint16_t>(gsk.m_verticesA[0].getId());
			featureIds[1] = (numFeatures >= 2) ? static_cast<uint16_t>(gsk.m_verticesA[1].getId()) : 0xFFFF;
			featureIds[2] = (numFeatures >= 3) ? static_cast<uint16_t>(gsk.m_verticesA[2].getId()) : 0xFFFF;
			const hkQuadReal n = cp.m_separatingNormal.m_quad;
			alignas(16) hkVector4 positionOnA;
			positionOnA.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, _MM_SHUFFLE(3, 3, 3, 3)), n), cp.m_position.m_quad);
			normalInOut.setNeg3(cp.m_separatingNormal);
			iWeldResult = pShapeA->weldContactPointImpl(featureIds, numFeatures, positionOnA, &wTa,
				static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape), &wTb, normalInOut);
			normalInOut.setNeg3(normalInOut);
			cp.m_separatingNormal = normalInOut;
		}
		if (iWeldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT &&
			in3.m_input->m_collisionQualityInfo->m_enableToiWeldRejection.m_bool) {
			goto done;
		}

		// The separating velocity at the contact point: A's angular motion about its mass centre less the linear
		// motion and B's angular motion about its mass centre, along the normal.
		const hkMotionState* pMotionA = in3.m_bodyA->m_motion.pState;
		const hkMotionState* pMotionB = in3.m_bodyB->m_motion.pState;
		const float fRelA = (fToi - pMotionA->m_sweptTransform.m_centerOfMass0.m_quad[3]) *
			pMotionA->m_sweptTransform.m_centerOfMass1.m_quad[3];
		// EXE-INLINED 0x5DBF70
		const hkQuadReal relA = _mm_shuffle_ps(_mm_set_ss(fRelA), _mm_set_ss(fRelA), _MM_SHUFFLE(0, 0, 0, 0));
		hkVector4 centerA;
		centerA.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, relA),
										pMotionA->m_sweptTransform.m_centerOfMass0.m_quad),
			_mm_mul_ps(pMotionA->m_sweptTransform.m_centerOfMass1.m_quad, relA));
		const float fRelB = (fToi - pMotionB->m_sweptTransform.m_centerOfMass0.m_quad[3]) *
			pMotionB->m_sweptTransform.m_centerOfMass1.m_quad[3];
		// EXE-INLINED 0x5DBF70
		const hkQuadReal relB = _mm_shuffle_ps(_mm_set_ss(fRelB), _mm_set_ss(fRelB), _MM_SHUFFLE(0, 0, 0, 0));
		hkVector4 centerB;
		centerB.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, relB),
										pMotionB->m_sweptTransform.m_centerOfMass0.m_quad),
			_mm_mul_ps(pMotionB->m_sweptTransform.m_centerOfMass1.m_quad, relB));
		hkVector4 armA;
		armA.m_quad = _mm_sub_ps(cp.m_position.m_quad, centerA.m_quad);
		hkVector4 armB;
		armB.m_quad = _mm_sub_ps(cp.m_position.m_quad, centerB.m_quad);
		hkVector4 velA;
		velA.setCross(sweepIn.m_deltaAngles[0], armA);
		hkVector4 negLinear;
		negLinear.setNeg4(in3.m_linearTimInfo);
		hkVector4 velB;
		velB.setCross(sweepIn.m_deltaAngles[1], armB);
		hkVector4 vel;
		vel.m_quad = _mm_sub_ps(_mm_add_ps(velA.m_quad, negLinear.m_quad), velB.m_quad);
		const hkQuadReal projected = _mm_mul_ps(vel.m_quad, cp.m_separatingNormal.m_quad);
		// EXE-INLINED 0x62B0A0
		float fProjectedVelocity = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(projected, projected, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(projected, projected, 0x55), projected)));

		// A welded contact that separates: sweep A's spheres against B's vertex plane again from the TOI, and take
		// the earliest point that reaches it.
		if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED &&
			fProjectedVelocity > -1.1920929e-07f) {
			alignas(16) hkp4dGskVertexCollidePointsInput sweepIn2;
			sweepIn2.m_startRt.m_storage = fT;
			const float fDeltaTime2 = in3.m_input->m_stepInfo.m_deltaTime;
			const float fDeltaRtA2 = fDeltaTime2 * pMotionA->m_sweptTransform.m_centerOfMass1.m_quad[3];
			const float fDeltaRtB2 = fDeltaTime2 * pMotionB->m_sweptTransform.m_centerOfMass1.m_quad[3];
			sweepIn2.m_deltaAngles[0].m_quad = _mm_mul_ps(_mm_set_ps1(fDeltaRtA2), pMotionA->m_deltaAngle.m_quad);
			sweepIn2.m_deltaAngles[1].m_quad = _mm_mul_ps(_mm_set_ps1(fDeltaRtB2), pMotionB->m_deltaAngle.m_quad);
			hkVector4 crossA;
			crossA.setCross(sweepIn2.m_deltaAngles[0], cp.m_separatingNormal);
			const hkQuadReal sqA = _mm_mul_ps(crossA.m_quad, crossA.m_quad);
			// EXE-INLINED 0x630B40
			const float fLenA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqA, sqA, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(sqA, sqA, 0x55), sqA))));
			const float fAngA = pMotionA->m_objectRadius * fLenA;
			hkVector4 crossB;
			crossB.setCross(sweepIn2.m_deltaAngles[1], cp.m_separatingNormal);
			const hkQuadReal sqB = _mm_mul_ps(crossB.m_quad, crossB.m_quad);
			// EXE-INLINED 0x630B40
			const float fLenB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqB, sqB, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(sqB, sqB, 0x55), sqB))));
			const float fAngB = pMotionB->m_objectRadius * fLenB;
			const float fAngular = fAngA + fAngB;
			const hkQuadReal lin = _mm_mul_ps(in3.m_linearTimInfo.m_quad, cp.m_separatingNormal.m_quad);
			// EXE-INLINED 0x62B0A0
			const float fLinear = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(lin, lin, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(lin, lin, 0x55), lin)));
			sweepIn2.m_worstCaseApproachingDelta.m_storage = fAngular + fLinear;
			const hkpConvexShape* pSweepShapeA = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape);
			const hkpConvexShape* pSweepShapeB = static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape);
			sweepIn2.m_motionB.m_storage = in3.m_bodyB->m_motion.pState;
			sweepIn2.m_motionA.m_storage = in3.m_bodyA->m_motion.pState;
			sweepIn2.m_radiusSum.m_storage = pSweepShapeA->m_radius + pSweepShapeB->m_radius;
			sweepIn2.m_linearTimInfo = in3.m_linearTimInfo;
			sweepIn2.m_stepInfo.m_storage = &in3.m_input->m_stepInfo;
			const int32_t iNumSpheres = pSweepShapeA->getNumCollisionSpheresImpl();
			sweepIn2.m_allocatedNumVertices.m_storage = iNumSpheres;
			sweepIn2.m_numVertices.m_storage = iNumSpheres;
			// EXE-INLINED none: hkAllocateStack<hkVector4>
			hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
			void* pBlock = stack.m_next.m_storage;
			stack.m_next.m_storage =
				static_cast<char*>(pBlock) + ((iNumSpheres * static_cast<int32_t>(sizeof(hkVector4)) + 127) & ~127);
			sweepIn2.m_verticesA.m_storage = static_cast<hkVector4*>(pBlock);
			// The stack block is untyped memory the spheres are written to and read back as vectors.
			pSweepShapeA->getCollisionSpheresImpl(static_cast<hkSphere*>(pBlock));
			const float fAccelA = sweepIn2.m_motionA.m_storage->m_objectRadius * sweepIn2.m_deltaAngles[0].m_quad[3] *
				sweepIn2.m_deltaAngles[0].m_quad[3];
			const float fAccelB = sweepIn2.m_motionB.m_storage->m_objectRadius * sweepIn2.m_deltaAngles[1].m_quad[3] *
				sweepIn2.m_deltaAngles[1].m_quad[3];
			sweepIn2.m_maxAccel.m_storage = fAccelA + fAccelB;
			sweepIn2.m_invMaxAccel.m_storage = 1.0 / (sweepIn2.m_maxAccel.m_storage + 1.1920928955078125e-07);
			sweepIn2.m_planeB.setRotatedInverseDir(wTb.getRotation(), cp.m_separatingNormal);
			sweepIn2.m_pointOnPlaneB = gsk.m_verticesBinB[0];
			if (sweepIn2.m_verticesA.m_storage) {
				sweepOut.m_Rtoi.m_storage = 1.0f;
				hk4dGskCollidePointsWithPlane(sweepIn2, tol, sweepOut);
				// EXE-INLINED none: hkDeallocateStack<hkVector4>
				hkMemoryRouter::getInstance().m_stack.m_next.m_storage =
					static_cast<char*>(static_cast<void*>(sweepIn2.m_verticesA.m_storage));
				if (1.0f > sweepOut.m_Rtoi.m_storage) {
					alignas(16) hkVector4 collidingPoint;
					collidingPoint.setTransformedPos(wTa, sweepOut.m_collidingPoint);
					cp.m_position = collidingPoint;
				}
			}
		}

		// The separating velocity is clamped to the approach the advancement measured.
		if (fProjectedVelocity > fApproachPerRt) {
			if (fProjectedVelocity * 1.2000000476837158 < fApproachPerRt) {
				fProjectedVelocity = fApproachPerRt;
			} else {
				fProjectedVelocity = hk4dGskCalcPointVelocityAssumingAngularVelError(&sweepIn.m_deltaAngles[0],
					in3.m_bodyB->m_motion.pState, in3.m_bodyA->m_motion.pState, &cp.m_position, &cp.m_separatingNormal,
					&in3.m_linearTimInfo, fToi);
				if (fProjectedVelocity > 0.0) {
					fProjectedVelocity = 0.0f;
				}
			}
		}
		fProjectedVelocity = stepInfo.m_invDeltaTime * fProjectedVelocity;

		alignas(16) hkContactPointPropertiesWithExtendedUserData16 properties;
		properties.m_impulseApplied = 0.0f;
		properties.m_internalSolverData = 0.0f;
		properties.m_userData = 0;
		properties.m_friction.m_value = 0;
		properties.m_restitution = 0;
		properties.m_maxImpulse.m_value = 0;
		properties.m_flags = 0;
		properties.m_internalDataA = 321.0f;
		for (int32_t i = 0; i < 7; ++i) {
			properties.m_extendedUserDatas[i] = 0;
		}
		alignas(16) hkGskCache16 cache;
		gsk.exitAndExportCacheImpl(cache);
		if (in3.m_contactMgr->addToi(*in3.m_bodyA, *in3.m_bodyB, *in3.m_input, out, fToi, cp, &cache,
				fProjectedVelocity, properties) == hkpContactMgr::TOI_ACCEPT) {
			if (out.m_toi.m_time.m_storage != 3.402820018375656e+38) {
				in3.m_contactMgr->removeToi(*out.m_constraintOwner.m_storage, out.m_toi.m_properties);
			}
			out.m_toi.m_seperatingVelocity.m_storage = fProjectedVelocity;
			out.m_toi.m_contactPoint.m_position = cp.m_position;
			out.m_toi.m_contactPoint.m_separatingNormal = cp.m_separatingNormal;
			out.m_toi.m_time.m_storage = fToi;
			out.m_toi.m_properties = properties;
			out.m_toi.m_gskCache = cache;
		}
	}
done:
	if (sweepIn.m_verticesA.m_storage) {
		// EXE-INLINED none: hkDeallocateStack<hkVector4>
		hkMemoryRouter::getInstance().m_stack.m_next.m_storage =
			static_cast<char*>(static_cast<void*>(sweepIn.m_verticesA.m_storage));
	}
	if (gsk.m_featureChange.m_storage) {
		gsk.exitAndExportCacheImpl(gskCache);
	}
}

float hk4dGskCalcPointVelocityAssumingAngularVelError(const hkVector4* apAngular, const hkMotionState* apMotionB,
	const hkMotionState* apMotionA, const hkVector4* apPoint, const hkVector4* apNormal,
	const hkVector4* apLinearTimInfo, float afTime) {
	double v13;
	uint32_t v22;
	uint32_t v23;
	*reinterpret_cast<float*>(&v22) = (afTime - apMotionA->m_sweptTransform.m_centerOfMass0.m_quad[3]) *
		apMotionA->m_sweptTransform.m_centerOfMass1.m_quad[3];
	__m128 v7 = _mm_mul_ps(_mm_xor_ps(apLinearTimInfo->m_quad, hkQuadRealNegateMask), apNormal->m_quad);
	float v8 = _mm_shuffle_ps(v7, v7, 170)[0] + static_cast<float>(_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
	__m128 v9 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v22)), _mm_set_ss(std::bit_cast<float>(v22)), 0);
	__m128 v10 = _mm_sub_ps(apPoint->m_quad,
		_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v9), apMotionA->m_sweptTransform.m_centerOfMass0.m_quad),
			_mm_mul_ps(apMotionA->m_sweptTransform.m_centerOfMass1.m_quad, v9)));
	__m128 v11 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(apAngular->m_quad, apAngular->m_quad, 201),
										   _mm_shuffle_ps(v10, v10, 210)),
								_mm_mul_ps(_mm_shuffle_ps(apAngular->m_quad, apAngular->m_quad, 210),
									_mm_shuffle_ps(v10, v10, 201))),
		apNormal->m_quad);
	double v12 = static_cast<float>(_mm_shuffle_ps(v11, v11, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]));
	if (!(v12 <= 0.0)) {
		v13 = v12 / 1.2000000476837158;
	} else {
		v13 = v12 * 1.2000000476837158;
	}
	float v20 = v13 + v8;
	*reinterpret_cast<float*>(&v23) = (afTime - apMotionB->m_sweptTransform.m_centerOfMass0.m_quad[3]) *
		apMotionB->m_sweptTransform.m_centerOfMass1.m_quad[3];
	__m128 v14 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v23)), _mm_set_ss(std::bit_cast<float>(v23)), 0);
	__m128 v15 = _mm_sub_ps(apPoint->m_quad,
		_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v14), apMotionB->m_sweptTransform.m_centerOfMass0.m_quad),
			_mm_mul_ps(apMotionB->m_sweptTransform.m_centerOfMass1.m_quad, v14)));
	__m128 v16 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(apAngular[1].m_quad, apAngular[1].m_quad, 201),
										   _mm_shuffle_ps(v15, v15, 210)),
								_mm_mul_ps(_mm_shuffle_ps(apAngular[1].m_quad, apAngular[1].m_quad, 210),
									_mm_shuffle_ps(v15, v15, 201))),
		apNormal->m_quad);
	double v17 = static_cast<float>(_mm_shuffle_ps(v16, v16, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v16, v16, 85)[0] + v16[0]));
	if (!(v17 >= 0.0)) {
		return v20 - v17 / 1.2000000476837158;
	} else {
		return v20 - 1.2000000476837158 * v17;
	}
}

// KEEP (partcopy:): the cache handed to addToi and copied whole into out->m_toi.m_gskCache shares its stack slot with
// the hint the B-vertex step builds from lastNormal (never written when every step runs GSK): exitAndExportCacheImpl
// writes only the dimensions and the used vertex ids, so the exe's cache carries that vector's bytes, or
// uninitialised stack, in the unused ids, m_maxDimA/m_maxDimB, m_gskFlags and the four padding bytes. src's cache has
// none of them.
//
// hk4dGskCollideCalcToi's tail as it stands there (lines from "Regula falsi" on), with the tolerances read from tol,
// the angular steps from deltaAngles and no stack block or cache export at the end. The exe calls
// hkVector4::setTransformedPos, setRotatedDir and hkMemoryRouter::getInstance out of line here.
void findToi(const hkpAgent3ProcessInput& in3, const hkp4dGskTolerances* tol, hkpGsk* gsk, int32_t iFeatureChange,
	const hkVector4* lastNormal, const hkVector4* deltaAngles, float fT0, float fT1, float fD0, float fD1,
	hkpProcessCollisionOutput* out) {
	alignas(16) hkVector4 sepOut;
	alignas(16) hkVector4 pointOnA;
	alignas(16) hkTransform wTa;
	alignas(16) hkTransform wTb;
	alignas(16) hkTransform aTb;
	const float fRadiusSum = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape)->m_radius +
		static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape)->m_radius;
	int32_t iStep = 0;
	const float fApproachPerRt = (fD1 - fD0) / (fT1 - fT0);
	bool bHaveFeature;
	float fT;
	float fDist;
	while (true) {
		float fDeltaDist = fD1 - fD0;
		if (-tol->m_toiAccuracy.m_storage < fDeltaDist) {
			fT1 = fT0;
			fDeltaDist = 0.5f;
		}
		const float fError = fD1 - tol->m_toiSeparation.m_storage;
		const float fAbsError = std::fabs(fError);
		if (fAbsError < tol->m_toiAccuracy.m_storage) {
			bHaveFeature = true;
			iFeatureChange = 0;
			fT = fT1;
		} else {
			bHaveFeature = false;
			float fFraction = (tol->m_toiSeparation.m_storage - fD0) / fDeltaDist;
			if (!(fFraction > 0.10000000149011612)) {
				fFraction = 0.1f;
			}
			if (!(fFraction < 0.8999999761581421)) {
				fFraction = 0.9f;
			}
			fT = fFraction * fT1 + (1.0 - fFraction) * fT0;
		}
		const float fTAddOn = in3.m_input->m_stepInfo.m_deltaTime * fT;
		hkSweptTransformUtil::lerp2Ha(in3.m_bodyA->m_motion.pState->m_sweptTransform,
			in3.m_input->m_stepInfo.m_startTime, fTAddOn, wTa);
		hkSweptTransformUtil::lerp2Ha(in3.m_bodyB->m_motion.pState->m_sweptTransform,
			in3.m_input->m_stepInfo.m_startTime, fTAddOn, wTb);
		aTb.setMulInverseMul(wTa, wTb);
		if (iFeatureChange) {
			gsk->getClosestFeature(static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape),
				static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape), aTb, sepOut);
			// The point on A of the simplex: A's vertex, else B's vertex moved back by the distance, else the
			// closest point of the two edges.
			if (gsk->m_dimA.m_storage == 1) {
				pointOnA = gsk->m_verticesA[0];
			} else if (gsk->m_dimB.m_storage == 1) {
				const hkQuadReal s = sepOut.m_quad;
				const hkQuadReal d = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
				pointOnA.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, _MM_SHUFFLE(0, 0, 0, 0)), s),
					gsk->m_verticesBinA[0].m_quad);
			} else {
				pointOnA = gsk->m_closestLineLineResult.m_closestPointA;
			}
		} else {
			// B's vertices moved into A's space by aTb, from the last down.
			const hkQuadReal c0 = aTb.m_rotation.m_col0.m_quad;
			const hkQuadReal c1 = aTb.m_rotation.m_col1.m_quad;
			const hkQuadReal c2 = aTb.m_rotation.m_col2.m_quad;
			const hkQuadReal t = aTb.m_translation.m_quad;
			int32_t i = gsk->m_dimB.m_storage - 1;
			const hkpCdVertex* pVertexB = gsk->m_verticesBinB + i;
			hkVector4* pVertexA = gsk->m_verticesBinA + i;
			do {
				const hkQuadReal v = pVertexB->m_quad;
				// EXE-INLINED 0xC7F640
				pVertexA->m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), c1),
												  _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), c0)),
					_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), c2), t));
				--i;
				--pVertexB;
				--pVertexA;
			} while (i >= 0);
			alignas(16) hkVector4 hint;
			hint._setRotatedInverseDir(wTa.getRotation(), *lastNormal);
			hkGskRecalcContactInternal(gsk->m_verticesA, gsk->m_verticesBinA, gsk->m_dimA.m_storage,
				gsk->m_dimB.m_storage, &hint, &pointOnA, &sepOut);
		}
		fDist = sepOut.m_quad[3] - fRadiusSum;
		const float fNewError = fDist - tol->m_toiSeparation.m_storage;
		const float fNewAbsError = std::fabs(fNewError);
		if (tol->m_toiAccuracy.m_storage > fNewAbsError) {
			break;
		}
		if (bHaveFeature) {
			break;
		}
		if (fT1 == fT0) {
			break;
		}
		if (fDist < tol->m_toiSeparation.m_storage) {
			fT1 = fT;
			fD1 = fDist;
		} else {
			fT0 = fT;
			fD0 = fDist;
		}
		++iStep;
		if (iStep >= 10) {
			break;
		}
	}

	const hkStepInfo& stepInfo = in3.m_input->m_stepInfo;
	const float fMinToiDeltaTime = tol->m_minToiDeltaTime.m_storage;
	const float fToiDeltaTime = stepInfo.m_deltaTime * fT;
	const float fSafeToiDeltaTime = (fMinToiDeltaTime < fToiDeltaTime) ? fToiDeltaTime : fMinToiDeltaTime;
	const float fToi = stepInfo.m_startTime + fSafeToiDeltaTime;
	if (!(out->m_toi.m_time.m_storage > fToi)) {
		return;
	}
	if (!(fToi < stepInfo.m_endTime - tol->m_minToiDeltaTime.m_storage)) {
		return;
	}

	// The contact point in world space: on B, the normal's w the distance.
	const hkpConvexShape* pShapeA = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape);
	alignas(16) hkVector4 pointWorld;
	pointWorld.setTransformedPos(wTa, pointOnA);
	alignas(16) hkContactPoint cp;
	cp.m_separatingNormal.setRotatedDir(wTa.getRotation(), sepOut);
	const float fPositionScale = -pShapeA->m_radius - fDist;
	const hkQuadReal rotatedNormal = cp.m_separatingNormal.m_quad;
	cp.m_separatingNormal.m_quad[3] = fDist;
	alignas(16) hkVector4 normalInOut = cp.m_separatingNormal;
	cp.m_position.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set_ps1(fPositionScale), rotatedNormal), pointWorld.m_quad);

	// Welding against a triangle: B's, else A's (with the position moved back onto A and the normal flipped).
	int32_t iWeldResult = hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED;
	uint16_t featureIds[3];
	uint8_t numFeatures;
	const hkpConvexShape* pShapeB = static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape);
	if (pShapeB->m_type.m_storage == HK_SHAPE_TRIANGLE) {
		numFeatures = static_cast<uint8_t>(gsk->m_dimB.m_storage);
		featureIds[0] = static_cast<uint16_t>(gsk->m_verticesBinB[0].getId());
		featureIds[1] = (numFeatures >= 2) ? static_cast<uint16_t>(gsk->m_verticesBinB[1].getId()) : 0xFFFF;
		featureIds[2] = (numFeatures >= 3) ? static_cast<uint16_t>(gsk->m_verticesBinB[2].getId()) : 0xFFFF;
		iWeldResult =
			pShapeB->weldContactPointImpl(featureIds, numFeatures, cp.m_position, &wTb, pShapeA, &wTa, normalInOut);
		cp.m_separatingNormal = normalInOut;
	} else if (pShapeA->m_type.m_storage == HK_SHAPE_TRIANGLE) {
		numFeatures = static_cast<uint8_t>(gsk->m_dimA.m_storage);
		featureIds[0] = static_cast<uint16_t>(gsk->m_verticesA[0].getId());
		featureIds[1] = (numFeatures >= 2) ? static_cast<uint16_t>(gsk->m_verticesA[1].getId()) : 0xFFFF;
		featureIds[2] = (numFeatures >= 3) ? static_cast<uint16_t>(gsk->m_verticesA[2].getId()) : 0xFFFF;
		const hkQuadReal n = cp.m_separatingNormal.m_quad;
		alignas(16) hkVector4 positionOnA;
		positionOnA.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, _MM_SHUFFLE(3, 3, 3, 3)), n), cp.m_position.m_quad);
		normalInOut.setNeg3(cp.m_separatingNormal);
		iWeldResult = pShapeA->weldContactPointImpl(featureIds, numFeatures, positionOnA, &wTa,
			static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape), &wTb, normalInOut);
		normalInOut.setNeg3(normalInOut);
		cp.m_separatingNormal = normalInOut;
	}
	if (iWeldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT &&
		in3.m_input->m_collisionQualityInfo->m_enableToiWeldRejection.m_bool) {
		return;
	}

	// The separating velocity at the contact point: A's angular motion about its mass centre less the linear motion
	// and B's angular motion about its mass centre, along the normal.
	const hkMotionState* pMotionA = in3.m_bodyA->m_motion.pState;
	const hkMotionState* pMotionB = in3.m_bodyB->m_motion.pState;
	const float fRelA = (fToi - pMotionA->m_sweptTransform.m_centerOfMass0.m_quad[3]) *
		pMotionA->m_sweptTransform.m_centerOfMass1.m_quad[3];
	// EXE-INLINED 0x5DBF70
	const hkQuadReal relA = _mm_shuffle_ps(_mm_set_ss(fRelA), _mm_set_ss(fRelA), _MM_SHUFFLE(0, 0, 0, 0));
	hkVector4 centerA;
	centerA.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, relA), pMotionA->m_sweptTransform.m_centerOfMass0.m_quad),
			_mm_mul_ps(pMotionA->m_sweptTransform.m_centerOfMass1.m_quad, relA));
	const float fRelB = (fToi - pMotionB->m_sweptTransform.m_centerOfMass0.m_quad[3]) *
		pMotionB->m_sweptTransform.m_centerOfMass1.m_quad[3];
	// EXE-INLINED 0x5DBF70
	const hkQuadReal relB = _mm_shuffle_ps(_mm_set_ss(fRelB), _mm_set_ss(fRelB), _MM_SHUFFLE(0, 0, 0, 0));
	hkVector4 centerB;
	centerB.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, relB), pMotionB->m_sweptTransform.m_centerOfMass0.m_quad),
			_mm_mul_ps(pMotionB->m_sweptTransform.m_centerOfMass1.m_quad, relB));
	hkVector4 armA;
	armA.m_quad = _mm_sub_ps(cp.m_position.m_quad, centerA.m_quad);
	hkVector4 armB;
	armB.m_quad = _mm_sub_ps(cp.m_position.m_quad, centerB.m_quad);
	hkVector4 velA;
	velA.setCross(deltaAngles[0], armA);
	hkVector4 negLinear;
	negLinear.setNeg4(in3.m_linearTimInfo);
	hkVector4 velB;
	velB.setCross(deltaAngles[1], armB);
	hkVector4 vel;
	vel.m_quad = _mm_sub_ps(_mm_add_ps(velA.m_quad, negLinear.m_quad), velB.m_quad);
	const hkQuadReal projected = _mm_mul_ps(vel.m_quad, cp.m_separatingNormal.m_quad);
	// EXE-INLINED 0x62B0A0
	float fProjectedVelocity = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(projected, projected, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(projected, projected, 0x55), projected)));

	// A welded contact that separates: sweep A's spheres against B's vertex plane again from the TOI, and take the
	// earliest point that reaches it.
	if (iWeldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED &&
		fProjectedVelocity > -1.1920929e-07f) {
		alignas(16) hkp4dGskVertexCollidePointsInput sweepIn2;
		sweepIn2.m_startRt.m_storage = fT;
		const float fDeltaTime2 = in3.m_input->m_stepInfo.m_deltaTime;
		const float fDeltaRtA2 = fDeltaTime2 * pMotionA->m_sweptTransform.m_centerOfMass1.m_quad[3];
		const float fDeltaRtB2 = fDeltaTime2 * pMotionB->m_sweptTransform.m_centerOfMass1.m_quad[3];
		sweepIn2.m_deltaAngles[0].m_quad = _mm_mul_ps(_mm_set_ps1(fDeltaRtA2), pMotionA->m_deltaAngle.m_quad);
		sweepIn2.m_deltaAngles[1].m_quad = _mm_mul_ps(_mm_set_ps1(fDeltaRtB2), pMotionB->m_deltaAngle.m_quad);
		hkVector4 crossA;
		crossA.setCross(sweepIn2.m_deltaAngles[0], cp.m_separatingNormal);
		const hkQuadReal sqA = _mm_mul_ps(crossA.m_quad, crossA.m_quad);
		// EXE-INLINED 0x630B40
		const float fLenA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqA, sqA, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(sqA, sqA, 0x55), sqA))));
		const float fAngA = pMotionA->m_objectRadius * fLenA;
		hkVector4 crossB;
		crossB.setCross(sweepIn2.m_deltaAngles[1], cp.m_separatingNormal);
		const hkQuadReal sqB = _mm_mul_ps(crossB.m_quad, crossB.m_quad);
		// EXE-INLINED 0x630B40
		const float fLenB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqB, sqB, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(sqB, sqB, 0x55), sqB))));
		const float fAngB = pMotionB->m_objectRadius * fLenB;
		const float fAngular = fAngA + fAngB;
		const hkQuadReal lin = _mm_mul_ps(in3.m_linearTimInfo.m_quad, cp.m_separatingNormal.m_quad);
		// EXE-INLINED 0x62B0A0
		const float fLinear =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(lin, lin, 0xAA), _mm_add_ss(_mm_shuffle_ps(lin, lin, 0x55), lin)));
		sweepIn2.m_worstCaseApproachingDelta.m_storage = fAngular + fLinear;
		const hkpConvexShape* pSweepShapeA = static_cast<const hkpConvexShape*>(in3.m_bodyA->m_shape);
		const hkpConvexShape* pSweepShapeB = static_cast<const hkpConvexShape*>(in3.m_bodyB->m_shape);
		sweepIn2.m_motionB.m_storage = in3.m_bodyB->m_motion.pState;
		sweepIn2.m_motionA.m_storage = in3.m_bodyA->m_motion.pState;
		sweepIn2.m_radiusSum.m_storage = pSweepShapeB->m_radius + pSweepShapeA->m_radius;
		sweepIn2.m_linearTimInfo = in3.m_linearTimInfo;
		sweepIn2.m_stepInfo.m_storage = &in3.m_input->m_stepInfo;
		const int32_t iNumSpheres = pSweepShapeA->getNumCollisionSpheresImpl();
		sweepIn2.m_allocatedNumVertices.m_storage = iNumSpheres;
		sweepIn2.m_numVertices.m_storage = iNumSpheres;
		// EXE-INLINED none: hkAllocateStack<hkVector4>
		hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
		void* pBlock = stack.m_next.m_storage;
		stack.m_next.m_storage =
			static_cast<char*>(pBlock) + ((iNumSpheres * static_cast<int32_t>(sizeof(hkVector4)) + 127) & ~127);
		sweepIn2.m_verticesA.m_storage = static_cast<hkVector4*>(pBlock);
		// The stack block is untyped memory the spheres are written to and read back as vectors.
		pSweepShapeA->getCollisionSpheresImpl(static_cast<hkSphere*>(pBlock));
		const float fAccelB = sweepIn2.m_motionB.m_storage->m_objectRadius * sweepIn2.m_deltaAngles[1].m_quad[3] *
			sweepIn2.m_deltaAngles[1].m_quad[3];
		const float fAccelA = sweepIn2.m_motionA.m_storage->m_objectRadius * sweepIn2.m_deltaAngles[0].m_quad[3] *
			sweepIn2.m_deltaAngles[0].m_quad[3];
		sweepIn2.m_maxAccel.m_storage = fAccelB + fAccelA;
		sweepIn2.m_invMaxAccel.m_storage = 1.0 / (sweepIn2.m_maxAccel.m_storage + 1.1920928955078125e-07);
		sweepIn2.m_planeB.setRotatedInverseDir(wTb.getRotation(), cp.m_separatingNormal);
		sweepIn2.m_pointOnPlaneB = gsk->m_verticesBinB[0];
		if (sweepIn2.m_verticesA.m_storage) {
			alignas(16) hkp4dGskVertexCollidePointsOutput sweepOut;
			sweepOut.m_Rtoi.m_storage = 1.0f;
			hk4dGskCollidePointsWithPlane(sweepIn2, *tol, sweepOut);
			// EXE-INLINED none: hkDeallocateStack<hkVector4>
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage =
				static_cast<char*>(static_cast<void*>(sweepIn2.m_verticesA.m_storage));
			if (1.0f > sweepOut.m_Rtoi.m_storage) {
				alignas(16) hkVector4 collidingPoint;
				collidingPoint.setTransformedPos(wTa, sweepOut.m_collidingPoint);
				cp.m_position = collidingPoint;
			}
		}
	}

	// The separating velocity is clamped to the approach the regula falsi measured.
	if (fProjectedVelocity > fApproachPerRt) {
		if (fProjectedVelocity * 1.2000000476837158 < fApproachPerRt) {
			fProjectedVelocity = fApproachPerRt;
		} else {
			fProjectedVelocity =
				hk4dGskCalcPointVelocityAssumingAngularVelError(deltaAngles, in3.m_bodyB->m_motion.pState,
					in3.m_bodyA->m_motion.pState, &cp.m_position, &cp.m_separatingNormal, &in3.m_linearTimInfo, fToi);
			if (fProjectedVelocity > 0.0) {
				fProjectedVelocity = 0.0f;
			}
		}
	}
	fProjectedVelocity = stepInfo.m_invDeltaTime * fProjectedVelocity;

	alignas(16) hkContactPointPropertiesWithExtendedUserData16 properties;
	properties.m_impulseApplied = 0.0f;
	properties.m_internalSolverData = 0.0f;
	properties.m_userData = 0;
	properties.m_friction.m_value = 0;
	properties.m_restitution = 0;
	properties.m_maxImpulse.m_value = 0;
	properties.m_flags = 0;
	properties.m_internalDataA = 321.0f;
	for (int32_t i = 0; i < 7; ++i) {
		properties.m_extendedUserDatas[i] = 0;
	}
	alignas(16) hkGskCache16 cache;
	gsk->exitAndExportCacheImpl(cache);
	if (in3.m_contactMgr->addToi(*in3.m_bodyA, *in3.m_bodyB, *in3.m_input, *out, fToi, cp, &cache, fProjectedVelocity,
			properties) == hkpContactMgr::TOI_ACCEPT) {
		if (out->m_toi.m_time.m_storage != 3.402820018375656e+38) {
			in3.m_contactMgr->removeToi(*out->m_constraintOwner.m_storage, out->m_toi.m_properties);
		}
		out->m_toi.m_seperatingVelocity.m_storage = fProjectedVelocity;
		out->m_toi.m_contactPoint.m_position = cp.m_position;
		out->m_toi.m_contactPoint.m_separatingNormal = cp.m_separatingNormal;
		out->m_toi.m_time.m_storage = fToi;
		out->m_toi.m_properties = properties;
		out->m_toi.m_gskCache = cache;
	}
}
