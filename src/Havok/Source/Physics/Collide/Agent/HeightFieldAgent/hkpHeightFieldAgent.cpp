#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkpHeightFieldAgent.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkHeightFieldRayForwardingCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "xfnv/Compiler.h"

// With a contact manager, one invalid contact point id per collision sphere of body A's shape.
hkpHeightFieldAgent::hkpHeightFieldAgent(
    const hkpCdBody& A,
    const hkpCdBody& B,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
)
  : hkpCollisionAgent(mgr) {
    if (mgr) {
        const hkpSphereRepShape* csShape = static_cast<const hkpSphereRepShape*>(A.m_shape);
        const int32_t numSpheres = csShape->getNumCollisionSpheresImpl();
        m_contactPointId.reserve(numSpheres);
        for (int32_t i = 0; i < numSpheres; ++i) {
            m_contactPointId.m_data[i] = 0xFFFF;
        }
        m_contactPointId.m_size = numSpheres;
    }
}

// m_contactPointId frees its buffer in its own destructor.
hkpHeightFieldAgent::~hkpHeightFieldAgent() {}

// The agent keeps no state for the query: the static version does the work.
void hkpHeightFieldAgent::getPenetrations(
    const hkpCdBody& csBody,
    const hkpCdBody& hfBody,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    staticGetPenetrations(csBody, hfBody, input, collector);
}

// The agent keeps no state for the query: the static version does the work.
void hkpHeightFieldAgent::getClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    staticGetClosestPoints(bodyA, bodyB, input, collector);
}

void hkpHeightFieldAgent::linearCast(
    const hkpCdBody& csBody,
    const hkpCdBody& hfBody,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    staticLinearCast(csBody, hfBody, input, collector, startCollector);
}

// The SDK's processCollision: body A's collision spheres moved into the height field's space and collided with it in
// one call; then, per sphere, a contact point kept or added where its distance is within the tolerance, else removed.
void hkpHeightFieldAgent::processCollision(
    const hkpCdBody& csBody,
    const hkpCdBody& hfBody,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& result
) {
    HK_TIMER_BEGIN("HeightField", nullptr);
    const hkpSphereRepShape* csShape = static_cast<const hkpSphereRepShape*>(csBody.m_shape);
    const hkpHeightFieldShape* hfShape = static_cast<const hkpHeightFieldShape*>(hfBody.m_shape);

    alignas(16) hkTransform bTa;
    bTa.setMulInverseMul(*hfBody.m_motion.pMotion, *csBody.m_motion.pMotion);

    const int32_t nSpheres = m_contactPointId.m_size;
    {
        hkLocalBuffer<hkSphere> kSphereBuffer(nSpheres, nullptr);
        hkSphere* sphereBuffer = kSphereBuffer.begin();
        uint16_t* cpId = m_contactPointId.m_data;
        const hkSphere* spheres = csShape->getCollisionSpheresImpl(sphereBuffer);

        // Each sphere's centre moved into the height field's space, its radius kept. The loop tests its counter only at
        // the end, so it deliberately runs once with no spheres too.
        int32_t i = nSpheres - 1;
        do {
            const hkQuadReal s = spheres[i].m_pos.m_quad;
            hkVector4 p;
            p.setTransformedPos(bTa, spheres[i].m_pos);
            // x, y and z of the moved centre, w (the radius) of the sphere.
            sphereBuffer[i].m_pos.m_quad = _mm_shuffle_ps(p.m_quad, _mm_unpackhi_ps(p.m_quad, s), 0xC4);
        }
        while (--i >= 0);

        hkLocalBuffer<hkVector4> kOutputs(nSpheres, nullptr);
        hkVector4* outputs = kOutputs.begin();
        hkpHeightFieldShape::CollideSpheresInput in;
        in.m_spheres = sphereBuffer;
        in.m_numSpheres = nSpheres;
        in.m_tolerance = input.m_tolerance;
        hfShape->collideSpheres(in, outputs);

        const float tolerance = input.m_tolerance;
        const hkSphere* sphere = sphereBuffer;
        const hkVector4* output = outputs;
        for (int32_t j = nSpheres - 1; j >= 0; --j, ++sphere, ++output, ++cpId) {
            if (tolerance < (*output)(3)) {
                if (*cpId != 0xFFFF) {
                    m_contactMgr->removeContactPointImpl(*cpId, *result.m_constraintOwner.m_storage);
                    *cpId = 0xFFFF;
                }
                continue;
            }
            hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
            // The point on the sphere's surface nearest the height field, in world space.
            const float negRadius = -sphere->m_pos(3);
            const hkQuadReal r = _mm_set_ss(negRadius);
            const hkQuadReal o = output->m_quad;
            hkVector4 cpPos;
            cpPos.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0), o), sphere->m_pos.m_quad);
            const hkTransform& hfTransform = hfBody.m_motion.pState->m_transform;
            point->m_contact.m_position.setTransformedPos(hfTransform, cpPos);
            point->m_contact.m_separatingNormal._setRotatedDir(hfBody.m_motion.pState->m_transform.m_rotation, *output);
            // The distance.
            XFNV_M128_F32(point->m_contact.m_separatingNormal.m_quad, 3) = (*output)(3);
            if (*cpId == 0xFFFF) {
                const uint16_t id = m_contactMgr
                                        ->addContactPointImpl(csBody, hfBody, input, result, nullptr, point->m_contact);
                *cpId = id;
                if (id != 0xFFFF) {
                    ++result.m_firstFreeContactPoint.m_storage;
                }
            }
            else {
                ++result.m_firstFreeContactPoint.m_storage;
            }
            point->m_contactPointId.m_storage = *cpId;
        }
    }
    HK_TIMER_END();
}

