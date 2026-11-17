#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/hkpToiResourceMgr.h"

#include <stdint.h>

class hkStatisticsCollector;

// The default resource manager of the time of impact solver: a scratch pad of a fixed capacity.
class hkpDefaultToiResourceMgr : public hkpToiResourceMgr {
public:
	hkpDefaultToiResourceMgr();
	~hkpDefaultToiResourceMgr() override;

	hkResult beginToiAndSetupResources(const hkpToiEvent& event, const hkArray<hkpToiEvent>& otherEvents,
		hkpToiResources& resourcesOut) override;
	hkpToiResourceMgrResponse cannotSolve(hkArray<ConstraintViolationInfo>& violatedConstraints) override;
	hkpToiResourceMgrResponse resourcesDepleted() override;
	void endToiAndFreeResources(const hkpToiEvent& event, const hkArray<hkpToiEvent>& otherEvents,
		const hkpToiResources& resources) override;
	const hkClass* getClassType() const override;
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

	// Whether the event is handled: always. A private non-virtual member.
	hkBool* shouldHandleGivenToi(hkBool* apResult, const hkpToiEvent& event);

	int32_t m_scratchPadCapacity;
	int32_t m_defaultScratchpadSize;
};

extern hkClass hkpDefaultToiResourceMgrClass;
