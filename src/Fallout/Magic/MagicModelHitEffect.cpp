#include "Fallout/Magic/MagicModelHitEffect.h"

#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

NiRTTI MagicModelHitEffect::ms_RTTI("MagicModelHitEffect", &MagicHitEffect::ms_RTTI);

MagicModelHitEffect::MagicModelHitEffect()
  : MagicHitEffect(), bOwnPath(false), b3rdPerson(false), pFilename(nullptr), spObject(nullptr), spTarget(nullptr) {}

MagicModelHitEffect::MagicModelHitEffect(TESObjectREFR* apTarget, ActiveEffect* apEffect)
  : MagicHitEffect(apTarget, apEffect), bOwnPath(false), b3rdPerson(false), pFilename(nullptr), spObject(nullptr),
    spTarget(nullptr) {
    EffectSetting* pAVEffect = apEffect->pSpell->GetAVEffect(MagicSystem::SELF);
    pFilename = pAVEffect->GetModel();
}

MagicModelHitEffect::MagicModelHitEffect(TESObjectREFR* apTarget, const char* apFilename, float afTime)
  : MagicHitEffect(apTarget, nullptr), bOwnPath(false), b3rdPerson(false), pFilename(apFilename), spObject(nullptr),
    spTarget(nullptr) {
    if (afTime < 0.0) {
        fLifetime = std::numeric_limits<float>::max();
    }
    else {
        fLifetime = afTime;
    }
}

MagicModelHitEffect::~MagicModelHitEffect() {
    if (spObject.m_pObject) {
        NiNode* pparent = spObject->m_pkParent;
        if (pparent) {
            pparent->DetachChild(spObject.m_pObject);
        }
    }
    if (pFilename && *pFilename) {
        pModelLoader->ReleaseModel(pFilename, false, 1u);
        if (bOwnPath) {
            // An owned path is the block this effect allocated.
            ourFree(const_cast<char*>(pFilename));
        }
    }

    pFilename = nullptr;
}

const NiRTTI* MagicModelHitEffect::GetRTTI() const {
    return &MagicModelHitEffect::ms_RTTI;
}

bool MagicModelHitEffect::Update(float afTimeDelta) {
    if (!pTarget || !pTarget->Get3D() || !MagicHitEffect::Update(afTimeDelta)) {
        return false;
    }
    if (spObject.m_pObject) {
        UpdatePosition();
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, spObject->GetControllers())
        );
        if (pmanager) {
            NiControllerSequence* pseq = pmanager->GetSequenceByName("SpecialIdle_HitEffect");
            if (pseq) {
                double dLastTime = pseq->m_fLastTime;
                if (pseq->m_fEndKeyTime < dLastTime &&
                    pseq->GetState() &&
                    (pseq->m_eCycleType == NiControllerSequence::CYCLE_CLAMP || bFinished)) {
                    bFinished = true;
                    pseq->Deactivate(0.0f, false);
                    fLifetime = this->MagicHitEffect::fAge;
                }
            }
        }
    }
    return !bFinished || !(this->MagicHitEffect::fAge > fLifetime + 2.0);
}

TEMP_EFFECT_TYPE MagicModelHitEffect::GetType() {
    return MAGIC_HIT_EFFECT_MODEL;
}

bool MagicModelHitEffect::CheckShouldSave() {
    if (pEffect) {
        return false;
    }
    return pTarget && pFilename;
}

// The target's form ID, the file name's length byte and characters, then the hit effect's own data.
uint16_t MagicModelHitEffect::GetSaveSize() {
    uint16_t usSize = 0;
    usSize += 4;
    usSize += 1;
    usSize += BSstrlen(pFilename);
    usSize += GetSaveSize(nullptr, pTarget);
    return usSize;
}

// The base size, two bytes, and the size of the model's controller manager.
uint16_t MagicModelHitEffect::GetSaveSize(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    uint16_t usSize = 0;
    usSize += MagicHitEffect::GetSaveSize(apEffect, apTarget);
    usSize += 2;
    if (spObject) {
        NiTimeController* pControllers = spObject->GetControllers();
        NiControllerManager* pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
        if (pManager) {
            usSize += TESObjectREFR::GetControllerManagerSaveSize(pManager);
        }
    }
    return usSize;
}

