#include "BSShader/BSShaderBloodSplatterProperty.h"

#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI BSShaderBloodSplatterProperty::ms_RTTI("BSShaderBloodSplatterProperty", &BSShaderProperty::ms_RTTI);

// Shader 7, property type 23, clamped textures and no fade alpha source.
BSShaderBloodSplatterProperty::BSShaderBloodSplatterProperty()
	: TextureClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T), pfFadeAlpha(nullptr) {
	iShader = 7;
	iShaderPropertyType = 23;
}

BSShaderBloodSplatterProperty::~BSShaderBloodSplatterProperty() {
	for (NiPointer<NiTexture>& rspTexture : spTexture) {
		rspTexture = nullptr;
	}
}

// Constructs an empty property, then copies the members into it.
NiObject* BSShaderBloodSplatterProperty::CreateClone(NiCloningProcess& arCloneData) {
	BSShaderBloodSplatterProperty* pClone =
		static_cast<BSShaderBloodSplatterProperty*>(NiMemObject::operator new(sizeof(BSShaderBloodSplatterProperty)));
	BSShaderBloodSplatterProperty* pResult = nullptr;
	if (pClone) {
		pResult = ::new (pClone) BSShaderBloodSplatterProperty;
	}
	CopyMembers(pResult, arCloneData);
	return pResult;
}

void BSShaderBloodSplatterProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSShaderProperty::GetViewerStrings(apStrings);
	char* ViewerString = NiGetViewerString(BSShaderBloodSplatterProperty::ms_RTTI.GetName());
	uint32_t m_usSize = apStrings->m_usSize;
	uint32_t m_usMaxSize = apStrings->m_usMaxSize;
	char* v13 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(m_usSize, v13);
	NiPointer<NiTexture>* spTexture = this->spTexture;
	int32_t v8 = 2;
	do {
		char* ViewerString_9;
		const char* pname = BSUtilities::GetTextureName(spTexture->m_pObject);
		if (pname) {
			ViewerString_9 = NiGetViewerString("base texture", pname);
		} else {
			ViewerString_9 = NiGetViewerString("base texture", static_cast<const void*>(spTexture->m_pObject));
		}
		uint32_t v11 = apStrings->m_usMaxSize;
		uint32_t v12 = apStrings->m_usSize;
		v13 = ViewerString_9;
		if (v12 >= v11) {
			apStrings->SetSize(v12 + apStrings->m_usGrowBy);
		}
		apStrings->SetAt(v12, v13);
		++spTexture;
		--v8;
	} while (v8);
}

// One pass (0x2F0, or 0x2F1 with a shade texture), rebuilt when the enabled passes or render mode change; each call
// records the shape and its pass in pDD while there is room.
BSShaderProperty::RenderPassArray* BSShaderBloodSplatterProperty::GetRenderPasses(NiGeometry* apGeom,
	int32_t aiEnabledPasses, uint16_t& asCount, uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator,
	int32_t abUpdateList) {
	uint32_t uiPassState = aiEnabledPasses | (auiRenderMode << 8);
	if (iLastRenderPassState != uiPassState) {
		CheckCreateRenderPassArray(1);
		pRenderPassList->iSize = 0;
		pRenderPassList->Add(apGeom, (spTexture[TEXTURE_SHADE].m_pObject != nullptr) + 0x2F0, true, 0, nullptr, nullptr,
			nullptr, nullptr);
		iLastRenderPassState = uiPassState;
	}
	int32_t iCount = uiShapeCount;
	if (iCount < 100) {
		pDD[iCount].pGeom = apGeom;
		pDD[iCount].pRenderPass = pRenderPassList->pPassA.m_pBase[0];
		uiShapeCount = iCount + 1;
	}
	return pRenderPassList;
}

void BSShaderBloodSplatterProperty::GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) {
	NiPointer<NiTexture>* spTexture = this->spTexture;
	int32_t v4 = 2;
	do {
		if (spTexture->m_pObject) {
			const NiRTTI* v5 = spTexture->m_pObject->GetRTTI();
			if (v5) {
				while (v5 != &NiSourceTexture::ms_RTTI) {
					v5 = v5->GetBaseRTTI();
					if (!v5) {
						goto LABEL_6;
					}
				}
			} else {
			LABEL_6:;
				if (spTexture->m_pObject) {
					for (const NiRTTI* i = spTexture->m_pObject->GetRTTI(); i; i = i->GetBaseRTTI()) {
						if (i == &NiRenderedTexture::ms_RTTI) {
							break;
						}
					}
				}
			}
			apTextureUseCB(spTexture->m_pObject, 0, apParam);
		}
		++spTexture;
		--v4;
	} while (v4);
}

void BSShaderBloodSplatterProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
	TextureClampMode =
		(aClampMode != D3DTADDRESS_CLAMP) ? NiTexturingProperty::WRAP_S_WRAP_T : NiTexturingProperty::CLAMP_S_CLAMP_T;
}

void BSShaderBloodSplatterProperty::CopyMembers(BSShaderBloodSplatterProperty* pDest, NiCloningProcess& arCloneData) {
	BSShaderProperty::CopyMembers(pDest, arCloneData);
	for (uint32_t i = 0; i < 2; ++i) {
		// EXE-INLINED 0x6E5CC0
		NiPointer<NiTexture>& rspDest = pDest->spTexture[i];
		NiTexture* pOld = rspDest.m_pObject;
		if (pOld != spTexture[i].m_pObject) {
			if (pOld) {
				NiDecRefCount(pOld);
			}
			rspDest.m_pObject = spTexture[i].m_pObject;
			if (rspDest.m_pObject) {
				NiIncRefCount(rspDest.m_pObject);
			}
		}
	}
	pDest->TextureClampMode = TextureClampMode;
}

// Walks apObject's subtree: every blood splatter shade property found gets apfAlphaSource as its fade alpha source.
void BSShaderBloodSplatterProperty::SetFadeSourceRecurse(NiAVObject* apObject, float* apfAlphaSource) {
	if (!apObject) {
		return;
	}
	if (apObject->IsGeometry()) {
		NiProperty* pProperty = apObject->GetProperty(NiProperty::SHADE);
		// 0x17 is the blood splatter shader property type (iShaderPropertyType).
		if (pProperty && static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType == 0x17) {
			static_cast<BSShaderBloodSplatterProperty*>(pProperty)->pfFadeAlpha = apfAlphaSource;
		}
		return;
	}
	NiNode* pNode = apObject->IsNode();
	if (!pNode) {
		return;
	}
	uint16_t usChildren = pNode->m_kChildren.m_usSize;
	for (uint32_t i = 0; i < usChildren; ++i) {
		NiAVObject* pChild = pNode->m_kChildren.m_pBase[i];
		if (pChild) {
			SetFadeSourceRecurse(pChild, apfAlphaSource);
		}
	}
}

void BSShaderBloodSplatterProperty::ResetShapeCount() {
	BSShaderBloodSplatterProperty::uiShapeCount = 0;
}
