#include "BSHavok/bhkMalleableConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBallAndSocketConstraint.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConstraintManager.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"

NiRTTI bhkMalleableConstraint::ms_RTTI("bhkMalleableConstraint", &bhkConstraint::ms_RTTI);

ahkpMalleableConstraintData::ahkpMalleableConstraintData() {}

void ahkpMalleableConstraintData::SetChild(hkpConstraintData* a1) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	if (a1) {
		a1->addReference();
	}
	hkpConstraintData* m_constraintData = this->m_constraintData;
	if (m_constraintData) {
		m_constraintData->removeReference();
	}
	this->m_constraintData = a1;
	v3->eMemContext = eMemContext;
}

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkMalleableConstraintCinfo::hkMalleableConstraintCinfo() {}

void hkMalleableConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	ahkpMalleableConstraintData* pnd = static_cast<ahkpMalleableConstraintData*>(apData);
	ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
	if (!apData) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		pnd = static_cast<ahkpMalleableConstraintData*>(Instance->m_heap
				->blockAlloc(sizeof(ahkpMalleableConstraintData)));
		pnd->m_memSizeAndFlags = sizeof(ahkpMalleableConstraintData);
		::new (pnd) ahkpMalleableConstraintData;
	}
	hkpConstraintData* pcd = pd->m_constraintData;
	if (pcd) {
		hkpConstraintData* pclone = bhkConstraintManager::Clone(pcd, arCloneData);
		hkReferencedObject* pclone_2 = pclone;
		if (pclone) {
			pnd->SetChild(pclone);
			pclone_2->removeReference();
		}
	}
	pnd->m_strength = pd->m_strength;
	pnd->m_atoms.m_bridgeAtom.init(pnd);
	hkConstraintCinfo::CloneData(pnd, arCloneData);
	v3->eMemContext = eMemContext;
}

void hkMalleableConstraintCinfo::LoadBinary(NiStream& arStream) {
	uint32_t v8;
	NiStream* v2 = &arStream;
	hkConstraintCinfo::LoadBinary(arStream);
	ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
	hkpConstraintData* pcd = bhkConstraintManager::LoadBinary(*v2);
	pd->SetChild(pcd);
	pd->m_atoms.m_bridgeAtom.init(pd);
	pcd->removeReference();
	NiBinaryStream* m_pkIstr = v2->Istr();
	uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
	v8 = 4;
	m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pd->m_strength, 4, &v8, 1);
}

void hkMalleableConstraintCinfo::SaveBinary(NiStream& arStream) {
	uint32_t v7;
	NiStream* v2 = &arStream;
	hkConstraintCinfo::SaveBinary(arStream);
	ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
	bhkConstraintManager::SaveBinary(*v2, pd->m_constraintData);
	NiBinaryStream* m_pkOstr = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
	v7 = 4;
	m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_strength, 4, &v7, 1);
}

uint32_t hkMalleableConstraintCinfo::CreateData() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_HAVOK;
	if (!pData) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		ahkpMalleableConstraintData* pkData = static_cast<ahkpMalleableConstraintData*>(Instance->m_heap
				->blockAlloc(sizeof(ahkpMalleableConstraintData)));
		pkData->m_memSizeAndFlags = sizeof(ahkpMalleableConstraintData);
		pData = ::new (pkData) ahkpMalleableConstraintData;
	}
	v2->eMemContext = eMemContext;
	return sizeof(ahkpMalleableConstraintData);
}

void hkMalleableConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	hkConstraintCinfo::GetViewerStrings(apStrings);
	// EXE-INLINED none: hkMalleableConstraintCinfo::GetStrength
	ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
	float fStrength = pd ? pd->m_strength : 0.0f;
	char* pcString = NiGetViewerString("Strength", fStrength);
	apStrings->Add(pcString);
	// EXE-INLINED none: hkMalleableConstraintCinfo::GetWrappedConstraintData
	pd = static_cast<ahkpMalleableConstraintData*>(pData);
	// EXE-INLINED none: ahkpMalleableConstraintData::GetChild
	hkpConstraintData* pcd = pd ? pd->m_constraintData : nullptr;
	if (pcd) {
		hkConstraintCinfo* pi = bhkConstraintManager::GetInfo(pcd->getType());
		if (pi) {
			pi->SetData(pcd);
			pi->GetViewerStrings(apStrings);
			delete pi;
		}
	}
	pTLSData->eMemContext = eMemContext;
}

