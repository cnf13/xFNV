#include "Gamebryo2.2/CoreLibs/NiMain/NiCompUtility.h"

void NiCompUtility::DecompressFloatArray(
    const int16_t* psSrcArray,
    uint32_t uiSrcCount,
    float fOffset,
    float fHalfRange,
    float* pfDestArray,
    uint32_t uiDestCount
) {
    const int16_t* psQuad;
    uint32_t uiQuads;
    float* pfQuad;
    int32_t iQuadValue;
    int32_t iValue;
    double dHalfRange = fHalfRange;
    const int16_t* psSrc = psSrcArray;
    double dOffset = fOffset;
    uint32_t ui = 0;
    if (static_cast<int32_t>(uiDestCount) >= 4) {
        psQuad = psSrcArray + 2;
        uiQuads = ((uiDestCount - 4) >> 2) + 1;
        pfQuad = pfDestArray + 2;
        ui = 4 * uiQuads;
        do {
            iQuadValue = *(psQuad - 2);
            psQuad += 4;
            pfQuad += 4;
            --uiQuads;
            *(pfQuad - 6) = static_cast<float>(((iQuadValue / 32767.0) * dHalfRange) + dOffset);
            *(pfQuad - 5) = static_cast<float>(((*(psQuad - 5) / 32767.0) * dHalfRange) + dOffset);
            *(pfQuad - 4) = static_cast<float>(((*(psQuad - 4) / 32767.0) * dHalfRange) + dOffset);
            *(pfQuad - 3) = static_cast<float>(((*(psQuad - 3) / 32767.0) * dHalfRange) + dOffset);
        }
        while (uiQuads);
        psSrc = psSrcArray;
    }
    for (; ui < uiDestCount; pfDestArray[ui - 1] = static_cast<float>(((iValue / 32767.0) * dHalfRange) + dOffset)) {
        iValue = psSrc[ui++];
    }
}
