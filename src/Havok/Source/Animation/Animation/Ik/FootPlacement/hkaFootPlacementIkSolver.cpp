#include "Havok/Source/Animation/Animation/Ik/FootPlacement/hkaFootPlacementIkSolver.h"

#include "Havok/Source/Animation/Animation/Ik/TwoJoints/hkaTwoJointsIkSolver.h"
#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <xmmintrin.h>

hkBool hkaRaycastInterface::castRay(const hkVector4& a1, const hkVector4& a2, float& a3, hkVector4& a4) {
	hkBool result;
	result.m_bool = 0;
	return result;
}

hkaFootPlacementIkSolver::Input::Input()
	: m_footPlacementOn(true), m_raycastInterface(nullptr), m_onOffGain(0.2f), m_groundAscendingGain(1.0f),
	  m_groundDescendingGain(1.0f), m_footPlantedGain(1.0f), m_footRaisedGain(1.0f), m_footUnlockGain(1.0f),
	  m_collisionFilterInfo(0), m_useCollisionFilterInfo(false) {}

hkaFootPlacementIkSolver::Setup::Setup()
	: m_skeleton(nullptr), m_hipIndex(-1), m_kneeIndex(-1), m_ankleIndex(-1), m_kneeAxisLS(hkVector4Zero),
	  m_footEndLS(hkVector4Zero), m_worldUpDirectionWS(hkVector4Zero), m_modelUpDirectionMS(hkVector4Zero),
	  m_originalGroundHeightMS(0.0f), m_footPlantedAnkleHeightMS(0.0f), m_footRaisedAnkleHeightMS(0.0f),
	  m_maxAnkleHeightMS(0.7f), m_minAnkleHeightMS(-0.1f), m_cosineMaxKneeAngle(-1.0f), m_cosineMinKneeAngle(1.0f),
	  m_raycastDistanceUp(0.5f), m_raycastDistanceDown(0.8f), m_useFootLocking(false) {}

hkaFootPlacementIkSolver::hkaFootPlacementIkSolver(const hkaFootPlacementIkSolver::Setup& setup)
	: m_setup(setup), m_currentWeight(0.0f), m_previousGroundHeightWS(0.0f),
	  m_previousGroundNormalWS(setup.m_worldUpDirectionWS), m_previousVerticalDisplacement(0.0f), m_isFootLocked(false),
	  m_lockedFootPositionInWS(hkVector4Zero), m_lockedFootEndPositionInWS(hkVector4Zero),
	  m_footUnlockingOffset(hkVector4Zero) {}

// The solver has nothing of its own to release.
hkaFootPlacementIkSolver::~hkaFootPlacementIkSolver() {}

