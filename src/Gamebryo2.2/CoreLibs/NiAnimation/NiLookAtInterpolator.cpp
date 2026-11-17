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

#include <cfloat>

NiRTTI NiLookAtInterpolator::ms_RTTI("NiLookAtInterpolator", &NiInterpolator::ms_RTTI);

NiLookAtInterpolator::NiLookAtInterpolator(NiAVObject* pkLookAt, NiLookAtInterpolator::Axis eAxis, bool bFlip)
  : m_uFlags(0), m_pkLookAt(pkLookAt) {
    m_kTransformValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kTransformValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kTransformValue.m_fScale = -3.4028235e+38f;
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
    NiLookAtInterpolator* v3 =
        static_cast<NiLookAtInterpolator*>(NiMemObject::operator new(sizeof(NiLookAtInterpolator)));
    if (v3) {
        NiLookAtInterpolator* pkObject = ::new (v3) NiLookAtInterpolator(nullptr, NiLookAtInterpolator::X, 0);
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(0, kCloning);
        return 0;
    }
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
    for (uint32_t i = 0; i < 3; i++) {
        m_aspInterpolators[i] = static_cast<NiInterpolator*>(kStream.ResolveLinkID());
    }
}

void NiLookAtInterpolator::LinkObject(NiStream& kStream) {
    NiInterpolator::LinkObject(kStream);
}

bool NiLookAtInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiInterpolator::RegisterStreamables(kStream)) {
        return false;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i] && !m_aspInterpolators[i]->RegisterStreamables(kStream)) {
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
    for (uint32_t i = 0; i < 3; i++) {
        kStream.SaveLinkID(m_aspInterpolators[i]);
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
    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            m_aspInterpolators[i]->GetViewerStrings(pkStrings);
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
// KEEP: the fourth axis value (3, from the flags' two axis bits) leaves kRot unwritten, and the exe reads it in
// TransposeTimes (0xA4C317 falls through to 0xA4C321): an uninitialised read on that path.
bool NiLookAtInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    if (!m_pkLookAt) {
        kValue.m_kTranslate.x = -FLT_MAX;
        kValue.m_kRotate.m_fX = -FLT_MAX;
        kValue.m_fScale = -FLT_MAX;
        m_kTransformValue.m_kTranslate.x = -FLT_MAX;
        m_kTransformValue.m_kRotate.m_fX = -FLT_MAX;
        m_kTransformValue.m_fScale = -FLT_MAX;
        return false;
    }

    NiPoint3 kLookAtPos = m_pkLookAt->m_kWorld.m_Translate;
    NiAVObject* pkTarget = static_cast<NiAVObject*>(pkInterpTarget);
    NiNode* pkParent = pkTarget->m_pkParent;

    NiPoint3 kTranslate;
    if (m_aspInterpolators[0]) {
        if (!m_aspInterpolators[0]->Update(fTime, pkInterpTarget, kTranslate)) {
            m_kTransformValue.m_kTranslate.x = -FLT_MAX;
            m_kTransformValue.m_kRotate.m_fX = -FLT_MAX;
            m_kTransformValue.m_fScale = -FLT_MAX;
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
        // EXE-INLINED 0x45BB20
        NiPoint3 kScaled;
        kScaled.x = kRotated.x * fParentScale;
        kScaled.y = kRotated.y * fParentScale;
        kScaled.z = kRotated.z * fParentScale;
        // EXE-INLINED 0x439E90
        kWorldTranslate.x = pkParent->m_kWorld.m_Translate.x + kScaled.x;
        kWorldTranslate.y = pkParent->m_kWorld.m_Translate.y + kScaled.y;
        kWorldTranslate.z = pkParent->m_kWorld.m_Translate.z + kScaled.z;
    }
    else {
        kParentRot = NiMatrix3_IDENTITY;
        kWorldTranslate = kTranslate;
    }

    // EXE-INLINED 0x439EF0
    NiPoint3 kAt;
    kAt.x = kLookAtPos.x - kWorldTranslate.x;
    kAt.y = kLookAtPos.y - kWorldTranslate.y;
    kAt.z = kLookAtPos.z - kWorldTranslate.z;
    NiMatrix3 kRot;
    // EXE-INLINED 0x4A7290
    float fSqrLength = kAt.x * kAt.x + kAt.y * kAt.y + kAt.z * kAt.z;
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
            kUp = NiPoint3_UNIT_Y;
            fDot = kAt.y;
        }
        else {
            kUp = NiPoint3_UNIT_Z;
            fDot = kAt.z;
        }
        // EXE-INLINED 0x45BB20
        NiPoint3 kAlong;
        kAlong.x = kAt.x * fDot;
        kAlong.y = kAt.y * fDot;
        kAlong.z = kAt.z * fDot;
        // EXE-INLINED 0x439EF0
        kUp.x = kUp.x - kAlong.x;
        kUp.y = kUp.y - kAlong.y;
        kUp.z = kUp.z - kAlong.z;
        kUp.Unitize();

        if (!(m_uFlags & FLIP_MASK)) {
            // EXE-INLINED 0x4A0BD0
            kAt.x = -kAt.x;
            kAt.y = -kAt.y;
            kAt.z = -kAt.z;
        }

        // EXE-INLINED 0x4B3800
        NiPoint3 kRight;
        kRight.x = kUp.y * kAt.z - kUp.z * kAt.y;
        kRight.y = kUp.z * kAt.x - kUp.x * kAt.z;
        kRight.z = kUp.x * kAt.y - kUp.y * kAt.x;
        // The fourth axis value leaves kRot unwritten, deliberately, as the game does.
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
                m_kTransformValue.m_kTranslate.x = -FLT_MAX;
                m_kTransformValue.m_kRotate.m_fX = -FLT_MAX;
                m_kTransformValue.m_fScale = -FLT_MAX;
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
            m_kTransformValue.m_kTranslate.x = -FLT_MAX;
            m_kTransformValue.m_kRotate.m_fX = -FLT_MAX;
            m_kTransformValue.m_fScale = -FLT_MAX;
            return false;
        }
        m_kTransformValue.m_fScale = fScale;
    }

    if (m_kTransformValue.IsTransformInvalid()) {
        kValue.m_kTranslate.x = -FLT_MAX;
        kValue.m_kRotate.m_fX = -FLT_MAX;
        kValue.m_fScale = -FLT_MAX;
        return false;
    }

    kValue = m_kTransformValue;
    m_fLastTime = fTime;
    return true;
}

