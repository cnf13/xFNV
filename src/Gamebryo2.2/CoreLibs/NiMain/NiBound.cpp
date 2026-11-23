#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

void NiBound::Merge(const NiBound* pBound) {
    float v8 = m_kCenter.x - pBound->m_kCenter.x;
    float v9 = m_kCenter.y - pBound->m_kCenter.y;
    float v10 = m_kCenter.z - pBound->m_kCenter.z;
    float v7 = v8 * v8 + v9 * v9 + v10 * v10;
    float v6 = pBound->m_fRadius - this->m_fRadius;
    float apBounda = v6 * v6;
    bool v3 = !(v7 >= static_cast<double>(apBounda));
    bool v4 = v7 == apBounda;
    if (!(v6 >= 0.0f)) {
        if (v3 || v4) {
            return;
        }
    }
    else {
        if (v3 || v4) {
            *this = *pBound;
            return;
        }
    }
    float apBoundb = std::sqrt(static_cast<double>(v7));
    double v5 = apBoundb;
    if (ms_fTolerance < static_cast<double>(apBoundb)) {
        NiPoint3 v17;
        float apBoundc = (v5 - v6) / (v5 + v5);
        float v11 = v8 * apBoundc;
        float v12 = v9 * apBoundc;
        float v13 = apBoundc * v10;
        v17.x = pBound->m_kCenter.x + v11;
        v17.y = v12 + pBound->m_kCenter.y;
        v17.z = pBound->m_kCenter.z + v13;
        m_kCenter = v17;
    }
    this->m_fRadius = (v5 + pBound->m_fRadius + this->m_fRadius) * 0.5;
}

void NiBound::SetCenterAndRadius(const NiPoint3& kCenter, float fRadius) {
    m_kCenter = kCenter;
    m_fRadius = fRadius;
}

NiBound* NiBound::Copy(const NiBound* apBound) {
    SetCenterAndRadius(apBound->m_kCenter, apBound->m_fRadius);
    return this;
}

// A NaN distance or radius takes neither side's branch, so 0 is returned then.
int32_t NiBound::WhichSide(const NiPlane& kPlane) const {
    float fDistance = kPlane.Distance(m_kCenter);
    if (fDistance <= -m_fRadius) {
        return 2;
    }
    return m_fRadius <= fDistance ? 1 : 0;
}

void NiBound::Update(const NiBound& arBound, const NiTransform& arTransform) {
    const NiMatrix3& kRot = arTransform.m_Rotate;
    float fX = kRot.m_pEntry[0][1] * arBound.m_kCenter.y + arBound.m_kCenter.x * kRot.m_pEntry[0][0] +
        kRot.m_pEntry[0][2] * arBound.m_kCenter.z;
    float fY = kRot.m_pEntry[1][0] * arBound.m_kCenter.x + kRot.m_pEntry[1][1] * arBound.m_kCenter.y +
        kRot.m_pEntry[1][2] * arBound.m_kCenter.z;
    float fZ = kRot.m_pEntry[2][0] * arBound.m_kCenter.x + kRot.m_pEntry[2][1] * arBound.m_kCenter.y +
        kRot.m_pEntry[2][2] * arBound.m_kCenter.z;
    float fScale = arTransform.m_fScale;
    float fScaledX = fX * fScale;
    float fScaledY = fY * fScale;
    float fScaledZ = fScale * fZ;
    m_kCenter.x = arTransform.m_Translate.x + fScaledX;
    m_kCenter.y = arTransform.m_Translate.y + fScaledY;
    m_kCenter.z = arTransform.m_Translate.z + fScaledZ;
    m_fRadius = arBound.m_fRadius * arTransform.m_fScale;
}

void NiBound::Update(const NiTransform& arTransform) {
    const NiMatrix3& kRot = arTransform.m_Rotate;
    float fX = kRot.m_pEntry[0][1] * m_kCenter.y + kRot.m_pEntry[0][0] * m_kCenter.x +
        kRot.m_pEntry[0][2] * m_kCenter.z;
    float fY = kRot.m_pEntry[1][0] * m_kCenter.x + kRot.m_pEntry[1][1] * m_kCenter.y +
        kRot.m_pEntry[1][2] * m_kCenter.z;
    float fZ = kRot.m_pEntry[2][0] * m_kCenter.x + kRot.m_pEntry[2][1] * m_kCenter.y +
        kRot.m_pEntry[2][2] * m_kCenter.z;
    float fScale = arTransform.m_fScale;
    float fScaledX = fX * fScale;
    float fScaledY = fY * fScale;
    float fScaledZ = fScale * fZ;
    m_kCenter.x = arTransform.m_Translate.x + fScaledX;
    m_kCenter.y = arTransform.m_Translate.y + fScaledY;
    m_kCenter.z = arTransform.m_Translate.z + fScaledZ;
    m_fRadius = arTransform.m_fScale * m_fRadius;
}

