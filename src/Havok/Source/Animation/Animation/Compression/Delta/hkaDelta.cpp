#include "Havok/Source/Animation/Animation/Compression/Delta/hkaDelta.h"

// Replaces every sample but the first with its difference from the sample before it, in place, walking the array
// backwards. The exe unrolls the loop by four; nothing happens for fewer than two samples.
void hkDeltaTransform(float* rawData, uint32_t nValues) {
	for (uint32_t i = 1; i < nValues; ++i) {
		const uint32_t j = nValues - i;
		rawData[j] = rawData[j] - rawData[j - 1];
	}
}
