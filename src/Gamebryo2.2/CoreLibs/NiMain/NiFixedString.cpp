#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "SDK/MSVC/stdlib.h"

#include <cstring>

NiFixedString EmptyFS;

// Gamebryo stores NULL for a null text; this loads NiGlobalStringTable::NULL_STRING instead.
NiFixedString::NiFixedString(const char* pcString) noexcept
  : m_kHandle(pcString ? NiGlobalStringTable::AddString(pcString) : NiGlobalStringTable::NULL_STRING) {}

NiFixedString::NiFixedString(const NiFixedString& arString) : m_kHandle(arString.m_kHandle) {
    NiGlobalStringTable::IncHandleRefCount(m_kHandle);
}

NiFixedString::~NiFixedString() {
    NiGlobalStringTable::DecHandleRefCount(m_kHandle);
}

bool NiFixedString::Equals(const char* pcStr) const {
    if (m_kHandle == pcStr) {
        return true;
    }
    return m_kHandle && pcStr && std::strcmp(m_kHandle, pcStr) == 0;
}

bool NiFixedString::Bool() const {
    return m_kHandle != NiGlobalStringTable::NULL_STRING;
}

void* NiFixedString::Set(char* pcString) {
    const char* m_kHandle = this->GetHandle();
    if (this->GetHandle() != pcString) {
        this->m_kHandle = NiGlobalStringTable::AddString(pcString);
        NiGlobalStringTable::ReleaseHandle(m_kHandle);
    }
    return this;
}

bool NiFixedString::HandlesEqual(const NiFixedString& arString1, const NiFixedString& arString2) {
    return arString1.m_kHandle == arString2.m_kHandle;
}
