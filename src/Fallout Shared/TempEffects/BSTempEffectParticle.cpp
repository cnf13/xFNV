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
        pModelLoader->ReleaseModel(pFilename, false, 1u);
    }
    if (spParticleObject.m_pObject && spParticleObject->m_pkParent) {
        NiNode* pParent = spParticleObject->m_pkParent;
        pParent->DetachChild(spParticleObject.m_pObject);
    }

    spParticleObject = static_cast<NiAVObject*>(nullptr);
}

const NiRTTI* BSTempEffectParticle::GetRTTI() const {
    return &BSTempEffectParticle::ms_RTTI;
}

void BSTempEffectParticle::Detach() {
    NiNode* pnode;
    if (spParticleObject) {
        NiRefObject* pObject = spParticleObject;
        pnode = static_cast<NiNode*>(pObject)->IsNode();
    }
    else {
        pnode = nullptr;
    }
    if (pnode) {
        TESObjectREFR::RemoveMasterParticleAddonNodes(pnode);
    }
}

bool BSTempEffectParticle::Update(float afTime) {
    NiTransform kDest;
    uint8_t ucResult = BSTempEffect::Update(afTime);
    NiRefObject* pFollowRef = spFollowObject;
    if (pFollowRef) {
        if (pFollowRef->NiRefObject::GetRefCount() == 1) {
            spFollowObject = nullptr;
        }
    }
    NiRefObject* pParticleRef = spParticleObject;
    if (pParticleRef) {
        pFollowRef = spFollowObject;
        if (pFollowRef) {
            kDest = static_cast<NiAVObject*>(spFollowObject)->GetWorldTransform() * kFollowOffset;
            static_cast<NiAVObject*>(spParticleObject)->SetTranslate(kDest.m_Translate);
            static_cast<NiAVObject*>(spParticleObject)->SetRotate(kDest.m_Rotate);
            static_cast<NiAVObject*>(spParticleObject)->SetScale(kDest.m_fScale);
        }
    }
    return ucResult != 0;
}

TEMP_EFFECT_TYPE BSTempEffectParticle::GetType() {
    return TEMP_EFFECT_PARTICLE;
}

bool BSTempEffectParticle::CheckShouldSave() {
    return pFilename && spParticleObject;
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
    NiObjectNET* pParticleObject = static_cast<NiObjectNET*>(spParticleObject);
    NiTimeController* pControllers = pParticleObject->GetControllers();
    NiControllerManager* pManager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
    );
    if (pManager) {
        sSize += TESObjectREFR::GetControllerManagerSaveSize(pManager);
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
                372,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                sSize - sStartSize,
                372,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    BSTempEffect::SaveGame();
    cfileNameLength = BSstrlen(pFilename);
    pSaveLoadGameOLD->SaveGameDataOLD(&cfileNameLength, sizeof(cfileNameLength));
    pSaveLoadGameOLD->SaveGameDataOLD(pFilename, cfileNameLength);
    quat.FromRotation(spParticleObject->GetLocalTransform().m_Rotate);
    pSaveLoadGameOLD->SaveGameDataOLD(&quat, sizeof(quat));
    loc = spParticleObject->GetTranslate();
    pSaveLoadGameOLD->SaveGameDataOLD(&loc, sizeof(loc));
    fscale = spParticleObject->GetScale();
    pSaveLoadGameOLD->SaveGameDataOLD(&fscale, sizeof(fscale));
    icontrollerDataSize = 0;
    NiTimeController* pControllers = spParticleObject->GetControllers();
    NiControllerManager* pmanager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
    );
    if (pmanager) {
        icontrollerDataSize = TESObjectREFR::GetControllerManagerSaveSize(pmanager);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&icontrollerDataSize, sizeof(icontrollerDataSize));
    if (icontrollerDataSize) {
        TESObjectREFR::SaveControllerManager(pmanager, -1.0f);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBuffer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pBuffer - itempStartPosition),
                pheader->iFormID,
                pEditorID,
                iFlags,
                411,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBuffer - itempStartPosition),
                411,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                411
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

