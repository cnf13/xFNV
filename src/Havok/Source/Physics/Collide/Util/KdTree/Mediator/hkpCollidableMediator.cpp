#include "Havok/Source/Physics/Collide/Util/KdTree/Mediator/hkpCollidableMediator.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <array>
#include <bit>
#include <emmintrin.h>

// The SDK's constructor: the mediator keeps the collidable array; with a world, every collidable whose stored AABB is
// not valid gets it recalculated: an entity's through hkpEntityAabbUtil, a phantom's through its own calcAabb.
hkpCollidableMediator::hkpCollidableMediator(
    hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
    const hkpWorld* world
)
  : m_collidables(&collidables) {
    if (world) {
        for (const hkpCollidable* pCollidable : *m_collidables) {
            if (!pCollidable->m_boundingVolumeData.isValid()) {
                if (pCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
                    hkpEntity* pEntity = static_cast<hkpEntity*>(pCollidable->getOwner());
                    hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, &pEntity, 1);
                }
                else {
                    hkAabb aabb;
                    hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidable->getOwner());
                    pPhantom->calcAabb(aabb);
                    pPhantom->setBoundingVolumeData(aabb);
                }
            }
        }
    }
}

hkpCollidableMediator::~hkpCollidableMediator() {}

int32_t hkpCollidableMediator::getNumPrimitives() const {
    return m_collidables->m_size;
}

// The SDK's: a primitive's id is its collidable's address as an integer (its hkUlong(...) cast; the tree treats the id
// as untyped and projectPrimitive turns it back into the collidable).
hkUlong hkpCollidableMediator::getPrimitiveId(int32_t primitiveIdx) const {
    return reinterpret_cast<hkUlong>(m_collidables->m_data[primitiveIdx]);
}

// The collidable's AABB in 32-bit integer space: the one it stores while that is valid, else its shape's AABB quantised
// with the world's collision input with the expansion bytes cleared. Then projected to the tree's 16 bits: every 16-bit
// lane of the bounds shifted left once and the high half of each 32-bit word taken, the upper bound rounded up by a
// saturating add of 1 to every lane. The two spare words get the shifted high halves of the AABB's fourth words; the
// caller sets the primitive id over them.
void hkpCollidableMediator::projectPrimitive(hkUlong primitiveId, hkKdTreeProjectedEntry& bounds) {
    // The primitive id is the collidable's address kept as an integer (getPrimitiveId; hkPrimitiveId is untyped).
    const hkpCollidable* collidable = reinterpret_cast<const hkpCollidable*>(primitiveId);
    hkAabbUint32 aabb;
    if (collidable->m_boundingVolumeData.isValid()) {
        hkString::memCpy16<32>(&aabb, &collidable->m_boundingVolumeData);
    }
    else {
        const hkpWorld* world = static_cast<const hkpWorldObject*>(collidable->getOwner())->m_world;
        hkAabb shapeAabb;
        collidable->m_shape->getAabb(*collidable->m_motion.pMotion, 0.0f, shapeAabb);
        const hkpCollisionInput::Aabb32Info& info = world->m_collisionInput->m_aabb32Info;
        hkVector4 lo;
        lo.m_quad = _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(info.m_bitOffsetLow.m_quad, shapeAabb.m_min.m_quad), info.m_bitScale.m_quad),
                hkVector4Util::hkAabbUint32MaxVal
            ),
            hkVector4Zero.m_quad
        );
        aabb.m_min[0] = static_cast<uint32_t>(hkMath::hkToIntFast(lo(0)));
        aabb.m_min[1] = static_cast<uint32_t>(hkMath::hkToIntFast(lo(1)));
        aabb.m_min[2] = static_cast<uint32_t>(hkMath::hkToIntFast(lo(2)));
        hkVector4 hi;
        hi.m_quad = _mm_max_ps(
            _mm_min_ps(
                _mm_mul_ps(_mm_add_ps(info.m_bitOffsetHigh.m_quad, shapeAabb.m_max.m_quad), info.m_bitScale.m_quad),
                hkVector4Util::hkAabbUint32MaxVal
            ),
            hkVector4Zero.m_quad
        );
        aabb.m_max[0] = static_cast<uint32_t>(hkMath::hkToIntFast(hi(0)));
        aabb.m_max[1] = static_cast<uint32_t>(hkMath::hkToIntFast(hi(1)));
        aabb.m_max[2] = static_cast<uint32_t>(hkMath::hkToIntFast(hi(2)));
        // The w lane's 32-bit value lands on the expansion bytes and m_shapeKeyByte; only its top byte survives the
        // clears.
        const uint32_t hiW = static_cast<uint32_t>(hkMath::hkToIntFast(hi(3)));
        aabb.m_expansionShift = 0;
        aabb.m_expansionMin[0] = 0;
        aabb.m_expansionMin[1] = 0;
        aabb.m_expansionMin[2] = 0;
        aabb.m_expansionMax[0] = 0;
        aabb.m_expansionMax[1] = 0;
        aabb.m_expansionMax[2] = 0;
        aabb.m_shapeKeyByte = static_cast<uint8_t>(hiW >> 24);
    }
    // Loads the two 16-byte halves (m_min with its expansion word, m_max with its), read as eight 16-bit lanes each.
    const std::array<__m128i, 2> quads = std::bit_cast<std::array<__m128i, 2>>(aabb);
    const std::array<uint16_t, 8> loWords = std::bit_cast<std::array<uint16_t, 8>>(_mm_slli_epi32(quads[0], 1));
    const std::array<uint16_t, 8> hiWords = std::bit_cast<std::array<uint16_t, 8>>(
        _mm_adds_epu16(_mm_slli_epi32(quads[1], 1), _mm_set1_epi16(1))
    );
    hkKdTreeProjectedEntry entry;
    entry.m_min[0] = loWords[1];
    entry.m_min[1] = loWords[3];
    entry.m_min[2] = loWords[5];
    entry.m_primIdHigh = loWords[7];
    entry.m_max[0] = hiWords[1];
    entry.m_max[1] = hiWords[3];
    entry.m_max[2] = hiWords[5];
    entry.m_primIdLow = hiWords[7];
    bounds = entry;
}
