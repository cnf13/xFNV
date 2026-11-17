#include "BSHavok/bhkExtendedMeshShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkPackedNiTriStripsShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/hkPackedNiTriStripsShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "SDK/MSVC/ehvec.h"

#include <xmmintrin.h>

NiRTTI bhkExtendedMeshShape::ms_RTTI("bhkExtendedMeshShape", &bhkShapeCollection::ms_RTTI);
NiRTTI bhkExtendedMeshShapeData::ms_RTTI("bhkExtendedMeshShapeData", &NiObject::ms_RTTI);

bool bhkExtendedMeshShape::IsEqual(NiObject* apObject) {
    uint8_t result = (uint8_t)bhkShape::IsEqual(apObject);
    if (result) {
#if FNVD_64BIT
        // The exe reads the other shape's spData (+0x14) as apObject[2]'s reference count: the field itself here,
        // whose offset that NiObject arithmetic does not give on x64. bhkShape::IsEqual found the same class.
        return (bool)((uint8_t)(spData.m_pObject == static_cast<bhkExtendedMeshShape*>(apObject)->spData.m_pObject));
#else
        return (bool)((uint8_t)(spData.m_pObject ==
                                reinterpret_cast<bhkExtendedMeshShapeData*>(
                                    static_cast<uintptr_t>(apObject[2].m_uiRefCount)
                                )));
#endif
    }
    return (bool)result;
}

bhkExtendedMeshSubPart::bhkExtendedMeshSubPart()
  : uiNumVerts(0), pVertexBaseA(nullptr), uiNumTris(0), pIndexBaseA(nullptr), pMat(nullptr) {}

// bhkExtendedMeshSubPart::~bhkExtendedMeshSubPart, which the sub-part array cleanup runs on every entry: each block
// goes back to the heap under the Havok context, the material block through the aligned free (the padding length sits
// in the byte before it). Each block goes back to the heap under the Havok context, the material block through the
// aligned free (the padding length sits in the byte before it).
bhkExtendedMeshSubPart::~bhkExtendedMeshSubPart() {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    if (pVertexBaseA) {
        MemoryManager_Singleton.Deallocate(pVertexBaseA);
    }
    if (pIndexBaseA) {
        MemoryManager_Singleton.Deallocate(pIndexBaseA);
    }
    if (pMat) {
        // The aligned block is untyped memory: its padding length is the byte before the aligned pointer.
        uint8_t* pMatBlock = reinterpret_cast<uint8_t*>(pMat);
        MemoryManager_Singleton.Deallocate(pMatBlock - pMatBlock[-1]);
    }
    riMemContext = iSavedMemContext;
}

// Writes the sub-part: the vertex count and its 12-byte vertices, the triangle count and its 6-byte triangles, then
// the material block's first two dwords. Each write reloads the stream's output stream, and each has its own
// component size (four-byte values, the indices as 16-bit ones).
void bhkExtendedMeshSubPart::SaveBinary(NiStream& arStream) {
    uint32_t uiCountComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&uiNumVerts, 4, &uiCountComponentSize, 1);
    uint32_t uiVertexBytes = 12 * uiNumVerts;
    uint32_t uiVertexComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(pVertexBaseA, uiVertexBytes, &uiVertexComponentSize, 1);
    uint32_t uiTriangleCountComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&uiNumTris, 4, &uiTriangleCountComponentSize, 1);
    uint32_t uiIndexBytes = 6 * uiNumTris;
    uint32_t uiIndexComponentSize = 2;
    arStream.m_pkOstr->BinaryWrite(pIndexBaseA, uiIndexBytes, &uiIndexComponentSize, 1);
    uint32_t uiMatComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&pMat->m_filterInfo, 4, &uiMatComponentSize, 1);
    uint32_t uiMatCountComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&pMat->uiMaterialCRC, 4, &uiMatCountComponentSize, 1);
}

