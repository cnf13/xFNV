#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"

// The product is rounded to float in a temporary before the sum.
NiPoint3 NiSegment::GetPoint(float fS) const {
    NiPoint3 kOffset = fS * m_kDirection;
    NiPoint3 kPoint;
    kPoint.x = m_kOrigin.x + kOffset.x;
    kPoint.y = m_kOrigin.y + kOffset.y;
    kPoint.z = m_kOrigin.z + kOffset.z;
    return kPoint;
}

int32_t NiSegment::IsEqual(NiSegment* pkOther) {
    return (pkOther->m_kOrigin.x == m_kOrigin.x) &&
        ((pkOther->m_kOrigin.y == m_kOrigin.y) &&
         ((pkOther->m_kOrigin.z == m_kOrigin.z) &&
          ((pkOther->m_kDirection.x == m_kDirection.x) &&
           ((pkOther->m_kDirection.y == m_kDirection.y) && (pkOther->m_kDirection.z == m_kDirection.z)))));
}
