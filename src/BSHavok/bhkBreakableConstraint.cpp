#include "BSHavok/bhkBreakableConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConstraintManager.h"
#include "BSHavok/bhkSerializable.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

NiRTTI bhkBreakableConstraint::ms_RTTI("bhkBreakableConstraint", &bhkConstraint::ms_RTTI);

bhkBreakableConstraint::bhkBreakableConstraint() {
    ++iCount;
}

bhkBreakableConstraint::~bhkBreakableConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkBreakableConstraint::GetRTTI() const {
    return &bhkBreakableConstraint::ms_RTTI;
}

// The clone is made under the Havok heap context and the clone lock, then the creation info and entities are copied
// over.
NiObject* bhkBreakableConstraint::CreateClone(NiCloningProcess& arCloneData) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkBreakableConstraint* pClone =
        static_cast<bhkBreakableConstraint*>(NiMemObject::operator new(sizeof(bhkBreakableConstraint)));
    if (pClone) {
        ::new (pClone) bhkBreakableConstraint;
    }
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    GetThreadMemContext() = iOldMemContext;
    return pClone;
}

void bhkBreakableConstraint::LoadBinary(NiStream& arStream) {
    bhkConstraint::LoadBinary(arStream);
}

void bhkBreakableConstraint::LinkObject(NiStream& arStream) {
    bool v3;
    CreateCinfo(v3);
    bhkConstraint::LinkObject(arStream);
    arStream.GetNumberOfLinkIDs();
    arStream.GetObjectFromLinkID();
    arStream.GetObjectFromLinkID();
}

// Deletes the creation info CreateCinfo made, under the Havok heap context.
void bhkBreakableConstraint::KillCinfo(bool abCreated) {
    // The heap-context guard saves the thread's context, charges this scope to Havok's and restores the saved one on
    // every path.
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            delete static_cast<hkConstraintCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

uint32_t bhkBreakableConstraint::QCinfoSize() const {
    return sizeof(hkBreakableConstraintCinfo);
}

// Makes the creation info from the wrapped constraint, or returns the one already made, under the Havok heap context.
hkSerializableCinfo& bhkBreakableConstraint::CreateCinfo(bool& arCreated) {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkBreakableConstraintCinfo* pCinfo = static_cast<hkBreakableConstraintCinfo*>(
            MemoryManager_Singleton.Allocate(sizeof(hkBreakableConstraintCinfo))
        );
        if (pCinfo) {
            ::new (pCinfo) hkBreakableConstraintCinfo;
        }
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself (null when the allocation failed), not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    hkSerializableCinfo* pResult = pInfo;
    pTLS->eMemContext = eMemContext;
    return *pResult;
}

NiObject* bhkBreakableConstraint::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(bhkBreakableConstraint));
    if (pMemory == nullptr) {
        return nullptr;
    }
    return ::new (pMemory) bhkBreakableConstraint;
}

// hkConstraintCinfo's constructor clears the data and entities and sets the PSI priority; this class adds nothing.
hkBreakableConstraintCinfo::hkBreakableConstraintCinfo() : hkConstraintCinfo() {}

// The creation info adds nothing to hkConstraintCinfo's destruction.
hkBreakableConstraintCinfo::~hkBreakableConstraintCinfo() {}

// Clones this info's breakable data into apData (a new breakable data when apData is null): the wrapped constraint data
// through the constraint manager, then the limit and the remove-when-broken flag, then the base's part, under the Havok
// heap context.
void hkBreakableConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    hkpBreakableConstraintData* pnd = static_cast<hkpBreakableConstraintData*>(apData);
    hkpBreakableConstraintData* pd = static_cast<hkpBreakableConstraintData*>(pData);
    if (!apData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        pnd =
            static_cast<hkpBreakableConstraintData*>(Instance->m_heap->blockAlloc(sizeof(ahkpBreakableConstraintData)));
        pnd->m_memSizeAndFlags = sizeof(ahkpBreakableConstraintData);
        pnd = ::new (pnd) ahkpBreakableConstraintData;
    }
    hkpConstraintData* pcd = pd->m_constraintData;
    if (pcd) {
        hkpConstraintData* pclone = bhkConstraintManager::Clone(pcd, arCloneData);
        if (pclone) {
            pnd->SetWrappedConstraintData(pclone);
            pclone->removeReference();
        }
    }
    pnd->m_solverResultLimit = pd->m_solverResultLimit;
    pnd->m_removeWhenBroken.m_bool = pd->m_removeWhenBroken.m_bool;
    hkConstraintCinfo::CloneData(pnd, arCloneData);
    pTLS->eMemContext = eMemContext;
}

