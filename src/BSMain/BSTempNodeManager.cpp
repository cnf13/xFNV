#include "BSMain/BSTempNodeManager.h"

#include "Fallout Shared/BSTempNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

NiRTTI BSTempNodeManager::ms_RTTI("BSTempNodeManager", &NiNode::ms_RTTI);
NiRTTI BSTempNode::ms_RTTI("BSTempNode", &NiNode::ms_RTTI);

BSTempNodeManager::BSTempNodeManager() : NiNode(0) {}

BSTempNodeManager::~BSTempNodeManager() {}

const NiRTTI* BSTempNodeManager::GetRTTI() const {
    return &BSTempNodeManager::ms_RTTI;
}

void BSTempNodeManager::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    RemoveExpired(UpdateData.fTime);
    NiNode::UpdateDownwardPass(UpdateData, uFlags);
}

void BSTempNodeManager::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    RemoveExpired(UpdateData.fTime);
    NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
}

void BSTempNodeManager::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    RemoveExpired(UpdateData.fTime);
    NiNode::UpdateRigidDownwardPass(UpdateData, uFlags);
}

void BSTempNodeManager::RemoveExpired(float afTime) {
    double fPrevTime;
    NiPointer<NiAVObject> v13;
    if (this->fPrevTime == 0.0) {
        fPrevTime = this->fPrevTime;
    }
    else {
        fPrevTime = afTime - this->fPrevTime;
    }
    this->fPrevTime = afTime;
    uint32_t m_usSize = m_kChildren.m_usSize;
    while (m_usSize--) {
        BSTempNode* v11;
        NiAVObject* m_pObject = m_kChildren.m_pBase[m_usSize].m_pObject;
        if (m_pObject) {
            char v7 = 0;
            for (const NiRTTI* i = m_pObject->GetRTTI(); i; i = i->GetBaseRTTI()) {
                if (i == &BSTempNode::ms_RTTI) {
                    v7 = 1;
                    break;
                }
            }
            v11 = v7 != 0 ? static_cast<BSTempNode*>(m_pObject) : nullptr;
        }
        else {
            v11 = nullptr;
        }
        if (v11) {
            float v9 = fPrevTime;
            float v6 = v11->fTime - v9;
            v11->fTime = v6;
            if (v11->fTime < 0.0) {
                v11->fTime = 0.0;
            }
            if (v11->fTime == 0.0) {
                DetachChildAt(m_usSize, v13);
            }
        }
    }
}
