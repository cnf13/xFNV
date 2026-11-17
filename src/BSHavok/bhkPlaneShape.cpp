#include "BSHavok/bhkPlaneShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkHeightFieldShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/Plane/hkpPlaneShape.h"

#include <bit>

NiRTTI bhkPlaneShape::ms_RTTI("bhkPlaneShape", &bhkHeightFieldShape::ms_RTTI);

bhkPlaneShape::bhkPlaneShape() {
	++iCount;
}

bhkPlaneShape::~bhkPlaneShape() {
	--iCount;
}

const NiRTTI* bhkPlaneShape::GetRTTI() const {
	return &bhkPlaneShape::ms_RTTI;
}

// A new default-constructed plane shape built under the engine's clone critical section and filled from this one; the
// whole body charges its allocations to Havok's heap context. Its cleanup-only unwind frame is left out.
NiObject* bhkPlaneShape::CreateClone(NiCloningProcess& arCloning) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkSerializable::CloneCritLock();
	bhkPlaneShape* pClone = nullptr;
	bhkPlaneShape* pShape = static_cast<bhkPlaneShape*>(NiMemObject::operator new(sizeof(bhkPlaneShape)));
	if (pShape) {
		pClone = ::new (pShape) bhkPlaneShape;
	}
	// The exe calls CopyMembers through the vtable (slot +0xC4), so it goes through a base pointer.
	static_cast<bhkShape*>(this)->CopyMembers(pClone, arCloning);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

void bhkPlaneShape::LoadBinary(NiStream& arStream) {
	NiStream* pStream = &arStream;
	NiStream* v2 = pStream;
	bhkShape::LoadBinary(*pStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&v2->m_bSourceIsLittleEndian)) != 1) {
		// The exe keeps the flag in the dead stream parameter's slot.
		bool bCreated;
		hkPlaneShapeCinfo* v4 = static_cast<hkPlaneShapeCinfo*>(&CreateCinfo(bCreated));
		v4->m_plane.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_plane.fX)));
		v4->m_plane.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_plane.fY)));
		v4->m_plane.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_plane.fZ)));
		v4->m_plane.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_plane.fW)));
		v4->m_aabbHalfExtents.fX =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbHalfExtents.fX)));
		v4->m_aabbHalfExtents.fY =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbHalfExtents.fY)));
		v4->m_aabbHalfExtents.fZ =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbHalfExtents.fZ)));
		v4->m_aabbHalfExtents.fW =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbHalfExtents.fW)));
		v4->m_aabbCenter.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbCenter.fX)));
		v4->m_aabbCenter.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbCenter.fY)));
		v4->m_aabbCenter.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbCenter.fZ)));
		v4->m_aabbCenter.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(v4->m_aabbCenter.fW)));
	}
}

// Writes the cinfo through the base, byte-swapping its three vectors (a word at a time) before the write and back
// after it when the stream is not little-endian.
void bhkPlaneShape::SaveBinary(NiStream& arStream) {
	bool bCreated;
	hkPlaneShapeCinfo* pCinfo = static_cast<hkPlaneShapeCinfo*>(&CreateCinfo(bCreated));
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		pCinfo->m_plane.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fX)));
		pCinfo->m_plane.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fY)));
		pCinfo->m_plane.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fZ)));
		pCinfo->m_plane.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fW)));
		pCinfo->m_aabbHalfExtents.fX =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fX)));
		pCinfo->m_aabbHalfExtents.fY =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fY)));
		pCinfo->m_aabbHalfExtents.fZ =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fZ)));
		pCinfo->m_aabbHalfExtents.fW =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fW)));
		pCinfo->m_aabbCenter.fX =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fX)));
		pCinfo->m_aabbCenter.fY =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fY)));
		pCinfo->m_aabbCenter.fZ =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fZ)));
		pCinfo->m_aabbCenter.fW =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fW)));
	}
	bhkShape::SaveBinary(arStream);
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
		pCinfo->m_plane.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fX)));
		pCinfo->m_plane.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fY)));
		pCinfo->m_plane.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fZ)));
		pCinfo->m_plane.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_plane.fW)));
		pCinfo->m_aabbHalfExtents.fX =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fX)));
		pCinfo->m_aabbHalfExtents.fY =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fY)));
		pCinfo->m_aabbHalfExtents.fZ =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fZ)));
		pCinfo->m_aabbHalfExtents.fW =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbHalfExtents.fW)));
		pCinfo->m_aabbCenter.fX =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fX)));
		pCinfo->m_aabbCenter.fY =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fY)));
		pCinfo->m_aabbCenter.fZ =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fZ)));
		pCinfo->m_aabbCenter.fW =
			std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(pCinfo->m_aabbCenter.fW)));
	}
}

