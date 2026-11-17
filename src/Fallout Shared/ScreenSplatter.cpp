#include "Fallout Shared/ScreenSplatter.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderBloodSplatter.h"
#include "BSShader/BSShaderBloodSplatterProperty.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BSSplatterData.h"
#include "Fallout Shared/BSSplatterExtraData.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI BSSplatterExtraData::ms_RTTI("BSSplatterExtraData", &NiExtraData::ms_RTTI);
SettingT<INISettingCollection> bBloodSplatterEnabled("bBloodSplatterEnabled:ScreenSplatter", true);
NiPointer<NiCamera> ScreenSplatter::spCamera;
NiPointer<NiNode> ScreenSplatter::spRoot;
ScreenSplatter::spTextures_t ScreenSplatter::spTextures;

void ScreenSplatter::Activate(uint32_t auiCount, float afDuration, ScreenSplatterType aeType) {
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	BSSplatterData splatterData;
	if (ScreenSplatter::bEnabled) {
		int32_t* v3 = &iBloodSplatterMaxCount.Int();
		uint32_t uicount = BSMIN(auiCount, *v3 - ScreenSplatter::uiCount);
		if (uicount) {
			NiExtraData* ped;
			NiNode* pnode = ScreenSplatter::CreateGeometry(uicount);
			ScreenSplatter::uiCount += uicount;
			splatterData.fAlpha = 1.0f;
			splatterData.fAge = 0.0;
			if (!(afDuration > 0.0)) {
				splatterData.fDuration = fBloodSplatterDuration.Float();
			} else {
				splatterData.fDuration = afDuration;
			}
			splatterData.uiCount = uicount;
			void* apMemory = NiMemObject::operator new(sizeof(BSSplatterExtraData));
			if (apMemory) {
				ped = ::new (apMemory) BSSplatterExtraData(splatterData);
			} else {
				ped = 0;
			}
			BSSplatterExtraData* v14 = static_cast<BSSplatterExtraData*>(ped);
			pnode->AddExtraData(ped);
			NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("ScreenSplatterRoot");
			pnode->SetName(*a2);
			apString.~NiFixedString();
			NiNode* v5 = ScreenSplatter::spRoot.m_pObject;
			v5->AttachChild(pnode, 1);
			float* BroadPhaseHandle = &v14->SplatterData.fAlpha;
			BSShaderBloodSplatterProperty::SetFadeSourceRecurse(pnode, BroadPhaseHandle);
			NiUpdateData UpdateData(0.0f, 0, 0);
			ScreenSplatter::spRoot.m_pObject->Update(UpdateData);
		}
	}
}

void ScreenSplatter::Clear() {
	if (ScreenSplatter::bEnabled) {
		ScreenSplatter::spRoot->RemoveAllChildren();
		ScreenSplatter::uiCount = 0;
	}
}

bool ScreenSplatter::IsActiveSplatter() {
	if (!(&ScreenSplatter::spRoot)->operator->()) {
		return (bool)0;
	}
	NiNode* v1 = (&ScreenSplatter::spRoot)->operator->();
	return (v1->GetChildCount() != 0);
}

NiCamera* ScreenSplatter::GetCamera() {
	return (&ScreenSplatter::spCamera)->operator->();
}

NiNode* ScreenSplatter::GetRoot() {
	return (&ScreenSplatter::spRoot)->operator->();
}

