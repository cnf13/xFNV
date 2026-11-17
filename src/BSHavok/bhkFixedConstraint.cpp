#include "BSHavok/bhkFixedConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkGenericConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpConstraintConstructionKit.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

NiRTTI bhkFixedConstraint::ms_RTTI("bhkFixedConstraint", &bhkGenericConstraint::ms_RTTI);

// The two indices are left unwritten, as in the exe.
hkFixedConstraintCinfo::hkFixedConstraintCinfo() {}

// The exe's cleanup-only C++ EH frame is left out.
void hkFixedConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
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
	hkGenericConstraintCinfo::CloneData(pnd, arCloneData);
	v4->eMemContext = eMemContext;
}

void hkFixedConstraintCinfo::LoadBinary(NiStream& arStream) {
	hkGenericConstraintCinfo::LoadBinary(arStream);
}

void hkFixedConstraintCinfo::SaveBinary(NiStream& arStream) {
	hkGenericConstraintCinfo::SaveBinary(arStream);
}

// The base's lines: the exe jumps to 0xCC5080, the base's (folded) copy.
void hkFixedConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	hkGenericConstraintCinfo::GetViewerStrings(apStrings);
}

// Rebuilds the generic data from scratch through the construction kit: each body's pivot and angular basis are the
// reference transform seen from that body; all six degrees of freedom are constrained at strength 0.1.
void hkFixedConstraintCinfo::SetTransformInRef(const hkTransform& arTrans) {
	CreateData();
	hkpConstraintConstructionKit kKit;
	kKit.begin(static_cast<hkpGenericConstraintData*>(pData));
	hkpEntity* pBodyA = pEntityA;
	hkpEntity* pBodyB = pEntityB;
	hkTransform kInA;
	kInA.setMulInverseMul(pBodyA->m_motion.m_motionState.m_transform, arTrans);
	hkTransform kInB;
	kInB.setMulInverseMul(pBodyB->m_motion.m_motionState.m_transform, arTrans);
	kKit.setPivotA(kInA.m_translation);
	kKit.setAngularBasisA(kInA.m_rotation);
	RefPivotIndex = kKit.setPivotB(kInB.m_translation);
	RefAngularBasisIndex = kKit.setAngularBasisB(kInB.m_rotation);
	kKit.constrainAllLinearDof();
	kKit.constrainAllAngularDof();
	kKit.setStrength(0.1f);
	kKit.restoreStrength();
	kKit.end();
}

bhkFixedConstraint::bhkFixedConstraint() {
	++iCount;
}

// The exe's cleanup-only C++ EH frame is left out. The base constructors are inlined: each sets its vtable and counts
// itself. The info goes to Init as its hkSerializableCinfo base, which follows the info's vtable pointer: the exe
// converts the pointer with a null test.
// The default chain counts each level, then the info goes to bhkConstraint's Init.
bhkFixedConstraint::bhkFixedConstraint(const hkFixedConstraintCinfo& arInfo) {
	bhkConstraint::Init(arInfo);
	++iCount;
}

bhkFixedConstraint::~bhkFixedConstraint() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkFixedConstraint::GetRTTI() const {
	return &bhkFixedConstraint::ms_RTTI;
}

// Clones under the Havok heap context and the clone lock: a new constraint built by the default constructor (inlined by
// the exe), then the constraint's members copied. The exe's cleanup-only EH frame (0xf340f7) is left out.
NiObject* bhkFixedConstraint::CreateClone(NiCloningProcess& arCloneData) {
	int32_t& riMemContext = GetThreadMemContext();
	int32_t iOldMemContext = riMemContext;
	riMemContext = MEM_CONTEXT_HAVOK;
	bhkSerializable::CloneCritLock();
	bhkFixedConstraint* pClone =
		static_cast<bhkFixedConstraint*>(NiMemObject::operator new(sizeof(bhkFixedConstraint)));
	if (pClone) {
		::new (pClone) bhkFixedConstraint;
	} else {
		pClone = nullptr;
	}
	CopyCloneMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	riMemContext = iOldMemContext;
	return pClone;
}

uint32_t bhkFixedConstraint::QCinfoSize() const {
	return 28;
}

