#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"

namespace {

    using PackingParameters = NiD3DShaderDeclaration::PackingParameters;

    // Clears m_uiOutSize bytes of every output vertex (the packers' path for missing input data). Returns the bytes
    // cleared.
    inline uint32_t PackClearOutput(PackingParameters& kParams) {
        uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
        uint32_t uiPacked = 0;
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            std::memset(pucOut, 0, kParams.m_uiOutSize);
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
        return uiPacked;
    }

    // Number of input components: m_uiDataType counted from eOneComponent (the type with one component), in 16 bits.
    inline uint16_t PackInputCount(const PackingParameters& kParams, NiAdditionalGeometryData::Types eOneComponent) {
        return static_cast<uint16_t>(kParams.m_uiDataType - eOneComponent + 1);
    }

    // One vertex into shorts: usCopyCount components converted (the low 16 bits of their 32-bit integer value; floats
    // truncate), then zeros up to usOutCount.
    template <class TIn>
    inline void PackVertexToShort(const uint8_t* pucIn, uint8_t* pucOut, uint16_t usCopyCount, uint16_t usOutCount) {
        const TIn* pkIn = reinterpret_cast<const TIn*>(pucIn);
        int16_t* psOut = reinterpret_cast<int16_t*>(pucOut);
        uint16_t j = 0;
        for (; j < usCopyCount; j++) {
            *psOut++ = static_cast<int16_t>(static_cast<int32_t>(*pkIn++));
        }
        for (; j < usOutCount; j++) {
            *psOut++ = 0;
        }
    }

    // The *ToShort packers: 2 outputs for SPTYPE_SHORT2, else 4; the input components past them are dropped.
    template <class TIn>
    uint32_t PackToShort(PackingParameters& kParams, NiAdditionalGeometryData::Types eOneComponent) {
        const uint8_t* pucIn = static_cast<const uint8_t*>(kParams.m_pvInData);
        if (!pucIn) {
            return PackClearOutput(kParams);
        }

        uint16_t usInCount = PackInputCount(kParams, eOneComponent);
        uint16_t usOutCount = kParams.m_ePackType != NiShaderDeclaration::SPTYPE_SHORT2 ? 4 : 2;
        uint16_t usCopyCount = usInCount > usOutCount ? usOutCount : usInCount;
        const uint16_t* pusVertexMap = kParams.m_pusVertexMap;
        uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
        uint32_t uiPacked = 0;
        if (pusVertexMap) {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                PackVertexToShort<TIn>(pucIn + pusVertexMap[i] * kParams.m_uiInStride, pucOut, usCopyCount, usOutCount);
                uiPacked += kParams.m_uiOutSize;
                pucOut += kParams.m_uiOutStride;
            }
        }
        else {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                PackVertexToShort<TIn>(pucIn, pucOut, usCopyCount, usOutCount);
                uiPacked += kParams.m_uiOutSize;
                pucIn += kParams.m_uiInStride;
                pucOut += kParams.m_uiOutStride;
            }
        }
        return uiPacked;
    }

    // One vertex into 4 unsigned bytes: usCopyCount components converted (the low byte of their 32-bit integer value;
    // floats truncate), the remaining bytes cleared with memset.
    template <class TIn>
    inline void PackVertexToUnsignedByte(const uint8_t* pucIn, uint8_t* pucOut, uint16_t usCopyCount) {
        const TIn* pkIn = reinterpret_cast<const TIn*>(pucIn);
        uint16_t j = 0;
        for (; j < usCopyCount; j++) {
            *pucOut++ = static_cast<uint8_t>(static_cast<int32_t>(*pkIn++));
        }
        if (j < 4) {
            std::memset(pucOut, 0, static_cast<uint16_t>(4 - j));
        }
    }

    // The *ToUnsignedByte packers: 4 outputs; the input components past them are dropped.
    template <class TIn>
    uint32_t PackToUnsignedByte(PackingParameters& kParams, NiAdditionalGeometryData::Types eOneComponent) {
        const uint8_t* pucIn = static_cast<const uint8_t*>(kParams.m_pvInData);
        if (!pucIn) {
            return PackClearOutput(kParams);
        }

        uint16_t usInCount = PackInputCount(kParams, eOneComponent);
        uint16_t usCopyCount = usInCount > 4 ? 4 : usInCount;
        const uint16_t* pusVertexMap = kParams.m_pusVertexMap;
        uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
        uint32_t uiPacked = 0;
        if (pusVertexMap) {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                PackVertexToUnsignedByte<TIn>(pucIn + pusVertexMap[i] * kParams.m_uiInStride, pucOut, usCopyCount);
                uiPacked += kParams.m_uiOutSize;
                pucOut += kParams.m_uiOutStride;
            }
        }
        else {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                PackVertexToUnsignedByte<TIn>(pucIn, pucOut, usCopyCount);
                uiPacked += kParams.m_uiOutSize;
                pucIn += kParams.m_uiInStride;
                pucOut += kParams.m_uiOutStride;
            }
        }
        return uiPacked;
    }

    // One vertex of 3 or 4 16-bit components into a D3DCOLOR: the high byte of each, components 0..2 as red, green and
    // blue, component 3 as alpha (0xFF when there are only 3).
    template <class TIn>
    inline uint32_t PackShortsToUByteColor(const uint8_t* pucIn, uint16_t usInCount) {
        const TIn* pkIn = reinterpret_cast<const TIn*>(pucIn);
        TIn kAlpha = usInCount == 3 ? static_cast<TIn>(-1) : pkIn[3];
        uint32_t uiA = static_cast<uint8_t>(kAlpha >> 8);
        uint32_t uiR = static_cast<uint8_t>(pkIn[0] >> 8);
        uint32_t uiG = static_cast<uint8_t>(pkIn[1] >> 8);
        uint32_t uiB = static_cast<uint8_t>(pkIn[2] >> 8);
        return (uiA << 24) | (uiR << 16) | (uiG << 8) | uiB;
    }

    // The (Unsigned)ShortToUByteColor packers; without input data every vertex is opaque white.
    template <class TIn>
    uint32_t PackShortToUByteColor(PackingParameters& kParams, NiAdditionalGeometryData::Types eOneComponent) {
        const uint16_t* pusVertexMap = kParams.m_pusVertexMap;
        uint32_t* puiOut = static_cast<uint32_t*>(kParams.m_pvOutData);
        const uint8_t* pucIn = static_cast<const uint8_t*>(kParams.m_pvInData);
        uint32_t uiPacked = 0;
        if (!pucIn) {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                *puiOut = 0xFFFFFFFF;
                puiOut = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(puiOut) + kParams.m_uiOutStride);
                uiPacked += kParams.m_uiOutSize;
            }
            return uiPacked;
        }

        uint16_t usInCount = PackInputCount(kParams, eOneComponent);
        if (pusVertexMap) {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                *puiOut = PackShortsToUByteColor<TIn>(pucIn + pusVertexMap[i] * kParams.m_uiInStride, usInCount);
                puiOut = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(puiOut) + kParams.m_uiOutStride);
                uiPacked += kParams.m_uiOutSize;
            }
        }
        else {
            for (uint16_t i = 0; i < kParams.m_usCount; i++) {
                *puiOut = PackShortsToUByteColor<TIn>(pucIn, usInCount);
                pucIn += kParams.m_uiInStride;
                puiOut = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(puiOut) + kParams.m_uiOutStride);
                uiPacked += kParams.m_uiOutSize;
            }
        }
        return uiPacked;
    }

} // namespace

