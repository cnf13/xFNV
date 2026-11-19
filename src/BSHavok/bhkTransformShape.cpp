#include "BSHavok/bhkTransformShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkMatrix3.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkTransformShape::ms_RTTI("bhkTransformShape", &bhkShape::ms_RTTI);

bhkTransformShape::bhkTransformShape() {
    ++iCount;
}

bhkTransformShape::bhkTransformShape(const hkTransformShapeCinfo& arInfo) {
    bhkTransformShape::Init(arInfo);
    ++iCount;
}

bhkTransformShape::~bhkTransformShape() {
    bhkSerializable::Kill();
    --iCount;
}

// Clones the shape under the Havok heap context and the clone lock: a new shape filled by CopyMembers through the
// vtable.
NiObject* bhkTransformShape::CreateClone(NiCloningProcess& arCloneData) {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iOldMemContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkTransformShape* pClone = static_cast<bhkTransformShape*>(NiMemObject::operator new(sizeof(bhkTransformShape)));
    if (pClone) {
        ::new (pClone) bhkTransformShape;
    }
    bhkShape* pThis = this;
    pThis->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    riMemContext = iOldMemContext;
    return pClone;
}

// The child's link ID, the base's part, then the creation info's transform with the child cleared: byte-swapped
// when the source is not little-endian, and the rotation renormalized for files before version 50.
void bhkTransformShape::LoadBinary(NiStream& arStream) {
    arStream.ReadLinkID();
    bhkShape::LoadBinary(arStream);
    bool bCreated;
    hkTransformShapeCinfo& rCinfo = static_cast<hkTransformShapeCinfo&>(CreateCinfo(bCreated));
    rCinfo.m_childShape = nullptr;
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bhkStream::Endian(rCinfo.m_transform);
    }
    if (arStream.m_BSStreamHeader.uiVersion < 50) {
        rCinfo.m_transform.Rot.Renormalize();
    }
}

void bhkTransformShape::LinkObject(NiStream& arStream) {
    bool v5;
    hkTransformShapeCinfo* cinfo = &static_cast<hkTransformShapeCinfo&>(CreateCinfo(v5));
    bhkShape* ps = static_cast<bhkShape*>(arStream.GetObjectFromLinkID());
    if (ps) {
        cinfo->m_childShape = static_cast<hkpShape*>(ps->phkObject);
    }
    else {
        cinfo->m_childShape = nullptr;
    }
    bhkShape::LinkObject(arStream);
}

// Registers the child shape's wrapper, when there is one, then the base's part.
bool bhkTransformShape::RegisterStreamables(NiStream& arStream) {
    bhkShape* ps = GetChildShape();
    if (ps) {
        ps->RegisterStreamables(arStream);
    }
    return bhkShape::RegisterStreamables(arStream);
}

// The child's link ID, then the base's part with the creation info's child cleared and its transform byte-swapped
// for a big-endian save; the transform is swapped back afterwards.
void bhkTransformShape::SaveBinary(NiStream& arStream) {
    arStream.SaveLinkID(GetChildShape());
    bool bCreated;
    hkTransformShapeCinfo& rCinfo = static_cast<hkTransformShapeCinfo&>(CreateCinfo(bCreated));
    rCinfo.m_childShape = nullptr;
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(rCinfo.m_transform);
    }
    bhkShape::SaveBinary(arStream);
    rCinfo.m_childShape = nullptr;
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(rCinfo.m_transform);
    }
}

bool bhkTransformShape::IsEqual(NiObject* apObject) {
    return bhkShape::IsEqual(apObject);
}

// The base's lines, the class name, the shape's translation and rotation (the identity without a Havok shape), then the
// child shape's wrapper's lines.
void bhkTransformShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    const hkTransform* pTransform;
    hkpTransformShape* pShape = this ? static_cast<hkpTransformShape*>(phkObject) : nullptr;
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
    bhkShape* pChildBhk = GetChildShape();
    if (pChildBhk) {
        pChildBhk->GetViewerStrings(apStrings);
    }
}

