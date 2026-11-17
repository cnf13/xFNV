#include "BSHavok/bhkConvexTransformShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"

NiRTTI bhkConvexTransformShape::ms_RTTI("bhkConvexTransformShape", &bhkConvexShape::ms_RTTI);

bhkConvexTransformShape::bhkConvexTransformShape() {
    ++bhkConvexTransformShape::iCount;
}

bhkConvexTransformShape::~bhkConvexTransformShape() {
    bhkSerializable::Kill();
    --bhkConvexTransformShape::iCount;
}

const NiRTTI* bhkConvexTransformShape::GetRTTI() const {
    return &bhkConvexTransformShape::ms_RTTI;
}

// Clones the shape: a new default constructed object under the engine's clone lock, filled from this one.
NiObject* bhkConvexTransformShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkConvexTransformShape* pClone = nullptr;
    bhkConvexTransformShape* pObject =
        static_cast<bhkConvexTransformShape*>(NiMemObject::operator new(sizeof(bhkConvexTransformShape)));
    if (pObject) {
        pClone = ::new (pObject) bhkConvexTransformShape;
    }
    // CopyMembers is called through a base reference so that it dispatches virtually.
    bhkShape& rShape = *this;
    rShape.CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Reads the child shape's link and the base's data; the child is linked later (LinkObject). A big-endian source's
// transform is swapped in place: the translation, then the rotation.
void bhkConvexTransformShape::LoadBinary(NiStream& arStream) {
    arStream.ReadLinkID();
    bhkConvexShape::LoadBinary(arStream);
    bool bCreated;
    hkConvexTransformShapeCinfo& rCinfo = static_cast<hkConvexTransformShapeCinfo&>(CreateCinfo(bCreated));
    rCinfo.m_childShape = nullptr;
    if (!arStream.m_bSourceIsLittleEndian) {
        bhkStream::Endian(rCinfo.m_transform.m_translation);
        bhkStream::Endian(rCinfo.m_transform.m_rotation);
    }
}

void bhkConvexTransformShape::LinkObject(NiStream& arStream) {
    bool v5;
    hkConvexTransformShapeCinfo* cinfo = &static_cast<hkConvexTransformShapeCinfo&>(CreateCinfo(v5));
    bhkConvexShape* ps = static_cast<bhkConvexShape*>(arStream.GetObjectFromLinkID());
    if (ps) {
        cinfo->m_childShape = static_cast<hkpConvexShape*>(ps->phkObject);
    }
    else {
        cinfo->m_childShape = 0;
    }
    bhkShape::LinkObject(arStream);
}

// Registers the child shape's wrapper, then the base's part, whose result it returns.
bool bhkConvexTransformShape::RegisterStreamables(NiStream& arStream) {
    bhkShape* pChild = GetChildShape();
    if (pChild) {
        pChild->RegisterStreamables(arStream);
    }
    return bhkShape::RegisterStreamables(arStream);
}

// Writes the child shape's link, then the base's data with the transform in the target's byte order, swapped back
// afterwards. CreateCinfo's created flag is ignored, as the game does.
void bhkConvexTransformShape::SaveBinary(NiStream& arStream) {
    arStream.SaveLinkID(GetChildShape());
    bool bCreated;
    hkConvexTransformShapeCinfo& rCinfo = static_cast<hkConvexTransformShapeCinfo&>(CreateCinfo(bCreated));
    if (!arStream.m_bSaveLittleEndian) {
        bhkStream::Endian(rCinfo.m_transform.m_translation);
        bhkStream::Endian(rCinfo.m_transform.m_rotation);
    }
    bhkConvexShape::SaveBinary(arStream);
    if (!arStream.m_bSaveLittleEndian) {
        bhkStream::Endian(rCinfo.m_transform.m_translation);
        bhkStream::Endian(rCinfo.m_transform.m_rotation);
    }
}

bool bhkConvexTransformShape::IsEqual(NiObject* apObject) {
    return bhkShape::IsEqual(apObject);
}

// The base's lines, the class name, the shape's translation and rotation (the identity without a Havok shape), then the
// child shape's wrapper's lines.
void bhkConvexTransformShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    const hkTransform* pTransform;
    const hkpConvexTransformShape* pShape = this ? static_cast<const hkpConvexTransformShape*>(phkObject) : nullptr;
    if (pShape) {
        pTransform = &pShape->m_transform;
    }
    else {
        pTransform = &hkTransformIdentity;
    }
    NiMatrix3 kRotate;
    NiPoint3 kTranslate;
    HK2NI(kRotate, pTransform->m_rotation);
    HK2NI(kTranslate, pTransform->m_translation);
    pcString = kTranslate.GetViewerString("Trans");
    apStrings->Add(pcString);
    pcString = kRotate.GetViewerString("Rot");
    apStrings->Add(pcString);
    bhkShape* pChild = GetChildShape();
    if (pChild) {
        pChild->GetViewerStrings(apStrings);
    }
}

