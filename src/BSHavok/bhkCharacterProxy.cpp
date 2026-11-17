#include "BSHavok/bhkCharacterProxy.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/ahkpCharacterProxy.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterPointCollector.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkCharacterProxy::ms_RTTI("bhkCharacterProxy", &bhkSerializable::ms_RTTI);

// The exe's cleanup-only C++ EH frame is left out.
bhkCharacterProxy::bhkCharacterProxy() : m_ignoredCollisionStartCollector(nullptr) {
	++bhkCharacterProxy::iNumObjects;
}

// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
bhkCharacterProxy::~bhkCharacterProxy() {
	bhkCharacterProxy::GethkWorld();
	bhkSerializable::Kill();
	--bhkCharacterProxy::iNumObjects;
}

// Clones the wrapper with the clone critical section held and the thread's memory context on Havok, as the exe does;
// the exe's cleanup-only EH frame is left out.
NiObject* bhkCharacterProxy::CreateClone(NiCloningProcess& arCloneData) {
	int32_t& iMemContext = GetThreadMemContext();
	int32_t iSavedMemContext = iMemContext;
	iMemContext = MEM_CONTEXT_HAVOK;
	bhkCharacterProxy* pClone = nullptr;
	bhkSerializable::CloneCritLock();
	void* pMemory = NiMemObject::operator new(sizeof(bhkCharacterProxy));
	if (pMemory)
		pClone = ::new (pMemory) bhkCharacterProxy;
	bhkSerializable::CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	iMemContext = iSavedMemContext;
	return pClone;
}

// The cinfo CreateCinfo hands back is the proxy's bhkCharacterProxyCinfo; its phantom is linked in LinkObject.
void bhkCharacterProxy::LoadBinary(NiStream& arStream) {
	arStream.ReadLinkID();
	bhkSerializable::LoadBinary(arStream);
	bool bCreated;
	static_cast<bhkCharacterProxyCinfo&>(CreateCinfo(bCreated)).m_shapePhantom = nullptr;
}

// Links the shape phantom's wrapper read in LoadBinary: its Havok phantom goes into the cinfo.
void bhkCharacterProxy::LinkObject(NiStream& arStream) {
	bhkShapePhantom* pShape = static_cast<bhkShapePhantom*>(arStream.GetObjectFromLinkID());
	bool bCreated;
	bhkCharacterProxyCinfo& rCinfo = static_cast<bhkCharacterProxyCinfo&>(CreateCinfo(bCreated));
	if (pShape) {
		rCinfo.m_shapePhantom = static_cast<hkpShapePhantom*>(pShape->phkObject);
	}
	bhkSerializable::LinkObject(arStream);
}

bool bhkCharacterProxy::RegisterStreamables(NiStream& arStream) {
	ahkpCharacterProxy* phkObject = static_cast<ahkpCharacterProxy*>(this->phkObject);
	if (phkObject) {
		hkpShapePhantom* ShapePhantom = phkObject->GetShapePhantom();
		if (ShapePhantom) {
			bhkShapePhantom* ps = static_cast<bhkShapePhantom*>(ShapePhantom->m_userData);
			if (ps) {
				ps->RegisterStreamables(arStream);
			}
		}
	}
	return bhkSerializable::RegisterStreamables(arStream);
}

// The game object the wrapped proxy's shape phantom belongs to goes into the stream's link table first, then the
// Havok object itself is saved by the base. The exe calls the phantom getter out of line.
void bhkCharacterProxy::SaveBinary(NiStream& arStream) {
	const NiObject* pUserData = nullptr;
	if (phkObject) {
		hkpShapePhantom* pPhantom;
		pPhantom = static_cast<ahkpCharacterProxy*>(phkObject)->GetShapePhantom();
		if (pPhantom)
			pUserData = pPhantom->m_userData;
	}
	arStream.SaveLinkID(pUserData);
	bhkSerializable::SaveBinary(arStream);
}