void hkpHeightFieldAgent::cleanup(hkpConstraintOwner& constraintOwner) {
    if (m_contactMgr) {
        for (int32_t i = 0; i < m_contactPointId.m_size; ++i) {
            int32_t v4 = m_contactPointId.m_data[i];
            if (static_cast<uint16_t>(v4) != 0xFFFF) {
                m_contactMgr->removeContactPointImpl(v4, constraintOwner);
            }
        }
    }
    delete this;
}

// Collides bodyA's collision spheres, moved into bodyB's space, with bodyB's height field and reports the pair to
// collector as soon as one sphere penetrates the field (its output distance is below zero).
void hkpHeightFieldAgent::staticGetPenetrations(
    const hkpCdBody& csBody,
    const hkpCdBody& hfBody,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    HK_TIMER_BEGIN_LIST("HeightField", "GetSpheres");
    const hkpSphereRepShape* csShape = static_cast<const hkpSphereRepShape*>(csBody.m_shape);
    const hkpHeightFieldShape* hfShape = static_cast<const hkpHeightFieldShape*>(hfBody.m_shape);

    alignas(16) hkTransform bTa;
    bTa.setMulInverseMul(*hfBody.m_motion.pMotion, *csBody.m_motion.pMotion);

    const int32_t nSpheres = csShape->getNumCollisionSpheresImpl();
    {
        hkLocalBuffer<hkSphere> kSphereBuffer(nSpheres, nullptr);
        hkSphere* sphereBuffer = kSphereBuffer.begin();
        const hkSphere* spheres = csShape->getCollisionSpheresImpl(sphereBuffer);

        // The SDK's do-while: it runs once even for no spheres.
        hkVector4Util::transformSpheres(bTa, &spheres->m_pos, nSpheres, &sphereBuffer->m_pos);

        HK_TIMER_SPLIT_LIST("Collide");
        hkpHeightFieldShape::CollideSpheresInput collideInput;
        collideInput.m_tolerance = input.m_tolerance;
        hkLocalBuffer<hkVector4> kOutputBuffer(nSpheres, nullptr);
        collideInput.m_numSpheres = nSpheres;
        collideInput.m_spheres = sphereBuffer;
        hkVector4* outputBuffer = kOutputBuffer.begin();
        hfShape->collideSpheres(collideInput, outputBuffer);

        HK_TIMER_SPLIT_LIST("Examine");
        const hkVector4* output = outputBuffer;
        for (int32_t i = nSpheres - 1; i >= 0; --i) {
            if ((*output)(3) < 0.0f) {
                collector.addCdBodyPair(csBody, hfBody);
                break;
            }
            ++output;
        }
    }
    HK_TIMER_END_LIST();
}

