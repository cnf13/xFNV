#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"

NiTextKey::NiTextKey() {}

// The text's reference is released by its NiFixedString member.
NiTextKey::~NiTextKey() {}

void NiTextKey::LoadBinary(NiStream& stream) {
    NiStreamLoadBinary(stream, m_fTime);

    if (stream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
        stream.LoadCStringAsFixedString(m_kText);
    }
    else {
        stream.LoadFixedString(m_kText);
    }
}

void NiTextKey::SaveBinary(NiStream& stream) {
    NiStreamSaveBinary(stream, m_fTime);
    stream.SaveFixedString(m_kText);
}

void NiTextKey::RegisterStreamables(NiStream& stream) {
    stream.RegisterFixedString(m_kText);
}

void NiTextKey::SetText(const NiFixedString& kText) {
    const char* m_kHandle = (&kText)->GetHandle();
    if (m_kText.GetHandle() != (&kText)->GetHandle()) {
        NiGlobalStringTable::AcquireHandle(m_kHandle);
        const char* v4 = m_kText.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v4);
        m_kText.m_kHandle = m_kHandle;
    }
}
