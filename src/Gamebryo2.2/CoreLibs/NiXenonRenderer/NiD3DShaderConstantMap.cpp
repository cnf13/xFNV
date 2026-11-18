#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAmbientLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBooleanExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColorExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatsExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSCMExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSpotLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>

NiRTTI NiD3DShaderConstantMap::ms_RTTI("NiD3DShaderConstantMap", nullptr);

// NiTObjectArray<NiPointer<NiShaderConstantMapEntry>>::SetAt and ::AddFirstEmpty: SetAt keeps its zero element in a
// local static of its own, and AddFirstEmpty calls that SetAt.
void ShaderConstantMapEntryArray_SetAt(
    NiTObjectArray<NiPointer<NiShaderConstantMapEntry>>* apArray,
    uint32_t auiIndex,
    const NiPointer<NiShaderConstantMapEntry>& akElement
) {
    static NiPointer<NiShaderConstantMapEntry> tcmp;
    if (auiIndex < apArray->m_usSize) {
        NiPointer<NiShaderConstantMapEntry>* pBase = apArray->m_pBase;
        if (akElement.m_pObject == tcmp.m_pObject) {
            if (pBase[auiIndex].m_pObject != tcmp.m_pObject) {
                --apArray->m_usESize;
            }
        }
        else {
            if (pBase[auiIndex].m_pObject == tcmp.m_pObject) {
                ++apArray->m_usESize;
            }
        }
    }
    else {
        apArray->m_usSize = auiIndex + 1;
        if (akElement.m_pObject != tcmp.m_pObject) {
            ++apArray->m_usESize;
        }
    }
    apArray->m_pBase[auiIndex] = akElement;
}

uint32_t ShaderConstantMapEntryArray_AddFirstEmpty(
    NiTObjectArray<NiPointer<NiShaderConstantMapEntry>>* apArray,
    const NiPointer<NiShaderConstantMapEntry>& akElement
) {
    if (!akElement.m_pObject) {
        return -1;
    }
    uint16_t m_usSize = apArray->m_usSize;
    uint16_t v5 = 0;
    if (m_usSize) {
        NiPointer<NiShaderConstantMapEntry>* m_pBase = apArray->m_pBase;
        while (m_pBase[v5].m_pObject) {
            if (++v5 >= apArray->m_usSize) {
                goto LABEL_7;
            }
        }
        uint32_t v7 = v5;
        NiShaderConstantMapEntry* m_pObject = m_pBase[v5].m_pObject;
        if (m_pObject != akElement.m_pObject) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            NiShaderConstantMapEntry* v9 = akElement.m_pObject;
            m_pBase[v7].m_pObject = akElement.m_pObject;
            if (v9) {
                NiIncRefCount(v9);
            }
        }
        ++apArray->m_usESize;
        return v7;
    }

LABEL_7:;
    if (m_usSize >= static_cast<uint32_t>(apArray->m_usMaxSize)) {
        apArray->SetSize(m_usSize + apArray->m_usGrowBy);
    }
    // NiTArray::SetAt takes an unsigned int index; the size is passed zero-extended.
    ShaderConstantMapEntryArray_SetAt(apArray, m_usSize, akElement);
    return m_usSize;
}

// Copies the matrix's sixteen floats, in memory order, into the constant data.
void FillDataArray(float* pfData, const _D3DMATRIX& kMatrix) {
    pfData[0] = kMatrix._11;
    pfData[1] = kMatrix._12;
    pfData[2] = kMatrix._13;
    pfData[3] = kMatrix._14;
    pfData[4] = kMatrix._21;
    pfData[5] = kMatrix._22;
    pfData[6] = kMatrix._23;
    pfData[7] = kMatrix._24;
    pfData[8] = kMatrix._31;
    pfData[9] = kMatrix._32;
    pfData[10] = kMatrix._33;
    pfData[11] = kMatrix._34;
    pfData[12] = kMatrix._41;
    pfData[13] = kMatrix._42;
    pfData[14] = kMatrix._43;
    pfData[15] = kMatrix._44;
}

NiD3DShaderConstantMap::NiD3DShaderConstantMap(NiDX9Renderer* apRenderer, NiGPUProgram::ProgramType aeType)
  : NiShaderConstantMap(aeType), m_aspEntries(0, 1), m_aspDefinedEntries(0, 1), m_bModified(true),
    m_pkLastShaderProgram(nullptr), m_eLastError(0), m_pkD3DDevice(nullptr), m_pkD3DRenderer(nullptr),
    m_pkD3DRenderState(nullptr) {
    SetD3DRenderer(apRenderer);
    m_aspEntries.RemoveAll();
}

// Releases the global constants both entry arrays hold, empties them and drops the device; the arrays free their
// buffers themselves.
NiD3DShaderConstantMap::~NiD3DShaderConstantMap() {
    for (uint32_t i = 0; i < m_aspEntries.m_usMaxSize; ++i) {
        NiShaderConstantMapEntry* pkEntry = m_aspEntries.m_pBase[i];
        if (pkEntry &&
            (pkEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) ==
                NiShaderConstantMapEntry::SCME_MAP_GLOBAL) {
            NiShaderFactory::ReleaseGlobalShaderConstant(pkEntry->m_kKey);
        }
    }
    m_aspEntries.RemoveAll();
    for (uint32_t i = 0; i < m_aspDefinedEntries.m_usMaxSize; ++i) {
        NiShaderConstantMapEntry* pkEntry = m_aspDefinedEntries.m_pBase[i];
        if (pkEntry &&
            (pkEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) ==
                NiShaderConstantMapEntry::SCME_MAP_GLOBAL) {
            NiShaderFactory::ReleaseGlobalShaderConstant(pkEntry->m_kKey);
        }
    }
    m_aspDefinedEntries.RemoveAll();
    m_pkD3DRenderer = nullptr;
    if (m_pkD3DDevice) {
        m_pkD3DDevice->Release();
    }
    m_pkD3DDevice = nullptr;
    m_pkD3DRenderState = nullptr;
}

// Routes an entry to the Add function of its mapping (the top nibble of the flags); an entry whose key is already
// mapped with the same extra value is refused with 0x20, an unknown mapping with 0x40.
uint32_t NiD3DShaderConstantMap::AddEntry(
    const char* pszKey,
    uint32_t uiFlags,
    uint32_t uiExtra,
    uint32_t uiShaderRegister,
    uint32_t uiRegisterCount,
    const char* pszVariableName,
    uint32_t uiDataSize,
    uint32_t uiDataStride,
    const void* pvDataSource,
    bool bCopyData
) {
    m_eLastError = 0;
    NiShaderConstantMapEntry* pkEntry = GetEntry(pszKey);
    if (pkEntry && pkEntry->m_uiExtra == uiExtra) {
        m_eLastError = 0x20;
        return m_eLastError;
    }

    switch (uiFlags & 0xF0000000) {
        case 0x30000000:
            m_eLastError = AddAttributeEntry(
                pszKey, uiFlags, uiExtra, uiShaderRegister, uiRegisterCount, pszVariableName, uiDataSize, uiDataStride,
                pvDataSource, bCopyData
            );
            return m_eLastError;
        case 0x10000000:
            m_eLastError = AddConstantEntry(
                pszKey, uiFlags, uiExtra, uiShaderRegister, uiRegisterCount, pszVariableName, uiDataSize, uiDataStride,
                pvDataSource, bCopyData
            );
            return m_eLastError;
        case 0x20000000:
            m_eLastError = AddPredefinedEntry(pszKey, uiExtra, uiShaderRegister, pszVariableName, 0, 0, nullptr, false);
            return m_eLastError;
        case 0x40000000:
            m_eLastError = AddGlobalEntry(
                pszKey, uiFlags, uiExtra, uiShaderRegister, uiRegisterCount, pszVariableName, uiDataSize, uiDataStride,
                pvDataSource, bCopyData
            );
            return m_eLastError;
        case 0x50000000:
            m_eLastError =
                AddOperatorEntry(pszKey, uiFlags, uiExtra, uiShaderRegister, uiRegisterCount, pszVariableName);
            return m_eLastError;
        case 0x60000000:
            m_eLastError = AddObjectEntry(
                pszKey, uiShaderRegister, pszVariableName, uiExtra,
                NiShaderConstantMapEntry::ms_aeObjectType[(uiFlags >> 8) & 0xFF]
            );
            return m_eLastError;
    }

    m_eLastError = 0x40;
    return m_eLastError;
}

// An attribute (0x30000000) entry; its data is set only when a source is given. An entry the map refuses is deleted.
uint32_t NiD3DShaderConstantMap::AddAttributeEntry(
    const char* pszKey,
    uint32_t uiFlags,
    uint32_t uiExtra,
    uint32_t uiShaderRegister,
    uint32_t uiRegisterCount,
    const char* pszVariableName,
    uint32_t uiDataSize,
    uint32_t uiDataStride,
    const void* pvDataSource,
    bool bCopyData
) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiShaderConstantMapEntry));
    NiShaderConstantMapEntry* pkEntry = pvMemory ? ::new (pvMemory) NiShaderConstantMapEntry : nullptr;

    pkEntry->m_kKey = NiFixedString(pszKey);
    pkEntry->m_uiFlags = (uiFlags & 0x0FFFFFFF) | 0x30000000;
    pkEntry->m_uiExtra = uiExtra;
    pkEntry->m_uiShaderRegister = uiShaderRegister;
    pkEntry->m_uiRegisterCount = uiRegisterCount;

    pkEntry->m_kVariableName = NiFixedString(pszVariableName);

    if (pvDataSource) {
        pkEntry->SetData(uiDataSize, uiDataStride, const_cast<void*>(pvDataSource), bCopyData);
    }

    m_eLastError = InsertEntry(pkEntry);
    if (m_eLastError != 0) {
        delete pkEntry;
    }
    return m_eLastError;
}

// A constant (0x10000000) entry. Its data size picks the attribute flags (8: point2 ... 0x20/0x30/0x40: matrix4); float
// data of a vertex or pixel map that is not bool or unsigned int gets extra 1 or 2, so SetShaderConstants sets it on
// the device directly. An entry the map refuses is deleted.
uint32_t NiD3DShaderConstantMap::AddConstantEntry(
    const char* pszKey,
    uint32_t auiFlags,
    uint32_t auiExtra,
    uint32_t uiShaderRegister,
    uint32_t uiRegisterCount,
    const char* pszVariableName,
    uint32_t uiDataSize,
    uint32_t uiDataStride,
    const void* pvDataSource,
    bool bCopyData
) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiShaderConstantMapEntry));
    NiShaderConstantMapEntry* pkEntry = pvMemory ? ::new (pvMemory) NiShaderConstantMapEntry : nullptr;

    pkEntry->m_kKey = NiFixedString(pszKey);

    uint32_t uiFlags = (auiFlags & 0x0FFFFFFF) | 0x10000000;
    if (uiDataSize == 8) {
        uiFlags |= 5;
    }
    else if (uiDataSize == 0xC) {
        uiFlags |= 6;
    }
    else if (uiDataSize == 0x10) {
        uiFlags |= 7;
    }
    else if (uiDataSize == 0x20) {
        uiFlags |= 9;
    }
    else if (uiDataSize == 0x24) {
        uiFlags |= 8;
    }
    else if (uiDataSize == 0x30 || uiDataSize == 0x40) {
        uiFlags |= 9;
    }
    pkEntry->m_uiFlags = uiFlags;

    uint32_t uiExtra = auiExtra;
    if ((uiDataSize & 3) == 0 && uiDataStride == 4) {
        NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType[uiFlags & 0xFF];
        if (eType != NiShaderAttributeDesc::ATTRIB_TYPE_BOOL &&
            eType != NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT) {
            if (m_eProgramType == NiGPUProgram::PROGRAM_VERTEX) {
                uiExtra = 1;
            }
            else if (m_eProgramType == NiGPUProgram::PROGRAM_PIXEL) {
                uiExtra = 2;
            }
        }
    }
    pkEntry->m_uiExtra = uiExtra;
    pkEntry->m_uiRegisterCount = uiRegisterCount;
    pkEntry->m_uiShaderRegister = uiShaderRegister;

    pkEntry->m_kVariableName = NiFixedString(pszVariableName);

    pkEntry->m_uiDataStride = uiDataStride;
    pkEntry->m_uiDataSize = uiDataSize;
    if (bCopyData) {
        pkEntry->m_bOwnData = true;
        _NiFree(pkEntry->m_pvDataSource);
        pkEntry->m_pvDataSource = _NiMalloc(uiDataSize);
        rt::memcpy(pkEntry->m_pvDataSource, pvDataSource, uiDataSize);
    }
    else {
        pkEntry->m_bOwnData = false;
        pkEntry->m_pvDataSource = const_cast<void*>(pvDataSource);
    }

    m_eLastError = InsertEntry(pkEntry);
    if (m_eLastError != 0) {
        delete pkEntry;
    }
    return m_eLastError;
}

// A global (0x40000000) entry; once the map holds it, the shader factory registers the global constant under its key
// with the attribute type its flags name. An entry the map refuses is deleted.
uint32_t NiD3DShaderConstantMap::AddGlobalEntry(
    const char* pszKey,
    uint32_t auiFlags,
    uint32_t uiExtra,
    uint32_t uiShaderRegister,
    uint32_t uiRegisterCount,
    const char* pszVariableName,
    uint32_t uiDataSize,
    uint32_t uiDataStride,
    const void* pvDataSource,
    bool bCopyData
) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiShaderConstantMapEntry));
    NiShaderConstantMapEntry* pkEntry = pvMemory ? ::new (pvMemory) NiShaderConstantMapEntry : nullptr;

    pkEntry->m_kKey = NiFixedString(pszKey);
    uint32_t uiFlags = (auiFlags & 0x0FFFFFFF) | 0x40000000;
    pkEntry->m_uiRegisterCount = uiRegisterCount;
    pkEntry->m_uiFlags = uiFlags;
    pkEntry->m_uiExtra = uiExtra;
    pkEntry->m_uiShaderRegister = uiShaderRegister;

    pkEntry->m_kVariableName = NiFixedString(pszVariableName);

    pkEntry->m_uiDataSize = uiDataSize;
    pkEntry->m_uiDataStride = uiDataStride;
    if (bCopyData) {
        pkEntry->m_bOwnData = true;
        _NiFree(pkEntry->m_pvDataSource);
        pkEntry->m_pvDataSource = _NiMalloc(uiDataSize);
        rt::memcpy(pkEntry->m_pvDataSource, pvDataSource, uiDataSize);
    }
    else {
        pkEntry->m_bOwnData = false;
        pkEntry->m_pvDataSource = const_cast<void*>(pvDataSource);
    }

    m_eLastError = InsertEntry(pkEntry);
    if (m_eLastError != 0) {
        delete pkEntry;
        return m_eLastError;
    }

    NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType[uiFlags & 0xFF];
    NiShaderFactory::RegisterGlobalShaderConstant(pszKey, eType, uiDataSize, pvDataSource);
    return m_eLastError;
}

// An operator (0x50000000) entry; one the map refuses is deleted.
uint32_t NiD3DShaderConstantMap::AddOperatorEntry(
    const char* pszKey,
    uint32_t uiFlags,
    uint32_t uiExtra,
    uint32_t uiShaderRegister,
    uint32_t uiRegisterCount,
    const char* pszVariableName
) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiShaderConstantMapEntry));
    NiShaderConstantMapEntry* pkEntry = pvMemory ? ::new (pvMemory) NiShaderConstantMapEntry : nullptr;

    pkEntry->m_kKey = NiFixedString(pszKey);
    pkEntry->m_uiShaderRegister = uiShaderRegister;
    pkEntry->m_uiFlags = (uiFlags & 0x0FFFFFFF) | 0x50000000;
    pkEntry->m_uiExtra = uiExtra;
    pkEntry->m_uiRegisterCount = uiRegisterCount;

    pkEntry->m_kVariableName = NiFixedString(pszVariableName);

    m_eLastError = InsertEntry(pkEntry);
    if (m_eLastError != 0) {
        delete pkEntry;
    }
    return m_eLastError;
}

// An object (0x60000000) entry: the object type's flags go in bits 8-15, the object index in the extra value. It is set
// up from its key's object mapping, then inserted; an entry that fails either step is deleted.
uint32_t NiD3DShaderConstantMap::AddObjectEntry(
    const char* pszKey,
    uint32_t uiShaderRegister,
    const char* pszVariableName,
    uint32_t uiObjectIndex,
    NiShaderAttributeDesc::ObjectType eObjectType
) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiShaderConstantMapEntry));
    NiShaderConstantMapEntry* pkEntry = pvMemory ? ::new (pvMemory) NiShaderConstantMapEntry : nullptr;

    pkEntry->m_kKey = NiFixedString(pszKey);
    pkEntry->m_uiShaderRegister = uiShaderRegister;

    pkEntry->m_kVariableName = NiFixedString(pszVariableName);

    pkEntry->m_uiExtra = uiObjectIndex;
    pkEntry->m_uiFlags = NiShaderConstantMapEntry::ms_ausObjectFlags[eObjectType] | 0x60000000;

    m_eLastError = SetupObjectEntry(pkEntry);
    if (m_eLastError == 0) {
        m_eLastError = InsertEntry(pkEntry);
        return m_eLastError;
    }
    delete pkEntry;
    return m_eLastError;
}

