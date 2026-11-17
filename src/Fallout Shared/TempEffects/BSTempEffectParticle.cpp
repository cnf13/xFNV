#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/NiObjectGarbage.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI BSTempEffectParticle::ms_RTTI("BSTempEffectParticle", &BSTempEffect::ms_RTTI);

BSTempEffectParticle::BSTempEffectParticle(
    TESObjectCELL* apCell,
    float afLifetime,
    const char* apFilename,
    NiAVObject* apParent
)
  : BSTempEffect(apCell, afLifetime), spParticleObject(nullptr), pFilename(apFilename), spFollowObject(apParent),
    pDecalImpactData(nullptr) {
    kFollowOffset.MakeIdentity();
}

BSTempEffectParticle::~BSTempEffectParticle() {
    StopSounds();
    if (pFilename) {
        pModelLoader->ReleaseModel(pFilename, 0, 1u);
    }
    if (spParticleObject.m_pObject) {
        if (spParticleObject.m_pObject->m_pkParent) {
            NiNode* pParent = spParticleObject.m_pObject->m_pkParent;
            pParent->DetachChild(spParticleObject.m_pObject);
        }
    }
    spParticleObject = static_cast<NiAVObject*>(nullptr);
}

const NiRTTI* BSTempEffectParticle::GetRTTI() const {
    return &BSTempEffectParticle::ms_RTTI;
}

void BSTempEffectParticle::Detach() {
    NiNode* v1;
    NiRefObject* v2;
    if ((&spParticleObject)->operator->()) {
        v2 = (&spParticleObject)->operator->();
        v1 = static_cast<NiNode*>(v2)->IsNode();
    }
    else {
        v1 = nullptr;
    }
    if (v1) {
        TESObjectREFR::RemoveMasterParticleAddonNodes(v1);
    }
}

bool BSTempEffectParticle::Update(float afTime) {
    NiRefObject* v2;
    NiAVObject* v3;
    const NiTransform* worldTransform;
    NiAVObject* v5;
    NiAVObject* v6;
    NiAVObject* v7;
    float x;
    NiTransform kDest;
    uint8_t v12 = (uint8_t)BSTempEffect::Update(afTime);
    NiRefObject* vCondition1;
    vCondition1 = (&spFollowObject)->operator->();
    if (vCondition1) {
        v2 = (&spFollowObject)->operator->();
        if (v2->NiRefObject::GetRefCount() == 1) {
            (&spFollowObject)->operator=(nullptr);
        }
    }
    NiRefObject* vCondition2;
    vCondition2 = (&spParticleObject)->operator->();
    if (vCondition2) {
        NiRefObject* vCondition3;
        vCondition3 = (&spFollowObject)->operator->();
        if (vCondition3) {
            v3 = static_cast<NiAVObject*>((&spFollowObject)->operator->());
            worldTransform = &v3->GetWorldTransform();
            (kDest = worldTransform->operator*(kFollowOffset));
            v5 = static_cast<NiAVObject*>((&spParticleObject)->operator->());
            v5->SetTranslate(kDest.m_Translate);
            v6 = static_cast<NiAVObject*>((&spParticleObject)->operator->());
            v6->SetRotate(kDest.m_Rotate);
            x = kDest.m_fScale;
            v7 = static_cast<NiAVObject*>((&spParticleObject)->operator->());
            v7->SetScale(x);
        }
    }
    return (bool)v12;
}

TEMP_EFFECT_TYPE BSTempEffectParticle::GetType() {
    return TEMP_EFFECT_PARTICLE;
}

bool BSTempEffectParticle::CheckShouldSave() {
    return pFilename && spParticleObject.operator->();
}

uint16_t BSTempEffectParticle::GetSaveSize() {
    uint16_t sSize = 0;
    uint16_t sStartSize = sSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sSize += 4;
        sSize += 2;
    }
    sSize += BSTempEffect::GetSaveSize();
    sSize += 1;
    sSize += BSstrlen(pFilename);
    sSize += 16;
    sSize += 12;
    sSize += 4;
    sSize += 2;
    NiObjectNET* pParticleObject = static_cast<NiObjectNET*>(spParticleObject.operator->());
    NiTimeController* pControllers = pParticleObject->GetControllers();
    NiControllerManager* pManager =
        static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers));
    if (pManager) {
        sSize += TESObjectREFR::GetControllerManagerSaveSize(pManager);
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", sSize - sStartSize,
                pHeader->iFormID, pEditorID, pHeader->iFlags, 372,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", sSize - sStartSize, 372,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp"
            );
        }
    }
    return sSize;
}

