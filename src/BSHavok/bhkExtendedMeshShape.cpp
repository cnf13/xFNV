#include "BSHavok/bhkExtendedMeshShape.h"

#include "BSCore/MemoryManager.h"
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

#include <xmmintrin.h>

NiRTTI bhkExtendedMeshShape::ms_RTTI("bhkExtendedMeshShape", &bhkShapeCollection::ms_RTTI);
NiRTTI bhkExtendedMeshShapeData::ms_RTTI("bhkExtendedMeshShapeData", &NiObject::ms_RTTI);

bool bhkExtendedMeshShape::IsEqual(NiObject* apObject) {
    bool bResult = bhkShape::IsEqual(apObject);
    if (bResult) {
        // The exe reads the other shape's spData (+0x14) as apObject[2]'s reference count: the field itself here,
        // whose offset that NiObject arithmetic does not give here. bhkShape::IsEqual found the same class.
        return spData.m_pObject == static_cast<bhkExtendedMeshShape*>(apObject)->spData.m_pObject;
    }
    return bResult;
}

bhkExtendedMeshSubPart::bhkExtendedMeshSubPart()
  : uiNumVerts(0), pVertexBaseA(nullptr), uiNumTris(0), pIndexBaseA(nullptr), pMat(nullptr) {}

// bhkExtendedMeshSubPart::~bhkExtendedMeshSubPart, which the sub-part array cleanup runs on every entry: each block
// goes back to the heap under the Havok context.
bhkExtendedMeshSubPart::~bhkExtendedMeshSubPart() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    delete[] pVertexBaseA;
    delete[] pIndexBaseA;
    if (pMat) {
        MemoryManager::Instance().Deallocate(pMat);
    }
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
// indices, then the material's filter info and CRC into a new material block. The filter info and the CRC are read
// into locals that a short read leaves unwritten, and both are stored into the new material anyway.
void bhkExtendedMeshSubPart::LoadBinary(NiStream& arStream) {
    uint32_t uiFilterInfo;
    uint32_t uiMaterialCRC;
    uint32_t uiComponentSize;
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiBinaryStream* pIstr = arStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiNumVerts, 4, &uiComponentSize, 1);
    float* pVertices = new float[3 * uiNumVerts];
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
    uint16_t* pIndices = new uint16_t[3 * uiNumTris];
    uint32_t uiIndexCount = 3 * uiNumTris;
    pIndexBaseA = pIndices;
    pIstr = arStream.Istr();
    uiComponentSize = 2;
    pIstr->m_uiAbsoluteCurrentPos += pIstr->m_pfnRead(
        pIstr,
        pIndices,
        static_cast<uint32_t>(uiIndexCount * sizeof(uint16_t)),
        &uiComponentSize,
        1
    );
    pIstr = arStream.Istr();
    pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiFilterInfo, 4, &uiComponentSize, 1);
    pIstr = arStream.Istr();
    pfnRead = pIstr->m_pfnRead;
    uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiMaterialCRC, 4, &uiComponentSize, 1);
    // The block is raw memory, written as the material.
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(sizeof(hkpExtendedMaterial)));
    hkpExtendedMaterial* pMaterial = reinterpret_cast<hkpExtendedMaterial*>(pMemory);
    pMaterial->uiMaterialCRC = uiMaterialCRC;
    pMaterial->m_filterInfo = uiFilterInfo;
    pMat = pMaterial;
}

bhkExtendedMeshShapeData::bhkExtendedMeshShapeData() : uiNumSubParts(0), pSubDataA(nullptr) {}

// The sub parts are destroyed and freed under the Havok heap context.
bhkExtendedMeshShapeData::~bhkExtendedMeshShapeData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    delete[] pSubDataA;
}

const NiRTTI* bhkExtendedMeshShapeData::GetRTTI() const {
    return &bhkExtendedMeshShapeData::ms_RTTI;
}

