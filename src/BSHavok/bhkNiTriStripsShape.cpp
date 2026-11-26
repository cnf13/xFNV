#include "BSHavok/bhkNiTriStripsShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkMigrationUtilFunctions.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/hkNiTriStripsShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>

NiRTTI bhkNiTriStripsShape::ms_RTTI("bhkNiTriStripsShape", &bhkShapeCollection::ms_RTTI);
hkNiTriStripsData bhkNiTriStripsShape::EmptySubpart;

bhkNiTriStripsShape::bhkNiTriStripsShape() {
    ++iCount;
}

bhkNiTriStripsShape::~bhkNiTriStripsShape() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkNiTriStripsShape::GetRTTI() const {
    return &bhkNiTriStripsShape::ms_RTTI;
}

NiObject* bhkNiTriStripsShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkNiTriStripsShape* pClone = new bhkNiTriStripsShape;
    // Called through the base, so the class's CopyMembers is reached through the vtable.
    static_cast<bhkShape*>(this)->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

void bhkNiTriStripsShape::LoadBinary(NiStream& arStream) {
    bool bCreated;
    hkNiTriStripsShapeCinfo& rInfo = static_cast<hkNiTriStripsShapeCinfo&>(CreateCinfo(bCreated));
    if (arStream.m_BSStreamHeader.uiVersion < 2) {
        // A file holding an old bhkpMeshShape: warn, read its creation info whole and keep only its material (the
        // user data); the shape's cinfo gets the radius, then user data 0 and radius 0.1.
        const char* pAuthor = arStream.m_BSStreamHeader.cAuthor;
        if (!*pAuthor) {
            pAuthor = "Please";
        }
        {
            char acBuffer[512];
            hkOstream kStream(acBuffer, 512, true);
            kStream << "File " << arStream.m_acFileName << " contains an old bhkpMeshShape! " << pAuthor
                    << " re-export\n";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING,
                0x234F2250,
                acBuffer,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkNiTriStripsShape.cpp",
                170
            );
        }
        NiObject::LoadBinary(arStream);
        hkpMeshShapeCinfo kCinfo;
        {
            hkpMeshShapeCinfo::Serialized kForm{};
            arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
            kCinfo.ReadSerialized(kForm);
        }
        // The array headers read from the file are not the arrays': each array is emptied, first to last.
        kCinfo.m_vertexBaseArray.clearAndDeallocate();
        kCinfo.m_vertexStridingArray.clearAndDeallocate();
        kCinfo.m_numVerticesArray.clearAndDeallocate();
        kCinfo.m_index32BaseArray.clearAndDeallocate();
        kCinfo.m_stridingTypeArray.clearAndDeallocate();
        kCinfo.m_indexStridingArray.clearAndDeallocate();
        kCinfo.m_numTrianglesArray.clearAndDeallocate();
        kCinfo.m_materialIndexBaseArray.clearAndDeallocate();
        kCinfo.m_materialIndexStridingArray.clearAndDeallocate();
        kCinfo.m_materialBaseArray.clearAndDeallocate();
        kCinfo.m_materialStridingArray.clearAndDeallocate();
        kCinfo.m_numMaterialsArray.clearAndDeallocate();
        // Compares the flag's raw byte with 1, here and below: any other byte swaps.
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
            kCinfo.m_userData = static_cast<int32_t>(std::byteswap<uint32_t>(static_cast<uint32_t>(kCinfo.m_userData)));
            kCinfo.m_radius = std::bit_cast<float>(std::byteswap<uint32_t>(std::bit_cast<uint32_t>(kCinfo.m_radius)));
        }
        int32_t iMaterial = kCinfo.m_userData;
        rInfo.m_radius = kCinfo.m_radius;
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
            XFNV_M128_F32(kCinfo.m_scaling.m_quad, 0) = std::bit_cast<float>(
                std::byteswap<uint32_t>(std::bit_cast<uint32_t>(XFNV_M128_F32(kCinfo.m_scaling.m_quad, 0)))
            );
            XFNV_M128_F32(kCinfo.m_scaling.m_quad, 1) = std::bit_cast<float>(
                std::byteswap<uint32_t>(std::bit_cast<uint32_t>(XFNV_M128_F32(kCinfo.m_scaling.m_quad, 1)))
            );
            XFNV_M128_F32(kCinfo.m_scaling.m_quad, 2) = std::bit_cast<float>(
                std::byteswap<uint32_t>(std::bit_cast<uint32_t>(XFNV_M128_F32(kCinfo.m_scaling.m_quad, 2)))
            );
            XFNV_M128_F32(kCinfo.m_scaling.m_quad, 3) = std::bit_cast<float>(
                std::byteswap<uint32_t>(std::bit_cast<uint32_t>(XFNV_M128_F32(kCinfo.m_scaling.m_quad, 3)))
            );
        }
        if (iMaterial >= BHK_MATERIAL_MAX) {
            iMaterial = BHK_MATERIAL_STONE;
            kCinfo.m_userData = BHK_MATERIAL_STONE;
        }
        eMaterial = static_cast<HavokMaterialType>(iMaterial);
        rInfo.m_userData = 0;
        rInfo.m_radius = 0.1f;
        arStream.ReadMultipleLinkIDs();
    }
    else {
        bhkShape::LoadBinary(arStream);
        arStream.ReadMultipleLinkIDs();
        alignas(16) uint32_t uiCount = 0;
        NiStreamLoadBinary(arStream, uiCount);
        // Swaps the cinfo's radius here even though this path never reads it from the file.
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
            rInfo.m_radius = std::bit_cast<float>(std::byteswap<uint32_t>(std::bit_cast<uint32_t>(rInfo.m_radius)));
        }
        for (uint32_t i = 0; i < uiCount; ++i) {
            alignas(16) hkNiTriStripsData kData{};
            NiStreamLoadBinary(arStream, kData.iFilter);
            // Files before version 9: the filter's bit 5 moves to bit 15.
            if (arStream.m_BSStreamHeader.uiVersion < 9 && (kData.iFilter & 0x20) != 0) {
                kData.iFilter = (kData.iFilter & ~0x20u) | 0x8000u;
            }
            rInfo.pShapesA.Add(kData);
        }
    }
}

