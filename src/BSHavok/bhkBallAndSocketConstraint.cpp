#include "BSHavok/bhkBallAndSocketConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <xmmintrin.h>

NiRTTI bhkBallAndSocketConstraint::ms_RTTI("bhkBallAndSocketConstraint", &bhkConstraint::ms_RTTI);

bhkBallAndSocketConstraint::bhkBallAndSocketConstraint() {
	++iCount;
}

bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(const hkBallAndSocketConstraintCinfo& arInfo)
	: bhkConstraint(arInfo) {
	++iCount;
}

bhkBallAndSocketConstraint::~bhkBallAndSocketConstraint() {
	bhkSerializable::Kill();
	--iCount;
}

// A clone under the engine's clone critical section: built by the default constructor (inlined by the exe), then the
// members copied through the constraint's helper, with the clone even when the allocation failed. The exe inlines the
// heap-context guard and leaves out only its cleanup frame.
NiObject* bhkBallAndSocketConstraint::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkSerializable::CloneCritLock();
	bhkBallAndSocketConstraint* pClone =
		static_cast<bhkBallAndSocketConstraint*>(NiMemObject::operator new(sizeof(bhkBallAndSocketConstraint)));
	if (pClone) {
		::new (pClone) bhkBallAndSocketConstraint;
	}
	bhkConstraint::CopyCloneMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

void bhkBallAndSocketConstraint::LoadBinary(NiStream& arStream) {
	bhkConstraint::LoadBinary(arStream);
}

void bhkBallAndSocketConstraint::KillCinfo(bool abCreated) {
	// The exe inlines the heap-context guard: it saves the thread's context, charges this scope to Havok's and
	// restores the saved one on every path.
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		// The exe calls the cinfo's deleting destructor through the first slot of its vtable, with the scalar
		// delete flag.
		if (pInfo) {
			delete static_cast<hkConstraintCinfo*>(pInfo);
		}
		pInfo = nullptr;
	}
}

uint32_t bhkBallAndSocketConstraint::QCinfoSize() const {
	return (uint32_t)20;
}

hkSerializableCinfo& bhkBallAndSocketConstraint::CreateCinfo(bool& arCreated) {
	// The exe inlines the heap-context guard around the allocation, as KillCinfo does.
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	arCreated = (pInfo == nullptr);
	if (arCreated) {
		hkBallAndSocketConstraintCinfo* pCinfo = static_cast<hkBallAndSocketConstraintCinfo*>(MemoryManager_Singleton
				.Allocate(sizeof(hkBallAndSocketConstraintCinfo)));
		// The cinfo's constructor (0xCB9C30), which the exe inlines here.
		if (pCinfo) {
			::new (pCinfo) hkBallAndSocketConstraintCinfo;
		}
		// The exe stores the cinfo's tag base, four bytes into the object, because its hkSerializableCinfo is the
		// non-polymorphic base; src's tag base is an empty base at the object's start.
		pInfo = pCinfo;
		if (phkObject) {
			// The exe passes the cinfo object itself (unchecked after the allocation), not its tag base.
			bhkConstraint::CopyCinfo(*pCinfo);
		}
	}
	return *pInfo;
}

NiObject* bhkBallAndSocketConstraint::CreateObject() {
	// The exe's cleanup-only EH frame around the allocation is left out.
	void* pMemory = NiMemObject::operator new(sizeof(bhkBallAndSocketConstraint));
	if (!pMemory) {
		return nullptr;
	}
	return ::new (pMemory) bhkBallAndSocketConstraint;
}

// The exe's deleting destructor (0xCC8920, shared with hkStiffSpringConstraintCinfo) adds nothing to the base's.
hkBallAndSocketConstraintCinfo::~hkBallAndSocketConstraintCinfo() {}