void BSTempEffectParticle::SaveGame() {
    uint32_t iblockHeader;
    uint16_t iblockSize = 0;
    NiPoint3 loc;
    uint8_t cfileNameLength;
    NiQuaternion quat;
    uint16_t icontrollerDataSize;
    float fscale;
    void* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    BSTempEffect::SaveGame();
    cfileNameLength = BSstrlen(pFilename);
    pSaveLoadGameOLD->SaveGameDataOLD(&cfileNameLength, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(pFilename, cfileNameLength);
    quat.FromRotation(spParticleObject.m_pObject->GetLocalTransform().m_Rotate);
    pSaveLoadGameOLD->SaveGameDataOLD(&quat, 16u);
    loc = spParticleObject.m_pObject->GetTranslate();
    pSaveLoadGameOLD->SaveGameDataOLD(&loc, 12u);
    fscale = spParticleObject.m_pObject->GetScale();
    pSaveLoadGameOLD->SaveGameDataOLD(&fscale, 4u);
    icontrollerDataSize = 0;
    NiTimeController* pControllers = spParticleObject.m_pObject->GetControllers();
    NiControllerManager* pmanager =
        static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers));
    if (pmanager) {
        icontrollerDataSize = TESObjectREFR::GetControllerManagerSaveSize(pmanager);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&icontrollerDataSize, 2u);
    if (icontrollerDataSize) {
        TESObjectREFR::SaveControllerManager(pmanager, -1.0f);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, pEditorID, iFlags, 411,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 411,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pcurrent > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp", 411
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

bool BSTempEffectParticle::LoadGame() {
    FNVD_INLINED_LOCAL(NiPointer<Model>, spmodel, std::nullopt, std::nullopt);
    uint32_t iblockHeader;
    char modelPath[260];
    NiPoint3 loc;
    uint8_t cfileNameLength;
    NiQuaternion quat;
    uint16_t icontrollerDataSize;
    float fscale;
    NiMatrix3 rot;
    bool bvalid = 1;
    uint16_t iblockSize = 0;
    uint8_t* istartBufferPosition = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        // 'BLOK': the save game block header.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    420, pheader->iFormID, pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    420, pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    bvalid = BSTempEffect::LoadGame();
    pSaveLoadGameOLD->LoadGameDataOLD(&cfileNameLength, 1u);
    ourMemset(modelPath, 0, 260u);
    pSaveLoadGameOLD->LoadGameDataOLD(modelPath, cfileNameLength);
    pSaveLoadGameOLD->LoadGameDataOLD(&quat, 16u);
    quat.ToRotation(rot);
    pSaveLoadGameOLD->LoadGameDataOLD(&loc, 12u);
    pSaveLoadGameOLD->LoadGameDataOLD(&fscale, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&icontrollerDataSize, 2u);
    if (bvalid) {
        NiNode* pnode = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
        if (pnode) {
            spParticleObject.operator=(static_cast<NiAVObject*>(pnode->Clone()));
            if (spParticleObject.m_pObject) {
                ::new (&spmodel) NiPointer<Model>(static_cast<Model*>(0));
                pModelLoader->FindModel(modelPath, spmodel);
                pFilename = spmodel.m_pObject->pFilename;
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::SetTranslate(loc);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::SetRotate(rot);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::SetScale(fscale);
                NiNode* pparentnode = static_cast<NiNode*>(pCell->GetDynamicNode());
                pparentnode->AttachChild(static_cast<NiAVObject*>(spParticleObject.m_pObject), 1);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::UpdateProperties();
                float afTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
                NiUpdateData UpdateData(afTime, 1, 0);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::Update(UpdateData);
                NiTimeController::StartAnimations(static_cast<NiAVObject*>(spParticleObject.m_pObject));
                if (icontrollerDataSize) {
                    NiControllerManager* pmanager = static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(
                        &NiControllerManager::ms_RTTI,
                        static_cast<NiAVObject*>(spParticleObject.m_pObject)->GetControllers()
                    ));
                    if (pmanager) {
                        TESObjectREFR::LoadControllerManager(
                            pmanager, static_cast<NiAVObject*>(spParticleObject.m_pObject), -1.0f
                        );
                    }
                    else {
                        bvalid = 0;
                    }
                }
                NiPointer<Model>::Destroy(&spmodel);
            }
            else {
                bvalid = 0;
            }
        }
        else {
            bvalid = 0;
        }
    }
    if (!bvalid) {
        if (icontrollerDataSize) {
            pSaveLoadGameOLD->AdvanceBuffer(icontrollerDataSize);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* iendBufferPosition = &istartBufferPosition[iblockSize];
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= iendBufferPosition) {
                if (icurrentBufferPosition < iendBufferPosition) {
                    uint32_t iflags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* pEditorID = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                        492, pheader_2->iFormID, pEditorID, cVersion, iflags
                    );
                }
            }
            else {
                uint32_t iflags = pheader_2->iFlags;
                int32_t cVersion = pheader_2->cVersion;
                const char* pEditorID = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    492, pheader_2->iFormID, pEditorID, cVersion, iflags
                );
            }
        }
        else {
            if (icurrentBufferPosition <= iendBufferPosition) {
                if (icurrentBufferPosition < iendBufferPosition) {
                    uint8_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                        492, CurrentVersion
                    );
                }
            }
            else {
                uint8_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    492, CurrentVersion
                );
            }
        }
    }
    return bvalid;
}

