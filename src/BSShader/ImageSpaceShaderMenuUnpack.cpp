#include "BSShader/ImageSpaceShaderMenuUnpack.h"

#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

ImageSpaceShaderMenuUnpack::kPackedRegions_t ImageSpaceShaderMenuUnpack::kPackedRegions;

// The exe's body is the ImageSpaceEffect subobject's slot 1, so it takes that subobject as this.
// The exe's cleanup-only C++ EH frame is left out.
void ImageSpaceShaderMenuUnpack::Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer,
	ImageSpaceEffectParam* apParam, int32_t abEndFrame) {
	NiRect<float> kPort;
	kPort.m_left = 0.0f;
	kPort.m_right = 1.0f;
	kPort.m_top = 1.0f;
	kPort.m_bottom = 0.0f;
	if (ImageSpaceShaderMenuUnpack::kPackedRegions.m_usSize) {
		ImageSpaceShaderParam* v34 = static_cast<ImageSpaceShaderParam*>(apParam);
		if (!apParam) {
			v34 = &ImageSpaceShader::DefaultParam;
		}
		if (ImageSpaceShaderMenuUnpack::bChangedFlag) {
			NiScreenElements* v11;
			NiUpdateData arData;
			BSRenderedTexture* RenderedTexture = (*pTextures.m_pBase)->GetRenderedTexture();
			if (RenderedTexture) {
				NiIncRefCount(RenderedTexture);
			}
			NiTriShape* spMenuUnpackGeom = BSShaderManager::pImageSpaceManager->spMenuUnpackGeom;
			NiTriShape** p_spMenuUnpackGeom = &BSShaderManager::pImageSpaceManager->spMenuUnpackGeom;
			if (spMenuUnpackGeom) {
				NiDecRefCount(spMenuUnpackGeom);
				*p_spMenuUnpackGeom = 0;
			}
			NiScreenElements* v8 = static_cast<NiScreenElements*>(NiMemObject::operator new(sizeof(NiScreenElements)));
			if (v8) {
				NiScreenElementsData* v9 =
					static_cast<NiScreenElementsData*>(NiMemObject::operator new(sizeof(NiScreenElementsData)));
				if (v9) {
					NiScreenElementsData* v10 = ::new (v9) NiScreenElementsData(0, 0, 1, 1, 1, 4, 1, 2, 1);
					v11 = ::new (v8) NiScreenElements(v10);
				} else {
					v11 = ::new (v8) NiScreenElements(nullptr);
				}
			} else {
				v11 = 0;
			}
			int32_t v12 = 0;
			if (ImageSpaceShaderMenuUnpack::kPackedRegions.m_usSize) {
				do {
					uint32_t m_uiWidth;
					uint32_t m_uiHeight;
					FNVD_INLINED_LOCAL(NiFixedString, a2, std::nullopt, std::nullopt);
					NiRenderTargetGroup* v13 = apRenderer->GetDefaultRenderTargetGroup();
					if (v13) {
						Ni2DBuffer* v15 = v13->GetBuffer(0);
						if (v15) {
							m_uiWidth = v15->m_uiWidth;
						} else {
							m_uiWidth = 0;
						}
					} else {
						m_uiWidth = 0;
					}
					NiRenderTargetGroup* v16 = apRenderer->GetDefaultRenderTargetGroup();
					if (v16) {
						Ni2DBuffer* v18 = v16->GetBuffer(0);
						if (v18) {
							m_uiHeight = v18->m_uiHeight;
						} else {
							m_uiHeight = 0;
						}
					} else {
						m_uiHeight = 0;
					}
					float apParama = static_cast<double>(m_uiWidth) / static_cast<double>(m_uiHeight);
					float apParamb = apParama / 1.3333333730697632;
					static_cast<NiScreenElementsData*>(v11->m_spModelData.m_pObject)
						->NiScreenElementsData::Insert(4, 0, 0);
					double v19 = apParamb * 1280.0;
					NiFrustum* v20 = static_cast<NiFrustum*>(ImageSpaceShaderMenuUnpack::kPackedRegions.m_pBase[v12]);
					NiScreenElementsData* m_pObject = static_cast<NiScreenElementsData*>(v11->m_spModelData.m_pObject);
					float fLeft = v20->m_fNear / v19;
					float fTop = v20->m_fFar / 960.0;
					float fWidth = v20->m_fTop / v19;
					float fHeight = v20->m_fBottom / 960.0;
					float v31 = v20->m_fLeft / v19;
					float apParamc = v20->m_fRight / 960.0;
					fLeft = fLeft * 2.0 - 1.0 + ImageSpaceShaderMenuUnpack::fOffsetX;
					fTop = 1.0 - fTop * 2.0 - ImageSpaceShaderMenuUnpack::fOffsetY;
					double v22 = fWidth;
					fWidth = fWidth * 2.0;
					double v23 = fHeight;
					fHeight = 2.0 * -fHeight;
					float v41 = v22 + v31;
					float v39 = v23 + apParamc;
					m_pObject->SetRectangle(v12, fLeft, fTop, fWidth, fHeight);
					static_cast<NiScreenElementsData*>(v11->m_spModelData.m_pObject)
						->NiScreenElementsData::UpdateBound();
					static_cast<NiScreenElementsData*>(v11->m_spModelData.m_pObject)
						->NiScreenElementsData::SetTextures(v12, 0, v31, apParamc, v41, v39);
					v11->m_spModelData.m_pObject->SetConsistency(NiGeometryData::MUTABLE);
					// MAYBE-INLINED 0xBB7858: a write opening NiFixedString's uses: may be its inlined constructor
					a2.m_kHandle = NiGlobalStringTable::AddString("Interface: HUD Unpack Geometry");
					v11->SetName(a2);
					// The exe's inlined destructor of a2.
					// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
					NiGlobalStringTable::ReleaseHandle(a2.GetHandle());
					++v12;
				} while (v12 < ImageSpaceShaderMenuUnpack::kPackedRegions.m_usSize);
			}
			v11->UpdateProperties();
			arData.fTime = 0.0f;
			arData.bUpdateControllers = 0;
			arData.bParallelUpdate = 0;
			arData.bFoundParticles = 0;
			arData.bFoundMorphController = 0;
			arData.bSceneGraphChange = 0;
			v11->Update(arData);
			v11->m_spShader = (BSShader*)this;
			NiTriShape* v24 = BSShaderManager::pImageSpaceManager->spMenuUnpackGeom;
			NiTriShape** v25 = &BSShaderManager::pImageSpaceManager->spMenuUnpackGeom;
			if (v24 != v11) {
				if (v24) {
					NiDecRefCount(v24);
				}
				*v25 = (NiTriShape*)v11;
				NiIncRefCount(v11);
			}
			NiRefObject* v26 = (NiRefObject*)RenderedTexture;
			if (RenderedTexture) {
				if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&RenderedTexture->m_uiRefCount))) {
					v26->DeleteThis();
				}
			}
		}
		NiScreenElements* v27 = static_cast<NiScreenElements*>(BSShaderManager::pImageSpaceManager->spMenuUnpackGeom);
		ImageSpaceShader* v28 = (ImageSpaceShader*)this;
		this->SetupGeometry(v27);
		v28->SetConstantMap(v34);
		v28->SetVSConstants();
		NiRenderer* v29 = NiRenderer::ms_pkRenderer;
		NiRenderer::EFrameState m_eFrameState = NiRenderer::ms_pkRenderer->m_eFrameState;
		if ((m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
				m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
			NiRenderer::ms_pkRenderer->m_bRenderTargetGroupActive) {
			v29->Do_SetScreenSpaceCameraData(&kPort);
			v29 = NiRenderer::ms_pkRenderer;
		}
		v27->Draw(v29);
		ImageSpaceShaderMenuUnpack::bChangedFlag = 0;
	}
}