// Sort both entry arrays by their enabled flags.
void NiD3DShaderConstantMap::SortByEnabled() {
    SortListByEnabled(&m_aspEntries);
    SortListByEnabled(&m_aspDefinedEntries);
}

// A predefined (0x20000000) entry: set up from its key's mapping, then kept with the defined entries; an entry that
// fails either step is deleted.
uint32_t NiD3DShaderConstantMap::AddPredefinedEntry(
    const char* pszKey,
    uint32_t uiExtra,
    uint32_t uiShaderRegister,
    const char* pszVariableName,
    uint32_t uiDataSize,
    uint32_t uiDataStride,
    const void* pvDataSource,
    bool bCopyData
) {
    void* pvMemory = NiMemObject::operator new(sizeof(NiShaderConstantMapEntry));
    NiShaderConstantMapEntry* pkEntry = pvMemory ? ::new (pvMemory) NiShaderConstantMapEntry : nullptr;

    pkEntry->m_kKey = NiFixedString(pszKey);
    pkEntry->m_uiExtra = uiExtra;
    pkEntry->m_uiShaderRegister = uiShaderRegister;

    pkEntry->m_kVariableName = NiFixedString(pszVariableName);

    pkEntry->m_uiDataSize = uiDataSize;
    pkEntry->m_uiDataStride = uiDataStride;
    if (bCopyData) {
        pkEntry->m_bOwnData = true;
        _NiFree(pkEntry->m_pvDataSource);
        pkEntry->m_pvDataSource = _NiMalloc(uiDataSize);
        rt::memcpy(pkEntry->m_pvDataSource, pvDataSource, uiDataSize);
    }
    else {
        pkEntry->m_bOwnData = false;
        // The source is passed as const; the entry keeps it in its non-const m_pvDataSource.
        pkEntry->m_pvDataSource = const_cast<void*>(pvDataSource);
    }
    pkEntry->m_uiFlags = 0x20000000;

    m_eLastError = SetupPredefinedEntry(pkEntry);
    if (m_eLastError == 0) {
        m_eLastError = InsertDefinedEntry(pkEntry);
        return m_eLastError;
    }
    delete pkEntry;
    return m_eLastError;
}

// Clears the entry with this key from the map (a global entry also drops its factory constant); 0x10 when no entry has
// the key.
uint32_t NiD3DShaderConstantMap::RemoveEntry(const char* pszKey) {
    int32_t iIndex = static_cast<int32_t>(GetEntryIndex(pszKey));
    if (iIndex == -1) {
        m_eLastError = 0x10;
        return m_eLastError;
    }

    NiShaderConstantMapEntry* pkEntry = m_aspEntries.m_pBase[iIndex];
    if (pkEntry && (pkEntry->m_uiFlags & 0xF0000000) == 0x40000000) {
        NiShaderFactory::ReleaseGlobalShaderConstant(NiFixedString(pkEntry->m_kKey));
    }
    ShaderConstantMapEntryArray_SetAt(&m_aspEntries, iIndex, NiPointer<NiShaderConstantMapEntry>(nullptr));
    return m_eLastError;
}

// The entry with this key (case-insensitive), searching the defined entries first; null when none has it.
NiShaderConstantMapEntry* NiD3DShaderConstantMap::GetEntry(const char* pszKey) {
    uint32_t uiCount = m_aspDefinedEntries.m_usSize;
    for (uint32_t i = 0; i < uiCount; i++) {
        NiShaderConstantMapEntry* pkEntry = m_aspDefinedEntries.m_pBase[i];
        if (pkEntry && NiStricmp(pszKey, pkEntry->m_kKey) == 0) {
            return pkEntry;
        }
    }

    uiCount = m_aspEntries.m_usSize;
    for (uint32_t i = 0; i < uiCount; i++) {
        NiShaderConstantMapEntry* pkEntry = m_aspEntries.m_pBase[i];
        if (pkEntry && NiStricmp(pszKey, pkEntry->m_kKey) == 0) {
            return pkEntry;
        }
    }
    return nullptr;
}

// Sets every enabled entry that has a register or a variable name on the program: the defined entries first, then
// the others by mapping. Constant entries flagged for the vertex (extra 1) or pixel (extra 2) program go straight to
// the device. Returns 0 when every entry was set, else 1 (an unknown mapping also counts as a failure).
uint32_t NiD3DShaderConstantMap::SetShaderConstants(
    NiD3DShaderProgram* pkShaderProgram,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiSkinPartition::Partition* pkPartition,
    NiGeometryBufferData* pkBuffData,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass,
    bool bGlobal
) {
    m_pkLastShaderProgram = pkShaderProgram;
    bool bSuccess = true;

    uint32_t uiCount = m_aspDefinedEntries.m_usSize;
    for (uint32_t i = 0; i < uiCount; i++) {
        NiShaderConstantMapEntry* pkEntry = m_aspDefinedEntries.m_pBase[i];
        if (!pkEntry || !pkEntry->m_bEnabled) {
            continue;
        }
        if (pkEntry->m_uiShaderRegister == 0xFFFFFFFF && !pkEntry->m_kVariableName.Exists()) {
            continue;
        }
        if ((pkEntry->m_uiFlags & 0xF0000000) != 0x20000000) {
            continue;
        }
        if (SetDefinedConstant(
                pkShaderProgram, pkEntry, pkGeometry, pkSkin, pkPartition, pkBuffData, pkState, pkEffects, kWorld,
                kWorldBound, uiPass
            ) != 0) {
            bSuccess = false;
        }
    }

    uiCount = m_aspEntries.m_usSize;
    for (uint32_t i = 0; i < uiCount; i++) {
        NiShaderConstantMapEntry* pkEntry = m_aspEntries.m_pBase[i];
        if (!pkEntry || !pkEntry->m_bEnabled) {
            continue;
        }
        uint32_t uiRegister = pkEntry->m_uiShaderRegister;
        if (uiRegister == 0xFFFFFFFF && !pkEntry->m_kVariableName.Exists()) {
            continue;
        }

        uint32_t uiMapping = pkEntry->m_uiFlags & 0xF0000000;
        if (uiMapping == 0x20000000) {
            continue;
        }

        uint32_t eResult;
        if (uiMapping == 0x10000000) {
            if (pkEntry->m_uiExtra == 1) {
                eResult = m_pkD3DDevice->SetVertexShaderConstantF(
                              uiRegister, static_cast<const float*>(pkEntry->m_pvDataSource), pkEntry->m_uiRegisterCount
                          ) < 0;
            }
            else if (pkEntry->m_uiExtra == 2) {
                eResult = m_pkD3DDevice->SetPixelShaderConstantF(
                              uiRegister, static_cast<const float*>(pkEntry->m_pvDataSource), pkEntry->m_uiRegisterCount
                          ) < 0;
            }
            else {
                eResult = SetConstantConstant(pkShaderProgram, pkEntry, uiPass);
            }
        }
        else if (uiMapping == 0x30000000) {
            eResult = SetAttributeConstant(
                pkShaderProgram, pkEntry, pkGeometry, pkSkin, pkPartition, pkBuffData, pkState, pkEffects, kWorld,
                kWorldBound, uiPass, bGlobal, nullptr
            );
        }
        else if (uiMapping == 0x40000000) {
            eResult = SetGlobalConstant(pkShaderProgram, pkEntry, uiPass);
        }
        else if (uiMapping == 0x50000000) {
            eResult = SetOperatorConstant(
                pkShaderProgram, pkEntry, pkGeometry, pkSkin, pkState, pkEffects, kWorld, kWorldBound, uiPass
            );
        }
        else if (uiMapping == 0x60000000) {
            eResult = SetObjectConstant(
                pkShaderProgram, pkEntry, pkGeometry, pkSkin, pkPartition, pkBuffData, pkState, pkEffects, kWorld,
                kWorldBound, uiPass
            );
        }
        else {
            bSuccess = false;
            continue;
        }
        if (eResult != 0) {
            bSuccess = false;
        }
    }

    return bSuccess ? 0 : 1;
}

// The index of the (non-defined) entry with this key, case-insensitive; -1 (0xFFFFFFFF) when none has it.
uint32_t NiD3DShaderConstantMap::GetEntryIndex(const char* pszKey) {
    uint32_t uiCount = m_aspEntries.m_usSize;
    for (uint32_t i = 0; i < uiCount; i++) {
        NiShaderConstantMapEntry* pkEntry = m_aspEntries.m_pBase[i];
        if (pkEntry && NiStricmp(pszKey, pkEntry->m_kKey) == 0) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// Puts the entry in the first free slot of the entries; 0x30 when it is null.
uint32_t NiD3DShaderConstantMap::InsertEntry(NiShaderConstantMapEntry* pkEntry) {
    bool bFailed =
        ShaderConstantMapEntryArray_AddFirstEmpty(&m_aspEntries, NiPointer<NiShaderConstantMapEntry>(pkEntry)) ==
        0xFFFFFFFF;
    if (bFailed) {
        m_eLastError = 0x30;
    }
    return m_eLastError;
}

// Puts the entry in the first free slot of the defined entries; 0x30 when it is null.
uint32_t NiD3DShaderConstantMap::InsertDefinedEntry(NiShaderConstantMapEntry* pkEntry) {
    bool bFailed =
        ShaderConstantMapEntryArray_AddFirstEmpty(&m_aspDefinedEntries, NiPointer<NiShaderConstantMapEntry>(pkEntry)) ==
        0xFFFFFFFF;
    if (bFailed) {
        m_eLastError = 0x30;
    }
    return m_eLastError;
}

// Resolves a defined entry's key to its predefined mapping: the mapping's attribute flags go in the low byte of the
// flags and the mapping ID in the internal value. Below SCM_DEF_MATRIXTYPE_COUNT the register count is set too
// (bone palettes: 3 or 4 per bone in the high word of the extra value; skinned matrices: 4). The Taylor constants
// point at their coefficients. 0x40 when the key names no mapping.
uint32_t NiD3DShaderConstantMap::SetupPredefinedEntry(NiShaderConstantMapEntry* pkEntry) {
    // uiMappingID starts as the entry pointer, so a lookup that stores nothing leaves the entry pointer as the ID, as
    // the game does.
    uint32_t uiMappingID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkEntry));
    uint32_t uiNumRegisters = 4;
    if (!NiShaderConstantMap::LookUpPredefinedMapping(pkEntry->m_kKey, uiMappingID, uiNumRegisters)) {
        return 0x40;
    }

    NiShaderAttributeDesc::AttributeType eType =
        NiShaderConstantMap::LookUpPredefinedMappingType(uiMappingID, uiNumRegisters);
    // Rewrites the whole flags word, keeping the upper 24 bits.
    uint32_t& ruiFlags = pkEntry->m_uiFlags;
    ruiFlags = (ruiFlags & 0xFFFFFF00) | NiShaderConstantMapEntry::ms_aucAttribFlags[eType];
    if (uiMappingID == SCM_DEF_INVALID) {
        return 0x40;
    }

    if (uiMappingID < SCM_DEF_MATRIXTYPE_COUNT) {
        // The bone count is the high word of the extra value.
        if (uiMappingID == SCM_DEF_BONE_MATRIX_3 || uiMappingID == SCM_DEF_SKINBONE_MATRIX_3) {
            pkEntry->m_uiRegisterCount = static_cast<uint16_t>(pkEntry->m_uiExtra >> 16) * 3;
        }
        else if (uiMappingID == SCM_DEF_BONE_MATRIX_4 || uiMappingID == SCM_DEF_SKINBONE_MATRIX_4) {
            pkEntry->m_uiRegisterCount = static_cast<uint16_t>(pkEntry->m_uiExtra >> 16) * 4;
        }
        else if (
            uiMappingID == SCM_DEF_SKINWORLDVIEW || uiMappingID == SCM_DEF_INVSKINWORLDVIEW ||
            uiMappingID == SCM_DEF_SKINWORLDVIEW_T || uiMappingID == SCM_DEF_INVSKINWORLDVIEW_T ||
            uiMappingID == SCM_DEF_SKINWORLDVIEWPROJ || uiMappingID == SCM_DEF_INVSKINWORLDVIEWPROJ ||
            uiMappingID == SCM_DEF_SKINWORLDVIEWPROJ_T || uiMappingID == SCM_DEF_INVSKINWORLDVIEWPROJ_T ||
            uiMappingID == SCM_DEF_SKINWORLD || uiMappingID == SCM_DEF_INVSKINWORLD ||
            uiMappingID == SCM_DEF_SKINWORLD_T || uiMappingID == SCM_DEF_INVSKINWORLD_T
        ) {
            pkEntry->m_uiRegisterCount = 4;
        }
        else {
            pkEntry->m_uiRegisterCount = uiNumRegisters;
        }
    }
    pkEntry->m_uiInternal = uiMappingID;

    if (uiMappingID == SCM_DEF_CONSTS_TAYLOR_SIN) {
        pkEntry->m_pvDataSource = &ms_vTaylorSin;
    }
    else if (uiMappingID == SCM_DEF_CONSTS_TAYLOR_COS) {
        pkEntry->m_pvDataSource = &ms_vTaylorCos;
    }
    else {
        return 0;
    }
    pkEntry->m_bOwnData = false;
    pkEntry->m_uiDataStride = sizeof(D3DXVECTOR4);
    pkEntry->m_uiDataSize = sizeof(D3DXVECTOR4);
    return 0;
}

