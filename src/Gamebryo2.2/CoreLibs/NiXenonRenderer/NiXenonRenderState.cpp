#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

bool NiPixelFormat::operator==(const NiPixelFormat& kOther) const {
	if (kOther.m_eFormat != m_eFormat)
		return false;
	if (kOther.m_uFlags != m_uFlags)
		return false;
	if (kOther.m_eTiling != m_eTiling)
		return false;
	if (kOther.m_ucBitsPerPixel != m_ucBitsPerPixel)
		return false;
	const uint32_t uiOtherHint = kOther.m_uiRendererHint;
	const uint32_t uiHint = m_uiRendererHint;
	if (uiOtherHint != uiHint && uiHint != 0xFFFFFFFFu && uiOtherHint != 0xFFFFFFFFu)
		return false;

	for (int32_t i = 0; i < static_cast<int32_t>(NUM_COMPS); ++i) {
		if (kOther.m_akComponents[i].m_eComponent != m_akComponents[i].m_eComponent ||
			kOther.m_akComponents[i].m_eRepresentation != m_akComponents[i].m_eRepresentation ||
			kOther.m_akComponents[i].m_ucBitsPerComponent != m_akComponents[i].m_ucBitsPerComponent ||
			kOther.m_akComponents[i].m_bSigned != m_akComponents[i].m_bSigned)
			return false;
	}
	return true;
}
