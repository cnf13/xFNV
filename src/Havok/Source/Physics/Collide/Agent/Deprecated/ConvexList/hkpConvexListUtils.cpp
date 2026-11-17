#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListUtils.h"

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

// Maps a point between convex list shapes to the children it lies on: each convex list side is replaced by a body for
// its child, keyed by the shape key in the high byte of the cache's vertex ids. A point whose vertices span several
// children is dropped (HK_INVALID_CONTACT_POINT) and flagged.
uint16_t hkpMapPointsToSubShapeContactMgr::addContactPointImpl(const hkpCdBody& a, const hkpCdBody& b,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& output, const hkpGskCache* contactCache,
	hkContactPoint& cp) {
	const hkpCdBody* aMod = &a;
	const hkpCdBody* bMod = &b;
	hkpCdBody aTemp;
	hkpCdBody bTemp;
	if (a.m_shape->m_type.m_storage == HK_SHAPE_CONVEX_LIST) {
		int32_t shapeKey = (0xff00 & contactCache->m_vertices[0]) >> 8;
		for (int32_t i = 1; i < contactCache->m_dimA; ++i) {
			int32_t shapeKey2 = (0xff00 & contactCache->m_vertices[i]) >> 8;
			if (shapeKey2 != shapeKey) {
				m_invalidPointHit = true;
				// HK_INVALID_CONTACT_POINT
				return 0xFFFF;
			}
		}
		alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
		const hkpShape* childShape = static_cast<const hkpConvexListShape*>(a.m_shape)->getChildShape(shapeKey, buffer);
		aTemp.m_parent = &a;
		aTemp.m_motion = a.m_motion;
		aTemp.m_shape = childShape;
		aTemp.m_shapeKey = shapeKey;
		aMod = &aTemp;
	}
	if (b.m_shape->m_type.m_storage == HK_SHAPE_CONVEX_LIST) {
		int32_t shapeKey = (0xff00 & contactCache->m_vertices[contactCache->m_dimA]) >> 8;
		for (int32_t i = contactCache->m_dimA + 1; i < contactCache->m_dimA + contactCache->m_dimB; ++i) {
			int32_t shapeKey2 = (0xff00 & contactCache->m_vertices[i]) >> 8;
			if (shapeKey2 != shapeKey) {
				m_invalidPointHit = true;
				// HK_INVALID_CONTACT_POINT
				return 0xFFFF;
			}
		}
		alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
		const hkpShape* childShape = static_cast<const hkpConvexListShape*>(b.m_shape)->getChildShape(shapeKey, buffer);
		bTemp.m_parent = &b;
		bTemp.m_motion = b.m_motion;
		bTemp.m_shape = childShape;
		bTemp.m_shapeKey = shapeKey;
		bMod = &bTemp;
	}
	return m_contactMgr->addContactPoint(*aMod, *bMod, input, output, contactCache, cp);
}

// The time-of-impact form of addContactPointImpl: the same mapping, TOI_REJECT for a point spanning several
// children (Havok SDK, hkpConvexListUtils.h).
hkpContactMgr::ToiAccept hkpMapPointsToSubShapeContactMgr::addToiImpl(const hkpCdBody& a, const hkpCdBody& b,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& output, float toi, hkContactPoint& cp,
	const hkpGskCache* gskCache, float& projectedVelocity, hkpContactPointProperties& propertiesOut) {
	const hkpCdBody* aMod = &a;
	const hkpCdBody* bMod = &b;
	hkpCdBody aTemp;
	hkpCdBody bTemp;
	if (a.m_shape->m_type.m_storage == HK_SHAPE_CONVEX_LIST) {
		int32_t shapeKey = (0xff00 & gskCache->m_vertices[0]) >> 8;
		for (int32_t i = 1; i < gskCache->m_dimA; ++i) {
			int32_t shapeKey2 = (0xff00 & gskCache->m_vertices[i]) >> 8;
			if (shapeKey2 != shapeKey) {
				m_invalidPointHit = true;
				return TOI_REJECT;
			}
		}
		alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
		const hkpShape* childShape = static_cast<const hkpConvexListShape*>(a.m_shape)->getChildShape(shapeKey, buffer);
		aTemp.m_parent = &a;
		aTemp.m_motion = a.m_motion;
		aTemp.m_shape = childShape;
		aTemp.m_shapeKey = shapeKey;
		aMod = &aTemp;
	}
	if (b.m_shape->m_type.m_storage == HK_SHAPE_CONVEX_LIST) {
		int32_t shapeKey = (0xff00 & gskCache->m_vertices[gskCache->m_dimA]) >> 8;
		for (int32_t i = gskCache->m_dimA + 1; i < gskCache->m_dimA + gskCache->m_dimB; ++i) {
			int32_t shapeKey2 = (0xff00 & gskCache->m_vertices[i]) >> 8;
			if (shapeKey2 != shapeKey) {
				m_invalidPointHit = true;
				return TOI_REJECT;
			}
		}
		alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
		const hkpShape* childShape = static_cast<const hkpConvexListShape*>(b.m_shape)->getChildShape(shapeKey, buffer);
		bTemp.m_parent = &b;
		bTemp.m_motion = b.m_motion;
		bTemp.m_shape = childShape;
		bTemp.m_shapeKey = shapeKey;
		bMod = &bTemp;
	}
	return m_contactMgr->addToi(*aMod, *bMod, input, output, toi, cp, gskCache, projectedVelocity, propertiesOut);
}
