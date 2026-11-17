#include "BSMain/BSCullingProcess.h"

#include "BSMain/BSCompoundFrustum.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"

NiRTTI BSCullingProcess::ms_RTTI("BSCullingProcess", &NiCullingProcess::ms_RTTI);

BSCullingProcess::BSCullingProcess(NiVisibleArray* apVisibleSet)
	: NiCullingProcess(apVisibleSet), kCullMode(BSCP_CULL_NORMAL), iCTStackIndex(0), pCompoundFrustum(nullptr),
	  spAccumulator(nullptr) {}

BSCullingProcess::~BSCullingProcess() {}

void BSCullingProcess::Process(NiAVObject* pkObject) {
	if (kCullMode == BSCullingProcess::BSCP_CULL_ALLFAIL || kCullMode == BSCullingProcess::BSCP_CULL_ALLPASS) {
		if (kCullMode != BSCullingProcess::BSCP_CULL_ALLPASS) {
			return;
		}
		goto LABEL_23;
	}
	if ((pkObject->m_uFlags & NiAVObject::PREPROCESSED_NODE) != 0) {
		if ((pkObject->m_uFlags & NiAVObject::NOT_VISIBLE) == 0) {
			pkObject->OnVisible(*this);
		}
		return;
	}
	if (!pCompoundFrustum ||
		(!pCompoundFrustum->iFreeOp || kCullMode == BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS)) {
		if ((pkObject->m_uFlags & NiAVObject::ALWAYS_DRAW) == 0 && m_kPlanes.GetActivePlaneState()) {
			DoCulling(pkObject);
			return;
		}
		goto LABEL_23;
	}
	if ((pkObject->m_uFlags & NiAVObject::ALWAYS_DRAW) != 0 ||
		((!pCompoundFrustum || !pCompoundFrustum->iFreeOp) && !m_kPlanes.GetActivePlaneState())) {
	LABEL_23:;
		pkObject->OnVisible(*this);
		return;
	}
	int32_t* planeCount = pCompoundFrustum->GetActivePlaneState();
	if (pCompoundFrustum->Process(pkObject)) {
		pkObject->OnVisible(*this);
	}
	pCompoundFrustum->SetActivePlaneState(planeCount);
}

// The exe's cleanup-only C++ EH frame is left out.
void BSCullingProcess::Process(const NiCamera* pkCamera, NiAVObject* pkScene, NiVisibleArray* pkVisibleSet) {
	if (pkCamera) {
		if (pkScene) {
			// The PDB's m_pkCamera is const NiCamera*; src's field is NiCamera* until it is retyped (its users span
			// many files), as in NiCullingProcess::Process.
			m_pkCamera = const_cast<NiCamera*>(pkCamera);
			SetFrustum(m_pkCamera->m_kViewFrustum);
			NiVisibleArray* pOrgVisibleSet = 0;
			if (pkVisibleSet) {
				pOrgVisibleSet = m_pkVisibleSet;
				m_pkVisibleSet = pkVisibleSet;
			}
			NiPointer<NiAccumulator> spSorter;
			if (!m_pkVisibleSet) {
				// The PDB's spSorter = spAccumulator: the exe calls NiPointer<T>::operator=(const NiPointer&) out of
				// line, its NiPointer<NiRefObject> copy.
				spSorter.SetPointer_(&spAccumulator);
				if (!spSorter.m_pObject) {
					spSorter.operator=(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject);
				}
				if (spSorter.m_pObject) {
					spSorter.m_pObject->StartAccumulating(pkCamera);
				}
			}
			pkScene->Cull(*this);
			if (pkVisibleSet) {
				m_pkVisibleSet = pOrgVisibleSet;
			}
		}
	}
}

void BSCullingProcess::AppendVirtual(NiGeometry& kVisible) {
	bool bRegistered = 0;
	if (spAccumulator.m_pObject) {
		bRegistered = spAccumulator.m_pObject->RegisterObject(&kVisible);
	}
	if (!bRegistered) {
		const char* m_kHandle = kVisible.m_kName.GetHandle();
		if (m_kHandle) {
			m_kHandle = kVisible.m_kName.GetHandle();
		}
	}
}

void BSCullingProcess::SetAccumulator(NiAccumulator* apAccumulator) {
	// The exe calls NiPointer's folded raw-pointer assignment (0x66B0D0), a transparent copy of the inline operator=.
	spAccumulator = apAccumulator;
}

BSShaderAccumulator* BSCullingProcess::GetAccumulator() const {
	return static_cast<BSShaderAccumulator*>(static_cast<NiAccumulator*>(spAccumulator));
}

void BSCullingProcess::SetCullMode(BSCPCullingType abTF) {
	kCullMode = abTF;
}

void BSCullingProcess::PushCullMode(BSCPCullingType aeType) {
	eTypeStackA[iCTStackIndex++] = kCullMode;
	kCullMode = aeType;
}

void BSCullingProcess::PopCullMode() {
	kCullMode = eTypeStackA[--iCTStackIndex];
}

void BSCullingProcess::ClearSharedMap() {
	// made without a null check
	spAccumulator.m_pObject->ClearSharedMap();
}

bool BSCullingProcess::AddShared(NiAVObject* apObject) {
	return (bool)((uint8_t)spAccumulator.m_pObject->AddShared(apObject));
}
