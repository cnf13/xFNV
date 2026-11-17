#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Gjk/GskManifold/hkpGskManifoldUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Agent/hkpGskAgentUtil.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"

// The agent's cache starts from the two shapes' first vertices, through the triangle's own init when B is a triangle;
// the contact-TIM flag is cleared and the manifold's counts reset. The free memory ends at the next multiple of 16
// after the cache and the manifold, computed from the manifold's (just cleared) counts.
hkpAgentData* hkPredGskAgent3::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
    hkPredGskAgent3Data* data = static_cast<hkPredGskAgent3Data*>(agentData);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(input.m_bodyA->m_shape);
    const hkpShape* shapeB = input.m_bodyB->m_shape;
    if (shapeB->m_type == HK_SHAPE_TRIANGLE) {
        data->m_gskCache.initTriangle(shapeA, static_cast<const hkpTriangleShape*>(shapeB), input.m_aTb);
    }
    else {
        data->m_gskCache.init(shapeA, static_cast<const hkpConvexShape*>(shapeB), input.m_aTb);
    }
    entry->m_numContactPoints = 0;
    data->m_gskCache.m_gskFlags &= ~hkpGskCache::GSK_FLAGS_DISABLE_CONTACT_TIMS;
    data->m_gskManifold.init();
    return static_cast<hkpAgentData*>(
        static_cast<uint8_t*>(agentData) +
        ((sizeof(hkpGskCache) + data->m_gskManifold.getTotalSizeInBytes() + 15) & ~15)
    );
}

