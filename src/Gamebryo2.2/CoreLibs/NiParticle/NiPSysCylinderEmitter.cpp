#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCylinderEmitter.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"

NiRTTI NiPSysCylinderEmitter::ms_RTTI("NiPSysCylinderEmitter", &NiPSysVolumeEmitter::ms_RTTI);

// This file's own non-const copy of 2 pi; no code writes it.
static float s_fTwoPi = 6.2831855f;

NiPSysCylinderEmitter::NiPSysCylinderEmitter()
  : NiPSysVolumeEmitter(), m_fEmitterRadius(0.0f), m_fEmitterHeight(0.0f) {}

NiPSysCylinderEmitter::~NiPSysCylinderEmitter() {}

NiObject* NiPSysCylinderEmitter::CreateClone(NiCloningProcess& kCloning) {
    NiPSysCylinderEmitter* pkObject_2 = new NiPSysCylinderEmitter;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_fEmitterRadius = m_fEmitterRadius;
    pkObject_2->m_fEmitterHeight = m_fEmitterHeight;
    return pkObject_2;
}

void NiPSysCylinderEmitter::LoadBinary(NiStream& kStream) {
    NiPSysVolumeEmitter::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fEmitterRadius);
    NiStreamLoadBinary(kStream, m_fEmitterHeight);
}

void NiPSysCylinderEmitter::LinkObject(NiStream& kStream) {
    NiPSysVolumeEmitter::LinkObject(kStream);
}

bool NiPSysCylinderEmitter::RegisterStreamables(NiStream& kStream) {
    return NiPSysVolumeEmitter::RegisterStreamables(kStream);
}

void NiPSysCylinderEmitter::SaveBinary(NiStream& kStream) {
    NiPSysVolumeEmitter::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fEmitterRadius);
    NiStreamSaveBinary(kStream, m_fEmitterHeight);
}

bool NiPSysCylinderEmitter::IsEqual(NiObject* pkObject) {
    if (!NiPSysVolumeEmitter::IsEqual(pkObject)) {
        return false;
    }

    NiPSysCylinderEmitter* pkOther = static_cast<NiPSysCylinderEmitter*>(pkObject);
    if (m_fEmitterRadius != pkOther->m_fEmitterRadius) {
        return false;
    }
    if (m_fEmitterHeight != pkOther->m_fEmitterHeight) {
        return false;
    }
    return true;
}

void NiPSysCylinderEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysVolumeEmitter::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysCylinderEmitter::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Radius", m_fEmitterRadius));
    pkStrings->Add(NiGetViewerString("Height", m_fEmitterHeight));
}

void NiPSysCylinderEmitter::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysVolumeEmitter::ProcessClone(kCloning);
}

// A random radius and angle on the disk (the angle's sine and cosine come from the 512-entry tables), then a random
// height along the axis.
void NiPSysCylinderEmitter::ComputeVolumeInitialPositionAndVelocity(
    NiTransform& kEmitterToPSys,
    NiPoint3& kPosition,
    NiPoint3& kVelocity
) {
    float fRadius = m_fEmitterRadius * NiUnitRandom();
    float fAngle = s_fTwoPi * NiUnitRandom();
    int32_t iAngle = static_cast<int32_t>(512.0 / s_fTwoPi * fAngle) & 0x1FF;
    kPosition.x = fRadius * s_fCosTableA[iAngle];
    kPosition.y = fRadius * s_fSinTableA[iAngle];
    kPosition.z = static_cast<float>((NiUnitRandom() - 0.5) * m_fEmitterHeight);

    kPosition = kEmitterToPSys * kPosition;
    const NiMatrix3& kRotate = kEmitterToPSys.m_Rotate;
    float fX = kVelocity.x;
    float fY = kVelocity.y;
    float fZ = kVelocity.z;
    kVelocity.x = kRotate.m_pEntry[0][0] * fX + kRotate.m_pEntry[0][1] * fY + kRotate.m_pEntry[0][2] * fZ;
    kVelocity.y = kRotate.m_pEntry[1][0] * fX + kRotate.m_pEntry[1][1] * fY + kRotate.m_pEntry[1][2] * fZ;
    kVelocity.z = kRotate.m_pEntry[2][0] * fX + kRotate.m_pEntry[2][1] * fY + kRotate.m_pEntry[2][2] * fZ;
}

void NiPSysCylinderEmitter::SetEmitterHeight(float fEmitterHeight) {
    m_fEmitterHeight = fEmitterHeight;
}

NiObject* NiPSysCylinderEmitter::CreateObject() {
    return new NiPSysCylinderEmitter;
}
