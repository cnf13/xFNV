#include "BSHavok/bhkOrientHingedBodyAction.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkUnaryAction.h"
#include "BSHavok/hkOrientHingedBodyAction.h"
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
#include "Fallout/AI/Combat/CombatPlanner.h"

NiRTTI bhkOrientHingedBodyAction::ms_RTTI("bhkOrientHingedBodyAction", &bhkUnaryAction::ms_RTTI);

bhkOrientHingedBodyAction::bhkOrientHingedBodyAction() {}

// The exe's destructor sequence (0xCB83F0): the wrapper's kill, then the class's live count.
bhkOrientHingedBodyAction::~bhkOrientHingedBodyAction() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkOrientHingedBodyAction::GetRTTI() const {
	return &bhkOrientHingedBodyAction::ms_RTTI;
}

// The clone is allocated in the Havok heap context while the engine's clone critical section is held, and its
// members are copied through the vtable (slot 50, CopyMembers). The exe inlines the constructor; its SEH frame is
// left out.
NiObject* bhkOrientHingedBodyAction::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkSerializable::CloneCritLock();
	bhkOrientHingedBodyAction* pkClone =
		static_cast<bhkOrientHingedBodyAction*>(NiMemObject::operator new(sizeof(bhkOrientHingedBodyAction)));
	if (pkClone)
		::new (pkClone) bhkOrientHingedBodyAction;
	CopyMembers(pkClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pkClone;
}

// The base's fields, then, for a big-endian source, the creation info's two axes, strength and damping byte-swapped in
// place. The stream's byte is compared with 1 (`cmp byte ptr [edi+0x1E5], 1`): any other byte swaps, so it is read as
// a byte rather than a bool tested. A cinfo built here is not killed. The exe swaps each float's four bytes as an
// untyped dword (mov, bswap, mov), hence the uint32_t views.
void bhkOrientHingedBodyAction::LoadBinary(NiStream& arStream) {
	bhkUnaryAction::LoadBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
		bool bCreated;
		hkOrientHingedBodyActionCinfo& cinfo = static_cast<hkOrientHingedBodyActionCinfo&>(CreateCinfo(bCreated));
		// EXE-INLINED none: bhkStream::Endian
		uint32_t* puiHingeAxis = reinterpret_cast<uint32_t*>(&cinfo.HingeAxisLS);
		puiHingeAxis[0] = __builtin_bswap32(puiHingeAxis[0]);
		puiHingeAxis[1] = __builtin_bswap32(puiHingeAxis[1]);
		puiHingeAxis[2] = __builtin_bswap32(puiHingeAxis[2]);
		puiHingeAxis[3] = __builtin_bswap32(puiHingeAxis[3]);
		// EXE-INLINED none: bhkStream::Endian
		uint32_t* puiForward = reinterpret_cast<uint32_t*>(&cinfo.ForwardLS);
		puiForward[0] = __builtin_bswap32(puiForward[0]);
		puiForward[1] = __builtin_bswap32(puiForward[1]);
		puiForward[2] = __builtin_bswap32(puiForward[2]);
		puiForward[3] = __builtin_bswap32(puiForward[3]);
		// EXE-INLINED none: BSEndian
		uint32_t* puiStrength = reinterpret_cast<uint32_t*>(&cinfo.fStrength);
		*puiStrength = __builtin_bswap32(*puiStrength);
		// EXE-INLINED none: BSEndian
		uint32_t* puiDamping = reinterpret_cast<uint32_t*>(&cinfo.fDamping);
		*puiDamping = __builtin_bswap32(*puiDamping);
	}
}

