#include "BSHavok/bhkStream.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiEndian.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

// Both calls swap the same words at the creation info, treated as raw words (hence the char* casts); the array's three
// words are read after the first one and written back after the second, being kept in locals across that call.
template <class T>
void bhkStream::SwapHKArray(hkSerializableCinfo* apCInfo, int32_t aiSize, hkArray<T>* apArray) {
    int32_t iCount = aiSize / 4;
    NiEndian::Swap32(reinterpret_cast<char*>(apCInfo), iCount);
    T* pData = apArray->m_data;
    uint32_t uiSize = apArray->m_size;
    uint32_t uiCapacityAndFlags = apArray->m_capacityAndFlags;
    NiEndian::Swap32(reinterpret_cast<char*>(apCInfo), iCount);
    apArray->m_data = pData;
    apArray->m_size = uiSize;
    apArray->m_capacityAndFlags = uiCapacityAndFlags;
}

template void bhkStream::SwapHKArray<hkVector4>(hkSerializableCinfo*, int32_t, hkArray<hkVector4>*);
template void
bhkStream::SwapHKArray<const hkpConvexShape*>(hkSerializableCinfo*, int32_t, hkArray<const hkpConvexShape*>*);
template void bhkStream::SwapHKArray<const hkpShape*>(hkSerializableCinfo*, int32_t, hkArray<const hkpShape*>*);
template void bhkStream::SwapHKArray<uint32_t>(hkSerializableCinfo*, int32_t, hkArray<uint32_t>*);
template void bhkStream::SwapHKArray<hkWorldObjectCinfo::Property>(
    hkSerializableCinfo*,
    int32_t,
    hkArray<hkWorldObjectCinfo::Property>*
);

// True when both arrays hold the same number of vectors and every pair is within 0.001 componentwise. The compare of
// the absolute difference is false for a NaN lane. The elements are read through the array's data pointer. The size
// compare and the loop bound are signed.
bool bhkStream::IsEqual(const hkArray<hkVector4>& arA, const hkArray<hkVector4>& arB) {
    if (arA.getSize() != arB.getSize()) {
        return false;
    }
    const int32_t iSize = arA.getSize();
    for (int32_t i = 0; i < iSize; ++i) {
        const hkQuadReal& kA = arA.m_data[i].m_quad;
        const hkQuadReal& kB = arB.m_data[i].m_quad;
        if (!(__builtin_fabsf(kA[0] - kB[0]) <= 0.001f) || !(__builtin_fabsf(kA[1] - kB[1]) <= 0.001f) ||
            !(__builtin_fabsf(kA[2] - kB[2]) <= 0.001f) || !(__builtin_fabsf(kA[3] - kB[3]) <= 0.001f)) {
            return false;
        }
    }
    return true;
}

// Each part is read as floats into locals and stored as one vector; translation and scale get w 0.0f. Every float is
// read into an uninitialised local, so a short read stores that local's garbage into the transform, as the game does.
void bhkStream::LoadBinary(NiStream& arStream, hkQsTransform& arTransf) {
    float fX;
    float fY;
    float fZ;
    float fW;
    NiStreamLoadBinary(arStream, fX);
    NiStreamLoadBinary(arStream, fY);
    NiStreamLoadBinary(arStream, fZ);
    // EXE-INLINED 0x4B4D50
    arTransf.m_translation.m_quad[0] = fX;
    arTransf.m_translation.m_quad[1] = fY;
    arTransf.m_translation.m_quad[2] = fZ;
    arTransf.m_translation.m_quad[3] = 0.0f;
    NiStreamLoadBinary(arStream, fX);
    NiStreamLoadBinary(arStream, fY);
    NiStreamLoadBinary(arStream, fZ);
    NiStreamLoadBinary(arStream, fW);
    // EXE-INLINED 0x4B4D50
    arTransf.m_rotation.m_vec.m_quad[0] = fX;
    arTransf.m_rotation.m_vec.m_quad[1] = fY;
    arTransf.m_rotation.m_vec.m_quad[2] = fZ;
    arTransf.m_rotation.m_vec.m_quad[3] = fW;
    NiStreamLoadBinary(arStream, fX);
    NiStreamLoadBinary(arStream, fY);
    NiStreamLoadBinary(arStream, fZ);
    // EXE-INLINED 0x4B4D50
    arTransf.m_scale.m_quad[0] = fX;
    arTransf.m_scale.m_quad[1] = fY;
    arTransf.m_scale.m_quad[2] = fZ;
    arTransf.m_scale.m_quad[3] = 0.0f;
}

