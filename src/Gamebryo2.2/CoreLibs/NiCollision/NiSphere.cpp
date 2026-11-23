#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphere.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"

bool NiSphere::operator==(const NiSphere& kSphere) const {
    return m_kCenter.x == kSphere.m_kCenter.x &&
        m_kCenter.y == kSphere.m_kCenter.y &&
        m_kCenter.z == kSphere.m_kCenter.z &&
        m_fRadius == kSphere.m_fRadius;
}

void NiSphere::LoadBinary(NiStream& kStream) {
    m_kCenter.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fRadius);
}

void NiSphere::SaveBinary(NiStream& kStream) {
    m_kCenter.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fRadius);
}

NiSphere* NiSphere::Copy(NiSphere* apSphere) {
    SetCenterAndRadius(apSphere->m_kCenter, apSphere->m_fRadius);
    return this;
}

void NiSphere::SetCenterAndRadius(const NiPoint3& kCenter, float fRadius) {
    m_kCenter = kCenter;
    m_fRadius = fRadius;
}
