#include "Fallout Shared/Havok/CameraCaster.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> fCameraCasterFadeSittingRadius("fCameraCasterFadeSittingRadius:HAVOK", 45.0f);

CameraCaster::CameraCaster(float afRadius, uint32_t aiGroup)
	: spPlayerShapePhantom(nullptr), spHorseShapePhantom(nullptr) {
	Init(afRadius, aiGroup);
}

CameraCaster::~CameraCaster() {}

void CameraCaster::Add(bhkWorld* apWorld) {
	bhkSimpleShapePhantom* v2;
	bhkSimpleShapePhantom* v3;
	bhkSimpleShapePhantom* v4;
	uint32_t v5;
	uint32_t v6;
	bhkSimpleShapePhantom* v7;
	if (GetPlayerShapePhantom()) {
		v7 = GetPlayerShapePhantom();
		v7->Add(apWorld);
		if (apWorld) {
			v5 = (uint32_t)hkColor::rgbFromChars(0xFFu, 0x2Eu, 0x2Eu, 0x19u);
			v2 = GetPlayerShapePhantom();
			apWorld->SetDebugObjectColor(v2, v5);
		}
	}
	if ((&spHorseShapePhantom)->m_pObject) {
		v3 = (&spHorseShapePhantom)->m_pObject;
		v3->Add(apWorld);
		if (apWorld) {
			v6 = (uint32_t)hkColor::rgbFromChars(0xFFu, 0x2Eu, 0x2Eu, 0x19u);
			v4 = (&spHorseShapePhantom)->m_pObject;
			apWorld->SetDebugObjectColor(v4, v6);
		}
	}
}

void CameraCaster::SetPlayerGroup(uint32_t aiGroup) {
	bhkSimpleShapePhantom* v2 = spPlayerShapePhantom;
	v2->bhkWorldObject::SetCollisionGroup(aiGroup);
}

// Casts the player phantom from arFrom to arTo and walks the sorted hits for the first that counts: ground always,
// otherwise a hit on visible 3D, except one on the VATS target during VATS playback (weapons and bipeds count there
// only) or on a mobile object whose character controller chases its biped. The hit goes to arPoint and its position
// to arTo.
// The exe's cleanup-only EH frame (destroying the collector on unwind) is left out.
bool CameraCaster::Pick(NiPoint3& arFrom, NiPoint3& arTo, hkpRootCdPoint& arPoint) {
	bool bResult = false;
	if (!GetPlayerShapePhantom())
		return bResult;
	alignas(16) hkVector4 kStart;
	// The exe's empty hkVector4 constructor (0x6815C0), the same three-byte body as hkVector4::getQuad.
	kStart.getQuad();
	// Raw storage: the collector is built and destroyed through the exe's constructor and destructor.
	alignas(16) uint8_t aCollector[sizeof(hkpAllCdPointCollector)];
	hkpAllCdPointCollector* pCollector = ::new (aCollector) hkpAllCdPointCollector;
	alignas(16) hkpLinearCastInput kInput;
	kInput.~hkpLinearCastInput();
	::new (&kInput) hkpLinearCastInput;
	NI2HK(kStart, arFrom);
	NI2HK(kInput.m_to, arTo);
	GetPlayerShapePhantom()->SetPositionAndLinearCast(kStart, kInput, *pCollector, nullptr);
	hkBool kHit;
	bResult = *pCollector->hasHit(&kHit);
	if (bResult) {
		pCollector->sortHits();
		int32_t iIndex = 0;
		bool bFound = false;
		bool bVATSPlayback = pVATS.GetMode() == VATS::VATS_PLAYBACK;
		TESObjectREFR* pTarget = nullptr;
		if (bVATSPlayback) {
			VATS_COMMAND* pAction = pVATS.GetCurrentAction();
			if (pAction)
				pTarget = pAction->pTarget;
			else
				pTarget = pVATSTarget;
			if (!pTarget)
				pTarget = pPlayer;
		}
		while (!bFound) {
			if (iIndex >= pCollector->getHits().getSize())
				break;
			const hkpCollidable* pCollidable = pCollector->getHits()[iIndex].m_rootCollidableB;
			CFilter kFilter;
			COL_LAYER eLayer = FNVD_RECONSTRUCT(CFilter)(&kFilter, pCollidable->getCollisionFilterInfo())->GetLayer();
			NiAVObject* pObject = bhkUtilFunctions::GetNiAVObject(const_cast<hkpCollidable*>(pCollidable));
			if (eLayer != L_GROUND && (!pObject || pObject->GetAppCulled())) {
				++iIndex;
				continue;
			}
			bFound = true;
			switch (eLayer) {
			case L_WEAPON:
			case L_BIPED:
			case L_DEADBIP:
				if (!bVATSPlayback)
					break;
				[[fallthrough]];
			case L_DEBRIS_SMALL:
			case L_DEBRIS_LARGE:
			case L_SHELLCASING:
			case L_CHARCONTROLLER:
			case L_COLLISIONBOX:
				if (pObject) {
					TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
					if (pRef) {
						if (bVATSPlayback && pRef == pTarget) {
							bFound = false;
						} else if (pRef->IsMobileObject()) {
							bhkCharacterController* pController =
								static_cast<bhkCharacterController*>(static_cast<MobileObject*>(pRef)
										->GetCharController());
							if (pController && pController->QChaseBip())
								bFound = false;
						}
					}
				}
				break;
			default:
				break;
			}
			if (!bFound) {
				++iIndex;
			} else if (bVATSPlayback) {
				// The exe reads the hit's collidable into a local it never uses.
				[[maybe_unused]] const hkpCollidable* pUnused = pCollector->getHits()[iIndex].m_rootCollidableB;
			}
		}
		if (bFound) {
			arPoint.Assign(pCollector->getHits()[iIndex]);
			// The exe's empty hkVector4 constructor (0x6815C0) on the point's position.
			arPoint.m_contact.m_position.getQuad();
			HK2NI(arTo, arPoint.m_contact.m_position);
		} else {
			bResult = false;
		}
	}
	pCollector->~hkpAllCdPointCollector();
	return bResult;
}

