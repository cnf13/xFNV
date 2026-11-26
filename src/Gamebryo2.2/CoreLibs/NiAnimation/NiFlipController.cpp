#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFlipController.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <limits>

NiRTTI NiFlipController::ms_RTTI("NiFlipController", &NiFloatInterpController::ms_RTTI);

NiFlipController::NiFlipController() : m_kTextures(1, 1), m_uiIndex(0), m_uiAffectedMap(0), m_pcCtlrID(nullptr) {}

NiFlipController::~NiFlipController() {
    _NiFree(m_pcCtlrID);
    m_pcCtlrID = nullptr;
}

const NiRTTI* NiFlipController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiFlipController::CreateClone(NiCloningProcess& kCloning) {
    NiFlipController* pkObject = new NiFlipController;
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

// Files before 10.1.0.104 store a start time and the seconds per frame in place of an interpolator: the controller
// builds one step key per texture plus a closing key from them, into a new float interpolator. A failed key allocation
// leaves the keys written through a null pointer.
void NiFlipController::LoadBinary(NiStream& kStream) {
    NiFloatInterpController::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uiAffectedMap);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadMultipleLinkIDs();
        return;
    }

    float fStartTime;
    float fSecsPerFrame;
    NiStreamLoadBinary(kStream, fStartTime);
    NiStreamLoadBinary(kStream, fSecsPerFrame);
    uint32_t uiNumTextures = kStream.ReadMultipleLinkIDs();

    NiFloatData* pkFloatData = new NiFloatData;

    if (uiNumTextures > 0) {
        uint32_t uiNumKeys = uiNumTextures + 1;
        NiStepFloatKey* pkFloatKeys = NiAnimationKey::AllocateKeyArray<NiStepFloatKey>(uiNumKeys);

        // Each index is converted as signed, and 2^32 is added when it is negative.
        float fAccumTime = fStartTime;
        for (uint32_t ui = 0; ui < uiNumTextures; ui++) {
            pkFloatKeys[ui].SetTime(fAccumTime);
            int32_t iValue = static_cast<int32_t>(ui);
            double dValue = iValue;
            if (iValue < 0) {
                dValue += 4294967296.0f;
            }
            pkFloatKeys[ui].SetValue(static_cast<float>(dValue));
            fAccumTime += fSecsPerFrame;
        }

        pkFloatKeys[uiNumTextures].SetTime(fAccumTime);
        int32_t iLast = static_cast<int32_t>(uiNumTextures - 1);
        double dLast = iLast;
        if (iLast < 0) {
            dLast += 4294967296.0f;
        }
        pkFloatKeys[uiNumTextures].SetValue(static_cast<float>(dLast));
        pkFloatData->ReplaceAnim(pkFloatKeys, uiNumKeys, NiAnimationKey::STEPKEY);
        m_fLoKeyTime = fStartTime;
        m_fHiKeyTime = fAccumTime;
    }

    NiFloatInterpolator* pkInterp = new NiFloatInterpolator(pkFloatData);
    m_spInterpolator = pkInterp;
    m_spInterpolator->Collapse();
}

// The texture links are only resolved when the controller has a target.
void NiFlipController::LinkObject(NiStream& kStream) {
    NiFloatInterpController::LinkObject(kStream);
    if (!m_pkTarget) {
        return;
    }

    uint32_t uiCount = kStream.GetNumberOfLinkIDs();
    m_kTextures.SetSize(uiCount);
    for (uint32_t i = 0; i < uiCount; i++) {
        NiPointer<NiTexture> spTexture = static_cast<NiTexture*>(kStream.GetObjectFromLinkID());
        m_kTextures.SetAt(i, spTexture);
    }
}

bool NiFlipController::RegisterStreamables(NiStream& kStream) {
    if (!NiFloatInterpController::RegisterStreamables(kStream)) {
        return false;
    }

    uint32_t uiCount = m_kTextures.GetSize();
    for (uint32_t i = 0; i < uiCount; i++) {
        NiTexture* pkTexture = m_kTextures.m_pBase[i];
        if (pkTexture) {
            pkTexture->RegisterStreamables(kStream);
        }
    }
    return true;
}

void NiFlipController::SaveBinary(NiStream& kStream) {
    NiFloatInterpController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiAffectedMap);
    uint32_t uiCount = m_kTextures.GetSize();
    NiStreamSaveBinary(kStream, uiCount);
    for (uint32_t i = 0; i < uiCount; i++) {
        kStream.SaveLinkID(m_kTextures.m_pBase[i]);
    }
}

