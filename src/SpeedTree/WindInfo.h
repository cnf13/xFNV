#pragma once

#include "SpeedTree/SpeedTreeRT.h"

class CTreeFileAccess;

// A tree's wind settings: the leaf factors, the leaf oscillation (minimum, maximum, frequency) and the strength.
class SIdvWindInfo {
public:
	enum EOscillation : int32_t {
		MIN = 0x0,
		MAX = 0x1,
		FREQUENCY = 0x2,
	};

	SIdvWindInfo();

	void Parse(CTreeFileAccess& cFile);

	CVec3 m_cLeafFactors;
	CVec3 m_cLeafOscillation;
	float m_fStrength;
};