// The base's strings, this class's name, then the wrapped plane shape's normal (in game units) and its constant (the
// plane's w), all charged to Havok's heap context; the array grows by its grow-by past its allocated size. The exe's
// cleanup-only C++ EH frame is left out.
// KEEP: with no wrapped shape (or a null this) the exe leaves its plane local unwritten and still passes it to HK2NI
// and reads its w for the constant: an uninitialised read.
void bhkPlaneShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiPoint3 kNormal;
	hkVector4 kPlane;
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkHeightFieldShape::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(bhkPlaneShape::ms_RTTI.GetName());
	apStrings->Add(pcString);
	if (this) {
		hkReferencedObject* pObject = this->phkObject;
		if (pObject) {
			kPlane.m_quad = static_cast<hkpPlaneShape*>(pObject)->m_plane.m_quad;
		}
	}
	HK2NI(kNormal, kPlane);
	pcString = kNormal.GetViewerString("Normal");
	apStrings->Add(pcString);
	pcString = NiGetViewerString("Constant", kPlane.m_quad[3]);
	apStrings->Add(pcString);
}

// Releases a creation info this shape made: the aligned block it sits in, found through the padding byte in front
// of it, under the Havok heap context. The exe's cleanup-only EH frame is left out.
void bhkPlaneShape::KillCinfo(bool abCreated) {
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

uint32_t bhkPlaneShape::QCinfoSize() const {
	return (uint32_t)64;
}

// Builds the wrapped hkpPlaneShape from the cinfo's plane, centre and half extents (each copied lane by lane into a
// 16-aligned stack vector), hands it to SethkObject and drops the constructor's reference, then the vtable's PostInit
// with the same cinfo; all charged to Havok's heap context. The exe's cleanup-only C++ EH frame is left out.
void bhkPlaneShape::Init(const hkSerializableCinfo& arInfo) {
	const hkPlaneShapeCinfo& rInfo = static_cast<const hkPlaneShapeCinfo&>(arInfo);
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
	hkpPlaneShape* pShape = static_cast<hkpPlaneShape*>(kHeap.blockAlloc(sizeof(hkpPlaneShape)));
	// The exe writes the allocated size into the block before the constructor runs (a 2-byte store).
	pShape->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpPlaneShape));
	hkVector4 kHalfExtents;
	hkVector4 kCenter;
	hkVector4 kPlane;
	// EXE-INLINED 0x4B4D10
	kHalfExtents.set(rInfo.m_aabbHalfExtents.fX, rInfo.m_aabbHalfExtents.fY, rInfo.m_aabbHalfExtents.fZ,
		rInfo.m_aabbHalfExtents.fW);
	// EXE-INLINED 0x4B4D10
	kCenter.set(rInfo.m_aabbCenter.fX, rInfo.m_aabbCenter.fY, rInfo.m_aabbCenter.fZ, rInfo.m_aabbCenter.fW);
	// EXE-INLINED 0x4B4D10
	kPlane.set(rInfo.m_plane.fX, rInfo.m_plane.fY, rInfo.m_plane.fZ, rInfo.m_plane.fW);
	// The SDK's hkpPlaneShape(direction, center, halfExtents) constructor.
	pShape = ::new (pShape) hkpPlaneShape(kPlane, kCenter, kHalfExtents);
	SethkObject(pShape);
	pShape->removeReference();
	PostInit(arInfo);
}

