#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <bit>
#include <cmath>
#include <cstring>

float& NiPoint3::operator[](int32_t i) {
    return (&x)[i];
}

NiPoint3& NiPoint3::operator+=(const NiPoint3& pt) {
    x += pt.x;
    y += pt.y;
    z += pt.z;
    return *this;
}

NiPoint3& NiPoint3::operator-=(const NiPoint3& pt) {
    x -= pt.x;
    y -= pt.y;
    z -= pt.z;
    return *this;
}

NiPoint3& NiPoint3::operator*=(float fScalar) {
    x *= fScalar;
    y *= fScalar;
    z *= fScalar;
    return *this;
}

NiPoint3& NiPoint3::operator/=(float fScalar) {
    float v3 = 1.0 / fScalar;
    x = x * v3;
    y = y * v3;
    z = z * v3;
    return *this;
}

NiPoint3 NiPoint3::operator*(float fScalar) const {
    return NiPoint3(fScalar * x, fScalar * y, fScalar * z);
}

NiPoint3 NiPoint3::operator/(float fScalar) const {
    const float fInvScalar = 1.0f / fScalar;
    return NiPoint3(fInvScalar * x, fInvScalar * y, fInvScalar * z);
}

NiPoint3 NiPoint3::operator+(const NiPoint3& pt) const {
    return NiPoint3(x + pt.x, y + pt.y, z + pt.z);
}

NiPoint3 NiPoint3::operator-(const NiPoint3& pt) const {
    return NiPoint3(x - pt.x, y - pt.y, z - pt.z);
}

NiPoint3 NiPoint3::operator-() const {
    return NiPoint3(-x, -y, -z);
}

bool NiPoint3::operator==(const NiPoint3& pt) const {
    return x == pt.x && y == pt.y && z == pt.z;
}

bool NiPoint3::operator!=(const NiPoint3& pt) const {
    return !(*this == pt);
}

// Zeroes the point by calling the three-float constructor on it.
NiPoint3& NiPoint3::Clear() {
    return *(::new (this) NiPoint3(0.0f, 0.0f, 0.0f));
}

float NiPoint3::Length() const {
    return NiSqrt(x * x + y * y + z * z);
}

float NiPoint3::SqrLength() const {
    return x * x + y * y + z * z;
}

float NiPoint3::SqrXYLength() const {
    return x * x + y * y;
}

float NiPoint3::XYLength() const {
    float fSqrLength = (x * x) + (y * y);
    return NiSqrt(fSqrLength);
}

float NiPoint3::Dot(const NiPoint3& pt) const {
    return x * pt.x + y * pt.y + z * pt.z;
}

NiPoint3 NiPoint3::Cross(const NiPoint3& pt) const {
    return NiPoint3(y * pt.z - z * pt.y, z * pt.x - x * pt.z, x * pt.y - y * pt.x);
}

void NiPoint3::Unitize() {
    const float length = Length();

    if (length > 1e-6f) {
        const float reciprocal = 1.0f / length;
        x *= reciprocal;
        y *= reciprocal;
        z *= reciprocal;
    }
    else {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
}

float NiPoint3::UnitizeGetLength() {
    float v5 = Length();
    if (!(v5 > 9.999999974752427e-07)) {
        x = 0.0;
        y = 0.0;
        z = 0.0;
        return 0.0;
    }
    else {
        float v4 = 1.0 / v5;
        x = x * v4;
        y = y * v4;
        z = z * v4;
    }
    return v5;
}

void NiPoint3::LoadBinary(NiStream& stream) {
    NiStreamLoadBinary(stream, x);
    NiStreamLoadBinary(stream, y);
    NiStreamLoadBinary(stream, z);
}

void NiPoint3::SaveBinary(NiStream& stream) {
    NiStreamSaveBinary(stream, x);
    NiStreamSaveBinary(stream, y);
    NiStreamSaveBinary(stream, z);
}

char* NiPoint3::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 64;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString, stLength, "%s = (%g,%g,%g)", pPrefix, static_cast<double>(x), static_cast<double>(y),
        static_cast<double>(z)
    );
    return pcString;
}

int32_t NiPoint3::IsMagnitudeLongerThanRadius(const NiPoint3* apPoint, float fRadius) {
    const float fSqrLength = apPoint->SqrLength();
    const float fSqrRadius = fRadius * fRadius;

    if (fSqrRadius > fSqrLength) {
        return -1;
    }
    if (fSqrRadius < fSqrLength) {
        return 1;
    }
    return 0;
}

