#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathInterpolator.h"

#include "Fallout Shared/Magic/EffectSetting.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

#include <cfloat>

// The 5-point Gauss-Legendre quadrature's weights and its roots on [0, 1], used by PathLength.
using QuadratureTable = float[5];
static QuadratureTable s_afModCoeff{0.11846344f, 0.23931433f, 0.28444445f, 0.23931433f, 0.11846344f};
static QuadratureTable s_afModRoot{0.046910077f, 0.23076534f, 0.5f, 0.76923466f, 0.9530899f};

// CalcRefFrame's lazily initialised static s_fFourOverPi (4/pi) and its guard.
static float s_fFourOverPi;
static int32_t s_fFourOverPi_guard;

NiRTTI NiPathInterpolator::ms_RTTI("NiPathInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

NiPathInterpolator::NiPathInterpolator(NiPosData* pkPathData, NiFloatData* pkPctData)
  : m_uFlags(3), m_uiLastPathIdx(0), m_uiLastPctIdx(0), m_spPathData(pkPathData), m_spPctData(pkPctData),
    m_pfPartialLength(nullptr), m_fTotalLength(-1.0f), m_fMaxBankAngle(0.0f), m_fSmoothing(0.0f), m_sFollowAxis(0),
    m_fMaxCurvature(0.0f), m_eBankDir(POSITIVE), m_kRotation(INVALID_QUATERNION), m_kPosition(INVALID_POINT3),
    m_fLastTime(-3.4028235e+38f) {}

NiPathInterpolator::~NiPathInterpolator() {
    m_spPathData = nullptr;
    m_spPctData = nullptr;
    _NiFree(m_pfPartialLength);
}

NiObject* NiPathInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiPathInterpolator* v3 = static_cast<NiPathInterpolator*>(NiMemObject::operator new(sizeof(NiPathInterpolator)));
    if (v3) {
        NiPathInterpolator* v4 = ::new (v3) NiPathInterpolator(nullptr, nullptr);
        CopyMembers(v4, &kCloning);
        return v4;
    }

    CopyMembers(nullptr, &kCloning);
    return nullptr;
}

// The bank direction is read into an uninitialised local and stored to m_eBankDir; a short read leaves the local
// unwritten, so the field gets whatever it held, an uninitialised read kept deliberately as the game does.
void NiPathInterpolator::LoadBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uFlags);
    int32_t iBankDir;
    NiStreamLoadBinary(kStream, iBankDir);
    m_eBankDir = static_cast<BankDir>(iBankDir);
    NiStreamLoadBinary(kStream, m_fMaxBankAngle);
    NiStreamLoadBinary(kStream, m_fSmoothing);
    NiStreamLoadBinary(kStream, m_sFollowAxis);

    kStream.ReadLinkID();
    kStream.ReadLinkID();
}

void NiPathInterpolator::LinkObject(NiStream& kStream) {
    NiKeyBasedInterpolator::LinkObject(kStream);

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

bool NiPathInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
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

void NiPathInterpolator::SaveBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveBinary(kStream, m_eBankDir);
    NiStreamSaveBinary(kStream, m_fMaxBankAngle);
    NiStreamSaveBinary(kStream, m_fSmoothing);
    NiStreamSaveBinary(kStream, m_sFollowAxis);
    kStream.SaveLinkID(m_spPathData);
    kStream.SaveLinkID(m_spPctData);
}

