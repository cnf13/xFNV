#include "BSHavok/bhkMotorAction.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkUnaryAction.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpMotorAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

NiRTTI bhkMotorAction::ms_RTTI("bhkMotorAction", &bhkUnaryAction::ms_RTTI);

bhkMotorAction::bhkMotorAction() {
	++iCount;
}

bhkMotorAction::~bhkMotorAction() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkMotorAction::GetRTTI() const {
	return &bhkMotorAction::ms_RTTI;
}

// A clone is built the way CreateObject builds one, under the engine's clone critical section and charged to the Havok
// heap context, then the unary action's members are copied into it. bhkUnaryAction::CopyMembers is called directly, not
// through the vtable.
NiObject* bhkMotorAction::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext memContext(MEM_CONTEXT_HAVOK);
	bhkSerializable::CloneCritLock();
	bhkMotorAction* pClone = static_cast<bhkMotorAction*>(NiMemObject::operator new(sizeof(bhkMotorAction)));
	if (pClone)
		::new (pClone) bhkMotorAction;
	bhkUnaryAction::CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's axis, spin rate and gain byte-swapped in place.
// The stream's byte is compared with 1: any other byte swaps. A cinfo built here is not killed. Each float's four bytes
// are swapped as an untyped 32-bit value, hence the uint32_t views.
void bhkMotorAction::LoadBinary(NiStream& arStream) {
	bhkUnaryAction::LoadBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
		bool bCreated;
		hkMotorActionCinfo& rInfo = static_cast<hkMotorActionCinfo&>(CreateCinfo(bCreated));
		// EXE-INLINED none: bhkStream::Endian
		uint32_t* puiAxis = reinterpret_cast<uint32_t*>(&rInfo.m_axis);
		puiAxis[0] = __builtin_bswap32(puiAxis[0]);
		puiAxis[1] = __builtin_bswap32(puiAxis[1]);
		puiAxis[2] = __builtin_bswap32(puiAxis[2]);
		puiAxis[3] = __builtin_bswap32(puiAxis[3]);
		// EXE-INLINED none: BSEndian
		uint32_t* puiSpinRate = reinterpret_cast<uint32_t*>(&rInfo.m_spinRate);
		*puiSpinRate = __builtin_bswap32(*puiSpinRate);
		// EXE-INLINED none: BSEndian
		uint32_t* puiGain = reinterpret_cast<uint32_t*>(&rInfo.m_gain);
		*puiGain = __builtin_bswap32(*puiGain);
	}
}

// For a big-endian save, the creation info's axis, spin rate and gain are byte-swapped in place around the base's write
// and swapped back after it. The stream's byte is compared with 1 and read again after the base call. Each float's four
// bytes are swapped as an untyped 32-bit word, hence the uint32_t views.
void bhkMotorAction::SaveBinary(NiStream& arStream) {
	bool bCreated;
	hkMotorActionCinfo& rInfo = static_cast<hkMotorActionCinfo&>(CreateCinfo(bCreated));
	uint32_t* puiAxis = reinterpret_cast<uint32_t*>(&rInfo.m_axis);
	uint32_t* puiSpinRate = reinterpret_cast<uint32_t*>(&rInfo.m_spinRate);
	uint32_t* puiGain = reinterpret_cast<uint32_t*>(&rInfo.m_gain);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		// EXE-INLINED none: bhkStream::Endian
		puiAxis[0] = __builtin_bswap32(puiAxis[0]);
		puiAxis[1] = __builtin_bswap32(puiAxis[1]);
		puiAxis[2] = __builtin_bswap32(puiAxis[2]);
		puiAxis[3] = __builtin_bswap32(puiAxis[3]);
		// EXE-INLINED none: BSEndian
		*puiSpinRate = __builtin_bswap32(*puiSpinRate);
		// EXE-INLINED none: BSEndian
		*puiGain = __builtin_bswap32(*puiGain);
	}
	bhkUnaryAction::SaveBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		// EXE-INLINED none: bhkStream::Endian
		puiAxis[0] = __builtin_bswap32(puiAxis[0]);
		puiAxis[1] = __builtin_bswap32(puiAxis[1]);
		puiAxis[2] = __builtin_bswap32(puiAxis[2]);
		puiAxis[3] = __builtin_bswap32(puiAxis[3]);
		// EXE-INLINED none: BSEndian
		*puiSpinRate = __builtin_bswap32(*puiSpinRate);
		// EXE-INLINED none: BSEndian
		*puiGain = __builtin_bswap32(*puiGain);
	}
}