// Reads the sub-part count, builds the sub-part array, then reads each sub-part, all under the Havok heap context.
void bhkExtendedMeshShapeData::LoadBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiBinaryStream* pIstr = arStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pIstr->m_pfnRead;
    uint32_t uiComponentSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pfnRead(pIstr, &uiNumSubParts, 4, &uiComponentSize, 1);
    pSubDataA = new bhkExtendedMeshSubPart[uiNumSubParts];
    for (uint32_t i = 0; i < uiNumSubParts; ++i) {
        pSubDataA[i].LoadBinary(arStream);
    }
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
    return static_cast<const void*>(apObject) == static_cast<const void*>(this);
}

NiObject* bhkExtendedMeshShapeData::CreateObject() {
    return new bhkExtendedMeshShapeData;
}

hkExtendedMeshShapeCinfo::hkExtendedMeshShapeCinfo() : spData(nullptr), fRadius(0.0f) {
    vScale.setAll(1.0f);
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
    for (hkpExtendedMeshShape::TrianglesSubpart& rSubpart : pShape->m_trianglesSubparts) {
        rSubpart.m_numTriangleShapes = 0;
    }
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkExtendedMeshShape::GetRTTI() const {
    return &bhkExtendedMeshShape::ms_RTTI;
}

NiObject* bhkExtendedMeshShape::CreateClone(NiCloningProcess& arCloneData) {
    // The clone is charged to the Havok heap context for the duration of the copy.
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkExtendedMeshShape* pClone = new bhkExtendedMeshShape;
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The shape builds its creation info before its base streams: the cinfo builder first, then the base's fields, then the
// radius and the scale read into the cinfo, then the link. The two reads each have their own component size.
void bhkExtendedMeshShape::LoadBinary(NiStream& arStream) {
    // bCreated is passed uninitialised; the builder writes it and nothing reads it.
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
    spData->RegisterStreamables(arStream);
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
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShapeCollection::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkExtendedMeshShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v8 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v8);
}

// Destroys and frees the creation info CreateCinfo made, under the Havok heap context.
void bhkExtendedMeshShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        hkSerializableCinfo* pInfo = this->pInfo;
        delete static_cast<hkExtendedMeshShapeCinfo*>(pInfo);
        this->pInfo = nullptr;
    }
}

uint32_t bhkExtendedMeshShape::QCinfoSize() const {
    return sizeof(hkExtendedMeshShapeCinfo);
}

// The creation info is read into a scratch block from the Havok heap context and dropped again: nothing is kept, and
// the size read is returned.
int32_t bhkExtendedMeshShape::LoadCInfo(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // Slot 42 (+0xA8), through the vtable.
    uint32_t uiCinfoSize = static_cast<const bhkSerializable*>(this)->QCinfoSize();
    void* pCinfo = MemoryManager::Instance().Allocate(uiCinfoSize);
    NiBinaryStream* pIstr = arStream.m_pkIstr;
    pIstr->BinaryRead(pCinfo, uiCinfoSize, nullptr, 0);
    MemoryManager::Instance().Deallocate(pCinfo);
    return static_cast<int32_t>(uiCinfoSize);
}

