#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

void NiAVObject::UpdateWorldLoc() {
	NiNode* pkParent = m_pkParent;
	if (pkParent) {
		if (m_uFlags & TRANS_IS_IDENTITY) {
			m_kWorld = pkParent->m_kWorld;
		} else {
			m_kWorld = pkParent->m_kWorld * m_kLocal;
		}
	} else {
		m_kWorld = m_kLocal;
	}
}