int32_t NiPoint3::IsDistanceLongerThanRadius(const NiPoint3* apPoint1, const NiPoint3* apPoint2, float afRadius) {
    NiPoint3 v5;
    NiPoint3* v3 = &(v5 = apPoint1->operator-(*apPoint2));
    return NiPoint3::IsMagnitudeLongerThanRadius(v3, afRadius);
}

// Reads and writes the points in the order of this loop for every uiVerts.
void NiPoint3::PointsPlusEqualFloatTimesPoints(NiPoint3* pkDst, float f, const NiPoint3* pkSrc, uint32_t uiVerts) {
    for (uint32_t i = 0; i < uiVerts; i++) {
        pkDst[i] += f * pkSrc[i];
    }
}

float NiPoint3::VectorLength(const NiPoint3* apPoint) {
    float v4;
    float* v5 = &v4;
    v4 = apPoint->x * apPoint->x + apPoint->y * apPoint->y + apPoint->z * apPoint->z;
    if (v4 == 0.0f) {
        return 0.0f;
    }

    int16_t v3 = (*reinterpret_cast<uint32_t*>(v5) >> 23) - 127;
    *reinterpret_cast<uint32_t*>(v5) &= 0x7FFFFFu;
    if ((v3 & 1) != 0) {
        *reinterpret_cast<uint32_t*>(v5) |= 0x800000u;
    }
    *reinterpret_cast<uint32_t*>(v5) =
        NiPoint3::ms_pSqrtTable[static_cast<uint16_t>((*reinterpret_cast<uint32_t*>(v5)) >> 16)] |
        ((v3 >> 1) + 127) << 23;
    return v4;
}

void NiPoint3::UnitizeVector(NiPoint3& v) {
    float v2;
    float* v3 = &v2;
    v2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (v2 == 0.0f) {
        v2 = 0.0f;
    }
    else {
        int16_t v1 = (*reinterpret_cast<uint32_t*>(v3) >> 23) - 127;
        *reinterpret_cast<uint32_t*>(v3) &= 0x7FFFFFu;
        if ((v1 & 1) != 0) {
            *reinterpret_cast<uint32_t*>(v3) |= 0x800000u;
        }
        *reinterpret_cast<uint32_t*>(v3) =
            NiPoint3::ms_pSqrtTable[static_cast<uint16_t>((*reinterpret_cast<uint32_t*>(v3)) >> 16)] |
            ((v1 >> 1) + 127) << 23;
        v2 = 1.0 / v2;
    }
    v.x = v.x * v2;
    v.y = v.y * v2;
    v.z = v.z * v2;
}

// The vectors are handled in the order of this loop for every uiNum. The length is tested by its bit pattern.
void NiPoint3::UnitizeVectors(NiPoint3* pkV, uint32_t uiNum, uint32_t uiStride) {
    for (uint32_t i = 0; i < uiNum; i++) {
        float fLeng = pkV->x * pkV->x + pkV->y * pkV->y + pkV->z * pkV->z;
        uint32_t uiRep = std::bit_cast<uint32_t>(fLeng);
        if (!uiRep) {
            fLeng = 0.0f;
        }
        else {
            // Exponent, mantissa and the table's square root of the mantissa, as NiPoint3_Win32.cpp.
            int16_t sExp = static_cast<int16_t>((uiRep >> 23) - 127);
            uiRep &= 0x7FFFFF;
            if (sExp & 1) {
                uiRep |= 0x800000;
            }
            sExp >>= 1;
            uiRep = ms_pSqrtTable[uiRep >> 16] | ((sExp + 127) << 23);
            fLeng = 1.0f / std::bit_cast<float>(uiRep);
        }
        pkV->x *= fLeng;
        pkV->y *= fLeng;
        pkV->z *= fLeng;
        // The vectors sit uiStride bytes apart in a vertex buffer that is walked as raw memory.
        pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<uintptr_t>(pkV) + uiStride);
    }
}

uint32_t* NiPoint3::InitSqrtTable() {
    float v4;
    uint32_t* pSqrtTable = static_cast<uint32_t*>(_NiMalloc(256 * sizeof(uint32_t)));
    for (uint32_t i = 0; i < 128; pSqrtTable[i + 127] = std::bit_cast<uint32_t>(v4) & 0x7FFFFF) {
        float v3 = std::sqrt((double)std::bit_cast<float>(((i | 0x3F80) << 16)));
        pSqrtTable[i] = std::bit_cast<uint32_t>(v3) & 0x7FFFFF;
        v4 = std::sqrt((double)std::bit_cast<float>(((i++ | 0x4000) << 16)));
    }
    return pSqrtTable;
}

NiPoint3 operator*(float fScalar, const NiPoint3& pt) {
    return NiPoint3(fScalar * pt.x, fScalar * pt.y, fScalar * pt.z);
}