NiD3DShaderDeclaration::ShaderRegisterEntry::ShaderRegisterEntry()
  : m_uiRegister(0), m_uiPackingOffset(0), m_eInput(NiShaderDeclaration::SHADERPARAM_INVALID),
    m_eType(NiShaderDeclaration::SPTYPE_NONE), m_eTesselator(NiD3DShaderDeclaration::SPTESS_DEFAULT),
    m_eUsage(NiShaderDeclaration::SPUSAGE_POSITION), m_uiUsageIndex(0) {}

NiD3DShaderDeclaration::ShaderRegisterStream::ShaderRegisterStream()
  : m_bValid(false), m_uiEntryCount(0), m_pkEntries(nullptr), m_uiStride(0) {}

NiD3DShaderDeclaration::NiPackerEntry::NiPackerEntry()
  : m_uiShaderParameterType(NiShaderDeclaration::SPTYPE_NONE), m_kPackFunctions(0, 1) {
    m_kPackFunctions.RemoveAll();
}

NiD3DShaderDeclaration::NiD3DShaderDeclaration()
  : m_pkRenderer(nullptr), m_pkVBManager(nullptr), m_pkD3DDevice(nullptr), m_uiDeclSize(0), m_uiCurrDeclIndex(0),
    m_uiMaxStreamEntryCount(0), m_uiStreamCount(0), m_pkStreamEntries(nullptr), m_bModified(false) {}

// Frees each stream's register entries and the stream array, then releases the device.
NiD3DShaderDeclaration::~NiD3DShaderDeclaration() {
    if (m_pkStreamEntries) {
        for (uint32_t i = 0; i < m_uiStreamCount; ++i) {
            delete[] m_pkStreamEntries[i].m_pkEntries;
        }
        delete[] m_pkStreamEntries;
    }
    m_pkStreamEntries = nullptr;
    m_pkRenderer = nullptr;
    m_pkD3DDevice->Release();
    m_pkD3DDevice = nullptr;
}

// Keeps the renderer, its vertex buffer manager and a reference to its device, then allocates the streams and, per
// stream, uiMaxStreamEntryCount entries.
bool NiD3DShaderDeclaration::Initialize(
    NiDX9Renderer* pkRenderer,
    uint32_t uiMaxStreamEntryCount,
    uint32_t uiStreamCount
) {
    m_pkRenderer = pkRenderer;
    m_pkVBManager = pkRenderer->m_pkVBManager;
    IDirect3DDevice9* pkDevice = pkRenderer->m_pkD3DDevice9;
    m_pkD3DDevice = pkDevice;
    pkDevice->AddRef();

    m_uiMaxStreamEntryCount = uiMaxStreamEntryCount;
    m_uiStreamCount = uiStreamCount;
    ShaderRegisterStream* pkStreams = new ShaderRegisterStream[uiStreamCount];
    m_pkStreamEntries = pkStreams;

    for (uint32_t i = 0; i < m_uiStreamCount; i++) {
        uint32_t uiEntryCount = m_uiMaxStreamEntryCount;
        ShaderRegisterEntry* pkEntries = new ShaderRegisterEntry[uiEntryCount];
        if (pkEntries) {
            ShaderRegisterEntry* pkEntry = pkEntries;
            for (int32_t iLeft = static_cast<int32_t>(uiEntryCount) - 1; iLeft >= 0; iLeft--) {
                ::new (pkEntry++) ShaderRegisterEntry;
            }
        }
        m_pkStreamEntries[i].m_pkEntries = pkEntries;
    }
    return true;
}