// Links each sub part's geometry, its normals cleared, into the creation info's sub parts; a file older than stream
// version 2 gets one empty sub part per link ID first.
void bhkNiTriStripsShape::LinkObject(NiStream& arStream) {
    bool bCreated;
    // CreateCinfo is called virtually.
    hkNiTriStripsShapeCinfo* pCinfo = &static_cast<hkNiTriStripsShapeCinfo&>(
        static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated)
    );
    uint32_t uiLinks = arStream.GetNumberOfLinkIDs();
    if (arStream.m_BSStreamHeader.uiVersion < 2u) {
        NiTLargePrimitiveArray<hkNiTriStripsData>* pShapes = &pCinfo->pShapesA;
        pShapes->SetSize(uiLinks);
        if (uiLinks) {
            // The empty sub part.
            hkNiTriStripsData kEmpty{};
            for (uint32_t uiLeft = uiLinks; uiLeft != 0; --uiLeft) {
                pShapes->Add(kEmpty);
            }
        }
    }
    for (uint32_t i = 0; i < uiLinks; ++i) {
        NiTriStripsData* pData = static_cast<NiTriStripsData*>(arStream.GetObjectFromLinkID());
        pData->ClearNormals();
        pCinfo->pShapesA.m_pBase[i].spGeom = pData;
    }
    bhkShape::LinkObject(arStream);
}

