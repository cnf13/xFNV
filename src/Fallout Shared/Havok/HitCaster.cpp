#include "Fallout Shared/Havok/HitCaster.h"

#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

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
    return iGroup;
}

void HitCaster::SetCustomPickCollisions() {
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, 0x6016777Eull);
}

// Casts the shape phantom from aOrigin towards aOrigin + aDirection * afPickDistance, sorts the contact points it found
// and reports whether it hit anything.
bool HitCaster::CastShape(const NiPoint3& aOrigin, const NiPoint3& aDirection, float afPickDistance) {
    NiPoint3 kOffset = aDirection * afPickDistance;
    NiPoint3 kTarget = aOrigin + kOffset;
    SetCustomPickCollisions();
    if (spShapePhantom) {
        hkVector4 kStart;
        kStart.getQuad();
        hkpLinearCastInput kInput;
        kInput.m_startPointTolerance = 0.01f;
        kInput.m_maxExtraPenetration = 0.01f;
        NI2HK(kStart, aOrigin);
        NI2HK(kInput.m_to, kTarget);
        reset();
        bhkShapePhantom* pPhantom = spShapePhantom;
        pPhantom->SetPositionAndLinearCast(kStart, kInput, *this, nullptr);
        hkBool kHit = hasHit();
        if (kHit) {
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
    CFilter kFilter;
    spShapePhantom->GetCollisionFilter(&kFilter);
    kFilter.SetGroup(aiGroup);
    iGroup = aiGroup;
    spShapePhantom->SetCollisionFilter(kFilter);
}

void HitCaster::Add(bhkWorld* apWorld) {
    if (spShapePhantom.m_pObject) {
        spShapePhantom->Add(apWorld);
        if (apWorld) {
            uint32_t uiColor = hkColor::rgbFromChars(88, 173, 86, 40);
            apWorld->SetDebugObjectColor(spShapePhantom.m_pObject, uiColor);
        }
    }
}

// Builds the pick phantom: a sphere of afRadius on layer LAYER_CUSTOMPICK1 in collision group aiGroup.
void HitCaster::Init(float afRadius, uint32_t aiGroup) {
    bhkSphereShape* pShape = new bhkSphereShape(afRadius, true);
    hkSimpleShapePhantomCinfo csi;
    CFilter kFilter;
    kFilter.iFilter = 0;
    kFilter.SetLayer(L_CUSTOMPICK1);
    kFilter.SetGroup(aiGroup);
    iGroup = aiGroup;
    csi.m_collisionFilterInfo = kFilter.iFilter;
    csi.m_shape = **pShape;
    csi.m_transform.setIdentity();
    bhkSimpleShapePhantom* pPhantom = new bhkSimpleShapePhantom(csi);
    spShapePhantom = pPhantom;
    bInit = true;
}

HitCaster* HitCaster::GetInstance(HitCaster::HitCasterSize eSize, bool abCreate) {
    if (eSize) {
        if (eSize == HitCaster::SIZE_LARGE) {
            if (!HitCaster::pLargeHitCaster && abCreate) {
                HitCaster::pLargeHitCaster = new HitCaster(
                    fHitCasterSizeLarge.Float(),
                    bhkCollisionFilter::GetNewSystemGroup()
                );
            }
            return HitCaster::pLargeHitCaster;
        }
    }
    else {
        if (!HitCaster::pSmallHitCaster && abCreate) {
            HitCaster::pSmallHitCaster = new HitCaster(
                fHitCasterSizeSmall.Float(),
                bhkCollisionFilter::GetNewSystemGroup()
            );
        }
        return HitCaster::pSmallHitCaster;
    }
    return nullptr;
}

void* HitCaster::operator new(size_t aSize) {
    return MemoryManager::Instance().Allocate(aSize);
}

void HitCaster::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
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
