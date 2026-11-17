#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

// Unlike the decomp's signed mask, the executable uses SHR after reversing the four bytes.
// Explicitly mask shift counts to reproduce the x86 shifts even for component widths above 32.
uint32_t NiPixelFormat::GetMask(Component eComp) const {
	uint32_t uiShift = 0;
	uint32_t uiComponent = 0;
	for (; uiComponent < NUM_COMPS; ++uiComponent) {
		if (m_akComponents[uiComponent].m_eComponent == eComp)
			break;
		uiShift += m_akComponents[uiComponent].m_ucBitsPerComponent;
	}
	if (uiComponent == NUM_COMPS)
		return 0;

	const uint8_t ucBits = m_akComponents[uiComponent].m_ucBitsPerComponent;
	if (ucBits == 0)
		return 0;

	uint32_t uiMask = 0;
	for (uint32_t uiBit = ucBits - 1; uiBit != 0; --uiBit)
		uiMask += 1u << (uiBit & 31);
	uiMask = (uiMask + 1) << (uiShift & 31);
	if ((m_uFlags & 1) == 0) {
		uiMask = (uiMask >> 24) | ((uiMask >> 8) & 0xFF00u) | ((uiMask << 8) & 0xFF0000u) | (uiMask << 24);
		uiMask >>= (32u - m_ucBitsPerPixel) & 31;
	}
	return uiMask;
}

uint8_t NiPixelFormat::GetBits(Component eComp) const {
	for (uint32_t uiComponent = 0; uiComponent < NUM_COMPS; ++uiComponent) {
		if (m_akComponents[uiComponent].m_eComponent == eComp)
			return m_akComponents[uiComponent].m_ucBitsPerComponent;
	}
	return 0;
}

bool NiPixelFormat::GetComponent(uint32_t uiWhichComponent, Component& areComponent, Representation& eRep,
	uint8_t& ucBPC, bool& bSigned) const {
	const NiComponentSpec& kComponent = m_akComponents[uiWhichComponent];
	bSigned = kComponent.m_bSigned;
	ucBPC = kComponent.m_ucBitsPerComponent;
	eRep = kComponent.m_eRepresentation;
	const Component eComponent = kComponent.m_eComponent;
	areComponent = eComponent;
	return eComponent != COMP_EMPTY;
}

uint32_t NiPixelFormat::GetNumComponents() const {
	uint32_t result = 0;
	for (const NiPixelFormat::NiComponentSpec* i = m_akComponents; i->m_eComponent != NiPixelFormat::COMP_EMPTY; ++i) {
		if (++result >= 4) {
			return 4;
		}
	}
	return result;
}

int32_t NiPixelFormat::GetPalettized() const {
	NiPixelFormat::Format m_eFormat = this->m_eFormat;
	return m_eFormat == NiPixelFormat::FORMAT_PAL || m_eFormat == NiPixelFormat::FORMAT_PALALPHA;
}
