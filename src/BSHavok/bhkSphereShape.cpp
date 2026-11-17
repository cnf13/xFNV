#include "BSHavok/bhkSphereShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSTestObjects.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "SDK/MSVC/stdlib.h"

namespace bhkConvert {} // namespace bhkConvert

static uint32_t dbgGeomNameSC_guard;

// The guard of the PDB's bhkSphereShape::BuildDisplayGeometry::dbgGeomNameSC, the debug geometry's name, is
// dbgGeomNameSC_guard (bit 0).
NiRTTI bhkSphereShape::ms_RTTI("bhkSphereShape", &bhkConvexShape::ms_RTTI);

bhkSphereShape::bhkSphereShape() {
	++iCount;
}

// The destructor sequence the exe keeps out of line in Destruct (0xCA0660).
bhkSphereShape::~bhkSphereShape() {
	bhkSerializable::Kill();
	--iCount;
}

const NiRTTI* bhkSphereShape::GetRTTI() const {
	return &bhkSphereShape::ms_RTTI;
}

// Clones the shape, charging the allocation to the Havok heap context and holding the engine's clone critical
// section across it. The exe inlines the context switch (the thread block's +0x2B4 gets 0x10) around the same calls;
// its cleanup-only EH frame is left out.
NiObject* bhkSphereShape::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkSphereShape* pClone = nullptr;
	bhkSerializable::CloneCritLock();
	bhkSphereShape* pObject = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
	if (pObject) {
		pClone = ::new (pObject) bhkSphereShape;
	}
	// The exe calls CopyMembers through the vtable (slot 49); a base pointer keeps the call virtual.
	bhkShape* pThis = this;
	pThis->CopyMembers(pClone, arCloneData);
	bhkSerializable::CloneCritUnlock();
	return pClone;
}

// The convex shape's own viewer lines, then the shape's RTTI heading appended to the panel's array. The exe reads the
// array's 16-bit counts and calls its out-of-line SetSize/SetAt; its cleanup-only EH frame is left out.
void bhkSphereShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkConvexShape::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
	apStrings->Add(pcString);
}

// The size of the shape's creation info, hkSphereShapeCinfo.
uint32_t bhkSphereShape::QCinfoSize() const {
	return 8;
}

// Builds the Havok sphere from the creation info's radius on the thread's Havok heap and hands it to the wrapper,
// which takes its own reference. The exe's cleanup-only EH frame is left out.
void bhkSphereShape::Init(const hkSerializableCinfo& arInfo) {
	const hkSphereShapeCinfo& rInfo = static_cast<const hkSphereShapeCinfo&>(arInfo);
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
	hkpSphereShape* pShape = static_cast<hkpSphereShape*>(kHeap.blockAlloc(sizeof(hkpSphereShape)));
	// The exe writes the allocated size into the block before the constructor runs (a 2-byte store).
	pShape->m_memSizeAndFlags = sizeof(hkpSphereShape);
	pShape = ::new (pShape) hkpSphereShape(rInfo.m_radius);
	// The exe dispatches both through the vtable (+0x8C and +0xBC), so call them through a base reference.
	bhkSerializable& rThis = *this;
	rThis.SethkObject(pShape);
	pShape->removeReference();
	rThis.PostInit(arInfo);
}

// The exe's cleanup-only C++ EH frame is left out.
hkSerializableCinfo& bhkSphereShape::CreateCinfo(bool& arCreated) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	bool v5 = pInfo == 0;
	arCreated = v5;
	if (v5) {
		hkConvexShapeCinfo* v6 =
			static_cast<hkConvexShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkConvexShapeCinfo)));
		if (v6) {
			v6->m_userData = 0;
			v6->m_radius = 0.1f;
		} else {
			v6 = 0;
		}
		bool v7 = phkObject == 0;
		pInfo = v6;
		if (!v7) {
			CopyCinfo(*v6);
		}
	}
	hkSerializableCinfo* result = pInfo;
	v3->eMemContext = eMemContext;
	return *result;
}

// The creation info's radius scaled by the cloning pass's scale (its x: the pass is always a BSCloningProcess), then
// the shape's members. The exe calls CreateCinfo through the vtable (+0xB4) and does not kill the info it may make.
void bhkSphereShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
	bool bCreated;
	bhkSerializable* pThis = this;
	hkConvexShapeCinfo& rInfo = static_cast<hkConvexShapeCinfo&>(pThis->CreateCinfo(bCreated));
	float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
	rInfo.m_radius = fScale * rInfo.m_radius;
	bhkShape::CopyMembers(apDest, arCloneData);
}

// A debug sphere (10 slices, 10 stacks, no colour) of the Havok sphere's radius in game units, named "bhkSphereShape"
// and attached to apNode; the colour is not used. Without a Havok shape the radius is the default convex radius, 0.1
// (the float at 0x10C72BC). The exe rounds the radius and the scaled radius to float in its frame. The exe's
// cleanup-only C++ EH frame is left out.
void bhkSphereShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) {
	// EXE-INLINED none: bhkSphereShape::Gethk
	float fradius = (this && phkObject) ? static_cast<hkpSphereShape*>(phkObject)->m_radius : 0.1f;
	float fScaledRadius = fradius * bhkConvert::fHk2BSScaleSC;
	NiAVObject* pav = BSTestObjects::CreateSphere(fScaledRadius, 10, 10, nullptr);
	if (pav) {
		// The PDB's function static dbgGeomNameSC; the exe inlines its NiFixedString(const char*) constructor. The
		// hybrid binds the name to the exe's object at its address, untyped memory there, as FNVD_FUNCTION_STATIC does.
		static NiFixedString dbgGeomNameSC("bhkSphereShape");
		pav->SetName(dbgGeomNameSC);
		apNode->AttachChild(pav, 1);
	}
}

// Builds a sphere of radius afRadius: the creation info of a sphere carries the radius as its only field, and the
// shape is then initialised from it. The exe builds the info on the stack (0x56EA40), stores the radius at its +4
// and calls the virtual Init through slot 44 (+0xB0).
void bhkSphereShape::Init(float afRadius) {
	hkSphereShapeCinfo kCinfo;
	kCinfo.~hkSphereShapeCinfo();
	::new (&kCinfo) hkSphereShapeCinfo;
	kCinfo.m_radius = afRadius;
	bhkSerializable* pThis = this;
	pThis->Init(kCinfo);
}

// The engine's factory for the class: a bare shape that has no creation info yet (cdecl, ret 0, 0xCA06D0). The exe's
// cleanup-only EH frame around the allocation is left out.
NiObject* bhkSphereShape::CreateObject() {
	bhkSphereShape* pObject = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
	if (pObject)
		return ::new (pObject) bhkSphereShape;
	return nullptr;
}

hkSphereShapeCinfo::hkSphereShapeCinfo() {}
