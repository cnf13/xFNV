#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpPredGskfAgent.h"
#include "Havok/Source/Physics/Collide/Agent3/ConvexList3/hkpConvexListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkStatisticsCollector;
class hkpAgent3Input;
class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpLinearCastCollisionInput;
class hkpProcessCollisionOutput;

// The agent between a convex list shape and another shape: it collides the list's hull as a GSK agent and switches
// to a stream of child agents when the hull is penetrated. Its code stays with the exe.
class hkpConvexListAgent : public hkpPredGskfAgent {
public:
	// The agent's state in stream mode, kept over the GSK manifold (m_manifold): the child agents' track and the
	// time distance of stream mode.
	struct StreamData {
		hkpAgent1nTrack m_agentTrack;
		float m_inStreamModeTimDist;
	};

	hkpConvexListAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& listShapeBodyB,
		const hkpCollisionInput& input, hkpConstraintOwner& constraintOwner) override;
	void invalidateTim(const hkpCollisionInput& input) override;
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
	void removePoint(uint16_t idToRemove) override;
	void commitPotential(uint16_t idToCommit) override;
	void createZombie(uint16_t idTobecomeZombie) override;

	// Havok SDK (cdecl).
	// Cleans the stream's child agents away and starts the GSK manifold empty (protected in the PDB).
	void switchToGskMode(hkpConstraintOwner& constraintOwner);
	// The SDK's inline getStream (its assert compiled out): stream mode reuses the manifold's storage as raw bytes
	// for its own data, so the cast is on untyped memory.
	StreamData& getStream() { return *reinterpret_cast<StreamData*>(&m_manifold); }
	// Havok SDK signature (hkpCollide.lib, tied by bytes).
	void switchToStreamMode(hkpConstraintOwner& constraintOwner);

	// Havok SDK.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Havok SDK.
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	static hkpCollisionAgent* createConvexConvexListAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static hkpCollisionAgent* createConvexListConvexAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static hkpCollisionAgent* createConvexListConvexListAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	hkpCollisionDispatcher* m_dispatcher;
	hkBool m_inGskMode;
	hkBool m_processFunctionCalled;
	int16_t m_inStreamModeCounter;
	// The tail of the 16-byte aligned object (hkpGskBaseAgent holds an hkVector4).
	uint8_t padding88[8];
};
inline constexpr fnvd::Vtable hkpConvexListAgent_vftable{};