// The flag bits are compared one at a time; bits 7 and up are not compared.
bool NiPathInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiPathInterpolator* pkOther = static_cast<NiPathInterpolator*>(pkObject);
    if ((m_spPathData && !pkOther->m_spPathData) || (!m_spPathData && pkOther->m_spPathData) ||
        (m_spPathData && !m_spPathData->IsEqual(pkOther->m_spPathData))) {
        return false;
    }
    if ((m_spPctData && !pkOther->m_spPctData) || (!m_spPctData && pkOther->m_spPctData) ||
        (m_spPctData && !m_spPctData->IsEqual(pkOther->m_spPctData))) {
        return false;
    }

    if ((m_pfPartialLength && !pkOther->m_pfPartialLength) || (!m_pfPartialLength && pkOther->m_pfPartialLength)) {
        return false;
    }
    if (m_pfPartialLength) {
        uint32_t uiNumKeys;
        NiPosKey::KeyType eType;
        uint8_t ucSize;
        GetPathKeys(uiNumKeys, eType, ucSize);
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

void NiPathInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiKeyBasedInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiPathInterpolator::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);

    if (m_spPathData) {
        m_spPathData->ProcessClone(kCloning);
    }
    if (m_spPctData) {
        m_spPctData->ProcessClone(kCloning);
    }
}

// The unchanged-time check reads NiInterpolator::m_fLastTime, while the time is recorded in this class's own
// m_fLastTime. The rotation is only produced when following the path.
bool NiPathInterpolator::Update(float fTime, NiObjectNET*, NiQuatTransform& kValue) {
    if (fTime == NiInterpolator::m_fLastTime) {
        if (m_kPosition == INVALID_POINT3) {
            kValue.m_kTranslate.x = -FLT_MAX;
            kValue.m_kRotate.m_fX = -FLT_MAX;
            kValue.m_fScale = -FLT_MAX;
            return false;
        }

        kValue.m_kTranslate = m_kPosition;
        if (m_uFlags & FOLLOW_MASK) {
            kValue.m_kRotate = m_kRotation;
        }
        return true;
    }

    NiFloatData* pkPctData = m_spPctData;
    if (!pkPctData) {
        return false;
    }
    uint8_t ucPctSize = pkPctData->m_ucKeySize;
    uint32_t uiNumPctKeys = pkPctData->m_uiNumKeys;
    NiFloatKey::KeyType ePctType = pkPctData->m_eType;
    NiFloatKey* pkPctKeys = pkPctData->m_pkKeys;
    if (!pkPctKeys || !m_spPathData || !m_spPathData->m_pkKeys) {
        return false;
    }

    // The percentage and the normalised time share one variable: GetLoc's afNormTime starts out holding the percentage.
    float fNormTime = NiFloatKey::GenInterp(fTime, pkPctKeys, ePctType, uiNumPctKeys, m_uiLastPctIdx, ucPctSize);

    uint32_t uiI1;
    uint32_t uiI2;
    GetLoc(fNormTime, uiI1, uiI2, fNormTime);

    if (m_uFlags & FOLLOW_MASK) {
        NiMatrix3 kRefFrame;
        CalcRefFrame(uiI1, uiI2, fNormTime, kRefFrame);
        m_kRotation.FromRotation(kRefFrame);
    }

    m_kPosition = GenPathInterp(uiI1, uiI2, fNormTime);
    kValue.m_kTranslate = m_kPosition;
    if (m_uFlags & FOLLOW_MASK) {
        kValue.m_kRotate = m_kRotation;
    }

    m_fLastTime = fTime;
    return true;
}

// The range of the percentage keys; [0, 0] without them.
void NiPathInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
    fBeginKeyTime = FLT_MAX;
    fEndKeyTime = -FLT_MAX;

    if (GetKeyCount(PERCENTAGE) > 0) {
        NiAnimationKey* pkBeginKey = GetKeyAt(0, PERCENTAGE);
        NiAnimationKey* pkEndKey = GetKeyAt(GetKeyCount(PERCENTAGE) - 1, PERCENTAGE);
        if (pkBeginKey && pkEndKey) {
            if (fBeginKeyTime > pkBeginKey->m_fTime) {
                fBeginKeyTime = pkBeginKey->m_fTime;
            }
            if (fEndKeyTime < pkEndKey->m_fTime) {
                fEndKeyTime = pkEndKey->m_fTime;
            }
        }
    }

    if (fBeginKeyTime == FLT_MAX && fEndKeyTime == -FLT_MAX) {
        fBeginKeyTime = 0.0f;
        fEndKeyTime = 0.0f;
    }
}

void NiPathInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spPctData) {
        m_spPctData->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
    }
}

// The sequence copy gets the percentage data cut to [afStartTime, afEndTime]; the copy is not null-checked.
NiInterpolator* NiPathInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiPathInterpolator* pkSeqInterp =
        static_cast<NiPathInterpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
    if (m_spPctData) {
        NiPointer<NiFloatData> spNewData;
        m_spPctData->GetSequenceData(fStartTime, fEndTime, spNewData);
        pkSeqInterp->m_spPctData = spNewData.m_pObject;
        pkSeqInterp->m_uiLastPctIdx = 0;
    }
    return pkSeqInterp;
}

uint32_t NiPathInterpolator::GetKeyCount(uint16_t usChannel) const {
    if (usChannel == PATH) {
        if (m_spPathData) {
            return m_spPathData->m_uiNumKeys;
        }
    }
    else if (usChannel == PERCENTAGE) {
        if (m_spPctData) {
            return m_spPctData->m_uiNumKeys;
        }
    }
    return 0;
}

NiAnimationKey::KeyType NiPathInterpolator::GetKeyType(uint16_t usChannel) const {
    if (usChannel == PATH) {
        if (m_spPathData) {
            return m_spPathData->m_eType;
        }
    }
    else if (usChannel == PERCENTAGE) {
        if (m_spPctData) {
            return m_spPctData->m_eType;
        }
    }
    return NiAnimationKey::NOINTERP;
}

NiAnimationKey::KeyContent NiPathInterpolator::GetKeyContent(uint16_t usChannel) const {
    if (usChannel == PATH) {
        return NiAnimationKey::POSKEY;
    }
    if (usChannel == PERCENTAGE) {
        return NiAnimationKey::FLOATKEY;
    }
    return NiAnimationKey::NUMKEYCONTENTS;
}

NiAnimationKey* NiPathInterpolator::GetKeyArray(uint16_t usChannel) const {
    if (usChannel == PATH) {
        if (m_spPathData) {
            return m_spPathData->m_pkKeys;
        }
    }
    else if (usChannel == PERCENTAGE) {
        if (m_spPctData) {
            return m_spPctData->m_pkKeys;
        }
    }
    return nullptr;
}

uint8_t NiPathInterpolator::GetKeyStride(uint16_t usChannel) const {
    if (usChannel == PATH) {
        if (m_spPathData) {
            return m_spPathData->m_ucKeySize;
        }
    }
    else if (usChannel == PERCENTAGE) {
        if (m_spPctData) {
            return m_spPctData->m_ucKeySize;
        }
    }
    return 0;
}

NiPosKey* NiPathInterpolator::GetPathKeys(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
    if (!m_spPathData) {
        uiNumKeys = 0;
        eType = NiAnimationKey::NOINTERP;
        ucSize = 0;
        return nullptr;
    }

    uiNumKeys = m_spPathData->m_uiNumKeys;
    eType = m_spPathData->m_eType;
    ucSize = m_spPathData->m_ucKeySize;
    return m_spPathData->m_pkKeys;
}

float NiPathInterpolator::GetTotalPathLength() {
    if (m_fTotalLength < 0.0f) {
        NiPosData* pkPathData = m_spPathData.m_pObject;
        uint32_t uiNumKeys = pkPathData ? pkPathData->m_uiNumKeys : 0;
        _NiFree(m_pfPartialLength);
        m_pfPartialLength = static_cast<float*>(_NiMalloc(sizeof(float) * uiNumKeys));
        m_pfPartialLength[0] = 0.0f;
        uint32_t ui0 = 0;
        if (uiNumKeys - 1 != 0) {
            do {
                float fLength = PathLength(static_cast<int32_t>(ui0), ui0 + 1, 1.0f);
                m_pfPartialLength[ui0 + 1] = m_pfPartialLength[ui0] + fLength;
                ++ui0;
            }
            while (ui0 < uiNumKeys - 1);
        }
        // Reads the last element of m_pfPartialLength; with no keys this reads the element before the array, as the
        // game does.
        m_fTotalLength = m_pfPartialLength[static_cast<int32_t>(uiNumKeys) - 1];
    }
    return m_fTotalLength;
}