// Casts a ray down through the foot and, unless the foot end is the same point, one through the foot end, and reports
// the nearer ground hit.
// KEEP: kFootEndNormalWS is read uninitialised when the second cast is skipped and the first one's hit fraction
// exceeds 3.40282e+38f (an uninitialised read on a path, however unlikely: .omp/rules/porting.md "Bodies").
bool hkaFootPlacementIkSolver::castFoot(const hkaFootPlacementIkSolver::Input& input, const hkVector4& footWS,
	const hkVector4& footEndWS, hkVector4& projectedFootWSOut, hkVector4& groundNormalWSOut) const {
	const float fDistanceUp = m_setup.m_raycastDistanceUp;
	const float fDistanceDown = m_setup.m_raycastDistanceDown;
	const float fTotalDistance = fDistanceDown + fDistanceUp;
	float fFootHitFraction = 3.40282e+38f;
	const float fNegDistanceDown = -fDistanceDown;

	hkVector4 kFromWS;
	hkVector4 kToWS;
	hkVector4 kFootNormalWS;
	hkVector4 kFootEndNormalWS;

	hkVector4 kUpWS;
	kUpWS.m_quad = m_setup.m_worldUpDirectionWS.m_quad;
	kToWS.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fNegDistanceDown), _mm_set_ss(fNegDistanceDown), 0),
								  kUpWS.m_quad),
		footWS.m_quad);
	kFromWS.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fDistanceUp), _mm_set_ss(fDistanceUp), 0), kUpWS.m_quad),
			footWS.m_quad);
	hkaRaycastInterface* pRaycastInterface = input.m_raycastInterface;
	bool bFootHit;
	if (input.m_useCollisionFilterInfo) {
		bFootHit =
			pRaycastInterface->castRay(kFromWS, kToWS, input.m_collisionFilterInfo, fFootHitFraction, kFootNormalWS)
				.m_bool != 0;
	} else {
		bFootHit = pRaycastInterface->castRay(kFromWS, kToWS, fFootHitFraction, kFootNormalWS).m_bool != 0;
	}

	float fFootEndHitFraction = 3.40282e+38f;
	bool bFootEndHit = false;
	// The second cast only where the foot end differs from the foot by more than 0.001 on x, y or z.
	if ((_mm_movemask_ps(_mm_cmple_ps(_mm_and_ps(_mm_sub_ps(footEndWS.m_quad, footWS.m_quad),
										  hkVector4_setAbs4_absMask),
			 _mm_shuffle_ps(_mm_set_ss(0.001f), _mm_set_ss(0.001f), 0))) &
			7) != 7) {
		float fEndHitFraction = fNegDistanceDown;
		kUpWS.m_quad = m_setup.m_worldUpDirectionWS.m_quad;
		pRaycastInterface = input.m_raycastInterface;
		kToWS.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fEndHitFraction), _mm_set_ss(fEndHitFraction), 0),
						   kUpWS.m_quad),
				footEndWS.m_quad);
		kFromWS.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fDistanceUp), _mm_set_ss(fDistanceUp), 0), kUpWS.m_quad),
				footEndWS.m_quad);
		if (input.m_useCollisionFilterInfo) {
			bFootEndHit = pRaycastInterface
							  ->castRay(kFromWS, kToWS, input.m_collisionFilterInfo, fEndHitFraction, kFootEndNormalWS)
							  .m_bool != 0;
		} else {
			bFootEndHit = pRaycastInterface->castRay(kFromWS, kToWS, fEndHitFraction, kFootEndNormalWS).m_bool != 0;
		}
		if (bFootEndHit) {
			kUpWS.m_quad = m_setup.m_worldUpDirectionWS.m_quad;
			const __m128 kUpDot = _mm_mul_ps(kUpWS.m_quad, kFootEndNormalWS.m_quad);
			const float fUpDot = _mm_shuffle_ps(kUpDot, kUpDot, 170)[0] +
				static_cast<float>(_mm_shuffle_ps(kUpDot, kUpDot, 85)[0] + kUpDot[0]);
			if (fUpDot <= 0.0f) {
				bFootEndHit = false;
			} else {
				// The foot end's hit point, and the plane through it, met by the vertical through the foot.
				const __m128 kHitDot = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fEndHitFraction),
																			_mm_set_ss(fEndHitFraction), 0),
																 _mm_sub_ps(kToWS.m_quad, kFromWS.m_quad)),
													  kFromWS.m_quad),
					kFootEndNormalWS.m_quad);
				const float fNegHitDot = 0.0f -
					static_cast<float>(_mm_shuffle_ps(kHitDot, kHitDot, 170)[0] +
						static_cast<float>(_mm_shuffle_ps(kHitDot, kHitDot, 85)[0] + kHitDot[0]));
				const __m128 kFromDot = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fDistanceUp),
																			 _mm_set_ss(fDistanceUp), 0),
																  kUpWS.m_quad),
													   footWS.m_quad),
					kFootEndNormalWS.m_quad);
				const float fFromDot = _mm_shuffle_ps(kFromDot, kFromDot, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(kFromDot, kFromDot, 85)[0] + kFromDot[0]);
				const float fNumerator = -(fFromDot + fNegHitDot);
				const float fDenominator = fUpDot * -fTotalDistance;
				fFootEndHitFraction = fNumerator / fDenominator;
			}
		}
	}

	float fHitFraction = fFootEndHitFraction;
	if (bFootHit) {
		if (fFootHitFraction > fFootEndHitFraction) {
			groundNormalWSOut.m_quad = kFootEndNormalWS.m_quad;
		} else {
			fHitFraction = fFootHitFraction;
			groundNormalWSOut.m_quad = kFootNormalWS.m_quad;
		}
	} else if (bFootEndHit) {
		groundNormalWSOut.m_quad = kFootEndNormalWS.m_quad;
	} else {
		return false;
	}
	float fOffset = fHitFraction * fTotalDistance - fDistanceUp;
	fOffset = -fOffset;
	projectedFootWSOut.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fOffset), _mm_set_ss(fOffset), 0),
											   m_setup.m_worldUpDirectionWS.m_quad),
		footWS.m_quad);
	return true;
}

