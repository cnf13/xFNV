#include "Fallout Shared/BSBipNode.h"

#include "BSHavok/bhkNiCollisionObject.h"
#include "Fallout Shared/BSTransCon.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

void BSBipNode::UpdateAllButBip(float afTime) {
    for (NiTimeController* pkControl = GetControllers(); pkControl; pkControl = pkControl->GetNext()) {
        if (pkControl->GetRTTI() == &NiMultiTargetTransformController::ms_RTTI) {
            // BSTransCon adds no data to the multi-target transform controller the RTTI test found.
            static_cast<BSTransCon*>(pkControl)->UpdateAllButTarget(afTime, false);
        }
        else if (pkControl->GetRTTI() == &BSMultiTargetTreadTransfController::ms_RTTI) {
            NiUpdateData updateData(afTime, false, false);
            pkControl->Update(updateData);
        }
    }
}

// Removes apObject from the targets of every multi-target transform controller on this node.
void BSBipNode::RemoveObject(NiAVObject* apObject) {
    for (NiTimeController* pkController = GetControllers(); pkController; pkController = pkController->GetNext()) {
        NiMultiTargetTransformController* pkMultiTarget = static_cast<NiMultiTargetTransformController*>(
            NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, pkController)
        );
        BSMultiTargetTreadTransfController* pkTread = static_cast<BSMultiTargetTreadTransfController*>(
            NiObject::DynamicCastStatic(&BSMultiTargetTreadTransfController::ms_RTTI, pkController)
        );
        if (pkMultiTarget) {
            pkMultiTarget->RemoveNodeRecurse(static_cast<NiNode*>(apObject));
        }
        if (pkTread) {
            pkTread->RemoveNodeRecurse(static_cast<NiNode*>(apObject));
        }
    }
}

void BSBipNode::UpdateBipOnly(float afTime, NiAVObject* apAccumRoot) {
    bhkNiCollisionObject::SetSyncMode(bhkNiCollisionObject::SYNC_NONE);
    for (BSMultiTargetTreadTransfController* pkController =
             static_cast<BSMultiTargetTreadTransfController*>(GetControllers());
         pkController;
         pkController = static_cast<BSMultiTargetTreadTransfController*>(pkController->GetNext())) {
        bool bMultiTarget = NiObject::IsExactKindOf(&NiMultiTargetTransformController::ms_RTTI, pkController);
        if (!NiObject::IsExactKindOf(&BSMultiTargetTreadTransfController::ms_RTTI, pkController) && !bMultiTarget) {
            NiUpdateData kUpdateData(afTime, false, false);
            pkController->Update(kUpdateData);
            break;
        }
    }
    if (apAccumRoot) {
        NiUpdateData kUpdateData(afTime, true, false);
        apAccumRoot->UpdateObjectControllers(kUpdateData, true);
    }
    bhkNiCollisionObject::SetSyncMode(bhkNiCollisionObject::SYNC_BOTH);
}

void bhkNiCollisionObject::SetSyncMode(bhkNiCollisionObject::SYNCMODE aeMode) {
    eHavokSyncMode = aeMode;
}

void BSTransCon::UpdateAllButTarget(float afTime, bool abSkipSelectiveUpdateTest) {
    if (GetActive() && m_usNumInterps) {
        int32_t i;
        NiQuatTransform kTransform;
        int32_t iLast = m_usNumInterps - 1;
        for (i = 0; i < iLast; ++i) {
            NiNode* pkTarget = static_cast<NiNode*>(m_ppkTargets[i]);
            if (pkTarget &&
                (abSkipSelectiveUpdateTest || pkTarget->GetSelectiveUpdate()) &&
                (!abSkipSelectiveUpdateTest || !pkTarget->GetSelectiveUpdate()) &&
                m_pkBlendInterps[i].Update(afTime, pkTarget, kTransform)) {
                if (kTransform.IsTranslateValid()) {
                    pkTarget->SetTranslate(kTransform.m_kTranslate);
                }
                if (kTransform.IsRotateValid()) {
                    pkTarget->SetRotate_0(kTransform.m_kRotate);
                }
                if (kTransform.IsScaleValid()) {
                    pkTarget->SetScale(kTransform.m_fScale);
                }
            }
        }
        NiNode* pkTarget = static_cast<NiNode*>(m_ppkTargets[i]);
        if (pkTarget &&
            (abSkipSelectiveUpdateTest || pkTarget->GetSelectiveUpdate()) &&
            m_pkBlendInterps[i].Update(afTime, pkTarget, kTransform)) {
            if (kTransform.IsTranslateValid()) {
                pkTarget->SetTranslate(kTransform.m_kTranslate);
            }
            if (kTransform.IsRotateValid()) {
                pkTarget->SetRotate_0(kTransform.m_kRotate);
            }
            if (kTransform.IsScaleValid()) {
                pkTarget->SetScale(kTransform.m_fScale);
            }
        }
    }
}

void NiAVObject::UpdateObjectControllers(NiUpdateData& UpdateData, bool bProperties) {
    if (bProperties && static_cast<uint8_t>(QHasPropController())) {
        auto* pkPosition = m_kPropertyList.GetHeadPos();
        while (pkPosition) {
            NiPointer<NiProperty>& spProperty = m_kPropertyList.GetNext(pkPosition);
            NiProperty* pkProperty = spProperty;
            if (pkProperty && pkProperty->GetControllers()) {
                pkProperty->Update(UpdateData);
            }
        }
    }
    for (NiTimeController* pkController = GetControllers(); pkController; pkController = pkController->GetNext()) {
        pkController->Update(UpdateData);
    }
}

void NiAVObject::SetRotate_0(const NiQuaternion& kQuat) {
    kQuat.ToRotation(m_kLocal.m_Rotate);
}
