#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineTransformInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBSplineTransformInterpolator::ms_RTTI("NiBSplineTransformInterpolator", &NiBSplineInterpolator::ms_RTTI);

// The transform value starts invalid.
NiBSplineTransformInterpolator::NiBSplineTransformInterpolator(
    NiBSplineData* pkData,
    uint32_t kTransCPHandle,
    uint32_t kRotCPHandle,
    uint32_t kScaleCPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineInterpolator(pkData, pkBasisData), m_kTransCPHandle(kTransCPHandle), m_kRotCPHandle(kRotCPHandle),
    m_kScaleCPHandle(kScaleCPHandle) {
    m_kTransformValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kTransformValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kTransformValue.m_fScale = -0x1.fffffe0000000p+127f;
}

NiBSplineTransformInterpolator::~NiBSplineTransformInterpolator() {}

const NiRTTI* NiBSplineTransformInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineTransformInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineTransformInterpolator* v4;
    v4 = new NiBSplineTransformInterpolator(nullptr, 0xFFFFu, 0xFFFFu, 0xFFFFu, nullptr);
    NiBSplineInterpolator::CopyMembers(v4, kCloning);
    __builtin_memcpy(
        &v4->m_kTransformValue, &m_kTransformValue,
        sizeof(NiBSplineTransformInterpolator) - offsetof(NiBSplineTransformInterpolator, m_kTransformValue)
    );
    return v4;
}

void NiBSplineTransformInterpolator::LoadBinary(NiStream& kStream) {
    NiBSplineInterpolator::LoadBinary(kStream);
    m_kTransformValue.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_kTransCPHandle);
    NiStreamLoadBinary(kStream, m_kRotCPHandle);
    NiStreamLoadBinary(kStream, m_kScaleCPHandle);
}

void NiBSplineTransformInterpolator::LinkObject(NiStream& kStream) {
    NiBSplineInterpolator::LinkObject(kStream);
}

bool NiBSplineTransformInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineInterpolator::RegisterStreamables(kStream);
}

void NiBSplineTransformInterpolator::SaveBinary(NiStream& kStream) {
    NiBSplineInterpolator::SaveBinary(kStream);
    m_kTransformValue.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_kTransCPHandle);
    NiStreamSaveBinary(kStream, m_kRotCPHandle);
    NiStreamSaveBinary(kStream, m_kScaleCPHandle);
}

bool NiBSplineTransformInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineTransformInterpolator* pkOther = static_cast<NiBSplineTransformInterpolator*>(pkObject);
    return m_kTransformValue == pkOther->m_kTransformValue && m_kTransCPHandle == pkOther->m_kTransCPHandle &&
        m_kRotCPHandle == pkOther->m_kRotCPHandle && m_kScaleCPHandle == pkOther->m_kScaleCPHandle;
}

void NiBSplineTransformInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    m_kTransformValue.GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString("m_kTransCPHandle", m_kTransCPHandle));
    pkStrings->Add(NiGetViewerString("m_kRotCPHandle", m_kRotCPHandle));
    pkStrings->Add(NiGetViewerString("m_kScaleCPHandle", m_kScaleCPHandle));
}

// Each part with control points is evaluated; the others keep their loaded values. Fails while every part is
// invalid; only a successful update caches the time.
bool NiBSplineTransformInterpolator::Update(float fTime, NiObjectNET*, NiQuatTransform& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kTransformValue;
        return !m_kTransformValue.IsTransformInvalid();
    }

    float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);

    if (m_kTransCPHandle != NiBSplineData::INVALID_HANDLE) {
        float afValue[3];
        m_spData->GetValueDegree3(fNormTime, afValue, 3, m_spBasisData, m_kTransCPHandle);
        m_kTransformValue.m_kTranslate.x = afValue[0];
        m_kTransformValue.m_kTranslate.y = afValue[1];
        m_kTransformValue.m_kTranslate.z = afValue[2];
    }

    if (m_kRotCPHandle != NiBSplineData::INVALID_HANDLE) {
        float afValue[4];
        m_spData->GetValueDegree3(fNormTime, afValue, 4, m_spBasisData, m_kRotCPHandle);
        NiQuaternion kRotate;
        kRotate.SetValues(afValue[0], afValue[1], afValue[2], afValue[3]);
        kRotate.FastNormalize();
        m_kTransformValue.m_kRotate = kRotate;
    }

    if (m_kScaleCPHandle != NiBSplineData::INVALID_HANDLE) {
        float fScale;
        m_spData->GetValueDegree3(fNormTime, &fScale, 1, m_spBasisData, m_kScaleCPHandle);
        m_kTransformValue.m_fScale = fScale;
    }

    kValue = m_kTransformValue;
    if (m_kTransformValue.IsTransformInvalid()) {
        return false;
    }
    m_fLastTime = fTime;
    return true;
}

// Position and rotation take 3 and 4 floats per control point, scale 1.
uint32_t NiBSplineTransformInterpolator::GetDimension(int32_t usChannel) const {
    switch (static_cast<uint16_t>(usChannel)) {
        case POSITION: return 3;
        case ROTATION: return 4;
        case SCALE: return 1;
        default: return 0;
    }
}

uint32_t NiBSplineTransformInterpolator::GetDegree(uint16_t usChannel) const {
    switch (usChannel) {
        case POSITION:
        case ROTATION:
        case SCALE: return 3;
        default: return 0;
    }
}

// A part without control points holds its loaded value, if that value is valid. Switches on the low 16 bits of the
// channel and passes the whole value on to GetControlPointCount.
bool NiBSplineTransformInterpolator::GetChannelPosed(int32_t usChannel) const {
    switch (static_cast<uint16_t>(usChannel)) {
        case POSITION: return GetControlPointCount(usChannel) == 0 && m_kTransformValue.IsTranslateValid();
        case ROTATION: return GetControlPointCount(usChannel) == 0 && m_kTransformValue.IsRotateValid();
        case SCALE: return GetControlPointCount(usChannel) == 0 && m_kTransformValue.IsScaleValid();
        default: return false;
    }
}

NiBSplineData::Handle NiBSplineTransformInterpolator::GetControlHandle(int32_t usChannel) const {
    switch (static_cast<uint16_t>(usChannel)) {
        case POSITION: return m_kTransCPHandle;
        case ROTATION: return m_kRotCPHandle;
        case SCALE: return m_kScaleCPHandle;
        default: return NiBSplineData::INVALID_HANDLE;
    }
}

void NiBSplineTransformInterpolator::SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) {
    switch (usChannel) {
        case POSITION: m_kTransCPHandle = kControlHandle; break;
        case ROTATION: m_kRotCPHandle = kControlHandle; break;
        case SCALE: m_kScaleCPHandle = kControlHandle; break;
    }
}

void NiBSplineTransformInterpolator::CopyMembers(NiBSplineTransformInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiBSplineInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_kTransformValue = m_kTransformValue;
    pkDest->m_kTransCPHandle = m_kTransCPHandle;
    pkDest->m_kRotCPHandle = m_kRotCPHandle;
    pkDest->m_kScaleCPHandle = m_kScaleCPHandle;
}

NiObject* NiBSplineTransformInterpolator::CreateObject() {
    return new NiBSplineTransformInterpolator(nullptr, 0xFFFFu, 0xFFFFu, 0xFFFFu, nullptr);
}