bool NiPathInterpolator::GetFollow() {
    return (m_uFlags & FOLLOW_MASK) != 0;
}

NiPoint3 NiPathInterpolator::GenPathInterp(int32_t iI1, int32_t iI2, float fNormTime) {
    // EXE-INLINED none: GetPathKeys (the exe reads the path data's fields inline here)
    NiPosData* pkPathData = m_spPathData.m_pObject;
    NiPosKey* pkPathKeys = nullptr;
    NiPosKey::KeyType ePathType;
    uint8_t ucSize;
    if (pkPathData) {
        ePathType = pkPathData->m_eType;
        ucSize = pkPathData->m_ucKeySize;
        pkPathKeys = pkPathData->m_pkKeys;
    }
    else {
        ePathType = NiPosKey::NOINTERP;
        ucSize = 0;
    }
    NiPoint3 kResult;
    NiAnimationKey::InterpFunction pfnInterp = NiAnimationKey::ms_interps
        [static_cast<uint32_t>(NiAnimationKey::POSKEY) * NiAnimationKey::NUMKEYTYPES + ePathType];
    pfnInterp(fNormTime, pkPathKeys->GetKeyAt(iI1, ucSize), pkPathKeys->GetKeyAt(iI2, ucSize), &kResult);
    return kResult;
}

