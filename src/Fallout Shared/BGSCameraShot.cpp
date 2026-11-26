#include "Fallout Shared/BGSCameraShot.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// The game settings this file defines, with the game's IDs and values, in initialization order.
NiPoint3 currentCameraPos(NiPoint3::ZERO);
NiPoint3 dollyStartingPos(NiPoint3::ZERO);
NiPoint3 dollyDelta(NiPoint3::ZERO);
SettingT<GameSettingCollection> fVATSCameraDollyTime("fVATSCameraDollyTime", 1.0f);
SettingT<GameSettingCollection> fVATSCameraDollyMin("fVATSCameraDollyMin", 256.0f);
SettingT<GameSettingCollection> fVATSCameraCutAwayDistance("fVATSCameraCutAwayDistance", 25.0f);
SettingT<GameSettingCollection> fVATSCameraEffectVolume("fVATSCameraEffectVolume", 1.0f);

// The time multipliers first, then the enums and flags, then the times and the target percentage; the order is the
// game's.
void BGSCameraShot::CAMERA_SHOT_DATA::Endian() {
    BSEndian(fPlayerTimeMult, false);
    BSEndian(fTargetTimeMult, false);
    BSEndian(fGlobalTimeMult, false);
    BSEndian(eCameraAction, false);
    BSEndian(eLocation, false);
    BSEndian(eTarget, false);
    BSEndian(iFlags, false);
    BSEndian(fMaxTime, false);
    BSEndian(fMinTime, false);
    BSEndian(fTargetPercentBetweenActors, false);
}

// The time multipliers first, then the enums and flags, then the times; the order is the game's.
void BGSCameraShot::OLD_CAMERA_SHOT_DATA::Endian() {
    BSEndian(fPlayerTimeMult, false);
    BSEndian(fGlobalTimeMult, false);
    BSEndian(eCameraAction, false);
    BSEndian(eLocation, false);
    BSEndian(eTarget, false);
    BSEndian(iFlags, false);
    BSEndian(fMaxTime, false);
    BSEndian(fMinTime, false);
}

BGSCameraShot::BGSCameraShot()
  : pTargetRef(nullptr), cCameraHasRotation(-1), bCameraTooClose(false), bCameraIsKillCam(false) {
    cFormType = CAMS_ID;
    BGSCameraShot::InitializeData();
}

BGSCameraShot::~BGSCameraShot() {
    FreeCamera();
}

void BGSCameraShot::InitializeData() {
    Data.eCameraAction = CA_HIT;
    Data.eLocation = CO_TARGET;
    Data.eTarget = CO_ATTACKER;
    Data.fGlobalTimeMult = 1.0f;
    Data.fPlayerTimeMult = 0.0f;
    Data.fTargetTimeMult = 0.0f;
    Data.iFlags = POS_FOLLOWS_TARGET | ROT_FOLLOWS_TARGET;
    Data.fMaxTime = 0.0f;
    Data.fMinTime = 0.0f;
    Data.fTargetPercentBetweenActors = 0.0f;
}

