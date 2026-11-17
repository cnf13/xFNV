#pragma once

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpCollidable;
class hkpCollisionInput;
class hkpContactMgr;

// Interface for creating the contact manager of a collidable pair.
class hkpContactMgrFactory : public hkReferencedObject {
public:
	virtual hkpContactMgr* createContactMgr(const hkpCollidable&, const hkpCollidable&, const hkpCollisionInput&) = 0;

	// The SDK's inline; the exe's out-of-line copy 0xC90E40 is registered in hkpWorld.cpp.
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override {
		collector->pushDir("hkpContactMgrFactory");
		collector->popDir();
	}
};
