#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiPlane::NiPlane() : m_kNormal(NiPoint3_ZERO), m_fConstant(0.0f) {}

NiPlane::NiPlane(NiPoint3* kP0, NiPoint3* kP1, NiPoint3* kP2) {
    NiPoint3 kDif1(kP1->x - kP0->x, kP1->y - kP0->y, kP1->z - kP0->z);
    NiPoint3 kDif2(kP2->x - kP1->x, kP2->y - kP1->y, kP2->z - kP1->z);
    m_kNormal = kDif1.UnitCross(kDif2);
    m_fConstant = m_kNormal.y * kP0->y + kP0->x * m_kNormal.x + m_kNormal.z * kP0->z;
}

NiPlane::NiPlane(const NiPoint3& kNormal, float fConstant) : m_kNormal(kNormal), m_fConstant(fConstant) {}

NiPlane::NiPlane(const NiPoint3& kNormal, const NiPoint3& kPoint)
  : m_kNormal(kNormal), m_fConstant(kNormal.y * kPoint.y + kNormal.x * kPoint.x + kNormal.z * kPoint.z) {}

float NiPlane::Distance(const NiPoint3& kPoint) const {
    return m_kNormal.Dot(kPoint) - m_fConstant;
}

bool NiPlane::operator==(const NiPlane& kPlane) const {
    return m_kNormal.x == kPlane.m_kNormal.x && m_kNormal.y == kPlane.m_kNormal.y &&
        m_kNormal.z == kPlane.m_kNormal.z && m_fConstant == kPlane.m_fConstant;
}

bool NiPlane::operator!=(const NiPlane& kPlane) const {
    return !(
        m_kNormal.x == kPlane.m_kNormal.x && m_kNormal.y == kPlane.m_kNormal.y && m_kNormal.z == kPlane.m_kNormal.z &&
        m_fConstant == kPlane.m_fConstant
    );
}

int32_t NiPlane::WhichSide(const NiPoint3& kPoint) const {
    float fDistance = m_kNormal.x * kPoint.x + m_kNormal.y * kPoint.y + m_kNormal.z * kPoint.z - m_fConstant;
    if (fDistance < 0.0f) {
        return 2;
    }
    if (fDistance > 0.0f) {
        return 1;
    }
    return 0;
}

void NiPlane::LoadBinary(NiStream& stream) {
    m_kNormal.LoadBinary(stream);
    NiStreamLoadBinary(stream, m_fConstant);
}

void NiPlane::SaveBinary(NiStream& stream) {
    m_kNormal.SaveBinary(stream);
    NiStreamSaveBinary(stream, m_fConstant);
}

char* NiPlane::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 56;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString, stLength, "(%g,%g,%g), %g", static_cast<double>(m_kNormal.x), static_cast<double>(m_kNormal.y),
        static_cast<double>(m_kNormal.z), static_cast<double>(m_fConstant)
    );
    return pcString;
}
