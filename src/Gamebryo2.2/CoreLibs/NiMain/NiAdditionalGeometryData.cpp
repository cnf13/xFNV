#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

NiRTTI NiAdditionalGeometryData::ms_RTTI("NiAdditionalGeometryData", &NiObject::ms_RTTI);

void NiAdditionalGeometryData::NiAGDDataStream::LoadBinary(NiStream& kStream) {
    NiStreamLoadBinary(kStream, m_uiType);
    NiStreamLoadBinary(kStream, m_uiUnitSize);
    NiStreamLoadBinary(kStream, m_uiTotalSize);
    NiStreamLoadBinary(kStream, m_uiStride);
    NiStreamLoadBinary(kStream, m_uiBlockIndex);
    NiStreamLoadBinary(kStream, m_uiBlockOffset);
    NiStreamLoadBinary(kStream, m_uFlags);
}

void NiAdditionalGeometryData::NiAGDDataStream::SaveBinary(NiStream& kStream) {
    NiStreamSaveBinary(kStream, m_uiType);
    NiStreamSaveBinary(kStream, m_uiUnitSize);
    NiStreamSaveBinary(kStream, m_uiTotalSize);
    NiStreamSaveBinary(kStream, m_uiStride);
    NiStreamSaveBinary(kStream, m_uiBlockIndex);
    NiStreamSaveBinary(kStream, m_uiBlockOffset);
    NiStreamSaveBinary(kStream, m_uFlags);
}

