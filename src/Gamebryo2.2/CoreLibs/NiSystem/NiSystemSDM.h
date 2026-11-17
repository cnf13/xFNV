#pragma once

#include "Reconstruction.hpp"

// The static data manager of the system library (Init and Shutdown). Everything in it is static.
class NiSystemSDM {
public:
	static void Init();
	static void Shutdown();

	static inline bool ms_bInitialized;
};
