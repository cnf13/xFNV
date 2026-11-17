#include "BSHavok/bhkCachingShapePhantom.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"
#include "Fallout Shared/ModelLoader.h"

NiRTTI bhkCachingShapePhantom::ms_RTTI("bhkCachingShapePhantom", &bhkShapePhantom::ms_RTTI);

bhkCachingShapePhantom::bhkCachingShapePhantom() {
	++iCount;
	ucFlags = 0;
}

bhkCachingShapePhantom::bhkCachingShapePhantom(hkShapePhantomCinfo* arInfo) {
	bhkCachingShapePhantom::Init(*arInfo);
	++iCount;
	ucFlags = 0;
}

// Takes the phantom's block from the Havok heap, stamps its size on it, builds the wrapped phantom from the info,
// hands it to SethkObject, drops the constructor's reference and then drops the creation info. The exe converts the
// info's transform to a stack temporary for the constructor (bhkTransform::ToHK, inlined) and leaves its
// cleanup-only EH frame out here (thiscall, ret 4, 0xCC2530).
void bhkCachingShapePhantom::Init(const hkSerializableCinfo& arInfo) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	const hkShapePhantomCinfo& rInfo = static_cast<const hkShapePhantomCinfo&>(arInfo);
	void* pvMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCachingShapePhantom));
	static_cast<hkpCachingShapePhantom*>(pvMemory)->m_memSizeAndFlags = sizeof(hkpCachingShapePhantom);
	hkTransform kTransform;
	rInfo.m_transform.ToHK(kTransform);
	hkpCachingShapePhantom* pPhantom =
		::new (pvMemory) hkpCachingShapePhantom(rInfo.m_shape, kTransform, rInfo.m_collisionFilterInfo);
	SethkObject(pPhantom);
	pPhantom->removeReference();
	PostInit(arInfo);
}

// The scope is charged to the Havok heap context (the exe inlines the guard); the info is freed through
// hkWorldObjectCinfo's deleting destructor, called directly. The exe's cleanup-only C++ EH frame is left out.
void bhkCachingShapePhantom::KillCinfo(bool abCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		if (pInfo) {
			delete static_cast<hkWorldObjectCinfo*>(pInfo);
		}
		pInfo = nullptr;
	}
}

// Makes the phantom's creation info: a fresh hkShapePhantomCinfo in a 16-byte aligned block of 0x70 bytes, filled
// from the wrapped phantom when there is one. The exe inlines the alignment, storing the padding in the byte in
// front of the block, and leaves its cleanup-only EH frame out here.
hkSerializableCinfo& bhkCachingShapePhantom::CreateCinfo(bool& arCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	const bool bCreated = this->pInfo == nullptr;
	arCreated = bCreated;
	if (bCreated) {
		uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(hkShapePhantomCinfo) + 0x10));
		const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
		pMemory += ucPadding;
		pMemory[-1] = ucPadding;
		hkShapePhantomCinfo* pInfo = ::new (pMemory) hkShapePhantomCinfo;
		const bool bHasObject = this->phkObject != nullptr;
		this->pInfo = pInfo;
		if (bHasObject) {
			CopyCinfo(*pInfo);
		}
	}
	return *this->pInfo;
}

const NiRTTI* bhkCachingShapePhantom::GetRTTI() const {
	return &bhkCachingShapePhantom::ms_RTTI;
}

bhkCachingShapePhantom::~bhkCachingShapePhantom() {
	bhkSerializable::Kill();
	--iCount;
}

// Clones the phantom into memory charged to the Havok context, under the engine's clone critical section
// (thiscall, ret 4, 0xCC2480). The exe's EH frame is left out.
NiObject* bhkCachingShapePhantom::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkCachingShapePhantom* pClone = nullptr;
	bhkSerializable::CloneCritLock();
	void* pvMemory = NiMemObject::operator new(sizeof(bhkCachingShapePhantom));
	if (pvMemory) {
		pClone = ::new (pvMemory) bhkCachingShapePhantom;
	}
	CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

