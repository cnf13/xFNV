#include "Fallout Shared/Animation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/AnimSequenceBase.h"
#include "Fallout Shared/AnimSequenceMultiple.h"
#include "Fallout Shared/AnimSequenceSingle.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSBipNode.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectANIO.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendAccumTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cstdint>

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> fAnimationMult("fAnimationMult:General", 1.0f);
SettingT<INISettingCollection> fAnimationDefaultBlend("fAnimationDefaultBlend:General", 0.2f);
SettingT<INISettingCollection> fMenuModeAnimBlend("fMenuModeAnimBlend:Interface", 0.0f);
SettingT<INISettingCollection> iAnimationClonePerLoop("iAnimationClonePerLoop:BackgroundLoad", 5);
SettingT<INISettingCollection> bClampWarning("bClampWarning:AnimationWarning", false);
SettingT<INISettingCollection> bPriorityWarning("bPriorityWarning:AnimationWarning", false);

using NiTMapBaseUShortAnimSequenceBase = NiTMapBase<NiTPointerAllocator<size_t>, uint16_t, AnimSequenceBase*>;

AnimIdle::AnimIdle(
    TESIdleForm* apIdleForm,
    ANIM_GROUP_SECTION aeSection,
    PLAY_TYPE_ENUM aeType,
    Actor* apActor,
    bool abForceLoad,
    Animation* apAnimation
)
  : eType(aeType), eSection(aeSection), pIdleForm(apIdleForm), pAnimation(apAnimation), pActor(apActor) {
    for (int32_t n = 0; n < 2; ++n) {
        pAnimObj[n] = pDataHandler->GetAnimObject(apIdleForm, n);
    }
    BSStringT<char> filename;
    filename.SPrintF("%s\\%s", "Meshes", apIdleForm->GetModel());
    if (abForceLoad) {
        spKFModel = pModelLoader->LoadKF(filename.pString);
    }
    else {
        spKFModel = pModelLoader->QueueAnimIdle(filename.pString, this, IO_TASK_PRIORITY_CRITICAL, apActor);
    }
    if (spKFModel) {
        for (int32_t n = 0; n < 2; ++n) {
            if (pAnimObj[n] && apActor) {
                spAddOnObj[n] = static_cast<NiNode*>(
                    BipedAnim::LoadAndAttachAddOn(pAnimObj[n], pAnimObj[n], BO_NONE, apActor, nullptr)
                );
                NiControllerManager* pManager = apActor->GetAnimation()->GetControllerManager();
                RecurseAndAddObjectsToPalette(
                    spAddOnObj[n], static_cast<NiDefaultAVObjectPalette*>(pManager->GetObjectPalette())
                );
                apActor->SetUpdateLighting(true);
            }
        }
        if (!abForceLoad) {
            spKFModel->IncManualRefCount();
        }
        eFlags = ANIM_IDLE_LOADED;
    }
    else {
        eFlags = ANIM_IDLE_QUEUED;
    }
}

AnimIdle::~AnimIdle() {
    FreeAnimIdleData(this);
}

BSAnimGroupSequence* AnimIdle::GetSequence() {
    return pSeq;
}

uint16_t AnimIdle::GetSaveSize() {
    BSAnimGroupSequence* v1;
    int16_t v4 = 13;
    if (pSeq) {
        v1 = pSeq;
        return (int16_t)(v1->GetSaveSize() + 14);
    }
    return v4;
}

AnimIdle::ANIM_IDLE_ENUM AnimIdle::GetFlag() {
    return eFlags;
}

KFModel* AnimIdle::GetKFModel() {
    return spKFModel;
}

void AnimIdle::Loaded(KFModel* apKFModel) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x1101
    );
    spKFModel = apKFModel;
    if (!apKFModel) {
        eFlags = AnimIdle::ANIM_IDLE_QUEUED;
        goto LABEL_35;
    }
    for (int32_t n = 0; n < 2; ++n) {
        if ((pAnimObj[n]) && (pActor)) {
            NiNode* v2;
            TESObjectANIO* v5 = pAnimObj[n];
            if (v5) {
                v2 = static_cast<NiNode*>(BipedAnim::LoadAndAttachAddOn(pAnimObj[n], v5, BO_NONE, pActor, nullptr));
            }
            else {
                v2 =
                    static_cast<NiNode*>(BipedAnim::LoadAndAttachAddOn(pAnimObj[n], nullptr, BO_NONE, pActor, nullptr));
            }
            spAddOnObj[n] = v2;
            Animation* v3 = pActor->GetAnimation();
            NiControllerManager* ControllerManager = v3->GetControllerManager();
            NiDefaultAVObjectPalette* apPalette =
                static_cast<NiDefaultAVObjectPalette*>(ControllerManager->GetObjectPalette());
            RecurseAndAddObjectsToPalette(spAddOnObj[n].m_pObject, apPalette);
            pActor->SetUpdateLighting(true);
        }
    }
    apKFModel->IncManualRefCount();
    eFlags = AnimIdle::ANIM_IDLE_LOADED;
    if (pActor && (eType == AnimIdle::PLAY_TYPE_AUTO || eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE)) {
        Animation* panim;
        panim = pAnimation;
        if (!panim) {
            panim = pActor->GetAnimation();
        }
        if (!panim) {
            if (!this) {
                goto LABEL_35;
            }
        LABEL_18:;
            delete this;
            goto LABEL_35;
        }
        if (panim->SpecialIdlePlay()) {
            if (eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE) {
                pActor->SetAnimAction(ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE, pSeq.m_pObject);
            }
        }
        else {
            panim->SpecialIdleFree(true, false);
        }
    }
    else {
        if (pActor && eType == AnimIdle::PLAY_TYPE_REPLACE) {
            Animation* panim_2 = pAnimation;
            if (!panim_2) {
                panim_2 = pActor->GetAnimation();
            }
            if (panim_2) {
                panim_2->SpecialIdleReplace();
                goto LABEL_35;
            }
            if (!this) {
                goto LABEL_35;
            }
            goto LABEL_18;
        }
    }
LABEL_35:;
}

TESIdleForm* AnimIdle::GetIdleForm() {
    return pIdleForm;
}

void AnimIdle::SetSequence(BSAnimGroupSequence* apSeq) {
    pSeq = apSeq;
}

void AnimIdle::LoadGame(BGSLoadFormBuffer* apBuffer, Animation* apAnimation) {
    KFModel* v3;
    apBuffer->LoadData(static_cast<char*>(static_cast<void*>(&eFlags)), 4u);
    apBuffer->LoadData(static_cast<char*>(static_cast<void*>(&eType)), 4u);
    apBuffer->LoadData(static_cast<char*>(static_cast<void*>(&eSection)), 4u);
    if (eFlags == ANIM_IDLE_QUEUED) {
        eFlags = ANIM_IDLE_LOADED;
    }
    if ((eFlags == ANIM_IDLE_PLAYING) && (spKFModel)) {
        v3 = spKFModel;
        apAnimation->AddAnimation(v3, false);
    }
}

void AnimIdle::SaveGame(float afTime, Animation* apAnimation) {
    int8_t v8;
    bool apData;
    pSaveLoadGameOLD->SaveGameDataOLD(&eFlags, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&eType, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&eSection, 4u);
    apData = pSeq.m_pObject != nullptr;
    pSaveLoadGameOLD->SaveGameDataOLD(&apData, 1u);
    if (apData) {
        AnimSequenceBase* val;
        v8 = -1;
        uint16_t Type = spKFModel.m_pObject->GetAnimGroup()->GetType();
        NiTPointerMap<uint16_t, AnimSequenceBase*>* MapAnimSequenceBase = apAnimation->GetMapAnimSequenceBase();
        if (MapAnimSequenceBase->GetAt(Type, val)) {
            BSAnimGroupSequence* a2 = pSeq.m_pObject;
            v8 = val->GetSequenceIndex(a2);
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&v8, 1u);
        pSeq.m_pObject->SaveGame(afTime);
    }
}

void AnimIdle::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer, Animation* apAnimation) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eFlags), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eType), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eSection), 4u, 0);
}

bool AnimIdle::PlayAnim(BSAnimGroupSequence* apSeq) {
    if (eFlags != ANIM_IDLE_LOADED) {
        return false;
    }
    pSeq = apSeq;
    eFlags = ANIM_IDLE_PLAYING;
    if (eType == PLAY_TYPE_AUTO_EXCLUSIVE) {
        pActor->SetAnimAction(ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE, pSeq);
    }
    return true;
}

void AnimIdle::Update(Animation* apAnimation) {
    if (eFlags != ANIM_IDLE_PLAYING) {
        return;
    }
    if (pSeq) {
        pSeq.operator->();
        if (pSeq->GetState() != NiControllerSequence::kInactive) {
            return;
        }
    }
    eFlags = ANIM_IDLE_DONE_PLAYING;
    if (pActor && pActor->GetAnimAction() == ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE) {
        pActor->SetAnimAction(ANIM_ACTION_NONE, nullptr);
    }
    if ((eType == PLAY_TYPE_AUTO || eType == PLAY_TYPE_AUTO_EXCLUSIVE) && apAnimation) {
        apAnimation->SpecialIdleFree(true, false);
    }
}

void AnimIdle::LoadGameTES(float a1, Animation* a2) {
    char csequenceIndex;
    bool bhasSequence;
    pSaveLoadGameOLD->LoadGameDataOLD(&eFlags, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&eType, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&eSection, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&bhasSequence, 1u);
    if (bhasSequence) {
        pSaveLoadGameOLD->LoadGameDataOLD(&csequenceIndex, 1u);
        if (a2->AddAnimation(spKFModel.m_pObject, false)) {
            if (eFlags == AnimIdle::ANIM_IDLE_PLAYING) {
                AnimSequenceBase* panimSeq;
                uint16_t Type = spKFModel.m_pObject->GetAnimGroup()->GetType();
                NiTPointerMap<uint16_t, AnimSequenceBase*>* MapAnimSequenceBase = a2->GetMapAnimSequenceBase();
                if (MapAnimSequenceBase->GetAt(Type, panimSeq)) {
                    a2->SkipNextBlend();
                    ANIM_GROUP_SECTION afTime = eSection;
                    uint16_t v8 = spKFModel.m_pObject->GetAnimGroup()->GetType();
                    BSAnimGroupSequence* v4 = panimSeq->GetSequenceToPlay(csequenceIndex);
                    pSeq = a2->StartGroup(v4, v8, afTime);
                }
            }
            else {
                if (eFlags) {
                    if (eFlags == AnimIdle::ANIM_IDLE_DONE_PLAYING) {
                        AnimSequenceBase* panimSeq_2 = nullptr;
                        uint16_t v9 = spKFModel.m_pObject->GetAnimGroup()->GetType();
                        NiTPointerMap<uint16_t, AnimSequenceBase*>* v5 = a2->GetMapAnimSequenceBase();
                        if (v5->GetAt(v9, panimSeq_2)) {
                            pSeq = panimSeq_2->GetSequenceToPlay(csequenceIndex);
                        }
                    }
                }
                else {
                    eFlags = AnimIdle::ANIM_IDLE_LOADED;
                }
            }
        }
        if (pSeq.m_pObject) {
            pSeq.m_pObject->LoadGame(a1);
        }
        uint16_t SequenceSaveSize = BSAnimGroupSequence::GetSequenceSaveSize();
        pSaveLoadGameOLD->AdvanceBuffer(SequenceSaveSize);
    }
    else {
        if (eFlags == AnimIdle::ANIM_IDLE_QUEUED) {
            eFlags = AnimIdle::ANIM_IDLE_LOADED;
        }
    }
}

// The debug log goes through an empty printf stub (InitializationFlow), as the game does.
uint16_t AnimIdle::GetAnimIdleSaveSize(Actor* apActor, AnimIdle* apAnimIdle) {
    uint16_t sSize = 0;
    uint16_t sStartSize = sSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sSize += 4;
        sSize += 2;
    }
    sSize += 4;
    if (apAnimIdle && apAnimIdle->GetIdleForm()) {
        sSize += 2;
        sSize += apAnimIdle->GetSaveSize();
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", sSize - sStartSize,
                pHeader->iFormID, pEditorID, pHeader->iFlags, 4595,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", sSize - sStartSize, 4595,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
    }
    return sSize;
}

// With save blocks, writes the KBLK block ID and a 2-byte size placeholder, then patches the placeholder with the
// block's size at the end. The debug log goes through an empty printf stub.
void AnimIdle::SaveAnimIdle(Actor* apActor, AnimIdle* apAnimIdle, Animation* apAnimation, float afTime) {
    uint16_t sBlockSize = 0;
    char* pBlockStart = nullptr;
    char* pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iBlockID = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, 4);
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, 2);
    }
    uint32_t iIdleFormID = 0;
    if (apAnimIdle && apAnimIdle->GetIdleForm()) {
        iIdleFormID = apAnimIdle->GetIdleForm()->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&iIdleFormID, 4);
    if (iIdleFormID) {
        uint16_t sSaveSize = apAnimIdle->GetSaveSize();
        pSaveLoadGameOLD->SaveGameDataOLD(&sSaveSize, 2);
        apAnimIdle->SaveGame(afTime, apAnimation);
    }
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart), pHeader->iFormID, pEditorID, pHeader->iFlags, 4622,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart), 4622,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        char* pBlockDataStart = pBlockStart;
        if (pEnd > pBlockDataStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 4622
            );
        }
        *reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockDataStart);
    }
}

void AnimIdle::FreeAnimIdleData(AnimIdle* apFreeIdle) {
    if (apFreeIdle) {
        for (int32_t n = 0; n < 2; ++n) {
            if (apFreeIdle->spAddOnObj[n].m_pObject) {
                if (apFreeIdle->pActor) {
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        NiNode* v8 = apFreeIdle->spAddOnObj[n].m_pObject;
                        TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                        Instance->QueueSSNRemoveObject(v8, 0);
                    }
                    else {
                        NiNode* v9 = apFreeIdle->spAddOnObj[n].m_pObject;
                        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
                        ShadowSceneNode_->RemoveObject(v9);
                    }
                    Animation* v3 = apFreeIdle->pActor->GetAnimation();
                    NiControllerManager* ControllerManager = v3->GetControllerManager();
                    NiDefaultAVObjectPalette* apPalette =
                        static_cast<NiDefaultAVObjectPalette*>(ControllerManager->GetObjectPalette());
                    RecurseAndRemoveObjectsFromPalette(apFreeIdle->spAddOnObj[n].m_pObject, apPalette);
                    Animation* pa = apFreeIdle->pActor->GetAnimation();
                    if (pa) {
                        pa->RemoveObject(apFreeIdle->spAddOnObj[n].m_pObject);
                    }
                }
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    NiNode* v10 = apFreeIdle->spAddOnObj[n].m_pObject;
                    TaskQueueInterface* v5 = TaskQueueInterface::GetInstance();
                    v5->QueueNodeDetach(v10);
                }
                else {
                    NiNode* Parent = apFreeIdle->spAddOnObj[n].m_pObject->m_pkParent;
                    Parent->DetachChild(apFreeIdle->spAddOnObj[n].m_pObject);
                }
                const char* v6 = apFreeIdle->pAnimObj[n]->GetModel();
                pModelLoader->ReleaseFile(v6);
                apFreeIdle->spAddOnObj[n] = static_cast<NiNode*>(nullptr);
                if (apFreeIdle->pSeq.m_pObject) {
                    apFreeIdle->pSeq.m_pObject->ValidateTargets();
                }
            }
        }
        if (apFreeIdle->eFlags == AnimIdle::ANIM_IDLE_QUEUED) {
            pModelLoader->CancelAnimIdle(apFreeIdle);
        }
        if ((!pSaveLoadGameOLD->GetLoading()) && (apFreeIdle->pActor) &&
            (apFreeIdle->pActor->GetAnimAction() == ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE)) {
            BaseProcess* CurrentAIProcess = apFreeIdle->pActor->pCurrentProcess;
            BSAnimGroupSequence* v7 = CurrentAIProcess->GetAnimActionAnimSeq();
            if (v7 == apFreeIdle->pSeq.m_pObject) {
                apFreeIdle->pActor->SetAnimAction(ANIM_ACTION_NONE, nullptr);
            }
        }

        apFreeIdle->pActor = nullptr;
    }
}

AnimIdle* AnimIdle::LoadAnimIdle(Actor* apActor, Animation* apAnimation, float afTime) {
    uint16_t isize;
    uint32_t iblockHeader;
    uint16_t iblockSize;
    uint32_t iformID;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x1218
    );
    AnimIdle* panimIdle = nullptr;
    iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x121C, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x121C,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    bool bforcedToZero = pSaveLoadGameOLD->LoadNumericID(&iformID, 4u);
    if (iformID || bforcedToZero) {
        pSaveLoadGameOLD->LoadGameDataOLD(&isize, 2u);
        TESIdleForm* pidle = nullptr;
        if (iformID) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
            pidle = dynamic_cast<TESIdleForm*>(FormByNumericID);
        }
        if (pidle) {
            AnimIdle* panimIdle_2;
            panimIdle_2 = new AnimIdle(pidle, AGS_SPECIAL_IDLE, AnimIdle::PLAY_TYPE_QUEUE, apActor, true, nullptr);
            panimIdle = panimIdle_2;
            panimIdle_2->LoadGameTES(afTime, apAnimation);
        }
        else {
            const char* v4 = apActor->GetFormEditorID();
            Warning("FORMS: Could not find IdleForm %08X when loading actor %s", iformID, v4);
            pSaveLoadGameOLD->AdvanceBuffer(isize);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t a1a = pheader_2->cVersion;
                    const char* v7 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x1237,
                        pheader_2->iFormID, v7, a1a, iFlags
                    );
                }
            }
            else {
                uint32_t v10 = pheader_2->iFlags;
                int32_t a1 = pheader_2->cVersion;
                const char* v6 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x1237, pheader_2->iFormID,
                    v6, a1, v10
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x1237, CurrentVersion
                    );
                }
            }
            else {
                int32_t v12 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x1237, v12
                );
            }
        }
    }
    return panimIdle;
}

Animation::Animation()
  : pActorRef(nullptr), pAccumRoot(nullptr), movementDelta(NiPoint3_ZERO), AccumRootTranslate(NiPoint3_ZERO),
    pSoundPriorityBone{}, m_fLooking(0.0f), cSkipUpdate(-1), bShutDown(false), time(0.0f),
    pAnimSequenceMap(new NiTPointerMap<uint16_t, AnimSequenceBase*>(101)), pCurrentSequence{},
    pLastMovementSequence(nullptr), m_fMoveSpeed(1.0f), m_fAttackSpeed(1.0f), m_fGlobalTimeMultiplier(1.0f),
    m_fReloadModifier(1.0f), m_fEquipModifier(1.0f), cSkipNextBlend(0), sQueuedReloadGroup(255) {
    SetField(0, 255, 0);
    // Fills the per-section tables with 0xFF bytes: no group, no action, loop counts of -1.
    for (int32_t i = 0; i < 8; ++i) {
        group[i] = 0xFFFF;
        nextGroup[i] = 0xFFFF;
        action[i] = static_cast<ANIM_GROUP_ACTION>(-1);
        loopCount[i] = -1;
        nextLoops[i] = -1;
    }
}

Animation::~Animation() {
    ShutdownAllAnimIdles();
    pModelLoader->CancelReplacementKFList(this);
    if (spManager) {
        NiNode* pRoot = pAnimRoot;
        NiTimeController* pMulti = pRoot ? pRoot->GetController(&NiMultiTargetTransformController::ms_RTTI) : nullptr;
        NiTimeController* pMultiTread =
            pRoot ? pRoot->GetController(&BSMultiTargetTreadTransfController::ms_RTTI) : nullptr;
        if (pMulti) {
            pRoot->RemoveController(pMulti);
        }
        if (pMultiTread) {
            pRoot->RemoveController(pMultiTread);
        }
        spManager->DeactivateAll(0.0f);
        spManager->RemoveAllSequences();
        NiNode* pTarget = static_cast<NiNode*>(spManager->m_pkTarget);
        if (pTarget && pTarget->IsNode()) {
            pTarget->RemoveController(spManager);
        }
        spManager = nullptr;
    }
    NiTMapItem<uint16_t, AnimSequenceBase*>* pPos = pAnimSequenceMap->GetFirstPos();
    while (pPos) {
        uint16_t usAnimGroup;
        AnimSequenceBase* pAnimSeq = nullptr;
        pAnimSequenceMap->GetNext(pPos, usAnimGroup, pAnimSeq);
        delete pAnimSeq;
    }
    for (BSSimpleList<KFModel*>* pNode = &kfModelList; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        pNode->m_item->DecUseCount();
    }
    kfModelList.RemoveAll();
    pAnimSequenceMap->RemoveAll();
    delete pAnimSequenceMap;
    pAnimSequenceMap = nullptr;
    pAccumRoot = nullptr;
    if (!replayDelayList.IsEmpty()) {
        for (IDLE_REPLAY_DELAY*& rpItem : replayDelayList) {
            ourFree(rpItem);
        }
        replayDelayList.RemoveAll();
    }
}

void Animation::QueueReplacementAnimation(KFModel* apKFModel) {
    NiPointer<TESAnimGroup> spanimGroup(apKFModel->GetAnimGroup());
    if (spanimGroup.m_pObject) {
        apKFModel->IncManualRefCount();
        apKFModel->IncManualRefCount();
        kfModelList.AddTail(apKFModel);
    }
}

void Animation::ClearBlendInterps() {
    NiControllerManager* v1;
    NiControllerManager* v2;
    KFModel* v4;
    BSAnimGroupSequence* v5;
    NiControllerSequence* pkObject;
    uint32_t uiIndex;
    uint32_t sequenceCount;
    if ((&spManager)->m_pObject) {
        v1 = (&spManager)->m_pObject;
        sequenceCount = v1->GetSequenceCount();
        for (uiIndex = 0; uiIndex < sequenceCount; ++uiIndex) {
            v2 = (&spManager)->m_pObject;
            pkObject = v2->GetSequenceAt(uiIndex);
            v5 =
                static_cast<BSAnimGroupSequence*>(NiObject::DynamicCastStatic(&BSAnimGroupSequence::ms_RTTI, pkObject));
            if (v5) {
                v5->ClearBlendInterp();
            }
        }
        while (GetAnimsLoading()) {
            v4 = (&kfModelList)->m_item;
            v4->DecUseCount();
            v4->DecUseCount();
            (&kfModelList)->RemoveHead();
        }
    }
}