// Reads the sub-part under the Havok heap context: the vertex count and its vertices, the triangle count and its
// indices, then the material's filter info and CRC into a 16-aligned material block (the padding length in the byte
// before it, as the aligned free in Destruct expects). The filter info and the CRC are read into locals that a short
// read leaves unwritten, and both are stored into the new material anyway, as the game does.
void bhkExtendedMeshSubPart::LoadBinary(NiStream& arStream) {
    uint32_t uiFilterInfo;
    uint32_t uiMaterialCRC;
    uint32_t uiComponentSize;
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eSavedContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    NiBinaryStream* pIstr = arStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiNumVerts, 4, &uiComponentSize, 1);
    // Three floats per vertex; an oversized count turns into a request for 0xFFFFFFFF bytes.
    uint64_t ullVertexBytes = static_cast<uint64_t>(3 * uiNumVerts) * sizeof(float);
    float* pVertices = static_cast<float*>(MemoryManager_Singleton.Allocate(
        (ullVertexBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullVertexBytes)
    ));
    uint32_t uiVertexBytes = static_cast<uint32_t>(3 * uiNumVerts * sizeof(float));
    pVertexBaseA = pVertices;
    pIstr = arStream.Istr();
    pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, pVertices, uiVertexBytes, &uiComponentSize, 1);
    pIstr = arStream.Istr();
    pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiNumTris, 4, &uiComponentSize, 1);
    // Three 16-bit indices per triangle, with the same overflow guard.
    uint64_t ullIndexBytes = static_cast<uint64_t>(3 * uiNumTris) * sizeof(uint16_t);
    uint16_t* pIndices = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
        (ullIndexBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullIndexBytes)
    ));
    uint32_t uiIndexCount = 3 * uiNumTris;
    pIndexBaseA = pIndices;
    pIstr = arStream.Istr();
    uiComponentSize = 2;
    pIstr->m_uiAbsoluteCurrentPos +=
        pIstr->m_pfnRead(pIstr, pIndices, static_cast<uint32_t>(uiIndexCount * sizeof(uint16_t)), &uiComponentSize, 1);
    pIstr = arStream.Istr();
    pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiFilterInfo, 4, &uiComponentSize, 1);
    pIstr = arStream.Istr();
    pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiMaterialCRC, 4, &uiComponentSize, 1);
    // Aligned allocation: the block is treated as untyped memory, 16 bytes more than the material, aligned, and the
    // padding length is stored in the byte before the aligned pointer.
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkpExtendedMaterial) + 16));
    uint8_t uiPadding = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
    hkpExtendedMaterial* pMaterial = reinterpret_cast<hkpExtendedMaterial*>(pMemory + uiPadding);
    pMemory[uiPadding - 1] = uiPadding;
    pMaterial->uiMaterialCRC = uiMaterialCRC;
    pMaterial->m_filterInfo = uiFilterInfo;
    pMat = pMaterial;
    pTLSData->eMemContext = eSavedContext;
}

bhkExtendedMeshShapeData::bhkExtendedMeshShapeData() : uiNumSubParts(0), pSubDataA(nullptr) {}

// The sub parts are destroyed and freed under the Havok heap context.
bhkExtendedMeshShapeData::~bhkExtendedMeshShapeData() {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    delete[] pSubDataA;
    riMemContext = iSavedMemContext;
}

const NiRTTI* bhkExtendedMeshShapeData::GetRTTI() const {
    return &bhkExtendedMeshShapeData::ms_RTTI;
}

// Reads the sub-part count, builds the sub-part array, then reads each sub-part, all under the Havok heap context.
void bhkExtendedMeshShapeData::LoadBinary(NiStream& arStream) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eSavedContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    NiBinaryStream* pIstr = arStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pIstr->m_pfnRead;
    uint32_t uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiNumSubParts, 4, &uiComponentSize, 1);
    // Computes the size for new bhkExtendedMeshSubPart[uiCount], where an oversized count becomes a request for
    // 0xFFFFFFFF bytes, as the game does.
    uint32_t uiCount = uiNumSubParts;
    uint64_t ullEntryBytes = static_cast<uint64_t>(uiCount) * sizeof(bhkExtendedMeshSubPart);
    uint32_t uiEntryBytes = (ullEntryBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullEntryBytes);
    uint32_t uiTotalBytes = static_cast<uint32_t>(uiEntryBytes + sizeof(size_t));
    if (uiTotalBytes < uiEntryBytes) {
        uiTotalBytes = 0xFFFFFFFFu;
    }
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiTotalBytes));
    bhkExtendedMeshSubPart* pEntries;
    if (puiCookie) {
        *puiCookie = uiCount;
        // The entries follow the count in the untyped block; the constructor iterator builds them there.
        pEntries = reinterpret_cast<bhkExtendedMeshSubPart*>(puiCookie + 1);
        rt::eh_vector_constructor_iterator(
            pEntries, sizeof(bhkExtendedMeshSubPart), uiCount, &fnvd::constructElement<bhkExtendedMeshSubPart>,
            &fnvd::destructElement<bhkExtendedMeshSubPart>
        );
    }
    else {
        pEntries = nullptr;
    }
    pSubDataA = pEntries;
    for (uint32_t i = 0; i < uiNumSubParts; ++i) {
        pSubDataA[i].LoadBinary(arStream);
    }
    pTLSData->eMemContext = eSavedContext;
}