void CameraCaster::Remove() {
	Add(nullptr);
}

bhkWorld* CameraCaster::GetWorld() {
	bhkWorld* pWorld = nullptr;
	if (spPlayerShapePhantom.m_pObject) {
		pWorld = bhkWorld::Getbhk(spPlayerShapePhantom.m_pObject->bhkWorldObject::GethkWorld());
	}
	return pWorld;
}

// The exe's cleanup-only C++ EH frame is left out. The exe's empty hkVector4 constructor (0x6815C0) on cameraPos
// and capsulePos is a trivial accessor and not written.
bool CameraCaster::KeepOutOfCharacter(bhkCharacterController* apController, NiPoint3& arPoint, float afRadius,
	float afScaleOffset) {
	hkBool kHit;
	hkpCollisionInput input;
	alignas(16) hkVector4 capsulePos;
	alignas(16) hkVector4 cameraPos;
	bool bres = false;
	if (apController->QChaseBip() && pPlayer && pPlayer->pCurrentProcess && pPlayer->pCurrentProcess->GetHeadNode()) {
		NiAVObject* pHead = pPlayer->pCurrentProcess->GetHeadNode();
		NiPoint3 delta = arPoint - pHead->GetWorldTranslate();
		double fLength = delta.Length();
		if (fCameraCasterFadeSittingRadius.Float() > fLength) {
			bres = true;
		}
		return bres;
	}
	NI2HK(cameraPos, arPoint);
	const hkVector4* pCapsule = &apController->GethkPosition();
	capsulePos.operator=(*pCapsule);
	bhkSimpleShapePhantom* pphantom = nullptr;
	if (apController == pPlayer->GetCharController()) {
		pphantom = spPlayerShapePhantom.m_pObject;
		double fCameraZ = cameraPos.getSimdAt(2).operator float();
		if (capsulePos.getSimdAt(2).operator float() < fCameraZ) {
			float* pCameraZ = &cameraPos.operator()(2);
			*pCameraZ = *pCameraZ - NI2HK(afScaleOffset);
			float* pCapsuleZ = &capsulePos.operator()(2);
			*pCapsuleZ = *pCapsuleZ - NI2HK(afScaleOffset);
		}
	} else {
		pphantom = spHorseShapePhantom.m_pObject;
	}
	input.~hkpCollisionInput();
	::new (&input) hkpCollisionInput;
	FNVD_CONSTRUCTED_LOCAL(hkpAllCdPointCollector, collector, &hkpAllCdPointCollector::Destruct);
	input.setTolerance(0.0f);
	const hkpCollidable* pca = pphantom->GetCollidable();
	const hkpCollidable* pcb = &apController->GethkShapePhantom()->m_collidable;
	bhkWorld* pw = apController->GetWorld();
	if (pw) {
		pw->MarkForRead();
		hkpCollisionDispatcher* pdis = pw->operator*()->m_collisionDispatcher;
		pw->MarkForRead();
		if (pdis) {
			input.m_dispatcher = pdis;
			pw->MarkForRead();
			input.m_filter = pw->operator*()->m_collisionFilter;
			pw->MarkForRead();
			pphantom->SetPosition(cameraPos);
			hkpSphereShape* ps = static_cast<hkpSphereShape*>(pphantom->GethkShape());
			// The exe reads body B's shape type first (its argument is pushed first).
			hkpShapeType eTypeB = pcb->m_shape->getType();
			hkpShapeType eTypeA = pca->m_shape->getType();
			hkpCollisionDispatcher::GetClosestPointsFunc pfunc = pdis->getGetClosestPointsFunc(eTypeA, eTypeB);
			if (pfunc) {
				// The exe copies the radius through its float slot with fld/fstp.
				float foldRad = ps->m_radius;
				ps->setRadius(afRadius);
				pfunc(*pca, *pcb, input, collector);
				bres = collector.hasHit(&kHit)->operator bool();
				ps->setRadius(foldRad);
			}
		}
	}
	return bres;
}

