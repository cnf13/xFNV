#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathController.h"

#include "Fallout Shared/Magic/EffectSetting.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

// PathLength's function-local static s_afModRoot: the 5-point Gauss-Legendre quadrature's roots on [0, 1]
// (PathLength_s_afModCoeff holds its weights).
using QuadratureTable = float[5];
static QuadratureTable s_afModRoot{0.046910077f, 0.23076534f, 0.5f, 0.76923466f, 0.9530899f};

// CalcRefFrame's guarded static s_fFourOverPi (4/pi) and its guard.
static float s_fFourOverPi;
static int32_t s_fFourOverPi_guard;

NiRTTI NiPathController::ms_RTTI("NiPathController", &NiTimeController::ms_RTTI);

NiPathController::NiPathController()
  : NiTimeController(), m_uFlags(CVDATA_NEEDS_UPDATE_MASK | CURVE_TYPE_OPEN_MASK), m_uiLastPathIdx(0),
    m_uiLastPctIdx(0), m_spPathData(nullptr), m_spPctData(nullptr), m_pfPartialLength(nullptr), m_fTotalLength(-1.0f),
    m_fMaxBankAngle(0.0f), m_fSmoothing(0.0f), m_sFollowAxis(0), m_fMaxCurvature(0.0f), m_eBankDir(POSITIVE) {}

NiPathController::~NiPathController() {
    m_spPathData = nullptr;
    m_spPctData = nullptr;
    _NiFree(m_pfPartialLength);
}

NiObject* NiPathController::CreateClone(NiCloningProcess& kCloning) {
    NiPathController* pkObject = static_cast<NiPathController*>(NiMemObject::operator new(sizeof(NiPathController)));
    if (pkObject) {
        ::new (pkObject) NiPathController;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The bank direction is read into an uninitialised local and stored to m_eBankDir; a short read leaves the local
// unwritten, so the field gets an uninitialised value, as the game does. Files before 10.0.1.2 keep the flags in the
// base controller's flag word, above its own five bits.
void NiPathController::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= 0x0A000102) {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    else {
        m_uFlags = static_cast<uint16_t>(kStream.m_usNiTimeControllerFlags >> 5);
    }

    int32_t iBankDir;
    NiStreamLoadBinary(kStream, iBankDir);
    m_eBankDir = static_cast<BankDir>(iBankDir);
    NiStreamLoadBinary(kStream, m_fMaxBankAngle);
    NiStreamLoadBinary(kStream, m_fSmoothing);
    NiStreamLoadBinary(kStream, m_sFollowAxis);

    kStream.ReadLinkID();
    kStream.ReadLinkID();
}

// Marks the constant-velocity data stale, rebuilds it when constant velocity is on, and caches the path's largest
// curvature.
void NiPathController::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);

    m_spPathData = static_cast<NiPosData*>(kStream.GetObjectFromLinkID());
    m_spPctData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());

    m_uFlags |= CVDATA_NEEDS_UPDATE_MASK;
    if ((m_uFlags & CONST_VELOCITY_MASK) && (m_uFlags & CVDATA_NEEDS_UPDATE_MASK)) {
        m_fTotalLength = GetTotalPathLength();
        m_uFlags &= ~CVDATA_NEEDS_UPDATE_MASK;
    }

    NiPosData* pkPathData = m_spPathData;
    if (pkPathData) {
        uint32_t uiNumKeys = pkPathData->m_uiNumKeys;
        NiPosKey* pkKeys = pkPathData->m_pkKeys;
        if (uiNumKeys) {
            m_fMaxCurvature = NiPosKey::GetCurvatureFunction(pkPathData->m_eType)(pkKeys, uiNumKeys);
        }
    }
}

bool NiPathController::RegisterStreamables(NiStream& kStream) {
    if (!NiTimeController::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spPathData) {
        m_spPathData->RegisterStreamables(kStream);
    }
    if (m_spPctData) {
        m_spPctData->RegisterStreamables(kStream);
    }
    return true;
}

