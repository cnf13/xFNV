#include "Havok/Source/Physics/Collide/Agent3/PredGskCylinderAgent3/hkpPredGskCylinderAgent3.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Gjk/GskManifold/hkpGskManifoldUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Agent/hkpGskAgentUtil.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// Each cylinder body of the pair is stood in for by a temporary body holding a capsule on the cylinder's vertices, the
// original kept in originalCdBodies.
void replaceCylindersWithCapsulesInCdBodies(
    const hkpCdBody** cdBodies[2],
    hkpCdBody* isReplacedWithCapsule,
    const int32_t* isCylinder,
    const hkpCdBody** originalCdBodies
) {
    for (int32_t i = 0; i < 2; ++i) {
        if (isCylinder[i]) {
            const hkpCylinderShape* pCylinder = static_cast<const hkpCylinderShape*>((*cdBodies[i])->m_shape);
            void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCapsuleShape));
            static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags =
                static_cast<uint16_t>(sizeof(hkpCapsuleShape));
            const float fConvexRadius = pCylinder->m_radius;
            // Adds 0.002f as a double constant, as the game does.
            const float fCapsuleRadius = pCylinder->getCylinderRadius() + fConvexRadius + 0.0020000000949949026;
            hkpCapsuleShape* pCapsule =
                ::new (pMemory) hkpCapsuleShape(pCylinder->m_vertexA, pCylinder->m_vertexB, fCapsuleRadius);
            // The hkpCdBody(parent) constructor, placed on the temporary body (its address is tested for null), then
            // setShape.
            hkpCdBody* pTemporary = &isReplacedWithCapsule[i];
            if (pTemporary) {
                pTemporary->m_parent = *cdBodies[i];
                pTemporary->m_motion = (*cdBodies[i])->m_motion;
            }
            pTemporary->m_shape = pCapsule;
            pTemporary->m_shapeKey = HK_INVALID_SHAPE_KEY;
            originalCdBodies[i] = *cdBodies[i];
            *cdBodies[i] = pTemporary;
        }
        else {
            originalCdBodies[i] = nullptr;
        }
    }
}

// Havok SDK: a file-local function of hkpPredGskCylinderAgent3.cpp (hkpCollide.lib's object of that file holds it
// between replaceCylindersWithCapsulesInCdBodies and process). The separating distance (w of hkPredGskAgent3's
// separating normal from a fresh GSK cache) of the two bodies, each body the agent data's GSK flags mark as a
// cylinder replaced for the call by a capsule over the cylinder's vertices, its radius the cylinder radius plus the
// convex radius plus 0.002.
float separatingDistanceUsingCapsuleRepresentations(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    void* agentData
) {
    hkpAgent3ProcessInput inputLocal = input;

    const hkpCdBody** cdBodiesLocal[2] = {&inputLocal.m_bodyA, &inputLocal.m_bodyB};
    // EXE-INLINED none: hkPredGskAgent3::getGskFlag
    const uint8_t gskFlags = static_cast<const hkpGskCache*>(agentData)->m_gskFlags;
    const int32_t isReplacedWithCapsuleLocal[2] = {
        gskFlags & hkpGskCache::GSK_FLAGS_SHAPE_A_IS_CYLINDER, gskFlags & hkpGskCache::GSK_FLAGS_SHAPE_B_IS_CYLINDER
    };
    const hkpCdBody* originalCdBodiesLocal[2];
    hkpCdBody temporaryCdBodiesLocal[2];

    // EXE-INLINED 0xD40A10
    for (int32_t i = 0; i < 2; ++i) {
        if (isReplacedWithCapsuleLocal[i]) {
            const hkpCdBody* pBody = *cdBodiesLocal[i];
            const hkpCylinderShape* pCylinder = static_cast<const hkpCylinderShape*>(pBody->m_shape);
            // EXE-INLINED 0x56D280
            void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCapsuleShape));
            static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags =
                static_cast<uint16_t>(sizeof(hkpCapsuleShape));
            const float fConvexRadius = pCylinder->m_radius;
            // Adds 0.002f as a double constant, as the game does.
            const float fCapsuleRadius = pCylinder->getCylinderRadius() + fConvexRadius + 0.0020000000949949026;
            hkpCapsuleShape* pCapsule =
                ::new (pMemory) hkpCapsuleShape(pCylinder->m_vertexA, pCylinder->m_vertexB, fCapsuleRadius);
            // The SDK's hkpCdBody(parent) constructor, then setShape.
            hkpCdBody& rTemporary = temporaryCdBodiesLocal[i];
            rTemporary.m_parent = pBody;
            rTemporary.m_motion = pBody->m_motion;
            rTemporary.m_shape = pCapsule;
            rTemporary.m_shapeKey = HK_INVALID_SHAPE_KEY;
            originalCdBodiesLocal[i] = *cdBodiesLocal[i];
            *cdBodiesLocal[i] = &rTemporary;
        }
        else {
            originalCdBodiesLocal[i] = nullptr;
        }
    }

    hkpGskCache emptyCache;
    emptyCache.init(
        static_cast<const hkpConvexShape*>(inputLocal.m_bodyA->m_shape),
        static_cast<const hkpConvexShape*>(inputLocal.m_bodyB->m_shape), input.m_aTb
    );
    hkVector4 separatingNormal;
    hkPredGskAgent3::sepNormal(inputLocal, entry, &emptyCache, separatingNormal);

    // EXE-INLINED 0xD409D0
    for (int32_t i = 0; i < 2; ++i) {
        if (originalCdBodiesLocal[i]) {
            (*cdBodiesLocal[i])->m_shape->removeReference();
            *cdBodiesLocal[i] = originalCdBodiesLocal[i];
        }
    }
    return separatingNormal(3);
}