void NiLookAtInterpolator::Collapse() {
    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            m_aspInterpolators[i]->Collapse();
        }
    }
}

// The union of the sub-interpolators' ranges, skipping empty ones; zero to zero when none has a range.
void NiLookAtInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
    fBeginKeyTime = FLT_MAX;
    fEndKeyTime = -FLT_MAX;
    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            float fBegin, fEnd;
            m_aspInterpolators[i]->GetActiveTimeRange(fBegin, fEnd);
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

    if (fBeginKeyTime == FLT_MAX && fEndKeyTime == -FLT_MAX) {
        fBeginKeyTime = 0.0f;
        fEndKeyTime = 0.0f;
    }
}

void NiLookAtInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            m_aspInterpolators[i]->GuaranteeTimeRange(fStartTime, fEndTime);
        }
    }
}

// The base call returns a clone of this interpolator; its sub-interpolators are replaced by their sequence parts.
NiInterpolator* NiLookAtInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiLookAtInterpolator* pkSeqInterp =
        static_cast<NiLookAtInterpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
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
        NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString(pcName);
        // EXE-INLINED 0x43BA10
        if (m_kLookAtName.GetHandle() != kName) {
            NiGlobalStringTable::AcquireHandle(kName);
            NiGlobalStringTable::ReleaseHandle(m_kLookAtName.GetHandle());
            m_kLookAtName.m_kHandle = kName;
        }
        // EXE-INLINED 0x4381B0
        NiGlobalStringTable::ReleaseHandle(kName);
        m_pkLookAt = nullptr;
        return true;
    }

    NiGlobalStringTable::GlobalStringHandle kEmpty = EmptyFS.GetHandle();
    // EXE-INLINED 0x43BA10
    if (m_kLookAtName.GetHandle() != kEmpty) {
        NiGlobalStringTable::AcquireHandle(kEmpty);
        NiGlobalStringTable::ReleaseHandle(m_kLookAtName.GetHandle());
        m_kLookAtName.m_kHandle = kEmpty;
    }
    return false;
}

// The NiFixedString assignment: the new handle is taken before the old one is released.
void NiLookAtInterpolator::SetLookAtName(const NiFixedString& kName) {
    NiGlobalStringTable::GlobalStringHandle kHandle = kName.GetHandle();
    // EXE-INLINED 0x43BA10
    if (m_kLookAtName.GetHandle() != kHandle) {
        NiGlobalStringTable::AcquireHandle(kHandle);
        NiGlobalStringTable::ReleaseHandle(m_kLookAtName.GetHandle());
        m_kLookAtName.m_kHandle = kHandle;
    }
}

// The clone's name is set to the empty string (SetUpDependencies/ResolveDependencies carry the target), and each
// sub-interpolator is cloned: the old one is released before the new one is stored and referenced, and this order
// matters.
void NiLookAtInterpolator::CopyMembers(NiLookAtInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);

    uint16_t uFlags = m_uFlags;
    pkDest->m_uFlags = uFlags;
    // SetFlip(GetFlip()), then SetAxis(GetAxis()).
    uint16_t uNewFlags;
    if (m_uFlags & FLIP_MASK) {
        uNewFlags = uFlags | FLIP_MASK;
    }
    else {
        uNewFlags = uFlags & ~FLIP_MASK;
    }
    pkDest->m_uFlags = uNewFlags;
    pkDest->m_uFlags ^= (m_uFlags ^ uNewFlags) & AXIS_MASK;
    pkDest->m_pkLookAt = m_pkLookAt;
    NiGlobalStringTable::GlobalStringHandle kEmpty = EmptyFS.GetHandle();
    // EXE-INLINED 0x43BA10
    if (pkDest->m_kLookAtName.GetHandle() != kEmpty) {
        NiGlobalStringTable::AcquireHandle(kEmpty);
        NiGlobalStringTable::ReleaseHandle(pkDest->m_kLookAtName.GetHandle());
        pkDest->m_kLookAtName.m_kHandle = kEmpty;
    }
    pkDest->m_kTransformValue = m_kTransformValue;

    for (uint32_t i = 0; i < 3; i++) {
        if (m_aspInterpolators[i]) {
            NiInterpolator* pkClone = static_cast<NiInterpolator*>(m_aspInterpolators[i]->Clone());
            pkDest->m_aspInterpolators[i] = pkClone;
        }
    }
}

NiObject* NiLookAtInterpolator::CreateObject() {
    NiLookAtInterpolator* v0 =
        static_cast<NiLookAtInterpolator*>(NiMemObject::operator new(sizeof(NiLookAtInterpolator)));
    if (v0) {
        return ::new (v0) NiLookAtInterpolator(nullptr, NiLookAtInterpolator::X, 0);
    }
    else {
        return 0;
    }
}