void NiAdditionalGeometryData::NiAGDDataStream::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTPrimitiveArray<char*>* v4;
    char* v57;
    char* a3;
    switch (m_uiType) {
        case 0u: {
            char* ViewerString_9 = NiGetViewerString("        m_uiType", "AGD_NITYPE_INVALID");
            v4 = pkStrings;
            a3 = ViewerString_9;
            pkStrings->Add(a3);
            break;
        }
        case 1u: {
            char* v5 = NiGetViewerString("        m_uiType", "AGD_NITYPE_FLOAT1");
            v4 = pkStrings;
            a3 = v5;
            pkStrings->Add(a3);
            break;
        }
        case 2u: {
            char* v6 = NiGetViewerString("        m_uiType", "AGD_NITYPE_FLOAT2");
            v4 = pkStrings;
            a3 = v6;
            pkStrings->Add(a3);
            break;
        }
        case 3u: {
            char* v7 = NiGetViewerString("        m_uiType", "AGD_NITYPE_FLOAT3");
            v4 = pkStrings;
            a3 = v7;
            pkStrings->Add(a3);
            break;
        }
        case 4u: {
            char* v8 = NiGetViewerString("        m_uiType", "AGD_NITYPE_FLOAT4");
            v4 = pkStrings;
            a3 = v8;
            pkStrings->Add(a3);
            break;
        }
        case 5u: {
            char* v9 = NiGetViewerString("        m_uiType", "AGD_NITYPE_LONG1");
            v4 = pkStrings;
            a3 = v9;
            pkStrings->Add(a3);
            break;
        }
        case 6u: {
            char* v10 = NiGetViewerString("        m_uiType", "AGD_NITYPE_LONG2");
            v4 = pkStrings;
            a3 = v10;
            pkStrings->Add(a3);
            break;
        }
        case 7u: {
            char* v11 = NiGetViewerString("        m_uiType", "AGD_NITYPE_LONG3");
            v4 = pkStrings;
            a3 = v11;
            pkStrings->Add(a3);
            break;
        }
        case 8u: {
            char* v12 = NiGetViewerString("        m_uiType", "AGD_NITYPE_LONG4");
            v4 = pkStrings;
            a3 = v12;
            pkStrings->Add(a3);
            break;
        }
        case 9u: {
            char* v13 = NiGetViewerString("        m_uiType", "AGD_NITYPE_ULONG1");
            v4 = pkStrings;
            a3 = v13;
            pkStrings->Add(a3);
            break;
        }
        case 10u: {
            char* v14 = NiGetViewerString("        m_uiType", "AGD_NITYPE_ULONG2");
            v4 = pkStrings;
            a3 = v14;
            pkStrings->Add(a3);
            break;
        }
        case 11u: {
            char* v15 = NiGetViewerString("        m_uiType", "AGD_NITYPE_ULONG3");
            v4 = pkStrings;
            a3 = v15;
            pkStrings->Add(a3);
            break;
        }
        case 12u: {
            char* v16 = NiGetViewerString("        m_uiType", "AGD_NITYPE_ULONG4");
            v4 = pkStrings;
            a3 = v16;
            pkStrings->Add(a3);
            break;
        }
        case 13u: {
            char* v17 = NiGetViewerString("        m_uiType", "AGD_NITYPE_SHORT1");
            v4 = pkStrings;
            a3 = v17;
            pkStrings->Add(a3);
            break;
        }
        case 14u: {
            char* v18 = NiGetViewerString("        m_uiType", "AGD_NITYPE_SHORT2");
            v4 = pkStrings;
            a3 = v18;
            pkStrings->Add(a3);
            break;
        }
        case 15u: {
            char* v19 = NiGetViewerString("        m_uiType", "AGD_NITYPE_SHORT3");
            v4 = pkStrings;
            a3 = v19;
            pkStrings->Add(a3);
            break;
        }
        case 16u: {
            char* v20 = NiGetViewerString("        m_uiType", "AGD_NITYPE_SHORT4");
            v4 = pkStrings;
            a3 = v20;
            pkStrings->Add(a3);
            break;
        }
        case 17u: {
            char* v21 = NiGetViewerString("        m_uiType", "AGD_NITYPE_USHORT1");
            v4 = pkStrings;
            a3 = v21;
            pkStrings->Add(a3);
            break;
        }
        case 18u: {
            char* v22 = NiGetViewerString("        m_uiType", "AGD_NITYPE_USHORT2");
            v4 = pkStrings;
            a3 = v22;
            pkStrings->Add(a3);
            break;
        }
        case 19u: {
            char* v23 = NiGetViewerString("        m_uiType", "AGD_NITYPE_USHORT3");
            v4 = pkStrings;
            a3 = v23;
            pkStrings->Add(a3);
            break;
        }
        case 20u: {
            char* v24 = NiGetViewerString("        m_uiType", "AGD_NITYPE_USHORT4");
            v4 = pkStrings;
            a3 = v24;
            pkStrings->Add(a3);
            break;
        }
        case 21u: {
            char* v25 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BYTE1");
            v4 = pkStrings;
            a3 = v25;
            pkStrings->Add(a3);
            break;
        }
        case 22u: {
            char* v26 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BYTE2");
            v4 = pkStrings;
            a3 = v26;
            pkStrings->Add(a3);
            break;
        }
        case 23u: {
            char* v27 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BYTE3");
            v4 = pkStrings;
            a3 = v27;
            pkStrings->Add(a3);
            break;
        }
        case 24u: {
            char* v28 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BYTE4");
            v4 = pkStrings;
            a3 = v28;
            pkStrings->Add(a3);
            break;
        }
        case 25u: {
            char* v29 = NiGetViewerString("        m_uiType", "AGD_NITYPE_UBYTE1");
            v4 = pkStrings;
            a3 = v29;
            pkStrings->Add(a3);
            break;
        }
        case 26u: {
            char* v30 = NiGetViewerString("        m_uiType", "AGD_NITYPE_UBYTE2");
            v4 = pkStrings;
            a3 = v30;
            pkStrings->Add(a3);
            break;
        }
        case 27u: {
            char* v31 = NiGetViewerString("        m_uiType", "AGD_NITYPE_UBYTE3");
            v4 = pkStrings;
            a3 = v31;
            pkStrings->Add(a3);
            break;
        }
        case 28u: {
            char* v32 = NiGetViewerString("        m_uiType", "AGD_NITYPE_UBYTE4");
            v4 = pkStrings;
            a3 = v32;
            pkStrings->Add(a3);
            break;
        }
        case 29u: {
            char* v33 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BLEND1");
            v4 = pkStrings;
            a3 = v33;
            pkStrings->Add(a3);
            break;
        }
        case 30u: {
            char* v34 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BLEND2");
            v4 = pkStrings;
            a3 = v34;
            pkStrings->Add(a3);
            break;
        }
        case 31u: {
            char* v35 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BLEND3");
            v4 = pkStrings;
            a3 = v35;
            pkStrings->Add(a3);
            break;
        }
        case 32u: {
            char* v36 = NiGetViewerString("        m_uiType", "AGD_NITYPE_BLEND4");
            v4 = pkStrings;
            a3 = v36;
            pkStrings->Add(a3);
            break;
        }
        case 33u: {
            char* v37 = NiGetViewerString("        m_uiType", "AGD_NITYPE_COUNT");
            v4 = pkStrings;
            a3 = v37;
            pkStrings->Add(a3);
            break;
        }
        default: {
            char* v38 = NiGetViewerString("        m_uiType", "UNKNOWN!!!");
            v4 = pkStrings;
            uint32_t m_usSize = pkStrings->m_usSize;
            uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
            a3 = v38;
            if (m_usSize >= m_usMaxSize) {
                pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
            }
            v4->SetAt(m_usSize, a3);
            break;
        }
    }
    char* ViewerString_6 = NiGetViewerString("        m_uiUnitSize", m_uiUnitSize);
    uint32_t v42 = v4->m_usSize;
    uint32_t v43 = v4->m_usMaxSize;
    char* v61 = ViewerString_6;
    if (v42 >= v43) {
        v4->SetSize(v42 + v4->m_usGrowBy);
    }
    v4->SetAt(v42, v61);
    char* v44 = NiGetViewerString("        m_uiTotalSize", m_uiTotalSize);
    uint32_t v45 = v4->m_usSize;
    v61 = v44;
    if (v45 >= v4->m_usMaxSize) {
        v4->SetSize(v45 + v4->m_usGrowBy);
    }
    v4->SetAt(v45, v61);
    char* v46 = NiGetViewerString("        m_uiStride", m_uiStride);
    uint32_t v47 = v4->m_usSize;
    uint32_t v48 = v4->m_usMaxSize;
    v61 = v46;
    if (v47 >= v48) {
        v4->SetSize(v47 + v4->m_usGrowBy);
    }
    v4->SetAt(v47, v61);
    char* v49 = NiGetViewerString("        m_uiBlockIndex", m_uiBlockIndex);
    uint32_t v50 = v4->m_usSize;
    uint32_t v51 = v4->m_usMaxSize;
    v61 = v49;
    if (v50 >= v51) {
        v4->SetSize(v50 + v4->m_usGrowBy);
    }
    v4->SetAt(v50, v61);
    char* v52 = NiGetViewerString("        m_uiBlockOffset", m_uiBlockOffset);
    uint32_t v53 = v4->m_usSize;
    v61 = v52;
    if (v53 >= v4->m_usMaxSize) {
        v4->SetSize(v53 + v4->m_usGrowBy);
    }
    v4->SetAt(v53, v61);
    char* ViewerString_0 = NiGetViewerString("        Keep", (m_uFlags & 1) != 0);
    uint32_t v55 = v4->m_usSize;
    uint32_t v56 = v4->m_usMaxSize;
    v61 = ViewerString_0;
    if (v55 >= v56) {
        v4->SetSize(v55 + v4->m_usGrowBy);
    }
    v4->SetAt(v55, v61);
    if ((m_uFlags & 6) == 2) {
        v57 = NiGetViewerString("        Consistency", "MUTABLE");
    }
    else {
        v57 = NiGetViewerString("        Consistency", "STATIC");
    }
    uint32_t v58 = v4->m_usMaxSize;
    uint32_t v59 = v4->m_usSize;
    v61 = v57;
    if (v59 >= v58) {
        v4->SetSize(v59 + v4->m_usGrowBy);
    }
    v4->SetAt(v59, v61);
}

