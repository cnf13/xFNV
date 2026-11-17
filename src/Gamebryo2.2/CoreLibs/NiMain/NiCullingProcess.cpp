#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

NiRTTI NiCullingProcess::ms_RTTI("NiCullingProcess", nullptr);

// AppendVirtual (0x872ED0) is a debug-build stub that calls DebugBreak; it stays with the exe.
NiCullingProcess::NiCullingProcess(NiVisibleArray* pkVisibleSet)
	: m_bUseVirtualAppend(false), m_pkVisibleSet(pkVisibleSet), m_pkCamera(nullptr),
	  m_kFrustum(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false) {}

NiCullingProcess::~NiCullingProcess() {}

// Without a visible set the renderer's sorter collects the visible geometry and draws it at the end.
void NiCullingProcess::Process(const NiCamera* pkCamera, NiAVObject* pkScene, NiVisibleArray* pkVisibleSet) {
	if (!pkCamera || !pkScene)
		return;

	m_pkCamera = const_cast<NiCamera*>(pkCamera);
	SetFrustum(pkCamera->m_kViewFrustum);

	NiVisibleArray* pkSaveVisibleSet = nullptr;
	if (pkVisibleSet) {
		pkSaveVisibleSet = m_pkVisibleSet;
		m_pkVisibleSet = pkVisibleSet;
	}

	NiPointer<NiAccumulator> spSorter;
	if (!m_pkVisibleSet) {
		spSorter = NiRenderer::ms_pkRenderer->m_spAccum.m_pObject;
		if (spSorter)
			spSorter->StartAccumulating(pkCamera);
	}

	pkScene->Cull(*this);

	if (spSorter) {
		NiRenderer::ms_pkRenderer->SetCameraData(pkCamera);
		spSorter->FinishAccumulating();
	}

	if (pkVisibleSet)
		m_pkVisibleSet = pkSaveVisibleSet;

	m_pkCamera = nullptr;
}

void NiCullingProcess::Process(NiAVObject* pkObject) {
	uint32_t uiFlags = pkObject->m_uFlags;
	if (uiFlags & NiAVObject::PREPROCESSED_NODE) {
		if (!(uiFlags & NiAVObject::NOT_VISIBLE))
			pkObject->OnVisible(*this);
	} else if ((uiFlags & NiAVObject::ALWAYS_DRAW) || !m_kPlanes.IsAnyPlaneActive()) {
		pkObject->OnVisible(*this);
	} else {
		DoCulling(pkObject);
	}
}

void NiCullingProcess::AppendVirtual(NiGeometry& kVisible) {
	platform::DebuggerBreak();
}

// IsNode through IsBhkLimitedHingeConstraint are the engine-wide "return null" stub 0xACBB70; it stays with the exe.
void NiCullingProcess::SetCamera(NiCamera* apCamera) {
	m_pkCamera = apCamera;
}

void NiCullingProcess::SetFrustum(const NiFrustum& kFrustum) {
	if (!m_pkCamera)
		return;

	// The exe copies the frustum with rep movsd, first dword first (padding included): an overlapping source
	// reads what the copy has already written, so the members are copied in that order.
	m_kFrustum.m_fLeft = kFrustum.m_fLeft;
	m_kFrustum.m_fRight = kFrustum.m_fRight;
	m_kFrustum.m_fTop = kFrustum.m_fTop;
	m_kFrustum.m_fBottom = kFrustum.m_fBottom;
	m_kFrustum.m_fNear = kFrustum.m_fNear;
	m_kFrustum.m_fFar = kFrustum.m_fFar;
	m_kFrustum.m_bOrtho = kFrustum.m_bOrtho;
	m_kFrustum.padding19[0] = kFrustum.padding19[0];
	m_kFrustum.padding19[1] = kFrustum.padding19[1];
	m_kFrustum.padding19[2] = kFrustum.padding19[2];
	m_kPlanes.Set(m_kFrustum, m_pkCamera->m_kWorld);
	// all six planes
	m_kPlanes.SetActivePlaneState(0x3F);
}

void NiCullingProcess::DoCulling(NiAVObject* apObject) {
	const NiBound* pkBound = apObject->m_kWorldBound;
	if (!pkBound || pkBound->m_fRadius == 0.0f)
		return;

	uint32_t uiSaveActive = m_kPlanes.GetActivePlaneState();
	uint32_t i;
	for (i = 0; i < NiFrustumPlanes::MAX_PLANES; i++) {
		if (!m_kPlanes.IsPlaneActive(i))
			continue;
		// 1: wholly on the positive (inner) side, 2: wholly on the negative side
		int32_t iSide = apObject->GetWorldBound().WhichSide(m_kPlanes.GetPlane(i));
		if (iSide == 2)
			break;
		if (iSide == 1)
			m_kPlanes.DisablePlane(i);
	}

	if (i == NiFrustumPlanes::MAX_PLANES)
		apObject->OnVisible(*this);

	m_kPlanes.SetActivePlaneState(uiSaveActive);
}

void NiCullingProcess::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