// The texture lists are not compared.
bool NiFlipController::IsEqual(NiObject* pkObject) {
    if (!NiFloatInterpController::IsEqual(pkObject)) {
        return false;
    }

    return m_uiAffectedMap == static_cast<NiFlipController*>(pkObject)->m_uiAffectedMap;
}

// The animated value picks the texture: it is rounded down after adding 0.01 (in double) and clamped to the last
// texture. A missing map slot leaves the target unchanged.
void NiFlipController::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -std::numeric_limits<float>::max();
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

    uint32_t uiSize = m_kTextures.GetSize();
    m_uiIndex = static_cast<uint32_t>(static_cast<int64_t>(fValue + static_cast<double>(0.01f)));
    if (m_uiIndex >= uiSize) {
        m_uiIndex = uiSize - 1;
    }

    NiTexturingProperty* pkProperty = static_cast<NiTexturingProperty*>(m_pkTarget);
    if (!pkProperty) {
        return;
    }

    if (m_uiAffectedMap < pkProperty->m_kMaps.GetSize() && pkProperty->m_kMaps.m_pBase[m_uiAffectedMap]) {
        pkProperty->m_kMaps.m_pBase[m_uiAffectedMap]->SetTexture(m_kTextures.m_pBase[m_uiIndex].m_pObject);
        return;
    }

    // Affected-map numbers from SHADER_MAP_OFFSET on address the texturing property's shader maps.
    constexpr uint32_t SHADER_MAP_OFFSET = 1024;
    uint32_t uiShaderMap = m_uiAffectedMap - SHADER_MAP_OFFSET;
    if (pkProperty->m_pkShaderMaps &&
        uiShaderMap < pkProperty->m_pkShaderMaps->GetSize() &&
        pkProperty->GetShaderMap(uiShaderMap)) {
        pkProperty->GetShaderMap(uiShaderMap)->SetTexture(m_kTextures.m_pBase[m_uiIndex].m_pObject);
    }
}

// The ID is the affected map number, made on first use.
const char* NiFlipController::GetCtlrID() {
    if (!m_pcCtlrID) {
        m_pcCtlrID = static_cast<char*>(_NiMalloc(15));
        NiSprintf(m_pcCtlrID, 15, "%d", m_uiAffectedMap);
    }
    return m_pcCtlrID;
}

// The flag is set through the result even when the allocation failed.
NiBlendInterpolator* NiFlipController::CreateBlendInterpolator(
    uint16_t usIndex,
    int32_t bManagerControlled,
    bool bAccumulateAnimations,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    NiBlendInterpolator* pkInterp = new NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
    pkInterp->m_uFlags |= NiBlendInterpolator::ONLY_USE_HIGHEST_WEIGHT_MASK;
    return pkInterp;
}

bool NiFlipController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&NiTexturingProperty::ms_RTTI, pkTarget);
}

void NiFlipController::GetTargetFloatValue(float& fValue) {
    fValue = static_cast<float>(m_uiIndex);
}

NiTexture* NiFlipController::GetTextureAt(uint32_t uiIndex) {
    if (uiIndex >= m_kTextures.m_usSize) {
        return nullptr;
    }

    return m_kTextures.m_pBase[uiIndex].m_pObject;
}

void NiFlipController::CopyMembers(NiFlipController* pkDest, NiCloningProcess& kCloning) {
    NiFloatInterpController::CopyMembers(pkDest, kCloning);
    pkDest->m_uiAffectedMap = m_uiAffectedMap;
    for (uint32_t uiI = 0; uiI < m_kTextures.m_usSize; ++uiI) {
        pkDest->AttachTexture(m_kTextures.m_pBase[uiI].m_pObject, uiI);
    }
}

// Grows the list to hold uiIndex, then stores the texture through a smart pointer.
void NiFlipController::AttachTexture(NiTexture* pkTexture, uint32_t uiIndex) {
    if (uiIndex >= m_kTextures.GetSize()) {
        m_kTextures.SetSize(uiIndex + 1);
    }
    NiPointer<NiTexture> spTexture = pkTexture;
    m_kTextures.SetAtGrow(uiIndex, spTexture);
}

NiObject* NiFlipController::CreateObject() {
    return new NiFlipController;
}