// Under continuous physics the TOI is searched when distAtT1 falls below the minimum separation, and the full GSK
// collision follows. Else, while the bodies stay beyond the manifold's TIM distance and contact TIMs are allowed, the
// manifold's points are reused, moved to the bodies' transforms; otherwise the full GSK collision runs. This is the
// flow of hkPredGskCylinderAgent3::process without the capsule stand-ins. Returns the free memory after the agent's
// data.
hkpAgentData* hkPredGskAgent3::process(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormal,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("PredGskf3", nullptr);

    hkPredGskAgent3Data* data = static_cast<hkPredGskAgent3Data*>(agentData);
    hkpGskCache& gskCache = data->m_gskCache;
    hkpGskManifold& gskManifold = data->m_gskManifold;
    const float distAtT1 = input.m_distAtT1.m_storage;
    const hkpCollisionQualityInfo& qualityInfo = *input.m_input->m_collisionQualityInfo;
    if (qualityInfo.m_useContinuousPhysics.m_storage) {
        // EXE-INLINED 0x624020
        const hkpCdBody* rootA = input.m_bodyA;
        while (rootA->m_parent) {
            rootA = rootA->m_parent;
        }
        const float depthA = static_cast<const hkpCollidable*>(rootA)->m_allowedPenetrationDepth;
        // EXE-INLINED 0x624020
        const hkpCdBody* rootB = input.m_bodyB;
        while (rootB->m_parent) {
            rootB = rootB->m_parent;
        }
        const float depthB = static_cast<const hkpCollidable*>(rootB)->m_allowedPenetrationDepth;
        // EXE-INLINED 0xC9E0F0
        const float allowedPenetrationDepth = (depthA < depthB) ? depthA : depthB;
        const float distAtT0 = (*separatingNormal)(3);
        const float minSeparationA = qualityInfo.m_minSeparation * allowedPenetrationDepth;
        const float minSeparationB = distAtT0 + qualityInfo.m_minExtraSeparation * allowedPenetrationDepth;
        // EXE-INLINED 0xC9E0F0
        const float minSeparation = (minSeparationA < minSeparationB) ? minSeparationA : minSeparationB;
        // The TOI is searched unless distAtT1 reaches minSeparation (a NaN searches it).
        if (!(minSeparation <= distAtT1)) {
            const float toiSeparationA = qualityInfo.m_toiSeparation * allowedPenetrationDepth;
            const float toiSeparationB = distAtT0 + qualityInfo.m_toiExtraSeparation * allowedPenetrationDepth;
            // EXE-INLINED 0xC9E0F0
            const float toiSeparation = (toiSeparationA < toiSeparationB) ? toiSeparationA : toiSeparationB;
            hk4dGskCollideCalcToi(
                input, allowedPenetrationDepth, minSeparation, toiSeparation, gskCache, *separatingNormal, result
            );
            goto processCollision;
        }
    }

    // The bodies stay beyond the TIM distance: the manifold's points are reused, moved to the bodies' transforms.
    if (qualityInfo.m_manifoldTimDistance < distAtT1 &&
        !(gskCache.m_gskFlags & hkpGskCache::GSK_FLAGS_DISABLE_CONTACT_TIMS)) {
        (*separatingNormal)(3) = distAtT1;
        if (gskManifold.m_numContactPoints) {
            hkpGskManifoldWork work;
            work.m_keepContact.m_storage = input.m_input->m_tolerance;
            const hkpCdBody* bodyA = input.m_bodyA;
            const hkpCdBody* bodyB = input.m_bodyB;
            const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA->m_shape);
            const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB->m_shape);
            work.m_radiusA.m_storage = shapeA->m_radius;
            work.m_radiusB.m_storage = shapeB->m_radius;
            work.m_masterNormal = *separatingNormal;
            const uint16_t* vertexIds = gskManifold.getVertexIds();
            // Adds the tolerance to radius A, then radius B, and keeps the sum as a float.
            const float radiusSum = work.m_radiusA.m_storage + work.m_keepContact.m_storage + work.m_radiusB.m_storage;
            work.m_radiusSumSqrd.m_storage = radiusSum * radiusSum;
            shapeA->convertVertexIdsToVerticesImpl(vertexIds, gskManifold.m_numVertsA, work.m_vertices);
            const int32_t numVertsA = gskManifold.m_numVertsA;
            // EXE-INLINED none: hkVector4Util::transformPoints (each point's _setTransformedPos)
            {
                const hkTransform& rTransform = *bodyA->m_motion.pMotion;
                const hkQuadReal col0 = rTransform.m_rotation.m_col0.m_quad;
                const hkQuadReal col1 = rTransform.m_rotation.m_col1.m_quad;
                const hkQuadReal col2 = rTransform.m_rotation.m_col2.m_quad;
                const hkQuadReal translation = rTransform.m_translation.m_quad;
                int32_t j = numVertsA - 1;
                do {
                    const hkQuadReal v = work.m_vertices[j].m_quad;
                    work.m_vertices[j].m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)), col1),
                            _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)), col0)
                        ),
                        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)), col2), translation)
                    );
                }
                while (--j >= 0);
            }
            hkpCdVertex* verticesB = &work.m_vertices[numVertsA];
            shapeB->convertVertexIdsToVerticesImpl(vertexIds + numVertsA, gskManifold.m_numVertsB, verticesB);
            // EXE-INLINED none: hkVector4Util::transformPoints (each point's _setTransformedPos)
            {
                const hkTransform& rTransform = *bodyB->m_motion.pMotion;
                const hkQuadReal col0 = rTransform.m_rotation.m_col0.m_quad;
                const hkQuadReal col1 = rTransform.m_rotation.m_col1.m_quad;
                const hkQuadReal col2 = rTransform.m_rotation.m_col2.m_quad;
                const hkQuadReal translation = rTransform.m_translation.m_quad;
                int32_t j = gskManifold.m_numVertsB - 1;
                do {
                    const hkQuadReal v = verticesB[j].m_quad;
                    verticesB[j].m_quad = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)), col1),
                            _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)), col0)
                        ),
                        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)), col2), translation)
                    );
                }
                while (--j >= 0);
            }
            hkGskManifold_verifyAndGetPoints(gskManifold, work, 0, result, input.m_contactMgr);
            const uint8_t numContactPoints = gskManifold.m_numContactPoints;
            if (numContactPoints) {
                hkpProcessCollisionOutput::PotentialInfo* potentialInfo = result.m_potentialContacts.m_storage;
                if (potentialInfo) {
                    *potentialInfo->m_firstFreeRepresentativeContact++ =
                        result.m_firstFreeContactPoint.m_storage - numContactPoints;
                }
            }
        }
        goto done;
    }