uint32_t NiD3DShaderDeclaration::PackEntry(
    uint16_t ausCount,
    NiGeometryData* apData,
    NiSkinInstance* apSkin,
    NiSkinPartition::Partition* apPartition,
    ShaderRegisterEntry* apEntry,
    uint32_t auiStride,
    uint8_t* apucData,
    void* apv8,
    void* apv9
) {
    PackingParameters kPackParams;
    kPackParams.m_usCount = ausCount;
    // Stores only the low byte of the void* argument apv8.
    kPackParams.m_bPCOnly28 = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(apv8));
    kPackParams.m_uiOutStride = auiStride;
    kPackParams.m_pvOutData = apucData;

    ShaderParameterType eType = apEntry->m_eType;
    if (eType == SPTYPE_NONE) {
        return 0;
    }
    if (eType == SPTYPE_UBYTECOLOR && apEntry->m_eInput == SHADERPARAM_NI_BLENDINDICES) {
        eType = SPTYPE_UBYTE4;
    }
    kPackParams.m_uiOutSize = ms_auiTypeSizes[eType];

    NiPackerEntry* pkPackEntry = ms_pkPackFunctions->m_pBase[eType];
    if (!pkPackEntry) {
        return 0;
    }

    NiAdditionalGeometryData* pkExtraData = apData->m_spAdditionalGeomData.m_pObject;
    uint32_t uiDataType = ausCount;
    ShaderParameter eInput = apEntry->m_eInput;
    switch (eInput) {
        case SHADERPARAM_INVALID: return 0;
        case SHADERPARAM_NI_POSITION:
            uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT3;
            kPackParams.m_pvInData = apData->m_pkVertex;
            kPackParams.m_uiInSize = sizeof(NiPoint3);
            kPackParams.m_uiInStride = sizeof(NiPoint3);
            break;
        case SHADERPARAM_NI_BLENDWEIGHT:
            if (!apPartition) {
                return 0;
            }
            switch (apPartition->m_usBonesPerVertex) {
                case 1:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT1;
                    kPackParams.m_uiInSize = sizeof(float);
                    kPackParams.m_uiInStride = sizeof(float);
                    break;
                case 2:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT2;
                    kPackParams.m_uiInSize = 2 * sizeof(float);
                    kPackParams.m_uiInStride = 2 * sizeof(float);
                    break;
                case 3:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT3;
                    kPackParams.m_uiInSize = 3 * sizeof(float);
                    kPackParams.m_uiInStride = 3 * sizeof(float);
                    break;
                case 4:
                    // Four bones keep four weights per vertex, of which three are packed.
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT3;
                    kPackParams.m_uiInSize = 3 * sizeof(float);
                    kPackParams.m_uiInStride = 4 * sizeof(float);
                    break;
                // Packs nothing (a departure): with 0 or more than 4 bones per vertex nothing sets the input size and
                // stride; the exe reads them unset and indexes the packers with the vertex count.
                default: return 0;
            }
            kPackParams.m_pvInData = apPartition->m_pfWeights;
            break;
        case SHADERPARAM_NI_BLENDINDICES:
            if (!apPartition || !apPartition->m_pucBonePalette) {
                return 0;
            }
            switch (apPartition->m_usBonesPerVertex) {
                case 1:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_UBYTE1;
                    kPackParams.m_uiInSize = 1;
                    kPackParams.m_uiInStride = 1;
                    break;
                case 2:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_UBYTE2;
                    kPackParams.m_uiInSize = 2;
                    kPackParams.m_uiInStride = 2;
                    break;
                case 3:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_UBYTE3;
                    kPackParams.m_uiInSize = 3;
                    kPackParams.m_uiInStride = 3;
                    break;
                case 4:
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_UBYTE4;
                    kPackParams.m_uiInSize = 4;
                    kPackParams.m_uiInStride = 4;
                    break;
                // Packs nothing (a departure), as for the blend weights.
                default: return 0;
            }
            kPackParams.m_pvInData = apPartition->m_pucBonePalette;
            break;
        case SHADERPARAM_NI_NORMAL:
            uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT3;
            kPackParams.m_pvInData = apData->m_pkNormal;
            kPackParams.m_uiInSize = sizeof(NiPoint3);
            kPackParams.m_uiInStride = sizeof(NiPoint3);
            break;
        case SHADERPARAM_NI_COLOR:
            uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT4;
            kPackParams.m_pvInData = apData->m_pkColor;
            kPackParams.m_uiInSize = sizeof(NiColorA);
            kPackParams.m_uiInStride = sizeof(NiColorA);
            break;
        case SHADERPARAM_NI_TEXCOORD0:
        case SHADERPARAM_NI_TEXCOORD1:
        case SHADERPARAM_NI_TEXCOORD2:
        case SHADERPARAM_NI_TEXCOORD3:
        case SHADERPARAM_NI_TEXCOORD4:
        case SHADERPARAM_NI_TEXCOORD5:
        case SHADERPARAM_NI_TEXCOORD6:
        case SHADERPARAM_NI_TEXCOORD7:
            uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT2;
            kPackParams.m_pvInData = apData->GetTextureSet(static_cast<uint16_t>(eInput - SHADERPARAM_NI_TEXCOORD0));
            kPackParams.m_uiInSize = sizeof(NiPoint2);
            kPackParams.m_uiInStride = sizeof(NiPoint2);
            break;
        case SHADERPARAM_NI_TANGENT: {
            // With an NBT method the normal array holds the normals, then the binormals, then the tangents.
            NiPoint3* pkNormals = apData->m_pkNormal;
            uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT3;
            if (pkNormals && (apData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) != 0) {
                kPackParams.m_pvInData = &pkNormals[2 * apData->m_usVertices];
            }
            else {
                kPackParams.m_pvInData = nullptr;
            }
            kPackParams.m_uiInSize = sizeof(NiPoint3);
            kPackParams.m_uiInStride = sizeof(NiPoint3);
            break;
        }
        case SHADERPARAM_NI_BINORMAL: {
            NiPoint3* pkNormals = apData->m_pkNormal;
            uiDataType = NiAdditionalGeometryData::AGD_NITYPE_FLOAT3;
            if (pkNormals && (apData->m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) != 0) {
                kPackParams.m_pvInData = &pkNormals[apData->m_usVertices];
            }
            else {
                kPackParams.m_pvInData = nullptr;
            }
            kPackParams.m_uiInSize = sizeof(NiPoint3);
            kPackParams.m_uiInStride = sizeof(NiPoint3);
            break;
        }
        default:
            // An extra data stream of the additional geometry data.
            if (eInput < 0) {
                uint8_t* pucData;
                uint16_t usCount;
                uint32_t uiTotalSize;
                if (pkExtraData->GetDataStream(
                        static_cast<uint32_t>(eInput & ~SHADERPARAM_EXTRA_DATA_MASK),
                        pucData,
                        uiDataType,
                        usCount,
                        uiTotalSize,
                        kPackParams.m_uiInSize,
                        kPackParams.m_uiInStride
                    )) {
                    kPackParams.m_pvInData = pucData;
                }
                else {
                    uiDataType = NiAdditionalGeometryData::AGD_NITYPE_INVALID;
                    kPackParams.m_pvInData = nullptr;
                    kPackParams.m_uiInSize = 0;
                    kPackParams.m_uiInStride = 0;
                }
            }
            else {
                // No input (a departure), as for a missing data stream: nothing sets up SHADERPARAM_DATA_ZERO and
                // above; the exe reads the input pointer unset and indexes the packers with the vertex count.
                uiDataType = NiAdditionalGeometryData::AGD_NITYPE_INVALID;
                kPackParams.m_pvInData = nullptr;
                kPackParams.m_uiInSize = 0;
                kPackParams.m_uiInStride = 0;
            }
            break;
    }

    PackFunction pfnPack = pkPackEntry->m_kPackFunctions.m_pBase[uiDataType];
    if (!pfnPack) {
        return 0;
    }

    // Blend weights and indices are stored per partition vertex already; everything else goes through the map.
    if (apPartition &&
        apEntry->m_eInput != SHADERPARAM_NI_BLENDWEIGHT &&
        apEntry->m_eInput != SHADERPARAM_NI_BLENDINDICES) {
        kPackParams.m_pusVertexMap = apPartition->m_pusVertexMap;
    }
    else {
        kPackParams.m_pusVertexMap = nullptr;
    }

    kPackParams.m_ePackType = eType;
    kPackParams.m_uiDataType = uiDataType;
    kPackParams.m_pvPCOnly24 = apv9;
    if (!kPackParams.m_pvInData) {
        return 0;
    }
    return pfnPack(kPackParams);
}

