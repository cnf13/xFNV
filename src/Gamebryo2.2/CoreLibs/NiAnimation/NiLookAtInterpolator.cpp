#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLookAtInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <limits>

NiRTTI NiLookAtInterpolator::ms_RTTI("NiLookAtInterpolator", &NiInterpolator::ms_RTTI);

NiLookAtInterpolator::NiLookAtInterpolator(NiAVObject* pkLookAt, NiLookAtInterpolator::Axis eAxis, bool bFlip)
  : m_uFlags(0), m_pkLookAt(pkLookAt) {
    m_kTransformValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kTransformValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kTransformValue.m_fScale = -std::numeric_limits<float>::max();
    if (bFlip) {
        m_uFlags |= FLIP_MASK;
    }
    else {
        m_uFlags &= ~FLIP_MASK;
    }
    m_uFlags = static_cast<uint16_t>((m_uFlags & ~AXIS_MASK) | (eAxis << AXIS_POS));
}

NiLookAtInterpolator::~NiLookAtInterpolator() {}

NiObject* NiLookAtInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiLookAtInterpolator* pkObject = new NiLookAtInterpolator(nullptr, NiLookAtInterpolator::X, false);
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

// The look-at target is kept as a raw pointer: it is resolved again by name through ResolveDependencies.
void NiLookAtInterpolator::LoadBinary(NiStream& kStream) {
    NiInterpolator::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uFlags);
    m_pkLookAt = static_cast<NiAVObject*>(kStream.ResolveLinkID());
    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
        kStream.LoadCStringAsFixedString(m_kLookAtName);
    }
    else {
        kStream.LoadFixedString(m_kLookAtName);
    }
    m_kTransformValue.LoadBinary(kStream);
    for (NiPointer<NiInterpolator>& spInterpolator : m_aspInterpolators) {
        spInterpolator = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
    }
}

void NiLookAtInterpolator::LinkObject(NiStream& kStream) {
    NiInterpolator::LinkObject(kStream);
}

bool NiLookAtInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiInterpolator::RegisterStreamables(kStream)) {
        return false;
    }

    for (NiInterpolator* pkInterpolator : m_aspInterpolators) {
        if (pkInterpolator && !pkInterpolator->RegisterStreamables(kStream)) {
            return false;
        }
    }

    kStream.RegisterFixedString(m_kLookAtName);
    return true;
}

void NiLookAtInterpolator::SaveBinary(NiStream& kStream) {
    NiInterpolator::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uFlags);
    kStream.SaveLinkID(m_pkLookAt);
    kStream.SaveFixedString(m_kLookAtName);
    m_kTransformValue.SaveBinary(kStream);
    for (NiInterpolator* pkInterpolator : m_aspInterpolators) {
        kStream.SaveLinkID(pkInterpolator);
    }
}

// The look-at name is not compared.
bool NiLookAtInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiLookAtInterpolator* pkOther = static_cast<NiLookAtInterpolator*>(pkObject);
    if (m_uFlags != pkOther->m_uFlags) {
        return false;
    }
    if ((m_pkLookAt && !pkOther->m_pkLookAt) || (!m_pkLookAt && pkOther->m_pkLookAt)) {
        return false;
    }
    if (m_pkLookAt && pkOther->m_pkLookAt && !m_pkLookAt->IsEqual(pkOther->m_pkLookAt)) {
        return false;
    }
    if (!(m_kTransformValue == pkOther->m_kTransformValue)) {
        return false;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            if (!pkOther->m_aspInterpolators[i] || !m_aspInterpolators[i]->IsEqual(pkOther->m_aspInterpolators[i])) {
                return false;
            }
        }
        else if (pkOther->m_aspInterpolators[i]) {
            return false;
        }
    }

    return true;
}

void NiLookAtInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiInterpolator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_pkLookAt", static_cast<const void*>(m_pkLookAt)));
    m_kTransformValue.GetViewerStrings(pkStrings);
    for (NiInterpolator* pkInterpolator : m_aspInterpolators) {
        if (pkInterpolator) {
            pkInterpolator->GetViewerStrings(pkStrings);
        }
    }
}