void NiPathInterpolator::CalcRefFrame(uint32_t ui1, uint32_t ui2, float afNormTime, NiMatrix3& kRefFrame) {
    NiMatrix3 mat;
    NiPosData* pkPathData = m_spPathData.m_pObject;
    mat = NiMatrix3_IDENTITY;
    if (pkPathData) {
        NiAnimationKey::KeyType ePathType = pkPathData->m_eType;
        uint8_t ucSize = pkPathData->m_ucKeySize;
        NiPosKey* pkPathKeys = pkPathData->m_pkKeys;
        uint32_t uiNumPathKeys = pkPathData->m_uiNumKeys;
        if (uiNumPathKeys >= 2) {
            float x;
            float y;
            float z;
            float fCurvature;
            NiPoint3 kTangent;
            NiPoint3 kNormal;
            NiPoint3 kBinormal;
            if ((m_uFlags & ALLOW_FLIP_MASK) != 0) {
                NiPosKey* pKey1 = pkPathKeys->GetKeyAt(ui2, ucSize);
                NiPosKey* pKey0 = pkPathKeys->GetKeyAt(ui1, ucSize);
                NiPosKey::CoordinateFrame(
                    afNormTime, pKey0, pKey1, ePathType, kTangent, kNormal, kBinormal, fCurvature
                );
                if (0.0f != m_fSmoothing) {
                    // fA0 weighs this segment's frame, fA1 the neighbouring segment's.
                    float fA0;
                    float fA1;
                    float fCurvature1;
                    uint32_t uiI;
                    NiPoint3 kTmpTan;
                    NiPoint3 kNormal1;
                    double fNormTime = afNormTime;
                    if (!(1.0 - m_fSmoothing <= afNormTime)) {
                        if (m_fSmoothing >= fNormTime) {
                            if ((m_uFlags & CURVE_TYPE_OPEN_MASK) != 0 || ui1) {
                                uiI = ui1;
                            }
                            else {
                                uiI = uiNumPathKeys - 1;
                            }
                            if (uiI) {
                                fA1 = (m_fSmoothing - fNormTime) / (m_fSmoothing + m_fSmoothing);
                                fA0 = 1.0 - fA1;
                                NiPosKey::CoordinateFrame(
                                    m_fSmoothing, pKey0, pKey1, ePathType, kTmpTan, kNormal, kBinormal, fCurvature
                                );
                                float fTime = 1.0 - m_fSmoothing;
                                NiPosKey::CoordinateFrame(
                                    fTime, pkPathKeys->GetKeyAt(uiI - 1, ucSize), pkPathKeys->GetKeyAt(uiI, ucSize),
                                    ePathType, kTmpTan, kNormal1, kBinormal, fCurvature1
                                );
                                NiPoint3 kTmp1 = fA1 * kNormal1;
                                NiPoint3 kTmp0 = fA0 * kNormal;
                                kNormal = kTmp0 + kTmp1;
                                kNormal.Unitize();
                                fCurvature = fCurvature1 * fA1 + fA0 * fCurvature;
                                kBinormal = kTangent.Cross(kNormal);
                            }
                        }
                    }
                    else {
                        if ((m_uFlags & CURVE_TYPE_OPEN_MASK) != 0 || ui2 != uiNumPathKeys - 1) {
                            uiI = ui2;
                        }
                        else {
                            uiI = 0;
                        }
                        if (uiI < uiNumPathKeys - 1) {
                            fA0 = (1.0 - fNormTime + m_fSmoothing) / (m_fSmoothing + m_fSmoothing);
                            fA1 = 1.0 - fA0;
                            float fTime = 1.0 - m_fSmoothing;
                            NiPosKey::CoordinateFrame(
                                fTime, pKey0, pKey1, ePathType, kTmpTan, kNormal, kBinormal, fCurvature
                            );
                            NiPosKey::CoordinateFrame(
                                m_fSmoothing, pkPathKeys->GetKeyAt(uiI, ucSize), pkPathKeys->GetKeyAt(uiI + 1, ucSize),
                                ePathType, kTmpTan, kNormal1, kBinormal, fCurvature1
                            );
                            NiPoint3 kTmp1 = fA1 * kNormal1;
                            NiPoint3 kTmp0 = fA0 * kNormal;
                            kNormal = kTmp0 + kTmp1;
                            kNormal.Unitize();
                            fCurvature = fCurvature1 * fA1 + fA0 * fCurvature;
                            kBinormal = kTangent.Cross(kNormal);
                        }
                    }
                }
                x = kTangent.x;
                mat.m_pEntry[0][0] = kTangent.x;
                y = kTangent.y;
                mat.m_pEntry[1][0] = kTangent.y;
                z = kTangent.z;
                mat.m_pEntry[2][0] = kTangent.z;
                mat.m_pEntry[0][1] = -kNormal.x;
                mat.m_pEntry[1][1] = -kNormal.y;
                mat.m_pEntry[2][1] = -kNormal.z;
                mat.m_pEntry[0][2] = -kBinormal.x;
                mat.m_pEntry[1][2] = -kBinormal.y;
                mat.m_pEntry[2][2] = -kBinormal.z;
            }
            else {
                NiPosKey::CoordinateFrame(
                    afNormTime, pkPathKeys->GetKeyAt(ui1, ucSize), pkPathKeys->GetKeyAt(ui2, ucSize), ePathType,
                    kTangent, kNormal, kBinormal, fCurvature
                );
                mat.m_pEntry[0][0] = kTangent.x;
                mat.m_pEntry[1][0] = kTangent.y;
                mat.m_pEntry[2][0] = kTangent.z;
                NiPoint3 kUp = NiPoint3_UNIT_Z.UnitCross(kTangent);
                mat.m_pEntry[0][1] = kUp.x;
                mat.m_pEntry[1][1] = kUp.y;
                mat.m_pEntry[2][1] = kUp.z;
                NiPoint3 kCross = kTangent.Cross(kUp);
                mat.m_pEntry[0][2] = kCross.x;
                mat.m_pEntry[1][2] = kCross.y;
                mat.m_pEntry[2][2] = kCross.z;
                y = kTangent.y;
                z = kTangent.z;
                x = kTangent.x;
            }
            uint16_t uFlags = m_uFlags;
            if ((uFlags & FLIP_MASK) != 0) {
                mat.m_pEntry[0][0] = -x;
                mat.m_pEntry[1][0] = -y;
                mat.m_pEntry[2][0] = -z;
                mat.m_pEntry[0][1] = -mat.m_pEntry[0][1];
                mat.m_pEntry[1][1] = -mat.m_pEntry[1][1];
                mat.m_pEntry[2][1] = -mat.m_pEntry[2][1];
            }
            // The follow axis: 1 is Y, 2 is Z (the reference source's values); X needs no swap.
            if (m_sFollowAxis == 1) {
                float fX = mat.m_pEntry[0][0];
                float fY = mat.m_pEntry[1][0];
                float fZ = mat.m_pEntry[2][0];
                mat.m_pEntry[0][0] = -mat.m_pEntry[0][1];
                mat.m_pEntry[1][0] = -mat.m_pEntry[1][1];
                mat.m_pEntry[2][0] = -mat.m_pEntry[2][1];
                mat.m_pEntry[0][1] = fX;
                mat.m_pEntry[1][1] = fY;
                mat.m_pEntry[2][1] = fZ;
            }
            else if (m_sFollowAxis == 2) {
                float fX = mat.m_pEntry[0][0];
                float fY = mat.m_pEntry[1][0];
                float fZ = mat.m_pEntry[2][0];
                mat.m_pEntry[0][0] = -mat.m_pEntry[0][2];
                mat.m_pEntry[1][0] = -mat.m_pEntry[1][2];
                mat.m_pEntry[2][0] = -mat.m_pEntry[2][2];
                mat.m_pEntry[0][2] = fX;
                mat.m_pEntry[1][2] = fY;
                mat.m_pEntry[2][2] = fZ;
            }
            if ((uFlags & BANK_MASK) != 0) {
                NiMatrix3 kBankMat;
                if ((s_fFourOverPi_guard & 1) == 0) {
                    s_fFourOverPi_guard |= 1u;
                    s_fFourOverPi = 1.2732395f;
                }
                float fBankAmt = static_cast<int32_t>(m_eBankDir) * m_fMaxBankAngle;
                if (m_fMaxCurvature > fCurvature) {
                    float fTmp = fCurvature / m_fMaxCurvature;
                    fTmp = std::atan(fTmp);
                    fBankAmt = fTmp * s_fFourOverPi * fBankAmt;
                    x = kTangent.x;
                    z = kTangent.z;
                    y = kTangent.y;
                }
                kBankMat.MakeRotation(fBankAmt, x, y, z);
                mat = kBankMat * mat;
            }
        }
    }
    kRefFrame = mat;
}

