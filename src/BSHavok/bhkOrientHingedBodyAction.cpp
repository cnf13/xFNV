#include "BSHavok/bhkOrientHingedBodyAction.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/bhkUnaryAction.h"
#include "BSHavok/hkOrientHingedBodyAction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

NiRTTI bhkOrientHingedBodyAction::ms_RTTI("bhkOrientHingedBodyAction", &bhkUnaryAction::ms_RTTI);

bhkOrientHingedBodyAction::bhkOrientHingedBodyAction() {}

bhkOrientHingedBodyAction::~bhkOrientHingedBodyAction() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkOrientHingedBodyAction::GetRTTI() const {
    return &bhkOrientHingedBodyAction::ms_RTTI;
}

// The clone is allocated in the Havok heap context while the engine's clone critical section is held, and its members
// are copied through CopyMembers.
NiObject* bhkOrientHingedBodyAction::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkOrientHingedBodyAction* pkClone = new bhkOrientHingedBodyAction;
    CopyMembers(pkClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pkClone;
}

// The base's fields, then, for a big-endian source, the creation info's two axes, strength and damping byte-swapped in
// place. The stream's byte is compared with 1: any other byte swaps, so it is read as a byte rather than a bool tested.
// A cinfo built here is not killed.
void bhkOrientHingedBodyAction::LoadBinary(NiStream& arStream) {
    bhkUnaryAction::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkOrientHingedBodyActionCinfo& cinfo = static_cast<hkOrientHingedBodyActionCinfo&>(CreateCinfo(bCreated));
        bhkStream::Endian(cinfo.HingeAxisLS);
        bhkStream::Endian(cinfo.ForwardLS);
        ENDIAN(cinfo.fStrength, false);
        ENDIAN(cinfo.fDamping, false);
    }
}

// For a big-endian save, the creation info's two axes, strength and damping are byte-swapped in place around the base's
// write and swapped back after it. The stream's byte is compared with 1 and read again after the base call.
void bhkOrientHingedBodyAction::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkOrientHingedBodyActionCinfo& cinfo = static_cast<hkOrientHingedBodyActionCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.HingeAxisLS);
        bhkStream::Endian(cinfo.ForwardLS);
        ENDIAN(cinfo.fStrength, false);
        ENDIAN(cinfo.fDamping, false);
    }
    bhkUnaryAction::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.HingeAxisLS);
        bhkStream::Endian(cinfo.ForwardLS);
        ENDIAN(cinfo.fStrength, false);
        ENDIAN(cinfo.fDamping, false);
    }
}

// The base's strings, then this class's RTTI name appended to the array (the array grows when it is full and the name
// is stored at m_usSize). The append's allocation is charged to the Havok heap context.
void bhkOrientHingedBodyAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkUnaryAction::GetViewerStrings(apStrings);
    char* pcName = NiGetViewerString(ms_RTTI.m_pcName);
    uint16_t usSize = apStrings->m_usSize;
    if (usSize >= apStrings->m_usMaxSize) {
        apStrings->SetSize(static_cast<uint32_t>(usSize) + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(usSize, pcName);
}

// Releases a creation info this action made, under the Havok heap context.
void bhkOrientHingedBodyAction::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkOrientHingedBodyActionCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

// The size of the creation info in a file: its serialized form's.
uint32_t bhkOrientHingedBodyAction::QCinfoSize() const {
    return sizeof(hkOrientHingedBodyActionCinfo::Serialized);
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkOrientHingedBodyAction::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkOrientHingedBodyActionCinfo& rCinfo = static_cast<hkOrientHingedBodyActionCinfo&>(CreateCinfo(bCreated));
    hkOrientHingedBodyActionCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

void bhkOrientHingedBodyAction::Init(const hkSerializableCinfo& arInfo) {
    const hkOrientHingedBodyActionCinfo& cinfo = static_cast<const hkOrientHingedBodyActionCinfo&>(arInfo);
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkOrientHingedBodyAction* po = new hkOrientHingedBodyAction(
        cinfo.m_body,
        cinfo.HingeAxisLS,
        cinfo.ForwardLS,
        cinfo.fStrength,
        cinfo.fDamping,
        0
    );
    SethkObject(po);
    po->removeReference();
    PostInit(arInfo);
}

// Builds the action's creation info when there is none: a block allocated under the Havok heap context holding the
// unary action's zeroed user data and entity, two zero axes, and a 0.5 strength with 0.1 damping.
hkSerializableCinfo& bhkOrientHingedBodyAction::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = this->pInfo == nullptr;
    if (arCreated) {
        hkOrientHingedBodyActionCinfo* pCinfo = new hkOrientHingedBodyActionCinfo;
        bool bHasObject = this->phkObject != nullptr;
        this->pInfo = pCinfo;
        if (bHasObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *this->pInfo;
}

void bhkOrientHingedBodyAction::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bhkUnaryAction::CopyMembers(apDest, arCloneData);
}

void bhkOrientHingedBodyAction::CopyCinfo(hkOrientHingedBodyActionCinfo& arInfo) {
    hkOrientHingedBodyAction* phkObject;
    bhkUnaryAction::CopyCinfo(arInfo);
    if (this) {
        phkObject = static_cast<hkOrientHingedBodyAction*>(this->phkObject);
    }
    else {
        phkObject = nullptr;
    }
    arInfo.HingeAxisLS = phkObject->HingeAxisLS;
    arInfo.ForwardLS = phkObject->ForwardLS;
    arInfo.fStrength = phkObject->fStrength;
    arInfo.fDamping = phkObject->fDamping;
}

void bhkOrientHingedBodyAction::SetTargetDir(const hkVector4& arDir) {
    hkOrientHingedBodyAction* pAction = static_cast<hkOrientHingedBodyAction*>(bhkRefObject::operator*());
    pAction->setTargetDir(arDir);
}

// The class loader's factory: a new object, or null when the allocation fails.
NiObject* bhkOrientHingedBodyAction::CreateObject() {
    return new bhkOrientHingedBodyAction;
}

hkOrientHingedBodyActionCinfo::hkOrientHingedBodyActionCinfo()
  : hkUnaryActionCinfo(), HingeAxisLS(hkVector4Zero), ForwardLS(hkVector4Zero), fStrength(0.5f), fDamping(0.1f) {}
