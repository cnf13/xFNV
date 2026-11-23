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

// The game settings this file defines, with their IDs and values, in their initialization order.
SettingT<INISettingCollection> fCameraCasterFadeSittingRadius("fCameraCasterFadeSittingRadius:HAVOK", 45.0f);

CameraCaster::CameraCaster(float afRadius, uint32_t aiGroup)
  : spPlayerShapePhantom(nullptr), spHorseShapePhantom(nullptr) {
    Init(afRadius, aiGroup);
}

CameraCaster::~CameraCaster() {}

void CameraCaster::Add(bhkWorld* apWorld) {
    if (GetPlayerShapePhantom()) {
        GetPlayerShapePhantom()->Add(apWorld);
        if (apWorld) {
            uint32_t uiColor = static_cast<uint32_t>(hkColor::rgbFromChars(0xFFu, 0x2Eu, 0x2Eu, 0x19u));
            apWorld->SetDebugObjectColor(GetPlayerShapePhantom(), uiColor);
        }
    }
    if (spHorseShapePhantom.m_pObject) {
        spHorseShapePhantom->Add(apWorld);
        if (apWorld) {
            uint32_t uiColor = static_cast<uint32_t>(hkColor::rgbFromChars(0xFFu, 0x2Eu, 0x2Eu, 0x19u));
            apWorld->SetDebugObjectColor(spHorseShapePhantom.m_pObject, uiColor);
        }
    }
}

void CameraCaster::SetPlayerGroup(uint32_t aiGroup) {
    spPlayerShapePhantom->bhkWorldObject::SetCollisionGroup(aiGroup);
}

// Casts the player phantom from arFrom to arTo and walks the sorted hits for the first that counts: ground always,
// otherwise a hit on visible 3D, except one on the VATS target during VATS playback (weapons and bipeds count there
// only) or on a mobile object whose character controller chases its biped. The hit goes to arPoint and its position to
// arTo.
bool CameraCaster::Pick(NiPoint3& arFrom, NiPoint3& arTo, hkpRootCdPoint& arPoint) {
    bool bResult = false;
    if (!GetPlayerShapePhantom()) {
        return bResult;
    }
    alignas(16) hkVector4 kStart;
    kStart.getQuad();
    // Raw storage: the collector is built and destroyed explicitly through its constructor and destructor.
    alignas(16) uint8_t aCollector[sizeof(hkpAllCdPointCollector)];
    hkpAllCdPointCollector* pCollector = ::new (aCollector) hkpAllCdPointCollector;
    alignas(16) hkpLinearCastInput kInput;
    NI2HK(kStart, arFrom);
    NI2HK(kInput.m_to, arTo);
    GetPlayerShapePhantom()->SetPositionAndLinearCast(kStart, kInput, *pCollector, nullptr);
    bResult = pCollector->hasHit();
    if (bResult) {
        pCollector->sortHits();
        int32_t iIndex = 0;
        bool bFound = false;
        bool bVATSPlayback = pVATS.GetMode() == VATS::VATS_PLAYBACK;
        TESObjectREFR* pTarget = nullptr;
        if (bVATSPlayback) {
            VATS_COMMAND* pAction = pVATS.GetCurrentAction();
            if (pAction) {
                pTarget = pAction->pTarget;
            }
            else {
                pTarget = pVATSTarget;
            }
            if (!pTarget) {
                pTarget = pPlayer;
            }
        }
        while (!bFound && iIndex < pCollector->getHits().getSize()) {
            const hkpCollidable* pCollidable = pCollector->getHits()[iIndex].m_rootCollidableB;
            COL_LAYER eLayer = CFilter(pCollidable->getCollisionFilterInfo()).GetLayer();
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
                    if (!bVATSPlayback) {
                        break;
                    }
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
                            }
                            else if (pRef->IsMobileObject()) {
                                bhkCharacterController* pController = static_cast<bhkCharacterController*>(
                                    static_cast<MobileObject*>(pRef)->GetCharController()
                                );
                                if (pController && pController->QChaseBip()) {
                                    bFound = false;
                                }
                            }
                        }
                    }
                    break;
                default: break;
            }
            if (!bFound) {
                ++iIndex;
            }
        }
        if (bFound) {
            arPoint.Assign(pCollector->getHits()[iIndex]);
            arPoint.m_contact.m_position.getQuad();
            HK2NI(arTo, arPoint.m_contact.m_position);
        }
        else {
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
        pWorld = bhkWorld::Getbhk(spPlayerShapePhantom->bhkWorldObject::GethkWorld());
    }
    return pWorld;
}