// Attaching takes a reference on the child shape's wrapper (AddbhkShapeRef), detaching releases it; then the base's
// part.
void bhkConvexTransformShape::Link(bool abAttach) {
    if (abAttach) {
        // EXE-INLINED none: bhkConvexTransformShape::AddbhkShapeRef
        bhkShape* pChild = GetChildShape();
        if (pChild) {
            pChild->IncRefCount();
        }
    }
    else {
        RembhkShapeRef();
    }
    bhkShape::Link(abAttach);
}

// Releases a creation info this shape made: the aligned block it sits in, found through the padding byte in front of
// it, under the Havok heap context.
void bhkConvexTransformShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            // The block is treated as raw memory: the alignment padding byte sits in front of the pointer.
            uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pInfo));
            MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
        }
        pInfo = nullptr;
    }
}

// The size of the creation info in a file: its serialized form's.
uint32_t bhkConvexTransformShape::QCinfoSize() const {
    return sizeof(hkConvexTransformShapeCinfo::Serialized);
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkConvexTransformShape::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkConvexTransformShapeCinfo& rCinfo = static_cast<hkConvexTransformShapeCinfo&>(CreateCinfo(bCreated));
    hkConvexTransformShapeCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

// Builds the Havok shape from the creation info: a block from the thread's Havok heap holding the child shape under the
// info's transform, which the wrapper takes over; the material then comes from the child's own wrapper.
void bhkConvexTransformShape::Init(const hkSerializableCinfo& arInfo) {
    const hkConvexTransformShapeCinfo& rCinfo = static_cast<const hkConvexTransformShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
    hkpConvexTransformShape* pObject =
        static_cast<hkpConvexTransformShape*>(rHeap.blockAlloc(sizeof(hkpConvexTransformShape)));
    pObject->m_memSizeAndFlags = sizeof(hkpConvexTransformShape);
    hkpConvexTransformShape* pShape = ::new (pObject)
        hkpConvexTransformShape(rCinfo.m_childShape, rCinfo.m_transform, hkpShapeContainer::REFERENCE_POLICY_INCREMENT);
    // SethkObject and PostInit are both dispatched virtually, so call them through a base reference.
    bhkSerializable& rThis = *this;
    rThis.SethkObject(pShape);
    pShape->removeReference();
    rThis.PostInit(arInfo);
    if (phkObject) {
        const hkpShape* pChildShape = static_cast<hkpConvexTransformShape*>(phkObject)->m_childShape.m_childShape;
        if (pChildShape) {
            bhkShape* pChildBhk = pChildShape->m_userData;
            if (pChildBhk) {
                eMaterial = static_cast<HavokMaterialType>(static_cast<int32_t>(pChildBhk->eMaterial) & 0x1F);
            }
        }
    }
}

// Builds the creation info: the shape's defaults, then the wrapped shape's own transform and child shape when it
// already has one.
hkSerializableCinfo& bhkConvexTransformShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        uint8_t* pMemory =
            static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkConvexTransformShapeCinfo) + 0x10));
        uint8_t uiPadding = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        uint8_t* pAligned = pMemory + uiPadding;
        *(pAligned - 1) = uiPadding;
        // The block is raw memory: only the creation info's fields are written into it.
        hkConvexTransformShapeCinfo* pCinfo = reinterpret_cast<hkConvexTransformShapeCinfo*>(pAligned);
        pCinfo->m_userData = 0;
        pCinfo->m_radius = 0.1f;
        pCinfo->m_childShape = nullptr;
        // Writes an identity hkTransform: the three rotation columns, then the zero translation.
        hkQuadReal kZero = {0.0f, 0.0f, 0.0f, 0.0f};
        pCinfo->m_transform.m_rotation.m_col0.m_quad = kZero;
        pCinfo->m_transform.m_rotation.m_col1.m_quad = kZero;
        pCinfo->m_transform.m_rotation.m_col2.m_quad = kZero;
        pCinfo->m_transform.m_rotation.m_col0.m_quad[0] = 1.0f;
        pCinfo->m_transform.m_rotation.m_col1.m_quad[1] = 1.0f;
        pCinfo->m_transform.m_rotation.m_col2.m_quad[2] = 1.0f;
        pCinfo->m_transform.m_translation.m_quad = kZero;
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Copies the creation info to the clone: the child shape is cloned when the clone process scales (the transform's
// translation is scaled first) or when the child cannot be shared; else the clone keeps the same Havok child. The clone
// process is always a BSCloningProcess here; its Scale.x is read.
void bhkConvexTransformShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkConvexTransformShapeCinfo& rCinfo = static_cast<hkConvexTransformShapeCinfo&>(CreateCinfo(bCreated));
    bhkShape* pShape = rCinfo.m_childShape ? rCinfo.m_childShape->m_userData : nullptr;
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (1.0f != fScale) {
        // EXE-INLINED none: hkConvexTransformShapeCinfo::Scale
        __m128 kScale = _mm_set_ss(fScale);
        rCinfo.m_transform.m_translation.m_quad =
            _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), rCinfo.m_transform.m_translation.m_quad);
    }
    else if (!pShape || pShape->CanShare(arCloneData)) {
        bhkShape::CopyMembers(apDest, arCloneData);
        return;
    }
    // The scaling path clones without a null test of the child, as the game does.
    bhkConvexShape* pClone = static_cast<bhkConvexShape*>(pShape->CreateClone(arCloneData));
    rCinfo.m_childShape = pClone ? static_cast<hkpConvexShape*>(pClone->phkObject) : nullptr;
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Shareable when the base part is and, where there is a child shape's wrapper, when that is too.
bool bhkConvexTransformShape::CanShare(NiCloningProcess& arCloneData) {
    bool bResult = bhkShape::CanShare(arCloneData);
    if (bResult) {
        bhkShape* pChild = GetChildShape();
        if (pChild) {
            return pChild->CanShare(arCloneData);
        }
    }
    return bResult;
}