void NiPathInterpolator::GetLoc(float fTime, uint32_t& uiI1, uint32_t& uiI2, float& fNormTime) {
    if ((m_uFlags & CONST_VELOCITY_MASK) != 0) {
        GetPercentLoc(fTime, uiI1, uiI2, fNormTime);
        m_uiLastPathIdx = uiI1;
        return;
    }

    // EXE-INLINED none: GetPathKeys (the exe reads the path data's fields inline here)
    NiPosData* pkPathData = m_spPathData.m_pObject;
    NiPosKey* pkPathKeys = nullptr;
    uint32_t uiNumPathKeys;
    uint8_t ucSize;
    if (pkPathData) {
        uiNumPathKeys = pkPathData->m_uiNumKeys;
        ucSize = pkPathData->m_ucKeySize;
        pkPathKeys = pkPathData->m_pkKeys;
    }
    else {
        uiNumPathKeys = 0;
        ucSize = 0;
    }

    if (pkPathKeys->GetKeyAt(0, ucSize)->m_fTime >= fTime) {
        uiI1 = 0;
        uiI2 = 1;
        fNormTime = 0.0f;
        return;
    }
    if (pkPathKeys->GetKeyAt(uiNumPathKeys - 1, ucSize)->m_fTime <= fTime) {
        uiI1 = uiNumPathKeys - 2;
        uiI2 = uiNumPathKeys - 1;
        fNormTime = 1.0f;
        return;
    }

    uint32_t ui1 = m_uiLastPathIdx;
    uint32_t ui2 = 0;
    bool bFirstLoop = true;
    if (ui1 < uiNumPathKeys - 1) {
        while (pkPathKeys->GetKeyAt(ui1, ucSize)->m_fTime != fTime &&
               (!(pkPathKeys->GetKeyAt(ui1, ucSize)->m_fTime < fTime) ||
                !(pkPathKeys->GetKeyAt(ui1 + 1, ucSize)->m_fTime > fTime))) {
            if (ui1 == uiNumPathKeys - 2 && bFirstLoop) {
                ui1 = 0;
                bFirstLoop = false;
            }
            else {
                ++ui1;
            }
            if (ui1 >= uiNumPathKeys - 1) {
                goto NotFound;
            }
        }
        ui2 = ui1 + 1;
        m_uiLastPathIdx = ui1;
    }
NotFound:
    fNormTime = (fTime - pkPathKeys->GetKeyAt(ui1, ucSize)->m_fTime) /
        (pkPathKeys->GetKeyAt(ui2, ucSize)->m_fTime - pkPathKeys->GetKeyAt(ui1, ucSize)->m_fTime);
    uiI1 = ui1;
    uiI2 = ui2;
}

