#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCompUtility.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <bit>

NiRTTI NiBSplineData::ms_RTTI("NiBSplineData", &NiObject::ms_RTTI);

// Unrolled degree-3 evaluation: uniform knots, the time scaled to [0, m_iQuantity - 3], the four non-zero basis values
// from the knot span holding it, with the end spans clamped. Fewer than seven control points have their own formulas;
// fewer than four compute nothing.
template <>
void NiBSplineBasis<float, 3>::Compute(float fTime, int32_t& riMin, int32_t& riMax) const {
    // One third as a float, widened where it is used.
    constexpr float kfOneThird = 0.33333334f;

    // The cache test and update copy the bits, so equal times of other representations are recomputed.
    if (std::bit_cast<uint32_t>(fTime) == std::bit_cast<uint32_t>(m_fLastTime)) {
        riMin = m_iMin;
        riMax = m_iMax;
        return;
    }
    m_fLastTime = std::bit_cast<float>(std::bit_cast<uint32_t>(fTime));

    int32_t iQm3 = m_iQuantity - 3;
    int32_t iMax;
    if (fTime < 1.0f) {
        // The product truncates unrounded.
        iMax = static_cast<int32_t>(static_cast<double>(fTime) * iQm3) + 3;
    }
    else {
        iMax = m_iQuantity - 1;
    }
    int32_t iMin = iMax - 3;
    riMin = iMin;
    m_iMin = iMin;
    riMax = iMax;
    m_iMax = iMax;

    // Each integer is rounded to a float, as the game does.
    float fQm3 = static_cast<float>(static_cast<double>(iQm3));
    float fT = fTime * fQm3;

    if (m_iQuantity >= 7) {
        // The knots of the span and its neighbours, clamped to [0, m_iQuantity - 3].
        float fKnotA = iMax > 5 ? static_cast<float>(static_cast<double>(iMax - 5)) : 0.0f;
        float fKnotB = iMax > 4 ? static_cast<float>(static_cast<double>(iMax - 4)) : 0.0f;
        float fKnotC = static_cast<float>(static_cast<double>(iMin));
        float fKnotD = static_cast<float>(static_cast<double>(iMax - 2));
        float fKnotE = iMax < iQm3 + 1 ? static_cast<float>(static_cast<double>(iMax - 1)) : fQm3;
        float fKnotF = iMax < iQm3 ? static_cast<float>(static_cast<double>(iMax)) : fQm3;

        // The reciprocal knot distances, shorter at the clamped ends.
        float fInvG = iMax == 3 ? 1.0f : 0.5f;
        float fInvH = iMax == iQm3 + 2 ? 1.0f : 0.5f;
        float fInvI = iMax == 3 ? 1.0f : (iMax == 4 ? 0.5f : kfOneThird);
        float fInvJ = (iMax == 3 || iMax == iQm3 + 2) ? 0.5f : kfOneThird;
        float fInvK = iMax == iQm3 + 2 ? 1.0f : (iMax == iQm3 + 1 ? 0.5f : kfOneThird);

        float fL = fT - fKnotB;
        float fM = fT - fKnotC;
        float fN = fKnotD - fT;
        float fO = fKnotE - fT;
        // Intermediate values are kept in double precision, as the game does; only the stores to float round.
        float fP = fN * fInvG;
        float fQ = fM * fInvH;
        float fR = static_cast<float>(static_cast<double>(fN) * fInvI * fP);
        float fS = static_cast<float>((static_cast<double>(fP) * fL + static_cast<double>(fQ) * fO) * fInvJ);
        float fU = static_cast<float>(static_cast<double>(fM) * fInvK * fQ);
        float fV = fT - fKnotA;
        m_afValue[0] = fN * fR;
        m_afValue[1] = static_cast<float>(static_cast<double>(fO) * fS + static_cast<double>(fR) * fV);
        float fW = fKnotF - fT;
        m_afValue[2] = static_cast<float>(static_cast<double>(fS) * fL + static_cast<double>(fW) * fU);
        m_afValue[3] = fM * fU;
    }
    else if (m_iQuantity == 6) {
        if (iMax == 3) {
            float fA = 1.0f - fT;
            float fB = 2.0 - fT;
            float fC = fT * 0.5;
            float fD = fA * fA;
            float fE = 0.5 * (static_cast<double>(fC) * fB + static_cast<double>(fA) * fT);
            float fF = fC * (fT * static_cast<double>(kfOneThird));
            m_afValue[0] = fA * fD;
            m_afValue[1] = static_cast<float>(static_cast<double>(fD) * fT + static_cast<double>(fB) * fE);
            float fG = 3.0 - fT;
            m_afValue[2] = static_cast<float>(static_cast<double>(fE) * fT + static_cast<double>(fG) * fF);
            m_afValue[3] = fT * fF;
        }
        else if (iMax == 4) {
            float fA = fT - 1.0f;
            float fB = 3.0 - fT;
            float fC = fT * 0.5;
            float fD = 1.0f - fC;
            float fE = 0.5 * fA;
            float fG = fD * fD;
            float fH = (static_cast<double>(fD) * fT + static_cast<double>(fE) * fB) * kfOneThird;
            float fI = fE * fE;
            float fJ = 2.0 - fT;
            m_afValue[0] = fJ * fG;
            m_afValue[1] = static_cast<float>(static_cast<double>(fG) * fT + static_cast<double>(fH) * fB);
            m_afValue[2] = static_cast<float>(static_cast<double>(fH) * fT + static_cast<double>(fB) * fI);
            m_afValue[3] = fI * fA;
        }
        else {
            float fA = fT - 1.0;
            float fB = fT - 2.0;
            float fC = 3.0 - fT;
            float fD = fC * 0.5;
            float fE = fB * fB;
            float fF = static_cast<double>(fC) * kfOneThird * fD;
            float fG = 0.5 * (static_cast<double>(fD) * fA + static_cast<double>(fC) * fB);
            m_afValue[0] = fF * fC;
            m_afValue[1] = static_cast<float>(static_cast<double>(fT) * fF + static_cast<double>(fG) * fC);
            m_afValue[2] = static_cast<float>(static_cast<double>(fA) * fG + static_cast<double>(fC) * fE);
            m_afValue[3] = fE * fB;
        }
    }
    else if (m_iQuantity == 5) {
        if (iMax == 3) {
            float fA = 1.0f - fT;
            float fB = 2.0 - fT;
            float fC = fT * 0.5;
            float fD = fA * fA;
            float fE = 0.5 * (static_cast<double>(fC) * fB + static_cast<double>(fA) * fT);
            float fF = fC * fC;
            m_afValue[0] = fA * fD;
            m_afValue[1] = static_cast<float>(static_cast<double>(fD) * fT + static_cast<double>(fE) * fB);
            m_afValue[2] = static_cast<float>(static_cast<double>(fE) * fT + static_cast<double>(fB) * fF);
            m_afValue[3] = fT * fF;
        }
        else {
            float fA = fT - 1.0f;
            float fB = 2.0 - fT;
            float fC = fT * 0.5;
            float fD = fA * fA;
            float fE = 1.0f - fC;
            float fF = fE * fE;
            float fG = (static_cast<double>(fC) + fA) * fE;
            m_afValue[0] = fF * fB;
            m_afValue[1] = static_cast<float>(static_cast<double>(fF) * fT + static_cast<double>(fG) * fB);
            m_afValue[2] = static_cast<float>(static_cast<double>(fG) * fT + static_cast<double>(fB) * fD);
            m_afValue[3] = fD * fA;
        }
    }
    else if (m_iQuantity == 4) {
        // A single span: the Bernstein polynomials.
        float fA = 1.0f - fT;
        float fT2 = fT * fT;
        float fA2 = fA * fA;
        m_afValue[0] = fA2 * fA;
        m_afValue[1] = fA2 * (fT * 3.0);
        m_afValue[2] = fA * (3.0 * fT2);
        m_afValue[3] = fT * fT2;
    }
}