// Puts back the two bodies a convex agent swapped for temporary ones, releasing each temporary's shape.
void restoreCdBodies(const hkpCdBody* const* originalCdBodies, const hkpCdBody** const* cdBodies) {
    for (int32_t i = 0; i < 2; ++i) {
        if (originalCdBodies[i]) {
            (*cdBodies[i])->m_shape->removeReference();
            *cdBodies[i] = originalCdBodies[i];
        }
    }
}

// The manifold's contact points go (hkGskManifold_cleanup, with the contact manager and the constraint owner), then the
// entry takes the manifold's point count; returns the entry.
hkpAgentEntry* cleanupManifold(
    hkpAgentEntry* entry,
    hkpGskManifold& manifold,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    hkGskManifold_cleanup(manifold, mgr, constraintOwner);
    entry->m_numContactPoints = manifold.m_numContactPoints;
    return entry;
}

// hkPredGskCylinderAgent3::process's cache conversion: a capsule has two vertices, ids 0 and 16; each body replaced by
// a capsule takes its cylinder's vertex id over to the capsule's end on the same side (bit 7 of the id), and a
// cylinder's face or edge to the capsule's segment.
void convertCapsuleCache(const int32_t* isReplacedWithCapsule, hkpGskCache* cache) {
    if (isReplacedWithCapsule[0]) {
        const uint8_t dimA = cache->m_dimA;
        cache->m_vertices[0] = static_cast<uint16_t>((1 - ((cache->m_vertices[0] >> 7) & 1)) << 4);
        if (dimA > 1) {
            cache->m_vertices[0] = 16;
            cache->m_vertices[1] = 0;
            if (dimA == 3) {
                cache->m_dimA = 2;
                cache->m_vertices[2] = cache->m_vertices[3];
            }
        }
    }
    if (isReplacedWithCapsule[1]) {
        uint16_t& rFirstB = cache->m_vertices[cache->m_dimA];
        rFirstB = static_cast<uint16_t>((1 - ((rFirstB >> 7) & 1)) << 4);
        if (cache->m_dimB > 1) {
            cache->m_vertices[cache->m_dimA] = 16;
            cache->m_vertices[cache->m_dimA + 1] = 0;
            if (cache->m_dimB == 3) {
                cache->m_dimB = 2;
            }
        }
    }
}