void ScreenSplatter::Update() {
	if (ScreenSplatter::bEnabled) {
		uint32_t uichildCount = ScreenSplatter::spRoot.m_pObject->GetArrayCount();
		float felapsedtime = appTimer.fDelta;
		for (uint32_t ui = 0; ui < uichildCount; ++ui) {
			NiAVObject* pchild = ScreenSplatter::spRoot.m_pObject->GetAt(ui);
			if (pchild) {
				NiFixedString* Tag = BSSplatterExtraData::GetTag();
				BSSplatterExtraData* ped = static_cast<BSSplatterExtraData*>(pchild->GetExtraData(*Tag));
				BSSplatterData* splatterData = &ped->SplatterData;
				splatterData->fAge = splatterData->fAge + felapsedtime;
				if (!(splatterData->fDuration <= static_cast<double>(splatterData->fAge))) {
					double v8 = splatterData->fAge / splatterData->fDuration;
					if (fBloodSplatterFadeStart.Float() <= v8) {
						float value = splatterData->fAge;
						float y2 = splatterData->fDuration;
						float* v1 = &fBloodSplatterFadeStart.Float();
						float v7 = *v1 * splatterData->fDuration;
						double v2 = FLerp(0.0f, 1.0f, v7, y2, value);
						splatterData->fAlpha = 1.0 - v2;
					}
					NiUpdateData UpdateData(splatterData->fAge, 1, 0);
					pchild->Update(UpdateData);
				} else {
					ScreenSplatter::uiCount -= splatterData->uiCount;
					NiNode* v9 = ScreenSplatter::spRoot.m_pObject;
					v9->DetachChildAt(ui);
				}
			}
		}
		float* v3 = &fBloodSplatterFlareOffsetScale.Float();
		BSShaderBloodSplatter::SetFlareOffsetScale(*v3);
		float* v4 = &fBloodSplatterFlareMult.Float();
		BSShaderBloodSplatter::SetFlareMult(*v4);
		float fglobalAlpha = BSShaderBloodSplatter::GetGlobalAlpha();
		if (fglobalAlpha < 1.0) {
			fglobalAlpha = felapsedtime / 0.4 + fglobalAlpha;
			fglobalAlpha = BSMIN(1.0f, fglobalAlpha);
			BSShaderBloodSplatter::SetGlobalAlpha(fglobalAlpha);
		}
	}
}

