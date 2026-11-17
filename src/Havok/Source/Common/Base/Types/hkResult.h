#pragma once

#include "Reconstruction.hpp"

// The SDK's result code; the PDB has no source path, so it lives with the base types.
enum hkResult : int32_t {
	HK_SUCCESS = 0,
	HK_FAILURE = 1,
};