// KEEP: partcopy. hkpCollisionDispatcher::registerAgent3 (0xCFBD60) copies all 0x40 bytes of f (rep movsd) into the
// dispatcher's table, and f's bytes 0x33-0x3F (the tail padding after m_reusePreviousEntry) are never written here:
// stack garbage src cannot reproduce. The Agent3Funcs constructor is
// inlined (the flags and the optional functions cleared, the four after m_createZombieFunc as one 16-byte clear).
void hkPredGskCylinderAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
    alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
    // EXE-INLINED none: hkpCollisionDispatcher::Agent3Funcs::Agent3Funcs
    f.m_reusePreviousEntry.m_bool = 0;
    f.m_updateFilterFunc = nullptr;
    f.m_calcStatisticsFunc = nullptr;
    f.m_invalidateTimFunc = nullptr;
    f.m_warpTimeFunc = nullptr;
    f.m_createFunc = &hkPredGskCylinderAgent3::create;
    f.m_processFunc = &hkPredGskCylinderAgent3::process;
    f.m_sepNormalFunc = &hkPredGskAgent3::sepNormal;
    f.m_cleanupFunc = &hkPredGskAgent3::cleanup;
    f.m_removePointFunc = &hkPredGskAgent3::removePoint;
    f.m_commitPotentialFunc = &hkPredGskAgent3::commitPotential;
    f.m_createZombieFunc = &hkPredGskAgent3::createZombie;
    f.m_destroyFunc = &hkPredGskAgent3::destroy;
    f.m_isPredictive.m_bool = 1;
    f.m_ignoreSymmetricVersion.m_bool = 1;
    dispatcher->registerAgent3(f, HK_SHAPE_CYLINDER, HK_SHAPE_CONVEX);
    f.m_reusePreviousEntry.m_bool = 1;
    dispatcher->registerAgent3(f, HK_SHAPE_CONVEX, HK_SHAPE_CYLINDER);
}

// Havok SDK: hkPredGskAgent3's create, then the cache's cylinder flags set for each body whose shape is a cylinder
// (marked as such and to be replaced by a capsule).
hkpAgentData*
hkPredGskCylinderAgent3::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
    hkpAgentData* result = hkPredGskAgent3::create(input, entry, agentData);
    hkpGskCache* cache = static_cast<hkpGskCache*>(agentData);
    cache->m_gskFlags &= ~hkpGskCache::GSK_FLAGS_CYLINDER_AGENT_FLAGS;
    if (input.m_bodyA->m_shape->m_type.m_storage == HK_SHAPE_CYLINDER) {
        cache->m_gskFlags |=
            hkpGskCache::GSK_FLAGS_SHAPE_A_IS_CYLINDER | hkpGskCache::GSK_FLAGS_REPLACE_SHAPE_A_WITH_CAPSULE;
    }
    if (input.m_bodyB->m_shape->m_type.m_storage == HK_SHAPE_CYLINDER) {
        cache->m_gskFlags |=
            hkpGskCache::GSK_FLAGS_SHAPE_B_IS_CYLINDER | hkpGskCache::GSK_FLAGS_REPLACE_SHAPE_B_WITH_CAPSULE;
    }
    return result;
}

