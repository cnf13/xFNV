#pragma once

#include "SDK/MSVC/ehvec.h"

class NiNode;
class NiStream;

class NiPoint3 {
public:
    // Leaves the coordinates undefined, like Gamebryo's empty `NiPoint3() {}`. Defaulted so that a zero global
    // (`NiPoint3 x{}`) is constant data.
    NiPoint3() = default;
    // Constexpr so that a global built from three floats is constant data.
    constexpr NiPoint3(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ) {}

    float& operator[](int32_t i);

    NiPoint3& operator+=(const NiPoint3& pt);
    NiPoint3& operator-=(const NiPoint3& pt);
    NiPoint3& operator*=(float fScalar);
    // Divides each coordinate by fScalar.
    NiPoint3& operator/=(float fScalar);
    NiPoint3 operator*(float fScalar) const;
    NiPoint3 operator/(float fScalar) const;
    NiPoint3 operator+(const NiPoint3& pt) const;
    NiPoint3 operator-(const NiPoint3& pt) const;

    // unary minus
    NiPoint3 operator-() const;

    bool operator==(const NiPoint3& pt) const;
    bool operator!=(const NiPoint3& pt) const;

    // Sets all three coordinates to zero and returns this.
    NiPoint3& Clear();
    float Length() const;
    float SqrLength() const;
    // Returns x * x + y * y.
    float SqrXYLength() const;
    // sqrt(x * x + y * y).
    float XYLength() const;
    float Dot(const NiPoint3& pt) const;
    NiPoint3 Cross(const NiPoint3& pt) const;
    void Unitize();

    // Unitizes this point and returns its old length.
    float UnitizeGetLength();
    // The unitized cross product (hidden-pointer return).
    NiPoint3 UnitCross(const NiPoint3& pt) const;
    // Reads / writes x, y, z as three 4-byte transfers.
    void LoadBinary(NiStream& stream);
    void SaveBinary(NiStream& stream);

    // "<prefix> = (x,y,z)" in a fresh _NiMalloc block the caller owns.
    char* GetViewerString(const char* pPrefix) const;
    NiPoint3 Perpendicular();

    // Compares the squared length of apPoint with fRadius squared: -1 when the point is shorter, 1 when longer, 0 when
    // equal or unordered.
    static int32_t IsMagnitudeLongerThanRadius(const NiPoint3* apPoint, float fRadius);
    // IsMagnitudeLongerThanRadius of *apPoint1 - *apPoint2.
    static int32_t IsDistanceLongerThanRadius(const NiPoint3* apPoint1, const NiPoint3* apPoint2, float afRadius);

    // pkDst[i] += f * pkSrc[i] for uiVerts points.
    static void PointsPlusEqualFloatTimesPoints(NiPoint3* pkDst, float f, const NiPoint3* pkSrc, uint32_t uiVerts);
    // The squared length of *apPoint.
    static float VectorLength(const NiPoint3* apPoint);
    // Unitizes v (a zero vector stays zero).
    static void UnitizeVector(NiPoint3& v);
    // Unitizes uiNum points spaced uiStride bytes apart.
    static void UnitizeVectors(NiPoint3* pkV, uint32_t uiNum, uint32_t uiStride);
    // The component-wise product of p0 and p1.
    static NiPoint3 ComponentProduct(const NiPoint3& p0, const NiPoint3& p1);
    static void _SDMShutdown();
    static uint32_t* InitSqrtTable();
    static void _SDMInit();

    float x;
    float y;
    float z;

    static inline uint32_t* ms_pSqrtTable;
    static NiPoint3 INFINITY_POINT3;
};

// Scalar times point.
NiPoint3 operator*(float fScalar, const NiPoint3& pt);

inline NiPoint3 NiPoint3::INFINITY_POINT3{3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f};

// Stands for NiPoint3::ZERO; a global because the class is incomplete inside its own body.
inline NiPoint3 NiPoint3_ZERO{};

// These stand for NiPoint3::UNIT_X, NiPoint3::UNIT_Y and NiPoint3::UNIT_Z; they are globals for the same reason.
inline NiPoint3 NiPoint3_UNIT_X{1.0f, 0.0f, 0.0f};
inline NiPoint3 NiPoint3_UNIT_Y{0.0f, 1.0f, 0.0f};
inline NiPoint3 NiPoint3_UNIT_Z{0.0f, 0.0f, 1.0f};

// Stands in for NiPoint3::UNIT_ALL; a global for the same reason.
inline NiPoint3 NiPoint3_UNIT_ALL{1.0f, 1.0f, 1.0f};

// The empty constructor the point types share, used for arrays of NiPoint3 (12 bytes), NiPoint2 (8) and 16-byte points.
// It stores nothing, whatever the element's size (a NiPoint3 constructed there would write 12 bytes into 8-byte
// elements).
inline void emptyPointConstructor(void*) {}
