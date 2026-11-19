#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStringPalette.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiStringPalette::ms_RTTI("NiStringPalette", &NiObject::ms_RTTI);

NiStringPalette::NiStringPalette() : NiObject(), m_pcBuffer(nullptr), m_uiAllocatedSize(0), m_uiEndOfBuffer(0) {}

NiStringPalette::~NiStringPalette() {
    _NiFree(m_pcBuffer);
}

const NiRTTI* NiStringPalette::GetRTTI() const {
    return &ms_RTTI;
}

void NiStringPalette::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiAllocatedSize);
    if (m_uiAllocatedSize) {
        m_pcBuffer = static_cast<char*>(_NiMalloc(m_uiAllocatedSize));
        NiStreamLoadBinary(kStream, m_pcBuffer, m_uiAllocatedSize);
    }

    NiStreamLoadBinary(kStream, m_uiEndOfBuffer);
}

void NiStringPalette::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiStringPalette::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

// Only the used part of the buffer is written; its size goes out as both the allocated and the used size.
void NiStringPalette::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    uint32_t uiSize = m_uiEndOfBuffer;
    NiStreamSaveBinary(kStream, uiSize);
    if (m_uiEndOfBuffer) {
        NiStreamSaveBinary(kStream, m_pcBuffer, m_uiEndOfBuffer);
    }

    NiStreamSaveBinary(kStream, m_uiEndOfBuffer);
}

bool NiStringPalette::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiStringPalette* pPalette = static_cast<NiStringPalette*>(pkObject);
    return pPalette->m_uiAllocatedSize == m_uiAllocatedSize && pPalette->m_uiEndOfBuffer == m_uiEndOfBuffer &&
        std::memcmp(m_pcBuffer, pPalette->m_pcBuffer, m_uiAllocatedSize) == 0;
}

NiObject* NiStringPalette::CreateObject() {
    return new NiStringPalette;
}
