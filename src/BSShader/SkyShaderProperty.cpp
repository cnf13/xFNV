#include "BSShader/SkyShaderProperty.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/SkyShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// Numbers the cloud layers in load order.
extern uint16_t SkyShaderProperty_usCloudLayerCount;
uint16_t SkyShaderProperty_usCloudLayerCount;

NiRTTI SkyShaderProperty::ms_RTTI("SkyShaderProperty", &BSShaderProperty::ms_RTTI);

SkyShaderProperty::SkyShaderProperty()
	: VertexColor(NiColorA_WHITE), spBaseMap(nullptr), TextureClampMode(NiTexturingProperty::WRAP_S_WRAP_T),
	  spBlendTexture(nullptr), fBlendValue(0.0f), usCloudLayer(0), soType(SO_OTHER) {
	iShaderPropertyType = 13;
}

SkyShaderProperty::~SkyShaderProperty() {
	spBlendTexture = nullptr;
	fBlendValue = 0.0f;
	soType = SO_OTHER;
	usCloudLayer = 0;
	spBaseMap = nullptr;
}

// The clone comes from a 0x90-byte engine block and is filled in by CopyMembers; arCloning is passed through.
NiObject* SkyShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
	SkyShaderProperty* pClone = static_cast<SkyShaderProperty*>(NiMemObject::operator new(sizeof(SkyShaderProperty)));
	if (pClone)
		pClone = ::new (pClone) SkyShaderProperty;
	CopyMembers(pClone, arCloneData);
	return pClone;
}

void SkyShaderProperty::LoadBinary(NiStream& kStream) {
	BSShaderProperty::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, TextureClampMode);
	char* pcName = nullptr;
	kStream.LoadCString(pcName);
	TextureFilename.Set(pcName, 0);
	_NiFree(pcName);
	NiStreamLoadBinary(kStream, soType);
	if (soType == 3)
		usCloudLayer = SkyShaderProperty_usCloudLayerCount++;
}

void SkyShaderProperty::LinkObject(NiStream& kStream) {
	BSShaderProperty::LinkObject(kStream);
}

bool SkyShaderProperty::RegisterStreamables(NiStream& kStream) {
	return BSShaderProperty::RegisterStreamables(kStream);
}

void SkyShaderProperty::SaveBinary(NiStream& kStream) {
	BSShaderProperty::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, TextureClampMode);
	kStream.SaveCString(TextureFilename.pString);
	NiStreamSaveBinary(kStream, soType);
}

bool SkyShaderProperty::IsEqual(NiObject* pkObject) {
	return BSShaderProperty::IsEqual(pkObject);
}

void SkyShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSShaderProperty::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString(SkyShaderProperty::ms_RTTI.GetName()));
	apStrings->Add(NiGetViewerString("SOType", static_cast<int32_t>(soType)));
	const char* pcBlend = (spBlendTexture.m_pObject && spBlendTexture.m_pObject->m_kName.GetHandle())
		? spBlendTexture.m_pObject->m_kName.GetHandle()
		: "";
	apStrings->Add(NiGetViewerString("Blend Texture", pcBlend));
	apStrings->Add(NiGetViewerString("Blend Value", fBlendValue));
	const char* pcBase = BSUtilities::GetTextureName(spBaseMap.m_pObject);
	if (pcBase)
		apStrings->Add(NiGetViewerString("base texture", pcBase));
	else
		apStrings->Add(NiGetViewerString("base texture", static_cast<const void*>(spBaseMap.m_pObject)));
	if (soType == 3) {
		char acBuffer[256];
		BSsprintf(acBuffer, sizeof(acBuffer), "%d %d %d %d", static_cast<int32_t>(VertexColor.r * 255.0),
			static_cast<int32_t>(VertexColor.g * 255.0), static_cast<int32_t>(VertexColor.b * 255.0),
			static_cast<int32_t>(VertexColor.a * 255.0));
		apStrings->Add(NiGetViewerString("Blend Color", acBuffer));
	}
}

void SkyShaderProperty::PostLinkObject(NiStream& kStream) {
	NiObjectNET::PostLinkObject(kStream);
	if (TextureFilename.pString && soType != 3)
		BSShaderManager::GetTexture(TextureFilename.pString, true, *reinterpret_cast<NiPointer<NiTexture>*>(&spBaseMap),
			true, false);
}

bool SkyShaderProperty::SetupGeometry(NiGeometry*) {
	// NiAVObject::UpdateProperties is not called: the game calls it only when &apGeometry->m_PropertyState is null,
	// which never happens.
	return true;
}

