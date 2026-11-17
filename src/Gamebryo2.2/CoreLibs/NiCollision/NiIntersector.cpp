#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"

#include "BSCore/MemoryManager.h"

// The contact stays at NI_INFINITY (FLT_MAX) until a test finds one.
NiIntersector::NiIntersector(
    float fMaxTime,
    float fDerivativeTimeStep,
    float fPseudodistanceThreshold,
    int32_t iMaxIterations
)
  : m_fMaxTime(fMaxTime), m_fDerivativeTimeStep(fDerivativeTimeStep),
    m_fInvDerivativeTimeStep(static_cast<float>(1.0 / fDerivativeTimeStep)),
    m_fPseudodistanceThreshold(fPseudodistanceThreshold), m_iMaxIterations(iMaxIterations),
    m_eIntersectionType(IT_INVALID), m_fContactTime(0x1.fffffe0000000p+127f) {
    m_kContactPoint.x = 0x1.fffffe0000000p+127f;
    m_kContactPoint.y = 0x1.fffffe0000000p+127f;
    m_kContactPoint.z = 0x1.fffffe0000000p+127f;
    m_kContactNormal.x = 0x1.fffffe0000000p+127f;
    m_kContactNormal.y = 0x1.fffffe0000000p+127f;
    m_kContactNormal.z = 0x1.fffffe0000000p+127f;
}

NiIntersector::~NiIntersector() {}

// The base test runs the full find.
void NiIntersector::Test(const NiPoint3& kV0, const NiPoint3& kV1) {
    Find(kV0, kV1);
}

// Newton's iteration on the pseudodistance over [0, m_fMaxTime]. Every intermediate is a float; the contact time is
// written before the intersection type on every path, and Pseudodistance and ComputeContactInformation are called
// virtually. The derived classes share this body.
void NiIntersector::Find(const NiPoint3& kV0, const NiPoint3& kV1) {
    float fT0 = 0.0f;
    float fF0 = Pseudodistance(kV0, kV1, fT0);
    if (fF0 <= -m_fPseudodistanceThreshold) {
        m_fContactTime = 0.0f;
        m_eIntersectionType = IT_OVERLAP;
        ComputeContactInformation(kV0, kV1);
        return;
    }
    // 1.1920929e-07f is FLT_EPSILON.
    if (fF0 <= 1.1920929e-07f) {
        m_fContactTime = 0.0f;
        m_eIntersectionType = IT_CONTACT;
        ComputeContactInformation(kV0, kV1);
        return;
    }
    // NI_INFINITY (FLT_MAX) in the empty cases.
    if (kV0 == kV1) {
        m_fContactTime = 0x1.fffffe0000000p+127f;
        m_eIntersectionType = IT_EMPTY;
        return;
    }
    float fT1 = fT0 - m_fDerivativeTimeStep;
    float fF1 = Pseudodistance(kV0, kV1, fT1);
    float fFDer0 = (fF0 - fF1) * m_fInvDerivativeTimeStep;
    if (fFDer0 >= 0.0f) {
        m_fContactTime = 0x1.fffffe0000000p+127f;
        m_eIntersectionType = IT_EMPTY;
        return;
    }
    float fF2 = Pseudodistance(kV0, kV1, m_fMaxTime);
    if (fF2 > 0.0f) {
        fT1 = m_fMaxTime - m_fDerivativeTimeStep;
        fF1 = Pseudodistance(kV0, kV1, fT1);
        float fFDer1 = (fF2 - fF1) * m_fInvDerivativeTimeStep;
        if (fFDer1 < 0.0f) {
            m_fContactTime = 0x1.fffffe0000000p+127f;
            m_eIntersectionType = IT_EMPTY;
            return;
        }
    }
    for (int32_t i = 1; i <= m_iMaxIterations; i++) {
        fT0 -= fF0 / fFDer0;
        if (fT0 > m_fMaxTime) {
            m_fContactTime = 0x1.fffffe0000000p+127f;
            m_eIntersectionType = IT_EMPTY;
            return;
        }
        fF0 = Pseudodistance(kV0, kV1, fT0);
        if (fF0 <= m_fPseudodistanceThreshold) {
            break;
        }
        fT1 = fT0 - m_fDerivativeTimeStep;
        fF1 = Pseudodistance(kV0, kV1, fT1);
        fFDer0 = (fF0 - fF1) * m_fInvDerivativeTimeStep;
        if (fFDer0 >= 0.0f) {
            m_fContactTime = 0x1.fffffe0000000p+127f;
            m_eIntersectionType = IT_EMPTY;
            return;
        }
    }
    m_fContactTime = fT0;
    m_eIntersectionType = IT_CONTACT;
    ComputeContactInformation(kV0, kV1);
}

float NiIntersector::GetContactTime() const {
    return m_fContactTime;
}

const NiPoint3& NiIntersector::GetContactPoint() const {
    return m_kContactPoint;
}

const NiPoint3& NiIntersector::GetContactNormal() const {
    return m_kContactNormal;
}

void NiIntersector::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