// For a big-endian save, the creation info's two axes, strength and damping are byte-swapped in place around the
// base's write and swapped back after it. The stream's byte is compared with 1 and read again after the base call.
// The exe swaps each float's four bytes as an untyped dword (mov, bswap, mov), hence the uint32_t views.
void bhkOrientHingedBodyAction::SaveBinary(NiStream& arStream) {
	bool bCreated;
	hkOrientHingedBodyActionCinfo& cinfo = static_cast<hkOrientHingedBodyActionCinfo&>(CreateCinfo(bCreated));
	uint32_t* puiHingeAxis = reinterpret_cast<uint32_t*>(&cinfo.HingeAxisLS);
	uint32_t* puiForward = reinterpret_cast<uint32_t*>(&cinfo.ForwardLS);
	uint32_t* puiStrength = reinterpret_cast<uint32_t*>(&cinfo.fStrength);
	uint32_t* puiDamping = reinterpret_cast<uint32_t*>(&cinfo.fDamping);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		// EXE-INLINED none: bhkStream::Endian
		puiHingeAxis[0] = __builtin_bswap32(puiHingeAxis[0]);
		puiHingeAxis[1] = __builtin_bswap32(puiHingeAxis[1]);
		puiHingeAxis[2] = __builtin_bswap32(puiHingeAxis[2]);
		puiHingeAxis[3] = __builtin_bswap32(puiHingeAxis[3]);
		// EXE-INLINED none: bhkStream::Endian
		puiForward[0] = __builtin_bswap32(puiForward[0]);
		puiForward[1] = __builtin_bswap32(puiForward[1]);
		puiForward[2] = __builtin_bswap32(puiForward[2]);
		puiForward[3] = __builtin_bswap32(puiForward[3]);
		// EXE-INLINED none: BSEndian
		*puiStrength = __builtin_bswap32(*puiStrength);
		// EXE-INLINED none: BSEndian
		*puiDamping = __builtin_bswap32(*puiDamping);
	}
	bhkUnaryAction::SaveBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		// EXE-INLINED none: bhkStream::Endian
		puiHingeAxis[0] = __builtin_bswap32(puiHingeAxis[0]);
		puiHingeAxis[1] = __builtin_bswap32(puiHingeAxis[1]);
		puiHingeAxis[2] = __builtin_bswap32(puiHingeAxis[2]);
		puiHingeAxis[3] = __builtin_bswap32(puiHingeAxis[3]);
		// EXE-INLINED none: bhkStream::Endian
		puiForward[0] = __builtin_bswap32(puiForward[0]);
		puiForward[1] = __builtin_bswap32(puiForward[1]);
		puiForward[2] = __builtin_bswap32(puiForward[2]);
		puiForward[3] = __builtin_bswap32(puiForward[3]);
		// EXE-INLINED none: BSEndian
		*puiStrength = __builtin_bswap32(*puiStrength);
		// EXE-INLINED none: BSEndian
		*puiDamping = __builtin_bswap32(*puiDamping);
	}
}

// The base's strings, then this class's RTTI name appended to the array (the exe's inlined Add: it grows the array
// when it is full and stores at m_usSize). The append's allocation is charged to the Havok heap context.
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

// Releases a creation info this action made: the aligned block it sits in, found through the padding byte in front
// of it, under the Havok heap context. The exe's cleanup-only EH frame is left out.
void bhkOrientHingedBodyAction::KillCinfo(bool abCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		if (pInfo) {
			// The exe treats the block as raw memory: the alignment padding byte sits in front of the pointer.
			uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pInfo));
			MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
		}
		pInfo = nullptr;
	}
}

// The size of the creation info in a file: its serialized form's.
uint32_t bhkOrientHingedBodyAction::QCinfoSize() const {
	return sizeof(hkOrientHingedBodyActionCinfo::Serialized);
}

