#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendIndices.h"

void NiGeometryDataIteratorNiBlendIndices::Get(int32_t aiIndex, NiBlendIndices& arData) {
    // The stream is raw vertex memory walked as untyped bytes.
    arData = *reinterpret_cast<NiBlendIndices*>(static_cast<char*>(this->GetCurrentPosition()) + m_iStride * aiIndex);
}
