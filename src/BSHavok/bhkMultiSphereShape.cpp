#include "BSHavok/bhkMultiSphereShape.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Reconstruction.hpp"

NiRTTI bhkMultiSphereShape::ms_RTTI("bhkMultiSphereShape", &bhkSphereRepShape::ms_RTTI);

// hkArray<hkVector4>'s destructor (it frees with 16-byte elements): its callers are hkMultiSphereShapeCinfo's
// destructor (m_spheres) and EH unwind funclets (0xF30F78-0xF345F0; src has none, the compiler makes its own) that
// destroy a local or member array. It frees the array (0x66DAB0), then runs the empty base destructor, folded with
// BaseProcess::ComputeLastTimeProcessed (0x483710), which reads nothing: the same on x64. Thiscall: the object
// comes first, then the unused edx.
void DestroyVector4Array(void* apThis, [[maybe_unused]] void* apEdx) {
	::clearAndDeallocate(apThis, nullptr);
	static_cast<BaseProcess*>(apThis)->BaseProcess::ComputeLastTimeProcessed();
}

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx. Returns
// nothing: eax is what _clearAndDeallocate (0x66DAE0) left.
void clearAndDeallocate(void* apThis, [[maybe_unused]] void* apEdx) {
	hkContainerHeapAllocator v4 = hkContainerHeapAllocator{};
	hkMemoryAllocator* v1 = &(&v4)->get(apThis);
	static_cast<hkArrayBase<hkVector4>*>(apThis)->_clearAndDeallocate(*v1);
}

bhkMultiSphereShape::bhkMultiSphereShape() {
	++iCount;
}

bhkMultiSphereShape::bhkMultiSphereShape(const hkMultiSphereShapeCinfo& arInfo) {
	bhkMultiSphereShape::Init(arInfo);
	++iCount;
}

bhkMultiSphereShape::~bhkMultiSphereShape() {
	bhkSerializable::Kill();
	--iCount;
}

// Clones the shape under the engine's clone critical section: the default constructor, then CopyMembers through the
// vtable (slot 49, +0xC4). The exe's cleanup-only EH frame is left out.
NiObject* bhkMultiSphereShape::CreateClone(NiCloningProcess& arCloneData) {
	bhkSerializable::CloneCritLock();
	bhkMultiSphereShape* pClone =
		static_cast<bhkMultiSphereShape*>(NiMemObject::operator new(sizeof(bhkMultiSphereShape)));
	if (pClone) {
		::new (pClone) bhkMultiSphereShape;
	}
	bhkShape* pThis = this;
	pThis->CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

// Reads the base shape's data, then the creation info's spheres, byte-swapped when the stream's source-endian byte
// (0x1E5) is not 1. The exe calls CreateCinfo (slot 45, +0xB4) and QCinfoSize (slot 42, +0xA8) through the vtable.
void bhkMultiSphereShape::LoadBinary(NiStream& arStream) {
	bhkShape::LoadBinary(arStream);
	bool bCreated;
	bhkSerializable* pThis = this;
	hkMultiSphereShapeCinfo* pCinfo = &static_cast<hkMultiSphereShapeCinfo&>(pThis->CreateCinfo(bCreated));
	// The exe compares the flag's raw byte with 1 (`cmp byte ptr [ebx+0x1E5], 1`): any other byte swaps. A bool
	// compare would only test for zero, so the byte is read as untyped memory.
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
		bhkStream::SwapHKArray(pCinfo, static_cast<int32_t>(pThis->QCinfoSize()), &pCinfo->m_spheres);
	}
	bhkStream::LoadBinary(arStream, pCinfo->m_spheres);
}