void NiPathController::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveBinary(kStream, m_eBankDir);
    NiStreamSaveBinary(kStream, m_fMaxBankAngle);
    NiStreamSaveBinary(kStream, m_fSmoothing);
    NiStreamSaveBinary(kStream, m_sFollowAxis);
    kStream.SaveLinkID(m_spPathData);
    kStream.SaveLinkID(m_spPctData);
}

// Both controllers must have path and percentage data (they are compared without a null check). The flag bits are
// compared one at a time; bits 7 and up are not compared.
bool NiPathController::IsEqual(NiObject* pkObject) {
    if (!NiTimeController::IsEqual(pkObject)) {
        return false;
    }

    NiPathController* pkOther = static_cast<NiPathController*>(pkObject);
    if (!m_spPathData->IsEqual(pkOther->m_spPathData)) {
        return false;
    }
    if (!m_spPctData->IsEqual(pkOther->m_spPctData)) {
        return false;
    }

    if (m_pfPartialLength && !pkOther->m_pfPartialLength) {
        return false;
    }
    if (!m_pfPartialLength && pkOther->m_pfPartialLength) {
        return false;
    }
    if (m_pfPartialLength) {
        NiPosKey* pkPathKeys;
        uint32_t uiNumKeys;
        NiPosKey::KeyType ePathType;
        uint8_t ucSize;
        GetPathData(pkPathKeys, uiNumKeys, ePathType, ucSize);
        // EXE-INLINED none: memcmp (a dword loop, then a byte tail; only equality reaches the result)
        const uint8_t* pucThis = static_cast<const uint8_t*>(static_cast<const void*>(m_pfPartialLength));
        const uint8_t* pucOther = static_cast<const uint8_t*>(static_cast<const void*>(pkOther->m_pfPartialLength));
        uint32_t uiSize = uiNumKeys * static_cast<uint32_t>(sizeof(float));
        for (uint32_t i = 0; i < uiSize; ++i) {
            if (pucThis[i] != pucOther[i]) {
                return false;
            }
        }
    }

    uint16_t uDiff = m_uFlags ^ pkOther->m_uFlags;
    return m_fTotalLength == pkOther->m_fTotalLength && !(uDiff & CVDATA_NEEDS_UPDATE_MASK) &&
        !(uDiff & CURVE_TYPE_OPEN_MASK) && m_eBankDir == pkOther->m_eBankDir && !(uDiff & ALLOW_FLIP_MASK) &&
        !(uDiff & BANK_MASK) && !(uDiff & CONST_VELOCITY_MASK) && !(uDiff & FOLLOW_MASK) &&
        m_fMaxBankAngle == pkOther->m_fMaxBankAngle && m_fSmoothing == pkOther->m_fSmoothing &&
        m_sFollowAxis == pkOther->m_sFollowAxis && !(uDiff & FLIP_MASK);
}

void NiPathController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTimeController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiPathController::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);

    if (m_spPathData) {
        m_spPathData->ProcessClone(kCloning);
    }
    if (m_spPctData) {
        m_spPctData->ProcessClone(kCloning);
    }
}

