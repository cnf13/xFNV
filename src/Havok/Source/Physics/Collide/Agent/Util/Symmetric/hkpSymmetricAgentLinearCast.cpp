#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/CapsuleTriangle/hkpCapsuleTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereCapsule/hkpSphereCapsuleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereTriangle/hkpSphereTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphere/hkpMultiSphereAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphereTriangle/hkpMultiSphereTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkpHeightFieldAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Bv/hkpBvAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/MultirayConvex/hkpMultiRayConvexAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Transform/hkpTransformAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

void hkpSymmetricAgentFlipCollector::addCdPoint(const hkpCdPoint& point) {
    hkVector4 v2;
    hkVector4 v5;
    hkpCdPoint v7;
    v2.m_quad = point.m_contact.m_separatingNormal.m_quad;
    const hkpCdBody* m_cdBodyA = point.m_cdBodyA;
    v5.m_quad =
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v2.m_quad, v2.m_quad, 255), v2.m_quad), point.m_contact.m_position.m_quad);
    v7.m_cdBodyA = point.m_cdBodyB;
    hkpCdPointCollector* m_collector = this->m_collector;
    v7.m_cdBodyB = m_cdBodyA;
    v7.m_contact.m_position.m_quad = v5.m_quad;
    v7.m_contact.m_separatingNormal.m_quad = _mm_xor_ps(v2.m_quad, rdata_10C4C00);
    m_collector->addCdPoint(v7);
    m_earlyOutDistance = this->m_collector->m_earlyOutDistance;
}

void hkpSymmetricAgentFlipCastCollector::addCdPoint(const hkpCdPoint& point) {
    hkVector4 v6;
    hkVector4 v8;
    hkVector4 v9;
    hkpCdPoint v11;
    const hkpCdBody* m_cdBodyA = point.m_cdBodyA;
    __m128 v3 =
        _mm_shuffle_ps(point.m_contact.m_separatingNormal.m_quad, point.m_contact.m_separatingNormal.m_quad, 255);
    __m128 v4 = _mm_shuffle_ps(v3, v3, 0);
    v6.m_quad = m_path.m_quad;
    const hkpCdBody* m_cdBodyB = point.m_cdBodyB;
    v8.m_quad = _mm_add_ps(_mm_mul_ps(v4, v6.m_quad), point.m_contact.m_position.m_quad);
    v9.m_quad = point.m_contact.m_separatingNormal.m_quad;
    v11.m_contact.m_position.m_quad = v8.m_quad;
    v11.m_cdBodyA = m_cdBodyB;
    hkpCdPointCollector* m_collector = this->m_collector;
    v11.m_cdBodyB = m_cdBodyA;
    v11.m_contact.m_separatingNormal.m_quad = _mm_xor_ps(v9.m_quad, rdata_10C4C00);
    m_collector->addCdPoint(v11);
    m_earlyOutDistance = this->m_collector->m_earlyOutDistance;
}

void hkpSymmetricAgentFlipBodyCollector::addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) {
    m_collector->addCdBodyPair(bodyB, bodyA);
    m_earlyOut.m_bool = m_collector->m_earlyOut.m_bool;
}

void hkpSymmetricAgentFlipBodyCollector::operator delete(void* apMemory) {
    hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, sizeof(hkpSymmetricAgentFlipBodyCollector));
}