NiBSplineData::~NiBSplineData() {
    _NiFree(m_pafControlPoints);
    _NiFree(m_pasCompactControlPoints);
}

const NiRTTI* NiBSplineData::GetRTTI() const {
    return &ms_RTTI;
}

void NiBSplineData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uiControlPointCount);
    if (m_uiControlPointCount) {
        m_pafControlPoints = static_cast<float*>(_NiMalloc(sizeof(float) * m_uiControlPointCount));
        NiStreamLoadBinary(kStream, m_pafControlPoints, m_uiControlPointCount);
    }

    NiStreamLoadBinary(kStream, m_uiCompactControlPointCount);
    if (m_uiCompactControlPointCount) {
        m_pasCompactControlPoints = static_cast<int16_t*>(_NiMalloc(sizeof(int16_t) * m_uiCompactControlPointCount));
        NiStreamLoadBinary(kStream, m_pasCompactControlPoints, m_uiCompactControlPointCount);
    }
}

void NiBSplineData::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
}

bool NiBSplineData::RegisterStreamables(NiStream& stream) {
    return NiObject::RegisterStreamables(stream);
}

void NiBSplineData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiControlPointCount);
    if (m_uiControlPointCount) {
        NiStreamSaveBinary(kStream, m_pafControlPoints, m_uiControlPointCount);
    }

    NiStreamSaveBinary(kStream, m_uiCompactControlPointCount);
    if (m_uiCompactControlPointCount) {
        NiStreamSaveBinary(kStream, m_pasCompactControlPoints, m_uiCompactControlPointCount);
    }
}