// Registers each sub part's geometry, then the shape's own streamables.
bool bhkNiTriStripsShape::RegisterStreamables(NiStream& arStream) {
    int32_t iSubparts = GetNumSubparts();
    for (int32_t i = 0; i < iSubparts; ++i) {
        hkNiTriStripsData* pSubpart = &GetSubpartAt(i);
        if (pSubpart->spGeom.m_pObject) {
            pSubpart->spGeom->RegisterStreamables(arStream);
        }
    }
    return bhkShape::RegisterStreamables(arStream);
}

// Saves the sub parts from a local copy: they are moved out of the creation info, whose radius is set to 0.1 and whose
// radius and scale are byte-swapped around the base save for a big-endian file; then come the sub parts' count, their
// link IDs, the count again and their filters.
void bhkNiTriStripsShape::SaveBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated;
    // CreateCinfo is called virtually.
    hkNiTriStripsShapeCinfo* pCinfo = &static_cast<hkNiTriStripsShapeCinfo&>(
        static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated)
    );
    NiTLargePrimitiveArray<hkNiTriStripsData> kShapes(0u, 1u);
    int32_t iSubparts = static_cast<int32_t>(pCinfo->pShapesA.m_uiSize);
    for (int32_t i = 0; i < iSubparts; ++i) {
        kShapes.Add(pCinfo->pShapesA.m_pBase[i]);
    }
    pCinfo->pShapesA.RemoveAll();
    pCinfo->pShapesA.SetSize(0);
    pCinfo->m_radius = 0.1f;
    // Compares the flag's raw byte with 1 and swaps the radius and the scale's four 32-bit words as raw memory, not as
    // floats.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        uint32_t* pRadius = reinterpret_cast<uint32_t*>(&pCinfo->m_radius);
        *pRadius = std::byteswap<uint32_t>(*pRadius);
        uint32_t* pScale = reinterpret_cast<uint32_t*>(&pCinfo->Scale);
        pScale[0] = std::byteswap<uint32_t>(pScale[0]);
        pScale[1] = std::byteswap<uint32_t>(pScale[1]);
        pScale[2] = std::byteswap<uint32_t>(pScale[2]);
        pScale[3] = std::byteswap<uint32_t>(pScale[3]);
    }
    bhkShape::SaveBinary(arStream);
    uint32_t uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&iSubparts, 4, &uiComponentSize, 1);
    for (int32_t i = 0; i < iSubparts; ++i) {
        arStream.SaveLinkID(kShapes.m_pBase[i].spGeom.m_pObject);
    }
    uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&iSubparts, 4, &uiComponentSize, 1);
    for (int32_t i = 0; i < iSubparts; ++i) {
        uint32_t uiFilter = kShapes.m_pBase[i].iFilter;
        uiComponentSize = 4;
        arStream.m_pkOstr->BinaryWrite(&uiFilter, 4, &uiComponentSize, 1);
    }
    // The same raw byte test and 32-bit byte swaps, undoing the first.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        uint32_t* pRadius = reinterpret_cast<uint32_t*>(&pCinfo->m_radius);
        *pRadius = std::byteswap<uint32_t>(*pRadius);
        uint32_t* pScale = reinterpret_cast<uint32_t*>(&pCinfo->Scale);
        pScale[0] = std::byteswap<uint32_t>(pScale[0]);
        pScale[1] = std::byteswap<uint32_t>(pScale[1]);
        pScale[2] = std::byteswap<uint32_t>(pScale[2]);
        pScale[3] = std::byteswap<uint32_t>(pScale[3]);
    }
    static_cast<bhkSerializable*>(this)->KillCinfo(bCreated);
}

// Equal when the base finds the objects equal and both wrap as many sub parts.
bool bhkNiTriStripsShape::IsEqual(NiObject* apObject) {
    bool bEqual = bhkShape::IsEqual(apObject);
    if (!bEqual) {
        return bEqual;
    }
    uint32_t uiSubparts = GetNumSubparts();
    bhkNiTriStripsShape* pOther = static_cast<bhkNiTriStripsShape*>(apObject);
    uint32_t uiOtherSubparts = pOther->GetNumSubparts();
    return uiSubparts == uiOtherSubparts;
}