// The percentage keys are interpolated with the path data's key size rather than their own, as the game does.
void NiPathController::Update(NiUpdateData& UpdateData) {
    NiFloatData* pkPctData = m_spPctData;
    if (!pkPctData) {
        return;
    }

    NiFloatKey::KeyType ePctType = pkPctData->m_eType;
    NiFloatKey* pkPctKeys = pkPctData->m_pkKeys;
    uint32_t uiNumPctKeys = pkPctData->m_uiNumKeys;
    if (!pkPctKeys) {
        return;
    }

    NiPosData* pkPathData = m_spPathData;
    if (!pkPathData) {
        return;
    }

    uint8_t ucSize = pkPathData->m_ucKeySize;
    if (!pkPathData->m_pkKeys) {
        return;
    }

    if (DontDoUpdate(UpdateData.fTime)) {
        return;
    }

    float fPercent = NiFloatKey::GenInterp(m_fScaledTime, pkPctKeys, ePctType, uiNumPctKeys, m_uiLastPctIdx, ucSize);

    uint32_t uiI1;
    uint32_t uiI2;
    float fNormTime;
    GetPercentLoc(fPercent, uiI1, uiI2, fNormTime);

    if (m_uFlags & FOLLOW_MASK) {
        NiMatrix3 kRefFrame;
        CalcRefFrame(uiI1, uiI2, fNormTime, kRefFrame);
        static_cast<NiAVObject*>(m_pkTarget)->m_kLocal.m_Rotate = kRefFrame;
    }

    NiPoint3 kLoc = GetLoc(uiI1, uiI2, fNormTime);
    static_cast<NiAVObject*>(m_pkTarget)->m_kLocal.m_Translate = kLoc;
}

bool NiPathController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

// Also does the work of SetConstantVelocityData: the partial lengths are rebuilt when the cached total is negative.
float NiPathController::GetTotalPathLength() {
    if (m_fTotalLength < 0.0f) {
        // EXE-INLINED 0xA49D00
        NiPosData* pkPathData = m_spPathData.m_pObject;
        uint32_t uiNumKeys = pkPathData ? pkPathData->m_uiNumKeys : 0;
        _NiFree(m_pfPartialLength);
        m_pfPartialLength = static_cast<float*>(_NiMalloc(sizeof(float) * uiNumKeys));
        m_pfPartialLength[0] = 0.0f;
        for (uint32_t ui0 = 0; ui0 < uiNumKeys - 1; ++ui0) {
            float fLength = PathLength(static_cast<int32_t>(ui0), static_cast<int32_t>(ui0 + 1), 1.0f);
            m_pfPartialLength[ui0 + 1] = m_pfPartialLength[ui0] + fLength;
        }
        // Reads the last length at index uiNumKeys - 1; with no keys this reads index -1, deliberately, as the game
        // does.
        m_fTotalLength = m_pfPartialLength[static_cast<int32_t>(uiNumKeys) - 1];
    }
    return m_fTotalLength;
}