void bhkExtendedMeshShapeData::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
}

bool bhkExtendedMeshShapeData::RegisterStreamables(NiStream& arStream) {
    return NiObject::RegisterStreamables(arStream);
}

// Writes the sub-part count, then every sub-part; the count is tested before the loop and the compare is unsigned.
void bhkExtendedMeshShapeData::SaveBinary(NiStream& arStream) {
    uint32_t uiCountComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&uiNumSubParts, 4, &uiCountComponentSize, 1);
    uint32_t uiSubPart = 0;
    if (uiNumSubParts != 0) {
        do {
            pSubDataA[uiSubPart].SaveBinary(arStream);
            ++uiSubPart;
        }
        while (uiSubPart < uiNumSubParts);
    }
}

bool bhkExtendedMeshShapeData::IsEqual(NiObject* apObject) {
    return (bool)((uint8_t)(static_cast<const void*>(apObject) == static_cast<const void*>(this)));
}

NiObject* bhkExtendedMeshShapeData::CreateObject() {
    bhkExtendedMeshShapeData* pObject =
        static_cast<bhkExtendedMeshShapeData*>(NiMemObject::operator new(sizeof(bhkExtendedMeshShapeData)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) bhkExtendedMeshShapeData;
}

// Holds a reference on apData; hkShapeCinfo's constructor clears the user data.
hkExtendedMeshShapeCinfo::hkExtendedMeshShapeCinfo(bhkExtendedMeshShapeData* apData) : spData(apData) {
    if (apData) {
        NiIncRefCount(apData);
    }
}

// Releases the mesh.
hkExtendedMeshShapeCinfo::~hkExtendedMeshShapeCinfo() {
    if (spData) {
        NiDecRefCount(spData);
        spData = nullptr;
    }
}

bhkExtendedMeshShape::bhkExtendedMeshShape() {
    ++iCount;
}

// The wrapped Havok shape drops its triangle sub parts' triangle counts before it is killed; spData's own destructor
// then releases the sub-part list.
bhkExtendedMeshShape::~bhkExtendedMeshShape() {
    hkpExtendedMeshShape* pShape = static_cast<hkpExtendedMeshShape*>(phkObject);
    for (int32_t i = 0; i < pShape->m_trianglesSubparts.getSize(); ++i) {
        pShape->m_trianglesSubparts.m_data[i].m_numTriangleShapes = 0;
    }
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkExtendedMeshShape::GetRTTI() const {
    return &bhkExtendedMeshShape::ms_RTTI;
}

NiObject* bhkExtendedMeshShape::CreateClone(NiCloningProcess& arCloneData) {
    // The clone is charged to the Havok heap context for the duration of the copy.
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkExtendedMeshShape* pClone =
        static_cast<bhkExtendedMeshShape*>(NiMemObject::operator new(sizeof(bhkExtendedMeshShape)));
    if (pClone) {
        ::new (pClone) bhkExtendedMeshShape;
    }
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    riMemContext = iSavedMemContext;
    return pClone;
}

// The shape builds its creation info before its base streams: the cinfo builder first, then the base's fields, then the
// radius and the scale read into the cinfo, then the link. The two reads each have their own component size.
void bhkExtendedMeshShape::LoadBinary(NiStream& arStream) {
    // bCreated is passed uninitialised, as the game does; the builder writes it and nothing reads it.
    bool bCreated;
    // Slot 45 (+0xB4), through the vtable.
    hkSerializableCinfo* pRawCinfo = &static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated);
    bhkShape::LoadBinary(arStream);
    hkExtendedMeshShapeCinfo* pCinfo = static_cast<hkExtendedMeshShapeCinfo*>(pRawCinfo);
    uint32_t uiRadiusComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&pCinfo->fRadius, 4, &uiRadiusComponentSize, 1);
    uint32_t uiScaleComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&pCinfo->vScale, 0x10, &uiScaleComponentSize, 1);
    arStream.ReadLinkID();
}

