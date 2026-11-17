#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "SDK/MSVC/ehvec.h"

// A string interned in the global string table; equal strings share one handle.
class NiFixedString {
public:
    // The empty string: the shared null handle, no reference taken.
    NiFixedString() : m_kHandle(NiGlobalStringTable::NULL_STRING) {}
    // noexcept: this constructor never throws.
    NiFixedString(const char* pcString) noexcept;
    // Takes a reference on arString's handle.
    NiFixedString(const NiFixedString& arString);
    // Drops the handle's reference.
    ~NiFixedString();

    // equal by handle, else by contents
    bool Equals(const char* pcStr) const;

    // The interned characters, or null; interned strings with equal contents share this pointer.
    const char* GetHandle() const {
        return m_kHandle;
    }

    // Stores kHandle without touching any reference count, as a constructor does when writing a default name.
    void SetHandle(NiGlobalStringTable::GlobalStringHandle kHandle) {
        m_kHandle = kHandle;
    }

    operator const char*() const {
        return m_kHandle;
    }
    // Does the same as operator const char*() const.
    const char* GetHandleString() const;

    bool Exists() const {
        return m_kHandle != nullptr;
    }

    // equal by handle only
    bool operator==(const NiFixedString& kString) const {
        return m_kHandle == kString.m_kHandle;
    }

    // Takes a reference on the new handle, then drops the old one's. Copy construction takes no reference here (there
    // is no destructor to give it back), so a copy must not be assigned to afterwards.
    NiFixedString& operator=(const NiFixedString& kString) {
        if (m_kHandle != kString.m_kHandle) {
            NiGlobalStringTable::GlobalStringHandle kHandle = kString.m_kHandle;
            NiGlobalStringTable::IncRefCount(kHandle);
            NiGlobalStringTable::DecRefCount(m_kHandle);
            m_kHandle = kHandle;
        }
        return *this;
    }

    // Does the same as operator=(const NiFixedString&) but calls IncRefCount and DecRefCount out of line. Defined in
    // ModelLoader.cpp.
    NiFixedString& Assign(const NiFixedString& kString);

    // Interns pcString (AddString's reference is the one kept), then drops the old handle's.
    NiFixedString& operator=(const char* pcString) {
        if (m_kHandle != pcString) {
            NiGlobalStringTable::GlobalStringHandle kHandle = m_kHandle;
            m_kHandle = NiGlobalStringTable::AddString(pcString);
            NiGlobalStringTable::DecRefCount(kHandle);
        }
        return *this;
    }

    bool Bool() const;
    void* Set(char* pcString);
    bool EqualsNoCase(const char* pcStr);

    // Handle compare of two strings (Gamebryo's operator==(const NiFixedString&, const NiFixedString&)).
    static bool HandlesEqual(const NiFixedString& arString1, const NiFixedString& arString2);
    // Freed through ourFree.
    static void operator delete(void* apMemory);

    NiGlobalStringTable::GlobalStringHandle m_kHandle;
};

// Replaces kString with its standardized path (NiPath::Standardize on a 260-byte copy).
void NiStandardizeFilePath(NiFixedString& kString);

// An empty fixed string read in many places. A global here because the class is incomplete inside its own body.
extern NiFixedString EmptyFS;