void hkMalleableConstraintCinfo::CA9E50(hkpConstraintData* a2) {
	ahkpMalleableConstraintData* pData;
	pData = static_cast<ahkpMalleableConstraintData*>(this->pData);
	if (pData) {
		pData->SetChild(a2);
	}
}

void hkMalleableConstraintCinfo::SetStrength(float afStrength) {
	ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(pData);
	if (pd) {
		pd->m_strength = afStrength;
	}
}

// Scales the wrapped constraint's data by afScale; apInfo is the malleable data wrapping it. The wrapped data is
// reloaded after its getType call.
void hkMalleableConstraintCinfo::ApplyScale(float afScale, hkpConstraintData* apInfo) {
	ahkpMalleableConstraintData* pd = static_cast<ahkpMalleableConstraintData*>(apInfo);
	// EXE-INLINED none: ahkpMalleableConstraintData::GetChild
	if (pd && pd->m_constraintData) {
		if (1.0f != afScale) {
			switch (pd->m_constraintData->getType()) {
			case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
				hkBallAndSocketConstraintCinfo::ApplyScale(afScale, pd->m_constraintData);
				break;
			case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
			case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
			case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
				hkHingeConstraintCinfo::ApplyScale(afScale, pd->m_constraintData);
				break;
			default:
				break;
			}
		}
	}
}

bhkMalleableConstraint::bhkMalleableConstraint() {
	++iCount;
}

bhkMalleableConstraint::bhkMalleableConstraint(const hkMalleableConstraintCinfo& arInfo) {
	bhkConstraint::Init(arInfo);
	++iCount;
}

bhkMalleableConstraint::~bhkMalleableConstraint() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkMalleableConstraint::GetRTTI() const {
	return &bhkMalleableConstraint::ms_RTTI;
}

// A new constraint built as CreateObject builds one, filled from this one under the clone lock with the Havok heap
// context selected.
NiObject* bhkMalleableConstraint::CreateClone(NiCloningProcess& arCloneData) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	bhkSerializable::CloneCritLock();
	bhkMalleableConstraint* pClone =
		static_cast<bhkMalleableConstraint*>(NiMemObject::operator new(sizeof(bhkMalleableConstraint)));
	if (pClone) {
		::new (pClone) bhkMalleableConstraint;
	}
	CopyCloneMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	GetThreadMemContext() = iOldMemContext;
	return pClone;
}

void bhkMalleableConstraint::LoadBinary(NiStream& arStream) {
	bhkConstraint::LoadBinary(arStream);
}

// The cinfo through its slot (the result unused), the base's links, then the link IDs the stream holds for this
// object: their count and two objects, read and dropped.
void bhkMalleableConstraint::LinkObject(NiStream& arStream) {
	bool bCreated;
	CreateCinfo(bCreated);
	bhkConstraint::LinkObject(arStream);
	arStream.GetNumberOfLinkIDs();
	arStream.GetObjectFromLinkID();
	arStream.GetObjectFromLinkID();
}

// Under the Havok heap context, a cinfo this constraint created goes through its deleting destructor (the first slot of
// its vtable, scalar delete flag).
void bhkMalleableConstraint::KillCinfo(bool abCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		if (pInfo) {
			delete static_cast<hkMalleableConstraintCinfo*>(pInfo);
		}
		pInfo = nullptr;
	}
}

uint32_t bhkMalleableConstraint::QCinfoSize() const {
	return 20;
}

hkSerializableCinfo& bhkMalleableConstraint::CreateCinfo(bool& arCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	arCreated = (pInfo == nullptr);
	if (arCreated) {
		hkMalleableConstraintCinfo* pCinfo = static_cast<hkMalleableConstraintCinfo*>(MemoryManager_Singleton
				.Allocate(sizeof(hkMalleableConstraintCinfo)));
		if (pCinfo) {
			::new (pCinfo) hkMalleableConstraintCinfo;
		}
		pInfo = pCinfo;
		if (phkObject) {
			// Converts the stored tag base back to the cinfo object, null-checked.
			bhkConstraint::CopyCinfo(*pCinfo);
		}
	}
	return *pInfo;
}

NiObject* bhkMalleableConstraint::CreateObject() {
	bhkMalleableConstraint* pThis =
		static_cast<bhkMalleableConstraint*>(NiMemObject::operator new(sizeof(bhkMalleableConstraint)));
	if (!pThis)
		return nullptr;
	return ::new (pThis) bhkMalleableConstraint;
}