// Each vector is copied to a 16-aligned local before it is written.
int32_t bhkStream::SaveBinary(NiStream& arStream, hkArray<hkVector4>& arArray) {
    alignas(16) int32_t iSize = arArray.m_size;
    NiStreamSaveBinary(arStream, iSize);
    int32_t iBytes = iSize * static_cast<int32_t>(sizeof(hkVector4));
    for (int32_t i = 0; i < iSize; ++i) {
        alignas(16) hkVector4 kValue = arArray.m_data[i];
        NiStreamSaveBinary(arStream, kValue);
    }
    return iBytes;
}

int32_t bhkStream::SaveBinary(NiStream& arStream, const hkpMoppCode* apMoppCode) {
    if (!apMoppCode) {
        return 0;
    }
    int32_t iSize = apMoppCode->m_data.m_size;
    NiStreamSaveBinary(arStream, iSize);
    alignas(16) hkVector4 kOffset = apMoppCode->m_info.m_offset;
    NiStreamSaveBinary(arStream, kOffset);
    if (iSize) {
        NiStreamSaveBinary(arStream, apMoppCode->m_data.m_data, iSize);
    }
    return iSize;
}

// A MOPP code read into a new hkpMoppCode under the Havok heap context: the byte count, the code info, for streams
// before version 6 a legacy 32-byte block (the count then includes 48 bytes for it, and a zero count ends the read),
// then the code bytes; a big-endian source has the info's four words swapped. The context is read through one TLS block
// pointer and restored through it.
int32_t bhkStream::LoadBinary(NiStream& arStream, hkpMoppCode*& arMoppCode) {
    // EXE-INLINED none: AutoMemContext
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iOldMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    hkpMoppCode* pMoppCode =
        static_cast<hkpMoppCode*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMoppCode)));
    pMoppCode->m_memSizeAndFlags = sizeof(hkpMoppCode);
    ::new (pMoppCode) hkpMoppCode;
    arMoppCode = pMoppCode;
    int32_t iSize = 0;
    NiStreamLoadBinary(arStream, iSize);
    arStream.m_pkIstr->BinaryRead(&arMoppCode->m_info, 0x10, nullptr, 0);
    if (arStream.m_BSStreamHeader.uiVersion < 6 && iSize) {
        iSize -= 0x30;
        uint8_t aucLegacy[0x20];
        uint32_t uiComponentSize = 4;
        arStream.m_pkIstr->BinaryRead(aucLegacy, 0x20, &uiComponentSize, 1);
    }
    if (iSize) {
        arMoppCode->m_data.setSize(iSize);
        arStream.m_pkIstr->BinaryRead(arMoppCode->m_data.m_data, iSize, nullptr, 0);
    }
    // Compares the flag byte with 1, and swaps the info's four floats as raw 32-bit words.
    if (arStream.m_bSourceIsLittleEndian != 1) {
        uint32_t* puiInfo = reinterpret_cast<uint32_t*>(&arMoppCode->m_info.m_offset.m_quad);
        puiInfo[0] = __builtin_bswap32(puiInfo[0]);
        puiInfo[1] = __builtin_bswap32(puiInfo[1]);
        puiInfo[2] = __builtin_bswap32(puiInfo[2]);
        puiInfo[3] = __builtin_bswap32(puiInfo[3]);
    }
    riMemContext = iOldMemContext;
    return iSize;
}

