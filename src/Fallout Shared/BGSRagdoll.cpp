#include "Fallout Shared/BGSRagdoll.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "SDK/MSVC/rtti.h"

#include <new>

using FootIKLegArray = BSSimpleArray<FootIK_DATA::FootIK_Leg_DATA>;
using LookIKHeadArray = BSSimpleArray<LookIK_DATA::LookIK_Head_DATA>;
using GrabIKArmArray = BSSimpleArray<GrabIK_DATA::GrabIK_Arm_DATA>;

using BSSimpleArray_GrabIK_DATA_GrabIK_Arm_DATA_1024 = BSSimpleArray<GrabIK_DATA::GrabIK_Arm_DATA, 1024>;

// The enabled flags are single bytes.
void RagdollSaveStruct::Endian() {
    ENDIAN(usFeedbackDynamicBoneCount, false);
    ENDIAN(usLegCount, false);
    ENDIAN(usHeadCount, false);
    ENDIAN(usArmCount, false);
}

RagdollFeedbackData::RagdollFeedbackSystemData::RagdollFeedbackSystemData()
  : fKeyDynIntrp(0.9f), fHeirGain(0.8f), fPosGain(0.4f), fVelGain(0.2f), fAccGain(0.1f), fSnapGain(0.3f),
    fVelDamp(0.0f), fSnapMaxLinVel(50.0f), fSnapMaxAngVel(50.0f), fSnapMaxLinDist(25.0f), fSnapMaxAngDist(25.0f),
    fPosMaxLinVel(50.0f), fPosMaxAngVel(50.0f), uiImpulseMult1(10000), uiImpulseMult2(30000) {}

void RagdollFeedbackData::RagdollFeedbackSystemData::Endian() {
    ENDIAN(fKeyDynIntrp, false);
    ENDIAN(fHeirGain, false);
    ENDIAN(fPosGain, false);
    ENDIAN(fVelGain, false);
    ENDIAN(fAccGain, false);
    ENDIAN(fSnapGain, false);
    ENDIAN(fVelDamp, false);
    ENDIAN(fSnapMaxLinVel, false);
    ENDIAN(fSnapMaxAngVel, false);
    ENDIAN(fSnapMaxLinDist, false);
    ENDIAN(fSnapMaxAngDist, false);
    ENDIAN(fPosMaxLinVel, false);
    ENDIAN(fPosMaxAngVel, false);
    ENDIAN(uiImpulseMult1, false);
    ENDIAN(uiImpulseMult2, false);
}

RagdollFeedbackData::RagdollFeedbackData() : bEnabled(false) {}

RagdollFeedbackData::~RagdollFeedbackData() {}

void RagdollFeedbackData::Init() {
    bEnabled = false;
    SystemData.~RagdollFeedbackSystemData();
    ::new (&SystemData) RagdollFeedbackData::RagdollFeedbackSystemData;
    DynamicBoneIdxA.~BSSimpleArray<int16_t, 1024>();
    ::new (&DynamicBoneIdxA) BSSimpleArray<int16_t, 1024>;
}

bool RagdollFeedbackData::Compare(RagdollFeedbackData* a1) {
    uint32_t iSize = DynamicBoneIdxA.iSize;
    bool v3 = false;
    if (iSize != a1->DynamicBoneIdxA.iSize) {
        return true;
    }
    int32_t v4 = 0;
    if (iSize) {
        while (!v3) {
            v3 = DynamicBoneIdxA.pBuffer[v4] != a1->DynamicBoneIdxA.pBuffer[v4];
            if (++v4 >= iSize) {
                if (v3) {
                    return true;
                }
                goto LABEL_6;
            }
        }
        return true;
    }
LABEL_6:;
    if (bEnabled != a1->bEnabled ||
        (a1->SystemData.fKeyDynIntrp != SystemData.fKeyDynIntrp ||
         (a1->SystemData.fHeirGain != SystemData.fHeirGain ||
          (a1->SystemData.fPosGain != SystemData.fPosGain ||
           (a1->SystemData.fVelGain != SystemData.fVelGain ||
            (a1->SystemData.fAccGain != SystemData.fAccGain ||
             (a1->SystemData.fSnapGain != SystemData.fSnapGain ||
              (a1->SystemData.fVelDamp != SystemData.fVelDamp ||
               (a1->SystemData.fSnapMaxLinVel != SystemData.fSnapMaxLinVel ||
                (a1->SystemData.fSnapMaxAngVel != SystemData.fSnapMaxAngVel ||
                 (a1->SystemData.fSnapMaxLinDist != SystemData.fSnapMaxLinDist ||
                  (a1->SystemData.fSnapMaxAngDist != SystemData.fSnapMaxAngDist ||
                   (a1->SystemData.fPosMaxLinVel != SystemData.fPosMaxLinVel ||
                    (SystemData.uiImpulseMult1 != a1->SystemData.uiImpulseMult1 ||
                     (SystemData.uiImpulseMult2 != a1->SystemData.uiImpulseMult2 ||
                      a1->SystemData.fPosMaxAngVel != SystemData.fPosMaxAngVel))))))))))))))) {
        return true;
    }
    return v3;
}

