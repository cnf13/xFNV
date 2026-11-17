#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiFloatController::ms_RTTI("NiFloatController", &NiTimeController::ms_RTTI);

NiFloatController::~NiFloatController() {
    m_spFloatData = nullptr;
}

void NiFloatController::LoadBinary(NiStream& stream) {
    NiTimeController::LoadBinary(stream);
    stream.ReadLinkID();
}

void NiFloatController::LinkObject(NiStream& stream) {
    NiTimeController::LinkObject(stream);
    m_spFloatData = static_cast<NiFloatData*>(stream.GetObjectFromLinkID());
}

// The float data is taken to be set.
bool NiFloatController::RegisterStreamables(NiStream& stream) {
    if (!NiTimeController::RegisterStreamables(stream)) {
        return false;
    }

    m_spFloatData->RegisterStreamables(stream);
    return true;
}

void NiFloatController::SaveBinary(NiStream& stream) {
    NiTimeController::SaveBinary(stream);
    stream.SaveLinkID(m_spFloatData);
}

// The float data is taken to be set on both.
bool NiFloatController::IsEqual(NiObject* pObject) {
    if (!NiTimeController::IsEqual(pObject)) {
        return false;
    }

    return m_spFloatData->IsEqual(static_cast<NiFloatController*>(pObject)->m_spFloatData);
}

void NiFloatController::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiTimeController::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(NiFloatController::ms_RTTI.GetName()));
}

void NiFloatController::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);
    if (m_spFloatData) {
        m_spFloatData->ProcessClone(kCloning);
    }
}

void NiFloatController::CopyMembers(NiFloatController* pDest, NiCloningProcess& kCloning) {
    NiTimeController::CopyMembers(pDest, kCloning);

    if (m_spFloatData) {
        pDest->m_spFloatData = static_cast<NiFloatData*>(m_spFloatData->CreateClone(kCloning));
    }
}

NiFloatKey* NiFloatController::GetAnim(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
    if (!m_spFloatData) {
        uiNumKeys = 0;
        eType = NiFloatKey::NOINTERP;
        ucSize = 0;
        return nullptr;
    }

    uiNumKeys = m_spFloatData->m_uiNumKeys;
    eType = m_spFloatData->m_eType;
    ucSize = m_spFloatData->m_ucKeySize;
    return m_spFloatData->m_pkKeys;
}
