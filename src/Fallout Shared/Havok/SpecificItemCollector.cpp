#include "Fallout Shared/Havok/SpecificItemCollector.h"

#include "BSCore/MemoryManager.h"
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
        CFilter kFilter;
        XFNV_RECONSTRUCT(CFilter)(&kFilter, pRoot->getCollisionFilterInfo());
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

void bhkCollisionFilter::SetCollisionFlags(COL_LAYER aeLayer, uint64_t aiFlags) {
    if (aeLayer >= L_CAMERA) {
        bhkCollisionFilter::ulLayerFlagA[aeLayer] = static_cast<uint64_t>(
            (bhkCollisionFilter::ulLayerFlagA[aeLayer] & 0xFFFFFFFF00000000ull) |
            (static_cast<uint64_t>(static_cast<uint32_t>(aiFlags)))
        );
        bhkCollisionFilter::ulLayerFlagA[aeLayer] = static_cast<uint64_t>(
            (bhkCollisionFilter::ulLayerFlagA[aeLayer] & 0xFFFFFFFFull) |
            (static_cast<uint64_t>(static_cast<uint32_t>(aiFlags >> 32)) << 32)
        );
    }
}

hkpClosestRayHitCollector::hkpClosestRayHitCollector() {
    reset();
}

// Keeps the hit when it is closer than the one held: the hit's output, its shape keys and its root collidable, and
// the hit's fraction as the new early-out fraction.
void hkpClosestRayHitCollector::addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) {
    if (m_rayHit.m_hitFraction > static_cast<double>(hitInfo.m_hitFraction)) {
        // EXE-INLINED 0x9C1B00
        m_rayHit.m_normal = hitInfo.m_normal;
        m_rayHit.m_hitFraction = hitInfo.m_hitFraction;
        m_rayHit.m_extraInfo = hitInfo.m_extraInfo;
        m_rayHit.m_pad[0] = hitInfo.m_pad[0];
        m_rayHit.m_pad[1] = hitInfo.m_pad[1];
        hkpRayHitCollector::shapeKeysFromCdBody(m_rayHit.m_shapeKeys, 8, cdBody);
        // EXE-INLINED 0x624020
        const hkpCdBody* pRoot = &cdBody;
        while (pRoot->m_parent) {
            pRoot = pRoot->m_parent;
        }
        // m_rootCollidable is not const because the game code that uses it takes it non-const, hence the const_cast.
        m_rayHit.m_rootCollidable = const_cast<hkpCollidable*>(static_cast<const hkpCollidable*>(pRoot));
        m_earlyOutHitFraction = hitInfo.m_hitFraction;
    }
}

hkpClosestRayHitCollector::~hkpClosestRayHitCollector() {}

void hkpClosestRayHitCollector::reset() {
    m_rayHit.reset();
    hkpRayHitCollector::reset();
}