// The base's lines, the class name, then the wrapped action's spin rate and gain through their null-safe getters (0
// when this or the wrapped object is null).
void bhkMotorAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	bhkUnaryAction::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(bhkMotorAction::ms_RTTI.GetName());
	uint32_t m_usSize = apStrings->m_usSize;
	uint32_t m_usMaxSize = apStrings->m_usMaxSize;
	if (m_usSize >= m_usMaxSize) {
		apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(m_usSize, pcString);
	// EXE-INLINED none: bhkMotorAction::GetSpinRate
	float fSpinRate = (this && phkObject) ? static_cast<hkpMotorAction*>(phkObject)->m_spinRate : 0.0f;
	pcString = NiGetViewerString("SpinRate", fSpinRate);
	uint32_t v11 = apStrings->m_usSize;
	uint32_t v12 = apStrings->m_usMaxSize;
	if (v11 >= v12) {
		apStrings->SetSize(v11 + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(v11, pcString);
	// EXE-INLINED none: bhkMotorAction::GetGain
	float fGain = (this && phkObject) ? static_cast<hkpMotorAction*>(phkObject)->m_gain : 0.0f;
	pcString = NiGetViewerString("Gain", fGain);
	uint32_t v16 = apStrings->m_usSize;
	uint32_t v17 = apStrings->m_usMaxSize;
	if (v16 >= v17) {
		apStrings->SetSize(v16 + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(v16, pcString);
	v3->eMemContext = eMemContext;
}

// Releases a creation info this action made: the aligned block it sits in, found through the padding byte in front of
// it, under the Havok heap context.
void bhkMotorAction::KillCinfo(bool abCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		if (pInfo) {
			// The block is treated as raw memory: the alignment padding byte sits in front of the pointer.
			uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pInfo));
			MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
		}
		pInfo = nullptr;
	}
}

uint32_t bhkMotorAction::QCinfoSize() const {
	return 0x30;
}

// Builds the wrapped hkpMotorAction in the Havok heap context: a block from the calling thread's heap allocator, its
// size word set, the action's constructor run on it with the creation info's body, axis, spin rate and gain, then the
// action handed to SethkObject and the constructor's reference dropped. PostInit drops the creation info.
void bhkMotorAction::Init(const hkSerializableCinfo& arInfo) {
	int32_t& eMemContext = GetThreadMemContext();
	int32_t iSavedMemContext = eMemContext;
	eMemContext = MEM_CONTEXT_HAVOK;
	const hkMotorActionCinfo& kCinfo = static_cast<const hkMotorActionCinfo&>(arInfo);
	hkpMotorAction* pAction =
		static_cast<hkpMotorAction*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpMotorAction)));
	pAction->m_memSizeAndFlags = sizeof(hkpMotorAction);
	hkVector4 kAxis;
	// EXE-INLINED 0x4B4D10
	kAxis.set(kCinfo.m_axis.fX, kCinfo.m_axis.fY, kCinfo.m_axis.fZ, kCinfo.m_axis.fW);
	pAction = ::new (pAction) hkpMotorAction(kCinfo.m_body, kAxis, kCinfo.m_spinRate, kCinfo.m_gain);
	static_cast<bhkSerializable*>(this)->SethkObject(pAction);
	pAction->removeReference();
	static_cast<bhkSerializable*>(this)->PostInit(arInfo);
	eMemContext = iSavedMemContext;
}

// Builds the creation info, from the wrapped action when there is one and from the motor's defaults otherwise. The
// engine's aligned allocation is done here: the padding length sits in the byte before the block, and the block holds
// the cinfo's size plus the 16 bytes alignment can need. The bytes between m_body and m_axis are left unwritten, as the
// game does.
hkSerializableCinfo& bhkMotorAction::CreateCinfo(bool& arCreated) {
	int32_t& eMemContext = GetThreadMemContext();
	int32_t iSavedMemContext = eMemContext;
	eMemContext = MEM_CONTEXT_HAVOK;
	arCreated = pInfo == nullptr;
	if (arCreated) {
		uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkMotorActionCinfo) + 16));
		uint8_t uiPadding = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
		// Raw memory the engine handed back is aligned here; the cinfo is laid over it.
		hkMotorActionCinfo* pCinfo = reinterpret_cast<hkMotorActionCinfo*>(pMemory + uiPadding);
		pMemory[uiPadding - 1] = uiPadding;
		pCinfo->m_userData = 0;
		pCinfo->m_body = nullptr;
		pCinfo->m_axis.fX = 0.0f;
		pCinfo->m_axis.fY = 0.0f;
		pCinfo->m_axis.fZ = 0.0f;
		pCinfo->m_axis.fW = 0.0f;
		pCinfo->m_spinRate = 0.0f;
		pCinfo->m_gain = 2.0f;
		pCinfo->m_active.m_bool = 1;
		pInfo = pCinfo;
		if (phkObject)
			CopyCinfo(*pCinfo);
	}
	hkSerializableCinfo* pResult = pInfo;
	eMemContext = iSavedMemContext;
	return *pResult;
}

// The unary action's fields, then the motor's: active set, then the wrapped action's axis, gain and spin rate, each
// through a null-safe getter that tests this and the wrapped object and falls back to the zero vector or 0. The axis
// goes through bhkVector4::operator=.
void bhkMotorAction::CopyCinfo(hkMotorActionCinfo& arInfo) {
	bhkUnaryAction::CopyCinfo(arInfo);
	// EXE-INLINED none: hkBool::operator=
	arInfo.m_active.m_bool = 1;
	// EXE-INLINED none: bhkMotorAction::GethkAxis
	const hkVector4* pAxis = (this && phkObject) ? &static_cast<hkpMotorAction*>(phkObject)->m_axis : &hkVector4Zero;
	arInfo.m_axis = *pAxis;
	// EXE-INLINED none: bhkMotorAction::GetGain
	float fGain = (this && phkObject) ? static_cast<hkpMotorAction*>(phkObject)->m_gain : 0.0f;
	arInfo.m_gain = fGain;
	// EXE-INLINED none: bhkMotorAction::GetSpinRate
	float fSpinRate = (this && phkObject) ? static_cast<hkpMotorAction*>(phkObject)->m_spinRate : 0.0f;
	arInfo.m_spinRate = fSpinRate;
}

// The creation info starts null.
NiObject* bhkMotorAction::CreateObject() {
	bhkMotorAction* pThis = static_cast<bhkMotorAction*>(NiMemObject::operator new(sizeof(bhkMotorAction)));
	if (!pThis)
		return nullptr;
	return ::new (pThis) bhkMotorAction;
}
