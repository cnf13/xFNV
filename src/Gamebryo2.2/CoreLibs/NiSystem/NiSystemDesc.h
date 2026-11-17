#pragma once

#include "Reconstruction.hpp"

// The description of the machine the game runs on.
class NiSystemDesc {
public:
	// Always true on PC: the engine's shared "return true" thiscall stub (0x8D0360).
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
