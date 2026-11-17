#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"

#include "BSMain/BSParentVelocityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

// This file's own copy of 2π (stored data, never written); Update and Initialize read it, not NI_TWO_PI.
static float s_fTwoPi = 6.2831855f;

// The angle limit is a function-local static with a guard word. The guard is read once, before the loop, its bit is set
// and the value written on the first particle that needs it, and the guard is written back after the loop, as the game
// does.
static float fMaxAngle;
static uint32_t fMaxAngle_guard;

NiRTTI NiPSysRotationModifier::ms_RTTI("NiPSysRotationModifier", &NiPSysModifier::ms_RTTI);

NiPSysRotationModifier::NiPSysRotationModifier()
  : NiPSysModifier(), m_fInitialRotSpeed(0.0f), m_fInitialRotSpeedVar(0.0f), m_fInitialRotAngle(0.0f),
    m_fInitialRotAngleVar(0.0f), m_kInitialAxis(NiPoint3_UNIT_X), m_bRandomInitialAxis(true),
    m_bRandomRotSpeedSign(false) {}

NiPSysRotationModifier::~NiPSysRotationModifier() {}

NiObject* NiPSysRotationModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysRotationModifier* pkObject =
        static_cast<NiPSysRotationModifier*>(NiMemObject::operator new(sizeof(NiPSysRotationModifier)));
    if (pkObject) {
        ::new (pkObject) NiPSysRotationModifier;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(0, kCloning);
        return 0;
    }
}

// Files before 20.0.0.2 store only the initial speed and the random-axis flag.
// KEEP: the exe reads the random-axis flag into a byte of its frame nothing wrote (and the random-sign flag into a
// reused slot), so a short read (malformed data) leaves stack garbage there and the exe stores it; src cannot express
// it. porting.md's unwritten-slot exception covers GetChunkData only.
void NiPSysRotationModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fInitialRotSpeed);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 0, 0, 2)) {
        NiStreamLoadBinary(kStream, m_fInitialRotSpeedVar);
        NiStreamLoadBinary(kStream, m_fInitialRotAngle);
        NiStreamLoadBinary(kStream, m_fInitialRotAngleVar);
        uint8_t ucRandomRotSpeedSign;
        NiStreamLoadBinary(kStream, ucRandomRotSpeedSign);
        m_bRandomRotSpeedSign = ucRandomRotSpeedSign != 0;
    }

    uint8_t ucRandomInitialAxis;
    NiStreamLoadBinary(kStream, ucRandomInitialAxis);
    m_bRandomInitialAxis = ucRandomInitialAxis != 0;
    m_kInitialAxis.LoadBinary(kStream);
}

void NiPSysRotationModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
}

bool NiPSysRotationModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysRotationModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fInitialRotSpeed);
    NiStreamSaveBinary(kStream, m_fInitialRotSpeedVar);
    NiStreamSaveBinary(kStream, m_fInitialRotAngle);
    NiStreamSaveBinary(kStream, m_fInitialRotAngleVar);
    uint8_t ucRandomRotSpeedSign = m_bRandomRotSpeedSign;
    NiStreamSaveBinary(kStream, ucRandomRotSpeedSign);
    uint8_t ucRandomInitialAxis = m_bRandomInitialAxis;
    NiStreamSaveBinary(kStream, ucRandomInitialAxis);
    m_kInitialAxis.SaveBinary(kStream);
}

bool NiPSysRotationModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysRotationModifier* pkOther = static_cast<NiPSysRotationModifier*>(pkObject);
    if (pkOther->m_fInitialRotSpeed != m_fInitialRotSpeed || pkOther->m_fInitialRotSpeedVar != m_fInitialRotSpeedVar ||
        pkOther->m_bRandomRotSpeedSign != m_bRandomRotSpeedSign || pkOther->m_fInitialRotAngle != m_fInitialRotAngle ||
        pkOther->m_fInitialRotAngleVar != m_fInitialRotAngleVar ||
        pkOther->m_bRandomInitialAxis != m_bRandomInitialAxis || pkOther->m_kInitialAxis != m_kInitialAxis) {
        return false;
    }

    return true;
}

void NiPSysRotationModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysRotationModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Initial Rotation Speed", m_fInitialRotSpeed));
    pkStrings->Add(NiGetViewerString("Initial Rotation Speed Variation", m_fInitialRotSpeedVar));
    pkStrings->Add(NiGetViewerString("Random Rot Speed Sign", m_bRandomRotSpeedSign));
    pkStrings->Add(NiGetViewerString("Initial Rotation Angle", m_fInitialRotAngle));
    pkStrings->Add(NiGetViewerString("Initial Rotation Angle Variation", m_fInitialRotAngleVar));
    pkStrings->Add(NiGetViewerString("Random Initial Axis", m_bRandomInitialAxis));
    pkStrings->Add(m_kInitialAxis.GetViewerString("Initial Axis"));
}