// Points the clone at the clone of the look-at target when the target was cloned too, else at the same target.
void NiLookAtInterpolator::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);

    NiObject* pkClone = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkClone);
    NiLookAtInterpolator* pkDest = static_cast<NiLookAtInterpolator*>(pkClone);

    NiObject* pkClonedLookAt;
    if (kCloning.m_pkCloneMap->GetAt(m_pkLookAt, pkClonedLookAt)) {
        pkDest->m_pkLookAt = static_cast<NiAVObject*>(pkClonedLookAt);
    }
    else {
        pkDest->m_pkLookAt = m_pkLookAt;
    }
}

// Translation and scale come from the first and third sub-interpolators (else the target's own translation). The
// rotation turns the chosen axis toward the look-at target in the parent's frame, then rolls about Z by the second
// sub-interpolator. A target almost at the look-at point gets the identity rotation and no roll. A failing
// sub-interpolator invalidates the cached transform but leaves akValue untouched.
// KEEP: the fourth axis value (3, from the flags' two axis bits) leaves kRot unwritten, and TransposeTimes then
// reads it: an uninitialised read on that path.
bool NiLookAtInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    if (!m_pkLookAt) {
        kValue.m_kTranslate.x = -std::numeric_limits<float>::max();
        kValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
        kValue.m_fScale = -std::numeric_limits<float>::max();
        m_kTransformValue.m_kTranslate.x = -std::numeric_limits<float>::max();
        m_kTransformValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
        m_kTransformValue.m_fScale = -std::numeric_limits<float>::max();
        return false;
    }

    NiPoint3 kLookAtPos = m_pkLookAt->m_kWorld.m_Translate;
    NiAVObject* pkTarget = static_cast<NiAVObject*>(pkInterpTarget);
    NiNode* pkParent = pkTarget->m_pkParent;

    NiPoint3 kTranslate;
    if (m_aspInterpolators[0]) {
        if (!m_aspInterpolators[0]->Update(fTime, pkInterpTarget, kTranslate)) {
            m_kTransformValue.m_kTranslate.x = -std::numeric_limits<float>::max();
            m_kTransformValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
            m_kTransformValue.m_fScale = -std::numeric_limits<float>::max();
            return false;
        }
        m_kTransformValue.m_kTranslate = kTranslate;
    }
    else {
        kTranslate = pkTarget->m_kLocal.m_Translate;
    }

    NiMatrix3 kParentRot;
    NiPoint3 kWorldTranslate;
    if (pkParent) {
        kParentRot = pkParent->m_kWorld.m_Rotate;
        float fParentScale = pkParent->m_kWorld.m_fScale;
        NiPoint3 kRotated = kParentRot * kTranslate;
        NiPoint3 kScaled = kRotated * fParentScale;
        kWorldTranslate = pkParent->m_kWorld.m_Translate + kScaled;
    }
    else {
        kParentRot = NiMatrix3::IDENTITY;
        kWorldTranslate = kTranslate;
    }

    NiPoint3 kAt = kLookAtPos - kWorldTranslate;
    NiMatrix3 kRot;
    float fSqrLength = kAt.SqrLength();
    if (fSqrLength < 0.001f) {
        kRot.m_pEntry[0][0] = 1.0f;
        kRot.m_pEntry[0][1] = 0.0f;
        kRot.m_pEntry[0][2] = 0.0f;
        kRot.m_pEntry[1][0] = 0.0f;
        kRot.m_pEntry[1][1] = 1.0f;
        kRot.m_pEntry[1][2] = 0.0f;
        kRot.m_pEntry[2][0] = 0.0f;
        kRot.m_pEntry[2][1] = 0.0f;
        kRot.m_pEntry[2][2] = 1.0f;
    }
    else {
        kAt.Unitize();

        // World Z is up unless the look direction is nearly along it; then world Y is.
        NiPoint3 kUp;
        float fDot;
        if ((kAt.z < 1.001f && kAt.z > 0.999f) || (kAt.z > -1.001f && kAt.z < -0.999f)) {
            kUp = NiPoint3::UNIT_Y;
            fDot = kAt.y;
        }
        else {
            kUp = NiPoint3::UNIT_Z;
            fDot = kAt.z;
        }
        NiPoint3 kAlong = kAt * fDot;
        kUp = kUp - kAlong;
        kUp.Unitize();

        if (!(m_uFlags & FLIP_MASK)) {
            kAt = -kAt;
        }

        NiPoint3 kRight = kUp.Cross(kAt);
        // The fourth axis value leaves kRot unwritten, deliberately.
        switch ((m_uFlags & AXIS_MASK) >> AXIS_POS) {
            case X:
                kRot.m_pEntry[0][0] = kAt.x;
                kRot.m_pEntry[1][0] = kAt.y;
                kRot.m_pEntry[2][0] = kAt.z;
                kRot.m_pEntry[0][1] = kUp.x;
                kRot.m_pEntry[1][1] = kUp.y;
                kRot.m_pEntry[2][1] = kUp.z;
                kRot.m_pEntry[0][2] = -kRight.x;
                kRot.m_pEntry[1][2] = -kRight.y;
                kRot.m_pEntry[2][2] = -kRight.z;
                break;
            case Y:
                kRot.m_pEntry[0][0] = kRight.x;
                kRot.m_pEntry[1][0] = kRight.y;
                kRot.m_pEntry[2][0] = kRight.z;
                kRot.m_pEntry[0][1] = kAt.x;
                kRot.m_pEntry[1][1] = kAt.y;
                kRot.m_pEntry[2][1] = kAt.z;
                kRot.m_pEntry[0][2] = -kUp.x;
                kRot.m_pEntry[1][2] = -kUp.y;
                kRot.m_pEntry[2][2] = -kUp.z;
                break;
            case Z:
                kRot.m_pEntry[0][0] = kRight.x;
                kRot.m_pEntry[1][0] = kRight.y;
                kRot.m_pEntry[2][0] = kRight.z;
                kRot.m_pEntry[0][1] = kUp.x;
                kRot.m_pEntry[1][1] = kUp.y;
                kRot.m_pEntry[2][1] = kUp.z;
                kRot.m_pEntry[0][2] = kAt.x;
                kRot.m_pEntry[1][2] = kAt.y;
                kRot.m_pEntry[2][2] = kAt.z;
                break;
        }

        kRot = kParentRot.TransposeTimes(kRot);

        if (m_aspInterpolators[1]) {
            float fRoll;
            if (!m_aspInterpolators[1]->Update(fTime, pkInterpTarget, fRoll)) {
                m_kTransformValue.m_kTranslate.x = -std::numeric_limits<float>::max();
                m_kTransformValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
                m_kTransformValue.m_fScale = -std::numeric_limits<float>::max();
                return false;
            }
            NiMatrix3 kRoll;
            kRoll.MakeZRotation(-fRoll);
            kRot = kRot * kRoll;
        }
    }

    NiQuaternion kRotate;
    kRotate.FromRotation(kRot);
    m_kTransformValue.m_kRotate = kRotate;

    if (m_aspInterpolators[2]) {
        float fScale;
        if (!m_aspInterpolators[2]->Update(fTime, pkInterpTarget, fScale)) {
            m_kTransformValue.m_kTranslate.x = -std::numeric_limits<float>::max();
            m_kTransformValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
            m_kTransformValue.m_fScale = -std::numeric_limits<float>::max();
            return false;
        }
        m_kTransformValue.m_fScale = fScale;
    }

    if (m_kTransformValue.IsTransformInvalid()) {
        kValue.m_kTranslate.x = -std::numeric_limits<float>::max();
        kValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
        kValue.m_fScale = -std::numeric_limits<float>::max();
        return false;
    }

    kValue = m_kTransformValue;
    m_fLastTime = fTime;
    return true;
}

