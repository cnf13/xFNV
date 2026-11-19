#include "BSHavok/bhkAngularDashpotAction.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBinaryAction.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "Fallout Shared/TESFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"

NiRTTI bhkAngularDashpotAction::ms_RTTI("bhkAngularDashpotAction", &bhkBinaryAction::ms_RTTI);

bhkAngularDashpotAction::bhkAngularDashpotAction() {
    ++iCount;
}

bhkAngularDashpotAction::~bhkAngularDashpotAction() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkAngularDashpotAction::GetRTTI() const {
    return &bhkAngularDashpotAction::ms_RTTI;
}

// Built as in CreateObject, then the members copied through a virtual call.
NiObject* bhkAngularDashpotAction::CreateClone(NiCloningProcess& arCloneData) {
    bhkSerializable::CloneCritLock();
    bhkAngularDashpotAction* pClone = new bhkAngularDashpotAction;
    bhkBinaryAction* pAction = this;
    pAction->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's rotation (bhkStream::Endian) and its strength
// and damping (ENDIAN) byte-swapped in place. The stream's byte is compared with 1: any other byte swaps, so it is
// read as a byte rather than a bool tested. A cinfo built here is not killed.
void bhkAngularDashpotAction::LoadBinary(NiStream& arStream) {
    bhkBinaryAction::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkAngularDashpotActionCinfo& cinfo = static_cast<hkAngularDashpotActionCinfo&>(CreateCinfo(bCreated));
        bhkStream::Endian(cinfo.m_rotation);
        ENDIAN(cinfo.m_strength, false);
        ENDIAN(cinfo.m_damping, false);
    }
}

// For a big-endian save, the creation info's rotation, strength and damping are byte-swapped in place around the base's
// write and swapped back after it. The stream's byte is compared with 1 and read again after the base call.
void bhkAngularDashpotAction::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkAngularDashpotActionCinfo& cinfo = static_cast<hkAngularDashpotActionCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_rotation);
        ENDIAN(cinfo.m_strength, false);
        ENDIAN(cinfo.m_damping, false);
    }
    bhkBinaryAction::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_rotation);
        ENDIAN(cinfo.m_strength, false);
        ENDIAN(cinfo.m_damping, false);
    }
}

// The class' own viewer line, after the base's.
void bhkAngularDashpotAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext memContext(MEM_CONTEXT_HAVOK);
    bhkBinaryAction::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
    apStrings->Add(pcString);
}

// The size of the action's creation info, hkAngularDashpotActionCinfo.
uint32_t bhkAngularDashpotAction::QCinfoSize() const {
    return 0x30;
}

// Builds the Havok action in the Havok heap: the block's size in its header, the constructor, the creation info's
// values, then the wrapper's hooks.
void bhkAngularDashpotAction::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext memContext(MEM_CONTEXT_HAVOK);
    const hkAngularDashpotActionCinfo& rInfo = static_cast<const hkAngularDashpotActionCinfo&>(arInfo);
    hkpAngularDashpotAction* pAction = static_cast<hkpAngularDashpotAction*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAngularDashpotAction))
    );
    // The block's m_memSizeAndFlags holds the size it was allocated with.
    pAction->m_memSizeAndFlags = sizeof(hkpAngularDashpotAction);
    ::new (pAction) hkpAngularDashpotAction(rInfo.m_bodyA, rInfo.m_bodyB, 0);
    pAction->m_strength = rInfo.m_strength;
    pAction->m_damping = rInfo.m_damping;
    pAction->m_rotation.m_vec.m_quad = rInfo.m_rotation.m_vec.m_quad;
    bhkSerializable* pSerializable = this;
    pSerializable->SethkObject(pAction);
    pAction->removeReference();
    pSerializable->PostInit(arInfo);
}

// Builds the creation info in Havok's heap context when the action has none yet, and reports whether it built it. The
// wrapped object, not the cinfo, guards the copy.
hkSerializableCinfo& bhkAngularDashpotAction::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkAngularDashpotActionCinfo* pCinfo = nullptr;
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(hkAngularDashpotActionCinfo));
        if (pMemory) {
            pCinfo = ::new (pMemory) hkAngularDashpotActionCinfo;
        }
        pInfo = pCinfo;
        // A failed allocation still reaches the copy, with null.
        if (phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

// The base's fields, then the wrapped action's damping, rotation and strength, each through a getter that tests this
// and the wrapped object for null and falls back to 0 or the identity quaternion.
void bhkAngularDashpotAction::CopyCinfo(hkAngularDashpotActionCinfo& arInfo) {
    bhkBinaryAction::CopyCinfo(arInfo);
    // EXE-INLINED none: bhkAngularDashpotAction::GetDamping
    float fDamping = (this && phkObject) ? static_cast<hkpAngularDashpotAction*>(phkObject)->m_damping : 0.0f;
    arInfo.m_damping = fDamping;
    const hkQuaternion* pRotation = &GethkRotation();
    // EXE-INLINED 0x4B4DB0
    arInfo.m_rotation.m_vec(0) = pRotation->m_vec(0);
    arInfo.m_rotation.m_vec(1) = pRotation->m_vec(1);
    arInfo.m_rotation.m_vec(2) = pRotation->m_vec(2);
    arInfo.m_rotation.m_vec(3) = pRotation->m_vec(3);
    // EXE-INLINED none: bhkAngularDashpotAction::GetStrength
    float fStrength = (this && phkObject) ? static_cast<hkpAngularDashpotAction*>(phkObject)->m_strength : 0.0f;
    arInfo.m_strength = fStrength;
}

NiObject* bhkAngularDashpotAction::CreateObject() {
    return new bhkAngularDashpotAction;
}

// The base's fields cleared by its constructor, the identity quaternion and the default strength and damping.
hkAngularDashpotActionCinfo::hkAngularDashpotActionCinfo()
  : hkBinaryActionCinfo(), m_rotation(hkQuaternionIdentity), m_strength(0.1f), m_damping(0.01f) {}
