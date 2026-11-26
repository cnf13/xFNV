#include "BSHavok/bhkPackedNiTriStripsShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/hkPackedNiTriStripsShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkPackedNiTriStripsShape::ms_RTTI("bhkPackedNiTriStripsShape", &bhkShapeCollection::ms_RTTI);

bhkPackedNiTriStripsShape::bhkPackedNiTriStripsShape() {
    ++iCount;
}

bhkPackedNiTriStripsShape::~bhkPackedNiTriStripsShape() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkPackedNiTriStripsShape::GetRTTI() const {
    return &bhkPackedNiTriStripsShape::ms_RTTI;
}

NiObject* bhkPackedNiTriStripsShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkPackedNiTriStripsShape* pClone = new bhkPackedNiTriStripsShape;
    // Called through the base so that the class's CopyMembers is reached by virtual dispatch.
    static_cast<bhkShape*>(this)->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The cinfo through its slot, the shape's base data, then the radius and the scale read into the cinfo, then the link.
// Each of the two reads uses its own component size.
void bhkPackedNiTriStripsShape::LoadBinary(NiStream& arStream) {
    // bCreated is passed uninitialised; the builder writes it and nothing reads it.
    bool bCreated;
    // Slot 45 (+0xB4), through the vtable.
    hkSerializableCinfo* pRawCinfo = &static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated);
    bhkShape::LoadBinary(arStream);
    hkPackedNiTriStripsShapeCinfo* pCinfo = static_cast<hkPackedNiTriStripsShapeCinfo*>(pRawCinfo);
    uint32_t uiRadiusComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&pCinfo->fRadius, 4, &uiRadiusComponentSize, 1);
    uint32_t uiScaleComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&pCinfo->vScale, 0x10, &uiScaleComponentSize, 1);
    arStream.ReadLinkID();
}

// Links the data object the stream named into the creation info, then the base's links. The cinfo's data pointer is
// loaded once: the old object is released, the new one stored and a reference taken on it.
void bhkPackedNiTriStripsShape::LinkObject(NiStream& arStream) {
    bool bCreated;
    hkPackedNiTriStripsShapeCinfo& rCinfo = static_cast<hkPackedNiTriStripsShapeCinfo&>(CreateCinfo(bCreated));
    hkPackedNiTriStripsData* pData = static_cast<hkPackedNiTriStripsData*>(arStream.GetObjectFromLinkID());
    rCinfo.spData = pData;
    bhkShapeCollection::LinkObject(arStream);
}

// Registers the wrapped shape's strips data (through its vtable), then the base's streamables.
bool bhkPackedNiTriStripsShape::RegisterStreamables(NiStream& arStream) {
    hkReferencedObject* phkObject;
    if (this) {
        phkObject = this->phkObject;
    }
    else {
        phkObject = nullptr;
    }
    hkPackedNiTriStripsData* pData = static_cast<hkPackedNiTriStripsShape*>(phkObject)->spTriStripsData.m_pObject;
    pData->RegisterStreamables(arStream);
    return bhkShape::RegisterStreamables(arStream);
}

void bhkPackedNiTriStripsShape::SaveBinary(NiStream& arStream) {
    // Makes the creation info, streams the shape, then drops the info again.
    bool bCreated;
    static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated);
    hkPackedNiTriStripsShape* pShape = static_cast<hkPackedNiTriStripsShape*>(phkObject);
    bhkShape::SaveBinary(arStream);
    NiBinaryStream* pOstr = arStream.m_pkOstr;
    float fRadius = pShape->fRadius;
    uint32_t uiSize = 4;
    pOstr->BinaryWrite(&fRadius, 4, &uiSize, 1);
    uiSize = 0x10;
    pOstr->BinaryWrite(&pShape->vScale, 0x10, &uiSize, 1);
    arStream.SaveLinkID(pShape->spTriStripsData.m_pObject);
    static_cast<bhkSerializable*>(this)->KillCinfo(bCreated);
}

// The base's comparison, then whether both wrapped shapes hold the same mesh.
bool bhkPackedNiTriStripsShape::IsEqual(NiObject* apObject) {
    bool bEqual = bhkShape::IsEqual(apObject);
    if (!bEqual) {
        return bEqual;
    }
    hkPackedNiTriStripsShape* phkOther = apObject
        ? static_cast<hkPackedNiTriStripsShape*>(static_cast<bhkPackedNiTriStripsShape*>(apObject)->phkObject)
        : nullptr;
    hkPackedNiTriStripsShape* phkShape = this ? static_cast<hkPackedNiTriStripsShape*>(phkObject) : nullptr;
    return phkShape->spTriStripsData.m_pObject == phkOther->spTriStripsData.m_pObject;
}