void BSTempEffectParticle::Init(
    NiNode* apSceneParent,
    NiMatrix3 aRotation,
    NiPoint3 aPosition,
    float afScale,
    uint32_t aiFlags
) {
    NiTransform particleTransform_2;
    NiTransform followTransform;
    NiPointer<NiAVObject> spmodel(
        static_cast<NiAVObject*>(pModelLoader->LoadFile(pFilename, LOD_MULT_NONE, 1, 0, 0, 1))
    );
    if (spmodel.m_pObject) {
        spParticleObject.operator=(static_cast<NiAVObject*>(0));
        uint32_t iresult = 0;
        if ((aiFlags & 1) != 0) {
            if (TES::pInstance) {
                iresult = TES::pInstance->GetUnusedCachedParticleObject(spmodel.m_pObject, spParticleObject);
            }
        }
        if (!iresult) {
            if (spmodel.m_pObject->m_uiRefCount > 2) {
                if ((aiFlags & 2) != 0) {
                    BSCloningProcess cloneProcess(1.0f);
                    spParticleObject.operator=(static_cast<NiAVObject*>(spmodel.m_pObject->Clone(cloneProcess)));
                }
            }
            else {
                spParticleObject = spmodel;
            }
        }
        if (spParticleObject.m_pObject) {
            if ((aiFlags & 4) != 0) {
                float fcontrollerLifetime = GetControllerLifetime(spParticleObject.m_pObject);
                if (fcontrollerLifetime != -1.0) {
                    if (fcontrollerLifetime < 0.10000000149011612) {
                        fcontrollerLifetime = 0.1f;
                    }
                    fLifetime = fcontrollerLifetime;
                }
            }
            NiNode* pnode = spParticleObject.m_pObject->IsNode();
            if (pnode) {
                TESObjectREFR::AddMasterParticleAddonNodes(pnode);
            }
            spParticleObject.m_pObject->SetTranslate(aPosition);
            spParticleObject.m_pObject->SetRotate(aRotation);
            spParticleObject.m_pObject->SetScale(afScale);
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                NiAVObject* pobject = spParticleObject.m_pObject;
                TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                Instance->QueueNodeAttach(pobject, apSceneParent, 1, 1);
            }
            else {
                apSceneParent->AttachChild(spParticleObject.m_pObject, 1);
                spParticleObject.m_pObject->NiAVObject::UpdateProperties();
            }
            ResetGravity(spParticleObject.m_pObject);
            NiTimeController::StartAnimations(spParticleObject.m_pObject);
            if (spFollowObject.m_pObject) {
                NiTransform particleTransform;
                particleTransform.m_fScale = afScale;
                particleTransform.m_Rotate = aRotation;
                particleTransform.m_Translate = aPosition;
                followTransform = spFollowObject.m_pObject->GetWorldTransform();
                NiTransform followTransInvert;
                followTransform.Invert(followTransInvert);
                kFollowOffset = *::new (&particleTransform_2) NiTransform(followTransInvert * particleTransform);
            }
        }
    }
}

