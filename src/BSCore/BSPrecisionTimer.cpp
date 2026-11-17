#include "BSCore/BSPrecisionTimer.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// A start-up object whose constructor reads the performance counter's frequency.
class BSPrecisionTimerInitializer {
public:
	BSPrecisionTimerInitializer() { BSPrecisionTimer::Initialize(); }
};

static BSPrecisionTimerInitializer PrecisionTimerInitializer;

BSPrecisionTimer::BSPrecisionTimer() : iStartTime(0), iTargetTime(0) {}

void BSPrecisionTimer::Reset(float afTargetTime) {
	iStartTime = BSPrecisionTimer::GetTimer();
	iTargetTime = (int64_t)((uint64_t)(int64_t)(afTargetTime * BSPrecisionTimer::fFrequencyMS));
}

// Whether more than iTargetTime has passed since iStartTime (a signed 64-bit compare).
bool BSPrecisionTimer::Check() {
	return GetTimer() - iStartTime > iTargetTime;
}

int64_t BSPrecisionTimer::GetTimer() {
	_LARGE_INTEGER performanceCount;
	performanceCount.QuadPart = (int64_t)0;
	QueryPerformanceCounter(&performanceCount);
	return performanceCount.QuadPart;
}

void BSPrecisionTimer::Initialize() {
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	BSPrecisionTimer::fFrequencyMS = static_cast<double>(Frequency.QuadPart) / 1000.0;
}
