#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiProcessor.h"

#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProcessorSpecificCode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Reconstruction.hpp"

#include <bit>
#include <stdint.h>

NiProcessorSpecificCode::VectorTransformFunc NiProcessorSpecificCode::ms_pVectorsTransform{&TransformVectorsBasic};
NiProcessorSpecificCode::PointTransformFunc NiProcessorSpecificCode::ms_pPointsTransform{&TransformPointsBasic};

void TransformPointsBasic(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform) {
	for (uint32_t i = 0; i < uiVerts; ++i) {
		*pWorld = (pXform->m_Rotate.m_pEntry[0][0] * *pModel + pModel[1] * pXform->m_Rotate.m_pEntry[0][1] +
					  pModel[2] * pXform->m_Rotate.m_pEntry[0][2]) *
				pXform->m_fScale +
			pXform->m_Translate.x;
		pWorld[1] = (pXform->m_Rotate.m_pEntry[1][0] * *pModel + pModel[1] * pXform->m_Rotate.m_pEntry[1][1] +
						pModel[2] * pXform->m_Rotate.m_pEntry[1][2]) *
				pXform->m_fScale +
			pXform->m_Translate.y;
		pWorld[2] = (pXform->m_Rotate.m_pEntry[2][0] * *pModel + pModel[1] * pXform->m_Rotate.m_pEntry[2][1] +
						pModel[2] * pXform->m_Rotate.m_pEntry[2][2]) *
				pXform->m_fScale +
			pXform->m_Translate.z;
		pModel += 3;
		pWorld += 3;
	}
}

void TransformVectorsBasic(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix) {
	for (uint32_t i = 0; i < uiVerts; ++i) {
		*pWorld = pMatrix->m_pEntry[0][0] * *pModel + pModel[1] * pMatrix->m_pEntry[0][1] +
			pModel[2] * pMatrix->m_pEntry[0][2];
		pWorld[1] = pMatrix->m_pEntry[1][0] * *pModel + pModel[1] * pMatrix->m_pEntry[1][1] +
			pModel[2] * pMatrix->m_pEntry[1][2];
		pWorld[2] = pMatrix->m_pEntry[2][0] * *pModel + pModel[1] * pMatrix->m_pEntry[2][1] +
			pModel[2] * pMatrix->m_pEntry[2][2];
		pModel += 3;
		pWorld += 3;
	}
}

void NiTransformVectorsPentium(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix) {
	int32_t v4 = uiVerts;
	if (uiVerts) {
		do {
			double v7 = *pModel * pMatrix->m_pEntry[1][0];
			double v8 = pModel[1] * pMatrix->m_pEntry[1][1];
			double v9 = pModel[2] * pMatrix->m_pEntry[1][2];
			*pWorld = pModel[2] * pMatrix->m_pEntry[0][2] +
				(*pModel * pMatrix->m_pEntry[0][0] + pModel[1] * pMatrix->m_pEntry[0][1]);
			double v10 = v7 + v8;
			double v11 = v9;
			double v12 = *pModel * pMatrix->m_pEntry[2][0] + pModel[1] * pMatrix->m_pEntry[2][1];
			double v13 = v10 + v11;
			double v14 = pModel[2] * pMatrix->m_pEntry[2][2];
			pWorld[1] = v13;
			pModel += 3;
			pWorld += 3;
			*(pWorld - 1) = v12 + v14;
			--v4;
		} while (v4);
	}
}

void NiTransformPointsPentium(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform) {
	float v14[10];
	v14[9] = std::bit_cast<float>(static_cast<uint32_t>(static_cast<uint32_t>(uiVerts)));
	double x = pXform->m_Translate.x;
	float* v15 = v14;
	float v16 = x;
	float y = pXform->m_Translate.y;
	float pXforma = pXform->m_Translate.z;
	double m_fScale = pXform->m_fScale;
	v14[0] = pXform->m_Rotate.m_pEntry[0][0] * m_fScale;
	v14[1] = pXform->m_Rotate.m_pEntry[0][1] * m_fScale;
	v14[2] = pXform->m_Rotate.m_pEntry[0][2] * m_fScale;
	v14[3] = pXform->m_Rotate.m_pEntry[1][0] * m_fScale;
	v14[4] = pXform->m_Rotate.m_pEntry[1][1] * m_fScale;
	v14[5] = pXform->m_Rotate.m_pEntry[1][2] * m_fScale;
	v14[6] = pXform->m_Rotate.m_pEntry[2][0] * m_fScale;
	v14[7] = pXform->m_Rotate.m_pEntry[2][1] * m_fScale;
	v14[8] = m_fScale * pXform->m_Rotate.m_pEntry[2][2];
	int32_t v7 = uiVerts;
	if (uiVerts) {
		do {
			double v11 = v15[8] * pModel[2];
			double v12 = v15[6] * *pModel + v15[7] * pModel[1];
			double v13 = pModel[2] * v15[5] + (*pModel * v15[3] + pModel[1] * v15[4]) + y;
			*pWorld = pModel[2] * v15[2] + (*pModel * *v15 + pModel[1] * v15[1]) + v16;
			pWorld[1] = v13;
			pModel += 3;
			pWorld += 3;
			*(pWorld - 1) = v11 + v12 + pXforma;
			--v7;
		} while (v7);
	}
}

