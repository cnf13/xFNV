#include "BSHavok/bhkMouseSpringAction.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/bhkUnaryAction.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

NiRTTI bhkMouseSpringAction::ms_RTTI("bhkMouseSpringAction", &bhkUnaryAction::ms_RTTI);

bhkMouseSpringAction::bhkMouseSpringAction() {
    ++iCount;
}

bhkMouseSpringAction::bhkMouseSpringAction(const hkMouseSpringActionCinfo& arInfo) {
    bhkMouseSpringAction::Init(arInfo);
    ++iCount;
}

bhkMouseSpringAction::~bhkMouseSpringAction() {
    bhkSerializable::Kill();
    --iCount;
}

NiObject* bhkMouseSpringAction::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkMouseSpringAction* pClone =
        static_cast<bhkMouseSpringAction*>(NiMemObject::operator new(sizeof(bhkMouseSpringAction)));
    if (pClone) {
        ::new (pClone) bhkMouseSpringAction;
    }
    // The base body is called directly, not through the vtable, so the call is qualified.
    bhkUnaryAction::CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's two positions and its four floats byte-swapped
// in place. The stream's byte is compared with 1: any other byte swaps. A cinfo built here is not killed. Each float's
// four bytes are swapped as an untyped 32-bit value, hence the uint32_t views.
void bhkMouseSpringAction::LoadBinary(NiStream& arStream) {
    bhkUnaryAction::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkMouseSpringActionCinfo& rInfo = static_cast<hkMouseSpringActionCinfo&>(CreateCinfo(bCreated));
        bhkStream::Endian(rInfo.m_positionInRbLocal);
        bhkStream::Endian(rInfo.m_mousePositionInWorld);
        // EXE-INLINED none: BSEndian
        uint32_t* puiSpringDamping = reinterpret_cast<uint32_t*>(&rInfo.m_springDamping);
        *puiSpringDamping = __builtin_bswap32(*puiSpringDamping);
        // EXE-INLINED none: BSEndian
        uint32_t* puiSpringElasticity = reinterpret_cast<uint32_t*>(&rInfo.m_springElasticity);
        *puiSpringElasticity = __builtin_bswap32(*puiSpringElasticity);
        // EXE-INLINED none: BSEndian
        uint32_t* puiMaxRelativeForce = reinterpret_cast<uint32_t*>(&rInfo.m_maxRelativeForce);
        *puiMaxRelativeForce = __builtin_bswap32(*puiMaxRelativeForce);
        // EXE-INLINED none: BSEndian
        uint32_t* puiObjectDamping = reinterpret_cast<uint32_t*>(&rInfo.m_objectDamping);
        *puiObjectDamping = __builtin_bswap32(*puiObjectDamping);
    }
}

// For a big-endian save, the creation info's two positions and four floats are byte-swapped in place around the base's
// write and swapped back after it. The stream's byte is compared with 1 and read again after the base call. Each
// float's four bytes are swapped as an untyped 32-bit word, hence the uint32_t views.
void bhkMouseSpringAction::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkMouseSpringActionCinfo& rInfo = static_cast<hkMouseSpringActionCinfo&>(CreateCinfo(bCreated));
    uint32_t* puiSpringDamping = reinterpret_cast<uint32_t*>(&rInfo.m_springDamping);
    uint32_t* puiSpringElasticity = reinterpret_cast<uint32_t*>(&rInfo.m_springElasticity);
    uint32_t* puiMaxRelativeForce = reinterpret_cast<uint32_t*>(&rInfo.m_maxRelativeForce);
    uint32_t* puiObjectDamping = reinterpret_cast<uint32_t*>(&rInfo.m_objectDamping);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(rInfo.m_positionInRbLocal);
        bhkStream::Endian(rInfo.m_mousePositionInWorld);
        // EXE-INLINED none: BSEndian
        *puiSpringDamping = __builtin_bswap32(*puiSpringDamping);
        // EXE-INLINED none: BSEndian
        *puiSpringElasticity = __builtin_bswap32(*puiSpringElasticity);
        // EXE-INLINED none: BSEndian
        *puiMaxRelativeForce = __builtin_bswap32(*puiMaxRelativeForce);
        // EXE-INLINED none: BSEndian
        *puiObjectDamping = __builtin_bswap32(*puiObjectDamping);
    }
    bhkUnaryAction::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(rInfo.m_positionInRbLocal);
        bhkStream::Endian(rInfo.m_mousePositionInWorld);
        // EXE-INLINED none: BSEndian
        *puiSpringDamping = __builtin_bswap32(*puiSpringDamping);
        // EXE-INLINED none: BSEndian
        *puiSpringElasticity = __builtin_bswap32(*puiSpringElasticity);
        // EXE-INLINED none: BSEndian
        *puiMaxRelativeForce = __builtin_bswap32(*puiMaxRelativeForce);
        // EXE-INLINED none: BSEndian
        *puiObjectDamping = __builtin_bswap32(*puiObjectDamping);
    }
}

void bhkMouseSpringAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkUnaryAction::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkMouseSpringAction::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* a3 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, a3);
    hkMouseSpringActionCinfo arInfo;
    arInfo.m_userData = 0;
    arInfo.m_body = nullptr;
    CopyCinfo(arInfo);
    char* ViewerString_7 = NiGetViewerString("Damping", arInfo.m_springDamping);
    uint32_t v9 = apStrings->m_usSize;
    uint32_t v10 = apStrings->m_usMaxSize;
    a3 = ViewerString_7;
    if (v9 >= v10) {
        apStrings->SetSize(v9 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v9, a3);
    char* v11 = NiGetViewerString("Elasticity", arInfo.m_springElasticity);
    uint32_t v12 = apStrings->m_usSize;
    uint32_t v13 = apStrings->m_usMaxSize;
    a3 = v11;
    if (v12 >= v13) {
        apStrings->SetSize(v12 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v12, a3);
    char* v14 = NiGetViewerString("Object Damping", arInfo.m_objectDamping);
    uint32_t v15 = apStrings->m_usSize;
    uint32_t v16 = apStrings->m_usMaxSize;
    a3 = v14;
    if (v15 >= v16) {
        apStrings->SetSize(v15 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v15, a3);
    char* v17 = NiGetViewerString("Max Force", arInfo.m_maxRelativeForce);
    uint32_t v18 = apStrings->m_usSize;
    uint32_t v19 = apStrings->m_usMaxSize;
    a3 = v17;
    if (v18 >= v19) {
        apStrings->SetSize(v18 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v18, a3);
    v3->eMemContext = eMemContext;
}

// Releases a creation info this action made: the aligned block it sits in, found through the padding byte in front of
// it, under the Havok heap context.
void bhkMouseSpringAction::KillCinfo(bool abCreated) {
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

uint32_t bhkMouseSpringAction::QCinfoSize() const {
    return 0x40;
}

// Builds the wrapped hkpMouseSpringAction in the Havok heap context: a block from the heap allocator with its size word
// set, the full constructor with the creation info's two positions (each converted with bhkVector4::ToHK into a
// 16-byte-aligned local), damping, elasticity, object damping and body and no callbacks, then its force limit,
// SethkObject, the constructor's reference dropped and PostInit.
void bhkMouseSpringAction::Init(const hkSerializableCinfo& arInfo) {
    const hkMouseSpringActionCinfo& cinfo = static_cast<const hkMouseSpringActionCinfo&>(arInfo);
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    hkpMouseSpringAction* pAction = static_cast<hkpMouseSpringAction*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpMouseSpringAction))
    );
    pAction->m_memSizeAndFlags = sizeof(hkpMouseSpringAction);
    hkVector4 kMousePositionInWorld;
    // EXE-INLINED 0x4B4D10
    kMousePositionInWorld.set(
        cinfo.m_mousePositionInWorld.fX, cinfo.m_mousePositionInWorld.fY, cinfo.m_mousePositionInWorld.fZ,
        cinfo.m_mousePositionInWorld.fW
    );
    hkVector4 kPositionInRbLocal;
    // EXE-INLINED 0x4B4D10
    kPositionInRbLocal.set(
        cinfo.m_positionInRbLocal.fX, cinfo.m_positionInRbLocal.fY, cinfo.m_positionInRbLocal.fZ,
        cinfo.m_positionInRbLocal.fW
    );
    pAction = ::new (pAction) hkpMouseSpringAction(
        kPositionInRbLocal, kMousePositionInWorld, cinfo.m_springDamping, cinfo.m_springElasticity,
        cinfo.m_objectDamping, cinfo.m_body, nullptr
    );
    pAction->setMaxRelativeForce(cinfo.m_maxRelativeForce);
    bhkSerializable* pSerializable = this;
    pSerializable->SethkObject(pAction);
    pAction->removeReference();
    pSerializable->PostInit(arInfo);
    v3->eMemContext = eMemContext;
}

hkSerializableCinfo& bhkMouseSpringAction::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        // 0x50 bytes: the cinfo (0x40) plus the slack the 16-byte alignment below can consume.
        void* pRaw = MemoryManager_Singleton.Allocate(sizeof(hkMouseSpringActionCinfo) + 0x10);
        // Aligns the block up to 16 bytes and keeps the offset in the byte before the cinfo.
        uint8_t uiOffset = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pRaw) & 0xF));
        char* pAligned = static_cast<char*>(pRaw) + uiOffset;
        pAligned[-1] = static_cast<char>(uiOffset);
        hkMouseSpringActionCinfo* pCinfo = ::new (pAligned) hkMouseSpringActionCinfo;
        pInfo = pCinfo;
        if (phkObject != nullptr) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

