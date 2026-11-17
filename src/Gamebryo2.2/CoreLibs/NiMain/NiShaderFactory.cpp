#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalConstantEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <cstdarg>

NiShaderFactory::NiShaderFactory()
  : m_kGlobalConstantMap(37), m_pfnClassCreate(nullptr), m_pfnRunParser(nullptr), m_pfnErrorCallback(nullptr) {}

NiShaderFactory::~NiShaderFactory() {
    if (ms_pkShaderFactory) {
        ms_pkShaderFactory->RemoveAllShaders();
        if (ms_pkShaderFactory) {
            ms_pkShaderFactory->RemoveAllLibraries();
        }
    }
    NiTMapItem<NiFixedString, NiGlobalConstantEntry*>* kIter = m_kGlobalConstantMap.GetFirstPos();
    while (kIter) {
        NiFixedString kName;
        NiGlobalConstantEntry* pkEntry = nullptr;
        m_kGlobalConstantMap.GetNext(kIter, kName, pkEntry);
        if (pkEntry) {
            ReleaseGlobalShaderConstant(kName);
        }
    }
    if (ms_pkShaderFactory == this) {
        ms_pkShaderFactory = nullptr;
    }
}

void NiShaderFactory::ReleaseAllShaders() {
    if (NiShaderFactory::ms_pkShaderFactory) {
        NiShaderFactory::ms_pkShaderFactory->RemoveAllShaders();
    }
}

bool NiShaderFactory::RegisterGlobalShaderConstant(
    const NiFixedString& kKey,
    NiShaderAttributeDesc::AttributeType eType,
    uint32_t uiDataSize,
    const void* pvInitialData
) {
    FNVD_INLINED_LOCAL(NiFixedString, kGlobalKey, std::nullopt, std::nullopt);
    NiShaderFactory* pkFactory = NiShaderFactory::ms_pkShaderFactory;
    if (!pkFactory) {
        return false;
    }
    const char* pcHandle = kKey.GetHandle();
    // NiTFixedStringMap hashes the shared string's address.
    NiTMapItem<NiFixedString, NiGlobalConstantEntry*>* v5 =
        pkFactory->m_kGlobalConstantMap.m_ppkHashTable[static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(pcHandle) % pkFactory->m_kGlobalConstantMap.m_uiHashSize
        )];
    if (v5) {
        while (pcHandle != v5->m_key.GetHandle()) {
            v5 = v5->m_pkNext;
            if (!v5) {
                goto LABEL_6;
            }
        }
        NiGlobalConstantEntry* m_val = v5->m_val;
        if (m_val) {
            if (m_val->m_eType == eType) {
                NiIncRefCount(m_val);
                return true;
            }
            return false;
        }
    }
LABEL_6:;
    NiGlobalConstantEntry* v6 =
        static_cast<NiGlobalConstantEntry*>(NiMemObject::operator new(sizeof(NiGlobalConstantEntry)));
    if (!v6) {
        return false;
    }
    NiGlobalConstantEntry* pkEntry =
        ::new (v6) NiGlobalConstantEntry(kKey.GetHandle(), eType, uiDataSize, pvInitialData);
    if (!pkEntry) {
        return false;
    }
    // The map holds a raw pointer: the factory takes its reference by hand.
    NiIncRefCount(pkEntry);
    NiGlobalStringTable::AcquireHandle(kKey.GetHandle());
    // MAYBE-INLINED 0xAA04BC: a write opening NiFixedString's uses: may be its inlined constructor
    kGlobalKey.m_kHandle = kKey.GetHandle();
    const char* m_kHandle = kGlobalKey.GetHandle();
    NiShaderFactory::ms_pkShaderFactory->m_kGlobalConstantMap.SetAt(kGlobalKey, pkEntry);
    // Destructor of kGlobalKey.
    NiGlobalStringTable::ReleaseHandle(m_kHandle);
    return true;
}

bool NiShaderFactory::ReleaseGlobalShaderConstant(const NiFixedString& kKey) {
    FNVD_INLINED_LOCAL(NiFixedString, kRemoveKey, std::nullopt, std::nullopt);
    NiShaderFactory* v1 = NiShaderFactory::ms_pkShaderFactory;
    if (!NiShaderFactory::ms_pkShaderFactory) {
        return false;
    }
    const char* m_kHandle = kKey.GetHandle();
    // NiTFixedStringMap hashes the shared string's address.
    NiTMapItem<NiFixedString, NiGlobalConstantEntry*>* v4 =
        v1->m_kGlobalConstantMap.m_ppkHashTable[static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(m_kHandle) % v1->m_kGlobalConstantMap.m_uiHashSize
        )];
    if (!v4) {
        return false;
    }
    while (m_kHandle != v4->m_key.GetHandle()) {
        v4 = v4->m_pkNext;
        if (!v4) {
            return false;
        }
    }
    NiGlobalConstantEntry* m_pObject = v4->m_val;
    if (!m_pObject) {
        return false;
    }
    // The copy of akKey into kRemoveKey takes its reference through the first load of the handle and stores a second
    // load; the factory is reloaded only on the path that takes the reference.
    if (m_kHandle != NiGlobalStringTable::NULL_STRING) {
        NiGlobalStringTable::AcquireHandle(m_kHandle);
        v1 = NiShaderFactory::ms_pkShaderFactory;
    }
    bool v6 = m_pObject->m_uiRefCount == 1;
    const char* v7 = kKey.GetHandle();
    // MAYBE-INLINED 0xAA0552: a write opening NiFixedString's uses: may be its inlined constructor
    kRemoveKey.m_kHandle = v7;
    if (v6) {
        v1->m_kGlobalConstantMap.RemoveAt(kRemoveKey);
    }
    NiDecRefCount(m_pObject);
    NiGlobalStringTable::ReleaseHandle(v7);
    return true;
}

// Formats the message and hands it to the factory's error callback; the factory is read again after formatting.
uint32_t NiShaderFactory::ReportError(uint32_t eError, int32_t bRecoverable, const char* pacFmt, ...) {
    if (!ms_pkShaderFactory) {
        return 0;
    }
    char acBuffer[2048];
    va_list kArgs;
    va_start(kArgs, pacFmt);
    vsprintf_s(acBuffer, sizeof(acBuffer), pacFmt, kArgs);
    va_end(kArgs);
    if (!ms_pkShaderFactory->m_pfnErrorCallback) {
        return 0;
    }
    return ms_pkShaderFactory->m_pfnErrorCallback(acBuffer, static_cast<NiShaderError>(eError), bRecoverable);
}

NiShaderFactory* NiShaderFactory::GetInstance() {
    return NiShaderFactory::ms_pkShaderFactory;
}

NiGlobalConstantEntry::NiGlobalConstantEntry(
    const char* pcKey,
    NiShaderAttributeDesc::AttributeType eType,
    uint32_t uiDataSize,
    const void* pvDataSource
)
  : m_kKey(pcKey), m_eType(eType), m_uiDataSize(0), m_pvDataSource(nullptr) {
    SetData(uiDataSize, pvDataSource);
}
