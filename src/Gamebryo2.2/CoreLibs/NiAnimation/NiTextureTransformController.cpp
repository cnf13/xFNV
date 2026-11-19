#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextureTransformController.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cfloat>

NiRTTI NiTextureTransformController::ms_RTTI("NiTextureTransformController", &NiFloatInterpController::ms_RTTI);

// Sets the target, caches pkMap and finds it among the target's stage maps, then its shader maps.
NiTextureTransformController::NiTextureTransformController(
    NiTexturingProperty* pkTarget,
    NiTexturingProperty::Map* pkMap,
    TransformMember eMember
)
  : m_uiLastIdx(0), m_pkMap(nullptr), m_bShaderMap(false), m_uiMapIndex(0), m_eMember(eMember), m_pcCtlrID(nullptr) {
    NiTimeController::SetTarget(pkTarget);
    m_pkMap = pkMap;
    if (!pkMap) {
        return;
    }
    NiTexturingProperty* pkProperty = static_cast<NiTexturingProperty*>(m_pkTarget);
    if (!pkProperty) {
        return;
    }
    for (uint32_t i = 0; i < pkProperty->m_kMaps.GetSize(); ++i) {
        if (pkProperty->m_kMaps.m_pBase[i] == pkMap) {
            m_bShaderMap = false;
            m_uiMapIndex = i;
            SetInternalMap();
            return;
        }
    }
    uint32_t uiShaderCount = pkProperty->GetShaderArrayCount();
    NiTPrimitiveArray<NiTexturingProperty::ShaderMap*>* pkShaderMaps = pkProperty->m_pkShaderMaps;
    for (uint32_t i = 0; i < uiShaderCount; ++i) {
        NiTexturingProperty::Map* pkShaderMap =
            pkShaderMaps && i < pkShaderMaps->GetSize() ? pkShaderMaps->m_pBase[i] : nullptr;
        if (pkShaderMap == pkMap) {
            m_bShaderMap = true;
            m_uiMapIndex = i;
            SetInternalMap();
            return;
        }
    }
}

NiTextureTransformController::~NiTextureTransformController() {
    _NiFree(m_pcCtlrID);
    m_pcCtlrID = nullptr;
}

const NiRTTI* NiTextureTransformController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiTextureTransformController::CreateClone(NiCloningProcess& kCloning) {
    NiTextureTransformController* v4;
    v4 = new NiTextureTransformController(nullptr, nullptr, TT_TRANSLATE_U);
    NiFloatInterpController::CopyMembers(v4, kCloning);
    v4->m_uiLastIdx = m_uiLastIdx;
    v4->m_bShaderMap = (bool)((uint8_t)m_bShaderMap);
    v4->m_uiMapIndex = m_uiMapIndex;
    v4->m_eMember = static_cast<NiTextureTransformController::TransformMember>((int32_t)m_eMember);
    return v4;
}

// Files older than 10.1.0.104 carry a link to an old data block, which is skipped.
void NiTextureTransformController::LoadBinary(NiStream& kStream) {
    NiFloatInterpController::LoadBinary(kStream);

    uint8_t ucShaderMap;
    NiStreamLoadBinary(kStream, ucShaderMap);
    m_bShaderMap = ucShaderMap != 0;
    NiStreamLoadBinary(kStream, m_uiMapIndex);
    uint32_t uiMember;
    NiStreamLoadBinary(kStream, uiMember);
    m_eMember = static_cast<TransformMember>(uiMember);

    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

// KEEP: the exe reads the shader-map byte into the stream argument's own slot (lea ecx,[esp+0x24] at 0xA46994) and
// the member into a slot nothing wrote ([esp+0x44] at 0xA469E8), so a short read (malformed data) leaves the
// stream's address or stack garbage there, which src cannot express; porting.md's unwritten-slot exception covers
// GetChunkData only.
// Files older than 10.1.0.104 carry a link to old-style float data; it becomes a float interpolator, collapsed.
void NiTextureTransformController::LinkObject(NiStream& kStream) {
    NiFloatInterpController::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiFloatData* pkData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
        NiFloatInterpolator* pkInterp = new NiFloatInterpolator(pkData);
        m_spInterpolator = pkInterp;
        m_spInterpolator->Collapse();
    }
}

// NiLightColorController, NiLightDimmerController and NiMaterialColorController use the same function.
bool NiTextureTransformController::RegisterStreamables(NiStream& kStream) {
    return NiFloatInterpController::RegisterStreamables(kStream);
}

void NiTextureTransformController::SaveBinary(NiStream& kStream) {
    NiFloatInterpController::SaveBinary(kStream);
    uint8_t ucShaderMap = m_bShaderMap;
    NiStreamSaveBinary(kStream, ucShaderMap);
    NiStreamSaveBinary(kStream, m_uiMapIndex);
    NiStreamSaveBinary(kStream, m_eMember);
}

bool NiTextureTransformController::IsEqual(NiObject* pkObject) {
    if (!NiFloatInterpController::IsEqual(pkObject)) {
        return false;
    }

    NiTextureTransformController* pkOther = static_cast<NiTextureTransformController*>(pkObject);
    return pkOther->m_uiLastIdx == m_uiLastIdx && pkOther->m_bShaderMap == m_bShaderMap &&
        pkOther->m_uiMapIndex == m_uiMapIndex && pkOther->m_eMember == m_eMember;
}

