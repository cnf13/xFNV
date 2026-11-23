#include "Fallout Shared/Animation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
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
#include <cstring>
#include <limits>

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
                    spAddOnObj[n],
                    static_cast<NiDefaultAVObjectPalette*>(pManager->GetObjectPalette())
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
    if (pSeq) {
        return static_cast<int16_t>(pSeq->GetSaveSize() + 14);
    }
    return 13;
}

AnimIdle::ANIM_IDLE_ENUM AnimIdle::GetFlag() {
    return eFlags;
}

KFModel* AnimIdle::GetKFModel() {
    return spKFModel;
}

void AnimIdle::Loaded(KFModel* apKFModel) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        0x1101
    );
    spKFModel = apKFModel;
    if (!apKFModel) {
        eFlags = AnimIdle::ANIM_IDLE_QUEUED;
        return;
    }
    for (int32_t n = 0; n < 2; ++n) {
        if (pAnimObj[n] && pActor) {
            spAddOnObj[n] = static_cast<NiNode*>(
                BipedAnim::LoadAndAttachAddOn(pAnimObj[n], pAnimObj[n], BO_NONE, pActor, nullptr)
            );
            NiDefaultAVObjectPalette* pPalette = static_cast<NiDefaultAVObjectPalette*>(
                pActor->GetAnimation()->GetControllerManager()->GetObjectPalette()
            );
            RecurseAndAddObjectsToPalette(spAddOnObj[n].m_pObject, pPalette);
            pActor->SetUpdateLighting(true);
        }
    }
    apKFModel->IncManualRefCount();
    eFlags = AnimIdle::ANIM_IDLE_LOADED;
    if (pActor && (eType == AnimIdle::PLAY_TYPE_AUTO || eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE)) {
        Animation* pAnim;
        pAnim = pAnimation;
        if (!pAnim) {
            pAnim = pActor->GetAnimation();
        }
        if (pAnim) {
            if (pAnim->SpecialIdlePlay()) {
                if (eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE) {
                    pActor->SetAnimAction(ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE, pSeq.m_pObject);
                }
            }
            else {
                pAnim->SpecialIdleFree(true, false);
            }
            return;
        }
    }
    else if (pActor && eType == AnimIdle::PLAY_TYPE_REPLACE) {
        Animation* pAnim = pAnimation;
        if (!pAnim) {
            pAnim = pActor->GetAnimation();
        }
        if (pAnim) {
            pAnim->SpecialIdleReplace();
            return;
        }
    }
    else {
        return;
    }
    if (this) {
        delete this;
    }
}

TESIdleForm* AnimIdle::GetIdleForm() {
    return pIdleForm;
}

void AnimIdle::SetSequence(BSAnimGroupSequence* apSeq) {
    pSeq = apSeq;
}

void AnimIdle::LoadGame(BGSLoadFormBuffer* apBuffer, Animation* apAnimation) {
    apBuffer->LoadData(static_cast<char*>(static_cast<void*>(&eFlags)), sizeof(eFlags));
    apBuffer->LoadData(static_cast<char*>(static_cast<void*>(&eType)), sizeof(eType));
    apBuffer->LoadData(static_cast<char*>(static_cast<void*>(&eSection)), sizeof(eSection));
    if (eFlags == ANIM_IDLE_QUEUED) {
        eFlags = ANIM_IDLE_LOADED;
    }
    if (eFlags == ANIM_IDLE_PLAYING && spKFModel) {
        apAnimation->AddAnimation(spKFModel, false);
    }
}

void AnimIdle::SaveGame(float afTime, Animation* apAnimation) {
    pSaveLoadGameOLD->SaveGameDataOLD(&eFlags, sizeof(eFlags));
    pSaveLoadGameOLD->SaveGameDataOLD(&eType, sizeof(eType));
    pSaveLoadGameOLD->SaveGameDataOLD(&eSection, sizeof(eSection));
    bool bHasSequence = pSeq.m_pObject != nullptr;
    pSaveLoadGameOLD->SaveGameDataOLD(&bHasSequence, sizeof(bHasSequence));
    if (bHasSequence) {
        AnimSequenceBase* pAnimSeq;
        int8_t cSequenceIndex = -1;
        uint16_t usType = spKFModel->GetAnimGroup()->GetType();
        if (apAnimation->GetMapAnimSequenceBase()->GetAt(usType, pAnimSeq)) {
            cSequenceIndex = pAnimSeq->GetSequenceIndex(pSeq.m_pObject);
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&cSequenceIndex, sizeof(cSequenceIndex));
        pSeq->SaveGame(afTime);
    }
}

void AnimIdle::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer, Animation* apAnimation) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eFlags), sizeof(eFlags), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eType), sizeof(eType), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eSection), sizeof(eSection), 0);
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

void AnimIdle::LoadGameTES(float afTime, Animation* apAnimation) {
    char cSequenceIndex;
    bool bHasSequence;
    pSaveLoadGameOLD->LoadGameDataOLD(&eFlags, sizeof(eFlags));
    pSaveLoadGameOLD->LoadGameDataOLD(&eType, sizeof(eType));
    pSaveLoadGameOLD->LoadGameDataOLD(&eSection, sizeof(eSection));
    pSaveLoadGameOLD->LoadGameDataOLD(&bHasSequence, sizeof(bHasSequence));
    if (bHasSequence) {
        pSaveLoadGameOLD->LoadGameDataOLD(&cSequenceIndex, sizeof(cSequenceIndex));
        if (apAnimation->AddAnimation(spKFModel.m_pObject, false)) {
            if (eFlags == AnimIdle::ANIM_IDLE_PLAYING) {
                AnimSequenceBase* pAnimSeq;
                uint16_t usType = spKFModel->GetAnimGroup()->GetType();
                if (apAnimation->GetMapAnimSequenceBase()->GetAt(usType, pAnimSeq)) {
                    apAnimation->SkipNextBlend();
                    ANIM_GROUP_SECTION eGroupSection = eSection;
                    uint16_t usGroup = spKFModel->GetAnimGroup()->GetType();
                    pSeq = apAnimation->StartGroup(pAnimSeq->GetSequenceToPlay(cSequenceIndex), usGroup, eGroupSection);
                }
            }
            else if (eFlags) {
                if (eFlags == AnimIdle::ANIM_IDLE_DONE_PLAYING) {
                    AnimSequenceBase* pAnimSeq = nullptr;
                    uint16_t usType = spKFModel->GetAnimGroup()->GetType();
                    if (apAnimation->GetMapAnimSequenceBase()->GetAt(usType, pAnimSeq)) {
                        pSeq = pAnimSeq->GetSequenceToPlay(cSequenceIndex);
                    }
                }
            }
            else {
                eFlags = AnimIdle::ANIM_IDLE_LOADED;
            }
        }
        if (pSeq.m_pObject) {
            pSeq->LoadGame(afTime);
        }
        pSaveLoadGameOLD->AdvanceBuffer(BSAnimGroupSequence::GetSequenceSaveSize());
    }
    else if (eFlags == AnimIdle::ANIM_IDLE_QUEUED) {
        eFlags = AnimIdle::ANIM_IDLE_LOADED;
    }
}

// The debug log goes through an empty printf stub (InitializationFlow).
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
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                sSize - sStartSize,
                pHeader->iFormID,
                pEditorID,
                pHeader->iFlags,
                4595,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                sSize - sStartSize,
                4595,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, sizeof(iBlockID));
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    uint32_t iIdleFormID = 0;
    if (apAnimIdle && apAnimIdle->GetIdleForm()) {
        iIdleFormID = apAnimIdle->GetIdleForm()->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&iIdleFormID, 4);
    if (iIdleFormID) {
        uint16_t sSaveSize = apAnimIdle->GetSaveSize();
        pSaveLoadGameOLD->SaveGameDataOLD(&sSaveSize, sizeof(sSaveSize));
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
                static_cast<int32_t>(pEnd - pStart),
                pHeader->iFormID,
                pEditorID,
                pHeader->iFlags,
                4622,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                4622,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                4622
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
                        NiNode* pAddOn = apFreeIdle->spAddOnObj[n].m_pObject;
                        TaskQueueInterface::GetInstance()->QueueSSNRemoveObject(pAddOn, 0);
                    }
                    else {
                        NiNode* pAddOn = apFreeIdle->spAddOnObj[n].m_pObject;
                        BSShaderManager::GetShadowSceneNode(0)->RemoveObject(pAddOn);
                    }
                    NiDefaultAVObjectPalette* pPalette = static_cast<NiDefaultAVObjectPalette*>(
                        apFreeIdle->pActor->GetAnimation()->GetControllerManager()->GetObjectPalette()
                    );
                    RecurseAndRemoveObjectsFromPalette(apFreeIdle->spAddOnObj[n].m_pObject, pPalette);
                    Animation* pa = apFreeIdle->pActor->GetAnimation();
                    if (pa) {
                        pa->RemoveObject(apFreeIdle->spAddOnObj[n].m_pObject);
                    }
                }
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    NiNode* pAddOn = apFreeIdle->spAddOnObj[n].m_pObject;
                    TaskQueueInterface::GetInstance()->QueueNodeDetach(pAddOn);
                }
                else {
                    apFreeIdle->spAddOnObj[n]->m_pkParent->DetachChild(apFreeIdle->spAddOnObj[n].m_pObject);
                }
                const char* pModel = apFreeIdle->pAnimObj[n]->GetModel();
                pModelLoader->ReleaseFile(pModel);
                apFreeIdle->spAddOnObj[n] = static_cast<NiNode*>(nullptr);
                if (apFreeIdle->pSeq.m_pObject) {
                    apFreeIdle->pSeq->ValidateTargets();
                }
            }
        }
        if (apFreeIdle->eFlags == AnimIdle::ANIM_IDLE_QUEUED) {
            pModelLoader->CancelAnimIdle(apFreeIdle);
        }
        if (!pSaveLoadGameOLD->GetLoading() &&
            apFreeIdle->pActor &&
            apFreeIdle->pActor->GetAnimAction() == ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE) {
            BSAnimGroupSequence* pActionSeq = apFreeIdle->pActor->pCurrentProcess->GetAnimActionAnimSeq();
            if (pActionSeq == apFreeIdle->pSeq.m_pObject) {

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
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        0x1218
    );
    AnimIdle* panimIdle = nullptr;
    iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    0x121C,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    0x121C,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    bool bforcedToZero = pSaveLoadGameOLD->LoadNumericID(&iformID, 4u);
    if (iformID || bforcedToZero) {
        pSaveLoadGameOLD->LoadGameDataOLD(&isize, sizeof(isize));
        TESIdleForm* pidle = nullptr;
        if (iformID) {
            TESForm* pForm = TESForm::GetFormByNumericID(iformID);
            pidle = dynamic_cast<TESIdleForm*>(pForm);
        }
        if (pidle) {
            panimIdle = new AnimIdle(pidle, AGS_SPECIAL_IDLE, AnimIdle::PLAY_TYPE_QUEUE, apActor, true, nullptr);
            panimIdle->LoadGameTES(afTime, apAnimation);
        }
        else {
            const char* pEditorID = apActor->GetFormEditorID();
            Warning("FORMS: Could not find IdleForm %08X when loading actor %s", iformID, pEditorID);
            pSaveLoadGameOLD->AdvanceBuffer(isize);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t iVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                        0x1237,
                        pheader->iFormID,
                        pEditorID,
                        iVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t iVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    0x1237,
                    pheader->iFormID,
                    pEditorID,
                    iVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
            if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    0x1237,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                icurrentBufferPosition - &istartBufferPosition[iblockSize],
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                0x1237,
                iCurrentVersion
            );
        }
    }
    return panimIdle;
}

