#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatsExtraDataController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cfloat>
#include <cstring>

NiRTTI NiFloatsExtraDataController::ms_RTTI("NiFloatsExtraDataController", &NiExtraDataController::ms_RTTI);

NiFloatsExtraDataController::NiFloatsExtraDataController() : m_iFloatsExtraDataIndex(-1) {}

NiFloatsExtraDataController::~NiFloatsExtraDataController() {}

const NiRTTI* NiFloatsExtraDataController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiFloatsExtraDataController::CreateClone(NiCloningProcess& kCloning) {
    NiFloatsExtraDataController* pkObject_2 = new NiFloatsExtraDataController;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_iFloatsExtraDataIndex = m_iFloatsExtraDataIndex;
    return pkObject_2;
}

// Files before 10.1.0.104 also store the link to an old-style float data, which LinkObject resolves.
void NiFloatsExtraDataController::LoadBinary(NiStream& kStream) {
    NiExtraDataController::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_iFloatsExtraDataIndex);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

// Files older than 10.1.0.104 carry a link to old-style float data; it becomes a float interpolator, collapsed.
void NiFloatsExtraDataController::LinkObject(NiStream& kStream) {
    NiExtraDataController::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiFloatData* pkData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
        NiFloatInterpolator* pkInterp = new NiFloatInterpolator(pkData);
        m_spInterpolator = pkInterp;
        m_spInterpolator->Collapse();
    }
}

bool NiFloatsExtraDataController::RegisterStreamables(NiStream& kStream) {
    return NiExtraDataController::RegisterStreamables(kStream);
}

void NiFloatsExtraDataController::SaveBinary(NiStream& kStream) {
    NiExtraDataController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_iFloatsExtraDataIndex);
}

bool NiFloatsExtraDataController::IsEqual(NiObject* pkObject) {
    if (!NiExtraDataController::IsEqual(pkObject)) {
        return false;
    }

    return m_iFloatsExtraDataIndex == static_cast<NiFloatsExtraDataController*>(pkObject)->m_iFloatsExtraDataIndex;
}

void NiFloatsExtraDataController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiExtraDataController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_iFloatsExtraDataIndex", m_iFloatsExtraDataIndex));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX).
void NiFloatsExtraDataController::Update(NiUpdateData& UpdateData) {
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
    if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue) && m_spExtraData) {
        static_cast<NiFloatsExtraData*>(static_cast<NiExtraData*>(m_spExtraData))
            ->SetValue(m_iFloatsExtraDataIndex, fValue);
    }
}

const char* NiFloatsExtraDataController::GetCtlrID() {
    if (m_kCtlrID.GetHandle() == NiGlobalStringTable::NULL_STRING) {
        m_kCtlrID = static_cast<const char*>(nullptr);
        const char* pcName = m_kExtraDataName;
        if (pcName) {
            size_t stLen = strlen(pcName) + 15;
            char* pcID = static_cast<char*>(_NiMalloc(stLen));
            NiSprintf(pcID, stLen, "%s[%d]", pcName, m_iFloatsExtraDataIndex);
            m_kCtlrID = static_cast<const char*>(pcID);
            _NiFree(pcID);
        }
    }
    return m_kCtlrID;
}

NiInterpolator* NiFloatsExtraDataController::CreatePoseInterpolator(uint16_t usIndex) {
    return new NiFloatInterpolator(
        static_cast<NiFloatsExtraData*>(m_spExtraData.m_pObject)->GetValue(m_iFloatsExtraDataIndex)
    );
}

// apInterp must be a NiFloatInterpolator; the extra data is not null-checked.
void NiFloatsExtraDataController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
    NiFloatInterpolator* pkInterp = apInterp
        ? static_cast<NiFloatInterpolator*>(apInterp->DynamicCastMember(&NiFloatInterpolator::ms_RTTI))
        : nullptr;
    NiFloatsExtraData* pkData = static_cast<NiFloatsExtraData*>(static_cast<NiExtraData*>(m_spExtraData));
    pkInterp->SetPoseValue(pkData->GetValue(m_iFloatsExtraDataIndex));
}

// A new blend interpolator for the animated float, which the controller then feeds its sources. bManagerControlled and
// ucArraySize are passed on to the constructor as the caller's whole int32_t arguments.
NiBlendInterpolator* NiFloatsExtraDataController::CreateBlendInterpolator(
    uint16_t,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    return new NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiFloatsExtraDataController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsFloatValueSupported();
}

NiObject* NiFloatsExtraDataController::CreateObject() {
    return new NiFloatsExtraDataController;
}