processCollision:
    hkGskAgentUtil_processCollisionNoTim(input, entry, agentData, gskCache, gskManifold, *separatingNormal, result);

done:
    entry->m_numContactPoints = gskManifold.m_numContactPoints;
    HK_TIMER_END();
    return static_cast<hkpAgentData*>(
        static_cast<uint8_t*>(agentData) + ((sizeof(hkpGskCache) + gskManifold.getTotalSizeInBytes() + 15) & ~15)
    );
}

// Stores this agent's own functions; the agent is predictive.
void hkPredGskAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
    f.m_createFunc = &hkPredGskAgent3::create;
    f.m_processFunc = &hkPredGskAgent3::process;
    f.m_sepNormalFunc = &hkPredGskAgent3::sepNormal;
    f.m_cleanupFunc = &hkPredGskAgent3::cleanup;
    f.m_removePointFunc = &hkPredGskAgent3::removePoint;
    f.m_commitPotentialFunc = &hkPredGskAgent3::commitPotential;
    f.m_createZombieFunc = &hkPredGskAgent3::createZombie;
    f.m_destroyFunc = &hkPredGskAgent3::destroy;
    f.m_isPredictive.m_bool = 1;
}

void hkPredGskAgent3::destroy(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    hkpCollisionDispatcher* dispatcher
) {
    cleanup(entry, agentData, mgr, constraintOwner);
}

// Havok SDK: the manifold's points leave the contact manager and the entry has none; returns the memory after the
// agent's data, the cache and the manifold rounded up to 16 bytes (the manifold's counts read after the cleanup).
hkpAgentData* hkPredGskAgent3::cleanup(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    hkPredGskAgent3Data* data = static_cast<hkPredGskAgent3Data*>(agentData);
    hkGskManifold_cleanup(data->m_gskManifold, mgr, constraintOwner);
    hkpAgentData* result = static_cast<hkpAgentData*>(
        static_cast<uint8_t*>(agentData) +
        ((sizeof(hkpGskCache) + data->m_gskManifold.getTotalSizeInBytes() + 15) & ~15)
    );
    entry->m_numContactPoints = 0;
    return result;
}

// Havok SDK: the manifold's point with the id loses it through hkGskManifold_removePoint; no such point, nothing.
void hkPredGskAgent3::removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove) {
    hkpGskManifold& manifold = static_cast<hkPredGskAgent3Data*>(agentData)->m_gskManifold;
    for (int32_t i = 0; i < manifold.m_numContactPoints; ++i) {
        if (manifold.m_contactPoints[i].m_id == idToRemove) {
            hkGskManifold_removePoint(manifold, i);
            return;
        }
    }
}

// Havok SDK: the manifold's first potential point (no id yet) takes the new id.
void hkPredGskAgent3::commitPotential(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToCommit) {
    hkpGskManifold& manifold = static_cast<hkPredGskAgent3Data*>(agentData)->m_gskManifold;
    for (int32_t i = 0; i < manifold.m_numContactPoints; ++i) {
        if (manifold.m_contactPoints[i].m_id == HK_INVALID_CONTACT_POINT) {
            manifold.m_contactPoints[i].m_id = idToCommit;
            return;
        }
    }
}

