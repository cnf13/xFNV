#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"

// Below hkUFloat8_eps (0.01f) the value is 0. Otherwise a binary search on the float bits taken as signed integers (all
// table values are positive): the half the value falls in (0x4358CCCD is 216.8f), six halvings, then the last midpoint,
// stepped up by one entry where the value lies above it. Each midpoint is halved address plus halved address, as the
// game computes it.
hkUFloat8& hkUFloat8::operator=(const float& fv) {
    const int32_t iValue = *reinterpret_cast<const int32_t*>(&fv);
    if (iValue < 0x3C23D70A) {
        m_value = 0;
        return *this;
    }
    uintptr_t uiLower = reinterpret_cast<uintptr_t>(&hkUFloat8_intToReal[0]);
    uintptr_t uiUpper = reinterpret_cast<uintptr_t>(&hkUFloat8_intToReal[0] + MAX_VALUE);
    if (iValue > 0x4358CCCD) {
        uiLower = reinterpret_cast<uintptr_t>(&hkUFloat8_intToReal[MAX_VALUE / 2]);
    }
    else {
        uiUpper = reinterpret_cast<uintptr_t>(&hkUFloat8_intToReal[MAX_VALUE / 2]);
    }
    for (int32_t i = 0; i < 6; ++i) {
        const uintptr_t uiMid = (uiUpper >> 1) + (uiLower >> 1);
        if (iValue > *reinterpret_cast<const int32_t*>(uiMid)) {
            uiLower = uiMid;
        }
        else {
            uiUpper = uiMid;
        }
    }
    const float* pMid = reinterpret_cast<const float*>((uiUpper >> 1) + (uiLower >> 1));
    if (iValue > *reinterpret_cast<const int32_t*>(pMid) && pMid < &hkUFloat8_intToReal[MAX_VALUE - 1]) {
        ++pMid;
    }
    m_value = static_cast<uint8_t>(pMid - &hkUFloat8_intToReal[0]);
    return *this;
}