void BSTempEffectParticle::StartSounds() {
    FNVD_INLINED_LOCAL(BSSoundHandle, kHandle2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, kHandle1, std::nullopt, std::nullopt);
    StopSounds();
    if (spParticleObject.m_pObject && pDecalImpactData) {
        if (pDecalImpactData->pSound1) {
            uint32_t isoundFileID = pDecalImpactData->pSound1->iFormID;
            Sound1 = *BSAudio::QInstance().GetSoundHandleByNumericID(&kHandle1, isoundFileID, 258u);
            Sound1.SetPosition(static_cast<NiAVObject*>(spParticleObject.m_pObject)->GetWorldTranslate());
            Sound1.SetObjectToFollow(static_cast<NiAVObject*>(spParticleObject.m_pObject));
            Sound1.Play(0);
        }
        if (pDecalImpactData->pSound2) {
            uint32_t isoundFileID_2 = pDecalImpactData->pSound2->iFormID;
            Sound2 = *BSAudio::QInstance().GetSoundHandleByNumericID(&kHandle2, isoundFileID_2, 258u);
            Sound2.SetPosition(static_cast<NiAVObject*>(spParticleObject.m_pObject)->GetWorldTranslate());
            Sound2.SetObjectToFollow(static_cast<NiAVObject*>(spParticleObject.m_pObject));
            Sound2.Play(0);
        }
    }
}

float BSTempEffectParticle::GetControllerLifetime(NiAVObject* apAVObject) {
    float freturn = -1.0f;
    for (NiTimeController* ptimecontroller = apAVObject->GetControllers(); ptimecontroller;
         ptimecontroller = ptimecontroller->GetNext()) {
        double v6 = ptimecontroller->m_fHiKeyTime;
        float v5 = v6 - ptimecontroller->m_fLoKeyTime;
        freturn = BSMAX(freturn, v5);
    }
    NiNode* apnode = apAVObject->IsNode();
    if (apnode) {
        int32_t ichild = apnode->GetChildCount();
        for (int32_t i = 0; i < ichild; ++i) {
            NiAVObject* pchild = apnode->GetAt(i);
            if (pchild) {
                float a2 = GetControllerLifetime(pchild);
                freturn = BSMAX(freturn, a2);
            }
        }
    }
    return freturn;
}

void BSTempEffectParticle::PlaySequence(const char* apcSequenceName) {
    NiObjectNET* v2;
    NiFixedString* a2;
    NiFixedString apString;
    float endKeyTime;
    NiControllerSequence* a1;
    int32_t v10;
    v2 = static_cast<NiObjectNET*>((&spParticleObject)->operator->());
    NiTimeController* controllers = v2->NiObjectNET::GetControllers();
    NiControllerManager* v9 =
        static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, controllers));
    if (v9) {
        a2 = ::new (static_cast<void*>(&apString)) NiFixedString(const_cast<char*>(apcSequenceName));
        v10 = 0;
        a1 = v9->GetSequenceByName(*a2);
        v10 = (-1);
        if (a1) {
            v9->ActivateSequence(a1, 0, 0, 1.0, 0.0, nullptr);
            endKeyTime = (float)a1->GetEndKeyTime();
            if (fLifetime < ((double)endKeyTime)) {
                fLifetime = endKeyTime;
            }
        }
    }
}

NiAVObject* BSTempEffectParticle::GetParticleObject() {
    return static_cast<NiAVObject*>(spParticleObject.operator->());
}

NiAVObject* BSTempEffectParticle::GetFollowObject() {
    return spFollowObject.operator->();
}

void BSTempEffectParticle::StopSounds() {
    if ((&Sound1)->IsValid()) {
        (&Sound1)->Stop();
        if ((&Sound1)->IsValid()) {
            (&Sound1)->Release();
        }
    }
    if ((&Sound2)->IsValid()) {
        (&Sound2)->Stop();
        if ((&Sound2)->IsValid()) {
            (&Sound2)->Release();
        }
    }
}

void BSTempEffectParticle::SetDecalImpactData(BGSImpactData* apObject) {
    pDecalImpactData = apObject;
    StartSounds();
}