// hkPredGskAgent3's process with each cylinder of the pair stood in for by a capsule while the cache's flags say so.
// First each cylinder's representation is switched by the angle between its axis and the separating normal (the
// capsule's distance decides). Then, as hkPredGskAgent3: the TOI check under continuous physics, else the manifold's
// points are reused while the bodies stay within the TIM distance, else the full GSK collision on a copy of the cache
// whose cylinder vertex ids are made the capsule's. Returns the free memory after the agent's data.
hkpAgentData* hkPredGskCylinderAgent3::process(
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
    // The SDK's own cast: a capsule body stands in for a cylinder's through the input's body pointers while the
    // manifold is processed.
    const hkpCdBody** cdBodies[2] = {
        const_cast<const hkpCdBody**>(&input.m_bodyA), const_cast<const hkpCdBody**>(&input.m_bodyB)
    };
    // EXE-INLINED none: hkPredGskAgent3::getGskFlag
    const uint8_t gskFlags = gskCache.m_gskFlags;
    const int32_t isCylinder[2] = {
        gskFlags & hkpGskCache::GSK_FLAGS_SHAPE_A_IS_CYLINDER, gskFlags & hkpGskCache::GSK_FLAGS_SHAPE_B_IS_CYLINDER
    };
    int32_t isReplacedWithCapsule[2] = {
        gskFlags & hkpGskCache::GSK_FLAGS_REPLACE_SHAPE_A_WITH_CAPSULE,
        gskFlags & hkpGskCache::GSK_FLAGS_REPLACE_SHAPE_B_WITH_CAPSULE
    };
    const hkpCdBody* originalCdBodies[2] = {nullptr, nullptr};
    hkpCdBody temporaryCdBodies[2];
    hkVector4 separatingNormalOut;
    hkPredGskAgent3::sepNormal(input, entry, agentData, separatingNormalOut);

    for (int32_t i = 0; i < 2; ++i) {
        if (!isCylinder[i]) {
            continue;
        }
        const hkpCylinderShape* cylinder = static_cast<const hkpCylinderShape*>((*cdBodies[i])->m_shape);
        hkVector4 axis;
        axis.m_quad = _mm_sub_ps(cylinder->m_vertexB.m_quad, cylinder->m_vertexA.m_quad);
        axis.normalize3();
        axis.setRotatedDir((*cdBodies[i])->m_motion.pMotion->getRotation(), axis);
        const float cosAngle = std::fabs(static_cast<float>(separatingNormalOut.dot3(axis)));
        if (isReplacedWithCapsule[i]) {
            if (cosAngle > 0.10000000149011612 &&
                separatingDistanceUsingCapsuleRepresentations(input, entry, agentData) < 0.0f) {
                isReplacedWithCapsule[i] = 0;
                gskCache.m_gskFlags &= ~(hkpGskCache::GSK_FLAGS_REPLACE_SHAPE_A_WITH_CAPSULE << i);
                hkGskManifold_cleanup(gskManifold, input.m_contactMgr, *result.m_constraintOwner.m_storage);
                entry->m_numContactPoints = gskManifold.m_numContactPoints;
            }
        }
        else if (
            cosAngle < 0.05000000074505806 ||
            separatingDistanceUsingCapsuleRepresentations(input, entry, agentData) > 0.01f
        ) {
            isReplacedWithCapsule[i] = 1;
            gskCache.m_gskFlags |= hkpGskCache::GSK_FLAGS_REPLACE_SHAPE_A_WITH_CAPSULE << i;
            hkGskManifold_cleanup(gskManifold, input.m_contactMgr, *result.m_constraintOwner.m_storage);
            entry->m_numContactPoints = gskManifold.m_numContactPoints;
        }
    }

    const hkpProcessCollisionInput* processInput = input.m_input;
    const hkpCollisionQualityInfo& qualityInfo = *processInput->m_collisionQualityInfo;
    if (qualityInfo.m_useContinuousPhysics.m_storage) {
        const float depthA = input.m_bodyA->getRootCollidable()->m_allowedPenetrationDepth;
        const float depthB = input.m_bodyB->getRootCollidable()->m_allowedPenetrationDepth;
        const float allowedPenetrationDepth = hkMath::min2(depthA, depthB);
        const float distAtT0 = (*separatingNormal)(3);
        const float minSeparationA = qualityInfo.m_minSeparation * allowedPenetrationDepth;
        const float minSeparationB = distAtT0 + qualityInfo.m_minExtraSeparation * allowedPenetrationDepth;
        const float minSeparation = hkMath::min2(minSeparationA, minSeparationB);
        // The TOI is searched unless distAtT1 reaches minSeparation (a NaN searches it).
        if (!(minSeparation <= distAtT1)) {
            const float toiSeparationA = qualityInfo.m_toiSeparation * allowedPenetrationDepth;
            const float toiSeparationB = distAtT0 + qualityInfo.m_toiExtraSeparation * allowedPenetrationDepth;
            const float toiSeparation = hkMath::min2(toiSeparationA, toiSeparationB);
            hk4dGskCollideCalcToi(
                input, allowedPenetrationDepth, minSeparation, toiSeparation, gskCache, *separatingNormal, result
            );
            goto processManifold;
        }
    }

    // The bodies stay within the TIM distance: the manifold's points are reused, moved to the bodies' transforms.
    if (qualityInfo.m_manifoldTimDistance < distAtT1 &&
        !(gskCache.m_gskFlags & hkpGskCache::GSK_FLAGS_DISABLE_CONTACT_TIMS) &&
        processInput->m_tolerance > separatingNormalOut(3)) {
        (*separatingNormal)(3) = distAtT1;
        if (gskManifold.m_numContactPoints) {
            // EXE-INLINED 0xD40A10
            for (int32_t i = 0; i < 2; ++i) {
                if (isReplacedWithCapsule[i]) {
                    const hkpCdBody* pBody = *cdBodies[i];
                    const hkpCylinderShape* pCylinder = static_cast<const hkpCylinderShape*>(pBody->m_shape);
                    void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCapsuleShape));
                    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags =
                        static_cast<uint16_t>(sizeof(hkpCapsuleShape));
                    const float fConvexRadius = pCylinder->m_radius;
                    // Adds 0.002f as a double constant, as the game does.
                    const float fCapsuleRadius = pCylinder->getCylinderRadius() + fConvexRadius + 0.0020000000949949026;
                    hkpCapsuleShape* pCapsule =
                        ::new (pMemory) hkpCapsuleShape(pCylinder->m_vertexA, pCylinder->m_vertexB, fCapsuleRadius);
                    // The SDK's hkpCdBody(parent) constructor, then setShape.
                    hkpCdBody& rTemporary = temporaryCdBodies[i];
                    rTemporary.m_parent = *cdBodies[i];
                    rTemporary.m_motion = (*cdBodies[i])->m_motion;
                    rTemporary.m_shape = pCapsule;
                    rTemporary.m_shapeKey = HK_INVALID_SHAPE_KEY;
                    originalCdBodies[i] = *cdBodies[i];
                    *cdBodies[i] = &rTemporary;
                }
                else {
                    originalCdBodies[i] = nullptr;
                }
            }

            hkpGskManifoldWork work;
            work.m_keepContact.m_storage = input.m_input->m_tolerance;
            const hkpCdBody* bodyA = input.m_bodyA;
            const hkpCdBody* bodyB = input.m_bodyB;
            const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(bodyA->m_shape);
            const hkpConvexShape* shapeB = static_cast<const hkpConvexShape*>(bodyB->m_shape);
            work.m_radiusA.m_storage = shapeA->m_radius;
            work.m_radiusB.m_storage = shapeB->m_radius;
            work.m_masterNormal = *separatingNormal;
            const float radiusSum = work.m_keepContact.m_storage + work.m_radiusA.m_storage + work.m_radiusB.m_storage;
            work.m_radiusSumSqrd.m_storage = radiusSum * radiusSum;
            if (gskManifold.m_numContactPoints) {
                const uint16_t* vertexIds = gskManifold.getVertexIds();
                shapeA->convertVertexIdsToVerticesImpl(vertexIds, gskManifold.m_numVertsA, work.m_vertices);
                const int32_t numVertsA = gskManifold.m_numVertsA;
                hkVector4Util::transformPoints(*bodyA->m_motion.pMotion, work.m_vertices, numVertsA, work.m_vertices);
                hkpCdVertex* verticesB = &work.m_vertices[numVertsA];
                shapeB->convertVertexIdsToVerticesImpl(vertexIds + numVertsA, gskManifold.m_numVertsB, verticesB);
                hkVector4Util::transformPoints(*bodyB->m_motion.pMotion, verticesB, gskManifold.m_numVertsB, verticesB);
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
        goto restoreBodies;
    }

processManifold:
    if (input.m_input->m_tolerance > separatingNormalOut(3)) {
        // EXE-INLINED 0xD40A10
        for (int32_t i = 0; i < 2; ++i) {
            if (isReplacedWithCapsule[i]) {
                const hkpCdBody* pBody = *cdBodies[i];
                const hkpCylinderShape* pCylinder = static_cast<const hkpCylinderShape*>(pBody->m_shape);
                void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCapsuleShape));
                static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags =
                    static_cast<uint16_t>(sizeof(hkpCapsuleShape));
                const float fConvexRadius = pCylinder->m_radius;
                // Adds 0.002f as a double constant, as the game does.
                const float fCapsuleRadius = pCylinder->getCylinderRadius() + fConvexRadius + 0.0020000000949949026;
                hkpCapsuleShape* pCapsule =
                    ::new (pMemory) hkpCapsuleShape(pCylinder->m_vertexA, pCylinder->m_vertexB, fCapsuleRadius);
                // The SDK's hkpCdBody(parent) constructor, then setShape.
                hkpCdBody& rTemporary = temporaryCdBodies[i];
                rTemporary.m_parent = *cdBodies[i];
                rTemporary.m_motion = (*cdBodies[i])->m_motion;
                rTemporary.m_shape = pCapsule;
                rTemporary.m_shapeKey = HK_INVALID_SHAPE_KEY;
                originalCdBodies[i] = *cdBodies[i];
                *cdBodies[i] = &rTemporary;
            }
            else {
                originalCdBodies[i] = nullptr;
            }
        }

        // A capsule has two vertices, ids 0 and 16: a copy of the cache takes each cylinder's vertex ids over to
        // the capsule's end on the same side (bit 7 of the id), and a cylinder's face or edge to the capsule's
        // segment.
        hkpGskCache* cache = &gskCache;
        hkpGskCache capsuleCache;
        if (isReplacedWithCapsule[0] || isReplacedWithCapsule[1]) {
            capsuleCache = gskCache;
            cache = &capsuleCache;
            if (isReplacedWithCapsule[0]) {
                capsuleCache.m_vertices[0] = static_cast<uint16_t>((1 - ((capsuleCache.m_vertices[0] >> 7) & 1)) << 4);
                if (capsuleCache.m_dimA > 1) {
                    capsuleCache.m_vertices[0] = 16;
                    capsuleCache.m_vertices[1] = 0;
                    if (capsuleCache.m_dimA == 3) {
                        capsuleCache.m_dimA = 2;
                        capsuleCache.m_vertices[2] = capsuleCache.m_vertices[3];
                    }
                }
            }
            if (isReplacedWithCapsule[1]) {
                uint16_t& rFirstB = capsuleCache.m_vertices[capsuleCache.m_dimA];
                rFirstB = static_cast<uint16_t>((1 - ((rFirstB >> 7) & 1)) << 4);
                if (capsuleCache.m_dimB > 1) {
                    capsuleCache.m_vertices[capsuleCache.m_dimA] = 16;
                    capsuleCache.m_vertices[capsuleCache.m_dimA + 1] = 0;
                    if (capsuleCache.m_dimB == 3) {
                        capsuleCache.m_dimB = 2;
                    }
                }
            }
        }
        hkGskAgentUtil_processCollisionNoTim(input, entry, agentData, *cache, gskManifold, *separatingNormal, result);
    }
    else if (gskManifold.m_numContactPoints) {
        hkGskManifold_cleanup(gskManifold, input.m_contactMgr, *result.m_constraintOwner.m_storage);
    }
    *separatingNormal = separatingNormalOut;

restoreBodies:
    // EXE-INLINED 0xD409D0
    for (int32_t i = 0; i < 2; ++i) {
        if (originalCdBodies[i]) {
            (*cdBodies[i])->m_shape->removeReference();
            *cdBodies[i] = originalCdBodies[i];
        }
    }
    entry->m_numContactPoints = gskManifold.m_numContactPoints;
    HK_TIMER_END();
    return static_cast<hkpAgentData*>(
        static_cast<uint8_t*>(agentData) + ((sizeof(hkpGskCache) + gskManifold.getTotalSizeInBytes() + 15) & ~15)
    );
}