BSShaderProperty::RenderPassArray* SkyShaderProperty::GetRenderPasses(NiGeometry* apGeom, int32_t, uint16_t&, uint32_t,
	BSShaderAccumulator*, int32_t) {
	CheckCreateRenderPassArray(2);
	if (pRenderPassList->iSize)
		return pRenderPassList;
	uint32_t uiPass;
	switch (soType) {
	case 1:
		uiPass = BSShaderManager::BSSM_SKY_SUNGLARE;
		break;
	case 3:
		uiPass =
			SkyShader::bUnderWater ? BSShaderManager::BSSM_SKY_CLOUDS_UNDERWATER : BSShaderManager::BSSM_SKY_CLOUDS;
		break;
	case 5:
		uiPass = BSShaderManager::BSSM_SKY_STARS;
		break;
	case 7:
		uiPass = BSShaderManager::BSSM_SKY_MOON_STARS_MASK;
		break;
	default:
		uiPass = (soType == 2 || soType == 4) ? BSShaderManager::BSSM_SKY : BSShaderManager::BSSM_SKY_TEXTURE;
		break;
	}
	pRenderPassList->Add(apGeom, uiPass, true, 0, nullptr, nullptr, nullptr, nullptr);
	if (!BSShaderManager::bHDR) {
		if (soType == 0)
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SELFILLUM_SKY_SUN, true, 0, nullptr, nullptr, nullptr,
				nullptr);
		else if (soType == 3)
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SELFILLUM_SKY_CLOUDS, true, 0, nullptr, nullptr, nullptr,
				nullptr);
		else if (soType == 4)
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SELFILLUM_SKY_SKY_QUAD, true, 0, nullptr, nullptr,
				nullptr, nullptr);
	}
	return pRenderPassList;
}

void SkyShaderProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
	TextureClampMode =
		(aClampMode == D3DTADDRESS_CLAMP) ? NiTexturingProperty::CLAMP_S_CLAMP_T : NiTexturingProperty::WRAP_S_WRAP_T;
}

void SkyShaderProperty::SetTexture(NiSourceTexture* apTexture) {
	// NiSourceTexture derives from NiRefObject at offset 0, so its pointer is the NiRefObject pointer.
	if (spBaseMap.m_pObject == apTexture)
		return;

	if (spBaseMap.m_pObject)
		NiDecRefCount(reinterpret_cast<NiRefObject*>(spBaseMap.m_pObject));
	spBaseMap.m_pObject = apTexture;
	if (apTexture)
		NiIncRefCount(reinterpret_cast<NiRefObject*>(apTexture));
}

void SkyShaderProperty::SetBlendValue(float afBlend) {
	fBlendValue = afBlend;
}

void SkyShaderProperty::SetTextureFile(const NiFixedString& aFilename) {
	TextureFilename = aFilename.GetHandleString();
}

void SkyShaderProperty::SetBlendTexture(NiTexture* apTexture) {
	if (spBlendTexture.m_pObject == apTexture)
		return;

	if (spBlendTexture.m_pObject)
		NiDecRefCount(reinterpret_cast<NiRefObject*>(spBlendTexture.m_pObject));
	spBlendTexture.m_pObject = static_cast<NiSourceTexture*>(apTexture);
	if (apTexture)
		NiIncRefCount(reinterpret_cast<NiRefObject*>(apTexture));
}

NiTexture* SkyShaderProperty::GetBaseTexture() const {
	return (&spBaseMap)->operator->();
}

// Copies this property's members into pDest.
void SkyShaderProperty::CopyMembers(SkyShaderProperty* pDest, NiCloningProcess& arCloneData) {
	BSShaderProperty::CopyMembers(pDest, arCloneData);
	pDest->spBlendTexture = spBlendTexture;
	pDest->fBlendValue = fBlendValue;
	pDest->soType = soType;
	NiSourceTexture* v6 = pDest->spBaseMap.m_pObject;
	if (v6 == spBaseMap.m_pObject) {
		pDest->TextureClampMode = TextureClampMode;
	} else {
		if (v6) {
			NiDecRefCount(v6);
		}
		NiSourceTexture* v7 = spBaseMap.m_pObject;
		pDest->spBaseMap.m_pObject = v7;
		if (v7) {
			NiIncRefCount(v7);
		}
		pDest->TextureClampMode = TextureClampMode;
	}
}

// The stream's factory: a property from a 0x90-byte engine block, or null when the block could not be had.
NiObject* SkyShaderProperty::CreateObject() {
	SkyShaderProperty* pProperty =
		static_cast<SkyShaderProperty*>(NiMemObject::operator new(sizeof(SkyShaderProperty)));
	if (pProperty)
		return ::new (pProperty) SkyShaderProperty;
	return nullptr;
}
