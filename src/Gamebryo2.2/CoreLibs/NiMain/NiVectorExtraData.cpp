#include "Gamebryo2.2/CoreLibs/NiMain/NiVectorExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiRTTI NiVectorExtraData::ms_RTTI("NiVectorExtraData", &NiExtraData::ms_RTTI);

NiVectorExtraData::NiVectorExtraData() : m_afVector{0.0f, 0.0f, 0.0f, 0.0f} {}

NiVectorExtraData::~NiVectorExtraData() {}

NiObject* NiVectorExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiVectorExtraData* pkObject_2 = new NiVectorExtraData;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_afVector[0] = m_afVector[0];
    pkObject_2->m_afVector[1] = m_afVector[1];
    pkObject_2->m_afVector[2] = m_afVector[2];
    pkObject_2->m_afVector[3] = m_afVector[3];
    return pkObject_2;
}

void NiVectorExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(5, 0, 0, 14)) {
        // Older files store a three-component point; the fourth component is zero.
        NiPoint3 kPoint;
        kPoint.LoadBinary(kStream);
        m_afVector[0] = kPoint.x;
        m_afVector[1] = kPoint.y;
        m_afVector[2] = kPoint.z;
        m_afVector[3] = 0.0f;
    }
    else {
        NiStreamLoadBinary(kStream, m_afVector[0]);
        NiStreamLoadBinary(kStream, m_afVector[1]);
        NiStreamLoadBinary(kStream, m_afVector[2]);
        NiStreamLoadBinary(kStream, m_afVector[3]);
    }
}

void NiVectorExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiVectorExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiVectorExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_afVector[0]);
    NiStreamSaveBinary(kStream, m_afVector[1]);
    NiStreamSaveBinary(kStream, m_afVector[2]);
    NiStreamSaveBinary(kStream, m_afVector[3]);
}

// Neither the base class nor the type is checked; a NaN component never matches.
bool NiVectorExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    NiVectorExtraData* pkOther = static_cast<NiVectorExtraData*>(pObject);
    for (uint32_t i = 0; i < 4; i++) {
        if (m_afVector[i] != pkOther->m_afVector[i]) {
            return false;
        }
    }
    return true;
}

void NiVectorExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiVectorExtraData::ms_RTTI.GetName()));

    const size_t stLen = 55;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(
        pcString,
        stLen,
        "Vector = (%5.3f,%5.3f,%5.3f,%5.3f)",
        static_cast<double>(m_afVector[0]),
        static_cast<double>(m_afVector[1]),
        static_cast<double>(m_afVector[2]),
        static_cast<double>(m_afVector[3])
    );
    pStrings->Add(pcString);
}

NiObject* NiVectorExtraData::CreateObject() {
    NiVectorExtraData* pkObject = new NiVectorExtraData;
    return pkObject;
}
