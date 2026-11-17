#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Reconstruction.hpp"

// The contact manager that keeps no contacts: it adds no data to hkpContactMgr, and every slot does nothing. It
// overrides every hkpContactMgr slot because the exe has no hkpContactMgr vtable to inherit from.
class hkpNullContactMgr : public hkpContactMgr {
public:
	hkpNullContactMgr() : hkpContactMgr(hkpContactMgr::TYPE_NULL_CONTACT_MGR) {}
	~hkpNullContactMgr() override;

	uint16_t addContactPointImpl(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& env,
		hkpProcessCollisionOutput& output, const hkpGskCache* contactCache, hkContactPoint& cp) override;
	hkResult reserveContactPointsImpl(int32_t numPoints) override;
	void removeContactPointImpl(uint16_t cpId, hkpConstraintOwner& constraintOwner) override {}

	void processContactImpl(const hkpCollidable& a, const hkpCollidable& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionData& collisionData) override {}

	hkpContactMgr::ToiAccept addToiImpl(const hkpCdBody& a, const hkpCdBody& b, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& output, float toi, hkContactPoint& cp, const hkpGskCache* gskCache,
		float& projectedVelocity, hkpContactPointProperties& properties) override {
		return TOI_ACCEPT;
	}

	void removeToiImpl(hkpConstraintOwner& constraintOwner, hkpContactPointProperties& properties) override {}
	void cleanup() override {}
};
inline constexpr fnvd::Vtable hkpNullContactMgr_vftable{};