// The exe has none here (it reads the cinfo raw through bhkSerializable::LoadCInfo); this one reads the serialized
// form and fills the creation info from it, and returns the form's size.
int32_t bhkOrientHingedBodyAction::LoadCInfo(NiStream& arStream) {
	bool bCreated;
	hkOrientHingedBodyActionCinfo& rCinfo = static_cast<hkOrientHingedBodyActionCinfo&>(CreateCinfo(bCreated));
	hkOrientHingedBodyActionCinfo::Serialized kForm{};
	arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
	rCinfo.ReadSerialized(kForm);
	return sizeof(kForm);
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkOrientHingedBodyAction::Init(const hkSerializableCinfo& arInfo) {
	const hkOrientHingedBodyActionCinfo& cinfo = static_cast<const hkOrientHingedBodyActionCinfo&>(arInfo);
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkOrientHingedBodyAction* v5 =
		static_cast<hkOrientHingedBodyAction*>(Instance->m_heap->blockAlloc(sizeof(hkOrientHingedBodyAction)));
	// The native sizeof (the exe's 96 is the 32-bit size; the heap frees by this word).
	v5->m_memSizeAndFlags = sizeof(hkOrientHingedBodyAction);
	hkOrientHingedBodyAction* po = ::new (v5)
		hkOrientHingedBodyAction(cinfo.m_body, cinfo.HingeAxisLS, cinfo.ForwardLS, cinfo.fStrength, cinfo.fDamping, 0);
	SethkObject(po);
	po->removeReference();
	PostInit(arInfo);
	v3->eMemContext = eMemContext;
}

// Builds the action's creation info when there is none: a block the engine aligns to 16 from the Havok heap context
// holding the unary action's zeroed user data and entity, two zero axes, and a 0.5 strength with 0.1 damping. The
// exe's cleanup-only EH frame is left out.
hkSerializableCinfo& bhkOrientHingedBodyAction::CreateCinfo(bool& arCreated) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	arCreated = this->pInfo == nullptr;
	if (arCreated) {
		char* pBlock =
			static_cast<char*>(MemoryManager_Singleton.Allocate(sizeof(hkOrientHingedBodyActionCinfo) + 0x10));
		uint8_t uiPad = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pBlock) & 0xF));
		// The block is the engine's raw memory: the cinfo starts after the padding the alignment needed.
		hkOrientHingedBodyActionCinfo* pCinfo = reinterpret_cast<hkOrientHingedBodyActionCinfo*>(pBlock + uiPad);
		pBlock[uiPad - 1] = static_cast<char>(uiPad);
		pCinfo->m_userData = 0;
		pCinfo->m_body = nullptr;
		pCinfo->HingeAxisLS = hkVector4Zero;
		pCinfo->ForwardLS = hkVector4Zero;
		pCinfo->fStrength = 0.5f;
		pCinfo->fDamping = 0.1f;
		bool bHasObject = this->phkObject != nullptr;
		this->pInfo = pCinfo;
		if (bHasObject) {
			CopyCinfo(*pCinfo);
		}
	}
	hkSerializableCinfo* pInfo = this->pInfo;
	GetThreadMemContext() = iOldMemContext;
	return *pInfo;
}

void bhkOrientHingedBodyAction::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
	bhkUnaryAction::CopyMembers(apDest, arCloneData);
}

void bhkOrientHingedBodyAction::CopyCinfo(hkOrientHingedBodyActionCinfo& arInfo) {
	hkOrientHingedBodyAction* phkObject;
	bhkUnaryAction::CopyCinfo(arInfo);
	if (this) {
		phkObject = static_cast<hkOrientHingedBodyAction*>(this->phkObject);
	} else {
		phkObject = nullptr;
	}
	(&arInfo)->HingeAxisLS = phkObject->HingeAxisLS;
	(&arInfo)->ForwardLS = phkObject->ForwardLS;
	(&arInfo)->fStrength = phkObject->fStrength;
	(&arInfo)->fDamping = phkObject->fDamping;
}

void bhkOrientHingedBodyAction::SetTargetDir(const hkVector4& arDir) {
	hkOrientHingedBodyAction* pAction = static_cast<hkOrientHingedBodyAction*>(bhkRefObject::operator*());
	pAction->setTargetDir(arDir);
}

// The class loader's factory: a new object, or null when the allocation fails (cdecl, ret 0, 0xCB84F0). The exe
// inlines the constructor; its SEH frame is left out.
NiObject* bhkOrientHingedBodyAction::CreateObject() {
	bhkOrientHingedBodyAction* pkObject =
		static_cast<bhkOrientHingedBodyAction*>(NiMemObject::operator new(sizeof(bhkOrientHingedBodyAction)));
	if (!pkObject)
		return nullptr;
	return ::new (pkObject) bhkOrientHingedBodyAction;
}
