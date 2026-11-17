#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;
class hkpLinearCastCollisionInput;

// The agent between a height field shape and another shape. Its code stays with the exe apart from getClosestPoints.
// It inherits hkpCollisionAgent's updateShapeCollectionFilter and the slots 9 to 13: the exe's vtable holds the base's
// functions there (0xD3DD00, 0xD9FCA0 to 0xD9FCE0), and the PDB declares no such override. Its destructor (0xD327E0)
// is its own: m_contactPointId needs freeing.
class hkpHeightFieldAgent : public hkpCollisionAgent {
public:
	hkpHeightFieldAgent(const hkpCdBody& A, const hkpCdBody& B, const hkpCollisionInput& input, hkpContactMgr* mgr);
	~hkpHeightFieldAgent() override;

	void getPenetrations(const hkpCdBody& csBody, const hkpCdBody& hfBody, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& csBody, const hkpCdBody& hfBody, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& csBody, const hkpCdBody& hfBody, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;

	// Havok SDK.
	static void staticGetPenetrations(const hkpCdBody& csBody, const hkpCdBody& hfBody, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// Havok SDK.
	static void staticGetClosestPoints(const hkpCdBody& csBody, const hkpCdBody& hfBody, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Havok SDK.
	static void staticLinearCast(const hkpCdBody& csBody, const hkpCdBody& hfBody,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// Protected in the SDK.
	static hkpCollisionAgent* createHeightFieldAAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static hkpCollisionAgent* createHeightFieldBAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	// hkContactPointId (an unsigned short) elements.
	hkArray<uint16_t> m_contactPointId;
};
inline constexpr fnvd::Vtable hkpHeightFieldAgent_vftable{};