// Builds the Havok extended mesh shape from the creation info under the Havok heap context: one triangles subpart per
// streamed sub-part (its vertices, its 16-bit indices and its one material), then takes the info's data object, hands
// the shape to the wrapper (which keeps its own reference) and finishes the base's setup. The triangles subpart passed
// to addTrianglesSubpart is copied field by field into the shape's subpart array, m_triangleOffset included, which
// nothing writes; this is deliberate.
void bhkExtendedMeshShape::Init(const hkSerializableCinfo& arInfo) {
    const hkExtendedMeshShapeCinfo& rCinfo = static_cast<const hkExtendedMeshShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpExtendedMeshShape* pShape = new hkpExtendedMeshShape(hkConvexShapeDefaultRadius, 12);
    pShape->m_numBitsForSubpartIndex = 15;
    pShape->m_triangleRadius = rCinfo.fRadius;
    pShape->setScaling(rCinfo.vScale);
    bhkExtendedMeshShapeData* pData = rCinfo.spData;
    for (uint32_t i = 0; i < pData->uiNumSubParts; ++i) {
        const bhkExtendedMeshSubPart& rSubPart = pData->pSubDataA[i];
        hkpExtendedMeshShape::TrianglesSubpart kPart;
        kPart.m_vertexStriding = 3 * sizeof(float);
        kPart.m_vertexBase = rSubPart.pVertexBaseA;
        kPart.m_numVertices = static_cast<int32_t>(rSubPart.uiNumVerts);
        kPart.m_numTriangleShapes = static_cast<int32_t>(rSubPart.uiNumTris);
        kPart.m_stridingType = hkpExtendedMeshShape::INDICES_INT16;
        kPart.m_indexStriding = 3 * sizeof(uint16_t);
        kPart.m_indexBase = rSubPart.pIndexBaseA;
        kPart.m_numMaterials = 1;
        kPart.m_materialStriding = static_cast<int16_t>(sizeof(hkpExtendedMaterial));
        kPart.m_materialBase = rSubPart.pMat;
        kPart.m_materialIndexStriding = 1;
        kPart.m_materialIndexStridingType = hkpExtendedMeshShape::MATERIAL_INDICES_INT8;
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
}

// The shape's creation info: filled from the wrapped Havok object when there is one, else a new one from the Havok
// context, zeroed with a radius of 0 and a scale of one.
hkSerializableCinfo& bhkExtendedMeshShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = pInfo == nullptr;
    if (arCreated) {
        hkExtendedMeshShapeCinfo* pCinfo = new hkExtendedMeshShapeCinfo;
        // Reads the wrapped object before it stores the new info, then fills the info from it.
        bool bHasObject = phkObject != nullptr;
        pInfo = pCinfo;
        if (bHasObject) {
            CCBCE0(pCinfo);
        }
    }
    return *pInfo;
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
    return bhkShape::CanShare(arCloneData);
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
// its first vertex), a copy of its vertices and a material; a sub part without triangles only lowers the count. Then
// initialises the shape from an extended-mesh info holding the list, the radius and the scale. The index list is a
// local NiTPrimitiveArray<unsigned short>.
void bhkExtendedMeshShape::Init(const hkPackedNiTriStripsShapeCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkPackedNiTriStripsData* pSrcData = arInfo.spData.m_pObject;
    // Both counters are kept as 32-bit values and only their low 16 bits are used.
    uint32_t uiVertOffset = 0;
    uint32_t uiTri = 0;

    bhkExtendedMeshShapeData* pdata = new bhkExtendedMeshShapeData;

    pdata->pSubDataA = new bhkExtendedMeshSubPart[pSrcData->usPartCount];
    pdata->uiNumSubParts = pSrcData->usPartCount;

    {
        // The array starts with no slots and a grow-by of one.
        NiTPrimitiveArray<uint16_t> kIndices(0u, 1u);

        bhkExtendedMeshSubPart* currPart = pdata->pSubDataA;
        for (uint32_t ui = 0; static_cast<int32_t>(ui) < static_cast<int32_t>(pSrcData->usPartCount);
             ++ui, ++currPart) {
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
                    kIndices.Add(usIndex);
                }
                ++uiTri;
                ++uiNumTris;
            }

            uint32_t uiNumVerts = 0;
            if (uiNumTris > 0) {
                uiNumVerts = static_cast<uint16_t>(rSrcPart.uiNumVerts);
                // One vertex more than the sub part has, three floats each.
                uint32_t uiNumFloats = 3 * uiNumVerts + 3;
                float* pVerts = new float[uiNumFloats];
                int32_t iFirstVert = static_cast<uint16_t>(uiVertOffset);
                for (uint32_t uiVert = static_cast<uint32_t>(iFirstVert);
                     static_cast<uint16_t>(uiVert) < iFirstVert + static_cast<int32_t>(uiNumVerts);
                     ++uiVert) {
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
                uint16_t* pIndices = new uint16_t[uiNumIndices];
                rt::memcpy(pIndices, kIndices.m_pBase, uiNumIndices * sizeof(uint16_t));
                // The block is raw memory, written as the material.
                uint8_t* pMemory = static_cast<uint8_t*>(
                    MemoryManager::Instance().Allocate(sizeof(hkpExtendedMaterial))
                );
                hkpExtendedMaterial* pMat = reinterpret_cast<hkpExtendedMaterial*>(pMemory);
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

            kIndices.RemoveAll();
            uiVertOffset += uiNumVerts;
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
    }
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
    return new bhkExtendedMeshShape;
}