// Adds the phantom's own RTTI title to the array, in the Havok context (thiscall, ret 4, 0xCC2950). The exe inlines
// the array's grow check and then calls SetSize/SetAt out of line.
void bhkCachingShapePhantom::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkShapePhantom::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
	uint32_t uiSize = apStrings->m_usSize;
	uint32_t uiMaxSize = apStrings->m_usMaxSize;
	if (uiSize >= uiMaxSize) {
		apStrings->SetSize(uiSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(uiSize, pcString);
}

// The class's factory: allocates the 0x18-byte phantom and constructs it in place.
NiObject* bhkCachingShapePhantom::CreateObject() {
	void* pvMemory = NiMemObject::operator new(sizeof(bhkCachingShapePhantom));
	if (pvMemory) {
		return ::new (pvMemory) bhkCachingShapePhantom;
	}
	return nullptr;
}

// Casts the ray into each cached collidable's space and against its shape, from the last detail to the first,
// skipping collidables the game's filter keeps apart from the ray; a hit nearer than the best so far gives arOutput
// its normal, hit fraction and root collidable (the rest of arOutput is left alone). The best normal is turned back
// into world space at the end.
void hkpCachingShapePhantomRay::castRay(const hkpWorldRayCastInput& arInput, hkpWorldRayCastOutput& arOutput) {
	HK_TIMER_BEGIN("rcCachingPhantom", nullptr);
	FNVD_INLINED_LOCAL(hkpShapeRayCastInput, sinput, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(hkpShapeRayCastOutput, sout, std::nullopt, std::nullopt);
	// EXE-INLINED 0x4A3D20
	sout.m_hitFraction = 1.0f;
	sout.m_extraInfo = -1;
	sout.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
	sout.m_shapeKeyIndex = 0;
	// EXE-INLINED 0x632CA0
	sinput.m_rayShapeCollectionFilter = nullptr;
	sinput.m_filterInfo = arInput.m_filterInfo;
	hkpCollisionFilter* pFilter;
	if (arInput.m_enableShapeCollectionFilter.m_bool && (pFilter = m_world->m_collisionFilter) != nullptr) {
		sinput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(pFilter);
	} else {
		sinput.m_rayShapeCollectionFilter = nullptr;
	}
	bool bHit = false;
	for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
		hkpCollidable* pCollidable = m_collisionDetails.m_data[i].m_collidable;
		const hkpShape* pShape = pCollidable->m_shape;
		if (!pShape) {
			continue;
		}
		if (!bhkCollisionFilter::IsCollisionEnabled(CFilter{arInput.m_filterInfo},
				CFilter{pCollidable->m_broadPhaseHandle.m_collisionFilterInfo})) {
			continue;
		}
		const hkTransform& kTransform = *pCollidable->m_motion.pMotion;
		const __m128 lo = _mm_unpacklo_ps(kTransform.m_rotation.m_col0.m_quad, kTransform.m_rotation.m_col1.m_quad);
		const __m128 hi = _mm_unpackhi_ps(kTransform.m_rotation.m_col0.m_quad, kTransform.m_rotation.m_col1.m_quad);
		const __m128 row0 = _mm_movelh_ps(lo, kTransform.m_rotation.m_col2.m_quad);
		const __m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), kTransform.m_rotation.m_col2.m_quad, 0xD4);
		const __m128 row2 = _mm_shuffle_ps(hi, kTransform.m_rotation.m_col2.m_quad, 0xE4);
		// EXE-INLINED 0xC7F690: hkVector4::setTransformedInversePos
		const __m128 from = _mm_sub_ps(arInput.m_from.m_quad, kTransform.m_translation.m_quad);
		sinput.m_from.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(from, from, 0x55), row1),
											  _mm_mul_ps(_mm_shuffle_ps(from, from, 0x00), row0)),
			_mm_mul_ps(row2, _mm_shuffle_ps(from, from, 0xAA)));
		// EXE-INLINED 0xC7F690: hkVector4::setTransformedInversePos
		const __m128 to = _mm_sub_ps(arInput.m_to.m_quad, kTransform.m_translation.m_quad);
		sinput.m_to.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(to, to, 0x55), row1),
											_mm_mul_ps(_mm_shuffle_ps(to, to, 0x00), row0)),
			_mm_mul_ps(row2, _mm_shuffle_ps(to, to, 0xAA)));
		// EXE-INLINED 0x4A3D80
		sout.m_hitFraction = 1.0f;
		sout.m_extraInfo = -1;
		sout.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
		sout.m_shapeKeyIndex = 0;
		if (pShape->castRay(sinput, sout).m_bool) {
			// The exe tests the fcomp of the best fraction against this one with ah & 0x41: only a strictly
			// greater best fraction (ordered) is replaced.
			if (!bHit || arOutput.m_hitFraction > sout.m_hitFraction) {
				arOutput.m_hitFraction = sout.m_hitFraction;
				bHit = true;
				arOutput.m_normal.m_quad = sout.m_normal.m_quad;
				arOutput.m_rootCollidable = pCollidable;
			}
		}
	}
	if (arOutput.m_rootCollidable) {
		arOutput.m_normal.setRotatedDir(arOutput.m_rootCollidable->m_motion.pMotion->m_rotation, arOutput.m_normal);
	}
	HK_TIMER_END();
}
