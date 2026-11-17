#include "BSMain/BSSceneGraph.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>

SettingT<INISettingCollection> BSSceneGraph::fNoLODFarDistanceMin("fNoLODFarDistanceMin:Display", 1700.0f);
SettingT<INISettingCollection> BSSceneGraph::fNoLODFarDistanceMax("fNoLODFarDistanceMax:Display", 10240.0f);
SettingT<INISettingCollection> BSSceneGraph::fNoLODFarDistancePct("fNoLODFarDistancePct:Display", 1.0f);
SettingT<INISettingCollection> BSSceneGraph::fNearDistance("fNearDistance:Display", 5.0f);
SettingT<INISettingCollection> BSSceneGraph::fDefaultFOV("fDefaultFOV:Display", 75.0f);
SettingT<INISettingCollection> BSSceneGraph::fDefaultWorldFOV("fDefaultWorldFOV:Display", 75.0f);
SettingT<INISettingCollection> BSSceneGraph::fDefault1stPersonFOV("fDefault1stPersonFOV:Display", 75.0f);
SettingT<INISettingCollection> BSSceneGraph::fPipboy1stPersonFOV("fPipboy1stPersonFOV:Display", 68.0f);

NiRTTI BSSceneGraph::ms_RTTI("BSSceneGraph", &NiNode::ms_RTTI);

SettingT<INISettingCollection> gs_bAutoViewDistance_Display("bAutoViewDistance:Display", false);
SettingT<INISettingCollection> gs_iAutoViewLowFrameRate_Display("iAutoViewLowFrameRate:Display", 20);
SettingT<INISettingCollection> gs_iAutoViewHiFrameRate_Display("iAutoViewHiFrameRate:Display", 40);
SettingT<INISettingCollection> gs_iAutoViewMinDistance_Display("iAutoViewMinDistance:Display", 2000);

// Names the root "<a1>Root Node", hangs a camera root node "<a1>Root CameraNode" under it, and takes a3 as the
// camera, or builds "<a1>Root Camera" with the full viewport, the scene's axes, the INI's near distance and field of
// view. Then the camera is moved under the camera root, and the scene is always drawn with a fixed bound.
BSSceneGraph::BSSceneGraph(const char* a1, bool a2, NiCamera* a3)
	: NiNode(0), spCamera(nullptr), pVisArray(nullptr), pCuller(nullptr), bMenuSceneGraph(a2) {
	char pcString[512];
	void* pCullerMemory = MemoryManager_Singleton.Allocate(sizeof(BSCullingProcess));
	pCuller = pCullerMemory ? new (pCullerMemory) BSCullingProcess(nullptr) : nullptr;
	BSsprintf(pcString, 512, "%sRoot Node", a1);
	NiFixedString kRootName(pcString);
	SetName(kRootName);
	NiNode* pCameraRoot = new NiNode(0);
	BSsprintf(pcString, 512, "%sRoot CameraNode", a1);
	NiFixedString kCameraRootName(pcString);
	pCameraRoot->SetName(kCameraRootName);
	NiNode::AttachChild(pCameraRoot, 1);
	if (a3) {
		spCamera = a3;
	} else {
		spCamera = new NiCamera();
		BSsprintf(pcString, 512, "%sRoot Camera", a1);
		NiFixedString kCameraName(pcString);
		spCamera->SetName(kCameraName);
		NiRect<float> kPort;
		kPort.m_left = 0.0f;
		kPort.m_right = 1.0f;
		kPort.m_top = 1.0f;
		kPort.m_bottom = 0.0f;
		spCamera->m_kPort = kPort;
		NiMatrix3 kRot;
		kRot.m_pEntry[0][0] = 0.0f;
		kRot.m_pEntry[0][1] = 0.0f;
		kRot.m_pEntry[0][2] = 1.0f;
		kRot.m_pEntry[1][0] = 1.0f;
		kRot.m_pEntry[1][1] = 0.0f;
		kRot.m_pEntry[1][2] = 0.0f;
		kRot.m_pEntry[2][0] = 0.0f;
		kRot.m_pEntry[2][1] = 1.0f;
		kRot.m_pEntry[2][2] = 0.0f;
		spCamera->m_kLocal.m_Rotate = kRot;
		spCamera->m_fMinNearPlaneDist = 1.0f;
		spCamera->m_fMaxFarNearRatio = BSSceneGraph::GetFarDistance() / fNearDistance.uValue.f;
		SetCameraFOV(fDefaultFOV.uValue.f, true, nullptr, false);
	}
	SetCameraScene();
	SetAlwaysDraw(true);
	m_uFlags |= NiAVObject::FIXED_BOUND;
}