// Allocates the packer entry array, then fills the packing functions.
void NiD3DShaderDeclaration::_SDMInit() {
    NiTPrimitiveArray<NiPackerEntry*>* pkPackFunctions = ::new NiTPrimitiveArray<NiPackerEntry*>(0u, 1u);
    ms_pkPackFunctions = pkPackFunctions;
    InitializePackingFunctions();
}

// Deletes the packer entries and the packer entry array.
void NiD3DShaderDeclaration::_SDMShutdown() {
    NiTPrimitiveArray<NiPackerEntry*>* pkPackFunctions = ms_pkPackFunctions;
    for (uint32_t i = 0; i < pkPackFunctions->m_usSize; i++) {
        NiPackerEntry* pkEntry = pkPackFunctions->m_pBase[i];
        if (pkEntry) {
            delete pkEntry;
            pkPackFunctions = ms_pkPackFunctions;
        }
    }

    pkPackFunctions->RemoveAll();

    delete ms_pkPackFunctions;
}

// Runs once: with every output type's entry in place it only returns true. Otherwise it fills the type sizes (all but
// SPTYPE_NONE), makes one packer entry per output type (its function array sized 0x26), stores it with SetAtGrow, then
// sets the default packer of every type pair.
bool NiD3DShaderDeclaration::InitializePackingFunctions() {
    if (ms_pkPackFunctions->m_usSize != SPTYPE_COUNT) {
        ms_auiTypeSizes[SPTYPE_FLOAT1] = 4;
        ms_auiTypeSizes[SPTYPE_FLOAT2] = 8;
        ms_auiTypeSizes[SPTYPE_FLOAT3] = 12;
        ms_auiTypeSizes[SPTYPE_FLOAT4] = 16;
        ms_auiTypeSizes[SPTYPE_UBYTECOLOR] = 4;
        ms_auiTypeSizes[SPTYPE_UBYTE4] = 4;
        ms_auiTypeSizes[SPTYPE_SHORT2] = 4;
        ms_auiTypeSizes[SPTYPE_SHORT4] = 8;
        ms_auiTypeSizes[SPTYPE_NORMUBYTE4] = 4;
        ms_auiTypeSizes[SPTYPE_NORMSHORT2] = 4;
        ms_auiTypeSizes[SPTYPE_NORMSHORT4] = 8;
        ms_auiTypeSizes[SPTYPE_NORMUSHORT2] = 4;
        ms_auiTypeSizes[SPTYPE_NORMUSHORT4] = 8;
        ms_auiTypeSizes[SPTYPE_UDEC3] = 4;
        ms_auiTypeSizes[SPTYPE_NORMDEC3] = 4;
        ms_auiTypeSizes[SPTYPE_FLOAT16_2] = 4;
        ms_auiTypeSizes[SPTYPE_FLOAT16_4] = 8;
        for (uint32_t i = 0; i < SPTYPE_COUNT; ++i) {
            NiPackerEntry* pkEntry = new NiPackerEntry;
            pkEntry->m_uiShaderParameterType = i;
            pkEntry->m_kPackFunctions.SetSize(0x26);
            ms_pkPackFunctions->SetAtGrow(i, pkEntry);
        }
        for (uint32_t eType = 0; eType < SPTYPE_COUNT; ++eType) {
            for (uint32_t uiAGDType = 0; uiAGDType < NiAdditionalGeometryData::AGD_NITYPE_COUNT; ++uiAGDType) {
                SetDefaultPackingFunction(static_cast<NiShaderDeclaration::ShaderParameterType>(eType), uiAGDType);
            }
        }
    }
    return true;
}

uint32_t NiD3DShaderDeclaration::Pack_IllegalCombination([[maybe_unused]] PackingParameters& kParams) {
    return 0;
}

// Copies each vertex's 32-bit color; without input data every vertex is opaque white.
uint32_t NiD3DShaderDeclaration::Pack_UnsignedLongToUByteColor(PackingParameters& kParams) {
    uint32_t* puiOut = static_cast<uint32_t*>(kParams.m_pvOutData);
    const uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    const uint8_t* pucIn = static_cast<const uint8_t*>(kParams.m_pvInData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            *puiOut = 0xFFFFFFFF;
            puiOut = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(puiOut) + kParams.m_uiOutStride);
            uiPacked += kParams.m_uiOutSize;
        }
        return uiPacked;
    }

    if (pusVertexMap) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            *puiOut = *reinterpret_cast<const uint32_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
            puiOut = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(puiOut) + kParams.m_uiOutStride);
            uiPacked += kParams.m_uiOutSize;
        }
    }
    else {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            *puiOut = *reinterpret_cast<const uint32_t*>(pucIn);
            pucIn += kParams.m_uiInStride;
            puiOut = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(puiOut) + kParams.m_uiOutStride);
            uiPacked += kParams.m_uiOutSize;
        }
    }
    return uiPacked;
}