// Restores the animation from a save: adds the KF models still queued, puts the actor's weapon and sit/sleep idle back,
// rebases the playing sequences, refreshes the scene graph with Havok velocities off, then reads the movement state,
// the idle replay delays, the special idle, the sequences of each section (a section whose saved sequence is gone still
// has its data read into a throwaway sequence), the anim action and the idle's sequence, and updates the bip nodes.
void Animation::LoadGameBGS(BGSLoadFormBuffer* apBuffer) {
    if (!pAnimRoot) {
        return;
    }
    Actor* pActor = apBuffer->GetActor();
    while (!kfModelList.IsEmpty()) {
        KFModel* pKFModel = kfModelList.GetItem();
        AddAnimation(pKFModel, false);
        pKFModel->DecUseCount();
        kfModelList.RemoveHead();
    }
    pActor->GetCurrentAIProcess()->LoadSitSleepIdle(pActor, this);
    if (pActor->IsDead(false) || pActor->IsDying()) {
        ForceSection(AGS_WHOLE_BODY, 0xE0, AGA_NONE, 0.0f, -1);
    }
    bool bWeaponDrawn = pActor->GetCurrentAIProcess()->GetWeaponDrawn();
    SkipNextBlend();
    if (pActor == pPlayer) {
        if (this == pPlayer->GetAnimation(true)) {
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            pProcess->ForceWeaponDrawnSheathed(bWeaponDrawn, pPlayer->GetBiped(true), this, pPlayer);
        }
        else {
            BaseProcess* pProcess = pPlayer->GetCurrentAIProcess();
            pProcess->ForceWeaponDrawnSheathed(bWeaponDrawn, pPlayer->GetBiped(false), this, pPlayer);
        }
    }
    else {
        BaseProcess* pProcess = pActor->GetCurrentAIProcess();
        pProcess->ForceWeaponDrawnSheathed(bWeaponDrawn, pActor->GetBiped(), this, pActor);
    }
    pActor->GetCurrentAIProcess()->UpdateReparentWeapon(pActor);
    pActor->GetCurrentAIProcess()->ClearPostAnimationActions();
    pActor->GetCurrentAIProcess()->SetAnimAction(ANIM_ACTION_NONE, nullptr);
    for (BSAnimGroupSequence*& pSequence : pCurrentSequence) {
        if (pSequence) {
            float fOffset = pSequence->GetOffset();
            float fNewOffset = pSequence->GetEndKeyTime() + fOffset;
            pSequence->SetOffset(fNewOffset);
        }
    }
    UpdateBipOnly(time, &AccumRootTranslate, true);
    static_cast<BSBipNode*>(pAnimRoot.operator->())->UpdateAllButBip(time);
    bhkNiCollisionObject* pCollisionObject = bhkUtilFunctions::FindNextCollisionObject(pAnimRoot);
    bool bUseVelocity = pCollisionObject ? pCollisionObject->GetUseVelocity() : false;
    if (bUseVelocity) {
        bhkUtilFunctions::SetUseVelocityRecurse(false, pAnimRoot);
    }
    UpdateSceneGraphNoController();
    if (bUseVelocity) {
        bhkUtilFunctions::SetUseVelocityRecurse(bUseVelocity, pAnimRoot);
    }
    ClearGroup(AGS_WHOLE_BODY, 0.0f);
    ZeroGlobalTransform();
    if (spManager) {
        spManager->DeactivateAll(0.0f);
    }
    // LoadData copies raw bytes into each field, passed by address as an untyped buffer.
    apBuffer->LoadData(reinterpret_cast<char*>(&m_fMoveSpeed), 4);
    apBuffer->LoadData(reinterpret_cast<char*>(&m_fAttackSpeed), 4);
    apBuffer->LoadData(reinterpret_cast<char*>(&movementDelta), 0xC);
    apBuffer->LoadData(reinterpret_cast<char*>(&m_fLooking), 4);
    apBuffer->LoadData(&cSkipUpdate, 1);
    apBuffer->LoadData(reinterpret_cast<char*>(&fLipTime), 4);
    apBuffer->LoadData(reinterpret_cast<char*>(&sQueuedReloadGroup), 2);
    uint32_t uiDelayCount = apBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiDelayCount; ++i) {
        IDLE_REPLAY_DELAY* pDelay = static_cast<IDLE_REPLAY_DELAY*>(ourMalloc(sizeof(IDLE_REPLAY_DELAY)));
        uint32_t uiFormID = apBuffer->LoadFormID();
        pDelay->pIdleForm = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(uiFormID));
        apBuffer->LoadData(reinterpret_cast<char*>(&pDelay->fDelay), 4);
        replayDelayList.AddHead(pDelay);
    }
    // The special idle: its form ID, then its data, skipped when the form is gone.
    uint32_t uiIdleID = 0;
    if (apBuffer->LoadFormID(uiIdleID)) {
        apBuffer->AdvanceBuffer(apBuffer->LoadVariableSizedValue());
    }
    else if (uiIdleID) {
        TESIdleForm* pIdleForm = uiIdleID ? dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(uiIdleID)) : nullptr;
        if (!pIdleForm) {
            apBuffer->AdvanceBuffer(apBuffer->LoadVariableSizedValue());
        }
        else {
            apBuffer->LoadVariableSizedValue();
            AnimIdle* pAnimIdle =
                new AnimIdle(pIdleForm, AGS_SPECIAL_IDLE, AnimIdle::PLAY_TYPE_QUEUE, pActor, true, nullptr);
            spAnimIdle = pAnimIdle;
            spAnimIdle->LoadGame(apBuffer, this);
        }
    }
    int8_t cSequenceMask = 0;
    apBuffer->LoadData(reinterpret_cast<char*>(&cSequenceMask), 1);
    for (uint32_t i = 0; i < ANIM_GROUP_SECTION_COUNT; ++i) {
        if (cSequenceMask & (1 << i)) {
            apBuffer->LoadData(reinterpret_cast<char*>(&group[i]), 2);
            apBuffer->LoadData(reinterpret_cast<char*>(&action[i]), 4);
            apBuffer->LoadData(reinterpret_cast<char*>(&nextGroup[i]), 2);
            apBuffer->LoadData(reinterpret_cast<char*>(&loopCount[i]), 4);
            apBuffer->LoadData(reinterpret_cast<char*>(&nextLoops[i]), 4);
            pCurrentSequence[i] = nullptr;
            char cSequenceIndex = -1;
            apBuffer->LoadData(&cSequenceIndex, 1);
            // -2: no sequence was playing in the section.
            if (cSequenceIndex != -2) {
                SkipNextBlend();
                ForceSection(static_cast<ANIM_GROUP_SECTION>(i), group[i], action[i], time, cSequenceIndex);
                if (pCurrentSequence[i]) {
                    pCurrentSequence[i]->LoadGame(apBuffer, time);
                }
                else {
                    alignas(BSAnimGroupSequence) uint8_t aSequenceStorage[sizeof(BSAnimGroupSequence)];
                    BSAnimGroupSequence* pSequence = ::new (aSequenceStorage) BSAnimGroupSequence;
                    pSequence->LoadGame(apBuffer, time);
                    ClearGroup(static_cast<ANIM_GROUP_SECTION>(i), 0.0f);
                    pSequence->~BSAnimGroupSequence();
                }
            }
        }
    }
    apBuffer->LoadData(&cSkipNextBlend, 1);
    int16_t sAnimAction = -1;
    char cActionSequence = -1;
    apBuffer->LoadData(reinterpret_cast<char*>(&sAnimAction), 2);
    apBuffer->LoadData(&cActionSequence, 1);
    if (cActionSequence != -1 && pCurrentSequence[cActionSequence]) {
        pActor->GetCurrentAIProcess()->SetAnimAction(
            static_cast<ANIMATION_ACTION>(sAnimAction), pCurrentSequence[cActionSequence]
        );
    }
    char cIdleSequence = -1;
    apBuffer->LoadData(&cIdleSequence, 1);
    if (spAnimIdle) {
        if (cIdleSequence != -1) {
            spAnimIdle->SetSequence(pCurrentSequence[cIdleSequence]);
        }
        if (spAnimIdle->GetFlag() == AnimIdle::ANIM_IDLE_LOADED) {
            SkipNextBlend();
            SpecialIdlePlay();
        }
    }
    UpdateBipOnly(time, &AccumRootTranslate, true);
    static_cast<BSBipNode*>(pAnimRoot.operator->())->UpdateAllButBip(time);
}

uint16_t Animation::PickBestAnimation(uint16_t asAnimGroup, bool abRecursive) {
    ANIM_GROUP_WEAPON_ENUM Weapon_0;
    AnimSequenceBase* panimSeq;
    uint16_t sreturnGroup;
    uint16_t stestGroup;
    if (pAnimSequenceMap->GetAt(asAnimGroup, panimSeq) && panimSeq->GetSequenceToPlay(-1)) {
        return asAnimGroup;
    }
    if ((asAnimGroup & 0x8000) != 0) {
        sreturnGroup = PickBestAnimation(asAnimGroup & 0x7FFF, true);
        if (static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(asAnimGroup) ||
            (TESAnimGroup::IsIronSightsAction(asAnimGroup) &&
             static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(asAnimGroup - 3))) {
            return sreturnGroup;
        }
    }
    if (pActorRef == pPlayer && this == pPlayer->GetAnimation(true) && (asAnimGroup & 0x7000) != 0) {
        ANIM_GROUP_SECTION eSectionLayer = animGroupInfo[static_cast<uint8_t>(asAnimGroup)].eSectionLayer;
        if (eSectionLayer >= AGS_WEAPON && eSectionLayer <= AGS_WEAPON_DOWN) {
            sreturnGroup = PickBestAnimation(asAnimGroup & 4095, true);
            if (static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(asAnimGroup) ||
                (TESAnimGroup::IsIronSightsAction(asAnimGroup) &&
                 static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(asAnimGroup - 3))) {
                return sreturnGroup;
            }
        }
    }
    if (TESAnimGroup::IsIronSightsAction(asAnimGroup)) {
        return PickBestAnimation(asAnimGroup - 3, true);
    }
    ANIM_GROUP_SECTION v5 = animGroupInfo[TESAnimGroup::GetType(asAnimGroup)].eSectionLayer;
    if (v5 >= AGS_WEAPON_UP && v5 <= AGS_WEAPON_DOWN) {
        return 0;
    }
    if ((asAnimGroup & 3840) == 0) {
        goto LABEL_61;
    }
    Weapon_0 = TESAnimGroup::GetWeapon(asAnimGroup);
    if (Weapon_0 != AGW_ONE_HAND_MELEE) {
        if (Weapon_0 == AGW_TWO_HAND_MELEE) {
            if (pAnimSequenceMap->GetAt((asAnimGroup & 0xF0FF) | 512, panimSeq) && panimSeq->GetSequenceToPlay(-1)) {
                return (asAnimGroup & 0xF0FF) | 512;
            }
        }
        else {
            if (Weapon_0 != AGW_ONE_HAND_PISTOL &&
                (pAnimSequenceMap->GetAt((asAnimGroup & 0xF0FF) | 1024, panimSeq) && panimSeq->GetSequenceToPlay(-1))) {
                return (asAnimGroup & 0xF0FF) | 1024;
            }
        }
    }
    if (pAnimSequenceMap->GetAt(asAnimGroup & 0xF0FF, panimSeq) && panimSeq->GetSequenceToPlay(-1)) {
        return asAnimGroup & 0xF0FF;
    }
LABEL_61:;
    if ((asAnimGroup & 0x8000) != 0) {
        sreturnGroup = PickBestAnimation(asAnimGroup & 0x7FFF, true);
        if (static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(asAnimGroup)) {
            return sreturnGroup;
        }
    }
    if (!asAnimGroup) {
        goto LABEL_51;
    }
    stestGroup = 255;
    switch (TESAnimGroup::GetType(asAnimGroup)) {
        case ANIM_GROUP_FAST_FORWARD: {
            stestGroup = (asAnimGroup & 0x7F00) | 3;
            break;
        }
        case ANIM_GROUP_FAST_BACK: {
            stestGroup = (asAnimGroup & 0x7F00) | 4;
            break;
        }
        case ANIM_GROUP_FAST_LEFT: {
            stestGroup = (asAnimGroup & 0x7F00) | 5;
            break;
        }
        case ANIM_GROUP_FAST_RIGHT: {
            stestGroup = (asAnimGroup & 0x7F00) | 6;
            break;
        }
        default: {
            break;
        }
    }
    if (stestGroup != 255) {
        sreturnGroup = PickBestAnimation(stestGroup, true);
        if (static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(stestGroup)) {
            return sreturnGroup;
        }
    }
LABEL_51:;
    if (abRecursive) {
        return 0;
    }
    if ((asAnimGroup & 0x7000) != 0) {
        sreturnGroup = PickBestAnimation(asAnimGroup & 4095, true);
        if (static_cast<uint8_t>(sreturnGroup) == static_cast<uint8_t>(asAnimGroup)) {
            return sreturnGroup;
        }
    }
    if (!static_cast<uint8_t>(asAnimGroup)) {
        return 0;
    }
    stestGroup = asAnimGroup & 0x7F00;
    return PickBestAnimation(asAnimGroup & 0x7F00, false);
}

bool Animation::SpecialIdlePlaying() {
    if (!spAnimIdle) {
        return false;
    }
    AnimIdle* v2 = spAnimIdle;
    return (bool)((uint8_t)(((int32_t)v2->AnimIdle::GetFlag()) == 2u));
}

NiTPointerMap<uint16_t, AnimSequenceBase*>* Animation::GetMapAnimSequenceBase() {
    return pAnimSequenceMap;
}

TESAnimGroup* Animation::GetTESAnimGroup(uint16_t asGroup) {
    AnimSequenceBase* val;
    if (!pAnimSequenceMap->GetAt(asGroup, val)) {
        return nullptr;
    }
    BSAnimGroupSequence* v2 = val->GetSequenceToPlay(-1);
    return v2->GetAnimGroup();
}

int16_t Animation::GetGroupSpeed(uint16_t asAnimGroup) {
    AnimSequenceBase* v5;
    bool vCondition1;
    vCondition1 = !pAnimSequenceMap->GetAt(asAnimGroup, v5);
    if (vCondition1) {
        return 0;
    }
    if (!v5->IsSingleSeq()) {
        return 0;
    }
    if (!v5->GetSequenceToPlay((int8_t)(-1))) {
        return 0;
    }
    BSAnimGroupSequence* v3 = v5->GetSequenceToPlay((int8_t)(-1));
    TESAnimGroup* animGroup = v3->GetAnimGroup();
    return (int32_t)animGroup->GetSpeed();
}

TESIdleForm* Animation::GetSpecialIdleForm() {
    if (!spAnimIdle) {
        return nullptr;
    }
    AnimIdle* v1 = spAnimIdle;
    return v1->GetIdleForm();
}

bool Animation::SpecialIdlePlay() {
    if (spAnimIdleQueued.m_pObject) {
        if (spAnimIdleQueued.m_pObject->eFlags != AnimIdle::ANIM_IDLE_LOADED) {
            return false;
        }
        SpecialIdleFree(false, true);
    }
    if (!spAnimIdle.m_pObject || spAnimIdle.m_pObject->eFlags != AnimIdle::ANIM_IDLE_LOADED) {
        return false;
    }
    KFModel* pkfModel = spAnimIdle.m_pObject->GetKFModel();
    if (!pkfModel || (!AddAnimation(pkfModel, false) || !pkfModel->GetAnimGroup())) {
        spAnimIdle.m_pObject->eFlags = AnimIdle::ANIM_IDLE_DONE_PLAYING;
        if (spAnimIdle.m_pObject->eType == AnimIdle::PLAY_TYPE_AUTO ||
            spAnimIdle.m_pObject->eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE) {
            SpecialIdleFree(true, false);
        }
        return false;
    }
    BSAnimGroupSequence* pseq = nullptr;
    if (spAnimIdle.m_pObject->eSection ||
        spAnimIdle.m_pObject->GetKFModel()->GetAnimGroup()->GetType() != ANIM_GROUP_DYNAMIC_IDLE) {
        // Reads the section, then the idle's loop count, then the group: locals keep that order.
        ANIM_GROUP_SECTION esection = spAnimIdle.m_pObject->eSection;
        uint8_t cloops = spAnimIdle.m_pObject->pIdleForm->GetLoops();
        pseq = PlayGroup(pkfModel->GetAnimGroup()->GetType(), ACTION_START, cloops, esection);
    }
    else {
        ANIM_GROUP_SECTION esection = spAnimIdle.m_pObject->eSection;
        pseq = PlayGroup(pkfModel->GetAnimGroup()->GetType(), ACTION_START, -1, esection);
    }
    if (!pseq) {
        spAnimIdle.m_pObject->eFlags = AnimIdle::ANIM_IDLE_DONE_PLAYING;
        if (spAnimIdle.m_pObject->eType == AnimIdle::PLAY_TYPE_AUTO ||
            spAnimIdle.m_pObject->eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE) {
            SpecialIdleFree(true, false);
        }
        return false;
    }
    if (static_cast<double>(spAnimIdle.m_pObject->pIdleForm->GetReplayDelay()) > 0.0) {
        IDLE_REPLAY_DELAY* preplayDelay = static_cast<IDLE_REPLAY_DELAY*>(ourMalloc(sizeof(IDLE_REPLAY_DELAY)));
        preplayDelay->pIdleForm = spAnimIdle.m_pObject->GetIdleForm();
        preplayDelay->fDelay = static_cast<float>(spAnimIdle.m_pObject->pIdleForm->GetReplayDelay());
        replayDelayList.AddTail(preplayDelay);
    }
    spAnimIdle.m_pObject->PlayAnim(pseq);
    return true;
}

// Queues an automatic idle and plays it.
void Animation::SpecialIdleAuto(
    TESIdleForm* apIdleForm,
    Actor* apActor,
    ANIM_GROUP_SECTION aeSection,
    AnimIdle::PLAY_TYPE_ENUM aeType
) {
    // Raw stack storage, so no C++ constructor or destructor runs automatically; the storage's address is used, not the
    // constructor's result.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 4765
    );
    AnimIdle::PLAY_TYPE_ENUM iType = aeType;
    if (aeSection >= AGS_IDLE && (aeSection <= AGS_MOVEMENT || aeSection == AGS_WHOLE_BODY)) {
        iType = AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE;
    }
    AnimIdle* pIdle = new AnimIdle(apIdleForm, aeSection, iType, apActor, false, this);
    spAnimIdleQueued = pIdle;
    BaseProcess* pProcess = apActor->GetCurrentAIProcess();
    pProcess->SetLastIdlePlayed(apIdleForm);
    SpecialIdlePlay();
    pContext->~AutoMemContext();
}

bool Animation::IsIdleNotLoaded(TESIdleForm* apIdle) {
    AnimIdle* v10;
    if (spAnimIdle) {
        AnimIdle* v2 = spAnimIdle;
        if (v2->GetIdleForm() == apIdle) {
            AnimIdle* v3 = spAnimIdle;
            if (v3->GetFlag() != AnimIdle::ANIM_IDLE_LOADED) {
                return true;
            }
        }
    }
    if (spAnimIdleQueued) {
        AnimIdle* v5 = spAnimIdleQueued;
        if (v5->GetIdleForm() == apIdle) {
            AnimIdle* v6 = spAnimIdleQueued;
            if (v6->GetFlag() != AnimIdle::ANIM_IDLE_LOADED) {
                return true;
            }
        }
    }
    if (*spAnimIdleFreeWhenInactiveA) {
        AnimIdle* v7 = *spAnimIdleFreeWhenInactiveA;
        if (v7->GetIdleForm() == apIdle) {
            return true;
        }
        if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
            AnimIdle* v8 = *spAnimIdleFreeWhenInactiveA;
            if (v8->GetSequence()) {
                return true;
            }
        }
    }
    if (!spAnimIdleFreeWhenInactiveA[1]) {
        return false;
    }
    AnimIdle* v9 = spAnimIdleFreeWhenInactiveA[1];
    if (v9->GetIdleForm() == apIdle) {
        return true;
    }
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && (v10 = spAnimIdleFreeWhenInactiveA[1], v10->GetSequence())) {
        return true;
    }

    return false;
}

char Animation::FindSwitchKey(float* pTime) {
    const char* pkeyText;
    uint32_t i;
    uint32_t inumKeys;
    const char* psearchKey = "a:";
    if (!pCurrentSequence[4]) {
        return 0;
    }
    NiTextKeyExtraData* ptext = pCurrentSequence[4]->GetTextKeys();
    NiTextKey* pkeys = ptext->GetKeys(inumKeys);
    uint32_t isearchKeyLength = BSstrlen(psearchKey);
    if (!isearchKeyLength) {
        return 0;
    }
    for (i = 0;; ++i) {
        if (i >= inumKeys) {
            return 0;
        }
        NiFixedString* ActorPackageThatIsRunning = &pkeys[i].m_kText;
        pkeyText = ActorPackageThatIsRunning->GetHandleString();
        if ((pkeyText) && (!_strnicmp(pkeyText, psearchKey, isearchKeyLength))) {
            break;
        }
    }
    if (pTime) {
        *pTime = pkeys[i].m_fTime;
    }
    return std::tolower(pkeyText[isearchKeyLength]);
}

bool Animation::SpecialIdleDonePlaying() {
    if (spAnimIdleQueued) {
        return false;
    }
    if (!spAnimIdle) {
        return true;
    }
    AnimIdle* v2 = spAnimIdle;
    if (!v2->AnimIdle::GetSequence()) {
        return false;
    }
    AnimIdle* v3 = spAnimIdle;
    return (bool)((uint8_t)(((int32_t)v3->AnimIdle::GetFlag()) == 3));
}

// Only a multiple sequence can pick the sequence that matches apObject, and only when it is not a single one;
// anything else returns null.
BSAnimGroupSequence* Animation::SyncSequences(BSAnimGroupSequence* apSeq, uint16_t asGroup) {
    uint16_t usGroup = asGroup;
    AnimSequenceBase* pSequence;
    if (usGroup == 0xFF || !pAnimSequenceMap->GetAt(usGroup, pSequence)) {
        return nullptr;
    }
    AnimSequenceMultiple* pMultiSequence = nullptr;
    if (!pSequence->IsSingleSeq()) {
        pMultiSequence = static_cast<AnimSequenceMultiple*>(pSequence);
    }
    if (!pMultiSequence) {
        return nullptr;
    }
    BSAnimGroupSequence* pCorresponding = pMultiSequence->GetCorrespondingSequence(apSeq, AGS_NONE);
    if (!pCorresponding) {
        return nullptr;
    }
    return StartGroup(pCorresponding, usGroup, AGS_NONE);
}

void Animation::ZeroGlobalTransform() {
    NiBlendAccumTransformInterpolator* v2;
    NiInterpolator* pkObject;
    NiSingleInterpController* v4;
    NiTimeController* i;
    if (pAccumRoot) {
        ClearAccumRootTranslate();
        pAccumRoot->SetRotate(NiMatrix3_IDENTITY);
        pAccumRoot->SetTranslate(NiPoint3_ZERO);
        for (i = pAccumRoot->NiObjectNET::GetControllers(); i; i = i->GetNext()) {
            v4 = static_cast<NiSingleInterpController*>(
                NiObject::DynamicCastStatic(&NiSingleInterpController::ms_RTTI, i)
            );
            if (v4) {
                pkObject = v4->GetInterpolator((uint16_t)0);
                if (pkObject) {
                    v2 = static_cast<NiBlendAccumTransformInterpolator*>(
                        NiObject::DynamicCastStatic(&NiBlendAccumTransformInterpolator::ms_RTTI, pkObject)
                    );
                    if (v2) {
                        v2->ClearAccumulatedTransform();
                    }
                }
            }
        }
    }
}

void Animation::SetField(uint8_t uVal, uint8_t uMask, uint8_t uPos) {
    this->m_uFlags = static_cast<uint8_t>((this->m_uFlags & ~uMask) | (uVal << uPos));
}

void Animation::SetBit(bool bVal, uint8_t uMask) {
    if (bVal) {
        this->m_uFlags |= uMask;
    }
    else {
        this->m_uFlags &= ~uMask;
    }
}

bool Animation::GetBit(uint8_t uMask) {
    return (this->m_uFlags & uMask) != 0;
}

bool Animation::GetAnimsLoading() {
    return !this->kfModelList.IsEmpty();
}