void hkBreakableConstraintCinfo::LoadBinary(NiStream& arStream) {
    uint32_t v10;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::LoadBinary(arStream);
    hkpBreakableConstraintData* pd = static_cast<hkpBreakableConstraintData*>(pData);
    hkpConstraintData* pcd = bhkConstraintManager::LoadBinary(*v2);
    pd->SetWrappedConstraintData(pcd);
    pcd->removeReference();
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v10 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pd->m_solverResultLimit, 4, &v10, 1);
    NiBinaryStream* v8 = v2->Istr();
    uint32_t (*v9)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v8->m_pfnRead;
    v10 = 1;
    v8->m_uiAbsoluteCurrentPos += v9(v8, &pd->m_removeWhenBroken, 1, &v10, 1);
}

void hkBreakableConstraintCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v9;
    NiStream* v2 = &arStream;
    hkConstraintCinfo::SaveBinary(arStream);
    hkpBreakableConstraintData* pd = static_cast<hkpBreakableConstraintData*>(pData);
    bhkConstraintManager::SaveBinary(*v2, pd->m_constraintData);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v9 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_solverResultLimit, 4, &v9, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v9 = 1;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_removeWhenBroken, 1, &v9, 1);
}

uint32_t hkBreakableConstraintCinfo::CreateData() {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    if (!pData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpBreakableConstraintData* v5 =
            static_cast<hkpBreakableConstraintData*>(Instance->m_heap->blockAlloc(sizeof(ahkpBreakableConstraintData)));
        v5->m_memSizeAndFlags = sizeof(ahkpBreakableConstraintData);
        v5 = ::new (v5) ahkpBreakableConstraintData;
        pData = v5;
    }
    v2->eMemContext = eMemContext;
    return sizeof(ahkpBreakableConstraintData);
}

// Lists the base's strings, the threshold and the remove-when-broken flag, then the wrapped constraint's strings
// through a temporary creation info of its type, under the Havok heap context.
void hkBreakableConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    hkConstraintCinfo::GetViewerStrings(apStrings);
    hkpBreakableConstraintData* pd = static_cast<hkpBreakableConstraintData*>(pData);
    float fThreshold = pd ? pd->m_solverResultLimit : 0.0f;
    char* pcThreshold = NiGetViewerString("Threshold", fThreshold);
    apStrings->Add(pcThreshold);
    pd = static_cast<hkpBreakableConstraintData*>(pData);
    bool bRemove = pd ? pd->m_removeWhenBroken.m_bool != 0 : false;
    char* pcRemove = NiGetViewerString("RemIfBroke", bRemove);
    apStrings->Add(pcRemove);
    pd = static_cast<hkpBreakableConstraintData*>(pData);
    hkpConstraintData* pWrapped = pd ? pd->m_constraintData : nullptr;
    if (pWrapped) {
        hkConstraintCinfo* pi = bhkConstraintManager::GetInfo(pWrapped->getType());
        if (pi) {
            pi->SetData(pWrapped);
            pi->GetViewerStrings(apStrings);
            delete pi;
        }
    }
    pTLS->eMemContext = eMemContext;
}

ahkpBreakableConstraintData::ahkpBreakableConstraintData() : hkpBreakableConstraintData(nullptr) {
    m_solverResultLimit = 10.0f;
    m_removeWhenBroken.m_bool = 0;
    m_revertBackVelocityOnBreak.m_bool = 0;
}