// Files before 20.0.0.2 have no per-particle speeds: every active particle gets the initial speed.
void NiPSysRotationModifier::PostLinkObject(NiStream& kStream) {
    NiPSysModifier::PostLinkObject(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 0, 0, 2)) {
        NiPSysData* pkData = static_cast<NiPSysData*>(m_pkTarget->m_spModelData.m_pObject);
        float* pfRotationSpeeds = pkData->m_pfRotationSpeeds;
        if (pfRotationSpeeds) {
            for (uint16_t i = 0; i < static_cast<NiPSysData*>(m_pkTarget->m_spModelData.m_pObject)->m_usActive; i++) {
                pfRotationSpeeds[i] = m_fInitialRotSpeed;
            }
        }
    }
}

void NiPSysRotationModifier::Update(float fTime, NiPSysData* pkData) {
    float* pfRotationAngles = pkData->m_pfRotationAngles;
    if (!pfRotationAngles) {
        return;
    }

    uint32_t uiGuard = fMaxAngle_guard;
    float* pfRotationSpeeds = pkData->m_pfRotationSpeeds;
    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        float fDeltaT = fTime - pkData->m_pkParticleInfo[i].m_fLastUpdate;
        float fAngle = fDeltaT * pfRotationSpeeds[i] + pfRotationAngles[i];
        pfRotationAngles[i] = fAngle;

        // Ten half-turns, as a float.
        if (!(uiGuard & 1)) {
            uiGuard |= 1;
            fMaxAngle = 31.4159279f;
        }
        if (fAngle > fMaxAngle) {
            pfRotationAngles[i] = 0.0f;
        }
        else {
            while (pfRotationAngles[i] > s_fTwoPi) {
                pfRotationAngles[i] -= s_fTwoPi;
            }
        }
    }
    fMaxAngle_guard = uiGuard;
}

// A random axis is a uniform point on the sphere, from the lookup tables. A random sign flips the speed half the
// time.
void NiPSysRotationModifier::Initialize(NiPSysData* pkData, uint16_t usNewParticle) {
    NiPoint3* pkRotationAxes = pkData->m_pkRotationAxes;
    if (pkRotationAxes) {
        NiPoint3& kAxis = pkRotationAxes[usNewParticle];
        if (m_bRandomInitialAxis) {
            float fPolar = NiUnitRandom();
            fPolar = fPolar * 3.1415927410125732;
            float fCosPolar = s_fCosTableA[static_cast<int32_t>(512.0 / s_fTwoPi * fPolar) & 0x1FF];
            float fSqr = 1.0f - fCosPolar * fCosPolar;
            float fSinPolar = static_cast<float>(std::sqrt(fSqr));

            float fAzimuth = NiUnitRandom();
            fAzimuth = s_fTwoPi * fAzimuth;
            int32_t iIndex = static_cast<int32_t>(512.0 / s_fTwoPi * fAzimuth) & 0x1FF;
            float fCosAzimuth = s_fCosTableA[iIndex];
            kAxis.x = fSinPolar * s_fSinTableA[iIndex];
            kAxis.y = fSinPolar * fCosAzimuth;
            kAxis.z = fCosPolar;
        }
        else {
            kAxis = m_kInitialAxis;
        }
    }

    if (pkData->m_pfRotationAngles) {
        float fAngle = m_fInitialRotAngleVar * NiSymmetricRandom() + m_fInitialRotAngle;
        pkData->m_pfRotationAngles[usNewParticle] = fAngle;

        float fSpeed = m_fInitialRotSpeedVar * NiSymmetricRandom() + m_fInitialRotSpeed;
        if (m_bRandomRotSpeedSign && NiUnitRandom() <= 0.5f) {
            fSpeed = -fSpeed;
        }
        pkData->m_pfRotationSpeeds[usNewParticle] = fSpeed;
    }
}

void NiPSysRotationModifier::CopyMembers(NiPSysRotationModifier* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifier::CopyMembers(pkDest, kCloning);
    pkDest->m_fInitialRotSpeed = m_fInitialRotSpeed;
    pkDest->m_fInitialRotSpeedVar = m_fInitialRotSpeedVar;
    pkDest->m_bRandomRotSpeedSign = m_bRandomRotSpeedSign;
    pkDest->m_fInitialRotAngle = m_fInitialRotAngle;
    pkDest->m_fInitialRotAngleVar = m_fInitialRotAngleVar;
    pkDest->m_bRandomInitialAxis = m_bRandomInitialAxis;
    pkDest->m_kInitialAxis.x = m_kInitialAxis.x;
    pkDest->m_kInitialAxis.y = m_kInitialAxis.y;
    pkDest->m_kInitialAxis.z = m_kInitialAxis.z;
}

NiObject* NiPSysRotationModifier::CreateObject() {
    NiPSysRotationModifier* pkObject =
        static_cast<NiPSysRotationModifier*>(NiMemObject::operator new(sizeof(NiPSysRotationModifier)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiPSysRotationModifier;
}