uint32_t Animation::GetAnimsLoadingCount() {
    uint32_t iCount = 0;
    if (!kfModelList.IsEmpty()) {
        for (BSSimpleList<KFModel*>* pIter = &kfModelList; pIter; pIter = pIter->GetNext()) {
            ++iCount;
        }
    }
    return iCount;
}

NiControllerManager* Animation::GetControllerManager() {
    return this->spManager;
}

void Animation::ReloadTargets(bool abReload) {
    if (spManager.m_pObject) {
        NiAVObjectPalette* ppalette = spManager.m_pObject->GetObjectPalette();
        if (ppalette) {
            NiDefaultAVObjectPalette* pdefaultPalette = static_cast<NiDefaultAVObjectPalette*>(
                NiObject::DynamicCastStatic(&NiDefaultAVObjectPalette::ms_RTTI, ppalette)
            );
            if (pdefaultPalette) {
                pdefaultPalette->ResetAndFillFromScenegraph();
            }
        }
        NiTimeController* pmulti = pAnimRoot.m_pObject->GetController(&NiMultiTargetTransformController::ms_RTTI);
        NiTimeController* pmultiTread =
            pAnimRoot.m_pObject->GetController(&BSMultiTargetTreadTransfController::ms_RTTI);
        if (pmulti) {
            pAnimRoot.m_pObject->RemoveController(pmulti);
        }
        if (pmultiTread) {
            pAnimRoot.m_pObject->RemoveController(pmultiTread);
        }
        spManager.m_pObject->DeactivateAll(0.0f);
        uint32_t itransControllerCount = 0;
        for (uint32_t n = 0; n < spManager.m_pObject->GetSequenceCount(); ++n) {
            BSAnimGroupSequence* pseq = static_cast<BSAnimGroupSequence*>(spManager.m_pObject->GetSequenceAt(n));
            if ((pseq) && (pseq->m_uiArraySize > itransControllerCount)) {
                itransControllerCount = pseq->m_uiArraySize;
            }
        }
        for (uint32_t n_2 = 0; n_2 < spManager.m_pObject->GetSequenceCount(); ++n_2) {
            BSAnimGroupSequence* pseq_2 = static_cast<BSAnimGroupSequence*>(spManager.m_pObject->GetSequenceAt(n_2));
            if (pseq_2) {
                if (abReload) {
                    if ((!pAnimRoot.m_pObject->GetController(&NiMultiTargetTransformController::ms_RTTI)) &&
                        (itransControllerCount)) {
                        pseq_2->ResolveTransformInterpolators(pAnimRoot.m_pObject, itransControllerCount);
                    }

                    const char* pName = pseq_2->GetName().GetHandleString();
                    if (pName[0] != '_' || pName[1] != '_') {
                        pseq_2->StoreTargets(pAnimRoot.m_pObject);
                    }
                }
                else {
                    pseq_2->ValidateTargetsFromPalette();
                    pseq_2->ResolveTransformInterpolators(pAnimRoot.m_pObject, itransControllerCount);
                }
            }
        }
        for (BSAnimGroupSequence* pSequence : pCurrentSequence) {
            if (pSequence) {
                spManager.m_pObject->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
            }
        }
    }
}

BSAnimGroupSequence* Animation::GetCurrentSequence(ANIM_GROUP_SECTION aeSection) {
    int32_t eSection = aeSection;
    switch (aeSection) {
        case AGS_WHOLE_BODY: eSection = AGS_MOVEMENT; break;
        case AGS_UPPER_BODY: eSection = AGS_WEAPON; break;
    }
    return this->pCurrentSequence[eSection];
}

bool Animation::GetDelta(NiPoint3* delta, TESObjectREFR* apRef, bool applyRot, bool zeroZ) {
    TESAnimGroup* pAnimGroup;
    TESAnimGroup* pWeaponAnimGroup;
    double v11;
    double v12;
    float scale;
    float v15;
    float v16;
    float v17;
    NiMatrix3 kLocalRot;
    if (!(&pAnimRoot)->m_pObject) {
        return false;
    }
    float fTimeDelta = (&appTimer)->GetSeconds() * m_fGlobalTimeMultiplier;
    NiPoint3 kZero = NiPoint3_ZERO;
    NiPoint3 kVector = movementDelta;
    NiNode* pRootNode = (&pAnimRoot)->m_pObject;
    __builtin_memcpy(&kLocalRot, &pRootNode->GetLocalTransform(), sizeof(kLocalRot));
    if (((!apRef->IsCreature()) && static_cast<Actor*>(apRef)->IsMoving()) && (pCurrentSequence[1])) {
        pAnimGroup = pCurrentSequence[1]->GetAnimGroup();
        if (((int32_t)pAnimGroup->GetType()) >= 3) {
            pAnimGroup = pCurrentSequence[1]->GetAnimGroup();
            if (((int32_t)pAnimGroup->GetType()) <= 16) {
                pAnimGroup = pCurrentSequence[1]->GetAnimGroup();
                v12 = (double)((pAnimGroup->GetSpeed() * m_fMoveSpeed) * fTimeDelta);
                v16 = (float)(pActorRef->GetScale() * v12);
                if ((pCurrentSequence[4]) && (((int32_t)pCurrentSequence[4]->NiControllerSequence::GetState()) == 1)) {
                    pWeaponAnimGroup = pCurrentSequence[4]->GetAnimGroup();
                    v11 = (double)((pWeaponAnimGroup->GetSpeed() * m_fAttackSpeed) * fTimeDelta);
                    v15 = (float)(pActorRef->GetScale() * v11);
                    if (v16 < ((double)v15)) {
                        v16 = v15;
                    }
                }

                v17 = (float)(v16 * 4.0);
                if (v16 > 0.0) {
                    if (v17 < ((double)kVector.x)) {
                        kVector.x = v16;
                    }
                    if (v17 < ((double)kVector.y)) {
                        kVector.y = v16;
                    }
                    if (v17 < ((double)kVector.z)) {
                        kVector.z = v16;
                    }
                    if (kVector.x < (-v17)) {
                        kVector.x = (-v16);
                    }
                    if (kVector.y < (-v17)) {
                        kVector.y = (-v16);
                    }
                    if (kVector.z < (-v17)) {
                        kVector.z = (-v16);
                    }
                }
            }
        }
    }

    if (apRef) {
        switch ((int32_t)apRef->GetSitSleepState()) {
            case 3:
            case 5:
            case 8:
            case 10: {
                scale = apRef->GetScale();
                kVector.x = (kVector.x / scale);
                kVector.y = (kVector.y / scale);
                break;
            }
            default: {
                break;
            }
        }
    }
    if (zeroZ) {
        kVector.z = (float)0.0;
    }
    if (applyRot) {
        NiMatrix3::TransformVertices(kLocalRot, kZero, 1u, &kVector, delta);
    }
    else {
        *delta = kVector;
    }
    return true;
}

ANIM_GROUP_ACTION Animation::GetGroupAction(ANIM_GROUP_SECTION aeSection) {
    int32_t iSection = aeSection;
    if (iSection == 20) {
        iSection = 1;
    }
    else {
        if (iSection == 21) {
            iSection = 4;
        }
    }
    return action[iSection];
}

void Animation::ClearGroup(ANIM_GROUP_SECTION aeSection, float afEaseOutTime) {
    int32_t eSection = aeSection;
    switch (aeSection) {
        case AGS_WHOLE_BODY:
            if (Interface::GetMenuModeType() != Interface::eMainFour) {
                ClearGroup(AGS_SPECIAL_IDLE, afEaseOutTime);
                if (!cSkipNextBlend) {
                    ClearGroup(AGS_IDLE, afEaseOutTime);
                }
            }
            ClearGroup(AGS_MOVEMENT, afEaseOutTime);
            [[fallthrough]];
        case AGS_UPPER_BODY:
            if (Interface::GetMenuModeType() != Interface::eMainFour) {
                ClearGroup(AGS_LEFT_ARM, afEaseOutTime);
            }
            ClearGroup(AGS_TALKING, afEaseOutTime);
            [[fallthrough]];
        case AGS_WEAPON:
            ClearGroup(AGS_WEAPON_UP, afEaseOutTime);
            ClearGroup(AGS_WEAPON_DOWN, afEaseOutTime);
            eSection = AGS_WEAPON;
            break;
    }
    if (spManager && eSection < ANIM_GROUP_SECTION_COUNT && pCurrentSequence[eSection] &&
        pCurrentSequence[eSection]->GetState()) {
        if (pCurrentSequence[eSection]->GetPartnerSequence()) {
            NiControllerSequence* pPartner = pCurrentSequence[eSection]->GetPartnerSequence();
            spManager->DeactivateSequence(pPartner, afEaseOutTime);
        }
        spManager->DeactivateSequence(pCurrentSequence[eSection], afEaseOutTime);
        if (eSection == AGS_MOVEMENT) {
            pLastMovementSequence = pCurrentSequence[eSection];
        }
    }
    pCurrentSequence[eSection] = nullptr;
    group[eSection] = 0xFF;
    nextGroup[eSection] = 0xFF;
    action[eSection] = AGA_NONE;
}

float Animation::GetTime() {
    return time;
}

float Animation::GetTime(NiControllerSequence* apSequence) {
    // The sum is computed in double precision and then stored to a float, as the game does.
    return static_cast<float>(static_cast<double>(apSequence->GetOffset()) + time);
}

bool Animation::SpecialIdleLoaded() {
    if (!spAnimIdle) {
        return false;
    }
    return spAnimIdle->GetFlag() == 1;
}

bool Animation::SpecialIdleWorking(BSAnimGroupSequence* apSeq) {
    if (spAnimIdle && spAnimIdle->AnimIdle::GetSequence() == apSeq) {
        return true;
    }
    if (spAnimIdleQueued && spAnimIdleQueued->AnimIdle::GetSequence() == apSeq) {
        return true;
    }
    if (spAnimIdleFreeWhenInactiveA[0] && spAnimIdleFreeWhenInactiveA[0]->AnimIdle::GetSequence() == apSeq) {
        return true;
    }
    if (spAnimIdleFreeWhenInactiveA[1] && spAnimIdleFreeWhenInactiveA[1]->AnimIdle::GetSequence() == apSeq) {
        return true;
    }
    return false;
}

bool Animation::SpecialIdleWorking() {
    if (spAnimIdle) {
        if (!spAnimIdle->AnimIdle::GetSequence()) {
            // The idle's KF model is still loading unless the idle is done playing.
            return spAnimIdle->GetFlag() != 3;
        }
        NiControllerSequence::State eState = spAnimIdle->AnimIdle::GetSequence()->GetState();
        if (eState == NiControllerSequence::kEaseIn || eState == NiControllerSequence::kTransDest) {
            return true;
        }
    }
    if (!spAnimIdleQueued) {
        return false;
    }
    if (!spAnimIdleQueued->AnimIdle::GetSequence()) {
        return true;
    }
    NiControllerSequence::State eState = spAnimIdleQueued->AnimIdle::GetSequence()->GetState();
    return eState == NiControllerSequence::kEaseIn || eState == NiControllerSequence::kTransDest;
}

void Animation::SetLipTimeMember(float afPercent) {
    float v2;
    double endKeyTime;
    float v4;
    float v6;
    if (afPercent == 0.0) {
        if (pCurrentSequence[3]) {
            pCurrentSequence[3]->Update(0.0, true);
            ClearGroup(AGS_TALKING, 0.0);
        }
    }
    else {
        if (!pCurrentSequence[3]) {
            PlayGroup(0xE1u, ACTION_START, (-1), AGS_NONE);
            if (pCurrentSequence[3]) {
                v4 = pCurrentSequence[3]->GetBeginKeyTime() + time;
                pCurrentSequence[3]->Update(v4, true);
            }
        }
        if ((pCurrentSequence[3]) && (((int32_t)pCurrentSequence[3]->NiControllerSequence::GetState()) == 1)) {
            endKeyTime = (double)pCurrentSequence[3]->GetEndKeyTime();
            v6 = (float)(endKeyTime - pCurrentSequence[3]->GetBeginKeyTime());
            fLipTime = (pCurrentSequence[3]->GetBeginKeyTime() + (v6 * afPercent));
            v2 = fLipTime - time;
            pCurrentSequence[3]->SetPhase(v2, true);
        }
    }
}

// The whole-body and upper-body sections queue in the movement and weapon slots. StartGroup gets the caller's
// aeSection, not the resolved one; a group type of 0xFF (no group) returns null at once.
BSAnimGroupSequence*
Animation::PlayGroup(uint16_t asNewGroup, ACTION_FLAGS aeFlags, int32_t aiLoops, ANIM_GROUP_SECTION aeSection) {
    int32_t iType = TESAnimGroup::GetType(asNewGroup);
    int32_t eSection;
    if (aeSection == AGS_NONE) {
        eSection = animGroupInfo[iType].eSectionLayer;
    }
    else {
        eSection = aeSection;
    }
    int32_t eLayer = eSection;
    BSAnimGroupSequence* pStarted = nullptr;
    switch (eSection) {
        case AGS_WHOLE_BODY: eLayer = AGS_MOVEMENT; break;
        case AGS_UPPER_BODY: eLayer = AGS_WEAPON; break;
    }
    if (iType == 0xFF) {
        return pStarted;
    }
    switch (animGroupInfo[iType].eAction) {
        case AGAT_START_STOP:
        case AGAT_LOOP:
        case AGAT_SPECIAL_IDLE_LOOP:
            switch (aeFlags) {
                case ACTION_NORMAL:
                    nextGroup[eLayer] = asNewGroup;
                    nextLoops[eLayer] = aiLoops;
                    break;
                case ACTION_START:
                    nextGroup[eLayer] = 0xFF;
                    loopCount[eLayer] = aiLoops;
                    pStarted = StartGroup(asNewGroup, aeSection);
                    UpdateBipOnly(time, nullptr, true);
                    break;
            }
            break;
        case AGAT_EQUIP:
        case AGAT_UNEQUIP:
        case AGAT_ATTACK:
        case AGAT_ATTACK_POWER:
        case AGAT_ATTACK_THROW:
        case AGAT_ATTACK_MINE:
        case AGAT_ATTACK_SPIN:
        case AGAT_LOOPING_RELOAD:
            pStarted = StartGroup(asNewGroup, aeSection);
            UpdateBipOnly(time, nullptr, true);
            break;
        default: break;
    }
    return pStarted;
}

// AGS_NONE takes the section layer from the group's table entry. The weapon sections take the group's own multiple
// sequence, every other section and a single sequence play GetSequenceToPlay(-1). The started sequence gets the
// caller's aeSection, not the resolved layer.
BSAnimGroupSequence* Animation::StartGroup(uint16_t asNewGroup, ANIM_GROUP_SECTION aeSection) {
    AnimSequenceBase* pSequence;
    if (asNewGroup == 0xFF || !pAnimSequenceMap->GetAt(asNewGroup, pSequence)) {
        return nullptr;
    }
    int32_t eSection = aeSection;
    if (aeSection == AGS_NONE) {
        eSection = animGroupInfo[TESAnimGroup::GetType(asNewGroup)].eSectionLayer;
    }
    BSAnimGroupSequence* pStarted;
    if (eSection < AGS_WEAPON_UP || eSection > AGS_WEAPON_DOWN || pSequence->IsSingleSeq()) {
        pStarted = pSequence->GetSequenceToPlay(-1);
    }
    else {
        pStarted = static_cast<AnimSequenceMultiple*>(pSequence)->GetCorrespondingSequence(
            pCurrentSequence[AGS_WEAPON], static_cast<ANIM_GROUP_SECTION>(eSection)
        );
    }
    return StartGroup(pStarted, asNewGroup, aeSection);
}

BSAnimGroupSequence*
Animation::StartGroup(BSAnimGroupSequence* apSeq, uint16_t asNewGroup, ANIM_GROUP_SECTION aeSection) {
    NiNode* v5;
    NiNode* v7;
    if (SpecialIdleWorking(apSeq)) {
        return nullptr;
    }
    ANIM_GROUP_ENUM egroup = TESAnimGroup::GetType(asNewGroup);
    ANIM_GROUP_SECTION esection = aeSection;
    PlayerCharacter* pactor = static_cast<PlayerCharacter*>(pActorRef);
    if (aeSection == AGS_NONE) {
        esection = animGroupInfo[egroup].eSectionLayer;
    }
    ANIM_GROUP_SECTION eclearSection = esection;
    if (esection == AGS_WHOLE_BODY) {
        esection = AGS_MOVEMENT;
    }
    else {
        if (esection == AGS_UPPER_BODY) {
            esection = AGS_WEAPON;
        }
    }
    BSAnimGroupSequence* plastSequence = pCurrentSequence[esection];
    ANIM_GROUP_ENUM elastGroup = TESAnimGroup::GetType(group[esection]);
    bool bmorph = false;
    NiControllerSequence::State elastState = NiControllerSequence::kInactive;
    if (plastSequence) {
        elastState = plastSequence->GetState();
    }
    if (!apSeq || asNewGroup == 255) {
        return nullptr;
    }
    if (esection == AGS_WEAPON || (esection == AGS_WEAPON_UP || esection == AGS_WEAPON_DOWN)) {
        MiddleHighProcess* pmid;
        bool bclear;
        if (pActorRef == pPlayer && this == pPlayer->GetAnimation(true)) {
            if (esection == AGS_WEAPON) {
                apSeq->SetSequenceWeight(1.0f);
                goto LABEL_33;
            }
            goto LABEL_32;
        }
        pmid = static_cast<MiddleHighProcess*>(static_cast<MobileObject*>(pActorRef)->GetCurrentAIProcess());
        bclear = true;
        if ((TESAnimGroup::IsAimAction(egroup) || TESAnimGroup::IsAttackAction(egroup)) && (pmid) &&
            (pmid->eLevel <= PROCESS_TYPE_MIDDLE_HIGH)) {
            BaseProcess* pbase = (BaseProcess*)pmid;
            BSAnimGroupSequence* poldSeq = pmid->GetAimSequence(static_cast<AIM_ENUM>(esection - AGS_WEAPON));
            if (poldSeq) {
                float blendAmount = poldSeq->m_fSeqWeight;
                apSeq->SetSequenceWeight(blendAmount);
                bclear = false;
            }
        }

        if (bclear && (esection == AGS_WEAPON_UP || esection == AGS_WEAPON_DOWN)) {
        LABEL_32:;
            apSeq->SetSequenceWeight(0.0f);
        }
    }
LABEL_33:;
    if (apSeq->GetState() == NiControllerSequence::kEaseOut) {
        apSeq->Deactivate(0.0f, false);
    }
    if (((apSeq->GetState() && apSeq->m_eCycleType == NiControllerSequence::CYCLE_LOOP) ||
         (apSeq->GetState() == NiControllerSequence::kAnimating && apSeq == plastSequence)) &&
        (!Interface::IsInMenuMode() || apSeq->GetState() == NiControllerSequence::kAnimating)) {
        action[esection] = AGA_START;
        apSeq->ResetSequence();
        return apSeq;
    }
    if (!Interface::IsInMenuMode() ||
        ((v5 = pAnimRoot.m_pObject, v5 != pPlayer->Get3D(false)) || (pSaveLoadGameOLD->GetLoading() || pVATS.eMode))) {
        NiNode* v6;
        if (Interface::IsInPipboyMenu() &&
            ((v6 = pAnimRoot.m_pObject, v6 == pPlayer->Get3D(true)) &&
             (egroup == ANIM_GROUP_DYNAMIC_IDLE && (aeSection == AGS_IDLE && plastSequence)))) {
            ClearGroup(eclearSection, 0.0f);
            plastSequence = nullptr;
            cSkipNextBlend = 1;
        }
        else {
            if (plastSequence && esection == AGS_LEFT_ARM) {
                ANIM_GROUP_ENUM eoldGroup = plastSequence->GetAnimGroup()->GetType();
                if (egroup == ANIM_GROUP_PIPBOY ||
                    (egroup == ANIM_GROUP_PIPBOY_CHILD &&
                     (eoldGroup >= ANIM_GROUP_HAND_GRIP_1 && eoldGroup <= ANIM_GROUP_HAND_GRIP_6))) {
                    ClearGroup(eclearSection, 0.0f);
                    plastSequence = nullptr;
                }
            }
            else {
                if (elastState && plastSequence) {
                    if (elastState == NiControllerSequence::kAnimating) {
                        if (eclearSection != esection || (cSkipNextBlend || apSeq->GetState())) {
                            ClearGroup(eclearSection, 0.0f);
                        }
                        else {
                            if (eclearSection == AGS_WEAPON &&
                                (egroup <= ANIM_GROUP_UNEQUIP || egroup > ANIM_GROUP_JAM_M) &&
                                !TESAnimGroup::IsAimAction(egroup)) {
                                ClearGroup(AGS_WEAPON_UP, 0.0f);
                                ClearGroup(AGS_WEAPON_DOWN, 0.0f);
                            }
                        }
                    }
                    else {
                        if (pActorRef != pPlayer && esection == AGS_MOVEMENT) {
                            return nullptr;
                        }
                        ClearGroup(eclearSection, 0.0f);
                        plastSequence = nullptr;
                    }
                }
            }
        }
    }
    group[esection] = asNewGroup;
    pCurrentSequence[esection] = apSeq;
    if ((!Interface::IsInMenuMode() || (v7 = pAnimRoot.m_pObject, v7 != pPlayer->Get3D(false))) && (plastSequence)) {
        NiPointer<TESAnimGroup> spcurrentAnimGroup(apSeq->GetAnimGroup());
        NiPointer<TESAnimGroup> splastAnimGroup(plastSequence->GetAnimGroup());
        if ((splastAnimGroup.m_pObject->cMorphKey) &&
            (splastAnimGroup.m_pObject->cMorphKey == spcurrentAnimGroup.m_pObject->cMorphKey)) {
            int32_t CastingType = plastSequence->m_uiArraySize;
            if (CastingType == apSeq->m_uiArraySize) {
                if (apSeq->CanSyncTo(plastSequence)) {
                    bmorph = true;
                }
                else {
                    const char* blendAmounta = (&pAnimRoot.m_pObject->m_kName)->GetHandleString();
                    const char* a6 = (&apSeq->m_kName)->GetHandleString();
                    Warning(
                        "ANIMATION: Morph Error - Morph tags are different in '%s' and '%s' for '%s'.",
                        (&plastSequence->m_kName)->GetHandleString(), a6, blendAmounta
                    );
                }
            }
            else {
                const char* blendAmountb = (&pAnimRoot.m_pObject->m_kName)->GetHandleString();
                int32_t a6a = apSeq->m_uiArraySize;
                const char* a4 = (&apSeq->m_kName)->GetHandleString();
                int32_t a2 = plastSequence->m_uiArraySize;
                Warning(
                    "ANIMATION: Morph Error - Controller count not the same.\r\n'%s' has %d controllers "
                    "and\r\n'%s' has %d on '%s'.",
                    (&plastSequence->m_kName)->GetHandleString(), a2, a4, a6a, blendAmountb
                );
            }
        }

        if (bmorph && apSeq == plastSequence) {
            const char* blendAmountc = (&pAnimRoot.m_pObject->m_kName)->GetHandleString();
            Warning(
                "ANIMATION: Morph Error - Trying to morph from sequence to itself.\r\n'%s' on '%s'.",
                (&plastSequence->m_kName)->GetHandleString(), blendAmountc
            );
            bmorph = false;
        }
    }

    float fduration = fAnimationDefaultBlend.Float();
    uint8_t cblendFrames = 0;
    if (plastSequence) {
        TESAnimGroup* AnimGroup = plastSequence->GetAnimGroup();
        cblendFrames = AnimGroup->GetBlendOutFrames();
    }
    TESAnimGroup* v11 = apSeq->GetAnimGroup();
    uint8_t cblendFrames_2 = v11->GetBlendInFrames();
    if (cblendFrames_2 > static_cast<int32_t>(cblendFrames)) {
        TESAnimGroup* v13 = apSeq->GetAnimGroup();
        cblendFrames = v13->GetBlendInFrames();
    }
    if (cblendFrames) {
        fduration = static_cast<double>(cblendFrames) / 30.0;
    }
    if ((Interface::IsInMenuMode() && pAnimRoot == pPlayer->GetInventory3D()) ||
        Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0)) {
        fduration = fMenuModeAnimBlend.Float();
    }
    if ((egroup >= ANIM_GROUP_RELOAD_START && egroup <= ANIM_GROUP_RELOAD_Z) && (plastSequence) &&
        (pactor != pPlayer && fduration < 0.5)) {
        TESAnimGroup* v14 = apSeq->GetAnimGroup();
        if (v14->GetMove() == AGM_WALK) {
            TESAnimGroup* v15 = plastSequence->GetAnimGroup();
            if (v15->GetMove() != AGM_WALK) {
                fduration = 0.5f;
            }
        }
        else {
            TESAnimGroup* v16 = plastSequence->GetAnimGroup();
            if (v16->GetMove() == AGM_WALK) {
                fduration = 0.5f;
            }
        }
    }

    if (TESAnimGroup::IsAttackAction(elastGroup) && egroup >= ANIM_GROUP_LOOPING_RELOAD_START_ANIMS &&
        egroup <= ANIM_GROUP_RELOAD_Z && pactor == pPlayer) {
        fduration = 0.0f;
    }
    if (cSkipNextBlend) {
        fduration = 0.0f;
    }
    float* v17 = &fAnimationMult.Float();
    fduration = fduration / *v17;
    apSeq->SetPhase(0.0f, false);
    if (!plastSequence && (esection == AGS_WEAPON_UP || esection == AGS_WEAPON_DOWN)) {
        float a4a = apSeq->m_fSeqWeight;
        spManager.m_pObject->ActivateSequence(apSeq, 0, true, a4a, fduration, nullptr);
    }
    else {
        if (!(fduration < 0.009999999776482582)) {
            if (bmorph) {
                float blendAmountd = apSeq->m_fSeqWeight;
                float a6c = plastSequence->m_fSeqWeight;
                spManager.m_pObject->Morph(plastSequence, apSeq, fduration, 0, a6c, blendAmountd);
            }
            else {
                bool bretVal = false;
                if ((plastSequence) && (plastSequence->GetState())) {
                    float a6d = apSeq->m_fSeqWeight;
                    bretVal = spManager.m_pObject->CrossFade(plastSequence, apSeq, fduration, 0, true, a6d, nullptr);
                }

                if (!bretVal) {
                    if (TES::pInstance->CellTestRunning()) {
                        spManager.m_pObject->ActivateSequence(apSeq, 0, true, 1.0f, 0.0f, nullptr);
                    }
                    else {
                        if ((pActorRef) && (pActorRef->IsActor())) {
                            Actor* pactor_2 = static_cast<Actor*>(pActorRef);
                            if ((bhkRagdollController::QFootIK()) && (pactor_2->pRagdollController) &&
                                (pactor_2->pRagdollController->QFootIKActive())) {
                                pactor_2->pRagdollController->ApplyBoneTransforms();
                            }
                        }

                        spManager.m_pObject->BlendFromPose(apSeq, 0.0f, fduration, 0, nullptr);
                    }
                }
            }
        }
        else {
            float a4b = apSeq->m_fSeqWeight;
            spManager.m_pObject->ActivateSequence(apSeq, 0, true, a4b, 0.0f, nullptr);
        }
    }
    if ((plastSequence) && (plastSequence != apSeq)) {
        NiControllerSequence::State elastState_2 = plastSequence->GetState();
        if (elastState_2 > NiControllerSequence::kInactive &&
            (elastState_2 <= NiControllerSequence::kEaseIn || elastState_2 == NiControllerSequence::kTransDest)) {
            spManager.m_pObject->DeactivateSequence(plastSequence, 0.0f);
        }
    }

    action[esection] = AGA_START;
    return apSeq;
}