// Gets the closest points between bodyA's collision spheres and bodyB's height field: the spheres, moved into bodyB's
// space, go to the field's collideSpheres, and each sphere whose output distance is within the tolerance becomes a
// contact point (in world space) for collector.
void hkpHeightFieldAgent::staticGetClosestPoints(
    const hkpCdBody& csBody,
    const hkpCdBody& hfBody,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    HK_TIMER_BEGIN_LIST("HeightField", "bTA");
    const hkpSphereRepShape* csShape = static_cast<const hkpSphereRepShape*>(csBody.m_shape);
    const hkpHeightFieldShape* hfShape = static_cast<const hkpHeightFieldShape*>(hfBody.m_shape);

    alignas(16) hkTransform bTa;
    bTa.setMulInverseMul(*hfBody.m_motion.pMotion, *csBody.m_motion.pMotion);

    const int32_t nSpheres = csShape->getNumCollisionSpheresImpl();
    {
        hkLocalBuffer<hkSphere> kSphereBuffer(nSpheres, nullptr);
        hkSphere* sphereBuffer = kSphereBuffer.begin();

        HK_TIMER_SPLIT_LIST("getSpheres");
        const hkSphere* spheres = csShape->getCollisionSpheresImpl(sphereBuffer);

        HK_TIMER_SPLIT_LIST("transform");
        // The SDK's do-while: it runs once even for no spheres.
        hkVector4Util::transformSpheres(bTa, &spheres->m_pos, nSpheres, &sphereBuffer->m_pos);

        HK_TIMER_SPLIT_LIST("collide");
        hkpHeightFieldShape::CollideSpheresInput collideInput;
        collideInput.m_tolerance = input.m_tolerance;
        hkLocalBuffer<hkVector4> kOutputBuffer(nSpheres, nullptr);
        collideInput.m_numSpheres = nSpheres;
        collideInput.m_spheres = sphereBuffer;
        hkVector4* outputBuffer = kOutputBuffer.begin();
        hfShape->collideSpheres(collideInput, outputBuffer);

        HK_TIMER_SPLIT_LIST("examine");
        const float tolerance = input.m_tolerance;
        const hkVector4* output = outputBuffer;
        const hkSphere* sphere = sphereBuffer;
        for (int32_t i = nSpheres - 1; i >= 0; --i) {
            if ((*output)(3) <= tolerance) {
                // The contact point in bodyB's space: the sphere's centre moved along the output normal by minus its
                // radius and distance.
                const float scale = -sphere->m_pos(3) - (*output)(3);
                const hkQuadReal normal = output->m_quad;
                hkVector4 pointInB;
                pointInB.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), normal), sphere->m_pos.m_quad);

                hkpCdPoint event;
                event.m_cdBodyA = &csBody;
                event.m_cdBodyB = &hfBody;
                const hkTransform& transformB = *hfBody.m_motion.pMotion;
                event.m_contact.m_position.setTransformedPos(transformB, pointInB);
                event.m_contact.m_separatingNormal.setRotatedDir(transformB.m_rotation, *output);
                event.m_contact.m_separatingNormal(3) = (*output)(3);
                collector.addCdPoint(event);
            }
            ++output;
            ++sphere;
        }
    }
    HK_TIMER_END_LIST();
}

// Sweeps bodyA's collision spheres along input.m_path over bodyB's height field: each sphere, moved into the field's
// space, is cast as a ray from its centre, and rayCollector turns the field's hits into contact points for collector.
// startCollector, when given, first gets the closest points at the start of the sweep.
void hkpHeightFieldAgent::staticLinearCast(
    const hkpCdBody& csBody,
    const hkpCdBody& hfBody,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    HK_TIMER_BEGIN_LIST("HeightField", "ClosestPoints");
    if (startCollector) {
        staticGetClosestPoints(csBody, hfBody, input, *startCollector);
    }

    HK_TIMER_SPLIT_LIST("GetSpheres");
    const hkpSphereRepShape* csShape = static_cast<const hkpSphereRepShape*>(csBody.m_shape);
    const hkpHeightFieldShape* hfShape = static_cast<const hkpHeightFieldShape*>(hfBody.m_shape);

    alignas(16) hkTransform bTa;
    bTa.setMulInverseMul(*hfBody.m_motion.pMotion, *csBody.m_motion.pMotion);

    const int32_t nSpheres = csShape->getNumCollisionSpheresImpl();
    {
        hkLocalBuffer<hkSphere> buffer(nSpheres, nullptr);
        alignas(16) hkVector4 pathLocal;
        pathLocal._setRotatedInverseDir(hfBody.m_motion.pMotion->m_rotation, input.m_path);
        const hkSphere* spheres = csShape->getCollisionSpheresImpl(buffer.begin());

        HK_TIMER_SPLIT_LIST("CastSpheres");
        alignas(16) hkpHeightFieldShape::hkpSphereCastInput ray;
        ray.m_maxExtraPenetration = input.m_maxExtraPenetration;

        hkHeightFieldRayForwardingCollector rayCollector(input.m_path, csBody, collector);

        for (int32_t i = 0; i < nSpheres; ++i) {
            // The SDK's hkSphere::getRadius, inline: the w component.
            ray.m_radius = spheres->m_pos(3);
            ray.m_from.setTransformedPos(bTa, spheres->m_pos);
            ray.m_to.setAdd4(ray.m_from, pathLocal);
            const hkTransform& aTransform = *csBody.m_motion.pMotion;
            rayCollector.m_currentRadius = spheres->m_pos(3);
            rayCollector.m_currentFrom.setTransformedPos(aTransform, spheres->m_pos);
            hfShape->castSphere(ray, hfBody, rayCollector);
            ++spheres;
        }
    }
    HK_TIMER_END_LIST();
}