// KEEP: several mappings hand the program a buffer smaller than the register it sets, so the rest of the register
// comes from uninitialised stack: SCM_DEF_PARALLAX_OFFSET writes one float ([esp+0x24], 0xE84875; its address at
// 0xE8488E), SCM_DEF_BUMP_LUMA_OFFSET_AND_SCALE two (0xE848EA, 0xE848F0) and the SCM_DEF_TEXSIZE* entries two
// (0xE84994, 0xE84998), for a four-float register. The exe reads the time extra data's value even when
// GetExtraData returned null (0xE84700), as here. It calls __CItan (0xECA5D0) with the argument in ST0.
uint32_t NiD3DShaderConstantMap::SetDefinedConstant(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiSkinPartition::Partition* pkPartition,
    NiGeometryBufferData* pkBuffData,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass
) {
    alignas(16) D3DXMATRIX kResult;
    alignas(16) D3DXMATRIX kMat0;
    alignas(16) D3DXMATRIX kMat1;
    alignas(16) float afVector[4];
    float afPair[2];
    bool bInv;
    bool bTrans;
    bool bSet;
    const void* pvData;
    uint32_t uiFirst;
    uint32_t uiMap;
    NiTexturingProperty* pkTexProp;
    NiTexturingProperty::Map* pkMap;
    const NiMatrix3* pkTexMatrix;
    uint32_t uiInternal = pkEntry->m_uiInternal;
    uint32_t eResult = 0;
    switch (uiInternal) {
        case SCM_DEF_PROJ:
        case SCM_DEF_INVPROJ:
        case SCM_DEF_PROJ_T:
        case SCM_DEF_INVPROJ_T:
            bInv = uiInternal == SCM_DEF_INVPROJ || uiInternal == SCM_DEF_INVPROJ_T;
            bTrans = uiInternal == SCM_DEF_PROJ_T || uiInternal == SCM_DEF_INVPROJ_T;
            kResult = m_pkD3DRenderer->m_kD3DProj;
            goto InvertTranspose;
        case SCM_DEF_VIEW:
        case SCM_DEF_INVVIEW:
        case SCM_DEF_VIEW_T:
        case SCM_DEF_INVVIEW_T:
            bInv = uiInternal == SCM_DEF_INVVIEW || uiInternal == SCM_DEF_INVVIEW_T;
            bTrans = uiInternal == SCM_DEF_VIEW_T || uiInternal == SCM_DEF_INVVIEW_T;
            kResult = m_pkD3DRenderer->m_kD3DView;
            goto InvertTranspose;
        case SCM_DEF_WORLD:
        case SCM_DEF_INVWORLD:
        case SCM_DEF_WORLD_T:
        case SCM_DEF_INVWORLD_T:
            bInv = uiInternal == SCM_DEF_INVWORLD || uiInternal == SCM_DEF_INVWORLD_T;
            bTrans = uiInternal == SCM_DEF_WORLD_T || uiInternal == SCM_DEF_INVWORLD_T;
            kResult = m_pkD3DRenderer->m_kD3DMat;
            goto InvertTranspose;
        case SCM_DEF_WORLDVIEW:
        case SCM_DEF_INVWORLDVIEW:
        case SCM_DEF_WORLDVIEW_T:
        case SCM_DEF_INVWORLDVIEW_T:
            bInv = uiInternal == SCM_DEF_INVWORLDVIEW || uiInternal == SCM_DEF_INVWORLDVIEW_T;
            bTrans = uiInternal == SCM_DEF_WORLDVIEW_T || uiInternal == SCM_DEF_INVWORLDVIEW_T;
            kMat0 = m_pkD3DRenderer->m_kD3DMat;
            kMat1 = m_pkD3DRenderer->m_kD3DView;
            D3DXMatrixMultiply_Thunk(&kResult, &kMat0, &kMat1);
            goto InvertTranspose;
        case SCM_DEF_VIEWPROJ:
        case SCM_DEF_INVVIEWPROJ:
        case SCM_DEF_VIEWPROJ_T:
        case SCM_DEF_INVVIEWPROJ_T:
            bInv = uiInternal == SCM_DEF_INVVIEWPROJ || uiInternal == SCM_DEF_INVVIEWPROJ_T;
            bTrans = uiInternal == SCM_DEF_VIEWPROJ_T || uiInternal == SCM_DEF_INVVIEWPROJ_T;
            goto ViewProj;
        case SCM_DEF_WORLDVIEWPROJ:
        case SCM_DEF_INVWORLDVIEWPROJ:
        case SCM_DEF_WORLDVIEWPROJ_T:
        case SCM_DEF_INVWORLDVIEWPROJ_T:
            D3DXMatrixMultiplyTranspose_Thunk(&kMat0, &m_pkD3DRenderer->m_kD3DMat, &m_pkD3DRenderer->m_kViewProj);
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &kMat0, 0);
            goto Done;
        case SCM_DEF_SKINWORLDVIEW:
        case SCM_DEF_INVSKINWORLDVIEW:
        case SCM_DEF_SKINWORLDVIEW_T:
        case SCM_DEF_INVSKINWORLDVIEW_T:
            // The skin's bone matrices hold the world part.
            if (!pkSkin) {
                return eResult;
            }
            bInv = uiInternal == SCM_DEF_INVSKINWORLDVIEW || uiInternal == SCM_DEF_INVSKINWORLDVIEW_T;
            bTrans = uiInternal == SCM_DEF_SKINWORLDVIEW_T || uiInternal == SCM_DEF_INVSKINWORLDVIEW_T;
            kResult = m_pkD3DRenderer->m_kD3DView;
            goto InvertTranspose;
        case SCM_DEF_SKINWORLDVIEWPROJ:
        case SCM_DEF_INVSKINWORLDVIEWPROJ:
        case SCM_DEF_SKINWORLDVIEWPROJ_T:
        case SCM_DEF_INVSKINWORLDVIEWPROJ_T:
            if (!pkSkin) {
                return eResult;
            }
            bInv = uiInternal == SCM_DEF_INVSKINWORLDVIEWPROJ || uiInternal == SCM_DEF_INVSKINWORLDVIEWPROJ_T;
            bTrans = uiInternal == SCM_DEF_SKINWORLDVIEWPROJ_T || uiInternal == SCM_DEF_INVSKINWORLDVIEWPROJ_T;
        ViewProj:
            kMat1 = m_pkD3DRenderer->m_kD3DView;
            kMat0 = m_pkD3DRenderer->m_kD3DProj;
            D3DXMatrixMultiply_Thunk(&kResult, &kMat1, &kMat0);
            goto InvertTranspose;
        case SCM_DEF_SKINWORLD:
        case SCM_DEF_INVSKINWORLD:
        case SCM_DEF_SKINWORLD_T:
        case SCM_DEF_INVSKINWORLD_T:
            if (!pkSkin) {
                return eResult;
            }
            // The identity: the skin's bone matrices hold the world transform.
            kResult._12 = 0.0f;
            kResult._13 = 0.0f;
            kResult._14 = 0.0f;
            kResult._21 = 0.0f;
            kResult._23 = 0.0f;
            kResult._24 = 0.0f;
            kResult._31 = 0.0f;
            kResult._32 = 0.0f;
            kResult._34 = 0.0f;
            kResult._41 = 0.0f;
            kResult._42 = 0.0f;
            kResult._43 = 0.0f;
            kResult._11 = 1.0f;
            kResult._22 = 1.0f;
            kResult._33 = 1.0f;
            kResult._44 = 1.0f;
            bInv = uiInternal == SCM_DEF_INVSKINWORLD || uiInternal == SCM_DEF_INVSKINWORLD_T;
            bTrans = uiInternal == SCM_DEF_SKINWORLD_T || uiInternal == SCM_DEF_INVSKINWORLD_T;
        InvertTranspose:
            if (bInv) {
                D3DXMatrixInverse_Thunk(&kResult, nullptr, &kResult);
            }
            if (bTrans) {
                D3DXMatrixTranspose_Thunk(&kResult, &kResult);
            }
            pvData = &kResult;
            break;
        case SCM_DEF_BONE_MATRIX_3:
        case SCM_DEF_SKINBONE_MATRIX_3:
            if (!pkSkin || !pkPartition) {
                return eResult;
            }
            bSet = pkShaderProgram->SetShaderConstantArray(
                pkEntry, pkSkin->m_pvBoneMatrices, pkPartition->m_usBones, pkSkin->m_uiNumRegisters,
                pkPartition->m_pusBones
            );
            goto Done;
        case SCM_DEF_BONE_MATRIX_4:
        case SCM_DEF_SKINBONE_MATRIX_4:
            if (!pkSkin) {
                return eResult;
            }
            bSet = pkShaderProgram->SetShaderConstantArray(
                pkEntry, pkSkin->m_pvBoneMatrices, pkPartition->m_usBones, pkSkin->m_uiNumRegisters,
                pkPartition->m_pusBones
            );
            goto Done;
        case SCM_DEF_TEXTRANSFORMBASE:
        case SCM_DEF_INVTEXTRANSFORMBASE:
        case SCM_DEF_TEXTRANSFORMBASE_T:
        case SCM_DEF_INVTEXTRANSFORMBASE_T:
            uiFirst = SCM_DEF_TEXTRANSFORMBASE;
            uiMap = NiTexturingProperty::BASE_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMDARK:
        case SCM_DEF_INVTEXTRANSFORMDARK:
        case SCM_DEF_TEXTRANSFORMDARK_T:
        case SCM_DEF_INVTEXTRANSFORMDARK_T:
            uiFirst = SCM_DEF_TEXTRANSFORMDARK;
            uiMap = NiTexturingProperty::DARK_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMDETAIL:
        case SCM_DEF_INVTEXTRANSFORMDETAIL:
        case SCM_DEF_TEXTRANSFORMDETAIL_T:
        case SCM_DEF_INVTEXTRANSFORMDETAIL_T:
            uiFirst = SCM_DEF_TEXTRANSFORMDETAIL;
            uiMap = NiTexturingProperty::DETAIL_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMGLOSS:
        case SCM_DEF_INVTEXTRANSFORMGLOSS:
        case SCM_DEF_TEXTRANSFORMGLOSS_T:
        case SCM_DEF_INVTEXTRANSFORMGLOSS_T:
            uiFirst = SCM_DEF_TEXTRANSFORMGLOSS;
            uiMap = NiTexturingProperty::GLOSS_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMGLOW:
        case SCM_DEF_INVTEXTRANSFORMGLOW:
        case SCM_DEF_TEXTRANSFORMGLOW_T:
        case SCM_DEF_INVTEXTRANSFORMGLOW_T:
            uiFirst = SCM_DEF_TEXTRANSFORMGLOW;
            uiMap = NiTexturingProperty::GLOW_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMBUMP:
        case SCM_DEF_INVTEXTRANSFORMBUMP:
        case SCM_DEF_TEXTRANSFORMBUMP_T:
        case SCM_DEF_INVTEXTRANSFORMBUMP_T:
            uiFirst = SCM_DEF_TEXTRANSFORMBUMP;
            uiMap = NiTexturingProperty::BUMP_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMNORMAL:
        case SCM_DEF_INVTEXTRANSFORMNORMAL:
        case SCM_DEF_TEXTRANSFORMNORMAL_T:
        case SCM_DEF_INVTEXTRANSFORMNORMAL_T:
            uiFirst = SCM_DEF_TEXTRANSFORMNORMAL;
            uiMap = NiTexturingProperty::NORMAL_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMPARALLAX:
        case SCM_DEF_INVTEXTRANSFORMPARALLAX:
        case SCM_DEF_TEXTRANSFORMPARALLAX_T:
        case SCM_DEF_INVTEXTRANSFORMPARALLAX_T:
            uiFirst = SCM_DEF_TEXTRANSFORMPARALLAX;
            uiMap = NiTexturingProperty::PARALLAX_INDEX;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMDECAL:
        case SCM_DEF_INVTEXTRANSFORMDECAL:
        case SCM_DEF_TEXTRANSFORMDECAL_T:
        case SCM_DEF_INVTEXTRANSFORMDECAL_T:
            uiFirst = SCM_DEF_TEXTRANSFORMDECAL;
            uiMap = pkEntry->m_uiExtra + NiTexturingProperty::DECAL_BASE;
            goto MapByIndex;
        case SCM_DEF_TEXTRANSFORMSHADER:
        case SCM_DEF_INVTEXTRANSFORMSHADER:
        case SCM_DEF_TEXTRANSFORMSHADER_T:
        case SCM_DEF_INVTEXTRANSFORMSHADER_T:
            uiFirst = SCM_DEF_TEXTRANSFORMSHADER;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->GetShaderMap(pkEntry->m_uiExtra) : nullptr;
            goto MapTransform;
        MapByIndex:
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[uiMap] : nullptr;
        MapTransform:
            // The four entries of a map are its transform, inverse, transpose and inverse transpose.
            pkTexMatrix = nullptr;
            bInv = false;
            bTrans = false;
            if (pkMap && pkMap->m_pkTextureTransform) {
                pkTexMatrix = pkMap->m_pkTextureTransform->GetMatrix();
                bInv = uiInternal == uiFirst + 1 || uiInternal == uiFirst + 3;
                bTrans = uiInternal == uiFirst + 2 || uiInternal == uiFirst + 3;
            }
            SetupTextureTransformMatrix(kMat0, pkTexMatrix, bInv, bTrans, 4);
            pvData = &kMat0;
            break;
        case SCM_DEF_MATERIAL_DIFFUSE:
        case SCM_DEF_MATERIAL_AMBIENT: {
            NiMaterialProperty* pkMaterial =
                static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return eResult;
            }
            afVector[0] = NiColor_BLACK.r;
            afVector[1] = NiColor_BLACK.g;
            afVector[2] = NiColor_BLACK.b;
            afVector[3] = pkMaterial->m_fAlpha;
            pvData = afVector;
            break;
        }
        case SCM_DEF_MATERIAL_SPECULAR: {
            NiMaterialProperty* pkMaterial =
                static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return eResult;
            }
            afVector[0] = pkMaterial->m_spec.r;
            afVector[1] = pkMaterial->m_spec.g;
            afVector[2] = pkMaterial->m_spec.b;
            afVector[3] = pkMaterial->m_fAlpha;
            pvData = afVector;
            break;
        }
        case SCM_DEF_MATERIAL_EMISSIVE: {
            NiMaterialProperty* pkMaterial =
                static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return eResult;
            }
            const NiColor* pkEmit = pkMaterial->m_external_emit;
            if (!pkEmit) {
                pkEmit = &pkMaterial->m_emit;
            }
            afVector[0] = pkEmit->r;
            afVector[1] = pkEmit->g;
            afVector[2] = pkEmit->b;
            afVector[3] = pkMaterial->m_fAlpha;
            pvData = afVector;
            break;
        }
        case SCM_DEF_MATERIAL_POWER: {
            NiMaterialProperty* pkMaterial =
                static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return eResult;
            }
            afVector[0] = pkMaterial->m_fShine;
            afVector[1] = pkMaterial->m_fShine;
            afVector[2] = pkMaterial->m_fShine;
            afVector[3] = pkMaterial->m_fShine;
            pvData = afVector;
            break;
        }
        case SCM_DEF_EYE_POS:
            // The inverse view's translation row.
            kResult = m_pkD3DRenderer->m_kInvView;
            afVector[0] = kResult._41;
            afVector[1] = kResult._42;
            afVector[2] = kResult._43;
            afVector[3] = kResult._44;
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, afVector, 0);
            goto Done;
        case SCM_DEF_EYE_DIR:
            kResult = m_pkD3DRenderer->m_kInvView;
            afVector[0] = kResult._31;
            afVector[1] = kResult._32;
            afVector[2] = kResult._33;
            afVector[3] = kResult._34;
            pvData = afVector;
            break;
        case SCM_DEF_CONSTS_TAYLOR_SIN:
        case SCM_DEF_CONSTS_TAYLOR_COS: bSet = pkShaderProgram->SetShaderConstant(pkEntry, nullptr, 0); goto Done;
        case SCM_DEF_CONSTS_TIME:
        case SCM_DEF_CONSTS_SINTIME:
        case SCM_DEF_CONSTS_COSTIME:
        case SCM_DEF_CONSTS_TANTIME:
        case SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME: {
            NiFloatExtraData* pkFloatED;
            {
                NiFixedString kName(NiShaderConstantMap::ms_pcTimeExtraDataName);
                pkFloatED = static_cast<NiFloatExtraData*>(pkGeometry->GetExtraData(kName));
            }
            if (!pkFloatED) {
                eResult = 16;
            }
            float fTime = pkFloatED->m_fValue;
            switch (uiInternal) {
                case SCM_DEF_CONSTS_TIME:
                    afVector[0] = fTime;
                    afVector[1] = fTime;
                    afVector[2] = fTime;
                    afVector[3] = fTime;
                    break;
                case SCM_DEF_CONSTS_SINTIME: {
                    float fSin = std::sin(fTime);
                    afVector[0] = fSin;
                    afVector[1] = fSin;
                    afVector[2] = fSin;
                    afVector[3] = fSin;
                    break;
                }
                case SCM_DEF_CONSTS_COSTIME: {
                    float fCos = std::cos(fTime);
                    afVector[0] = fCos;
                    afVector[1] = fCos;
                    afVector[2] = fCos;
                    afVector[3] = fCos;
                    break;
                }
                case SCM_DEF_CONSTS_TANTIME: {
                    float fTan = std::tan(fTime);
                    afVector[0] = fTan;
                    afVector[1] = fTan;
                    afVector[2] = fTan;
                    afVector[3] = fTan;
                    break;
                }
                default:
                    afVector[0] = fTime;
                    afVector[1] = std::sin(fTime);
                    afVector[2] = std::cos(fTime);
                    afVector[3] = std::tan(fTime);
                    break;
            }
            pvData = afVector;
            break;
        }
        case SCM_DEF_AMBIENTLIGHT:
            // The sum of the ambient lights' colours, each times its dimmer.
            afVector[0] = 0.0f;
            afVector[1] = 0.0f;
            afVector[2] = 0.0f;
            afVector[3] = 1.0f;
            if (pkEffects) {
                NiDynamicEffectState::ListItem* pkItem = pkEffects->m_pLightList;
                while (pkItem) {
                    NiDynamicEffect* pkEffect = pkItem->m_pItem;
                    pkItem = pkItem->m_pNext;
                    if (pkEffect) {
                        NiAmbientLight* pkLight =
                            static_cast<NiAmbientLight*>(pkEffect->DynamicCastMember(&NiAmbientLight::ms_RTTI));
                        if (pkLight) {
                            NiColor kAmb = pkLight->m_kAmb * pkLight->m_fDimmer;
                            afVector[0] += kAmb.r;
                            afVector[1] += kAmb.g;
                            afVector[2] += kAmb.b;
                        }
                    }
                }
            }
            pvData = afVector;
            break;
        case SCM_DEF_BUMP_MATRIX: {
            afVector[0] = 1.0f;
            afVector[1] = 1.0f;
            afVector[2] = 1.0f;
            afVector[3] = 1.0f;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            if (pkTexProp) {
                NiTexturingProperty::BumpMap* pkBump = static_cast<NiTexturingProperty::BumpMap*>(
                    pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BUMP_INDEX]
                );
                if (pkBump) {
                    afVector[0] = pkBump->m_fBumpMat00;
                    afVector[1] = pkBump->m_fBumpMat01;
                    afVector[2] = pkBump->m_fBumpMat10;
                    afVector[3] = pkBump->m_fBumpMat11;
                }
            }
            pvData = afVector;
            break;
        }
        case SCM_DEF_BUMP_LUMA_OFFSET_AND_SCALE: {
            afPair[0] = 0.0f;
            afPair[1] = 1.0f;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            if (pkTexProp) {
                NiTexturingProperty::BumpMap* pkBump = static_cast<NiTexturingProperty::BumpMap*>(
                    pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BUMP_INDEX]
                );
                if (pkBump) {
                    afPair[0] = pkBump->m_fLumaOffset;
                    afPair[1] = pkBump->m_fLumaScale;
                }
            }
            pvData = afPair;
            break;
        }
        case SCM_DEF_PARALLAX_OFFSET: {
            afPair[0] = 0.0f;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            if (pkTexProp) {
                NiTexturingProperty::ParallaxMap* pkParallax = static_cast<NiTexturingProperty::ParallaxMap*>(
                    pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::PARALLAX_INDEX]
                );
                if (pkParallax) {
                    afPair[0] = pkParallax->m_fOffset;
                }
            }
            pvData = afPair;
            break;
        }
        case SCM_DEF_TEXSIZEBASE:
        case SCM_DEF_TEXSIZEDARK:
        case SCM_DEF_TEXSIZEDETAIL:
        case SCM_DEF_TEXSIZEGLOSS:
        case SCM_DEF_TEXSIZEGLOW:
        case SCM_DEF_TEXSIZEBUMP:
        case SCM_DEF_TEXSIZENORMAL:
        case SCM_DEF_TEXSIZEPARALLAX:
        case SCM_DEF_TEXSIZEDECAL:
        case SCM_DEF_TEXSIZESHADER: {
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            if (!pkTexProp) {
                return eResult;
            }
            if (uiInternal == SCM_DEF_TEXSIZESHADER) {
                pkMap = pkTexProp->GetShaderMap(pkEntry->m_uiExtra);
            }
            else if (uiInternal == SCM_DEF_TEXSIZEDECAL) {
                pkMap = pkTexProp->m_kMaps.m_pBase[pkEntry->m_uiExtra + NiTexturingProperty::DECAL_BASE];
            }
            else {
                // SCM_DEF_TEXSIZEBASE to SCM_DEF_TEXSIZEPARALLAX follow the MapEnum order.
                pkMap = pkTexProp->m_kMaps.m_pBase[uiInternal - SCM_DEF_TEXSIZEBASE];
            }
            afPair[0] = 0.0f;
            afPair[1] = 0.0f;
            if (pkMap) {
                NiTexture* pkTexture = pkMap->m_spTexture.m_pObject;
                if (pkTexture) {
                    afPair[0] = static_cast<float>(pkTexture->GetWidth());
                    afPair[1] = static_cast<float>(pkTexture->GetHeight());
                }
            }
            pvData = afPair;
            break;
        }
        default: return eResult;
    }
    bSet = pkShaderProgram->SetShaderConstant(pkEntry, pvData, 0);
