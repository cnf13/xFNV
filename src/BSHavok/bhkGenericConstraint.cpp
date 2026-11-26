#include "BSHavok/bhkGenericConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkSerializable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

NiRTTI bhkGenericConstraint::ms_RTTI("bhkGenericConstraint", &bhkConstraint::ms_RTTI);

bhkGenericConstraint::bhkGenericConstraint() {
    ++iCount;
}

bhkGenericConstraint::~bhkGenericConstraint() {
    bhkSerializable::Kill();
    --iCount;
}

// Clones the constraint under the Havok heap context and the shared clone lock: a new generic constraint, filled by
// bhkConstraint's clone helper.
NiObject* bhkGenericConstraint::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkGenericConstraint* pClone = new bhkGenericConstraint;
    CopyCloneMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Under the Havok heap context, a cinfo this constraint created goes through its deleting destructor (the first slot of
// its vtable, scalar delete flag).
void bhkGenericConstraint::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkGenericConstraintCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

uint32_t bhkGenericConstraint::QCinfoSize() const {
    return 20;
}

hkSerializableCinfo& bhkGenericConstraint::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkGenericConstraintCinfo* pCinfo = new hkGenericConstraintCinfo;
        pInfo = pCinfo;
        if (phkObject) {
            // Passes the cinfo object itself, not its tag base.
            bhkConstraint::CopyCinfo(*pCinfo);
        }
    }
    return *pInfo;
}

hkGenericConstraintCinfo::~hkGenericConstraintCinfo() {}

void hkGenericConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpGenericConstraintData* pnd = static_cast<hkpGenericConstraintData*>(apData);
    if (!apData) {
        pnd = new hkpGenericConstraintData;
    }
    hkConstraintCinfo::CloneData(pnd, arCloneData);
}

void hkGenericConstraintCinfo::LoadBinary(NiStream& arStream) {
    hkConstraintCinfo::LoadBinary(arStream);
}

void hkGenericConstraintCinfo::SaveBinary(NiStream& arStream) {
    hkConstraintCinfo::SaveBinary(arStream);
}

uint32_t hkGenericConstraintCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!pData) {
        pData = new hkpGenericConstraintData;
    }
    return sizeof(hkpGenericConstraintData);
}

void hkGenericConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
}