void NiAdditionalGeometryData::NiAGDDataStream::SetConsistency(int32_t eConsistency) {
    m_uFlags = (uint8_t)(((uint8_t)ConvertFromConsistency(eConsistency)) | (m_uFlags & 249));
}

uint8_t NiAdditionalGeometryData::NiAGDDataStream::ConvertFromConsistency(int32_t eConsistency) {
    if (eConsistency) {
        return 3;
    }
    else {
        return 2;
    }
}

void NiAdditionalGeometryData::NiAGDDataStream::FillInByteSizeArray(
    uint32_t* pauiDataArray,
    uint32_t& uiIndex,
    uint32_t uiArraySize
) {
    switch (m_uiType) {
        case 1u:
        case 5u:
        case 9u: {
            goto LABEL_5;
        }
        case 2u:
        case 6u:
        case 10u: {
            goto LABEL_4;
        }
        case 3u:
        case 7u:
        case 11u: {
            goto LABEL_3;
        }
        case 4u:
        case 8u:
        case 12u: {
            pauiDataArray[uiIndex++] = 4;
        LABEL_3:;
            pauiDataArray[uiIndex++] = 4;
        LABEL_4:;
            pauiDataArray[uiIndex++] = 4;
        LABEL_5:;
            pauiDataArray[uiIndex++] = 4;
            return;
        }
        case 13u:
        case 17u: {
            goto LABEL_9;
        }
        case 14u:
        case 18u: {
            goto LABEL_8;
        }
        case 15u:
        case 19u: {
            goto LABEL_7;
        }
        case 16u:
        case 20u: {
            pauiDataArray[uiIndex++] = 2;
        LABEL_7:;
            pauiDataArray[uiIndex++] = 2;
        LABEL_8:;
            pauiDataArray[uiIndex++] = 2;
        LABEL_9:;
            pauiDataArray[uiIndex++] = 2;
            return;
        }
        case 21u:
        case 25u:
        case 29u: {
            goto LABEL_13;
        }
        case 22u:
        case 26u:
        case 30u: {
            goto LABEL_12;
        }
        case 23u:
        case 27u:
        case 31u: {
            goto LABEL_11;
        }
        case 24u:
        case 28u:
        case 32u: {
            pauiDataArray[uiIndex++] = 1;
        LABEL_11:;
            pauiDataArray[uiIndex++] = 1;
        LABEL_12:;
            pauiDataArray[uiIndex++] = 1;
        LABEL_13:;
            pauiDataArray[uiIndex++] = 1;
            break;
        }
        default: {
            return;
        }
    }
}

NiAdditionalGeometryData::NiAGDDataBlock::NiAGDDataBlock()
  : m_uiDataBlockSize(0), m_pucDataBlock(nullptr), m_bOwnData(false) {}

// The buffer is not freed here.
NiAdditionalGeometryData::NiAGDDataBlock::~NiAGDDataBlock() {}

uint8_t* NiAdditionalGeometryData::NiAGDDataBlock::AllocateBuffer(uint32_t uiBufferSize) {
    if (!uiBufferSize) {
        return nullptr;
    }
    return static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(uiBufferSize));
}

// Without a buffer argument the block's own buffer is freed, if the block owns it.
void NiAdditionalGeometryData::NiAGDDataBlock::FreeBuffer(void* pvBuffer) {
    uint8_t* pucBuffer = static_cast<uint8_t*>(pvBuffer);
    if (!pvBuffer && m_bOwnData) {
        pucBuffer = m_pucDataBlock;
    }
    MemoryManager_Singleton.Deallocate(pucBuffer);
}

