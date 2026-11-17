#pragma once

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphere/hkpMultiSphereAgent.h"
#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkpHeightFieldAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Bv/hkpBvAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Transform/hkpTransformAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Reconstruction.hpp"

// The agent registered for a flipped shape pair, adding the linear cast to hkpSymmetricAgentLinearCast: TAgent's
// linear cast with the bodies swapped, the points flipped back through an hkpSymmetricAgentFlipCastCollector. The
// layout is TAgent's; the body stays with the exe, which instantiates it in each agent's .cpp.
template <class TAgent> class hkpSymmetricAgent : public hkpSymmetricAgentLinearCast<TAgent> {
public:
	// The SDK's constructor: hkpSymmetricAgentLinearCast's.
	using hkpSymmetricAgentLinearCast<TAgent>::hkpSymmetricAgentLinearCast;

	// The SDK's hkpSymmetricAgent(mgr): the base's constructor, then the class's vtable, as the exe inlines it in the
	// create functions (0xD2FA00, 0xD2FA40: hkpBvTreeAgent; 0xD2F240: hkpMoppAgent).
	explicit hkpSymmetricAgent(hkpContactMgr* mgr) : hkpSymmetricAgentLinearCast<TAgent>(mgr) {}

	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpBvTreeAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpBvTreeStreamAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpShapeCollectionAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpHeightFieldAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpConvexListAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpListAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpTransformAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpBvAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpMoppAgent_vftable{};
inline constexpr fnvd::Vtable hkpSymmetricAgent_hkpMultiSphereAgent_vftable{};
// TAgent's linear cast (a direct call) with bodies A and B swapped, along the negated path, through flip cast
// collectors. The exe's nine copies (0xD2EF30 ... 0xD46340) are the same code but for the callee.
template <class TAgent>
void hkpSymmetricAgent<TAgent>::linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	hkpSymmetricAgentFlipCastCollector flip(input.m_path, collector);
	hkpLinearCastCollisionInput flippedInput = input;
	flippedInput.m_path.setNeg4(input.m_path);
	if (startCollector) {
		hkpSymmetricAgentFlipCastCollector startFlip(input.m_path, *startCollector);
		TAgent::linearCast(bodyB, bodyA, flippedInput, flip, &startFlip);
	} else {
		TAgent::linearCast(bodyB, bodyA, flippedInput, flip, nullptr);
	}
}