void NiLookAtInterpolator::Collapse() {
    for (NiInterpolator* pkInterpolator : m_aspInterpolators) {
        if (pkInterpolator) {
            pkInterpolator->Collapse();
        }
    }
}

// The union of the sub-interpolators' ranges, skipping empty ones; zero to zero when none has a range.
void NiLookAtInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
    fBeginKeyTime = std::numeric_limits<float>::max();
    fEndKeyTime = -std::numeric_limits<float>::max();
    for (NiInterpolator* pkInterpolator : m_aspInterpolators) {
        if (pkInterpolator) {
            float fBegin;
            float fEnd;
            pkInterpolator->GetActiveTimeRange(fBegin, fEnd);
            if (fBegin != fEnd) {
                if (fBegin < fBeginKeyTime) {
                    fBeginKeyTime = fBegin;
                }
                if (fEnd > fEndKeyTime) {
                    fEndKeyTime = fEnd;
                }
            }
        }
    }

    if (fBeginKeyTime == std::numeric_limits<float>::max() && fEndKeyTime == -std::numeric_limits<float>::max()) {
        fBeginKeyTime = 0.0f;
        fEndKeyTime = 0.0f;
    }
}

void NiLookAtInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    for (NiInterpolator* pkInterpolator : m_aspInterpolators) {
        if (pkInterpolator) {
            pkInterpolator->GuaranteeTimeRange(fStartTime, fEndTime);
        }
    }
}