// Adds a node named for the class under apNode, placed by the wrapped shape's transform, and lets the child shape's
// wrapper build its own geometry under it.
void bhkConvexTransformShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iOldMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    const hkTransform* pTransform;
    hkpConvexTransformShape* pShape = this ? static_cast<hkpConvexTransformShape*>(phkObject) : nullptr;
    if (pShape) {
        pTransform = &pShape->m_transform;
    }
    else {
        pTransform = &hkTransformIdentity;
    }
    alignas(16) hkTransform kTransform = *pTransform;
    NiMatrix3 kRotate;
    alignas(16) NiPoint3 kTranslate;
    HK2NI(kRotate, kTransform.getRotation());
    HK2NI(kTranslate, kTransform.getTranslation());
    NiNode* pNode = nullptr;
    void* pMemory = NiMemObject::operator new(sizeof(NiNode));
    if (pMemory) {
        pNode = ::new (pMemory) NiNode(0);
    }
    // kName is 16-aligned deliberately, as the game keeps the name's handle.
    alignas(16) NiFixedString kName;
    kName.SetHandle(NiGlobalStringTable::AddString("bhkConvexTransformShape"));
    pNode->SetName(kName);
    // EXE-INLINED 0x4381B0
    NiGlobalStringTable::GlobalStringHandle kHandle = kName.GetHandle();
    if (kHandle != NiGlobalStringTable::NULL_STRING) {
        // The reference count is the first word of the string's buffer, 8 bytes before the characters.
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
    }
    apNode->AttachChild(pNode, true);
    pNode->m_kLocal.m_Translate = kTranslate;
    pNode->m_kLocal.m_Rotate = kRotate;
    if (this && phkObject) {
        const hkpShape* pChildShape = static_cast<hkpConvexTransformShape*>(phkObject)->m_childShape.m_childShape;
        if (pChildShape) {
            bhkShape* pChildBhk = pChildShape->m_userData;
            if (pChildBhk) {
                pChildBhk->BuildDisplayGeometry(pNode, arDefColor);
            }
        }
    }
    riMemContext = iOldMemContext;
}

// The base's part, then the child Havok shape and the shape's transform (the identity without a Havok shape). this is
// tested for null before reading phkObject, as the game does.
void bhkConvexTransformShape::CopyCinfo(hkConvexTransformShapeCinfo& arInfo) {
    bhkConvexShape::CopyCinfo(arInfo);
    const hkpConvexTransformShape* pShape = this ? static_cast<const hkpConvexTransformShape*>(phkObject) : nullptr;
    arInfo.m_childShape = pShape ? const_cast<hkpConvexShape*>(pShape->getChildShape()) : nullptr;
    pShape = this ? static_cast<const hkpConvexTransformShape*>(phkObject) : nullptr;
    const hkTransform* pTransform = pShape ? &pShape->m_transform : &hkTransformIdentity;
    arInfo.m_transform = *pTransform;
}

// Releases the child shape's wrapper in the Havok memory context.
void bhkConvexTransformShape::RembhkShapeRef() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShape* pChild = GetChildShape();
    if (pChild) {
        pChild->DecRefCount();
    }
}

// The class's factory entry: a default constructed shape, the engine's NiImplementCreateObject body.
NiObject* bhkConvexTransformShape::CreateObject() {
    bhkConvexTransformShape* pObject =
        static_cast<bhkConvexTransformShape*>(NiMemObject::operator new(sizeof(bhkConvexTransformShape)));
    if (pObject) {
        return ::new (pObject) bhkConvexTransformShape;
    }
    return nullptr;
}