// Only the counts are compared, not the control points.
bool NiBSplineData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineData* pkOther = static_cast<NiBSplineData*>(pkObject);
    return m_uiControlPointCount == pkOther->m_uiControlPointCount &&
        m_uiCompactControlPointCount == pkOther->m_uiCompactControlPointCount;
}

void NiBSplineData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiControlPointCount", m_uiControlPointCount));
    pStrings->Add(NiGetViewerString("m_uiCompactControlPointCount", m_uiCompactControlPointCount));
}

// The four control points of the span weigh in by the basis values, each point uiDimension floats long.
void NiBSplineData::GetValueDegree3(
    float fTime,
    float* afPos,
    uint32_t uiDimension,
    NiBSplineBasisData* pkBasisData,
    Handle kControlPointHandle
) const {
    NiBSplineBasis<float, 3>& kBasis = pkBasisData->GetDegree3Basis();
    int32_t iMin;
    int32_t iMax;
    kBasis.Compute(fTime, iMin, iMax);

    const float* pfSource = m_pafControlPoints + (kControlPointHandle + uiDimension * iMin);
    float fBasisValue = kBasis.GetValue(0);
    for (uint32_t j = 0; j < uiDimension; j++) {
        afPos[j] = *pfSource++ * fBasisValue;
    }

    // Compares iMin + 1 with iMax once (signed, wrapping) and then counts the spans down, so a span range ending at
    // INT32_MAX still ends.
    int32_t iFirst = static_cast<int32_t>(static_cast<uint32_t>(iMin) + 1u);
    if (iFirst > iMax) {
        return;
    }
    uint32_t uiBasis = static_cast<uint32_t>(iFirst) - static_cast<uint32_t>(iMin);
    uint32_t uiSpans = static_cast<uint32_t>(iMax) - static_cast<uint32_t>(iFirst) + 1u;
    do {
        fBasisValue = kBasis.GetValue(uiBasis++);
        // The product and the sum stay on the x87 stack (53-bit); only the store rounds to float.
        for (uint32_t j = 0; j < uiDimension; j++) {
            afPos[j] = static_cast<float>(static_cast<double>(*pfSource++) * fBasisValue + afPos[j]);
        }
    }
    while (--uiSpans);
}

// As GetValueDegree3, over the span's control points decompressed first.
void NiBSplineData::GetCompactedValueDegree3(
    float fTime,
    float* afPos,
    uint32_t uiDimension,
    NiBSplineBasisData* pkBasisData,
    Handle kControlPointHandle,
    float fOffset,
    float fHalfRange
) const {
    NiBSplineBasis<float, 3>& kBasis = pkBasisData->GetDegree3Basis();
    int32_t iMin;
    int32_t iMax;
    kBasis.Compute(fTime, iMin, iMax);

    // Four control points of at most four floats each.
    float afControlPoints[16];
    uint32_t uiCount = uiDimension * 4;
    NiCompUtility::DecompressFloatArray(
        m_pasCompactControlPoints + (kControlPointHandle + uiDimension * iMin), uiCount, fOffset, fHalfRange,
        afControlPoints, uiCount
    );

    const float* pfSource = afControlPoints;
    float fBasisValue = kBasis.GetValue(0);
    for (uint32_t j = 0; j < uiDimension; j++) {
        afPos[j] = *pfSource++ * fBasisValue;
    }

    for (int32_t i = iMin + 1; i <= iMax; i++) {
        fBasisValue = kBasis.GetValue(i - iMin);
        for (uint32_t j = 0; j < uiDimension; j++) {
            afPos[j] += *pfSource++ * fBasisValue;
        }
    }
}

NiObject* NiBSplineData::CreateObject() {
    NiBSplineData* pkObject = static_cast<NiBSplineData*>(NiMemObject::operator new(sizeof(NiBSplineData)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiBSplineData;
}
