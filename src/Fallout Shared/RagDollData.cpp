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
    ourFree(pBoneArray);
    iBoneCount = 0;
    pBoneArray = nullptr;
}

// apRef's 3D is fetched twice, as the game does. A failed node update re-creates the bone array, and the result is true
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
    NiMatrix3 result;
    uint32_t aiBoneIndex = 0;
    bool bUpdated = true;
    if ((apRef) && (apRef->TESObjectREFR::Get3D()) && (iBoneCount)) {
        NiNode* pRootNode;
        NiNode* pRoot = apRef->TESObjectREFR::Get3D();
        if (pRoot) {
            pRootNode = pRoot->IsNode();
        }
        else {
            pRootNode = nullptr;
        }
        NiAVObject* a1 = pRootNode;
        if (apRef->IsActor()) {
            UpdateBip01NodeFromRagdollData(apRef);
        }
        bUpdated = UpdateNodeFromRagDollData(static_cast<NiNode*>(a1), aiBoneIndex);
        if (pRoot) {
            NiMatrix3* v2;
            if ((&pRoot->GetTranslate())->operator!=(apRef->GetLocationOnReference()) ||
                (v2 = &(result = apRef->GetOrientation()), (&pRoot->m_kLocal)->m_Rotate.operator!=(*v2))) {
                const NiPoint3* v3 = &apRef->GetLocationOnReference();
                pRoot->SetTranslate(*v3);
                pRoot->SetRotate(apRef->GetOrientation());
                bhkNiCollisionObject::ResetSim(pRoot, true);
                NiUpdateData a2(0.0f, false, false);
                pRoot->Update(a2);
            }
        }
        if (!bUpdated) {
            bhkWorld::SetMotion(a1, hkpMotion::MOTION_DYNAMIC, true, false, true);
        }
    }

    return aiBoneIndex == iBoneCount && bUpdated;
}