// Links the data object the stream named into the creation info, then the base's links. The cinfo holds the data as a
// plain pointer: the old object is released, the new one stored and a reference taken on it.
void bhkExtendedMeshShape::LinkObject(NiStream& arStream) {
    bool bCreated;
    hkExtendedMeshShapeCinfo& rCinfo = static_cast<hkExtendedMeshShapeCinfo&>(CreateCinfo(bCreated));
    bhkExtendedMeshShapeData* pData = static_cast<bhkExtendedMeshShapeData*>(arStream.GetObjectFromLinkID());
    bhkExtendedMeshShapeData* pOld = rCinfo.spData;
    if (pOld != pData) {
        if (pOld) {
            pOld->DecRefCount();
        }
        rCinfo.spData = pData;
        if (pData) {
            pData->IncRefCount();
        }
    }
    bhkShape::LinkObject(arStream);
}

bool bhkExtendedMeshShape::RegisterStreamables(NiStream& arStream) {
    spData.m_pObject->RegisterStreamables(arStream);
    return bhkShape::RegisterStreamables(arStream);
}

// Builds the creation info (unused but for its flag), writes the base's data, then the wrapped shape's triangle
// radius and scaling and the sub-part data's link, and releases the info.
void bhkExtendedMeshShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    CreateCinfo(bCreated);
    hkpExtendedMeshShape* pShape = static_cast<hkpExtendedMeshShape*>(phkObject);
    bhkShape::SaveBinary(arStream);
    float fRadius = pShape->m_triangleRadius;
    uint32_t uiRadiusComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite((&fRadius), 4, &uiRadiusComponentSize, 1);
    uint32_t uiScaleComponentSize = 0x10;
    arStream.m_pkOstr->BinaryWrite(&pShape->m_scaling, 0x10, &uiScaleComponentSize, 1);
    arStream.SaveLinkID(spData.m_pObject);
    KillCinfo(bCreated);
}

void bhkExtendedMeshShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    bhkShapeCollection::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkExtendedMeshShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v8 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v8);
    v2->eMemContext = eMemContext;
}

// Destroys and frees the creation info CreateCinfo made, under the Havok heap context.
void bhkExtendedMeshShape::KillCinfo(bool abCreated) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    if (abCreated) {
        hkSerializableCinfo* pInfo = this->pInfo;
        if (pInfo) {
            hkExtendedMeshShapeCinfo* pCinfo = static_cast<hkExtendedMeshShapeCinfo*>(pInfo);
            pCinfo->~hkExtendedMeshShapeCinfo();
            // CreateCinfo aligned the block to 16 bytes; the byte before it holds the padding it was shifted by.
            // The allocation is raw memory: its padding byte is not part of the class.
            uint8_t* puiBlock = static_cast<uint8_t*>(static_cast<void*>(pCinfo));
            uint8_t uiPadding = puiBlock[-1];
            MemoryManager_Singleton.Deallocate(puiBlock - uiPadding);
        }
        this->pInfo = nullptr;
    }
    GetThreadMemContext() = iOldMemContext;
}

uint32_t bhkExtendedMeshShape::QCinfoSize() const {
    return sizeof(hkExtendedMeshShapeCinfo);
}

// The creation info is read into a scratch block from the Havok heap context and dropped again: nothing is kept, and
// the size read is returned.
int32_t bhkExtendedMeshShape::LoadCInfo(NiStream& arStream) {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    // Slot 42 (+0xA8), through the vtable.
    uint32_t uiCinfoSize = static_cast<const bhkSerializable*>(this)->QCinfoSize();
    void* pCinfo = MemoryManager_Singleton.Allocate(uiCinfoSize);
    NiBinaryStream* pIstr = arStream.m_pkIstr;
    pIstr->BinaryRead(pCinfo, uiCinfoSize, nullptr, 0);
    MemoryManager_Singleton.Deallocate(pCinfo);
    riMemContext = iSavedMemContext;
    return static_cast<int32_t>(uiCinfoSize);
}

