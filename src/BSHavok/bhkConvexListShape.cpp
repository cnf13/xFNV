#include "BSHavok/bhkConvexListShape.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

NiRTTI bhkConvexListShape::ms_RTTI("bhkConvexListShape", &bhkConvexShape::ms_RTTI);

bhkConvexListShape::bhkConvexListShape() {
    ++bhkConvexListShape::iCount;
}

bhkConvexListShape::~bhkConvexListShape() {
    bhkSerializable::Kill();
    --bhkConvexListShape::iCount;
}

const NiRTTI* bhkConvexListShape::GetRTTI() const {
    return &bhkConvexListShape::ms_RTTI;
}

// Clones the shape: builds an empty copy under the clone lock and has it copy this shape's members.
NiObject* bhkConvexListShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexListShape* pkClone = nullptr;
    bhkSerializable::CloneCritLock();
    pkClone = new bhkConvexListShape;
    CopyMembers(pkClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pkClone;
}

// Reads the convex list shape's own fields after the convex shape's.
void bhkConvexListShape::LoadBinary(NiStream& arStream) {
    arStream.ReadMultipleLinkIDs();
    bhkConvexShape::LoadBinary(arStream);

    bool bCreated;
    hkConvexListShapeCinfo* pCinfo = &static_cast<hkConvexListShapeCinfo&>(CreateCinfo(bCreated));
    // The child shape array is swapped on both sides of the two scalar reads. Each test compares the flag's raw byte
    // with 1: any other byte swaps, so the byte is read as untyped memory rather than tested as a bool.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bhkStream::SwapHKArray(pCinfo, QCinfoSize(), &pCinfo->m_childShapes);
    }

    uint32_t uiComponentSize = 1;
    arStream.m_pkIstr->BinaryRead(&pCinfo->m_useCachedAabb, 1, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&pCinfo->m_minDistanceToUseConvexHullForGetClosestPoints, 4, &uiComponentSize, 1);

    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bhkStream::SwapHKArray(pCinfo, QCinfoSize(), &pCinfo->m_childShapes);
    }
}

// Rebuilds the creation info's child shapes from the stream's link IDs: the array is emptied and its storage freed,
// grown to the link count, then each linked wrapper's Havok shape is appended (a null link adds nothing).
void bhkConvexListShape::LinkObject(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated;
    hkConvexListShapeCinfo& rInfo = static_cast<hkConvexListShapeCinfo&>(CreateCinfo(bCreated));
    uint32_t uiLinks = arStream.GetNumberOfLinkIDs();
    hkArray<const hkpConvexShape*>& rShapes = rInfo.m_childShapes;
    rShapes.clearAndDeallocate();
    rShapes.reserve(static_cast<int32_t>(uiLinks));
    for (uint32_t i = uiLinks; i != 0; --i) {
        bhkConvexShape* pShape = static_cast<bhkConvexShape*>(arStream.GetObjectFromLinkID());
        if (pShape) {
            const hkpConvexShape* phkShape = static_cast<const hkpConvexShape*>(pShape->phkObject);
            rShapes.pushBack(phkShape);
        }
    }
    bhkShape::LinkObject(arStream);
}

// Registers the bhkShape of every child with the stream, then the shape itself.
bool bhkConvexListShape::RegisterStreamables(NiStream& arStream) {
    // The child count comes from the wrapped shape's container interface and is compared unsigned.
    uint32_t uiNumChildShapes = 0;
    if (this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        uiNumChildShapes = static_cast<hkpShapeContainer*>(pkListShape)->getNumChildShapes();
    }
    for (uint32_t i = 0; i < uiNumChildShapes; ++i) {
        if (this && phkObject) {
            const hkpConvexShape* pkChildShape = static_cast<hkpConvexListShape*>(phkObject)->m_childShapes.m_data[i];
            if (pkChildShape) {
                bhkShape* pkChildBhkShape = pkChildShape->m_userData;
                if (pkChildBhkShape) {
                    pkChildBhkShape->RegisterStreamables(arStream);
                }
            }
        }
    }
    return bhkShape::RegisterStreamables(arStream);
}