void ScreenSplatter::Init() {
	NiPoint3 dst;
	AutoMemContext tempMemContext(MEM_CONTEXT_EFFECTS_OVERHEAD, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ScreenSplatter.cpp", 181);
	ScreenSplatter::bEnabled = bBloodSplatterEnabled.Bool();
	// Tests the whole byte of bEnabled, not its low bit.
	if (static_cast<uint8_t>(ScreenSplatter::bEnabled)) {
		NiNode* v4;
		NiCamera* v5;
		NiRenderer* prenderer = NiRenderer::GetRenderer();
		double ScreenWidth = static_cast<double>(prenderer->GetScreenWidth());
		float faspectratio = ScreenWidth / static_cast<double>(prenderer->GetScreenHeight());
		NiCamera* v9 = static_cast<NiCamera*>(NiMemObject::operator new(sizeof(NiCamera)));
		if (v9) {
			v5 = ::new (v9) NiCamera;
		} else {
			v5 = 0;
		}
		NiRefObject* v10 = v5;
		ScreenSplatter::spCamera.operator=(v5);
		NiPoint3* v0 = ::new (&dst) NiPoint3(0.0f, 0.0f, 1.00001f);
		ScreenSplatter::spCamera.m_pObject->SetTranslate(*v0);
		NiUpdateData UpdateData1(0.0f, 0, 0);
		ScreenSplatter::spCamera.m_pObject->Update(UpdateData1);
		ScreenSplatter::spCamera.m_pObject->LookAtWorldPoint(NiPoint3_ZERO, NiPoint3_UNIT_Y);
		NiFrustum frustum(0);
		frustum.m_bOrtho = 0;
		frustum.m_fTop = 0.5f;
		frustum.m_fBottom = -0.5f;
		frustum.m_fLeft = faspectratio * -0.5;
		frustum.m_fRight = faspectratio * 0.5;
		frustum.m_fFar = 10000.0f;
		frustum.m_fNear = 1.0f;
		ScreenSplatter::spCamera.m_pObject->SetViewFrustum(frustum);
		NiUpdateData UpdateData2(0.0f, 0, 0);
		ScreenSplatter::spCamera.m_pObject->Update(UpdateData2);
		ScreenSplatter::fWRadius = frustum.m_fRight;
		ScreenSplatter::fHRadius = frustum.m_fTop;
		NiNode* v7 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
		if (v7) {
			v4 = ::new (v7) NiNode(0);
		} else {
			v4 = 0;
		}
		ScreenSplatter::spRoot.operator=(v4);
		ScreenSplatter::uiCount = 0;
		const char* v1 = sBloodSplatterAlpha01OPTFilename.String();
		SkyTextureManager::LoadTextureInBackground(&ScreenSplatter::spTextures[1], v1);
		const char* v2 = sBloodSplatterColor01OPTFilename.String();
		SkyTextureManager::LoadTextureInBackground(ScreenSplatter::spTextures, v2);
		if (BSShaderManager::bHDR) {
			const char* v3 = sBloodSplatterFlare01Filename.String();
			SkyTextureManager::LoadTextureInBackground(&ScreenSplatter::spTextures[2], v3);
		}
	}
}

void ScreenSplatter::Shutdown() {
	if (ScreenSplatter::bEnabled) {
		ScreenSplatter::spRoot.m_pObject->RemoveAllChildren();
		ScreenSplatter::spRoot.operator=(static_cast<NiNode*>(0));
		ScreenSplatter::spCamera.operator=(static_cast<NiCamera*>(0));
	}
	ScreenSplatter::bEnabled = 0;
	for (int32_t i = 0; i < 3; ++i) {
		ScreenSplatter::spTextures[i].operator=(static_cast<NiTexture*>(0));
	}
}

NiNode* ScreenSplatter::CreateGeometry(uint32_t auiCount) {
	BSShaderBloodSplatterProperty* pshaderprop;
	NiTriShape* pgeom;
	NiPoint2* ptexcoordsA;
	NiPoint3* pverticesA;
	NiNode* pnode;
	NiPoint2 v101;
	NiPoint2 v102;
	NiPoint2 v103;
	NiPoint2 v104;
	NiPoint2 v105;
	NiPoint2 v106;
	NiPoint2 v107;
	NiPoint2 v108;
	NiPoint2 v109;
	NiPoint2 v110;
	NiPoint2 v111;
	NiPoint2 v112;
	NiPoint3 v115;
	NiPoint3 v118;
	NiPoint3 v121;
	NiPoint3 v124;
	NiPoint3 v127;
	NiPoint3 v130;
	NiPoint3 v133;
	NiPoint3 v136;
	void* apMemory = NiMemObject::operator new(sizeof(NiNode));
	if (apMemory) {
		pnode = ::new (apMemory) NiNode(0);
	} else {
		pnode = 0;
	}
	int32_t numObjs = 4 * auiCount;
	void* ptr = ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (4 * auiCount) : SIZE_MAX);
	if (ptr) {
		rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), numObjs, &emptyPointConstructor);
		pverticesA = static_cast<NiPoint3*>(ptr);
	} else {
		pverticesA = 0;
	}
	int32_t count = 4 * auiCount;
	void* v145 = ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * (4 * auiCount) : SIZE_MAX);
	if (v145) {
		rt::eh_vector_constructor_iterator(v145, sizeof(NiPoint2), count, &emptyPointConstructor);
		ptexcoordsA = static_cast<NiPoint2*>(v145);
	} else {
		ptexcoordsA = 0;
	}
	uint16_t* v144 =
		static_cast<uint16_t*>(ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount)
																					 : SIZE_MAX));
	NiPoint3* pflareVerticesA_2 = 0;
	NiPoint2* pflareTexcoordsA_2 = 0;
	uint16_t* pflareIndicesA = 0;
	if (BSShaderManager::bHDR) {
		NiPoint2* pflareTexcoordsA;
		NiPoint3* pflareVerticesA;
		int32_t aiCount = 4 * auiCount;
		void* v141 =
			ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (4 * auiCount) : SIZE_MAX);
		if (v141) {
			rt::eh_vector_constructor_iterator(v141, sizeof(NiPoint3), aiCount, &emptyPointConstructor);
			pflareVerticesA = static_cast<NiPoint3*>(v141);
		} else {
			pflareVerticesA = 0;
		}
		pflareVerticesA_2 = pflareVerticesA;
		int32_t v140 = 4 * auiCount;
		void* v138 =
			ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * (4 * auiCount) : SIZE_MAX);
		if (v138) {
			rt::eh_vector_constructor_iterator(v138, sizeof(NiPoint2), v140, &emptyPointConstructor);
			pflareTexcoordsA = static_cast<NiPoint2*>(v138);
		} else {
			pflareTexcoordsA = 0;
		}
		pflareTexcoordsA_2 = pflareTexcoordsA;
		uint16_t* v137 = static_cast<uint16_t*>(ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t)
				? sizeof(uint16_t) * (6 * auiCount)
				: SIZE_MAX));
		pflareIndicesA = v137;
	}
	for (uint32_t i = 0; i < auiCount; ++i) {
		double fvoffset;
		double fuoffset;
		NiMatrix3 rotationMatrix;
		float fx = RandomFloatNeg1To1() * ScreenSplatter::fWRadius;
		float fy = RandomFloatNeg1To1() * ScreenSplatter::fHRadius;
		NiPoint3 translate(fx, fy, 0.0f);
		float BloodSplatterMaxSize = fBloodSplatterMaxSize.Float();
		float* v1 = &fBloodSplatterMinSize.Float();
		float fscale = RandomFloat(*v1, BloodSplatterMaxSize);
		float BloodSplatterMaxSizea = fBloodSplatterMaxOpacity.Float();
		float* v2 = &fBloodSplatterMinOpacity.Float();
		float falpha1 = RandomFloat(*v2, BloodSplatterMaxSizea);
		float BloodSplatterMaxSizeb = fBloodSplatterMaxOpacity2.Float();
		float* v3 = &fBloodSplatterMinOpacity2.Float();
		float falpha2 = RandomFloat(*v3, BloodSplatterMaxSizeb);
		rotationMatrix.MakeIdentity();
		float* v4 = &fBloodSplatterOpacityChance.Float();
		if (RandomBool(*v4)) {
			falpha1 = falpha2;
		}
		if (!(RandomFloatNeg1To1() > 0.0)) {
			fuoffset = 0.5;
		} else {
			fuoffset = 0.0;
		}
		float fuoffset_2 = fuoffset;
		if (!(RandomFloatNeg1To1() > 0.0)) {
			fvoffset = 0.5;
		} else {
			fvoffset = 0.0;
		}
		float fvoffset_2 = fvoffset;
		NiPoint2 texTranslate(fuoffset_2, fvoffset_2);
		float fflareSize = fBloodSplatterFlareSize.Float();
		uint32_t uibase = 4 * i;
		NiPoint3* v5 = ::new (&v136) NiPoint3(-fscale, fscale, falpha1);
		pverticesA[uibase] = rotationMatrix * *v5 + translate;
		NiPoint3* v8 = ::new (&v133) NiPoint3(fscale, fscale, falpha1);
		pverticesA[uibase + 1] = rotationMatrix * *v8 + translate;
		float fY = -fscale;
		NiPoint3* v11 = ::new (&v130) NiPoint3(fY, fY, falpha1);
		pverticesA[uibase + 2] = rotationMatrix * *v11 + translate;
		NiPoint3* v14 = ::new (&v127) NiPoint3(fscale, -fscale, falpha1);
		pverticesA[uibase + 3] = rotationMatrix * *v14 + translate;
		if (BSShaderManager::bHDR) {
			NiPoint3* v17 = ::new (&v124) NiPoint3(-fflareSize, fflareSize, falpha1);
			pflareVerticesA_2[uibase] = rotationMatrix * *v17 + translate;
			NiPoint3* v20 = ::new (&v121) NiPoint3(fflareSize, fflareSize, falpha1);
			pflareVerticesA_2[uibase + 1] = rotationMatrix * *v20 + translate;
			float fYb = -fflareSize;
			NiPoint3* v23 = ::new (&v118) NiPoint3(fYb, fYb, falpha1);
			pflareVerticesA_2[uibase + 2] = rotationMatrix * *v23 + translate;
			NiPoint3* v26 = ::new (&v115) NiPoint3(fflareSize, -fflareSize, falpha1);
			pflareVerticesA_2[uibase + 3] = rotationMatrix * *v26 + translate;
		}
		NiPoint2* v29 = ::new (&v112) NiPoint2(0.5f, 0.5f);
		NiPoint2* v30 = ::new (&v111) NiPoint2(*v29 + texTranslate);
		float x = v30->x;
		float y = v30->y;
		uint32_t v33 = uibase;
		ptexcoordsA[uibase].x = x;
		ptexcoordsA[v33].y = y;
		NiPoint2* v35 = ::new (&v110) NiPoint2(0.0f, 0.5f);
		NiPoint2* v36 = ::new (&v109) NiPoint2(*v35 + texTranslate);
		float v37 = v36->x;
		float v38 = v36->y;
		uint32_t v39 = uibase;
		ptexcoordsA[uibase + 1].x = v37;
		ptexcoordsA[v39 + 1].y = v38;
		NiPoint2* v41 = ::new (&v108) NiPoint2(0.5f, 0.0f);
		NiPoint2* v42 = ::new (&v107) NiPoint2(*v41 + texTranslate);
		float v43 = v42->x;
		float v44 = v42->y;
		uint32_t v45 = uibase;
		ptexcoordsA[uibase + 2].x = v43;
		ptexcoordsA[v45 + 2].y = v44;
		NiPoint2* v47 = ::new (&v106) NiPoint2(0.0f, 0.0f);
		NiPoint2* v48 = ::new (&v105) NiPoint2(*v47 + texTranslate);
		float v49 = v48->x;
		float v50 = v48->y;
		uint32_t v51 = uibase;
		ptexcoordsA[uibase + 3].x = v49;
		ptexcoordsA[v51 + 3].y = v50;
		if (BSShaderManager::bHDR) {
			NiPoint2* v53 = ::new (&v104) NiPoint2(1.0f, 1.0f);
			float v54 = v53->x;
			float v55 = v53->y;
			uint32_t v56 = uibase;
			pflareTexcoordsA_2[uibase].x = v54;
			pflareTexcoordsA_2[v56].y = v55;
			NiPoint2* v58 = ::new (&v103) NiPoint2(0.0f, 1.0f);
			float v59 = v58->x;
			float v60 = v58->y;
			uint32_t v61 = uibase;
			pflareTexcoordsA_2[uibase + 1].x = v59;
			pflareTexcoordsA_2[v61 + 1].y = v60;
			NiPoint2* v63 = ::new (&v102) NiPoint2(1.0f, 0.0f);
			float v64 = v63->x;
			float v65 = v63->y;
			uint32_t v66 = uibase;
			pflareTexcoordsA_2[uibase + 2].x = v64;
			pflareTexcoordsA_2[v66 + 2].y = v65;
			NiPoint2* v68 = ::new (&v101) NiPoint2(0.0f, 0.0f);
			float v69 = v68->x;
			float v70 = v68->y;
			uint32_t v71 = uibase;
			pflareTexcoordsA_2[uibase + 3].x = v69;
			pflareTexcoordsA_2[v71 + 3].y = v70;
		}
		uint32_t uiindexBase = 6 * i;
		v144[6 * i] = uibase;
		v144[uiindexBase + 1] = uibase + 2;
		v144[uiindexBase + 2] = uibase + 1;
		v144[uiindexBase + 3] = uibase + 1;
		v144[uiindexBase + 4] = uibase + 2;
		v144[uiindexBase + 5] = uibase + 3;
		if (BSShaderManager::bHDR) {
			pflareIndicesA[uiindexBase] = uibase;
			pflareIndicesA[uiindexBase + 1] = uibase + 2;
			pflareIndicesA[uiindexBase + 2] = uibase + 1;
			pflareIndicesA[uiindexBase + 3] = uibase + 1;
			pflareIndicesA[uiindexBase + 4] = uibase + 2;
			pflareIndicesA[uiindexBase + 5] = uibase + 3;
		}
	}
	NiTriShape* v100 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
	if (v100) {
		pgeom = ::new (v100) NiTriShape(4 * auiCount, pverticesA, nullptr, nullptr, ptexcoordsA, 1,
			NiGeometryData::NBT_METHOD_NONE, 2 * auiCount, v144);
	} else {
		pgeom = 0;
	}
	NiTriShape* pgeom_2 = pgeom;
	BSShaderBloodSplatterProperty* v99 =
		static_cast<BSShaderBloodSplatterProperty*>(NiMemObject::operator new(sizeof(BSShaderBloodSplatterProperty)));
	if (v99) {
		pshaderprop = ::new (v99) BSShaderBloodSplatterProperty;
	} else {
		pshaderprop = 0;
	}
	BSShaderBloodSplatterProperty* pshaderprop_2 = pshaderprop;
	NiTexture* BloodSplatterMaxSizec = ScreenSplatter::spTextures[1].m_pObject;
	pshaderprop_2->SetTexture(0, BloodSplatterMaxSizec);
	NiTexture* BloodSplatterMaxSized = ScreenSplatter::spTextures->m_pObject;
	pshaderprop_2->SetTexture(1u, BloodSplatterMaxSized);
	pgeom_2->AttachProperty(pshaderprop_2);
	BSShaderManager::PrepareObject(pgeom_2, 0, 0);
	pnode->AttachChild(pgeom_2, 1);
	if (BSShaderManager::bHDR) {
		BSShaderBloodSplatterProperty* pshaderprop_3;
		NiTriShape* pgeom_3;
		NiTriShape* v98 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
		if (v98) {
			pgeom_3 = ::new (v98) NiTriShape(4 * auiCount, pflareVerticesA_2, nullptr, nullptr, pflareTexcoordsA_2, 1,
				NiGeometryData::NBT_METHOD_NONE, 2 * auiCount, pflareIndicesA);
		} else {
			pgeom_3 = 0;
		}
		pgeom_2 = pgeom_3;
		BSShaderBloodSplatterProperty* v97 = static_cast<
			BSShaderBloodSplatterProperty*>(NiMemObject::operator new(sizeof(BSShaderBloodSplatterProperty)));
		if (v97) {
			pshaderprop_3 = ::new (v97) BSShaderBloodSplatterProperty;
		} else {
			pshaderprop_3 = 0;
		}
		pshaderprop_2 = pshaderprop_3;
		NiTexture* BloodSplatterMaxSizee = ScreenSplatter::spTextures[2].m_pObject;
		pshaderprop_2->SetTexture(0, BloodSplatterMaxSizee);
		pgeom_2->AttachProperty(pshaderprop_2);
		BSShaderManager::PrepareObject(pgeom_2, 0, 0);
		pnode->AttachChild(pgeom_2, 1);
	}
	return pnode;
}

BSSplatterExtraData::BSSplatterExtraData(const BSSplatterData& aSplatterData)
	: NiExtraData(BSSplatterExtraData::GetTag()), SplatterData(aSplatterData) {}

BSSplatterExtraData::~BSSplatterExtraData() {}

NiFixedString* BSSplatterExtraData::GetTag() {
	return BSSplatterExtraData::pTag;
}

void BSSplatterExtraData::InitTag() {
	NiFixedString* v0;
	void* v1 = ourMalloc(sizeof(NiFixedString));
	if (v1) {
		v0 = ::new (static_cast<void*>(v1)) NiFixedString(const_cast<char*>("SPD"));
	} else {
		v0 = nullptr;
	}
	BSSplatterExtraData::pTag = v0;
}

void BSSplatterExtraData::KillSDM() {
	if (BSSplatterExtraData::pTag) {
		delete BSSplatterExtraData::pTag;
	}
	BSSplatterExtraData::pTag = 0;
}
