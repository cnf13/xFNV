#include "BSHavok/bhkAngularDashpotAction.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBinaryAction.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
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

// Built as in CreateObject, then the members copied through the vtable (slot 52, +0xD0).
NiObject* bhkAngularDashpotAction::CreateClone(NiCloningProcess& arCloneData) {
	bhkSerializable::CloneCritLock();
	bhkAngularDashpotAction* pClone =
		static_cast<bhkAngularDashpotAction*>(NiMemObject::operator new(sizeof(bhkAngularDashpotAction)));
	if (pClone) {
		::new (pClone) bhkAngularDashpotAction;
	}
	// The exe calls CopyMembers indirectly; a base pointer keeps the call virtual.
	bhkBinaryAction* pAction = this;
	pAction->CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's rotation (bhkStream::Endian) and its strength
// and damping (BSEndian) byte-swapped in place. The stream's byte is compared with 1 (`cmp byte ptr [edi+0x1E5], 1`):
// any other byte swaps, so it is read as a byte rather than a bool tested. A cinfo built here is not killed. The exe
// swaps each float's four bytes as an untyped dword (mov, bswap, mov), hence the uint32_t views.
void bhkAngularDashpotAction::LoadBinary(NiStream& arStream) {
	bhkBinaryAction::LoadBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
		bool bCreated;
		hkAngularDashpotActionCinfo& cinfo = static_cast<hkAngularDashpotActionCinfo&>(CreateCinfo(bCreated));
		// EXE-INLINED none: bhkStream::Endian
		uint32_t* puiRotation = reinterpret_cast<uint32_t*>(&cinfo.m_rotation);
		puiRotation[0] = __builtin_bswap32(puiRotation[0]);
		puiRotation[1] = __builtin_bswap32(puiRotation[1]);
		puiRotation[2] = __builtin_bswap32(puiRotation[2]);
		puiRotation[3] = __builtin_bswap32(puiRotation[3]);
		// EXE-INLINED none: BSEndian
		uint32_t* puiStrength = reinterpret_cast<uint32_t*>(&cinfo.m_strength);
		*puiStrength = __builtin_bswap32(*puiStrength);
		// EXE-INLINED none: BSEndian
		uint32_t* puiDamping = reinterpret_cast<uint32_t*>(&cinfo.m_damping);
		*puiDamping = __builtin_bswap32(*puiDamping);
	}
}

// For a big-endian save, the creation info's rotation, strength and damping are byte-swapped in place around the
// base's write and swapped back after it. The stream's byte is compared with 1 and read again after the base call.
// The exe swaps each float's four bytes as an untyped dword (mov, bswap, mov), hence the uint32_t views.
void bhkAngularDashpotAction::SaveBinary(NiStream& arStream) {
	bool bCreated;
	hkAngularDashpotActionCinfo& cinfo = static_cast<hkAngularDashpotActionCinfo&>(CreateCinfo(bCreated));
	uint32_t* puiRotation = reinterpret_cast<uint32_t*>(&cinfo.m_rotation);
	uint32_t* puiStrength = reinterpret_cast<uint32_t*>(&cinfo.m_strength);
	uint32_t* puiDamping = reinterpret_cast<uint32_t*>(&cinfo.m_damping);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		// EXE-INLINED none: bhkStream::Endian
		puiRotation[0] = __builtin_bswap32(puiRotation[0]);
		puiRotation[1] = __builtin_bswap32(puiRotation[1]);
		puiRotation[2] = __builtin_bswap32(puiRotation[2]);
		puiRotation[3] = __builtin_bswap32(puiRotation[3]);
		// EXE-INLINED none: BSEndian
		*puiStrength = __builtin_bswap32(*puiStrength);
		// EXE-INLINED none: BSEndian
		*puiDamping = __builtin_bswap32(*puiDamping);
	}
	bhkBinaryAction::SaveBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		// EXE-INLINED none: bhkStream::Endian
		puiRotation[0] = __builtin_bswap32(puiRotation[0]);
		puiRotation[1] = __builtin_bswap32(puiRotation[1]);
		puiRotation[2] = __builtin_bswap32(puiRotation[2]);
		puiRotation[3] = __builtin_bswap32(puiRotation[3]);
		// EXE-INLINED none: BSEndian
		*puiStrength = __builtin_bswap32(*puiStrength);
		// EXE-INLINED none: BSEndian
		*puiDamping = __builtin_bswap32(*puiDamping);
	}
}

// The class' own viewer line, after the base's; the exe calls bhkBinaryAction's body directly.
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

