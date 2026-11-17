#include "BSShader/BSFadeNodeCuller.h"

#include "BSShader/BSFadeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"

NiRTTI BSFadeNodeCuller::ms_RTTI("BSFadeNodeCuller", &NiCullingProcess::ms_RTTI);

const NiRTTI* BSFadeNodeCuller::GetRTTI() const {
	return &BSFadeNodeCuller::ms_RTTI;
}

void BSFadeNodeCuller::Process(NiAVObject* apObject) {
	BSFadeNode* pn = apObject->IsFadeNode();
	if (pn) {
		pn->m_uFlags |= NiAVObject::PREPROCESSED_NODE;
		uint32_t m_uFlags = pn->m_uFlags;
		uint32_t i = 0;
		float* p_z = &m_kPlanes.m_akCullingPlanes[0u].m_kNormal.z;
		do {
			const NiBound* WorldBound = &pn->GetWorldBound();
			float apObjecta = *(p_z - 1) * WorldBound->m_kCenter.y + *(p_z - 2) * WorldBound->m_kCenter.x +
				WorldBound->m_kCenter.z * *p_z;
			float apObjectb = apObjecta - p_z[1];
			if (-WorldBound->m_fRadius >= apObjectb) {
				break;
			}
			++i;
			p_z += 4;
		} while (i < 6);
		if (i == 6) {
			pn->m_uFlags = m_uFlags & ~NiAVObject::NOT_VISIBLE;
		} else {
			pn->m_uFlags = m_uFlags | NiAVObject::NOT_VISIBLE;
		}
	}
}

void BSFadeNodeCuller::Process(const NiCamera* apCamera, NiAVObject* apScene, NiVisibleArray* apVisibleSet) {
	m_pkCamera = const_cast<NiCamera*>(apCamera);
}
