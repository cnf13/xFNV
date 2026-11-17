#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;
class hkpShapeContainer;

// Havok SDK name; not in the PDB.
hkpCollisionAgent* createShapeBvAgent(const hkpCdBody& a1, const hkpCdBody& a2, const hkpCollisionInput& a3,
	hkpContactMgr* a4);
// Havok SDK name; not in the PDB.
hkpCollisionAgent* createBvTreeShapeAgent(const hkpCdBody& a1, const hkpCdBody& a2, const hkpCollisionInput& a3,
	hkpContactMgr* a4);

// The agent between a bounding volume tree shape and another shape. hkpMoppAgent inherits its getPenetrations.
// Its code stays with the exe apart from getPenetrations; the slots 11 to 13 hold hkpCollisionAgent's empty defaults
// (0xD9FCC0, 0xD9FCD0, 0xD9FCE0), which the exe has no base vtable to own.
class hkpBvTreeAgent : public hkpCollisionAgent {
public:
	// A child agent of the tree: the child's shape key, the user info and the agent.
	struct hkpBvAgentEntryInfo {
		uint32_t m_key;
		int32_t m_userInfo;
		hkpCollisionAgent* m_collisionAgent;
	};

	hkpBvTreeAgent(hkpContactMgr* mgr);
	~hkpBvTreeAgent() override;

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& info) override;
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override;
	void invalidateTim(const hkpCollisionInput& input) override;
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
	void removePoint(uint16_t idToRemove) override {}
	void commitPotential(uint16_t newId) override {}
	void createZombie(uint16_t idToBecomeZombie) override {}

protected:
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

public:
	// Havok SDK: collects the children of bodyB's tree that overlap bodyA's box (grown by
	// 1.5 times the input's tolerance), unless the box is inside m_cachedAabb while caching is on, and brings
	// m_collisionPartners up to date: new partners get agents from the input's dispatcher, gone ones are cleaned up
	// through constraintOwner (thiscall, ret 0x10, 0xD2FAC0; the exe has no caller of this copy).
	void prepareCollisionPartners(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner);

	// Registers the agent for bounding volume trees against multi-ray shapes, both ways round. The PDB (another
	// build) lists no such function; Havok's SDK names it so.
	static void registerMultiRayAgent(hkpCollisionDispatcher* dispatcher);
	// Registers the agent for bounding volume trees against every shape, both ways round, and
	// tree against tree.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);
	// The closest points between bodyA and each child of bodyB's tree that overlaps bodyA's box and that the
	// collision filter lets through, by the agent the dispatcher holds for the pair of shape types.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);
	// Casts bodyA along its path against each child of bodyB's tree that overlaps the swept box, by the agent the
	// dispatcher holds for the pair of shape types; no filter is asked.
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);

protected:
	// bodyA's box in bodyB's space, grown by the cast path.
	static void calcAabbLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkAabb& aabbOut);
	// bodyA's box in bodyB's space, grown by the input's tolerance.
	static void staticCalcAabb(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkAabb& aabbOut);

public:
	// Havok SDK signature (hkpCollide.lib, tied by bytes).
	static hkpCollisionAgent* defaultAgentCreate(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static hkpCollisionAgent* createBvBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static uint8_t* getUseAabbCaching(uint8_t* a1);
	static char setUseAabbCaching(char a1);

	// The child agents, one per overlapping child shape.
	hkArray<hkpBvAgentEntryInfo> m_collisionPartners;
	// The other body's box the partners were last collected for.
	hkAabb m_cachedAabb;

	// Whether the agent caches the AABB it queried the tree with (.data, true; setUseAabbCaching and
	// getUseAabbCaching, 0xD2F540 and 0xD2F530, write and read it).
	static inline hkBool m_useAabbCaching{true};
	// Whether the agent updates its child agents through hkpAgentDispatchUtil's fastUpdate (.bss, false;
	// prepareCollisionPartners and processCollision read it).
	static inline hkBool m_useFastUpdate{false};
};

// The helper hkpAgentDispatchUtil gets in the BvTree agent: bodyB's shape container, the body for a child shape and
// the buffer for the child shape. Members and layout from Havok's release hkpcollide.pdb (sizeof 0x220; the game
// PDB places it in hkpBvTreeAgent.cpp).
class hkAgentDispatchUtilHelper {
public:
	const hkpShapeContainer* m_container;
	hkpCdBody m_bodyB;
	// The PDB's hkpShapeBuffer: 512 bytes, 16-byte aligned.
	alignas(16) char m_shapeBuffer[512];
};

// Havok's static helper that updates a list of agent partners through HELPER (the PDB's one instance: <uint32_t,
// hkpBvTreeAgent::hkpBvAgentEntryInfo, hkAgentDispatchUtilHelper>). No data: sizeof 1 in Havok's release
// hkpcollide.pdb.
template <typename KEY, typename INFO, typename HELPER> class hkpAgentDispatchUtil {};

inline constexpr fnvd::Vtable hkpBvTreeAgent_vftable{};
