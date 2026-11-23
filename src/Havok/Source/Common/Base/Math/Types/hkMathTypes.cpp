#include "Havok/Source/Common/Base/Math/hkMath.h"

// Truncating float to int conversion, done with integer operations on the bit pattern: the exponent picks the
// mantissa's shift, the sign is applied to the truncated value and an exponent below zero yields zero. The shift counts
// are masked to five bits and the right shifts are arithmetic.
int32_t hkMath::hkFloatToInt(float r) {
    union {
        float f;
        uint32_t uiBits;
    } kBits;
    kBits.f = r;
    const int32_t iBits = static_cast<int32_t>(kBits.uiBits);
    const int32_t iExponent = static_cast<int32_t>((kBits.uiBits >> 23) & 0xFF) - 127;
    // -1 when the value is not zero, 0 when it is: the `not` of the sign of |r| - 1.
    const int32_t iNonZero = ~(((iBits & 0x7FFFFFFF) - 1) >> 31);
    const int32_t iValue = iBits & iNonZero;
    // -1 when the exponent is below 24 (the value needs a right shift), else 0.
    const int32_t iSmall = (iExponent - 24) >> 31;
    const int32_t iShiftDown = (23 - iExponent) & iSmall;
    const int32_t iShiftCount = (iSmall & 23) - iShiftDown;
    const int32_t iMask = (-8388608 >> (iShiftCount & 31)) | ~iSmall;
    int32_t iMantissa = ((iValue & 0x7FFFFF) | 0x800000) & iMask;
    const int32_t iSign = iValue >> 31;
    int32_t iResult = ((iMantissa + iMantissa - 1) | iSign) - iMantissa + 1;
    iResult &= ~(iExponent >> 31);
    iResult = (iResult >> (iShiftDown & 31)) << (((iExponent - 23) & ~iSmall) & 31);
    return iResult;
}