uint32_t NiD3DShaderDeclaration::Pack_ShortToUByteColor(PackingParameters& kParams) {
    return PackShortToUByteColor<int16_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_SHORT1);
}

uint32_t NiD3DShaderDeclaration::Pack_UnsignedShortToUByteColor(PackingParameters& kParams) {
    return PackShortToUByteColor<uint16_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_USHORT1);
}

uint32_t NiD3DShaderDeclaration::Pack_FloatToShort(PackingParameters& kParams) {
    return PackToShort<float>(kParams, NiAdditionalGeometryData::AGD_NITYPE_FLOAT1);
}

uint32_t NiD3DShaderDeclaration::Pack_LongToShort(PackingParameters& kParams) {
    return PackToShort<int32_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_LONG1);
}

uint32_t NiD3DShaderDeclaration::Pack_UnsignedLongToShort(PackingParameters& kParams) {
    return PackToShort<uint32_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_ULONG1);
}

// Copies the components with memcpy, then zeros the remaining outputs.
uint32_t NiD3DShaderDeclaration::Pack_ShortToShort(PackingParameters& kParams) {
    const uint8_t* pucIn = static_cast<const uint8_t*>(kParams.m_pvInData);
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_SHORT1);
    uint16_t usOutCount = kParams.m_ePackType != SPTYPE_SHORT2 ? 4 : 2;
    uint16_t usCopyCount = usInCount > usOutCount ? usOutCount : usInCount;
    uint32_t uiCopySize = usCopyCount * sizeof(int16_t);
    const uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const uint8_t* pucVertex = pusVertexMap ? pucIn + pusVertexMap[i] * kParams.m_uiInStride : pucIn;
        rt::memcpy(pucOut, pucVertex, uiCopySize);
        int16_t* psOut = reinterpret_cast<int16_t*>(pucOut + uiCopySize);
        for (uint16_t j = usCopyCount; j < usOutCount; j++) {
            *psOut++ = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

uint32_t NiD3DShaderDeclaration::Pack_UnsignedShortToShort(PackingParameters& kParams) {
    return PackToShort<uint16_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_USHORT1);
}

uint32_t NiD3DShaderDeclaration::Pack_ByteToShort(PackingParameters& kParams) {
    return PackToShort<int8_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_BYTE1);
}

uint32_t NiD3DShaderDeclaration::Pack_UnsignedByteToShort(PackingParameters& kParams) {
    return PackToShort<uint8_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_UBYTE1);
}

uint32_t NiD3DShaderDeclaration::Pack_FloatToUnsignedByte(PackingParameters& kParams) {
    return PackToUnsignedByte<float>(kParams, NiAdditionalGeometryData::AGD_NITYPE_FLOAT1);
}

uint32_t NiD3DShaderDeclaration::Pack_LongToUnsignedByte(PackingParameters& kParams) {
    return PackToUnsignedByte<int32_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_LONG1);
}

uint32_t NiD3DShaderDeclaration::Pack_UnsignedLongToUnsignedByte(PackingParameters& kParams) {
    return PackToUnsignedByte<uint32_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_ULONG1);
}

uint32_t NiD3DShaderDeclaration::Pack_ShortToUnsignedByte(PackingParameters& kParams) {
    return PackToUnsignedByte<int16_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_SHORT1);
}

uint32_t NiD3DShaderDeclaration::Pack_UnsignedShortToUnsignedByte(PackingParameters& kParams) {
    return PackToUnsignedByte<uint16_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_USHORT1);
}

uint32_t NiD3DShaderDeclaration::Pack_ByteToUnsignedByte(PackingParameters& kParams) {
    return PackToUnsignedByte<int8_t>(kParams, NiAdditionalGeometryData::AGD_NITYPE_BYTE1);
}

// Copies the bytes with memcpy, then clears the remaining outputs with memset.
uint32_t NiD3DShaderDeclaration::Pack_UnsignedByteToUnsignedByte(PackingParameters& kParams) {
    const uint8_t* pucIn = static_cast<const uint8_t*>(kParams.m_pvInData);
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_UBYTE1);
    uint16_t usCopyCount = usInCount > 4 ? 4 : usInCount;
    const uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const uint8_t* pucVertex = pusVertexMap ? pucIn + pusVertexMap[i] * kParams.m_uiInStride : pucIn;
        rt::memcpy(pucOut, pucVertex, usCopyCount);
        if (usCopyCount < 4) {
            std::memset(pucOut + usCopyCount, 0, static_cast<uint16_t>(4 - usCopyCount));
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Copies m_uiOutSize bytes of each vertex's input to the output; without input data every vertex is cleared.
// Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_Float_DropIns(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }
    if (pusVertexMap) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            rt::memcpy(pucOut, pucIn + pusVertexMap[i] * kParams.m_uiInStride, kParams.m_uiOutSize);
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
    }
    else {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            rt::memcpy(pucOut, pucIn, kParams.m_uiOutSize);
            pucIn += kParams.m_uiInStride;
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
    }
    return uiPacked;
}

// Copies each vertex's m_uiInSize input bytes and zeroes the output components the input lacks (pack type
// components minus input components); without input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_Float_ExtraOutsToZero(PackingParameters& kParams) {
    uint16_t usExtra = static_cast<uint16_t>(kParams.m_ePackType - kParams.m_uiDataType + 1);
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }
    if (pusVertexMap) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            rt::memcpy(pucOut, pucIn + pusVertexMap[i] * kParams.m_uiInStride, kParams.m_uiInSize);
            uint32_t* puiExtra = reinterpret_cast<uint32_t*>(pucOut + kParams.m_uiInSize);
            for (uint16_t j = 0; j < usExtra; j++) {
                puiExtra[j] = 0;
            }
            uiPacked += kParams.m_uiOutSize;
            pucOut += kParams.m_uiOutStride;
        }
    }
    else {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            rt::memcpy(pucOut, pucIn, kParams.m_uiInSize);
            uint32_t* puiExtra = reinterpret_cast<uint32_t*>(pucOut + kParams.m_uiInSize);
            for (uint16_t j = 0; j < usExtra; j++) {
                puiExtra[j] = 0;
            }
            uiPacked += kParams.m_uiOutSize;
            pucIn += kParams.m_uiInStride;
            pucOut += kParams.m_uiOutStride;
        }
    }
    return uiPacked;
}