// Each element is written as one 4-byte component.
int32_t bhkStream::SaveBinary(NiStream& arStream, hkArray<uint32_t>& arArray) {
    int32_t iSize = arArray.m_size;
    NiStreamSaveBinary(arStream, iSize);
    int32_t iBytes = iSize * static_cast<int32_t>(sizeof(uint32_t));
    for (int32_t i = 0; i < iSize; ++i) {
        uint32_t uiValue = arArray.m_data[i];
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
            uiValue = __builtin_bswap32(uiValue);
        }
        NiStreamSaveBinary(arStream, uiValue);
    }
    return iBytes;
}

int32_t LoadUIntArray(NiStream& arStream, hkArray<uint32_t, hkContainerHeapAllocator>& arArray) {
    int32_t v19;
    uint32_t v21[4];
    uint32_t v22;
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    NiBinaryStream* m_pkIstr = (&arStream)->Istr();
    int32_t v4 = 0;
    v19 = 0;
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v21[3] = 0;
    v21[0] = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &v19, 4, v21, 1);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkArray<uint32_t, hkContainerHeapAllocator>* v7 = &arArray;
    hkMemoryAllocator* m_heap = Instance->m_heap;
    int32_t m_capacityAndFlags = arArray.m_capacityAndFlags;
    arArray.m_size = 0;
    if (m_capacityAndFlags >= 0) {
        m_heap->bufFree(v7->begin(), sizeof(uint32_t) * m_capacityAndFlags);
    }
    int32_t v10 = v19;
    v7->m_data = nullptr;
    v7->m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator* v11 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v12 = v7->m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
    if (v12 < v10) {
        int32_t v13 = 2 * v12;
        if (v10 >= v13) {
            v13 = v10;
        }
        hkArrayUtil::_reserve(*v11, v7, v13, sizeof(uint32_t));
    }
    bool v14 = v19 <= 0;
    int32_t result = 4 * v19;
    v7->m_size = v10;
    v21[0] = result;
    if (v14) {
        v2->eMemContext = eMemContext;
    }
    else {
        do {
            NiBinaryStream* v16 = (&arStream)->Istr();
            uint32_t* v18 = &v7->m_data[v4];
            uint32_t (*v17)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v16->m_pfnRead;
            v22 = 4;
            v16->m_uiAbsoluteCurrentPos += v17(v16, v18, 4, &v22, 1);
            ++v4;
        }
        while (v4 < v19);
        result = v21[0];
        v2->eMemContext = eMemContext;
    }
    return result;
}

// Reads the array's element count, frees its old buffer, sizes it to the count, then reads each vector, swapping its
// four words when the source is not little-endian, in Havok's heap context. Returns the count times 16.
int32_t bhkStream::LoadBinary(NiStream& arStream, hkArray<hkVector4>& arArray) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    NiBinaryStream* pIstr = arStream.Istr();
    uint32_t ielements = 0;
    NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pIstr->m_pfnRead;
    uint32_t uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &ielements, 4, &uiComponentSize, 1);
    arArray.clearAndDeallocate();
    arArray.setSize(static_cast<int32_t>(ielements));
    int32_t isize = static_cast<int32_t>(ielements * sizeof(hkVector4));
    for (uint32_t I = 0; I < ielements; ++I) {
        NiBinaryStream* pElementIstr = arStream.Istr();
        pElementIstr->m_uiAbsoluteCurrentPos +=
            pElementIstr->m_pfnRead(pElementIstr, &arArray.m_data[I], sizeof(hkVector4), nullptr, 0);
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
            bhkStream::Endian(arArray.m_data[I]);
        }
    }
    pTLSData->eMemContext = eMemContext;
    return isize;
}