// A copy of this cinfo's data into apData (a new one when null) in Havok's heap context: both pivots, scaled by the
// cloning process's scale when it is not 1, then the base's members. arCloneData is a BSCloningProcess (the PDB's
// BSCloningProcess::QScaleU reads its scale). The exe's cleanup-only C++ EH frame is left out.
void hkBallAndSocketConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
	TLSData* v4 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v4->eMemContext;
	v4->eMemContext = MEM_CONTEXT_HAVOK;
	hkpBallAndSocketConstraintData* pnd = static_cast<hkpBallAndSocketConstraintData*>(apData);
	hkpBallAndSocketConstraintData* pd = static_cast<hkpBallAndSocketConstraintData*>(pData);
	if (!apData) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkpBallAndSocketConstraintData* v8 = static_cast<hkpBallAndSocketConstraintData*>(Instance->m_heap
				->blockAlloc(sizeof(hkpBallAndSocketConstraintData)));
		v8->m_memSizeAndFlags = sizeof(hkpBallAndSocketConstraintData);
		pnd = ::new (v8) hkpBallAndSocketConstraintData;
	}
	pnd->m_atoms.m_pivots.m_translationA = pd->m_atoms.m_pivots.m_translationA;
	pnd->m_atoms.m_pivots.m_translationB = pd->m_atoms.m_pivots.m_translationB;
	// EXE-INLINED none: BSCloningProcess::QScaleU
	float fscale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
	if (fscale != 1.0f) {
		hkBallAndSocketConstraintCinfo::ApplyScale(fscale, pnd);
	}
	hkConstraintCinfo::CloneData(pnd, arCloneData);
	v4->eMemContext = eMemContext;
}

// The base's fields, then the two pivots of the constraint data, each one 16-byte component. The exe loads the data
// pointer once, after the base call.
void hkBallAndSocketConstraintCinfo::LoadBinary(NiStream& arStream) {
	hkConstraintCinfo::LoadBinary(arStream);
	hkpBallAndSocketConstraintData* pBallData = static_cast<hkpBallAndSocketConstraintData*>(pData);
	NiStreamLoadBinary(arStream, pBallData->m_atoms.m_pivots.m_translationA);
	NiStreamLoadBinary(arStream, pBallData->m_atoms.m_pivots.m_translationB);
}

// The same two pivots written after the base's fields.
void hkBallAndSocketConstraintCinfo::SaveBinary(NiStream& arStream) {
	hkConstraintCinfo::SaveBinary(arStream);
	hkpBallAndSocketConstraintData* pBallData = static_cast<hkpBallAndSocketConstraintData*>(pData);
	NiStreamSaveBinary(arStream, pBallData->m_atoms.m_pivots.m_translationA);
	NiStreamSaveBinary(arStream, pBallData->m_atoms.m_pivots.m_translationB);
}

// The exe's cleanup-only C++ EH frame is left out.
uint32_t hkBallAndSocketConstraintCinfo::CreateData() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_HAVOK;
	if (!pData) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkpBallAndSocketConstraintData* v5 = static_cast<hkpBallAndSocketConstraintData*>(Instance->m_heap
				->blockAlloc(sizeof(hkpBallAndSocketConstraintData)));
		v5->m_memSizeAndFlags = sizeof(hkpBallAndSocketConstraintData);
		pData = ::new (v5) hkpBallAndSocketConstraintData;
	}
	v2->eMemContext = eMemContext;
	return sizeof(hkpBallAndSocketConstraintData);
}

void hkBallAndSocketConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	hkConstraintCinfo::GetViewerStrings(apStrings);
}

// Scales both pivots of the constraint data by afScale, broadcast to the four lanes (scale times pivot, in that
// operand order), unless the data is null or the scale is exactly 1.
// Not registered: the exe's caller hkBallAndSocketConstraintCinfo::CloneData (0xCC9790) passes edx on to
// hkConstraintCinfo::CloneData after this call (`push edx` at 0xCC9861) without reloading it, relying on this body
// leaving edx intact, which clang does not guarantee.
void hkBallAndSocketConstraintCinfo::ApplyScale(float afScale, hkpConstraintData* apInfo) {
	if (apInfo) {
		if (1.0f != afScale) {
			hkpBallAndSocketConstraintData* pBallData = static_cast<hkpBallAndSocketConstraintData*>(apInfo);
			hkVector4& rTranslationA = pBallData->m_atoms.m_pivots.m_translationA;
			hkVector4& rTranslationB = pBallData->m_atoms.m_pivots.m_translationB;
			rTranslationA.m_quad =
				_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(afScale), _mm_set_ss(afScale), 0), rTranslationA.m_quad);
			rTranslationB.m_quad =
				_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(afScale), _mm_set_ss(afScale), 0), rTranslationB.m_quad);
		}
	}
}
