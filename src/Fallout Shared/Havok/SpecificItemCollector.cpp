#include "Fallout Shared/Havok/SpecificItemCollector.h"

#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "Fallout/AI/MobileObject.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"

SpecificItemCollector::SpecificItemCollector(COL_LAYER aePick, MobileObject* arFilter) : iFilter(0) {
    if (arFilter) {
        CFilter kFilter;
        arFilter->GetCollisionFilter(&kFilter.iFilter);
        Init(aePick, kFilter);
    }
}

// Passes on the hits on the filter's layer (other layers' only when they are of the same group) that are at least
// fMinHitFraction along the ray; a NaN fraction is dropped.
void SpecificItemCollector::addRayHit(const hkpCdBody& arBody, const hkpShapeRayCastCollectorOutput& arHitInfo) {
    const hkpCollidable* pRoot = arBody.getRootCollidable();
    if (pRoot) {
        CFilter kFilter(pRoot->getCollisionFilterInfo());
        if ((kFilter.GetLayer() != iFilter.GetLayer() || kFilter.GetGroup() == iFilter.GetGroup()) &&
            (arHitInfo.m_hitFraction >= fMinHitFraction)) {
            hkpClosestRayHitCollector::addRayHit(arBody, arHitInfo);
        }
    }
}

SpecificItemCollector::~SpecificItemCollector() {}

// Sets the filter and the collision flags of the pick layer.
void SpecificItemCollector::Init(COL_LAYER aePick, CFilter& arFilter) {
    fMinHitFraction = 0.0f;
    iFilter.iFilter = arFilter.iFilter;
    // The compare is signed.
    if (static_cast<int32_t>(aePick) < L_CAMERA) {
        aePick = L_CUSTOMPICK1;
    }
    // The pick collides with these layers and with the filter's own. The shift uses the low byte of the layer and gives
    // 0 for a count of 64 or more (GetLayer returns up to 0x7F).
    uint8_t ucLayer = static_cast<uint8_t>(iFilter.GetLayer());
    uint64_t uiLayerBit = ucLayer >= 64 ? 0 : 1ull << ucLayer;
    bhkCollisionFilter::SetCollisionFlags(
        aePick,
        uiLayerBit | (1ull << L_UNIDENTIFIED) | (1ull << L_STATIC) | (1ull << L_ANIMSTATIC) | (1ull << L_CLUTTER) |
            (1ull << L_PROJECTILE) | (1ull << L_BIPED) | (1ull << L_TREES) | (1ull << L_PROPS) | (1ull << L_TERRAIN) |
            (1ull << L_GROUND)
    );
}

void SpecificItemCollector::SetMinHitFraction(float afValue) {
    fMinHitFraction = afValue;
}

void SpecificItemCollector::operator delete(void* apMemory) {
    ourFree(apMemory);
}
