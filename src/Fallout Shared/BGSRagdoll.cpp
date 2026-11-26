#include "Fallout Shared/BGSRagdoll.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <new>

using FootIKLegArray = BSSimpleArray<FootIK_DATA::FootIK_Leg_DATA>;
using LookIKHeadArray = BSSimpleArray<LookIK_DATA::LookIK_Head_DATA>;
using GrabIKArmArray = BSSimpleArray<GrabIK_DATA::GrabIK_Arm_DATA>;

using BSSimpleArray_GrabIK_DATA_GrabIK_Arm_DATA_1024 = BSSimpleArray<GrabIK_DATA::GrabIK_Arm_DATA, 1024>;

// The enabled flags are single bytes.
void RagdollSaveStruct::Endian() {
    BSEndian(usFeedbackDynamicBoneCount, false);
    BSEndian(usLegCount, false);
    BSEndian(usHeadCount, false);
    BSEndian(usArmCount, false);
}

PoseMatching_DATA::PoseMatching_System_DATA::PoseMatching_System_DATA()
  : sRootMatchingBone(-1), sMatchingBone2(-1), sMatchingBone3(-1), bDisableOnActivate(false), fMotorStrength(0.0f),
    fDelayTime(0.0f), fErrorAllowance(0.1f), fDisableDisplacement(0.0f) {}

// The disable flag is a single byte.
void PoseMatching_DATA::PoseMatching_System_DATA::Endian() {
    BSEndian(sRootMatchingBone, false);
    BSEndian(sMatchingBone2, false);
    BSEndian(sMatchingBone3, false);
    BSEndian(fMotorStrength, false);
    BSEndian(fDelayTime, false);
    BSEndian(fErrorAllowance, false);
    BSEndian(fDisableDisplacement, false);
}

PoseMatching_DATA::PoseMatching_DATA() : bEnabled(false) {}

PoseMatching_DATA::~PoseMatching_DATA() {}

