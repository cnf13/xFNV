#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// Plane n . p = c, with a normal and a constant.
class NiPlane {
public:
    // WhichSide's results.
    enum { NO_SIDE, POSITIVE_SIDE, NEGATIVE_SIDE };

    // The plane through the origin with a zero normal.
    NiPlane();
    NiPlane(NiPoint3* kP0, NiPoint3* kP1, NiPoint3* kP2);
    NiPlane(const NiPoint3& kNormal, float fConstant);

    // The plane with the given normal through kPoint.
    NiPlane(const NiPoint3& kNormal, const NiPoint3& kPoint);

    // Signed distance of kPoint from the plane.
    float Distance(const NiPoint3& kPoint) const;

    // Exact compare of normal and constant (unordered, i.e. NaN, is unequal).
    bool operator==(const NiPlane& kPlane) const;
    bool operator!=(const NiPlane& kPlane) const;
    int32_t WhichSide(const NiPoint3& kPoint) const;

    // The normal as three 4-byte transfers, then the constant.
    void LoadBinary(NiStream& stream);
    void SaveBinary(NiStream& stream);

    // "(nx,ny,nz), c" with "%g", in a fresh _NiMalloc block the caller owns. The format ignores the prefix.
    char* GetViewerString(const char* pPrefix) const;
    const NiPoint3& GetNormal() const;
    float GetConstant() const;

    NiPoint3 m_kNormal;
    float m_fConstant;
};