// The exe's cleanup-only C++ EH frame is left out.
void CameraCaster::Init(float afRadius, uint32_t aiGroup) {
	bhkSphereShape* pplayerShape;
	bhkSphereShape* pMemory = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
	if (pMemory) {
		pplayerShape = ::new (pMemory) bhkSphereShape(afRadius, 1);
	} else {
		pplayerShape = nullptr;
	}
	// The exe constructs both with hkCachingShapePhantomCinfo's constructor (0x56E090), the same function.
	FNVD_CONSTRUCTED_LOCAL(hkSimpleShapePhantomCinfo, csi1, &hkSimpleShapePhantomCinfo::Destruct);
	FNVD_CONSTRUCTED_LOCAL(hkSimpleShapePhantomCinfo, csi2, &hkSimpleShapePhantomCinfo::Destruct);
	FNVD_CONSTRUCTED_LOCAL(CFilter, iplayerFilter, nullptr, 0u);
	FNVD_CONSTRUCTED_LOCAL(CFilter, ihorseFilter, nullptr, 0u);
	iplayerFilter.SetLayer(L_CAMERA);
	iplayerFilter.SetGroup(aiGroup);
	ihorseFilter.SetLayer(L_CAMERA);
	ihorseFilter.SetGroup(bhkCollisionFilter::GetNewSystemGroup());
	csi1.m_collisionFilterInfo = iplayerFilter.iFilter;
	csi1.m_shape = pplayerShape->operator*();
	csi1.m_transform.setIdentity();
	bhkSimpleShapePhantom* pPlayerPhantom;
	bhkSimpleShapePhantom* pPlayerMemory =
		static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
	if (pPlayerMemory) {
		pPlayerPhantom = ::new (pPlayerMemory) bhkSimpleShapePhantom(csi1);
	} else {
		pPlayerPhantom = nullptr;
	}
	spPlayerShapePhantom = pPlayerPhantom;
	csi2.m_collisionFilterInfo = ihorseFilter.iFilter;
	csi2.m_shape = pplayerShape->operator*();
	csi2.m_transform.setIdentity();
	bhkSimpleShapePhantom* pHorsePhantom;
	bhkSimpleShapePhantom* pHorseMemory =
		static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
	if (pHorseMemory) {
		pHorsePhantom = ::new (pHorseMemory) bhkSimpleShapePhantom(csi2);
	} else {
		pHorsePhantom = nullptr;
	}
	spHorseShapePhantom = pHorsePhantom;
}

