#pragma once

#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpNullContactMgr.h"

// Provides a shared null contact manager for pairs that do not need contact processing.
class hkpNullContactMgrFactory : public hkpContactMgrFactory {
public:
	~hkpNullContactMgrFactory() override;

	hkpContactMgr* createContactMgr(const hkpCollidable&, const hkpCollidable&, const hkpCollisionInput&) override;

	hkpNullContactMgr m_contactMgr;
};