void NiPathController::CopyMembers(NiPathController* pkDest, NiCloningProcess& kCloning) {
    NiTimeController::CopyMembers(pkDest, kCloning);
    pkDest->m_uFlags = m_uFlags;
    NiPosData* m_pObject = m_spPathData.m_pObject;
    if (m_pObject) {
        NiPosData* v5 = static_cast<NiPosData*>(m_pObject->CreateClone(kCloning));
        pkDest->m_spPathData = v5;
    }
    NiFloatData* v8 = m_spPctData.m_pObject;
    if (v8) {
        NiFloatData* v9 = static_cast<NiFloatData*>(v8->CreateClone(kCloning));
        pkDest->m_spPctData = v9;
    }
    if ((m_uFlags & NiPathController::ALLOW_FLIP_MASK) != 0) {
        pkDest->m_uFlags |= NiPathController::ALLOW_FLIP_MASK;
    }
    else {
        pkDest->m_uFlags &= ~4u;
    }
    if ((m_uFlags & NiPathController::BANK_MASK) != 0) {
        pkDest->m_uFlags |= NiPathController::BANK_MASK;
    }
    else {
        pkDest->m_uFlags &= ~8u;
    }
    pkDest->m_eBankDir = m_eBankDir;
    if ((m_uFlags & NiPathController::CONST_VELOCITY_MASK) != 0) {
        pkDest->m_uFlags |= NiPathController::CONST_VELOCITY_MASK;
        if ((pkDest->m_uFlags & NiPathController::CVDATA_NEEDS_UPDATE_MASK) != 0) {
            pkDest->m_fTotalLength = pkDest->GetTotalPathLength();
            pkDest->m_uFlags &= ~1u;
        }
    }
    else {
        pkDest->m_uFlags &= ~16u;
    }
    if ((m_uFlags & NiPathController::FOLLOW_MASK) != 0) {
        pkDest->m_uFlags |= NiPathController::FOLLOW_MASK;
    }
    else {
        pkDest->m_uFlags &= ~32u;
    }
    pkDest->m_fMaxBankAngle = m_fMaxBankAngle;
    pkDest->m_fSmoothing = m_fSmoothing;
    pkDest->m_sFollowAxis = m_sFollowAxis;
    if ((m_uFlags & NiPathController::FLIP_MASK) != 0) {
        pkDest->m_uFlags |= NiPathController::FLIP_MASK;
    }
    else {
        pkDest->m_uFlags &= ~64u;
    }
    if ((m_uFlags & NiPathController::CURVE_TYPE_OPEN_MASK) != 0) {
        pkDest->m_uFlags |= NiPathController::CURVE_TYPE_OPEN_MASK;
    }
    else {
        pkDest->m_uFlags &= ~2u;
    }
    if (m_pfPartialLength) {
        uint32_t m_uiNumKeys;
        NiPosData* v12 = m_spPathData.m_pObject;
        if (v12) {
            m_uiNumKeys = v12->m_uiNumKeys;
        }
        else {
            m_uiNumKeys = 0;
        }
        size_t v14 = 4 * m_uiNumKeys;
        float* v15 = static_cast<float*>(_NiMalloc(sizeof(float) * m_uiNumKeys));
        pkDest->m_pfPartialLength = v15;
        rt::memcpy(v15, m_pfPartialLength, v14);
    }
    pkDest->m_fTotalLength = m_fTotalLength;
}

// Finds the keys bracketing the time fPercent, searched from the last segment found and wrapping round once. A time
// outside every segment leaves uiI2 0.
void NiPathController::GetPercentLoc(float fPercent, uint32_t& uiI1, uint32_t& uiI2, float& fNormTime) {
    if (m_uFlags & CONST_VELOCITY_MASK) {
        GetPercentLocFromLength(fPercent, uiI1, uiI2, fNormTime);
        m_uiLastPathIdx = uiI1;
        return;
    }

    // EXE-INLINED 0xA49D00
    NiPosData* pkPathData = m_spPathData.m_pObject;
    uint32_t uiNumPathKeys;
    uint8_t ucSize;
    NiPosKey* pkPathKeys;
    if (pkPathData) {
        uiNumPathKeys = pkPathData->m_uiNumKeys;
        ucSize = pkPathData->m_ucKeySize;
        pkPathKeys = pkPathData->m_pkKeys;
    }
    else {
        uiNumPathKeys = 0;
        pkPathKeys = nullptr;
        ucSize = 0;
    }

    if (fPercent <= pkPathKeys->GetKeyAt(0, ucSize)->GetTime()) {
        uiI1 = 0;
        uiI2 = 1;
        fNormTime = 0.0f;
        return;
    }

    if (fPercent >= pkPathKeys->GetKeyAt(uiNumPathKeys - 1, ucSize)->GetTime()) {
        uiI1 = uiNumPathKeys - 2;
        uiI2 = uiNumPathKeys - 1;
        fNormTime = 1.0f;
        return;
    }

    bool bFirstLoop = true;
    uint32_t ui1 = m_uiLastPathIdx;
    uint32_t ui2 = 0;
    while (ui1 < uiNumPathKeys - 1) {
        if (fPercent == pkPathKeys->GetKeyAt(ui1, ucSize)->GetTime() ||
            (fPercent > pkPathKeys->GetKeyAt(ui1, ucSize)->GetTime() &&
             fPercent < pkPathKeys->GetKeyAt(ui1 + 1, ucSize)->GetTime())) {
            ui2 = ui1 + 1;
            m_uiLastPathIdx = ui1;
            break;
        }
        if (ui1 == uiNumPathKeys - 2 && bFirstLoop) {
            ui1 = 0;
            bFirstLoop = false;
        }
        else {
            ui1++;
        }
    }

    fNormTime = (fPercent - pkPathKeys->GetKeyAt(ui1, ucSize)->GetTime()) /
        (pkPathKeys->GetKeyAt(ui2, ucSize)->GetTime() - pkPathKeys->GetKeyAt(ui1, ucSize)->GetTime());
    uiI1 = ui1;
    uiI2 = ui2;
}

