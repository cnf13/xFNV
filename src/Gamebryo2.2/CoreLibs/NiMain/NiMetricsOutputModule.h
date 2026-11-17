#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class NiMetricsTimer;

// A destination of the metrics NiMetricsLayer collects: named values, timers and events, flushed by Update. The
// interface only; the game registers no module. Slots 2 to 6 follow NiRefObject's two (Gamebryo's order).
class NiMetricsOutputModule : public NiRefObject {
public:
	virtual void AddValue(const char* pcName, float fValue) = 0;
	virtual void StartTimer(NiMetricsTimer& kTimer) = 0;
	virtual void EndTimer(NiMetricsTimer& kTimer) = 0;
	virtual void AddImmediateEvent(const char* pcName, float fValue) = 0;
	virtual void Update() = 0;
};

using NiMetricsOutputModulePtr = NiPointer<NiMetricsOutputModule>;