// Copies each vertex's m_uiInSize input bytes; without input data every vertex is cleared. Sequential 8, 12 and
// 16-byte inputs are copied as dwords, other sizes with memcpy. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_FloatToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }
    if (pusVertexMap) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            rt::memcpy(pucOut, pucIn + pusVertexMap[i] * kParams.m_uiInStride, kParams.m_uiInSize);
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
        return uiPacked;
    }

    uint32_t uiOutSize = kParams.m_uiOutSize;
    uint32_t uiInSize = kParams.m_uiInSize;
    uint32_t uiOutStride = kParams.m_uiOutStride;
    uint32_t uiInStride = kParams.m_uiInStride;
    uint32_t uiCount = kParams.m_usCount;
    switch (uiInSize) {
        case 8:
            for (uint32_t i = 0; i < uiCount; i++) {
                const uint32_t* puiIn = reinterpret_cast<const uint32_t*>(pucIn);
                uint32_t* puiOut = reinterpret_cast<uint32_t*>(pucOut);
                puiOut[0] = puiIn[0];
                puiOut[1] = puiIn[1];
                pucIn += uiInStride;
                pucOut += uiOutStride;
                uiPacked += uiOutSize;
            }
            break;
        case 12:
            for (uint32_t i = 0; i < uiCount; i++) {
                const uint32_t* puiIn = reinterpret_cast<const uint32_t*>(pucIn);
                uint32_t* puiOut = reinterpret_cast<uint32_t*>(pucOut);
                puiOut[0] = puiIn[0];
                puiOut[1] = puiIn[1];
                puiOut[2] = puiIn[2];
                pucIn += uiInStride;
                pucOut += uiOutStride;
                uiPacked += uiOutSize;
            }
            break;
        case 16:
            for (uint32_t i = 0; i < uiCount; i++) {
                const uint32_t* puiIn = reinterpret_cast<const uint32_t*>(pucIn);
                uint32_t* puiOut = reinterpret_cast<uint32_t*>(pucOut);
                puiOut[0] = puiIn[0];
                puiOut[1] = puiIn[1];
                puiOut[2] = puiIn[2];
                puiOut[3] = puiIn[3];
                pucIn += uiInStride;
                pucOut += uiOutStride;
                uiPacked += uiOutSize;
            }
            break;
        default:
            for (uint32_t i = 0; i < kParams.m_usCount; i++) {
                rt::memcpy(pucOut, pucIn, kParams.m_uiInSize);
                pucOut += uiOutStride;
                uiPacked += uiOutSize;
                pucIn += uiInStride;
            }
            break;
    }
    return uiPacked;
}