bool PoseMatching_DATA::Compare(PoseMatching_DATA* apOther) {
    bool bret = PSAFilename.StrCmp(apOther->PSAFilename, true) != 0;
    if (bret ||
        bEnabled != apOther->bEnabled ||
        SystemData.sRootMatchingBone != apOther->SystemData.sRootMatchingBone ||
        SystemData.sMatchingBone2 != apOther->SystemData.sMatchingBone2 ||
        SystemData.sMatchingBone3 != apOther->SystemData.sMatchingBone3 ||
        apOther->SystemData.fMotorStrength != SystemData.fMotorStrength ||
        apOther->SystemData.fErrorAllowance != SystemData.fErrorAllowance ||
        SystemData.bDisableOnActivate != apOther->SystemData.bDisableOnActivate ||
        apOther->SystemData.fDisableDisplacement != SystemData.fDisableDisplacement ||
        apOther->SystemData.fDelayTime != SystemData.fDelayTime) {
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
    if (sNeckIdx == arData.sNeckIdx &&
        sHeadIdx == arData.sHeadIdx &&
        arData.fLimitConeAngle == fLimitConeAngle &&
        arData.fLookAtTargetGain == fLookAtTargetGain &&
        arData.fLookAtGain == fLookAtGain &&
        arData.fForwardNeckMS[0u] == fForwardNeckMS[0u] &&
        arData.fForwardNeckMS[1] == fForwardNeckMS[1] &&
        arData.fForwardNeckMS[2] == fForwardNeckMS[2] &&
        arData.fForwardHeadLS[0u] == fForwardHeadLS[0u] &&
        arData.fForwardHeadLS[1] == fForwardHeadLS[1]) {
        return arData.fForwardHeadLS[2] == fForwardHeadLS[2];
    }
    return false;
}

bool LookIK_DATA::LookIK_Head_DATA::operator!=(const LookIK_DATA::LookIK_Head_DATA& arData) const {
    return !(*this == arData);
}

LookIK_DATA::LookIK_DATA() : bEnabled(false) {}

char LookIK_DATA::Compare(LookIK_DATA* apOther) {
    uint32_t uiCount = HeadSystemsA.iSize;
    bool bret = uiCount != apOther->HeadSystemsA.iSize;
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < HeadSystemsA.iSize && !bret; ++uiIndex) {
            bret = HeadSystemsA.GetAt(static_cast<int32_t>(uiIndex)) !=
                apOther->HeadSystemsA.GetAt(static_cast<int32_t>(uiIndex));
        }
    }
    uint32_t uiNameCount = HeadSystemsNameA.iSize;
    if (uiNameCount != apOther->HeadSystemsNameA.iSize) {
        bret = true;
    }
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < HeadSystemsNameA.iSize && !bret; ++uiIndex) {
            bret = BSstrcmp(
                       HeadSystemsNameA.GetAt(static_cast<int32_t>(uiIndex)),
                       apOther->HeadSystemsNameA.GetAt(static_cast<int32_t>(uiIndex))
                   ) != 0;
        }
    }
    if (bret || bEnabled != apOther->bEnabled) {
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
    HeadSystemsA.SetSize(arData.HeadSystemsA.iSize, true);
    HeadSystemsNameA.SetSize(arData.HeadSystemsNameA.iSize, true);
    for (uint32_t uiIndex = 0; uiIndex < arData.HeadSystemsA.iSize; ++uiIndex) {
        // The source's element is read first (C++17 sequences the right operand of = first).
        HeadSystemsA.GetAt(static_cast<int32_t>(uiIndex)) = arData.HeadSystemsA.GetAt(static_cast<int32_t>(uiIndex));
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
    if (sShoulderIndex == arData.sShoulderIndex && sElbowIndex == arData.sElbowIndex) {
        return sHandIndex == arData.sHandIndex;
    }
    return false;
}

bool GrabIK_DATA::GrabIK_Arm_DATA::operator!=(const GrabIK_DATA::GrabIK_Arm_DATA& arData) {
    return !(*this == arData);
}

GrabIK_DATA::GrabIK_DATA() : bEnabled(false) {}

char GrabIK_DATA::Compare(GrabIK_DATA* apOther) {
    uint32_t uiCount = ArmSystemsA.iSize;
    bool bret = uiCount != apOther->ArmSystemsA.iSize;
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < ArmSystemsA.iSize && !bret; ++uiIndex) {
            GrabIK_DATA::GrabIK_Arm_DATA* pOther = &apOther->ArmSystemsA.GetAt(static_cast<int32_t>(uiIndex));
            GrabIK_DATA::GrabIK_Arm_DATA* pArm = &ArmSystemsA.GetAt(static_cast<int32_t>(uiIndex));
            bret = *pArm != *pOther;
        }
    }
    uint32_t uiNameCount = ArmSystemsNameA.iSize;
    if (uiNameCount != apOther->ArmSystemsNameA.iSize) {
        bret = true;
    }
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < ArmSystemsNameA.iSize && !bret; ++uiIndex) {
            bret = BSstrcmp(
                       ArmSystemsNameA.GetAt(static_cast<int32_t>(uiIndex)),
                       apOther->ArmSystemsNameA.GetAt(static_cast<int32_t>(uiIndex))
                   ) != 0;
        }
    }
    if (bret || bEnabled != apOther->bEnabled) {
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
    ArmSystemsA.SetSize(arData.ArmSystemsA.iSize, true);
    ArmSystemsNameA.SetSize(arData.ArmSystemsNameA.iSize, true);
    for (uint32_t uiIndex = 0; uiIndex < arData.ArmSystemsA.iSize; ++uiIndex) {
        const GrabIK_DATA::GrabIK_Arm_DATA* pArm = &arData.ArmSystemsA.GetAt(static_cast<int32_t>(uiIndex));
        ArmSystemsA.GetAt(static_cast<int32_t>(uiIndex)) = *pArm;
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
    if (sHipIndex == arData.sHipIndex &&
        sKneeIndex == arData.sKneeIndex &&
        sAnkleIndex == arData.sAnkleIndex &&
        bAlignFootToTerrain == arData.bAlignFootToTerrain &&
        arData.fMinAnkleHeightMS == fMinAnkleHeightMS &&
        arData.fMaxAnkleHeightMS == fMaxAnkleHeightMS &&
        arData.fFootPlantedAnkleHeightMS == fFootPlantedAnkleHeightMS &&
        arData.fFootRaisedAnkleHeightMS == fFootRaisedAnkleHeightMS &&
        arData.fCosMaxKneeAngle == fCosMaxKneeAngle &&
        arData.fCosMinKneeAngle == fCosMinKneeAngle &&
        arData.fKneeAxis[0u] == fKneeAxis[0u] &&
        arData.fKneeAxis[1] == fKneeAxis[1] &&
        arData.fKneeAxis[2] == fKneeAxis[2] &&
        arData.fFootEndLocLS[0u] == fFootEndLocLS[0u] &&
        arData.fFootEndLocLS[1] == fFootEndLocLS[1]) {
        return arData.fFootEndLocLS[2] == fFootEndLocLS[2];
    }
    return false;
}

bool FootIK_DATA::FootIK_Leg_DATA::operator!=(const FootIK_DATA::FootIK_Leg_DATA& arData) {
    return !(*this == arData);
}

FootIK_DATA::FootIK_DATA() : bEnabled(false) {}

FootIK_DATA::~FootIK_DATA() {}

char FootIK_DATA::Compare(FootIK_DATA* apOther) {
    uint32_t uiCount = LegSystemsA.iSize;
    bool bret = uiCount != apOther->LegSystemsA.iSize;
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < LegSystemsA.iSize && !bret; ++uiIndex) {
            FootIK_DATA::FootIK_Leg_DATA* pOther = &apOther->LegSystemsA.GetAt(static_cast<int32_t>(uiIndex));
            FootIK_DATA::FootIK_Leg_DATA* pLeg = &LegSystemsA.GetAt(static_cast<int32_t>(uiIndex));
            bret = *pLeg != *pOther;
        }
    }
    uint32_t uiNameCount = LegSystemsNameA.iSize;
    if (uiNameCount != apOther->LegSystemsNameA.iSize) {
        bret = true;
    }
    if (!bret) {
        for (uint32_t uiIndex = 0; uiIndex < LegSystemsNameA.iSize && !bret; ++uiIndex) {
            bret = BSstrcmp(
                       LegSystemsNameA.GetAt(static_cast<int32_t>(uiIndex)),
                       apOther->LegSystemsNameA.GetAt(static_cast<int32_t>(uiIndex))
                   ) != 0;
        }
    }
    if (bret ||
        bEnabled != apOther->bEnabled ||
        apOther->SystemData.fOnOffGain != SystemData.fOnOffGain ||
        apOther->SystemData.fGroundAscendingGain != SystemData.fGroundAscendingGain ||
        apOther->SystemData.fGroundDescendingGain != SystemData.fGroundDescendingGain ||
        apOther->SystemData.fFootRaisedGain != SystemData.fFootRaisedGain ||
        apOther->SystemData.fFootPlantedGain != SystemData.fFootPlantedGain ||
        apOther->SystemData.fRaycastDistUp != SystemData.fRaycastDistUp ||
        apOther->SystemData.fRaycastDistDown != SystemData.fRaycastDistDown) {
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
    LegSystemsA.SetSize(arData.LegSystemsA.iSize, true);
    LegSystemsNameA.SetSize(arData.LegSystemsNameA.iSize, true);
    for (uint32_t uiIndex = 0; uiIndex < arData.LegSystemsA.iSize; ++uiIndex) {
        const FootIK_DATA::FootIK_Leg_DATA* pLeg = &arData.LegSystemsA.GetAt(static_cast<int32_t>(uiIndex));
        LegSystemsA.GetAt(static_cast<int32_t>(uiIndex)) = *pLeg;
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
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
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
                        BSEndian(FeedbackData.DynamicBoneIdxA.GetAt(i), false);
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
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

// KEEP: the DATA chunk is the 14-byte local kSaveStruct; Save writes its four counts and five flag bytes but never
// byte 13, and __AddChunkData copies all 14 bytes, so that byte is uninitialised stack written into the form buffer on
// every path.
void BGSRagdoll::Save() {
    RagdollSaveStruct kSaveStruct;
    StartForm();
    TESModel::Save(MODL_ID, MODT_ID);
    TESForm::AddChunk(NVER_ID, 1u);
    kSaveStruct.usFeedbackDynamicBoneCount = FeedbackData.DynamicBoneIdxA.iSize;
    kSaveStruct.usLegCount = FootIKData.LegSystemsA.iSize;
    kSaveStruct.usHeadCount = LookIKData.HeadSystemsA.iSize;
    kSaveStruct.usArmCount = GrabIKData.ArmSystemsA.iSize;
    kSaveStruct.bFeedbackEnabled = FeedbackData.bEnabled;
    kSaveStruct.bFootIKEnabled = FootIKData.bEnabled;
    kSaveStruct.bLookIKEnabled = LookIKData.bEnabled;
    kSaveStruct.bGrabIKEnabled = GrabIKData.bEnabled;
    kSaveStruct.bPoseMatchEnabled = PoseMatchingData.bEnabled;
    if (TESForm::QEndianSwapOnSave()) {
        kSaveStruct.Endian();
    }
    TESForm::__AddChunkData(DATA_ID, &kSaveStruct, 14u);
    if (TESForm::QEndianSwapOnSave()) {
        kSaveStruct.Endian();
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
        uint32_t uiLength = PoseMatchingData.PSAFilename.GetLength();
        TESForm::AddChunkArray(ANAM_ID, PoseMatchingData.PSAFilename.pString, uiLength + 1);
    }
    CloseForm();
}

void BGSRagdoll::InitItem() {
    if (!GetInitialized()) {
        uint32_t uitemp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pBodyPartData));
        if (uitemp) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uitemp, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uitemp);
            pBodyPartData = dynamic_cast<BGSBodyPartData*>(pForm);
            if (!pBodyPartData) {
                int32_t iRagdollID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find body part data (%08X) for ragdoll '%s' (%08X).",
                    uitemp,
                    pEditorID,
                    iRagdollID
                );
            }
        }
        uitemp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPreviewActor));
        if (uitemp) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uitemp, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uitemp);
            pPreviewActor = dynamic_cast<TESActorBase*>(pForm);
            if (!pPreviewActor) {
                int32_t iRagdollID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find preview actor (%08X) for ragdoll '%s' (%08X).",
                    uitemp,
                    pEditorID,
                    iRagdollID
                );
            }
        }
        SetInitialized(true);
    }
}

