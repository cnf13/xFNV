#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"

// The product is rounded to float in a temporary before the sum.
NiPoint3 NiSegment::GetPoint(float fS) const {
	// EXE-INLINED 0x4A3760
	NiPoint3 kOffset;
	kOffset.x = m_kDirection.x * fS;
	kOffset.y = m_kDirection.y * fS;
	kOffset.z = m_kDirection.z * fS;
	NiPoint3 kPoint;
	kPoint.x = m_kOrigin.x + kOffset.x;
	kPoint.y = m_kOrigin.y + kOffset.y;
	kPoint.z = m_kOrigin.z + kOffset.z;
	return kPoint;
}

int32_t NiSegment::IsEqual(NiSegment* a2) {
	return (int32_t)((uint8_t)((a2->m_kOrigin.x == m_kOrigin.x) &&
		((a2->m_kOrigin.y == m_kOrigin.y) &&
			((a2->m_kOrigin.z == m_kOrigin.z) &&
				((a2->m_kDirection.x == m_kDirection.x) &&
					((a2->m_kDirection.y == m_kDirection.y) && (a2->m_kDirection.z == m_kDirection.z)))))));
}
