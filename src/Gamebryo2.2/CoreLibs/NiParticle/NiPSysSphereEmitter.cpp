#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSphereEmitter.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"

NiRTTI NiPSysSphereEmitter::ms_RTTI("NiPSysSphereEmitter", &NiPSysVolumeEmitter::ms_RTTI);

// This file's non-const copy of 2 pi; no code writes it.
static float s_fTwoPi = 6.2831855f;

NiPSysSphereEmitter::NiPSysSphereEmitter() : NiPSysVolumeEmitter(), m_fEmitterRadius(0.0f) {}

NiPSysSphereEmitter::~NiPSysSphereEmitter() {}

NiObject* NiPSysSphereEmitter::CreateClone(NiCloningProcess& kCloning) {
    NiPSysSphereEmitter* pkObject_2 = new NiPSysSphereEmitter;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_fEmitterRadius = m_fEmitterRadius;
    return pkObject_2;
}

void NiPSysSphereEmitter::LoadBinary(NiStream& kStream) {
    NiPSysVolumeEmitter::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fEmitterRadius);
}

void NiPSysSphereEmitter::LinkObject(NiStream& kStream) {
    NiPSysVolumeEmitter::LinkObject(kStream);
}

bool NiPSysSphereEmitter::RegisterStreamables(NiStream& kStream) {
    return NiPSysVolumeEmitter::RegisterStreamables(kStream);
}

void NiPSysSphereEmitter::SaveBinary(NiStream& kStream) {
    NiPSysVolumeEmitter::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fEmitterRadius);
}

bool NiPSysSphereEmitter::IsEqual(NiObject* pkObject) {
    if (!NiPSysVolumeEmitter::IsEqual(pkObject)) {
        return false;
    }

    NiPSysSphereEmitter* pkOther = static_cast<NiPSysSphereEmitter*>(pkObject);
    return m_fEmitterRadius == pkOther->m_fEmitterRadius;
}

void NiPSysSphereEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysVolumeEmitter::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysSphereEmitter::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Radius", m_fEmitterRadius));
}

void NiPSysSphereEmitter::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysVolumeEmitter::ProcessClone(kCloning);
}

// A random radius and two random angles (sines and cosines from the 512-entry tables). The radius times the second
// angle's sine is not rounded to float before the last multiply.
void NiPSysSphereEmitter::ComputeVolumeInitialPositionAndVelocity(
    NiTransform& kEmitterToPSys,
    NiPoint3& kPosition,
    NiPoint3& kVelocity
) {
    float fRadius = m_fEmitterRadius * NiUnitRandom();
    float fAngle1 = s_fTwoPi * NiUnitRandom();
    float fRandom = NiUnitRandom();
    double dToIndex = 512.0 / s_fTwoPi;
    int32_t iAngle1 = static_cast<int32_t>(fAngle1 * dToIndex) & 0x1FF;
    float fAngle2 = (s_fTwoPi * fRandom);
    int32_t iAngle2 = static_cast<int32_t>(dToIndex * fAngle2) & 0x1FF;
    kPosition.x = fRadius * s_fSinTableA[iAngle2] * s_fCosTableA[iAngle1];
    kPosition.y = fRadius * s_fSinTableA[iAngle2] * s_fSinTableA[iAngle1];
    kPosition.z = fRadius * s_fCosTableA[iAngle2];

    kPosition = kEmitterToPSys * kPosition;
    const NiMatrix3& kRotate = kEmitterToPSys.m_Rotate;
    float fX = kVelocity.x;
    float fY = kVelocity.y;
    float fZ = kVelocity.z;
    kVelocity.x = kRotate.m_pEntry[0][0] * fX + kRotate.m_pEntry[0][1] * fY + kRotate.m_pEntry[0][2] * fZ;
    kVelocity.y = kRotate.m_pEntry[1][0] * fX + kRotate.m_pEntry[1][1] * fY + kRotate.m_pEntry[1][2] * fZ;
    kVelocity.z = kRotate.m_pEntry[2][0] * fX + kRotate.m_pEntry[2][1] * fY + kRotate.m_pEntry[2][2] * fZ;
}

NiObject* NiPSysSphereEmitter::CreateObject() {
    return new NiPSysSphereEmitter;
}
