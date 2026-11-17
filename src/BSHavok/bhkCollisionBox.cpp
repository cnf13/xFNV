#include "BSHavok/bhkCollisionBox.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCachingShapePhantom.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkCollisionBox::ms_RTTI("bhkCollisionBox", &bhkSimpleShapePhantom::ms_RTTI);

// The exe's cleanup-only C++ EH frame is left out.
bhkCollisionBox::bhkCollisionBox(hkSerializableCinfo* a1, bhkCharacterController* a2) {
	bhkCollisionBox::Init(*a1, a2);
}

// The exe's own free callee for the box is MemoryManager::Deallocate on the aligned block: the padding byte in
// front of the object gives the distance back to the block's start.
bhkCollisionBox::~bhkCollisionBox() {}

const NiRTTI* bhkCollisionBox::GetRTTI() const {
	return &bhkCollisionBox::ms_RTTI;
}

// Adds the box's viewer lines to apStrings: the base phantom's lines, then the class heading. The exe's EH frame
// (scope table 0xf30788) is cleanup-only and left out.
void bhkCollisionBox::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	// The exe inlines the heap context's save and restore and reads the thread's TLS block once, before the base call.
	int32_t& iMemContext = GetThreadMemContext();
	int32_t iSavedMemContext = iMemContext;
	iMemContext = MEM_CONTEXT_HAVOK;

	// The exe calls the base body directly, not through the vtable.
	bhkSimpleShapePhantom::GetViewerStrings(apStrings);

	char* pcString = NiGetViewerString(bhkCollisionBox::ms_RTTI.m_pcName);
	uint16_t usSize = apStrings->m_usSize;
	uint16_t usMaxSize = apStrings->m_usMaxSize;
	if (usSize >= usMaxSize) {
		apStrings->SetSize(usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(usSize, pcString);

	iMemContext = iSavedMemContext;
}

bool bhkCollisionBox::Add(bhkWorld* apWorld) {
	bool result = 0;
	if (!bDisabled) {
		bool v4 = bhkWorldObject::Add(apWorld);
		if (v4) {
			uint32_t v5 = hkColor::rgbFromChars(30u, 130u, 0, 20u);
			apWorld->SetDebugObjectColor(this, v5);
		}
		return v4;
	}
	return result;
}

// The box takes the scene graph object of the controller's shape phantom, or none when the controller has no
// phantom.
void bhkCollisionBox::Init(const hkSerializableCinfo& arInfo, bhkCharacterController* apCharController) {
	bDisabled = false;
	bhkSimpleShapePhantom::Init(arInfo);
	pCharController = apCharController;
	if (apCharController) {
		bhkCachingShapePhantom* pPhantom = apCharController->spShapePhantom;
		if (pPhantom) {
			SetAVObject(pPhantom->GetAVObject(0), 0);
		} else {
			SetAVObject(nullptr, 0);
		}
	}
}

// Moves the box to its controller: to the centre of the controller phantom's scene graph bound, with the bound's
// radius, or without one to the controller's Havok position scaled to game units, with radius 1. The phantom's
// convex shape takes the radius (1 when not positive) scaled to Havok units.
void bhkCollisionBox::Update() {
	if (bhkCollisionBox::bNoCollision) {
		return;
	}
	bhkCharacterController* pController = pCharController;
	if (!pController) {
		return;
	}
	// The exe passes this slot's address to SetPosition; it sits 16-aligned in the realigned frame.
	alignas(16) NiPoint3 kPosition;
	float fRadius;
	NiAVObject* pObject;
	bhkCachingShapePhantom* pPhantom = pController->spShapePhantom;
	if (pPhantom && (pObject = pPhantom->GetAVObject(0)) != nullptr) {
		const NiBound* pBound = pObject->m_kWorldBound;
		if (!pBound) {
			pBound = &NiAVObject::NullBoundS;
		}
		kPosition.x = pBound->m_kCenter.x;
		kPosition.y = pBound->m_kCenter.y;
		kPosition.z = pBound->m_kCenter.z;
		fRadius = pBound->m_fRadius;
	} else {
		hkVector4 kHavokPosition = pCharController->GethkPosition();
		kPosition.x = kHavokPosition(0) * bhkConvert::fHk2BSScaleSC;
		kPosition.y = kHavokPosition(1) * bhkConvert::fHk2BSScaleSC;
		kPosition.z = bhkConvert::fHk2BSScaleSC * kHavokPosition(2);
		fRadius = 1.0f;
	}
	// The exe's MarkRead and UnMarkRead around the shape read are the empty function 0xA29680.
	hkpWorldObject* pWorldObject = static_cast<hkpWorldObject*>(phkObject);
	const hkpShape* pShape;
	hkpLinkedCollidable* pCollidable;
	if (pWorldObject && (pCollidable = &pWorldObject->m_collidable) != nullptr) {
		pShape = pCollidable->m_shape;
	} else {
		pShape = nullptr;
	}
	if (!(fRadius > 0.0f)) {
		fRadius = 1.0f;
	}
	// The phantom's shape is convex: the exe stores the radius at hkpConvexShape::m_radius.
	hkpConvexShape* pConvexShape = const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pShape));
	// EXE-INLINED none: hkpConvexShape::setRadius
	pConvexShape->m_radius = bhkConvert::fBS2HkScaleSC * fRadius;
	SetPosition(kPosition);
}

void bhkCollisionBox::operator delete(void* apMemory) {
	uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
	MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
}