// Moves the creation info's child shapes out into a local array (the info keeps an empty one, which is what a
// big-endian save swaps), writes their count and each child's wrapper as a link ID (up to the Havok shape's own child
// count, 0 without one), then the base's data and the two settings, and frees the moved storage.
void bhkConvexListShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkConvexListShapeCinfo& rInfo = static_cast<hkConvexListShapeCinfo&>(CreateCinfo(bCreated));
    hkArray<const hkpConvexShape*>& rShapes = rInfo.m_childShapes;
    hkArray<const hkpConvexShape*> kShapes;
    kShapes.swap(rShapes);
    if (!arStream.m_bSaveLittleEndian) {
        bhkStream::SwapHKArray(&rInfo, QCinfoSize(), &rShapes);
    }
    NiStreamSaveBinary(arStream, kShapes.m_size);
    for (int32_t i = 0;; ++i) {
        hkReferencedObject* phkObject = this->phkObject;
        int32_t iCount = phkObject ? static_cast<hkpConvexListShape*>(phkObject)->getNumChildShapes() : 0;
        if (i >= iCount) {
            break;
        }
        const hkpConvexShape* phkShape = kShapes[i];
        if (phkShape) {
            arStream.SaveLinkID(phkShape->m_userData);
        }
        else {
            arStream.SaveLinkID(nullptr);
        }
    }
    bhkConvexShape::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, rInfo.m_useCachedAabb);
    NiStreamSaveBinary(arStream, rInfo.m_minDistanceToUseConvexHullForGetClosestPoints);
}

bool bhkConvexListShape::IsEqual(NiObject* apObject) {
    return bhkConvexShape::IsEqual(apObject);
}

// Appends the class's heading and the Havok shape's child count (0 without one) after the convex shape's lines. A
// "Shape%d" name is formatted per child and none of them is added.
void bhkConvexListShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkReferencedObject* phkObject;
    int32_t iShapes;
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkConvexListShape::ms_RTTI.GetName());
    apStrings->Add(pcString);
    if (this && (phkObject = this->phkObject) != nullptr) {
        iShapes = static_cast<hkpConvexListShape*>(phkObject)->getNumChildShapes();
    }
    else {
        iShapes = 0;
    }
    pcString = NiGetViewerString("Shapes", iShapes);
    apStrings->Add(pcString);
    for (int32_t i = 0; i < iShapes; ++i) {
        char acName[32];
        BSsprintf(acName, 32, "Shape%d", i);
    }
}

// Attaching adds a reference to every child's wrapper, detaching releases them; then the base's part.
void bhkConvexListShape::Link(bool abAttach) {
    if (abAttach) {
        AddbhkShapeRef();
    }
    else {
        RembhkShapeRef();
    }
    bhkShape::Link(abAttach);
}

void bhkConvexListShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkConvexListShapeCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkConvexListShape::QCinfoSize() const {
    return 28;
}

// Builds the wrapped Havok convex list shape from the creation info.
void bhkConvexListShape::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkConvexListShapeCinfo* pCinfo = static_cast<const hkConvexListShapeCinfo*>(&arInfo);
    const hkpConvexShape* const* ppChildShapes = pCinfo->m_childShapes.m_data;
    int32_t iNumChildShapes = pCinfo->m_childShapes.getSize();

    hkpConvexListShape* pkShape = new hkpConvexListShape(ppChildShapes, iNumChildShapes);
    SethkObject(pkShape);
    pkShape->removeReference();
    PostInit(arInfo);
}