// Builds the Havok extended mesh shape from the creation info under the Havok heap context: one triangles subpart per
// streamed sub-part (its vertices, its 16-bit indices and its one material), then takes the info's data object, hands
// the shape to the wrapper (which keeps its own reference) and finishes the base's setup. The triangles subpart passed
// to addTrianglesSubpart is copied field by field into the shape's subpart array, m_triangleOffset included, which
// nothing writes; this is deliberate, as the game does.
void bhkExtendedMeshShape::Init(const hkSerializableCinfo& arInfo) {
    const hkExtendedMeshShapeCinfo& rCinfo = static_cast<const hkExtendedMeshShapeCinfo&>(arInfo);
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eSavedContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    hkpExtendedMeshShape* pMemory =
        static_cast<hkpExtendedMeshShape*>(pRouter->m_heap->blockAlloc(sizeof(hkpExtendedMeshShape)));
    pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpExtendedMeshShape));
    hkpExtendedMeshShape* pShape = ::new (pMemory) hkpExtendedMeshShape(hkConvexShapeDefaultRadius, 12);
    pShape->m_numBitsForSubpartIndex = 15;
    pShape->m_triangleRadius = rCinfo.fRadius;
    pShape->setScaling(rCinfo.vScale);
    bhkExtendedMeshShapeData* pData = rCinfo.spData;
    for (uint32_t i = 0; i < pData->uiNumSubParts; ++i) {
        const bhkExtendedMeshSubPart& rSubPart = pData->pSubDataA[i];
        // Constructor defaults that are overwritten at once are left out; the stores below are the values the subpart
        // holds at the call.
        // EXE-INLINED none: hkpExtendedMeshShape::TrianglesSubpart::TrianglesSubpart
        hkpExtendedMeshShape::TrianglesSubpart kPart;
        // EXE-INLINED none: hkEnum<E, S>::operator=
        kPart.m_type.m_storage = static_cast<int8_t>(hkpExtendedMeshShape::SUBPART_TRIANGLES);
        kPart.m_materialIndexBase = nullptr;
        kPart.m_materialClass = nullptr;
        kPart.m_extrusion.setZero4();
        kPart.m_flipAlternateTriangles = 0;
        kPart.m_vertexStriding = 3 * sizeof(float);
        kPart.m_vertexBase = rSubPart.pVertexBaseA;
        kPart.m_numVertices = static_cast<int32_t>(rSubPart.uiNumVerts);
        kPart.m_numTriangleShapes = static_cast<int32_t>(rSubPart.uiNumTris);
        // EXE-INLINED none: hkEnum<E, S>::operator=
        kPart.m_stridingType.m_storage = static_cast<int8_t>(hkpExtendedMeshShape::INDICES_INT16);
        kPart.m_indexStriding = 3 * sizeof(uint16_t);
        kPart.m_indexBase = rSubPart.pIndexBaseA;
        kPart.m_numMaterials = 1;
        kPart.m_materialStriding = static_cast<int16_t>(sizeof(hkpExtendedMaterial));
        kPart.m_materialBase = rSubPart.pMat;
        kPart.m_materialIndexStriding = 1;
        // EXE-INLINED none: hkEnum<E, S>::operator=
        kPart.m_materialIndexStridingType.m_storage = static_cast<int8_t>(hkpExtendedMeshShape::MATERIAL_INDICES_INT8);
        kPart.m_userData = 0;
        pShape->addTrianglesSubpart(kPart);
    }
    // spData = rCinfo.spData: the old object is released first and the info's pointer read again after it.
    bhkExtendedMeshShapeData* pOld = spData.m_pObject;
    if (pOld != rCinfo.spData) {
        if (pOld) {
            pOld->DecRefCount();
        }
        bhkExtendedMeshShapeData* pNew = rCinfo.spData;
        spData.m_pObject = pNew;
        if (pNew) {
            pNew->IncRefCount();
        }
    }
    SethkObject(pShape);
    pShape->removeReference();
    PostInit(arInfo);
    pTLSData->eMemContext = eSavedContext;
}

