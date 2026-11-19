#include "Havok/Source/Physics/Internal/Collide/Gjk/Agent/hkpGskAgentUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Gjk/GskManifold/hkpGskManifoldUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

// The welding of a triangle shape's closest point (B first, else A with the normal negated) may move the point and the
// normal; a modified point disables the contact TIMs in the agent's cache. The vertices of the manifold are taken to
// world space in place, from the last one (a do-while).
void hkGskAgentUtil_processCollisionNoTim(
    const hkpAgent3Input& input,
    hkpAgentEntry* entry,
    void* agentData,
    hkpGskCache& gskCache,
    hkpGskManifold& gskManifold,
    hkVector4& separatingNormal,
    hkpProcessCollisionOutput& output
) {
    hkpGsk::GetClosesetPointInput gskInput;
    gskInput.m_shapeA.m_storage = static_cast<const hkpConvexShape*>(input.m_bodyA->m_shape);
    gskInput.m_shapeB.m_storage = static_cast<const hkpConvexShape*>(input.m_bodyB->m_shape);
    gskInput.m_aTb.m_storage = &input.m_aTb;
    gskInput.m_transformA.m_storage = input.m_bodyA->m_motion.pMotion;
    gskInput.m_collisionTolerance.m_storage = input.m_input->m_tolerance;
    // The agent's data starts with its hkpGskCache.
    hkpGskCache* agentCache = static_cast<hkpGskCache*>(agentData);
    agentCache->m_gskFlags &= static_cast<uint8_t>(~hkpGskCache::GSK_FLAGS_DISABLE_CONTACT_TIMS);
    hkVector4 pointOnB;
    if (hkpGsk::getClosestPoint(gskInput, gskCache, separatingNormal, pointOnB) == HK_FAILURE) {
        if (gskManifold.m_numContactPoints) {
            hkGskManifold_cleanup(gskManifold, input.m_contactMgr, *output.m_constraintOwner.m_storage);
        }
        return;
    }

    hkVector4 weldedNormal = separatingNormal;
    const hkpConvexShape* shapeA = gskInput.m_shapeA.m_storage;
    const hkpConvexShape* shapeB = gskInput.m_shapeB.m_storage;
    bool welded = true;
    int32_t weldResult = 0;
    if (shapeB->m_type.m_storage == HK_SHAPE_TRIANGLE) {
        weldResult = shapeB->weldContactPointImpl(
            &gskCache.m_vertices[gskCache.m_dimA], gskCache.m_dimB, pointOnB, input.m_bodyB->m_motion.pMotion, shapeA,
            input.m_bodyA->m_motion.pMotion, weldedNormal
        );
    }
    else if (
        shapeA->m_type.m_storage == HK_SHAPE_TRIANGLE &&
        static_cast<const hkpTriangleShape*>(shapeA)->m_weldingType.m_storage != hkpWeldingUtility::WELDING_TYPE_NONE
    ) {
        weldedNormal.setNeg3(weldedNormal);
        weldResult = shapeA->weldContactPointImpl(
            gskCache.m_vertices, gskCache.m_dimA, pointOnB, input.m_bodyA->m_motion.pMotion, shapeB,
            input.m_bodyB->m_motion.pMotion, weldedNormal
        );
        weldedNormal.setNeg3(weldedNormal);
    }
    else {
        welded = false;
    }
    if (welded) {
        if (weldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
            hkGskManifold_cleanup(gskManifold, input.m_contactMgr, *output.m_constraintOwner.m_storage);
            return;
        }
        if (weldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
            agentCache->m_gskFlags |= hkpGskCache::GSK_FLAGS_DISABLE_CONTACT_TIMS;
        }
    }

    const int32_t closestPointInManifold = hkGskManifold_doesPointExistAndResort(gskManifold, gskCache);
    hkpProcessCdPoint* const resultPointArray = output.m_firstFreeContactPoint.m_storage;
    const uint8_t numContactPoints = gskManifold.m_numContactPoints;
    if (numContactPoints > closestPointInManifold) {
        const hkpCdBody* bodyA = input.m_bodyA;
        const hkpCdBody* bodyB = input.m_bodyB;
        hkpGskManifoldWork work;
        work.m_keepContact.m_storage = input.m_input->m_tolerance;
        const hkpConvexShape* manifoldShapeA = static_cast<const hkpConvexShape*>(bodyA->m_shape);
        const hkpConvexShape* manifoldShapeB = static_cast<const hkpConvexShape*>(bodyB->m_shape);
        work.m_radiusA.m_storage = manifoldShapeA->m_radius;
        work.m_radiusB.m_storage = manifoldShapeB->m_radius;
        work.m_masterNormal = weldedNormal;
        const float radiusSum = work.m_radiusA.m_storage + work.m_keepContact.m_storage + work.m_radiusB.m_storage;
        work.m_radiusSumSqrd.m_storage = radiusSum * radiusSum;
        if (numContactPoints) {
            const uint16_t* vertexIds = gskManifold.getVertexIds();
            manifoldShapeA->convertVertexIdsToVerticesImpl(vertexIds, gskManifold.m_numVertsA, work.m_vertices);
            {
                const hkTransform& transformA = *bodyA->m_motion.pMotion;
                const int32_t numVertsA = gskManifold.m_numVertsA;
                int32_t i = numVertsA - 1;
                hkpCdVertex* vertex = &work.m_vertices[i];
                // Each vertex in place.
                do {
                    vertex->setTransformedPos(transformA, *vertex);
                    --i;
                    --vertex;
                }
                while (i >= 0);

                hkpCdVertex* verticesB = &work.m_vertices[numVertsA];
                manifoldShapeB->convertVertexIdsToVerticesImpl(
                    vertexIds + numVertsA, gskManifold.m_numVertsB, verticesB
                );
                const hkTransform& transformB = *bodyB->m_motion.pMotion;
                int32_t j = gskManifold.m_numVertsB - 1;
                vertex = &verticesB[j];
                // Each vertex in place.
                do {
                    vertex->setTransformedPos(transformB, *vertex);
                    --j;
                    --vertex;
                }
                while (j >= 0);
            }
        }
        hkGskManifold_verifyAndGetPoints(gskManifold, work, closestPointInManifold, output, input.m_contactMgr);
    }

    hkpProcessCdPoint* resultPoint = output.m_firstFreeContactPoint.m_storage;
    resultPoint->m_contact.m_position = pointOnB;
    resultPoint->m_contact.m_separatingNormal = weldedNormal;
    if (closestPointInManifold) {
        resultPoint->m_contactPointId.m_storage = gskManifold.m_contactPoints[0].m_id;
        ++output.m_firstFreeContactPoint.m_storage;
    }
    else {
        const hkpProcessCollisionInput* processInput = input.m_input;
        const hkpCollisionQualityInfo* qualityInfo = processInput->m_collisionQualityInfo;
        const float createContact =
            (gskCache.m_dimA + gskCache.m_dimB == 4) ? qualityInfo->m_create4dContact : qualityInfo->m_createContact;
        if (createContact > separatingNormal.m_quad[3]) {
            const hkpGskManifoldAddStatus addStatus = hkGskManifold_addPoint(
                gskManifold, *input.m_bodyA, *input.m_bodyB, *processInput, output, gskCache, resultPoint,
                resultPointArray, input.m_contactMgr,
                static_cast<hkpGskManifoldUtilMgrHandling>(processInput->m_enableDeprecatedWelding.m_bool != 0)
            );
            switch (addStatus) {
                case HK_GSK_MANIFOLD_POINT_ADDED:
                    if (resultPoint->m_contactPointId.m_storage != HK_INVALID_CONTACT_POINT) {
                        ++output.m_firstFreeContactPoint.m_storage;
                        break;
                    }
                    if (output.m_potentialContacts.m_storage && entry) {
                        if (input.m_contactMgr->reserveContactPoints(1) == HK_SUCCESS) {
                            hkpProcessCollisionOutput::ContactRef* ref =
                                output.m_potentialContacts.m_storage->m_firstFreePotentialContact++;
                            ref->m_agentEntry = entry;
                            ref->m_contactPoint = resultPoint;
                            ref->m_agentData = agentData;
                            ++output.m_firstFreeContactPoint.m_storage;
                            break;
                        }
                    }
                    else {
                        const uint16_t id = input.m_contactMgr->addContactPoint(
                            *input.m_bodyA, *input.m_bodyB, *input.m_input, output, &gskCache, resultPoint->m_contact
                        );
                        resultPoint->m_contactPointId.m_storage = id;
                        if (id != HK_INVALID_CONTACT_POINT) {
                            gskManifold.m_contactPoints[0].m_id = id;
                            ++output.m_firstFreeContactPoint.m_storage;
                            break;
                        }
                    }
                    hkGskManifold_removePoint(gskManifold, 0);
                    resultPoint = resultPointArray;
                    break;
                case HK_GSK_MANIFOLD_POINT_REJECTED: resultPoint = resultPointArray; break;
                case HK_GSK_MANIFOLD_TWO_POINT2_REJECTED:
                    --output.m_firstFreeContactPoint.m_storage;
                    resultPoint = resultPointArray;
                    break;
                default: resultPoint = &resultPointArray[addStatus]; break;
            }
        }
    }

    hkpProcessCollisionOutput::PotentialInfo* potentialInfo = output.m_potentialContacts.m_storage;
    if (potentialInfo && resultPoint < output.m_firstFreeContactPoint.m_storage) {
        *potentialInfo->m_firstFreeRepresentativeContact = resultPoint;
        ++output.m_potentialContacts.m_storage->m_firstFreeRepresentativeContact;
    }
}