void Animation::BlendOut(ANIM_GROUP_SECTION aeSection, bool abIronSightsToggle) {
    ANIM_GROUP_SECTION eseqSection = aeSection;
    if (aeSection == AGS_WHOLE_BODY) {
        eseqSection = AGS_MOVEMENT;
    }
    else {
        if (aeSection == AGS_UPPER_BODY) {
            eseqSection = AGS_WEAPON;
        }
    }
    Actor* pactor = static_cast<Actor*>(pActorRef);
    if (eseqSection == AGS_MOVEMENT) {
        movementDelta = NiPoint3_ZERO;
    }
    BSAnimGroupSequence* pblendOutSeq = pCurrentSequence[eseqSection];
    float fduration = 0.0f;
    if ((pblendOutSeq) && (!cSkipNextBlend)) {
        float fduration_2 = fAnimationDefaultBlend.Float();
        TESAnimGroup* AnimGroup = pblendOutSeq->GetAnimGroup();
        char cblendFrames = AnimGroup->GetBlendOutFrames();
        if (cblendFrames) {
            fduration_2 = static_cast<double>(cblendFrames) / 30.0;
        }
        fduration = fduration_2 / fAnimationMult.Float();
    }

    bool bclearGroup = true;
    bool v17 = false;
    bool bstartAim = false;
    if ((pActorRef) &&
        ((eseqSection == AGS_WEAPON || aeSection == AGS_WHOLE_BODY) && pactor->IsWeaponDrawn() &&
         !pDataHandler->GetClearingData())) {
        BaseProcess* CurrentAIProcess;
        if (abIronSightsToggle &&
            (pactor->GetAnimAction() == ANIM_ACTION_ATTACK &&
             (CurrentAIProcess = pactor->pCurrentProcess, CurrentAIProcess->GetAnimActionAnimSeq()))) {
            v17 = true;
        }
        else {
            bstartAim = true;
        }
        if (eseqSection == AGS_WEAPON) {
            bclearGroup = false;
            if ((pactor->GetAnimAction() == ANIM_ACTION_RELOAD || pactor->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) &&
                pactor != pPlayer && fduration < 0.5) {
                TESAnimGroup* v4 = pblendOutSeq->GetAnimGroup();
                if (v4->GetMove() == AGM_WALK) {
                    if (!pactor->IsSneaking()) {
                        fduration = 0.5f;
                    }
                }
                else {
                    if (pactor->IsSneaking()) {
                        fduration = 0.5f;
                    }
                }
            }
            ClearGroup(AGS_WEAPON_UP, fduration);
            ClearGroup(AGS_WEAPON_DOWN, fduration);
        }
    }

    if (bclearGroup) {
        ClearGroup(aeSection, fduration);
    }
    if (v17) {
        BaseProcess* v6 = pactor->pCurrentProcess;
        BSAnimGroupSequence* v5 = v6->GetAnimActionAnimSeq();
        ANIM_GROUP_ENUM eattackAnim = v5->GetAnimGroup()->GetType();
        if (TESAnimGroup::IsIronSightsAction(eattackAnim)) {
            eattackAnim = static_cast<ANIM_GROUP_ENUM>(eattackAnim - 3);
        }
        pactor->StartWeaponGroup(eattackAnim, this);
    }
    else {
        if (bstartAim) {
            pactor->StartWeaponGroup(ANIM_GROUP_AIM, this);
        }
    }
}

// Animation::GetGroup is defined in ExtraDataObjects.cpp.
float Animation::GetGlobalTimeMultiplier() {
    return m_fGlobalTimeMultiplier;
}