void CameraCaster::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The Bethesda world of the controller's shape phantom's Havok world; the exe does not test the phantom.
bhkWorld* bhkCharacterController::GetWorld() {
	return bhkWorld::Getbhk(GethkShapePhantom()->getWorld());
}

void bhkShapePhantom::SetPosition(const hkVector4& arPos) {
	hkpShapePhantom* pPhantom = static_cast<hkpShapePhantom*>(**this);
	if (pPhantom) {
		MarkWrite();
		pPhantom->setPosition(arPos, 0.0f);
		UnMarkWrite();
	}
}

// The read mark is empty in the game's build: only the Havok world's lookup remains.
void bhkWorld::MarkForRead() {
	ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
	if (pw)
		pw->markForRead();
}

void bhkWorld::UnMarkForRead() {
	ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
	if (pw)
		pw->unmarkForRead();
}

// The Havok world's user data is the bhkWorld that wraps it (bhkWorld::Link sets it).
bhkWorld* bhkWorld::Getbhk(const hkpWorld* apWorld) {
	if (apWorld)
		return static_cast<bhkWorld*>(const_cast<ahkpWorld*>(static_cast<const ahkpWorld*>(apWorld))->getUserData());
	return nullptr;
}

void hkMemoryAllocator::bufFreeRayCastOutputs(void* p, int32_t numElem) {
	bufFree(p, sizeof(hkpWorldRayCastOutput) * numElem);
}

// No early out until the owner lowers the distance (reset()'s HK_REAL_MAX).
hkpCdPointCollector::hkpCdPointCollector() : m_earlyOutDistance(3.40282e38f) {}

hkpCdPointCollector::~hkpCdPointCollector() {}

// HK_DECLARE_CLASS_ALLOCATOR's operator delete: the block goes back to the thread's Havok heap.
void hkpCdPointCollector::operator delete(void* p) {
	if (p) {
		hkMemoryRouter::getInstance().heap().blockFree(p, sizeof(hkpCdPointCollector));
	}
}

// The three vectors' constructors are empty.
hkpCollisionInput::Aabb32Info::Aabb32Info() {}

// The exe's constructor writes nothing: only m_aabb32Info's (empty) constructor runs.
hkpCollisionInput::hkpCollisionInput() {}

hkpLinearCastInput::hkpLinearCastInput() : m_maxExtraPenetration(0x1p-23f), m_startPointTolerance(0x1p-23f) {}

void hkpAllCdPointCollector::reset() {
	m_hits.clear();
	hkpCdPointCollector::reset();
}

// HK_DECLARE_CLASS_ALLOCATOR's operator delete: the block goes back to the thread's Havok heap.
void hkpAllCdPointCollector::operator delete(void* p) {
	if (p) {
		hkMemoryRouter::getInstance().heap().blockFree(p, sizeof(hkpAllCdPointCollector));
	}
}

hkpRootCdPoint::hkpRootCdPoint() {}

hkpRootCdPoint::hkpRootCdPoint(const hkpRootCdPoint& __that) = default;

hkpRootCdPoint& hkpRootCdPoint::Assign(const hkpRootCdPoint& __that) {
	m_contact.operator=(__that.m_contact);
	m_rootCollidableA = __that.m_rootCollidableA;
	m_shapeKeyA = __that.m_shapeKeyA;
	for (uint32_t S1 = 0; S1 < 8; ++S1) {
		m_shapeKeysA[S1] = __that.m_shapeKeysA[S1];
	}
	m_rootCollidableB = __that.m_rootCollidableB;
	m_shapeKeyB = __that.m_shapeKeyB;
	for (uint32_t S2 = 0; S2 < 8; ++S2) {
		m_shapeKeysB[S2] = __that.m_shapeKeysB[S2];
	}
	return *this;
}

void hkpConvexShape::setRadius(float radius) {
	m_radius = radius;
}