// Attaching takes a reference on the child shape's wrapper, detaching releases it; then the base's part.
void bhkTransformShape::Link(bool abAttach) {
    if (abAttach) {
        // EXE-INLINED none: bhkTransformShape::AddbhkShapeRef
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

// Releases the creation info a CreateCinfo made: the aligned block it sits in, found through the padding byte in front
// of it, under the Havok heap context.
void bhkTransformShape::KillCinfo(bool abCreated) {
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
uint32_t bhkTransformShape::QCinfoSize() const {
    return sizeof(hkTransformShapeCinfo::Serialized);
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkTransformShape::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkTransformShapeCinfo& rCinfo = static_cast<hkTransformShapeCinfo&>(CreateCinfo(bCreated));
    hkTransformShapeCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

// Builds the Havok shape from the creation info: a block from the thread's Havok heap holding the child shape under the
// info's transform, which the wrapper takes over; the material then comes from the child shape's wrapper.
void bhkTransformShape::Init(const hkSerializableCinfo& arInfo) {
    const hkTransformShapeCinfo& rCinfo = static_cast<const hkTransformShapeCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpTransformShape* pObject =
        static_cast<hkpTransformShape*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpTransformShape)));
    pObject->m_memSizeAndFlags = sizeof(hkpTransformShape);
    const bhkMatrix3& rRot = rCinfo.m_transform.Rot;
    const bhkVector4& rTrans = rCinfo.m_transform.Trans;
    alignas(16) hkTransform kTransform;
    kTransform.m_rotation.m_col0.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(rRot.Col0.fX), _mm_set_ss(rRot.Col0.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(rRot.Col0.fY), _mm_set_ss(rRot.Col0.fW))
    );
    kTransform.m_rotation.m_col1.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(rRot.Col1.fX), _mm_set_ss(rRot.Col1.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(rRot.Col1.fY), _mm_set_ss(rRot.Col1.fW))
    );
    kTransform.m_rotation.m_col2.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(rRot.Col2.fX), _mm_set_ss(rRot.Col2.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(rRot.Col2.fY), _mm_set_ss(rRot.Col2.fW))
    );
    kTransform.m_translation.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(rTrans.fX), _mm_set_ss(rTrans.fZ)),
        _mm_unpacklo_ps(_mm_set_ss(rTrans.fY), _mm_set_ss(rTrans.fW))
    );
    hkpTransformShape* pShape = ::new (pObject) hkpTransformShape(rCinfo.m_childShape, kTransform);
    // SethkObject and PostInit are called through a base reference so that both dispatch virtually.
    bhkSerializable& rThis = *this;
    rThis.SethkObject(pShape);
    pShape->removeReference();
    rThis.PostInit(arInfo);
    if (phkObject) {
        const hkpShape* pChildShape = static_cast<hkpTransformShape*>(phkObject)->m_childShape.m_childShape;
        if (pChildShape) {
            bhkShape* pChildBhk = pChildShape->m_userData;
            if (pChildBhk) {
                eMaterial = static_cast<HavokMaterialType>(static_cast<int32_t>(pChildBhk->eMaterial) & 0x1F);
            }
        }
    }
}

// Makes the creation info on first use and fills it from the wrapped shape when there is one.
hkSerializableCinfo& bhkTransformShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        uint8_t* pMemory =
            static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkTransformShapeCinfo) + 0x10u));
        const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        pMemory += ucPadding;
        pMemory[-1] = ucPadding;
        hkTransformShapeCinfo* pCinfo = ::new (pMemory) hkTransformShapeCinfo;
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Clones or shares the child shape for the copy: with a cloning scale other than 1 the creation info's translation
// (all four words) is scaled and the child always cloned; otherwise it is cloned only when it cannot be shared. The
// creation info then takes the clone's Havok shape. arCloneData is a BSCloningProcess (its scale's x).
void bhkTransformShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkTransformShapeCinfo& rCinfo = static_cast<hkTransformShapeCinfo&>(CreateCinfo(bCreated));
    // EXE-INLINED 0x4A3A40
    bhkShape* ps = rCinfo.m_childShape ? rCinfo.m_childShape->m_userData : nullptr;
    // EXE-INLINED none: BSCloningProcess::QScaleU
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        rCinfo.m_transform.Trans.fX = rCinfo.m_transform.Trans.fX * fScale;
        rCinfo.m_transform.Trans.fY = rCinfo.m_transform.Trans.fY * fScale;
        rCinfo.m_transform.Trans.fZ = rCinfo.m_transform.Trans.fZ * fScale;
        rCinfo.m_transform.Trans.fW = fScale * rCinfo.m_transform.Trans.fW;
    }
    else if (!ps || ps->CanShare(arCloneData)) {
        bhkShape::CopyMembers(apDest, arCloneData);
        return;
    }
    bhkShape* pnewS = static_cast<bhkShape*>(ps->CreateClone(arCloneData));
    rCinfo.m_childShape = pnewS ? static_cast<hkpShape*>(pnewS->phkObject) : nullptr;
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Shareable when the base says so and, when there is a child shape wrapper, the child is too.
bool bhkTransformShape::CanShare(NiCloningProcess& arCloneData) {
    bool bres = bhkShape::CanShare(arCloneData);
    if (bres) {
        bhkShape* ps = GetChildShape();
        if (ps) {
            return ps->CanShare(arCloneData);
        }
    }
    return bres;
}