hkSerializableCinfo& bhkFixedConstraint::CreateCinfo(bool& arCreated) {
	// The exe inlines the heap-context guard around the allocation, as KillCinfo does. The exe's cleanup-only C++ EH
	// frame is left out.
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	arCreated = (pInfo == nullptr);
	if (arCreated) {
		hkFixedConstraintCinfo* pCinfo =
			static_cast<hkFixedConstraintCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkFixedConstraintCinfo)));
		// The cinfo's constructor, which the exe inlines here; it leaves the two indices unwritten.
		if (pCinfo) {
			::new (pCinfo) hkFixedConstraintCinfo;
		}
		// The exe stores the cinfo's tag base, four bytes into the object, because its hkSerializableCinfo is the
		// non-polymorphic base; src's tag base is an empty base at the object's start.
		pInfo = pCinfo;
		if (phkObject) {
			// The exe passes the cinfo object itself (null when the allocation failed), not its tag base.
			bhkConstraint::CopyCinfo(*pCinfo);
		}
	}
	return *pInfo;
}

// The exe's EH frame (scope table 0xf33daf) is cleanup-only and left out.
NiObject* bhkFixedConstraint::CreateObject() {
	bhkFixedConstraint* pkObject =
		static_cast<bhkFixedConstraint*>(NiMemObject::operator new(sizeof(bhkFixedConstraint)));
	if (!pkObject)
		return nullptr;
	return ::new (pkObject) bhkFixedConstraint;
}

// The exe's EH frame (scope table 0xf33fc6) is cleanup-only and left out. Its creation infos are stack objects whose
// constructors and destructors it inlines: the fixed one lives until the new constraint is built, the malleable one
// only in its branch.
bhkConstraint* bhkFixedConstraint::FixConstraint(bhkConstraint* apConstraint, float afStrength) {
	int32_t& riMemContext = GetThreadMemContext();
	int32_t iOldMemContext = riMemContext;
	riMemContext = MEM_CONTEXT_HAVOK;
	bhkConstraint* pConstraint;
	{
		FNVD_CONSTRUCTED_LOCAL(hkFixedConstraintCinfo, kFixedInfo, &hkFixedConstraintCinfo::Destruct);
		hkpEntity* pEntityB = apConstraint->GethkEntityB();
		bhkWorldObject* pObjectB = pEntityB ? pEntityB->m_userData : nullptr;
		hkpEntity* pEntityA = apConstraint->GethkEntityA();
		if (pEntityA && pEntityA->m_userData) {
			kFixedInfo.pEntityA = static_cast<hkpEntity*>(pEntityA->m_userData->phkObject);
		} else {
			kFixedInfo.pEntityA = nullptr;
		}
		if (pObjectB) {
			kFixedInfo.pEntityB = static_cast<hkpEntity*>(pObjectB->phkObject);
		} else {
			kFixedInfo.pEntityB = nullptr;
		}
		pEntityA = apConstraint->GethkEntityA();
		if (pEntityA) {
			kFixedInfo.SetTransformInRef(pEntityA->m_motion.m_motionState.m_transform);
		} else {
			kFixedInfo.SetTransformInRef(hkTransformIdentity);
		}
		// fld1; fcomp afStrength; test ah, 0x41: the malleable path only when 1 > afStrength (a NaN takes the fixed
		// one).
		if (afStrength < 1.0f) {
			// The exe's inlined constructor leaves the data pointer unwritten before CreateData tests it; the class's
			// constructor (0xCA9A80) stores null there.
			FNVD_CONSTRUCTED_LOCAL(hkMalleableConstraintCinfo, kMalleableInfo, &hkMalleableConstraintCinfo::Destruct);
			kMalleableInfo.hkMalleableConstraintCinfo::CreateData();
			kMalleableInfo.CA9E50(kFixedInfo.pData);
			kMalleableInfo.pEntityA = kFixedInfo.pEntityA;
			kMalleableInfo.pEntityB = kFixedInfo.pEntityB;
			kMalleableInfo.SetStrength(afStrength);
			void* pMemory = NiMemObject::operator new(sizeof(bhkMalleableConstraint));
			pConstraint = pMemory ? ::new (pMemory) bhkMalleableConstraint(kMalleableInfo) : nullptr;
		} else {
			void* pMemory = NiMemObject::operator new(sizeof(bhkFixedConstraint));
			pConstraint = pMemory ? ::new (pMemory) bhkFixedConstraint(kFixedInfo) : nullptr;
		}
	}
	riMemContext = iOldMemContext;
	return pConstraint;
}

const NiRTTI* bhkGenericConstraint::GetRTTI() const {
	return &bhkGenericConstraint::ms_RTTI;
}

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkGenericConstraintCinfo::hkGenericConstraintCinfo() {}
