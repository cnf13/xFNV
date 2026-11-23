#pragma once

#include "Havok/Source/Animation/Internal/Compression/hkaCompression.h"

// Decodes the block at encodedData, laid out as desc says, into rawDataSizeBytes bytes at rawData.
uint32_t hkBlockDecode(const uint8_t* encodedData, hkaBlockDesc& desc, uint8_t* rawData, uint32_t rawDataSizeBytes);
