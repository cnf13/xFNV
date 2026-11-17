#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cfloat>
#include <cmath>
#include <cstring>

NiRTTI NiMultiTargetTransformController::ms_RTTI("NiMultiTargetTransformController", &NiInterpController::ms_RTTI);

NiMultiTargetTransformController::NiMultiTargetTransformController()
  : NiInterpController(), m_pkBlendInterps(nullptr), m_ppkTargets(nullptr), m_usNumInterps(0) {}

NiMultiTargetTransformController::NiMultiTargetTransformController(uint16_t usCount)
  : NiMultiTargetTransformController() {
    AllocateStorage(usCount);
}

NiMultiTargetTransformController::~NiMultiTargetTransformController() {
    delete[] m_pkBlendInterps;
    _NiFree(m_ppkTargets);
}

const NiRTTI* NiMultiTargetTransformController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiMultiTargetTransformController::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiMultiTargetTransformController));
    if (v3) {
        NiMultiTargetTransformController* v4 = ::new (v3) NiMultiTargetTransformController;
        CopyMembers(v4, kCloning);
        return v4;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The targets are taken as they resolve, without a type check.
void NiMultiTargetTransformController::LoadBinary(NiStream& kStream) {
    NiInterpController::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_usNumInterps);
    AllocateStorage(m_usNumInterps);
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        m_ppkTargets[i] = static_cast<NiAVObject*>(kStream.ResolveLinkID());
    }
}

void NiMultiTargetTransformController::LinkObject(NiStream& kStream) {
    NiInterpController::LinkObject(kStream);
}

bool NiMultiTargetTransformController::RegisterStreamables(NiStream& kStream) {
    return NiInterpController::RegisterStreamables(kStream);
}

void NiMultiTargetTransformController::SaveBinary(NiStream& kStream) {
    NiInterpController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_usNumInterps);
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        kStream.SaveLinkID(m_ppkTargets[i]);
    }
}

// The targets must be equal pairwise (both null, or apObject's equal to ours); the blend interpolators are not
// compared.
bool NiMultiTargetTransformController::IsEqual(NiObject* pkObject) {
    if (!NiInterpController::IsEqual(pkObject)) {
        return false;
    }

    NiMultiTargetTransformController* pkOther = static_cast<NiMultiTargetTransformController*>(pkObject);
    if (pkOther->m_usNumInterps != m_usNumInterps) {
        return false;
    }

    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        if ((m_ppkTargets[i] && !pkOther->m_ppkTargets[i]) || (!m_ppkTargets[i] && pkOther->m_ppkTargets[i])) {
            return false;
        }

        if (m_ppkTargets[i] && !pkOther->m_ppkTargets[i]->IsEqual(m_ppkTargets[i])) {
            return false;
        }
    }

    return true;
}

void NiMultiTargetTransformController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_usNumInterps", m_usNumInterps));
}

// The clone's targets become the clones of ours (null when a target was not cloned). The clone itself is looked up
// without checking that it was found.
void NiMultiTargetTransformController::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);
    NiObject* pkClone = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkClone);
    NiMultiTargetTransformController* pkDest = static_cast<NiMultiTargetTransformController*>(pkClone);
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        m_pkBlendInterps[i].ProcessClone(kCloning);
        if (m_ppkTargets[i]) {
            NiObject* pkTargetClone;
            bool bFound = kCloning.m_pkCloneMap->GetAt(m_ppkTargets[i], pkTargetClone);
            pkDest->m_ppkTargets[i] = bFound ? static_cast<NiAVObject*>(pkTargetClone) : nullptr;
        }
    }
}

// Each selectively updated target takes the valid parts of its blend interpolator's transform. The transform is not
// reset between targets.
void NiMultiTargetTransformController::Update(NiUpdateData& UpdateData) {
    if (!(m_usFlags & ACTIVE_MASK)) {
        return;
    }

    NiQuatTransform kTransform;
    kTransform.m_fScale = -FLT_MAX;
    kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        NiAVObject* pkTarget = m_ppkTargets[i];
        if (!pkTarget || !(pkTarget->m_uFlags & NiAVObject::SELECTIVE_UPDATE)) {
            continue;
        }

        if (!m_pkBlendInterps[i].Update(UpdateData.fTime, pkTarget, kTransform)) {
            continue;
        }

        if (kTransform.IsTranslateValid()) {
            m_ppkTargets[i]->m_kLocal.m_Translate = kTransform.m_kTranslate;
        }
        if (kTransform.IsRotateValid()) {
            kTransform.m_kRotate.ToRotation(m_ppkTargets[i]->m_kLocal.m_Rotate);
        }
        if (kTransform.IsScaleValid()) {
            m_ppkTargets[i]->m_kLocal.m_fScale = std::fabs(kTransform.m_fScale);
        }
    }
}