// Near either end of a segment (within m_fSmoothing), the normal and curvature are blended with the neighbouring
// segment's; the blend weights are stored as floats.
void NiPathController::CalcRefFrame(uint32_t ui1, uint32_t ui2, float fNormTime, NiMatrix3& kRefFrame) {
    NiMatrix3 kTm;
    kTm = NiMatrix3_IDENTITY;

    // EXE-INLINED 0xA49D00
    NiPosData* pkPathData = m_spPathData.m_pObject;
    if (pkPathData && pkPathData->m_uiNumKeys >= 2) {
        uint32_t uiNumPathKeys = pkPathData->m_uiNumKeys;
        NiPosKey::KeyType ePathType = pkPathData->m_eType;
        uint8_t ucSize = pkPathData->m_ucKeySize;
        NiPosKey* pkPathKeys = pkPathData->m_pkKeys;

        NiPoint3 kTangent;
        NiPoint3 kNormal;
        NiPoint3 kBinormal;
        float fCurvature;
        if (m_uFlags & ALLOW_FLIP_MASK) {
            NiPosKey* pkKey0 = pkPathKeys->GetKeyAt(ui1, ucSize);
            NiPosKey* pkKey1 = pkPathKeys->GetKeyAt(ui2, ucSize);
            NiPosKey::CoordinateFrame(fNormTime, pkKey0, pkKey1, ePathType, kTangent, kNormal, kBinormal, fCurvature);
            if (m_fSmoothing != 0.0f) {
                NiPoint3 kTmpTangent;
                NiPoint3 kNormal1;
                float fCurvature1;
                if (fNormTime >= 1.0f - m_fSmoothing) {
                    uint32_t uiNext = (!(m_uFlags & CURVE_TYPE_OPEN_MASK) && ui2 == uiNumPathKeys - 1) ? 0 : ui2;
                    if (uiNext < uiNumPathKeys - 1) {
                        float fWeight0 = (1.0f - fNormTime + m_fSmoothing) / (m_fSmoothing + m_fSmoothing);
                        float fWeight1 = 1.0f - fWeight0;
                        float fTime = 1.0f - m_fSmoothing;
                        NiPosKey::CoordinateFrame(
                            fTime, pkKey0, pkKey1, ePathType, kTmpTangent, kNormal, kBinormal, fCurvature
                        );
                        NiPosKey::CoordinateFrame(
                            m_fSmoothing, pkPathKeys->GetKeyAt(uiNext, ucSize),
                            pkPathKeys->GetKeyAt(uiNext + 1, ucSize), ePathType, kTmpTangent, kNormal1, kBinormal,
                            fCurvature1
                        );
                        NiPoint3 kPart1 = fWeight1 * kNormal1;
                        NiPoint3 kPart0 = fWeight0 * kNormal;
                        kNormal = kPart0 + kPart1;
                        kNormal.Unitize();
                        fCurvature = fCurvature1 * fWeight1 + fWeight0 * fCurvature;
                        kBinormal = kTangent.Cross(kNormal);
                    }
                }
                else if (fNormTime <= m_fSmoothing) {
                    uint32_t uiPrev = (!(m_uFlags & CURVE_TYPE_OPEN_MASK) && ui1 == 0) ? uiNumPathKeys - 1 : ui1;
                    if (uiPrev >= 1) {
                        float fWeight1 = (m_fSmoothing - fNormTime) / (m_fSmoothing + m_fSmoothing);
                        float fWeight0 = 1.0f - fWeight1;
                        NiPosKey::CoordinateFrame(
                            m_fSmoothing, pkKey0, pkKey1, ePathType, kTmpTangent, kNormal, kBinormal, fCurvature
                        );
                        float fTime = 1.0f - m_fSmoothing;
                        NiPosKey::CoordinateFrame(
                            fTime, pkPathKeys->GetKeyAt(uiPrev - 1, ucSize), pkPathKeys->GetKeyAt(uiPrev, ucSize),
                            ePathType, kTmpTangent, kNormal1, kBinormal, fCurvature1
                        );
                        NiPoint3 kPart1 = fWeight1 * kNormal1;
                        NiPoint3 kPart0 = fWeight0 * kNormal;
                        kNormal = kPart0 + kPart1;
                        kNormal.Unitize();
                        fCurvature = fCurvature1 * fWeight1 + fWeight0 * fCurvature;
                        kBinormal = kTangent.Cross(kNormal);
                    }
                }
            }
            kTm.m_pEntry[0][0] = kTangent.x;
            kTm.m_pEntry[1][0] = kTangent.y;
            kTm.m_pEntry[2][0] = kTangent.z;
            kTm.m_pEntry[0][1] = -kNormal.x;
            kTm.m_pEntry[1][1] = -kNormal.y;
            kTm.m_pEntry[2][1] = -kNormal.z;
            kTm.m_pEntry[0][2] = -kBinormal.x;
            kTm.m_pEntry[1][2] = -kBinormal.y;
            kTm.m_pEntry[2][2] = -kBinormal.z;
        }
        else {
            NiPosKey::CoordinateFrame(
                fNormTime, pkPathKeys->GetKeyAt(ui1, ucSize), pkPathKeys->GetKeyAt(ui2, ucSize), ePathType, kTangent,
                kNormal, kBinormal, fCurvature
            );
            kTm.m_pEntry[0][0] = kTangent.x;
            kTm.m_pEntry[1][0] = kTangent.y;
            kTm.m_pEntry[2][0] = kTangent.z;
            NiPoint3 kUp = NiPoint3_UNIT_Z.UnitCross(kTangent);
            kTm.m_pEntry[0][1] = kUp.x;
            kTm.m_pEntry[1][1] = kUp.y;
            kTm.m_pEntry[2][1] = kUp.z;
            NiPoint3 kCross = kTangent.Cross(kUp);
            kTm.m_pEntry[0][2] = kCross.x;
            kTm.m_pEntry[1][2] = kCross.y;
            kTm.m_pEntry[2][2] = kCross.z;
        }

        if (m_uFlags & FLIP_MASK) {
            kTm.m_pEntry[0][0] = -kTangent.x;
            kTm.m_pEntry[1][0] = -kTangent.y;
            kTm.m_pEntry[2][0] = -kTangent.z;
            kTm.m_pEntry[0][1] = -kTm.m_pEntry[0][1];
            kTm.m_pEntry[1][1] = -kTm.m_pEntry[1][1];
            kTm.m_pEntry[2][1] = -kTm.m_pEntry[2][1];
        }

        // Follow axis 1: the tangent goes to the second column; 2: to the third.
        if (m_sFollowAxis == 1) {
            NiPoint3 kTmp(kTm.m_pEntry[0][0], kTm.m_pEntry[1][0], kTm.m_pEntry[2][0]);
            kTm.m_pEntry[0][0] = -kTm.m_pEntry[0][1];
            kTm.m_pEntry[1][0] = -kTm.m_pEntry[1][1];
            kTm.m_pEntry[2][0] = -kTm.m_pEntry[2][1];
            kTm.m_pEntry[0][1] = kTmp.x;
            kTm.m_pEntry[1][1] = kTmp.y;
            kTm.m_pEntry[2][1] = kTmp.z;
        }
        else if (m_sFollowAxis == 2) {
            NiPoint3 kTmp(kTm.m_pEntry[0][0], kTm.m_pEntry[1][0], kTm.m_pEntry[2][0]);
            kTm.m_pEntry[0][0] = -kTm.m_pEntry[0][2];
            kTm.m_pEntry[1][0] = -kTm.m_pEntry[1][2];
            kTm.m_pEntry[2][0] = -kTm.m_pEntry[2][2];
            kTm.m_pEntry[0][2] = kTmp.x;
            kTm.m_pEntry[1][2] = kTmp.y;
            kTm.m_pEntry[2][2] = kTmp.z;
        }

        if (m_uFlags & BANK_MASK) {
            // s_fFourOverPi (4/pi), initialised on first use.
            if (!(s_fFourOverPi_guard & 1)) {
                s_fFourOverPi_guard |= 1;
                s_fFourOverPi = 1.2732395f;
            }
            float fBankAngle = static_cast<int32_t>(m_eBankDir) * m_fMaxBankAngle;
            if (m_fMaxCurvature > fCurvature) {
                float fRatio = fCurvature / m_fMaxCurvature;
                fRatio = std::atan(fRatio);
                fBankAngle = fRatio * s_fFourOverPi * fBankAngle;
            }
            NiMatrix3 kBankMat;
            kBankMat.MakeRotation(fBankAngle, kTangent.x, kTangent.y, kTangent.z);
            kTm = kBankMat * kTm;
        }
    }
    kRefFrame = kTm;
}