Animation::Animation()
  : pActorRef(nullptr), pAccumRoot(nullptr), movementDelta(NiPoint3::ZERO), AccumRootTranslate(NiPoint3::ZERO),
    pSoundPriorityBone{}, m_fLooking(0.0f), cSkipUpdate(-1), bShutDown(false), time(0.0f),
    pAnimSequenceMap(new NiTPointerMap<uint16_t, AnimSequenceBase*>(101)), pCurrentSequence{},
    pLastMovementSequence(nullptr), m_fMoveSpeed(1.0f), m_fAttackSpeed(1.0f), m_fGlobalTimeMultiplier(1.0f),
    m_fReloadModifier(1.0f), m_fEquipModifier(1.0f), cSkipNextBlend(0), sQueuedReloadGroup(255) {
    SetField(0, 255, 0);
    // Fills the per-section tables with 0xFF bytes: no group, no action, loop counts of -1.
    for (int32_t i = 0; i < 8; ++i) {
        group[i] = 0xFFFF;
        nextGroup[i] = 0xFFFF;
        action[i] = AGA_NONE;
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
        NiTimeController* pMultiTread = pRoot ? pRoot->GetController(&BSMultiTargetTreadTransfController::ms_RTTI)
                                              : nullptr;
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
    if (spManager.m_pObject) {
        uint32_t uiSequenceCount = spManager->GetSequenceCount();
        for (uint32_t uiIndex = 0; uiIndex < uiSequenceCount; ++uiIndex) {
            NiControllerSequence* pkObject = spManager->GetSequenceAt(uiIndex);
            BSAnimGroupSequence* pSequence = static_cast<BSAnimGroupSequence*>(
                NiObject::DynamicCastStatic(&BSAnimGroupSequence::ms_RTTI, pkObject)
            );
            if (pSequence) {
                pSequence->ClearBlendInterp();
            }
        }
        while (GetAnimsLoading()) {
            KFModel* pKFModel = kfModelList.m_item;
            pKFModel->DecUseCount();
            pKFModel->DecUseCount();
            kfModelList.RemoveHead();
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
    NiSmartPointerCast<BSBipNode>(pAnimRoot)->UpdateAllButBip(time);
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
    apBuffer->LoadData(reinterpret_cast<char*>(&m_fMoveSpeed), sizeof(m_fMoveSpeed));
    apBuffer->LoadData(reinterpret_cast<char*>(&m_fAttackSpeed), sizeof(m_fAttackSpeed));
    apBuffer->LoadData(reinterpret_cast<char*>(&movementDelta), sizeof(movementDelta));
    apBuffer->LoadData(reinterpret_cast<char*>(&m_fLooking), sizeof(m_fLooking));
    apBuffer->LoadData(&cSkipUpdate, sizeof(cSkipUpdate));
    apBuffer->LoadData(reinterpret_cast<char*>(&fLipTime), sizeof(fLipTime));
    apBuffer->LoadData(reinterpret_cast<char*>(&sQueuedReloadGroup), sizeof(sQueuedReloadGroup));
    uint32_t uiDelayCount = apBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiDelayCount; ++i) {
        IDLE_REPLAY_DELAY* pDelay = static_cast<IDLE_REPLAY_DELAY*>(ourMalloc(sizeof(IDLE_REPLAY_DELAY)));
        uint32_t uiFormID = apBuffer->LoadFormID();
        pDelay->pIdleForm = dynamic_cast<TESIdleForm*>(TESForm::GetFormByNumericID(uiFormID));
        apBuffer->LoadData(reinterpret_cast<char*>(&pDelay->fDelay), sizeof(pDelay->fDelay));
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
            AnimIdle*
                pAnimIdle = new AnimIdle(pIdleForm, AGS_SPECIAL_IDLE, AnimIdle::PLAY_TYPE_QUEUE, pActor, true, nullptr);
            spAnimIdle = pAnimIdle;
            spAnimIdle->LoadGame(apBuffer, this);
        }
    }
    int8_t cSequenceMask = 0;
    apBuffer->LoadData(reinterpret_cast<char*>(&cSequenceMask), sizeof(cSequenceMask));
    for (uint32_t i = 0; i < ANIM_GROUP_SECTION_COUNT; ++i) {
        if (cSequenceMask & (1 << i)) {
            apBuffer->LoadData(reinterpret_cast<char*>(&group[i]), sizeof(group[i]));
            apBuffer->LoadData(reinterpret_cast<char*>(&action[i]), sizeof(action[i]));
            apBuffer->LoadData(reinterpret_cast<char*>(&nextGroup[i]), sizeof(nextGroup[i]));
            apBuffer->LoadData(reinterpret_cast<char*>(&loopCount[i]), sizeof(loopCount[i]));
            apBuffer->LoadData(reinterpret_cast<char*>(&nextLoops[i]), sizeof(nextLoops[i]));
            pCurrentSequence[i] = nullptr;
            char cSequenceIndex = -1;
            apBuffer->LoadData(&cSequenceIndex, sizeof(cSequenceIndex));
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
    apBuffer->LoadData(&cSkipNextBlend, sizeof(cSkipNextBlend));
    int16_t sAnimAction = -1;
    char cActionSequence = -1;
    apBuffer->LoadData(reinterpret_cast<char*>(&sAnimAction), sizeof(sAnimAction));
    apBuffer->LoadData(&cActionSequence, sizeof(cActionSequence));
    if (cActionSequence != -1 && pCurrentSequence[cActionSequence]) {
        pActor->GetCurrentAIProcess()->SetAnimAction(
            static_cast<ANIMATION_ACTION>(sAnimAction),
            pCurrentSequence[cActionSequence]
        );
    }
    char cIdleSequence = -1;
    apBuffer->LoadData(&cIdleSequence, sizeof(cIdleSequence));
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
    NiSmartPointerCast<BSBipNode>(pAnimRoot)->UpdateAllButBip(time);
}

uint16_t Animation::PickBestAnimation(uint16_t asAnimGroup, bool abRecursive) {
    ANIM_GROUP_WEAPON_ENUM eWeapon;
    AnimSequenceBase* pAnimSeq;
    uint16_t sReturnGroup;
    uint16_t sTestGroup;
    if (pAnimSequenceMap->GetAt(asAnimGroup, pAnimSeq) && pAnimSeq->GetSequenceToPlay(-1)) {
        return asAnimGroup;
    }
    if ((asAnimGroup & TESAnimGroup::POWER_ARMOR_MASK) != 0) {
        sReturnGroup = PickBestAnimation(asAnimGroup & ~TESAnimGroup::POWER_ARMOR_MASK, true);
        if (static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(asAnimGroup) ||
            (TESAnimGroup::IsIronSightsAction(asAnimGroup) &&
             static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(asAnimGroup - 3))) {
            return sReturnGroup;
        }
    }
    if (pActorRef == pPlayer && this == pPlayer->GetAnimation(true) && (asAnimGroup & TESAnimGroup::MOVE_MASK) != 0) {
        ANIM_GROUP_SECTION eSectionLayer = animGroupInfo[static_cast<uint8_t>(asAnimGroup)].eSectionLayer;
        if (eSectionLayer >= AGS_WEAPON && eSectionLayer <= AGS_WEAPON_DOWN) {
            sReturnGroup = PickBestAnimation(asAnimGroup & (TESAnimGroup::TYPE_MASK | TESAnimGroup::WEAPON_MASK), true);
            if (static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(asAnimGroup) ||
                (TESAnimGroup::IsIronSightsAction(asAnimGroup) &&
                 static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(asAnimGroup - 3))) {
                return sReturnGroup;
            }
        }
    }
    if (TESAnimGroup::IsIronSightsAction(asAnimGroup)) {
        return PickBestAnimation(asAnimGroup - 3, true);
    }
    ANIM_GROUP_SECTION eSectionLayer = animGroupInfo[TESAnimGroup::GetType(asAnimGroup)].eSectionLayer;
    if (eSectionLayer >= AGS_WEAPON_UP && eSectionLayer <= AGS_WEAPON_DOWN) {
        return 0;
    }
    if ((asAnimGroup & TESAnimGroup::WEAPON_MASK) != 0) {
        eWeapon = TESAnimGroup::GetWeapon(asAnimGroup);
        if (eWeapon != AGW_ONE_HAND_MELEE) {
            if (eWeapon == AGW_TWO_HAND_MELEE) {
                if (pAnimSequenceMap->GetAt(
                        (asAnimGroup & ~TESAnimGroup::WEAPON_MASK) | (AGW_ONE_HAND_MELEE << TESAnimGroup::WEAPON_POS),
                        pAnimSeq
                    ) &&
                    pAnimSeq->GetSequenceToPlay(-1)) {
                    return (asAnimGroup & ~TESAnimGroup::WEAPON_MASK) |
                        (AGW_ONE_HAND_MELEE << TESAnimGroup::WEAPON_POS);
                }
            }
            else if (
                eWeapon != AGW_ONE_HAND_PISTOL &&
                pAnimSequenceMap->GetAt(
                    (asAnimGroup & ~TESAnimGroup::WEAPON_MASK) | (AGW_ONE_HAND_PISTOL << TESAnimGroup::WEAPON_POS),
                    pAnimSeq
                ) &&
                pAnimSeq->GetSequenceToPlay(-1)
            ) {
                return (asAnimGroup & ~TESAnimGroup::WEAPON_MASK) | (AGW_ONE_HAND_PISTOL << TESAnimGroup::WEAPON_POS);
            }
        }
        if (pAnimSequenceMap->GetAt(asAnimGroup & ~TESAnimGroup::WEAPON_MASK, pAnimSeq) &&
            pAnimSeq->GetSequenceToPlay(-1)) {
            return asAnimGroup & ~TESAnimGroup::WEAPON_MASK;
        }
    }
    if ((asAnimGroup & TESAnimGroup::POWER_ARMOR_MASK) != 0) {
        sReturnGroup = PickBestAnimation(asAnimGroup & ~TESAnimGroup::POWER_ARMOR_MASK, true);
        if (static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(asAnimGroup)) {
            return sReturnGroup;
        }
    }
    if (asAnimGroup) {
        sTestGroup = 255;
        switch (TESAnimGroup::GetType(asAnimGroup)) {
            case ANIM_GROUP_FAST_FORWARD: {
                sTestGroup = (asAnimGroup & 0x7F00) | 3;
                break;
            }
            case ANIM_GROUP_FAST_BACK: {
                sTestGroup = (asAnimGroup & 0x7F00) | 4;
                break;
            }
            case ANIM_GROUP_FAST_LEFT: {
                sTestGroup = (asAnimGroup & 0x7F00) | 5;
                break;
            }
            case ANIM_GROUP_FAST_RIGHT: {
                sTestGroup = (asAnimGroup & 0x7F00) | 6;
                break;
            }
            default: {
                break;
            }
        }
        if (sTestGroup != 255) {
            sReturnGroup = PickBestAnimation(sTestGroup, true);
            if (static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(sTestGroup)) {
                return sReturnGroup;
            }
        }
    }
    if (abRecursive) {
        return 0;
    }
    if ((asAnimGroup & TESAnimGroup::MOVE_MASK) != 0) {
        sReturnGroup = PickBestAnimation(asAnimGroup & (TESAnimGroup::TYPE_MASK | TESAnimGroup::WEAPON_MASK), true);
        if (static_cast<uint8_t>(sReturnGroup) == static_cast<uint8_t>(asAnimGroup)) {
            return sReturnGroup;
        }
    }
    if (!static_cast<uint8_t>(asAnimGroup)) {
        return 0;
    }
    return PickBestAnimation(asAnimGroup & 0x7F00, false);
}

bool Animation::SpecialIdlePlaying() {
    if (!spAnimIdle) {
        return false;
    }
    return spAnimIdle->GetFlag() == AnimIdle::ANIM_IDLE_PLAYING;
}

NiTPointerMap<uint16_t, AnimSequenceBase*>* Animation::GetMapAnimSequenceBase() {
    return pAnimSequenceMap;
}

TESAnimGroup* Animation::GetTESAnimGroup(uint16_t asGroup) {
    AnimSequenceBase* pAnimSeq;
    if (!pAnimSequenceMap->GetAt(asGroup, pAnimSeq)) {
        return nullptr;
    }
    return pAnimSeq->GetSequenceToPlay(-1)->GetAnimGroup();
}

int16_t Animation::GetGroupSpeed(uint16_t asAnimGroup) {
    AnimSequenceBase* pAnimSeq;
    if (!pAnimSequenceMap->GetAt(asAnimGroup, pAnimSeq)) {
        return 0;
    }
    if (!pAnimSeq->IsSingleSeq()) {
        return 0;
    }
    if (!pAnimSeq->GetSequenceToPlay(-1)) {
        return 0;
    }
    return static_cast<int32_t>(pAnimSeq->GetSequenceToPlay(-1)->GetAnimGroup()->GetSpeed());
}

TESIdleForm* Animation::GetSpecialIdleForm() {
    if (!spAnimIdle) {
        return nullptr;
    }
    return spAnimIdle->GetIdleForm();
}

bool Animation::SpecialIdlePlay() {
    if (spAnimIdleQueued.m_pObject) {
        if (spAnimIdleQueued->eFlags != AnimIdle::ANIM_IDLE_LOADED) {
            return false;
        }
        SpecialIdleFree(false, true);
    }
    if (!spAnimIdle.m_pObject || spAnimIdle->eFlags != AnimIdle::ANIM_IDLE_LOADED) {
        return false;
    }
    KFModel* pkfModel = spAnimIdle->GetKFModel();
    if (!pkfModel || !AddAnimation(pkfModel, false) || !pkfModel->GetAnimGroup()) {
        spAnimIdle->eFlags = AnimIdle::ANIM_IDLE_DONE_PLAYING;
        if (spAnimIdle->eType == AnimIdle::PLAY_TYPE_AUTO || spAnimIdle->eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE) {
            SpecialIdleFree(true, false);
        }
        return false;
    }
    BSAnimGroupSequence* pseq = nullptr;
    if (spAnimIdle->eSection || spAnimIdle->GetKFModel()->GetAnimGroup()->GetType() != ANIM_GROUP_DYNAMIC_IDLE) {
        // Reads the section, then the idle's loop count, then the group: locals keep that order.
        ANIM_GROUP_SECTION esection = spAnimIdle->eSection;
        uint8_t cloops = spAnimIdle->pIdleForm->GetLoops();
        pseq = PlayGroup(pkfModel->GetAnimGroup()->GetType(), ACTION_START, cloops, esection);
    }
    else {
        ANIM_GROUP_SECTION esection = spAnimIdle->eSection;
        pseq = PlayGroup(pkfModel->GetAnimGroup()->GetType(), ACTION_START, -1, esection);
    }
    if (!pseq) {
        spAnimIdle->eFlags = AnimIdle::ANIM_IDLE_DONE_PLAYING;
        if (spAnimIdle->eType == AnimIdle::PLAY_TYPE_AUTO || spAnimIdle->eType == AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE) {
            SpecialIdleFree(true, false);
        }
        return false;
    }
    if (static_cast<double>(spAnimIdle->pIdleForm->GetReplayDelay()) > 0.0) {
        IDLE_REPLAY_DELAY* preplayDelay = static_cast<IDLE_REPLAY_DELAY*>(ourMalloc(sizeof(IDLE_REPLAY_DELAY)));
        preplayDelay->pIdleForm = spAnimIdle->GetIdleForm();
        preplayDelay->fDelay = static_cast<float>(spAnimIdle->pIdleForm->GetReplayDelay());
        replayDelayList.AddTail(preplayDelay);
    }
    spAnimIdle->PlayAnim(pseq);
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
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        4765
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
    if (spAnimIdle) {
        if (spAnimIdle->GetIdleForm() == apIdle) {
            if (spAnimIdle->GetFlag() != AnimIdle::ANIM_IDLE_LOADED) {
                return true;
            }
        }
    }
    if (spAnimIdleQueued) {
        if (spAnimIdleQueued->GetIdleForm() == apIdle) {
            if (spAnimIdleQueued->GetFlag() != AnimIdle::ANIM_IDLE_LOADED) {
                return true;
            }
        }
    }
    if (spAnimIdleFreeWhenInactiveA[0]) {
        if (spAnimIdleFreeWhenInactiveA[0]->GetIdleForm() == apIdle) {
            return true;
        }
        if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
            if (spAnimIdleFreeWhenInactiveA[0]->GetSequence()) {
                return true;
            }
        }
    }
    if (!spAnimIdleFreeWhenInactiveA[1]) {
        return false;
    }
    if (spAnimIdleFreeWhenInactiveA[1]->GetIdleForm() == apIdle) {
        return true;
    }
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && spAnimIdleFreeWhenInactiveA[1]->GetSequence()) {
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
        pkeyText = pkeys[i].m_kText.GetHandleString();
        if (pkeyText && !_strnicmp(pkeyText, psearchKey, isearchKeyLength)) {
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
    if (!spAnimIdle->AnimIdle::GetSequence()) {
        return false;
    }
    return spAnimIdle->GetFlag() == AnimIdle::ANIM_IDLE_DONE_PLAYING;
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
    if (pAccumRoot) {
        ClearAccumRootTranslate();
        pAccumRoot->SetRotate(NiMatrix3::IDENTITY);
        pAccumRoot->SetTranslate(NiPoint3::ZERO);
        for (NiTimeController* pkController = pAccumRoot->NiObjectNET::GetControllers(); pkController;
             pkController = pkController->GetNext()) {
            NiSingleInterpController* pkInterpController = static_cast<NiSingleInterpController*>(
                NiObject::DynamicCastStatic(&NiSingleInterpController::ms_RTTI, pkController)
            );
            if (pkInterpController) {
                NiInterpolator* pkObject = pkInterpController->GetInterpolator(0);
                if (pkObject) {
                    NiBlendAccumTransformInterpolator* pkAccumInterp = static_cast<NiBlendAccumTransformInterpolator*>(
                        NiObject::DynamicCastStatic(&NiBlendAccumTransformInterpolator::ms_RTTI, pkObject)
                    );
                    if (pkAccumInterp) {
                        pkAccumInterp->ClearAccumulatedTransform();
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
        NiAVObjectPalette* ppalette = spManager->GetObjectPalette();
        if (ppalette) {
            NiDefaultAVObjectPalette* pdefaultPalette = static_cast<NiDefaultAVObjectPalette*>(
                NiObject::DynamicCastStatic(&NiDefaultAVObjectPalette::ms_RTTI, ppalette)
            );
            if (pdefaultPalette) {
                pdefaultPalette->ResetAndFillFromScenegraph();
            }
        }
        NiTimeController* pmulti = pAnimRoot->GetController(&NiMultiTargetTransformController::ms_RTTI);
        NiTimeController* pmultiTread = pAnimRoot->GetController(&BSMultiTargetTreadTransfController::ms_RTTI);
        if (pmulti) {
            pAnimRoot->RemoveController(pmulti);
        }
        if (pmultiTread) {
            pAnimRoot->RemoveController(pmultiTread);
        }
        spManager->DeactivateAll(0.0f);
        uint32_t uiTransControllerCount = 0;
        for (uint32_t n = 0; n < spManager->GetSequenceCount(); ++n) {
            BSAnimGroupSequence* pseq = static_cast<BSAnimGroupSequence*>(spManager->GetSequenceAt(n));
            if (pseq && pseq->m_uiArraySize > uiTransControllerCount) {
                uiTransControllerCount = pseq->m_uiArraySize;
            }
        }
        for (uint32_t n = 0; n < spManager->GetSequenceCount(); ++n) {
            BSAnimGroupSequence* pseq = static_cast<BSAnimGroupSequence*>(spManager->GetSequenceAt(n));
            if (pseq) {
                if (abReload) {
                    if (!pAnimRoot->GetController(&NiMultiTargetTransformController::ms_RTTI) &&
                        uiTransControllerCount) {
                        pseq->ResolveTransformInterpolators(pAnimRoot.m_pObject, uiTransControllerCount);
                    }

                    const char* pName = pseq->GetName().GetHandleString();
                    if (pName[0] != '_' || pName[1] != '_') {
                        pseq->StoreTargets(pAnimRoot.m_pObject);
                    }
                }
                else {
                    pseq->ValidateTargetsFromPalette();
                    pseq->ResolveTransformInterpolators(pAnimRoot.m_pObject, uiTransControllerCount);
                }
            }
        }
        for (BSAnimGroupSequence* pSequence : pCurrentSequence) {
            if (pSequence) {
                spManager->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
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
    NiMatrix3 kLocalRot;
    if (!pAnimRoot.m_pObject) {
        return false;
    }
    float fTimeDelta = appTimer.GetSeconds() * m_fGlobalTimeMultiplier;
    NiPoint3 kZero = NiPoint3::ZERO;
    NiPoint3 kVector = movementDelta;
    NiNode* pRootNode = pAnimRoot.m_pObject;
    std::memcpy(&kLocalRot, &pRootNode->GetLocalTransform(), sizeof(kLocalRot));
    if (!apRef->IsCreature() && static_cast<Actor*>(apRef)->IsMoving() && pCurrentSequence[1]) {
        TESAnimGroup* pAnimGroup = pCurrentSequence[1]->GetAnimGroup();
        if (pAnimGroup->GetType() >= ANIM_GROUP_BEGIN_MOVEMENT) {
            pAnimGroup = pCurrentSequence[1]->GetAnimGroup();
            if (pAnimGroup->GetType() <= ANIM_GROUP_END_MOVEMENT) {
                pAnimGroup = pCurrentSequence[1]->GetAnimGroup();
                double dUnscaledMove = pAnimGroup->GetSpeed() * m_fMoveSpeed * fTimeDelta;
                float fMove = static_cast<float>(pActorRef->GetScale() * dUnscaledMove);
                if (pCurrentSequence[4] &&
                    pCurrentSequence[4]->NiControllerSequence::GetState() == NiControllerSequence::kAnimating) {
                    TESAnimGroup* pWeaponAnimGroup = pCurrentSequence[4]->GetAnimGroup();
                    double dUnscaledAttack = pWeaponAnimGroup->GetSpeed() * m_fAttackSpeed * fTimeDelta;
                    float fAttack = static_cast<float>(pActorRef->GetScale() * dUnscaledAttack);
                    if (fMove < static_cast<double>(fAttack)) {
                        fMove = fAttack;
                    }
                }

                float fMaxMove = static_cast<float>(fMove * 4.0);
                if (fMove > 0.0) {
                    if (fMaxMove < static_cast<double>(kVector.x)) {
                        kVector.x = fMove;
                    }
                    if (fMaxMove < static_cast<double>(kVector.y)) {
                        kVector.y = fMove;
                    }
                    if (fMaxMove < static_cast<double>(kVector.z)) {
                        kVector.z = fMove;
                    }
                    if (kVector.x < -fMaxMove) {
                        kVector.x = -fMove;
                    }
                    if (kVector.y < -fMaxMove) {
                        kVector.y = -fMove;
                    }
                    if (kVector.z < -fMaxMove) {
                        kVector.z = -fMove;
                    }
                }
            }
        }
    }

    if (apRef) {
        switch (apRef->GetSitSleepState()) {
            case SIT_SLEEP_WAITING_FOR_SIT_ANIM:
            case SIT_SLEEP_WANT_TO_STAND:
            case SIT_SLEEP_WAITING_FOR_SLEEP_ANIM:
            case SIT_SLEEP_WANT_TO_WAKE: {
                float fScale = apRef->GetScale();
                kVector.x = kVector.x / fScale;
                kVector.y = kVector.y / fScale;
                break;
            }
            default: {
                break;
            }
        }
    }
    if (zeroZ) {
        kVector.z = 0.0f;
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
    else if (iSection == 21) {
        iSection = 4;
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
    if (spManager &&
        eSection < ANIM_GROUP_SECTION_COUNT &&
        pCurrentSequence[eSection] &&
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
    // The sum is computed in double precision and then stored to a float.
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
    if (afPercent == 0.0) {
        if (pCurrentSequence[3]) {
            pCurrentSequence[3]->Update(0.0, true);
            ClearGroup(AGS_TALKING, 0.0);
        }
    }
    else {
        if (!pCurrentSequence[3]) {
            PlayGroup(0xE1u, ACTION_START, -1, AGS_NONE);
            if (pCurrentSequence[3]) {
                float fStartTime = pCurrentSequence[3]->GetBeginKeyTime() + time;
                pCurrentSequence[3]->Update(fStartTime, true);
            }
        }
        if (pCurrentSequence[3] &&
            pCurrentSequence[3]->NiControllerSequence::GetState() == NiControllerSequence::kAnimating) {
            double dEndKeyTime = pCurrentSequence[3]->GetEndKeyTime();
            float fLength = static_cast<float>(dEndKeyTime - pCurrentSequence[3]->GetBeginKeyTime());
            fLipTime = pCurrentSequence[3]->GetBeginKeyTime() + fLength * afPercent;
            float fPhase = fLipTime - time;
            pCurrentSequence[3]->SetPhase(fPhase, true);
        }
    }
}

// The whole-body and upper-body sections queue in the movement and weapon slots. StartGroup gets the caller's
// aeSection, not the resolved one; a group type of 0xFF (no group) returns null at once.
BSAnimGroupSequence*
Animation::PlayGroup(uint16_t asNewGroup, ACTION_FLAGS aeFlags, int32_t aiLoops, ANIM_GROUP_SECTION aeSection) {
    int32_t iType = TESAnimGroup::GetType(asNewGroup);
    int32_t eSection = aeSection == AGS_NONE ? animGroupInfo[iType].eSectionLayer : aeSection;
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
            pCurrentSequence[AGS_WEAPON],
            static_cast<ANIM_GROUP_SECTION>(eSection)
        );
    }
    return StartGroup(pStarted, asNewGroup, aeSection);
}

BSAnimGroupSequence*
Animation::StartGroup(BSAnimGroupSequence* apSeq, uint16_t asNewGroup, ANIM_GROUP_SECTION aeSection) {
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
    else if (esection == AGS_UPPER_BODY) {
        esection = AGS_WEAPON;
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
    if (esection == AGS_WEAPON || esection == AGS_WEAPON_UP || esection == AGS_WEAPON_DOWN) {
        MiddleHighProcess* pmid;
        bool bclear;
        if (pActorRef == pPlayer && this == pPlayer->GetAnimation(true)) {
            apSeq->SetSequenceWeight(esection == AGS_WEAPON ? 1.0f : 0.0f);
        }
        else {
            pmid = static_cast<MiddleHighProcess*>(static_cast<MobileObject*>(pActorRef)->GetCurrentAIProcess());
            bclear = true;
            if ((TESAnimGroup::IsAimAction(egroup) || TESAnimGroup::IsAttackAction(egroup)) &&
                pmid &&
                pmid->eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                BSAnimGroupSequence* poldSeq = pmid->GetAimSequence(static_cast<AIM_ENUM>(esection - AGS_WEAPON));
                if (poldSeq) {
                    apSeq->SetSequenceWeight(poldSeq->m_fSeqWeight);
                    bclear = false;
                }
            }

            if (bclear && (esection == AGS_WEAPON_UP || esection == AGS_WEAPON_DOWN)) {
                apSeq->SetSequenceWeight(0.0f);
            }
        }
    }
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
        pAnimRoot.m_pObject != pPlayer->Get3D(false) ||
        pSaveLoadGameOLD->GetLoading() ||
        pVATS.eMode) {
        if (Interface::IsInPipboyMenu() &&
            pAnimRoot.m_pObject == pPlayer->Get3D(true) &&
            egroup == ANIM_GROUP_DYNAMIC_IDLE &&
            aeSection == AGS_IDLE &&
            plastSequence) {
            ClearGroup(eclearSection, 0.0f);
            plastSequence = nullptr;
            cSkipNextBlend = 1;
        }
        else if (plastSequence && esection == AGS_LEFT_ARM) {
            ANIM_GROUP_ENUM eoldGroup = plastSequence->GetAnimGroup()->GetType();
            if (egroup == ANIM_GROUP_PIPBOY ||
                (egroup == ANIM_GROUP_PIPBOY_CHILD &&
                 eoldGroup >= ANIM_GROUP_HAND_GRIP_1 &&
                 eoldGroup <= ANIM_GROUP_HAND_GRIP_6)) {
                ClearGroup(eclearSection, 0.0f);
                plastSequence = nullptr;
            }
        }
        else if (elastState && plastSequence) {
            if (elastState == NiControllerSequence::kAnimating) {
                if (eclearSection != esection || cSkipNextBlend || apSeq->GetState()) {
                    ClearGroup(eclearSection, 0.0f);
                }
                else if (
                    eclearSection == AGS_WEAPON &&
                    (egroup <= ANIM_GROUP_UNEQUIP || egroup > ANIM_GROUP_JAM_M) &&
                    !TESAnimGroup::IsAimAction(egroup)
                ) {
                    ClearGroup(AGS_WEAPON_UP, 0.0f);
                    ClearGroup(AGS_WEAPON_DOWN, 0.0f);
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
    group[esection] = asNewGroup;
    pCurrentSequence[esection] = apSeq;
    if ((!Interface::IsInMenuMode() || pAnimRoot.m_pObject != pPlayer->Get3D(false)) && plastSequence) {
        NiPointer<TESAnimGroup> spcurrentAnimGroup(apSeq->GetAnimGroup());
        NiPointer<TESAnimGroup> splastAnimGroup(plastSequence->GetAnimGroup());
        if (splastAnimGroup->cMorphKey && splastAnimGroup->cMorphKey == spcurrentAnimGroup->cMorphKey) {
            int32_t iLastArraySize = plastSequence->m_uiArraySize;
            if (iLastArraySize == apSeq->m_uiArraySize) {
                if (apSeq->CanSyncTo(plastSequence)) {
                    bmorph = true;
                }
                else {
                    const char* pRootName = pAnimRoot->m_kName.GetHandleString();
                    const char* pSeqName = apSeq->m_kName.GetHandleString();
                    Warning(
                        "ANIMATION: Morph Error - Morph tags are different in '%s' and '%s' for '%s'.",
                        plastSequence->m_kName.GetHandleString(),
                        pSeqName,
                        pRootName
                    );
                }
            }
            else {
                const char* pRootName = pAnimRoot->m_kName.GetHandleString();
                int32_t iSeqArraySize = apSeq->m_uiArraySize;
                const char* pSeqName = apSeq->m_kName.GetHandleString();
                int32_t iLastArraySize = plastSequence->m_uiArraySize;
                Warning(
                    "ANIMATION: Morph Error - Controller count not the same.\r\n'%s' has %d controllers "
                    "and\r\n'%s' has %d on '%s'.",
                    plastSequence->m_kName.GetHandleString(),
                    iLastArraySize,
                    pSeqName,
                    iSeqArraySize,
                    pRootName
                );
            }
        }

        if (bmorph && apSeq == plastSequence) {
            const char* pRootName = pAnimRoot->m_kName.GetHandleString();
            Warning(
                "ANIMATION: Morph Error - Trying to morph from sequence to itself.\r\n'%s' on '%s'.",
                plastSequence->m_kName.GetHandleString(),
                pRootName
            );
            bmorph = false;
        }
    }

    float fduration = fAnimationDefaultBlend.Float();
    uint8_t cblendFrames = 0;
    if (plastSequence) {
        cblendFrames = plastSequence->GetAnimGroup()->GetBlendOutFrames();
    }
    uint8_t cBlendInFrames = apSeq->GetAnimGroup()->GetBlendInFrames();
    if (cBlendInFrames > static_cast<int32_t>(cblendFrames)) {
        cblendFrames = apSeq->GetAnimGroup()->GetBlendInFrames();
    }
    if (cblendFrames) {
        fduration = static_cast<double>(cblendFrames) / 30.0;
    }
    if ((Interface::IsInMenuMode() && pAnimRoot == pPlayer->GetInventory3D()) ||
        Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0)) {
        fduration = fMenuModeAnimBlend.Float();
    }
    if (egroup >= ANIM_GROUP_RELOAD_START &&
        egroup <= ANIM_GROUP_RELOAD_Z &&
        plastSequence &&
        pactor != pPlayer &&
        fduration < 0.5) {
        if (apSeq->GetAnimGroup()->GetMove() == AGM_WALK) {
            if (plastSequence->GetAnimGroup()->GetMove() != AGM_WALK) {
                fduration = 0.5f;
            }
        }
        else if (plastSequence->GetAnimGroup()->GetMove() == AGM_WALK) {
            fduration = 0.5f;
        }
    }

    if (TESAnimGroup::IsAttackAction(elastGroup) &&
        egroup >= ANIM_GROUP_LOOPING_RELOAD_START_ANIMS &&
        egroup <= ANIM_GROUP_RELOAD_Z &&
        pactor == pPlayer) {
        fduration = 0.0f;
    }
    if (cSkipNextBlend) {
        fduration = 0.0f;
    }
    fduration = fduration / fAnimationMult.Float();
    apSeq->SetPhase(0.0f, false);
    if (!plastSequence && (esection == AGS_WEAPON_UP || esection == AGS_WEAPON_DOWN)) {
        spManager->ActivateSequence(apSeq, 0, true, apSeq->m_fSeqWeight, fduration, nullptr);
    }
    else if (!(fduration < 0.009999999776482582)) {
        if (bmorph) {
            spManager->Morph(plastSequence, apSeq, fduration, 0, plastSequence->m_fSeqWeight, apSeq->m_fSeqWeight);
        }
        else {
            bool bretVal = false;
            if (plastSequence && plastSequence->GetState()) {
                bretVal = spManager->CrossFade(plastSequence, apSeq, fduration, 0, true, apSeq->m_fSeqWeight, nullptr);
            }

            if (!bretVal) {
                if (TES::pInstance->CellTestRunning()) {
                    spManager->ActivateSequence(apSeq, 0, true, 1.0f, 0.0f, nullptr);
                }
                else {
                    if (pActorRef && pActorRef->IsActor()) {
                        Actor* pActor = static_cast<Actor*>(pActorRef);
                        if (bhkRagdollController::QFootIK() &&
                            pActor->pRagdollController &&
                            pActor->pRagdollController->QFootIKActive()) {
                            pActor->pRagdollController->ApplyBoneTransforms();
                        }
                    }

                    spManager->BlendFromPose(apSeq, 0.0f, fduration, 0, nullptr);
                }
            }
        }
    }
    else {
        spManager->ActivateSequence(apSeq, 0, true, apSeq->m_fSeqWeight, 0.0f, nullptr);
    }
    if (plastSequence && plastSequence != apSeq) {
        elastState = plastSequence->GetState();
        if (elastState > NiControllerSequence::kInactive &&
            (elastState <= NiControllerSequence::kEaseIn || elastState == NiControllerSequence::kTransDest)) {
            spManager->DeactivateSequence(plastSequence, 0.0f);
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
    else if (aeSection == AGS_UPPER_BODY) {
        eseqSection = AGS_WEAPON;
    }
    Actor* pactor = static_cast<Actor*>(pActorRef);
    if (eseqSection == AGS_MOVEMENT) {
        movementDelta = NiPoint3::ZERO;
    }
    BSAnimGroupSequence* pblendOutSeq = pCurrentSequence[eseqSection];
    float fduration = 0.0f;
    if (pblendOutSeq && !cSkipNextBlend) {
        fduration = fAnimationDefaultBlend.Float();
        char cblendFrames = pblendOutSeq->GetAnimGroup()->GetBlendOutFrames();
        if (cblendFrames) {
            fduration = static_cast<double>(cblendFrames) / 30.0;
        }
        fduration = fduration / fAnimationMult.Float();
    }

    bool bclearGroup = true;
    bool bRestartAttack = false;
    bool bstartAim = false;
    if (pActorRef &&
        (eseqSection == AGS_WEAPON || aeSection == AGS_WHOLE_BODY) &&
        pactor->IsWeaponDrawn() &&
        !pDataHandler->GetClearingData()) {
        if (abIronSightsToggle &&
            pactor->GetAnimAction() == ANIM_ACTION_ATTACK &&
            pactor->pCurrentProcess->GetAnimActionAnimSeq()) {
            bRestartAttack = true;
        }
        else {
            bstartAim = true;
        }
        if (eseqSection == AGS_WEAPON) {
            bclearGroup = false;
            if ((pactor->GetAnimAction() == ANIM_ACTION_RELOAD || pactor->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP) &&
                pactor != pPlayer &&
                fduration < 0.5) {
                if (pblendOutSeq->GetAnimGroup()->GetMove() == AGM_WALK) {
                    if (!pactor->IsSneaking()) {
                        fduration = 0.5f;
                    }
                }
                else if (pactor->IsSneaking()) {
                    fduration = 0.5f;
                }
            }
            ClearGroup(AGS_WEAPON_UP, fduration);
            ClearGroup(AGS_WEAPON_DOWN, fduration);
        }
    }

    if (bclearGroup) {
        ClearGroup(aeSection, fduration);
    }
    if (bRestartAttack) {
        ANIM_GROUP_ENUM eattackAnim = pactor->pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType();
        if (TESAnimGroup::IsIronSightsAction(eattackAnim)) {
            eattackAnim = static_cast<ANIM_GROUP_ENUM>(eattackAnim - 3);
        }
        pactor->StartWeaponGroup(eattackAnim, this);
    }
    else if (bstartAim) {
        pactor->StartWeaponGroup(ANIM_GROUP_AIM, this);
    }
}

// Animation::GetGroup is defined in ExtraDataObjects.cpp.
float Animation::GetGlobalTimeMultiplier() {
    return m_fGlobalTimeMultiplier;
}

void Animation::Update(TESObjectREFR* ownerObject, float fSecondsPassed, float forceTime) {
    ANIM_GROUP_ACTION eWeaponAction;
    BSAnimGroupSequence* pweapSeq;
    float fAppTimeMod = appTimer.fDelta * m_fGlobalTimeMultiplier;
    if (GetAnimsLoading()) {
        while (true) {
            if (Animation::iAnimationsCloned >= static_cast<uint32_t>(iAnimationClonePerLoop.Int())) {
                break;
            }
            KFModel* pkfModel = kfModelList.m_item;
            AddAnimation(pkfModel, false);
            pkfModel->DecUseCount();
            kfModelList.RemoveHead();
            if (!GetAnimsLoading()) {
                break;
            }
            ++Animation::iAnimationsCloned;
        }
    }
    else if (sQueuedReloadGroup != 255) {
        pweapSeq = pCurrentSequence[4];
        eWeaponAction = action[4];
        switch (eWeaponAction) {
            case AGA_SPECIAL_IDLE_LOOP_START:
            case AGA_SPECIAL_IDLE_LOOP_STOP: {
                // An attack still playing keeps the reload queued.
                if (pweapSeq && !pweapSeq->GetAnimGroup()->IsAimAnim() && pweapSeq->GetAnimGroup()->IsAttackAction()) {
                    break;
                }
                [[fallthrough]];
            }
            case AGA_SPECIAL_IDLE_STOP: {
                if (static_cast<Actor*>(pActorRef)->Actor::IsWeaponDrawn()) {
                    PlayGroup(sQueuedReloadGroup, ACTION_START, -1, AGS_NONE);
                    if (ownerObject == pPlayer) {
                        pPlayer->StartAnimOn1stPerson(sQueuedReloadGroup, ACTION_START);
                    }
                    Actor* pAct = static_cast<Actor*>(ownerObject);
                    bool bLoopReload = false;
                    if (pAct->GetCurrentAIProcess()) {
                        TESObjectWEAP* pWeap;
                        if (pAct->pCurrentProcess->GetCurrentWeapon()) {
                            pWeap = static_cast<TESObjectWEAP*>(
                                pAct->pCurrentProcess->GetCurrentWeapon()->pContainerObj
                            );
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
                [[fallthrough]];
            }
            default: {
                sQueuedReloadGroup = 255;
                break;
            }
        }
    }
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
    if (ownerObject && ownerObject->IsActor()) {
        pactor = static_cast<Actor*>(ownerObject);
    }

    for (NiPointer<AnimIdle>& spAnimIdle : spAnimIdleFreeWhenInactiveA) {
        if (spAnimIdle.m_pObject) {
            if (spAnimIdle->GetSequence()) {
                if (spAnimIdle->GetSequence()->GetState() == NiControllerSequence::kAnimating) {
                    TESIdleForm* pIdleForm = spAnimIdle->pIdleForm;
                    const char* pSeqName = spAnimIdle->GetSequence()->m_kName.GetHandleString();
                    const char* pIdleEditorID = pIdleForm->GetFormEditorID();
                    Warning(
                        "'%s' is trying to free a special idle '%s' sequence '%s' that is animating.",
                        pActorRef->GetFormEditorID(),
                        pIdleEditorID,
                        pSeqName
                    );
                    spAnimIdle = static_cast<AnimIdle*>(nullptr);
                }
                else if (!spAnimIdle->GetSequence()->GetState()) {
                    AnimIdleFree(spAnimIdle);
                }
            }
            else if (spAnimIdle->eFlags) {
                AnimIdleFree(spAnimIdle);
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
            float ft = forceTime == -1.0 ? time : forceTime;
            NiUpdateData kUpdateData(ft, false, false);
            pAnimRoot->Update(kUpdateData);
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
            if (egroup != ANIM_GROUP_NONE && pcurrentSequence) {
                double dLastScaledTime = pcurrentSequence->GetScaledTime(time);
                flastScaledTime[esection] = dLastScaledTime;
            }
        }
        if (forceTime != -1.0) {
            time = forceTime;
            NiUpdateData kUpdateData(time, true, false);
            pAnimRoot->Update(kUpdateData);
            return;
        }
        float timeDelta = fSecondsPassed * m_fGlobalTimeMultiplier;
        time = time + timeDelta;
        for (esection = AGS_IDLE; esection < ANIM_GROUP_SECTION_COUNT;
             esection = static_cast<ANIM_GROUP_SECTION>(esection + 1)) {
            BSAnimGroupSequence* pcurrentSequence = GetCurrentSequence(esection);
            bool bupdate = true;
            egroup = TESAnimGroup::GetType(group[esection]);
            enextGroup = TESAnimGroup::GetType(nextGroup[esection]);
            if (egroup != ANIM_GROUP_NONE && pcurrentSequence) {
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
                        if (esection != AGS_SPECIAL_IDLE && esection) {
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
                    if (pcurrentSequence->GetState() == NiControllerSequence::kAnimating) {
                        if (egroup < ANIM_GROUP_MOVE_FORWARD || egroup > ANIM_GROUP_TURN_RIGHT) {
                            if (egroup == ANIM_GROUP_EQUIP || egroup == ANIM_GROUP_UNEQUIP) {
                                double dPhase = pcurrentSequence->GetPhase();
                                float fNewPhase = dPhase + (timeDelta * m_fEquipModifier - timeDelta);
                                pcurrentSequence->SetPhase(fNewPhase, false);
                            }
                            else if (egroup < ANIM_GROUP_EQUIP || egroup > ANIM_GROUP_COUNTER) {
                                if (egroup >= ANIM_GROUP_RELOAD_START && egroup <= ANIM_GROUP_RELOAD_Z) {
                                    double dPhase = pcurrentSequence->GetPhase();
                                    float fNewPhase = dPhase + (timeDelta * m_fReloadModifier - timeDelta);
                                    pcurrentSequence->SetPhase(fNewPhase, false);
                                }
                            }
                            else {
                                double dPhase = pcurrentSequence->GetPhase();
                                float fNewPhase = dPhase + (timeDelta * m_fAttackSpeed - timeDelta);
                                pcurrentSequence->SetPhase(fNewPhase, false);
                            }
                        }
                        else {
                            double dPhase = pcurrentSequence->GetPhase();
                            float fNewPhase = dPhase + (timeDelta * this->GetMoveSpeed() - timeDelta);
                            pcurrentSequence->SetPhase(fNewPhase, false);
                        }
                    }
                }
                else {
                    double dPhase = pcurrentSequence->GetPhase();
                    float fNewPhase = dPhase - timeDelta;
                    pcurrentSequence->SetPhase(fNewPhase, false);
                }
                if (bupdate && pcurrentSequence->GetState() == NiControllerSequence::kAnimating) {
                    switch (animGroupInfo[egroup].eAction) {
                        case AGAT_START_STOP:
                        case AGAT_LOOP: {
                            if (pcurrentSequence->m_eCycleType) {
                                if (action[esection] == AGA_START) {
                                    double dTime = GetTime(pcurrentSequence);
                                    ANIM_GROUP_ACTION eAction = action[esection];
                                    if (pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime) {
                                        action[esection] = AGA_STOP;
                                    }
                                }
                                if (enextGroup == ANIM_GROUP_NONE) {
                                    double dTime = GetTime(pcurrentSequence);
                                    if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP) <= dTime) {
                                        BlendOut(esection, false);
                                    }
                                }
                                else {
                                    double dTime = GetTime(pcurrentSequence);
                                    if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP) <= dTime) {
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
                                    double dTime = GetTime(pcurrentSequence);
                                    ANIM_GROUP_ACTION eAction = action[esection];
                                    if (!(pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime)) {
                                        break;
                                    }
                                    eAction = action[esection];
                                    if (eAction) {
                                        if (eAction == AGA_STOP) {
                                            action[esection] = AGA_STOP;
                                        }
                                    }
                                    else {
                                        action[esection] = AGA_STOP;
                                    }
                                }
                                if (enextGroup == ANIM_GROUP_NONE) {
                                    double dTime = GetTime(pcurrentSequence);
                                    if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP) <= dTime &&
                                        loopCount[esection]) {
                                        action[esection] = AGA_START;
                                        double dEndKeyTime = pcurrentSequence->m_fEndKeyTime;
                                        float floopDelta = dEndKeyTime - pcurrentSequence->GetBeginKeyTime();
                                        float fNewPhase = pcurrentSequence->GetPhase() - floopDelta;
                                        pcurrentSequence->SetPhase(fNewPhase, false);
                                        if (loopCount[esection] != -1) {
                                            --loopCount[esection];
                                        }
                                    }
                                    else {
                                        double dTime = GetTime(pcurrentSequence);
                                        if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP) <= dTime) {
                                            BlendOut(esection, false);
                                        }
                                    }
                                }
                                else {
                                    double dTime = GetTime(pcurrentSequence);
                                    if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP) <= dTime) {
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
                                double dTime = GetTime(pcurrentSequence);
                                ANIM_GROUP_ACTION eAction = action[esection];
                                if (!(pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime)) {
                                    break;
                                }
                                eAction = action[esection];
                                if (eAction) {
                                    if (eAction == AGA_STOP) {
                                        action[esection] = AGA_SPECIAL_IDLE_LOOP_STOP;
                                    }
                                    else if (eAction == AGA_SPECIAL_IDLE_LOOP_STOP) {
                                        action[esection] = AGA_SPECIAL_IDLE_STOP;
                                    }
                                }
                                else {
                                    action[esection] = AGA_STOP;
                                }
                            }
                            if (enextGroup == ANIM_GROUP_NONE) {
                                double dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP) <= dTime &&
                                    loopCount[esection]) {
                                    ZeroGlobalTransform();
                                    action[esection] = AGA_STOP;
                                    double dLoopStopTime = pcurrentSequence->GetAnimGroup()->GetTime(
                                        AGA_SPECIAL_IDLE_LOOP_STOP
                                    );
                                    float floopDelta = dLoopStopTime -
                                        pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP);
                                    float fNewPhase = pcurrentSequence->GetPhase() - floopDelta;
                                    pcurrentSequence->SetPhase(fNewPhase, false);
                                    flastScaledTime[esection] = flastScaledTime[esection] - floopDelta;
                                    if (loopCount[esection] != -1 && loopCount[esection] < 255) {
                                        --loopCount[esection];
                                    }
                                }
                                else {
                                    double dTime = GetTime(pcurrentSequence);
                                    if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_SPECIAL_IDLE_STOP) <= dTime) {
                                        if (spAnimIdle->eType == AnimIdle::PLAY_TYPE_QUEUE) {
                                            spAnimIdle->eFlags = AnimIdle::ANIM_IDLE_DONE_PLAYING;
                                        }
                                        else {
                                            BlendOut(esection, false);
                                        }
                                    }
                                }
                            }
                            else {
                                double dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_SPECIAL_IDLE_STOP) <= dTime) {
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
                                double dTime = GetTime(pcurrentSequence);
                                ANIM_GROUP_ACTION eAction = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_LOOP_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP) <= dTime) {
                                    if (enextGroup == ANIM_GROUP_NONE) {
                                        BlendOut(esection, false);
                                        if (pactor &&
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
                                    else {
                                        loopCount[esection] = nextLoops[esection];
                                        StartGroup(nextGroup[esection], AGS_NONE);
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK: {
                            if (action[esection] < AGA_ATTACK_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                ANIM_GROUP_ACTION eAction = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_ATTACK_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_ATTACK_STOP) <= dTime) {
                                    if (enextGroup == ANIM_GROUP_NONE) {
                                        BlendOut(esection, false);
                                    }
                                    else {
                                        loopCount[esection] = nextLoops[esection];
                                        StartGroup(nextGroup[esection], AGS_NONE);
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK_THROW: {
                            if (action[esection] < AGA_ATTACK_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                ANIM_GROUP_ACTION eAction = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_ATTACK_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_ATTACK_STOP) <= dTime) {
                                    if (enextGroup == ANIM_GROUP_NONE) {
                                        BlendOut(esection, false);
                                    }
                                    else {
                                        loopCount[esection] = nextLoops[esection];
                                        StartGroup(nextGroup[esection], AGS_NONE);
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK_MINE: {
                            if (action[esection] < AGA_SPECIAL_IDLE_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                ANIM_GROUP_ACTION eAction = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_SPECIAL_IDLE_STOP) <= dTime) {
                                    if (enextGroup == ANIM_GROUP_NONE) {
                                        BlendOut(esection, false);
                                    }
                                    else {
                                        loopCount[esection] = nextLoops[esection];
                                        StartGroup(nextGroup[esection], AGS_NONE);
                                        enextGroup = ANIM_GROUP_NONE;
                                        nextGroup[esection] = 255;
                                    }
                                }
                            }
                            break;
                        }
                        case AGAT_ATTACK_SPIN: {
                            if (action[esection] < AGA_SPECIAL_IDLE_STOP) {
                                double dTime = GetTime(pcurrentSequence);
                                ANIM_GROUP_ACTION eAction = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime) {
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_LOOP_STOP && pactor && pactor->pCurrentProcess) {
                                if ((pactor->pCurrentProcess->IsSustainedFire() &&
                                     pactor->GetAnimAction() == ANIM_ACTION_ATTACK) ||
                                    (pActorRef == pPlayer &&
                                     pVATS.eMode == VATS::VATS_PLAYBACK &&
                                     pVATS.GetCurrentAction())) {
                                    double dLoopStopTime = pcurrentSequence->GetAnimGroup()->GetTime(
                                        AGA_SPECIAL_IDLE_LOOP_STOP
                                    );
                                    if (flastScaledTime[esection] <= dLoopStopTime) {
                                        dLoopStopTime = pcurrentSequence->GetAnimGroup()->GetTime(
                                            AGA_SPECIAL_IDLE_LOOP_STOP
                                        );
                                        for (float floopDelta =
                                                 dLoopStopTime - pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP);
                                             ;
                                             flastScaledTime[esection] = flastScaledTime[esection] - floopDelta) {
                                            double dTime = GetTime(pcurrentSequence);
                                            ANIM_GROUP_ACTION eAction = action[esection];
                                            if (!(pcurrentSequence->GetAnimGroup()->GetTime(eAction) < dTime)) {
                                                break;
                                            }
                                            float fNewPhase = pcurrentSequence->GetPhase() - floopDelta;
                                            pcurrentSequence->SetPhase(fNewPhase, false);
                                        }
                                        action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] - 1);
                                    }
                                }
                            }

                            if (bSpinAttackReleasedEarly && action[esection] == AGA_STOP) {
                                double dLoopStopTime = pcurrentSequence->GetAnimGroup()->GetTime(
                                    AGA_SPECIAL_IDLE_LOOP_STOP
                                );
                                if (flastScaledTime[esection] <= dLoopStopTime) {
                                    dLoopStopTime = pcurrentSequence->GetAnimGroup()->GetTime(
                                        AGA_SPECIAL_IDLE_LOOP_STOP
                                    );
                                    float floopDelta = dLoopStopTime -
                                        pcurrentSequence->GetAnimGroup()->GetTime(AGA_STOP);
                                    float fNewPhase = pcurrentSequence->GetPhase() + floopDelta;
                                    pcurrentSequence->SetPhase(fNewPhase, false);
                                    flastScaledTime[esection] = flastScaledTime[esection] + floopDelta;
                                    action[esection] = static_cast<ANIM_GROUP_ACTION>(action[esection] + 1);
                                }
                            }
                            if (action[esection] == AGA_SPECIAL_IDLE_STOP) {
                                double dTime;
                                dTime = GetTime(pcurrentSequence);
                                if (pcurrentSequence->GetAnimGroup()->GetTime(AGA_SPECIAL_IDLE_STOP) <= dTime) {
                                    if (enextGroup == ANIM_GROUP_NONE) {
                                        BlendOut(esection, false);
                                    }
                                    else {
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
            NiPoint3 posFakeDelta = NiPoint3::ZERO;
            if (pactor->IsMoving()) {
                uint16_t smoveMode = pactor->GetMoveMode();
                if (pCurrentSequence[1] && (smoveMode & 15) != 0) {
                    ANIM_GROUP_ENUM eanimGroup = ANIM_GROUP_IDLE;
                    bfakeDelta = pCurrentSequence[1]->GetAnimGroup()->IsJumpingLoopAnim();
                    if (pCurrentSequence[1]->GetAnimGroup()->GetType() == ANIM_GROUP_JUMP_START) {
                        bfakeDelta = true;
                    }
                    else {
                        int32_t iState = pCurrentSequence[1]->GetState();
                        if (iState == 2 || iState == 5) {
                            bfakeDelta = true;
                        }
                    }
                    NiTransformController* ptransCont = static_cast<NiTransformController*>(
                        pAccumRoot->GetController(&NiTransformController::ms_RTTI)
                    );
                    NiBlendInterpolator* pblendInterp = static_cast<NiBlendInterpolator*>(
                        ptransCont->GetInterpolator(0)
                    );
                    if (pblendInterp) {
                        int32_t ihighest = pblendInterp->GetHighestPriority();
                        const char* pAccumRootName = pAccumRoot->m_kName.GetHandleString();
                        if (pCurrentSequence[1]->GetPriority(pAccumRootName, nullptr) < ihighest) {
                            bfakeDelta = false;
                        }
                    }
                    if (bfakeDelta) {
                        if ((smoveMode & 512) != 0) {
                            if ((smoveMode & 1) != 0) {
                                eanimGroup = ANIM_GROUP_FAST_FORWARD;
                            }
                            else if ((smoveMode & 2) != 0) {
                                eanimGroup = ANIM_GROUP_FAST_BACK;
                            }
                            else if ((smoveMode & 4) != 0) {
                                eanimGroup = ANIM_GROUP_FAST_LEFT;
                            }
                            else if ((smoveMode & 8) != 0) {
                                eanimGroup = ANIM_GROUP_FAST_RIGHT;
                            }
                        }
                        else if ((smoveMode & 0xFF00) != 0) {
                            if ((smoveMode & 1) != 0) {
                                eanimGroup = ANIM_GROUP_MOVE_FORWARD;
                            }
                            else if ((smoveMode & 2) != 0) {
                                eanimGroup = ANIM_GROUP_MOVE_BACK;
                            }
                            else if ((smoveMode & 4) != 0) {
                                eanimGroup = ANIM_GROUP_MOVE_LEFT;
                            }
                            else if ((smoveMode & 8) != 0) {
                                eanimGroup = ANIM_GROUP_MOVE_RIGHT;
                            }
                        }
                        if (eanimGroup == pCurrentSequence[1]->GetAnimGroup()->GetType()) {
                            float fMoveSpeed = this->GetMoveSpeed();
                            float fMult = fAnimationMult.Float();
                            posFakeDelta = pCurrentSequence[1]->GetAnimGroup()->GetDeltaPerSecond() * fAppTimeMod *
                                fMult * fMoveSpeed;
                        }
                        else {
                            uint16_t sanimGroup = pactor->GetAnimGroup(eanimGroup, nullptr, false, nullptr);
                            eanimGroup = TESAnimGroup::GetType(sanimGroup);
                            if (eanimGroup) {
                                AnimSequenceBase* panimSeq;
                                if (pAnimSequenceMap->GetAt(sanimGroup, panimSeq)) {
                                    BSAnimGroupSequence* pseq = panimSeq->GetSequenceToPlay(-1);
                                    if (pseq) {
                                        float fMoveSpeed = this->GetMoveSpeed();
                                        float fMult = fAnimationMult.Float();
                                        posFakeDelta = pseq->GetAnimGroup()->GetDeltaPerSecond() * fAppTimeMod * fMult *
                                            fMoveSpeed;
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
            if (!bfakeDelta && !pCurrentSequence[1] && pLastMovementSequence && pLastMovementSequence->GetState()) {
                movementDelta.y = 0.0f;
                movementDelta.x = 0.0f;
            }

            if (pLastMovementSequence && !pLastMovementSequence->GetState()) {
                pLastMovementSequence = nullptr;
            }
        }
        bool bInDialog = false;
        if (pactor && pactor->pCurrentProcess) {
            bInDialog = pactor->pCurrentProcess->GetTalkingtoPC();
        }

        if (!bInDialog &&
            (!pAnimRoot->GetAppCulled() || (this == pPlayer->GetAnimation(true) && Interface::IsGunScopeVisible()))) {
            for (esection = AGS_IDLE; esection < ANIM_GROUP_SECTION_COUNT;
                 esection = static_cast<ANIM_GROUP_SECTION>(esection + 1)) {
                char cSkipUpdate = this->cSkipUpdate;
                if (cSkipUpdate == 20) {
                    continue;
                }
                if (cSkipUpdate == 21) {
                    if (esection >= AGS_LEFT_ARM && esection <= AGS_WEAPON) {
                        continue;
                    }
                }
                else if (cSkipUpdate == 23) {
                    if (esection != AGS_SPECIAL_IDLE && esection != AGS_IDLE) {
                        continue;
                    }
                }
                else if (esection == this->cSkipUpdate) {
                    continue;
                }
                BSAnimGroupSequence* pcurrentSequence = GetCurrentSequence(esection);
                if (pcurrentSequence &&
                    ppreSeq[esection] == pcurrentSequence &&
                    (pcurrentSequence->GetState() == NiControllerSequence::kAnimating ||
                     pcurrentSequence->GetState() == NiControllerSequence::kEaseIn ||
                     pcurrentSequence->GetState() == NiControllerSequence::kTransDest)) {
                    bool bplaySounds = true;
                    if (pSoundPriorityBoneName[esection]) {
                        if (pSoundPriorityBone[esection]) {
                            BSAnimGroupSequence* phighestSeq = nullptr;
                            uint8_t cpriority = 0;
                            for (ANIM_GROUP_SECTION etestSection = AGS_IDLE; etestSection < ANIM_GROUP_SECTION_COUNT;
                                 etestSection = static_cast<ANIM_GROUP_SECTION>(etestSection + 1)) {
                                if (pCurrentSequence[etestSection] &&
                                    pSoundPriorityBone[esection] &&
                                    pSoundPriorityBone[esection]->m_kName.GetHandleString()) {
                                    uint8_t ctestPriority = pCurrentSequence[etestSection]
                                                                ->GetAnimGroup()
                                                                ->GetSectionPriority(esection);
                                    if (ctestPriority > static_cast<int32_t>(cpriority)) {
                                        phighestSeq = pCurrentSequence[etestSection];
                                        cpriority = ctestPriority;
                                    }
                                }
                            }
                            if (!phighestSeq || phighestSeq != pcurrentSequence) {
                                bplaySounds = false;
                            }
                        }
                    }
                    else {
                        bplaySounds = false;
                    }
                    if (bplaySounds) {
                        float fScaledTime = pcurrentSequence->GetScaledTime(time);
                        if (fScaledTime != -std::numeric_limits<float>::max() &&
                            flastScaledTime[esection] != -std::numeric_limits<float>::max() &&
                            flastScaledTime[esection] != fScaledTime) {
                            float fPrevScaledTime = flastScaledTime[esection];
                            pcurrentSequence->GetAnimGroup()
                                ->PlaySounds(ownerObject, fPrevScaledTime, fScaledTime, pcurrentSequence);
                        }
                    }
                }
            }
        }
        if (spAnimIdle.m_pObject) {
            spAnimIdle->Update(this);
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
            MEM_CONTEXT_ANIMATION,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
            750
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
    else if (aeSection == AGS_UPPER_BODY) {
        esection = AGS_WEAPON;
    }
    if (aeNewAction == AGA_NONE) {
        AnimSequenceBase* panimSeq;
        ClearGroup(aeSection, 0.0f);
        if (asNewGroup != 255 &&
            pAnimSequenceMap->GetAt(asNewGroup, panimSeq) &&
            panimSeq->GetSequenceToPlay(aiSequenceIndex)) {
            pCurrentSequence[esection] = panimSeq->GetSequenceToPlay(aiSequenceIndex);
            spManager->ActivateSequence(pCurrentSequence[esection], 0, false, 1.0f, 0.0f, nullptr);
        }
    }
    else if (GroupLoaded(asNewGroup)) {
        AnimSequenceBase* panimSeq;
        if (pAnimSequenceMap->GetAt(asNewGroup, panimSeq)) {
            BSAnimGroupSequence* pseq = panimSeq->GetSequenceToPlay(aiSequenceIndex);
            StartGroup(pseq, asNewGroup, aeSection);
        }
        action[esection] = aeNewAction;
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
    NiSmartPointerCast<BSBipNode>(pAnimRoot)->UpdateBipOnly(afTime, pkAccumRoot);
    if (pAccumRoot) {
        if (apTranslate) {
            *apTranslate = pAccumRoot->GetTranslate();
        }
        pAccumRoot->SetTranslate(NiPoint3::ZERO);
    }
}

void Animation::UpdateSceneGraphNoController() {
    if (pAnimRoot.m_pObject && pAccumRoot) {
        if (iNumHWThreads.Int() <= 1 ||
            cSkipThreadQueue ||
            GetDelayUpdateScenegraph() ||
            pSaveLoadGame->GetSaveGameLoading()) {
            NiUpdateData updateData(time, true, false);
            NiPointer<NiTimeController> spAnimRootCon(pAnimRoot->GetControllers());
            NiPointer<NiTimeController> spAccumRootCon(pAccumRoot->GetControllers());
            pAnimRoot->SetControllers(nullptr);
            pAccumRoot->SetControllers(nullptr);
            pAnimRoot->UpdateDownwardPass(updateData, 0);

            pAnimRoot->SetControllers(spAnimRootCon.m_pObject);
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
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        4797
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

// The reference's base form is compared with NPC__ID and the result is stored in a local that is never read.
// The second GetAt deliberately does not test pAnimSequenceMap for null.
bool Animation::Init(BSSimpleList<const char*>* apFileList, NiNode* apRoot, TESObjectREFR* apRef, bool abUpdate3D) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        568
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
        pAccumRoot->SetRotate(NiMatrix3::IDENTITY);
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
        char acPath[264];
        const char* pfilename = pfileList->m_item;
        BSsprintf(acPath, 260, "%s\\%s\\%s", apPath, "SpecialAnims", pfilename);
        KFModel* pkfModel = pModelLoader->LoadKF(acPath);
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
        pAnimRoot == pPlayer->PlayerCharacter::Get3D(false) ||
        pAnimRoot == pPlayer->PlayerCharacter::Get3D(true) ||
        pAnimRoot == pPlayer->GetInventory3D()
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
        return false;
    }
    eanimGroup = spanimGroup->GetType();
    sanimGroup = spanimGroup->sType;
    if (eanimGroup == ANIM_GROUP_NONE) {
        if (apKFModel->GetSequence()) {
            const char* pFilename = apKFModel->pFilename;
            Warning(
                "ANIMATION: Animation sequence '%s' not found in TESAnimGroup::GetSequenceType for file '%s'.",
                apKFModel->GetSequence()->m_kName.GetHandleString(),
                pFilename
            );
        }
        return false;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        794
    );
    AnimSequenceBase* panimSeq = nullptr;
    if (!pAnimSequenceMap->GetAt(sanimGroup, panimSeq)) {
        panimSeq = new AnimSequenceSingle;
        pAnimSequenceMap->SetAt(sanimGroup, panimSeq);
    }
    if (panimSeq->IsSingleSeq() && panimSeq->GetSequenceToPlay(-1)) {
        if (animGroupInfo[eanimGroup].bMultipleSequences) {
            pAnimSequenceMap->RemoveAt(sanimGroup);
            AnimSequenceSingle* panimSingle = static_cast<AnimSequenceSingle*>(panimSeq);
            panimSeq = new AnimSequenceMultiple(panimSingle);
            pAnimSequenceMap->SetAt(sanimGroup, panimSeq);
        }
        else {
            TESAnimGroup* pAnimGroup;
            pAnimGroup = panimSeq->GetSequenceToPlay(-1)->GetAnimGroup();
            if (pAnimGroup == apKFModel->GetAnimGroup()) {
                return true;
            }
            bool bremoveSequence = true;
            if (spAnimIdleFreeWhenInactiveA[0].m_pObject) {
                BSAnimGroupSequence* pFreeSeq = spAnimIdleFreeWhenInactiveA[0]->GetSequence();
                bremoveSequence = pFreeSeq != panimSeq->GetSequenceToPlay(-1);
            }
            if (bremoveSequence && spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                BSAnimGroupSequence* pIdleSeq = spAnimIdleFreeWhenInactiveA[1]->GetSequence();
                if (pIdleSeq == panimSeq->GetSequenceToPlay(-1)) {
                    bremoveSequence = false;
                }
            }
            if (bremoveSequence) {
                if (panimSeq->GetSequenceToPlay(-1) == pLastMovementSequence) {
                    pLastMovementSequence = nullptr;
                }
                BSAnimGroupSequence* pPlaying = panimSeq->GetSequenceToPlay(-1);
                spManager->RemoveSequence(pPlaying);
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
            if (pLastMovementSequence && panimSeq->GetSequenceToPlay(-1) == pLastMovementSequence) {
                pLastMovementSequence = nullptr;
            }

            panimSeq->SetSequence(nullptr, true);
        }
    }
    if (abDelaySequenceClone &&
        !Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0) &&
        eanimGroup != ANIM_GROUP_HOLSTER &&
        eanimGroup &&
        eanimGroup != ANIM_GROUP_DEATH) {
        apKFModel->IncManualRefCount();
        kfModelList.AddTail(apKFModel);
        return true;
    }
    NiControllerSequence* pkSequence = apKFModel->GetSequence();
    if (eanimGroup >= ANIM_GROUP_MOVE_FORWARD &&
        eanimGroup <= ANIM_GROUP_TURN_RIGHT &&
        pkSequence->m_eCycleType == NiControllerSequence::CYCLE_CLAMP) {
        pAnimSequenceMap->RemoveAt(sanimGroup);
        if (panimSeq) {
            delete panimSeq;
        }
        return false;
    }

    BSAnimGroupSequence* pagSeq = nullptr;
    bool bbsSeq = NiObject::IsKindOf_0(&BSAnimGroupSequence::ms_RTTI, pkSequence);
    if (bbsSeq && pkSequence->m_uiRefCount == 1) {
        pagSeq = static_cast<BSAnimGroupSequence*>(pkSequence);
    }
    else {
        pagSeq = new BSAnimGroupSequence(apKFModel->GetAnimGroup(), pkSequence);
    }
    panimSeq->SetSequence(pagSeq, true);
    if (spManager->AddSequence(pagSeq, EmptyFS, true)) {
        if (!pAccumRoot) {
            pAccumRoot = static_cast<NiNode*>(spManager->GetAccumRoot());
        }
        AddGroup(apKFModel->GetAnimGroup());
        return true;
    }
    bool bdeleteAnimSeq = false;
    const char* pRootName = pAnimRoot->m_kName.GetHandleString();
    Warning(
        "ANIMATION: Unable to add '%s' to keyframe manager on '%s'.\r\nMake sure the animation is not skinned "
        "to bones that don't exist in our skeleton.",
        pagSeq->m_kName.GetHandleString(),
        pRootName
    );
    MessageHandler::IncDisableWarningCount(true);
    for (uint32_t n = 0; n < pagSeq->m_uiArraySize; ++n) {
        // Departure: the name is written into a buffer; the exe passes a null pointer, so the lookup and warning saw no
        // name.
        char acName[264];
        pagSeq->GetObjectNameAt(n, acName);
        if (pAnimRoot->GetObjectByName(acName) == nullptr) {
            Warning("ANIMATION: Object '%s' in sequence but not skeleton.", acName);
        }
    }
    MessageHandler::IncDisableWarningCount(false);
    if (pagSeq) {
        if (pagSeq == pLastMovementSequence) {
            pLastMovementSequence = nullptr;
        }
        spManager->RemoveSequence(pagSeq);
        bdeleteAnimSeq = panimSeq->RemoveSequence(pagSeq, true);
    }
    if (panimSeq && bdeleteAnimSeq) {

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

// The reference is an actor.
void Animation::UpdateMovementNoWorldUpdate(TESObjectREFR* apRefObject) {
    Actor* apRef = static_cast<Actor*>(apRefObject);
    if (!pAnimRoot || !pAccumRoot) {
        return;
    }
    if (GetMoved()) {
        return;
    }
    NiNode* pActorNode = apRef->Get3D();
    BSFadeNode* pFadeNode = pActorNode ? pActorNode->IsFadeNode() : nullptr;
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
    NiSmartPointerCast<BSBipNode>(pAnimRoot)->UpdateAllButBip(time);
    movementDelta.z = pAccumRoot->GetAt(0)->GetTranslate().z - fOldZ;
    SetMoved(true);
}

// Each blend value is rounded to float before the compare.
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
        pAnimSequenceMap->GetAt(sanimGroup, panimSeq) &&
        eanimGroup != ANIM_GROUP_DYNAMIC_IDLE &&
        eanimGroup != ANIM_GROUP_SPECIAL_IDLE) {
        if (spManager->GetAccumRoot()) {
            bool bcheckForMovement = false;
            if (panimSeq->GetSequenceToPlay(-1) && !animGroupInfo[eanimGroup].bMultipleSequences) {
                switch (animGroupInfo[eanimGroup].eAction) {
                    case AGAT_START_STOP:
                    case AGAT_LOOP:
                    case AGAT_SPECIAL_IDLE_LOOP:
                    case AGAT_ATTACK:
                    case AGAT_ATTACK_POWER:
                    case AGAT_ATTACK_THROW:
                    case AGAT_ATTACK_MINE:
                    case AGAT_ATTACK_SPIN: {
                        bcheckForMovement = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            if (bcheckForMovement) {
                NiPointer<BSAnimGroupSequence> spseq(panimSeq->GetSequenceToPlay(-1));
                for (int32_t n = 0; n < spseq->m_uiArraySize; ++n) {
                    NiAVObject* pTarget = static_cast<NiAVObject*>(spseq->GetTargetAt(static_cast<uint32_t>(n)));
                    if (pTarget == spseq->m_pkAccumRoot) {
                        NiInterpolator* pinterp = spseq->GetInterpolatorAt(static_cast<uint32_t>(n));
                        float fstartTime = spseq->GetBeginKeyTime();
                        float fstopTime = spseq->m_fEndKeyTime;
                        NiQuatTransform quatStart;
                        NiQuatTransform quatStop;
                        pinterp->Update(fstartTime, nullptr, quatStart);
                        pinterp->Update(fstopTime, nullptr, quatStop);
                        if (quatStart.IsTranslateValid() && quatStop.IsTranslateValid()) {
                            ::new (&delta) NiPoint3(quatStop.m_kTranslate - quatStart.m_kTranslate);
                            float fDuration = fstopTime - fstartTime;
                            delta /= fDuration;
                            apAnimGroup->SetSpeed(&delta);
                        }
                        break;
                    }
                }
            }
            ANIM_GROUP_ENUM eType = TESAnimGroup::GetType(sanimGroup);
            if (eType >= ANIM_GROUP_MOVE_FORWARD && eType <= ANIM_GROUP_DODGE_RIGHT && apAnimGroup->GetSpeed() == 0.0) {
                const char* pRootName = pAnimRoot->m_kName.GetHandleString();
                const char* pName = animGroupInfo[apAnimGroup->GetType()].pName;
                const char* pWeaponName = animWeaponInfo[apAnimGroup->GetWeapon()];
                Warning(
                    "ANIMATION: AnimGroup '%s%s%s' for '%s' was exported with 'Animate in Place' from MAX.\r\n",
                    animMoveInfo[apAnimGroup->GetMove()],
                    pWeaponName,
                    pName,
                    pRootName
                );
            }
        }
        else {
            Warning("ANIMATION: Could not find AccumRoot in animation '%s'.", pAnimRoot->m_kName.GetHandleString());
        }
    }
}

void Animation::SpecialIdleFree(bool abFreeQueued, bool abForceDelete) {
    if (spAnimIdle.m_pObject) {
        ANIM_GROUP_SECTION esection = spAnimIdle->eSection;
        if (esection == AGS_WHOLE_BODY) {
            esection = AGS_MOVEMENT;
        }
        else if (esection == AGS_UPPER_BODY) {
            esection = AGS_WEAPON;
        }
        if (spAnimIdle->GetSequence() && pCurrentSequence[esection] == spAnimIdle->GetSequence()) {
            if (abForceDelete) {
                pCurrentSequence[esection] = nullptr;
            }
            else {
                int32_t iState = spAnimIdle->GetSequence()->GetState();
                if (iState == 2 || iState == 5) {
                    Warning(
                        "ANIMATION: Trying to SpecialIdleFree an animation '%s' that just started.",
                        spAnimIdle->GetSequence()->m_kName.GetHandleString()
                    );
                    return;
                }
                BlendOut(spAnimIdle->eSection, false);
            }
        }
        if (abForceDelete || spAnimIdleFreeWhenInactiveA[0].m_pObject) {
            if (abForceDelete || spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                AnimIdleFree(spAnimIdle);
            }
            else {
                spAnimIdleFreeWhenInactiveA[1] = spAnimIdle;
                spAnimIdle = static_cast<AnimIdle*>(nullptr);
            }
        }
        else {
            spAnimIdleFreeWhenInactiveA[0] = spAnimIdle;
            spAnimIdle = static_cast<AnimIdle*>(nullptr);
        }
    }
    if (abFreeQueued && spAnimIdleQueued.m_pObject) {
        ANIM_GROUP_SECTION esection = spAnimIdleQueued->eSection;
        if (esection == AGS_WHOLE_BODY) {
            esection = AGS_MOVEMENT;
        }
        else if (esection == AGS_UPPER_BODY) {
            esection = AGS_WEAPON;
        }
        if (spAnimIdleQueued->GetSequence() && pCurrentSequence[esection] == spAnimIdleQueued->GetSequence()) {
            if (abForceDelete) {
                pCurrentSequence[esection] = nullptr;
            }
            else {
                int32_t iState = spAnimIdleQueued->GetSequence()->GetState();
                if (iState == 2 || iState == 5) {
                    Warning(
                        "ANIMATION: Trying to SpecialIdleFree an animation '%s' that just started.",
                        spAnimIdleQueued->GetSequence()->m_kName.GetHandleString()
                    );
                    return;
                }
                BlendOut(esection, false);
            }
        }
        if (abForceDelete || spAnimIdleFreeWhenInactiveA[0].m_pObject) {
            if (abForceDelete || spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                AnimIdleFree(spAnimIdleQueued);
            }
            else {
                spAnimIdleFreeWhenInactiveA[1] = spAnimIdleQueued;
                spAnimIdleQueued = static_cast<AnimIdle*>(nullptr);
            }
        }
        else {
            spAnimIdleFreeWhenInactiveA[0] = spAnimIdleQueued;
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
    AccumRootTranslate = NiPoint3::ZERO;
}

void Animation::SkipNextBlend() {
    cSkipNextBlend = 1;
}

BSAnimGroupSequence* Animation::SpecialIdleGetSequence() {
    if (!spAnimIdle) {
        return nullptr;
    }
    return spAnimIdle->AnimIdle::GetSequence();
}

void Animation::SetMoveSpeed(float moveSpeed) {
    m_fMoveSpeed = moveSpeed;
}

void Animation::ClearDelta() {
    movementDelta = NiPoint3::ZERO;
}

BSAnimGroupSequence* Animation::GetSpecialIdleSequence() {
    if (!spAnimIdle) {
        return nullptr;
    }
    return spAnimIdle->AnimIdle::GetSequence();
}

void Animation::SetLoopCount(ANIM_GROUP_SECTION aeSection, int32_t aiCount) {
    loopCount[static_cast<uint32_t>(aeSection)] = aiCount;
}

void Animation::AddGroup(TESAnimGroup* pAnimGroup) {
    if (pAnimGroup) {
        if (group[1] == 255 && nextGroup[1] == 255 && !pAnimGroup->sType) {

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
        spManager->DeactivateAll(0.0);
    }
    NiAVObject* pSceneRoot = GetSceneRoot();
    Animation::ClearControllersInterpolators(pSceneRoot);
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
        spManager->DeactivateAll(0.0f);
    }
    for (IDLE_REPLAY_DELAY*& rpItem : replayDelayList) {
        ourFree(rpItem);
    }
    replayDelayList.RemoveAll();
}

void Animation::SaveGameBGS(BGSSaveGameBuffer* apSaveGameBuffer) {
    BSAnimGroupSequence* pIdleSequence;
    NiPointer<AnimIdle>* pspSavedIdle;
    char cSequenceIndex;
    // GetAnimAction's result is stored as a 16-bit value and those two bytes are saved.
    int16_t sAnimAction;
    char cIdleSequenceSection;
    char cSavedSections;
    char cActionSequenceSection;
    Actor* pActor = apSaveGameBuffer->GetActor();
    apSaveGameBuffer->SaveDataEndian(&m_fMoveSpeed, sizeof(m_fMoveSpeed), 0);
    apSaveGameBuffer->SaveDataEndian(&m_fAttackSpeed, sizeof(m_fAttackSpeed), 0);
    apSaveGameBuffer->SaveDataEndian(&movementDelta, sizeof(movementDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&m_fLooking, sizeof(m_fLooking), 0);
    apSaveGameBuffer->SaveDataEndian(&cSkipUpdate, sizeof(cSkipUpdate), 0);
    apSaveGameBuffer->SaveDataEndian(&fLipTime, sizeof(fLipTime), 0);
    apSaveGameBuffer->SaveDataEndian(&sQueuedReloadGroup, sizeof(sQueuedReloadGroup), 0);
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<IDLE_REPLAY_DELAY*>* pNode = &replayDelayList; pNode; pNode = pNode->GetNext()) {
        IDLE_REPLAY_DELAY* pDelay = pNode->GetItem();
        if (pDelay) {
            apSaveGameBuffer->SaveFormID(pDelay->pIdleForm, 0);
            apSaveGameBuffer->SaveDataEndian(&pDelay->fDelay, sizeof(pDelay->fDelay), 0);
            ++aiVariableValue;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
    pspSavedIdle = spAnimIdleQueued.m_pObject ? &spAnimIdleQueued : &spAnimIdle;
    AnimIdle* pAnimIdle = pspSavedIdle->m_pObject;
    // The idle form is a trivial accessor of pIdleForm.
    if (pAnimIdle) {
        TESForm* pIdleForm = pAnimIdle->pIdleForm;
        apSaveGameBuffer->SaveFormID(pIdleForm, 0);
    }
    else {
        apSaveGameBuffer->SaveFormID(static_cast<TESForm*>(nullptr), 0);
    }
    if (pAnimIdle && pAnimIdle->pIdleForm) {
        uint32_t uiSizePosition = apSaveGameBuffer->StartVariableSizedValue();
        int32_t iStartPosition = apSaveGameBuffer->iBufferPosition;
        pAnimIdle->SaveGame(static_cast<BGSSaveFormBuffer*>(apSaveGameBuffer), this);
        apSaveGameBuffer->SaveVariableSizedValue(apSaveGameBuffer->iBufferPosition - iStartPosition, uiSizePosition);
    }

    BSAnimGroupSequence* pSkippedSequence = nullptr;
    if (spAnimIdleQueued.m_pObject && spAnimIdle.m_pObject) {
        pSkippedSequence = spAnimIdle->GetSequence();
    }

    cSavedSections = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        if ((!pSkippedSequence || pCurrentSequence[i] != pSkippedSequence) && group[i] != 255 && group[i] != 0xFFFF) {
            cSavedSections |= 1 << i;
        }
    }
    apSaveGameBuffer->SaveDataEndian(&cSavedSections, sizeof(cSavedSections), 0);
    cActionSequenceSection = -1;
    sAnimAction = static_cast<int16_t>(pActor->pCurrentProcess->GetAnimAction());
    BSAnimGroupSequence* pActionSequence = pActor->pCurrentProcess->GetAnimActionAnimSeq();
    cIdleSequenceSection = -1;
    pIdleSequence = pAnimIdle ? pAnimIdle->GetSequence() : nullptr;
    for (uint32_t j = 0; j < 8; ++j) {
        if ((!pSkippedSequence || pCurrentSequence[j] != pSkippedSequence) && group[j] != 255 && group[j] != 0xFFFF) {
            apSaveGameBuffer->SaveDataEndian(&group[j], sizeof(group[j]), 0);
            apSaveGameBuffer->SaveDataEndian(&action[j], sizeof(action[j]), 0);
            apSaveGameBuffer->SaveDataEndian(&nextGroup[j], sizeof(nextGroup[j]), 0);
            apSaveGameBuffer->SaveDataEndian(&loopCount[j], sizeof(loopCount[j]), 0);
            apSaveGameBuffer->SaveDataEndian(&nextLoops[j], sizeof(nextLoops[j]), 0);
            AnimSequenceBase* pAnimSeq = nullptr;
            cSequenceIndex = -1;
            if (pAnimSequenceMap->GetAt(group[j], pAnimSeq)) {
                cSequenceIndex = pAnimSeq->GetSequenceIndex(pCurrentSequence[j]);
            }
            if (!pCurrentSequence[j]) {
                cSequenceIndex = -2;
            }
            apSaveGameBuffer->SaveDataEndian(&cSequenceIndex, sizeof(cSequenceIndex), 0);
            if (pCurrentSequence[j]) {
                pCurrentSequence[j]->SaveGame(apSaveGameBuffer, time);
            }
            if (pCurrentSequence[j] == pActionSequence) {
                cActionSequenceSection = j;
            }
            if (pCurrentSequence[j] == pIdleSequence) {
                cIdleSequenceSection = j;
            }
        }
    }
    apSaveGameBuffer->SaveDataEndian(&cSkipNextBlend, sizeof(cSkipNextBlend), 0);
    apSaveGameBuffer->SaveDataEndian(&sAnimAction, sizeof(sAnimAction), 0);
    apSaveGameBuffer->SaveDataEndian(&cActionSequenceSection, sizeof(cActionSequenceSection), 0);
    apSaveGameBuffer->SaveDataEndian(&cIdleSequenceSection, sizeof(cIdleSequenceSection), 0);
}

void Animation::SpecialIdleQueue(TESIdleForm* apIdleForm, ANIM_GROUP_SECTION aeSection) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
        0x125D
    );
    if (spAnimIdle.m_pObject) {
        ANIM_GROUP_SECTION esection = spAnimIdle->eSection;
        if (esection == AGS_WHOLE_BODY) {
            esection = AGS_MOVEMENT;
        }
        else if (esection == AGS_UPPER_BODY) {
            esection = AGS_WEAPON;
        }
        if (spAnimIdle->GetSequence() && pCurrentSequence[esection] == spAnimIdle->GetSequence()) {
            int32_t iState = spAnimIdle->GetSequence()->GetState();
            if (iState == 2 || iState == 5) {
                Warning(
                    "ANIMATION: Trying to SpecialIdleQueue an animation '%s' that just started.",
                    spAnimIdle->GetSequence()->m_kName.GetHandleString()
                );
                return;
            }
            BlendOut(spAnimIdle->eSection, false);
        }
        if (spAnimIdleFreeWhenInactiveA[0].m_pObject) {
            if (spAnimIdleFreeWhenInactiveA[1].m_pObject) {
                AnimIdleFree(spAnimIdle);
            }
            else {
                spAnimIdleFreeWhenInactiveA[1] = spAnimIdle;
                spAnimIdle = static_cast<AnimIdle*>(nullptr);
            }
        }
        else {
            spAnimIdleFreeWhenInactiveA[0] = spAnimIdle;
            spAnimIdle = static_cast<AnimIdle*>(nullptr);
        }
    }
    AnimIdle* pIdle = new AnimIdle(
        apIdleForm,
        aeSection,
        AnimIdle::PLAY_TYPE_QUEUE,
        static_cast<Actor*>(pActorRef),
        false,
        nullptr
    );
    spAnimIdle = pIdle;
}

uint16_t Animation::GetAnimationSaveSize(TESObjectREFR* apRef) {
    int16_t sIdleSize;
    int16_t sBlockSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sBlockSize = 6;
    }
    int16_t sSize = sBlockSize + 26;
    for (char i = 0; i < 8; ++i) {
        if (group[i] != 255 && group[i] != 0xFFFF) {
            sSize += 17;
            if (pCurrentSequence[i]) {
                sSize += pCurrentSequence[i]->GetSaveSize();
            }
        }
    }
    Actor* apActor = nullptr;
    if (apRef && apRef->IsActor()) {
        apActor = static_cast<Actor*>(apRef);
    }

    if (spAnimIdleQueued.m_pObject) {
        sIdleSize = AnimIdle::GetAnimIdleSaveSize(apActor, spAnimIdleQueued.m_pObject) + sSize;
    }
    else {
        sIdleSize = AnimIdle::GetAnimIdleSaveSize(apActor, spAnimIdle.m_pObject) + sSize;
    }
    uint16_t isize = sIdleSize + 1;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        ++isize;
    }
    if (apActor->GetCurrentSpell()) {
        NiAVObjectPalette* pPalette = GetControllerManager()->GetObjectPalette();
        NiNode* pMagicObject = static_cast<NiNode*>(pPalette->GetAVObject(FixedStrings::QMagicNode()));
        if (pMagicObject) {
            NiNode* pMagicNode = pMagicObject->IsNode();
            if (pMagicNode) {
                NiAVObject* pChild = pMagicNode->GetAtChecked(0);
                if (pChild) {
                    NiControllerManager* pCastManager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pChild->GetControllers())
                    );
                    if (pCastManager) {
                        NiFixedString* pCastName = get_g_SpecialIdle_Cast();
                        BSAnimGroupSequence* pCastSequence = static_cast<BSAnimGroupSequence*>(
                            pCastManager->GetSequenceByName(*pCastName)
                        );
                        if (pCastSequence) {
                            isize += pCastSequence->GetSaveSize() + 4;
                        }
                    }
                }
            }
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pFormHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pFormHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pFormHeader->iFormID,
                pForm->GetFormEditorID(),
                pFormHeader->iFlags,
                0x15B8,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                0x15B8,
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
    const char* plowerBodyBone = pSoundPriorityBone[1]->m_kName.GetHandleString();
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
        pkeyText = pkeys[i].m_kText.GetHandleString();
        if (pkeyText && !_strnicmp(pkeyText, apSearchKey, isearchKeyLength)) {
            double dKeyTime = pkeys[i].m_fTime;
            if (fcurrentTime <= dKeyTime) {

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
    spAnimIdle->pActor = apActor;
    BSAnimGroupSequence* pSequence = spAnimIdle->GetSequence();
    apActor->SetAnimAction(ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE, pSequence);
    return true;
}

void Animation::AnimIdleFree(NiPointer<AnimIdle>& aspAnimIdle) {
    KFModel* pkfModel = aspAnimIdle->GetKFModel();
    AnimSequenceBase* panimSeq = nullptr;
    uint16_t sanimGroup = 255;
    NiPointer<BSAnimGroupSequence> spfadeSeq(static_cast<BSAnimGroupSequence*>(nullptr));
    if (pkfModel && pkfModel->GetAnimGroup()) {
        sanimGroup = pkfModel->GetAnimGroup()->sType;
        pAnimSequenceMap->GetAt(sanimGroup, panimSeq);
        bool bplaying = true;
        if (panimSeq) {
            BSAnimGroupSequence* pPlayingSeq = panimSeq->GetSequenceToPlay(-1);
            bplaying = pPlayingSeq == aspAnimIdle->GetSequence();
        }
        if (bplaying) {
            // An idle of AGS_WHOLE_BODY (20) or AGS_UPPER_BODY (21) indexes pCurrentSequence past its 8 entries,
            // deliberately reading what follows the array: spAnimIdleFreeWhenInactiveA[1] (20)
            // and replayDelayList's first item (21). Those are read by name; any other section past the array
            // matches nothing.
            uint32_t uiSection = aspAnimIdle->eSection;
            const void* pCurrent;
            bool bInExeObject = true;
            if (uiSection < ANIM_GROUP_SECTION_COUNT) {
                pCurrent = pCurrentSequence[uiSection];
            }
            else if (uiSection == AGS_WHOLE_BODY) {
                pCurrent = spAnimIdleFreeWhenInactiveA[1].m_pObject;
            }
            else if (uiSection == AGS_UPPER_BODY) {
                pCurrent = replayDelayList.GetItem();
            }
            else {
                pCurrent = nullptr;
                bInExeObject = false;
            }
            if (bInExeObject && pCurrent == aspAnimIdle->GetSequence()) {
                ClearGroup(aspAnimIdle->eSection, 0.0f);
            }
        }
        else {
            panimSeq = nullptr;
        }
    }

    if (!panimSeq) {
        spfadeSeq = aspAnimIdle->GetSequence();
    }
    if (!bShutDown) {
        if (panimSeq) {
            if (panimSeq->GetSequenceToPlay(-1) == pLastMovementSequence) {
                pLastMovementSequence = nullptr;
            }
            BSAnimGroupSequence* pPlaying = panimSeq->GetSequenceToPlay(-1);
            spManager->RemoveSequence(pPlaying);
        }
        else if (spfadeSeq.m_pObject) {
            if (spfadeSeq == pLastMovementSequence) {
                pLastMovementSequence = nullptr;
            }
            spManager->RemoveSequence(spfadeSeq.m_pObject);
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, sizeof(iBlockID));
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fMoveSpeed, sizeof(m_fMoveSpeed));
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fAttackSpeed, sizeof(m_fAttackSpeed));
    pSaveLoadGameOLD->SaveGameDataOLD(&movementDelta, sizeof(movementDelta));
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fLooking, sizeof(m_fLooking));
    pSaveLoadGameOLD->SaveGameDataOLD(&cSkipUpdate, sizeof(cSkipUpdate));
    cSlotMask = 0;
    for (i = 0; i < 8; ++i) {
        if (group[i] != 255 && group[i] != 0xFFFF) {
            cSlotMask |= 1 << i;
        }
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cSlotMask, sizeof(cSlotMask));
    for (i = 0; i < 8; ++i) {
        if (group[i] != 255 && group[i] != 0xFFFF) {
            AnimSequenceBase* pAnimSeq;
            pSaveLoadGameOLD->SaveGameDataOLD(&group[i], sizeof(group[i]));
            pSaveLoadGameOLD->SaveGameDataOLD(&action[i], sizeof(action[i]));
            pSaveLoadGameOLD->SaveGameDataOLD(&loopCount[i], sizeof(loopCount[i]));
            pSaveLoadGameOLD->SaveGameDataOLD(&nextGroup[i], sizeof(nextGroup[i]));
            pSaveLoadGameOLD->SaveGameDataOLD(&nextLoops[i], sizeof(nextLoops[i]));
            cSequenceIndex = -1;
            if (pAnimSequenceMap->GetAt(group[i], pAnimSeq)) {
                cSequenceIndex = pAnimSeq->GetSequenceIndex(pCurrentSequence[i]);
            }
            if (!pCurrentSequence[i]) {
                ANIM_GROUP_ACTION eAction = action[i];
                int32_t iGroup = group[i];
                int32_t iSlot = i;
                int32_t iFormID = apForm->iFormID;
                Warning(
                    "SAVELOAD: %s %08X has a sequence in slot %i with group %i and action %i, but the sequence is "
                    "NULL.",
                    apForm->GetFormEditorID(),
                    iFormID,
                    iSlot,
                    iGroup,
                    eAction
                );
                cSequenceIndex = -2;
            }
            pSaveLoadGameOLD->SaveGameDataOLD(&cSequenceIndex, sizeof(cSequenceIndex));
            if (pCurrentSequence[i]) {
                pCurrentSequence[i]->SaveGame(time);
            }
        }
    }
    Actor* pActor = nullptr;
    if (apForm && apForm->IsActor()) {
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
    pSaveLoadGameOLD->SaveGameDataOLD(&cSkipNextBlend, sizeof(cSkipNextBlend));
    cHasCastSequence = 0;
    // pActor is not tested for null before GetCurrentSpell is called.
    if (pActor->GetCurrentSpell()) {
        NiAVObjectPalette* pPalette = GetControllerManager()->GetObjectPalette();
        NiAVObject* pMagicObject = pPalette->GetAVObject(FixedStrings::QMagicNode());
        if (pMagicObject) {
            NiNode* pMagicNode = pMagicObject->IsNode();
            if (pMagicNode) {
                NiObjectNET* pChild = pMagicNode->GetAtChecked(0);
                if (pChild) {
                    NiObject* pkObject = pChild->GetControllers();
                    NiControllerManager* pCastManager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pkObject)
                    );
                    if (pCastManager) {
                        NiFixedString* pCastName = get_g_SpecialIdle_Cast();
                        BSAnimGroupSequence* pCastSequence = static_cast<BSAnimGroupSequence*>(
                            pCastManager->GetSequenceByName(*pCastName)
                        );
                        if (pCastSequence) {
                            cHasCastSequence = 1;
                            if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
                                pSaveLoadGameOLD->SaveGameDataOLD(&cHasCastSequence, sizeof(cHasCastSequence));
                            }
                            pCastSequence->SaveGame(time);
                            MagicLight* pLight = pActor->pLight;
                            fFadeTimer = 0.0f;
                            if (pLight) {
                                fFadeTimer = pLight->fFadeTimer;
                            }
                            pSaveLoadGameOLD->SaveGameDataOLD(&fFadeTimer, sizeof(fFadeTimer));
                        }
                    }
                }
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u && !cHasCastSequence) {
        pSaveLoadGameOLD->SaveGameDataOLD(&cHasCastSequence, sizeof(cHasCastSequence));
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pFormHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pFormHeader->iFormID);
            int32_t iFlags = pFormHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                pFormHeader->iFormID,
                pEditorID,
                iFlags,
                5672,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                5672,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        if (pEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                5672
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockID, sizeof(iBlockID));
        if (iBlockID != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    5680,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    5680,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    Actor* apActor = nullptr;
    if (apref && apref->IsActor()) {
        apActor = static_cast<Actor*>(apref);
    }

    pSaveLoadGameOLD->LoadGameDataOLD(&m_fMoveSpeed, sizeof(m_fMoveSpeed));
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fAttackSpeed, sizeof(m_fAttackSpeed));
    pSaveLoadGameOLD->LoadGameDataOLD(&movementDelta, sizeof(movementDelta));
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fLooking, sizeof(m_fLooking));
    pSaveLoadGameOLD->LoadGameDataOLD(&cSkipUpdate, sizeof(cSkipUpdate));
    pSaveLoadGameOLD->LoadGameDataOLD(&cSlotMask, sizeof(cSlotMask));
    for (char i = 0; i < 8; ++i) {
        if ((1 << i & cSlotMask) != 0) {
            pSaveLoadGameOLD->LoadGameDataOLD(&group[i], sizeof(group[i]));
            pSaveLoadGameOLD->LoadGameDataOLD(&action[i], sizeof(action[i]));
            pSaveLoadGameOLD->LoadGameDataOLD(&loopCount[i], sizeof(loopCount[i]));
            pSaveLoadGameOLD->LoadGameDataOLD(&nextGroup[i], sizeof(nextGroup[i]));
            pSaveLoadGameOLD->LoadGameDataOLD(&nextLoops[i], sizeof(nextLoops[i]));
            pSaveLoadGameOLD->LoadGameDataOLD(&cSequenceIndex, sizeof(cSequenceIndex));
            pCurrentSequence[i] = nullptr;
            if (cSequenceIndex != -2) {
                SkipNextBlend();
                ForceSection(static_cast<ANIM_GROUP_SECTION>(i), group[i], action[i], time, cSequenceIndex);
                if (pCurrentSequence[i]) {
                    pCurrentSequence[i]->LoadGame(time);
                }
                else {
                    uint16_t usSaveSize = BSAnimGroupSequence::GetSequenceSaveSize();
                    pSaveLoadGameOLD->AdvanceBuffer(usSaveSize);
                    ClearGroup(static_cast<ANIM_GROUP_SECTION>(i), 0.0f);
                }
            }
        }
    }
    spAnimIdle = AnimIdle::LoadAnimIdle(apActor, this, time);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 76u) {
        spAnimIdleQueued = AnimIdle::LoadAnimIdle(apActor, this, time);
    }
    if (spAnimIdle.m_pObject && spAnimIdle->eFlags == AnimIdle::ANIM_IDLE_LOADED) {
        pCurrentSequence[7] = nullptr;
    }

    pSaveLoadGameOLD->LoadGameDataOLD(&cSkipNextBlend, sizeof(cSkipNextBlend));
    cHasCastSequence = 1;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&cHasCastSequence, sizeof(cHasCastSequence));
    }
    if (cHasCastSequence) {
        char bLoaded = 0;
        if (apActor->GetCurrentSpell()) {
            NiAVObjectPalette* pPalette = GetControllerManager()->GetObjectPalette();
            NiAVObject* pMagicObject = pPalette->GetAVObject(FixedStrings::QMagicNode());
            if (pMagicObject) {
                NiNode* pMagicNode = pMagicObject->IsNode();
                if (pMagicNode) {
                    NiObjectNET* pChild = pMagicNode->GetAtChecked(0);
                    if (pChild) {
                        NiObject* pkObject = pChild->GetControllers();
                        NiControllerManager* pCastManager = static_cast<NiControllerManager*>(
                            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pkObject)
                        );
                        if (pCastManager) {
                            NiFixedString* pCastName = get_g_SpecialIdle_Cast();
                            NiControllerSequence* pCastSequence = pCastManager->GetSequenceByName(*pCastName);
                            if (pCastSequence) {
                                pCastManager->DeactivateAll(0.0f);
                                pCastManager->ActivateSequence(pCastSequence, 0, false, 1.0f, 0.0f, nullptr);
                                pCastManager->SetActive(true);
                                static_cast<BSAnimGroupSequence*>(pCastSequence)->BSAnimGroupSequence::LoadGame(time);
                                pSaveLoadGameOLD->LoadGameDataOLD(&fFadeTimer, sizeof(fFadeTimer));
                                MagicLight* pLight = apActor->pLight;
                                if (pLight) {
                                    float fFadeTime = pCastSequence->m_fEndKeyTime * 0.75;
                                    pLight->Fade(true, fFadeTime);
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
            uint16_t usSaveSize = BSAnimGroupSequence::GetSequenceSaveSize();
            pSaveLoadGameOLD->AdvanceBuffer(usSaveSize + 4);
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    5804,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                    5804,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (pBlockEnd > &pBlockStart[sBlockSize]) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - &pBlockStart[sBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                5804,
                cVersion
            );
        }
        else if (pBlockEnd < &pBlockStart[sBlockSize]) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(&pBlockStart[sBlockSize] - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Animation.cpp",
                5804,
                cVersion
            );
        }
    }
}

void Animation::ClearControllersInterpolators(NiAVObject* apObj) {
    if (apObj) {
        for (NiTimeController* pController = apObj->NiObjectNET::GetControllers(); pController;
             pController = pController->GetNext()) {
            NiSingleInterpController* pSingleController = static_cast<NiSingleInterpController*>(
                NiObject::DynamicCastStatic(&NiSingleInterpController::ms_RTTI, pController)
            );
            if (pSingleController) {
                pSingleController->SetInterpolator(nullptr, 0);
            }
            else {
                NiMultiTargetTransformController* pMultiController = static_cast<NiMultiTargetTransformController*>(
                    NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, pController)
                );
                if (pMultiController) {
                    pMultiController->ReallocateStorage();
                }
            }
        }
        NiNode* pNode = apObj->IsNode();
        if (pNode) {
            for (uint32_t ui = 0; ui < pNode->GetArrayCount(); ++ui) {
                NiAVObject* pChild = pNode->GetAt(ui);
                Animation::ClearControllersInterpolators(pChild);
            }
        }
    }
}

void Animation::ResetCloneCount() {
    Animation::iAnimationsCloned = 0;
}

bool Animation::FindSkinnedNode(NiAVObject* apAVObject) {
    if (!apAVObject) {
        return false;
    }
    NiGeometry* pGeometry = apAVObject->IsGeometry();
    if (pGeometry && pGeometry->GetSkinInstance()) {
        return true;
    }
    NiNode* pNode = apAVObject->IsNode();
    if (pNode) {
        for (uint32_t ui = 0; ui < pNode->GetArrayCount(); ++ui) {
            NiAVObject* pChild = pNode->GetAt(ui);
            if (pChild) {
                if (Animation::FindSkinnedNode(pChild)) {
                    return true;
                }
            }
        }
    }
    return false;
}

float Animation::GetDuration(NiControllerSequence* apSeq) {
    if (!apSeq) {
        return 0.0f;
    }
    float fStartTime = apSeq->GetBeginKeyTime();
    float fEndTime = apSeq->GetEndKeyTime();
    // The subtraction is done in double precision and then stored to a float.
    return static_cast<float>(static_cast<double>(fEndTime) - fStartTime);
}

// Reads an old-format save's 2-byte animation record size and, when it is not 0, queues apRef's animation load. Only
// apRef is read; apAnimation is unused.
void Animation::LoadAnimation(TESObjectREFR* apRef, [[maybe_unused]] Animation* apAnimation) {
    uint16_t usSize;
    pSaveLoadGameOLD->LoadGameDataOLD(&usSize, sizeof(usSize));
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
        // The multiply and add are done in double precision before the result is stored as a float.
        float fPhase = static_cast<float>(apSeq->GetBeginKeyTime() + static_cast<double>(fDuration) * afPercent);
        float fTime = fPhase - appTimer.GetTime() / 1000.0;
        apSeq->SetPhase(fTime, true);
    }
}

void Animation::SaveAnimation(TESObjectREFR* apRef, Animation* apAnimation) {
    uint16_t isize = 0;
    if (apAnimation && !apRef->IsDead(false)) {
        isize = apAnimation->GetAnimationSaveSize(apRef);
    }

    pSaveLoadGameOLD->SaveGameDataOLD(&isize, sizeof(isize));
    if (apAnimation && !apRef->IsDead(false)) {
        apAnimation->SaveGameTES(apRef);
    }
}

NiFixedString* get_g_SpecialIdle_Cast() {
    return FixedStrings::SpecialIdle_Cast;
}

int16_t ComputeAnimationSaveSize(PlayerCharacter* apPlayer, Animation* apAnimation) {
    uint16_t isize = 2;
    if (apAnimation && !apPlayer->IsDead(false)) {
        return apAnimation->GetAnimationSaveSize(apPlayer) + 2;
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
    uint32_t uiIndex;
    if (aiSequenceIndex == -1 || aiSequenceIndex >= pSeqList->m_uiCount) {
        uiIndex = Random() % pSeqList->m_uiCount;
    }
    else {
        uiIndex = aiSequenceIndex;
    }
    for (NiTListItem<BSAnimGroupSequence*>* pPos = pSeqList->m_pkHead; pPos; pPos = pSeqList->GetNextPos(pPos)) {
        BSAnimGroupSequence* pSeq = pSeqList->Get(pPos);
        if (!uiIndex--) {
            return pSeq;
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
    char acTarget[264];
    char* ptmp;
    void* listIt = pSeqList->m_pkHead;
    const char* pName = apSeq->m_kName.GetHandleString();
    char* p3rdLastSlash = const_cast<char*>(std::strrchr(pName, 92));
    if (!p3rdLastSlash) {
        return nullptr;
    }
    if (aeSection == AGS_WEAPON_UP) {
        ourStrcpy(acTarget, p3rdLastSlash);
        ptmp = std::strchr(acTarget, 95);
        if (ptmp) {
            ourStrcpy(ptmp, "Up");
            ptmp = std::strchr(p3rdLastSlash, 95);
            std::strcat(acTarget, ptmp);
        }
        else {
            std::strcat(acTarget, "Up");
        }
        p3rdLastSlash = acTarget;
    }
    else if (aeSection == AGS_WEAPON_DOWN) {
        ourStrcpy(acTarget, p3rdLastSlash);
        ptmp = std::strchr(acTarget, 95);
        if (ptmp) {
            ourStrcpy(ptmp, "Down");
            ptmp = std::strchr(p3rdLastSlash, 95);
            std::strcat(acTarget, ptmp);
        }
        else {
            std::strcat(acTarget, "Down");
        }
        p3rdLastSlash = acTarget;
    }
    while (listIt) {
        BSAnimGroupSequence* pseq = pSeqList->Get(static_cast<NiTListItem<BSAnimGroupSequence*>*>(listIt));
        const char* pSeqName = pseq->m_kName.GetHandleString();
        const char* p1stLastSlash = std::strrchr(pSeqName, 92);
        if (p1stLastSlash && !BSstricmp(p1stLastSlash, p3rdLastSlash)) {
            return pseq;
        }
        listIt = pSeqList->GetNextPos(static_cast<NiTListItem<BSAnimGroupSequence*>*>(listIt));
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
    return -1;
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
            if (m_kSequenceArray.GetAt(I).m_pObject) {
                return m_kSequenceArray.GetAt(I)->m_pkAccumRoot;
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

// The index is re-read and the controller fetched again for the GetTarget call.
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
    double dOffset = GetOffset();
    return dOffset - GetBeginKeyTime();
}

void NiQuatTransform::SetRotateValid(bool bValid) {
    if (!bValid) {
        m_kRotate.SetX(-std::numeric_limits<float>::max());
    }
}

void NiQuaternion::SetX(float x) {
    m_fX = x;
}

NiObjectNET* NiTimeController::GetTarget() const {
    return m_pkTarget;
}