// The counts and offset tables are stream reads into uninitialised locals (uiNumRuns, the offsets, each run's component
// count and sizes), which a short read leaves unwritten and the body then uses, as the game does.
void NiAdditionalGeometryData::NiAGDDataBlock::LoadBinary(
    NiStream& kStream,
    uint32_t uiBlockIdx,
    NiAdditionalGeometryData::NiAGDDataStream* pkDataStreams,
    uint32_t uiNumStreams,
    uint16_t usVertexCount
) {
    uint32_t v25;
    uint32_t v26;
    int32_t v27;
    uint32_t v28;
    int32_t v32[32];
    NiBinaryStream* m_pkIstr = (&kStream)->Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = m_pkIstr->m_pfnRead;
    v25 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &this->m_uiDataBlockSize, 4, &v25, 1);
    uint32_t m_uiDataBlockSize = this->m_uiDataBlockSize;
    m_bBSHeap = usVertexCount < 64u;
    m_pucDataBlock = AllocateBuffer(m_uiDataBlockSize);
    m_bOwnData = 1;
    NiBinaryStream* v10 = (&kStream)->Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN v11 = v10->m_pfnRead;
    v25 = 4;
    v10->m_uiAbsoluteCurrentPos += v11(v10, &v28, 4, &v25, 1);
    NiBinaryStream* v12 = (&kStream)->Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN v13 = v12->m_pfnRead;
    v25 = 4;
    v12->m_uiAbsoluteCurrentPos += v13(v12, v32, 4 * v28, &v25, 1);
    v25 = 0;
    if (v28) {
        int32_t v30 = usVertexCount;
        do {
            uint32_t v23;
            uint32_t v31[32];
            NiBinaryStream* v14 = (&kStream)->Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN v15 = v14->m_pfnRead;
            int32_t v16 = 0;
            int32_t v29 = 0;
            v26 = 4;
            v14->m_uiAbsoluteCurrentPos += v15(v14, &v27, 4, &v26, 1);
            NiBinaryStream* v17 = (&kStream)->Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN v18 = v17->m_pfnRead;
            v26 = 4;
            v17->m_uiAbsoluteCurrentPos += v18(v17, v31, 4 * v27, &v26, 1);
            uint32_t v19 = v27;
            uint8_t* v20 = &m_pucDataBlock[v32[v25]];
            NiBinaryStream* v21 = (&kStream)->Istr();
            uint32_t v22 = 0;
            v26 = 0;
            if (v27 >= 2) {
                do {
                    v29 += v31[v22 + 1];
                    v16 += v31[v22];
                    v22 += 2;
                }
                while (v22 < v27 - 1);
                v19 = v27;
            }
            if (v22 >= v19) {
                v23 = v26;
            }
            else {
                v23 = v31[v22];
            }
            v21->m_uiAbsoluteCurrentPos += v21->m_pfnRead(v21, v20, v30 * (v23 + (v16 + v29)), v31, v19);
            ++v25;
        }
        while (v25 < v28);
    }
    ProtectBuffer(0, 0);
}

void NiAdditionalGeometryData::NiAGDDataBlock::SaveBinary(
    NiStream& kStream,
    uint32_t uiBlockIdx,
    NiAdditionalGeometryData::NiAGDDataStream* pkDataStreams,
    uint32_t uiNumStreams,
    uint16_t usVertexCount
) {
    uint32_t uiIndex;
    uint32_t v43;
    int32_t v44;
    NiAdditionalGeometryData::NiAGDDataStream* v47[32];
    uint32_t v49[32];
    int32_t v50[32];
    NiStream* v6 = &kStream;
    NiBinaryStream* m_pkOstr = (&kStream)->Ostr();
    uint32_t* p_m_uiDataBlockSize = &m_uiDataBlockSize;
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    uiIndex = 4;
    int32_t v9 = m_pfnWrite(m_pkOstr, p_m_uiDataBlockSize, 4, &uiIndex, 1);
    uint32_t v10 = uiNumStreams;
    m_pkOstr->m_uiAbsoluteCurrentPos += v9;
    uint32_t v11 = 0;
    uint32_t v42 = 0;
    if (uiNumStreams) {
        do {
            if (pkDataStreams->m_uiBlockIndex == uiBlockIdx) {
                v47[v11++] = pkDataStreams;
            }
            ++pkDataStreams;
            --v10;
        }
        while (v10);
        v42 = v11;
    }
    for (uint32_t i = 0; i < v11; ++i) {
        NiAdditionalGeometryData::NiAGDDataStream* v14 = v47[i];
        for (uint32_t j = 0; j < i; ++j) {
            NiAdditionalGeometryData::NiAGDDataStream* v16 = v47[j];
            if (v16->m_uiBlockOffset > v14->m_uiBlockOffset) {
                v47[j] = v14;
                v14 = v16;
            }
        }
        v47[i] = v14;
    }
    NiAdditionalGeometryData::NiAGDDataStream* v17 = v47[0];
    int32_t v18 = 1;
    v49[0] = 0;
    v50[0] = 0;
    v43 = 1;
    uint32_t v19 = 1;
    if (v11 > 1) {
        do {
            NiAdditionalGeometryData::NiAGDDataStream* v20 = v47[v19];
            uint32_t m_uiBlockOffset = v20->m_uiBlockOffset;
            if (m_uiBlockOffset > v17->m_uiBlockOffset + usVertexCount * v17->m_uiStride) {
                v49[v18] = v19;
                v50[v18++] = m_uiBlockOffset;
                v43 = v18;
                v17 = v20;
            }
            v11 = v42;
            ++v19;
        }
        while (v19 < v42);
        v6 = &kStream;
    }
    NiBinaryStream* v22 = v6->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v23 = v22->m_pfnWrite;
    v49[v18] = v11;
    uiIndex = 4;
    v22->m_uiAbsoluteCurrentPos += v23(v22, &v43, 4, &uiIndex, 1);
    NiBinaryStream* v24 = v6->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v25 = v24->m_pfnWrite;
    uiIndex = 4;
    v24->m_uiAbsoluteCurrentPos += v25(v24, v50, 4 * v43, &uiIndex, 1);
    uint32_t v26 = 0;
    v42 = 0;
    if (v43) {
        int32_t v46 = usVertexCount;
        while (1) {
            uint32_t pauiDataArray[32];
            uint32_t v27 = v49[v26];
            uint32_t v28 = v49[v26 + 1];
            uiIndex = 0;
            for (uint32_t k = v27; k < v28; ++k) {
                v47[k]->FillInByteSizeArray(pauiDataArray, uiIndex, 32u);
            }
            NiBinaryStream* v30 = v6->Ostr();
            v44 = uiIndex;
            NiBinaryStream::NIBINARYSTREAMWRITEFN v31 = v30->m_pfnWrite;
            int32_t v32 = 0;
            int32_t v33 = 0;
            uiIndex = 4;
            v30->m_uiAbsoluteCurrentPos += v31(v30, &v44, 4, &uiIndex, 1);
            NiBinaryStream* v34 = (&kStream)->Ostr();
            NiBinaryStream::NIBINARYSTREAMWRITEFN v35 = v34->m_pfnWrite;
            uiIndex = 4;
            v34->m_uiAbsoluteCurrentPos += v35(v34, pauiDataArray, 4 * v44, &uiIndex, 1);
            uint8_t* v36 = &this->m_pucDataBlock[v47[v27]->m_uiBlockOffset];
            NiBinaryStream* v37 = (&kStream)->Ostr();
            uint32_t v38 = 0;
            uint32_t v39 = 0;
            uiIndex = 0;
            if (v44 >= 2) {
                do {
                    v32 += pauiDataArray[v39];
                    v33 += pauiDataArray[v39 + 1];
                    v39 += 2;
                }
                while (v39 < v44 - 1);
                v38 = uiIndex;
            }
            if (v39 < v44) {
                v38 = pauiDataArray[v39];
            }
            v37->m_uiAbsoluteCurrentPos += v37->m_pfnWrite(v37, v36, v46 * (v38 + (v33 + v32)), pauiDataArray, v44);
            if (++v42 >= v43) {
                break;
            }
            v26 = v42;
            v6 = &kStream;
        }
    }
}