// The path interpolated between keys uiI1 and uiI2 through the key type's interpolation.
NiPoint3 NiPathController::GetLoc(uint32_t uiI1, uint32_t uiI2, float fNormTime) {
    // EXE-INLINED 0xA49D00
    NiPosData* pkPathData = m_spPathData.m_pObject;
    NiPosKey::KeyType ePathType;
    uint8_t ucSize;
    NiPosKey* pkPathKeys;
    if (pkPathData) {
        ePathType = pkPathData->m_eType;
        ucSize = pkPathData->m_ucKeySize;
        pkPathKeys = pkPathData->m_pkKeys;
    }
    else {
        ePathType = NiPosKey::NOINTERP;
        pkPathKeys = nullptr;
        ucSize = 0;
    }
    NiAnimationKey::InterpFunction interp = NiAnimationKey::ms_interps
        [static_cast<uint32_t>(NiAnimationKey::POSKEY) * NiAnimationKey::NUMKEYTYPES + ePathType];
    NiPoint3 kResult;
    interp(fNormTime, pkPathKeys->GetKeyAt(uiI1, ucSize), pkPathKeys->GetKeyAt(uiI2, ucSize), &kResult);
    return kResult;
}

float NiPathController::PathSpeed(int32_t iI0, int32_t iI1, float fTime) const {
    // EXE-INLINED 0xA49D00
    NiPosData* pkPathData = m_spPathData.m_pObject;
    NiPosKey* pkPathKeys;
    NiPosKey::KeyType ePathType;
    uint8_t ucSize;
    if (pkPathData) {
        ePathType = pkPathData->m_eType;
        ucSize = pkPathData->m_ucKeySize;
        pkPathKeys = pkPathData->m_pkKeys;
    }
    else {
        ePathType = NiPosKey::NOINTERP;
        pkPathKeys = nullptr;
        ucSize = 0;
    }
    NiAnimationKey::InterpFunction interpD1 = NiAnimationKey::ms_interpd1s
        [static_cast<uint32_t>(NiAnimationKey::POSKEY) * NiAnimationKey::NUMKEYTYPES + ePathType];
    NiPoint3 kDeriv;
    interpD1(fTime, pkPathKeys->GetKeyAt(iI0, ucSize), pkPathKeys->GetKeyAt(iI1, ucSize), &kDeriv);
    return kDeriv.Length();
}