bool BSTempEffectParticle::LoadGame() {
    uint32_t iblockHeader;
    char modelPath[260];
    NiPoint3 loc;
    uint8_t cfileNameLength;
    NiQuaternion quat;
    uint16_t icontrollerDataSize;
    float fscale;
    NiMatrix3 rot;
    bool bvalid = true;
    uint16_t iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        // 'BLOK': the save game block header.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    420,
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    420,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    bvalid = BSTempEffect::LoadGame();
    pSaveLoadGameOLD->LoadGameDataOLD(&cfileNameLength, sizeof(cfileNameLength));
    ourMemset(modelPath, 0, 260u);
    pSaveLoadGameOLD->LoadGameDataOLD(modelPath, cfileNameLength);
    pSaveLoadGameOLD->LoadGameDataOLD(&quat, sizeof(quat));
    quat.ToRotation(rot);
    pSaveLoadGameOLD->LoadGameDataOLD(&loc, sizeof(loc));
    pSaveLoadGameOLD->LoadGameDataOLD(&fscale, sizeof(fscale));
    pSaveLoadGameOLD->LoadGameDataOLD(&icontrollerDataSize, sizeof(icontrollerDataSize));
    if (bvalid) {
        NiNode* pnode = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
        if (pnode) {
            spParticleObject = static_cast<NiAVObject*>(pnode->Clone());
            if (spParticleObject.m_pObject) {
                NiPointer<Model> spmodel;
                pModelLoader->FindModel(modelPath, spmodel);
                pFilename = spmodel->pFilename;
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::SetTranslate(loc);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::SetRotate(rot);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::SetScale(fscale);
                NiNode* pparentnode = static_cast<NiNode*>(pCell->GetDynamicNode());
                pparentnode->AttachChild(static_cast<NiAVObject*>(spParticleObject.m_pObject), 1);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::UpdateProperties();
                float fTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
                NiUpdateData kUpdateData(fTime, true, false);
                static_cast<NiAVObject*>(spParticleObject.m_pObject)->NiAVObject::Update(kUpdateData);
                NiTimeController::StartAnimations(static_cast<NiAVObject*>(spParticleObject.m_pObject));
                if (icontrollerDataSize) {
                    NiControllerManager* pmanager = static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(
                        &NiControllerManager::ms_RTTI,
                        static_cast<NiAVObject*>(spParticleObject.m_pObject)->GetControllers()
                    ));
                    if (pmanager) {
                        TESObjectREFR::LoadControllerManager(
                            pmanager,
                            static_cast<NiAVObject*>(spParticleObject.m_pObject),
                            -1.0f
                        );
                    }
                    else {
                        bvalid = false;
                    }
                }
            }
            else {
                bvalid = false;
            }
        }
        else {
            bvalid = false;
        }
    }
    if (!bvalid && icontrollerDataSize) {
        pSaveLoadGameOLD->AdvanceBuffer(icontrollerDataSize);
    }

    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* iendBufferPosition = &istartBufferPosition[iblockSize];
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= iendBufferPosition) {
                if (icurrentBufferPosition < iendBufferPosition) {
                    uint32_t iflags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                        492,
                        pheader->iFormID,
                        pEditorID,
                        cVersion,
                        iflags
                    );
                }
            }
            else {
                uint32_t iflags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    492,
                    pheader->iFormID,
                    pEditorID,
                    cVersion,
                    iflags
                );
            }
        }
        else if (icurrentBufferPosition <= iendBufferPosition) {
            if (icurrentBufferPosition < iendBufferPosition) {
                uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                    492,
                    ucVersion
                );
            }
        }
        else {
            uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectParticle.cpp",
                492,
                ucVersion
            );
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
    NiTransform kOffsetTransform;
    NiTransform followTransform;
    NiPointer<NiAVObject> spmodel(
        static_cast<NiAVObject*>(pModelLoader->LoadFile(pFilename, LOD_MULT_NONE, true, false, false, true))
    );
    if (spmodel.m_pObject) {
        spParticleObject = static_cast<NiAVObject*>(nullptr);
        uint32_t iresult = 0;
        if ((aiFlags & INIT_FLAG_USE_CACHED) != 0 && TES::pInstance) {
            iresult = TES::pInstance->GetUnusedCachedParticleObject(spmodel.m_pObject, spParticleObject);
        }

        if (!iresult) {
            if (spmodel->m_uiRefCount > 2) {
                if ((aiFlags & INIT_FLAG_CLONE_SHARED) != 0) {
                    BSCloningProcess cloneProcess(1.0f);
                    spParticleObject = static_cast<NiAVObject*>(spmodel->Clone(cloneProcess));
                }
            }
            else {
                spParticleObject = spmodel;
            }
        }
        if (spParticleObject.m_pObject) {
            if ((aiFlags & INIT_FLAG_CONTROLLER_LIFETIME) != 0) {
                float fcontrollerLifetime = GetControllerLifetime(spParticleObject.m_pObject);
                if (fcontrollerLifetime != -1.0) {
                    if (fcontrollerLifetime < 0.10000000149011612) {
                        fcontrollerLifetime = 0.1f;
                    }
                    fLifetime = fcontrollerLifetime;
                }
            }
            NiNode* pnode = spParticleObject->IsNode();
            if (pnode) {
                TESObjectREFR::AddMasterParticleAddonNodes(pnode);
            }
            spParticleObject->SetTranslate(aPosition);
            spParticleObject->SetRotate(aRotation);
            spParticleObject->SetScale(afScale);
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                NiAVObject* pobject = spParticleObject.m_pObject;
                TaskQueueInterface::GetInstance()->QueueNodeAttach(pobject, apSceneParent, true, true);
            }
            else {
                apSceneParent->AttachChild(spParticleObject.m_pObject, 1);
                spParticleObject->NiAVObject::UpdateProperties();
            }
            ResetGravity(spParticleObject.m_pObject);
            NiTimeController::StartAnimations(spParticleObject.m_pObject);
            if (spFollowObject.m_pObject) {
                NiTransform particleTransform;
                particleTransform.m_fScale = afScale;
                particleTransform.m_Rotate = aRotation;
                particleTransform.m_Translate = aPosition;
                followTransform = spFollowObject->GetWorldTransform();
                NiTransform followTransInvert;
                followTransform.Invert(followTransInvert);
                kFollowOffset = *::new (&kOffsetTransform) NiTransform(followTransInvert * particleTransform);
            }
        }
    }
}