Done:
    if (!bSet) {
        eResult = 80;
    }
    return eResult;
}

// A constant entry: its data (MapConstantValue) set on the program; 1 when the entry has none, 0x50 when the
// program refuses it.
uint32_t NiD3DShaderConstantMap::SetConstantConstant(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    uint32_t uiPass
) {
    const void* pvData = MapConstantValue(pkEntry);
    if (!pvData) {
        return 1;
    }
    return pkShaderProgram->SetShaderConstant(pkEntry, pvData, 0) ? 0 : 0x50;
}

// An attribute entry: the geometry's extra data named by the key (through the per-object SCM cache when one is given,
// which is refilled on a miss) supplies the value, or the entry's own data when the geometry has none. The value is set
// on the program with a register count by its attribute type (matrix4: 4, matrix3: 3, the vector and scalar types: the
// entry's own). Returns 1 without geometry, 0x10/0x40 when no value maps, 0x50 when the program refuses.
uint32_t NiD3DShaderConstantMap::SetAttributeConstant(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiSkinPartition::Partition* pkPartition,
    NiGeometryBufferData* pkBuffData,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass,
    bool bGlobal,
    NiExtraData* pkExtraData
) {
    if (!pkGeometry) {
        return 1;
    }

    NiExtraData* pkExtra;
    if (pkExtraData) {
        NiSCMExtraData* pkSCMExtraData = static_cast<NiSCMExtraData*>(pkExtraData);
        pkExtra = pkSCMExtraData->GetNextEntry(pkEntry->m_uiShaderRegister, uiPass, m_eProgramType, bGlobal);
        if (!pkExtra || !(pkExtra->GetName() == pkEntry->m_kKey)) {
            pkExtra = pkGeometry->GetExtraData(pkEntry->m_kKey);
            if (pkExtra) {
                pkSCMExtraData->AddEntry(pkEntry->m_uiShaderRegister, uiPass, m_eProgramType, pkExtra, bGlobal);
            }
        }
    }
    else {
        pkExtra = pkGeometry->GetExtraData(pkEntry->m_kKey);
    }

    const void* pvData;
    if (!pkExtra) {
        pvData = MapConstantValue(pkEntry);
        if (!pvData) {
            return 0x10;
        }
    }
    else {
        pvData = MapAttributeValue(pkEntry, pkExtra);
        if (!pvData) {
            return 0x40;
        }
    }

    switch (NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF]) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4:
            if (!pkShaderProgram->SetShaderConstant(pkEntry, pvData, 4)) {
                return 0x50;
            }
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3:
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2:
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
        case NiShaderAttributeDesc::ATTRIB_TYPE_BOOL:
            if (!pkShaderProgram->SetShaderConstant(pkEntry, pvData, 0)) {
                return 0x50;
            }
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3:
            if (!pkShaderProgram->SetShaderConstant(pkEntry, pvData, 3)) {
                return 0x50;
            }
            break;
        default: break;
    }
    return 0;
}

// A global entry: its data widened into the shared mapping scratch for its attribute type (a bool to a BOOL, an
// unsigned int and a float to all four components, a point2 to xyxy, a point3 to xyz1, a matrix3 to the 3x4 rows
// of a matrix) and set on the program; a matrix4 goes as is. Other types set nothing. 0x50 when the program
// refuses.
uint32_t NiD3DShaderConstantMap::SetGlobalConstant(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    uint32_t uiPass
) {
    uint32_t eResult = 0;
    bool bSet;
    switch (NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF]) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_BOOL:
            ms_bMappingValue = *static_cast<const uint8_t*>(pkEntry->m_pvDataSource);
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &ms_bMappingValue, 0);
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT: {
            int32_t iValue = *static_cast<const int32_t*>(pkEntry->m_pvDataSource);
            ms_aiMappingValue[0] = iValue;
            ms_aiMappingValue[1] = iValue;
            ms_aiMappingValue[2] = iValue;
            ms_aiMappingValue[3] = iValue;
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, ms_aiMappingValue, 0);
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_vMappingValue.x = pfData[0];
            ms_vMappingValue.y = ms_vMappingValue.x;
            ms_vMappingValue.z = ms_vMappingValue.x;
            ms_vMappingValue.w = ms_vMappingValue.x;
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &ms_vMappingValue, 0);
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_vMappingValue.x = pfData[0];
            ms_vMappingValue.y = pfData[1];
            ms_vMappingValue.z = ms_vMappingValue.x;
            ms_vMappingValue.w = ms_vMappingValue.y;
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &ms_vMappingValue, 0);
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_vMappingValue.x = pfData[0];
            ms_vMappingValue.y = pfData[1];
            ms_vMappingValue.z = pfData[2];
            ms_vMappingValue.w = 1.0f;
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &ms_vMappingValue, 0);
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_vMappingValue.x = pfData[0];
            ms_vMappingValue.y = pfData[1];
            ms_vMappingValue.z = pfData[2];
            ms_vMappingValue.w = pfData[3];
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &ms_vMappingValue, 0);
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_mMappingValue._11 = pfData[0];
            ms_mMappingValue._12 = pfData[1];
            ms_mMappingValue._13 = pfData[2];
            ms_mMappingValue._14 = 0.0f;
            ms_mMappingValue._21 = pfData[3];
            ms_mMappingValue._22 = pfData[4];
            ms_mMappingValue._23 = pfData[5];
            ms_mMappingValue._24 = 0.0f;
            ms_mMappingValue._31 = pfData[6];
            ms_mMappingValue._32 = pfData[7];
            ms_mMappingValue._33 = pfData[8];
            ms_mMappingValue._34 = 0.0f;
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, &ms_mMappingValue, 0);
            break;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4:
            bSet = pkShaderProgram->SetShaderConstant(pkEntry, pkEntry->m_pvDataSource, 0);
            break;
        default: return eResult;
    }
    if (!bSet) {
        eResult = 0x50;
    }
    return eResult;
}

// An operator entry: computes its two operand entries' values (indices in the extra value) into the operator
// scratch, then sets the operation's result on the program. The first operand's constant mapping is set up like an
// attribute, the second's like a global. Missing operand entries or untyped operands report an error and give 1, as
// does an unknown operation.
uint32_t NiD3DShaderConstantMap::SetOperatorConstant(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass
) {
    uint32_t uiExtra = pkEntry->m_uiExtra;
    bool bTranspose = (uiExtra & NiShaderConstantMapEntry::SCME_OPERATOR_RESULT_TRANSPOSE) != 0;
    uint32_t eOperation = uiExtra & NiShaderConstantMapEntry::SCME_OPERATOR_MASK;
    bool bInverse = (uiExtra & NiShaderConstantMapEntry::SCME_OPERATOR_RESULT_INVERSE) != 0;
    NiShaderConstantMapEntry* pkEntry1 = GetEntryAtIndex(uiExtra & NiShaderConstantMapEntry::SCME_OPERATOR_ENTRY1_MASK);
    NiShaderConstantMapEntry* pkEntry2 = GetEntryAtIndex(
        (uiExtra & NiShaderConstantMapEntry::SCME_OPERATOR_ENTRY2_MASK) >>
        NiShaderConstantMapEntry::SCME_OPERATOR_ENTRY2_SHIFT
    );
    if (!pkEntry1 || !pkEntry2) {
        NiShaderFactory::ReportError(1, false, "Invalid entries in OperatorConstant\n");
        return 1;
    }

    NiShaderAttributeDesc::AttributeType eType1 = NiShaderConstantMapEntry::ms_aeAttribType[pkEntry1->m_uiFlags & 0xFF];
    NiShaderAttributeDesc::AttributeType eType2 = NiShaderConstantMapEntry::ms_aeAttribType[pkEntry2->m_uiFlags & 0xFF];

    uint32_t uiMapping1 = pkEntry1->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK;
    if (uiMapping1 == NiShaderConstantMapEntry::SCME_MAP_DEFINED) {
        eType1 = NiShaderConstantMap::LookUpPredefinedMappingType(pkEntry1->m_kKey);
        SetupDefinedConstantValue(0, pkEntry1, pkGeometry, pkSkin, pkState, pkEffects, kWorld, kWorldBound);
    }
    else if (uiMapping1 == NiShaderConstantMapEntry::SCME_MAP_GLOBAL) {
        SetupConstantConstantValue(0, pkEntry1, uiPass);
    }
    else if (
        uiMapping1 == NiShaderConstantMapEntry::SCME_MAP_ATTRIBUTE ||
        uiMapping1 == NiShaderConstantMapEntry::SCME_MAP_CONSTANT
    ) {
        SetupAttributeConstantValue(0, pkEntry1, pkGeometry, pkSkin, pkState, kWorld, kWorldBound, uiPass);
    }

    uint32_t uiMapping2 = pkEntry2->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK;
    if (uiMapping2 == NiShaderConstantMapEntry::SCME_MAP_DEFINED) {
        eType2 = NiShaderConstantMap::LookUpPredefinedMappingType(pkEntry2->m_kKey);
        SetupDefinedConstantValue(1, pkEntry2, pkGeometry, pkSkin, pkState, pkEffects, kWorld, kWorldBound);
    }
    else if (uiMapping2 == NiShaderConstantMapEntry::SCME_MAP_ATTRIBUTE) {
        SetupAttributeConstantValue(1, pkEntry2, pkGeometry, pkSkin, pkState, kWorld, kWorldBound, uiPass);
    }
    else if (
        uiMapping2 == NiShaderConstantMapEntry::SCME_MAP_GLOBAL ||
        uiMapping2 == NiShaderConstantMapEntry::SCME_MAP_CONSTANT
    ) {
        SetupConstantConstantValue(1, pkEntry2, uiPass);
    }

    if (eType1 == NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED ||
        eType2 == NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED) {
        NiShaderFactory::ReportError(1, false, "Invalid operands in OperatorConstant\n");
        return 1;
    }

    switch (eOperation) {
        case NiShaderConstantMapEntry::SCME_OPERATOR_MULTIPLY:
            return PerformOperatorMultiply(pkShaderProgram, pkEntry, eType1, eType2, bInverse, bTranspose);
        case NiShaderConstantMapEntry::SCME_OPERATOR_DIVIDE:
            return PerformOperatorDivide(pkShaderProgram, pkEntry, eType1, eType2, bInverse, bTranspose);
        case NiShaderConstantMapEntry::SCME_OPERATOR_ADD:
            return PerformOperatorAdd(pkShaderProgram, pkEntry, eType1, eType2, bInverse, bTranspose);
        case NiShaderConstantMapEntry::SCME_OPERATOR_SUBTRACT:
            return PerformOperatorSubtract(pkShaderProgram, pkEntry, eType1, eType2, bInverse, bTranspose);
    }
    return 1;
}

// Resolves an object entry's key to its object mapping: the mapping's attribute flags go in the low byte of the
// flags, the mapping ID in the internal value and its register count in the entry. 0x40 when the key names none.
uint32_t NiD3DShaderConstantMap::SetupObjectEntry(NiShaderConstantMapEntry* pkEntry) {
    // As in SetupPredefinedEntry, the mapping ID lives in the apEntry argument's own stack slot.
    uint32_t uiMappingID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkEntry));
    uint32_t uiRegCount = 0;
    uint32_t uiFloatCount;
    if (NiShaderConstantMap::LookUpObjectMapping(pkEntry->m_kKey, uiMappingID)) {
        NiShaderAttributeDesc::AttributeType eType =
            NiShaderConstantMap::LookUpObjectMappingType(uiMappingID, uiRegCount, uiFloatCount);
        // Stores the whole 32-bit flags value, as in SetupPredefinedEntry.
        uint32_t& ruiFlags = pkEntry->m_uiFlags;
        ruiFlags = (ruiFlags & 0xFFFFFF00) | NiShaderConstantMapEntry::ms_aucAttribFlags[eType];
        if (uiMappingID) {
            pkEntry->m_uiInternal = uiMappingID;
            pkEntry->m_uiRegisterCount = uiRegCount;
            return 0;
        }
    }
    return 0x40;
}

// An object entry: the dynamic effect its object type and index name fills the shared object data (as many
// registers as the mapping takes), which is set on the program. 0x10 when the mapping has no type, 0x50 when the
// program refuses the data, else 0x60 when the effect gave no data.
uint32_t NiD3DShaderConstantMap::SetObjectConstant(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiSkinPartition::Partition* pkPartition,
    NiGeometryBufferData* pkBuffData,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass
) {
    uint32_t eResult = 0;
    NiDynamicEffect* pkDynEffect = NiShaderConstantMap::GetDynamicEffectForObject(
        pkEffects, NiShaderConstantMapEntry::ms_aeObjectType[(pkEntry->m_uiFlags >> 8) & 0xFF], pkEntry->m_uiExtra
    );
    uint32_t uiMapping = pkEntry->m_uiInternal;
    // The counts are seeded with the pkEntry and pkEffects pointer values, so a lookup that stores nothing leaves those
    // pointers as the counts, as the game does.
    uint32_t uiRegCount = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkEntry));
    uint32_t uiFloatCount = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkEffects));
    if (NiShaderConstantMap::LookUpObjectMappingType(uiMapping, uiRegCount, uiFloatCount) ==
        NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED) {
        return 0x10;
    }

    if (!GetDynamicEffectData(
            ms_afObjectData, uiRegCount * 16, static_cast<NiShaderConstantMap::ObjectMappings>(uiMapping), pkDynEffect,
            pkGeometry, pkSkin, pkPartition, pkBuffData, pkState, pkEffects, kWorld, kWorldBound, uiPass
        )) {
        eResult = 0x60;
    }
    if (!pkShaderProgram->SetShaderConstant(pkEntry, ms_afObjectData, 0)) {
        return 0x50;
    }
    return eResult;
}

// Moves the enabled entries ahead of the disabled ones: each enabled entry found after the first disabled one
// swaps places with it, and the search for the next disabled entry stops the sort when it runs off the end.
// Empty slots are skipped by the outer scan only; the search after a swap reads every entry it reaches.
void NiD3DShaderConstantMap::SortListByEnabled(NiTObjectArray<NiPointer<NiShaderConstantMapEntry>>* apEntries) {
    uint32_t uiSize = apEntries->m_usSize;
    NiPointer<NiShaderConstantMapEntry> spEntry;
    uint32_t uiFirstDisabled = 0xFFFFFFFF;
    if (uiSize <= 1) {
        return;
    }

    for (uint32_t i = 0; i < uiSize; i++) {
        spEntry = apEntries->m_pBase[i];
        if (!spEntry) {
            continue;
        }
        if (spEntry->m_bEnabled) {
            if (uiFirstDisabled < i) {
                ShaderConstantMapEntryArray_SetAt(apEntries, i, apEntries->m_pBase[uiFirstDisabled]);
                ShaderConstantMapEntryArray_SetAt(apEntries, uiFirstDisabled, spEntry);
                uiFirstDisabled++;
                while (uiFirstDisabled < uiSize) {
                    spEntry = apEntries->m_pBase[uiFirstDisabled];
                    if (!spEntry->m_bEnabled) {
                        break;
                    }
                    uiFirstDisabled++;
                }
                if (uiFirstDisabled >= uiSize) {
                    break;
                }
            }
        }
        else if (uiFirstDisabled > i) {
            uiFirstDisabled = i;
        }
    }
}