float NiPathInterpolator::PathSpeed(int32_t iI0, int32_t iI1, float fTime) const {
    // EXE-INLINED none: GetPathKeys (the exe reads the path data's fields inline here)
    NiPosData* pkPathData = m_spPathData.m_pObject;
    NiPosKey* pkPathKeys = nullptr;
    NiPosKey::KeyType ePathType;
    uint8_t ucSize;
    if (pkPathData) {
        ePathType = pkPathData->m_eType;
        ucSize = pkPathData->m_ucKeySize;
        pkPathKeys = pkPathData->m_pkKeys;
    }
    else {
        ePathType = NiPosKey::NOINTERP;
        ucSize = 0;
    }
    NiAnimationKey::InterpFunction interpD1 = NiAnimationKey::ms_interpd1s
        [static_cast<uint32_t>(NiAnimationKey::POSKEY) * NiAnimationKey::NUMKEYTYPES + ePathType];
    NiPoint3 kDeriv;
    interpD1(fTime, pkPathKeys->GetKeyAt(iI0, ucSize), pkPathKeys->GetKeyAt(iI1, ucSize), &kDeriv);
    return kDeriv.Length();
}

float NiPathInterpolator::PathLength(int32_t iI0, int32_t iI1, float fTime) const {
    float fResult = 0.0f;
    for (int32_t i = 0; i < 5; i++) {
        fResult += s_afModCoeff[i] * PathSpeed(iI0, iI1, fTime * s_afModRoot[i]);
    }
    fResult *= fTime;
    return fResult;
}

// The partial-length difference before the divide and the Newton step's quotient are computed in double, as the game
// does; every stored intermediate is a float.
void NiPathInterpolator::GetPercentLoc(float fPercent, uint32_t& uiI1, uint32_t& uiI2, float& fNormTime) {
    // EXE-INLINED none: GetPathKeys (only the key count is read)
    NiPosData* pkPathData = m_spPathData.m_pObject;
    uint32_t uiNumPathKeys = pkPathData ? pkPathData->m_uiNumKeys : 0;
    if (fPercent == 0.0f) {
        uiI1 = 0;
        uiI2 = 1;
        fNormTime = 0.0f;
        return;
    }
    if (fPercent < 0.0f) {
        // EXE-INLINED none: NiFmod (the exe's __CIfmod)
        float fMod = static_cast<float>(std::fmod(fPercent, 1.0));
        fPercent = fMod + 1.0f;
    }
    if (fPercent == 1.0f) {
        uiI1 = uiNumPathKeys - 2;
        uiI2 = uiNumPathKeys - 1;
        fNormTime = 1.0f;
        return;
    }
    if (fPercent > 1.0f) {
        // EXE-INLINED none: NiFmod (the exe's __CIfmod)
        float fMod = static_cast<float>(std::fmod(fPercent, 1.0));
        fPercent = fMod;
    }
    float fLength = fPercent * m_fTotalLength;
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
    uiI1 = ui0;
    uiI2 = ui1;
    fNormTime = fTime;
}