float NiPathController::PathLength(int32_t iI0, int32_t iI1, float fTime) const {
    static PathLength_s_afModCoeff_t s_afModCoeff{0.11846344f, 0.23931433f, 0.28444445f, 0.23931433f, 0.11846344f};
    float v10 = 0.0f;
    for (int32_t i = 0; i < 5; ++i) {
        float aiI1a = s_afModRoot[i] * fTime;
        double v7 = PathSpeed(iI0, iI1, aiI1a) * s_afModCoeff[i];
        v10 = v7 + v10;
    }
    return v10 * fTime;
}

// The partial-length difference before the divide and the Newton step's quotient are computed in double, as the game
// does; every stored intermediate is a float.
void NiPathController::GetPercentLocFromLength(float afPercent, uint32_t& auiI1, uint32_t& auiI2, float& afNormTime) {
    // EXE-INLINED none: GetPathKeys (only the key count is read)
    NiPosData* pkPathData = m_spPathData.m_pObject;
    uint32_t uiNumPathKeys = pkPathData ? pkPathData->m_uiNumKeys : 0;
    if (afPercent == 0.0f) {
        auiI1 = 0;
        auiI2 = 1;
        afNormTime = 0.0f;
        return;
    }
    if (afPercent < 0.0f) {
        // EXE-INLINED none: NiFmod (the exe's __CIfmod)
        float fMod = static_cast<float>(std::fmod(afPercent, 1.0));
        afPercent = fMod + 1.0f;
    }
    if (afPercent == 1.0f) {
        auiI1 = uiNumPathKeys - 2;
        auiI2 = uiNumPathKeys - 1;
        afNormTime = 1.0f;
        return;
    }
    if (afPercent > 1.0f) {
        // EXE-INLINED none: NiFmod (the exe's __CIfmod)
        float fMod = static_cast<float>(std::fmod(afPercent, 1.0));
        afPercent = fMod;
    }
    float fLength = afPercent * m_fTotalLength;
    float fDist = 0.0f;
    float fTime = 0.0f;
    uint32_t ui0 = 0;
    uint32_t ui1 = 1;
    for (; ui0 < uiNumPathKeys - 1; ui0++, ui1++) {
        if (fLength <= m_pfPartialLength[ui1]) {
            fDist = fLength - m_pfPartialLength[ui0];
            fTime = static_cast<float>(
                static_cast<double>(fDist) /
                (static_cast<double>(m_pfPartialLength[ui1]) - static_cast<double>(m_pfPartialLength[ui0]))
            );
            break;
        }
    }
    for (int32_t i = 0; i < 32; i++) {
        float fDiff = PathLength(ui0, ui1, fTime) - fDist;
        float fAbs = NiAbs(fDiff);
        if (fAbs <= 9.999999747378752e-05) {
            break;
        }
        double dTime = fTime;
        fTime =
            static_cast<float>(dTime - static_cast<double>(fDiff) / static_cast<double>(PathSpeed(ui0, ui1, fTime)));
    }
    auiI1 = ui0;
    auiI2 = ui1;
    afNormTime = fTime;
}

void NiPathController::GetPathData(
    NiPosKey*& apPathKeys,
    uint32_t& auiNumKeys,
    NiPosKey::KeyType& aeType,
    uint8_t& aucSize
) const {
    NiPosData* pkPathData = m_spPathData.m_pObject;
    if (pkPathData) {
        auiNumKeys = pkPathData->m_uiNumKeys;
        aeType = pkPathData->m_eType;
        aucSize = pkPathData->m_ucKeySize;
        apPathKeys = pkPathData->m_pkKeys;
    }
    else {
        auiNumKeys = 0;
        aeType = NiPosKey::NOINTERP;
        apPathKeys = nullptr;
        aucSize = 0;
    }
}

NiObject* NiPathController::CreateObject() {
    return new NiPathController;
}