// The box of the points (each coordinate tested against both bounds, no else), its centre, then the largest
// squared distance from that centre, each one rounded to a float before it is compared.
void NiBound::ComputeFromData(int32_t iQuantity, const NiPoint3* pkData) {
    if (iQuantity > 0) {
        NiPoint3 kMin = *pkData;
        NiPoint3 kMax = *pkData;
        for (int32_t i = 1; i < iQuantity; ++i) {
            const NiPoint3& kPoint = pkData[i];
            if (kPoint.x < kMin.x) {
                kMin.x = kPoint.x;
            }
            if (kPoint.y < kMin.y) {
                kMin.y = kPoint.y;
            }
            if (kPoint.z < kMin.z) {
                kMin.z = kPoint.z;
            }
            if (kPoint.x > kMax.x) {
                kMax.x = kPoint.x;
            }
            if (kPoint.y > kMax.y) {
                kMax.y = kPoint.y;
            }
            if (kPoint.z > kMax.z) {
                kMax.z = kPoint.z;
            }
        }
        float fSumX = kMax.x + kMin.x;
        float fSumY = kMax.y + kMin.y;
        float fSumZ = kMax.z + kMin.z;
        float fCenterX = fSumX * 0.5;
        m_kCenter.x = fCenterX;
        float fCenterY = fSumY * 0.5;
        m_kCenter.y = fCenterY;
        float fCenterZ = 0.5 * fSumZ;
        m_kCenter.z = fCenterZ;
        float fMaxRadiusSqr = 0.0f;
        for (int32_t i = 0; i < iQuantity; ++i) {
            float fDiffX = pkData[i].x - m_kCenter.x;
            float fDiffY = pkData[i].y - m_kCenter.y;
            float fDiffZ = pkData[i].z - m_kCenter.z;
            float fRadiusSqr = fDiffY * fDiffY + fDiffX * fDiffX + fDiffZ * fDiffZ;
            if (fMaxRadiusSqr < fRadiusSqr) {
                fMaxRadiusSqr = fRadiusSqr;
            }
        }
        float fRadius = std::sqrt(static_cast<double>(fMaxRadiusSqr));
        this->m_fRadius = fRadius;
    }
    else {
        m_kCenter.x = NiPoint3::ZERO.x;
        m_kCenter.y = NiPoint3::ZERO.y;
        float z = NiPoint3::ZERO.z;
        this->m_fRadius = 0.0f;
        m_kCenter.z = z;
    }
}

void NiBound::SetCenter(const NiPoint3& akCenter) {
    m_kCenter = akCenter;
}

void NiBound::SetRadius(float fRadius) {
    m_fRadius = fRadius;
}

bool NiBound::operator==(const NiBound& bound) const {
    return m_kCenter.x == bound.m_kCenter.x &&
        m_kCenter.y == bound.m_kCenter.y &&
        m_kCenter.z == bound.m_kCenter.z &&
        m_fRadius == bound.m_fRadius;
}

bool NiBound::operator!=(const NiBound& kBound) const {
    return !(
        m_kCenter.x == kBound.m_kCenter.x &&
        m_kCenter.y == kBound.m_kCenter.y &&
        m_kCenter.z == kBound.m_kCenter.z &&
        m_fRadius == kBound.m_fRadius
    );
}

void NiBound::LoadBinary(NiStream& kStream) {
    m_kCenter.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fRadius);
}

void NiBound::SaveBinary(NiStream& kStream) {
    m_kCenter.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fRadius);
}

char* NiBound::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 65;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString,
        stLength,
        "%s = (%g, %g, %g) , %g",
        pPrefix,
        static_cast<double>(m_kCenter.x),
        static_cast<double>(m_kCenter.y),
        static_cast<double>(m_kCenter.z),
        static_cast<double>(m_fRadius)
    );
    return pcString;
}
