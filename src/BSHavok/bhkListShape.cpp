#include "BSHavok/bhkListShape.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBoxShape.h"
#include "BSHavok/bhkConvexVerticesShape.h"
#include "BSHavok/bhkMoppBvTreeShape.h"
#include "BSHavok/bhkNiTriStripsShape.h"
#include "BSHavok/bhkPackedNiTriStripsShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

NiRTTI bhkListShape::ms_RTTI("bhkListShape", &bhkShapeCollection::ms_RTTI);

bhkListShape::bhkListShape() {
    ++iCount;
}

// The base chain counts each level, then Init.
bhkListShape::bhkListShape(const hkListShapeCinfo& arInfo) {
    bhkListShape::Init(arInfo);
    ++iCount;
}

bhkListShape::~bhkListShape() {
    bhkSerializable::Kill();
    --iCount;
}

// Clones the shape: builds an empty copy under the clone lock and has it copy this shape's members.
NiObject* bhkListShape::CreateClone(NiCloningProcess& arCloneData) {
    int32_t& iContext = GetThreadMemContext();
    int32_t iSavedContext = iContext;
    iContext = MEM_CONTEXT_HAVOK;
    bhkListShape* pkClone = nullptr;
    bhkSerializable::CloneCritLock();
    bhkListShape* pkMemory = static_cast<bhkListShape*>(NiMemObject::operator new(sizeof(bhkListShape)));
    if (pkMemory) {
        ::new (pkMemory) bhkListShape;
        pkClone = pkMemory;
    }
    // Calls the shape's CopyMembers through the vtable.
    bhkShape* pkThisShape = this;
    pkThisShape->CopyMembers(pkClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    iContext = iSavedContext;
    return pkClone;
}

// Reads the child links and the base's data, then the creation info's filter infos, the info's arrays byte-swapped
// first for a big-endian source.
void bhkListShape::LoadBinary(NiStream& arStream) {
    arStream.ReadMultipleLinkIDs();
    bhkShape::LoadBinary(arStream);
    bool bCreated;
    hkListShapeCinfo& kCinfo = static_cast<hkListShapeCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        int32_t iSize = static_cast<int32_t>(QCinfoSize());
        bhkStream::SwapHKArray(&kCinfo, iSize, &kCinfo.m_childShapeFilterInfo);
        bhkStream::SwapHKArray(&kCinfo, iSize, &kCinfo.m_childShapes);
    }
    LoadUIntArray(arStream, kCinfo.m_childShapeFilterInfo);
}

// Rebuilds the creation info's child shapes from the stream's link IDs: the array is emptied and its storage freed,
// grown to the link count, then each linked wrapper's Havok shape is appended (a null link adds nothing). A
// bhkNiTriStripsShape or bhkPackedNiTriStripsShape is first wrapped in a new bhkMoppBvTreeShape, whose shape is
// appended instead. For a stream before version 17 whose children all wrap convex vertices or box shapes, a re-export
// warning is formatted into a local buffer and deliberately not used, as the game does.
void bhkListShape::LinkObject(NiStream& arStream) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    bool bCreated;
    hkListShapeCinfo& rInfo = static_cast<hkListShapeCinfo&>(CreateCinfo(bCreated));
    uint32_t uiLinks = arStream.GetNumberOfLinkIDs();
    hkArray<const hkpShape*>& rShapes = rInfo.m_childShapes;
    rShapes.clearAndDeallocate();
    rShapes.reserve(static_cast<int32_t>(uiLinks));
    for (uint32_t i = uiLinks; i != 0; --i) {
        bhkShape* pShape = static_cast<bhkShape*>(arStream.GetObjectFromLinkID());
        if (!pShape) {
            continue;
        }
        if (pShape->DynamicCast(&bhkNiTriStripsShape::ms_RTTI) ||
            pShape->DynamicCastMember(&bhkPackedNiTriStripsShape::ms_RTTI)) {
            void* pMemory = NiMemObject::operator new(sizeof(bhkMoppBvTreeShape));
            bhkMoppBvTreeShape* pTree = pMemory ? ::new (pMemory) bhkMoppBvTreeShape(pShape) : nullptr;
            if (pTree) {
                pShape = pTree;
            }
        }
        const hkpShape* phkShape = pShape ? static_cast<const hkpShape*>(pShape->phkObject) : nullptr;
        // EXE-INLINED 0x629AD0
        hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
        if (rShapes.m_size == (rShapes.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(rRouter.heap(), &rShapes, sizeof(const hkpShape*));
        }
        rShapes.m_data[rShapes.m_size] = phkShape;
        ++rShapes.m_size;
    }
    if (arStream.m_BSStreamHeader.uiVersion < 17) {
        uint32_t uiNumShapes = static_cast<uint32_t>(rShapes.m_size);
        bool bAllConvex = true;
        for (uint32_t i = 0; i < uiNumShapes; ++i) {
            const hkpShape* phkShape = rShapes.m_data[i];
            if (!phkShape) {
                continue;
            }
            bhkShape* pChild = phkShape->m_userData;
            if (!pChild) {
                continue;
            }
            if (!pChild->IsKindOf(&bhkConvexVerticesShape::ms_RTTI) && !pChild->IsKindOf(&bhkBoxShape::ms_RTTI)) {
                bAllConvex = false;
                break;
            }
        }
        if (bAllConvex) {
            char acMessage[1000];
            BSsprintf(
                acMessage, 1000,
                "HAVOK: %s contains an old (Havok 3.3) hkpConvexListShape (%d) and should be re-exported to 17.",
                arStream.m_acFileName, arStream.m_BSStreamHeader.uiVersion
            );
        }
    }
    bhkShape::LinkObject(arStream);
    pTLSData->eMemContext = eMemContext;
}