void NiPathInterpolator::SetConstantVelocityData() {
    if ((m_uFlags & NiPathInterpolator::CVDATA_NEEDS_UPDATE_MASK) != 0) {
        m_fTotalLength = GetTotalPathLength();
        m_uFlags &= ~1u;
    }
}

void NiPathInterpolator::CopyMembers(NiPathInterpolator* apDest, NiCloningProcess* arCloning) {
    NiKeyBasedInterpolator::CopyMembers(apDest, *arCloning);
    apDest->m_uFlags = m_uFlags;
    NiPosData* m_pObject = m_spPathData.m_pObject;
    if (m_pObject) {
        NiPosData* v5 = static_cast<NiPosData*>(m_pObject->CreateClone(*arCloning));
        apDest->m_spPathData = v5;
    }
    NiFloatData* v8 = m_spPctData.m_pObject;
    if (v8) {
        NiFloatData* v9 = static_cast<NiFloatData*>(v8->CreateClone(*arCloning));
        apDest->m_spPctData = v9;
    }
    if ((m_uFlags & NiPathInterpolator::ALLOW_FLIP_MASK) != 0) {
        apDest->m_uFlags |= NiPathInterpolator::ALLOW_FLIP_MASK;
    }
    else {
        apDest->m_uFlags &= ~4u;
    }
    if ((m_uFlags & NiPathInterpolator::BANK_MASK) != 0) {
        apDest->m_uFlags |= NiPathInterpolator::BANK_MASK;
    }
    else {
        apDest->m_uFlags &= ~8u;
    }
    apDest->m_eBankDir = m_eBankDir;
    if ((m_uFlags & NiPathInterpolator::CONST_VELOCITY_MASK) != 0) {
        apDest->m_uFlags |= NiPathInterpolator::CONST_VELOCITY_MASK;
        if ((apDest->m_uFlags & NiPathInterpolator::CVDATA_NEEDS_UPDATE_MASK) != 0) {
            apDest->m_fTotalLength = apDest->GetTotalPathLength();
            apDest->m_uFlags &= ~1u;
        }
    }
    else {
        apDest->m_uFlags &= ~16u;
    }
    if ((m_uFlags & NiPathInterpolator::FOLLOW_MASK) != 0) {
        apDest->m_uFlags |= NiPathInterpolator::FOLLOW_MASK;
    }
    else {
        apDest->m_uFlags &= ~32u;
    }
    apDest->m_fMaxBankAngle = m_fMaxBankAngle;
    apDest->m_fSmoothing = m_fSmoothing;
    apDest->m_sFollowAxis = m_sFollowAxis;
    if ((m_uFlags & NiPathInterpolator::FLIP_MASK) != 0) {
        apDest->m_uFlags |= NiPathInterpolator::FLIP_MASK;
    }
    else {
        apDest->m_uFlags &= ~64u;
    }
    if ((m_uFlags & NiPathInterpolator::CURVE_TYPE_OPEN_MASK) != 0) {
        apDest->m_uFlags |= NiPathInterpolator::CURVE_TYPE_OPEN_MASK;
    }
    else {
        apDest->m_uFlags &= ~2u;
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
        apDest->m_pfPartialLength = v15;
        rt::memcpy(v15, m_pfPartialLength, v14);
    }
    apDest->m_fTotalLength = m_fTotalLength;
}

NiObject* NiPathInterpolator::CreateObject() {
    NiPathInterpolator* v0 = static_cast<NiPathInterpolator*>(NiMemObject::operator new(sizeof(NiPathInterpolator)));
    if (v0) {
        return ::new (v0) NiPathInterpolator(nullptr, nullptr);
    }

    return nullptr;
}