void BSTempEffectParticle::StartSounds() {
    StopSounds();
    if (spParticleObject.m_pObject && pDecalImpactData) {
        if (pDecalImpactData->pSound1) {
            uint32_t isoundFileID = pDecalImpactData->pSound1->iFormID;
            Sound1 = BSAudio::QInstance().GetSoundHandleByNumericID(isoundFileID, 258u);
            Sound1.SetPosition(static_cast<NiAVObject*>(spParticleObject.m_pObject)->GetWorldTranslate());
            Sound1.SetObjectToFollow(static_cast<NiAVObject*>(spParticleObject.m_pObject));
            Sound1.Play(false);
        }
        if (pDecalImpactData->pSound2) {
            uint32_t isoundFileID = pDecalImpactData->pSound2->iFormID;
            Sound2 = BSAudio::QInstance().GetSoundHandleByNumericID(isoundFileID, 258u);
            Sound2.SetPosition(static_cast<NiAVObject*>(spParticleObject.m_pObject)->GetWorldTranslate());
            Sound2.SetObjectToFollow(static_cast<NiAVObject*>(spParticleObject.m_pObject));
            Sound2.Play(false);
        }
    }
}

float BSTempEffectParticle::GetControllerLifetime(NiAVObject* apAVObject) {
    float freturn = -1.0f;
    for (NiTimeController* ptimecontroller = apAVObject->GetControllers(); ptimecontroller;
         ptimecontroller = ptimecontroller->GetNext()) {
        double dHiKeyTime = ptimecontroller->m_fHiKeyTime;
        float fcontrollerLifetime = dHiKeyTime - ptimecontroller->m_fLoKeyTime;
        freturn = BSMAX(freturn, fcontrollerLifetime);
    }
    NiNode* pnode = apAVObject->IsNode();
    if (pnode) {
        int32_t ichild = pnode->GetChildCount();
        for (int32_t i = 0; i < ichild; ++i) {
            NiAVObject* pchild = pnode->GetAt(i);
            if (pchild) {
                float fchildLifetime = GetControllerLifetime(pchild);
                freturn = BSMAX(freturn, fchildLifetime);
            }
        }
    }
    return freturn;
}