PoseMatching_DATA::PoseMatching_System_DATA::PoseMatching_System_DATA()
  : sRootMatchingBone(-1), sMatchingBone2(-1), sMatchingBone3(-1), bDisableOnActivate(false), fMotorStrength(0.0f),
    fDelayTime(0.0f), fErrorAllowance(0.1f), fDisableDisplacement(0.0f) {}

// The disable flag is a single byte.
void PoseMatching_DATA::PoseMatching_System_DATA::Endian() {
    ENDIAN(sRootMatchingBone, false);
    ENDIAN(sMatchingBone2, false);
    ENDIAN(sMatchingBone3, false);
    ENDIAN(fMotorStrength, false);
    ENDIAN(fDelayTime, false);
    ENDIAN(fErrorAllowance, false);
    ENDIAN(fDisableDisplacement, false);
}

PoseMatching_DATA::PoseMatching_DATA() : bEnabled(false) {}

PoseMatching_DATA::~PoseMatching_DATA() {}

bool PoseMatching_DATA::Compare(PoseMatching_DATA* a1) {
    bool bret = PSAFilename.StrCmp(a1->PSAFilename, true) != 0;
    if (bret ||
        (bEnabled != a1->bEnabled ||
         (SystemData.sRootMatchingBone != a1->SystemData.sRootMatchingBone ||
          (SystemData.sMatchingBone2 != a1->SystemData.sMatchingBone2 ||
           (SystemData.sMatchingBone3 != a1->SystemData.sMatchingBone3 ||
            (a1->SystemData.fMotorStrength != SystemData.fMotorStrength ||
             (a1->SystemData.fErrorAllowance != SystemData.fErrorAllowance ||
              (SystemData.bDisableOnActivate != a1->SystemData.bDisableOnActivate ||
               (a1->SystemData.fDisableDisplacement != SystemData.fDisableDisplacement ||
                a1->SystemData.fDelayTime != SystemData.fDelayTime))))))))) {
        return true;
    }
    return bret;
}

void PoseMatching_DATA::Init() {
    bEnabled = false;
    PSAFilename.~BSStringT<char>();
    ::new (&PSAFilename) BSStringT<char>;
    SystemData.~PoseMatching_System_DATA();
    ::new (&SystemData) PoseMatching_DATA::PoseMatching_System_DATA;
}

void PoseMatching_DATA::Copy(const PoseMatching_DATA& arData) {
    PSAFilename.Copy(arData.PSAFilename);
    bEnabled = arData.bEnabled;
    SystemData.sRootMatchingBone = arData.SystemData.sRootMatchingBone;
    SystemData.sMatchingBone2 = arData.SystemData.sMatchingBone2;
    SystemData.sMatchingBone3 = arData.SystemData.sMatchingBone3;
    SystemData.fMotorStrength = arData.SystemData.fMotorStrength;
    SystemData.fErrorAllowance = arData.SystemData.fErrorAllowance;
    SystemData.bDisableOnActivate = arData.SystemData.bDisableOnActivate;
    SystemData.fDelayTime = arData.SystemData.fDelayTime;
    SystemData.fDisableDisplacement = arData.SystemData.fDisableDisplacement;
}

LookIK_DATA::LookIK_Head_DATA::LookIK_Head_DATA()
  : sNeckIdx(-1), sHeadIdx(-1), fLimitConeAngle(0.0f), fLookAtTargetGain(0.0f), fLookAtGain(0.0f) {}