bool bhkCharacterProxy::IsEqual(NiObject* apObject) {
	return bhkSerializable::IsEqual(apObject);
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkCharacterProxy::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiPoint3 pnt;
	TLSData* v18 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v18->eMemContext;
	v18->eMemContext = MEM_CONTEXT_HAVOK;
	bhkSerializable::GetViewerStrings(apStrings);
	char* ViewerString = NiGetViewerString(bhkCharacterProxy::ms_RTTI.GetName());
	uint32_t m_usSize = apStrings->m_usSize;
	uint32_t m_usMaxSize = apStrings->m_usMaxSize;
	char* v21 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(m_usSize, v21);
	hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(this->phkObject);
	const hkVector4* Position = &hkVector4Zero;
	if (phkObject) {
		GethkWorld();
		Position = &phkObject->getPosition();
		GethkWorld();
	}
	HK2NI(pnt, *Position);
	char* v9 = pnt.GetViewerString(rdata_10C8400);
	uint32_t v10 = apStrings->m_usSize;
	uint32_t v11 = apStrings->m_usMaxSize;
	v21 = v9;
	if (v10 >= v11) {
		apStrings->SetSize(v10 + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(v10, v21);
	hkReferencedObject* v12 = this->phkObject;
	if (v12) {
		GethkWorld();
		const hkVector4* LinearVelocity = &static_cast<hkpCharacterProxy*>(v12)->getLinearVelocity();
		HK2NI(pnt, *LinearVelocity);
		GethkWorld();
	}
	char* v14 = pnt.GetViewerString(rdata_10C83FC);
	uint32_t v15 = apStrings->m_usSize;
	uint32_t v16 = apStrings->m_usMaxSize;
	v21 = v14;
	if (v15 >= v16) {
		apStrings->SetSize(v15 + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(v15, v21);
	v18->eMemContext = eMemContext;
}

// The Havok proxy's user data holds this while it is attached; attaching also takes a reference on the shape
// phantom's wrapper, detaching runs RembhkRefs.
void bhkCharacterProxy::Link(bool abAttach) {
	ahkpCharacterProxy* pProxy = this ? static_cast<ahkpCharacterProxy*>(phkObject) : nullptr;
	if (abAttach) {
		ahkpCharacterProxy* pAttached = static_cast<ahkpCharacterProxy*>(phkObject);
		if (pAttached) {
			hkpShapePhantom* pPhantom = pAttached->GetShapePhantom();
			if (pPhantom) {
				bhkWorldObject* pWrapper = pPhantom->m_userData;
				if (pWrapper) {
					pWrapper->IncRefCount();
				}
			}
		}
	} else {
		RembhkRefs();
	}
	if (pProxy) {
		pProxy->m_userData = abAttach ? this : nullptr;
	}
	bhkRefObject::Link(abAttach);
}

// The exe reads the phantom through ahkpCharacterProxy::GetShapePhantom (0xCAC980), called out of line.
const hkpWorld* bhkCharacterProxy::GethkWorld() const {
	auto* pProxy = static_cast<ahkpCharacterProxy*>(phkObject);
	if (!pProxy)
		return nullptr;
	hkpPhantom* pPhantom;
	pPhantom = pProxy->GetShapePhantom();
	if (!pPhantom)
		return nullptr;
	return pPhantom->m_world;
}

hkpWorld* bhkCharacterProxy::GethkWorldRw() {
	return const_cast<hkpWorld*>(GethkWorld());
}

// Returns the creation info the proxy made to the thread's Havok heap, and forgets it. The exe's cleanup-only EH frame
// is left out.
void bhkCharacterProxy::KillCinfo(bool abCreated) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	if (abCreated) {
		hkSerializableCinfo* pCinfo = pInfo;
		if (pCinfo) {
			hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
			kHeap.blockFree(pCinfo, static_cast<int32_t>(sizeof(bhkCharacterProxyCinfo)));
		}
		pInfo = nullptr;
	}
	GetThreadMemContext() = iOldMemContext;
}

// Builds the proxy in the thread's Havok memory context: the block comes from the thread's heap allocator with its
// size recorded in the reference object header, then the proxy is constructed from arInfo and handed to the wrapper,
// which drops the reference the construction left. Error id 0x6CEE9071 is disabled while the proxy is built and
// enabled again afterwards. The exe's cleanup-only EH frame is left out.
void bhkCharacterProxy::Init(const hkSerializableCinfo& arInfo) {
	int32_t& iMemContext = GetThreadMemContext();
	int32_t iSavedMemContext = iMemContext;
	iMemContext = MEM_CONTEXT_HAVOK;
	hkError* pError = hkError::s_instance;
	hkBool bEnabled;
	bEnabled.m_bool = 0;
	pError->setEnabled(0x6CEE9071, bEnabled);
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	void* pMemory = heap.blockAlloc(sizeof(ahkpCharacterProxy));
	static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(ahkpCharacterProxy);
	ahkpCharacterProxy* pProxy = ::new (pMemory) ahkpCharacterProxy(static_cast<const bhkCharacterProxyCinfo&>(arInfo));
	bEnabled.m_bool = 1;
	pError->setEnabled(0x6CEE9071, bEnabled);
	SethkObject(pProxy);
	pProxy->removeReference();
	PostInit(arInfo);
	iMemContext = iSavedMemContext;
}

// Makes the proxy's creation info on the thread's Havok heap when the wrapper has none yet, and has the proxy fill it
// in. The exe's cleanup-only EH frame is left out.
hkSerializableCinfo& bhkCharacterProxy::CreateCinfo(bool& arCreated) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	bool bCreated = (pInfo == nullptr);
	arCreated = bCreated;
	if (bCreated) {
		hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
		bhkCharacterProxyCinfo* pCinfo = static_cast<bhkCharacterProxyCinfo*>(kHeap
				.blockAlloc(static_cast<int32_t>(sizeof(bhkCharacterProxyCinfo))));
		if (pCinfo) {
			// The exe calls the cinfo's constructor out of line on the block; the class's own constructor is inlined
			// into that call.
			::new (static_cast<hkpCharacterProxyCinfo*>(pCinfo)) hkpCharacterProxyCinfo;
		}
		hkReferencedObject* pProxy = phkObject;
		pInfo = pCinfo;
		if (pProxy) {
			static_cast<hkpCharacterProxy*>(pProxy)->getCinfo(*pCinfo);
		}
	}
	hkSerializableCinfo* pResult = pInfo;
	GetThreadMemContext() = iOldMemContext;
	return *pResult;
}

hkpShapePhantom* bhkCharacterProxy::GethkShapePhantom() {
	ahkpCharacterProxy* v3 = static_cast<ahkpCharacterProxy*>(bhkRefObject::GethkObject());
	if (v3) {
		return v3->GetShapePhantom();
	} else {
		return nullptr;
	}
}

// The exe's cleanup-only C++ EH frame is left out. GethkShapePhantom (0x621A90) is inlined without its null-this test.
void bhkCharacterProxy::RembhkRefs() {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_HAVOK;
	// EXE-INLINED 0x621A90
	ahkpCharacterProxy* pProxy = static_cast<ahkpCharacterProxy*>(phkObject);
	if (pProxy) {
		hkpShapePhantom* pp = pProxy->GetShapePhantom();
		if (pp) {
			// EXE-INLINED none: bhkShapePhantom::Getbhk
			bhkShapePhantom* pshape = static_cast<bhkShapePhantom*>(pp->m_userData);
			if (pshape) {
				pshape->DecRefCount();
			}
		}
	}
	pTLS->eMemContext = eMemContext;
}

void bhkCharacterProxy::GetCinfo(hkSerializableCinfo* apCinfo) {
	if (this) {
		hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(this->phkObject);
		if (phkObject) {
			// The cinfo is a bhkCharacterProxyCinfo, whose hkpCharacterProxyCinfo base is at its start.
			phkObject->getCinfo(*static_cast<bhkCharacterProxyCinfo*>(apCinfo));
		}
	}
}

void bhkCharacterProxy::CleanUp() {
	GethkWorldRw();
	static_cast<ahkpCharacterProxy*>(phkObject)->RemoveEntityListeners();
	GethkWorldRw();
	(&m_ignoredCollisionStartCollector)->reset();
}

// The exe reads the proxy through the shared getter bhkRefObject::GethkObject (0x620B80).
const hkVector4& bhkCharacterProxy::GethkPosition() {
	auto* pProxy = static_cast<hkpCharacterProxy*>(GethkObject());
	const hkVector4* pvPosition = &hkVector4::getZero();
	if (pProxy) {
		bhkCharacterProxy::GethkWorldRw();
		pvPosition = const_cast<hkVector4*>(&pProxy->getPosition());
		bhkCharacterProxy::GethkWorldRw();
	}
	return *pvPosition;
}

// The exe reads the proxy through the shared getter bhkRefObject::GethkObject (0x620B80).
const hkVector4& bhkCharacterProxy::GethkLinearVelocity() {
	auto* pProxy = static_cast<hkpCharacterProxy*>(GethkObject());
	const hkVector4* pvVelocity = &hkVector4::getZero();
	if (pProxy) {
		bhkCharacterProxy::GethkWorldRw();
		pvVelocity = &pProxy->getLinearVelocity();
		bhkCharacterProxy::GethkWorldRw();
	}
	return *pvVelocity;
}

hkpCharacterProxy* bhkCharacterProxy::operator*() {
	if (this) {
		return static_cast<hkpCharacterProxy*>(GethkObject());
	} else {
		return nullptr;
	}
}

// The exe reads the proxy through the shared null-safe getter 0x4AE750.
void bhkCharacterProxy::SetLinearVelocity(const hkVector4& arVel) {
	hkpCharacterProxy* pProxy = **this;
	if (pProxy)
		pProxy->setVelocity(&arVel);
}

// Built on the base's constructor; the up vector is +Z.
ahkpCharacterProxy::ahkpCharacterProxy(const bhkCharacterProxyCinfo& arInfo)
	: hkpCharacterProxy(arInfo), m_userData(nullptr) {
	m_up.set(0.0f, 0.0f, 1.0f, 0.0f);
}

hkArray<hkpRootCdPoint>* ahkpCharacterProxy::GetCdPoints() {
	return &m_manifold;
}

hkpShapePhantom* ahkpCharacterProxy::GetShapePhantom() {
	return m_shapePhantom;
}

// The exe's cleanup-only C++ EH frame is left out.
void ahkpCharacterProxy::RemoveEntityListeners() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_HAVOK;
	int32_t v3 = 0;
	if (m_bodies.m_size > 0) {
		do {
			m_bodies.m_data[v3++]->removeEntityListener(this);
		} while (v3 < m_bodies.m_size);
		v3 = 0;
	}
	bool v4 = m_phantoms.m_size <= 0;
	m_bodies.m_size = 0;
	if (v4) {
		m_phantoms.m_size = 0;
		v2->eMemContext = eMemContext;
	} else {
		do {
			m_phantoms.m_data[v3++]->removePhantomListener(this);
		} while (v3 < m_phantoms.m_size);
		m_phantoms.m_size = 0;
		v2->eMemContext = eMemContext;
	}
}
