#include "BSHavok/bhkSerializable.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBallSocketConstraintChain.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConstraintChain.h"
#include "BSHavok/bhkFixedConstraint.h"
#include "BSHavok/bhkMotorAction.h"
#include "BSHavok/bhkOrientHingedBodyAction.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkUnaryAction.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

NiRTTI bhkSerializable::ms_RTTI("bhkSerializable", &bhkRefObject::ms_RTTI);
BSCriticalSection bhkSerializable::CloneCrit;

bhkSerializable::bhkSerializable() : pInfo(nullptr) {}

// Releases the creation info under the Havok heap context.
bhkSerializable::~bhkSerializable() {
    int32_t& iMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;
    MemoryManager_Singleton.Deallocate(pInfo);
    pInfo = nullptr;
    iMemContext = iSavedMemContext;
}

// Loads the base body, then the class's own creation info; the base is called directly and the info loader virtually.
void bhkSerializable::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    LoadCInfo(arStream);
}

void bhkSerializable::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
    if (pInfo) {
        Init(*pInfo);
    }
}

bool bhkSerializable::RegisterStreamables(NiStream& arStream) {
    return NiObject::RegisterStreamables(arStream);
}

// Makes the creation info from the wrapped Havok object and writes it as one 4-byte component, adding the bytes the
// stream reports to its absolute position. KillCinfo reads only the low byte of the bool CreateCinfo fills.
void bhkSerializable::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkSerializableCinfo* pCinfo = &CreateCinfo(bCreated);
    NiObject::SaveBinary(arStream);
    uint32_t uiSize = QCinfoSize();
    NiBinaryStream* pkOstr = arStream.m_pkOstr;
    uint32_t uiComponentSize = 1;
    pkOstr->BinaryWrite(pCinfo, uiSize, &uiComponentSize, 1);
    KillCinfo(bCreated);
}

bool bhkSerializable::IsEqual(NiObject* apObject) {
    return NiObject::IsEqual(apObject);
}

// The base lines, then the class's heading line, appended to the array (16-bit sizes, unsigned compare), all under the
// Havok heap context.
void bhkSerializable::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    int32_t& iMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;
    bhkRefObject::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    iMemContext = iSavedMemContext;
}

void bhkSerializable::SethkObject(hkReferencedObject* aphkObject) {
    hkpRigidBody* phkObject;
    ahkpWorld* v4;
    if (this) {
        phkObject = static_cast<hkpRigidBody*>(this->phkObject);
    }
    else {
        phkObject = nullptr;
    }
    if (static_cast<const void*>(aphkObject) != static_cast<const void*>(phkObject)) {
        if (phkObject) {
            v4 = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
            if ((v4) && (v4->m_userData)) {
                Remove();
            }
        }
        bhkRefObject::SethkObject(aphkObject);
    }
}

void bhkSerializable::KillCinfo(bool abCreated) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    if (abCreated) {
        MemoryManager_Singleton.Deallocate(pInfo);
        pInfo = nullptr;
    }
    v3->eMemContext = eMemContext;
}

int32_t bhkSerializable::LoadCInfo(NiStream& arStream) {
    bool v7;
    hkSerializableCinfo* v3 = &CreateCinfo(v7);
    int32_t isize = QCinfoSize();
    NiBinaryStream* m_pkIstr = (&arStream)->Istr();
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pkIstr->m_pfnRead(m_pkIstr, v3, isize, nullptr, 0);
    return isize;
}

void bhkSerializable::Kill() {
    if (phkObject) {
        SethkObject(nullptr);
    }
    KillCinfo(true);
}

void bhkSerializable::PostInit(const hkSerializableCinfo&) {
    KillCinfo(true);
}

void bhkSerializable::CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    NiObject::CopyMembers(pDest, arCloneData);
    hkSerializableCinfo* cinfo = &CreateCinfo(bCreated);
    pDest->Init(*cinfo);
    KillCinfo(true);
}

void bhkSerializable::CloneCritLock() {
    EnterCriticalSection(&bhkSerializable::CloneCrit.CriticalSection);
}

void bhkSerializable::CloneCritUnlock() {
    LeaveCriticalSection(&bhkSerializable::CloneCrit.CriticalSection);
}