// The shape's creation info: filled from the wrapped Havok object when there is one, else a new one from the Havok
// context, zeroed with a radius of 0 and a scale of one.
hkSerializableCinfo& bhkExtendedMeshShape::CreateCinfo(bool& arCreated) {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    arCreated = pInfo == nullptr;
    if (arCreated) {
        // The padding length goes in the byte before the aligned pointer. The block is raw memory that is aligned and
        // then written as the cinfo.
        uint8_t* pMemory =
            static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkExtendedMeshShapeCinfo) + 0x10));
        uint8_t uiPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        hkExtendedMeshShapeCinfo* pCinfo = reinterpret_cast<hkExtendedMeshShapeCinfo*>(pMemory + uiPadding);
        pMemory[uiPadding - 1] = uiPadding;
        pCinfo->m_userData = 0;
        pCinfo->spData = nullptr;
        pCinfo->fRadius = 0.0f;
        pCinfo->vScale.m_quad = hkQuadReal{1.0f, 1.0f, 1.0f, 1.0f};
        // Reads the wrapped object before it stores the new info, then fills the info from it.
        bool bHasObject = phkObject != nullptr;
        pInfo = pCinfo;
        if (bHasObject) {
            CCBCE0(pCinfo);
        }
    }
    hkSerializableCinfo* pResult = pInfo;
    riMemContext = iSavedMemContext;
    return *pResult;
}

// Scales the creation info's scale by the clone's scale, gives the clone this shape's data object, then the base's
// copy. The clone process is always a BSCloningProcess here; it reads Scale.x.
void bhkExtendedMeshShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkExtendedMeshShapeCinfo& rCinfo = static_cast<hkExtendedMeshShapeCinfo&>(CreateCinfo(bCreated));
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    __m128 kScale = _mm_set_ss(fScale);
    rCinfo.vScale.m_quad = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), rCinfo.vScale.m_quad);
    static_cast<bhkExtendedMeshShape*>(apDest)->spData = spData;
    bhkShape::CopyMembers(apDest, arCloneData);
}

bool bhkExtendedMeshShape::CanShare(NiCloningProcess& arCloneData) {
    return (bool)((int32_t)bhkShape::CanShare(arCloneData));
}

HavokMaterialType bhkExtendedMeshShape::QSubMaterial(uint32_t aiKey) {
    hkpExtendedMeshShape* phkObject;
    HavokMaterialType uimaterialCRC = BHK_MATERIAL_MAX;
    if (this) {
        phkObject = static_cast<hkpExtendedMeshShape*>(this->phkObject);
    }
    else {
        phkObject = nullptr;
    }
    if (aiKey != -1) {
        return static_cast<HavokMaterialType>(phkObject->getMeshMaterial(aiKey)[1].m_filterInfo);
    }
    return uimaterialCRC;
}