bool CameraCaster::KeepOutOfCharacter(
    bhkCharacterController* apController,
    NiPoint3& arPoint,
    float afRadius,
    float afScaleOffset
) {
    alignas(16) hkVector4 capsulePos;
    alignas(16) hkVector4 cameraPos;
    bool bRes = false;
    if (apController->QChaseBip() && pPlayer && pPlayer->pCurrentProcess && pPlayer->pCurrentProcess->GetHeadNode()) {
        NiAVObject* pHead = pPlayer->pCurrentProcess->GetHeadNode();
        NiPoint3 delta = arPoint - pHead->GetWorldTranslate();
        double dLength = delta.Length();
        if (fCameraCasterFadeSittingRadius.Float() > dLength) {
            bRes = true;
        }
        return bRes;
    }
    NI2HK(cameraPos, arPoint);
    capsulePos = apController->GethkPosition();
    bhkSimpleShapePhantom* pPhantom = nullptr;
    if (apController == pPlayer->GetCharController()) {
        pPhantom = spPlayerShapePhantom.m_pObject;
        double dCameraZ = cameraPos.getSimdAt(2);
        if (capsulePos.getSimdAt(2) < dCameraZ) {
            float* pCameraZ = &cameraPos(2);
            *pCameraZ = *pCameraZ - NI2HK(afScaleOffset);
            float* pCapsuleZ = &capsulePos(2);
            *pCapsuleZ = *pCapsuleZ - NI2HK(afScaleOffset);
        }
    }
    else {
        pPhantom = spHorseShapePhantom.m_pObject;
    }
    hkpCollisionInput input;
    hkpAllCdPointCollector collector;
    input.setTolerance(0.0f);
    const hkpCollidable* pca = pPhantom->GetCollidable();
    const hkpCollidable* pcb = &apController->GethkShapePhantom()->m_collidable;
    bhkWorld* pw = apController->GetWorld();
    if (pw) {
        pw->MarkForRead();
        hkpCollisionDispatcher* pdis = (**pw)->m_collisionDispatcher;
        pw->MarkForRead();
        if (pdis) {
            input.m_dispatcher = pdis;
            pw->MarkForRead();
            input.m_filter = (**pw)->m_collisionFilter;
            pw->MarkForRead();
            pPhantom->SetPosition(cameraPos);
            hkpSphereShape* ps = static_cast<hkpSphereShape*>(pPhantom->GethkShape());
            // Reads body B's shape type before body A's.
            hkpShapeType eTypeB = pcb->m_shape->getType();
            hkpShapeType eTypeA = pca->m_shape->getType();
            hkpCollisionDispatcher::GetClosestPointsFunc pFunc = pdis->getGetClosestPointsFunc(eTypeA, eTypeB);
            if (pFunc) {
                float fOldRad = ps->m_radius;
                ps->setRadius(afRadius);
                pFunc(*pca, *pcb, input, collector);
                bRes = collector.hasHit();
                ps->setRadius(fOldRad);
            }
        }
    }
    return bRes;
}

void CameraCaster::Init(float afRadius, uint32_t aiGroup) {
    bhkSphereShape* pPlayerShape = new bhkSphereShape(afRadius, true);
    hkSimpleShapePhantomCinfo csi1;
    hkSimpleShapePhantomCinfo csi2;
    CFilter iPlayerFilter(0u);
    CFilter iHorseFilter(0u);
    iPlayerFilter.SetLayer(L_CAMERA);
    iPlayerFilter.SetGroup(aiGroup);
    iHorseFilter.SetLayer(L_CAMERA);
    iHorseFilter.SetGroup(bhkCollisionFilter::GetNewSystemGroup());
    csi1.m_collisionFilterInfo = iPlayerFilter.iFilter;
    csi1.m_shape = **pPlayerShape;
    csi1.m_transform.setIdentity();
    bhkSimpleShapePhantom* pPlayerPhantom = new bhkSimpleShapePhantom(csi1);
    spPlayerShapePhantom = pPlayerPhantom;
    csi2.m_collisionFilterInfo = iHorseFilter.iFilter;
    csi2.m_shape = **pPlayerShape;
    csi2.m_transform.setIdentity();
    bhkSimpleShapePhantom* pHorsePhantom = new bhkSimpleShapePhantom(csi2);
    spHorseShapePhantom = pHorsePhantom;
}

void CameraCaster::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The Bethesda world of the controller's shape phantom's Havok world; the phantom is deliberately not null-checked.
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
    if (pw) {
        pw->markForRead();
    }
}

void bhkWorld::UnMarkForRead() {
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (pw) {
        pw->unmarkForRead();
    }
}

// The Havok world's user data is the bhkWorld that wraps it (bhkWorld::Link sets it).
bhkWorld* bhkWorld::Getbhk(const hkpWorld* apWorld) {
    if (apWorld) {
        return const_cast<ahkpWorld*>(static_cast<const ahkpWorld*>(apWorld))->getUserData();
    }
    return nullptr;
}

void hkMemoryAllocator::bufFreeRayCastOutputs(void* p, int32_t numElem) {
    bufFree(p, sizeof(hkpWorldRayCastOutput) * numElem);
}

// No early out until the owner lowers the distance (reset()'s HK_REAL_MAX).
hkpCdPointCollector::hkpCdPointCollector() : m_earlyOutDistance(HK_REAL_MAX) {}

hkpCdPointCollector::~hkpCdPointCollector() {}

// HK_DECLARE_CLASS_ALLOCATOR's operator delete: the block goes back to the thread's Havok heap.
void hkpCdPointCollector::operator delete(void* p) {
    if (p) {
        hkMemoryRouter::getInstance().heap().blockFree(p, sizeof(hkpCdPointCollector));
    }
}

// The three vectors' constructors are empty.
hkpCollisionInput::Aabb32Info::Aabb32Info() {}

// The constructor writes nothing: only m_aabb32Info's (empty) constructor runs.
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
    m_contact = __that.m_contact;
    m_rootCollidableA = __that.m_rootCollidableA;
    m_shapeKeyA = __that.m_shapeKeyA;
    for (uint32_t i = 0; i < 8; ++i) {
        m_shapeKeysA[i] = __that.m_shapeKeysA[i];
    }
    m_rootCollidableB = __that.m_rootCollidableB;
    m_shapeKeyB = __that.m_shapeKeyB;
    for (uint32_t i = 0; i < 8; ++i) {
        m_shapeKeysB[i] = __that.m_shapeKeysB[i];
    }
    return *this;
}

void hkpConvexShape::setRadius(float radius) {
    m_radius = radius;
}
