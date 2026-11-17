#include "BSShader/TileShaderProperty.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI TileShaderProperty::ms_RTTI("TileShaderProperty", &BSShaderProperty::ms_RTTI);

// Points the texture pointer at apNew: releases the old texture, then references the new one. NiTexture derives from
// NiRefObject at offset 0, so its pointer is the NiRefObject pointer.
static void AssignTexture(NiPointer<NiTexture>& arpTexture, NiTexture* apNew) {
	if (arpTexture.m_pObject == apNew)
		return;

	if (arpTexture.m_pObject)
		NiDecRefCount(reinterpret_cast<NiRefObject*>(arpTexture.m_pObject));
	arpTexture.m_pObject = apNew;
	if (apNew)
		NiIncRefCount(reinterpret_cast<NiRefObject*>(apNew));
}

// The tile is untinted and opaque, wraps its texture and does not scroll it.
TileShaderProperty::TileShaderProperty(bool abUseTexturingProperty)
	: TintColor(1.0f, 1.0f, 1.0f, 1.0f), fAlpha(1.0f), fTexScrollX(0.0f), fTexScrollY(0.0f), fTexScaleX(1.0f),
	  fTexScaleY(1.0f), TexClampMode(NiTexturingProperty::WRAP_S_WRAP_T), bNoBlend(false), bUseTexScroll(false),
	  bUseVertexColors(false), bUseTexturingProperty(abUseTexturingProperty), bUseScissor(false) {
	iShader = 32;
	iShaderPropertyType = 19;
}

// The textures and the texture filename release themselves.
TileShaderProperty::~TileShaderProperty() {}

// The exe's cleanup-only EH frame around the construction is left out. CopyMembers still runs when the allocation
// failed, with a null destination, as in the exe.
NiObject* TileShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
	TileShaderProperty* pClone =
		static_cast<TileShaderProperty*>(NiMemObject::operator new(sizeof(TileShaderProperty)));
	if (pClone) {
		pClone = ::new (pClone) TileShaderProperty(false);
	}
	CopyMembers(pClone, arCloneData);
	return pClone;
}

void TileShaderProperty::LoadBinary(NiStream& kStream) {
	BSShaderProperty::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, TexClampMode);
	char* pcName = nullptr;
	kStream.LoadCString(pcName);
	TextureFilename.Set(pcName, 0);
	_NiFree(pcName);
}

void TileShaderProperty::LinkObject(NiStream& kStream) {
	BSShaderProperty::LinkObject(kStream);
}

bool TileShaderProperty::RegisterStreamables(NiStream& kStream) {
	return BSShaderProperty::RegisterStreamables(kStream);
}

void TileShaderProperty::SaveBinary(NiStream& kStream) {
	BSShaderProperty::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, TexClampMode);
	kStream.SaveCString(TextureFilename.pString);
}

void TileShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSShaderProperty::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString(TileShaderProperty::ms_RTTI.GetName()));
	apStrings->Add(NiGetViewerString("Tint Color R", TintColor.r));
	apStrings->Add(NiGetViewerString("Tint Color G", TintColor.g));
	apStrings->Add(NiGetViewerString("Tint Color B", TintColor.b));
	// The alpha line shows the tint's alpha component (offset 0x74), not fAlpha.
	apStrings->Add(NiGetViewerString("Alpha", TintColor.a));
	if (spTileTexture.m_pObject) {
		// The exe calls NiObject::DynamicCast out of line (0x653290).
		NiObject* pSource = spTileTexture.m_pObject->DynamicCastMember(&NiSourceTexture::ms_RTTI);
		if (pSource)
			apStrings->Add(NiGetViewerString("Texture",
				static_cast<NiSourceTexture*>(pSource)->GetFilename().GetHandle()));
	}
}

// Skips BSShaderProperty's PostLinkObject and goes straight to NiObjectNET's.
void TileShaderProperty::PostLinkObject(NiStream& kStream) {
	NiObjectNET::PostLinkObject(kStream);
	if (TextureFilename.pString)
		BSShaderManager::GetTexture(TextureFilename.pString, true, spTileTexture, false, false);
}

BSShaderProperty::RenderPassArray* TileShaderProperty::GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses,
	uint16_t& asCount, uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) {
	CheckCreateRenderPassArray(1);
	if (pRenderPassList->iSize)
		return pRenderPassList;

	pRenderPassList->Add(apGeom, BSShaderManager::BSSM_NOLIGHTING, true, 0, nullptr, nullptr, nullptr, nullptr);
	pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_SORT;
	iLastRenderPassState = aiEnabledPasses | (auiRenderMode << 8);
	return pRenderPassList;
}

NiTexture* TileShaderProperty::GetTileTexture() const {
	return spTileTexture.m_pObject;
}

NiTexture* TileShaderProperty::GetAlphaTexture() {
	return spAlphaMap.operator->();
}

void TileShaderProperty::SetTileTexture(NiTexture* apTexture) {
	AssignTexture(spTileTexture, apTexture);
}

void TileShaderProperty::SetAlphaMap(NiTexture* apTexture) {
	AssignTexture(spAlphaMap, apTexture);
}

void TileShaderProperty::SetTileAlpha(float afAlpha) {
	fAlpha = afAlpha;
}

void TileShaderProperty::SetUseTexScroll(bool abRotates) {
	bUseTexScroll = abRotates;
}

void TileShaderProperty::SetUseScissorTest(bool abUseScissor) {
	bUseScissor = abUseScissor;
}

void TileShaderProperty::SetUseVertexColors(bool abUseVertexColors) {
	bUseVertexColors = (bool)((uint8_t)abUseVertexColors);
}

void TileShaderProperty::SetTexScroll(float afScrollAmountX, float afScrollAmountY) {
	fTexScrollX = afScrollAmountX;
	fTexScrollY = afScrollAmountY;
}

void TileShaderProperty::CopyMembers(TileShaderProperty* pDest, NiCloningProcess& arCloneData) {
	BSShaderProperty::CopyMembers(pDest, arCloneData);
	pDest->spTileTexture = spTileTexture;
	pDest->TintColor.r = TintColor.r;
	pDest->TintColor.g = TintColor.g;
	pDest->TintColor.b = TintColor.b;
	pDest->TintColor.a = TintColor.a;
	pDest->TexClampMode = TexClampMode;
	pDest->TileShaderProperty::fAlpha = this->TileShaderProperty::fAlpha;
	pDest->bUseTexturingProperty = bUseTexturingProperty;
	pDest->bNoBlend = bNoBlend;
}

// The exe's cleanup-only EH frame around the construction is left out.
NiObject* TileShaderProperty::CreateObject() {
	TileShaderProperty* pObject =
		static_cast<TileShaderProperty*>(NiMemObject::operator new(sizeof(TileShaderProperty)));
	if (pObject) {
		return ::new (pObject) TileShaderProperty(false);
	}
	return nullptr;
}
