#pragma once

#include "Reconstruction.hpp"

// Run-time type information of a Gamebryo class: its name and its base class' information.
class NiRTTI {
public:
	NiRTTI(const char* pcName, const NiRTTI* pkBaseRTTI);

	const char* GetName() const { return m_pcName; }
	const NiRTTI* GetBaseRTTI() const { return m_pkBaseRTTI; }

	const char* m_pcName;
	const NiRTTI* m_pkBaseRTTI;
};