// The base's strings, this class's RTTI name and the mesh's sub part count, then for each of the first five sub parts
// its index, its collision filter's strings and its material. The appends' allocations are charged to the Havok heap
// context.
void bhkPackedNiTriStripsShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShapeCollection::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkPackedNiTriStripsShape::ms_RTTI.GetName());
    apStrings->Add(pcString);
    hkPackedNiTriStripsShape* phkShape = this ? static_cast<hkPackedNiTriStripsShape*>(phkObject) : nullptr;
    uint32_t uiParts = phkShape->spTriStripsData->usPartCount;
    pcString = NiGetViewerString("Subparts", uiParts);
    apStrings->Add(pcString);
    if (uiParts > 5) {
        uiParts = 5;
    }
    for (uint32_t i = 0; i < uiParts; ++i) {
        pcString = NiGetViewerString("Part", i);
        apStrings->Add(pcString);
        CFilter kFilter;
        kFilter.iFilter = phkShape->spTriStripsData->paSubPartData[i].uiFilter;
        kFilter.GetViewerStrings(apStrings);
        pcString = NiGetViewerString(
            "MATERIAL",
            static_cast<int32_t>(phkShape->spTriStripsData->paSubPartData[i].uiMaterial)
        );
        apStrings->Add(pcString);
    }
}

void bhkPackedNiTriStripsShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        hkSerializableCinfo* pInfo = this->pInfo;
        delete static_cast<hkPackedNiTriStripsShapeCinfo*>(pInfo);
        this->pInfo = nullptr;
    }
}

uint32_t bhkPackedNiTriStripsShape::QCinfoSize() const {
    return 32;
}

int32_t bhkPackedNiTriStripsShape::LoadCInfo(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // Reads the creation info into a scratch block and drops it: nothing is kept.
    uint32_t uiCinfoSize = static_cast<const bhkSerializable*>(this)->QCinfoSize();
    void* pCinfo = MemoryManager::Instance().Allocate(uiCinfoSize);
    NiBinaryStream* pIstr = arStream.m_pkIstr;
    pIstr->BinaryRead(pCinfo, uiCinfoSize, nullptr, 0);
    MemoryManager::Instance().Deallocate(pCinfo);
    return static_cast<int32_t>(uiCinfoSize);
}

void bhkPackedNiTriStripsShape::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkReferencedObject* pObject = new hkPackedNiTriStripsShape(
        static_cast<const hkPackedNiTriStripsShapeCinfo&>(arInfo)
    );
    // Both are called through the vtable.
    static_cast<bhkSerializable*>(this)->SethkObject(pObject);
    pObject->removeReference();
    static_cast<bhkSerializable*>(this)->PostInit(arInfo);
}

hkSerializableCinfo& bhkPackedNiTriStripsShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = this->pInfo == nullptr;
    if (arCreated) {
        hkPackedNiTriStripsShapeCinfo* pCinfo = new hkPackedNiTriStripsShapeCinfo;
        this->pInfo = pCinfo;
        // A shape that already exists is copied into the new creation info.
        if (this->phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *this->pInfo;
}

// The creation info's scale (all four components) multiplied by the cloning process's scale, then the base's members.
// The info CreateCinfo may make is not killed.
void bhkPackedNiTriStripsShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkPackedNiTriStripsShapeCinfo& rCinfo = static_cast<hkPackedNiTriStripsShapeCinfo&>(CreateCinfo(bCreated));
    // The clone process is always a BSCloningProcess; only Scale.x is read.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    rCinfo.vScale.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0), rCinfo.vScale.m_quad);
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Calls bhkShape's CanShare, skipping bhkShapeCollection's own.
bool bhkPackedNiTriStripsShape::CanShare(NiCloningProcess& arCloneData) {
    return bhkShape::CanShare(arCloneData);
}

// Adds to apNode one NiTriShape per sub part of the packed mesh, named for the class, scaled by the shape's |scale|,
// with a material property whose emittance is arDefColor, under the Havok heap context. The sub part's triangles are
// gathered into shapeArray as index triples relative to the part's first vertex, following a strip while each next
// triangle shares the last two indices.
void bhkPackedNiTriStripsShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkPackedNiTriStripsShape* pShape = this ? static_cast<hkPackedNiTriStripsShape*>(this->phkObject) : nullptr;
    hkPackedNiTriStripsData* pdata = pShape->spTriStripsData.m_pObject;
    {
        // The array starts with no slots and a grow-by of one.
        NiTPrimitiveArray<uint16_t> shapeArray(0u, 1u);
        uint16_t usvertCount = 0;
        uint16_t uslastKey = 0;
        uint16_t uscurrentKey = 0;
        for (int32_t i = 0; i < pdata->usPartCount; ++i) {
            uint16_t length = 0;
            uint16_t us0 = 0;
            uint16_t us1 = 0;
            uint16_t us2 = 0;
            do {
                TriangleData* ptri = &pdata->paTriData[uscurrentKey];
                if (!length) {
                    us0 = ptri->us1;
                    us1 = ptri->us2;
                    us2 = ptri->us3;
                    if (us0 >= usvertCount + pdata->paSubPartData[i].uiNumVerts) {
                        break;
                    }
                    shapeArray.Add(static_cast<uint16_t>(us0 - usvertCount));
                    shapeArray.Add(static_cast<uint16_t>(us1 - usvertCount));
                    shapeArray.Add(static_cast<uint16_t>(us2 - usvertCount));
                    ++uscurrentKey;
                    length = 1;
                }
                else if (us1 == ptri->us1 && us2 == ptri->us2) {
                    // The triangle continues the strip: it shares the last two indices.
                    us0 = us1;
                    us1 = us2;
                    us2 = ptri->us3;
                    shapeArray.Add(static_cast<uint16_t>(us0 - usvertCount));
                    shapeArray.Add(static_cast<uint16_t>(us1 - usvertCount));
                    shapeArray.Add(static_cast<uint16_t>(us2 - usvertCount));
                    ++length;
                    ++uscurrentKey;
                }
                else {
                    length = 0;
                }
            }
            while (us0 < usvertCount + pdata->paSubPartData[i].uiNumVerts && uscurrentKey < pdata->uiKeyCount);

            NiTriShapeData* ptsd = new NiTriShapeData;
            uint16_t uspartVerts = static_cast<uint16_t>(pdata->paSubPartData[i].uiNumVerts);
            NiPoint3* verts = new NiPoint3[uspartVerts];
            // Each point is multiplied by the Havok-to-game scale, which is loaded once before the loop.
            float fToGame = bhkConvert::fHk2BSScaleSC;
            for (uint16_t v = usvertCount; v < usvertCount + uspartVerts; ++v) {
                const NiPoint3& kVert = pdata->paVertData[v];
                NiPoint3& kOut = verts[v - usvertCount];
                kOut.x = kVert.x * fToGame;
                kOut.y = kVert.y * fToGame;
                kOut.z = kVert.z * fToGame;
            }
            ptsd->NiGeometryData::SetData(
                uspartVerts,
                verts,
                nullptr,
                nullptr,
                nullptr,
                0,
                NiGeometryData::NBT_METHOD_NONE
            );
            uint16_t usnumTris = static_cast<uint16_t>(uscurrentKey - uslastKey);
            uint16_t* ptriList = new uint16_t[static_cast<uint32_t>(usnumTris) * 3];
            rt::memcpy(ptriList, shapeArray.m_pBase, shapeArray.m_usSize * sizeof(uint16_t));
            ptsd->SetData(usnumTris, ptriList);

            NiTriShape* ptriShape = new NiTriShape(ptsd);
            alignas(16) NiFixedString kName("bhkPackedNiTriStripsShape");
            ptriShape->SetName(kName);
            float fScale = GetScale();
            fScale = std::fabsf(fScale);
            ptriShape->m_kLocal.m_fScale = fScale;
            shapeArray.RemoveAll();

            NiMaterialProperty* pmat = new NiMaterialProperty;
            pmat->SetEmittance(arDefColor);
            ptriShape->AttachProperty(pmat);
            apNode->AttachChild(ptriShape, 1);
            usvertCount += uspartVerts;
            uslastKey = uscurrentKey;
        }
    }
}