// Registers each child's bhkShape, then the base's streamables. The child count is compared unsigned, and both this and
// the Havok shape are null-tested.
bool bhkListShape::RegisterStreamables(NiStream& arStream) {
    // EXE-INLINED 0x4D8560
    int32_t iShapes = 0;
    if (this && phkObject) {
        iShapes = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(iShapes); ++i) {
        // EXE-INLINED 0x4D85A0
        if (this && phkObject) {
            // EXE-INLINED 0x4D8610
            const hkpShape* pShape = static_cast<hkpListShape*>(phkObject)->m_childInfo.m_data[i].m_shape;
            if (pShape && pShape->m_userData) {
                pShape->m_userData->RegisterStreamables(arStream);
            }
        }
    }
    return bhkShape::RegisterStreamables(arStream);
}

// Moves the creation info's child shapes and filter infos out into local arrays (the info keeps empty ones, which is
// what a big-endian save swaps, before and after; nothing moves them back), writes the child count and each child's
// wrapper as a link ID, then the base's data and the filter infos, and frees the moved storage, filter infos first.
void bhkListShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkListShapeCinfo& rInfo = static_cast<hkListShapeCinfo&>(CreateCinfo(bCreated));
    hkArray<const hkpShape*>& rShapes = rInfo.m_childShapes;
    // EXE-INLINED none: hkArray<const hkpShape*>::swap with an empty local array
    const hkpShape** ppShapes = rShapes.m_data;
    rShapes.m_data = nullptr;
    int32_t iNumShapes = rShapes.m_size;
    rShapes.m_size = 0;
    int32_t iShapesCapacityAndFlags = rShapes.m_capacityAndFlags;
    rShapes.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<uint32_t>::hkArray
    FNVD_INLINED_LOCAL(hkArray<uint32_t>, kFilterInfos, std::nullopt, std::nullopt);
    kFilterInfos.m_data = nullptr;
    kFilterInfos.m_size = 0;
    kFilterInfos.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
    kFilterInfos.swap(rInfo.m_childShapeFilterInfo);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        int32_t iCinfoSize = static_cast<int32_t>(QCinfoSize());
        bhkStream::SwapHKArray(&rInfo, iCinfoSize, &rInfo.m_childShapeFilterInfo);
        bhkStream::SwapHKArray(&rInfo, iCinfoSize, &rShapes);
    }
    uint32_t uiNumShapes = static_cast<uint32_t>(iNumShapes);
    NiStreamSaveBinary(arStream, uiNumShapes);
    for (uint32_t i = 0; i < uiNumShapes; ++i) {
        const hkpShape* phkShape = ppShapes[i];
        arStream.SaveLinkID(phkShape ? phkShape->m_userData : nullptr);
    }
    bhkShape::SaveBinary(arStream);
    bhkStream::SaveBinary(arStream, kFilterInfos);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        int32_t iCinfoSize = static_cast<int32_t>(QCinfoSize());
        bhkStream::SwapHKArray(&rInfo, iCinfoSize, &rInfo.m_childShapeFilterInfo);
        bhkStream::SwapHKArray(&rInfo, iCinfoSize, &rShapes);
    }
    // EXE-INLINED none: hkArray<uint32_t>::~hkArray
    hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
    kFilterInfos.m_size = 0;
    if ((kFilterInfos.m_capacityAndFlags & hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG) == 0) {
        rHeap._bufFree<uint32_t>(
            kFilterInfos.m_data, kFilterInfos.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK
        );
    }
    // EXE-INLINED none: hkArray<const hkpShape*>::~hkArray
    hkMemoryAllocator& rShapesHeap = hkMemoryRouter::getInstance().heap();
    if ((iShapesCapacityAndFlags & hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG) == 0) {
        rShapesHeap._bufFree<const hkpShape*>(
            ppShapes, iShapesCapacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK
        );
    }
}

