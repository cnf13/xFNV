#pragma once

#include "Reconstruction.hpp"

// Havok's main-memory access helpers for code that also runs on a PS3 SPU: static members only. On the PC they are
// plain loads and stores (hkDefaultSpuDmaUtils.inl), which the exe inlines into the job code.
struct hkSpuDmaUtils {};