// Adds a node named for the class under apNode, placed by the wrapped shape's transform (the identity without a Havok
// shape), and lets the child shape's wrapper build its own geometry under it.
void bhkTransformShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkTransform* pTransform;
    hkpTransformShape* pShape = this ? static_cast<hkpTransformShape*>(phkObject) : nullptr;
    if (pShape) {
        pTransform = &pShape->m_transform;
    }
    else {
        pTransform = &hkTransformIdentity;
    }
    alignas(16) hkTransform kTransform = *pTransform;
    NiMatrix3 kRotate;
    alignas(16) NiPoint3 kTranslate;
    HK2NI(kRotate, kTransform.m_rotation);
    HK2NI(kTranslate, kTransform.m_translation);
    NiNode* pNode = nullptr;
    void* pMemory = NiMemObject::operator new(sizeof(NiNode));
    if (pMemory) {
        pNode = ::new (pMemory) NiNode(0);
    }
    alignas(16) NiFixedString kName;
    kName.SetHandle(NiGlobalStringTable::AddString("bhkTransformShape"));
    pNode->SetName(kName);
    apNode->AttachChild(pNode, true);
    pNode->m_kLocal.m_Translate = kTranslate;
    pNode->m_kLocal.m_Rotate = kRotate;
    bhkShape* pChildBhk = GetChildShape();
    if (pChildBhk) {
        pChildBhk->BuildDisplayGeometry(pNode, arDefColor);
    }
}

bhkShape* bhkTransformShape::GetChildShape() {
    hkpTransformShape* pShape = **this;
    return bhkShape::Getbhk(pShape ? pShape->getChildShape() : nullptr);
}

void bhkTransformShape::RembhkShapeRef() {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    // EXE-INLINED 0x4D8630
    // EXE-INLINED 0x4AE750
    hkpTransformShape* pShape = this ? static_cast<hkpTransformShape*>(phkObject) : nullptr;
    if (pShape) {
        const hkpShape* pChild = pShape->getChildShape();
        // EXE-INLINED 0x4A3A40
        bhkShape* psc = pChild ? pChild->m_userData : nullptr;
        if (psc) {
            psc->DecRefCount();
        }
    }
    pTLS->eMemContext = eMemContext;
}

// The base's part, then the Havok shape's child and transform (the identity without a Havok shape) into arInfo; the
// transform is copied to the stack whole, then column by column through bhkVector4::operator=.
void bhkTransformShape::CopyCinfo(hkTransformShapeCinfo& arInfo) {
    hkVector4 kCol0;
    hkVector4 kCol1;
    hkVector4 kCol2;
    hkVector4 kTranslation;
    bhkShape::CopyCinfo(arInfo);
    // EXE-INLINED 0x4AE750
    hkpTransformShape* pShape = this ? static_cast<hkpTransformShape*>(phkObject) : nullptr;
    arInfo.m_childShape = pShape ? pShape->getChildShape() : nullptr;
    // EXE-INLINED 0x4AE750
    hkpTransformShape* pTransformShape = this ? static_cast<hkpTransformShape*>(phkObject) : nullptr;
    const hkTransform* pTransform = pTransformShape ? &pTransformShape->m_transform : &hkTransformIdentity;
    kCol0.m_quad = pTransform->m_rotation.m_col0.m_quad;
    kCol1.m_quad = pTransform->m_rotation.m_col1.m_quad;
    kCol2.m_quad = pTransform->m_rotation.m_col2.m_quad;
    kTranslation.m_quad = pTransform->m_translation.m_quad;
    arInfo.m_transform.Rot.Col0.operator=(kCol0);
    arInfo.m_transform.Rot.Col1.operator=(kCol1);
    arInfo.m_transform.Rot.Col2.operator=(kCol2);
    arInfo.m_transform.Trans.operator=(kTranslation);
}

// The streaming factory: a new shape with no Havok object yet.
NiObject* bhkTransformShape::CreateObject() {
    bhkTransformShape* pObject = static_cast<bhkTransformShape*>(NiMemObject::operator new(sizeof(bhkTransformShape)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) bhkTransformShape;
}

void bhkMatrix3::Renormalize() {
    hkVector4 v11;
    hkRotation a3;
    bhkVector4* p_Col1 = &Col1;
    bhkVector4* p_Col2 = &Col2;
    __m128 fZ_low = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col1.fZ)));
    __m128 fW_low = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col1.fW)));
    __m128 fY_low = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col1.fY)));
    a3.m_col0.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col0.fX))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col0.fZ)))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col0.fY))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col0.fW)))
        )
    );
    __m128 v7 = _mm_unpacklo_ps(fY_low, fW_low);
    __m128 v8 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col2.fW)));
    __m128 v9 = _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col1.fX))), fZ_low);
    __m128 v10 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col2.fZ)));
    v11.m_quad = _mm_unpacklo_ps(v9, v7);
    __m128 v12 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col2.fY)));
    a3.m_col1.m_quad = v11.m_quad;
    a3.m_col2.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(Col2.fX))), v10),
        _mm_unpacklo_ps(v12, v8)
    );
    a3.renormalize();
    Col0.operator=(a3.m_col0);
    p_Col1->operator=(a3.m_col1);
    p_Col2->operator=(a3.m_col2);
}