// Reads the shot's model, its data (the older 0x20-byte layout or the 0x28-byte one) and its image space modifier.
bool BGSCameraShot::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != CAMS_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk > MODL_ID) {
            if (uiChunk == MNAM_ID) {
                // The form ID is stored straight into the component's pointer field.
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                SetFormImageSpaceModifying(reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID)));
            }
            else if (uiChunk == MODT_ID) {
                TESModel::LoadModelChunk(this, apFile);
            }
        }
        else {
            switch (uiChunk) {
                case MODL_ID: TESModel::LoadModelChunk(this, apFile); break;
                case DATA_ID:
                    if (apFile->GetChunkSize() == 0x20) {
                        // the older files' 0x20-byte layout, expanded into the current one
                        OLD_CAMERA_SHOT_DATA kOldData;
                        ourMemset(&kOldData, 0, sizeof(kOldData));
                        LoadData(apFile, reinterpret_cast<char*>(&kOldData), sizeof(kOldData));
                        if (apFile->QEndian()) {
                            kOldData.Endian();
                        }
                        Data.eCameraAction = kOldData.eCameraAction;
                        Data.eLocation = kOldData.eLocation;
                        Data.eTarget = kOldData.eTarget;
                        Data.iFlags = kOldData.iFlags;
                        Data.fPlayerTimeMult = kOldData.fPlayerTimeMult;
                        Data.fGlobalTimeMult = kOldData.fGlobalTimeMult;
                        Data.fMaxTime = kOldData.fMaxTime;
                        Data.fMinTime = kOldData.fMinTime;
                    }
                    else {
                        ourMemset(&Data, 0, sizeof(Data));
                        apFile->GetChunkData(&Data, 0x28);
                        if (apFile->QEndian()) {
                            Data.Endian();
                        }
                    }
                    break;
                case EDID_ID: {
                    // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                    apFile->GetChunkData(pBuffer, 0x200);
                    SetFormEditorID(pBuffer);
                    break;
                }
                case OBND_ID: LoadObjectBound(apFile); break;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSCameraShot::Save() {
    TESForm::StartForm();
    static_cast<TESModel*>(this)->Save(MODL_ID, MODT_ID);
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__SaveData(&Data, sizeof(Data));
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    (static_cast<TESImageSpaceModifiableForm*>(this))->Save();
    TESForm::CloseForm();
}

void BGSCameraShot::InitItem() {
    if (!GetInitialized()) {
        TESImageSpaceModifiableForm::InitItem(this);
        if (Data.fGlobalTimeMult < 9.999999747378752e-05) {
            Warning(
                "Bad VATS CameraShot Data: '%s' has a Global Time Multiplier that is too small.",
                GetFormEditorID()
            );
            Data.fGlobalTimeMult = 0.1f;
        }
        SetInitialized(true);
    }
}

void BGSCameraShot::Copy(TESForm* apCopy) {
    BGSCameraShot* pCameraShot = dynamic_cast<BGSCameraShot*>(apCopy);
    if (pCameraShot) {
        CopyAllComponents(pCameraShot);
        ourMemcpy(&Data, &pCameraShot->Data, sizeof(Data));
    }
}

bool BGSCameraShot::Compare(TESForm* apCompare) {
    BGSCameraShot* pCameraShot = dynamic_cast<BGSCameraShot*>(apCompare);
    if (!pCameraShot) {
        return true;
    }
    if (CompareAllComponents(pCameraShot)) {
        return true;
    }
    return std::memcmp(&Data, &pCameraShot->Data, sizeof(Data)) != 0;
}

float BGSCameraShot::GetTargetTimeMult() {
    return Data.fTargetTimeMult;
}

BGSCameraShot::CAM_ACTION BGSCameraShot::GetCameraAction() {
    return Data.eCameraAction;
}

bool BGSCameraShot::GetPosFollowsTarget() {
    return (Data.iFlags & POS_FOLLOWS_TARGET) != 0;
}

bool BGSCameraShot::GetRotFollowsTarget() {
    return (Data.iFlags & ROT_FOLLOWS_TARGET) != 0;
}

bool BGSCameraShot::GetIsFirstPersonCamera() {
    return (Data.iFlags & FIRST_PERSON_CAMERA) != 0;
}

bool BGSCameraShot::IsInitialized() {
    if (Data.eCameraAction == 3) {
        return false;
    }
    if (static_cast<TESModel*>(this)->GetModelLength()) {
        return (spCameraNode != nullptr);
    }
    return true;
}

bool BGSCameraShot::IsStartAtTimeZero() {
    return (Data.iFlags & START_AT_TIME_ZERO) != 0;
}

void BGSCameraShot::FreeCamera() {
    spLocationNode = static_cast<NiAVObject*>(nullptr);
    spTargetNode = static_cast<NiAVObject*>(nullptr);
    pTargetRef = nullptr;
    spImagespaceTarget = static_cast<NiAVObject*>(nullptr);
    bCameraTooClose = false;
    if (spCameraNode.m_pObject) {
        spCameraNode = static_cast<NiNode*>(nullptr);
        const char* pcModel = GetModel();
        pModelLoader->ReleaseFile(pcModel);
    }
    if (bPlayCameraZoomSounds && !pVATS.GetCount()) {
        bPlayCameraZoomSounds = false;
        BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByName("UIVATSCameraOut", 289u);
        soundHand.Play(false);
    }
}

bool BGSCameraShot::GetFollowsBone() {
    return (Data.iFlags & DONT_FOLLOW_BONE) == 0;
}

bool BGSCameraShot::GetNoTracer() {
    return (Data.iFlags & NO_TRACER) != 0;
}

NiAVObject* BGSCameraShot::GetLocationNode() {
    return spLocationNode;
}

NiAVObject* BGSCameraShot::GetTargetNode() {
    return spTargetNode;
}

NiAVObject* BGSCameraShot::GetImagespaceTarget() {
    return spImagespaceTarget;
}

void BGSCameraShot::SetCameraLocation(NiAVObject* apLocation) {
    const NiPoint3* pWorldTranslate;
    NiMatrix3 kOrientation;
    if (GetPosFollowsTarget()) {
        spLocationNode = apLocation;
    }
    else if (apLocation) {
        spLocationNode = new NiNode(0);
        if (pTargetRef) {
            kOrientation = pTargetRef->GetOrientation();
            spLocationNode->SetRotate(kOrientation);
        }
        pWorldTranslate = &apLocation->GetWorldTranslate();
        spLocationNode->SetWorldTranslate(*pWorldTranslate);
    }
    PlayerCharacter::bSnapChaseCamera = true;
}

void BGSCameraShot::InitCamera(TESObjectREFR* apTargetRef, int32_t aiCameraZoom) {
    NiCamera* pCamera;
    uint32_t uiNumKeys;
    uint8_t ucSize;
    NiFrustum kVATSFrustum;
    if (GetModelLength()) {
        const char* pcModel = GetModel();
        spCameraNode = pModelLoader->LoadFile(pcModel, LOD_MULT_NONE, true, true, false, false);
    }
    if (spCameraNode.m_pObject) {
        NiAVObject* pChild = spCameraNode->GetAtChecked(0);
        pCamera = static_cast<NiCamera*>(NiObject::DynamicCastStatic(&NiCamera::ms_RTTI, pChild));
    }
    else {
        pCamera = nullptr;
    }
    if (!pCamera) {
        Warning(
            "DEFAULT: VATS Camera '%s' doesn't have an NiCamera as the first child. This camera will not be used.",
            GetModel()
        );
        spCameraNode = static_cast<NiNode*>(nullptr);
        const char* pcModel = GetModel();
        pModelLoader->ReleaseFile(pcModel);
    }
    if (spCameraNode.m_pObject) {
        NiUpdateData UpdateData(fVATSPlaybackTimer, true, false);
        if (IsStartAtTimeZero()) {
            float fPhase = -fVATSPlaybackTimer;
            SetPhase(spCameraNode.m_pObject, fPhase);
        }
        spCameraNode->Update(UpdateData);
        if (cCameraHasRotation == -1 && !bCameraIsKillCam) {
            NiSingleInterpController* pInterpController = static_cast<NiSingleInterpController*>(
                spCameraNode->GetController(&NiSingleInterpController::ms_RTTI)
            );
            if (pInterpController) {
                NiInterpolator* pInterp = pInterpController->GetInterpolator(0);
                NiTransformInterpolator* pTrans = static_cast<NiTransformInterpolator*>(
                    NiObject::DynamicCastStatic(&NiTransformInterpolator::ms_RTTI, pInterp)
                );
                NiPathInterpolator* pPath = nullptr;
                if (pTrans) {
                    NiAnimationKey::KeyType eType;
                    pTrans->GetRotData(uiNumKeys, eType, ucSize);
                    if (uiNumKeys) {
                        cCameraHasRotation = 1;
                    }
                }
                else {
                    pPath = static_cast<NiPathInterpolator*>(
                        NiObject::DynamicCastStatic(&NiPathInterpolator::ms_RTTI, pInterp)
                    );
                }
                if (pPath && pPath->GetFollow()) {
                    cCameraHasRotation = 1;
                }

                if (cCameraHasRotation == -1) {
                    cCameraHasRotation = 0;
                }
            }
        }
        if (spCameraNode->GetTranslate() == NiPoint3::ZERO) {
            Warning("DEFAULT: VATS Camera '%s' doesn't have any position data.", GetModel());
        }
        kVATSFrustum = pCamera->GetViewFrustum();
        if (kVATSFrustum.m_fTop / kVATSFrustum.m_fRight < 0.7400000095367432) {
            Warning("DEFAULT: VATS Camera '%s' has a bad FOV that was not created for a 4x3 aspect ratio.", GetModel());
        }
    }
    pTargetRef = apTargetRef;
    PlayerCharacter::bSnapChaseCamera = true;
    if (aiCameraZoom) {
        if (aiCameraZoom == 1) {
            bDollyToNewPos = false;
        }
        else if (aiCameraZoom == 2 && Data.eCameraAction == BGSCameraShot::CA_ZOOM) {
            bDollyToNewPos = true;
            if (!(Data.fMinTime > 0.0)) {
                g_fVATSCameraDollyTime = fVATSCameraDollyTime.Float();
            }
            else {
                g_fVATSCameraDollyTime = Data.fMinTime;
            }
        }
    }
    else {
        bDollyToNewPos = true;
        g_fVATSCameraDollyTime = fVATSCameraDollyTime.Float();
    }
    fDollyStartTime = 0.0f;
    fDollyPercentage = 0.0f;
    dollyDelta = NiPoint3::ZERO;
    dollyStartingPos = NiPoint3::ZERO;
    VATS_COMMAND* pCurrentCommand = pVATS.GetCurrentAction();
    if (pCurrentCommand &&
        !bPlayCameraZoomSounds &&
        (pCurrentCommand->pHitData->IsFlagSet(HitData::HD_CRITICAL) ||
         pCurrentCommand->pHitData->IsFlagSet(HitData::HD_CRITICAL_ON_DEATH) ||
         pCurrentCommand->pHitData->IsFlagSet(HitData::HD_FATAL))) {
        if (Data.eCameraAction == BGSCameraShot::CA_HIT) {
            bPlayCameraZoomSounds = true;
            BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByName("UIVATSCameraIn", 289u);
            soundHand.Play(false);
        }
    }
    else if (bPlayCameraZoomSounds && Data.eCameraAction != BGSCameraShot::CA_HIT) {
        bPlayCameraZoomSounds = false;
        BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByName("UIVATSCameraOut", 289u);
        soundHand.Play(false);
    }
    pPlayer->Setup3rdPersonCameraCaster();
}

void BGSCameraShot::SetCameraTarget(NiAVObject* apTarget) {
    if (apTarget) {
        NiNode* pNode;
        if (GetRotFollowsTarget()) {
            spTargetNode = apTarget;
        }
        else {
            spTargetNode = new NiNode(0);
            spTargetNode->SetWorldTranslate(apTarget->GetWorldTranslate());
        }
        for (pNode = static_cast<NiNode*>(apTarget); pNode && !pNode->IsFadeNode(); pNode = pNode->m_pkParent) {
            ;
        }
        if (pNode) {
            spImagespaceTarget = pNode;
        }
        else {
            spImagespaceTarget = apTarget;
        }
        PlayerCharacter::bSnapChaseCamera = true;
    }
    else {
        spTargetNode = static_cast<NiAVObject*>(nullptr);
        spImagespaceTarget = static_cast<NiAVObject*>(nullptr);
    }
}

void BGSCameraShot::Update(float afTime) {
    NiFrustum kVATSFrustum;
    NiFrustum kGameFrustum;
    NiPoint3 kTargetPos;
    NiPoint3 kCameraLocalPos;
    NiPoint3 kCameraPos;
    if (spCameraNode.m_pObject) {
        NiUpdateData UpdateData(afTime, true, false);
        spCameraNode->Update(UpdateData);
        currentCameraPos = spCameraNode->GetTranslate();
        kCameraPos = currentCameraPos;
        if (!bCameraTooClose || pPlayer->IsDead(false)) {
            if (GetPosFollowsTarget()) {
                if (GetFollowsBone() && pTargetRef && Data.eLocation == 2) {
                    kCameraLocalPos = kCameraPos;
                    NiMatrix3::TransformVertices(
                        pTargetRef->GetOrientation(),
                        NiPoint3::ZERO,
                        1u,
                        &kCameraLocalPos,
                        &kCameraPos
                    );
                }
                else {
                    kCameraLocalPos = kCameraPos;
                    NiMatrix3::TransformVertices(
                        pVATSAttacker->GetOrientation(),
                        NiPoint3::ZERO,
                        1u,
                        &kCameraLocalPos,
                        &kCameraPos
                    );
                }
            }
            else if (spLocationNode.m_pObject) {
                kCameraLocalPos = kCameraPos;
                NiMatrix3::TransformVertices(
                    spLocationNode->m_kLocal.m_Rotate,
                    NiPoint3::ZERO,
                    1u,
                    &kCameraLocalPos,
                    &kCameraPos
                );
            }
            if (spLocationNode.m_pObject) {
                bool bPlayZoomSound = false;
                if (dollyStartingPos == NiPoint3::ZERO) {
                    dollyStartingPos = chaseCameraPos;
                    bPlayZoomSound = true;
                }
                kTargetPos = spLocationNode->GetWorldTranslate();
                kCameraPos += kTargetPos;
                pPlayer->UpdateChaseCamera(&kCameraPos, &kTargetPos, false);
                chaseCameraPos = kCameraPos;
                if (bDollyToNewPos) {
                    dollyDelta = chaseCameraPos - dollyStartingPos;
                    double dDollyLength = dollyDelta.Length();
                    if (!(fVATSCameraDollyMin.Float() >= dDollyLength)) {
                        if (Data.eCameraAction) {
                            if (bPlayZoomSound && !bPlayCameraZoomSounds) {
                                BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByName(
                                    "UIVATSCameraIn",
                                    289u
                                );
                                soundHand.Play(false);
                            }
                            fDollyPercentage = 1.0 / g_fVATSCameraDollyTime * (afTime - fDollyStartTime);
                            if (!(fDollyPercentage >= 1.0)) {
                                chaseCameraPos = dollyStartingPos + dollyDelta * fDollyPercentage;
                            }
                            else {
                                bDollyToNewPos = false;
                                fDollyStartTime = 0.0f;
                            }
                        }
                        else {
                            bDollyToNewPos = false;
                            fDollyStartTime = 0.0f;
                        }
                    }
                    else {
                        bDollyToNewPos = false;
                        fDollyStartTime = 0.0f;
                    }
                }
                NiCamera* pCamera = static_cast<NiCamera*>(spCameraNode->GetAtChecked(0));
                if (pCamera) {
                    kVATSFrustum = pCamera->GetViewFrustum();
                    NiCamera* pGameCamera = Main::GetWorldSceneGraph()->GetCamera();
                    kGameFrustum = pGameCamera->GetViewFrustum();
                    NiRenderer* pRenderer = NiRenderer::GetRenderer();
                    double dScreenWidth = static_cast<double>(pRenderer->GetScreenWidth());
                    pRenderer = NiRenderer::GetRenderer();
                    float fWideMult = dScreenWidth / static_cast<double>(pRenderer->GetScreenHeight()) /
                        1.3333333730697632;
                    kGameFrustum.m_fLeft = kVATSFrustum.m_fLeft * fWideMult;
                    kGameFrustum.m_fRight = kVATSFrustum.m_fRight * fWideMult;
                    kGameFrustum.m_fTop = kVATSFrustum.m_fTop;
                    kGameFrustum.m_fBottom = kVATSFrustum.m_fBottom;
                    pGameCamera->SetViewFrustum(kGameFrustum);
                }
            }
        }
        else {
            pPlayer->SetFirstPerson(true);
        }
        double dDistance = (GetTargetPos() - kCameraPos).Length();
        if (fVATSCameraCutAwayDistance.Float() >= dDistance) {
            bCameraTooClose = true;
        }
    }
}

NiPoint3 BGSCameraShot::GetTargetPos() {
    NiPoint3 kTargetPos = NiPoint3::ZERO;
    if (!(Data.fTargetPercentBetweenActors > 0.0)) {
        if (spTargetNode.m_pObject) {
            kTargetPos = spTargetNode->GetWorldTranslate();
        }
    }
    else {
        float fPercentage = Data.fTargetPercentBetweenActors;
        kTargetPos = pVATS.GetTargetPercentagePos(fPercentage);
    }
    if (cCameraHasRotation == 1 && spCameraNode.m_pObject) {
        (&spCameraNode->m_kLocal)
            ->m_Rotate.ToEulerAnglesXYZ(kVATSCameraAngle.x, kVATSCameraAngle.y, kVATSCameraAngle.z);
    }
    else {
        kVATSCameraAngle.x = 0.0f;
        kVATSCameraAngle.y = 0.0f;
        kVATSCameraAngle.z = 0.0f;
    }
    return kTargetPos;
}

bool HitData::IsFlagSet(Flag aeFlag) {
    return (uiFlags & aeFlag) != 0;
}