void hkaFootPlacementIkSolver::doFootPlacement(const hkaFootPlacementIkSolver::Input& input,
	hkaFootPlacementIkSolver::Output& output, hkaPose& poseInOut) {
	hkQuaternion v5;
	hkVector4 v14;
	hkVector4 v20;
	const hkQsTransform* v25;
	hkVector4 v27;
	__m128 v36;
	double v37;
	bool v38;
	bool v39;
	__m128 m_quad_5;
	__m128 v64;
	__m128 v65;
	__m128 v66;
	__m128 m_quad_6;
	__m128 v68;
	__m128 v69;
	hkVector4 v70;
	double m_currentWeight_1;
	__m128 projectedFootWSOut__1;
	__m128 m_quad_7;
	__m128 v100;
	float v101;
	float v102;
	__m128 v103;
	double m_previousGroundHeightWS;
	hkVector4 v106;
	double m_footPlantedAnkleHeightMS_1;
	__m128 v108;
	double v109;
	__m128 v111;
	__m128 v112;
	__m128 v113;
	__m128 v114;
	__m128 v115;
	__m128 v117;
	__m128 m_quad_8;
	hkVector4 v119;
	__m128 v120;
	double m_maxAnkleHeightMS_1;
	char m_bool;
	__m128 v129;
	hkVector4 v147;
	float m_previousGroundHeightWS_1;
	float v150;
	float m_maxAnkleHeightMS;
	float m_currentWeight_3;
	float v155;
	float v157;
	float v158;
	float v159;
	hkBool apResult;
	float m_minAnkleHeightMS;
	hkVector4 footEndWS;
	hkVector4 m_lockedFootPositionInWS;
	hkVector4 projectedFootWSOut_;
	hkVector4 groundNormalWSOut;
	hkQuaternion v170;
	hkQuaternion q0_;
	hkVector4 from;
	v5.m_vec.m_quad = input.m_worldFromModel.m_rotation.m_vec.m_quad;
	__m128 v6 = _mm_mul_ps(input.m_worldFromModel.m_translation.m_quad, v5.m_vec.m_quad);
	__m128 v7 = _mm_shuffle_ps(v6, v6, 170);
	v7[0] = v7[0] + static_cast<float>(_mm_shuffle_ps(v6, v6, 85)[0] + v6[0]);
	__m128 v8 = _mm_shuffle_ps(v5.m_vec.m_quad, v5.m_vec.m_quad, 255);
	__m128 v9 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(input.m_worldFromModel.m_translation.m_quad,
																input.m_worldFromModel.m_translation.m_quad, 201),
													 _mm_shuffle_ps(v5.m_vec.m_quad, v5.m_vec.m_quad, 210)),
										  _mm_mul_ps(_mm_shuffle_ps(input.m_worldFromModel.m_translation.m_quad,
														 input.m_worldFromModel.m_translation.m_quad, 210),
											  _mm_shuffle_ps(v5.m_vec.m_quad, v5.m_vec.m_quad, 201))),
							   v8),
		_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v8, v8), hkQuadRealMinusHalf),
					   input.m_worldFromModel.m_translation.m_quad),
			_mm_mul_ps(_mm_shuffle_ps(v7, v7, 0), v5.m_vec.m_quad)));
	__m128 v172 = _mm_xor_ps(_mm_add_ps(v9, v9), hkQuadRealNegateMask);
	__m128 v10 = _mm_xor_ps(input.m_worldFromModel.m_rotation.m_vec.m_quad, rdata_10C4C00);
	__m128 v11 = _mm_shuffle_ps(input.m_worldFromModel.m_scale.m_quad,
		_mm_unpackhi_ps(input.m_worldFromModel.m_scale.m_quad, hkQuadReal1111), 196);
	__m128 v12 = _mm_rcp_ps(v11);
	__m128 v13 = _mm_and_ps(_mm_sub_ps(_mm_add_ps(v12, v12), _mm_mul_ps(_mm_mul_ps(v12, v11), v12)), rdata_10C4C10);
	v14.m_quad = m_setup.m_worldUpDirectionWS.m_quad;
	__m128 v15 = _mm_mul_ps(v10, v14.m_quad);
	__m128 v16 = _mm_shuffle_ps(v15, v15, 170);
	v16[0] = v16[0] + static_cast<float>(_mm_shuffle_ps(v15, v15, 85)[0] + v15[0]);
	__m128 v17 = _mm_shuffle_ps(v10, v10, 255);
	__m128 v173 = v10;
	__m128 v174 = v13;
	__m128 v168 = v17;
	__m128 v171 = _mm_shuffle_ps(v10, v10, 201);
	__m128 v169 = _mm_shuffle_ps(v10, v10, 210);
	hkaPose* poseInOut_1 = &poseInOut;
	__m128 v19 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v14.m_quad, v14.m_quad, 210), v171),
										   _mm_mul_ps(_mm_shuffle_ps(v14.m_quad, v14.m_quad, 201), v169)),
								v17),
		_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v17, v17), hkQuadRealMinusHalf), v14.m_quad),
			_mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), v10)));
	output.m_hitSomething = 1;
	v20.m_quad = m_setup.m_modelUpDirectionMS.m_quad;
	uint32_t* m_data = poseInOut.m_boneFlags.begin();
	__m128 v178 = _mm_add_ps(v19, v19);
	__m128 v22 = _mm_mul_ps(input.m_originalAnkleTransformMS.m_translation.m_quad, v20.m_quad);
	const hkQuaternion& kOriginalAnkleRotationMS = input.m_originalAnkleTransformMS.m_rotation;
	int32_t m_ankleIndex = m_setup.m_ankleIndex;
	float m_footPlantedAnkleHeightMS =
		_mm_shuffle_ps(v22, v22, 170)[0] + static_cast<float>(_mm_shuffle_ps(v22, v22, 85)[0] + v22[0]);
	if ((m_data[m_ankleIndex] & 2) != 0) {
		poseInOut.calculateBoneModelSpace(m_ankleIndex);
		v17 = v168;
		v13 = v174;
		v10 = v173;
		poseInOut_1 = &poseInOut;
	}
	int32_t m_ankleIndex_1 = m_setup.m_ankleIndex;
	if ((poseInOut_1->m_boneFlags.m_data[m_ankleIndex_1] & 2) != 0) {
		v25 = &poseInOut_1->calculateBoneModelSpace(m_ankleIndex_1);
		v17 = v168;
		v13 = v174;
		v10 = v173;
	} else {
		v25 = &poseInOut_1->m_modelPose.m_data[m_ankleIndex_1];
	}
	__m128 m_quad = v25->m_translation.m_quad;
	__m128 m_quad_1 = v25->m_scale.m_quad;
	v27.m_quad = input.m_worldFromModel.m_scale.m_quad;
	__m128 m_quad_2 = v25->m_rotation.m_vec.m_quad;
	char m_footPlacementOn = input.m_footPlacementOn;
	__m128 m_quad_3 = m_quad;
	__m128 v30 = _mm_mul_ps(m_quad, v27.m_quad);
	footEndWS.m_quad = input.m_worldFromModel.m_rotation.m_vec.m_quad;
	__m128 v31 = _mm_mul_ps(footEndWS.m_quad, v30);
	projectedFootWSOut_.m_quad = _mm_shuffle_ps(footEndWS.m_quad, footEndWS.m_quad, 255);
	float v32 = _mm_shuffle_ps(v31, v31, 85)[0] + v31[0];
	__m128 kRotatedDot = _mm_shuffle_ps(v31, v31, 170);
	kRotatedDot[0] = kRotatedDot[0] + v32;
	m_lockedFootPositionInWS.m_quad = kRotatedDot;
	__m128 v34 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(footEndWS.m_quad, footEndWS.m_quad, 201),
													  _mm_shuffle_ps(v30, v30, 210)),
										   _mm_mul_ps(_mm_shuffle_ps(footEndWS.m_quad, footEndWS.m_quad, 210),
											   _mm_shuffle_ps(v30, v30, 201))),
								projectedFootWSOut_.m_quad),
		_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(projectedFootWSOut_.m_quad, projectedFootWSOut_.m_quad),
								  hkQuadRealMinusHalf),
					   v30),
			_mm_mul_ps(_mm_shuffle_ps(m_lockedFootPositionInWS.m_quad, m_lockedFootPositionInWS.m_quad, 0),
				footEndWS.m_quad)));
	__m128 m_lockedFootPositionInWS_2 = _mm_add_ps(_mm_add_ps(v34, v34), input.m_worldFromModel.m_translation.m_quad);
	m_lockedFootPositionInWS.m_quad = m_lockedFootPositionInWS_2;
	apResult.m_bool = m_footPlacementOn;
	if (!m_footPlacementOn) {
		output.m_hitSomething = 0;
		groundNormalWSOut.m_quad = m_previousGroundNormalWS.m_quad;
		footEndWS.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(this->m_previousVerticalDisplacement),
													 _mm_set_ss(this->m_previousVerticalDisplacement), 0),
										  v178),
			m_quad_3);
		v36 = _mm_mul_ps(m_setup.m_worldUpDirectionWS.m_quad, m_lockedFootPositionInWS_2);
		v37 = static_cast<float>(_mm_shuffle_ps(v36, v36, 170)[0] +
				  static_cast<float>(_mm_shuffle_ps(v36, v36, 85)[0] + v36[0])) +
			this->m_previousVerticalDisplacement;
	LABEL_21:;
		m_previousGroundHeightWS_1 = v37 - m_footPlantedAnkleHeightMS;
		m_currentWeight_1 = 0.0;
		output.m_verticalError = 0.0f;
		m_isFootLocked.m_bool = 0;
		goto LABEL_48;
	}
	v38 = m_isFootLocked.m_bool != 0;
	v39 = !m_setup.m_useFootLocking;
	if (v39 ||
		(m_isFootLocked.m_bool = 0,
			!(m_setup.m_footPlantedAnkleHeightMS > static_cast<double>(m_footPlantedAnkleHeightMS)))) {
		m_quad_5 = m_quad_3;
	} else {
		if (v38) {
			__m128 v40 = _mm_mul_ps(v13, this->m_lockedFootPositionInWS.m_quad);
			m_lockedFootPositionInWS.m_quad = this->m_lockedFootPositionInWS.m_quad;
			__m128 v41 = _mm_mul_ps(v40, v10);
			__m128 v42 = _mm_shuffle_ps(v41, v41, 170);
			v42[0] = v42[0] + static_cast<float>(_mm_shuffle_ps(v41, v41, 85)[0] + v41[0]);
			__m128 v43 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v40, v40, 210), v171),
												   _mm_mul_ps(_mm_shuffle_ps(v40, v40, 201), v169)),
										v17),
				_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v17, v17), hkQuadRealMinusHalf), v40),
					_mm_mul_ps(_mm_shuffle_ps(v42, v42, 0), v10)));
			m_quad_5 = _mm_add_ps(_mm_add_ps(v43, v43), v172);
			m_quad_3 = m_quad_5;
		} else {
			hkQuaternion v54;
			hkVector4 v60;
			this->m_lockedFootPositionInWS.m_quad = m_lockedFootPositionInWS_2;
			footEndWS.m_quad = m_setup.m_footEndLS.m_quad;
			__m128 v48 = _mm_mul_ps(footEndWS.m_quad, m_quad_1);
			__m128 v49 = _mm_mul_ps(v48, m_quad_2);
			__m128 v50 = _mm_shuffle_ps(v49, v49, 170);
			v50[0] = v50[0] + static_cast<float>(_mm_shuffle_ps(v49, v49, 85)[0] + v49[0]);
			__m128 v51 = _mm_shuffle_ps(m_quad_2, m_quad_2, 255);
			__m128 v52 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v48, v48, 210),
															  _mm_shuffle_ps(m_quad_2, m_quad_2, 201)),
												   _mm_mul_ps(_mm_shuffle_ps(v48, v48, 201),
													   _mm_shuffle_ps(m_quad_2, m_quad_2, 210))),
										v51),
				_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v51, v51), hkQuadRealMinusHalf), v48),
					_mm_mul_ps(_mm_shuffle_ps(v50, v50, 0), m_quad_2)));
			__m128 v53 = _mm_mul_ps(_mm_add_ps(_mm_add_ps(v52, v52), m_quad_3), input.m_worldFromModel.m_scale.m_quad);
			v54.m_vec.m_quad = input.m_worldFromModel.m_rotation.m_vec.m_quad;
			__m128 v56 = _mm_mul_ps(v53, v54.m_vec.m_quad);
			__m128 v57 = _mm_shuffle_ps(v56, v56, 170);
			v57[0] = v57[0] + static_cast<float>(_mm_shuffle_ps(v56, v56, 85)[0] + v56[0]);
			__m128 v58 = _mm_shuffle_ps(v54.m_vec.m_quad, v54.m_vec.m_quad, 255);
			__m128 v59 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v53, v53, 210),
															  _mm_shuffle_ps(v54.m_vec.m_quad, v54.m_vec.m_quad, 201)),
												   _mm_mul_ps(_mm_shuffle_ps(v53, v53, 201),
													   _mm_shuffle_ps(v54.m_vec.m_quad, v54.m_vec.m_quad, 210))),
										v58),
				_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v58, v58), hkQuadRealMinusHalf), v53),
					_mm_mul_ps(_mm_shuffle_ps(v57, v57, 0), v54.m_vec.m_quad)));
			v60.m_quad = _mm_add_ps(v59, v59);
			m_lockedFootEndPositionInWS.m_quad = v60.m_quad;
			m_quad_5 = m_quad_3;
			m_lockedFootEndPositionInWS.m_quad = _mm_add_ps(v60.m_quad, input.m_worldFromModel.m_translation.m_quad);
		}
		m_isFootLocked.m_bool = 1;
	}
	v39 = !m_setup.m_useFootLocking;
	footEndWS.m_quad = m_setup.m_footEndLS.m_quad;
	v64 = _mm_mul_ps(footEndWS.m_quad, m_quad_1);
	v65 = _mm_mul_ps(v64, m_quad_2);
	v66 = _mm_shuffle_ps(v65, v65, 170);
	v66[0] = v66[0] + static_cast<float>(_mm_shuffle_ps(v65, v65, 85)[0] + v65[0]);
	q0_.m_vec.m_quad = _mm_shuffle_ps(m_quad_2, m_quad_2, 210);
	m_quad_6 = _mm_shuffle_ps(m_quad_2, m_quad_2, 255);
	from.m_quad = _mm_shuffle_ps(m_quad_2, m_quad_2, 201);
	v68 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v64, v64, 210), from.m_quad),
									_mm_mul_ps(_mm_shuffle_ps(v64, v64, 201), q0_.m_vec.m_quad)),
						 m_quad_6),
		_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(m_quad_6, m_quad_6), hkQuadRealMinusHalf), v64),
			_mm_mul_ps(_mm_shuffle_ps(v66, v66, 0), m_quad_2)));
	v170.m_vec.m_quad = m_quad_6;
	v69 = _mm_add_ps(_mm_add_ps(v68, v68), m_quad_5);
	if (v39 || !m_isFootLocked.m_bool) {
		hkQuaternion v72;
		__m128 v71 = _mm_mul_ps(v69, input.m_worldFromModel.m_scale.m_quad);
		v72.m_vec.m_quad = input.m_worldFromModel.m_rotation.m_vec.m_quad;
		__m128 v73 = _mm_mul_ps(v71, v72.m_vec.m_quad);
		__m128 v74 = _mm_shuffle_ps(v73, v73, 170);
		v74[0] = v74[0] + static_cast<float>(_mm_shuffle_ps(v73, v73, 85)[0] + v73[0]);
		__m128 v75 = _mm_shuffle_ps(v72.m_vec.m_quad, v72.m_vec.m_quad, 255);
		__m128 v76 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v71, v71, 210),
														  _mm_shuffle_ps(v72.m_vec.m_quad, v72.m_vec.m_quad, 201)),
											   _mm_mul_ps(_mm_shuffle_ps(v71, v71, 201),
												   _mm_shuffle_ps(v72.m_vec.m_quad, v72.m_vec.m_quad, 210))),
									v75),
			_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v75, v75), hkQuadRealMinusHalf), v71),
				_mm_mul_ps(_mm_shuffle_ps(v74, v74, 0), v72.m_vec.m_quad)));
		v70.m_quad = _mm_add_ps(_mm_add_ps(v76, v76), input.m_worldFromModel.m_translation.m_quad);
	} else {
		v70.m_quad = m_lockedFootEndPositionInWS.m_quad;
	}
	footEndWS.m_quad = v70.m_quad;
	if (!castFoot(input, m_lockedFootPositionInWS, footEndWS, projectedFootWSOut_, groundNormalWSOut)) {
		output.m_hitSomething = 0;
		double m_previousVerticalDisplacement = this->m_previousVerticalDisplacement;
		groundNormalWSOut.m_quad = m_previousGroundNormalWS.m_quad;
		footEndWS.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(this->m_previousVerticalDisplacement),
													 _mm_set_ss(this->m_previousVerticalDisplacement), 0),
										  v178),
			m_quad_3);
		__m128 v78 = _mm_mul_ps(m_setup.m_worldUpDirectionWS.m_quad, m_lockedFootPositionInWS.m_quad);
		v37 = m_previousVerticalDisplacement +
			static_cast<float>(_mm_shuffle_ps(v78, v78, 170)[0] +
				static_cast<float>(_mm_shuffle_ps(v78, v78, 85)[0] + v78[0]));
		goto LABEL_21;
	}
	projectedFootWSOut__1 = projectedFootWSOut_.m_quad;
	if (m_isFootLocked.m_bool) {
		hkQuaternion v93;
		hkVector4 v99;
		__m128 v82 = _mm_mul_ps(v174, projectedFootWSOut_.m_quad);
		this->m_lockedFootPositionInWS.m_quad = projectedFootWSOut_.m_quad;
		m_lockedFootPositionInWS.m_quad = projectedFootWSOut__1;
		__m128 v85 = _mm_mul_ps(v173, v82);
		__m128 v86 = _mm_shuffle_ps(v85, v85, 170);
		v86[0] = v86[0] + static_cast<float>(_mm_shuffle_ps(v85, v85, 85)[0] + v85[0]);
		__m128 v87 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v82, v82, 210), v171),
											   _mm_mul_ps(_mm_shuffle_ps(v82, v82, 201), v169)),
									v168),
			_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v168, v168), hkQuadRealMinusHalf), v82),
				_mm_mul_ps(_mm_shuffle_ps(v86, v86, 0), v173)));
		m_quad_7 = _mm_add_ps(_mm_add_ps(v87, v87), v172);
		footEndWS.m_quad = m_setup.m_footEndLS.m_quad;
		__m128 v89 = _mm_mul_ps(m_quad_1, footEndWS.m_quad);
		__m128 v90 = _mm_mul_ps(m_quad_2, v89);
		__m128 v91 = _mm_shuffle_ps(v90, v90, 170);
		v91[0] = v91[0] + static_cast<float>(_mm_shuffle_ps(v90, v90, 85)[0] + v90[0]);
		__m128 v92 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v89, v89, 210), from.m_quad),
											   _mm_mul_ps(_mm_shuffle_ps(v89, v89, 201), q0_.m_vec.m_quad)),
									v170.m_vec.m_quad),
			_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v170.m_vec.m_quad, v170.m_vec.m_quad), hkQuadRealMinusHalf),
						   v89),
				_mm_mul_ps(_mm_shuffle_ps(v91, v91, 0), m_quad_2)));
		v93.m_vec.m_quad = input.m_worldFromModel.m_rotation.m_vec.m_quad;
		__m128 v94 = _mm_mul_ps(_mm_add_ps(_mm_add_ps(v92, v92), m_quad_7), input.m_worldFromModel.m_scale.m_quad);
		__m128 v95 = _mm_shuffle_ps(v93.m_vec.m_quad, v93.m_vec.m_quad, 255);
		__m128 v96 = _mm_mul_ps(v93.m_vec.m_quad, v94);
		__m128 v97 = _mm_shuffle_ps(v96, v96, 170);
		v97[0] = v97[0] + static_cast<float>(_mm_shuffle_ps(v96, v96, 85)[0] + v96[0]);
		__m128 v98 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v94, v94, 210),
														  _mm_shuffle_ps(v93.m_vec.m_quad, v93.m_vec.m_quad, 201)),
											   _mm_mul_ps(_mm_shuffle_ps(v94, v94, 201),
												   _mm_shuffle_ps(v93.m_vec.m_quad, v93.m_vec.m_quad, 210))),
									v95),
			_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v95, v95), hkQuadRealMinusHalf), v94),
				_mm_mul_ps(_mm_shuffle_ps(v97, v97, 0), v93.m_vec.m_quad)));
		v99.m_quad = _mm_add_ps(v98, v98);
		m_lockedFootEndPositionInWS.m_quad = v99.m_quad;
		projectedFootWSOut__1 = projectedFootWSOut_.m_quad;
		m_lockedFootEndPositionInWS.m_quad = _mm_add_ps(v99.m_quad, input.m_worldFromModel.m_translation.m_quad);
	} else {
		m_quad_7 = m_quad_3;
	}
	output.m_hitSomething = 1;
	v100 = _mm_mul_ps(m_setup.m_worldUpDirectionWS.m_quad, projectedFootWSOut__1);
	v101 = _mm_shuffle_ps(v100, v100, 85)[0] + v100[0];
	v102 = _mm_shuffle_ps(v100, v100, 170)[0];
	v103 = _mm_mul_ps(m_setup.m_worldUpDirectionWS.m_quad, input.m_worldFromModel.m_translation.m_quad);
	m_previousGroundHeightWS = static_cast<float>(v102 + v101);
	output.m_verticalError = m_previousGroundHeightWS -
		(m_setup.m_originalGroundHeightMS +
			static_cast<float>(_mm_shuffle_ps(v103, v103, 170)[0] +
				static_cast<float>(_mm_shuffle_ps(v103, v103, 85)[0] + v103[0])));
	v106.m_quad = m_setup.m_worldUpDirectionWS.m_quad;
	groundNormalWSOut.m_quad[3] = 0.0;
	v155 = -m_previousGroundHeightWS;
	m_footPlantedAnkleHeightMS_1 = m_footPlantedAnkleHeightMS;
	v108 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v155), _mm_set_ss(v155), 0), v106.m_quad),
		projectedFootWSOut__1);
	v150 = 1.0 -
		(m_footPlantedAnkleHeightMS - m_setup.m_footPlantedAnkleHeightMS) /
			(m_setup.m_footRaisedAnkleHeightMS - m_setup.m_footPlantedAnkleHeightMS);
	v109 = v150;
	if (v150 < 0.0) {
		v109 = static_cast<float>(0.0);
	}
	if (v109 > 1.0) {
		v109 = 1.0f;
	}
	if (0.0 != this->m_currentWeight) {
		double m_groundAscendingGain;
		if (!(this->m_previousGroundHeightWS > m_previousGroundHeightWS)) {
			m_groundAscendingGain = input.m_groundAscendingGain;
		} else {
			m_groundAscendingGain = input.m_groundDescendingGain;
		}
		float m_groundAscendingGain_1 = m_groundAscendingGain;
		float v151 = v109 * input.m_footPlantedGain + (1.0 - v109) * input.m_footRaisedGain;
		float v156 = m_groundAscendingGain_1 * v151;
		m_previousGroundHeightWS =
			(m_previousGroundHeightWS - this->m_previousGroundHeightWS) * v156 + this->m_previousGroundHeightWS;
	}
	m_previousGroundHeightWS_1 = m_previousGroundHeightWS;
	if (m_setup.m_useFootLocking) {
		if (m_isFootLocked.m_bool) {
			m_footUnlockingOffset.m_quad = _mm_setzero_ps();
		} else {
			if (v38) {
				hkVector4 v122;
				v122.m_quad = _mm_sub_ps(this->m_lockedFootPositionInWS.m_quad, v108);
				m_footUnlockingOffset.m_quad = v122.m_quad;
				__m128 v123 = _mm_mul_ps(v106.m_quad, v122.m_quad);
				v122.m_quad[0] = _mm_shuffle_ps(v123, v123, 85)[0] + v123[0];
				__m128 v124 = _mm_shuffle_ps(v123, v123, 170);
				v124[0] = v124[0] + v122.m_quad[0];
				m_footUnlockingOffset.m_quad =
					_mm_sub_ps(m_footUnlockingOffset.m_quad, _mm_mul_ps(_mm_shuffle_ps(v124, v124, 0), v106.m_quad));
			}
			v108 = _mm_add_ps(v108, m_footUnlockingOffset.m_quad);
			v159 = 1.0 - input.m_footUnlockGain;
			m_footUnlockingOffset.m_quad =
				_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v159), _mm_set_ss(v159), 0), m_footUnlockingOffset.m_quad);
		}
	}
	v157 = m_footPlantedAnkleHeightMS_1 - m_setup.m_originalGroundHeightMS;
	v111 = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(m_previousGroundHeightWS_1),
												_mm_set_ss(m_previousGroundHeightWS_1), 0),
									 m_setup.m_worldUpDirectionWS.m_quad),
						  v108),
		v174);
	m_maxAnkleHeightMS = m_setup.m_maxAnkleHeightMS;
	v112 = _mm_mul_ps(v173, v111);
	m_minAnkleHeightMS = m_setup.m_minAnkleHeightMS;
	v113 = _mm_shuffle_ps(v112, v112, 170);
	v113[0] = v113[0] + static_cast<float>(_mm_shuffle_ps(v112, v112, 85)[0] + v112[0]);
	v114 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v111, v111, 210), v171),
						  _mm_mul_ps(_mm_shuffle_ps(v111, v111, 201), v169)),
		v168);
	v115 = _mm_add_ps(_mm_mul_ps(v168, v168), hkQuadRealMinusHalf);
	v117 = _mm_add_ps(v114, _mm_add_ps(_mm_mul_ps(v115, v111), _mm_mul_ps(_mm_shuffle_ps(v113, v113, 0), v173)));
	m_quad_8 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v157), _mm_set_ss(v157), 0), v178),
		_mm_add_ps(_mm_add_ps(v117, v117), v172));
	v119.m_quad = m_setup.m_modelUpDirectionMS.m_quad;
	v120 = _mm_mul_ps(m_quad_8, v119.m_quad);
	m_maxAnkleHeightMS_1 = static_cast<float>(_mm_shuffle_ps(v120, v120, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v120, v120, 85)[0] + v120[0]));
	footEndWS.m_quad = m_quad_8;
	if (m_maxAnkleHeightMS < m_maxAnkleHeightMS_1) {
		v158 = m_maxAnkleHeightMS - m_maxAnkleHeightMS_1;
		m_quad_8 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v158), _mm_set_ss(v158), 0), v119.m_quad), m_quad_8);
		footEndWS.m_quad = m_quad_8;
	}
	if (!(m_minAnkleHeightMS > m_maxAnkleHeightMS_1)) {
		m_bool = apResult.m_bool;
	} else {
		output.m_hitSomething = 0;
		__m128 v125 = _mm_mul_ps(m_setup.m_worldUpDirectionWS.m_quad, m_lockedFootPositionInWS.m_quad);
		groundNormalWSOut.m_quad = m_previousGroundNormalWS.m_quad;
		__m128 v126 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(this->m_previousVerticalDisplacement),
									 _mm_set_ss(this->m_previousVerticalDisplacement), 0),
			v178);
		m_minAnkleHeightMS =
			_mm_shuffle_ps(v125, v125, 170)[0] + static_cast<float>(_mm_shuffle_ps(v125, v125, 85)[0] + v125[0]);
		double v127 = m_minAnkleHeightMS + this->m_previousVerticalDisplacement;
		m_quad_8 = _mm_add_ps(v126, m_quad_7);
		m_bool = 0;
		footEndWS.m_quad = m_quad_8;
		m_previousGroundHeightWS_1 = v127 - m_footPlantedAnkleHeightMS_1;
		output.m_verticalError = 0.0;
	}
	v129 = _mm_mul_ps(_mm_sub_ps(m_quad_8, m_quad_7), m_setup.m_modelUpDirectionMS.m_quad);
	m_minAnkleHeightMS =
		_mm_shuffle_ps(v129, v129, 170)[0] + static_cast<float>(_mm_shuffle_ps(v129, v129, 85)[0] + v129[0]);
	this->m_previousVerticalDisplacement = m_minAnkleHeightMS;
	if (m_bool) {
		m_currentWeight_1 = 0.0;
		m_currentWeight_3 = 1.0f;
		goto LABEL_49;
	}
	m_currentWeight_1 = 0.0;