// Registers the agent for a height field against a sphere-rep shape and the other way. Only two flag bytes of f are
// written, and registerCollisionAgent copies the whole flags word, so the two padding bytes after m_isPredictive, never
// written, land in the dispatcher's table.
void hkpHeightFieldAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
    hkpCollisionDispatcher::AgentFuncs f;
    f.m_createFunc = &hkpHeightFieldAgent::createHeightFieldAAgent;
    f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpHeightFieldAgent>::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpHeightFieldAgent>::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpHeightFieldAgent>::staticLinearCast;
    f.m_isFlipped.m_bool = 1;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_HEIGHT_FIELD, HK_SHAPE_SPHERE_REP);
    f.m_createFunc = &hkpHeightFieldAgent::createHeightFieldBAgent;
    f.m_getPenetrationsFunc = &hkpHeightFieldAgent::staticGetPenetrations;
    f.m_getClosestPointFunc = &hkpHeightFieldAgent::staticGetClosestPoints;
    f.m_linearCastFunc = &hkpHeightFieldAgent::staticLinearCast;
    f.m_isFlipped.m_bool = 0;
    f.m_isPredictive.m_bool = 1;
    dispatcher->registerCollisionAgent(f, HK_SHAPE_SPHERE_REP, HK_SHAPE_HEIGHT_FIELD);
}

hkpCollisionAgent* hkpHeightFieldAgent::createHeightFieldAAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new hkpSymmetricAgent<hkpHeightFieldAgent>(bodyA, bodyB, input, mgr);
}

hkpCollisionAgent* hkpHeightFieldAgent::createHeightFieldBAgent(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr
) {
    return new hkpHeightFieldAgent(bodyA, bodyB, input, mgr);
}

// A ray hit as a contact point: the hit on the ray from m_currentFrom along m_path, moved back by the sphere's radius
// against the hit normal turned into world space, with the hit fraction as its distance.
void hkHeightFieldRayForwardingCollector::addRayHit(
    const hkpCdBody& cdBody,
    const hkpShapeRayCastCollectorOutput& hitInfo
) {
    hkpCdPoint point;
    point.m_cdBodyA = &m_csBody;
    const float negRadius = -m_currentRadius;
    point.m_cdBodyB = &cdBody;
    const hkQuadReal fraction = _mm_set_ss(hitInfo.m_hitFraction);
    const hkQuadReal pos = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(fraction, fraction, 0), m_path.m_quad),
        m_currentFrom.m_quad
    );
    point.m_contact.m_position.m_quad = pos;
    point.m_contact.m_separatingNormal._setRotatedDir(cdBody.m_motion.pState->m_transform.m_rotation, hitInfo.m_normal);
    const hkQuadReal normal = point.m_contact.m_separatingNormal.m_quad;
    XFNV_M128_F32(point.m_contact.m_separatingNormal.m_quad, 3) = hitInfo.m_hitFraction;
    const hkQuadReal r = _mm_set_ss(negRadius);
    point.m_contact.m_position.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r, r, 0), normal), pos);
    m_collector.addCdPoint(point);
    // hkMath::min2(m_collector.getEarlyOutDistance(), m_earlyOutHitFraction).
    const float collectorDistance = m_collector.m_earlyOutDistance;
    const float ownFraction = m_earlyOutHitFraction;
    m_earlyOutHitFraction = (collectorDistance < ownFraction) ? collectorDistance : ownFraction;
}

// Nothing of its own to free.
hkHeightFieldRayForwardingCollector::~hkHeightFieldRayForwardingCollector() {}
