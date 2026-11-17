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

bool NiPixelFormat::operator!=(const NiPixelFormat& kOther) const {
	if (kOther.m_eFormat != m_eFormat)
		return true;
	if (kOther.m_uFlags != m_uFlags)
		return true;
	if (kOther.m_eTiling != m_eTiling)
		return true;
	if (kOther.m_ucBitsPerPixel != m_ucBitsPerPixel)
		return true;
	const uint32_t uiOtherHint = kOther.m_uiRendererHint;
	const uint32_t uiHint = m_uiRendererHint;
	if (uiOtherHint != uiHint && uiHint != 0xFFFFFFFFu && uiOtherHint != 0xFFFFFFFFu)
		return true;

	for (int32_t i = 0; i < static_cast<int32_t>(NUM_COMPS); ++i) {
		if (kOther.m_akComponents[i].m_eComponent != m_akComponents[i].m_eComponent ||
			kOther.m_akComponents[i].m_eRepresentation != m_akComponents[i].m_eRepresentation ||
			kOther.m_akComponents[i].m_ucBitsPerComponent != m_akComponents[i].m_ucBitsPerComponent ||
			kOther.m_akComponents[i].m_bSigned != m_akComponents[i].m_bSigned)
			return true;
	}
	return false;
}

// Stores pkProp in the slot of its type. The type is compared signed and 7 is accepted, one past the PC's seven slots,
// as the game does.
void NiPropertyState::SetProperty(NiProperty* pkProp) {
	if (pkProp && static_cast<int32_t>(pkProp->Type()) <= 7) {
		m_aspProps[pkProp->Type()] = pkProp;
	}
}

// A null shader keeps the current one.
void NiRenderer::SetErrorShader(NiShader* pkErrorShader) {
	if (!pkErrorShader) {
		return;
	}
	NiShader* pkOld = m_spErrorShader.m_pObject;
	if (pkOld == pkErrorShader) {
		return;
	}
	if (pkOld) {
		pkOld->DecRefCount();
	}
	m_spErrorShader.m_pObject = pkErrorShader;
	pkErrorShader->IncRefCount();
}

NiVertexColorProperty::NiVertexColorProperty() : m_uFlags(8) {}

NiVertexColorProperty::~NiVertexColorProperty() {}

NiZBufferProperty::NiZBufferProperty() : m_uFlags(15) {}

NiZBufferProperty::~NiZBufferProperty() {}

char* NiStrncpy(char* pcDest, size_t stDestSize, const char* pcSrc, size_t stCount) {
	strncpy_s(pcDest, stDestSize, pcSrc, stCount);
	return pcDest;
}

void NiD3DUtility::GetD3DFromNi(D3DMATRIX& arD3D, const NiMatrix3& arNiRot, const NiPoint3& arNiTrans,
	float afNiScale) {
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

void NiD3DUtility::GetD3DTransposeFromNi(D3DMATRIX& kD3D, const NiMatrix3& kNiRot, const NiPoint3& kNiTrans,
	float fNiScale) {
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
	if (!uiStreamCount)
		return false;
	NiVBChip** ppkVBChip = m_ppkVBChip;
	uint32_t uiStream = 0;
	while (*ppkVBChip && (*ppkVBChip)->m_pkVB) {
		if (++uiStream >= uiStreamCount)
			return true;
		++ppkVBChip;
	}
	return false;
}