bool bhkListShape::IsEqual(NiObject* apObject) {
    return bhkShape::IsEqual(apObject);
}

// Appends the class's heading and the Havok shape's child count (0 without one) after the shape collection's lines.
void bhkListShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    bhkShapeCollection::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkListShape::ms_RTTI.GetName());
    apStrings->Add(pcString);
    int32_t iShapes = 0;
    if (this && phkObject) {
        iShapes = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    pcString = NiGetViewerString("Shapes", iShapes);
    apStrings->Add(pcString);
    for (int32_t i = 0; i < iShapes; ++i) {
        char acName[32];
        BSsprintf(acName, 32, "Shape%d", i);
    }
    pTLSData->eMemContext = eMemContext;
}

void bhkListShape::Link(bool abAttach) {
    if (abAttach) {
        AddbhkShapeRef();
    }
    else {
        RembhkShapeRef();
    }
    bhkShape::Link(abAttach);
}

// Destroys and frees the creation info this shape made, under the Havok heap context.
void bhkListShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        hkListShapeCinfo* pCinfo = static_cast<hkListShapeCinfo*>(pInfo);
        if (pCinfo) {
            // EXE-INLINED none: hkListShapeCinfo's scalar deleting destructor
            pCinfo->~hkListShapeCinfo();
            MemoryManager_Singleton.Deallocate(pCinfo);
        }
        pInfo = nullptr;
    }
}

// The size of the creation info in a file: its serialized form's.
uint32_t bhkListShape::QCinfoSize() const {
    return sizeof(hkListShapeCinfo::Serialized);
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkListShape::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkListShapeCinfo& rCinfo = static_cast<hkListShapeCinfo&>(CreateCinfo(bCreated));
    hkListShapeCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

// Builds the Havok list shape over the creation info's children, sets the children's filter infos, wraps it and lets
// the base finish, under the Havok heap context.
void bhkListShape::Init(const hkSerializableCinfo& arInfo) {
    const hkListShapeCinfo& kInfo = static_cast<const hkListShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkpShape* const* ppShapes = kInfo.m_childShapes.m_data;
    int32_t iNumShapes = kInfo.m_childShapes.m_size;
    // EXE-INLINED none: hkpListShape::operator new (HK_DECLARE_CLASS_ALLOCATOR)
    void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpListShape));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpListShape);
    hkpListShape* pkList =
        ::new (pMemory) hkpListShape(ppShapes, iNumShapes, hkpShapeContainer::REFERENCE_POLICY_INCREMENT);
    const hkpShapeContainer* pkContainer = pkList;
    uint32_t uiKey = pkContainer->getFirstKey();
    // The smaller of the two counts, compared unsigned.
    uint32_t uiCount = static_cast<uint32_t>(iNumShapes);
    if (uiCount >= static_cast<uint32_t>(kInfo.m_childShapeFilterInfo.m_size)) {
        uiCount = static_cast<uint32_t>(kInfo.m_childShapeFilterInfo.m_size);
    }
    for (uint32_t i = 0; i < uiCount; ++i) {
        pkList->setCollisionFilterInfo(uiKey, kInfo.m_childShapeFilterInfo.m_data[i]);
        // The next key is deliberately dropped, as the game does, so every filter info goes to the first key.
        pkContainer->getNextKey(uiKey);
    }
    SethkObject(pkList);
    pkList->removeReference();
    PostInit(arInfo);
}

