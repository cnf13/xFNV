#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

float NiPoint2::SqrLength() const {
    return x * x + y * y;
}

bool NiPoint2::operator!=(const NiPoint2& pt) const {
    return !(*this == pt);
}

void NiPoint2::LoadBinary(NiStream& arStream) {
    NiStreamLoadBinary(arStream, x);
    NiStreamLoadBinary(arStream, y);
}

void NiPoint2::SaveBinary(NiStream& arStream) {
    NiStreamSaveBinary(arStream, x);
    NiStreamSaveBinary(arStream, y);
}

float NiPoint2::Length() const {
    float v3 = (x * x) + (y * y);
    return NiSqrt(v3);
}

bool NiPoint2::operator==(const NiPoint2& pt) const {
    return pt.x == x && pt.y == y;
}

float NiPoint2::Dot(const NiPoint2& pt) const {
    return ((x * pt.x) + (y * pt.y));
}

NiPoint2 NiPoint2::operator*(float fScalar) const {
    // Copies *this and scales the copy in place; no constructor is called.
    NiPoint2 kResult = *this;
    kResult.x = kResult.x * fScalar;
    kResult.y = kResult.y * fScalar;
    return kResult;
}

NiPoint2 NiPoint2::operator+(const NiPoint2& pt) const {
    NiPoint2 kResult = *this;
    kResult.x = kResult.x + pt.x;
    kResult.y = kResult.y + pt.y;
    return kResult;
}

NiPoint2 NiPoint2::operator-(const NiPoint2& pt) const {
    // Subtracts in double precision and then stores to a float.
    NiPoint2 kResult = *this;
    kResult.x = static_cast<float>(static_cast<double>(kResult.x) - pt.x);
    kResult.y = static_cast<float>(static_cast<double>(kResult.y) - pt.y);
    return kResult;
}

NiPoint2* NiPoint2::Scale_(NiPoint2* apResult, float afScale, NiPoint2* apSource) {
    ::new (static_cast<void*>(apResult)) NiPoint2(*apSource * afScale);
    return apResult;
}

void NiPoint2::SaveBinary(NiStream& stream, NiPoint2* pkValues, uint32_t uiNumValues) {
    uint32_t v5;
    NiBinaryStream* m_pkOstr = stream.Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v5 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, pkValues, 8 * uiNumValues, &v5, 1);
}
