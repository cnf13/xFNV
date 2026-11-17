#include "Gamebryo2.2/CoreLibs/NiCollision/NiParallelogram.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

float NiSqrDistance::Compute(const NiTrigon& kTri, const NiParallelogram& kPgm, float& fTriParam0, float& fTriParam1,
	float& fPgmParam0, float& fPgmParam1) {
	float fTmpTriParam1;
	float fTmpPgmParam1;
	float fTmpPgmParam0;
	NiSegment kSeg;
	float y = kTri.m_kOrigin.y;
	float z = kTri.m_kOrigin.z;
	kSeg.m_kOrigin.x = kTri.m_kOrigin.x;
	float x = kTri.m_kEdge0.x;
	kSeg.m_kOrigin.y = y;
	float v11 = kTri.m_kEdge0.y;
	kSeg.m_kDirection.x = x;
	kSeg.m_kDirection.y = v11;
	kSeg.m_kOrigin.z = z;
	kSeg.m_kDirection.z = kTri.m_kEdge0.z;
	float fMin = NiSqrDistance::Compute(kSeg, kPgm, fTriParam0, fPgmParam0, fPgmParam1);
	fTriParam1 = 0.0f;
	float v12 = kTri.m_kEdge1.y;
	float v13 = kTri.m_kEdge1.z;
	kSeg.m_kDirection.x = kTri.m_kEdge1.x;
	kSeg.m_kDirection.y = v12;
	kSeg.m_kDirection.z = v13;
	float fMin0 = NiSqrDistance::Compute(kSeg, kPgm, fTmpTriParam1, fTmpPgmParam0, fTmpPgmParam1);
	float fTriParm0 = 0.0f;
	if (fMin > static_cast<double>(fMin0)) {
		fMin = fMin0;
		fTriParam0 = 0.0f;
		fTriParam1 = fTmpTriParam1;
		fPgmParam0 = fTmpPgmParam0;
		fPgmParam1 = fTmpPgmParam1;
	}
	float v39 = kTri.m_kEdge0.x + kSeg.m_kOrigin.x;
	double v14 = kTri.m_kEdge0.y;
	kSeg.m_kOrigin.x = v39;
	float v40 = v14 + kSeg.m_kOrigin.y;
	double v15 = kTri.m_kEdge0.z;
	kSeg.m_kOrigin.y = v40;
	float v41 = v15 + kSeg.m_kOrigin.z;
	kSeg.m_kOrigin.z = v41;
	v39 = kSeg.m_kDirection.x - kTri.m_kEdge0.x;
	kSeg.m_kDirection.x = v39;
	v40 = kSeg.m_kDirection.y - kTri.m_kEdge0.y;
	kSeg.m_kDirection.y = v40;
	v41 = kSeg.m_kDirection.z - kTri.m_kEdge0.z;
	kSeg.m_kDirection.z = v41;
	float fMin0_2 = NiSqrDistance::Compute(kSeg, kPgm, fTmpTriParam1, fTmpPgmParam0, fTmpPgmParam1);
	double v16 = fTmpTriParam1;
	float fTmpTriParam0 = 1.0 - fTmpTriParam1;
	if (fMin > static_cast<double>(fMin0_2)) {
		fMin = fMin0_2;
		fTriParam0 = fTmpTriParam0;
		fTriParam1 = v16;
		fPgmParam0 = fTmpPgmParam0;
		fPgmParam1 = fTmpPgmParam1;
	}
	float v17 = kPgm.m_kOrigin.y;
	float v18 = kPgm.m_kOrigin.z;
	kSeg.m_kOrigin.x = kPgm.m_kOrigin.x;
	float v19 = kPgm.m_kEdge0.x;
	kSeg.m_kOrigin.y = v17;
	float v20 = kPgm.m_kEdge0.y;
	kSeg.m_kDirection.x = v19;
	kSeg.m_kOrigin.z = v18;
	float v21 = kPgm.m_kEdge0.z;
	kSeg.m_kDirection.y = v20;
	kSeg.m_kDirection.z = v21;
	float fMin0_3 = NiSqrDistance::Compute(kSeg, kTri, fTmpPgmParam0, fTmpTriParam0, fTmpTriParam1);
	fTmpPgmParam1 = 0.0f;
	if (fMin > static_cast<double>(fMin0_3)) {
		fMin = fMin0_3;
		fTriParam0 = fTmpTriParam0;
		fTriParam1 = fTmpTriParam1;
		fPgmParam0 = fTmpPgmParam0;
		fPgmParam1 = 0.0f;
	}
	float v22 = kPgm.m_kEdge1.y;
	float v23 = kPgm.m_kEdge1.z;
	kSeg.m_kDirection.x = kPgm.m_kEdge1.x;
	kSeg.m_kDirection.y = v22;
	kSeg.m_kDirection.z = v23;
	float fMin0_4 = NiSqrDistance::Compute(kSeg, kTri, fTmpPgmParam1, fTmpTriParam0, fTmpTriParam1);
	fTmpPgmParam0 = 0.0f;
	if (fMin > static_cast<double>(fMin0_4)) {
		fMin = fMin0_4;
		fTriParam0 = fTmpTriParam0;
		fTriParam1 = fTmpTriParam1;
		fPgmParam0 = 0.0f;
		fPgmParam1 = fTmpPgmParam1;
	}
	v39 = kPgm.m_kEdge1.x + kPgm.m_kOrigin.x;
	double v24 = kPgm.m_kEdge1.y;
	kSeg.m_kOrigin.x = v39;
	double v25 = v24 + kPgm.m_kOrigin.y;
	kSeg.m_kDirection.x = kPgm.m_kEdge0.x;
	v40 = v25;
	double v26 = kPgm.m_kEdge1.z;
	kSeg.m_kOrigin.y = v40;
	double v27 = v26 + kPgm.m_kOrigin.z;
	kSeg.m_kDirection.y = kPgm.m_kEdge0.y;
	v41 = v27;
	kSeg.m_kOrigin.z = v41;
	kSeg.m_kDirection.z = kPgm.m_kEdge0.z;
	float fMin0_5 = NiSqrDistance::Compute(kSeg, kTri, fTmpPgmParam0, fTmpTriParam0, fTmpTriParam1);
	fTmpPgmParam1 = 1.0f;
	if (fMin > static_cast<double>(fMin0_5)) {
		fMin = fMin0_5;
		fTriParam0 = fTmpTriParam0;
		fTriParam1 = fTmpTriParam1;
		fPgmParam0 = fTmpPgmParam0;
		fPgmParam1 = 1.0f;
	}
	v39 = kPgm.m_kOrigin.x + kPgm.m_kEdge0.x;
	double v28 = kPgm.m_kEdge0.y;
	kSeg.m_kOrigin.x = v39;
	double v29 = v28 + kPgm.m_kOrigin.y;
	kSeg.m_kDirection.x = kPgm.m_kEdge1.x;
	v40 = v29;
	double v30 = kPgm.m_kEdge0.z;
	kSeg.m_kOrigin.y = v40;
	double v31 = v30 + kPgm.m_kOrigin.z;
	kSeg.m_kDirection.y = kPgm.m_kEdge1.y;
	v41 = v31;
	kSeg.m_kOrigin.z = v41;
	kSeg.m_kDirection.z = kPgm.m_kEdge1.z;
	float fMin0_6 = NiSqrDistance::Compute(kSeg, kTri, fTmpPgmParam1, fTmpTriParam0, fTmpTriParam1);
	if (!(fMin > static_cast<double>(fMin0_6))) {
		return fMin;
	} else {
		fTriParam0 = fTmpTriParam0;
		fTriParam1 = fTmpTriParam1;
		fPgmParam0 = 1.0f;
		fPgmParam1 = fTmpPgmParam1;
		return fMin0_6;
	}
}
