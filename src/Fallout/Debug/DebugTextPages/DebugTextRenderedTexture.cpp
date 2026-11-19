#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"

// The renderer data's pixel format; null without renderer data.
const NiPixelFormat* Ni2DBuffer::GetPixelFormat() const {
    if (!m_spRendererData.operator->()) {
        return nullptr;
    }
    return m_spRendererData->GetPixelFormat();
}
