#include "BSHavok/bhkDashpotAction.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkBinaryAction.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Utilities/Actions/Dashpot/hkpDashpotAction.h"
#include "SDK/MSVC/string.h"

#include <bit>

NiRTTI bhkDashpotAction::ms_RTTI("bhkDashpotAction", &bhkBinaryAction::ms_RTTI);

bhkDashpotAction::bhkDashpotAction() {
    ++iCount;
}

bhkDashpotAction::~bhkDashpotAction() {
    bhkSerializable::Kill();
    --bhkDashpotAction::iCount;
}

const NiRTTI* bhkDashpotAction::GetRTTI() const {
    return &bhkDashpotAction::ms_RTTI;
}

// The clone is charged to the Havok heap context, built as in CreateObject, then given the members through the virtual
// CopyMembers.
NiObject* bhkDashpotAction::CreateClone(NiCloningProcess& arCloneData) {
    int32_t& eMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = eMemContext;
    eMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkDashpotAction* pClone = static_cast<bhkDashpotAction*>(NiMemObject::operator new(sizeof(bhkDashpotAction)));
    if (pClone) {
        ::new (pClone) bhkDashpotAction;
    }
    else {
        pClone = nullptr;
    }
    static_cast<bhkBinaryAction*>(this)->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    eMemContext = iSavedMemContext;
    return pClone;
}

// For a big-endian file the creation info's two pivots, strength and damping are swapped after the base's load. The
// flag's byte is tested against 1, not for zero, so the byte is read as untyped memory.
void bhkDashpotAction::LoadBinary(NiStream& arStream) {
    bhkBinaryAction::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkDashpotActionCinfo& rInfo = static_cast<hkDashpotActionCinfo&>(CreateCinfo(bCreated));
        rInfo.m_pivotA.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fX)));
        rInfo.m_pivotA.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fY)));
        rInfo.m_pivotA.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fZ)));
        rInfo.m_pivotA.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fW)));
        rInfo.m_pivotB.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fX)));
        rInfo.m_pivotB.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fY)));
        rInfo.m_pivotB.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fZ)));
        rInfo.m_pivotB.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fW)));
        rInfo.m_strength = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_strength)));
        rInfo.m_damping = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_damping)));
    }
}

// For a big-endian file the creation info's two pivots, strength and damping are swapped around the base's save, then
// swapped back. The stream's flag is re-read for each check and its byte is tested against 1.
void bhkDashpotAction::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkDashpotActionCinfo& rInfo = static_cast<hkDashpotActionCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rInfo.m_pivotA.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fX)));
        rInfo.m_pivotA.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fY)));
        rInfo.m_pivotA.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fZ)));
        rInfo.m_pivotA.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fW)));
        rInfo.m_pivotB.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fX)));
        rInfo.m_pivotB.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fY)));
        rInfo.m_pivotB.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fZ)));
        rInfo.m_pivotB.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fW)));
        rInfo.m_strength = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_strength)));
        rInfo.m_damping = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_damping)));
    }
    bhkBinaryAction::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rInfo.m_pivotA.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fX)));
        rInfo.m_pivotA.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fY)));
        rInfo.m_pivotA.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fZ)));
        rInfo.m_pivotA.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotA.fW)));
        rInfo.m_pivotB.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fX)));
        rInfo.m_pivotB.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fY)));
        rInfo.m_pivotB.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fZ)));
        rInfo.m_pivotB.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_pivotB.fW)));
        rInfo.m_strength = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_strength)));
        rInfo.m_damping = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.m_damping)));
    }
}

void bhkDashpotAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    XFNV_INLINED_LOCAL(hkDashpotActionCinfo, arCinfo, std::nullopt, std::nullopt);
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    bhkBinaryAction::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkDashpotAction::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* a3 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, a3);
    arCinfo.m_pivotA.fX = 0.0f;
    arCinfo.m_pivotA.fY = 0.0f;
    arCinfo.m_pivotA.fZ = 0.0f;
    arCinfo.m_pivotA.fW = 0.0f;
    arCinfo.m_pivotB.fX = 0.0f;
    // Zeroes the binary action's part of the info (user data and both bodies).
    std::memset(static_cast<hkBinaryActionCinfo*>(&arCinfo), 0, sizeof(hkBinaryActionCinfo));
    arCinfo.m_pivotB.fY = 0.0f;
    arCinfo.m_pivotB.fZ = 0.0f;
    arCinfo.m_pivotB.fW = 0.0f;
    arCinfo.m_strength = 0.1f;
    arCinfo.m_damping = 0.01f;
    this->CopyCinfo(arCinfo);
    v2->eMemContext = eMemContext;
}

uint32_t bhkDashpotAction::QCinfoSize() const {
    return (uint32_t)64;
}

// A new hkpDashpotAction from the thread's heap over the cinfo's two bodies (user data 0), given the cinfo's points,
// strength and damping, handed to SethkObject (which takes its own reference), then the creation info is dropped.
void bhkDashpotAction::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkDashpotActionCinfo& rCinfo = static_cast<const hkDashpotActionCinfo&>(arInfo);
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpDashpotAction));
    // EXE-INLINED none: hkReferencedObject::operator new
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpDashpotAction);
    hkpDashpotAction* pAction = ::new (pMemory) hkpDashpotAction(rCinfo.m_bodyA, rCinfo.m_bodyB, 0);
    // EXE-INLINED 0x4B4D10
    pAction->m_point[1].set(rCinfo.m_pivotB.fX, rCinfo.m_pivotB.fY, rCinfo.m_pivotB.fZ, rCinfo.m_pivotB.fW);
    // EXE-INLINED 0x4B4D10
    pAction->m_point[0].set(rCinfo.m_pivotA.fX, rCinfo.m_pivotA.fY, rCinfo.m_pivotA.fZ, rCinfo.m_pivotA.fW);
    pAction->m_strength = rCinfo.m_strength;
    pAction->m_damping = rCinfo.m_damping;
    SethkObject(pAction);
    pAction->removeReference();
    PostInit(arInfo);
}