bool LookIK_DATA::LookIK_Head_DATA::operator==(const LookIK_DATA::LookIK_Head_DATA& arData) const {
    bool v3 = false;
    if (sNeckIdx == arData.sNeckIdx && sHeadIdx == arData.sHeadIdx && arData.fLimitConeAngle == fLimitConeAngle &&
        arData.fLookAtTargetGain == fLookAtTargetGain && arData.fLookAtGain == fLookAtGain &&
        arData.fForwardNeckMS[0u] == fForwardNeckMS[0u] && arData.fForwardNeckMS[1] == fForwardNeckMS[1] &&
        arData.fForwardNeckMS[2] == fForwardNeckMS[2] && arData.fForwardHeadLS[0u] == fForwardHeadLS[0u] &&
        arData.fForwardHeadLS[1] == fForwardHeadLS[1]) {
        return arData.fForwardHeadLS[2] == fForwardHeadLS[2];
    }
    return v3;
}

bool LookIK_DATA::LookIK_Head_DATA::operator!=(const LookIK_DATA::LookIK_Head_DATA& arData) const {
    return !(*this == arData);
}

LookIK_DATA::LookIK_DATA() : bEnabled(false) {}

char LookIK_DATA::Compare(LookIK_DATA* a1) {
    uint32_t FormFlags = HeadSystemsA.iSize;
    bool bret = FormFlags != a1->HeadSystemsA.iSize;
    if (!bret) {
        for (uint32_t auiIndex = 0; auiIndex < HeadSystemsA.iSize && !bret; ++auiIndex) {
            bret = HeadSystemsA.GetAt(static_cast<int32_t>(auiIndex)) !=
                a1->HeadSystemsA.GetAt(static_cast<int32_t>(auiIndex));
        }
    }
    uint32_t v3 = HeadSystemsNameA.iSize;
    if (v3 != a1->HeadSystemsNameA.iSize) {
        bret = true;
    }
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < HeadSystemsNameA.iSize && !bret; ++uiIndex) {
            bret = BSstrcmp(
                       HeadSystemsNameA.GetAt(static_cast<int32_t>(uiIndex)),
                       a1->HeadSystemsNameA.GetAt(static_cast<int32_t>(uiIndex))
                   ) != 0;
        }
    }
    if (bret || bEnabled != a1->bEnabled) {
        return 1;
    }
    return bret;
}

void LookIK_DATA::Copy(const LookIK_DATA& arData) {
    HeadSystemsA.Clear(true);
    // The names are const, so the const is cast away to free each one.
    for (const char* pName : HeadSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    HeadSystemsNameA.Clear(true);
    uint32_t FormFlags = arData.HeadSystemsA.iSize;
    HeadSystemsA.SetSize(FormFlags, true);
    HeadSystemsNameA.SetSize(arData.HeadSystemsNameA.iSize, true);
    for (uint32_t auiIndex = 0; auiIndex < arData.HeadSystemsA.iSize; ++auiIndex) {
        // The source's element is read first (C++17 sequences the right operand of = first).
        HeadSystemsA.GetAt(static_cast<int32_t>(auiIndex)) = arData.HeadSystemsA.GetAt(static_cast<int32_t>(auiIndex));
    }
    for (uint32_t i = 0; i < arData.HeadSystemsNameA.iSize; ++i) {
        uint32_t uiSize = BSstrlen(arData.HeadSystemsNameA.GetAt(static_cast<int32_t>(i))) + 1;
        char* pName = static_cast<char*>(ourMalloc(uiSize));
        BSstrcpy(pName, static_cast<int32_t>(uiSize), arData.HeadSystemsNameA.GetAt(static_cast<int32_t>(i)));
        HeadSystemsNameA.GetAt(static_cast<int32_t>(i)) = pName;
    }
    bEnabled = arData.bEnabled;
}

GrabIK_DATA::GrabIK_Arm_DATA::GrabIK_Arm_DATA() : sShoulderIndex(-1), sElbowIndex(-1), sHandIndex(-1) {}

bool GrabIK_DATA::GrabIK_Arm_DATA::operator==(const GrabIK_DATA::GrabIK_Arm_DATA& arData) {
    bool v3 = false;
    if (sShoulderIndex == arData.sShoulderIndex && sElbowIndex == arData.sElbowIndex) {
        return sHandIndex == arData.sHandIndex;
    }
    return v3;
}

bool GrabIK_DATA::GrabIK_Arm_DATA::operator!=(const GrabIK_DATA::GrabIK_Arm_DATA& arData) {
    return !(*this == arData);
}

GrabIK_DATA::GrabIK_DATA() : bEnabled(false) {}

char GrabIK_DATA::Compare(GrabIK_DATA* a1) {
    uint32_t FormFlags = ArmSystemsA.iSize;
    bool bret = FormFlags != a1->ArmSystemsA.iSize;
    if (!bret) {
        for (uint32_t auiIndex = 0; auiIndex < ArmSystemsA.iSize && !bret; ++auiIndex) {
            GrabIK_DATA::GrabIK_Arm_DATA* pOther = &a1->ArmSystemsA.GetAt(static_cast<int32_t>(auiIndex));
            GrabIK_DATA::GrabIK_Arm_DATA* pArm = &ArmSystemsA.GetAt(static_cast<int32_t>(auiIndex));
            bret = *pArm != *pOther;
        }
    }
    uint32_t v4 = ArmSystemsNameA.iSize;
    if (v4 != a1->ArmSystemsNameA.iSize) {
        bret = true;
    }
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < ArmSystemsNameA.iSize && !bret; ++uiIndex) {
            bret = BSstrcmp(
                       ArmSystemsNameA.GetAt(static_cast<int32_t>(uiIndex)),
                       a1->ArmSystemsNameA.GetAt(static_cast<int32_t>(uiIndex))
                   ) != 0;
        }
    }
    if (bret || bEnabled != a1->bEnabled) {
        return 1;
    }
    return bret;
}