hkpEntity* bhkMouseSpringAction::GetEntity() {
    hkpMouseSpringAction* pMouseSpringAction = operator*();
    if (pMouseSpringAction) {
        return static_cast<hkpEntity*>(pMouseSpringAction->GetEntity());
    }

    return nullptr;
}

// Copies the unary action's fields, then the wrapped action's damping, elasticity and object damping through null-safe
// getters (0 when this or the wrapped object is null). Then, with a wrapped action, its two positions and its force
// limit; without one, both vectors are zeroed and the limit is 0.
void bhkMouseSpringAction::CopyCinfo(hkMouseSpringActionCinfo& arInfo) {
    bhkUnaryAction::CopyCinfo(arInfo);
    // EXE-INLINED none: bhkMouseSpringAction::GetDamping
    float fDamping = (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject)->m_springDamping : 0.0f;
    arInfo.m_springDamping = fDamping;
    // EXE-INLINED none: bhkMouseSpringAction::GetElasticity
    float fElasticity = (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject)->m_springElasticity : 0.0f;
    arInfo.m_springElasticity = fElasticity;
    // EXE-INLINED none: bhkMouseSpringAction::GetObjectDamping
    float fObjectDamping = (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject)->m_objectDamping : 0.0f;
    arInfo.m_objectDamping = fObjectDamping;
    // EXE-INLINED 0x4AE750
    hkpMouseSpringAction* pAction = (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject) : nullptr;
    if (pAction) {
        arInfo.m_positionInRbLocal = pAction->m_positionInRbLocal;
        arInfo.m_mousePositionInWorld = pAction->m_mousePositionInWorld;
        arInfo.m_maxRelativeForce = pAction->m_maxRelativeForce;
    }
    else {
        // EXE-INLINED 0x4B4FC0
        arInfo.m_positionInRbLocal.fX = 0.0f;
        arInfo.m_positionInRbLocal.fY = 0.0f;
        arInfo.m_positionInRbLocal.fZ = 0.0f;
        arInfo.m_positionInRbLocal.fW = 0.0f;
        // EXE-INLINED 0x4B4FC0
        arInfo.m_mousePositionInWorld.fX = 0.0f;
        arInfo.m_mousePositionInWorld.fY = 0.0f;
        arInfo.m_mousePositionInWorld.fZ = 0.0f;
        arInfo.m_mousePositionInWorld.fW = 0.0f;
        arInfo.m_maxRelativeForce = 0.0f;
    }
}

bhkWorldObject* bhkMouseSpringAction::GetWorldObject() {
    hkpRigidBody* entity = static_cast<hkpRigidBody*>(GetEntity());
    return bhkEntity::Getbhk(entity);
}

NiObject* bhkMouseSpringAction::CreateObject() {
    bhkMouseSpringAction* pObject =
        static_cast<bhkMouseSpringAction*>(NiMemObject::operator new(sizeof(bhkMouseSpringAction)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) bhkMouseSpringAction;
}