// Havok SDK: the manifold's point with the id becomes a zombie: its dimensions are cleared.
void hkPredGskAgent3::createZombie(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToConvert) {
    hkpGskManifold& manifold = static_cast<hkPredGskAgent3Data*>(agentData)->m_gskManifold;
    for (int32_t i = 0; i < manifold.m_numContactPoints; ++i) {
        hkpGskManifold::ContactPoint& point = manifold.m_contactPoints[i];
        if (point.m_id == idToConvert) {
            point.m_dimA = 0;
            point.m_dimB = 0;
            return;
        }
    }
}

// Havok SDK: GSK from the agent's cache gives the separating normal in A's space, which is rotated to world space by
// A's rotation; its w is the distance less both radii. A cache whose feature changed is written back.
void hkPredGskAgent3::sepNormal(
    const hkpAgent3Input& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4& separatingNormalOut
) {
    const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(input.m_bodyB->m_shape);
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(input.m_bodyA->m_shape);
    hkpGskCache& cache = static_cast<hkPredGskAgent3Data*>(agentData)->m_gskCache;

    // hkpGsk::init, with hkPadSpu's stores and loads written as plain field accesses.
    hkpGsk gsk;
    gsk.m_dimA.m_storage = cache.m_dimA;
    gsk.m_dimB.m_storage = cache.m_dimB;
    gsk.m_maxDimA.m_storage = cache.m_maxDimA;
    gsk.m_maxDimB.m_storage = cache.m_maxDimB;
    gsk.m_doNotHandlePenetration.m_storage = 0;
    gsk.m_featureChange.m_storage = 0;
    shapeA->convertVertexIdsToVerticesImpl(&cache.m_vertices[0], gsk.m_dimA.m_storage, &gsk.m_verticesA[0]);
    shapeB->convertVertexIdsToVerticesImpl(
        &cache.m_vertices[gsk.m_dimA.m_storage], gsk.m_dimB.m_storage, &gsk.m_verticesBinB[0]
    );

    hkVector4 separatingNormal;
    gsk.getClosestFeature(shapeA, shapeB, input.m_aTb, separatingNormal);
    if (gsk.m_featureChange.m_storage) {
        gsk.exitAndExportCacheImpl(cache);
    }
    float distance = separatingNormal.m_quad[3];
    // EXE-INLINED 0xC7F700
    const hkRotation& rotation = input.m_bodyA->m_motion.pMotion->m_rotation;
    hkQuadReal n = separatingNormal.m_quad;
    separatingNormalOut.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0x55), rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(n, n, 0x00), rotation.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(n, n, 0xAA), rotation.m_col2.m_quad)
    );
    separatingNormalOut.m_quad[3] = distance - (shapeB->m_radius + shapeA->m_radius);
}

// KEEP: hkpCollisionDispatcher::registerAgent3 (0xCFBD60) copies 0x40 bytes from f (rep movsd, 16 dwords) into the
// dispatcher's table, but f is 0x34 bytes with its last byte (+0x33) never written: the copy takes that byte and
// the twelve bytes of the frame past f, stack garbage src cannot reproduce.
void hkPredGskAgent3::registerAgent3(hkpCollisionDispatcher* a1, hkpShapeType a2, hkpShapeType a3) {
    alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
    // The three flags, then the five optional functions from m_updateFilterFunc to m_sepNormalFunc nulled.
    // EXE-INLINED none: hkpCollisionDispatcher::Agent3Funcs::Agent3Funcs
    f.m_isPredictive.m_bool = 0;
    f.m_ignoreSymmetricVersion.m_bool = 0;
    f.m_reusePreviousEntry.m_bool = 0;
    f.m_updateFilterFunc = nullptr;
    f.m_calcStatisticsFunc = nullptr;
    f.m_invalidateTimFunc = nullptr;
    f.m_warpTimeFunc = nullptr;
    f.m_sepNormalFunc = nullptr;
    hkPredGskAgent3::initAgentFunc(f);
    a1->registerAgent3(f, a2, a3);
}