bool NiAdditionalGeometryData::NiAGDDataBlock::IsEqual(const NiAdditionalGeometryData::NiAGDDataBlock* pkOther) {
    if (m_uiDataBlockSize != pkOther->m_uiDataBlockSize) {
        return false;
    }
    return std::memcmp(m_pucDataBlock, pkOther->m_pucDataBlock, m_uiDataBlockSize) == 0;
}

void NiAdditionalGeometryData::NiAGDDataBlock::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    pkStrings->Add(NiGetViewerString("        m_uiDataBlockSize", m_uiDataBlockSize));
    pkStrings->Add(NiGetViewerString("        m_pucDataBlock", static_cast<const void*>(m_pucDataBlock)));
}

void NiAdditionalGeometryData::NiAGDDataBlock::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

NiAdditionalGeometryData::NiAdditionalGeometryData()
  : m_uiRevID(0), m_usVertexCount(0), m_uiDataStreamCount(0), m_pkDataStreamEntries(nullptr), m_uiD3DStream(0),
    m_aDataBlocks(0, 1) {}

NiAdditionalGeometryData::NiAdditionalGeometryData(uint16_t usVertexCount)
  : m_uiRevID(0), m_usVertexCount(usVertexCount), m_uiDataStreamCount(0), m_pkDataStreamEntries(nullptr),
    m_uiD3DStream(0), m_aDataBlocks(0, 1) {
    m_aDataBlocks.RemoveAll();
}

NiAdditionalGeometryData::NiAdditionalGeometryData(int32_t aiVertexCount, uint32_t uiDataStreamCount)
  : m_uiRevID(0), m_usVertexCount(static_cast<uint16_t>(aiVertexCount)), m_uiDataStreamCount(uiDataStreamCount),
    m_pkDataStreamEntries(nullptr), m_uiD3DStream(0), m_aDataBlocks(0, 1) {
    // The stream table comes from Bethesda's heap, which the destructor frees it to.
    void* pvMemory = MemoryManager_Singleton.Allocate(uiDataStreamCount * sizeof(NiAGDDataStream));
    if (pvMemory) {
        NiAGDDataStream* pkEntries = static_cast<NiAGDDataStream*>(pvMemory);
        for (uint32_t i = 0; i < uiDataStreamCount; ++i) {
            new (&pkEntries[i]) NiAGDDataStream();
        }
        m_pkDataStreamEntries = pkEntries;
    }
}

NiAdditionalGeometryData::NiAdditionalGeometryData(uint16_t usVertexCount, uint32_t uiDataStreamCount, int32_t a3)
  : m_uiRevID(0), m_usVertexCount(usVertexCount), m_uiDataStreamCount(uiDataStreamCount),
    m_pkDataStreamEntries(nullptr), m_uiD3DStream(a3), m_aDataBlocks(0, 1) {
    void* pvMemory = ourMalloc(uiDataStreamCount * sizeof(NiAGDDataStream));
    if (pvMemory) {
        NiAGDDataStream* pkEntries = static_cast<NiAGDDataStream*>(pvMemory);
        for (uint32_t i = 0; i < uiDataStreamCount; ++i) {
            new (&pkEntries[i]) NiAGDDataStream();
        }
        m_pkDataStreamEntries = pkEntries;
    }
    m_aDataBlocks.RemoveAll();
}

// The data block array's own destructor frees its buffer.
NiAdditionalGeometryData::~NiAdditionalGeometryData() {
    for (uint32_t i = 0; i < m_aDataBlocks.m_usSize; ++i) {
        RemoveDataBlock(i, true);
    }
    if (m_pkDataStreamEntries) {
        MemoryManager_Singleton.Deallocate(m_pkDataStreamEntries);
    }
}