void MagicModelHitEffect::SaveGame() {
    uint32_t uiTargetID = pTarget->GetFormID();
    pSaveLoadGameOLD->SaveNumericID(&uiTargetID, 4);
    uint8_t ucLength = BSstrlen(pFilename);
    pSaveLoadGameOLD->SaveGameDataOLD(&ucLength, sizeof(ucLength));
    pSaveLoadGameOLD->SaveGameDataOLD(const_cast<char*>(pFilename), ucLength);
    SaveGame(nullptr, pTarget);
}

// After the base data and the model's file name, the controller manager of the model is saved as a variable-sized
// value. The actor's animation time is read but the manager is saved at -1.0 regardless.
void MagicModelHitEffect::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    MagicHitEffect::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveString(pFilename, 0);
    NiControllerManager* pManager = nullptr;
    if (spObject) {
        NiTimeController* pControllers = spObject->GetControllers();
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
    }
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    uint32_t uiStart = apSaveGameBuffer->GetBufferPosition();
    if (pManager) {
        TESObjectREFR::SaveControllerManager(pManager, apSaveGameBuffer, -1.0f);
    }
    apSaveGameBuffer->SaveVariableSizedValue(apSaveGameBuffer->GetBufferPosition() - uiStart, uiPosition);
}

void MagicModelHitEffect::SaveGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    uint16_t isize;
    MagicHitEffect::SaveGame(apEffect, apTarget);
    NiControllerManager* pmanager = nullptr;
    if (spObject.m_pObject) {
        pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, spObject->GetControllers())
        );
    }
    isize = 0;
    if (pmanager) {
        isize = TESObjectREFR::GetControllerManagerSaveSize(pmanager);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&isize, sizeof(isize));
    if (isize) {
        float ftime = -1.0f;
        Actor* pactor = apTarget && apTarget->IsActor() ? static_cast<Actor*>(apTarget) : nullptr;
        if (pactor && apEffect) {
            MiddleHighProcess* pprocess = dynamic_cast<MiddleHighProcess*>(pactor->pCurrentProcess);
            if (pprocess && pprocess->GetAnimation()) {
                ftime = pprocess->GetAnimation()->GetTime();
            }
        }

        TESObjectREFR::SaveControllerManager(pmanager, ftime);
    }
}

// The loaded file name goes to FinishInitLoadGame, which runs only when the target is in an attached cell.
// LoadGameDataOLD fills the length byte from the save; a zero-size chunk would leave the stack slot unwritten, so
// it starts as the empty length 0.
bool MagicModelHitEffect::LoadGame() {
    uint8_t bAttached = 1;
    uint32_t uiTargetID;
    pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4);
    pTarget = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(uiTargetID));
    if (!pTarget || !pTarget->GetParentCell() || !pTarget->GetParentCell()->GetAttached()) {
        bAttached = 0;
    }
    uint8_t ucLength = 0;
    pSaveLoadGameOLD->LoadGameDataOLD(&ucLength, sizeof(ucLength));
    char* pPath = static_cast<char*>(ourMalloc(ucLength + 1));
    ourMemset(pPath, 0, ucLength + 1);
    pSaveLoadGameOLD->LoadGameDataOLD(pPath, ucLength);
    bOwnPath = true;
    LoadGame(nullptr, pTarget);
    if (bAttached) {
        InitLoadGame(nullptr, pTarget);
        FinishInitLoadGame(nullptr, pTarget, pPath);
    }
    return bAttached;
}

void MagicModelHitEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::LoadGame(apLoadGameBuffer);
    pFilename = apLoadGameBuffer->LoadString(nullptr);
    bOwnPath = true;
    LoadedDataSubBuffer.LoadGame(apLoadGameBuffer);
    if (pEffect && pEffect->pEffect->pEffectSetting->GetModel()) {
        const char* pModel = pEffect->pEffect->pEffectSetting->GetModel();
        pModelLoader->QueueModel(pModel, IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, false, false);
    }
}

