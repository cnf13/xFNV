#include "Fallout Shared/Havok/DecalCaster.h"

#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Reconstruction.hpp"

#include <cstdint>

extern float fMaxDecalWidthSmall;
extern float fMaxDecalWidthLarge;

float fMaxDecalWidthSmall = 32.0f;
float fMaxDecalWidthLarge = 256.0f;

DecalCaster::DecalCaster(float afRadius, uint32_t aiGroup) : spShapePhantom(nullptr) {
    Init(afRadius, aiGroup);
}

DecalCaster::~DecalCaster() {}

void DecalCaster::SetCustomPickCollisions(CUSTOM_PICK aeCustomPick) {
    switch (aeCustomPick) {
        case CUSTOM_PICK_ACTORS:
            bhkCollisionFilter::SetMultiLayerCollision(
                L_CUSTOMPICK1,
                (1ull << L_STATIC) | (1ull << L_ANIMSTATIC) | (1ull << L_CLUTTER) | (1ull << L_WEAPON) |
                    (1ull << L_TREES) | (1ull << L_PROPS) | (1ull << L_TERRAIN) | (1ull << L_TRAP) | (1ull << L_GROUND)
            );
            break;
        case CUSTOM_PICK_BIPED:
            bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, (1ull << L_BIPED) | (1ull << L_DEADBIP));
            break;
        default: break;
    }
}

const hkpRootCdPoint& DecalCaster::GetCollision(uint32_t aiIndex) {
    return PointCollector.getHits()[aiIndex];
}

// The number of collision points the collector holds.
uint32_t DecalCaster::QNumCollisions() {
    return (&PointCollector.getHits())->m_size;
}

// Sets the radius of the pick phantom's sphere.
void DecalCaster::SetRadius(float afRadius) {
    if (spShapePhantom.m_pObject) {
        // The phantom's shape is the sphere Init gave it.
        hkpSphereShape* ps = static_cast<hkpSphereShape*>(spShapePhantom.m_pObject->GethkShape());
        if (ps) {
            ps->setRadius(NI2HK(afRadius));
        }
    }
}

// Casts the pick phantom from apFrom to apTo, collecting the contact points in PointCollector, and returns how many
// collisions it holds. A nonzero afRadius replaces the phantom's radius first.
uint32_t DecalCaster::Pick(const NiPoint3& arFrom, const NiPoint3& arTo, float afRadius) {
    if (afRadius != 0.0) {
        SetRadius(afRadius);
    }
    SetCustomPickCollisions(CUSTOM_PICK_ACTORS);
    static_cast<hkpCdPointCollector*>(&PointCollector)->reset();
    if (!spShapePhantom.operator->()) {
        return QNumCollisions();
    }
    hkVector4 kStart;
    kStart.getQuad();
    hkpLinearCastInput kInput;
    kInput.~hkpLinearCastInput();
    ::new (&kInput) hkpLinearCastInput;
    NI2HK(kStart, arFrom);
    NI2HK(kInput.m_to, arTo);
    bhkShapePhantom* pPhantom = spShapePhantom.operator->();
    pPhantom->SetPositionAndLinearCast(kStart, kInput, PointCollector, nullptr);
    hkBool kHit = PointCollector.hasHit();
    // The result is deliberately discarded, as the game does.
    kHit.operator bool();
    return QNumCollisions();
}

void DecalCaster::Add(bhkWorld* apWorld) {
    if ((spShapePhantom.m_pObject) && (GetWorld() != apWorld)) {
        bhkSimpleShapePhantom* v3 = spShapePhantom.m_pObject;
        v3->Add(apWorld);
        if (apWorld) {
            uint32_t v2 = hkColor::rgbFromChars(255, 16, 16, 25);
            apWorld->SetDebugObjectColor(spShapePhantom.m_pObject, v2);
        }
    }
}

void DecalCaster::GetPenetrations(const NiPoint3& arPos, hkpCdBodyPairCollector& arCollector) {
    bhkSimpleShapePhantom* v3;
    bhkSimpleShapePhantom* v4;
    if ((&spShapePhantom)->m_pObject) {
        v3 = (&spShapePhantom)->m_pObject;
        v3->SetPosition(arPos);
        (&arCollector)->reset();
        v4 = (&spShapePhantom)->m_pObject;
        v4->GetPenetrations(&arCollector);
    }
}

