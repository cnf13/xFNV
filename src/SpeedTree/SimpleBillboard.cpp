#include "SpeedTree/SimpleBillboard.h"

#include "SpeedTree/CRotTransform.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <cmath>

// The guard bits of ComputeUnitBillboard's statics: afCorner1 to
// afCorner4 are bits 0 to 3 and afRawCorners bit 4.
static uint32_t ComputeUnitBillboard_uiGuard;

// Assign3d, VecRad2Deg (a double constant, after atan2's result is rounded to float), the CVec3 constructors and
// CRotTransform's identity constructor are inlined. The exe calls CRotTransform::RotateZ (0xB273D0) with +fAzimuth
// (speedtree-2's UPVECTOR_POS_Z branch, not its RotateY(-fAzimuth)).
void CSimpleBillboard::ComputeUnitBillboard(const float* pCameraDir) {
	CVec3 cAdjustedDirection;
	cAdjustedDirection.m_afData[2] = 0.0f;
	cAdjustedDirection.m_afData[1] = 0.0f;
	cAdjustedDirection.m_afData[0] = 0.0f;
	cAdjustedDirection.m_afData[0] = pCameraDir[0];
	cAdjustedDirection.m_afData[1] = pCameraDir[1];
	cAdjustedDirection.m_afData[2] = pCameraDir[2];
	float fY = cAdjustedDirection.m_afData[1];
	float fX = cAdjustedDirection.m_afData[0];
	float fRadians = static_cast<float>(std::atan2(static_cast<double>(fY), static_cast<double>(fX)));
	float fAzimuth = static_cast<float>(fRadians * 57.295780181884766);

	// The function statics (the PDB's names): the four unit corners and the raw corners they are copied into,
	// guarded by bits 0 to 4 of ComputeUnitBillboard_uiGuard. The exe inlines their CVec3 constructors. The hybrid
	// binds the names to the exe's objects at their addresses, untyped memory there, as FNVD_FUNCTION_STATIC does.
	static CVec3 afCorner1(0.0f, 0.5f, 1.0f);
	static CVec3 afCorner2(0.0f, -0.5f, 1.0f);
	static CVec3 afCorner3(0.0f, -0.5f, 0.0f);
	static CVec3 afCorner4(0.0f, 0.5f, 0.0f);
	static CVec3 afRawCorners[4];
	afRawCorners[0].m_afData[0] = afCorner1.m_afData[0];
	afRawCorners[0].m_afData[1] = afCorner1.m_afData[1];
	afRawCorners[0].m_afData[2] = afCorner1.m_afData[2];
	afRawCorners[1].m_afData[0] = afCorner2.m_afData[0];
	afRawCorners[1].m_afData[1] = afCorner2.m_afData[1];
	afRawCorners[1].m_afData[2] = afCorner2.m_afData[2];
	afRawCorners[2].m_afData[0] = afCorner3.m_afData[0];
	afRawCorners[2].m_afData[1] = afCorner3.m_afData[1];
	afRawCorners[2].m_afData[2] = afCorner3.m_afData[2];
	afRawCorners[3].m_afData[0] = afCorner4.m_afData[0];
	afRawCorners[3].m_afData[1] = afCorner4.m_afData[1];
	afRawCorners[3].m_afData[2] = afCorner4.m_afData[2];

	CRotTransform cTrans;
	cTrans.m_afData[0][0] = 1.0f;
	cTrans.m_afData[0][1] = 0.0f;
	cTrans.m_afData[0][2] = 0.0f;
	cTrans.m_afData[1][0] = 0.0f;
	cTrans.m_afData[1][1] = 1.0f;
	cTrans.m_afData[1][2] = 0.0f;
	cTrans.m_afData[2][0] = 0.0f;
	cTrans.m_afData[2][1] = 0.0f;
	cTrans.m_afData[2][2] = 1.0f;
	cTrans.RotateZ(fAzimuth);

	for (int32_t i = 0; i < 4; ++i) {
		CVec3 cCorner = afRawCorners[i] * cTrans;
		m_acUnitBillboard[i][0] = cCorner.m_afData[0];
		m_acUnitBillboard[i][1] = cCorner.m_afData[1];
		m_acUnitBillboard[i][2] = cCorner.m_afData[2];
	}
}