void BSTempEffectParticle::PlaySequence(const char* apcSequenceName) {
    NiFixedString kSequenceName;
    NiObjectNET* pParticleObject = static_cast<NiObjectNET*>(spParticleObject);
    NiTimeController* pControllers = pParticleObject->NiObjectNET::GetControllers();
    NiControllerManager* pManager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
    );
    if (pManager) {
        ::new (static_cast<void*>(&kSequenceName)) NiFixedString(const_cast<char*>(apcSequenceName));
        NiControllerSequence* pSequence = pManager->GetSequenceByName(kSequenceName);
        if (pSequence) {
            pManager->ActivateSequence(pSequence, 0, false, 1.0, 0.0, nullptr);
            float fEndKeyTime = pSequence->GetEndKeyTime();
            if (fLifetime < static_cast<double>(fEndKeyTime)) {
                fLifetime = fEndKeyTime;
            }
        }
    }
}

NiAVObject* BSTempEffectParticle::GetParticleObject() {
    return static_cast<NiAVObject*>(spParticleObject);
}

NiAVObject* BSTempEffectParticle::GetFollowObject() {
    return spFollowObject;
}

void BSTempEffectParticle::StopSounds() {
    if (Sound1.IsValid()) {
        Sound1.Stop();
        if (Sound1.IsValid()) {
            Sound1.Release();
        }
    }
    if (Sound2.IsValid()) {
        Sound2.Stop();
        if (Sound2.IsValid()) {
            Sound2.Release();
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
        NiParticleSystem* psys = static_cast<NiParticleSystem*>(
            NiObject::DynamicCastStatic(&NiParticleSystem::ms_RTTI, apObject)
        );
        if (psys) {
            for (uint32_t i = 0; i < psys->GetModifierCount(); ++i) {
                NiPSysModifier* pModifier = psys->GetModifierAt(i);
                NiPSysGravityModifier* pgrav = static_cast<NiPSysGravityModifier*>(
                    NiObject::DynamicCastStatic(&NiPSysGravityModifier::ms_RTTI, pModifier)
                );
                if (pgrav) {
                    bool badd = true;
                    for (int32_t c = aiCount - 1; badd && c >= 0; --c) {
                        if (iArray[c] == pgrav->m_pkGravityObj) {
                            badd = false;
                        }
                    }
                    if (badd) {
                        NiAVObject* pGravityObj = pgrav->m_pkGravityObj;
                        iArray.SetAt(aiCount++, pGravityObj);
                    }
                }
            }
        }
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            for (uint32_t i = 0; i < pnode->GetArrayCount(); ++i) {
                ResetGravityRecursive(pnode->GetAt(i), iArray, aiCount);
            }
        }
    }
}

void BSTempEffectParticle::ResetGravity(NiAVObject* apObject) {
    NiMatrix3 mat;
    NiTPrimitiveArray<NiAVObject*> array(10, 1);
    uint32_t icount = 0;
    ResetGravityRecursive(apObject, array, icount);
    for (uint32_t i = 0; i < array.m_usSize; ++i) {
        NiAVObject* pobj = array[i];
        if (pobj) {
            NiMatrix3 mat2;
            pobj->m_pkParent->GetWorldTransform().m_Rotate.Inverse(&mat);
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
    NiMatrix3 kRotation;
    BSTempEffectParticle* presult = nullptr;
    if (apCell) {
        NiMatrix3 krandrot;
        presult = new BSTempEffectParticle(apCell, afLifetime, apParticleFilename, apFollowObject);
        float frand = RandomFloatTwoPi();
        krandrot.MakeZRotation(frand);
        ::new (&kRotation) NiMatrix3(BSUtilities::GetRotationZToVectorTranspose(aVector) * krandrot);
        NiNode* pparentnode = apCell->GetChildNode(TESObjectCELL::CN_DYNAMIC);
        presult->Init(pparentnode, kRotation, aPosition, afScale, aiFlags);
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
    BSTempEffectParticle* presult = nullptr;
    if (apCell) {
        presult = new BSTempEffectParticle(apCell, afLifetime, apParticleFilename, apFollowObject);
        NiNode* pparentnode = apCell->GetChildNode(TESObjectCELL::CN_DYNAMIC);
        presult->Init(pparentnode, aRotation, aPosition, afScale, aiFlags);
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