// Adds the class heading and the sub part count, then each part's index, material and filter lines for the first five
// sub parts and for up to three more counted back from the last, under the Havok heap context.
void bhkNiTriStripsShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShapeCollection::GetViewerStrings(apStrings);
    char* pcHeading = NiGetViewerString(bhkNiTriStripsShape::ms_RTTI.GetName());
    apStrings->Add(pcHeading);
    uint32_t uiSubparts = GetNumSubparts();
    char* pcSubparts = NiGetViewerString("Subparts", uiSubparts);
    apStrings->Add(pcSubparts);
    uint32_t uiFirst = uiSubparts;
    uint32_t uiLast = 0;
    if (uiSubparts > 5) {
        uiFirst = 5;
        uiLast = uiSubparts - 5;
        if (uiLast > 3) {
            uiLast = 3;
        }
    }
    CFilter kFilter;
    for (uint32_t i = 0; i < uiFirst; ++i) {
        char* pcPart = NiGetViewerString("Part", i);
        apStrings->Add(pcPart);
        hkNiTriStripsShape* pCurShape;
        hkNiTriStripsData* pSubpart = (this &&
                                       (pCurShape = static_cast<hkNiTriStripsShape*>(this->phkObject)) != nullptr)
            ? &pCurShape->pShapesA.m_pBase[i]
            : &EmptySubpart;
        int32_t iMaterial = pSubpart->spGeom.m_pObject ? (pSubpart->spGeom->m_usDataFlags >> 6) & 0x1F : 0;
        char* pcMaterial = NiGetViewerString("MATERIAL", iMaterial);
        apStrings->Add(pcMaterial);
        kFilter.iFilter = pSubpart->iFilter;
        kFilter.GetViewerStrings(apStrings);
    }
    for (uint32_t j = 0; j < uiLast; ++j) {
        uint32_t uiPart = uiSubparts - j - 1;
        char* pcPart = NiGetViewerString("Part", uiPart);
        apStrings->Add(pcPart);
        hkNiTriStripsShape* pCurShape;
        hkNiTriStripsData* pSubpart = (this &&
                                       (pCurShape = static_cast<hkNiTriStripsShape*>(this->phkObject)) != nullptr)
            ? &pCurShape->pShapesA.m_pBase[uiPart]
            : &EmptySubpart;
        int32_t iMaterial = pSubpart->spGeom.m_pObject ? (pSubpart->spGeom->m_usDataFlags >> 6) & 0x1F : 0;
        char* pcMaterial = NiGetViewerString("MATERIAL", iMaterial);
        apStrings->Add(pcMaterial);
        kFilter.iFilter = pSubpart->iFilter;
        kFilter.GetViewerStrings(apStrings);
    }
}

// Destroys and frees the creation info when it was built here.
void bhkNiTriStripsShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkNiTriStripsShapeCinfo*>(this->pInfo);
        this->pInfo = nullptr;
    }
}

// The size of the shape's creation info in a file: its serialized form's.
uint32_t bhkNiTriStripsShape::QCinfoSize() const {
    return sizeof(hkNiTriStripsShapeCinfo::Serialized);
}