// Converts signed 32-bit components to floats (x / 2^31, clamped below at -1). Output components past the input's
// are zero; without input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_LongToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_LONG1);
    uint16_t usOutCount = static_cast<uint16_t>(kParams.m_ePackType + 1);
    uint16_t usCount = usInCount > usOutCount ? usOutCount : usInCount;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const int32_t* piIn;
        if (pusVertexMap) {
            piIn = reinterpret_cast<const int32_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            piIn = reinterpret_cast<const int32_t*>(pucIn);
        }
        float* pfOut = reinterpret_cast<float*>(pucOut);
        uint16_t j = 0;
        for (; j < usCount; j++) {
            float fValue = static_cast<float>(piIn[j] * 4.656612873077393e-10);
            pfOut[j] = fValue < -1.0f ? -1.0f : fValue;
        }
        for (; j < usOutCount; j++) {
            reinterpret_cast<uint32_t*>(pfOut)[j] = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Converts unsigned 32-bit components to floats (x / 2^32). Output components past the input's are zero; without
// input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_UnsignedLongToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_ULONG1);
    uint16_t usOutCount = static_cast<uint16_t>(kParams.m_ePackType + 1);
    uint16_t usCount = usInCount > usOutCount ? usOutCount : usInCount;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const uint32_t* puiIn;
        if (pusVertexMap) {
            puiIn = reinterpret_cast<const uint32_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            puiIn = reinterpret_cast<const uint32_t*>(pucIn);
        }
        float* pfOut = reinterpret_cast<float*>(pucOut);
        uint16_t j = 0;
        for (; j < usCount; j++) {
            pfOut[j] = static_cast<float>(puiIn[j] * 2.3283064365386963e-10);
        }
        for (; j < usOutCount; j++) {
            reinterpret_cast<uint32_t*>(pfOut)[j] = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Converts signed 16-bit components to floats (x / 32767, clamped below at -1). Output components past the input's
// are zero; without input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_ShortToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_SHORT1);
    uint16_t usOutCount = static_cast<uint16_t>(kParams.m_ePackType + 1);
    uint16_t usCount = usInCount > usOutCount ? usOutCount : usInCount;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const int16_t* psIn;
        if (pusVertexMap) {
            psIn = reinterpret_cast<const int16_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            psIn = reinterpret_cast<const int16_t*>(pucIn);
        }
        float* pfOut = reinterpret_cast<float*>(pucOut);
        uint16_t j = 0;
        for (; j < usCount; j++) {
            float fValue = static_cast<float>(psIn[j] / 32767.0);
            pfOut[j] = fValue < -1.0f ? -1.0f : fValue;
        }
        for (; j < usOutCount; j++) {
            reinterpret_cast<uint32_t*>(pfOut)[j] = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Converts unsigned 16-bit components to floats (x / 65535). Output components past the input's are zero; without
// input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_UnsignedShortToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_USHORT1);
    uint16_t usOutCount = static_cast<uint16_t>(kParams.m_ePackType + 1);
    uint16_t usCount = usInCount > usOutCount ? usOutCount : usInCount;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const uint16_t* pusIn;
        if (pusVertexMap) {
            pusIn = reinterpret_cast<const uint16_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            pusIn = reinterpret_cast<const uint16_t*>(pucIn);
        }
        float* pfOut = reinterpret_cast<float*>(pucOut);
        uint16_t j = 0;
        for (; j < usCount; j++) {
            pfOut[j] = static_cast<float>(pusIn[j] / 65535.0);
        }
        for (; j < usOutCount; j++) {
            reinterpret_cast<uint32_t*>(pfOut)[j] = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Converts signed 8-bit components to floats (x / 127, clamped below at -1). Output components past the input's
// are zero; without input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_ByteToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_BYTE1);
    uint16_t usOutCount = static_cast<uint16_t>(kParams.m_ePackType + 1);
    uint16_t usCount = usInCount > usOutCount ? usOutCount : usInCount;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const int8_t* pcIn;
        if (pusVertexMap) {
            pcIn = reinterpret_cast<const int8_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            pcIn = reinterpret_cast<const int8_t*>(pucIn);
        }
        float* pfOut = reinterpret_cast<float*>(pucOut);
        uint16_t j = 0;
        for (; j < usCount; j++) {
            float fValue = static_cast<float>(pcIn[j] / 127.0);
            pfOut[j] = fValue < -1.0f ? -1.0f : fValue;
        }
        for (; j < usOutCount; j++) {
            reinterpret_cast<uint32_t*>(pfOut)[j] = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Converts unsigned 8-bit components to floats (x / 255). Output components past the input's are zero; without
// input data every vertex is cleared. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_UnsignedByteToFloat(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        return PackClearOutput(kParams);
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_UBYTE1);
    uint16_t usOutCount = static_cast<uint16_t>(kParams.m_ePackType + 1);
    uint16_t usCount = usInCount > usOutCount ? usOutCount : usInCount;
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const uint8_t* pucVertex = pusVertexMap ? pucIn + pusVertexMap[i] * kParams.m_uiInStride : pucIn;
        float* pfOut = reinterpret_cast<float*>(pucOut);
        uint16_t j = 0;
        for (; j < usCount; j++) {
            pfOut[j] = static_cast<float>(pucVertex[j] / 255.0);
        }
        for (; j < usOutCount; j++) {
            reinterpret_cast<uint32_t*>(pfOut)[j] = 0;
        }
        uiPacked += kParams.m_uiOutSize;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
    }
    return uiPacked;
}

// Packs float components (r, g, b and a, or alpha 1 for three-component input) as a D3DCOLOR, each scaled by 255
// and rounded in the FPU's mode; without input data every vertex gets 0xFFFFFFFF. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_FloatToUByteColor(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            uint32_t uiA = static_cast<uint32_t>(NiFastFloatToInt(255.0f));
            uint32_t uiR = static_cast<uint32_t>(NiFastFloatToInt(255.0f));
            uint32_t uiG = static_cast<uint32_t>(NiFastFloatToInt(255.0f));
            uint32_t uiB = static_cast<uint32_t>(NiFastFloatToInt(255.0f));
            *reinterpret_cast<uint32_t*>(pucOut) = (((uiA << 8 | uiR) << 8 | uiG) << 8) | uiB;
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
        return uiPacked;
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_FLOAT1);
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const float* pfIn;
        if (pusVertexMap) {
            pfIn = reinterpret_cast<const float*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            pfIn = reinterpret_cast<const float*>(pucIn);
        }
        float fAlpha = usInCount == 3 ? 1.0f : pfIn[3];
        uint32_t uiA = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(fAlpha * 255.0)));
        uint32_t uiR = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pfIn[0] * 255.0)));
        uint32_t uiG = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pfIn[1] * 255.0)));
        uint32_t uiB = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pfIn[2] * 255.0)));
        *reinterpret_cast<uint32_t*>(pucOut) = (((uiA << 8 | uiR) << 8 | uiG) << 8) | uiB;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
        uiPacked += kParams.m_uiOutSize;
    }
    return uiPacked;
}

// Packs signed byte components (r, g, b and a, or alpha 0xFF for three-component input) as a D3DCOLOR; without
// input data every vertex is filled with 0xFF. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_ByteToUByteColor(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            std::memset(pucOut, 0xFF, kParams.m_uiOutSize);
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
        return uiPacked;
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_BYTE1);
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const int8_t* pcIn;
        if (pusVertexMap) {
            pcIn = reinterpret_cast<const int8_t*>(pucIn + pusVertexMap[i] * kParams.m_uiInStride);
        }
        else {
            pcIn = reinterpret_cast<const int8_t*>(pucIn);
        }
        int8_t cAlpha = usInCount == 3 ? static_cast<int8_t>(-1) : pcIn[3];
        uint32_t uiColor = static_cast<uint32_t>(cAlpha) << 8 | (static_cast<uint32_t>(pcIn[0]) & 0xFF);
        uiColor = uiColor << 8 | (static_cast<uint32_t>(pcIn[1]) & 0xFF);
        uiColor = uiColor << 8 | (static_cast<uint32_t>(pcIn[2]) & 0xFF);
        *reinterpret_cast<uint32_t*>(pucOut) = uiColor;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
        uiPacked += kParams.m_uiOutSize;
    }
    return uiPacked;
}