// Saves the shape's data and the creation info's spheres: the spheres are swapped out of the info into a local
// array, the emptied info array is byte-swapped around the save when the stream saves big-endian, the local array is
// written after the base's data, the info is killed and the local array freed. The exe calls CreateCinfo (+0xB4),
// QCinfoSize (+0xA8) and KillCinfo (+0xA4) through the vtable. The exe's cleanup-only C++ EH frame is left out.
void bhkMultiSphereShape::SaveBinary(NiStream& arStream) {
	bool bCreated;
	bhkSerializable* pThis = this;
	hkMultiSphereShapeCinfo* pCinfo = &static_cast<hkMultiSphereShapeCinfo&>(pThis->CreateCinfo(bCreated));
	using kSpheres_t = hkArray<hkVector4, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(kSpheres_t, kSpheres, std::nullopt, std::nullopt);
	// EXE-INLINED 0x6299A0
	kSpheres.m_data = nullptr;
	kSpheres.m_size = 0;
	kSpheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	kSpheres.swap(pCinfo->m_spheres);
	// The exe compares the flag's raw byte with 1 (`cmp byte ptr [ebx+0x1E4], 1`): any other byte swaps. A bool
	// compare would only test for zero, so the byte is read as untyped memory.
	const uint8_t* pucSaveLittleEndian =
		static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian));
	if (*pucSaveLittleEndian != 1) {
		bhkStream::SwapHKArray(pCinfo, static_cast<int32_t>(pThis->QCinfoSize()), &pCinfo->m_spheres);
	}
	bhkShape::SaveBinary(arStream);
	bhkStream::SaveBinary(arStream, kSpheres);
	pThis->KillCinfo(bCreated);
	if (!bCreated && *pucSaveLittleEndian != 1) {
		bhkStream::SwapHKArray(pCinfo, static_cast<int32_t>(pThis->QCinfoSize()), &pCinfo->m_spheres);
	}
	// EXE-INLINED 0x66DA50
	hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
	kSpheres.m_size = 0;
	if (kSpheres.m_capacityAndFlags >= 0) {
		rHeap.bufFree(kSpheres.m_data,
			static_cast<int32_t>(kSpheres.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(hkVector4)));
	}
}