// Builds the pick phantom: a sphere of afRadius on layer LAYER_CUSTOMPICK1 in collision group aiGroup, and sets which
// layers that layer collides with.
void DecalCaster::Init(float afRadius, uint32_t aiGroup) {
    bhkSphereShape* pMemory = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
    bhkSphereShape* pshape;
    if (pMemory) {
        pshape = ::new (pMemory) bhkSphereShape(afRadius, true);
    }
    else {
        pshape = nullptr;
    }
    hkSimpleShapePhantomCinfo csi;
    CFilter ifilter;
    ifilter.iFilter = 0;
    ifilter.SetLayer(L_CUSTOMPICK1);
    ifilter.SetGroup(aiGroup);
    csi.m_collisionFilterInfo = ifilter.iFilter;
    csi.m_shape = pshape->operator*();
    csi.m_transform.setIdentity();
    bhkSimpleShapePhantom* pPhantomMemory =
        static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
    bhkSimpleShapePhantom* pPhantom;
    if (pPhantomMemory) {
        pPhantom = ::new (pPhantomMemory) bhkSimpleShapePhantom(csi);
    }
    else {
        pPhantom = nullptr;
    }
    spShapePhantom = pPhantom;
    spShapePhantom.m_pObject->SetName("DecalCaster");
    // Every layer but these.
    uint64_t uimultiLayer =
        ~((1ull << L_TRANSPARENT) | (1ull << L_PROJECTILE) | (1ull << L_SPELL) | (1ull << L_BIPED) | (1ull << L_WATER) |
          (1ull << L_PORTAL) | (1ull << L_DEBRIS_SMALL) | (1ull << L_DEBRIS_LARGE) | (1ull << L_DEADBIP) |
          (1ull << L_CHARCONTROLLER) | (1ull << L_AVOIDBOX) | (1ull << L_CAMERA) | (1ull << L_ITEMPICKER) |
          (1ull << L_LOS) | (1ull << L_CUSTOMPICK1) | (1ull << L_SPELLEXPLOSION));
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, uimultiLayer);
}

bhkWorld* DecalCaster::GetWorld() {
    bhkWorld* v4 = nullptr;
    if (spShapePhantom.m_pObject) {
        bhkSimpleShapePhantom* v3 = spShapePhantom.m_pObject;
        const hkpWorld* v1 = v3->GethkWorld();
        return bhkWorld::Getbhk(v1);
    }
    return v4;
}

NiAVObject* DecalCaster::GetAVObject(uint32_t aiIndex) {
    NiAVObject* v6 = nullptr;
    const hkpRootCdPoint* Collision = &GetCollision(aiIndex);
    if (Collision->m_rootCollidableB) {
        hkpWorldObject* apBody = static_cast<hkpWorldObject*>(Collision->m_rootCollidableB->getOwner());
        bhkNiCollisionObject* v3 = bhkNiCollisionObject::Getbhk(apBody);
        if (v3) {
            return v3->GetSceneGraphObject();
        }
    }
    return v6;
}

void DecalCaster::GetCollisionData(uint32_t aiIndex, NiPoint3& arPos, NiPoint3& arNormal) {
    const hkpRootCdPoint* rpoint = &GetCollision(aiIndex);
    HK2NI(arPos, (&rpoint->m_contact)->m_position);
    HK2NI(arNormal, rpoint->m_contact.getNormal());
}

float DecalCaster::GetMaxDecalWidthSmall() {
    return fMaxDecalWidthSmall;
}

float DecalCaster::GetMaxDecalWidthLarge() {
    return fMaxDecalWidthLarge;
}

DecalCaster* DecalCaster::GetInstance(float afDecalRadius, bool abCreate) {
    if (!(static_cast<double>(afDecalRadius) > DecalCaster::GetMaxDecalWidthSmall())) {
        if (!DecalCaster::pSmallInstance && abCreate) {
            DecalCaster* v7;
            DecalCaster* v9 = static_cast<DecalCaster*>(bhkRagdollController::operator new(sizeof(DecalCaster)));
            if (v9) {
                uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
                float afRadiusa = DecalCaster::GetMaxDecalWidthSmall();
                v7 = ::new (v9) DecalCaster(afRadiusa, NewSystemGroup);
            }
            else {
                v7 = nullptr;
            }
            DecalCaster::pSmallInstance = v7;
        }
        return DecalCaster::pSmallInstance;
    }

    if (!DecalCaster::pLargeInstance && abCreate) {
        DecalCaster* v8;
        DecalCaster* apMemory = static_cast<DecalCaster*>(bhkRagdollController::operator new(sizeof(DecalCaster)));
        if (apMemory) {
            uint32_t v5 = bhkCollisionFilter::GetNewSystemGroup();
            float afRadius = DecalCaster::GetMaxDecalWidthLarge();
            v8 = ::new (apMemory) DecalCaster(afRadius, v5);
        }
        else {
            v8 = nullptr;
        }
        DecalCaster::pLargeInstance = v8;
    }
    return DecalCaster::pLargeInstance;
}

void DecalCaster::operator delete(void* apMemory) {
    ourMemalignFree(apMemory);
}

void DecalCaster::ClearInstance() {
    if (DecalCaster::pLargeInstance) {
        delete DecalCaster::pLargeInstance;
    }
    DecalCaster::pLargeInstance = nullptr;
    if (DecalCaster::pSmallInstance) {
        delete DecalCaster::pSmallInstance;
    }
    DecalCaster::pSmallInstance = nullptr;
}

void bhkShapePhantom::GetPenetrations(hkpCdBodyPairCollector* arCollector) {
    hkpSimpleShapePhantom* v3 = static_cast<hkpSimpleShapePhantom*>(operator*());
    if ((v3) && (v3->m_world)) {
        v3->getPenetrations(*arCollector, nullptr);
    }
}

void bhkWorldObject::SetName(const char* apName) {
    hkpWorldObject* pObject = operator*();
    if (pObject) {
        MarkWrite();
        pObject->setName(apName);
        UnMarkWrite();
    }
}
