#include "BSHavok/bhkAabbPhantom.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"

NiRTTI bhkAabbPhantom::ms_RTTI("bhkAabbPhantom", &bhkPhantom::ms_RTTI);

hkAabbPhantomCinfo::hkAabbPhantomCinfo() {
	aabb.m_min.setZero4();
	aabb.m_max.setZero4();
}

hkAabbPhantomCinfo::~hkAabbPhantomCinfo() {}

bhkAabbPhantom::bhkAabbPhantom() {
	++iCount;
	ucFlags = 0;
}

bhkAabbPhantom::bhkAabbPhantom(const hkAabbPhantomCinfo& arInfo) {
	bhkAabbPhantom::Init(arInfo);
	++iCount;
	ResetPhantomFlags();
}

bhkAabbPhantom::~bhkAabbPhantom() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkAabbPhantom::GetRTTI() const {
	return &bhkAabbPhantom::ms_RTTI;
}

// A copy of this wrapper for the cloning process: the default constructor as in CreateObject (inlined by the exe),
// then the world object's members copied into the clone, all inside the engine's clone critical section. The exe
// calls the world object's own out-of-line CopyMembers (0xC85900), not the serializable one.
NiObject* bhkAabbPhantom::CreateClone(NiCloningProcess& arCloneData) {
	bhkSerializable::CloneCritLock();
	bhkAabbPhantom* pClone = static_cast<bhkAabbPhantom*>(NiMemObject::operator new(sizeof(bhkAabbPhantom)));
	if (pClone) {
		::new (pClone) bhkAabbPhantom;
	} else {
		pClone = nullptr;
	}
	bhkWorldObject::CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

// Builds the Havok box phantom from the creation info: a 0xE0-byte heap block from the thread's Havok heap, its
// size word set before the constructor runs, then the wrapper takes it and drops the constructor's reference. The
// exe reads the box and the collision filter info straight out of the info.
void bhkAabbPhantom::Init(const hkSerializableCinfo& arInfo) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	const hkAabbPhantomCinfo& rInfo = static_cast<const hkAabbPhantomCinfo&>(arInfo);
	void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAabbPhantom));
	static_cast<hkpAabbPhantom*>(pMemory)->m_memSizeAndFlags = sizeof(hkpAabbPhantom);
	hkpAabbPhantom* pPhantom = ::new (pMemory) hkpAabbPhantom(rInfo.aabb, rInfo.m_collisionFilterInfo);
	static_cast<bhkSerializable*>(this)->SethkObject(pPhantom);
	pPhantom->removeReference();
	static_cast<bhkSerializable*>(this)->PostInit(arInfo);
}

// Appends this class's line, the RTTI name, after the phantom's own lines. The exe calls the base body, then grows
// the array by its own grow-by value when it is full.
void bhkAabbPhantom::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkPhantom::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
	apStrings->Add(pcString);
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkAabbPhantom::KillCinfo(bool abCreated) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	if (abCreated) {
		hkSerializableCinfo* pInfo = this->pInfo;
		if (pInfo) {
			delete static_cast<hkWorldObjectCinfo*>(pInfo);
		}
		this->pInfo = 0;
	}
	v3->eMemContext = eMemContext;
}

// Reads the world object's part, then swaps the box's byte order for a big-endian source, the box's max before its
// min. The PDB's GetCinfo is the virtual CreateCinfo here; the exe drops its created flag.
void bhkAabbPhantom::LoadBinary(NiStream& arStream) {
	bhkWorldObject::LoadBinary(arStream);
	bool bCreated;
	hkAabbPhantomCinfo& rInfo = static_cast<hkAabbPhantomCinfo&>(CreateCinfo(bCreated));
	if (!arStream.m_bSourceIsLittleEndian) {
		bhkStream::Endian(rInfo.aabb.m_max);
		bhkStream::Endian(rInfo.aabb.m_min);
	}
}

// Writes the world object's part with the box in the target's byte order, then swaps it back. The exe drops
// CreateCinfo's created flag here too.
void bhkAabbPhantom::SaveBinary(NiStream& arStream) {
	bool bCreated;
	hkAabbPhantomCinfo& rInfo = static_cast<hkAabbPhantomCinfo&>(CreateCinfo(bCreated));
	if (!arStream.m_bSaveLittleEndian) {
		bhkStream::Endian(rInfo.aabb.m_max);
		bhkStream::Endian(rInfo.aabb.m_min);
	}
	bhkWorldObject::SaveBinary(arStream);
	if (!arStream.m_bSaveLittleEndian) {
		bhkStream::Endian(rInfo.aabb.m_max);
		bhkStream::Endian(rInfo.aabb.m_min);
	}
}

// Makes the creation info on first use and fills it from the wrapped phantom when there is one. The exe treats the
// block as raw memory to align it to 16: it allocates 0x10 bytes more than the info and stores the padding in the
// byte in front of it. The info's default constructor is inlined. The exe's cleanup-only EH frame is left out.
hkSerializableCinfo& bhkAabbPhantom::CreateCinfo(bool& arCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	const bool bCreated = this->pInfo == nullptr;
	arCreated = bCreated;
	if (bCreated) {
		uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkAabbPhantomCinfo) + 0x10u));
		const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
		pMemory += ucPadding;
		pMemory[-1] = ucPadding;
		hkAabbPhantomCinfo* pInfo = static_cast<hkAabbPhantomCinfo*>(static_cast<void*>(pMemory));
		// EXE-INLINED 0x56E4E0
		pInfo->m_properties.m_data = nullptr;
		pInfo->m_properties.m_size = 0;
		pInfo->m_properties.m_capacityAndFlags = hkArrayBase<hkWorldObjectCinfo::Property>::DONT_DEALLOCATE_FLAG;
		pInfo->m_collisionFilterInfo = 0;
		pInfo->m_shape = nullptr;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		pInfo->m_broadPhaseType.m_storage = hkpWorldObject::BROAD_PHASE_PHANTOM;
		pInfo->aabb.m_min.setZero4();
		pInfo->aabb.m_max.setZero4();
		this->pInfo = pInfo;
		if (this->phkObject) {
			CopyCinfo(*pInfo);
		}
	}
	return *this->pInfo;
}

hkpAabbPhantom* bhkAabbPhantom::operator*() const {
	if (this) {
		return static_cast<hkpAabbPhantom*>(GethkObject());
	} else {
		return nullptr;
	}
}

// Fills arInfo's world-object part, then copies the wrapped phantom's box into it.
void bhkAabbPhantom::CopyCinfo(hkAabbPhantomCinfo& arInfo) {
	FNVD_INLINED_LOCAL(hkAabb, kBound, std::nullopt, std::nullopt);
	bhkWorldObject::CopyCinfo(arInfo);
	GetAabb(kBound);
	arInfo.aabb = kBound;
}

// Builds a bare wrapper: the default constructor, which the exe inlines here.
NiObject* bhkAabbPhantom::CreateObject() {
	bhkAabbPhantom* pObject = static_cast<bhkAabbPhantom*>(NiMemObject::operator new(sizeof(bhkAabbPhantom)));
	if (!pObject)
		return nullptr;
	return ::new (pObject) bhkAabbPhantom;
}
