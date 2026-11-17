#include "Gamebryo2.2/CoreLibs/NiAnimation/BSFrustumFOVController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>
#include <cmath>

NiRTTI BSFrustumFOVController::ms_RTTI("BSFrustumFOVController", &NiFloatInterpController::ms_RTTI);

BSFrustumFOVController::BSFrustumFOVController() {}

BSFrustumFOVController::~BSFrustumFOVController() {}

const NiRTTI* BSFrustumFOVController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSFrustumFOVController::CreateClone(NiCloningProcess& kCloning) {
    BSFrustumFOVController* v3 =
        static_cast<BSFrustumFOVController*>(NiMemObject::operator new(sizeof(BSFrustumFOVController)));
    if (v3) {
        ::new (v3) BSFrustumFOVController;
        CopyMembers(v3, kCloning);
        return v3;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

void BSFrustumFOVController::LoadBinary(NiStream& kStream) {
    NiFloatInterpController::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

// Files older than 10.1.0.104 keep the float data on the controller: it is wrapped in a new float interpolator,
// which is collapsed.
void BSFrustumFOVController::LinkObject(NiStream& kStream) {
    NiFloatInterpController::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiFloatData* pkData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
        NiFloatInterpolator* pkInterpolator =
            static_cast<NiFloatInterpolator*>(NiMemObject::operator new(sizeof(NiFloatInterpolator)));
        if (pkInterpolator) {
            pkInterpolator = ::new (pkInterpolator) NiFloatInterpolator(pkData);
        }
        m_spInterpolator = pkInterpolator;
        m_spInterpolator.m_pObject->Collapse();
    }
}

bool BSFrustumFOVController::RegisterStreamables(NiStream& kStream) {
    return NiFloatInterpController::RegisterStreamables(kStream);
}

void BSFrustumFOVController::SaveBinary(NiStream& kStream) {
    NiFloatInterpController::SaveBinary(kStream);
}

bool BSFrustumFOVController::IsEqual(NiObject* pkObject) {
    return NiFloatInterpController::IsEqual(pkObject);
}

void BSFrustumFOVController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiFloatInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The animated value is the horizontal frustum half-extent; the vertical one follows at a 4:3 aspect. The target
// camera is not null-checked.
void BSFrustumFOVController::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -FLT_MAX;
    }
    else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate())) {
        return;
    }

    if (!m_spInterpolator) {
        return;
    }

    float fValue;
    if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue)) {
        return;
    }

    NiCamera* pkCamera = static_cast<NiCamera*>(m_pkTarget);
    NiFrustum kFrustum = pkCamera->m_kViewFrustum;
    kFrustum.m_fLeft = -fValue;
    kFrustum.m_fRight = fValue;
    kFrustum.m_fTop = static_cast<float>(fValue * 0.75);
    kFrustum.m_fBottom = static_cast<float>(fValue * -0.75);
    pkCamera->SetViewFrustum(kFrustum);
}

bool BSFrustumFOVController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&NiCamera::ms_RTTI, pkTarget);
}

// The full angle: twice the arctangent of the camera frustum's right edge, rounded to float before the doubling.
void BSFrustumFOVController::GetTargetFloatValue(float& fValue) {
    float fHalfAngle = std::atan(static_cast<NiCamera*>(m_pkTarget)->m_kViewFrustum.m_fRight);
    fValue = fHalfAngle + fHalfAngle;
}

NiObject* BSFrustumFOVController::CreateObject() {
    BSFrustumFOVController* v0 =
        static_cast<BSFrustumFOVController*>(NiMemObject::operator new(sizeof(BSFrustumFOVController)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) BSFrustumFOVController;
}
