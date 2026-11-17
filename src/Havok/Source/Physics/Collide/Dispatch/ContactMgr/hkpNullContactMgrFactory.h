#pragma once

#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpNullContactMgr.h"
#include "Reconstruction.hpp"

// Provides a shared null contact manager for pairs that do not need contact processing.
class hkpNullContactMgrFactory : public hkpContactMgrFactory {
public:
	~hkpNullContactMgrFactory() override;

	hkpContactMgr* createContactMgr(const hkpCollidable&, const hkpCollidable&, const hkpCollisionInput&) override;

	hkpNullContactMgr m_contactMgr;
};
inline constexpr fnvd::Vtable hkpNullContactMgrFactory_vftable{};