void BSTempEffectParticle::ResetGravityRecursive(
    NiAVObject* apObject,
    NiTPrimitiveArray<NiAVObject*>& iArray,
    uint32_t& aiCount
) {
    if (apObject) {
        NiParticleSystem* psys =
            static_cast<NiParticleSystem*>(NiObject::DynamicCastStatic(&NiParticleSystem::ms_RTTI, apObject));
        if (psys) {
            for (uint32_t i = 0;; ++i) {
                uint32_t ModifierCount = psys->GetModifierCount();
                if (i >= ModifierCount) {
                    break;
                }
                NiPSysModifier* ModifierAt = psys->GetModifierAt(i);
                NiPSysGravityModifier* pgrav = static_cast<NiPSysGravityModifier*>(
                    NiObject::DynamicCastStatic(&NiPSysGravityModifier::ms_RTTI, ModifierAt)
                );
                if (pgrav) {
                    bool badd = 1;
                    for (int32_t c = aiCount - 1; badd && c >= 0; --c) {
                        NiAVObject** v6 = &iArray.operator[](c);
                        if (*v6 == pgrav->m_pkGravityObj) {
                            badd = 0;
                        }
                    }
                    if (badd) {
                        NiAVObject* a3 = pgrav->m_pkGravityObj;
                        iArray.SetAt(aiCount++, a3);
                    }
                }
            }
        }
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            for (uint32_t i_2 = 0;; ++i_2) {
                uint32_t ArrayCount = pnode->GetArrayCount();
                if (i_2 >= ArrayCount) {
                    break;
                }
                NiAVObject* v8 = pnode->GetAt(i_2);
                ResetGravityRecursive(v8, iArray, aiCount);
            }
        }
    }
}

void BSTempEffectParticle::ResetGravity(NiAVObject* apObject) {
    NiMatrix3 mat;
    NiTPrimitiveArray<NiAVObject*> array(10, 1);
    uint32_t icount = 0;
    ResetGravityRecursive(apObject, array, icount);
    for (uint32_t i = 0;; ++i) {
        uint32_t Size = array.m_usSize;
        if (i >= Size) {
            break;
        }
        NiAVObject* pobj = array.operator[](i);
        if (pobj) {
            NiMatrix3 mat2;
            (&pobj->m_pkParent->GetWorldTransform())->m_Rotate.Inverse(&mat);
            mat2.MakeYRotation(3.1415927f);
            pobj->SetRotate(mat * mat2);
        }
    }
}

BSTempEffectParticle* BSTempEffectParticle::Spawn(
    TESObjectCELL* apCell,
    float afLifetime,
    const char* apParticleFilename,
    NiPoint3 aVector,
    NiPoint3 aPosition,
    float afScale,
    uint32_t aiFlags,
    NiAVObject* apFollowObject
) {
    NiMatrix3 v10;
    NiMatrix3 krandrot_2;
    BSTempEffectParticle* presult = 0;
    if (apCell) {
        BSTempEffectParticle* presult_2;
        NiMatrix3 krandrot;
        void* apMemory = NiMemObject::operator new(sizeof(BSTempEffectParticle));
        if (apMemory) {
            presult_2 = ::new (apMemory) BSTempEffectParticle(apCell, afLifetime, apParticleFilename, apFollowObject);
        }
        else {
            presult_2 = 0;
        }
        presult = presult_2;
        float frand = RandomFloatTwoPi();
        krandrot.MakeZRotation(frand);
        ::new (&krandrot_2) NiMatrix3(BSUtilities::GetRotationZToVectorTranspose(aVector) * krandrot);
        NiNode* pparentnode = apCell->GetChildNode(TESObjectCELL::CN_DYNAMIC);
        v10 = krandrot_2;
        presult->Init(pparentnode, v10, aPosition, afScale, aiFlags);
        ReferenceProcessLists.AddTempEffect(presult);
    }
    return presult;
}

BSTempEffectParticle* BSTempEffectParticle::Spawn(
    TESObjectCELL* apCell,
    float afLifetime,
    const char* apParticleFilename,
    NiMatrix3 aRotation,
    NiPoint3 aPosition,
    float afScale,
    uint32_t aiFlags,
    NiAVObject* apFollowObject
) {
    NiMatrix3 v9;
    BSTempEffectParticle* presult = 0;
    if (apCell) {
        BSTempEffectParticle* presult_2;
        void* apMemory = NiMemObject::operator new(sizeof(BSTempEffectParticle));
        if (apMemory) {
            presult_2 = ::new (apMemory) BSTempEffectParticle(apCell, afLifetime, apParticleFilename, apFollowObject);
        }
        else {
            presult_2 = 0;
        }
        presult = presult_2;
        NiNode* pparentnode = apCell->GetChildNode(TESObjectCELL::CN_DYNAMIC);
        v9 = aRotation;
        presult_2->Init(pparentnode, v9, aPosition, afScale, aiFlags);
        ReferenceProcessLists.AddTempEffect(presult);
    }
    return presult;
}

TESSound* BGSImpactData::GetSound1() {
    return pSound1;
}

TESSound* BGSImpactData::GetSound2() {
    return pSound2;
}