// The base call returns a clone of this interpolator; its sub-interpolators are replaced by their sequence parts.
NiInterpolator* NiLookAtInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiLookAtInterpolator* pkSeqInterp = static_cast<NiLookAtInterpolator*>(
        NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime)
    );
    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            pkSeqInterp->m_aspInterpolators[i] = m_aspInterpolators[i]->GetSequenceInterpolator(fStartTime, fEndTime);
        }
    }
    return pkSeqInterp;
}

// Looks the target up by name when it is not set yet.
bool NiLookAtInterpolator::ResolveDependencies(NiAVObjectPalette* pkPalette) {
    if (!m_pkLookAt) {
        NiAVObject* pkLookAt = pkPalette->GetAVObject(m_kLookAtName);
        if (!pkLookAt) {
            return false;
        }
        m_pkLookAt = pkLookAt;
    }
    return true;
}

// Swaps the target pointer for its name, so ResolveDependencies can find it again; fails for an unnamed target.
bool NiLookAtInterpolator::SetUpDependencies() {
    const char* pcName = m_pkLookAt->m_kName.GetHandle();
    if (pcName) {
        m_kLookAtName.Assign(NiFixedString(pcName));
        m_pkLookAt = nullptr;
        return true;
    }

    m_kLookAtName.Assign(EmptyFS);
    return false;
}

// The NiFixedString assignment: the new handle is taken before the old one is released.
void NiLookAtInterpolator::SetLookAtName(const NiFixedString& kName) {
    m_kLookAtName.Assign(kName);
}

// The clone's name is set to the empty string (SetUpDependencies/ResolveDependencies carry the target), and each
// sub-interpolator is cloned: the old one is released before the new one is stored and referenced, and this order
// matters.
void NiLookAtInterpolator::CopyMembers(NiLookAtInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_uFlags = m_uFlags;
    // SetFlip(GetFlip()), then SetAxis(GetAxis()).
    if (m_uFlags & FLIP_MASK) {
        pkDest->m_uFlags |= FLIP_MASK;
    }
    else {
        pkDest->m_uFlags &= ~FLIP_MASK;
    }
    pkDest->m_uFlags = (m_uFlags & AXIS_MASK) | (pkDest->m_uFlags & ~AXIS_MASK);
    pkDest->m_pkLookAt = m_pkLookAt;
    pkDest->m_kLookAtName.Assign(EmptyFS);
    pkDest->m_kTransformValue = m_kTransformValue;

    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            NiInterpolator* pkClone = static_cast<NiInterpolator*>(m_aspInterpolators[i]->Clone());
            pkDest->m_aspInterpolators[i] = pkClone;
        }
    }
}

NiObject* NiLookAtInterpolator::CreateObject() {
    return new NiLookAtInterpolator(nullptr, NiLookAtInterpolator::X, false);
}
