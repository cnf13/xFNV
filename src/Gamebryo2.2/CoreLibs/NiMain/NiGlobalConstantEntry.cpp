#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalConstantEntry.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

// The key's reference is released by its NiFixedString member.
NiGlobalConstantEntry::~NiGlobalConstantEntry() {
    _NiFree(m_pvDataSource);
}

void NiGlobalConstantEntry::SetData(uint32_t uiDataSize, const void* pvDataSource) {
    if (m_uiDataSize < uiDataSize) {
        _NiFree(m_pvDataSource);
        m_pvDataSource = nullptr;
        m_uiDataSize = uiDataSize;
    }
    if (!m_pvDataSource) {
        m_pvDataSource = _NiMalloc(m_uiDataSize);
    }
    if (pvDataSource) {
        rt::memcpy(m_pvDataSource, pvDataSource, uiDataSize);
    }
}