HavokMaterialType bhkPackedNiTriStripsShape::QSubMaterial(uint32_t aiKey) {
    hkPackedNiTriStripsShape* phkObject;
    hkPackedNiTriStripsData* m_pObject;
    if (this) {
        phkObject = static_cast<hkPackedNiTriStripsShape*>(this->phkObject);
    }
    else {
        phkObject = nullptr;
    }
    int32_t v3 = static_cast<uint8_t>(aiKey >> 24);
    if (aiKey == (-1)) {
        return static_cast<HavokMaterialType>(eMaterial & 31);
    }
    m_pObject = phkObject->spTriStripsData.m_pObject;
    if (v3 >= m_pObject->usPartCount) {
        return static_cast<HavokMaterialType>(eMaterial & 31);
    }

    return m_pObject->paSubPartData[v3].uiMaterial;
}

// The base's data, then the wrapped shape's mesh, radius and scale; without a wrapped shape, a zero radius and a unit
// scale.
void bhkPackedNiTriStripsShape::CopyCinfo(hkPackedNiTriStripsShapeCinfo& arInfo) {
    bhkShape::CopyCinfo(arInfo);
    hkReferencedObject* phkShape;
    if (this && (phkShape = phkObject) != nullptr) {
        static_cast<hkPackedNiTriStripsShape*>(phkShape)->CopyCinfo(arInfo);
    }
    else {
        arInfo.fRadius = 0.0f;
        arInfo.vScale.setAll(1.0f);
    }
}

// The x component of the wrapped shape's scale, 1 without a shape.
float bhkPackedNiTriStripsShape::GetScale() {
    hkReferencedObject* phkObject;
    if (this && (phkObject = this->phkObject) != nullptr) {
        return static_cast<hkPackedNiTriStripsShape*>(phkObject)->vScale(0);
    }

    return 1.0f;
}

NiObject* bhkPackedNiTriStripsShape::CreateObject() {
    return new bhkPackedNiTriStripsShape;
}

// hkShapeCinfo's constructor clears the user data; spData's own destructor releases the data.
hkPackedNiTriStripsShapeCinfo::hkPackedNiTriStripsShapeCinfo() : spData(nullptr), fRadius(0.0f) {
    vScale.setAll(1.0f);
}

hkPackedNiTriStripsShapeCinfo::~hkPackedNiTriStripsShapeCinfo() {}
