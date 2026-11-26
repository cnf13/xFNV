#include "BSHavok/bhkSpringAction.h"

#include "BSCore/BSCoreDefines.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkBinaryAction.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"

NiRTTI bhkSpringAction::ms_RTTI("bhkSpringAction", &bhkBinaryAction::ms_RTTI);

bhkSpringAction::bhkSpringAction() {
    ++iCount;
}

bhkSpringAction::~bhkSpringAction() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkSpringAction::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* bhkSpringAction::CreateClone(NiCloningProcess& arCloneData) {
    bhkSerializable::CloneCritLock();
    bhkSpringAction* pClone = new bhkSpringAction;
    // Called through the base, so the class's CopyMembers is reached through the vtable.
    static_cast<bhkBinaryAction*>(this)->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's pivots (bhkStream::Endian) and its rest length,
// strength and damping (BSEndian) byte-swapped in place. The stream's byte is compared with 1: any other byte swaps, so
// it is read as a byte rather than a bool tested. A cinfo built here is not killed.
void bhkSpringAction::LoadBinary(NiStream& arStream) {
    bhkBinaryAction::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkSpringActionCinfo& rInfo = static_cast<hkSpringActionCinfo&>(CreateCinfo(bCreated));
        bhkStream::Endian(rInfo.m_pivotA);
        bhkStream::Endian(rInfo.m_pivotB);
        BSEndian(rInfo.m_restLength, false);
        BSEndian(rInfo.m_strength, false);
        BSEndian(rInfo.m_damping, false);
    }
}

// For a big-endian save, the creation info's pivots, rest length, strength and damping are byte-swapped in place around
// the base's write and swapped back after it. CreateCinfo is called virtually and the info it may make is deliberately
// not killed. The stream's byte is compared with 1 before and after the base's write: any other byte
// swaps, so it is read as a byte rather than a bool tested.
void bhkSpringAction::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkSpringActionCinfo& rInfo = static_cast<hkSpringActionCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(rInfo.m_pivotA);
        bhkStream::Endian(rInfo.m_pivotB);
        BSEndian(rInfo.m_restLength, false);
        BSEndian(rInfo.m_strength, false);
        BSEndian(rInfo.m_damping, false);
    }
    bhkBinaryAction::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(rInfo.m_pivotA);
        bhkStream::Endian(rInfo.m_pivotB);
        BSEndian(rInfo.m_restLength, false);
        BSEndian(rInfo.m_strength, false);
        BSEndian(rInfo.m_damping, false);
    }
}

// The binary action's own lines, then the class's RTTI heading appended to the panel's array.
void bhkSpringAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkBinaryAction::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
    apStrings->Add(pcString);
}

// Releases a creation info this action made, under the Havok heap context.
void bhkSpringAction::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkSpringActionCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

// The size of the action's creation info, hkSpringActionCinfo.
uint32_t bhkSpringAction::QCinfoSize() const {
    return 0x40;
}

// Builds the Havok spring in the Havok heap over the cinfo's two bodies (user data 0): the block's size in its header,
// the constructor, the cinfo's strength, damping, rest length and both flags, then both pivots in body space, then the
// wrapper's hooks through the vtable.
void bhkSpringAction::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkSpringActionCinfo& rInfo = static_cast<const hkSpringActionCinfo&>(arInfo);
    hkpSpringAction* pAction = new hkpSpringAction(rInfo.m_bodyA, rInfo.m_bodyB, 0);
    pAction->m_strength = rInfo.m_strength;
    pAction->m_damping = rInfo.m_damping;
    pAction->m_restLength = rInfo.m_restLength;
    pAction->m_onCompression.m_bool = rInfo.m_onCompression.m_bool;
    pAction->m_onExtension.m_bool = rInfo.m_onExtension.m_bool;
    alignas(16) hkVector4 kPivotB;
    rInfo.m_pivotB.ToHK(kPivotB);
    alignas(16) hkVector4 kPivotA;
    rInfo.m_pivotA.ToHK(kPivotA);
    pAction->setPositionsInBodySpace(kPivotA, kPivotB);
    bhkSerializable* pSerializable = this;
    pSerializable->SethkObject(pAction);
    pAction->removeReference();
    pSerializable->PostInit(arInfo);
}

// Builds the creation info in Havok's heap context when the action has none yet, and reports whether it built it. The
// wrapped object, not the cinfo, guards the copy.
hkSerializableCinfo& bhkSpringAction::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = pInfo == nullptr;
    if (pInfo == nullptr) {
        hkSpringActionCinfo* pCinfo = new hkSpringActionCinfo;
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// Scales the creation info's pivots by the cloning process's scale when it is not 1, then copies the binary action's
// members.
void bhkSpringAction::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkSpringActionCinfo& rInfo = static_cast<hkSpringActionCinfo&>(CreateCinfo(bCreated));
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

// Copies bhkBinaryAction's part, then, when there is a wrapped spring, its rest length, strength and damping, both
// pivots and both flags. Tests this and the wrapped object for null.
void bhkSpringAction::CopyCinfo(hkSpringActionCinfo& arInfo) {
    bhkBinaryAction::CopyCinfo(arInfo);
    if (this) {
        hkpSpringAction* pAction = static_cast<hkpSpringAction*>(phkObject);
        if (pAction) {
            arInfo.m_restLength = pAction->m_restLength;
            arInfo.m_strength = pAction->m_strength;
            arInfo.m_damping = pAction->m_damping;
            arInfo.m_pivotA = pAction->m_positionAinA;
            arInfo.m_pivotB = pAction->m_positionBinB;
            arInfo.m_onCompression.m_bool = pAction->m_onCompression.m_bool;
            arInfo.m_onExtension.m_bool = pAction->m_onExtension.m_bool;
        }
    }
}

NiObject* bhkSpringAction::CreateObject() {
    return new bhkSpringAction;
}

// bhkVector4's constructor zeroes the pivots.
hkSpringActionCinfo::hkSpringActionCinfo()
  : hkBinaryActionCinfo(), m_restLength(1.0f), m_strength(1000.0f), m_damping(0.1f) {
    m_onCompression.m_bool = 1;
    m_onExtension.m_bool = 1;
}
