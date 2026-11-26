#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

#include <new>
#include <stdint.h>

// A short integer 3-vector, used for object bounds.
struct NiNPShortPoint3 {
    // The BOUND_DATA constructor leaves the points unset; a user constructor removes the implicit default.
    NiNPShortPoint3() = default;
    // Callers pass whole ints, and the constructor reads only their low words.
    NiNPShortPoint3(int32_t aiX, int32_t aiY, int32_t aiZ);

    float GetFloatX() const;
    float GetFloatY() const;
    float GetFloatZ() const;
    float Length() const;
    // Acts as binary operator-, kept as a named member so that callers get the returned result pointer. Writes the
    // component-wise difference this - arPoint to apResult and returns apResult.
    NiNPShortPoint3* CalculateRelativeBounds(NiNPShortPoint3* apResult, const NiNPShortPoint3& arPoint) const;
    NiNPShortPoint3& operator=(const NiNPShortPoint3& arPt);

    static const NiNPShortPoint3& GetZERO();

    int16_t x;
    int16_t y;
    int16_t z;

    static NiNPShortPoint3 ZERO;
};

// The component-wise difference this - arPoint, constructed in apResult.
inline NiNPShortPoint3*
NiNPShortPoint3::CalculateRelativeBounds(NiNPShortPoint3* apResult, const NiNPShortPoint3& arPoint) const {
    ::new (apResult) NiNPShortPoint3(x - arPoint.x, y - arPoint.y, z - arPoint.z);
    return apResult;
}

inline NiNPShortPoint3& NiNPShortPoint3::operator=(const NiNPShortPoint3& arPt) {
    x = arPt.x;
    y = arPt.y;
    z = arPt.z;
    return *this;
}

inline NiNPShortPoint3 NiNPShortPoint3::ZERO{};

inline NiNPShortPoint3::NiNPShortPoint3(int32_t aiX, int32_t aiY, int32_t aiZ)
  : x(static_cast<int16_t>(aiX)), y(static_cast<int16_t>(aiY)), z(static_cast<int16_t>(aiZ)) {}

inline float NiNPShortPoint3::GetFloatX() const {
    return static_cast<float>(x);
}

inline float NiNPShortPoint3::GetFloatY() const {
    return static_cast<float>(y);
}

inline float NiNPShortPoint3::GetFloatZ() const {
    return static_cast<float>(z);
}

inline float NiNPShortPoint3::Length() const {
    float fLengthSquared = static_cast<float>(static_cast<double>(z * z + (y * y + x * x)));
    return NiSqrt(fLengthSquared);
}

// The guard of NiNPShortPoint3::GetZERO's function-local static ZERO.
inline int32_t ZERO_guard;

inline const NiNPShortPoint3& NiNPShortPoint3::GetZERO() {
    if ((ZERO_guard & 1) == 0) {
        ZERO_guard |= 1u;
        NiNPShortPoint3::ZERO.~NiNPShortPoint3();
        ::new (&NiNPShortPoint3::ZERO) NiNPShortPoint3(0, 0, 0);
    }
    return NiNPShortPoint3::ZERO;
}