// Builds the creation info, from the wrapped action when there is one and from the dashpot's defaults otherwise. The
// engine's aligned allocation is done here: the padding length sits in the byte before the block, and the block holds
// the cinfo's size plus the 16 bytes alignment can need. The four padding bytes before m_pivotA are deliberately left
// unwritten, as the game does.
hkSerializableCinfo& bhkDashpotAction::CreateCinfo(bool& arCreated) {
    int32_t& eMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = eMemContext;
    eMemContext = MEM_CONTEXT_HAVOK;
    arCreated = pInfo == nullptr;
    if (arCreated) {
        uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkDashpotActionCinfo) + 16));
        uint8_t uiPadding = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        // Raw memory the engine handed back, aligned here; the cinfo is laid over it.
        hkDashpotActionCinfo* pCinfo = reinterpret_cast<hkDashpotActionCinfo*>(pMemory + uiPadding);
        pMemory[uiPadding - 1] = uiPadding;
        pCinfo->m_userData = 0;
        pCinfo->m_bodyA = nullptr;
        pCinfo->m_bodyB = nullptr;
        pCinfo->m_pivotA.fX = 0.0f;
        pCinfo->m_pivotA.fY = 0.0f;
        pCinfo->m_pivotA.fZ = 0.0f;
        pCinfo->m_pivotA.fW = 0.0f;
        pCinfo->m_pivotB.fX = 0.0f;
        pCinfo->m_pivotB.fY = 0.0f;
        pCinfo->m_pivotB.fZ = 0.0f;
        pCinfo->m_pivotB.fW = 0.0f;
        pCinfo->m_strength = 0.1f;
        pCinfo->m_damping = 0.01f;
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    hkSerializableCinfo* pResult = pInfo;
    eMemContext = iSavedMemContext;
    return *pResult;
}

// Frees a creation info this action made, with the engine's aligned free, and forgets it.
void bhkDashpotAction::KillCinfo(bool abCreated) {
    if (abCreated) {
        if (pInfo) {
            // The block is treated as raw memory: the alignment padding byte sits in front of the pointer.
            uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pInfo));
            MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
        }
        pInfo = nullptr;
    }
}

// Scales the creation info's pivots by the cloning process's scale when it is not 1, then copies the binary action's
// members.
void bhkDashpotAction::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkDashpotActionCinfo& rInfo = static_cast<hkDashpotActionCinfo&>(CreateCinfo(bCreated));
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        rInfo.m_pivotA.fX = rInfo.m_pivotA.fX * fScale;
        rInfo.m_pivotA.fY = fScale * rInfo.m_pivotA.fY;
        rInfo.m_pivotA.fZ = rInfo.m_pivotA.fZ * fScale;
        rInfo.m_pivotA.fW = fScale * rInfo.m_pivotA.fW;
        rInfo.m_pivotB.fX = fScale * rInfo.m_pivotB.fX;
        rInfo.m_pivotB.fY = fScale * rInfo.m_pivotB.fY;
        rInfo.m_pivotB.fZ = rInfo.m_pivotB.fZ * fScale;
        rInfo.m_pivotB.fW = fScale * rInfo.m_pivotB.fW;
    }
    bhkBinaryAction::CopyMembers(apDest, arCloneData);
}

// Fills arInfo from the wrapped hkpDashpotAction: the binary action's part, the pivots, strength and damping, or zeros
// without an action. Both pivots are assigned to m_pivotA (the second over the first) and m_pivotB is left as it was,
// deliberately, as the game does.
void bhkDashpotAction::CopyCinfo(hkDashpotActionCinfo& arInfo) {
    bhkBinaryAction::CopyCinfo(arInfo);
    alignas(16) hkVector4 kPivotA;
    alignas(16) hkVector4 kPivotB;
    kPivotA.set(0.0f, 0.0f, 0.0f, 0.0f);
    kPivotB = kPivotA;
    if (this) {
        hkpDashpotAction* pAction = static_cast<hkpDashpotAction*>(phkObject);
        if (pAction) {
            kPivotA = pAction->m_point[0];
            kPivotB = pAction->m_point[1];
        }
    }
    arInfo.m_pivotA = kPivotA;
    arInfo.m_pivotA = kPivotB;
    float fStrength;
    if (this && phkObject) {
        fStrength = static_cast<hkpDashpotAction*>(phkObject)->m_strength;
    }
    else {
        fStrength = 0.0f;
    }
    arInfo.m_strength = fStrength;
    float fDamping = 0.0f;
    if (this && phkObject) {
        fDamping = static_cast<hkpDashpotAction*>(phkObject)->m_damping;
    }
    arInfo.m_damping = fDamping;
}

NiObject* bhkDashpotAction::CreateObject() {
    bhkDashpotAction* pObject = static_cast<bhkDashpotAction*>(NiMemObject::operator new(sizeof(bhkDashpotAction)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) bhkDashpotAction;
}