hkSerializableCinfo& bhkListShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkListShapeCinfo* pCinfo =
            static_cast<hkListShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkListShapeCinfo)));
        // EXE-INLINED none: hkListShapeCinfo::hkListShapeCinfo
        if (pCinfo) {
            // EXE-INLINED 0x4B5020
            pCinfo->m_userData = 0;
            pCinfo->m_childShapes.m_data = nullptr;
            pCinfo->m_childShapes.m_size = 0;
            pCinfo->m_childShapes.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
            pCinfo->m_childShapeFilterInfo.m_data = nullptr;
            pCinfo->m_childShapeFilterInfo.m_size = 0;
            pCinfo->m_childShapeFilterInfo.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
        }
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Clones each child the cloning process cannot share and puts the clone's Havok shape in this shape's creation info
// in place of the child's, then copies the base's members.
void bhkListShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    // EXE-INLINED none: bhkListShape::GetCinfo
    bool bCreated;
    hkListShapeCinfo& kCinfo = static_cast<hkListShapeCinfo&>(CreateCinfo(bCreated));
    for (int32_t i = 0; i < kCinfo.m_childShapes.m_size; ++i) {
        // EXE-INLINED 0x4A3A40
        const hkpShape* pShape = kCinfo.m_childShapes.m_data[i];
        bhkShape* pkShape = pShape ? pShape->m_userData : nullptr;
        if (pkShape && !pkShape->CanShare(arCloneData)) {
            bhkShape* pkClone = static_cast<bhkShape*>(pkShape->CreateClone(arCloneData));
            // EXE-INLINED 0x4AE750
            kCinfo.m_childShapes.m_data[i] = pkClone ? static_cast<hkpShape*>(pkClone->phkObject) : nullptr;
        }
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

// True when the base can share this shape and every child can, stopping at the first that cannot.
bool bhkListShape::CanShare(NiCloningProcess& arCloneData) {
    bool bResult = bhkShape::CanShare(arCloneData);
    if (!bResult) {
        return bResult;
    }
    // EXE-INLINED 0x4D8560
    int32_t iShapes = 0;
    if (this && phkObject) {
        iShapes = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    int32_t i = 0;
    do {
        if (i >= iShapes) {
            break;
        }
        // EXE-INLINED 0x4D85A0
        if (this && phkObject) {
            // EXE-INLINED 0x4D8610
            const hkpShape* pShape = static_cast<hkpListShape*>(phkObject)->m_childInfo.m_data[i].m_shape;
            if (pShape && pShape->m_userData) {
                bResult &= pShape->m_userData->CanShare(arCloneData);
            }
        }
        ++i;
    }
    while (bResult);
    return bResult;
}

// Has each child's wrapper build its display geometry. Tests this and the wrapped shape for null again for every child,
// and reads the child count through the shape container (0 without a wrapped shape).
void bhkListShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    int32_t iNumChildren = 0;
    if (this && phkObject) {
        iNumChildren =
            static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    for (int32_t i = 0; i < iNumChildren; ++i) {
        if (!this) {
            continue;
        }
        hkpListShape* pkListShape = static_cast<hkpListShape*>(phkObject);
        if (!pkListShape) {
            continue;
        }
        const hkpShape* phkChild = pkListShape->m_childInfo.m_data[i].m_shape;
        if (phkChild) {
            bhkShape* pkChild = phkChild->m_userData;
            if (pkChild) {
                pkChild->BuildDisplayGeometry(apNode, arDefColor);
            }
        }
    }
}

// The sub-material for auiKey of the first child that is a shape collection and gives one other than BHK_MATERIAL_MAX,
// children in order; without one, this shape's own material (its low five bits). FindShapeCollection is called on a
// null wrapper when a child or the wrapped shape is missing, as the game does.
HavokMaterialType bhkListShape::QSubMaterial(uint32_t aiKey) {
    HavokMaterialType eSubMaterial = BHK_MATERIAL_MAX;
    uint32_t uiNumChildren = 0;
    if (this && phkObject) {
        uiNumChildren = static_cast<uint32_t>(
            static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes()
        );
    }
    for (uint32_t i = 0; i < uiNumChildren; ++i) {
        if (eSubMaterial != BHK_MATERIAL_MAX) {
            return eSubMaterial;
        }
        bhkShape* pkChild = nullptr;
        if (this) {
            hkpListShape* pkListShape = static_cast<hkpListShape*>(phkObject);
            if (pkListShape) {
                const hkpShape* phkChild = pkListShape->m_childInfo.m_data[i].m_shape;
                if (phkChild) {
                    pkChild = phkChild->m_userData;
                }
            }
        }
        bhkShapeCollection* pkCollection = pkChild->FindShapeCollection();
        if (pkCollection) {
            eSubMaterial = pkCollection->QSubMaterial(aiKey);
        }
    }
    if (eSubMaterial != BHK_MATERIAL_MAX) {
        return eSubMaterial;
    }
    return static_cast<HavokMaterialType>(eMaterial & 0x1F);
}

hkpShape* bhkListShape::GetChildhkShape(int32_t aiAt) {
    hkpListShape* v4 = static_cast<hkpListShape*>(operator*());
    if (v4) {
        // GetChildhkShape returns hkpShape*, but getChildShapeInl returns a const one, so the const is cast away.
        return const_cast<hkpShape*>(v4->getChildShapeInl(aiAt));
    }

    return nullptr;
}

bhkShape* bhkListShape::GetChildShape(int32_t aiAt) {
    hkpShape* ChildhkShape = GetChildhkShape(aiAt);
    return bhkShape::Getbhk(ChildhkShape);
}

// Copies the shape's user data, then sizes both arrays to the shape's child count (grown to the larger of the count and
// double the capacity), then copies each child's shape and filter info, walking the container's keys.
void bhkListShape::CopyCinfo(hkListShapeCinfo& arInfo) {
    bhkShape::CopyCinfo(arInfo);
    int32_t iNumChildren = 0;
    if (phkObject) {
        iNumChildren =
            static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    hkArray<const hkpShape*>& rShapes = arInfo.m_childShapes;
    rShapes.m_size = 0;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iCapacity = static_cast<int32_t>(rShapes.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK);
    if (iCapacity < iNumChildren) {
        int32_t iReserve = 2 * iCapacity;
        if (iNumChildren >= iReserve) {
            iReserve = iNumChildren;
        }
        hkArrayUtil::_reserve(pHeap, &rShapes, iReserve, sizeof(const hkpShape*));
    }
    rShapes.m_size = static_cast<uint32_t>(iNumChildren);
    hkArray<uint32_t>& rFilterInfos = arInfo.m_childShapeFilterInfo;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iCapacity = static_cast<int32_t>(rFilterInfos.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK);
    if (iCapacity < iNumChildren) {
        int32_t iReserve = 2 * iCapacity;
        if (iNumChildren >= iReserve) {
            iReserve = iNumChildren;
        }
        hkArrayUtil::_reserve(pHeap, &rFilterInfos, iReserve, sizeof(uint32_t));
    }
    rFilterInfos.m_size = static_cast<uint32_t>(iNumChildren);
    uint32_t uiKey = 0;
    if (phkObject) {
        uiKey = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getFirstKey();
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(iNumChildren); ++i) {
        const hkpShape* pChild = nullptr;
        if (phkObject) {
            pChild = static_cast<hkpListShape*>(phkObject)->m_childInfo.m_data[i].m_shape;
        }
        rShapes.m_data[i] = pChild;
        uint32_t uiFilterInfo = 0;
        if (phkObject) {
            uiFilterInfo = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))
                               ->getCollisionFilterInfo(uiKey);
        }
        rFilterInfos.m_data[i] = uiFilterInfo;
        uint32_t uiNextKey = 0;
        if (phkObject) {
            uiNextKey = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNextKey(uiKey);
        }
        uiKey = uiNextKey;
    }
}

int32_t bhkListShape::GetNumChildShapes() const {
    hkpListShape* v3 = static_cast<hkpListShape*>(bhkShapeCollection::operator*());
    if (v3) {
        return static_cast<hkpShapeContainer*>(v3)->getNumChildShapes();
    }

    return 0;
}

// The bodies of GetNumChildShapes and GetChildShape are written out here, with their null tests of this and of the
// Havok shape, and NiRefObject::IncRefCount.
void bhkListShape::AddbhkShapeRef() {
    // EXE-INLINED 0x4D8560
    int32_t iShapes = 0;
    if (this && phkObject) {
        iShapes = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    while (iShapes--) {
        // EXE-INLINED 0x4D85A0
        hkpListShape* pkList = static_cast<hkpListShape*>(phkObject);
        if (pkList) {
            // EXE-INLINED 0x4D8610
            const hkpShape* pShape = pkList->m_childInfo.m_data[iShapes].m_shape;
            if (pShape && pShape->m_userData) {
                pShape->m_userData->IncRefCount();
            }
        }
    }
}

void bhkListShape::RembhkShapeRef() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // EXE-INLINED 0x4D8560
    int32_t iShapes = 0;
    if (this && phkObject) {
        iShapes = static_cast<const hkpShapeContainer*>(static_cast<hkpListShape*>(phkObject))->getNumChildShapes();
    }
    while (iShapes--) {
        // EXE-INLINED 0x4D85A0
        hkpListShape* pkList = static_cast<hkpListShape*>(phkObject);
        if (pkList) {
            // EXE-INLINED 0x4D8610
            const hkpShape* pShape = pkList->m_childInfo.m_data[iShapes].m_shape;
            if (pShape && pShape->m_userData) {
                pShape->m_userData->DecRefCount();
            }
        }
    }
}

// Builds an empty list shape.
NiObject* bhkListShape::CreateObject() {
    bhkListShape* pkShape = static_cast<bhkListShape*>(NiMemObject::operator new(sizeof(bhkListShape)));
    if (!pkShape) {
        return nullptr;
    }
    return ::new (pkShape) bhkListShape;
}
