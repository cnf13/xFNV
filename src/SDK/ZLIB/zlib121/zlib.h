#pragma once

#include "Reconstruction.hpp"
#include <stdint.h>

// The native programs link upstream zlib (external/zlib) in place of the exe's 1.2.1, and this header is upstream's.
// Callers' text stays the hybrid's: inflateInit_ and deflateInit_ compare only the version's first character and the
// stream's size, so "1.2.1" and sizeof(z_stream_s) pass on both widths. Upstream allocates its own stream state
// (through zalloc) and overwrites a `state` the caller presets; the exe's 1.2.1 keeps the preset one.
#include <zlib.h>
