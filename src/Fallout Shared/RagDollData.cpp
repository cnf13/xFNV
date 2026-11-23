#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/RagDollBone.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "SDK/MSVC/string.h"

// The warning CreateRagDollBoneArray passes, "HAVOK: Failed to initialize RagDollData.".
using sRagDollInitWarning_t = const char[41];
static sRagDollInitWarning_t sRagDollInitWarning{"HAVOK: Failed to initialize RagDollData."};

RagDollData::~RagDollData() {
    delete[] pBoneArray;
    iBoneCount = 0;
    pBoneArray = nullptr;
}

// apRef's 3D is fetched twice. A failed node update re-creates the bone array, and the result is true
// only when every bone was filled and the update succeeded.
bool RagDollData::UpdateDataFromReference(TESObjectREFR* apRef) {
    uint32_t uiBoneIndex = 0;
    bool bUpdated = true;
    if (apRef && apRef->Get3D()) {
        NiAVObject* p3D = apRef->Get3D();
        NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
        if (apRef->IsActor()) {
            CreateRotation(apRef);
        }
        if (!iBoneCount) {
            CreateRagDollBoneArray(apRef);
        }
        if (iBoneCount) {
            bUpdated = UpdateRagDollDataFromNode(pNode, uiBoneIndex, false);
        }
        if (!bUpdated) {
            CreateRagDollBoneArray(apRef);
        }
    }
    return uiBoneIndex == iBoneCount && bUpdated;
}

bool RagDollData::UpdateReferenceFromData(TESObjectREFR* apRef) {
    NiMatrix3 kOrientation;
    uint32_t uiBoneIndex = 0;
    bool bUpdated = true;
    if (apRef && apRef->TESObjectREFR::Get3D() && iBoneCount) {
        NiNode* pRoot = apRef->TESObjectREFR::Get3D();
        NiNode* pRootNode = pRoot ? pRoot->IsNode() : nullptr;
        NiAVObject* pRootObject = pRootNode;
        if (apRef->IsActor()) {
            UpdateBip01NodeFromRagdollData(apRef);
        }
        bUpdated = UpdateNodeFromRagDollData(static_cast<NiNode*>(pRootObject), uiBoneIndex);
        if (pRoot) {
            if (pRoot->GetTranslate() != apRef->GetLocationOnReference() ||
                (kOrientation = apRef->GetOrientation(), pRoot->m_kLocal.m_Rotate != kOrientation)) {
                pRoot->SetTranslate(apRef->GetLocationOnReference());
                pRoot->SetRotate(apRef->GetOrientation());
                bhkNiCollisionObject::ResetSim(pRoot, true);
                NiUpdateData UpdateData(0.0f, false, false);
                pRoot->Update(UpdateData);
            }
        }
        if (!bUpdated) {
            bhkWorld::SetMotion(pRootObject, hkpMotion::MOTION_DYNAMIC, true, false, true);
        }
    }

    return uiBoneIndex == iBoneCount && bUpdated;
}

// The bone count is the chunk size over the bone size, and the chunk must be a whole multiple of the count. The size is
// divided by the count, so a chunk shorter than one bone faults.
bool RagDollData::Load(TESFile* apFile) {
    bool bLoaded = false;
    if (apFile->GetTESChunk() == XRGD_ID) {
        uint32_t uiSize = apFile->GetChunkSize();
        iBoneCount = static_cast<uint8_t>(uiSize / sizeof(RagDollBone));
        if (uiSize % iBoneCount) {
            iBoneCount = 0;
        }
        else {
            pBoneArray = new RagDollBone[iBoneCount];
            apFile->GetChunkData(pBoneArray, uiSize);
            if (apFile->QEndian()) {
                for (uint32_t i = 0; i < iBoneCount; ++i) {
                    pBoneArray[i].Endian();
                }
            }
            bLoaded = true;
        }
    }
    if (apFile->GetTESChunk() == XRGB_ID) {
        apFile->GetChunkData(&Bip01Rot, sizeof(NiPoint3));
        if (apFile->QEndian()) {
            ENDIAN(Bip01Rot.x, false);
            ENDIAN(Bip01Rot.y, false);
            ENDIAN(Bip01Rot.z, false);
        }
        bLoaded = true;
    }
    return bLoaded;
}