void Animation::Update(TESObjectREFR* ownerObject, float fSecondsPassed, float forceTime) {
    TESAnimGroup* AnimGroup;
    TESAnimGroup* v6;
    ANIM_GROUP_ACTION v124;
    BSAnimGroupSequence* pweapSeq;
    float fAppTimeMod = appTimer.fDelta * m_fGlobalTimeMultiplier;
    if (GetAnimsLoading()) {
        while (true) {
            int32_t* v4 = &iAnimationClonePerLoop.Int();
            if (Animation::iAnimationsCloned >= static_cast<uint32_t>(*v4)) {
                break;
            }
            KFModel* pkfModel = (&kfModelList)->m_item;
            AddAnimation(pkfModel, false);
            pkfModel->DecUseCount();
            kfModelList.RemoveHead();
            if (!GetAnimsLoading()) {
                break;
            }
            ++Animation::iAnimationsCloned;
        }
        goto LABEL_27;
    }
    if (sQueuedReloadGroup == 255) {
        goto LABEL_27;
    }
    pweapSeq = pCurrentSequence[4];
    v124 = action[4];
    if (v124 <= AGA_START) {
    LABEL_26:;
        sQueuedReloadGroup = 255;
        goto LABEL_27;
    }
    if (v124 > AGA_SPECIAL_IDLE_LOOP_STOP) {
        if (v124 != AGA_SPECIAL_IDLE_STOP) {
            goto LABEL_26;
        }
    LABEL_14:;
        if (static_cast<Actor*>(pActorRef)->Actor::IsWeaponDrawn()) {
            PlayGroup(sQueuedReloadGroup, ACTION_START, -1, AGS_NONE);
            if (ownerObject == pPlayer) {
                pPlayer->StartAnimOn1stPerson(sQueuedReloadGroup, ACTION_START);
            }
            Actor* pAct = static_cast<Actor*>(ownerObject);
            bool bLoopReload = false;
            if (pAct->GetCurrentAIProcess()) {
                TESObjectWEAP* pWeap;
                BaseProcess* CurrentAIProcess = pAct->pCurrentProcess;
                if (CurrentAIProcess->GetCurrentWeapon()) {
                    BaseProcess* v122 = pAct->pCurrentProcess;
                    pWeap = static_cast<TESObjectWEAP*>(v122->GetCurrentWeapon()->pContainerObj);
                }
                else {
                    pWeap = nullptr;
                }
                if (pWeap->GetIsLoopingReload()) {
                    bLoopReload = true;
                }
            }
            if (bLoopReload) {
                pAct->SetAnimAction(ANIM_ACTION_RELOAD_LOOP, pCurrentSequence[4]);
            }
            else {
                pAct->SetAnimAction(ANIM_ACTION_RELOAD, pCurrentSequence[4]);
            }
        }
        goto LABEL_26;
    }
    if (!pweapSeq) {
        goto LABEL_14;
    }
    AnimGroup = pweapSeq->GetAnimGroup();
    if (AnimGroup->IsAimAnim()) {
        goto LABEL_14;
    }
    v6 = pweapSeq->GetAnimGroup();
    if (!v6->IsAttackAction()) {
        goto LABEL_14;
    }
LABEL_27:;
    if (!replayDelayList.IsEmpty()) {
        BSSimpleList<IDLE_REPLAY_DELAY*>* pdelayList = &replayDelayList;
        while (pdelayList) {
            IDLE_REPLAY_DELAY* pdelay = pdelayList->m_item;
            pdelay->fDelay = pdelay->fDelay - fSecondsPassed;
            if (!(pdelay->fDelay <= 0.0)) {
                pdelayList = pdelayList->m_pkNext;
            }
            else {
                if (pdelayList == &replayDelayList) {
                    pdelayList = nullptr;
                }
                else {
                    pdelayList = pdelayList->m_pkNext;
                }
                replayDelayList.Remove(pdelay);
                ourFree(pdelay);
                if (!pdelayList && !replayDelayList.IsEmpty()) {
                    pdelayList = &replayDelayList;
                }
            }
        }
    }
    Actor* pactor = nullptr;
    if ((ownerObject) && (ownerObject->IsActor())) {
        pactor = static_cast<Actor*>(ownerObject);
    }

    for (NiPointer<AnimIdle>& spAnimIdle : spAnimIdleFreeWhenInactiveA) {
        if (spAnimIdle.m_pObject) {
            if (spAnimIdle.m_pObject->GetSequence()) {
                if (spAnimIdle.m_pObject->GetSequence()->GetState() == NiControllerSequence::kAnimating) {
                    TESIdleForm* IdleForm = spAnimIdle.m_pObject->pIdleForm;
                    const char* afTime = (&spAnimIdle.m_pObject->GetSequence()->m_kName)->GetHandleString();
                    const char* arg8 = IdleForm->GetFormEditorID();
                    Warning(
                        "'%s' is trying to free a special idle '%s' sequence '%s' that is animating.",
                        pActorRef->GetFormEditorID(), arg8, afTime
                    );
                    spAnimIdle = static_cast<AnimIdle*>(nullptr);
                }
                else {
                    if (!spAnimIdle.m_pObject->GetSequence()->GetState()) {
                        AnimIdleFree(spAnimIdle);
                    }
                }
            }
            else {
                if (spAnimIdle.m_pObject->eFlags) {
                    AnimIdleFree(spAnimIdle);
                }
            }
        }
    }
    if (pAnimRoot.m_pObject) {
        BSAnimGroupSequence* ppreSeq[8];
        ANIM_GROUP_ENUM enextGroup;
        ANIM_GROUP_SECTION esection;
        float flastScaledTime[8];
        SetMoved(false);
        movementDelta.x = 0.0f;
        movementDelta.y = 0.0f;
        if (this->cSkipUpdate == 20) {
            float ft;
            if (forceTime == -1.0) {
                ft = time;
            }
            else {
                ft = forceTime;
            }
            NiUpdateData UpdateData(ft, false, false);
            pAnimRoot.m_pObject->Update(UpdateData);
            return;
        }
        ANIM_GROUP_ENUM egroup = ANIM_GROUP_NONE;
        for (esection = AGS_IDLE; esection < ANIM_GROUP_SECTION_COUNT;
             esection = static_cast<ANIM_GROUP_SECTION>(esection + 1)) {
            egroup = TESAnimGroup::GetType(group[esection]);
            enextGroup = TESAnimGroup::GetType(nextGroup[esection]);
            if (egroup == ANIM_GROUP_NONE && enextGroup != ANIM_GROUP_NONE) {
                loopCount[esection] = nextLoops[esection];
                StartGroup(nextGroup[esection], AGS_NONE);
                enextGroup = ANIM_GROUP_NONE;
                nextGroup[esection] = 255;
                egroup = TESAnimGroup::GetType(group[esection]);
            }
            BSAnimGroupSequence* pcurrentSequence = GetCurrentSequence(esection);
            ppreSeq[esection] = pcurrentSequence;
            flastScaledTime[esection] = 0.0f;
            if ((egroup != ANIM_GROUP_NONE) && (pcurrentSequence)) {
                double fscaledTime = pcurrentSequence->GetScaledTime(time);
                flastScaledTime[esection] = fscaledTime;
            }
        }
        if (forceTime != -1.0) {
            time = forceTime;
            NiUpdateData UpdateData_2(time, true, false);
            pAnimRoot.m_pObject->Update(UpdateData_2);
            return;
        }
        float timeDelta = fSecondsPassed * m_fGlobalTimeMultiplier;
        time = time + timeDelta;
        for (esection = AGS_IDLE; esection < ANIM_GROUP_SECTION_COUNT;
             esection = static_cast<ANIM_GROUP_SECTION>(esection + 1)) {
            BSAnimGroupSequence* pcurrentSequence_2 = GetCurrentSequence(esection);
            bool bupdate = true;
            egroup = TESAnimGroup::GetType(group[esection]);
            enextGroup = TESAnimGroup::GetType(nextGroup[esection]);
            if ((egroup != ANIM_GROUP_NONE) && (pcurrentSequence_2)) {
                bupdate = esection != AGS_TALKING;
                switch (this->cSkipUpdate) {
                    case 4: {
                        if (esection >= AGS_WEAPON && esection <= AGS_WEAPON_DOWN) {
                            bupdate = false;
                        }
                        break;
                    }
                    case 20: {
                        bupdate = false;
                        break;
                    }
                    case 21: {
                        if (esection >= AGS_LEFT_ARM && esection <= AGS_WEAPON_DOWN) {
                            bupdate = false;
                        }
                        break;
                    }
                    case 23: {
                        if ((esection != AGS_SPECIAL_IDLE) && (esection)) {
                            bupdate = false;
                        }

                        break;
                    }
                    default: {
                        if (esection == this->cSkipUpdate) {
                            bupdate = false;
                        }
                        break;
                    }
                }
                if (bupdate) {
                    if (pcurrentSequence_2->GetState() == NiControllerSequence::kAnimating) {
                        if (egroup < ANIM_GROUP_MOVE_FORWARD || egroup > ANIM_GROUP_TURN_RIGHT) {
                            if (egroup == ANIM_GROUP_EQUIP || egroup == ANIM_GROUP_UNEQUIP) {
                                double Phase = pcurrentSequence_2->GetPhase();
                                float v116 = Phase + (timeDelta * m_fEquipModifier - timeDelta);
                                pcurrentSequence_2->SetPhase(v116, false);
                            }
                            else {
                                if (egroup < ANIM_GROUP_EQUIP || egroup > ANIM_GROUP_COUNTER) {
                                    if (egroup >= ANIM_GROUP_RELOAD_START && egroup <= ANIM_GROUP_RELOAD_Z) {
                                        double v12 = pcurrentSequence_2->GetPhase();
                                        float v114 = v12 + (timeDelta * m_fReloadModifier - timeDelta);
                                        pcurrentSequence_2->SetPhase(v114, false);
                                    }
                                }
                                else {
                                    double v11 = pcurrentSequence_2->GetPhase();
                                    float v115 = v11 + (timeDelta * m_fAttackSpeed - timeDelta);
                                    pcurrentSequence_2->SetPhase(v115, false);
                                }
                            }
                        }
                        else {
                            double v9 = pcurrentSequence_2->GetPhase();
                            float v117 = v9 + (timeDelta * this->GetMoveSpeed() - timeDelta);
                            pcurrentSequence_2->SetPhase(v117, false);
                        }
                    }
                }
                else {
                    double v8 = pcurrentSequence_2->GetPhase();
                    float v118 = v8 - timeDelta;
                    pcurrentSequence_2->SetPhase(v118, false);
                }
                if (bupdate && pcurrentSequence_2->GetState() == NiControllerSequence::kAnimating) {
                    switch (animGroupInfo[egroup].eAction) {
                        case AGAT_START_STOP:
                        case AGAT_LOOP: {
                            if (pcurrentSequence_2->m_eCycleType) {
                                if (action[esection] == AGA_START) {
                                    double Time_0 = GetTime(pcurrentSequence_2);
                                    ANIM_GROUP_ACTION afTimei = action[esection];
                                    TESAnimGroup* v65 = pcurrentSequence_2->GetAnimGroup();
                                    if (v65->GetTime(afTimei) < Time_0) {
                                        action[esection] = AGA_STOP;
                                    }
                                }
                                if (enextGroup == ANIM_GROUP_NONE) {
                                    double v68 = GetTime(pcurrentSequence_2);
                                    TESAnimGroup* v69 = pcurrentSequence_2->GetAnimGroup();
                                    if (v69->GetTime(AGA_STOP) <= v68) {
                                        BlendOut(esection, false);
                                    }
                                }
                                else {
                                    double v66 = GetTime(pcurrentSequence_2);
                                    TESAnimGroup* v67 = pcurrentSequence_2->GetAnimGroup();
                                    if (v67->GetTime(AGA_STOP) <= v66) {
                                        if (esection == AGS_MOVEMENT && (pactor->GetMoveMode() & 15) == 0) {
                                            BlendOut(esection, false);
                                        }
                                        else {
                                            loopCount[esection] = nextLoops[esection];
                                            StartGroup(nextGroup[esection], AGS_NONE);
                                        }
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            else {
                                while (action[esection] < AGA_STOP) {
                                    double v56 = GetTime(pcurrentSequence_2);
                                    ANIM_GROUP_ACTION afTimeh = action[esection];
                                    TESAnimGroup* v57 = pcurrentSequence_2->GetAnimGroup();
                                    if (!(v57->GetTime(afTimeh) < v56)) {
                                        break;
                                    }
                                    ANIM_GROUP_ACTION v108 = action[esection];
                                    if (v108) {
                                        if (v108 == AGA_STOP) {
                                            action[esection] = AGA_STOP;
                                        }
                                    }
                                    else {
                                        action[esection] = AGA_STOP;
                                    }
                                }
                                if (enextGroup == ANIM_GROUP_NONE) {
                                    double v60 = GetTime(pcurrentSequence_2);
                                    TESAnimGroup* v61 = pcurrentSequence_2->GetAnimGroup();
                                    if (v61->GetTime(AGA_STOP) <= v60 && loopCount[esection]) {
                                        action[esection] = AGA_START;
                                        double EndKeyTime = pcurrentSequence_2->m_fEndKeyTime;
                                        float floopDelta_4 = EndKeyTime - pcurrentSequence_2->GetBeginKeyTime();
                                        float v106 = pcurrentSequence_2->GetPhase() - floopDelta_4;
                                        pcurrentSequence_2->SetPhase(v106, false);
                                        if (loopCount[esection] != -1) {
                                            --loopCount[esection];
                                        }
                                    }
                                    else {
                                        double v62 = GetTime(pcurrentSequence_2);
                                        TESAnimGroup* v63 = pcurrentSequence_2->GetAnimGroup();
                                        if (v63->GetTime(AGA_STOP) <= v62) {
                                            BlendOut(esection, false);
                                        }
                                    }
                                }
                                else {
                                    double v58 = GetTime(pcurrentSequence_2);
                                    TESAnimGroup* v59 = pcurrentSequence_2->GetAnimGroup();
                                    if (v59->GetTime(AGA_STOP) <= v58) {
                                        loopCount[esection] = nextLoops[esection];
                                        StartGroup(nextGroup[esection], AGS_NONE);
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            break;
                        }
                        case AGAT_SPECIAL_IDLE_LOOP: {
                            while (action[esection] < AGA_SPECIAL_IDLE_STOP) {
                                double v45 = GetTime(pcurrentSequence_2);
                                ANIM_GROUP_ACTION afTimeg = action[esection];
                                TESAnimGroup* v46 = pcurrentSequence_2->GetAnimGroup();
                                if (!(v46->GetTime(afTimeg) < v45)) {
                                    break;
                                }
                                ANIM_GROUP_ACTION v110 = action[esection];
                                if (v110) {
                                    if (v110 == AGA_STOP) {
                                        action[esection] = AGA_SPECIAL_IDLE_LOOP_STOP;
                                    }
                                    else {
                                        if (v110 == AGA_SPECIAL_IDLE_LOOP_STOP) {
                                            action[esection] = AGA_SPECIAL_IDLE_STOP;
                                        }
                                    }
                                }
                                else {
                                    action[esection] = AGA_STOP;
                                }
                            }
                            if (enextGroup == ANIM_GROUP_NONE) {
                                double v49 = GetTime(pcurrentSequence_2);
                                TESAnimGroup* v50 = pcurrentSequence_2->GetAnimGroup();
                                if (v50->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP) <= v49 && loopCount[esection]) {
                                    ZeroGlobalTransform();
                                    action[esection] = AGA_STOP;
                                    TESAnimGroup* v51 = pcurrentSequence_2->GetAnimGroup();
                                    double Time = v51->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                                    TESAnimGroup* v53 = pcurrentSequence_2->GetAnimGroup();
                                    float floopDelta_3 = Time - v53->GetTime(AGA_STOP);
                                    float v109 = pcurrentSequence_2->GetPhase() - floopDelta_3;
                                    pcurrentSequence_2->SetPhase(v109, false);
                                    flastScaledTime[esection] = flastScaledTime[esection] - floopDelta_3;
                                    if (loopCount[esection] != -1 && loopCount[esection] < 255) {
                                        --loopCount[esection];
                                    }
                                }
                                else {
                                    double v54 = GetTime(pcurrentSequence_2);
                                    TESAnimGroup* v55 = pcurrentSequence_2->GetAnimGroup();
                                    if (v55->GetTime(AGA_SPECIAL_IDLE_STOP) <= v54) {
                                        if (spAnimIdle.m_pObject->eType == AnimIdle::PLAY_TYPE_QUEUE) {
                                            spAnimIdle.m_pObject->eFlags = AnimIdle::ANIM_IDLE_DONE_PLAYING;
                                        }
                                        else {
                                            BlendOut(esection, false);
                                        }
                                    }
                                }
                            }
                            else {
                                double v47 = GetTime(pcurrentSequence_2);
                                TESAnimGroup* v48 = pcurrentSequence_2->GetAnimGroup();
                                if (v48->GetTime(AGA_SPECIAL_IDLE_STOP) <= v47) {
                                    loopCount[esection] = nextLoops[esection];
                                    StartGroup(nextGroup[esection], AGS_NONE);
                                    enextGroup = ANIM_GROUP_NONE;
                                    nextGroup[esection] = 255;
                                }
                            }
                            break;
                        }
                        case AGAT_EQUIP:
                        case AGAT_UNEQUIP:
                        case AGAT_ATTACK_POWER:
                        case AGAT_LOOPING_RELOAD: {
                            if (action[esection] < AGA_SPECIAL_IDLE_LOOP_STOP) {
                                double v17 = GetTime(pcurrentSequence_2);
                                ANIM_GROUP_ACTION afTimeb = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                TESAnimGroup* v18 = pcurrentSequence_2->GetAnimGroup();
                                if (v18->GetTime(afTimeb) < v17) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_LOOP_STOP) {
                                double v19 = GetTime(pcurrentSequence_2);
                                TESAnimGroup* v20 = pcurrentSequence_2->GetAnimGroup();
                                if (v20->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP) <= v19) {
                                    if (enextGroup != ANIM_GROUP_NONE) {
                                        goto LABEL_176;
                                    }
                                    BlendOut(esection, false);
                                    if ((pactor) &&
                                        (animGroupInfo[egroup].eAction == AGAT_EQUIP ||
                                         animGroupInfo[egroup].eAction == AGAT_UNEQUIP)) {
                                        BaseProcess* pprocess = pactor->pCurrentProcess;
                                        if (pprocess) {
                                            if (pactor == pPlayer) {
                                                pactor->SetHavokWeapon();
                                            }
                                            else {
                                                pprocess->AddPostAnimationAction(
                                                    BaseProcess::POSTANIM_ACTION_HAVOKWEAPON
                                                );
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK: {
                            if (action[esection] < AGA_ATTACK_STOP) {
                                double v13 = GetTime(pcurrentSequence_2);
                                ANIM_GROUP_ACTION afTimea = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                TESAnimGroup* v14 = pcurrentSequence_2->GetAnimGroup();
                                if (v14->GetTime(afTimea) < v13) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_ATTACK_STOP) {
                                double v15 = GetTime(pcurrentSequence_2);
                                TESAnimGroup* v16 = pcurrentSequence_2->GetAnimGroup();
                                if (v16->GetTime(AGA_ATTACK_STOP) <= v15) {
                                    if (enextGroup != ANIM_GROUP_NONE) {
                                        goto LABEL_176;
                                    }
                                    BlendOut(esection, false);
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK_THROW: {
                            if (action[esection] < AGA_ATTACK_STOP) {
                                double v21 = GetTime(pcurrentSequence_2);
                                ANIM_GROUP_ACTION afTimec = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                TESAnimGroup* v22 = pcurrentSequence_2->GetAnimGroup();
                                if (v22->GetTime(afTimec) < v21) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_ATTACK_STOP) {
                                double v23 = GetTime(pcurrentSequence_2);
                                TESAnimGroup* v24 = pcurrentSequence_2->GetAnimGroup();
                                if (v24->GetTime(AGA_ATTACK_STOP) <= v23) {
                                    if (enextGroup != ANIM_GROUP_NONE) {
                                        goto LABEL_176;
                                    }
                                    BlendOut(esection, false);
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK_MINE: {
                            if (action[esection] < AGA_SPECIAL_IDLE_STOP) {
                                double v25 = GetTime(pcurrentSequence_2);
                                ANIM_GROUP_ACTION afTimed = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                TESAnimGroup* v26 = pcurrentSequence_2->GetAnimGroup();
                                if (v26->GetTime(afTimed) < v25) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_STOP) {
                                double v27 = GetTime(pcurrentSequence_2);
                                TESAnimGroup* v28 = pcurrentSequence_2->GetAnimGroup();
                                if (v28->GetTime(AGA_SPECIAL_IDLE_STOP) <= v27) {
                                    if (enextGroup != ANIM_GROUP_NONE) {
                                        goto LABEL_176;
                                    }
                                    BlendOut(esection, false);
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK_SPIN: {
                            if (action[esection] < AGA_SPECIAL_IDLE_STOP) {
                                double v29 = GetTime(pcurrentSequence_2);
                                ANIM_GROUP_ACTION afTimee = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                TESAnimGroup* v30 = pcurrentSequence_2->GetAnimGroup();
                                if (v30->GetTime(afTimee) < v29) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if ((action[esection] == AGA_SPECIAL_IDLE_LOOP_STOP) && (pactor) &&
                                (pactor->pCurrentProcess)) {
                                BaseProcess* v113 = pactor->pCurrentProcess;
                                if ((v113->IsSustainedFire() && pactor->GetAnimAction() == ANIM_ACTION_ATTACK) ||
                                    (pActorRef == pPlayer &&
                                     (pVATS.eMode == VATS::VATS_PLAYBACK && pVATS.GetCurrentAction()))) {
                                    TESAnimGroup* v31 = pcurrentSequence_2->GetAnimGroup();
                                    double v32 = v31->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                                    if (flastScaledTime[esection] <= v32) {
                                        TESAnimGroup* v33 = pcurrentSequence_2->GetAnimGroup();
                                        double v34 = v33->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                                        TESAnimGroup* v35 = pcurrentSequence_2->GetAnimGroup();
                                        for (float floopDelta = v34 - v35->GetTime(AGA_STOP);;
                                             flastScaledTime[esection] = flastScaledTime[esection] - floopDelta) {
                                            double v36 = GetTime(pcurrentSequence_2);
                                            ANIM_GROUP_ACTION afTimef = action[esection];
                                            TESAnimGroup* v37 = pcurrentSequence_2->GetAnimGroup();
                                            if (!(v37->GetTime(afTimef) < v36)) {
                                                break;
                                            }
                                            float v112 = pcurrentSequence_2->GetPhase() - floopDelta;
                                            pcurrentSequence_2->SetPhase(v112, false);
                                        }
                                        action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] - 1);
                                    }
                                }
                            }

                            if (bSpinAttackReleasedEarly && action[esection] == AGA_STOP) {
                                TESAnimGroup* v38 = pcurrentSequence_2->GetAnimGroup();
                                double v39 = v38->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                                if (flastScaledTime[esection] <= v39) {
                                    TESAnimGroup* v40 = pcurrentSequence_2->GetAnimGroup();
                                    double v41 = v40->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                                    TESAnimGroup* v42 = pcurrentSequence_2->GetAnimGroup();
                                    float floopDelta_2 = v41 - v42->GetTime(AGA_STOP);
                                    float v111 = pcurrentSequence_2->GetPhase() + floopDelta_2;
                                    pcurrentSequence_2->SetPhase(v111, false);
                                    flastScaledTime[esection] = flastScaledTime[esection] + floopDelta_2;
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_STOP) {
                                double v43;
                                TESAnimGroup* v44;
                                v43 = GetTime(pcurrentSequence_2);
                                v44 = pcurrentSequence_2->GetAnimGroup();
                                if (v44->GetTime(AGA_SPECIAL_IDLE_STOP) <= v43) {
                                    if (enextGroup == ANIM_GROUP_NONE) {
                                        BlendOut(esection, false);
                                    }
                                    else {
                                    LABEL_176:;
                                        loopCount[esection] = nextLoops[esection];
                                        StartGroup(nextGroup[esection], AGS_NONE);
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            continue;
                        }
                    }
                }
            }
        }
        if (pAccumRoot) {
            NiPoint3 lastPos = AccumRootTranslate;
            bool bfakeDelta = false;
            NiPoint3 posFakeDelta = NiPoint3_ZERO;
            if (pactor->IsMoving()) {
                uint16_t smoveMode = pactor->GetMoveMode();
                if ((pCurrentSequence[1]) && ((smoveMode & 15) != 0)) {
                    ANIM_GROUP_ENUM eanimGroup = ANIM_GROUP_IDLE;
                    TESAnimGroup* v70 = pCurrentSequence[1]->GetAnimGroup();
                    bfakeDelta = v70->IsJumpingLoopAnim();
                    if (pCurrentSequence[1]->GetAnimGroup()->GetType() == ANIM_GROUP_JUMP_START) {
                        bfakeDelta = true;
                    }
                    else {
                        int32_t State = pCurrentSequence[1]->GetState();
                        if (State == 2 || State == 5) {
                            bfakeDelta = true;
                        }
                    }
                    NiTransformController* ptransCont =
                        static_cast<NiTransformController*>(pAccumRoot->GetController(&NiTransformController::ms_RTTI));
                    NiTimeController* ptimeCont = (NiTimeController*)ptransCont;
                    NiBlendInterpolator* pblendInterp =
                        static_cast<NiBlendInterpolator*>(ptransCont->GetInterpolator(0));
                    if (pblendInterp) {
                        NiInterpolator* pinter = (NiInterpolator*)pblendInterp;
                        int32_t ihighest = pblendInterp->GetHighestPriority();
                        NiInterpolator* pmoveInterp = pCurrentSequence[1]->GetInterpolatorAt(0);
                        pinter = static_cast<NiInterpolator*>(
                            NiObject::DynamicCastStatic(&NiBlendInterpolator::ms_RTTI, pmoveInterp)
                        );
                        const char* HandleString = (&pAccumRoot->m_kName)->GetHandleString();
                        if (pCurrentSequence[1]->GetPriority(HandleString, nullptr) < ihighest) {
                            bfakeDelta = false;
                        }
                    }
                    if (bfakeDelta) {
                        if ((smoveMode & 512) != 0) {
                            if ((smoveMode & 1) != 0) {
                                eanimGroup = ANIM_GROUP_FAST_FORWARD;
                            }
                            else {
                                if ((smoveMode & 2) != 0) {
                                    eanimGroup = ANIM_GROUP_FAST_BACK;
                                }
                                else {
                                    if ((smoveMode & 4) != 0) {
                                        eanimGroup = ANIM_GROUP_FAST_LEFT;
                                    }
                                    else {
                                        if ((smoveMode & 8) != 0) {
                                            eanimGroup = ANIM_GROUP_FAST_RIGHT;
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            if ((smoveMode & 0xFF00) != 0) {
                                if ((smoveMode & 1) != 0) {
                                    eanimGroup = ANIM_GROUP_MOVE_FORWARD;
                                }
                                else {
                                    if ((smoveMode & 2) != 0) {
                                        eanimGroup = ANIM_GROUP_MOVE_BACK;
                                    }
                                    else {
                                        if ((smoveMode & 4) != 0) {
                                            eanimGroup = ANIM_GROUP_MOVE_LEFT;
                                        }
                                        else {
                                            if ((smoveMode & 8) != 0) {
                                                eanimGroup = ANIM_GROUP_MOVE_RIGHT;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (eanimGroup == pCurrentSequence[1]->GetAnimGroup()->GetType()) {
                            float afTimej = this->GetMoveSpeed();
                            float fAnimationMult_1 = fAnimationMult.Float();
                            TESAnimGroup* v72 = pCurrentSequence[1]->GetAnimGroup();
                            posFakeDelta = v72->GetDeltaPerSecond() * fAppTimeMod * fAnimationMult_1 * afTimej;
                        }
                        else {
                            uint16_t sanimGroup = pactor->GetAnimGroup(eanimGroup, nullptr, false, nullptr);
                            eanimGroup = TESAnimGroup::GetType(sanimGroup);
                            if (eanimGroup) {
                                AnimSequenceBase* panimSeq;
                                if (pAnimSequenceMap->GetAt(sanimGroup, panimSeq)) {
                                    BSAnimGroupSequence* pseq = panimSeq->GetSequenceToPlay(-1);
                                    if (pseq) {
                                        float afTimek = this->GetMoveSpeed();
                                        float fAnimationMult_1a = fAnimationMult.Float();
                                        TESAnimGroup* v76 = pseq->GetAnimGroup();
                                        posFakeDelta =
                                            v76->GetDeltaPerSecond() * fAppTimeMod * fAnimationMult_1a * afTimek;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            UpdateBipOnly(time, &AccumRootTranslate, true);
            if (bfakeDelta) {
                movementDelta.x = posFakeDelta.x;
                movementDelta.y = posFakeDelta.y;
            }
            else {
                NiPoint3 delta = AccumRootTranslate - lastPos;
                movementDelta.x = delta.x;
                movementDelta.y = delta.y;
            }
            movementDelta.x = ownerObject->GetScale() * movementDelta.x;
            movementDelta.y = ownerObject->GetScale() * movementDelta.y;
            if ((!bfakeDelta && !pCurrentSequence[1]) && (pLastMovementSequence) &&
                (pLastMovementSequence->GetState())) {
                movementDelta.y = 0.0f;
                movementDelta.x = 0.0f;
            }

            if ((pLastMovementSequence) && (!pLastMovementSequence->GetState())) {
                pLastMovementSequence = nullptr;
            }
        }
        bool bInDialog = false;
        if ((pactor) && (pactor->pCurrentProcess)) {
            BaseProcess* v104 = pactor->pCurrentProcess;
            bInDialog = v104->GetTalkingtoPC();
        }

        if (!bInDialog &&
            (!pAnimRoot.m_pObject->GetAppCulled() ||
             (this == pPlayer->GetAnimation(true) && Interface::IsGunScopeVisible()))) {
            for (esection = AGS_IDLE; esection < ANIM_GROUP_SECTION_COUNT;
                 esection = static_cast<ANIM_GROUP_SECTION>(esection + 1)) {
                char cSkipUpdate = this->cSkipUpdate;
                if (cSkipUpdate != 20) {
                    if (cSkipUpdate == 21) {
                        if (esection < AGS_LEFT_ARM || esection > AGS_WEAPON) {
                            goto LABEL_307;
                        }
                    }
                    else {
                        if (cSkipUpdate != 23) {
                            if (esection == this->cSkipUpdate) {
                                continue;
                            }
                        LABEL_307:;
                            BSAnimGroupSequence* pcurrentSequence_3 = GetCurrentSequence(esection);
                            if ((pcurrentSequence_3) &&
                                (ppreSeq[esection] == pcurrentSequence_3 &&
                                 (pcurrentSequence_3->GetState() == NiControllerSequence::kAnimating ||
                                  (pcurrentSequence_3->GetState() == NiControllerSequence::kEaseIn ||
                                   pcurrentSequence_3->GetState() == NiControllerSequence::kTransDest)))) {
                                bool bplaySounds = true;
                                if (pSoundPriorityBoneName[esection]) {
                                    if (pSoundPriorityBone[esection]) {
                                        BSAnimGroupSequence* phighestSeq = nullptr;
                                        uint8_t cpriority = 0;
                                        for (ANIM_GROUP_SECTION etestSection = AGS_IDLE;
                                             etestSection < ANIM_GROUP_SECTION_COUNT;
                                             etestSection = static_cast<ANIM_GROUP_SECTION>(etestSection + 1)) {
                                            if ((pCurrentSequence[etestSection]) && (pSoundPriorityBone[esection]) &&
                                                ((&pSoundPriorityBone[esection]->m_kName)->GetHandleString())) {
                                                ANIM_GROUP_SECTION afTimel = esection;
                                                TESAnimGroup* v80 = pCurrentSequence[etestSection]->GetAnimGroup();
                                                uint8_t ctestPriority = v80->GetSectionPriority(afTimel);
                                                if (ctestPriority > static_cast<int32_t>(cpriority)) {
                                                    phighestSeq = pCurrentSequence[etestSection];
                                                    cpriority = ctestPriority;
                                                }
                                            }
                                        }
                                        if (!phighestSeq || phighestSeq != pcurrentSequence_3) {
                                            bplaySounds = false;
                                        }
                                    }
                                }
                                else {
                                    bplaySounds = false;
                                }
                                if (bplaySounds) {
                                    float fscaledTime_2 = pcurrentSequence_3->GetScaledTime(time);
                                    if (fscaledTime_2 != -3.4028235e+38f &&
                                        flastScaledTime[esection] != -3.4028235e+38f &&
                                        flastScaledTime[esection] != fscaledTime_2) {
                                        BSAnimGroupSequence* pcurrentSequence_4 = pcurrentSequence_3;
                                        float fscaledTime_3 = fscaledTime_2;
                                        float fAnimationMult_1b = flastScaledTime[esection];
                                        TESAnimGroup* v81 = pcurrentSequence_3->GetAnimGroup();
                                        v81->PlaySounds(
                                            ownerObject, fAnimationMult_1b, fscaledTime_3, pcurrentSequence_4
                                        );
                                    }
                                }
                            }

                            continue;
                        }
                        if (esection == AGS_SPECIAL_IDLE || esection == AGS_IDLE) {
                            goto LABEL_307;
                        }
                    }
                }
            }
        }
        if (spAnimIdle.m_pObject) {
            spAnimIdle.m_pObject->Update(this);
        }
        if (cSkipNextBlend) {
            cSkipNextBlend = 0;
        }
        this->cSkipUpdate = -1;
    }
}

bool Animation::GroupLoaded(uint16_t asTestGroup) {
    AnimSequenceBase* pSequence;
    return pAnimSequenceMap->GetAt(asTestGroup, pSequence);
}

// The list owns its file names: each is freed through ourFree.
void Animation::AddAnimationsFromList(BSSimpleList<const char*>* apFileList) {
    if (apFileList) {
        AutoMemContext tempMemContext(
            MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 750
        );
        bool bqueueAnims = ShouldQueueSequenceCloning();
        while (!apFileList->IsEmpty()) {
            const char* pfilename = apFileList->m_item;
            KFModel* pkfModel = pModelLoader->LoadKF(pfilename);
            AddAnimation(pkfModel, bqueueAnims);
            ourFree(const_cast<char*>(pfilename));
            apFileList->RemoveHead();
        }
        delete apFileList;
    }
}

void Animation::ForceSection(
    ANIM_GROUP_SECTION aeSection,
    uint16_t asNewGroup,
    ANIM_GROUP_ACTION aeNewAction,
    float afNewTime,
    char aiSequenceIndex
) {
    ANIM_GROUP_SECTION esection = aeSection;
    if (aeSection == AGS_WHOLE_BODY) {
        esection = AGS_MOVEMENT;
    }
    else {
        if (aeSection == AGS_UPPER_BODY) {
            esection = AGS_WEAPON;
        }
    }
    if (aeNewAction == AGA_NONE) {
        AnimSequenceBase* panimSeq;
        ClearGroup(aeSection, 0.0f);
        if ((asNewGroup != 255 && pAnimSequenceMap->GetAt(asNewGroup, panimSeq)) &&
            (panimSeq->GetSequenceToPlay(aiSequenceIndex))) {
            BSAnimGroupSequence* v6 = panimSeq->GetSequenceToPlay(aiSequenceIndex);
            pCurrentSequence[esection] = v6;
            spManager.m_pObject->ActivateSequence(pCurrentSequence[esection], 0, false, 1.0f, 0.0f, nullptr);
        }
    }
    else {
        if (GroupLoaded(asNewGroup)) {
            AnimSequenceBase* panimSeq_2;
            if (pAnimSequenceMap->GetAt(asNewGroup, panimSeq_2)) {
                BSAnimGroupSequence* pseq = panimSeq_2->GetSequenceToPlay(aiSequenceIndex);
                StartGroup(pseq, asNewGroup, aeSection);
            }
            action[esection] = aeNewAction;
        }
    }
    group[esection] = asNewGroup;
    time = afNewTime;
}

void Animation::UpdateMovement(TESObjectREFR* apRef) {
    UpdateMovementNoWorldUpdate(static_cast<Actor*>(apRef));
    UpdateSceneGraphNoController();
}

// abControllers is unused.
void Animation::UpdateBipOnly(float afTime, NiPoint3* apTranslate, bool abControllers) {
    if (pAccumRoot) {
        pAccumRoot->SetTranslate(AccumRootTranslate);
    }
    NiNode* pkAccumRoot = pAccumRoot;
    static_cast<BSBipNode*>(pAnimRoot.operator->())->UpdateBipOnly(afTime, pkAccumRoot);
    if (pAccumRoot) {
        if (apTranslate) {
            *apTranslate = pAccumRoot->GetTranslate();
        }
        pAccumRoot->SetTranslate(NiPoint3_ZERO);
    }
}

void Animation::UpdateSceneGraphNoController() {
    if ((pAnimRoot.m_pObject) && (pAccumRoot)) {
        if (iNumHWThreads.Int() <= 1 ||
            (cSkipThreadQueue || (GetDelayUpdateScenegraph() || pSaveLoadGame->GetSaveGameLoading()))) {
            NiUpdateData updateData(time, true, false);
            NiPointer<NiTimeController> spAnimRootCon(pAnimRoot.m_pObject->GetControllers());
            NiPointer<NiTimeController> spAccumRootCon(pAccumRoot->GetControllers());
            pAnimRoot.m_pObject->SetControllers(nullptr);
            pAccumRoot->SetControllers(nullptr);
            NiNode* v1 = pAnimRoot.m_pObject;
            v1->UpdateDownwardPass(updateData, 0);
            pAnimRoot.m_pObject->SetControllers(spAnimRootCon.m_pObject);
            pAccumRoot->SetControllers(spAccumRootCon.m_pObject);
        }
        else {
            SetDelayUpdateScenegraph(true);
        }
    }
}

void Animation::SetAttackSpeed(float attackSpeed) {
    if (!(attackSpeed < 0.10000000149011612)) {
        m_fAttackSpeed = attackSpeed;
    }
    else {
        m_fAttackSpeed = 0.10000000149011612f;
    }
}

void Animation::SkipUpdate(ANIM_GROUP_SECTION aeSection) {
    cSkipUpdate = static_cast<char>(aeSection);
}

void Animation::SpecialIdleReplace(TESIdleForm* apIdleForm, Actor* apActor, ANIM_GROUP_SECTION aeSection) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    // Uses the context buffer's address, not the constructor's result.
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 4797
    );
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        AnimIdle* pIdle = new AnimIdle(apIdleForm, aeSection, AnimIdle::PLAY_TYPE_REPLACE, apActor, false, this);
        spAnimIdleQueued = pIdle;
    }
    else {
        AnimIdle* pIdle = new AnimIdle(apIdleForm, aeSection, AnimIdle::PLAY_TYPE_REPLACE, apActor, true, nullptr);
        spAnimIdleQueued = pIdle;
    }
    SpecialIdleReplace();
    pContext->~AutoMemContext();
}

bool Animation::UpdateQueuedScenegraph() {
    if (!GetDelayUpdateScenegraph()) {
        return false;
    }
    UpdateSceneGraphNoController();
    SetDelayUpdateScenegraph(false);
    return true;
}

void Animation::SetTime(float newTime) {
    time = newTime;
}

void Animation::SetEquipSpeed(float afSpeed) {
    m_fEquipModifier = afSpeed;
}

// The reference's base form is compared with NPC__ID and the result is stored in a local that is never read, as the
// game does. The second GetAt deliberately does not test pAnimSequenceMap for null, as the game does.
bool Animation::Init(BSSimpleList<const char*>* apFileList, NiNode* apRoot, TESObjectREFR* apRef, bool abUpdate3D) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 568
    );
    if (!apFileList || !apRoot || !apRef) {
        pContext->~AutoMemContext();
        return false;
    }
    pActorRef = apRef;
    AnimSequenceBase* pSequence;
    if (pAnimSequenceMap && pAnimSequenceMap->GetAt(0, pSequence)) {
        pContext->~AutoMemContext();
        return false;
    }
    bool bIsNPC = apRef->GetObjectReference()->GetFormType() == NPC__ID;
    (void)bIsNPC;
    pAnimRoot = apRoot;
    for (int32_t i = 0; i <= 7; ++i) {
        if (pSoundPriorityBoneName[i]) {
            pSoundPriorityBone[i] = apRef->GetActorBone(apRoot, pSoundPriorityBoneName[i]);
        }
    }
    pAnimRoot->SetSelectiveUpdate(false);
    NiControllerManager* pManager = new NiControllerManager(pAnimRoot, true);
    spManager = pManager;
    bool bDelaySequenceClone = ShouldQueueSequenceCloning();
    while (!apFileList->IsEmpty()) {
        const char* pFileName = apFileList->GetItem();
        KFModel* pKFModel = pModelLoader->LoadKF(pFileName);
        if (pKFModel && pKFModel->GetAnimGroup()) {
            AddAnimation(pKFModel, bDelaySequenceClone);
        }
        ourFree(const_cast<char*>(pFileName));
        apFileList->RemoveHead();
    }
    if (pAccumRoot) {
        pAccumRoot->SetRotate(NiMatrix3_IDENTITY);
    }
    if (apFileList) {
        delete apFileList;
    }
    bool bResult = pAnimSequenceMap->GetAt(0, pSequence);
    if (bResult && abUpdate3D) {
        BSAnimGroupSequence* pPlay = pSequence->GetSequenceToPlay(-1);
        if (pPlay) {
            pPlay->Activate(100, true, 1.0f, 0.0f, nullptr, false);
            alignas(NiUpdateData) uint8_t aucUpdateData[sizeof(NiUpdateData)];
            NiUpdateData* pUpdateData = ::new (aucUpdateData) NiUpdateData(0.0f, true, false);
            pAnimRoot->Update(*pUpdateData);
            static_cast<NiControllerSequence*>(pPlay)->Deactivate(0.0f, false);
        }
    }
    pContext->~AutoMemContext();
    return bResult;
}

void Animation::AddSpecialAnimations(BSSimpleList<const char*>* apFileList, const char* apPath) {
    if (!apFileList) {
        return;
    }
    for (BSSimpleList<const char*>* pfileList = apFileList; pfileList; pfileList = pfileList->m_pkNext) {
        char apDest[264];
        const char* pfilename = pfileList->m_item;
        BSsprintf(apDest, 260, "%s\\%s\\%s", apPath, "SpecialAnims", pfilename);
        KFModel* pkfModel = pModelLoader->LoadKF(apDest);
        bool bqueueAnims = ShouldQueueSequenceCloning();
        if (pkfModel) {
            AddAnimation(pkfModel, bqueueAnims);
        }
    }
}

bool Animation::ShouldQueueSequenceCloning() {
    bool bCanClone = true;
    if (!pPlayer || !pPlayer->PlayerCharacter::Get3D(false) || !iAnimationClonePerLoop.UInt()) {
        bCanClone = false;
    }
    else if (
        pAnimRoot.operator->() == pPlayer->PlayerCharacter::Get3D(false) ||
        pAnimRoot.operator->() == pPlayer->PlayerCharacter::Get3D(true) || pAnimRoot == pPlayer->GetInventory3D()
    ) {
        if (!Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0)) {
            bCanClone = false;
        }
    }
    else if (pSaveLoadGame->GetSaveGameLoading()) {
        bCanClone = false;
    }
    else if (pActorRef == TESObjectREFR::GetBackgroundLoadingRef()) {
        bCanClone = false;
    }
    else if (Interface::IsInMenuMode()) {
        bCanClone = false;
    }
    else if (TES::pInstance->CellTestRunning()) {
        bCanClone = false;
    }
    return bCanClone;
}

bool Animation::AddAnimation(KFModel* apKFModel, bool abDelaySequenceClone) {
    ANIM_GROUP_ENUM eanimGroup;
    uint16_t sanimGroup;
    NiPointer<TESAnimGroup> spanimGroup(apKFModel->GetAnimGroup());
    if (!spanimGroup.m_pObject) {
        goto LABEL_2;
    }
    eanimGroup = spanimGroup.m_pObject->GetType();
    sanimGroup = spanimGroup.m_pObject->sType;
    if (eanimGroup == ANIM_GROUP_NONE) {
        if (apKFModel->GetSequence()) {
            const char* v10 = apKFModel->pFilename;
            Warning(
                "ANIMATION: Animation sequence '%s' not found in TESAnimGroup::GetSequenceType for file '%s'.",
                (&apKFModel->GetSequence()->m_kName)->GetHandleString(), v10
            );
        }
    LABEL_2:;
        return false;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 794
    );
    AnimSequenceBase* panimSeq = nullptr;
    if (!pAnimSequenceMap->GetAt(sanimGroup, panimSeq)) {
        AnimSequenceBase* panimSeq_2;
        AnimSequenceSingle* v26 = static_cast<AnimSequenceSingle*>(ourMalloc(sizeof(AnimSequenceSingle)));
        if (v26) {
            panimSeq_2 = (AnimSequenceBase*)::new (v26) AnimSequenceSingle;
        }
        else {
            panimSeq_2 = nullptr;
        }
        panimSeq = panimSeq_2;
        pAnimSequenceMap->SetAt(sanimGroup, panimSeq_2);
    }
    if (panimSeq->IsSingleSeq() && panimSeq->GetSequenceToPlay(-1)) {
        if (animGroupInfo[eanimGroup].bMultipleSequences) {
            AnimSequenceBase* panimSeq_3;
            pAnimSequenceMap->RemoveAt(sanimGroup);
            AnimSequenceSingle* panimSingle = static_cast<AnimSequenceSingle*>(panimSeq);
            AnimSequenceMultiple* pAnimSeqMultiple =
                static_cast<AnimSequenceMultiple*>(ourMalloc(sizeof(AnimSequenceMultiple)));
            if (pAnimSeqMultiple) {
                panimSeq_3 = (AnimSequenceBase*)::new (pAnimSeqMultiple) AnimSequenceMultiple(panimSingle);
            }
            else {
                panimSeq_3 = nullptr;
            }
            panimSeq = panimSeq_3;
            pAnimSequenceMap->SetAt(sanimGroup, panimSeq_3);
        }
        else {
            BSAnimGroupSequence* v4;
            TESAnimGroup* AnimGroup;
            BSAnimGroupSequence* v6;
            v4 = panimSeq->GetSequenceToPlay(-1);
            AnimGroup = v4->GetAnimGroup();
            if (AnimGroup == apKFModel->GetAnimGroup()) {
            LABEL_77:;
                return true;
            }
            bool bremoveSequence = true;
            if (spAnimIdleFreeWhenInactiveA->m_pObject &&
                (v6 = spAnimIdleFreeWhenInactiveA->m_pObject->GetSequence(), v6 == panimSeq->GetSequenceToPlay(-1))) {
                bremoveSequence = false;
            }
            else {
                if (spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                    BSAnimGroupSequence* Sequence = spAnimIdleFreeWhenInactiveA[1].m_pObject->GetSequence();
                    if (Sequence == panimSeq->GetSequenceToPlay(-1)) {
                        bremoveSequence = false;
                    }
                }
            }
            if (bremoveSequence) {
                if (panimSeq->GetSequenceToPlay(-1) == pLastMovementSequence) {
                    pLastMovementSequence = nullptr;
                }
                BSAnimGroupSequence* v8 = panimSeq->GetSequenceToPlay(-1);
                spManager.m_pObject->RemoveSequence(v8);
            }
            BSAnimGroupSequence* pseq = panimSeq->GetSequenceToPlay(-1);
            if (pseq) {
                for (int32_t n = 0; n < 8; ++n) {
                    if (pCurrentSequence[n] == pseq) {
                        pCurrentSequence[n] = nullptr;
                        group[n] = 0;
                    }
                }
            }
            if ((pLastMovementSequence) && (panimSeq->GetSequenceToPlay(-1) == pLastMovementSequence)) {
                pLastMovementSequence = nullptr;
            }

            panimSeq->SetSequence(nullptr, true);
        }
    }
    if (abDelaySequenceClone &&
        (!Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0) &&
         (eanimGroup != ANIM_GROUP_HOLSTER && (eanimGroup && eanimGroup != ANIM_GROUP_DEATH)))) {
        apKFModel->IncManualRefCount();
        kfModelList.AddTail(apKFModel);
        goto LABEL_77;
    }
    NiControllerSequence* pseq_2 = apKFModel->GetSequence();
    if (eanimGroup >= ANIM_GROUP_MOVE_FORWARD &&
        (eanimGroup <= ANIM_GROUP_TURN_RIGHT && pseq_2->m_eCycleType == NiControllerSequence::CYCLE_CLAMP)) {
        pAnimSequenceMap->RemoveAt(sanimGroup);
        AnimSequenceBase* v22 = panimSeq;
        AnimSequenceBase* panimSeq_4 = panimSeq;
        if (panimSeq) {
            delete panimSeq_4;
        }
        return false;
    }

    BSAnimGroupSequence* pagSeq = nullptr;
    bool bbsSeq = NiObject::IsKindOf_0(&BSAnimGroupSequence::ms_RTTI, pseq_2);
    if (bbsSeq && pseq_2->m_uiRefCount == 1) {
        pagSeq = static_cast<BSAnimGroupSequence*>(pseq_2);
    }
    else {
        BSAnimGroupSequence* pagSeq_2;
        pagSeq_2 = new BSAnimGroupSequence(apKFModel->GetAnimGroup(), pseq_2);
        pagSeq = pagSeq_2;
    }
    panimSeq->SetSequence(pagSeq, true);
    if (spManager.m_pObject->AddSequence(pagSeq, EmptyFS, true)) {
        if (!pAccumRoot) {
            pAccumRoot = static_cast<NiNode*>(spManager.m_pObject->GetAccumRoot());
        }
        AddGroup(apKFModel->GetAnimGroup());
        goto LABEL_77;
    }
    bool bdeleteAnimSeq = false;
    const char* HandleString = (&pAnimRoot.m_pObject->m_kName)->GetHandleString();
    Warning(
        "ANIMATION: Unable to add '%s' to keyframe manager on '%s'.\r\nMake sure the animation is not skinned "
        "to bones that don't exist in our skeleton.",
        (&pagSeq->m_kName)->GetHandleString(), HandleString
    );
    MessageHandler::IncDisableWarningCount(true);
    for (uint32_t n_2 = 0;; ++n_2) {
        uint32_t CastingType = pagSeq->m_uiArraySize;
        if (n_2 >= CastingType) {
            break;
        }
        char* pname = nullptr;
        pagSeq->GetObjectNameAt(n_2, nullptr);
        NiNode* v13 = pAnimRoot.m_pObject;
        bool v19 = v13->GetObjectByName(pname) == nullptr;
        if (v19) {
            Warning("ANIMATION: Object '%s' in sequence but not skeleton.", pname);
        }
        ourFree(pname);
    }
    MessageHandler::IncDisableWarningCount(false);
    if (pagSeq) {
        if (pagSeq == pLastMovementSequence) {
            pLastMovementSequence = nullptr;
        }
        spManager.m_pObject->RemoveSequence(pagSeq);
        bdeleteAnimSeq = panimSeq->RemoveSequence(pagSeq, true);
    }
    if ((panimSeq) && (bdeleteAnimSeq)) {
        pAnimSequenceMap->RemoveAt(sanimGroup);
        if (panimSeq) {
            delete panimSeq;
        }
    }

    return false;
}

void Animation::SetMoved(bool abFlag) {
    SetBit(abFlag, 1);
}

bool Animation::GetMoved() {
    return GetBit(1);
}

// The fade percent is tested against 0.0 (a double) for a flag that is never read; the call is kept deliberately, as
// the game does. The reference is an actor.
void Animation::UpdateMovementNoWorldUpdate(TESObjectREFR* apRefObject) {
    Actor* apRef = static_cast<Actor*>(apRefObject);
    if (!pAnimRoot || !pAccumRoot) {
        return;
    }
    if (GetMoved()) {
        return;
    }
    bool bNotPlayer = apRef != pPlayer;
    bool bFadedOut = false;
    NiNode* pActorNode = apRef->Get3D();
    BSFadeNode* pFadeNode = pActorNode ? pActorNode->IsFadeNode() : nullptr;
    if (bNotPlayer && (apRef->GetLifeState() == ACTOR_LIFE_STATE_DYING || apRef->IsInCombat())) {
        bNotPlayer = false;
    }
    else if (pFadeNode && pFadeNode->GetLastFadePercent() <= 0.0) {
        bFadedOut = true;
    }
    (void)bNotPlayer;
    (void)bFadedOut;
    apRef->SetVisibilityMask(true, ACTOR_VISIBILITY_FRUSTUM);
    BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
    if (pAccumulator && pAccumulator->IsBoundVolumeOccluded(apRef->GetFormID(), false)) {
        apRef->SetVisibilityMask(false, ACTOR_VISIBILITY_OCCLUSION);
    }
    else {
        apRef->SetVisibilityMask(true, ACTOR_VISIBILITY_OCCLUSION);
    }
    if (pFadeNode && pFadeNode->GetLastFadePercent() < 9.999999747378752e-06) {
        apRef->SetVisibilityMask(false, ACTOR_VISIBILITY_FADED);
    }
    else {
        apRef->SetVisibilityMask(true, ACTOR_VISIBILITY_FADED);
    }
    float fOldZ = pAccumRoot->GetAt(0)->GetTranslate().z;
    static_cast<BSBipNode*>(pAnimRoot.operator->())->UpdateAllButBip(time);
    movementDelta.z = pAccumRoot->GetAt(0)->GetTranslate().z - fOldZ;
    SetMoved(true);
}

// Each blend value is rounded to float before the compare, as the game does.
float Animation::GetSmallestHavokBlendValue(NiNode* apNode) {
    float fSmallest = 1.0f;
    if (apNode) {
        float fBlend = bhkBlendCollisionObject::GetBlendValue(apNode, nullptr);
        if (fSmallest > fBlend) {
            fSmallest = fBlend;
        }
        if (apNode != pAccumRoot) {
            fBlend = GetSmallestHavokBlendValue(apNode->GetParent());
            if (fSmallest > fBlend) {
                fSmallest = fBlend;
            }
        }
    }
    return fSmallest;
}

bool Animation::SpecialIdleReplace() {
    if (!spAnimIdleQueued) {
        return false;
    }
    if (spAnimIdleQueued->GetFlag() != 1) {
        return false;
    }
    if (!AddAnimation(spAnimIdleQueued->GetKFModel(), false)) {
        return false;
    }
    spAnimIdleQueued = nullptr;
    return true;
}

void Animation::InitGroupSpeed(TESAnimGroup* apAnimGroup) {
    NiPoint3 delta;
    AnimSequenceBase* panimSeq;
    uint16_t sanimGroup = apAnimGroup->sType;
    ANIM_GROUP_ENUM eanimGroup = apAnimGroup->GetType();
    if (spManager.m_pObject &&
        (pAnimSequenceMap->GetAt(sanimGroup, panimSeq) &&
         (eanimGroup != ANIM_GROUP_DYNAMIC_IDLE && eanimGroup != ANIM_GROUP_SPECIAL_IDLE))) {
        if (spManager.m_pObject->GetAccumRoot()) {
            bool bcheckForMovement = false;
            ANIM_GROUP_ACTION estart = AGA_START;
            ANIM_GROUP_ACTION estop = AGA_STOP;
            if ((panimSeq->GetSequenceToPlay(-1)) && (!animGroupInfo[eanimGroup].bMultipleSequences)) {
                int32_t bMultipleSequences = animGroupInfo[eanimGroup].bMultipleSequences;
                switch (animGroupInfo[eanimGroup].eAction) {
                    case AGAT_START_STOP:
                    case AGAT_LOOP: {
                        estart = AGA_START;
                        estop = AGA_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    case AGAT_SPECIAL_IDLE_LOOP: {
                        estart = AGA_START;
                        estop = AGA_SPECIAL_IDLE_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    case AGAT_ATTACK: {
                        estart = AGA_START;
                        estop = AGA_ATTACK_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    case AGAT_ATTACK_POWER: {
                        estart = AGA_START;
                        estop = AGA_SPECIAL_IDLE_LOOP_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    case AGAT_ATTACK_THROW: {
                        estart = AGA_START;
                        estop = AGA_ATTACK_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    case AGAT_ATTACK_MINE: {
                        estart = AGA_START;
                        estop = AGA_SPECIAL_IDLE_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    case AGAT_ATTACK_SPIN: {
                        estart = AGA_START;
                        estop = AGA_SPECIAL_IDLE_STOP;
                        bcheckForMovement = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            if (bcheckForMovement) {
                BSAnimGroupSequence* v3 = panimSeq->GetSequenceToPlay(-1);
                NiPointer<BSAnimGroupSequence> spseq(v3);
                NiPoint3 interpDelta = NiPoint3_ZERO;
                for (int32_t n = 0; n < spseq.m_pObject->m_uiArraySize; ++n) {
                    NiAVObject* TargetAt =
                        static_cast<NiAVObject*>(spseq.m_pObject->GetTargetAt(static_cast<uint32_t>(n)));
                    if (TargetAt == spseq.m_pObject->m_pkAccumRoot) {
                        NiInterpolator* pinterp = spseq.m_pObject->GetInterpolatorAt(static_cast<uint32_t>(n));
                        float fstartTime = spseq.m_pObject->GetBeginKeyTime();
                        float fstopTime__pinterp = spseq.m_pObject->m_fEndKeyTime;
                        NiQuatTransform quatStart;
                        NiQuatTransform quatStop;
                        pinterp->Update(fstartTime, nullptr, quatStart);
                        pinterp->Update(fstopTime__pinterp, nullptr, quatStop);
                        if (quatStart.IsTranslateValid() && quatStop.IsTranslateValid()) {
                            ::new (&delta) NiPoint3((&quatStop)->m_kTranslate - (&quatStart)->m_kTranslate);
                            float v9 = fstopTime__pinterp - fstartTime;
                            delta /= v9;
                            apAnimGroup->SetSpeed(&delta);
                        }
                        break;
                    }
                }
            }
            ANIM_GROUP_ENUM Type_0 = TESAnimGroup::GetType(sanimGroup);
            if (Type_0 >= ANIM_GROUP_MOVE_FORWARD && Type_0 <= ANIM_GROUP_DODGE_RIGHT &&
                apAnimGroup->GetSpeed() == 0.0) {
                const char* fScalar = (&pAnimRoot.m_pObject->m_kName)->GetHandleString();
                const char* pName = animGroupInfo[apAnimGroup->GetType()].pName;
                const char* v5 = animWeaponInfo[apAnimGroup->GetWeapon()];
                Warning(
                    "ANIMATION: AnimGroup '%s%s%s' for '%s' was exported with 'Animate in Place' from MAX.\r\n",
                    animMoveInfo[apAnimGroup->GetMove()], v5, pName, fScalar
                );
            }
        }
        else {
            Warning(
                "ANIMATION: Could not find AccumRoot in animation '%s'.",
                (&pAnimRoot.m_pObject->m_kName)->GetHandleString()
            );
        }
    }
}

void Animation::SpecialIdleFree(bool abFreeQueued, bool abForceDelete) {
    if (spAnimIdle.m_pObject) {
        ANIM_GROUP_SECTION esection = spAnimIdle.m_pObject->eSection;
        if (esection == AGS_WHOLE_BODY) {
            esection = AGS_MOVEMENT;
        }
        else {
            if (esection == AGS_UPPER_BODY) {
                esection = AGS_WEAPON;
            }
        }
        if (spAnimIdle.m_pObject->GetSequence() && pCurrentSequence[esection] == spAnimIdle.m_pObject->GetSequence()) {
            if (abForceDelete) {
                pCurrentSequence[esection] = nullptr;
            }
            else {
                int32_t State = spAnimIdle.m_pObject->GetSequence()->GetState();
                if (State == 2 || State == 5) {
                    Warning(
                        "ANIMATION: Trying to SpecialIdleFree an animation '%s' that just started.",
                        (&spAnimIdle.m_pObject->GetSequence()->m_kName)->GetHandleString()
                    );
                    return;
                }
                ANIM_GROUP_SECTION esection_2 = spAnimIdle.m_pObject->eSection;
                BlendOut(esection_2, false);
            }
        }
        if (abForceDelete || spAnimIdleFreeWhenInactiveA->m_pObject) {
            if (abForceDelete || spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                AnimIdleFree(spAnimIdle);
            }
            else {
                spAnimIdleFreeWhenInactiveA[1] = spAnimIdle;
                spAnimIdle = static_cast<AnimIdle*>(nullptr);
            }
        }
        else {
            *spAnimIdleFreeWhenInactiveA = spAnimIdle;
            spAnimIdle = static_cast<AnimIdle*>(nullptr);
        }
    }
    if (abFreeQueued && spAnimIdleQueued.m_pObject) {
        ANIM_GROUP_SECTION esection_3 = spAnimIdleQueued.m_pObject->eSection;
        if (esection_3 == AGS_WHOLE_BODY) {
            esection_3 = AGS_MOVEMENT;
        }
        else {
            if (esection_3 == AGS_UPPER_BODY) {
                esection_3 = AGS_WEAPON;
            }
        }
        if (spAnimIdleQueued.m_pObject->GetSequence() &&
            pCurrentSequence[esection_3] == spAnimIdleQueued.m_pObject->GetSequence()) {
            if (abForceDelete) {
                pCurrentSequence[esection_3] = nullptr;
            }
            else {
                int32_t v4 = spAnimIdleQueued.m_pObject->GetSequence()->GetState();
                if (v4 == 2 || v4 == 5) {
                    Warning(
                        "ANIMATION: Trying to SpecialIdleFree an animation '%s' that just started.",
                        (&spAnimIdleQueued.m_pObject->GetSequence()->m_kName)->GetHandleString()
                    );
                    return;
                }
                BlendOut(esection_3, false);
            }
        }
        if (abForceDelete || spAnimIdleFreeWhenInactiveA->m_pObject) {
            if (abForceDelete || spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                AnimIdleFree(spAnimIdleQueued);
            }
            else {
                spAnimIdleFreeWhenInactiveA[1] = spAnimIdleQueued;
                spAnimIdleQueued = static_cast<AnimIdle*>(nullptr);
            }
        }
        else {
            *spAnimIdleFreeWhenInactiveA = spAnimIdleQueued;
            spAnimIdleQueued = static_cast<AnimIdle*>(nullptr);
        }
    }
    else {
        spAnimIdle = spAnimIdleQueued;
        spAnimIdleQueued = static_cast<AnimIdle*>(nullptr);
    }
}

float Animation::GetReloadSpeed() const {
    return m_fReloadModifier;
}

void Animation::ClearAccumRootTranslate() {
    AccumRootTranslate = NiPoint3_ZERO;
}

void Animation::SkipNextBlend() {
    cSkipNextBlend = 1;
}

BSAnimGroupSequence* Animation::SpecialIdleGetSequence() {
    if (!spAnimIdle) {
        return nullptr;
    }
    AnimIdle* v2 = spAnimIdle;
    return v2->AnimIdle::GetSequence();
}

void Animation::SetMoveSpeed(float moveSpeed) {
    m_fMoveSpeed = moveSpeed;
}

void Animation::ClearDelta() {
    movementDelta = NiPoint3_ZERO;
}

BSAnimGroupSequence* Animation::GetSpecialIdleSequence() {
    if (!spAnimIdle) {
        return nullptr;
    }
    AnimIdle* v1 = spAnimIdle;
    return v1->AnimIdle::GetSequence();
}

void Animation::SetLoopCount(ANIM_GROUP_SECTION aeSection, int32_t aiCount) {
    loopCount[(uint32_t)aeSection] = aiCount;
}

void Animation::AddGroup(TESAnimGroup* pAnimGroup) {
    if (pAnimGroup) {
        if (((group[1] == 255) && (nextGroup[1] == 255)) && (!pAnimGroup->sType)) {
            nextGroup[1] = pAnimGroup->sType;
        }
        InitGroupSpeed(pAnimGroup);
    }
}

void Animation::RemoveObject(NiAVObject* apObject) {
    NiNode* sceneRoot = Animation::GetSceneRoot();
    if (sceneRoot) {
        static_cast<BSBipNode*>(sceneRoot)->RemoveObject(apObject);
    }
}

bool Animation::GetDelayUpdateScenegraph() {
    return GetBit(2);
}

void Animation::SetDelayUpdateScenegraph(bool abFlag) {
    SetBit(abFlag, 2);
}

void Animation::RevertTES(TESObjectREFR* apRef) {
    ClearGroup(AGS_WHOLE_BODY, 0.0);
    ZeroGlobalTransform();
    if (spAnimIdle) {
        AnimIdleFree(spAnimIdle);
    }
    spAnimIdle = static_cast<AnimIdle*>(nullptr);
    if (spAnimIdleQueued) {
        AnimIdleFree(spAnimIdleQueued);
    }
    spAnimIdleQueued = static_cast<AnimIdle*>(nullptr);
    pCurrentSequence[7] = nullptr;
    ClearGroup(AGS_SPECIAL_IDLE, 0.0);
    if (spManager.m_pObject) {
        NiControllerManager* v2 = spManager.m_pObject;
        v2->DeactivateAll(0.0);
    }
    NiAVObject* SceneRoot = GetSceneRoot();
    Animation::ClearControllersInterpolators(SceneRoot);
    ClearBlendInterps();
    ReloadTargets(true);
}

void Animation::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    cSkipNextBlend = 0;
    ClearGroup(AGS_WHOLE_BODY, 0.0f);
    ZeroGlobalTransform();
    if (spAnimIdle.m_pObject) {
        AnimIdleFree(spAnimIdle);
    }
    spAnimIdle = static_cast<AnimIdle*>(nullptr);
    if (spAnimIdleQueued.m_pObject) {
        AnimIdleFree(spAnimIdleQueued);
    }
    spAnimIdleQueued = static_cast<AnimIdle*>(nullptr);
    pCurrentSequence[7] = nullptr;
    ClearGroup(AGS_SPECIAL_IDLE, 0.0f);
    if (spManager.m_pObject) {
        spManager.m_pObject->DeactivateAll(0.0f);
    }
    for (IDLE_REPLAY_DELAY*& rpItem : replayDelayList) {
        ourFree(rpItem);
    }
    replayDelayList.RemoveAll();
}

void Animation::SaveGameBGS(BGSSaveGameBuffer* apSaveGameBuffer) {
    BSAnimGroupSequence* pIdleSequence;
    NiPointer<AnimIdle>* p_spAnimIdleQueued;
    char cSequenceIndex;
    // GetAnimAction's result is stored as a 16-bit value and those two bytes are saved.
    int16_t sAnimAction;
    char cIdleSequenceSection;
    char apData;
    char cActionSequenceSection;
    Actor* pActor = apSaveGameBuffer->GetActor();
    apSaveGameBuffer->SaveDataEndian(&m_fMoveSpeed, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&m_fAttackSpeed, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&movementDelta, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&m_fLooking, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&cSkipUpdate, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&fLipTime, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&sQueuedReloadGroup, 2u, 0);
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<IDLE_REPLAY_DELAY*>* pNode = &replayDelayList; pNode; pNode = pNode->GetNext()) {
        IDLE_REPLAY_DELAY* pDelay = pNode->GetItem();
        if (pDelay) {
            apSaveGameBuffer->SaveFormID(pDelay->pIdleForm, 0);
            apSaveGameBuffer->SaveDataEndian(&pDelay->fDelay, 4u, 0);
            ++aiVariableValue;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
    if (spAnimIdleQueued.m_pObject) {
        p_spAnimIdleQueued = &spAnimIdleQueued;
    }
    else {
        p_spAnimIdleQueued = &spAnimIdle;
    }
    AnimIdle* pAnimIdle = p_spAnimIdleQueued->m_pObject;
    // The idle form is a trivial accessor of pIdleForm.
    if (pAnimIdle) {
        TESForm* apForm = pAnimIdle->pIdleForm;
        apSaveGameBuffer->SaveFormID(apForm, 0);
    }
    else {
        apSaveGameBuffer->SaveFormID(static_cast<TESForm*>(nullptr), 0);
    }
    if ((pAnimIdle) && (pAnimIdle->pIdleForm)) {
        uint32_t started = apSaveGameBuffer->StartVariableSizedValue();
        int32_t CastingType = apSaveGameBuffer->iBufferPosition;
        pAnimIdle->SaveGame(static_cast<BGSSaveFormBuffer*>(apSaveGameBuffer), this);
        int32_t v12 = apSaveGameBuffer->iBufferPosition;
        apSaveGameBuffer->SaveVariableSizedValue(v12 - CastingType, started);
    }

    BSAnimGroupSequence* Sequence = nullptr;
    if ((spAnimIdleQueued.m_pObject) && (spAnimIdle.m_pObject)) {
        Sequence = spAnimIdle.m_pObject->GetSequence();
    }

    apData = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        if ((!Sequence || pCurrentSequence[i] != Sequence) && group[i] != 255 && group[i] != 0xFFFF) {
            apData |= 1 << i;
        }
    }
    apSaveGameBuffer->SaveDataEndian(&apData, 1u, 0);
    cActionSequenceSection = -1;
    BaseProcess* CurrentAIProcess = pActor->pCurrentProcess;
    sAnimAction = static_cast<int16_t>(CurrentAIProcess->GetAnimAction());
    BaseProcess* v3 = pActor->pCurrentProcess;
    BSAnimGroupSequence* v27 = v3->GetAnimActionAnimSeq();
    cIdleSequenceSection = -1;
    if (pAnimIdle) {
        pIdleSequence = pAnimIdle->GetSequence();
    }
    else {
        pIdleSequence = nullptr;
    }
    for (uint32_t j = 0; j < 8; ++j) {
        if ((!Sequence || pCurrentSequence[j] != Sequence) && group[j] != 255 && group[j] != 0xFFFF) {
            apSaveGameBuffer->SaveDataEndian(&group[j], 2u, 0);
            apSaveGameBuffer->SaveDataEndian(&action[j], 4u, 0);
            apSaveGameBuffer->SaveDataEndian(&nextGroup[j], 2u, 0);
            apSaveGameBuffer->SaveDataEndian(&loopCount[j], 4u, 0);
            apSaveGameBuffer->SaveDataEndian(&nextLoops[j], 4u, 0);
            AnimSequenceBase* val = nullptr;
            cSequenceIndex = -1;
            if (pAnimSequenceMap->GetAt(group[j], val)) {
                cSequenceIndex = val->GetSequenceIndex(pCurrentSequence[j]);
            }
            if (!pCurrentSequence[j]) {
                cSequenceIndex = -2;
            }
            apSaveGameBuffer->SaveDataEndian(&cSequenceIndex, 1u, 0);
            if (pCurrentSequence[j]) {
                pCurrentSequence[j]->SaveGame(apSaveGameBuffer, time);
            }
            if (pCurrentSequence[j] == v27) {
                cActionSequenceSection = j;
            }
            if (pCurrentSequence[j] == pIdleSequence) {
                cIdleSequenceSection = j;
            }
        }
    }
    apSaveGameBuffer->SaveDataEndian(&cSkipNextBlend, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&sAnimAction, 2u, 0);
    apSaveGameBuffer->SaveDataEndian(&cActionSequenceSection, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cIdleSequenceSection, 1u, 0);
}

void Animation::SpecialIdleQueue(TESIdleForm* apIdleForm, ANIM_GROUP_SECTION aeSection) {
    AnimIdle* v4;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 0x125D
    );
    if (spAnimIdle.m_pObject) {
        ANIM_GROUP_SECTION esection = spAnimIdle.m_pObject->eSection;
        if (esection == AGS_WHOLE_BODY) {
            esection = AGS_MOVEMENT;
        }
        else {
            if (esection == AGS_UPPER_BODY) {
                esection = AGS_WEAPON;
            }
        }
        if (spAnimIdle.m_pObject->GetSequence() && pCurrentSequence[esection] == spAnimIdle.m_pObject->GetSequence()) {
            int32_t State = spAnimIdle.m_pObject->GetSequence()->GetState();
            if (State == 2 || State == 5) {
                Warning(
                    "ANIMATION: Trying to SpecialIdleQueue an animation '%s' that just started.",
                    (&spAnimIdle.m_pObject->GetSequence()->m_kName)->GetHandleString()
                );
                return;
            }
            BlendOut(spAnimIdle.m_pObject->eSection, false);
        }
        if (spAnimIdleFreeWhenInactiveA->m_pObject) {
            if (spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                AnimIdleFree(spAnimIdle);
            }
            else {
                spAnimIdleFreeWhenInactiveA[1] = spAnimIdle;
                spAnimIdle = static_cast<AnimIdle*>(nullptr);
            }
        }
        else {
            *spAnimIdleFreeWhenInactiveA = spAnimIdle;
            spAnimIdle = static_cast<AnimIdle*>(nullptr);
        }
    }
    v4 = new AnimIdle(apIdleForm, aeSection, AnimIdle::PLAY_TYPE_QUEUE, static_cast<Actor*>(pActorRef), false, nullptr);
    spAnimIdle = v4;
}

uint16_t Animation::GetAnimationSaveSize(TESObjectREFR* apRef) {
    int16_t v19;
    int16_t v17 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v17 = 6;
    }
    int16_t v18 = v17 + 26;
    for (char i = 0; i < 8; ++i) {
        if (group[i] != 255 && group[i] != 0xFFFF) {
            v18 += 17;
            if (pCurrentSequence[i]) {
                v18 += pCurrentSequence[i]->GetSaveSize();
            }
        }
    }
    Actor* apActor = nullptr;
    if ((apRef) && (apRef->IsActor())) {
        apActor = static_cast<Actor*>(apRef);
    }

    if (spAnimIdleQueued.m_pObject) {
        v19 = AnimIdle::GetAnimIdleSaveSize(apActor, spAnimIdleQueued.m_pObject) + v18;
    }
    else {
        v19 = AnimIdle::GetAnimIdleSaveSize(apActor, spAnimIdle.m_pObject) + v18;
    }
    uint16_t isize = v19 + 1;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        ++isize;
    }
    if (apActor->GetCurrentSpell()) {
        NiControllerManager* ControllerManager = GetControllerManager();
        NiAVObjectPalette* ObjectPalette = ControllerManager->GetObjectPalette();
        NiFixedString* v3 = &FixedStrings::QMagicNode();
        NiNode* v14 = static_cast<NiNode*>(ObjectPalette->GetAVObject(*v3));
        if (v14) {
            NiNode* v13 = v14->IsNode();
            if (v13) {
                NiAVObject* v12 = v13->GetAtChecked(0);
                if (v12) {
                    NiControllerManager* v11 = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, v12->GetControllers())
                    );
                    if (v11) {
                        NiFixedString* g_SpecialIdle_Cast = get_g_SpecialIdle_Cast();
                        BSAnimGroupSequence* SequenceByName =
                            static_cast<BSAnimGroupSequence*>(v11->GetSequenceByName(*g_SpecialIdle_Cast));
                        if (SequenceByName) {
                            isize += SequenceByName->GetSaveSize() + 4;
                        }
                    }
                }
            }
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* CurrentlySavingFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (CurrentlySavingFormHeader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlySavingFormHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                CurrentlySavingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                CurrentlySavingFormHeader->iFlags, 0x15B8,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 0x15B8,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
    }
    return isize;
}

void Animation::ShutdownAllAnimIdles() {
    bShutDown = true;
    SpecialIdleFree(true, true);
    for (NiPointer<AnimIdle>& spAnimIdle : spAnimIdleFreeWhenInactiveA) {
        if (spAnimIdle) {
            AnimIdleFree(spAnimIdle);
            spAnimIdle = static_cast<AnimIdle*>(nullptr);
        }
    }
}

bool Animation::ShouldBeMoving() {
    if (!pCurrentSequence[1]) {
        return false;
    }
    if (!pSoundPriorityBone[1]) {
        return false;
    }
    const char* plowerBodyBone = (&pSoundPriorityBone[1]->m_kName)->GetHandleString();
    ANIM_GROUP_ENUM eType = TESAnimGroup::GetType(group[1]);
    if (eType < ANIM_GROUP_BEGIN_MOVEMENT) {
        return false;
    }
    if (eType > ANIM_GROUP_DODGE_RIGHT) {
        return false;
    }
    BSAnimGroupSequence* phighestSeq = nullptr;
    uint8_t cpriority = 0;
    for (ANIM_GROUP_SECTION etestSection = AGS_IDLE; etestSection < ANIM_GROUP_SECTION_COUNT;
         etestSection = static_cast<ANIM_GROUP_SECTION>(etestSection + 1)) {
        if (pCurrentSequence[etestSection]) {
            uint8_t ctestPriority = pCurrentSequence[etestSection]->GetPriority(plowerBodyBone, nullptr);
            if (ctestPriority <= static_cast<int32_t>(cpriority)) {
                if (ctestPriority == cpriority && etestSection == AGS_MOVEMENT) {
                    phighestSeq = pCurrentSequence[1u];
                    cpriority = ctestPriority;
                }
            }
            else {
                phighestSeq = pCurrentSequence[etestSection];
                cpriority = ctestPriority;
            }
        }
    }
    return phighestSeq == pCurrentSequence[1];
}

char Animation::FindNextNoteKey(ANIM_GROUP_SECTION aeSection, const char* apSearchKey, float* apKeyTime) {
    const char* pkeyText;
    uint32_t i;
    uint32_t inumKeys;
    if (apKeyTime) {
        *apKeyTime = 0.0f;
    }
    if (!pCurrentSequence[aeSection] || !apSearchKey) {
        return 0;
    }
    float fcurrentTime = pCurrentSequence[aeSection]->m_fLastScaledTime;
    NiTextKeyExtraData* ptext = pCurrentSequence[aeSection]->GetTextKeys();
    NiTextKey* pkeys = ptext->GetKeys(inumKeys);
    uint32_t isearchKeyLength = BSstrlen(apSearchKey);
    if (!isearchKeyLength) {
        return 0;
    }
    for (i = 0;; ++i) {
        if (i >= inumKeys) {
            return 0;
        }
        NiFixedString* ActorPackageThatIsRunning = &pkeys[i].m_kText;
        pkeyText = ActorPackageThatIsRunning->GetHandleString();
        if ((pkeyText) && (!_strnicmp(pkeyText, apSearchKey, isearchKeyLength))) {
            double DecalMinWidth = pkeys[i].m_fTime;
            if (fcurrentTime <= DecalMinWidth) {
                break;
            }
        }
    }
    if (apKeyTime) {
        *apKeyTime = pkeys[i].m_fTime;
    }
    return std::tolower(pkeyText[isearchKeyLength]);
}

bool Animation::SpecialIdlePlay(Actor* apActor) {
    if (!SpecialIdlePlay()) {
        return false;
    }
    AnimIdle* v2 = spAnimIdle;
    v2->pActor = apActor;
    AnimIdle* v3 = spAnimIdle;
    BSAnimGroupSequence* Sequence = v3->GetSequence();
    apActor->SetAnimAction(ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE, Sequence);
    return true;
}

void Animation::AnimIdleFree(NiPointer<AnimIdle>& aspAnimIdle) {
    KFModel* pkfModel = aspAnimIdle.m_pObject->GetKFModel();
    AnimSequenceBase* panimSeq = nullptr;
    uint16_t sanimGroup = 255;
    NiPointer<BSAnimGroupSequence> spfadeSeq(static_cast<BSAnimGroupSequence*>(nullptr));
    if ((pkfModel) && (pkfModel->GetAnimGroup())) {
        BSAnimGroupSequence* v2;
        sanimGroup = pkfModel->GetAnimGroup()->sType;
        pAnimSequenceMap->GetAt(sanimGroup, panimSeq);
        if (!panimSeq || (v2 = panimSeq->GetSequenceToPlay(-1), v2 == aspAnimIdle.m_pObject->GetSequence())) {
            // An idle of AGS_WHOLE_BODY (20) or AGS_UPPER_BODY (21) indexes pCurrentSequence past its 8 entries,
            // deliberately reading what follows the array, as the game does: spAnimIdleFreeWhenInactiveA[1] (20)
            // and replayDelayList's first item (21). Those are read by name; any other section past the array
            // matches nothing.
            uint32_t Time = aspAnimIdle.m_pObject->eSection;
            const void* pCurrent;
            bool bInExeObject = true;
            if (Time < ANIM_GROUP_SECTION_COUNT) {
                pCurrent = pCurrentSequence[Time];
            }
            else if (Time == AGS_WHOLE_BODY) {
                pCurrent = spAnimIdleFreeWhenInactiveA[1].m_pObject;
            }
            else if (Time == AGS_UPPER_BODY) {
                pCurrent = replayDelayList.GetItem();
            }
            else {
                pCurrent = nullptr;
                bInExeObject = false;
            }
            if (bInExeObject && pCurrent == aspAnimIdle.m_pObject->GetSequence()) {
                ANIM_GROUP_SECTION v4 = aspAnimIdle.m_pObject->eSection;
                ClearGroup(v4, 0.0f);
            }
        }
        else {
            panimSeq = nullptr;
        }
    }

    if (!panimSeq) {
        spfadeSeq = aspAnimIdle.m_pObject->GetSequence();
    }
    if (!bShutDown) {
        if (panimSeq) {
            if (panimSeq->GetSequenceToPlay(-1) == pLastMovementSequence) {
                pLastMovementSequence = nullptr;
            }
            BSAnimGroupSequence* v5 = panimSeq->GetSequenceToPlay(-1);
            spManager.m_pObject->RemoveSequence(v5);
        }
        else {
            if (spfadeSeq.m_pObject) {
                if (spfadeSeq == pLastMovementSequence) {
                    pLastMovementSequence = nullptr;
                }
                spManager.m_pObject->RemoveSequence(spfadeSeq.m_pObject);
            }
        }
    }
    AnimIdle::FreeAnimIdleData(aspAnimIdle.m_pObject);
    aspAnimIdle = static_cast<AnimIdle*>(nullptr);
    if (panimSeq) {
        pAnimSequenceMap->RemoveAt(sanimGroup);
        if (panimSeq) {
            delete panimSeq;
        }
        panimSeq = nullptr;
    }
    if (!spfadeSeq.m_pObject && !panimSeq) {
        pAnimSequenceMap->RemoveAt(sanimGroup);
    }
}

// With save blocks, writes the KBLK block ID and a 2-byte size placeholder, then patches the placeholder with the
// block's size at the end. The debug log goes through an empty printf stub.
void Animation::SaveGameTES(TESForm* apForm) {
    AnimIdle* pIdle;
    float fTime;
    float fFadeTimer;
    char cSequenceIndex;
    uint32_t iBlockID;
    uint16_t sBlockSize = 0;
    char cSlotMask;
    char cHasCastSequence;
    char i;
    char* pBlockStart = nullptr;
    uint8_t* pStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iBlockID = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, 4u);
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, 2u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fMoveSpeed, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fAttackSpeed, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&movementDelta, 12u);
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fLooking, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&cSkipUpdate, 1u);
    cSlotMask = 0;
    for (i = 0; i < 8; ++i) {
        if (group[i] != 255 && group[i] != 0xFFFF) {
            cSlotMask |= 1 << i;
        }
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cSlotMask, 1u);
    for (i = 0; i < 8; ++i) {
        if (group[i] != 255 && group[i] != 0xFFFF) {
            AnimSequenceBase* val;
            pSaveLoadGameOLD->SaveGameDataOLD(&group[i], 2u);
            pSaveLoadGameOLD->SaveGameDataOLD(&action[i], 4u);
            pSaveLoadGameOLD->SaveGameDataOLD(&loopCount[i], 4u);
            pSaveLoadGameOLD->SaveGameDataOLD(&nextGroup[i], 2u);
            pSaveLoadGameOLD->SaveGameDataOLD(&nextLoops[i], 4u);
            cSequenceIndex = -1;
            if (pAnimSequenceMap->GetAt(group[i], val)) {
                cSequenceIndex = val->GetSequenceIndex(pCurrentSequence[i]);
            }
            if (!pCurrentSequence[i]) {
                ANIM_GROUP_ACTION uiActiona = action[i];
                int32_t v11 = group[i];
                int32_t v8 = i;
                int32_t CastingType = apForm->iFormID;
                Warning(
                    "SAVELOAD: %s %08X has a sequence in slot %i with group %i and action %i, but the sequence is "
                    "NULL.",
                    apForm->GetFormEditorID(), CastingType, v8, v11, uiActiona
                );
                cSequenceIndex = -2;
            }
            pSaveLoadGameOLD->SaveGameDataOLD(&cSequenceIndex, 1u);
            if (pCurrentSequence[i]) {
                pCurrentSequence[i]->SaveGame(time);
            }
        }
    }
    Actor* pActor = nullptr;
    if ((apForm) && (apForm->IsActor())) {
        pActor = static_cast<Actor*>(apForm);
    }

    if (spAnimIdleQueued.m_pObject) {
        fTime = time;
        pIdle = spAnimIdleQueued.m_pObject;
    }
    else {
        fTime = time;
        pIdle = spAnimIdle.m_pObject;
    }
    AnimIdle::SaveAnimIdle(pActor, pIdle, this, fTime);
    pSaveLoadGameOLD->SaveGameDataOLD(&cSkipNextBlend, 1u);
    cHasCastSequence = 0;
    // pActor is not tested for null before GetCurrentSpell is called, as the game does.
    if (pActor->GetCurrentSpell()) {
        NiControllerManager* ControllerManager = GetControllerManager();
        NiAVObjectPalette* ObjectPalette = ControllerManager->GetObjectPalette();
        NiFixedString* v3 = &FixedStrings::QMagicNode();
        NiAVObject* v28 = ObjectPalette->GetAVObject(*v3);
        if (v28) {
            NiNode* v27 = v28->IsNode();
            if (v27) {
                NiObjectNET* v26 = (NiObjectNET*)v27->GetAtChecked(0);
                if (v26) {
                    NiObject* pkObject = (NiObject*)v26->GetControllers();
                    NiControllerManager* v24 = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pkObject)
                    );
                    if (v24) {
                        NiFixedString* g_SpecialIdle_Cast = get_g_SpecialIdle_Cast();
                        BSAnimGroupSequence* SequenceByName =
                            static_cast<BSAnimGroupSequence*>(v24->GetSequenceByName(*g_SpecialIdle_Cast));
                        if (SequenceByName) {
                            cHasCastSequence = 1;
                            if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
                                pSaveLoadGameOLD->SaveGameDataOLD(&cHasCastSequence, 1u);
                            }
                            SequenceByName->SaveGame(time);
                            MagicLight* pLight = pActor->pLight;
                            fFadeTimer = 0.0f;
                            if (pLight) {
                                fFadeTimer = pLight->fFadeTimer;
                            }
                            pSaveLoadGameOLD->SaveGameDataOLD(&fFadeTimer, 4u);
                        }
                    }
                }
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u && !cHasCastSequence) {
        pSaveLoadGameOLD->SaveGameDataOLD(&cHasCastSequence, 1u);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* CurrentlySavingFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (CurrentlySavingFormHeader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlySavingFormHeader->iFormID);
            int32_t iFlags = CurrentlySavingFormHeader->iFlags;
            const char* v5 = FormByNumericID->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart), CurrentlySavingFormHeader->iFormID, v5, iFlags, 5672,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart), 5672,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        if (pEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5672
            );
        }
        // The save buffer is raw bytes: the 2-byte size placeholder is patched in place.
        *reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockStart);
    }
}

// A save-game block: the KBLK block ID and its 2-byte size, checked against the bytes read at the end.
void Animation::LoadGameTES(TESObjectREFR* apref) {
    float fFadeTimer;
    char cSequenceIndex;
    uint32_t iBlockID;
    uint16_t sBlockSize = 0;
    char cSlotMask;
    char cHasCastSequence;
    uint8_t* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockID, 4u);
        if (iBlockID != 0x424C4F4B) {
            LoadFormHeader* CurrentlyLoadingFormHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (CurrentlyLoadingFormHeader) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlyLoadingFormHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5680,
                    CurrentlyLoadingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                    CurrentlyLoadingFormHeader->cVersion, CurrentlyLoadingFormHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5680,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, 2u);
    }
    Actor* apActor = nullptr;
    if ((apref) && (apref->IsActor())) {
        apActor = static_cast<Actor*>(apref);
    }

    pSaveLoadGameOLD->LoadGameDataOLD(&m_fMoveSpeed, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fAttackSpeed, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&movementDelta, 12u);
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fLooking, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&cSkipUpdate, 1u);
    pSaveLoadGameOLD->LoadGameDataOLD(&cSlotMask, 1u);
    for (char i = 0; i < 8; ++i) {
        if ((1 << i & cSlotMask) != 0) {
            pSaveLoadGameOLD->LoadGameDataOLD(&group[i], 2u);
            pSaveLoadGameOLD->LoadGameDataOLD(&action[i], 4u);
            pSaveLoadGameOLD->LoadGameDataOLD(&loopCount[i], 4u);
            pSaveLoadGameOLD->LoadGameDataOLD(&nextGroup[i], 2u);
            pSaveLoadGameOLD->LoadGameDataOLD(&nextLoops[i], 4u);
            pSaveLoadGameOLD->LoadGameDataOLD(&cSequenceIndex, 1u);
            pCurrentSequence[i] = nullptr;
            if (cSequenceIndex != -2) {
                SkipNextBlend();
                ForceSection(static_cast<ANIM_GROUP_SECTION>(i), group[i], action[i], time, cSequenceIndex);
                if (pCurrentSequence[i]) {
                    pCurrentSequence[i]->LoadGame(time);
                }
                else {
                    uint16_t SequenceSaveSize = BSAnimGroupSequence::GetSequenceSaveSize();
                    pSaveLoadGameOLD->AdvanceBuffer(SequenceSaveSize);
                    ClearGroup(static_cast<ANIM_GROUP_SECTION>(i), 0.0f);
                }
            }
        }
    }
    spAnimIdle = AnimIdle::LoadAnimIdle(apActor, this, time);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 76u) {
        spAnimIdleQueued = AnimIdle::LoadAnimIdle(apActor, this, time);
    }
    if ((spAnimIdle.m_pObject) && (spAnimIdle.m_pObject->eFlags == AnimIdle::ANIM_IDLE_LOADED)) {
        pCurrentSequence[7] = nullptr;
    }

    pSaveLoadGameOLD->LoadGameDataOLD(&cSkipNextBlend, 1u);
    cHasCastSequence = 1;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&cHasCastSequence, 1u);
    }
    if (cHasCastSequence) {
        char bLoaded = 0;
        if (apActor->GetCurrentSpell()) {
            NiControllerManager* ControllerManager = GetControllerManager();
            NiAVObjectPalette* ObjectPalette = ControllerManager->GetObjectPalette();
            NiFixedString* v4 = &FixedStrings::QMagicNode();
            NiAVObject* v29 = ObjectPalette->GetAVObject(*v4);
            if (v29) {
                NiNode* v28 = v29->IsNode();
                if (v28) {
                    NiObjectNET* v27 = (NiObjectNET*)v28->GetAtChecked(0);
                    if (v27) {
                        NiObject* pkObject = (NiObject*)v27->GetControllers();
                        NiControllerManager* v25 = static_cast<NiControllerManager*>(
                            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pkObject)
                        );
                        if (v25) {
                            NiFixedString* g_SpecialIdle_Cast = get_g_SpecialIdle_Cast();
                            NiControllerSequence* a1 = v25->GetSequenceByName(*g_SpecialIdle_Cast);
                            if (a1) {
                                v25->DeactivateAll(0.0f);
                                v25->ActivateSequence(a1, 0, false, 1.0f, 0.0f, nullptr);
                                v25->SetActive(true);
                                static_cast<BSAnimGroupSequence*>(a1)->BSAnimGroupSequence::LoadGame(time);
                                pSaveLoadGameOLD->LoadGameDataOLD(&fFadeTimer, 4u);
                                MagicLight* pLight = apActor->pLight;
                                if (pLight) {
                                    float v17 = a1->m_fEndKeyTime * 0.75;
                                    pLight->Fade(true, v17);
                                    pLight->fFadeTimer = fFadeTimer;
                                }
                                bLoaded = 1;
                            }
                        }
                    }
                }
            }
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u && !bLoaded) {
            uint16_t v6 = BSAnimGroupSequence::GetSequenceSaveSize();
            pSaveLoadGameOLD->AdvanceBuffer(v6 + 4);
        }
    }
    UpdateBipOnly(time, &AccumRootTranslate, true);
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pBlockEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pBlockEnd > &pBlockStart[sBlockSize]) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - &pBlockStart[sBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5804, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
            else if (pBlockEnd < &pBlockStart[sBlockSize]) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(&pBlockStart[sBlockSize] - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5804, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
        }
        else if (pBlockEnd > &pBlockStart[sBlockSize]) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - &pBlockStart[sBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5804, cVersion
            );
        }
        else if (pBlockEnd < &pBlockStart[sBlockSize]) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(&pBlockStart[sBlockSize] - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp", 5804, cVersion
            );
        }
    }
}

void Animation::ClearControllersInterpolators(NiAVObject* apObj) {
    NiAVObject* apObja;
    uint32_t a1a;
    NiMultiTargetTransformController* v3;
    NiSingleInterpController* v4;
    NiNode* v5;
    NiTimeController* i;
    if (apObj) {
        for (i = apObj->NiObjectNET::GetControllers(); i; i = i->GetNext()) {
            v4 = static_cast<NiSingleInterpController*>(
                NiObject::DynamicCastStatic(&NiSingleInterpController::ms_RTTI, i)
            );
            if (v4) {
                v4->SetInterpolator(nullptr, (uint16_t)0);
            }
            else {
                v3 = static_cast<NiMultiTargetTransformController*>(
                    NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, i)
                );
                if (v3) {
                    v3->ReallocateStorage();
                }
            }
        }
        v5 = apObj->IsNode();
        if (v5) {
            for (a1a = 0; a1a < v5->GetArrayCount(); ++a1a) {
                apObja = v5->GetAt(a1a);
                Animation::ClearControllersInterpolators(apObja);
            }
        }
    }
}

void Animation::ResetCloneCount() {
    Animation::iAnimationsCloned = 0;
}

bool Animation::FindSkinnedNode(NiAVObject* apAVObject) {
    NiAVObject* v2;
    uint32_t a1;
    uint8_t skinnedNode = 0;
    if (!apAVObject) {
        return false;
    }
    NiGeometry* v5 = apAVObject->IsGeometry();
    if (v5 && v5->GetSkinInstance()) {
        return true;
    }
    NiNode* v4 = apAVObject->IsNode();
    if (v4) {
        for (a1 = 0; a1 < v4->GetArrayCount(); ++a1) {
            v2 = v4->GetAt(a1);
            if (v2) {
                skinnedNode = (uint8_t)Animation::FindSkinnedNode(v2);
                if (skinnedNode) {
                    return (bool)skinnedNode;
                }
            }
        }
    }
    return (bool)skinnedNode;
}

float Animation::GetDuration(NiControllerSequence* apSeq) {
    if (!apSeq) {
        return 0.0f;
    }
    float fStartTime = apSeq->GetBeginKeyTime();
    float fEndTime = apSeq->GetEndKeyTime();
    // The subtraction is done in double precision and then stored to a float, as the game does.
    return static_cast<float>(static_cast<double>(fEndTime) - fStartTime);
}

// Reads an old-format save's 2-byte animation record size and, when it is not 0, queues apRef's animation load. Only
// apRef is read; apAnimation is unused.
void Animation::LoadAnimation(TESObjectREFR* apRef, Animation* apAnimation) {
    (void)apAnimation;
    uint16_t usSize;
    pSaveLoadGameOLD->LoadGameDataOLD(&usSize, 2);
    if (usSize) {
        pSaveLoadGameOLD->QueueAnimationLoad(apRef, usSize);
    }
}

void Animation::SetLipTime(float afPercent, NiControllerManager* apManager, NiControllerSequence* apSeq) {
    if (!apManager || !apSeq) {
        return;
    }
    if (afPercent == 0.0) {
        if (apSeq->GetState() == NiControllerSequence::kAnimating) {
            apSeq->SetPhase(0.0f, true);
            apSeq->Update(0.0f, true);
        }
        return;
    }
    if (apSeq->GetState() == NiControllerSequence::kInactive) {
        apManager->SetActive(true);
        apManager->ActivateSequence(apSeq, 0, false, 1.0f, 0.0f, nullptr);
        double dTime = appTimer.GetTime() / 1000.0;
        float fTime = apSeq->GetBeginKeyTime() + dTime;
        apSeq->Update(fTime, true);
    }
    if (apSeq->GetState() == NiControllerSequence::kAnimating) {
        double dEndTime = apSeq->GetEndKeyTime();
        float fDuration = dEndTime - apSeq->GetBeginKeyTime();
        // The multiply and add are done in double precision before the result is stored as a float, as the game does.
        float fPhase = static_cast<float>(apSeq->GetBeginKeyTime() + static_cast<double>(fDuration) * afPercent);
        float fTime = fPhase - appTimer.GetTime() / 1000.0;
        apSeq->SetPhase(fTime, true);
    }
}

void Animation::SaveAnimation(TESObjectREFR* apRef, Animation* apAnimation) {
    uint16_t isize;
    isize = 0;
    if ((apAnimation) && (!apRef->IsDead(false))) {
        isize = apAnimation->GetAnimationSaveSize(apRef);
    }

    pSaveLoadGameOLD->SaveGameDataOLD(&isize, 2u);
    if ((apAnimation) && (!apRef->IsDead(false))) {
        apAnimation->SaveGameTES(apRef);
    }
}

NiFixedString* get_g_SpecialIdle_Cast() {
    return FixedStrings::SpecialIdle_Cast;
}

int16_t ComputeAnimationSaveSize(PlayerCharacter* a1, Animation* a2) {
    uint16_t isize = 2;
    if ((a2) && (!a1->IsDead(false))) {
        return a2->GetAnimationSaveSize(a1) + 2;
    }

    return isize;
}

bool bhkRagdollController::QFootIKActive() {
    return bActiveFootIK;
}

bool bhkRagdollController::QFootIK() {
    return bhkRagdollController::bFootIK.Bool();
}

AnimSequenceBase::AnimSequenceBase() {}

AnimSequenceBase::~AnimSequenceBase() {}

AnimSequenceMultiple::AnimSequenceMultiple(AnimSequenceSingle* apSingleSeq)
  : pSeqList(new NiTPointerList<BSAnimGroupSequence*>) {
    AnimSequenceBase* pSingleSeq = apSingleSeq;
    AnimSequenceMultiple::SetSequence(pSingleSeq->GetSequenceToPlay(-1), true);
    pSingleSeq->SetSequence(nullptr, false);
    delete pSingleSeq;
}

AnimSequenceMultiple::~AnimSequenceMultiple() {
    if (pSeqList) {
        for (NiTListItem<BSAnimGroupSequence*>* pPos = pSeqList->m_pkHead; pPos; pPos = pSeqList->GetNextPos(pPos)) {
            // The name is copied first: releasing the sequence may free it.
            char acName[264];
            BSAnimGroupSequence* pSeq = pSeqList->Get(pPos);
            BSstrcpy(acName, 260, pSeq->m_kName.GetHandleString());
            pSeq->DecRefCount();
            pModelLoader->ReleaseKFFile(acName);
        }
        delete pSeqList;
    }
}

// Replaces the list with apSequence, releasing what it held.
void AnimSequenceMultiple::SetSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) {
    apSeq->IncRefCount();
    pSeqList->AddTail(apSeq);
}

bool AnimSequenceMultiple::RemoveSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) {
    apSeq->DecRefCount();
    pSeqList->Remove(apSeq);
    if (!pSeqList->IsEmpty()) {
        return false;
    }
    if (pSeqList) {
        delete pSeqList;
    }
    pSeqList = nullptr;
    return true;
}

bool AnimSequenceMultiple::IsSingleSeq() {
    return false;
}

BSAnimGroupSequence* AnimSequenceMultiple::GetSequenceToPlay(char aiSequenceIndex) {
    uint32_t v8;
    if (aiSequenceIndex == -1 || aiSequenceIndex >= pSeqList->m_uiCount) {
        uint32_t v2 = Random();
        v8 = v2 % pSeqList->m_uiCount;
    }
    else {
        v8 = aiSequenceIndex;
    }
    for (NiTListItem<BSAnimGroupSequence*>* a1 = pSeqList->m_pkHead; a1; a1 = pSeqList->GetNextPos(a1)) {
        BSAnimGroupSequence* v6 = pSeqList->Get(a1);
        if (!v8--) {
            return v6;
        }
    }
    return nullptr;
}

BSAnimGroupSequence* AnimSequenceMultiple::GetSequence(TESAnimGroup* apAnimGroup) {
    for (NiTListItem<BSAnimGroupSequence*>* pos = pSeqList->GetHeadPos(); pos; pos = pSeqList->GetNextPos(pos)) {
        BSAnimGroupSequence* pSeq = pSeqList->Get(pos);
        if (pSeq->GetAnimGroup() == apAnimGroup) {
            return pSeq;
        }
    }
    return nullptr;
}

char AnimSequenceMultiple::GetSequenceIndex(BSAnimGroupSequence* apSeq) {
    int8_t cIndex = 0;
    for (NiTListItem<BSAnimGroupSequence*>* pos = pSeqList->GetHeadPos(); pos; pos = pSeqList->GetNextPos(pos)) {
        if (apSeq == pSeqList->Get(pos)) {
            return cIndex;
        }
        ++cIndex;
    }
    return -1;
}

// strrchr runs on the sequence's const name; the non-const strchr is then called on the result, so it is held as a
// char*.
BSAnimGroupSequence*
AnimSequenceMultiple::GetCorrespondingSequence(BSAnimGroupSequence* apSeq, ANIM_GROUP_SECTION aeSection) {
    char Destination[264];
    char* ptmp;
    void* listIt = (void*)pSeqList->m_pkHead;
    const char* HandleString = (&apSeq->m_kName)->GetHandleString();
    char* p3rdLastSlash = const_cast<char*>(std::strrchr(HandleString, 92));
    if (!p3rdLastSlash) {
        return nullptr;
    }
    if (aeSection == AGS_WEAPON_UP) {
        ourStrcpy(Destination, p3rdLastSlash);
        ptmp = std::strchr(Destination, 95);
        if (ptmp) {
            ourStrcpy(ptmp, "Up");
            ptmp = std::strchr(p3rdLastSlash, 95);
            std::strcat(Destination, ptmp);
        }
        else {
            std::strcat(Destination, "Up");
        }
        p3rdLastSlash = Destination;
    }
    else {
        if (aeSection == AGS_WEAPON_DOWN) {
            ourStrcpy(Destination, p3rdLastSlash);
            ptmp = std::strchr(Destination, 95);
            if (ptmp) {
                ourStrcpy(ptmp, "Down");
                ptmp = std::strchr(p3rdLastSlash, 95);
                std::strcat(Destination, ptmp);
            }
            else {
                std::strcat(Destination, "Down");
            }
            p3rdLastSlash = Destination;
        }
    }
    while (listIt) {
        BSAnimGroupSequence* pseq = pSeqList->Get(static_cast<NiTListItem<BSAnimGroupSequence*>*>(listIt));
        const char* v5 = (&pseq->m_kName)->GetHandleString();
        const char* p1stLastSlash = std::strrchr(v5, 92);
        if (p1stLastSlash && !BSstricmp(p1stLastSlash, p3rdLastSlash)) {
            return pseq;
        }
        listIt = (void*)pSeqList->GetNextPos(static_cast<NiTListItem<BSAnimGroupSequence*>*>(listIt));
    }
    return nullptr;
}

AnimSequenceSingle::AnimSequenceSingle() : pSeq(nullptr) {}

AnimSequenceSingle::~AnimSequenceSingle() {
    if (pSeq) {
        // The name is copied first: releasing the sequence may free it.
        char acName[268];
        BSstrcpy(acName, 0x104u, pSeq->GetName().GetHandleString());
        // Released through the NiPointer, which nulls it: its own destructor then releases nothing.
        pSeq = nullptr;
        pModelLoader->ReleaseKFFile(acName);
    }
}

// Reads pSeq again for each use and releases the old sequence before it adds a reference to the new one.
void AnimSequenceSingle::SetSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) {
    if (pSeq) {
        if (abReleaseModelLoader) {
            pModelLoader->ReleaseKFFile(pSeq->GetName().GetHandleString());
        }
        pSeq->DecRefCount();
    }
    pSeq = apSeq;
    if (pSeq) {
        pSeq->IncRefCount();
    }
}

bool AnimSequenceSingle::RemoveSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) {
    if (pSeq != apSeq) {
        return false;
    }
    SetSequence(nullptr, abReleaseModelLoader);
    return true;
}

bool AnimSequenceSingle::IsSingleSeq() {
    return true;
}

BSAnimGroupSequence* AnimSequenceSingle::GetSequenceToPlay(char aiSequenceIndex) {
    return pSeq;
}

// The one sequence, whatever the group.
BSAnimGroupSequence* AnimSequenceSingle::GetSequence(TESAnimGroup* apAnimGroup) {
    return pSeq;
}

char AnimSequenceSingle::GetSequenceIndex(BSAnimGroupSequence* apSeq) {
    return (-1);
}

void ModelLoader::ReleaseKFFile(const char* apFilename) {
    ReleaseKF(apFilename, true);
}

bool TESAnimGroup::IsAttackAction() {
    return IsAttackAction(sType);
}

uint8_t TESAnimGroup::GetBlendInFrames() {
    if (cBlendInFrames <= cBlendFrames) {
        return cBlendFrames;
    }
    return cBlendInFrames;
}

uint8_t TESAnimGroup::GetBlendOutFrames() {
    if (cBlendOutFrames <= cBlendFrames) {
        return cBlendFrames;
    }
    return cBlendOutFrames;
}

uint8_t TESAnimGroup::GetSectionPriority(ANIM_GROUP_SECTION aeSection) {
    return cSectionPriority[aeSection];
}

bool Actor::IsSneaking() {
    return (GetMoveMode() & 0x400) != 0 && (GetMoveMode() & 0x800) == 0;
}

BSAnimGroupSequence* MiddleHighProcess::GetAimSequence(AIM_ENUM aeDireciton) {
    return pAimSequence[aeDireciton];
}

uint32_t BGSSaveGameBuffer::GetBufferPosition() {
    return iBufferPosition;
}

void NiControllerManager::DeactivateAll(float fEaseOutTime) {
    uint32_t i = 0;
    while (i < m_kActiveSequences.GetSize()) {
        NiControllerSequence* pkSequence = m_kActiveSequences.GetAt(i);
        DeactivateSequence(pkSequence, fEaseOutTime);
        ++i;
    }
}

uint32_t NiControllerManager::GetSequenceCount() {
    return m_kSequenceArray.GetSize();
}

NiControllerSequence* NiControllerManager::GetSequenceAt(uint32_t uiIndex) {
    return m_kSequenceArray[uiIndex];
}

NiAVObject* NiControllerManager::GetAccumRoot() const {
    if (m_bCumulative) {
        for (uint32_t I = 0; I < m_kSequenceArray.m_usSize; ++I) {
            if ((&m_kSequenceArray.GetAt(I))->m_pObject) {
                return (&m_kSequenceArray.GetAt(I))->m_pObject->m_pkAccumRoot;
            }
        }
    }
    return nullptr;
}

float NiControllerSequence::GetLastScaledTime() {
    return m_fLastScaledTime;
}

NiInterpolator* NiControllerSequence::GetInterpolatorAt(uint32_t uiIndex) {
    return m_pkInterpArray[uiIndex].m_spInterpolator;
}

// The index is re-read and the controller fetched again for the GetTarget call, as the game does.
NiObjectNET* NiControllerSequence::GetTargetAt(uint32_t uiIndex) {
    if (m_pkInterpArray[uiIndex].m_spInterpCtlr) {
        return m_pkInterpArray[uiIndex].m_spInterpCtlr->GetTarget();
    }
    return nullptr;
}

NiTextKeyExtraData* NiControllerSequence::GetTextKeys() {
    return m_spTextKeys;
}

NiControllerSequence* NiControllerSequence::GetPartnerSequence() {
    return m_pkPartnerSequence;
}

NiControllerSequence::CycleType NiControllerSequence::GetCycleType() const {
    return m_eCycleType;
}

float NiControllerSequence::GetPhase() {
    double Offset = GetOffset();
    return Offset - GetBeginKeyTime();
}

void NiQuatTransform::SetRotateValid(bool bValid) {
    if (!bValid) {
        m_kRotate.SetX(-3.40282347e+38f);
    }
}

void NiQuaternion::SetX(float x) {
    m_fX = x;
}

NiObjectNET* NiTimeController::GetTarget() const {
    return m_pkTarget;
}