LABEL_48:;
	m_currentWeight_3 = m_currentWeight_1;
LABEL_49:;
	m_minAnkleHeightMS = (m_currentWeight_3 - this->m_currentWeight) * input.m_onOffGain + this->m_currentWeight;
	double m_minAnkleHeightMS_1 = m_minAnkleHeightMS;
	this->m_currentWeight = m_minAnkleHeightMS;
	if (m_minAnkleHeightMS_1 < 0.01f) {
		this->m_currentWeight = m_currentWeight_1;
	}
	if (!(m_currentWeight_1 < this->m_currentWeight)) {
		output.m_verticalError = m_currentWeight_1;
	} else {
		hkVector4 v133;
		hkVector4 v137;
		hkaTwoJointsIkSolver::Setup apMemory;
		int16_t m_hipIndex = m_setup.m_hipIndex;
		apMemory.m_cosineMinHingeAngle = m_setup.m_cosineMinKneeAngle;
		int16_t m_kneeIndex = m_setup.m_kneeIndex;
		apMemory.m_cosineMaxHingeAngle = m_setup.m_cosineMaxKneeAngle;
		v133.m_quad = m_setup.m_kneeAxisLS.m_quad;
		int16_t m_ankleIndex_2 = m_setup.m_ankleIndex;
		apMemory.m_maxHingeAngleDifference = this->m_currentWeight;
		double m_currentWeight = this->m_currentWeight;
		apMemory.m_firstJointIdx = m_hipIndex;
		apMemory.m_firstJointIkGain = m_currentWeight;
		double m_currentWeight_2 = this->m_currentWeight;
		apMemory.m_secondJointIdx = m_kneeIndex;
		apMemory.m_secondJointIkGain = m_currentWeight_2;
		apMemory.m_hingeAxisLS.m_quad = v133.m_quad;
		apMemory.m_endBoneIdx = m_ankleIndex_2;
		apMemory.m_endTargetMS.m_quad = footEndWS.m_quad;
		hkaTwoJointsIkSolver::solve(&apResult, apMemory, poseInOut);
		v137.m_quad = m_setup.m_worldUpDirectionWS.m_quad;
		__m128 v138 = _mm_mul_ps(v173, v137.m_quad);
		__m128 v139 = _mm_shuffle_ps(v138, v138, 170);
		v139[0] = v139[0] + static_cast<float>(_mm_shuffle_ps(v138, v138, 85)[0] + v138[0]);
		__m128 v140 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v137.m_quad, v137.m_quad, 210), v171),
												_mm_mul_ps(_mm_shuffle_ps(v137.m_quad, v137.m_quad, 201), v169)),
									 v168),
			_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v168, v168), hkQuadRealMinusHalf), v137.m_quad),
				_mm_mul_ps(_mm_shuffle_ps(v139, v139, 0), v173)));
		from.m_quad = _mm_add_ps(v140, v140);
		__m128 v141 = _mm_mul_ps(v173, groundNormalWSOut.m_quad);
		__m128 v142 = _mm_shuffle_ps(v141, v141, 170);
		v142[0] = v142[0] + static_cast<float>(_mm_shuffle_ps(v141, v141, 85)[0] + v141[0]);
		__m128 v143 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(groundNormalWSOut.m_quad,
																	  groundNormalWSOut.m_quad, 210),
														   v171),
												_mm_mul_ps(_mm_shuffle_ps(groundNormalWSOut.m_quad,
															   groundNormalWSOut.m_quad, 201),
													v169)),
									 v168),
			_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v168, v168), hkQuadRealMinusHalf), groundNormalWSOut.m_quad),
				_mm_mul_ps(_mm_shuffle_ps(v142, v142, 0), v173)));
		v170.m_vec.m_quad = _mm_add_ps(v143, v143);
		q0_.setShortestRotation(from, v170.m_vec);
		v170.setMul(q0_, kOriginalAnkleRotationMS);
		__m128 v145 = _mm_shuffle_ps(_mm_set_ss(this->m_currentWeight), _mm_set_ss(this->m_currentWeight), 0);
		hkQuaternion kAnkleRotationMS;
		kAnkleRotationMS.m_vec.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v145), kOriginalAnkleRotationMS.m_vec.m_quad),
				_mm_mul_ps(v170.m_vec.m_quad, v145));
		kAnkleRotationMS.normalize();
		hkQsTransform* v146 = &poseInOut.accessBoneModelSpace(m_setup.m_ankleIndex, hkaPose::PROPAGATE);
		v146->m_rotation = kAnkleRotationMS;
	}
	v147.m_quad = groundNormalWSOut.m_quad;
	this->m_previousGroundHeightWS = m_previousGroundHeightWS_1;
	m_previousGroundNormalWS.m_quad = v147.m_quad;
}