// The save holds the file name's length, then its characters; both are kept in one block, length first, for
// FinishInitLoadGame.
void MagicModelHitEffect::LoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    uint16_t isize;
    MagicHitEffect::LoadGame(apEffect, apTarget);
    if (apEffect && apEffect->pEffect->pEffectSetting->GetModel()) {
        const char* pModel = apEffect->pEffect->pEffectSetting->GetModel();
        pModelLoader->QueueModel(pModel, IO_TASK_PRIORITY_VERY_LOW, nullptr, LOD_MULT_NONE, true, false, false);
    }

    pSaveLoadGameOLD->LoadGameDataOLD(&isize, sizeof(isize));
    if (isize) {
        char* pdata = static_cast<char*>(ourMalloc(static_cast<uint32_t>(isize + sizeof(isize))));
        ourMemcpy(pdata, &isize, sizeof(isize));
        pSaveLoadGameOLD->LoadGameDataOLD(pdata + sizeof(isize), isize);
        pFilename = pdata;
    }
}

void MagicModelHitEffect::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::InitLoadGame(apLoadGameBuffer);
}

void MagicModelHitEffect::InitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
    MagicHitEffect::InitLoadGame(apEffect, apTarget);
}

// The empty base call is left out. The block LoadGame filled (the saved length and data) is read through
// the old save buffer, then freed.
void MagicModelHitEffect::FinishInitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget, void* apData) {
    uint16_t isize;
    // The source's cast: pFilename holds the block LoadGame allocated.
    void* pdata = const_cast<char*>(pFilename);
    if (apEffect) {
        TESModel* pModel = static_cast<TESModel*>(apEffect->pEffect->pEffectSetting);
        pFilename = pModel->GetModel();
    }
    else {
        pFilename = static_cast<char*>(apData);
    }
    float ftempAge = this->MagicHitEffect::fAge;
    bool btempFinished = bFinished;
    Init();
    this->MagicHitEffect::fAge = ftempAge;
    bFinished = btempFinished;
    void* ptempBuffer = pSaveLoadGameOLD->m_pBuffer;
    pSaveLoadGameOLD->m_pBuffer = pdata;
    pSaveLoadGameOLD->LoadGameDataOLD(&isize, sizeof(isize));
    if (spObject.m_pObject) {
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, spObject->GetControllers())
        );
        if (pmanager) {
            float ftime = -1.0f;
            Actor* pactor = apTarget && apTarget->IsActor() ? static_cast<Actor*>(apTarget) : nullptr;
            if (pactor && apEffect) {
                MiddleHighProcess* pprocess = dynamic_cast<MiddleHighProcess*>(pactor->pCurrentProcess);
                if (pprocess && pprocess->GetAnimation()) {
                    ftime = pprocess->GetAnimation()->GetTime();
                }
            }

            TESObjectREFR::LoadControllerManager(pmanager, spObject.m_pObject, ftime);
        }
    }
    pSaveLoadGameOLD->m_pBuffer = ptempBuffer;
    ourFree(pdata);
    Update(0.0f);
}

void MagicModelHitEffect::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    MagicHitEffect::FinishLoadGame(apLoadGameBuffer);
    float ftempAge = this->MagicHitEffect::fAge;
    bool btempFinished = bFinished;
    Init();
    this->MagicHitEffect::fAge = ftempAge;
    bFinished = btempFinished;
    if (LoadedDataSubBuffer.pBuffer) {
        NiControllerManager* pmanager = nullptr;
        if (spObject.m_pObject) {
            pmanager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, spObject->GetControllers())
            );
        }
        if (pmanager) {
            Actor* pactor = apLoadGameBuffer->GetActor();
            Animation* panim = pactor ? pactor->GetAnimation() : nullptr;
            float ftime = panim ? panim->GetTime() : -1.0f;
            BGSLoadFormBuffer* ptempLoadFormBuffer = LoadedDataSubBuffer.Create(nullptr);
            TESObjectREFR::LoadControllerManager(pmanager, ptempLoadFormBuffer, ftime);
            if (ptempLoadFormBuffer) {
                delete ptempLoadFormBuffer;
            }
        }
    }
    LoadedDataSubBuffer.DeleteBuffer();
    Update(0.0f);
}