// Builds the sub-part list from a packed triangle-strip info: one sub part per source sub part, each holding the
// triangles that follow in the source list while their first vertex lies inside the sub part (their indices rebased to
// its first vertex), a copy of its vertices and a 16-aligned material; a sub part without triangles only lowers the
// count. Then initialises the shape from an extended-mesh info holding the list, the radius and the scale. The index
// list is a local NiTPrimitiveArray<unsigned short>.
void bhkExtendedMeshShape::Init(const hkPackedNiTriStripsShapeCinfo& arInfo) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eSavedMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    hkPackedNiTriStripsData* pSrcData = arInfo.spData.m_pObject;
    // Both counters are kept as 32-bit values and only their low 16 bits are used.
    uint32_t uiVertOffset = 0;
    uint32_t uiTri = 0;

    bhkExtendedMeshShapeData* pdata =
        static_cast<bhkExtendedMeshShapeData*>(NiMemObject::operator new(sizeof(bhkExtendedMeshShapeData)));
    if (pdata) {
        ::new (pdata) bhkExtendedMeshShapeData;
    }

    // new bhkExtendedMeshSubPart[usPartCount]: the count is stored before the array, and the byte count is saturated as
    // operator new[] does. The block is raw memory written as the count and the array after it.
    uint32_t uiNumParts = pSrcData->usPartCount;
    size_t uiPartBytes =
        uiNumParts > SIZE_MAX / sizeof(bhkExtendedMeshSubPart) ? SIZE_MAX : uiNumParts * sizeof(bhkExtendedMeshSubPart);
    uiPartBytes = uiPartBytes + sizeof(size_t) < uiPartBytes ? SIZE_MAX : uiPartBytes + sizeof(size_t);
    size_t* pCount = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiPartBytes));
    bhkExtendedMeshSubPart* pParts = nullptr;
    if (pCount) {
        *pCount = uiNumParts;
        pParts = reinterpret_cast<bhkExtendedMeshSubPart*>(pCount + 1);
        rt::eh_vector_constructor_iterator(
            pParts, sizeof(bhkExtendedMeshSubPart), static_cast<int32_t>(uiNumParts),
            &fnvd::constructElement<bhkExtendedMeshSubPart>, &fnvd::destructElement<bhkExtendedMeshSubPart>
        );
    }
    pdata->pSubDataA = pParts;
    pdata->uiNumSubParts = pSrcData->usPartCount;

    // The array starts with no slots and a grow-by of one.
    FNVD_INLINED_LOCAL(NiTPrimitiveArray<uint16_t>, kIndices, std::nullopt, std::nullopt);
    ::new (&kIndices) NiTPrimitiveArray<uint16_t>(0u, 1u);

    bhkExtendedMeshSubPart* currPart = pParts;
    for (uint32_t ui = 0; static_cast<int32_t>(ui) < static_cast<int32_t>(pSrcData->usPartCount); ++ui, ++currPart) {
        const SubPartData& rSrcPart = pSrcData->paSubPartData[ui];
        uint32_t uiNumTris = 0;
        while (static_cast<uint16_t>(uiTri) < static_cast<uint16_t>(pSrcData->uiKeyCount)) {
            const TriangleData& rTri = pSrcData->paTriData[static_cast<uint16_t>(uiTri)];
            if (rTri.us1 >= static_cast<uint16_t>(uiVertOffset) + rSrcPart.uiNumVerts) {
                break;
            }
            const uint16_t ausIndex[3] = {
                static_cast<uint16_t>(rTri.us1 - uiVertOffset),
                static_cast<uint16_t>(rTri.us2 - uiVertOffset),
                static_cast<uint16_t>(rTri.us3 - uiVertOffset),
            };
            for (uint16_t usIndex : ausIndex) {
                // EXE-INLINED none: NiTArray<unsigned short, NiTMallocInterface<unsigned short>>::Add
                uint32_t uiAt = kIndices.m_usSize;
                if (uiAt >= kIndices.m_usMaxSize) {
                    kIndices.SetSize(uiAt + kIndices.m_usGrowBy);
                }
                // EXE-INLINED 0xA5B870: NiTArray<unsigned short, NiTMallocInterface<unsigned short>>::SetAt
                if (uiAt >= kIndices.m_usSize) {
                    kIndices.m_usSize = static_cast<uint16_t>(uiAt + 1);
                    if (usIndex) {
                        ++kIndices.m_usESize;
                    }
                }
                else if (usIndex) {
                    if (!kIndices.m_pBase[uiAt]) {
                        ++kIndices.m_usESize;
                    }
                }
                else if (kIndices.m_pBase[uiAt]) {
                    --kIndices.m_usESize;
                }
                kIndices.m_pBase[uiAt] = usIndex;
            }
            ++uiTri;
            ++uiNumTris;
        }

        uint32_t uiNumVerts = 0;
        if (uiNumTris > 0) {
            uiNumVerts = static_cast<uint16_t>(rSrcPart.uiNumVerts);
            // One vertex more than the sub part has, three floats each; the byte count saturates as above.
            uint32_t uiNumFloats = 3 * uiNumVerts + 3;
            float* pVerts = static_cast<float*>(MemoryManager_Singleton.Allocate(
                uiNumFloats > SIZE_MAX / sizeof(float) ? SIZE_MAX : uiNumFloats * sizeof(float)
            ));
            int32_t iFirstVert = static_cast<uint16_t>(uiVertOffset);
            for (uint32_t uiVert = static_cast<uint32_t>(iFirstVert);
                 static_cast<uint16_t>(uiVert) < iFirstVert + static_cast<int32_t>(uiNumVerts); ++uiVert) {
                uint16_t usVert = static_cast<uint16_t>(uiVert);
                float* pDest = pVerts + 3 * (usVert - iFirstVert);
                float fComponent = pSrcData->paVertData[usVert].x;
                pDest[0] = fComponent;
                fComponent = pSrcData->paVertData[usVert].y;
                pDest[1] = fComponent;
                fComponent = pSrcData->paVertData[usVert].z;
                pDest[2] = fComponent;
            }
            uint32_t uiNumIndices = kIndices.m_usSize;
            uint16_t* pIndices = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
                uiNumIndices > SIZE_MAX / sizeof(uint16_t) ? SIZE_MAX : uiNumIndices * sizeof(uint16_t)
            ));
            rt::memcpy(pIndices, kIndices.m_pBase, uiNumIndices * sizeof(uint16_t));
            // EXE-INLINED 0xC74EA0: MemoryManager::AlignedAllocate16
            // The block is raw memory the exe aligns and then writes as the material.
            uint8_t* pMemory =
                static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkpExtendedMaterial) + 0x10));
            uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
            pMemory[ucPadding - 1] = ucPadding;
            hkpExtendedMaterial* pMat = reinterpret_cast<hkpExtendedMaterial*>(pMemory + ucPadding);
            const SubPartData& rMatPart = pSrcData->paSubPartData[ui];
            pMat->m_filterInfo = rMatPart.uiFilter;
            pMat->uiMaterialCRC = static_cast<uint32_t>(rMatPart.uiMaterial);
            currPart->pMat = pMat;
            currPart->uiNumVerts = uiNumVerts;
            currPart->pVertexBaseA = pVerts;
            currPart->pIndexBaseA = pIndices;
            currPart->uiNumTris = uiNumTris;
        }
        else {
            --pdata->uiNumSubParts;
        }

        // EXE-INLINED none: NiTArray<unsigned short, NiTMallocInterface<unsigned short>>::RemoveAll
        for (uint32_t i = 0; i < kIndices.m_usSize; ++i) {
            kIndices.m_pBase[i] = 0;
        }
        uiVertOffset += uiNumVerts;
        kIndices.m_usSize = 0;
        kIndices.m_usESize = 0;
    }

    {
        // cinfo is typed storage for which C++ runs neither the constructor nor the destructor.
        hkExtendedMeshShapeCinfo cinfo(pdata);
        float fRadius = arInfo.fRadius;
        cinfo.fRadius = fRadius;
        cinfo.vScale.m_quad = arInfo.vScale.m_quad;
        // Slot 44 (+0xB0), through the vtable.
        Init(static_cast<const hkSerializableCinfo&>(cinfo));
    }
    // The destructor, which frees the buffer, runs before the memory context is restored.
    FNVD_DESTRUCT(NiTArray<uint16_t, NiTMallocInterface<uint16_t>>)(&kIndices);
    pTLSData->eMemContext = eSavedMemContext;
}

void bhkExtendedMeshShape::CCBCE0(hkExtendedMeshShapeCinfo* apInfo) {
    hkpExtendedMeshShape* phkObject;
    CopyCinfo(*apInfo);
    if (this) {
        phkObject = static_cast<hkpExtendedMeshShape*>(this->phkObject);
    }
    else {
        phkObject = nullptr;
    }
    bhkExtendedMeshShapeData* spData = apInfo->spData;
    if (spData != this->spData.m_pObject) {
        if (spData) {
            NiDecRefCount(spData);
        }
        bhkExtendedMeshShapeData* m_pObject = this->spData.m_pObject;
        apInfo->spData = m_pObject;
        if (m_pObject) {
            NiIncRefCount(m_pObject);
        }
    }
    apInfo->fRadius = phkObject->m_triangleRadius;
    apInfo->vScale.m_quad = phkObject->m_scaling.m_quad;
}

NiObject* bhkExtendedMeshShape::CreateObject() {
    bhkExtendedMeshShape* pObject =
        static_cast<bhkExtendedMeshShape*>(NiMemObject::operator new(sizeof(bhkExtendedMeshShape)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) bhkExtendedMeshShape;
}