// Reads the creation info's serialized form and fills the block CreateCinfo made from it, then swaps its scale words
// in place when the file's source was big-endian. Files older than the extended header version 4 carry no scale
// words: the form is read short and keeps the scale the info already holds.
int32_t bhkNiTriStripsShape::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkNiTriStripsShapeCinfo* pCinfo = &static_cast<hkNiTriStripsShapeCinfo&>(
        static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated)
    );
    int32_t iCinfoSize = static_cast<int32_t>(static_cast<const bhkSerializable*>(this)->QCinfoSize());
    if (arStream.m_BSStreamHeader.uiVersion < 4u) {
        iCinfoSize -= 0x10;
    }
    hkNiTriStripsShapeCinfo::Serialized kForm{};
    kForm.Scale = pCinfo->Scale;
    NiBinaryStream* pIstr = arStream.m_pkIstr;
    pIstr->BinaryRead(&kForm, static_cast<uint32_t>(iCinfoSize), nullptr, 0);
    pCinfo->ReadSerialized(kForm);
    // The flag's raw byte is compared with 1: any other byte swaps, so the byte is read as untyped memory rather than
    // tested as a bool.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1 &&
        arStream.m_BSStreamHeader.uiVersion >= 4u) {
        // Byte-swaps the scale's four 32-bit words as raw memory, not as floats.
        uint32_t* pScale = reinterpret_cast<uint32_t*>(&pCinfo->Scale);
        pScale[0] = std::byteswap<uint32_t>(pScale[0]);
        pScale[1] = std::byteswap<uint32_t>(pScale[1]);
        pScale[2] = std::byteswap<uint32_t>(pScale[2]);
        pScale[3] = std::byteswap<uint32_t>(pScale[3]);
    }
    return iCinfoSize;
}

// Builds the Havok shape in a block from the game's heap, then hands the object to the wrapper and drops the local
// reference.
void bhkNiTriStripsShape::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkReferencedObject* pObject = ::new hkNiTriStripsShape(static_cast<const hkNiTriStripsShapeCinfo&>(arInfo));
    // Both are called virtually.
    static_cast<bhkSerializable*>(this)->SethkObject(pObject);
    pObject->removeReference();
    static_cast<bhkSerializable*>(this)->PostInit(arInfo);
}

hkNiTriStripsShapeCinfo::hkNiTriStripsShapeCinfo() : m_radius(0.0f) {
    Scale.setAll3(1.0f);
}

// Builds the creation info under the Havok heap context when there is none, then copies the wrapped shape's data into
// it.
hkSerializableCinfo& bhkNiTriStripsShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = this->pInfo == nullptr;
    if (arCreated) {
        hkNiTriStripsShapeCinfo* pCinfo = new hkNiTriStripsShapeCinfo;
        this->pInfo = pCinfo;
        if (this->phkObject) {
            bhkShape::CopyCinfo(*pCinfo);
            hkNiTriStripsShape* pShape = static_cast<hkNiTriStripsShape*>(this->phkObject);
            if (pShape) {
                pShape->CopyCinfo(*pCinfo);
            }
            else {
                pCinfo->Scale.fX = 1.0f;
                pCinfo->Scale.fY = 1.0f;
                pCinfo->Scale.fZ = 1.0f;
            }
        }
    }
    return *this->pInfo;
}

// Scales the creation info's scale by the cloning process's, then copies the shape's members.
void bhkNiTriStripsShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    // CreateCinfo is called virtually.
    hkNiTriStripsShapeCinfo* pCinfo = &static_cast<hkNiTriStripsShapeCinfo&>(
        static_cast<bhkSerializable*>(this)->CreateCinfo(bCreated)
    );
    // The cloning process is a BSCloningProcess: its scale's x (+0x10; NiCloningProcess is 0x10 bytes).
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    pCinfo->Scale.fX = pCinfo->Scale.fX * fScale;
    pCinfo->Scale.fY = pCinfo->Scale.fY * fScale;
    pCinfo->Scale.fZ = pCinfo->Scale.fZ * fScale;
    pCinfo->Scale.fW = fScale * pCinfo->Scale.fW;
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Calls bhkShape's CanShare, skipping bhkShapeCollection's own.
bool bhkNiTriStripsShape::CanShare(NiCloningProcess& arCloneData) {
    return bhkShape::CanShare(arCloneData);
}

