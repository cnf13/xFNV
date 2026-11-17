#include "Havok/Source/Physics/Collide/Agent3/BoxBox/hkpBoxBoxAgent3.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxCollisionDetection.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxManifold.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <xmmintrin.h>

class hkpConstraintOwner;
class hkpProcessCollisionOutput;

// Each box's radius vector is its convex radius splatted plus its half extents; the detector is built on the stack over
// the input's bodies, transforms and environment, it updates the manifold in the agent's data, and the entry gets the
// manifold's point count.
hkpAgentData* hkBoxBoxAgent3::process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
	hkVector4* separatingNormal, hkpProcessCollisionOutput& output) {
	HK_TIMER_BEGIN("BoxBox3", nullptr);
	hkpBoxBoxManifold* manifold = static_cast<hkpBoxBoxManifold*>(agentData);
	const hkpBoxShape* boxA = static_cast<const hkpBoxShape*>(input.m_bodyA->m_shape);
	const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(input.m_bodyB->m_shape);

	hkVector4 rA;
	// EXE-INLINED 0x4589C0
	// EXE-INLINED none: hkVector4::add4
	rA.m_quad = _mm_add_ps(_mm_set1_ps(boxA->m_radius), boxA->m_halfExtents.m_quad);
	hkVector4 rB;
	// EXE-INLINED 0x4589C0
	// EXE-INLINED none: hkVector4::add4
	rB.m_quad = _mm_add_ps(_mm_set1_ps(boxB->m_radius), boxB->m_halfExtents.m_quad);

	const float tolerance = input.m_input->m_tolerance;
	// EXE-INLINED none: hkpBoxBoxCollisionDetection::hkpBoxBoxCollisionDetection
	hkpBoxBoxCollisionDetection detector(*input.m_bodyA, *input.m_bodyB, input.m_input, input.m_contactMgr, &output,
		*input.m_bodyA->m_motion.pMotion, *input.m_bodyB->m_motion.pMotion, input.m_aTb, rA, rB, tolerance);
	detector.calcManifold(*manifold);
	entry->m_numContactPoints = manifold->m_numPoints;
	HK_TIMER_END();
	return static_cast<char*>(agentData) + sizeof(hkpBoxBoxManifold);
}

// Removes each of the manifold's committed points (id not HK_INVALID_CONTACT_POINT) from the contact manager,
// re-reading the point count each round, then empties the manifold and the entry.
hkpAgentData* hkBoxBoxAgent3::cleanup(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner) {
	hkpBoxBoxManifold* manifold = static_cast<hkpBoxBoxManifold*>(agentData);
	for (int32_t i = 0; i < manifold->m_numPoints; i++) {
		if (manifold->m_contactPoints[i].m_contactPointId != HK_INVALID_CONTACT_POINT) {
			mgr->removeContactPointImpl(manifold->m_contactPoints[i].m_contactPointId, constraintOwner);
		}
	}
	manifold->m_numPoints = 0;
	entry->m_numContactPoints = 0;
	return static_cast<char*>(agentData) + sizeof(hkpBoxBoxManifold);
}

// The agent owns nothing beyond its contact points, which cleanup removes; the dispatcher is unused.
void hkBoxBoxAgent3::destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher) {
	cleanup(entry, agentData, mgr, constraintOwner);
}

// The SDK's placement new of the manifold (with its null test), then the memory past it.
hkpAgentData* hkBoxBoxAgent3::create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
	if (agentData) {
		::new (agentData) hkpBoxBoxManifold;
	}
	return static_cast<char*>(agentData) + sizeof(hkpBoxBoxManifold);
}

// Removes the manifold's point with the id idToRemove, if it has one, and counts it off the entry.
void hkBoxBoxAgent3::removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove) {
	hkpBoxBoxManifold* manifold = static_cast<hkpBoxBoxManifold*>(agentData);
	for (int32_t i = 0; i < manifold->m_numPoints; i++) {
		if (manifold->m_contactPoints[i].m_contactPointId == idToRemove) {
			manifold->removePoint(i);
			entry->m_numContactPoints--;
			return;
		}
	}
}

// Gives the manifold's first potential point (id HK_INVALID_CONTACT_POINT) the id newId.
void hkBoxBoxAgent3::commitPotential(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToCommit) {
	hkpBoxBoxManifold* manifold = static_cast<hkpBoxBoxManifold*>(agentData);
	for (int32_t i = 0; i < manifold->m_numPoints; i++) {
		if (manifold->m_contactPoints[i].m_contactPointId == HK_INVALID_CONTACT_POINT) {
			manifold->m_contactPoints[i].m_contactPointId = idToCommit;
			return;
		}
	}
}

void hkBoxBoxAgent3::createZombie(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToConvert) {
	;
}

// Stores this agent's own functions; there is no separating-normal or update-filter function, and the agent is not
// predictive.
void hkBoxBoxAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
	f.m_createFunc = &hkBoxBoxAgent3::create;
	f.m_processFunc = &hkBoxBoxAgent3::process;
	f.m_sepNormalFunc = nullptr;
	f.m_cleanupFunc = &hkBoxBoxAgent3::cleanup;
	f.m_removePointFunc = &hkBoxBoxAgent3::removePoint;
	f.m_commitPotentialFunc = &hkBoxBoxAgent3::commitPotential;
	f.m_createZombieFunc = &hkBoxBoxAgent3::createZombie;
	f.m_updateFilterFunc = nullptr;
	f.m_destroyFunc = &hkBoxBoxAgent3::destroy;
	f.m_isPredictive.m_bool = 0;
}

// KEEP: hkpCollisionDispatcher::registerAgent3 (0xCFBD60) copies all 0x40 bytes of f (rep movsd, 16 dwords) into the
// dispatcher's table, and f's bytes 0x33-0x3F (the tail padding after m_reusePreviousEntry) are never written: stack
// garbage src cannot reproduce. The Agent3Funcs constructor is inlined
// (the three flags and the five optional functions cleared), then initAgentFunc fills the table.
void hkBoxBoxAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
	alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
	// EXE-INLINED none: hkpCollisionDispatcher::Agent3Funcs::Agent3Funcs
	f.m_isPredictive.m_bool = 0;
	f.m_ignoreSymmetricVersion.m_bool = 0;
	f.m_reusePreviousEntry.m_bool = 0;
	f.m_updateFilterFunc = nullptr;
	f.m_calcStatisticsFunc = nullptr;
	f.m_invalidateTimFunc = nullptr;
	f.m_warpTimeFunc = nullptr;
	f.m_sepNormalFunc = nullptr;
	hkBoxBoxAgent3::initAgentFunc(f);
	dispatcher->registerAgent3(f, HK_SHAPE_BOX, HK_SHAPE_BOX);
}
