#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendWeights.h"

float& NiBlendWeights::operator[](int32_t aiIndex) {
    return fWeights[aiIndex];
}

// The stream is untyped vertex memory (m_pvCurrentPosition) holding three weights per element, read as floats; the
// fourth weight is one minus their sum.
void NiGeometryDataIteratorNiBlendWeights::Get(int32_t aiIndex, NiBlendWeights& arData) {
    const float* pfData = reinterpret_cast<const float*>(static_cast<char*>(m_pvCurrentPosition) + m_iStride * aiIndex);
    float fWeight = pfData[0];
    float fRemaining = 1.0 - fWeight;
    arData[0] = fWeight;
    fWeight = pfData[1];
    fRemaining = fRemaining - fWeight;
    arData[1] = fWeight;
    fWeight = pfData[2];
    fRemaining = fRemaining - fWeight;
    arData[2] = fWeight;
    arData[3] = fRemaining;
}