// Adds a NiTriStrips named for the class over each sub part whose geometry is a NiTriStripsData to apNode, scaled by
// the shape's |scale| and prepared with the geometry's material bits and arDefColor, under the Havok heap context.
void bhkNiTriStripsShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    int32_t iSubparts = GetNumSubparts();
    for (int32_t i = 0; i < iSubparts; ++i) {
        hkNiTriStripsData* pSubpart = &GetSubpartAt(i);
        NiTriStripsData* pData = pSubpart->spGeom.m_pObject;
        if (!pData) {
            continue;
        }
        if (!static_cast<NiObject*>(pData)->IsKindOf(&NiTriStripsData::ms_RTTI)) {
            continue;
        }
        NiTriStrips* pStrips = new NiTriStrips(pData);
        pStrips->SetName("bhkNiTriStripsShape");
        float fScale = std::fabsf(GetScale());
        pStrips->m_kLocal.m_fScale = fScale;
        uint32_t uiMaterial = pSubpart->spGeom.m_pObject ? (pSubpart->spGeom->m_usDataFlags >> 6) & 0x1F : 0;
        bhkMigrationUtilFunctions::PrepareDebugGeom(
            pStrips,
            apNode,
            static_cast<HavokMaterialType>(uiMaterial),
            &arDefColor
        );
        apNode->AttachChild(pStrips, true);
    }
}

// The material of the sub part the key's top twelve bits index, or the shape's own for the key -1.
HavokMaterialType bhkNiTriStripsShape::QSubMaterial(uint32_t aiKey) {
    // The material id is the low five bits of the material word.
    if (aiKey == 0xFFFFFFFFu) {
        return static_cast<HavokMaterialType>(eMaterial & 0x1F);
    }
    hkNiTriStripsData* pSubpart = &GetSubpartAt(aiKey >> 20);
    NiTriStripsData* pData = pSubpart->spGeom.m_pObject;
    if (pData) {
        return static_cast<HavokMaterialType>((pData->m_usDataFlags >> 6) & 0x1F);
    }
    return BHK_MATERIAL_STONE;
}

bool bhkNiTriStripsShape::GetSubMaterialVarType(uint32_t aiKey, MATERIAL_VARIANT_TYPE aeType) {
    hkNiTriStripsShape* phkObject;
    hkNiTriStripsData* v5;
    NiGeometryData* m_pObject;
    if (aiKey == (-1)) {
        return ((static_cast<int32_t>(aeType) & static_cast<int32_t>(eMaterial)) != 0);
    }
    if (this && ((phkObject = static_cast<hkNiTriStripsShape*>(this->phkObject)) != nullptr)) {
        v5 = &phkObject->pShapesA.m_pBase[aiKey >> 20];
    }
    else {
        v5 = &bhkNiTriStripsShape::EmptySubpart;
    }
    m_pObject = v5->spGeom.m_pObject;
    if (m_pObject) {
        if (aeType == MVT_PLATFORM) {
            return ((m_pObject->m_usDataFlags & NiGeometryData::HAVOK_PLATFORM_MASK) != 0);
        }

        if (aeType == MVT_STAIRS) {
            return ((m_pObject->m_usDataFlags & NiGeometryData::HAVOK_STAIRS_MASK) != 0);
        }
    }
    return false;
}

// The shape's scale (its x), or 1 without a shape.
float bhkNiTriStripsShape::GetScale() const {
    hkNiTriStripsShape* pShape;
    if (this && (pShape = static_cast<hkNiTriStripsShape*>(this->phkObject)) != nullptr) {
        return XFNV_M128_F32(pShape->Scale.m_quad, 0);
    }
    return 1.0f;
}

NiObject* bhkNiTriStripsShape::CreateObject() {
    return new bhkNiTriStripsShape;
}

// hkShapeCinfo's constructor clears the user data.
hkpMeshShapeCinfo::hkpMeshShapeCinfo() {
    m_scaling.setZero4();
}

hkpMeshShapeCinfo::~hkpMeshShapeCinfo() {}
