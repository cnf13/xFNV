#include "Fallout/AI/BSPlayerDistanceCheckController.h"

#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

NiRTTI BSPlayerDistanceCheckController::ms_RTTI("BSPlayerDistanceCheckController", &NiTimeController::ms_RTTI);

BSPlayerDistanceCheckController::BSPlayerDistanceCheckController() : fCheckDistance(0.0f), pfnCallback(nullptr) {}

BSPlayerDistanceCheckController::~BSPlayerDistanceCheckController() {}

const NiRTTI* BSPlayerDistanceCheckController::GetRTTI() const {
    return &BSPlayerDistanceCheckController::ms_RTTI;
}

NiObject* BSPlayerDistanceCheckController::CreateClone(NiCloningProcess& kCloning) {
    BSPlayerDistanceCheckController* pkClone = new BSPlayerDistanceCheckController;
    CopyMembers(pkClone, kCloning);
    return pkClone;
}

void BSPlayerDistanceCheckController::Update(NiUpdateData& UpdateData) {
    if (!(GetCheckDistance() < 0.0) && GetCallback() && pPlayer) {
        NiAVObject* pTarget = static_cast<NiAVObject*>(NiObject::DynamicCastStatic(&NiAVObject::ms_RTTI, m_pkTarget));
        if (pTarget) {
            const NiPoint3& kTargetPos = pTarget->GetWorldTranslate();
            const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
            NiPoint3 kDiff = kPlayerPos - kTargetPos;
            double dPlayerDistance = kDiff.Length();
            if (GetCheckDistance() > dPlayerDistance) {
                pfnCallback(pTarget);
            }
        }
    }
}

// Whether the target is a scene graph object.
bool BSPlayerDistanceCheckController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

float BSPlayerDistanceCheckController::GetCheckDistance() {
    return fCheckDistance;
}

void BSPlayerDistanceCheckController::SetCheckDistance(float afDist) {
    fCheckDistance = afDist;
}

// Copies the callback and the distance, then the base controller's members.
void BSPlayerDistanceCheckController::CopyMembers(BSPlayerDistanceCheckController* pkDest, NiCloningProcess& kCloning) {
    pkDest->SetCallback(GetCallback());
    pkDest->SetCheckDistance(GetCheckDistance());
    NiTimeController::CopyMembers(pkDest, kCloning);
}