// Takes the renderer, its device (holding a reference) and its render state; a null renderer drops them.
void NiD3DShaderConstantMap::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
    m_pkD3DRenderer = pkD3DRenderer;
    if (pkD3DRenderer) {
        IDirect3DDevice9* pkDevice = pkD3DRenderer->m_pkD3DDevice9;
        if (m_pkD3DDevice) {
            m_pkD3DDevice->Release();
        }
        m_pkD3DDevice = pkDevice;
        if (pkDevice) {
            pkDevice->AddRef();
        }
        m_pkD3DRenderState = m_pkD3DRenderer->m_pkRenderState;
    }
    else {
        if (m_pkD3DDevice) {
            m_pkD3DDevice->Release();
        }
        m_pkD3DDevice = nullptr;
        m_pkD3DRenderState = nullptr;
    }
}

// The data a constant entry is set from, by its element count (data size over stride) and attribute type: one to
// three elements are widened into the shared mapping scratch (bools to a BOOL, unsigned ints to ints with a 1 in
// the last of three, floats to xxxx, xyxy or xyz1), 8, 9 and 12 floats fill a matrix's rows (9 with a zero fourth
// column) and zero the rest; 4 and 16 elements, and any multiple of four 4-byte elements, are used as they are.
// Null otherwise.
const void* NiD3DShaderConstantMap::MapConstantValue(NiShaderConstantMapEntry* pkEntry) {
    uint32_t uiCount = pkEntry->m_uiDataSize / pkEntry->m_uiDataStride;
    NiShaderAttributeDesc::AttributeType eType = NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF];
    switch (uiCount) {
        case 1:
            if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_BOOL) {
                ms_bMappingValue = *static_cast<const uint8_t*>(pkEntry->m_pvDataSource);
                return &ms_bMappingValue;
            }
            if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT) {
                int32_t iValue = *static_cast<const int32_t*>(pkEntry->m_pvDataSource);
                ms_aiMappingValue[0] = iValue;
                ms_aiMappingValue[1] = iValue;
                ms_aiMappingValue[2] = iValue;
                ms_aiMappingValue[3] = iValue;
                return ms_aiMappingValue;
            }
            ms_vMappingValue.x = *static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_vMappingValue.y = ms_vMappingValue.x;
            ms_vMappingValue.z = ms_vMappingValue.x;
            ms_vMappingValue.w = ms_vMappingValue.x;
            return &ms_vMappingValue;
        case 2:
            if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT) {
                const int32_t* piData = static_cast<const int32_t*>(pkEntry->m_pvDataSource);
                int32_t iX = piData[0];
                ms_aiMappingValue[0] = iX;
                int32_t iY = piData[1];
                ms_aiMappingValue[2] = iX;
                ms_aiMappingValue[1] = iY;
                ms_aiMappingValue[3] = iY;
                return ms_aiMappingValue;
            }
            else {
                const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
                ms_vMappingValue.x = pfData[0];
                ms_vMappingValue.y = pfData[1];
                ms_vMappingValue.z = ms_vMappingValue.x;
                ms_vMappingValue.w = ms_vMappingValue.y;
                return &ms_vMappingValue;
            }
        case 3:
            if (eType == NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT) {
                const int32_t* piData = static_cast<const int32_t*>(pkEntry->m_pvDataSource);
                ms_aiMappingValue[0] = piData[0];
                ms_aiMappingValue[1] = piData[1];
                ms_aiMappingValue[2] = piData[2];
                ms_aiMappingValue[3] = 1;
                return ms_aiMappingValue;
            }
            else {
                const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
                ms_vMappingValue.x = pfData[0];
                ms_vMappingValue.y = pfData[1];
                ms_vMappingValue.z = pfData[2];
                ms_vMappingValue.w = 1.0f;
                return &ms_vMappingValue;
            }
        case 4:
        case 16: return pkEntry->m_pvDataSource;
        case 8: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_mMappingValue._11 = pfData[0];
            ms_mMappingValue._12 = pfData[1];
            ms_mMappingValue._13 = pfData[2];
            ms_mMappingValue._14 = pfData[3];
            ms_mMappingValue._21 = pfData[4];
            ms_mMappingValue._22 = pfData[5];
            ms_mMappingValue._23 = pfData[6];
            ms_mMappingValue._24 = pfData[7];
            ms_mMappingValue._31 = 0.0f;
            ms_mMappingValue._32 = 0.0f;
            ms_mMappingValue._33 = 0.0f;
            ms_mMappingValue._34 = 0.0f;
            ms_mMappingValue._41 = 0.0f;
            ms_mMappingValue._42 = 0.0f;
            ms_mMappingValue._43 = 0.0f;
            ms_mMappingValue._44 = 0.0f;
            return &ms_mMappingValue;
        }
        case 9: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_mMappingValue._11 = pfData[0];
            ms_mMappingValue._12 = pfData[1];
            ms_mMappingValue._13 = pfData[2];
            ms_mMappingValue._14 = 0.0f;
            ms_mMappingValue._21 = pfData[3];
            ms_mMappingValue._22 = pfData[4];
            ms_mMappingValue._23 = pfData[5];
            ms_mMappingValue._24 = 0.0f;
            ms_mMappingValue._31 = pfData[6];
            ms_mMappingValue._32 = pfData[7];
            ms_mMappingValue._33 = pfData[8];
            ms_mMappingValue._34 = 0.0f;
            ms_mMappingValue._41 = 0.0f;
            ms_mMappingValue._42 = 0.0f;
            ms_mMappingValue._43 = 0.0f;
            ms_mMappingValue._44 = 0.0f;
            return &ms_mMappingValue;
        }
        case 12: {
            const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
            ms_mMappingValue._11 = pfData[0];
            ms_mMappingValue._12 = pfData[1];
            ms_mMappingValue._13 = pfData[2];
            ms_mMappingValue._14 = pfData[3];
            ms_mMappingValue._21 = pfData[4];
            ms_mMappingValue._22 = pfData[5];
            ms_mMappingValue._23 = pfData[6];
            ms_mMappingValue._24 = pfData[7];
            ms_mMappingValue._31 = pfData[8];
            ms_mMappingValue._32 = pfData[9];
            ms_mMappingValue._33 = pfData[10];
            ms_mMappingValue._34 = pfData[11];
            ms_mMappingValue._41 = 0.0f;
            ms_mMappingValue._42 = 0.0f;
            ms_mMappingValue._43 = 0.0f;
            ms_mMappingValue._44 = 0.0f;
            return &ms_mMappingValue;
        }
        default:
            if (pkEntry->m_uiDataStride == 4 && (uiCount & 3) == 0) {
                return pkEntry->m_pvDataSource;
            }
            return nullptr;
    }
}

