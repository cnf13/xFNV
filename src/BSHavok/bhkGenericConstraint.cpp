#include "BSHavok/bhkGenericConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkSerializable.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
	int32_t& riMemContext = GetThreadMemContext();
	int32_t iOldContext = riMemContext;
	riMemContext = MEM_CONTEXT_HAVOK;
	bhkSerializable::CloneCritLock();
	bhkGenericConstraint* pClone =
		static_cast<bhkGenericConstraint*>(NiMemObject::operator new(sizeof(bhkGenericConstraint)));
	if (pClone) {
		::new (pClone) bhkGenericConstraint;
	} else {
		pClone = nullptr;
	}
	CopyCloneMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	riMemContext = iOldContext;
	return pClone;
}

// Under the Havok heap context, a cinfo this constraint created goes through its deleting destructor (the first slot of
// its vtable, scalar delete flag).
void bhkGenericConstraint::KillCinfo(bool abCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		if (pInfo) {
			delete static_cast<hkGenericConstraintCinfo*>(pInfo);
		}
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
		hkGenericConstraintCinfo* pCinfo =
			static_cast<hkGenericConstraintCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkGenericConstraintCinfo)));
		if (pCinfo) {
			::new (pCinfo) hkGenericConstraintCinfo;
		}
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
	TLSData* v4 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v4->eMemContext;
	v4->eMemContext = MEM_CONTEXT_HAVOK;
	hkpGenericConstraintData* pnd = static_cast<hkpGenericConstraintData*>(apData);
	if (!apData) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkpGenericConstraintData* v8 =
			static_cast<hkpGenericConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpGenericConstraintData)));
		v8->m_memSizeAndFlags = sizeof(hkpGenericConstraintData);
		pnd = ::new (v8) hkpGenericConstraintData;
	}
	hkConstraintCinfo::CloneData(pnd, arCloneData);
	v4->eMemContext = eMemContext;
}

void hkGenericConstraintCinfo::LoadBinary(NiStream& arStream) {
	hkConstraintCinfo::LoadBinary(arStream);
}

void hkGenericConstraintCinfo::SaveBinary(NiStream& arStream) {
	hkConstraintCinfo::SaveBinary(arStream);
}

uint32_t hkGenericConstraintCinfo::CreateData() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_HAVOK;
	if (!pData) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkpGenericConstraintData* v5 =
			static_cast<hkpGenericConstraintData*>(Instance->m_heap->blockAlloc(sizeof(hkpGenericConstraintData)));
		v5->m_memSizeAndFlags = sizeof(hkpGenericConstraintData);
		pData = ::new (v5) hkpGenericConstraintData;
	}
	v2->eMemContext = eMemContext;
	return sizeof(hkpGenericConstraintData);
}

void hkGenericConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	hkConstraintCinfo::GetViewerStrings(apStrings);
}