// Packs unsigned byte components (r, g, b and a, or alpha 0xFF for three-component input) as a D3DCOLOR; without
// input data every vertex is filled with 0xFF. Returns the bytes packed.
uint32_t NiD3DShaderDeclaration::Pack_UnsignedByteToUByteColor(PackingParameters& kParams) {
    uint8_t* pucIn = static_cast<uint8_t*>(kParams.m_pvInData);
    uint16_t* pusVertexMap = kParams.m_pusVertexMap;
    uint8_t* pucOut = static_cast<uint8_t*>(kParams.m_pvOutData);
    uint32_t uiPacked = 0;
    if (!pucIn) {
        for (uint16_t i = 0; i < kParams.m_usCount; i++) {
            std::memset(pucOut, 0xFF, kParams.m_uiOutSize);
            pucOut += kParams.m_uiOutStride;
            uiPacked += kParams.m_uiOutSize;
        }
        return uiPacked;
    }

    uint16_t usInCount = PackInputCount(kParams, NiAdditionalGeometryData::AGD_NITYPE_UBYTE1);
    for (uint16_t i = 0; i < kParams.m_usCount; i++) {
        const uint8_t* pucVertex = pusVertexMap ? pucIn + pusVertexMap[i] * kParams.m_uiInStride : pucIn;
        uint8_t ucAlpha = usInCount == 3 ? 0xFF : pucVertex[3];
        uint32_t uiColor = static_cast<uint32_t>(ucAlpha) << 8 | pucVertex[0];
        uiColor = uiColor << 8 | pucVertex[1];
        uiColor = uiColor << 8 | pucVertex[2];
        *reinterpret_cast<uint32_t*>(pucOut) = uiColor;
        if (!pusVertexMap) {
            pucIn += kParams.m_uiInStride;
        }
        pucOut += kParams.m_uiOutStride;
        uiPacked += kParams.m_uiOutSize;
    }
    return uiPacked;
}

// Picks the default packer for packing NiAdditionalGeometryData type uiAGDType into output type eType; type 0 gets
// Pack_IllegalCombination. An unsupported pair leaves the entry unchanged.
void NiD3DShaderDeclaration::SetDefaultPackingFunction(
    NiShaderDeclaration::ShaderParameterType eType,
    uint32_t uiAGDType
) {
    NiPackerEntry* pkPackEntry = ms_pkPackFunctions->m_pBase[eType];
    if (uiAGDType == NiAdditionalGeometryData::AGD_NITYPE_INVALID) {
        PackFunction pfnIllegal = &NiD3DShaderDeclaration::Pack_IllegalCombination;
        pkPackEntry->m_kPackFunctions.SetAt(0, pfnIllegal);
        return;
    }

    PackFunction pfnPack;
    // The type is compared unsigned.
    if (static_cast<uint32_t>(eType) <= NiShaderDeclaration::SPTYPE_FLOAT4) {
        // uiIn is the input's float count less one, compared unsigned with the output's.
        uint32_t uiIn = uiAGDType - 1;
        switch (uiAGDType) {
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT1:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT2:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT3:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT4:
                if (uiIn == static_cast<uint32_t>(eType)) {
                    pfnPack = &NiD3DShaderDeclaration::Pack_FloatToFloat;
                }
                else if (uiIn < static_cast<uint32_t>(eType)) {
                    pfnPack = &NiD3DShaderDeclaration::Pack_Float_ExtraOutsToZero;
                }
                else {
                    pfnPack = &NiD3DShaderDeclaration::Pack_Float_DropIns;
                }
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_LONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG4: pfnPack = &NiD3DShaderDeclaration::Pack_LongToFloat; break;
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedLongToFloat;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_ShortToFloat;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedShortToFloat;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE4: pfnPack = &NiD3DShaderDeclaration::Pack_ByteToFloat; break;
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedByteToFloat;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND1:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND2:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND3:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND4:
                pfnPack = &NiD3DShaderDeclaration::Pack_Float_ExtraOutsToZero;
                break;
            default: return;
        }
    }
    else if (eType == NiShaderDeclaration::SPTYPE_UBYTECOLOR) {
        switch (uiAGDType) {
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT1:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT2:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG4:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG4:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT3:
                pfnPack = &NiD3DShaderDeclaration::Pack_IllegalCombination;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT3:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT4:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND1:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND2:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND3:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND4:
                pfnPack = &NiD3DShaderDeclaration::Pack_FloatToUByteColor;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_LONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG1:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedLongToUByteColor;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_ShortToUByteColor;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedShortToUByteColor;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE4:
                pfnPack = &NiD3DShaderDeclaration::Pack_ByteToUByteColor;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedByteToUByteColor;
                break;
            default: return;
        }
    }
    else if (eType == NiShaderDeclaration::SPTYPE_SHORT2 || eType == NiShaderDeclaration::SPTYPE_SHORT4) {
        switch (uiAGDType) {
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT1:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT2:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT3:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_FloatToShort;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_LONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG4: pfnPack = &NiD3DShaderDeclaration::Pack_LongToShort; break;
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedLongToShort;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_ShortToShort;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedShortToShort;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE4: pfnPack = &NiD3DShaderDeclaration::Pack_ByteToShort; break;
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedByteToShort;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND1:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND2:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND3:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND4:
                pfnPack = &NiD3DShaderDeclaration::Pack_IllegalCombination;
                break;
            default: return;
        }
    }
    else if (eType == NiShaderDeclaration::SPTYPE_UBYTE4) {
        switch (uiAGDType) {
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT1:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT2:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT3:
            case NiAdditionalGeometryData::AGD_NITYPE_FLOAT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_FloatToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_LONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_LONG4:
                pfnPack = &NiD3DShaderDeclaration::Pack_LongToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG1:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG2:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG3:
            case NiAdditionalGeometryData::AGD_NITYPE_ULONG4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedLongToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_SHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_ShortToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT1:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT2:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT3:
            case NiAdditionalGeometryData::AGD_NITYPE_USHORT4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedShortToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_BYTE4:
                pfnPack = &NiD3DShaderDeclaration::Pack_ByteToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE1:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE2:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE3:
            case NiAdditionalGeometryData::AGD_NITYPE_UBYTE4:
                pfnPack = &NiD3DShaderDeclaration::Pack_UnsignedByteToUnsignedByte;
                break;
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND1:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND2:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND3:
            case NiAdditionalGeometryData::AGD_NITYPE_BLEND4:
                pfnPack = &NiD3DShaderDeclaration::Pack_IllegalCombination;
                break;
            default: return;
        }
    }
    else {
        return;
    }

    if (pfnPack) {
        pkPackEntry->m_kPackFunctions.SetAt(uiAGDType, pfnPack);
    }
}