// The data an attribute entry is set from, read from its extra data by attribute type and widened into the shared
// mapping scratch like SetGlobalConstant does (a point4 takes a floats or a colour extra data, matched exactly, and
// is left as it was for any other class); a matrix4 is the extra data's own floats. Null for other types.
const void* NiD3DShaderConstantMap::MapAttributeValue(NiShaderConstantMapEntry* pkEntry, NiExtraData* pkExtra) {
    switch (NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF]) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_BOOL:
            ms_bMappingValue = static_cast<NiBooleanExtraData*>(pkExtra)->m_bValue;
            return &ms_bMappingValue;
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT: {
            int32_t iValue = static_cast<NiIntegerExtraData*>(pkExtra)->m_iValue;
            ms_aiMappingValue[0] = iValue;
            ms_aiMappingValue[1] = iValue;
            ms_aiMappingValue[2] = iValue;
            ms_aiMappingValue[3] = iValue;
            return ms_aiMappingValue;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
            ms_vMappingValue.x = static_cast<NiFloatExtraData*>(pkExtra)->m_fValue;
            ms_vMappingValue.y = ms_vMappingValue.x;
            ms_vMappingValue.z = ms_vMappingValue.x;
            ms_vMappingValue.w = ms_vMappingValue.x;
            return &ms_vMappingValue;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2: {
            const float* pfData = static_cast<NiFloatsExtraData*>(pkExtra)->m_pfValue;
            ms_vMappingValue.x = pfData[0];
            ms_vMappingValue.y = pfData[1];
            ms_vMappingValue.z = pfData[0];
            ms_vMappingValue.w = pfData[1];
            return &ms_vMappingValue;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3: {
            const float* pfData = static_cast<NiFloatsExtraData*>(pkExtra)->m_pfValue;
            ms_vMappingValue.x = pfData[0];
            ms_vMappingValue.y = pfData[1];
            ms_vMappingValue.z = pfData[2];
            ms_vMappingValue.w = 1.0f;
            return &ms_vMappingValue;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
            if (NiObject::IsExactKindOf(&NiFloatsExtraData::ms_RTTI, pkExtra)) {
                const float* pfData = static_cast<NiFloatsExtraData*>(pkExtra)->m_pfValue;
                ms_vMappingValue.x = pfData[0];
                ms_vMappingValue.y = pfData[1];
                ms_vMappingValue.z = pfData[2];
                ms_vMappingValue.w = pfData[3];
            }
            else if (NiObject::IsExactKindOf(&NiColorExtraData::ms_RTTI, pkExtra)) {
                const NiColorA& kColor = static_cast<NiColorExtraData*>(pkExtra)->m_kColorA;
                ms_vMappingValue.x = kColor.r;
                ms_vMappingValue.y = kColor.g;
                ms_vMappingValue.z = kColor.b;
                ms_vMappingValue.w = kColor.a;
            }
            return &ms_vMappingValue;
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3: {
            const float* pfData = static_cast<NiFloatsExtraData*>(pkExtra)->m_pfValue;
            ms_mMappingValue._11 = pfData[0];
            ms_mMappingValue._12 = pfData[1];
            ms_mMappingValue._13 = pfData[2];
            ms_mMappingValue._14 = 0.0f;
            ms_mMappingValue._21 = pfData[3];
            ms_mMappingValue._22 = pfData[4];
            ms_mMappingValue._23 = pfData[5];
            ms_mMappingValue._24 = 0.0f;
            ms_mMappingValue._31 = pfData[6];
            ms_mMappingValue._32 = pfData[7];
            ms_mMappingValue._33 = pfData[8];
            ms_mMappingValue._34 = 0.0f;
            return &ms_mMappingValue;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4: return static_cast<NiFloatsExtraData*>(pkExtra)->m_pfValue;
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR: {
            const NiColorA& kColor = static_cast<NiColorExtraData*>(pkExtra)->m_kColorA;
            ms_vMappingValue.x = kColor.r;
            ms_vMappingValue.y = kColor.g;
            ms_vMappingValue.z = kColor.b;
            ms_vMappingValue.w = kColor.a;
            return &ms_vMappingValue;
        }
        default: return nullptr;
    }
}

// Operand auiOperatorNum of an operator entry from a predefined mapping, into ms_kD3DMat or ms_kD3DVector4. Returns
// the operand's attribute type, 0 for the mappings it does not take, NISHADERERR_ENTRYNOTFOUND without the time.
uint32_t NiD3DShaderConstantMap::SetupDefinedConstantValue(
    uint32_t uiOperatorNum,
    NiShaderConstantMapEntry* pkEntry,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound
) {
    alignas(16) D3DXMATRIX kMat0;
    alignas(16) D3DXMATRIX kMat1;
    alignas(16) D3DXMATRIX kMat2;
    alignas(16) D3DXMATRIX kTemp;
    alignas(16) D3DXMATRIX kResult;
    D3DXMATRIX* pkDest;
    bool bInv;
    bool bTrans;
    uint32_t uiFirst;
    NiTexturingProperty* pkTexProp;
    NiTexturingProperty::Map* pkMap;
    const NiMatrix3* pkTexMatrix;
    NiMaterialProperty* pkMaterial;
    uint32_t uiInternal = pkEntry->m_uiInternal;
    switch (uiInternal) {
        case SCM_DEF_PROJ:
        case SCM_DEF_INVPROJ:
        case SCM_DEF_PROJ_T:
        case SCM_DEF_INVPROJ_T:
            bInv = uiInternal == SCM_DEF_INVPROJ || uiInternal == SCM_DEF_INVPROJ_T;
            bTrans = uiInternal == SCM_DEF_PROJ_T || uiInternal == SCM_DEF_INVPROJ_T;
            kResult = m_pkD3DRenderer->m_kD3DProj;
            ms_kD3DMat[uiOperatorNum] = kResult;
            goto InvertTranspose;
        case SCM_DEF_VIEW:
        case SCM_DEF_INVVIEW:
        case SCM_DEF_VIEW_T:
        case SCM_DEF_INVVIEW_T:
            bInv = uiInternal == SCM_DEF_INVVIEW || uiInternal == SCM_DEF_INVVIEW_T;
            bTrans = uiInternal == SCM_DEF_VIEW_T || uiInternal == SCM_DEF_INVVIEW_T;
            kResult = m_pkD3DRenderer->m_kD3DView;
            ms_kD3DMat[uiOperatorNum] = kResult;
            goto InvertTranspose;
        case SCM_DEF_WORLD:
        case SCM_DEF_INVWORLD:
        case SCM_DEF_WORLD_T:
        case SCM_DEF_INVWORLD_T:
            bInv = uiInternal == SCM_DEF_INVWORLD || uiInternal == SCM_DEF_INVWORLD_T;
            bTrans = uiInternal == SCM_DEF_WORLD_T || uiInternal == SCM_DEF_INVWORLD_T;
            ms_kD3DMat[uiOperatorNum] = m_pkD3DRenderer->m_kD3DMat;
            goto InvertTranspose;
        case SCM_DEF_WORLDVIEW:
        case SCM_DEF_INVWORLDVIEW:
        case SCM_DEF_WORLDVIEW_T:
        case SCM_DEF_INVWORLDVIEW_T:
            bInv = uiInternal == SCM_DEF_INVWORLDVIEW || uiInternal == SCM_DEF_INVWORLDVIEW_T;
            bTrans = uiInternal == SCM_DEF_WORLDVIEW_T || uiInternal == SCM_DEF_INVWORLDVIEW_T;
            kMat0 = m_pkD3DRenderer->m_kD3DMat;
            kMat1 = m_pkD3DRenderer->m_kD3DView;
            D3DXMatrixMultiply_Thunk(&kResult, &kMat0, &kMat1);
            ms_kD3DMat[uiOperatorNum] = kResult;
            goto InvertTranspose;
        case SCM_DEF_VIEWPROJ:
        case SCM_DEF_INVVIEWPROJ:
        case SCM_DEF_VIEWPROJ_T:
        case SCM_DEF_INVVIEWPROJ_T:
            bInv = uiInternal == SCM_DEF_INVVIEWPROJ || uiInternal == SCM_DEF_INVVIEWPROJ_T;
            bTrans = uiInternal == SCM_DEF_VIEWPROJ_T || uiInternal == SCM_DEF_INVVIEWPROJ_T;
            kMat0 = m_pkD3DRenderer->m_kD3DView;
            kMat1 = m_pkD3DRenderer->m_kD3DProj;
            D3DXMatrixMultiply_Thunk(&kResult, &kMat0, &kMat1);
            ms_kD3DMat[uiOperatorNum] = kResult;
            goto InvertTranspose;
        case SCM_DEF_WORLDVIEWPROJ:
        case SCM_DEF_INVWORLDVIEWPROJ:
        case SCM_DEF_WORLDVIEWPROJ_T:
        case SCM_DEF_INVWORLDVIEWPROJ_T:
            bInv = uiInternal == SCM_DEF_INVWORLDVIEWPROJ || uiInternal == SCM_DEF_INVWORLDVIEWPROJ_T;
            bTrans = uiInternal == SCM_DEF_WORLDVIEWPROJ_T || uiInternal == SCM_DEF_INVWORLDVIEWPROJ_T;
            kMat0 = m_pkD3DRenderer->m_kD3DMat;
            kMat1 = m_pkD3DRenderer->m_kD3DView;
            kMat2 = m_pkD3DRenderer->m_kD3DProj;
            D3DXMatrixMultiply_Thunk(&kTemp, &kMat0, &kMat1);
            D3DXMatrixMultiply_Thunk(&kResult, &kTemp, &kMat2);
            ms_kD3DMat[uiOperatorNum] = kResult;
            goto InvertTranspose;
        case SCM_DEF_SKINWORLDVIEW:
        case SCM_DEF_INVSKINWORLDVIEW:
        case SCM_DEF_SKINWORLDVIEW_T:
        case SCM_DEF_INVSKINWORLDVIEW_T: uiFirst = SCM_DEF_SKINWORLDVIEW; goto SkinIdentity;
        case SCM_DEF_SKINWORLDVIEWPROJ:
        case SCM_DEF_INVSKINWORLDVIEWPROJ:
        case SCM_DEF_SKINWORLDVIEWPROJ_T:
        case SCM_DEF_INVSKINWORLDVIEWPROJ_T: uiFirst = SCM_DEF_SKINWORLDVIEWPROJ; goto SkinIdentity;
        case SCM_DEF_SKINWORLD:
        case SCM_DEF_INVSKINWORLD:
        case SCM_DEF_SKINWORLD_T:
        case SCM_DEF_INVSKINWORLD_T:
            uiFirst = SCM_DEF_SKINWORLD;
        SkinIdentity:
            // The skin's bone matrices hold the world transform: the world part is the identity. The four entries of a
            // group are the matrix, its inverse, transpose and inverse transpose.
            if (!pkSkin) {
                return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
            }
            pkDest = &ms_kD3DMat[uiOperatorNum];
            pkDest->_43 = 0.0f;
            pkDest->_42 = 0.0f;
            pkDest->_41 = 0.0f;
            pkDest->_34 = 0.0f;
            pkDest->_32 = 0.0f;
            pkDest->_31 = 0.0f;
            pkDest->_24 = 0.0f;
            pkDest->_23 = 0.0f;
            pkDest->_21 = 0.0f;
            pkDest->_14 = 0.0f;
            pkDest->_13 = 0.0f;
            pkDest->_12 = 0.0f;
            pkDest->_44 = 1.0f;
            pkDest->_33 = 1.0f;
            pkDest->_22 = 1.0f;
            pkDest->_11 = 1.0f;
            bInv = uiInternal == uiFirst + 1 || uiInternal == uiFirst + 3;
            bTrans = uiInternal == uiFirst + 2 || uiInternal == uiFirst + 3;
            if (uiFirst == SCM_DEF_SKINWORLDVIEW) {
                kMat0 = m_pkD3DRenderer->m_kD3DView;
                D3DXMatrixMultiply_Thunk(pkDest, pkDest, &kMat0);
            }
            else if (uiFirst == SCM_DEF_SKINWORLDVIEWPROJ) {
                kMat0 = m_pkD3DRenderer->m_kD3DView;
                kMat1 = m_pkD3DRenderer->m_kD3DProj;
                D3DXMatrixMultiply_Thunk(pkDest, pkDest, &kMat0);
                D3DXMatrixMultiply_Thunk(pkDest, pkDest, &kMat1);
            }
        InvertTranspose:
            if (bInv) {
                D3DXMatrixInverse_Thunk(&ms_kD3DMat[uiOperatorNum], nullptr, &ms_kD3DMat[uiOperatorNum]);
            }
            if (bTrans) {
                D3DXMatrixTranspose_Thunk(&ms_kD3DMat[uiOperatorNum], &ms_kD3DMat[uiOperatorNum]);
            }
            return NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4;
        case SCM_DEF_TEXTRANSFORMBASE:
        case SCM_DEF_INVTEXTRANSFORMBASE:
        case SCM_DEF_TEXTRANSFORMBASE_T:
        case SCM_DEF_INVTEXTRANSFORMBASE_T:
            uiFirst = SCM_DEF_TEXTRANSFORMBASE;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BASE_INDEX] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMDARK:
        case SCM_DEF_INVTEXTRANSFORMDARK:
        case SCM_DEF_TEXTRANSFORMDARK_T:
        case SCM_DEF_INVTEXTRANSFORMDARK_T:
            // The dark entries read the bump map (GetBumpMap), deliberately, as the game does.
            uiFirst = SCM_DEF_TEXTRANSFORMDARK;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BUMP_INDEX] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMDETAIL:
        case SCM_DEF_INVTEXTRANSFORMDETAIL:
        case SCM_DEF_TEXTRANSFORMDETAIL_T:
        case SCM_DEF_INVTEXTRANSFORMDETAIL_T:
            uiFirst = SCM_DEF_TEXTRANSFORMDETAIL;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::DETAIL_INDEX] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMGLOSS:
        case SCM_DEF_INVTEXTRANSFORMGLOSS:
        case SCM_DEF_TEXTRANSFORMGLOSS_T:
        case SCM_DEF_INVTEXTRANSFORMGLOSS_T:
            uiFirst = SCM_DEF_TEXTRANSFORMGLOSS;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::GLOSS_INDEX] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMGLOW:
        case SCM_DEF_INVTEXTRANSFORMGLOW:
        case SCM_DEF_TEXTRANSFORMGLOW_T:
        case SCM_DEF_INVTEXTRANSFORMGLOW_T:
            uiFirst = SCM_DEF_TEXTRANSFORMGLOW;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::GLOW_INDEX] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMBUMP:
        case SCM_DEF_INVTEXTRANSFORMBUMP:
        case SCM_DEF_TEXTRANSFORMBUMP_T:
        case SCM_DEF_INVTEXTRANSFORMBUMP_T:
            uiFirst = SCM_DEF_TEXTRANSFORMBUMP;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BUMP_INDEX] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMDECAL:
        case SCM_DEF_INVTEXTRANSFORMDECAL:
        case SCM_DEF_TEXTRANSFORMDECAL_T:
        case SCM_DEF_INVTEXTRANSFORMDECAL_T:
            uiFirst = SCM_DEF_TEXTRANSFORMDECAL;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap =
                pkTexProp ? pkTexProp->m_kMaps.m_pBase[pkEntry->m_uiExtra + NiTexturingProperty::DECAL_BASE] : nullptr;
            goto MapTransform;
        case SCM_DEF_TEXTRANSFORMSHADER:
        case SCM_DEF_INVTEXTRANSFORMSHADER:
        case SCM_DEF_TEXTRANSFORMSHADER_T:
        case SCM_DEF_INVTEXTRANSFORMSHADER_T:
            uiFirst = SCM_DEF_TEXTRANSFORMSHADER;
            pkTexProp = static_cast<NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
            pkMap = pkTexProp ? pkTexProp->GetShaderMap(pkEntry->m_uiExtra) : nullptr;
        MapTransform:
            // The four entries of a map are its transform, inverse, transpose and inverse transpose.
            pkTexMatrix = nullptr;
            bInv = false;
            bTrans = false;
            if (pkMap && pkMap->m_pkTextureTransform) {
                pkTexMatrix = pkMap->m_pkTextureTransform->GetMatrix();
                bInv = uiInternal == uiFirst + 1 || uiInternal == uiFirst + 3;
                bTrans = uiInternal == uiFirst + 2 || uiInternal == uiFirst + 3;
            }
            SetupTextureTransformMatrix(ms_kD3DMat[uiOperatorNum], pkTexMatrix, bInv, bTrans, 4);
            return NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4;
        case SCM_DEF_MATERIAL_DIFFUSE:
        case SCM_DEF_MATERIAL_AMBIENT:
            // The material has no diffuse or ambient colour: both read black, with the material's alpha.
            pkMaterial = static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
            }
            ms_kD3DVector4[uiOperatorNum].x = NiColor_BLACK.r;
            ms_kD3DVector4[uiOperatorNum].y = NiColor_BLACK.g;
            ms_kD3DVector4[uiOperatorNum].z = NiColor_BLACK.b;
            ms_kD3DVector4[uiOperatorNum].w = pkMaterial->m_fAlpha;
            return NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
        case SCM_DEF_MATERIAL_SPECULAR:
            pkMaterial = static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
            }
            ms_kD3DVector4[uiOperatorNum].x = pkMaterial->m_spec.r;
            ms_kD3DVector4[uiOperatorNum].y = pkMaterial->m_spec.g;
            ms_kD3DVector4[uiOperatorNum].z = pkMaterial->m_spec.b;
            ms_kD3DVector4[uiOperatorNum].w = pkMaterial->m_fAlpha;
            return NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
        case SCM_DEF_MATERIAL_EMISSIVE:
            // The external emissive colour when one is set, else the material's own; reread for each channel.
            pkMaterial = static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
            }
            ms_kD3DVector4[uiOperatorNum].x =
                (pkMaterial->m_external_emit ? pkMaterial->m_external_emit : &pkMaterial->m_emit)->r;
            ms_kD3DVector4[uiOperatorNum].y =
                (pkMaterial->m_external_emit ? pkMaterial->m_external_emit : &pkMaterial->m_emit)->g;
            ms_kD3DVector4[uiOperatorNum].z =
                (pkMaterial->m_external_emit ? pkMaterial->m_external_emit : &pkMaterial->m_emit)->b;
            ms_kD3DVector4[uiOperatorNum].w = pkMaterial->m_fAlpha;
            return NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
        case SCM_DEF_MATERIAL_POWER:
            pkMaterial = static_cast<NiMaterialProperty*>(pkState->m_aspProps[NiProperty::MATERIAL].m_pObject);
            if (!pkMaterial) {
                return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
            }
            ms_kD3DVector4[uiOperatorNum].x = pkMaterial->m_fShine;
            ms_kD3DVector4[uiOperatorNum].y = pkMaterial->m_fShine;
            ms_kD3DVector4[uiOperatorNum].z = pkMaterial->m_fShine;
            ms_kD3DVector4[uiOperatorNum].w = pkMaterial->m_fShine;
            return NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
        case SCM_DEF_EYE_POS:
            // The inverse view's translation row.
            kResult = m_pkD3DRenderer->m_kInvView;
            ms_kD3DVector4[uiOperatorNum].x = kResult._41;
            ms_kD3DVector4[uiOperatorNum].y = kResult._42;
            ms_kD3DVector4[uiOperatorNum].z = kResult._43;
            ms_kD3DVector4[uiOperatorNum].w = kResult._44;
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        case SCM_DEF_EYE_DIR:
            kResult = m_pkD3DRenderer->m_kInvView;
            ms_kD3DVector4[uiOperatorNum].x = kResult._31;
            ms_kD3DVector4[uiOperatorNum].y = kResult._32;
            ms_kD3DVector4[uiOperatorNum].z = kResult._33;
            ms_kD3DVector4[uiOperatorNum].w = kResult._34;
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        case SCM_DEF_CONSTS_TIME:
        case SCM_DEF_CONSTS_SINTIME:
        case SCM_DEF_CONSTS_COSTIME:
        case SCM_DEF_CONSTS_TANTIME:
        case SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME: {
            NiFloatExtraData* pkFloatED;
            {
                NiFixedString kName(NiShaderConstantMap::ms_pcTimeExtraDataName);
                pkFloatED = static_cast<NiFloatExtraData*>(pkGeometry->GetExtraData(kName));
            }
            if (!pkFloatED) {
                return NiShaderFactory::NISHADERERR_ENTRYNOTFOUND;
            }
            float fTime = pkFloatED->m_fValue;
            switch (uiInternal) {
                case SCM_DEF_CONSTS_TIME:
                    ms_kD3DVector4[uiOperatorNum].x = fTime;
                    ms_kD3DVector4[uiOperatorNum].y = fTime;
                    ms_kD3DVector4[uiOperatorNum].z = fTime;
                    ms_kD3DVector4[uiOperatorNum].w = fTime;
                    break;
                case SCM_DEF_CONSTS_SINTIME: {
                    float fSin = std::sin(fTime);
                    ms_kD3DVector4[uiOperatorNum].x = fSin;
                    ms_kD3DVector4[uiOperatorNum].y = fSin;
                    ms_kD3DVector4[uiOperatorNum].z = fSin;
                    ms_kD3DVector4[uiOperatorNum].w = fSin;
                    break;
                }
                case SCM_DEF_CONSTS_COSTIME: {
                    float fCos = std::cos(fTime);
                    ms_kD3DVector4[uiOperatorNum].x = fCos;
                    ms_kD3DVector4[uiOperatorNum].y = fCos;
                    ms_kD3DVector4[uiOperatorNum].z = fCos;
                    ms_kD3DVector4[uiOperatorNum].w = fCos;
                    break;
                }
                case SCM_DEF_CONSTS_TANTIME: {
                    float fTan = std::tan(fTime);
                    ms_kD3DVector4[uiOperatorNum].x = fTan;
                    ms_kD3DVector4[uiOperatorNum].y = fTan;
                    ms_kD3DVector4[uiOperatorNum].z = fTan;
                    ms_kD3DVector4[uiOperatorNum].w = fTan;
                    break;
                }
                case SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME:
                    ms_kD3DVector4[uiOperatorNum].x = fTime;
                    ms_kD3DVector4[uiOperatorNum].y = std::sin(fTime);
                    ms_kD3DVector4[uiOperatorNum].z = std::cos(fTime);
                    ms_kD3DVector4[uiOperatorNum].w = std::tan(fTime);
                    break;
                default: return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
            }
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        }
        case SCM_DEF_AMBIENTLIGHT:
            // The sum of the lit ambient lights' colours, each times its dimmer.
            ms_kD3DVector4[uiOperatorNum].x = 0.0f;
            ms_kD3DVector4[uiOperatorNum].y = 0.0f;
            ms_kD3DVector4[uiOperatorNum].z = 0.0f;
            ms_kD3DVector4[uiOperatorNum].w = 1.0f;
            if (pkEffects) {
                NiDynamicEffectState::ListItem* pkItem = pkEffects->m_pLightList;
                while (pkItem) {
                    NiDynamicEffect* pkEffect = pkItem->m_pItem;
                    pkItem = pkItem->m_pNext;
                    if (pkEffect) {
                        NiAmbientLight* pkLight =
                            static_cast<NiAmbientLight*>(pkEffect->DynamicCastMember(&NiAmbientLight::ms_RTTI));
                        if (pkLight && pkLight->m_bOn) {
                            NiColor kColor = pkLight->m_kAmb * pkLight->m_fDimmer;
                            ms_kD3DVector4[uiOperatorNum].x += kColor.r;
                            ms_kD3DVector4[uiOperatorNum].y += kColor.g;
                            ms_kD3DVector4[uiOperatorNum].z += kColor.b;
                        }
                    }
                }
            }
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        default: return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
    }
}

// Operand auiOperatorNum of an operator entry from a constant or global entry's data, by its attribute type: the
// vector types fill as many components of the operand vector as they have (an unsigned int is copied as a float),
// a matrix3 fills the operand matrix with a zero fourth column and an identity fourth row. Returns the type, or 0
// for the types it does not take.
uint32_t NiD3DShaderConstantMap::SetupConstantConstantValue(
    uint32_t uiOperatorNum,
    NiShaderConstantMapEntry* pkEntry,
    uint32_t uiPass
) {
    const float* pfData = static_cast<const float*>(pkEntry->m_pvDataSource);
    switch (NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF]) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            return NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT;
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            return NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT2;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            ms_kD3DVector4[uiOperatorNum].z = pfData[2];
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT3;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            ms_kD3DVector4[uiOperatorNum].z = pfData[2];
            ms_kD3DVector4[uiOperatorNum].w = pfData[3];
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3: {
            D3DXMATRIX& kMat = ms_kD3DMat[uiOperatorNum];
            kMat._11 = pfData[0];
            kMat._12 = pfData[1];
            kMat._13 = pfData[2];
            kMat._14 = 0.0f;
            kMat._21 = pfData[3];
            kMat._22 = pfData[4];
            kMat._23 = pfData[5];
            kMat._24 = 0.0f;
            kMat._31 = pfData[6];
            kMat._32 = pfData[7];
            kMat._33 = pfData[8];
            kMat._34 = 0.0f;
            kMat._41 = 0.0f;
            kMat._42 = 0.0f;
            kMat._43 = 0.0f;
            kMat._44 = 1.0f;
            return NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4: {
            D3DXMATRIX& kMat = ms_kD3DMat[uiOperatorNum];
            kMat._11 = pfData[0];
            kMat._12 = pfData[1];
            kMat._13 = pfData[2];
            kMat._14 = pfData[3];
            kMat._21 = pfData[4];
            kMat._22 = pfData[5];
            kMat._23 = pfData[6];
            kMat._24 = pfData[7];
            kMat._31 = pfData[8];
            kMat._32 = pfData[9];
            kMat._33 = pfData[10];
            kMat._34 = pfData[11];
            kMat._41 = pfData[12];
            kMat._42 = pfData[13];
            kMat._43 = pfData[14];
            kMat._44 = pfData[15];
            return NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            ms_kD3DVector4[uiOperatorNum].z = pfData[2];
            ms_kD3DVector4[uiOperatorNum].w = pfData[3];
            return NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
        default: return 0;
    }
}