// Compares the two shapes: the base's data first, then the spheres each shape's Havok object holds, gathered into
// creation infos and compared as arrays. The exe inlines the infos' construction and destruction (src's hkArray has
// neither): each info starts out empty, and after the comparison each array is freed through the thread's heap
// allocator (the exe's inlined hkArray destructor). The exe's cleanup-only EH frame is left out.
bool bhkMultiSphereShape::IsEqual(NiObject* apObject) {
	if (!bhkShape::IsEqual(apObject))
		return false;

	FNVD_INLINED_LOCAL(hkMultiSphereShapeCinfo, infoThis, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(hkMultiSphereShapeCinfo, infoOther, std::nullopt, std::nullopt);
	infoThis.m_userData = 0;
	infoThis.m_spheres.m_data = nullptr;
	infoThis.m_spheres.m_size = 0;
	infoThis.m_spheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	infoOther.m_userData = 0;
	infoOther.m_spheres.m_data = nullptr;
	infoOther.m_spheres.m_size = 0;
	infoOther.m_spheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	GetCinfo(infoThis);
	static_cast<bhkMultiSphereShape*>(apObject)->GetCinfo(infoOther);
	bool bEqual = bhkStream::IsEqual(infoThis.m_spheres, infoOther.m_spheres);

	// The exe asks the thread's router for its heap once per array, before testing whether the array owns storage.
	hkMemoryAllocator& rOtherHeap = hkMemoryRouter::getInstance().heap();
	infoOther.m_spheres.m_size = 0;
	if (infoOther.m_spheres.m_capacityAndFlags >= 0) {
		rOtherHeap.bufFree(infoOther.m_spheres.m_data,
			static_cast<int32_t>(infoOther.m_spheres.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(hkVector4)));
	}
	infoOther.m_spheres.m_data = nullptr;
	infoOther.m_spheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	hkMemoryAllocator& rThisHeap = hkMemoryRouter::getInstance().heap();
	infoThis.m_spheres.m_size = 0;
	if (infoThis.m_spheres.m_capacityAndFlags >= 0) {
		rThisHeap.bufFree(infoThis.m_spheres.m_data,
			static_cast<int32_t>(infoThis.m_spheres.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(hkVector4)));
	}
	infoThis.m_spheres.m_data = nullptr;
	infoThis.m_spheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	return bEqual;
}

// The sphere-rep shape's strings, then the class name, the sphere count and each sphere's centre (scaled from Havok
// to Gamebryo units by bhkMultiSphereShape_fHk2BSScaleSC) and radius, inside the HAVOK memory context. The exe inlines
// NiTPrimitiveArray::Add. The exe's cleanup-only C++ EH frame is left out.
void bhkMultiSphereShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	bhkSphereRepShape::GetViewerStrings(apStrings);
	char* pcName = NiGetViewerString(bhkMultiSphereShape::ms_RTTI.GetName());
	apStrings->Add(pcName);
	int32_t iNumSpheres;
	hkpMultiSphereShape* pShape;
	if (this && (pShape = static_cast<hkpMultiSphereShape*>(phkObject)) != nullptr) {
		iNumSpheres = pShape->m_numSpheres;
	} else {
		iNumSpheres = 0;
	}
	char* pcCount = NiGetViewerString("Num Spheres", iNumSpheres);
	apStrings->Add(pcCount);
	if (this) {
		pShape = static_cast<hkpMultiSphereShape*>(phkObject);
		if (pShape) {
			const hkVector4* pSphere = pShape->m_spheres;
			if (pSphere) {
				char acPrefix[32];
				for (int32_t i = 0; i < iNumSpheres; ++i, ++pSphere) {
					__m128 kSphere = pSphere->m_quad;
					// x87 copy at 0xCB7EE7: the radius, loaded from the sphere's w.
					float fRadius = pSphere->m_quad[3];
					float fX = kSphere[0];
					float fY = kSphere[1];
					float fZ = kSphere[2];
					alignas(16) NiPoint3 kPosition;
					kPosition.x = fX * bhkMultiSphereShape_fHk2BSScaleSC;
					kPosition.y = fY * bhkMultiSphereShape_fHk2BSScaleSC;
					kPosition.z = bhkMultiSphereShape_fHk2BSScaleSC * fZ;
					BSsprintf(acPrefix, 32, "Pos %d", i);
					char* pcPosition = kPosition.GetViewerString(acPrefix);
					apStrings->Add(pcPosition);
					BSsprintf(acPrefix, 32, "Radius %d", i);
					char* pcRadius = NiGetViewerString(acPrefix, fRadius);
					apStrings->Add(pcRadius);
				}
			}
		}
	}
	pTLSData->eMemContext = eMemContext;
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkMultiSphereShape::KillCinfo(bool abCreated) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	MEM_CONTEXT v8 = eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	if (abCreated) {
		hkMultiSphereShapeCinfo* pi = static_cast<hkMultiSphereShapeCinfo*>(pInfo);
		if (pi) {
			hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
			int32_t m_capacityAndFlags = pi->m_spheres.m_capacityAndFlags;
			pi->m_spheres.m_size = 0;
			if (m_capacityAndFlags >= 0) {
				m_heap->bufFree(pi->m_spheres.begin(), sizeof(hkVector4) * m_capacityAndFlags);
			}
			pi->m_spheres.m_data = 0;
			pi->m_spheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			MemoryManager_Singleton.Deallocate(pi);
			eMemContext = v8;
		}
		pInfo = 0;
	}
	v3->eMemContext = eMemContext;
}

uint32_t bhkMultiSphereShape::QCinfoSize() const {
	return 16;
}

// Builds the wrapped Havok shape from the creation info's spheres: the shape comes from the thread's Havok heap
// inside the HAVOK memory context, its size word is set, then it is constructed, attached and posted in. The exe's
// cleanup-only EH frame is left out.
void bhkMultiSphereShape::Init(const hkSerializableCinfo& arInfo) {
	const hkMultiSphereShapeCinfo& rCinfo = static_cast<const hkMultiSphereShapeCinfo&>(arInfo);
	int32_t iMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	hkpMultiSphereShape* pShape =
		static_cast<hkpMultiSphereShape*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpMultiSphereShape)));
	pShape->m_memSizeAndFlags = sizeof(hkpMultiSphereShape);
	hkpMultiSphereShape* pBuilt = ::new (pShape) hkpMultiSphereShape(rCinfo.m_spheres.m_data, rCinfo.m_spheres.m_size);
	bhkSerializable* pThis = this;
	pThis->SethkObject(pBuilt);
	pBuilt->removeReference();
	pThis->PostInit(arInfo);
	GetThreadMemContext() = iMemContext;
}

