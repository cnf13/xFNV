#include "Fallout Shared/BSBipNode.h"

#include "BSCore/TLSData.h"
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
        else {
            if (pkControl->GetRTTI() == &BSMultiTargetTreadTransfController::ms_RTTI) {
                NiUpdateData updateData(afTime, 0, 0);
                pkControl->Update(updateData);
            }
        }
    }
}

// Removes apObject from the targets of every multi-target transform controller on this node.
void BSBipNode::RemoveObject(NiAVObject* apObject) {
    for (NiTimeController* pkController = GetControllers(); pkController; pkController = pkController->GetNext()) {
        NiMultiTargetTransformController* pkMultiTarget;
        pkMultiTarget = static_cast<NiMultiTargetTransformController*>(
            NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, pkController)
        );
        BSMultiTargetTreadTransfController* pkTread;
        pkTread = static_cast<BSMultiTargetTreadTransfController*>(
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
    for (BSMultiTargetTreadTransfController* i = static_cast<BSMultiTargetTreadTransfController*>(GetControllers()); i;
         i = static_cast<BSMultiTargetTreadTransfController*>(i->GetNext())) {
        bool IsExactKindOf_0 = NiObject::IsExactKindOf(&NiMultiTargetTransformController::ms_RTTI, i);
        if (!NiObject::IsExactKindOf(&BSMultiTargetTreadTransfController::ms_RTTI, i) && !IsExactKindOf_0) {
            NiUpdateData apMemory(afTime, 0, 0);
            i->Update(apMemory);
            break;
        }
    }
    if (apAccumRoot) {
        NiUpdateData a2(afTime, 1, 0);
        apAccumRoot->UpdateObjectControllers(a2, 1);
    }
    bhkNiCollisionObject::SetSyncMode(bhkNiCollisionObject::SYNC_BOTH);
}

void bhkNiCollisionObject::SetSyncMode(bhkNiCollisionObject::SYNCMODE aeMode) {
    GetThreadTLSData()->eHavokSyncMode = aeMode;
}

void BSTransCon::UpdateAllButTarget(float afTime, bool abSkipSelectiveUpdateTest) {
    if (GetActive()) {
        if (m_usNumInterps) {
            NiNode* v12;
            int32_t i;
            NiQuatTransform ecx0;
            int32_t v10 = m_usNumInterps - 1;
            for (i = 0; i < v10; ++i) {
                v12 = static_cast<NiNode*>(m_ppkTargets[i]);
                if (v12) {
                    if ((abSkipSelectiveUpdateTest || v12->GetSelectiveUpdate()) &&
                        (!abSkipSelectiveUpdateTest || !v12->GetSelectiveUpdate()) &&
                        m_pkBlendInterps[i].Update(afTime, v12, ecx0)) {
                        if (ecx0.IsTranslateValid()) {
                            NiQuatTransform* X = &ecx0;
                            v12->SetTranslate(X->m_kTranslate);
                        }
                        if (ecx0.IsRotateValid()) {
                            NiQuaternion* Xa = &ecx0.m_kRotate;
                            v12->SetRotate_0(*Xa);
                        }
                        if (ecx0.IsScaleValid()) {
                            float Xb = ecx0.m_fScale;
                            v12->SetScale(Xb);
                        }
                    }
                }
            }
            v12 = static_cast<NiNode*>(m_ppkTargets[i]);
            if (v12) {
                if ((abSkipSelectiveUpdateTest || v12->GetSelectiveUpdate()) &&
                    m_pkBlendInterps[i].Update(afTime, v12, ecx0)) {
                    if (ecx0.IsTranslateValid()) {
                        NiQuatTransform* Xc = &ecx0;
                        v12->SetTranslate(Xc->m_kTranslate);
                    }
                    if (ecx0.IsRotateValid()) {
                        NiQuaternion* Xd = &ecx0.m_kRotate;
                        v12->SetRotate_0(*Xd);
                    }
                    if (ecx0.IsScaleValid()) {
                        float Xe = ecx0.m_fScale;
                        v12->SetScale(Xe);
                    }
                }
            }
        }
    }
}

void NiAVObject::UpdateObjectControllers(NiUpdateData& UpdateData, bool bProperties) {
    if (bProperties && static_cast<uint8_t>(QHasPropController())) {
        auto* pkPosition = m_kPropertyList.GetHeadPos();
        while (pkPosition) {
            NiPointer<NiProperty>& spProperty = m_kPropertyList.GetNext(pkPosition);
            NiProperty* pkProperty = spProperty.operator->();
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