float BSSceneGraph::GetFarDistance() {
	if (bMenuSceneGraph) {
		return 20480.0f;
	} else {
		Setting::Info* fNoLODFarDistanceMin;
		Setting::Info* fNoLODFarDistancePct;
		Setting::Info* fNoLODFarDistanceMax;
		// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
		fNoLODFarDistanceMax = &BSSceneGraph::fNoLODFarDistanceMax.uValue;
		float fNoLODFarDistanceMax_ = fNoLODFarDistanceMax->f;
		// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
		fNoLODFarDistancePct = &BSSceneGraph::fNoLODFarDistancePct.uValue;
		// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
		fNoLODFarDistanceMin = &BSSceneGraph::fNoLODFarDistanceMin.uValue;
		return fNoLODFarDistanceMin->f +
			(fNoLODFarDistanceMax_ - fNoLODFarDistanceMin->f) * ((fNoLODFarDistancePct->f - 0.0) / (1.0 - 0.0));
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BSSceneGraph::SetViewDistanceBasedOnFrameRate(float afTime) {
	Setting::Info* bAutoViewDistance;
	NiFrustum frustum;
	// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
	bAutoViewDistance = &gs_bAutoViewDistance_Display.uValue;
	if (bAutoViewDistance->b) {
		Setting::Info* iAutoViewHiFrameRate;
		Setting::Info* iAutoViewLowFrameRate;
		static float fCurrentDist = GetFarDistance();
		// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
		iAutoViewLowFrameRate = &gs_iAutoViewLowFrameRate_Display.uValue;
		// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
		if (afTime > 1.0 / static_cast<double>(iAutoViewLowFrameRate->i) ||
			(iAutoViewHiFrameRate = &gs_iAutoViewHiFrameRate_Display.uValue,
				afTime < 1.0 / static_cast<double>(iAutoViewHiFrameRate->i))) {
			Setting::Info* iAutoViewHiFrameRate_1;
			Setting::Info* iAutoViewLowFrameRate_1;
			Setting::Info* iAutoViewMinDistance;
			float fmaxDist = GetFarDistance();
			// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
			iAutoViewMinDistance = &gs_iAutoViewMinDistance_Display.uValue;
			float fminDist = static_cast<float>(iAutoViewMinDistance->i);
			float fcalcDist = fminDist;
			// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
			iAutoViewLowFrameRate_1 = &gs_iAutoViewLowFrameRate_Display.uValue;
			// The exe tests the setting's address (an inlined null-safe accessor), which is never null.
			iAutoViewHiFrameRate_1 = &gs_iAutoViewHiFrameRate_Display.uValue;
			float fpercent = (1.0 / afTime - static_cast<double>(iAutoViewLowFrameRate_1->i)) * 1.0 /
				static_cast<double>(iAutoViewHiFrameRate_1->i);
			if (!(fpercent > 1.0)) {
				if (fpercent > 0.0) {
					fcalcDist = (fmaxDist - fminDist) * fpercent + fcalcDist;
				}
			} else {
				fcalcDist = fmaxDist;
			}
			float fdelta = fcalcDist - fCurrentDist;
			float v2 = std::fabs(static_cast<double>(fdelta));
			if (v2 > 10.0) {
				NiCamera* pcamera = spCamera.m_pObject;
				frustum = pcamera->m_kViewFrustum;
				fCurrentDist = fdelta * afTime + fCurrentDist;
				frustum.m_fFar = fCurrentDist;
				pcamera->SetViewFrustum(frustum);
			}
		}
	}
}

// Drops the camera, deletes the culler and frees the visible set; spCamera releases itself.
BSSceneGraph::~BSSceneGraph() {
	spCamera = nullptr;
	if (pCuller)
		delete pCuller;
	// The visible set is destroyed (its destructor frees its buffer), then freed from the engine's heap.
	if (pVisArray) {
		pVisArray->~NiVisibleArray();
		MemoryManager_Singleton.Deallocate(pVisArray);
	}
}

NiCamera* BSSceneGraph::GetCamera() {
	return spCamera;
}

// Moves the camera under the scene's first child when it hangs elsewhere, then updates it. The exe reads the camera's
// parent before the child, reads the child count and array again for the attach, and calls AttachChild through the
// child's vtable (slot 55, the first NiNode one). The update data is built with fldz/fstp dword and five byte stores;
// its three padding bytes are not written.
void BSSceneGraph::SetCameraScene() {
	NiNode* pParent = spCamera.m_pObject->m_pkParent;
	NiNode* pScene;
	if (m_kChildren.m_usSize) {
		pScene = static_cast<NiNode*>(m_kChildren.m_pBase[0].m_pObject);
	} else {
		pScene = nullptr;
	}
	if (pParent != pScene) {
		NiNode* pTarget;
		if (m_kChildren.m_usSize) {
			pTarget = static_cast<NiNode*>(m_kChildren.m_pBase[0].m_pObject);
		} else {
			pTarget = nullptr;
		}
		pTarget->AttachChild(spCamera.m_pObject, true);
		NiUpdateData kData;
		kData.fTime = 0.0f;
		kData.bUpdateControllers = false;
		kData.bParallelUpdate = false;
		kData.bFoundParticles = false;
		kData.bFoundMorphController = false;
		kData.bSceneGraphChange = false;
		spCamera.m_pObject->Update(kData);
	}
}

// Sets the camera's frustum for a field of view of afFOV degrees (apCamera null: the scene's own camera, whose
// fCurrentFOV is updated, and nothing but the LOD multiplier changes when the FOV is unchanged and abForce is false).
// The exe reads both Settings through the inlined Setting::Float (their addresses are never null). Every float local
// the exe stores with fstp dword is a float here; the double casts reproduce the x87 operations between the stores.
void BSSceneGraph::SetCameraFOV(float afFOV, bool abForce, NiCamera* apCamera, bool abSetLODMult) {
	NiCamera* pCamera = apCamera;
	if (!pCamera)
		pCamera = spCamera.m_pObject;
	if (!apCamera) {
		// fucompp, test ah,0x44, jp: only an ordered equal FOV takes the early path.
		if (!abForce && afFOV == fCurrentFOV) {
			if (abSetLODMult) {
				const float fLODAdjust = (float)((double)afFOV / (double)fDefaultFOV.uValue.f);
				pCamera->m_fLODAdjust = fLODAdjust;
			}
			return;
		}
		fCurrentFOV = afFOV;
	}
	// Half the field of view in radians: two qword constants (DEGREES_2_RADIAN 0x1023128, 0.5 0x1011588).
	const float fHalfFOV = (float)((double)afFOV * 0.01745329238474369 * 0.5);
	NiFrustum kFrustum;
	kFrustum.~NiFrustum();
	::new (&kFrustum) NiFrustum(false);
	kFrustum.m_fFar = GetFarDistance();
	kFrustum.m_fNear = fNearDistance.uValue.f;
	const float fRatio = (float)((double)kFrustum.m_fFar / (double)kFrustum.m_fNear);
	pCamera->m_fMaxFarNearRatio = fRatio;

	NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
	float fHeight;
	float fWidth;
	if (pRenderer) {
		fHeight = (float)((double)pRenderer->GetScreenHeight() *
			((double)pCamera->m_kPort.m_top - (double)pCamera->m_kPort.m_bottom));
		fWidth = (float)((double)pRenderer->GetScreenWidth() *
			((double)pCamera->m_kPort.m_right - (double)pCamera->m_kPort.m_left));
	} else {
		fHeight = (float)((double)pCamera->m_kPort.m_top - (double)pCamera->m_kPort.m_bottom);
		fWidth = (float)((double)pCamera->m_kPort.m_right - (double)pCamera->m_kPort.m_left);
	}
	// A render target other than the default one (a menu's texture) gives the size instead.
	// The exe calls GetCurrentRenderTargetGroup again for each use, the second time before GetDefaultRenderTargetGroup.
	if (pRenderer && pRenderer->GetCurrentRenderTargetGroup()) {
		NiRenderTargetGroup* pCurrent = pRenderer->GetCurrentRenderTargetGroup();
		if (pCurrent != pRenderer->GetDefaultRenderTargetGroup()) {
			fHeight = (float)pRenderer->GetCurrentRenderTargetGroup()->GetBuffer(0)->m_uiHeight;
			fWidth = (float)pRenderer->GetCurrentRenderTargetGroup()->GetBuffer(0)->m_uiWidth;
		}
	}
	const float fAspect = (float)((double)fHeight / (double)fWidth);
	// The exe tests the apCamera argument here, not the camera it fell back to; it divides the qword 0.75
	// (0x101DE30) by the ratio recomputed from fHeight and fWidth.
	float fScale;
	if (bMenuSceneGraph && !apCamera)
		fScale = 1.0f;
	else
		fScale = (float)(0.75 / ((double)fHeight / (double)fWidth));

	const float fTan0 = (float)std::tan((double)fHalfFOV);
	kFrustum.m_fLeft = (float)(-(double)fTan0 * (double)fScale);
	const float fTan1 = (float)std::tan((double)fHalfFOV);
	kFrustum.m_fRight = (float)((double)fTan1 * (double)fScale);
	const float fTan2 = (float)std::tan((double)fHalfFOV);
	kFrustum.m_fBottom = (float)(-(double)fTan2 * (double)fAspect * (double)fScale);
	const float fTan3 = (float)std::tan((double)fHalfFOV);
	kFrustum.m_fTop = (float)((double)fTan3 * (double)fAspect * (double)fScale);
	pCamera->SetViewFrustum(kFrustum);

	NiUpdateData kData;
	kData.fTime = 0.0f;
	kData.bUpdateControllers = false;
	kData.bParallelUpdate = false;
	kData.bFoundParticles = false;
	kData.bFoundMorphController = false;
	kData.bSceneGraphChange = false;
	pCamera->Update(kData);
	if (abSetLODMult) {
		const float fLODAdjust = (float)((double)afFOV / (double)fDefaultFOV.uValue.f);
		pCamera->m_fLODAdjust = fLODAdjust;
	}
}

NiNode* BSSceneGraph::GetCameraRoot() {
	return static_cast<NiNode*>(NiNode::GetAtChecked(0));
}

float BSSceneGraph::GetCameraFOV() {
	return fCurrentFOV;
}

void BSSceneGraph::UpdateCameraFOV() {
	NiCamera* m_pObject;
	m_pObject = spCamera.m_pObject;
	m_pObject->m_kPort.m_left = (float)0.0;
	m_pObject->m_kPort.m_right = (float)1.0;
	m_pObject->m_kPort.m_top = (float)1.0;
	m_pObject->m_kPort.m_bottom = (float)0.0;
	SetCameraFOV(fCurrentFOV, 1, 0, 0);
}
