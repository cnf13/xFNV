#pragma once

#include <cstdint>

// The description of the machine the game runs on.
class NiSystemDesc {
public:
	// Always true on PC.
	bool IsLittleEndian() const;

	// The singleton ms_pkSystemDesc.
	static const NiSystemDesc& GetSystemDesc();
	static void ShutdownSystemDesc();
	// Allocates ms_pkSystemDesc (8 bytes) and fills it from GetSystemInfo and
	// QueryPerformanceFrequency.
	static void InitSystemDesc();

	uint32_t m_uiNumLogicalProcessors;
	float m_fPCCyclesPerSecond;

	static inline NiSystemDesc* ms_pkSystemDesc;
};