const char* NiMultiTargetTransformController::GetInterpolatorID(uint16_t usIndex) {
    return m_ppkTargets[usIndex]->m_kName;
}

// The index of the first target named apcID.
uint16_t NiMultiTargetTransformController::GetInterpolatorIndex(const char* pcID) const {
    if (!pcID) {
        return INVALID_INDEX;
    }

    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        if (m_ppkTargets[i] && strcmp(m_ppkTargets[i]->m_kName, pcID) == 0) {
            return i;
        }
    }

    return INVALID_INDEX;
}

// As GetInterpolatorIndex, comparing the fixed strings' handles.
uint16_t NiMultiTargetTransformController::GetInterpolatorIndexFx(const NiFixedString& arName) const {
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        if (m_ppkTargets[i] && m_ppkTargets[i]->m_kName == arName) {
            return i;
        }
    }

    return INVALID_INDEX;
}

NiInterpolator* NiMultiTargetTransformController::GetInterpolator(uint16_t usIndex) const {
    return &m_pkBlendInterps[usIndex];
}

// The extrema over every blend interpolator's active range; when none has one, the base's.
void NiMultiTargetTransformController::ResetTimeExtrema() {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        return;
    }

    m_fLoKeyTime = FLT_MAX;
    m_fHiKeyTime = -FLT_MAX;
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        float fHiKeyTime = -FLT_MAX;
        float fLoKeyTime = FLT_MAX;
        m_pkBlendInterps[i].GetActiveTimeRange(fLoKeyTime, fHiKeyTime);
        if (fLoKeyTime < m_fLoKeyTime) {
            m_fLoKeyTime = fLoKeyTime;
        }
        if (fHiKeyTime > m_fHiKeyTime) {
            m_fHiKeyTime = fHiKeyTime;
        }
    }

    if (m_fLoKeyTime == FLT_MAX && m_fHiKeyTime == -FLT_MAX) {
        NiInterpController::ResetTimeExtrema();
    }
}

// A transform interpolator posed at the target's current local transform.
NiInterpolator* NiMultiTargetTransformController::CreatePoseInterpolator(uint16_t usIndex) {
    NiAVObject* pkTarget = m_ppkTargets[usIndex];
    NiQuaternion kRotate;
    kRotate.FromRotation(pkTarget->m_kLocal.m_Rotate);
    NiQuatTransform kPoseValue;
    kPoseValue.m_fScale = pkTarget->m_kLocal.m_fScale;
    kPoseValue.m_kTranslate = pkTarget->m_kLocal.m_Translate;
    kPoseValue.m_kRotate = kRotate;
    NiTransformInterpolator* pkInterp =
        static_cast<NiTransformInterpolator*>(NiMemObject::operator new(sizeof(NiTransformInterpolator)));
    if (!pkInterp) {
        return nullptr;
    }
    return ::new (pkInterp) NiTransformInterpolator(kPoseValue);
}

// The target's blend interpolator; no new one is built.
NiBlendInterpolator*
NiMultiTargetTransformController::CreateBlendInterpolator(uint16_t usIndex, int32_t, bool, float, int32_t) const {
    return &m_pkBlendInterps[usIndex];
}

void NiMultiTargetTransformController::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        m_pkBlendInterps[i].GuaranteeTimeRange(fStartTime, fEndTime);
    }
}

bool NiMultiTargetTransformController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

bool NiMultiTargetTransformController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsTransformValueSupported();
}

void NiMultiTargetTransformController::SetInterpolatorTarget(uint16_t usIndex, NiAVObject* pkTarget) {
    m_ppkTargets[usIndex] = pkTarget;
    if (pkTarget) {
        pkTarget->NiAVObject::SetSelectiveUpdateTransformsOverride(true);
    }
}