void RagDollData::Copy(RagDollData* apCopy) {
    if (apCopy) {
        delete[] pBoneArray;
        iBoneCount = apCopy->iBoneCount;
        Bip01Rot = apCopy->Bip01Rot;
        pBoneArray = new RagDollBone[iBoneCount];
        ourMemcpy(pBoneArray, apCopy->pBoneArray, iBoneCount * sizeof(RagDollBone));
    }
}

// The old bone array is freed but its pointer is kept until a new array replaces it; apRef's 3D is fetched twice.
void RagDollData::CreateRagDollBoneArray(TESObjectREFR* apRef) {
    iBoneCount = 0;
    delete[] pBoneArray;
    if (apRef && apRef->Get3D()) {
        NiAVObject* p3D = apRef->Get3D();
        NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
        iBoneCount = CountRagDollBones(pNode);
        if (iBoneCount) {
            pBoneArray = new RagDollBone[iBoneCount];
            uint32_t uiBoneIndex = 0;
            if (!UpdateRagDollDataFromNode(pNode, uiBoneIndex, true)) {
                Warning(sRagDollInitWarning);
            }
        }
    }
}

bool RagDollData::UpdateRagDollDataFromNode(NiNode* apNode, uint32_t& aiBoneIndex, bool abInit) {
    CFilter kFilter;
    bool bSuccess = true;
    if (apNode && aiBoneIndex < iBoneCount) {
        bhkNiCollisionObject* pCol = bhkNiCollisionObject::GetbhkNiCollisionObject(apNode);
        if (pCol) {
            if (abInit) {
                CFilter* pFilter = pCol->GetWorldObject()->GetCollisionFilter(&kFilter);
                pBoneArray[aiBoneIndex].iBoneID = pFilter->GetPart();
            }
            else {
                int32_t iBoneID = pBoneArray[aiBoneIndex].iBoneID;
                CFilter* pFilter = pCol->GetWorldObject()->GetCollisionFilter(&kFilter);
                if (iBoneID != pFilter->GetPart()) {
                    return false;
                }
            }
            pBoneArray[aiBoneIndex].BonePos = apNode->GetTranslate();
            (&apNode->m_kLocal)
                ->m_Rotate.ToEulerAnglesXYZ(
                    pBoneArray[aiBoneIndex].BoneRot.x,
                    pBoneArray[aiBoneIndex].BoneRot.y,
                    pBoneArray[aiBoneIndex].BoneRot.z
                );
            ++aiBoneIndex;
        }
        uint32_t uiCount = apNode->GetArrayCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            NiAVObject* pAV = apNode->GetAt(i);
            NiNode* pNode = pAV ? pAV->IsNode() : nullptr;
            if (pNode && !UpdateRagDollDataFromNode(pNode, aiBoneIndex, abInit)) {
                bSuccess = false;
            }

            if (aiBoneIndex >= iBoneCount) {
                break;
            }
        }
    }
    return bSuccess;
}

void RagDollData::LoadGame() {
    pSaveLoadGameOLD->LoadGameDataOLD(&iBoneCount, sizeof(iBoneCount));
    pBoneArray = new RagDollBone[iBoneCount];
    pSaveLoadGameOLD->LoadGameDataOLD(pBoneArray, iBoneCount * sizeof(RagDollBone));
}

void RagDollData::UpdateBip01NodeFromRagdollData(TESObjectREFR* apRef) {
    NiNode* p3D;
    NiMatrix3 kRotate;
    if (Bip01Rot != NiPoint3::ZERO) {
        if (apRef && apRef->IsActor()) {
            p3D = apRef->Get3D();
        }
        else {
            p3D = nullptr;
        }
        NiNode* pRootNode = p3D ? p3D->IsNode() : nullptr;
        if (pRootNode) {
            NiAVObject* pBip01 = pRootNode->GetAtChecked(0);
            if (pBip01) {
                kRotate.FromEulerAnglesXYZ(Bip01Rot.x, Bip01Rot.y, Bip01Rot.z);
                pBip01->SetRotate(kRotate);
                NiUpdateData UpdateData(0.0, false, false);
                pBip01->UpdateWorldData(UpdateData);
            }
        }
    }
}

