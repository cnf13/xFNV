#include "Fallout Shared/Havok/HitCaster.h"

#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Reconstruction.hpp"

#include <cstdint>
#include <xmmintrin.h>

// The game settings this file defines, with the game's values, in its initialization order.
SettingT<GameSettingCollection> fHitCasterSizeSmall("fHitCasterSizeSmall", 12.0f);
SettingT<GameSettingCollection> fHitCasterSizeLarge("fHitCasterSizeLarge", 24.0f);

HitCaster::HitCaster(float afRadius, uint32_t aiGroup) : spShapePhantom(nullptr) {
    Init(afRadius, aiGroup);
}

HitCaster::~HitCaster() {}

uint32_t HitCaster::GetShapeGroup() {
    return (uint32_t)((int32_t)iGroup);
}

void HitCaster::SetCustomPickCollisions() {
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, 0x6016777Eull);
}

// Casts the shape phantom from aOrigin towards aOrigin + aDirection * afPickDistance, sorts the contact points it found
// and reports whether it hit anything.
bool HitCaster::CastShape(const NiPoint3& aOrigin, const NiPoint3& aDirection, float afPickDistance) {
    NiPoint3 kOffset = aDirection.operator*(afPickDistance);
    NiPoint3 kTarget = aOrigin.operator+(kOffset);
    SetCustomPickCollisions();
    if (spShapePhantom.operator->()) {
        hkVector4 kStart;
        kStart.getQuad();
        hkpLinearCastInput kInput;
        kInput.~hkpLinearCastInput();
        ::new (&kInput) hkpLinearCastInput;
        kInput.m_startPointTolerance = 0.01f;
        kInput.m_maxExtraPenetration = 0.01f;
        NI2HK(kStart, aOrigin);
        NI2HK(kInput.m_to, kTarget);
        reset();
        bhkShapePhantom* pPhantom = spShapePhantom.operator->();
        pPhantom->SetPositionAndLinearCast(kStart, kInput, *this, nullptr);
        hkBool kHit;
        hkBool* pHit = hasHit(&kHit);
        if (pHit->operator bool()) {
            sortHits();
            return true;
        }
    }
    else {
        reset();
    }
    return false;
}

void HitCaster::SetShapeGroup(uint32_t aiGroup) {
    CFilter aiFilter;
    bhkSimpleShapePhantom* v2 = (&spShapePhantom)->m_pObject;
    v2->GetCollisionFilter(&aiFilter);
    (&aiFilter)->SetGroup(aiGroup);
    iGroup = aiGroup;
    bhkSimpleShapePhantom* v3 = (&spShapePhantom)->m_pObject;
    v3->SetCollisionFilter(aiFilter);
}

void HitCaster::Add(bhkWorld* apWorld) {
    if (spShapePhantom.m_pObject) {
        bhkSimpleShapePhantom* v3 = spShapePhantom.m_pObject;
        v3->Add(apWorld);
        if (apWorld) {
            uint32_t v2 = hkColor::rgbFromChars(88, 173, 86, 40);
            apWorld->SetDebugObjectColor(spShapePhantom.m_pObject, v2);
        }
    }
}

// Builds the pick phantom: a sphere of afRadius on layer LAYER_CUSTOMPICK1 in collision group aiGroup.
void HitCaster::Init(float afRadius, uint32_t aiGroup) {
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
    iGroup = aiGroup;
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
    spShapePhantom.operator=(pPhantom);
    bInit = true;
}

HitCaster* HitCaster::GetInstance(HitCaster::HitCasterSize eSize, bool abCreate) {
    HitCaster* presult = nullptr;
    if (eSize) {
        if (eSize == HitCaster::SIZE_LARGE) {
            if (!HitCaster::pLargeHitCaster && abCreate) {
                HitCaster* v7;
                HitCaster* v9 = static_cast<HitCaster*>(bhkRagdollController::operator new(sizeof(HitCaster)));
                if (v9) {
                    uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
                    float* v3 = &fHitCasterSizeLarge.Float();
                    v7 = ::new (v9) HitCaster(*v3, NewSystemGroup);
                }
                else {
                    v7 = nullptr;
                }
                HitCaster::pLargeHitCaster = v7;
            }
            return HitCaster::pLargeHitCaster;
        }
    }
    else {
        if (!HitCaster::pSmallHitCaster && abCreate) {
            HitCaster* v8;
            HitCaster* apMemory = static_cast<HitCaster*>(bhkRagdollController::operator new(sizeof(HitCaster)));
            if (apMemory) {
                uint32_t v5 = bhkCollisionFilter::GetNewSystemGroup();
                float* v2 = &fHitCasterSizeSmall.Float();
                v8 = ::new (apMemory) HitCaster(*v2, v5);
            }
            else {
                v8 = nullptr;
            }
            HitCaster::pSmallHitCaster = v8;
        }
        return HitCaster::pSmallHitCaster;
    }
    return presult;
}

void HitCaster::operator delete(void* apMemory) {
    ourMemalignFree(apMemory);
}

void HitCaster::ClearInstance() {
    if (HitCaster::pLargeHitCaster) {
        delete HitCaster::pLargeHitCaster;
    }
    HitCaster::pLargeHitCaster = nullptr;
    if (HitCaster::pSmallHitCaster) {
        delete HitCaster::pSmallHitCaster;
    }
    HitCaster::pSmallHitCaster = nullptr;
}

void bhkWorldObject::SetCollisionFilter(CFilter aiFilter) {
    hkpCollidable* pc = GetCollidableRw();
    if (pc) {
        static_cast<hkpLinkedCollidable*>(pc)->hkpLinkedCollidable::setCollisionInfoFlags(aiFilter.iFilter);
    }
    UpdateCFilter();
}