// Builds the shape's creation info; arCreated tells whether it was allocated here.
hkSerializableCinfo& bhkConvexListShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkConvexListShapeCinfo* pCinfo = new hkConvexListShapeCinfo;
        bool bHasObject = (phkObject != nullptr);
        pInfo = pCinfo;
        if (bHasObject) {
            bhkConvexListShape::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Clones every child shape the clone cannot share and puts the clone's Havok shape in the creation info's child list,
// then copies bhkShape's members.
void bhkConvexListShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkConvexListShapeCinfo& rInfo = static_cast<hkConvexListShapeCinfo&>(CreateCinfo(bCreated));
    for (int32_t i = 0; i < rInfo.m_childShapes.m_size; ++i) {
        const hkpConvexShape* pkChildShape = rInfo.m_childShapes.m_data[i];
        if (pkChildShape) {
            // The child's user data is the bhkShape that owns it.
            bhkShape* pkChildBhkShape = pkChildShape->m_userData;
            if (pkChildBhkShape && !pkChildBhkShape->CanShare(arCloneData)) {
                bhkShape* pkClone = static_cast<bhkShape*>(pkChildBhkShape->CreateClone(arCloneData));
                rInfo.m_childShapes.m_data[i] = pkClone ? static_cast<hkpConvexShape*>(pkClone->phkObject) : nullptr;
            }
        }
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

// True when this shape and every shape it wraps can be shared with the clone.
bool bhkConvexListShape::CanShare(NiCloningProcess& arCloneData) {
    bool bCanShare = bhkShape::CanShare(arCloneData);
    if (!bCanShare) {
        return false;
    }

    // The child count comes from the wrapped shape's container interface.
    int32_t iNumChildShapes = 0;
    if (this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        iNumChildShapes = static_cast<hkpShapeContainer*>(pkListShape)->getNumChildShapes();
    }
    for (int32_t i = 0; i < iNumChildShapes && bCanShare; ++i) {
        if (this && phkObject) {
            hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
            const hkpConvexShape* pkChildShape = pkListShape->m_childShapes.m_data[i];
            if (pkChildShape) {
                // The wrapped shape's user data is the bhkShape that owns it.
                bhkShape* pkChildBhkShape = pkChildShape->m_userData;
                if (pkChildBhkShape) {
                    bCanShare &= pkChildBhkShape->CanShare(arCloneData);
                }
            }
        }
    }
    return bCanShare;
}

void bhkConvexListShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    hkpConvexListShape* phkObject;
    int32_t v5;
    if (this && (phkObject = static_cast<hkpConvexListShape*>(this->phkObject)) != nullptr) {
        v5 = phkObject->getNumChildShapes();
    }
    else {
        v5 = 0;
    }
    for (int32_t i = 0; i < v5; ++i) {
        if (this) {
            hkpConvexListShape* v7 = static_cast<hkpConvexListShape*>(this->phkObject);
            if (v7) {
                const hkpConvexShape* v8 = v7->m_childShapes.m_data[i];
                if (v8) {
                    bhkShape* ps = v8->m_userData;
                    if (ps) {
                        ps->BuildDisplayGeometry(apNode, arDefColor);
                    }
                }
            }
        }
    }
}

// Passes the call on to the wrapped convex shape.
void bhkConvexListShape::GetSupportingVertex(const hkVector4& arDir, hkpCdVertex& arSupportingVertexOut) {
    if (this) {
        hkReferencedObject* phkObject = this->phkObject;
        if (phkObject) {
            static_cast<hkpConvexShape*>(phkObject)->getSupportingVertexImpl(arDir, arSupportingVertexOut);
        }
    }
}

// Passes the call on to the wrapped convex shape.
void bhkConvexListShape::ConvertVertexIdsToVertices(const uint16_t* pIds, int32_t inumIds, hkpCdVertex* pverticesOut) {
    if (this) {
        hkReferencedObject* phkObject = this->phkObject;
        if (phkObject) {
            static_cast<hkpConvexShape*>(phkObject)->convertVertexIdsToVerticesImpl(pIds, inumIds, pverticesOut);
        }
    }
}

void bhkConvexListShape::GetFirstVertex(hkVector4& arVertex) {
    if (this) {
        hkpConvexListShape* ps = static_cast<hkpConvexListShape*>(phkObject);
        if (ps) {
            ps->getFirstVertex(arVertex);
        }
    }
}

// The material of child shape aiKey, or this shape's own when the child carries none.
HavokMaterialType bhkConvexListShape::QSubMaterial(uint32_t aiKey) {
    // The child count comes from the wrapped shape's container interface, and only for a key other than -1.
    int32_t iNumChildShapes = 0;
    if (aiKey != 0xFFFFFFFFu && this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        iNumChildShapes = static_cast<hkpShapeContainer*>(pkListShape)->getNumChildShapes();
    }
    // Compares the key against the count unsigned, then walks to the child's owning shape.
    if (aiKey <= static_cast<uint32_t>(iNumChildShapes) && this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        const hkpConvexShape* pkChildShape = pkListShape->m_childShapes.m_data[aiKey];
        if (pkChildShape) {
            // The child's user data is the bhkShape that owns it.
            bhkShape* pkChildBhkShape = pkChildShape->m_userData;
            if (pkChildBhkShape) {
                return static_cast<HavokMaterialType>(pkChildBhkShape->eMaterial & 0x1F);
            }
        }
    }
    return static_cast<HavokMaterialType>(eMaterial & 0x1F);
}

// Copies the wrapped shape's data into the creation info: the closest-point distance, the cached-AABB flag and the
// child shapes, walking the container's keys alongside.
void bhkConvexListShape::CopyCinfo(hkConvexListShapeCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::CopyCinfo(arInfo);

    float fMinDistance;
    if (this && phkObject) {
        fMinDistance = static_cast<hkpConvexListShape*>(phkObject)->m_minDistanceToUseConvexHullForGetClosestPoints;
    }
    else {
        fMinDistance = 0.0f;
    }
    arInfo.m_minDistanceToUseConvexHullForGetClosestPoints = fMinDistance;
    arInfo.m_useCachedAabb = this && phkObject && static_cast<hkpConvexListShape*>(phkObject)->getUseCachedAabb();

    // The child count comes from the wrapped shape's container interface.
    int32_t iNumChildShapes = 0;
    if (this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        iNumChildShapes = static_cast<hkpShapeContainer*>(pkListShape)->getNumChildShapes();
    }
    arInfo.m_childShapes.clear();
    arInfo.m_childShapes.setSize(iNumChildShapes);

    // Walks the keys while copying, though it never uses them; the count is compared unsigned.
    uint32_t uiKey = 0;
    if (this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        uiKey = static_cast<hkpShapeContainer*>(pkListShape)->getFirstKey();
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(iNumChildShapes); ++i) {
        const hkpConvexShape* pkChildShape = nullptr;
        if (this && phkObject) {
            pkChildShape = static_cast<hkpConvexListShape*>(phkObject)->m_childShapes.m_data[i];
        }
        arInfo.m_childShapes.m_data[i] = pkChildShape;
        if (this && phkObject) {
            hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
            uiKey = static_cast<hkpShapeContainer*>(pkListShape)->getNextKey(uiKey);
        }
        else {
            uiKey = 0;
        }
    }
}

// Adds a reference to the bhkShape of every child, last child first.
void bhkConvexListShape::AddbhkShapeRef() {
    if (!this || !phkObject) {
        return;
    }
    hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
    int32_t iChild = static_cast<hkpShapeContainer*>(pkListShape)->getNumChildShapes();
    while (iChild) {
        --iChild;
        if (phkObject) {
            const hkpConvexShape* pkChildShape = static_cast<hkpConvexListShape*>(phkObject)
                                                     ->m_childShapes.m_data[iChild];
            if (pkChildShape) {
                bhkShape* pkChildBhkShape = pkChildShape->m_userData;
                if (pkChildBhkShape) {
                    pkChildBhkShape->IncRefCount();
                }
            }
        }
    }
}

// Releases the reference AddbhkShapeRef took on the bhkShape of every child, last child first.
void bhkConvexListShape::RembhkShapeRef() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (this && phkObject) {
        hkpConvexListShape* pkListShape = static_cast<hkpConvexListShape*>(phkObject);
        int32_t iChild = static_cast<hkpShapeContainer*>(pkListShape)->getNumChildShapes();
        while (iChild) {
            --iChild;
            if (phkObject) {
                const hkpConvexShape* pkChildShape = static_cast<hkpConvexListShape*>(phkObject)
                                                         ->m_childShapes.m_data[iChild];
                if (pkChildShape) {
                    bhkShape* pkChildBhkShape = pkChildShape->m_userData;
                    if (pkChildBhkShape) {
                        pkChildBhkShape->DecRefCount();
                    }
                }
            }
        }
    }
}

// Builds an empty convex list shape.
NiObject* bhkConvexListShape::CreateObject() {
    return new bhkConvexListShape;
}