void GrabIK_DATA::Copy(const GrabIK_DATA& arData) {
    ArmSystemsA.Clear(true);
    // The names are const, so the const is cast away to free each one.
    for (const char* pName : ArmSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    ArmSystemsNameA.Clear(true);
    uint32_t FormFlags = arData.ArmSystemsA.iSize;
    ArmSystemsA.SetSize(FormFlags, true);
    ArmSystemsNameA.SetSize(arData.ArmSystemsNameA.iSize, true);
    for (uint32_t auiIndex = 0; auiIndex < arData.ArmSystemsA.iSize; ++auiIndex) {
        const GrabIK_DATA::GrabIK_Arm_DATA* pArm = &arData.ArmSystemsA.GetAt(static_cast<int32_t>(auiIndex));
        ArmSystemsA.GetAt(static_cast<int32_t>(auiIndex)) = *pArm;
    }
    for (uint32_t i = 0; i < arData.ArmSystemsNameA.iSize; ++i) {
        uint32_t uiSize = BSstrlen(arData.ArmSystemsNameA.GetAt(static_cast<int32_t>(i))) + 1;
        char* pName = static_cast<char*>(ourMalloc(uiSize));
        BSstrcpy(pName, static_cast<int32_t>(uiSize), arData.ArmSystemsNameA.GetAt(static_cast<int32_t>(i)));
        ArmSystemsNameA.GetAt(static_cast<int32_t>(i)) = pName;
    }
    bEnabled = arData.bEnabled;
}

FootIK_DATA::FootIKSystem_DATA::FootIKSystem_DATA()
  : fOnOffGain(0.0f), fGroundAscendingGain(0.0f), fGroundDescendingGain(0.0f), fFootRaisedGain(0.0f),
    fFootPlantedGain(0.0f), fRaycastDistUp(0.0f), fRaycastDistDown(0.0f) {}

FootIK_DATA::FootIK_Leg_DATA::FootIK_Leg_DATA()
  : sHipIndex(-1), sKneeIndex(-1), sAnkleIndex(-1), bAlignFootToTerrain(false), fMinAnkleHeightMS(0.0f),
    fMaxAnkleHeightMS(0.0f), fFootPlantedAnkleHeightMS(0.0f), fFootRaisedAnkleHeightMS(0.0f), fCosMaxKneeAngle(0.0f),
    fCosMinKneeAngle(0.0f) {}

bool FootIK_DATA::FootIK_Leg_DATA::operator==(const FootIK_DATA::FootIK_Leg_DATA& arData) {
    bool v3 = false;
    if (sHipIndex == arData.sHipIndex && sKneeIndex == arData.sKneeIndex && sAnkleIndex == arData.sAnkleIndex &&
        bAlignFootToTerrain == arData.bAlignFootToTerrain && arData.fMinAnkleHeightMS == fMinAnkleHeightMS &&
        arData.fMaxAnkleHeightMS == fMaxAnkleHeightMS &&
        arData.fFootPlantedAnkleHeightMS == fFootPlantedAnkleHeightMS &&
        arData.fFootRaisedAnkleHeightMS == fFootRaisedAnkleHeightMS && arData.fCosMaxKneeAngle == fCosMaxKneeAngle &&
        arData.fCosMinKneeAngle == fCosMinKneeAngle && arData.fKneeAxis[0u] == fKneeAxis[0u] &&
        arData.fKneeAxis[1] == fKneeAxis[1] && arData.fKneeAxis[2] == fKneeAxis[2] &&
        arData.fFootEndLocLS[0u] == fFootEndLocLS[0u] && arData.fFootEndLocLS[1] == fFootEndLocLS[1]) {
        return arData.fFootEndLocLS[2] == fFootEndLocLS[2];
    }
    return v3;
}

bool FootIK_DATA::FootIK_Leg_DATA::operator!=(const FootIK_DATA::FootIK_Leg_DATA& arData) {
    return !(*this == arData);
}

FootIK_DATA::FootIK_DATA() : bEnabled(false) {}

FootIK_DATA::~FootIK_DATA() {}

char FootIK_DATA::Compare(FootIK_DATA* a1) {
    uint32_t FormFlags = LegSystemsA.iSize;
    bool bret = FormFlags != a1->LegSystemsA.iSize;
    if (!bret) {
        for (uint32_t auiIndex = 0; auiIndex < LegSystemsA.iSize && !bret; ++auiIndex) {
            FootIK_DATA::FootIK_Leg_DATA* pOther = &a1->LegSystemsA.GetAt(static_cast<int32_t>(auiIndex));
            FootIK_DATA::FootIK_Leg_DATA* pLeg = &LegSystemsA.GetAt(static_cast<int32_t>(auiIndex));
            bret = *pLeg != *pOther;
        }
    }
    uint32_t v4 = LegSystemsNameA.iSize;
    if (v4 != a1->LegSystemsNameA.iSize) {
        bret = true;
    }
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < LegSystemsNameA.iSize && !bret; ++uiIndex) {
            bret = BSstrcmp(
                       LegSystemsNameA.GetAt(static_cast<int32_t>(uiIndex)),
                       a1->LegSystemsNameA.GetAt(static_cast<int32_t>(uiIndex))
                   ) != 0;
        }
    }
    if (bret ||
        (bEnabled != a1->bEnabled ||
         (a1->SystemData.fOnOffGain != SystemData.fOnOffGain ||
          (a1->SystemData.fGroundAscendingGain != SystemData.fGroundAscendingGain ||
           (a1->SystemData.fGroundDescendingGain != SystemData.fGroundDescendingGain ||
            (a1->SystemData.fFootRaisedGain != SystemData.fFootRaisedGain ||
             (a1->SystemData.fFootPlantedGain != SystemData.fFootPlantedGain ||
              (a1->SystemData.fRaycastDistUp != SystemData.fRaycastDistUp ||
               a1->SystemData.fRaycastDistDown != SystemData.fRaycastDistDown)))))))) {
        return 1;
    }
    return bret;
}