// The bone count is the chunk size over the bone size, and the chunk must be a whole multiple of the count. The size is
// divided by the count, so a chunk shorter than one bone faults, as the game does.
bool RagDollData::Load(TESFile* apFile) {
    bool bLoaded = false;
    if (apFile->GetTESChunk() == XRGD_ID) {
        uint32_t uiSize = apFile->GetChunkSize();
        iBoneCount = static_cast<uint8_t>(uiSize / sizeof(RagDollBone));
        if (uiSize % iBoneCount) {
            iBoneCount = 0;
        }
        else {
            uint32_t uiCount = iBoneCount;
            void* pMemory = ourMalloc(uiCount * sizeof(RagDollBone));
            RagDollBone* pBones = nullptr;
            if (pMemory) {
                rt::eh_vector_constructor_iterator(
                    pMemory, sizeof(RagDollBone), static_cast<int32_t>(uiCount), &fnvd::constructElement<REF_DECAL_DATA>
                );
                pBones = static_cast<RagDollBone*>(pMemory);
            }
            pBoneArray = pBones;
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

// The array size is not guarded against overflow; a byte count of bones never overflows, so the plain product is used.
void RagDollData::Copy(RagDollData* apCopy) {
    if (apCopy) {
        ourFree(pBoneArray);
        iBoneCount = apCopy->iBoneCount;
        Bip01Rot = apCopy->Bip01Rot;
        uint32_t uiCount = iBoneCount;
        void* pMemory = ourMalloc(uiCount * sizeof(RagDollBone));
        RagDollBone* pBones = nullptr;
        if (pMemory) {
            rt::eh_vector_constructor_iterator(
                pMemory, sizeof(RagDollBone), static_cast<int32_t>(uiCount), &fnvd::constructElement<REF_DECAL_DATA>
            );
            pBones = static_cast<RagDollBone*>(pMemory);
        }
        pBoneArray = pBones;
        ourMemcpy(pBoneArray, apCopy->pBoneArray, iBoneCount * sizeof(RagDollBone));
    }
}

// The old bone array is freed but its pointer is kept until a new array replaces it; apRef's 3D is fetched twice, as
// the game does. Overflow guard on the array size as in Load.
void RagDollData::CreateRagDollBoneArray(TESObjectREFR* apRef) {
    iBoneCount = 0;
    ourFree(pBoneArray);
    if (apRef && apRef->Get3D()) {
        NiAVObject* p3D = apRef->Get3D();
        NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
        iBoneCount = CountRagDollBones(pNode);
        if (iBoneCount) {
            uint32_t uiCount = iBoneCount;
            void* pMemory = ourMalloc(uiCount * sizeof(RagDollBone));
            RagDollBone* pBones = nullptr;
            if (pMemory) {
                rt::eh_vector_constructor_iterator(
                    pMemory, sizeof(RagDollBone), static_cast<int32_t>(uiCount), &fnvd::constructElement<REF_DECAL_DATA>
                );
                pBones = static_cast<RagDollBone*>(pMemory);
            }
            pBoneArray = pBones;
            uint32_t uiBoneIndex = 0;
            if (!UpdateRagDollDataFromNode(pNode, uiBoneIndex, true)) {
                Warning(sRagDollInitWarning);
            }
        }
    }
}

bool RagDollData::UpdateRagDollDataFromNode(NiNode* apNode, uint32_t& aiBoneIndex, bool abInit) {
    CFilter apResult;
    CFilter a2;
    bool bsuccess = true;
    if (apNode && aiBoneIndex < iBoneCount) {
        bhkNiCollisionObject* pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(apNode);
        if (pcol) {
            if (abInit) {
                CFilter* CollisionFilter = pcol->GetWorldObject()->GetCollisionFilter(&a2);
                pBoneArray[aiBoneIndex].iBoneID = CollisionFilter->GetPart();
            }
            else {
                int32_t iBoneID = pBoneArray[aiBoneIndex].iBoneID;
                CFilter* v6 = pcol->GetWorldObject()->GetCollisionFilter(&apResult);
                if (iBoneID != v6->GetPart()) {
                    return false;
                }
            }
            pBoneArray[aiBoneIndex].BonePos = apNode->GetTranslate();
            (&apNode->m_kLocal)
                ->m_Rotate.ToEulerAnglesXYZ(
                    pBoneArray[aiBoneIndex].BoneRot.x, pBoneArray[aiBoneIndex].BoneRot.y,
                    pBoneArray[aiBoneIndex].BoneRot.z
                );
            ++aiBoneIndex;
        }
        uint32_t icount = apNode->GetArrayCount();
        for (uint32_t i = 0; i < icount; ++i) {
            NiAVObject* pav = apNode->GetAt(i);
            NiNode* pnode = pav ? pav->IsNode() : nullptr;
            NiNode* pnode_2 = pnode;
            if ((pnode) && (!UpdateRagDollDataFromNode(pnode_2, aiBoneIndex, abInit))) {
                bsuccess = false;
            }

            if (aiBoneIndex >= iBoneCount) {
                break;
            }
        }
    }
    return bsuccess;
}

// Overflow guard on the array size as in Load.
void RagDollData::LoadGame() {
    pSaveLoadGameOLD->LoadGameDataOLD(this, 1);
    uint32_t uiCount = iBoneCount;
    void* pMemory = ourMalloc(uiCount * sizeof(RagDollBone));
    RagDollBone* pBones = nullptr;
    if (pMemory) {
        rt::eh_vector_constructor_iterator(
            pMemory, sizeof(RagDollBone), static_cast<int32_t>(uiCount), &fnvd::constructElement<REF_DECAL_DATA>
        );
        pBones = static_cast<RagDollBone*>(pMemory);
    }
    pBoneArray = pBones;
    pSaveLoadGameOLD->LoadGameDataOLD(pBoneArray, iBoneCount * sizeof(RagDollBone));
}

void RagDollData::UpdateBip01NodeFromRagdollData(TESObjectREFR* apRef) {
    NiNode* v2;
    NiNode* v3;
    NiUpdateData apMemory;
    NiMatrix3 ecx0;
    NiAVObject* v7;
    NiNode* v8;
    NiNode* v9;
    if ((&Bip01Rot)->operator!=(NiPoint3_ZERO)) {
        if (apRef && apRef->IsActor()) {
            v3 = apRef->Get3D();
        }
        else {
            v3 = nullptr;
        }
        v9 = v3;
        if (v3) {
            v2 = v9->IsNode();
        }
        else {
            v2 = nullptr;
        }
        v8 = v2;
        if (v2) {
            v7 = v8->GetAtChecked(0);
            if (v7) {
                ecx0;
                (&ecx0)->FromEulerAnglesXYZ(Bip01Rot.x, Bip01Rot.y, Bip01Rot.z);
                v7->SetRotate(ecx0);
                apMemory.~NiUpdateData();
                ::new (&apMemory) NiUpdateData(0.0, false, false);
                v7->UpdateWorldData(apMemory);
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
            for (uint32_t i_2 = 0; i_2 < iBoneCount; ++i_2) {
                pBoneArray[i_2].Endian();
            }
        }
    }
    if (Bip01Rot.operator!=(NiPoint3_ZERO)) {
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
    if (Bip01Rot.operator!=(apCompare->Bip01Rot)) {
        return true;
    }
    return iBoneCount && std::memcmp(pBoneArray, apCompare->pBoneArray, sizeof(RagDollBone) * iBoneCount);
}

bool RagDollData::UpdateNodeFromRagDollData(NiNode* apNode, uint32_t& aiBoneIndex) {
    CFilter a2;
    bool bretval = true;
    if (apNode && aiBoneIndex < iBoneCount) {
        bhkNiCollisionObject* pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(apNode);
        if (pcol) {
            NiMatrix3 rot;
            CFilter* CollisionFilter = pcol->GetWorldObject()->GetCollisionFilter(&a2);
            uint32_t iboneID = CollisionFilter->GetPart();
            if (pBoneArray[aiBoneIndex].iBoneID != iboneID) {
                return false;
            }
            apNode->SetTranslate(pBoneArray[aiBoneIndex].BonePos);
            rot.FromEulerAnglesXYZ(
                pBoneArray[aiBoneIndex].BoneRot.x, pBoneArray[aiBoneIndex].BoneRot.y, pBoneArray[aiBoneIndex].BoneRot.z
            );
            apNode->SetRotate(rot);
            pcol->SetReset(true);
            pcol->SetMotionType(hkpMotion::MOTION_DYNAMIC, nullptr, false);
            ++aiBoneIndex;
        }
        NiUpdateData UpdateData(0.0f, false, false);
        apNode->UpdateWorldData(UpdateData);
        uint32_t icount = apNode->GetArrayCount();
        for (uint32_t i = 0; i < icount; ++i) {
            NiAVObject* pav = apNode->GetAt(i);
            NiNode* pnode = pav ? pav->IsNode() : nullptr;
            NiNode* pnode_2 = pnode;
            if ((pnode) && (!UpdateNodeFromRagDollData(pnode_2, aiBoneIndex))) {
                bretval = false;
            }

            if (aiBoneIndex >= iBoneCount) {
                break;
            }
        }
    }
    return bretval;
}

uint8_t RagDollData::CountRagDollBones(NiNode* apNode) {
    NiNode* v2;
    NiAVObject* v3;
    uint32_t a1;
    uint32_t arrayCount;
    int8_t v6 = 0;
    if (apNode) {
        v6 = (int8_t)(bhkNiCollisionObject::GetbhkNiCollisionObject(apNode) != nullptr);
        arrayCount = apNode->GetArrayCount();
        for (a1 = 0; a1 < arrayCount; ++a1) {
            v3 = apNode->GetAt(a1);
            if (v3) {
                v2 = v3->IsNode();
            }
            else {
                v2 = nullptr;
            }
            if (v2) {
                v6 += ((uint32_t)RagDollData::CountRagDollBones(v2));
            }
        }
    }
    return (uint8_t)v6;
}

void RagDollBone::Endian() {
    ENDIAN(BonePos.x, false);
    ENDIAN(BonePos.y, false);
    ENDIAN(BonePos.z, false);
    ENDIAN(BoneRot.x, false);
    ENDIAN(BoneRot.y, false);
    ENDIAN(BoneRot.z, false);
}
