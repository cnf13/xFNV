#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiLinesData::ms_RTTI("NiLinesData", &NiGeometryData::ms_RTTI);

// No flags: a stream load sets them.
NiLinesData::NiLinesData() : m_pkFlags(nullptr) {}

// Without caller flags, every even vertex starts a segment.
NiLinesData::NiLinesData(
    int32_t usVertices,
    NiPoint3* pkVertex,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    int32_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod,
    uint8_t* pkFlags
)
  : NiGeometryData(usVertices, pkVertex, nullptr, pkColor, pkTexture, usNumTextureSets, eNBTMethod) {
    if (pkFlags) {
        m_pkFlags = pkFlags;
    }
    else {
        uint16_t usCount = static_cast<uint16_t>(usVertices);
        m_pkFlags = static_cast<uint8_t*>(_NiMalloc(usCount));
        for (uint16_t i = 0; i < usCount; i++) {
            m_pkFlags[i] = (i & 1) == 0;
        }
    }
}

NiLinesData::~NiLinesData() {
    _NiFree(m_pkFlags);
}

// The connect flags follow the base data, one byte per vertex.
void NiLinesData::LoadBinary(NiStream& kStream) {
    NiGeometryData::LoadBinary(kStream);
    m_pkFlags = static_cast<uint8_t*>(_NiMalloc(m_usVertices));
    NiStreamLoadBinary(kStream, m_pkFlags, m_usVertices);
}

void NiLinesData::LinkObject(NiStream& kStream) {
    NiGeometryData::LinkObject(kStream);
}

bool NiLinesData::RegisterStreamables(NiStream& kStream) {
    return NiGeometryData::RegisterStreamables(kStream);
}

void NiLinesData::SaveBinary(NiStream& kStream) {
    NiGeometryData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_pkFlags, m_usVertices);
}

// The flags compare as raw bytes; both sides without flags are equal.
bool NiLinesData::IsEqual(NiObject* pkObject) {
    if (!NiGeometryData::IsEqual(pkObject)) {
        return false;
    }

    NiLinesData* pkLines = static_cast<NiLinesData*>(pkObject);
    if ((m_pkFlags && !pkLines->m_pkFlags) || (!m_pkFlags && pkLines->m_pkFlags)) {
        return false;
    }

    if (m_pkFlags) {
        const uint8_t* pucFlags = m_pkFlags;
        const uint8_t* pucOtherFlags = pkLines->m_pkFlags;
        for (uint16_t i = 0; i < m_usVertices; i++) {
            if (pucFlags[i] != pucOtherFlags[i]) {
                return false;
            }
        }
    }
    return true;
}

void NiLinesData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiGeometryData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_pkFlags", static_cast<const void*>(m_pkFlags)));
}

NiObject* NiLinesData::CreateObject() {
    NiLinesData* pkObject = static_cast<NiLinesData*>(NiMemObject::operator new(sizeof(NiLinesData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiLinesData;
}