void NiTextureTransformController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiFloatInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). A map without
// a transform gets a Maya-style identity transform centred on the texture.
void NiTextureTransformController::Update(NiUpdateData& UpdateData) {
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
    if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue) || !SetInternalMap()) {
        return;
    }

    if (!m_pkMap->m_pkTextureTransform) {
        void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiTextureTransform));
        NiTextureTransform* pkNewTransform = nullptr;
        if (pvMemory) {
            NiPoint2 kCenter;
            kCenter.x = 0.5f;
            kCenter.y = 0.5f;
            NiPoint2 kScale;
            kScale.x = 1.0f;
            kScale.y = 1.0f;
            NiPoint2 kTranslate;
            kTranslate.x = 0.0f;
            kTranslate.y = 0.0f;
            pkNewTransform = ::new (pvMemory)
                NiTextureTransform(kTranslate, 0.0f, kScale, kCenter, NiTextureTransform::MAYA_TRANSFORM);
        }
        m_pkMap->m_pkTextureTransform = pkNewTransform;
    }

    NiTextureTransform* pkTransform = m_pkMap->m_pkTextureTransform;
    switch (m_eMember) {
        case TT_TRANSLATE_U: {
            NiPoint2 kTranslate = pkTransform->m_kTranslate;
            kTranslate.x = fValue;
            pkTransform->SetTranslate(kTranslate);
            break;
        }
        case TT_TRANSLATE_V: {
            NiPoint2 kTranslate = pkTransform->m_kTranslate;
            kTranslate.y = fValue;
            pkTransform->SetTranslate(kTranslate);
            break;
        }
        case TT_ROTATE: pkTransform->SetRotate(fValue); break;
        case TT_SCALE_U: {
            NiPoint2 kScale = pkTransform->m_kScale;
            kScale.x = fValue;
            pkTransform->SetScale(kScale);
            break;
        }
        case TT_SCALE_V: {
            NiPoint2 kScale = pkTransform->m_kScale;
            kScale.y = fValue;
            pkTransform->SetScale(kScale);
            break;
        }
    }
}

// A new target invalidates the cached map.
void NiTextureTransformController::SetTarget(NiObjectNET* pkTarget) {
    if (m_pkTarget != pkTarget) {
        m_pkMap = nullptr;
    }
    NiTimeController::SetTarget(pkTarget);
}

// The ID is "<shader map flag>-<map index>-<member>", made on first use.
const char* NiTextureTransformController::GetCtlrID() {
    if (!m_pcCtlrID) {
        m_pcCtlrID = static_cast<char*>(_NiMalloc(50));
        uint32_t uiShaderMap = m_bShaderMap;
        switch (m_eMember) {
            case TT_TRANSLATE_U: NiSprintf(m_pcCtlrID, 50, "%d-%d-TT_TRANSLATE_U", uiShaderMap, m_uiMapIndex); break;
            case TT_TRANSLATE_V: NiSprintf(m_pcCtlrID, 50, "%d-%d-TT_TRANSLATE_V", uiShaderMap, m_uiMapIndex); break;
            case TT_ROTATE: NiSprintf(m_pcCtlrID, 50, "%d-%d-TT_ROTATE", uiShaderMap, m_uiMapIndex); break;
            case TT_SCALE_U: NiSprintf(m_pcCtlrID, 50, "%d-%d-TT_SCALE_U", uiShaderMap, m_uiMapIndex); break;
            case TT_SCALE_V: NiSprintf(m_pcCtlrID, 50, "%d-%d-TT_SCALE_V", uiShaderMap, m_uiMapIndex); break;
        }
    }
    return m_pcCtlrID;
}

// NiFlipController uses the same function.
bool NiTextureTransformController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&NiTexturingProperty::ms_RTTI, pkTarget);
}

// A map without a transform reads as 0.
void NiTextureTransformController::GetTargetFloatValue(float& fValue) {
    if (!SetInternalMap()) {
        return;
    }

    NiTextureTransform* pkTransform = m_pkMap->m_pkTextureTransform;
    if (!pkTransform) {
        fValue = 0.0f;
        return;
    }

    switch (m_eMember) {
        case TT_TRANSLATE_U: fValue = pkTransform->m_kTranslate.x; break;
        case TT_TRANSLATE_V: fValue = pkTransform->m_kTranslate.y; break;
        case TT_ROTATE: fValue = pkTransform->m_fRotate; break;
        case TT_SCALE_U: fValue = pkTransform->m_kScale.x; break;
        case TT_SCALE_V: fValue = pkTransform->m_kScale.y; break;
    }
}

bool NiTextureTransformController::SetInternalMap() {
    if (m_pkMap) {
        return true;
    }

    NiTexturingProperty* pkProperty = static_cast<NiTexturingProperty*>(m_pkTarget);
    if (!pkProperty) {
        return false;
    }

    if (m_bShaderMap) {
        NiTPrimitiveArray<NiTexturingProperty::ShaderMap*>* pkShaderMaps = pkProperty->m_pkShaderMaps;
        m_pkMap =
            pkShaderMaps && m_uiMapIndex < pkShaderMaps->GetSize() ? pkShaderMaps->m_pBase[m_uiMapIndex] : nullptr;
    }
    else {
        m_pkMap = pkProperty->m_kMaps.m_pBase[m_uiMapIndex];
    }
    return m_pkMap != nullptr;
}

NiObject* NiTextureTransformController::CreateObject() {
    return new NiTextureTransformController(nullptr, nullptr, TT_TRANSLATE_U);
}