void RagDollData::Save() {
    if (iBoneCount) {
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t i = 0; i < iBoneCount; ++i) {
                pBoneArray[i].Endian();
            }
        }
        TESForm::__AddChunkData(XRGD_ID, pBoneArray, 28 * iBoneCount);
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t i = 0; i < iBoneCount; ++i) {
                pBoneArray[i].Endian();
            }
        }
    }
    if (Bip01Rot != NiPoint3::ZERO) {
        TESForm::AddChunkArray(XRGB_ID, &Bip01Rot.x, 3u);
    }
}

bool RagDollData::Compare(RagDollData* apCompare) {
    if (!apCompare) {
        return true;
    }
    if (iBoneCount != apCompare->iBoneCount) {
        return true;
    }
    if (Bip01Rot != apCompare->Bip01Rot) {
        return true;
    }
    return iBoneCount && std::memcmp(pBoneArray, apCompare->pBoneArray, sizeof(RagDollBone) * iBoneCount);
}

bool RagDollData::UpdateNodeFromRagDollData(NiNode* apNode, uint32_t& aiBoneIndex) {
    CFilter kFilter;
    bool bRetVal = true;
    if (apNode && aiBoneIndex < iBoneCount) {
        bhkNiCollisionObject* pCol = bhkNiCollisionObject::GetbhkNiCollisionObject(apNode);
        if (pCol) {
            NiMatrix3 kRot;
            CFilter* pFilter = pCol->GetWorldObject()->GetCollisionFilter(&kFilter);
            uint32_t uiBoneID = pFilter->GetPart();
            if (pBoneArray[aiBoneIndex].iBoneID != uiBoneID) {
                return false;
            }
            apNode->SetTranslate(pBoneArray[aiBoneIndex].BonePos);
            kRot.FromEulerAnglesXYZ(
                pBoneArray[aiBoneIndex].BoneRot.x,
                pBoneArray[aiBoneIndex].BoneRot.y,
                pBoneArray[aiBoneIndex].BoneRot.z
            );
            apNode->SetRotate(kRot);
            pCol->SetReset(true);
            pCol->SetMotionType(hkpMotion::MOTION_DYNAMIC, nullptr, false);
            ++aiBoneIndex;
        }
        NiUpdateData UpdateData(0.0f, false, false);
        apNode->UpdateWorldData(UpdateData);
        uint32_t uiCount = apNode->GetArrayCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            NiAVObject* pAV = apNode->GetAt(i);
            NiNode* pNode = pAV ? pAV->IsNode() : nullptr;
            if (pNode && !UpdateNodeFromRagDollData(pNode, aiBoneIndex)) {
                bRetVal = false;
            }

            if (aiBoneIndex >= iBoneCount) {
                break;
            }
        }
    }
    return bRetVal;
}

uint8_t RagDollData::CountRagDollBones(NiNode* apNode) {
    int8_t cBoneCount = 0;
    if (apNode) {
        cBoneCount = bhkNiCollisionObject::GetbhkNiCollisionObject(apNode) != nullptr;
        uint32_t uiCount = apNode->GetArrayCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            NiAVObject* pAV = apNode->GetAt(i);
            NiNode* pNode = pAV ? pAV->IsNode() : nullptr;
            if (pNode) {
                cBoneCount += static_cast<uint32_t>(RagDollData::CountRagDollBones(pNode));
            }
        }
    }
    return static_cast<uint8_t>(cBoneCount);
}

void RagDollBone::Endian() {
    ENDIAN(BonePos.x, false);
    ENDIAN(BonePos.y, false);
    ENDIAN(BonePos.z, false);
    ENDIAN(BoneRot.x, false);
    ENDIAN(BoneRot.y, false);
    ENDIAN(BoneRot.z, false);
}