// The stream table, then per block an exists flag and the block in the streams' layout. The stream table is
// allocated without a cookie and built through the CRT's vector constructor iterator.
void NiAdditionalGeometryData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_usVertexCount);
    NiStreamLoadBinary(kStream, m_uiDataStreamCount);
    if (m_uiDataStreamCount) {
        uint32_t uiCount = m_uiDataStreamCount;
        uint64_t ullSize = uint64_t{sizeof(NiAGDDataStream)} * uiCount;
        size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
        void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
        NiAGDDataStream* pkEntries = nullptr;
        if (pvMemory) {
            rt::eh_vector_constructor_iterator(
                pvMemory, sizeof(NiAGDDataStream), static_cast<int32_t>(uiCount),
                &fnvd::constructElement<NiAdditionalGeometryData::NiAGDDataStream>
            );
            pkEntries = static_cast<NiAGDDataStream*>(pvMemory);
        }
        m_pkDataStreamEntries = pkEntries;
        for (uint32_t i = 0; i < m_uiDataStreamCount; i++) {
            m_pkDataStreamEntries[i].LoadBinary(kStream);
        }
    }

    uint32_t uiNumBlocks = 0;
    NiStreamLoadBinary(kStream, uiNumBlocks);
    m_aDataBlocks.SetSize(uiNumBlocks);
    for (uint32_t i = 0; i < uiNumBlocks; i++) {
        bool bBlockExists;
        NiStreamLoadBinary(kStream, bBlockExists);
        if (bBlockExists) {
            NiAGDDataBlock* pkBlock = CreateAGDDataBlock();
            pkBlock->LoadBinary(kStream, i, m_pkDataStreamEntries, m_uiDataStreamCount, m_usVertexCount);
            m_aDataBlocks.SetAt(i, pkBlock);
        }
        else {
            m_aDataBlocks.SetAt(i, nullptr);
        }
    }
}

void NiAdditionalGeometryData::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiAdditionalGeometryData::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiAdditionalGeometryData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_usVertexCount);
    NiStreamSaveBinary(kStream, m_uiDataStreamCount);
    for (uint32_t i = 0; i < m_uiDataStreamCount; i++) {
        m_pkDataStreamEntries[i].SaveBinary(kStream);
    }

    uint32_t uiNumBlocks = m_aDataBlocks.GetSize();
    NiStreamSaveBinary(kStream, uiNumBlocks);
    for (uint32_t i = 0; i < m_aDataBlocks.GetSize(); i++) {
        NiAGDDataBlock* pkBlock = m_aDataBlocks.GetAt(i);
        bool bBlockExists = pkBlock != nullptr;
        NiStreamSaveBinary(kStream, bBlockExists);
        if (bBlockExists) {
            pkBlock->SaveBinary(kStream, i, m_pkDataStreamEntries, m_uiDataStreamCount, m_usVertexCount);
        }
    }
}

bool NiAdditionalGeometryData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiAdditionalGeometryData* pkOther = static_cast<NiAdditionalGeometryData*>(pkObject);
    if (m_usVertexCount != pkOther->m_usVertexCount || m_uiDataStreamCount != pkOther->m_uiDataStreamCount) {
        return false;
    }

    for (uint32_t i = 0; i < m_uiDataStreamCount; i++) {
        const NiAGDDataStream& kStream = m_pkDataStreamEntries[i];
        const NiAGDDataStream& kOtherStream = pkOther->m_pkDataStreamEntries[i];
        if (kStream.m_uiType != kOtherStream.m_uiType || kStream.m_uiUnitSize != kOtherStream.m_uiUnitSize ||
            kStream.m_uiTotalSize != kOtherStream.m_uiTotalSize || kStream.m_uiStride != kOtherStream.m_uiStride ||
            kStream.m_uiBlockIndex != kOtherStream.m_uiBlockIndex ||
            kStream.m_uiBlockOffset != kOtherStream.m_uiBlockOffset || kStream.m_uFlags != kOtherStream.m_uFlags) {
            return false;
        }
    }

    // Only this object's block count is walked.
    for (uint32_t i = 0; i < m_aDataBlocks.GetSize(); i++) {
        NiAGDDataBlock* pkBlock = m_aDataBlocks.GetAt(i);
        NiAGDDataBlock* pkOtherBlock = pkOther->m_aDataBlocks.GetAt(i);
        if (pkOtherBlock) {
            if (!pkBlock || !pkBlock->IsEqual(pkOtherBlock)) {
                return false;
            }
        }
        else if (pkBlock) {
            return false;
        }
    }
    return true;
}

void NiAdditionalGeometryData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_usVertexCount", m_usVertexCount));
    pkStrings->Add(NiGetViewerString("m_uiDataStreamCount", m_uiDataStreamCount));
    for (uint32_t i = 0; i < m_uiDataStreamCount; i++) {
        pkStrings->Add(NiGetViewerString("    DataStream Index", i));
        m_pkDataStreamEntries[i].GetViewerStrings(pkStrings);
    }

    pkStrings->Add(NiGetViewerString("m_aDataBlocks.GetSize()", m_aDataBlocks.GetSize()));
    for (uint32_t i = 0; i < m_aDataBlocks.GetSize(); i++) {
        pkStrings->Add(NiGetViewerString("    DataBlock Index", i));
        NiAGDDataBlock* pkBlock = m_aDataBlocks.GetAt(i);
        if (pkBlock) {
            pkBlock->GetViewerStrings(pkStrings);
        }
        else {
            pkStrings->Add(NiGetViewerString("        m_uiDataBlockSize", static_cast<int32_t>(0)));
            pkStrings->Add(NiGetViewerString("        m_pucDataBlock", "NULL"));
        }
    }
}