// Operand auiOperatorNum of an operator entry from the geometry's extra data named by the key (MapAttributeValue,
// whose result is not checked), filled like SetupConstantConstantValue except that an unsigned int is converted to
// a float. 1 without geometry, 0x10 when the geometry has no such extra data.
uint32_t NiD3DShaderConstantMap::SetupAttributeConstantValue(
    uint32_t uiOperatorNum,
    NiShaderConstantMapEntry* pkEntry,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiPropertyState* pkState,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass
) {
    if (!pkGeometry) {
        return 1;
    }
    NiExtraData* pkExtra = pkGeometry->GetExtraData(pkEntry->m_kKey);
    if (!pkExtra) {
        return 0x10;
    }

    const void* pvData = MapAttributeValue(pkEntry, pkExtra);
    const float* pfData = static_cast<const float*>(pvData);
    switch (NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF]) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
            ms_kD3DVector4[uiOperatorNum].x = static_cast<float>(*static_cast<const uint32_t*>(pvData));
            return NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT;
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            return NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT2;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            ms_kD3DVector4[uiOperatorNum].z = pfData[2];
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT3;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            ms_kD3DVector4[uiOperatorNum].z = pfData[2];
            ms_kD3DVector4[uiOperatorNum].w = pfData[3];
            return NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3: {
            D3DXMATRIX& kMat = ms_kD3DMat[uiOperatorNum];
            kMat._11 = pfData[0];
            kMat._12 = pfData[1];
            kMat._13 = pfData[2];
            kMat._14 = 0.0f;
            kMat._21 = pfData[3];
            kMat._22 = pfData[4];
            kMat._23 = pfData[5];
            kMat._24 = 0.0f;
            kMat._31 = pfData[6];
            kMat._32 = pfData[7];
            kMat._33 = pfData[8];
            kMat._34 = 0.0f;
            kMat._41 = 0.0f;
            kMat._42 = 0.0f;
            kMat._43 = 0.0f;
            kMat._44 = 1.0f;
            return NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4: {
            D3DXMATRIX& kMat = ms_kD3DMat[uiOperatorNum];
            kMat._11 = pfData[0];
            kMat._12 = pfData[1];
            kMat._13 = pfData[2];
            kMat._14 = pfData[3];
            kMat._21 = pfData[4];
            kMat._22 = pfData[5];
            kMat._23 = pfData[6];
            kMat._24 = pfData[7];
            kMat._31 = pfData[8];
            kMat._32 = pfData[9];
            kMat._33 = pfData[10];
            kMat._34 = pfData[11];
            kMat._41 = pfData[12];
            kMat._42 = pfData[13];
            kMat._43 = pfData[14];
            kMat._44 = pfData[15];
            return NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4;
        }
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
            ms_kD3DVector4[uiOperatorNum].x = pfData[0];
            ms_kD3DVector4[uiOperatorNum].y = pfData[1];
            ms_kD3DVector4[uiOperatorNum].z = pfData[2];
            ms_kD3DVector4[uiOperatorNum].w = pfData[3];
            return NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
        default: return 0;
    }
}

// The product of the operands, set on the program (0x50 when it refuses): a matrix4 times a scalar (each element
// scaled by the second vector's x), a vector (transformed by the first matrix) or a matrix4; a vector times a scalar,
// a vector (per component) or a matrix4 (the first vector transformed by the second matrix). Only matrix results
// are inverted and transposed as asked. 1 for any other pair of types.
uint32_t NiD3DShaderConstantMap::PerformOperatorMultiply(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiShaderAttributeDesc::AttributeType eType1,
    NiShaderAttributeDesc::AttributeType eType2,
    bool bInverse,
    bool bTranspose
) {
    switch (eType1) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4:
            switch (eType2) {
                case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
                case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT: {
                    const D3DXMATRIX& kMat = ms_kD3DMat[0];
                    float fScalar = ms_kD3DVector4[1].x;
                    ms_kD3DMatResult._11 = kMat._11 * fScalar;
                    ms_kD3DMatResult._12 = kMat._12 * fScalar;
                    ms_kD3DMatResult._13 = kMat._13 * fScalar;
                    ms_kD3DMatResult._14 = kMat._14 * fScalar;
                    ms_kD3DMatResult._21 = kMat._21 * fScalar;
                    ms_kD3DMatResult._22 = kMat._22 * fScalar;
                    ms_kD3DMatResult._23 = kMat._23 * fScalar;
                    ms_kD3DMatResult._24 = kMat._24 * fScalar;
                    ms_kD3DMatResult._31 = kMat._31 * fScalar;
                    ms_kD3DMatResult._32 = kMat._32 * fScalar;
                    ms_kD3DMatResult._33 = kMat._33 * fScalar;
                    ms_kD3DMatResult._34 = kMat._34 * fScalar;
                    ms_kD3DMatResult._41 = kMat._41 * fScalar;
                    ms_kD3DMatResult._42 = kMat._42 * fScalar;
                    ms_kD3DMatResult._43 = kMat._43 * fScalar;
                    ms_kD3DMatResult._44 = kMat._44 * fScalar;
                    if (bInverse) {
                        D3DXMatrixInverse_Thunk(&ms_kD3DMatResult, nullptr, &ms_kD3DMatResult);
                    }
                    if (bTranspose) {
                        D3DXMatrixTranspose_Thunk(&ms_kD3DMatResult, &ms_kD3DMatResult);
                    }
                    return pkShaderProgram->SetShaderConstant(pkEntry, &ms_kD3DMatResult, 0) ? 0 : 0x50;
                }
                case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
                case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
                    D3DXVec4Transform_0(&ms_kD3DVector4Result, &ms_kD3DVector4[1], &ms_kD3DMat[0]);
                    return pkShaderProgram->SetShaderConstant(pkEntry, &ms_kD3DVector4Result, 0) ? 0 : 0x50;
                case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4:
                    D3DXMatrixMultiply_Thunk(&ms_kD3DMatResult, &ms_kD3DMat[0], &ms_kD3DMat[1]);
                    if (bInverse) {
                        D3DXMatrixInverse_Thunk(&ms_kD3DMatResult, nullptr, &ms_kD3DMatResult);
                    }
                    if (bTranspose) {
                        D3DXMatrixTranspose_Thunk(&ms_kD3DMatResult, &ms_kD3DMatResult);
                    }
                    return pkShaderProgram->SetShaderConstant(pkEntry, &ms_kD3DMatResult, 0) ? 0 : 0x50;
                default: return 1;
            }
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
            switch (eType2) {
                case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
                case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT: {
                    float fScalar = ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x * fScalar;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y * fScalar;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z * fScalar;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w * fScalar;
                    break;
                }
                case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
                case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x * ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y * ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z * ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w * ms_kD3DVector4[1].w;
                    break;
                case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4:
                    D3DXVec4Transform_0(&ms_kD3DVector4Result, &ms_kD3DVector4[0], &ms_kD3DMat[1]);
                    break;
                default: return 1;
            }
            return pkShaderProgram->SetShaderConstant(pkEntry, &ms_kD3DVector4Result, 0) ? 0 : 0x50;
        default: return 1;
    }
}

uint32_t NiD3DShaderConstantMap::PerformOperatorDivide(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiShaderAttributeDesc::AttributeType eType1,
    NiShaderAttributeDesc::AttributeType eType2,
    bool bInverse,
    bool bTranspose
) {
    void* v8;
    float v9[16];
    if (eType1 == NiShaderAttributeDesc::ATTRIB_TYPE_POINT4) {
        uint32_t result;
    LABEL_13:;
        switch (eType2) {
            case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
            case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT: {
                float abInversea = 1.0 / NiD3DShaderConstantMap::ms_kD3DVector4[1].x;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.x =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].x * abInversea;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.y =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].y * abInversea;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.z =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].z * abInversea;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.w =
                    abInversea * NiD3DShaderConstantMap::ms_kD3DVector4[0].w;
                result = pkShaderProgram->SetShaderConstant(pkEntry, &NiD3DShaderConstantMap::ms_kD3DVector4Result, 0)
                    ? 0
                    : 80;
                break;
            }
            case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
            case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR: {
                v8 = (void*)&NiD3DShaderConstantMap::ms_kD3DVector4Result;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.x =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].x / NiD3DShaderConstantMap::ms_kD3DVector4[1].x;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.y =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].y / NiD3DShaderConstantMap::ms_kD3DVector4[1].y;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.z =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].z / NiD3DShaderConstantMap::ms_kD3DVector4[1].z;
                NiD3DShaderConstantMap::ms_kD3DVector4Result.w =
                    NiD3DShaderConstantMap::ms_kD3DVector4[0].w / NiD3DShaderConstantMap::ms_kD3DVector4[1].w;
                return pkShaderProgram->SetShaderConstant(pkEntry, v8, 0) ? 0 : 80;
            }
            default: {
                return 1;
            }
        }
        return result;
    }
    if (eType1 != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) {
        if (eType1 != NiShaderAttributeDesc::ATTRIB_TYPE_COLOR) {
            return 1;
        }
        goto LABEL_13;
    }
    if (eType2 < NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT || eType2 > NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT) {
        return 1;
    }
    float aeType2a = 1.0 / NiD3DShaderConstantMap::ms_kD3DVector4[1].x;
    v9[0] = NiD3DShaderConstantMap::ms_kD3DMat[0]._11 * aeType2a;
    v9[1] = NiD3DShaderConstantMap::ms_kD3DMat[0]._12 * aeType2a;
    v9[2] = NiD3DShaderConstantMap::ms_kD3DMat[0]._13 * aeType2a;
    v9[3] = NiD3DShaderConstantMap::ms_kD3DMat[0]._14 * aeType2a;
    v9[4] = NiD3DShaderConstantMap::ms_kD3DMat[0]._21 * aeType2a;
    v9[5] = NiD3DShaderConstantMap::ms_kD3DMat[0]._22 * aeType2a;
    v9[6] = NiD3DShaderConstantMap::ms_kD3DMat[0]._23 * aeType2a;
    v9[7] = NiD3DShaderConstantMap::ms_kD3DMat[0]._24 * aeType2a;
    v9[8] = NiD3DShaderConstantMap::ms_kD3DMat[0]._31 * aeType2a;
    v9[9] = NiD3DShaderConstantMap::ms_kD3DMat[0]._32 * aeType2a;
    v9[10] = NiD3DShaderConstantMap::ms_kD3DMat[0]._33 * aeType2a;
    v9[11] = NiD3DShaderConstantMap::ms_kD3DMat[0]._34 * aeType2a;
    v9[12] = NiD3DShaderConstantMap::ms_kD3DMat[0]._41 * aeType2a;
    v9[13] = NiD3DShaderConstantMap::ms_kD3DMat[0]._42 * aeType2a;
    v9[14] = NiD3DShaderConstantMap::ms_kD3DMat[0]._43 * aeType2a;
    v9[15] = aeType2a * NiD3DShaderConstantMap::ms_kD3DMat[0]._44;
    rt::memcpy(&NiD3DShaderConstantMap::ms_kD3DMatResult, v9, sizeof(NiD3DShaderConstantMap::ms_kD3DMatResult));
    if (bInverse) {
        D3DXMatrixInverse_Thunk(
            &NiD3DShaderConstantMap::ms_kD3DMatResult, nullptr, &NiD3DShaderConstantMap::ms_kD3DMatResult
        );
    }
    if (bTranspose) {
        D3DXMatrixTranspose_Thunk(&NiD3DShaderConstantMap::ms_kD3DMatResult, &NiD3DShaderConstantMap::ms_kD3DMatResult);
    }
    v8 = (void*)&NiD3DShaderConstantMap::ms_kD3DMatResult;
    return pkShaderProgram->SetShaderConstant(pkEntry, v8, 0) ? 0 : 80;
}

// The sum of the operands as a vector, set on the program (0x50 when it refuses): scalar operands are added per
// component (two scalars as whole vectors, a scalar and a vector through the scalar operand's x). 1 for any other
// pair of types.
uint32_t NiD3DShaderConstantMap::PerformOperatorAdd(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiShaderAttributeDesc::AttributeType eType1,
    NiShaderAttributeDesc::AttributeType eType2,
    bool bInverse,
    bool bTranspose
) {
    switch (eType1) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
            switch (eType2) {
                case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
                case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x + ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y + ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z + ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w + ms_kD3DVector4[1].w;
                    break;
                case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
                case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR: {
                    float fScalar = ms_kD3DVector4[0].x;
                    ms_kD3DVector4Result.x = fScalar + ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = fScalar + ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = fScalar + ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = fScalar + ms_kD3DVector4[1].w;
                    break;
                }
                default: return 1;
            }
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
            switch (eType2) {
                case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
                case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT: {
                    float fScalar = ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x + fScalar;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y + fScalar;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z + fScalar;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w + fScalar;
                    break;
                }
                case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
                case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x + ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y + ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z + ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w + ms_kD3DVector4[1].w;
                    break;
                default: return 1;
            }
            break;
        default: return 1;
    }
    return pkShaderProgram->SetShaderConstant(pkEntry, &ms_kD3DVector4Result, 0) ? 0 : 0x50;
}

// The difference of the operands as a vector, set on the program (0x50 when it refuses); scalars as in
// PerformOperatorAdd. 1 for any other pair of types.
uint32_t NiD3DShaderConstantMap::PerformOperatorSubtract(
    NiD3DShaderProgram* pkShaderProgram,
    NiShaderConstantMapEntry* pkEntry,
    NiShaderAttributeDesc::AttributeType eType1,
    NiShaderAttributeDesc::AttributeType eType2,
    bool bInverse,
    bool bTranspose
) {
    switch (eType1) {
        case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
        case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
            switch (eType2) {
                case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
                case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x - ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y - ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z - ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w - ms_kD3DVector4[1].w;
                    break;
                case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
                case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR: {
                    float fScalar = ms_kD3DVector4[0].x;
                    ms_kD3DVector4Result.x = fScalar - ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = fScalar - ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = fScalar - ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = fScalar - ms_kD3DVector4[1].w;
                    break;
                }
                default: return 1;
            }
            break;
        case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
        case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
            switch (eType2) {
                case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
                case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT: {
                    float fScalar = ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x - fScalar;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y - fScalar;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z - fScalar;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w - fScalar;
                    break;
                }
                case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
                case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
                    ms_kD3DVector4Result.x = ms_kD3DVector4[0].x - ms_kD3DVector4[1].x;
                    ms_kD3DVector4Result.y = ms_kD3DVector4[0].y - ms_kD3DVector4[1].y;
                    ms_kD3DVector4Result.z = ms_kD3DVector4[0].z - ms_kD3DVector4[1].z;
                    ms_kD3DVector4Result.w = ms_kD3DVector4[0].w - ms_kD3DVector4[1].w;
                    break;
                default: return 1;
            }
            break;
        default: return 1;
    }
    return pkShaderProgram->SetShaderConstant(pkEntry, &ms_kD3DVector4Result, 0) ? 0 : 0x50;
}

// The Direct3D texture matrix of a texture transform (its 2D part, with the translation in the third row for
// two-component coordinates or the fourth for three or four; other sizes leave those rows' first two columns
// alone), inverted and transposed as asked. Without a transform: the identity.
void NiD3DShaderConstantMap::SetupTextureTransformMatrix(
    D3DXMATRIX& kResult,
    const NiMatrix3* pkTexMatrix,
    bool bInv,
    bool bTrans,
    uint32_t uiTexCoordSize
) {
    if (!pkTexMatrix) {
        // Sets kResult to the identity matrix, as D3DXMatrixIdentity does.
        kResult._43 = 0.0f;
        kResult._42 = 0.0f;
        kResult._41 = 0.0f;
        kResult._34 = 0.0f;
        kResult._32 = 0.0f;
        kResult._31 = 0.0f;
        kResult._24 = 0.0f;
        kResult._23 = 0.0f;
        kResult._21 = 0.0f;
        kResult._14 = 0.0f;
        kResult._13 = 0.0f;
        kResult._12 = 0.0f;
        kResult._44 = 1.0f;
        kResult._33 = 1.0f;
        kResult._22 = 1.0f;
        kResult._11 = 1.0f;
        return;
    }

    kResult._11 = pkTexMatrix->m_pEntry[0][0];
    kResult._21 = pkTexMatrix->m_pEntry[0][1];
    kResult._12 = pkTexMatrix->m_pEntry[1][0];
    kResult._22 = pkTexMatrix->m_pEntry[1][1];
    if (uiTexCoordSize == 4 || uiTexCoordSize == 3) {
        kResult._31 = 0.0f;
        kResult._41 = pkTexMatrix->m_pEntry[0][2];
        kResult._32 = 0.0f;
        kResult._42 = pkTexMatrix->m_pEntry[1][2];
    }
    else if (uiTexCoordSize == 2) {
        kResult._31 = pkTexMatrix->m_pEntry[0][2];
        kResult._41 = 0.0f;
        kResult._32 = pkTexMatrix->m_pEntry[1][2];
        kResult._42 = 0.0f;
    }
    kResult._43 = 0.0f;
    kResult._33 = 0.0f;
    kResult._23 = 0.0f;
    kResult._13 = 0.0f;
    kResult._44 = 0.0f;
    kResult._34 = 0.0f;
    kResult._24 = 0.0f;
    kResult._14 = 0.0f;
    if (bInv) {
        D3DXMatrixInverse_Thunk(&kResult, nullptr, &kResult);
    }
    if (bTrans) {
        D3DXMatrixTranspose_Thunk(&kResult, &kResult);
    }
}

