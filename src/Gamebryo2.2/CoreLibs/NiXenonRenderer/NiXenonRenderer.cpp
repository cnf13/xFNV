#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "SDK/MSVC/string.h"

void NiD3DUtility::GetD3DFromNi(
    D3DMATRIX& arD3D,
    const NiMatrix3& arNiRot,
    const NiPoint3& arNiTrans,
    float afNiScale
) {
    arD3D._11 = arNiRot.m_pEntry[0][0] * afNiScale;
    arD3D._12 = arNiRot.m_pEntry[1][0] * afNiScale;
    arD3D._13 = arNiRot.m_pEntry[2][0] * afNiScale;
    arD3D._14 = 0.0;
    arD3D._21 = arNiRot.m_pEntry[0][1] * afNiScale;
    arD3D._22 = arNiRot.m_pEntry[1][1] * afNiScale;
    arD3D._23 = arNiRot.m_pEntry[2][1] * afNiScale;
    arD3D._24 = 0.0;
    arD3D._31 = arNiRot.m_pEntry[0][2] * afNiScale;
    arD3D._32 = arNiRot.m_pEntry[1][2] * afNiScale;
    arD3D._33 = afNiScale * arNiRot.m_pEntry[2][2];
    arD3D._34 = 0.0;
    arD3D._41 = arNiTrans.x - NiRenderer::PosAdjust.x;
    arD3D._42 = arNiTrans.y - NiRenderer::PosAdjust.y;
    arD3D._43 = arNiTrans.z - NiRenderer::PosAdjust.z;
    arD3D._44 = 1.0;
}

void NiD3DUtility::GetD3DFromNi(D3DMATRIX& arD3D, const NiTransform& arNi) {
    NiD3DUtility::GetD3DFromNi(arD3D, arNi.m_Rotate, arNi.m_Translate, arNi.m_fScale);
}

void NiD3DUtility::GetD3DTransposeFromNi(
    D3DMATRIX& kD3D,
    const NiMatrix3& kNiRot,
    const NiPoint3& kNiTrans,
    float fNiScale
) {
    kD3D._11 = kNiRot.m_pEntry[0][0] * fNiScale;
    kD3D._12 = kNiRot.m_pEntry[0][1] * fNiScale;
    kD3D._13 = kNiRot.m_pEntry[0][2] * fNiScale;
    kD3D._14 = kNiTrans.x;
    kD3D._21 = kNiRot.m_pEntry[1][0] * fNiScale;
    kD3D._22 = kNiRot.m_pEntry[1][1] * fNiScale;
    kD3D._23 = kNiRot.m_pEntry[1][2] * fNiScale;
    kD3D._24 = kNiTrans.y;
    kD3D._31 = kNiRot.m_pEntry[2][0] * fNiScale;
    kD3D._32 = kNiRot.m_pEntry[2][1] * fNiScale;
    kD3D._33 = fNiScale * kNiRot.m_pEntry[2][2];
    kD3D._34 = kNiTrans.z;
    kD3D._14 = kNiTrans.x - NiRenderer::PosAdjust.x;
    kD3D._24 = kNiTrans.y - NiRenderer::PosAdjust.y;
    kD3D._34 = kNiTrans.z - NiRenderer::PosAdjust.z;
    kD3D._44 = 1.0;
}

bool NiGeometryBufferData::IsVBChipValid() {
    const uint32_t uiStreamCount = m_uiStreamCount;
    if (!uiStreamCount) {
        return false;
    }
    NiVBChip** ppkVBChip = m_ppkVBChip;
    uint32_t uiStream = 0;
    while (*ppkVBChip && (*ppkVBChip)->m_pkVB) {
        if (++uiStream >= uiStreamCount) {
            return true;
        }
        ++ppkVBChip;
    }
    return false;
}