// Makes the shape's creation info, inside the HAVOK memory context: when the shape holds none, an empty one is
// allocated with the engine's memory manager, becomes the shape's, and the wrapped Havok shape fills its spheres.
// The exe's cleanup-only EH frame is left out.
hkSerializableCinfo& bhkMultiSphereShape::CreateCinfo(bool& arCreated) {
	int32_t iMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	arCreated = (pInfo == nullptr);
	if (arCreated) {
		hkMultiSphereShapeCinfo* pCinfo =
			static_cast<hkMultiSphereShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkMultiSphereShapeCinfo)));
		if (pCinfo) {
			pCinfo->m_userData = 0;
			pCinfo->m_spheres.m_data = nullptr;
			pCinfo->m_spheres.m_size = 0;
			pCinfo->m_spheres.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		}
		bool bHasObject = (phkObject != nullptr);
		pInfo = pCinfo;
		if (bHasObject)
			GetCinfo(*pCinfo);
	}
	hkSerializableCinfo* pResult = pInfo;
	GetThreadMemContext() = iMemContext;
	return *pResult;
}

// Scales the creation info's spheres (centres and radii, all four lanes) by the cloning process's scale when it is
// not 1, from the last sphere down, then copies the shape's members. The exe calls CreateCinfo through the vtable
// (+0xB4) and does not kill the info it may make.
void bhkMultiSphereShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
	bool bCreated;
	bhkSerializable* pThis = this;
	hkMultiSphereShapeCinfo& rCinfo = static_cast<hkMultiSphereShapeCinfo&>(pThis->CreateCinfo(bCreated));
	// The exe's clone process is always a BSCloningProcess; it reads Scale.x.
	float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
	if (fScale != 1.0f) {
		int32_t iIndex = rCinfo.m_spheres.m_size;
		while (iIndex != 0) {
			--iIndex;
			alignas(16) hkSimdReal kScale;
			kScale.~hkSimdReal();
			::new (&kScale) hkSimdReal(fScale);
			rCinfo.m_spheres.m_data[iIndex].mul4(kScale);
		}
	}
	bhkShape::CopyMembers(apDest, arCloneData);
}

// Copies the shape's own creation info into arCinfo, then the wrapped Havok shape's spheres into its m_spheres,
// inside the HAVOK memory context. The exe inlines the array's clear and setSize. The exe's cleanup-only C++ EH frame
// is left out.
void bhkMultiSphereShape::GetCinfo(hkMultiSphereShapeCinfo& arCinfo) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	int32_t iNumSpheres = 0;
	CopyCinfo(arCinfo);
	if (this) {
		hkpMultiSphereShape* pShape = static_cast<hkpMultiSphereShape*>(phkObject);
		if (pShape) {
			iNumSpheres = pShape->m_numSpheres;
		}
	}
	arCinfo.m_spheres.clear();
	arCinfo.m_spheres.setSize(iNumSpheres);
	const hkVector4* pSpheres;
	if (this && phkObject) {
		pSpheres = static_cast<hkpMultiSphereShape*>(phkObject)->m_spheres;
	} else {
		pSpheres = nullptr;
	}
	for (int32_t i = 0; i < iNumSpheres; ++i) {
		arCinfo.m_spheres.m_data[i] = pSpheres[i];
	}
	pTLSData->eMemContext = eMemContext;
}

// The engine's factory for the class: a bare shape with no creation info yet, from the default constructor. Its
// cleanup-only EH frame is left out.
NiObject* bhkMultiSphereShape::CreateObject() {
	bhkMultiSphereShape* pObject =
		static_cast<bhkMultiSphereShape*>(NiMemObject::operator new(sizeof(bhkMultiSphereShape)));
	if (!pObject)
		return nullptr;
	return ::new (pObject) bhkMultiSphereShape;
}

hkMultiSphereShapeCinfo::hkMultiSphereShapeCinfo() {}

hkMultiSphereShapeCinfo::~hkMultiSphereShapeCinfo() {}