bool MagicModelHitEffect::Init() {
    if (pEffect) {
        this->MagicHitEffect::fAge = pEffect->fElapsedSeconds;
        EffectSetting* pEffectSetting = pEffect->GetEffectSetting();
        bFinished = !pEffectSetting->IsFlagSet(EffectSetting::PERSIST);
    }
    if (!pTarget) {
        return false;
    }
    if (!pTarget->Get3D()) {
        return false;
    }
    if (pTarget == pPlayer) {
        b3rdPerson = pPlayer->Is3rdPerson();
    }
    if (!pFilename || !*pFilename) {
        return false;
    }
    NiAVObject* ptest3D = pModelLoader->LoadFile(pFilename, LOD_MULT_NONE, true, false, false, false);
    if (ptest3D) {
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, ptest3D->GetControllers())
        );
        if (pmanager) {
            NiFixedString kName("SpecialIdle_HitEffect");
            NiControllerSequence* pseq = pmanager->GetSequenceByName(kName);
            if (!pseq) {
                return false;
            }
        }
    }
    spObject = static_cast<NiNode*>(MagicProjectile::Clone3D(pFilename));
    pModelLoader->ReleaseModel(pFilename, false, 1u);
    if (!spObject.m_pObject) {
        return false;
    }
    if (!pTarget->Get3D()) {
        return false;
    }
    UpdatePosition();
    BSUtilities::ProcessSelectiveUpdate(spObject.m_pObject);
    float fTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
    NiUpdateData UpdateData(fTime, true, false);
    spObject->Update(UpdateData);
    NiControllerManager* pmanager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, spObject->GetControllers())
    );
    if (!pmanager) {
        return false;
    }
    NiControllerSequence* pseq = pmanager->GetSequenceByName("SpecialIdle_HitEffect");
    if (!pseq) {
        return false;
    }
    pmanager->DeactivateAll(0.0);
    pmanager->ActivateSequence(pseq, 0, false, 1.0f, 0.0, nullptr);
    pmanager->SetActive(true);
    if (this->MagicHitEffect::fAge > 0.0) {
        pseq->SetPhase(this->MagicHitEffect::fAge, false);
    }
    return true;
}

void MagicModelHitEffect::UpdatePosition() {
    NiAVObject* ptarget = nullptr;
    if (pTarget == pPlayer) {
        b3rdPerson = pPlayer->Is3rdPerson();
        ptarget = pPlayer->Get3D(false);
    }
    else {
        ptarget = pTarget->Get3D();
    }
    UpdateParentCell(spObject.m_pObject);
    if (ptarget) {
        NiPoint3 position;
        if ((pTarget->IsActor() && pTarget->IsDead(false)) || pTarget->IsKnockedOut()) {
            bool buseShift = true;
            NiNode* pnode = ptarget->IsNode();
            if (pnode && pnode->GetArrayCount()) {
                NiAVObject* pchild = pnode->GetAt(0);
                NiNode* pbip = pchild ? pchild->IsNode() : nullptr;
                if (pbip && pbip->GetArrayCount()) {
                    NiAVObject* pbipChild = pbip->GetAt(0);
                    NiNode* paccum = pbipChild ? pbipChild->IsNode() : nullptr;
                    if (paccum && paccum->GetTranslate() != NiPoint3::ZERO) {
                        buseShift = false;
                    }
                }
            }

            position = ptarget->GetWorldTranslate();
            if (buseShift) {
                position.z = position.z - 64.0;
            }
        }
        else if (pTarget->GetObjectReference() && pTarget->GetObjectReference()->GetFormType() == DOOR_ID) {
            position = ptarget->GetWorldBound().m_kCenter;
            position.z = ptarget->GetWorldTranslate().z;
        }
        else {
            position = ptarget->GetWorldTranslate();
        }
        if (spObject.m_pObject) {
            spObject->SetTranslate(position);
            spObject->SetRotate(ptarget->m_kLocal.m_Rotate);
        }
    }
}