void NiMultiTargetTransformController::RemoveNodeRecurse(NiAVObject* apNode) {
    if (apNode) {
        uint16_t InterpolatorTargetIndex = GetInterpolatorTargetIndex(apNode);
        if (InterpolatorTargetIndex != NiInterpController::INVALID_INDEX) {
            SetInterpolatorTarget(InterpolatorTargetIndex, nullptr);
        }
    }
    NiNode* v6 = apNode->IsNode();
    if (v6) {
        for (uint32_t a1 = 0; a1 < v6->GetArrayCount(); ++a1) {
            NiAVObject* v2 = v6->GetAt(a1);
            RemoveNodeRecurse(v2);
        }
    }
}

NiBlendTransformInterpolator* NiMultiTargetTransformController::GetBlendInterpolator(uint16_t usIndex) {
    return &m_pkBlendInterps[usIndex];
}

NiAVObject* NiMultiTargetTransformController::GetInterpolatorTarget(uint16_t usIndex) {
    return m_ppkTargets[usIndex];
}

uint16_t NiMultiTargetTransformController::GetInterpolatorTargetIndex(NiAVObject* pkObj) {
    uint16_t i;
    for (i = 0; ((int32_t)i) < ((int32_t)m_usNumInterps); ++i) {
        if (m_ppkTargets[i] == pkObj) {
            return i;
        }
    }
    return (uint16_t)(-1);
}

// Deletes m_pkBlendInterps and allocates a new array of m_usNumInterps NiBlendTransformInterpolator, holding a
// reference on each new blend interpolator; then the targets are cleared. The count is read again after the delete.
void NiMultiTargetTransformController::ReallocateStorage() {
    if (!m_usNumInterps) {
        return;
    }

    NiBlendTransformInterpolator* pkOldInterps = m_pkBlendInterps;
    if (!pkOldInterps) {
        return;
    }

    // The array cookie (the element count) is the raw word before the first entry.
    if (reinterpret_cast<size_t*>(pkOldInterps)[-1]) {
        delete[] pkOldInterps;
    }
    else {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldInterps) - 1);
    }

    uint32_t uiCount = m_usNumInterps;
    // A real new[], as in AllocateStorage.
    NiBlendTransformInterpolator* pkBlendInterps = new NiBlendTransformInterpolator[uiCount];
    m_pkBlendInterps = pkBlendInterps;
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        InterlockedIncrementUInt32(&m_pkBlendInterps[i].m_uiRefCount);
    }
    std::memset(m_ppkTargets, 0, sizeof(NiAVObject*) * m_usNumInterps);
}

// Does what `NiNew NiBlendTransformInterpolator[usCount]` does: the element count is stored in the word before the
// first entry, then the constructor runs on each entry.
void NiMultiTargetTransformController::AllocateStorage(uint16_t usCount) {
    m_usNumInterps = usCount;
    if (!usCount) {
        return;
    }

    uint32_t uiCount = usCount;
    // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and the
    // destructor's delete[] goes through it.
    NiBlendTransformInterpolator* pkBlendInterps = new NiBlendTransformInterpolator[uiCount];
    m_pkBlendInterps = pkBlendInterps;
    for (uint16_t i = 0; i < usCount; i++) {
        InterlockedIncrementUInt32(&m_pkBlendInterps[i].m_uiRefCount);
    }
    size_t stTargetsSize = sizeof(NiAVObject*) * uiCount;
    m_ppkTargets = static_cast<NiAVObject**>(_NiMalloc(stTargetsSize));
    std::memset(m_ppkTargets, 0, stTargetsSize);
}

void NiMultiTargetTransformController::CopyMembers(
    NiMultiTargetTransformController* pkDest,
    NiCloningProcess& kCloning
) {
    NiInterpController::CopyMembers(pkDest, kCloning);
    pkDest->AllocateStorage(m_usNumInterps);
    for (uint16_t i = 0; i < m_usNumInterps; i++) {
        m_pkBlendInterps[i].CopyMembers(&pkDest->m_pkBlendInterps[i], kCloning);
    }
}

NiObject* NiMultiTargetTransformController::CreateObject() {
    void* pkObject = NiMemObject::operator new(sizeof(NiMultiTargetTransformController));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiMultiTargetTransformController;
}