// KEEP: the exe runs each cpuid probe inside an SEH __try (scope table 0x1169D50) whose __except
// (EXCEPTION_EXECUTE_HANDLER) sets ms_eProcessorID to NI_UNSUPPORTED and returns (first and third probes) or clears
// ms_uiCPUIDFeatures and goes on to leaf 1 (second probe); src cannot express SEH __try/__except (the standalone build is
// x64 Linux).
// The second probe loads 0x80000001 into edx, not eax, so it repeats leaf 0x80000000 (eax still holds the masked
// result). The feature bits of leaf 1 (edx): 0x800000 MMX, 0x8000 CMOV, 0x2000000 SSE.
void NiProcessorSpecificCode::IdentifyProcessor() {
	uint32_t uiEAX;
	uint32_t uiEBX;
	uint32_t uiECX;
	uint32_t uiEDX;
	ms_bIDSet = true;
	uiEAX = 0x80000000u;
	__asm__ volatile("cpuid" : "+a"(uiEAX), "=b"(uiEBX), "=c"(uiECX), "=d"(uiEDX));
	ms_uiCPUIDFeatures = static_cast<int32_t>(uiEAX);
	if ((ms_uiCPUIDFeatures & 0x80000000u) != 0) {
		uiEAX = ms_uiCPUIDFeatures & 0x80000000u;
		uiEDX = 0x80000001u;
		__asm__ volatile("cpuid" : "+a"(uiEAX), "=b"(uiEBX), "=c"(uiECX), "+d"(uiEDX));
		ms_uiCPUIDFeatures = static_cast<int32_t>(uiEAX);
		if ((ms_uiCPUIDFeatures & 0x80000000u) != 0) {
			ms_eProcessorID = NI_3DNOW;
			return;
		}
	}
	uiEAX = 1;
	__asm__ volatile("cpuid" : "+a"(uiEAX), "=b"(uiEBX), "=c"(uiECX), "=d"(uiEDX));
	ms_uiCPUIDFeatures = static_cast<int32_t>(uiEDX);
	if ((ms_uiCPUIDFeatures & 0x800000) == 0) {
		if ((ms_uiCPUIDFeatures & 0x8000) == 0) {
			ms_eProcessorID = NI_PENTIUM;
		} else {
			ms_eProcessorID = NI_PENTIUM_PRO;
		}
		return;
	}
	if ((ms_uiCPUIDFeatures & 0x8000) == 0) {
		ms_eProcessorID = NI_PENTIUM_MMX;
		return;
	}
	ms_eProcessorID = NI_PENTIUM_II;
	if ((ms_uiCPUIDFeatures & 0x2000000) != 0) {
		ms_eProcessorID = NI_PENTIUM_III;
	}
}

void NiProcessorSpecificCode::SetPointTransformFunction(NiProcessorSpecificCode::PointTransformFunc ptf) {
	NiProcessorSpecificCode::ms_pPointsTransform = ptf;
}

void NiProcessorSpecificCode::SetVectorTransformFunction(NiProcessorSpecificCode::VectorTransformFunc vtf) {
	NiProcessorSpecificCode::ms_pVectorsTransform = vtf;
}

// The basic transforms first; every identified processor (1 to 6, a signed range test in the exe) then gets the
// Pentium ones.
void NiProcessorSpecificCode::OptimizeMainEngineForProcessor() {
	if (!ms_bIDSet)
		IdentifyProcessor();
	SetPointTransformFunction(&TransformPointsBasic);
	SetVectorTransformFunction(&TransformVectorsBasic);
	switch (ms_eProcessorID) {
	case NI_PENTIUM:
	case NI_PENTIUM_MMX:
	case NI_PENTIUM_PRO:
	case NI_PENTIUM_II:
	case NI_PENTIUM_III:
	case NI_3DNOW:
		SetPointTransformFunction(&NiTransformPointsPentium);
		SetVectorTransformFunction(&NiTransformVectorsPentium);
		break;
	default:
		break;
	}
}

void NiProcessorSpecificCode::TransformPoints(uint16_t uiVerts, const float* pModel, float* pWorld,
	const NiTransform* pXform) {
	ms_pPointsTransform(uiVerts, pModel, pWorld, pXform);
}