void FootIK_DATA::Copy(const FootIK_DATA& arData) {
    LegSystemsA.Clear(true);
    // The names are const, so the const is cast away to free each one.
    for (const char* pName : LegSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    LegSystemsNameA.Clear(true);
    uint32_t FormFlags = arData.LegSystemsA.iSize;
    LegSystemsA.SetSize(FormFlags, true);
    LegSystemsNameA.SetSize(arData.LegSystemsNameA.iSize, true);
    for (uint32_t auiIndex = 0; auiIndex < arData.LegSystemsA.iSize; ++auiIndex) {
        const FootIK_DATA::FootIK_Leg_DATA* pLeg = &arData.LegSystemsA.GetAt(static_cast<int32_t>(auiIndex));
        LegSystemsA.GetAt(static_cast<int32_t>(auiIndex)) = *pLeg;
    }
    for (uint32_t i = 0; i < arData.LegSystemsNameA.iSize; ++i) {
        uint32_t uiSize = BSstrlen(arData.LegSystemsNameA.GetAt(static_cast<int32_t>(i))) + 1;
        char* pName = static_cast<char*>(ourMalloc(uiSize));
        BSstrcpy(pName, static_cast<int32_t>(uiSize), arData.LegSystemsNameA.GetAt(static_cast<int32_t>(i)));
        LegSystemsNameA.GetAt(static_cast<int32_t>(i)) = pName;
    }
    SystemData = arData.SystemData;
    bEnabled = arData.bEnabled;
}

BGSRagdoll::BGSRagdoll() {
    SetFormType(RGDL_ID);
    BGSRagdoll::InitializeData();
}

BGSRagdoll::~BGSRagdoll() {
    BGSRagdoll::ClearData();
}

void BGSRagdoll::InitializeData() {
    FeedbackData.SystemData.fKeyDynIntrp = 0.8f;
    FeedbackData.SystemData.fHeirGain = 0.17f;
    FeedbackData.SystemData.fPosGain = 0.05f;
    FeedbackData.SystemData.fVelGain = 0.6f;
    FeedbackData.SystemData.fAccGain = 1.0f;
    FeedbackData.SystemData.fSnapGain = 0.1f;
    FeedbackData.SystemData.fVelDamp = 0.0f;
    FeedbackData.SystemData.fSnapMaxLinVel = 3.0f;
    FeedbackData.SystemData.fSnapMaxAngVel = 0.3f;
    FeedbackData.SystemData.fSnapMaxLinDist = 0.3f;
    FeedbackData.SystemData.fSnapMaxAngDist = 3.0f;
    FeedbackData.SystemData.fPosMaxLinVel = 14.0f;
    FeedbackData.SystemData.fPosMaxAngVel = 18.0f;
    FeedbackData.DynamicBoneIdxA.Clear(true);
    // The names are const, so the const is cast away to free each one.
    for (const char* pName : FootIKData.LegSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    FootIKData.LegSystemsA.Clear(true);
    FootIKData.LegSystemsNameA.Clear(true);
    FootIKData.SystemData.fOnOffGain = 0.0f;
    FootIKData.SystemData.fGroundAscendingGain = 0.0f;
    FootIKData.SystemData.fGroundDescendingGain = 0.0f;
    FootIKData.SystemData.fFootRaisedGain = 0.0f;
    FootIKData.SystemData.fRaycastDistUp = 0.0f;
    FootIKData.SystemData.fRaycastDistDown = 0.0f;
    for (const char* pName : LookIKData.HeadSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    LookIKData.HeadSystemsA.Clear(true);
    LookIKData.HeadSystemsNameA.Clear(true);
    for (const char* pName : GrabIKData.ArmSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    GrabIKData.ArmSystemsA.Clear(true);
    GrabIKData.ArmSystemsNameA.Clear(true);
    PoseMatchingData.SystemData.sRootMatchingBone = -1;
    PoseMatchingData.SystemData.sMatchingBone2 = -1;
    PoseMatchingData.SystemData.sMatchingBone3 = -1;
    FeedbackData.bEnabled = false;
    FootIKData.bEnabled = false;
    LookIKData.bEnabled = false;
    GrabIKData.bEnabled = false;
    PoseMatchingData.bEnabled = false;
    pPreviewActor = nullptr;
    pBodyPartData = nullptr;
}

void BGSRagdoll::ClearData() {
    FeedbackData.DynamicBoneIdxA.Clear(true);
    // The names are const, so the const is cast away to free each one.
    for (const char* pName : FootIKData.LegSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    FootIKData.LegSystemsA.Clear(true);
    FootIKData.LegSystemsNameA.Clear(true);
    for (const char* pName : LookIKData.HeadSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    LookIKData.HeadSystemsA.Clear(true);
    LookIKData.HeadSystemsNameA.Clear(true);
    for (const char* pName : GrabIKData.ArmSystemsNameA) {
        ourFree(const_cast<char*>(pName));
    }
    GrabIKData.ArmSystemsA.Clear(true);
    GrabIKData.ArmSystemsNameA.Clear(true);
}

// Reads the ragdoll's chunks. DATA sizes the bone and IK system arrays and sets which systems are enabled; the counts
// themselves are not kept. When a chunk is empty or short (malformed data), the DATA struct is zero-initialised because
// GetChunkData writes only the chunk's bytes, and the EDID and ANAM buffers are initialised to "" (one zeroed byte,
// allocated only for an empty chunk).
bool BGSRagdoll::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != RGDL_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten, so it is cleared first.
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case DATA_ID: {
                // A chunk shorter than the record leaves the rest unwritten (see above).
                RagdollSaveStruct kSave = {};
                apFile->GetChunkData(&kSave, 0xE);
                if (apFile->QEndian()) {
                    kSave.Endian();
                }
                FeedbackData.DynamicBoneIdxA.SetSize(kSave.usFeedbackDynamicBoneCount, true);
                FootIKData.LegSystemsA.SetSize(kSave.usLegCount, true);
                LookIKData.HeadSystemsA.SetSize(kSave.usHeadCount, true);
                GrabIKData.ArmSystemsA.SetSize(kSave.usArmCount, true);
                FeedbackData.bEnabled = kSave.bFeedbackEnabled;
                FootIKData.bEnabled = kSave.bFootIKEnabled;
                LookIKData.bEnabled = kSave.bLookIKEnabled;
                GrabIKData.bEnabled = kSave.bGrabIKEnabled;
                PoseMatchingData.bEnabled = kSave.bPoseMatchEnabled;
                break;
            }
            case XNAM_ID: {
                uint32_t uiPreviewActor = 0;
                apFile->GetChunkData(uiPreviewActor);
                // Stores the chunk's 32-bit value in the pointer field unchanged.
                pPreviewActor = reinterpret_cast<TESActorBase*>(static_cast<uintptr_t>(uiPreviewActor));
                break;
            }
            case TNAM_ID: {
                uint32_t uiBodyPartData = 0;
                apFile->GetChunkData(uiBodyPartData);
                // Stores the chunk's 32-bit value in the pointer field unchanged.
                pBodyPartData = reinterpret_cast<BGSBodyPartData*>(static_cast<uintptr_t>(uiBodyPartData));
                break;
            }
            case RAFD_ID:
                apFile->GetChunkData(&FeedbackData.SystemData, 0x3C);
                if (apFile->QEndian()) {
                    FeedbackData.SystemData.Endian();
                }
                break;
            case RAFB_ID: {
                uint32_t uiBytes = FeedbackData.DynamicBoneIdxA.GetSize() * 2;
                apFile->GetChunkData(&FeedbackData.DynamicBoneIdxA.GetAt(0), uiBytes);
                uint32_t uiCount = FeedbackData.DynamicBoneIdxA.GetSize();
                if (apFile->QEndian()) {
                    for (uint32_t i = 0; i < uiCount; ++i) {
                        ENDIAN(FeedbackData.DynamicBoneIdxA.GetAt(i), false);
                    }
                }
                break;
            }
            case RAPS_ID:
                apFile->GetChunkData(&PoseMatchingData.SystemData, 0x18);
                if (apFile->QEndian()) {
                    PoseMatchingData.SystemData.Endian();
                }
                break;
            case ANAM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten, so it is cleared first.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                PoseMatchingData.PSAFilename.Set(pBuffer, 0);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

// KEEP: the DATA chunk is a 14-byte RagdollSaveStruct on the stack ([ebp-0x10]); the exe writes its four counts
// and five flag bytes ([ebp-0x10]..[ebp-4]) but never byte +0xD ([ebp-3]), and __AddChunkData copies all 14 bytes
// (push 0xE at 0x5EE1DA), so that byte is uninitialised stack written into the form buffer on every path.
void BGSRagdoll::Save() {
    RagdollSaveStruct saveStruct;
    StartForm();
    TESModel::Save(MODL_ID, MODT_ID);
    TESForm::AddChunk(NVER_ID, 1u);
    saveStruct.usFeedbackDynamicBoneCount = FeedbackData.DynamicBoneIdxA.iSize;
    saveStruct.usLegCount = FootIKData.LegSystemsA.iSize;
    saveStruct.usHeadCount = LookIKData.HeadSystemsA.iSize;
    saveStruct.usArmCount = GrabIKData.ArmSystemsA.iSize;
    saveStruct.bFeedbackEnabled = FeedbackData.bEnabled;
    saveStruct.bFootIKEnabled = FootIKData.bEnabled;
    saveStruct.bLookIKEnabled = LookIKData.bEnabled;
    saveStruct.bGrabIKEnabled = GrabIKData.bEnabled;
    saveStruct.bPoseMatchEnabled = PoseMatchingData.bEnabled;
    if (TESForm::QEndianSwapOnSave()) {
        saveStruct.Endian();
    }
    TESForm::__AddChunkData(DATA_ID, &saveStruct, 14u);
    if (TESForm::QEndianSwapOnSave()) {
        saveStruct.Endian();
    }
    if (pPreviewActor) {
        TESForm::AddChunk(XNAM_ID, pPreviewActor->iFormID);
    }
    if (pBodyPartData) {
        TESForm::AddChunk(TNAM_ID, pBodyPartData->iFormID);
    }
    if (TESForm::QEndianSwapOnSave()) {
        FeedbackData.SystemData.Endian();
    }
    TESForm::__AddChunkData(RAFD_ID, &FeedbackData.SystemData, 60u);
    if (TESForm::QEndianSwapOnSave()) {
        FeedbackData.SystemData.Endian();
    }
    if (FeedbackData.DynamicBoneIdxA.iSize) {
        uint32_t uiCount = FeedbackData.DynamicBoneIdxA.iSize;
        int16_t* pBones = &FeedbackData.DynamicBoneIdxA.GetAt(0);
        TESForm::AddChunkArray(RAFB_ID, pBones, uiCount);
    }
    if (TESForm::QEndianSwapOnSave()) {
        PoseMatchingData.SystemData.Endian();
    }
    TESForm::__AddChunkData(RAPS_ID, &PoseMatchingData.SystemData, 24u);
    if (TESForm::QEndianSwapOnSave()) {
        PoseMatchingData.SystemData.Endian();
    }
    if (PoseMatchingData.PSAFilename.GetLength()) {
        uint32_t Length = PoseMatchingData.PSAFilename.GetLength();
        TESForm::AddChunkArray(ANAM_ID, PoseMatchingData.PSAFilename.pString, Length + 1);
    }
    CloseForm();
}

void BGSRagdoll::InitItem() {
    if (!GetInitialized()) {
        uint32_t uitemp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pBodyPartData));
        if (uitemp) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(uitemp, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(uitemp);
            pBodyPartData = dynamic_cast<BGSBodyPartData*>(FormByNumericID);
            if (!pBodyPartData) {
                int32_t CastingType = iFormID;
                const char* v5 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find body part data (%08X) for ragdoll '%s' (%08X).", uitemp, v5, CastingType
                );
            }
        }
        uitemp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPreviewActor));
        if (uitemp) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(uitemp, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(uitemp);
            pPreviewActor = dynamic_cast<TESActorBase*>(v4);
            if (!pPreviewActor) {
                int32_t v8 = iFormID;
                const char* v6 = GetFormEditorID();
                Warning("MASTERFILE: Could not find preview actor (%08X) for ragdoll '%s' (%08X).", uitemp, v6, v8);
            }
        }
        SetInitialized(true);
    }
}

void BGSRagdoll::Copy(TESForm* apCopy) {
    BGSRagdoll* pcopy = dynamic_cast<BGSRagdoll*>(apCopy);
    if (pcopy) {
        CopyAllComponents(apCopy);
        pPreviewActor = pcopy->pPreviewActor;
        pBodyPartData = pcopy->pBodyPartData;
        iSkeletonID = pcopy->iSkeletonID;
        FeedbackData.Copy(pcopy->FeedbackData);
        FootIKData.Copy(pcopy->FootIKData);
        LookIKData.Copy(pcopy->LookIKData);
        GrabIKData.Copy(pcopy->GrabIKData);
        PoseMatchingData.Copy(pcopy->PoseMatchingData);
    }
}

bool BGSRagdoll::Compare(TESForm* apCompare) {
    BGSRagdoll* pcomp = dynamic_cast<BGSRagdoll*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (iSkeletonID != pcomp->iSkeletonID) {
        return true;
    }
    if (FeedbackData.Compare(&pcomp->FeedbackData)) {
        return true;
    }
    if (FootIKData.Compare(&pcomp->FootIKData)) {
        return true;
    }
    if (LookIKData.Compare(&pcomp->LookIKData)) {
        return true;
    }
    if (GrabIKData.Compare(&pcomp->GrabIKData)) {
        return true;
    }
    if (PoseMatchingData.Compare(&pcomp->PoseMatchingData)) {
        return true;
    }
    return pBodyPartData != pcomp->pBodyPartData;
}