void BGSRagdoll::Copy(TESForm* apCopy) {
    BGSRagdoll* pCopy = dynamic_cast<BGSRagdoll*>(apCopy);
    if (pCopy) {
        CopyAllComponents(apCopy);
        pPreviewActor = pCopy->pPreviewActor;
        pBodyPartData = pCopy->pBodyPartData;
        iSkeletonID = pCopy->iSkeletonID;
        FeedbackData.Copy(pCopy->FeedbackData);
        FootIKData.Copy(pCopy->FootIKData);
        LookIKData.Copy(pCopy->LookIKData);
        GrabIKData.Copy(pCopy->GrabIKData);
        PoseMatchingData.Copy(pCopy->PoseMatchingData);
    }
}

bool BGSRagdoll::Compare(TESForm* apCompare) {
    BGSRagdoll* pComp = dynamic_cast<BGSRagdoll*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (iSkeletonID != pComp->iSkeletonID) {
        return true;
    }
    if (FeedbackData.Compare(&pComp->FeedbackData)) {
        return true;
    }
    if (FootIKData.Compare(&pComp->FootIKData)) {
        return true;
    }
    if (LookIKData.Compare(&pComp->LookIKData)) {
        return true;
    }
    if (GrabIKData.Compare(&pComp->GrabIKData)) {
        return true;
    }
    if (PoseMatchingData.Compare(&pComp->PoseMatchingData)) {
        return true;
    }
    return pBodyPartData != pComp->pBodyPartData;
}