// Builds the Havok action in the Havok heap: the block's size in its header, the SDK's constructor, the creation
// info's values, then the wrapper's hooks. The exe realigns the stack (and esp, 0FFFFFFF0h) without a 16-byte local.
void bhkAngularDashpotAction::Init(const hkSerializableCinfo& arInfo) {
	AutoMemContext memContext(MEM_CONTEXT_HAVOK);
	const hkAngularDashpotActionCinfo& rInfo = static_cast<const hkAngularDashpotActionCinfo&>(arInfo);
	hkpAngularDashpotAction* pAction = static_cast<hkpAngularDashpotAction*>(hkMemoryRouter::getInstance()
			.heap()
			.blockAlloc(sizeof(hkpAngularDashpotAction)));
	// The block's m_memSizeAndFlags holds the size it was allocated with (a 2-byte store in the exe).
	pAction->m_memSizeAndFlags = sizeof(hkpAngularDashpotAction);
	::new (pAction) hkpAngularDashpotAction(rInfo.m_bodyA, rInfo.m_bodyB, 0);
	pAction->m_strength = rInfo.m_strength;
	pAction->m_damping = rInfo.m_damping;
	// The exe copies the 16 bytes with movss/unpcklps/movaps.
	pAction->m_rotation.m_vec.m_quad = rInfo.m_rotation.m_vec.m_quad;
	bhkSerializable* pSerializable = this;
	pSerializable->SethkObject(pAction);
	pAction->removeReference();
	pSerializable->PostInit(arInfo);
}

// Builds the creation info in Havok's heap context when the action has none yet, and reports whether it built it. The
// cinfo's constructor is called out of line; the wrapped object, not the cinfo, guards the copy. The exe's
// cleanup-only unwind frame is left out.
hkSerializableCinfo& bhkAngularDashpotAction::CreateCinfo(bool& arCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	arCreated = (pInfo == nullptr);
	if (arCreated) {
		hkAngularDashpotActionCinfo* pCinfo = nullptr;
		void* pMemory = MemoryManager_Singleton.Allocate(sizeof(hkAngularDashpotActionCinfo));
		if (pMemory)
			pCinfo = ::new (pMemory) hkAngularDashpotActionCinfo;
		pInfo = pCinfo;
		// A failed allocation still reaches the copy, with null.
		if (phkObject)
			CopyCinfo(*pCinfo);
	}
	return *pInfo;
}

// The base's fields, then the wrapped action's damping, rotation and strength, each through its inlined getter that
// tests this and the wrapped object for null and falls back to 0 or the identity quaternion. The exe copies every
// float through x87 (fld, fstp), the damping and the strength through a float slot of its frame.
void bhkAngularDashpotAction::CopyCinfo(hkAngularDashpotActionCinfo& arInfo) {
	bhkBinaryAction::CopyCinfo(arInfo);
	// EXE-INLINED none: bhkAngularDashpotAction::GetDamping
	float fDamping = (this && phkObject) ? static_cast<hkpAngularDashpotAction*>(phkObject)->m_damping : 0.0f;
	arInfo.m_damping = fDamping;
	// EXE-INLINED none: bhkAngularDashpotAction::GethkRotation
	const hkQuaternion* pRotation =
		(this && phkObject) ? &static_cast<hkpAngularDashpotAction*>(phkObject)->m_rotation : &hkQuaternionIdentity;
	// EXE-INLINED 0x4B4DB0
	arInfo.m_rotation.m_vec(0) = pRotation->m_vec(0);
	arInfo.m_rotation.m_vec(1) = pRotation->m_vec(1);
	arInfo.m_rotation.m_vec(2) = pRotation->m_vec(2);
	arInfo.m_rotation.m_vec(3) = pRotation->m_vec(3);
	// EXE-INLINED none: bhkAngularDashpotAction::GetStrength
	float fStrength = (this && phkObject) ? static_cast<hkpAngularDashpotAction*>(phkObject)->m_strength : 0.0f;
	arInfo.m_strength = fStrength;
}

// The class loader's factory, the constructors inlined.
NiObject* bhkAngularDashpotAction::CreateObject() {
	bhkAngularDashpotAction* pObject =
		static_cast<bhkAngularDashpotAction*>(NiMemObject::operator new(sizeof(bhkAngularDashpotAction)));
	if (!pObject)
		return nullptr;
	return ::new (pObject) bhkAngularDashpotAction;
}

// The base's fields cleared by its constructor, the identity quaternion and the default strength and damping.
hkAngularDashpotActionCinfo::hkAngularDashpotActionCinfo()
	: hkBinaryActionCinfo(), m_rotation(hkQuaternionIdentity), m_strength(0.1f), m_damping(0.01f) {}