// Fills the data with the value an object mapping takes from its dynamic effect (a light or a texture effect, not
// type-checked except where noted): colours as rgb1, positions and directions as xyz1 (a directional light's position
// is far along the reverse of its direction; "model" values are in the space of kWorld), transforms and projections as
// 4x4 matrices, a clipping plane as normal and constant. Without an effect each mapping writes its fallback (a matrix
// one writes the identity) and returns false; an unknown mapping zeroes uiDataSize bytes. Each matrix goes through
// FillDataArray.
bool NiD3DShaderConstantMap::GetDynamicEffectData(
    void* pvData,
    uint32_t uiDataSize,
    NiShaderConstantMap::ObjectMappings eMapping,
    NiDynamicEffect* pkDynEffect,
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    const NiSkinPartition::Partition* pkPartition,
    NiGeometryBufferData* pkBuffData,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound,
    uint32_t uiPass
) {
    float* apfData = static_cast<float*>(pvData);
    D3DMATRIX kIdentMatrix;
    std::memset(&kIdentMatrix, 0, sizeof(kIdentMatrix));
    kIdentMatrix._44 = 1.0f;
    kIdentMatrix._33 = 1.0f;
    kIdentMatrix._22 = 1.0f;
    kIdentMatrix._11 = 1.0f;
    D3DMATRIX kMatrix;
    NiTransform kInvWorld;
    NiPoint3 kVector;

    switch (eMapping) {
        case SCM_OBJ_DIMMER:
            if (pkDynEffect) {
                apfData[0] = static_cast<NiLight*>(pkDynEffect)->m_fDimmer;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 0.0f;
                return true;
            }
            apfData[0] = 0.0f;
            apfData[1] = 0.0f;
            apfData[2] = 0.0f;
            apfData[3] = 0.0f;
            return false;

        case SCM_OBJ_UNDIMMEDAMBIENT:
        case SCM_OBJ_UNDIMMEDDIFFUSE:
        case SCM_OBJ_UNDIMMEDSPECULAR:
        case SCM_OBJ_AMBIENT:
        case SCM_OBJ_DIFFUSE:
        case SCM_OBJ_SPECULAR: {
            if (!pkDynEffect) {
                apfData[0] = 0.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 1.0f;
                return false;
            }
            NiLight* pkLight = static_cast<NiLight*>(pkDynEffect);
            const NiColor* pkColor = &pkLight->m_kSpec;
            if (eMapping == SCM_OBJ_UNDIMMEDAMBIENT || eMapping == SCM_OBJ_AMBIENT) {
                pkColor = &pkLight->m_kAmb;
            }
            else if (eMapping == SCM_OBJ_UNDIMMEDDIFFUSE || eMapping == SCM_OBJ_DIFFUSE) {
                pkColor = &pkLight->m_kDiff;
            }
            // The undimmed colours are copied straight into the data.
            if (eMapping < SCM_OBJ_AMBIENT) {
                apfData[0] = pkColor->r;
                apfData[1] = pkColor->g;
                apfData[2] = pkColor->b;
                apfData[3] = 1.0f;
                return true;
            }
            // The dimmed ones are copied field by field (no constructor call) and scaled by the dimmer.
            kVector.x = pkColor->r;
            kVector.y = pkColor->g;
            kVector.z = pkColor->b;
            kVector *= pkLight->m_fDimmer;
            apfData[0] = kVector.x;
            apfData[1] = kVector.y;
            apfData[2] = kVector.z;
            apfData[3] = 1.0f;
            return true;
        }

        case SCM_OBJ_WORLDPOSITION:
        case SCM_OBJ_MODELPOSITION:
            if (!pkDynEffect) {
                apfData[0] = 0.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 1.0f;
                return false;
            }
            if (eMapping == SCM_OBJ_MODELPOSITION) {
                kWorld.Invert(kInvWorld);
            }
            if (pkDynEffect->m_ucEffectType == NiDynamicEffect::DIR_LIGHT) {
                const NiPoint3& kDir = static_cast<NiDirectionalLight*>(pkDynEffect)->m_kWorldDir;
                kVector.x = static_cast<float>(kDir.x * -1048576.0);
                kVector.y = static_cast<float>(kDir.y * -1048576.0);
                kVector.z = static_cast<float>(kDir.z * -1048576.0);
                if (eMapping == SCM_OBJ_MODELPOSITION) {
                    kVector = kInvWorld * kVector;
                }
            }
            else if (eMapping == SCM_OBJ_MODELPOSITION) {
                kVector = kInvWorld * pkDynEffect->m_kWorld.m_Translate;
            }
            else {
                kVector = pkDynEffect->m_kWorld.m_Translate;
            }
            apfData[0] = kVector.x;
            apfData[1] = kVector.y;
            apfData[2] = kVector.z;
            apfData[3] = 1.0f;
            return true;

        case SCM_OBJ_WORLDDIRECTION:
        case SCM_OBJ_MODELDIRECTION:
            if (!pkDynEffect) {
                apfData[0] = 1.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 1.0f;
                return false;
            }
            // A point light shines from its position towards the bound's centre.
            if (pkDynEffect->m_ucEffectType == NiDynamicEffect::POINT_LIGHT) {
                kVector = kWorldBound.m_kCenter - pkDynEffect->m_kWorld.m_Translate;
                kVector.Unitize();
            }
            else if (pkDynEffect->m_ucEffectType == NiDynamicEffect::DIR_LIGHT) {
                kVector = static_cast<NiDirectionalLight*>(pkDynEffect)->m_kWorldDir;
            }
            else {
                kVector = static_cast<NiSpotLight*>(pkDynEffect)->m_kWorldDir;
            }
            if (eMapping == SCM_OBJ_MODELDIRECTION) {
                kVector = kWorld.m_Rotate.Transpose() * kVector;
            }
            apfData[0] = kVector.x;
            apfData[1] = kVector.y;
            apfData[2] = kVector.z;
            apfData[3] = 1.0f;
            return true;

        case SCM_OBJ_WORLDTRANSFORM:
            if (!pkDynEffect) {
                break;
            }
            kMatrix._14 = 0.0f;
            kMatrix._24 = 0.0f;
            kMatrix._34 = 0.0f;
            kMatrix._44 = 1.0f;
            if (pkDynEffect->m_ucEffectType == NiDynamicEffect::DIR_LIGHT) {
                const NiPoint3& kDir = static_cast<NiDirectionalLight*>(pkDynEffect)->m_kWorldDir;
                kVector.x = static_cast<float>(kDir.x * -1048576.0);
                kVector.y = static_cast<float>(kDir.y * -1048576.0);
                kVector.z = static_cast<float>(kDir.z * -1048576.0);
                NiD3DUtility::GetD3DFromNi(
                    kMatrix, pkDynEffect->m_kWorld.m_Rotate, kVector, pkDynEffect->m_kWorld.m_fScale
                );
            }
            else {
                NiD3DUtility::GetD3DFromNi(kMatrix, pkDynEffect->m_kWorld);
            }
            FillDataArray(apfData, kMatrix);
            return true;

        case SCM_OBJ_MODELTRANSFORM:
            if (!pkDynEffect) {
                break;
            }
            kWorld.Invert(kInvWorld);
            kMatrix._14 = 0.0f;
            kMatrix._24 = 0.0f;
            kMatrix._34 = 0.0f;
            kMatrix._44 = 1.0f;
            if (pkDynEffect->m_ucEffectType == NiDynamicEffect::DIR_LIGHT) {
                NiTransform kLightWorld = pkDynEffect->m_kWorld;
                const NiPoint3& kDir = static_cast<NiDirectionalLight*>(pkDynEffect)->m_kWorldDir;
                kVector.x = static_cast<float>(kDir.x * -1048576.0);
                kVector.y = static_cast<float>(kDir.y * -1048576.0);
                kVector.z = static_cast<float>(kDir.z * -1048576.0);
                kLightWorld.m_Translate = kVector;
                NiD3DUtility::GetD3DFromNi(kMatrix, kInvWorld * kLightWorld);
            }
            else {
                NiD3DUtility::GetD3DFromNi(kMatrix, kInvWorld * pkDynEffect->m_kWorld);
            }
            FillDataArray(apfData, kMatrix);
            return true;

        case SCM_OBJ_SPOTATTENUATION:
            // The cosines of the inner and outer angles (degrees), and the exponent.
            if (pkDynEffect && pkDynEffect->m_ucEffectType == NiDynamicEffect::SPOT_LIGHT) {
                NiSpotLight* pkSpot = static_cast<NiSpotLight*>(pkDynEffect);
                float fAngle = static_cast<float>(pkSpot->m_fInnerSpotAngle * 3.1415927410125732 / 180.0);
                float fCos = static_cast<float>(std::cos(static_cast<double>(fAngle)));
                apfData[0] = fCos;
                fAngle = static_cast<float>(pkSpot->m_fOuterSpotAngle * 3.1415927410125732 / 180.0);
                fCos = static_cast<float>(std::cos(static_cast<double>(fAngle)));
                apfData[1] = fCos;
                apfData[2] = pkSpot->m_fSpotExponent;
                apfData[3] = 0.0f;
                return true;
            }
            apfData[0] = -1.0f;
            apfData[1] = -1.0f;
            apfData[2] = 0.0f;
            apfData[3] = 0.0f;
            return pkDynEffect != nullptr;

        case SCM_OBJ_ATTENUATION:
            if (pkDynEffect && NiObject::IsKindOf(&NiPointLight::ms_RTTI, pkDynEffect)) {
                NiPointLight* pkPoint = static_cast<NiPointLight*>(pkDynEffect);
                apfData[0] = pkPoint->m_fAtten0;
                apfData[1] = pkPoint->m_fAtten1;
                apfData[2] = pkPoint->m_fAtten2;
                apfData[3] = 0.0f;
                return true;
            }
            apfData[0] = 1.0f;
            apfData[1] = 0.0f;
            apfData[2] = 0.0f;
            apfData[3] = 0.0f;
            return pkDynEffect != nullptr;

        case SCM_OBJ_WORLDPROJECTIONMATRIX:
            if (!pkDynEffect) {
                break;
            }
            kMatrix._14 = 0.0f;
            kMatrix._24 = 0.0f;
            kMatrix._34 = 0.0f;
            kMatrix._44 = 1.0f;
            NiD3DUtility::GetD3DFromNi(
                kMatrix, static_cast<NiTextureEffect*>(pkDynEffect)->m_kWorldProjMat, NiPoint3_ZERO, 1.0f
            );
            FillDataArray(apfData, kMatrix);
            return true;

        case SCM_OBJ_MODELPROJECTIONMATRIX:
            if (!pkDynEffect) {
                break;
            }
            kMatrix._14 = 0.0f;
            kMatrix._24 = 0.0f;
            kMatrix._34 = 0.0f;
            kMatrix._44 = 1.0f;
            NiD3DUtility::GetD3DFromNi(
                kMatrix, kWorld.m_Rotate.Transpose() * static_cast<NiTextureEffect*>(pkDynEffect)->m_kWorldProjMat,
                NiPoint3_ZERO, 1.0f
            );
            FillDataArray(apfData, kMatrix);
            return true;

        case SCM_OBJ_WORLDPROJECTIONTRANSLATION:
        case SCM_OBJ_MODELPROJECTIONTRANSLATION:
            if (!pkDynEffect) {
                apfData[0] = 0.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 1.0f;
                return false;
            }
            if (eMapping == SCM_OBJ_MODELPROJECTIONTRANSLATION) {
                kWorld.Invert(kInvWorld);
                kVector = static_cast<NiTextureEffect*>(pkDynEffect)->m_kWorldProjTrans;
                kVector = kInvWorld * kVector;
            }
            else {
                kVector = static_cast<NiTextureEffect*>(pkDynEffect)->m_kWorldProjTrans;
            }
            apfData[0] = kVector.x;
            apfData[1] = kVector.y;
            apfData[2] = kVector.z;
            apfData[3] = 1.0f;
            return true;

        case SCM_OBJ_WORLDCLIPPINGPLANE:
        case SCM_OBJ_MODELCLIPPINGPLANE: {
            if (!pkDynEffect) {
                apfData[0] = 1.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 0.0f;
                return false;
            }
            NiTextureEffect* pkTexEffect = static_cast<NiTextureEffect*>(pkDynEffect);
            if (!pkTexEffect->m_bPlaneEnable) {
                apfData[0] = 0.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 0.0f;
                return true;
            }
            if (eMapping == SCM_OBJ_WORLDCLIPPINGPLANE) {
                apfData[0] = pkTexEffect->m_kWorldPlane.m_kNormal.x;
                apfData[1] = pkTexEffect->m_kWorldPlane.m_kNormal.y;
                apfData[2] = pkTexEffect->m_kWorldPlane.m_kNormal.z;
                apfData[3] = pkTexEffect->m_kWorldPlane.m_fConstant;
                return true;
            }
            // The model plane: the rotated normal, and the constant through the moved point of the plane nearest the
            // origin.
            kWorld.Invert(kInvWorld);
            NiPlane kPlane = pkTexEffect->m_kWorldPlane;
            NiPoint3 kNormal = kInvWorld.m_Rotate * kPlane.m_kNormal;
            float fConstant = kPlane.m_fConstant;
            kVector.x = kPlane.m_kNormal.x * fConstant;
            kVector.y = kPlane.m_kNormal.y * fConstant;
            kVector.z = kPlane.m_kNormal.z * fConstant;
            kVector = kInvWorld * kVector;
            apfData[0] = kNormal.x;
            apfData[1] = kNormal.y;
            apfData[2] = kNormal.z;
            apfData[3] = kNormal.Dot(kVector);
            return true;
        }

        case SCM_OBJ_TEXCOORDGEN: {
            if (!pkDynEffect) {
                apfData[0] = 0.0f;
                apfData[1] = 0.0f;
                apfData[2] = 0.0f;
                apfData[3] = 0.0f;
                return false;
            }
            float fTexCoordGen = 0.0f;
            switch (static_cast<NiTextureEffect*>(pkDynEffect)->m_eCoordMode) {
                case NiTextureEffect::WORLD_PARALLEL:
                case NiTextureEffect::WORLD_PERSPECTIVE: fTexCoordGen = 2.0f; break;
                case NiTextureEffect::SPHERE_MAP:
                case NiTextureEffect::SPECULAR_CUBE_MAP: fTexCoordGen = 3.0f; break;
                case NiTextureEffect::DIFFUSE_CUBE_MAP: fTexCoordGen = 1.0f; break;
                default: break;
            }
            apfData[0] = fTexCoordGen;
            apfData[1] = 0.0f;
            apfData[2] = 0.0f;
            apfData[3] = 0.0f;
            return true;
        }

        case SCM_OBJ_WORLDPROJECTIONTRANSFORM: {
            if (!pkDynEffect) {
                break;
            }
            NiTextureEffect* pkTexEffect = static_cast<NiTextureEffect*>(pkDynEffect);
            kMatrix._14 = 0.0f;
            kMatrix._24 = 0.0f;
            kMatrix._34 = 0.0f;
            kMatrix._44 = 1.0f;
            NiD3DUtility::GetD3DFromNi(kMatrix, pkTexEffect->m_kWorldProjMat, pkTexEffect->m_kWorldProjTrans, 1.0f);
            FillDataArray(apfData, kMatrix);
            return true;
        }

        case SCM_OBJ_MODELPROJECTIONTRANSFORM: {
            if (!pkDynEffect) {
                break;
            }
            NiTextureEffect* pkTexEffect = static_cast<NiTextureEffect*>(pkDynEffect);
            kMatrix._34 = 0.0f;
            kMatrix._24 = 0.0f;
            kMatrix._14 = 0.0f;
            kMatrix._44 = 1.0f;
            kWorld.Invert(kInvWorld);
            // Copies the translation into the local first and transforms the copy.
            kVector = pkTexEffect->m_kWorldProjTrans;
            kVector = kInvWorld * kVector;
            NiD3DUtility::GetD3DFromNi(
                kMatrix, kWorld.m_Rotate.Transpose() * pkTexEffect->m_kWorldProjMat, kVector, 1.0f
            );
            FillDataArray(apfData, kMatrix);
            return true;
        }

        default: std::memset(apfData, 0, uiDataSize); return false;
    }

    // The matrix mappings without an effect.
    FillDataArray(apfData, kIdentMatrix);
    return false;
}

NiShaderConstantMapEntry::~NiShaderConstantMapEntry() {
    if (m_bOwnData) {
        _NiFree(m_pvDataSource);
    }
}

void NiShaderConstantMapEntry::SetVariableName(const NiFixedString& kVariableName) {
    const char* m_kHandle = (&kVariableName)->GetHandle();
    if (m_kVariableName.GetHandle() != (&kVariableName)->GetHandle()) {
        NiGlobalStringTable::AcquireHandle(m_kHandle);
        const char* v4 = m_kVariableName.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v4);
        m_kVariableName.m_kHandle = m_kHandle;
    }
}

const NiMatrix3* NiTextureTransform::GetMatrix() const {
    if (m_bMatrixDirty) {
        UpdateMatrix();
    }
    return &m_kMatrix;
}

// The product goes into a local and is then copied to the result, so D3DXMatrixMultiply never writes the caller's
// result directly.
_D3DMATRIX _D3DMATRIX::operator*(const _D3DMATRIX& mat) const {
    _D3DMATRIX kProduct;
    D3DXMatrixMultiply_Thunk(&kProduct, this, &mat);
    return kProduct;
}
