#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"

// The renderer data's pixel format; null without renderer data. The exe calls NiPointer::operator-> out of line
// (0x559450, twice), a trivial accessor src inlines.
const NiPixelFormat* Ni2DBuffer::GetPixelFormat() const {
	if (!m_spRendererData.operator->()) {
		return nullptr;
	}
	return m_spRendererData.operator->()->GetPixelFormat();
}