// The block is built with Bethesda's global new.
NiAdditionalGeometryData::NiAGDDataBlock* NiAdditionalGeometryData::CreateAGDDataBlock() {
    NiAdditionalGeometryData::NiAGDDataBlock* result = static_cast<NiAdditionalGeometryData::NiAGDDataBlock*>(
        MemoryManager_Singleton.Allocate(sizeof(NiAdditionalGeometryData::NiAGDDataBlock))
    );
    if (!result) {
        return 0;
    }
    return ::new (result) NiAdditionalGeometryData::NiAGDDataBlock;
}

void NiAdditionalGeometryData::SetDataBlockCount(uint32_t uiCount) {
    m_aDataBlocks.SetSize(uiCount);
}

void NiAdditionalGeometryData::SetDataStreamCount(uint32_t uiCount) {
    NiAdditionalGeometryData::NiAGDDataStream* m_pkDataStreamEntries = this->m_pkDataStreamEntries;
    NiAdditionalGeometryData::NiAGDDataStream* v4 =
        static_cast<NiAdditionalGeometryData::NiAGDDataStream*>(MemoryManager_Singleton.Allocate(
            uiCount <= SIZE_MAX / sizeof(NiAGDDataStream) ? sizeof(NiAGDDataStream) * uiCount : SIZE_MAX
        ));
    NiAdditionalGeometryData::NiAGDDataStream* v5 = v4;
    if (v4) {
        rt::eh_vector_constructor_iterator(
            v4, sizeof(NiAGDDataStream), static_cast<int32_t>(uiCount),
            &fnvd::constructElement<NiAdditionalGeometryData::NiAGDDataStream>
        );
    }
    else {
        v5 = 0;
    }
    this->m_pkDataStreamEntries = v5;
    if (m_pkDataStreamEntries) {
        rt::memcpy(v5, m_pkDataStreamEntries, sizeof(*v5) * m_uiDataStreamCount);
        MemoryManager_Singleton.Deallocate(m_pkDataStreamEntries);
    }
    m_uiDataStreamCount = uiCount;
}

bool NiAdditionalGeometryData::SetDataBlock(uint32_t uiIndex, uint8_t* pucData, uint32_t uiTotalSize, bool bCopyData) {
    NiAdditionalGeometryData::NiAGDDataBlock* v7;
    NiAdditionalGeometryData::NiAGDDataBlock* v11;
    if (uiIndex < m_aDataBlocks.m_usSize && (v7 = m_aDataBlocks.m_pBase[uiIndex], (v11 = v7) != 0)) {
        if (v7->m_uiDataBlockSize != uiTotalSize) {
            v7->FreeBuffer(0);
            v7->m_pucDataBlock = 0;
        }
    }
    else {
        v7 = CreateAGDDataBlock();
        v11 = v7;
        if (!v7) {
            return 0;
        }
    }
    if (!v7->m_pucDataBlock) {
        uint8_t* v9;
        if (bCopyData) {
            v7->m_bBSHeap = m_usVertexCount < 64u;
            v9 = v7->AllocateBuffer(uiTotalSize);
            if (v11) {
                rt::memcpy(v9, v11, uiTotalSize);
            }
            v7->ProtectBuffer(v9, 0);
        }
        else {
            v9 = pucData;
        }
        v7->m_uiDataBlockSize = uiTotalSize;
        v7->m_pucDataBlock = v9;
    }
    NiTPrimitiveArray<NiAdditionalGeometryData::NiAGDDataBlock*>* p_m_aDataBlocks = &m_aDataBlocks;
    if (uiIndex >= m_aDataBlocks.m_usMaxSize) {
        p_m_aDataBlocks->SetSize(uiIndex + m_aDataBlocks.m_usGrowBy);
    }
    p_m_aDataBlocks->SetAt(uiIndex, v11);
    v7->m_bOwnData = bCopyData;
    return 1;
}

bool NiAdditionalGeometryData::SetDataStream(
    uint32_t uiStreamIndex,
    uint32_t uiBlockIndex,
    uint32_t uiBlockOffset,
    uint32_t uiType,
    int32_t aiCount,
    uint32_t uiUnitSize,
    uint32_t uiStride
) {
    uint16_t usCount = static_cast<uint16_t>(aiCount);
    NiAdditionalGeometryData::NiAGDDataStream* m_pkDataStreamEntries;
    if (usCount != m_usVertexCount) {
        return (bool)0;
    }
    if (uiStreamIndex >= m_uiDataStreamCount) {
        return (bool)0;
    }
    m_pkDataStreamEntries = this->m_pkDataStreamEntries;
    if (!m_pkDataStreamEntries) {
        return (bool)0;
    }
    if (uiBlockIndex > ((uint32_t)((int32_t)m_aDataBlocks.m_usSize))) {
        return (bool)0;
    }
    NiAdditionalGeometryData::NiAGDDataStream* v10 = &m_pkDataStreamEntries[uiStreamIndex];
    v10->m_uiType = (uint32_t)uiType;
    v10->m_uiBlockIndex = uiBlockIndex;
    v10->m_uiBlockOffset = uiBlockOffset;
    v10->m_uiStride = uiStride;
    v10->m_uiUnitSize = uiUnitSize;
    v10->m_uiTotalSize = uiUnitSize * usCount;
    return (bool)1;
}