// The cinfo, made on first use and filled from the wrapped plane shape when there is one; charged to Havok's heap
// context. The exe inlines the aligned allocation (0x50 bytes: the cinfo and 16 bytes of slack) and the cinfo's
// constructor. Its cleanup-only C++ EH frame is left out.
hkSerializableCinfo& bhkPlaneShape::CreateCinfo(bool& arCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bool bCreated = pInfo == nullptr;
	arCreated = bCreated;
	if (bCreated) {
		// The exe treats the allocation as untyped memory: it aligns the pointer itself and writes the padding length
		// into the byte before it.
		uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkPlaneShapeCinfo) + 0x10));
		const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
		pMemory += ucPadding;
		pMemory[-1] = ucPadding;
		hkPlaneShapeCinfo* pCinfo = reinterpret_cast<hkPlaneShapeCinfo*>(pMemory);
		// EXE-INLINED none: hkPlaneShapeCinfo::hkPlaneShapeCinfo
		pCinfo->m_userData = 0;
		pCinfo->m_plane.fX = 0.0f;
		pCinfo->m_plane.fY = 0.0f;
		pCinfo->m_plane.fZ = 0.0f;
		pCinfo->m_plane.fW = 0.0f;
		pCinfo->m_aabbHalfExtents.fX = 0.0f;
		pCinfo->m_aabbHalfExtents.fY = 0.0f;
		pCinfo->m_aabbHalfExtents.fZ = 0.0f;
		pCinfo->m_aabbHalfExtents.fW = 0.0f;
		pCinfo->m_aabbCenter.fX = 0.0f;
		pCinfo->m_aabbCenter.fY = 0.0f;
		pCinfo->m_aabbCenter.fZ = 0.0f;
		pCinfo->m_aabbCenter.fW = 0.0f;
		bool bNoShape = phkObject == nullptr;
		pInfo = pCinfo;
		if (!bNoShape) {
			CopyCinfo(*pCinfo);
		}
	}
	return *pInfo;
}

// The clone's cinfo, its half extents and centre (all four words each) scaled by the cloning process's scale when that
// is not 1, then the base's members.
void bhkPlaneShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
	bool bCreated;
	hkPlaneShapeCinfo* pCinfo = static_cast<hkPlaneShapeCinfo*>(&CreateCinfo(bCreated));
	// The exe's clone process is always a BSCloningProcess; it reads Scale.x.
	float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
	if (fScale != 1.0f) {
		pCinfo->m_aabbHalfExtents.fX = pCinfo->m_aabbHalfExtents.fX * fScale;
		pCinfo->m_aabbHalfExtents.fY = fScale * pCinfo->m_aabbHalfExtents.fY;
		pCinfo->m_aabbHalfExtents.fZ = pCinfo->m_aabbHalfExtents.fZ * fScale;
		pCinfo->m_aabbHalfExtents.fW = fScale * pCinfo->m_aabbHalfExtents.fW;
		pCinfo->m_aabbCenter.fX = pCinfo->m_aabbCenter.fX * fScale;
		pCinfo->m_aabbCenter.fY = fScale * pCinfo->m_aabbCenter.fY;
		pCinfo->m_aabbCenter.fZ = pCinfo->m_aabbCenter.fZ * fScale;
		pCinfo->m_aabbCenter.fW = fScale * pCinfo->m_aabbCenter.fW;
	}
	bhkShape::CopyMembers(apDest, arCloneData);
}

// bhkShape's CopyCinfo, then the wrapped plane shape's centre, half extents and plane into arInfo, in that order.
void bhkPlaneShape::CopyCinfo(hkPlaneShapeCinfo& arInfo) {
	bhkShape::CopyCinfo(arInfo);
	if (this) {
		hkpPlaneShape* pShape = static_cast<hkpPlaneShape*>(this->phkObject);
		if (pShape) {
			arInfo.m_aabbCenter = pShape->m_aabbCenter;
			arInfo.m_aabbHalfExtents = pShape->m_aabbHalfExtents;
			arInfo.m_plane = pShape->m_plane;
		}
	}
}

// A new default-constructed plane shape, or null when the allocation fails. Its cleanup-only unwind frame is left
// out.
NiObject* bhkPlaneShape::CreateObject() {
	bhkPlaneShape* pShape = static_cast<bhkPlaneShape*>(NiMemObject::operator new(sizeof(bhkPlaneShape)));
	if (pShape) {
		::new (pShape) bhkPlaneShape;
	}
	return pShape;
}

bhkHeightFieldShape::bhkHeightFieldShape() {
	++iCount;
}
