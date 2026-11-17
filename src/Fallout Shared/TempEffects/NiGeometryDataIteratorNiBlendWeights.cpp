#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendWeights.h"

// Same function as hkVector4::operator() (0x560D30), registered there.
float& NiBlendWeights::operator[](int32_t aiIndex) {
	return fWeights[aiIndex];
}

// The exe first calls its shared empty stub 0x4534F0 with aiIndex (the release build's bounds check), which src
// leaves out. The stream is untyped vertex memory (m_pvCurrentPosition) holding three weights per element, read as
// floats; the fourth weight is one minus their sum.
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