// The block's buffer, or null; uiBlockSize gets its size (0 without a block).
uint8_t* NiAdditionalGeometryData::LockDataBlock(uint32_t uiIndex, uint32_t& uiBlockSize) {
    uiBlockSize = 0;
    if (uiIndex >= m_aDataBlocks.m_usSize) {
        return nullptr;
    }
    NiAGDDataBlock* pkBlock = m_aDataBlocks.m_pBase[uiIndex];
    if (!pkBlock) {
        return nullptr;
    }
    uiBlockSize = pkBlock->m_uiDataBlockSize;
    pkBlock->ProtectBuffer(nullptr, 0);
    return pkBlock->m_pucDataBlock;
}

void NiAdditionalGeometryData::UnlockDataBlock(uint32_t uiIndex) {
    if (uiIndex < m_aDataBlocks.m_usSize) {
        NiAGDDataBlock* pkBlock = m_aDataBlocks.m_pBase[uiIndex];
        if (pkBlock) {
            pkBlock->UnprotectBuffer(nullptr, 0);
            ++m_uiRevID;
        }
    }
}

bool NiAdditionalGeometryData::GetDataStream(
    uint32_t uiStreamIndex,
    uint8_t*& pucData,
    uint32_t& uiType,
    uint16_t& usCount,
    uint32_t& uiTotalSize,
    uint32_t& uiUnitSize,
    uint32_t& uiStride
) {
    NiAdditionalGeometryData::NiAGDDataStream* m_pkDataStreamEntries;
    uint32_t m_uiType;
    uint32_t m_uiBlockIndex;
    uint32_t m_uiBlockOffset;
    if (uiStreamIndex >= m_uiDataStreamCount) {
        return (bool)0;
    }
    m_pkDataStreamEntries = this->m_pkDataStreamEntries;
    if (!m_pkDataStreamEntries) {
        return (bool)0;
    }
    m_uiType = m_pkDataStreamEntries[uiStreamIndex].m_uiType;
    NiAdditionalGeometryData::NiAGDDataStream* v11 = &m_pkDataStreamEntries[uiStreamIndex];
    uiType = m_uiType;
    if (!m_uiType) {
        return (bool)0;
    }
    usCount = m_usVertexCount;
    uiTotalSize = v11->m_uiTotalSize;
    uiUnitSize = v11->m_uiUnitSize;
    uiStride = v11->m_uiStride;
    m_uiBlockIndex = v11->m_uiBlockIndex;
    if (m_uiBlockIndex > ((uint32_t)((int32_t)m_aDataBlocks.m_usSize))) {
        return (bool)0;
    }
    NiAdditionalGeometryData::NiAGDDataBlock* v13 = m_aDataBlocks.m_pBase[m_uiBlockIndex];
    if (!v13) {
        return (bool)0;
    }
    m_uiBlockOffset = v11->m_uiBlockOffset;
    if (v13->m_uiDataBlockSize < m_uiBlockOffset) {
        return (bool)0;
    }
    pucData = &v13->m_pucDataBlock[m_uiBlockOffset];
    return (bool)1;
}

uint32_t NiAdditionalGeometryData::GetSize() const {
    return m_aDataBlocks.m_usSize;
}

const uint8_t* NiAdditionalGeometryData::GetDataBlock(uint32_t uiIndex, uint32_t& uiBlockSize) {
    uiBlockSize = (uint32_t)0;
    if (uiIndex >= ((uint32_t)((int32_t)m_aDataBlocks.m_usSize))) {
        return nullptr;
    }
    NiAdditionalGeometryData::NiAGDDataBlock* v4 = m_aDataBlocks.m_pBase[uiIndex];
    if (!v4) {
        return nullptr;
    }
    uiBlockSize = v4->m_uiDataBlockSize;
    return static_cast<const uint8_t*>(static_cast<const void*>(v4->m_pucDataBlock));
}

bool NiAdditionalGeometryData::RemoveDataBlock(uint32_t uiIndex, bool bRemoveAssociatedStreams) {
    if (uiIndex >= m_aDataBlocks.m_usSize) {
        return false;
    }
    NiAdditionalGeometryData::NiAGDDataBlock* pkBlock = m_aDataBlocks.m_pBase[uiIndex];
    if (!pkBlock) {
        return false;
    }
    pkBlock->FreeBuffer(nullptr);
    delete pkBlock;
    m_aDataBlocks.SetAt(uiIndex, nullptr);
    if (bRemoveAssociatedStreams) {
        uint32_t uiStreamCount = m_uiDataStreamCount;
        for (uint32_t i = 0; i < uiStreamCount; i++) {
            NiAdditionalGeometryData::NiAGDDataStream* pkEntries = m_pkDataStreamEntries;
            // This is SetDataStream(i, 0, ...) written out in place, and it deliberately keeps that function's own
            // bound and null tests, as the game does.
            if (uiIndex == pkEntries[i].m_uiBlockIndex && i < uiStreamCount) {
                if (pkEntries) {
                    pkEntries[i].m_uiType = 0;
                    pkEntries[i].m_uiTotalSize = 0;
                    pkEntries[i].m_uiUnitSize = 0;
                    pkEntries[i].m_uiStride = 0;
                    pkEntries[i].m_uiBlockIndex = 0;
                    pkEntries[i].m_uiBlockOffset = 0;
                }
            }
            uiStreamCount = m_uiDataStreamCount;
        }
    }
    return true;
}

// The constructor's own result is returned, so a failed allocation is the only null case.
NiObject* NiAdditionalGeometryData::CreateObject() {
    NiAdditionalGeometryData* pkObject =
        static_cast<NiAdditionalGeometryData*>(NiMemObject::operator new(sizeof(NiAdditionalGeometryData)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiAdditionalGeometryData;
}
