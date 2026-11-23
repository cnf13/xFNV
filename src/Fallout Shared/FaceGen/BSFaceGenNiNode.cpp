#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"

#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenUtils.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>

NiRTTI BSFaceGenNiNode::ms_RTTI("BSFaceGenNiNode", &NiNode::ms_RTTI);

FaceGenUpdateData::FaceGenUpdateData() {}

BSFaceGenNiNode::BSFaceGenNiNode()
  : NiNode(0), spAnimationData(nullptr), BaseRotation(NiMatrix3::IDENTITY), bForceBaseMorph(false),
    bFixedNormals(false), bAnimationUpdate(true), bRotatedLastUpdate(true), bApplyRotationToParent(false),
    fLastTime(-1.0f), bUsingLoResHead(false), bIAmPlayerCharacter(false), bIAmInDialogue(false), pActor(nullptr),
    bCycleFaceGenCoord(false) {}

BSFaceGenNiNode::~BSFaceGenNiNode() {
    pActor = nullptr;
}

NiObject* BSFaceGenNiNode::CreateClone(NiCloningProcess& arCloneData) {
    BSFaceGenNiNode* pclone = new BSFaceGenNiNode;
    CopyMembers(pclone, arCloneData);
    ProcessClone(arCloneData);
    return pclone;
}

void BSFaceGenNiNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiNode::GetViewerStrings(apStrings);
    char* element = NiGetViewerString(BSFaceGenNiNode::ms_RTTI.m_pcName);
    apStrings->Add(element);
    char* message = static_cast<char*>(ourMalloc(32u));
    const char* pcValue = bForceBaseMorph ? "true" : "false";
    BSsprintf(message, 32, "bForceBaseMorph = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bFixedNormals ? "true" : "false";
    BSsprintf(message, 32, "bFixedNormals = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bAnimationUpdate ? "true" : "false";
    BSsprintf(message, 32, "bAnimationUpdate = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bRotatedLastUpdate ? "true" : "false";
    BSsprintf(message, 32, "bRotatedLastUpdate = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bApplyRotationToParent ? "true" : "false";
    BSsprintf(message, 32, "bApplyRotationToParent = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bUsingLoResHead ? "true" : "false";
    BSsprintf(message, 32, "bUsingLoResHead = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bIAmPlayerCharacter ? "true" : "false";
    BSsprintf(message, 32, "bIAmPlayerCharacter = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    pcValue = bIAmInDialogue ? "true" : "false";
    BSsprintf(message, 32, "bIAmInDialogue = %s", pcValue);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    BSsprintf(message, 32, "fLastTime = %0.3f", fLastTime);
    apStrings->Add(message);
    message = static_cast<char*>(ourMalloc(32u));
    BSsprintf(message, 32, "pActor = %08X", pActor);
    apStrings->Add(message);
    if (pActor) {
        message = static_cast<char*>(ourMalloc(32u));
        int32_t iFormID = pActor->iFormID;
        BSsprintf(message, 32, "pActor->iFormID = %08X", iFormID);
        apStrings->Add(message);
    }
    message = static_cast<char*>(ourMalloc(32u));
    BSsprintf(message, 32, "spAnimationData = %08X", spAnimationData.m_pObject);
    apStrings->Add(message);
    if (spAnimationData.m_pObject) {
        for (BS_FACEGEN_EXPRESSION eexp = BS_FG_EXPRESSION_ANGER; eexp < BS_FG_EXPRESSION_COUNT;
             eexp = static_cast<BS_FACEGEN_EXPRESSION>(eexp + 1)) {
            if (spAnimationData->GetExpressionValue(eexp) != 3.4028234663852886e+38) {
                float fValue = spAnimationData->GetExpressionValue(eexp);
                const char* pcText = BSFaceGenManager::GetFaceGenExpressionText(eexp);
                char* pcViewerString = NiGetViewerString(pcText, fValue);
                apStrings->Add(pcViewerString);
            }
        }
        for (BS_FACEGEN_MODIFIER emod = BS_FG_MODIFIER_BLINK_LEFT; emod < BS_FG_MODIFIER_COUNT;
             emod = static_cast<BS_FACEGEN_MODIFIER>(emod + 1)) {
            if (spAnimationData->GetModifierValue(emod) != 3.4028234663852886e+38) {
                float fValue = spAnimationData->GetModifierValue(emod);
                const char* pcText = BSFaceGenManager::GetFaceGenModifierText(emod);
                char* pcViewerString = NiGetViewerString(pcText, fValue);
                apStrings->Add(pcViewerString);
            }
        }
        for (BS_FACEGEN_PHONEME ephon = BS_FG_PHONEME_AAH; ephon < BS_FG_PHONEME_COUNT;
             ephon = static_cast<BS_FACEGEN_PHONEME>(ephon + 1)) {
            if (spAnimationData->GetPhonemeValue(ephon) != 3.4028234663852886e+38) {
                float fValue = spAnimationData->GetPhonemeValue(ephon);
                const char* pcText = BSFaceGenManager::GetFaceGenPhonemeText(ephon);
                char* pcViewerString = NiGetViewerString(pcText, fValue);
                apStrings->Add(pcViewerString);
            }
        }
    }
}

void BSFaceGenNiNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (!GetAppCulled()) {
        UpdateMorphing(UpdateData);
    }
    NiNode::UpdateDownwardPass(UpdateData, uFlags);
}

void BSFaceGenNiNode::UpdateTransformAndBounds(NiUpdateData& UpdateData) {
    UpdateMorphing(UpdateData);
    NiNode::UpdateTransformAndBounds(UpdateData);
}

// The accumulator is the culler's: the culling process is downcast to BSCullingProcess, and its accumulator getter is
// called twice.
void BSFaceGenNiNode::OnVisible(NiCullingProcess& arCuller) {
    BSShaderAccumulator* paccum = static_cast<BSCullingProcess&>(arCuller).GetAccumulator();
    uint32_t kmode = static_cast<BSCullingProcess&>(arCuller).GetAccumulator()->eRenderMode;
    if (!kmode) {
        paccum->StartGroupingAlphas(GetWorldBound(), false);
    }
    NiNode::OnVisible(arCuller);
    if (!kmode) {
        paccum->StopGroupingAlphas();
    }
}

BSFaceGenAnimationData* BSFaceGenNiNode::GetAnimationData() {
    return spAnimationData;
}

void BSFaceGenNiNode::SetAnimationData(BSFaceGenAnimationData* apData) {
    if (spAnimationData.m_pObject == apData) {
        return;
    }
    if (spAnimationData.m_pObject) {
        NiDecRefCount(spAnimationData.m_pObject);
    }
    spAnimationData.m_pObject = apData;
    if (apData) {
        NiIncRefCount(apData);
    }
}

NiMatrix3 BSFaceGenNiNode::GetBaseRotation() {
    return BaseRotation;
}

void BSFaceGenNiNode::SetBaseRotation(const NiMatrix3& aRot) {
    BaseRotation = aRot;
    bRotatedLastUpdate = true;
    SetRotate(GetBaseRotation());
}

bool BSFaceGenNiNode::GetAnimationUpdate() {
    return bAnimationUpdate;
}

void BSFaceGenNiNode::SetAnimationUpdate(bool abUpdate) {
    bAnimationUpdate = abUpdate;
}

bool BSFaceGenNiNode::GetApplyRotToParent() {
    return bApplyRotationToParent;
}

void BSFaceGenNiNode::SetApplyRotToParent(bool abApply) {
    bApplyRotationToParent = abApply;
}

bool BSFaceGenNiNode::GetFixedNormals() {
    return bFixedNormals;
}

void BSFaceGenNiNode::SetFixedNormals(bool abFixedNormals) {
    bFixedNormals = abFixedNormals;
}

void BSFaceGenNiNode::FixSkinInstances(NiNode* apRoot, bool abWarnOnFail) {
    if (apRoot) {
        uint32_t iobjCount = GetArrayCount();
        for (uint32_t iobj = 0; iobj < iobjCount; ++iobj) {
            NiAVObject* pobjTemp = GetAt(iobj);
            if (pobjTemp) {
                NiTriBasedGeom* ptri = pobjTemp->IsTriBasedGeom();
                if (ptri) {
                    NiSkinInstance* pskin = ptri->GetSkinInstance();
                    if (pskin && pskin->GetSkinData()) {
                        uint32_t iboneCount = pskin->GetSkinData()->m_uiBones;
                        NiAVObject** pbones = pskin->m_ppkBones;
                        if (pbones && iboneCount) {
                            for (uint32_t ibone = 0; ibone < iboneCount; ++ibone) {
                                NiAVObject* pbone = apRoot->GetObjectByName(*&pbones[ibone]->m_kName);
                                if (pbone) {
                                    pskin->SetBone(ibone, pbone);
                                }
                                else if (abWarnOnFail) {
                                    Warning(
                                        "MODELS: Could not find bone \"%s\" for the head node.",
                                        pbones[ibone]->m_kName.GetHandleString()
                                    );
                                }
                            }
                            pskin->m_pkRootParent = this;
                        }
                    }
                }
            }
        }
    }
}

void BSFaceGenNiNode::SetIAmPlayerCharacter(bool abIAmPC) {
    bIAmPlayerCharacter = abIAmPC;
}

bool BSFaceGenNiNode::GetIAmPlayerCharacter() {
    return bIAmPlayerCharacter;
}

void BSFaceGenNiNode::UpdateAllChildrenMorphData(bool abChanged) {
    FaceGenUpdateData objdata;
    uint32_t iobjCount = GetArrayCount();
    for (uint32_t i = 0; i < iobjCount; ++i) {
        objdata.bbaseApplied = false;
        NiAVObject* pobj = GetAt(i);
        if (pobj) {
            objdata.ptri = pobj->IsTriBasedGeom();
            if (objdata.ptri && objdata.ptri->GetModelData()) {
                objdata.pmorph = BSFaceGenManager::GetFaceGenMorphDataHead(objdata.ptri);
                if (objdata.pmorph) {
                    objdata.pmorphBase = static_cast<BSFaceGenBaseMorphExtraData*>(
                        BSFaceGenUtils::GetFaceGenExtraData<BSFaceGenBaseMorphExtraData>(objdata.ptri)
                    );
                    if (objdata.pmorphBase && objdata.ptri->GetModelData()->LockPackedVertexData(true)) {
                        objdata.ptri->GetModelData()->GetVerticesIterator(objdata.iter);
                        objdata.ivertCount = objdata.ptri->GetVertexCount();
                        if (abChanged || bForceBaseMorph) {
                            BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(objdata.ptri, objdata.iter);
                            objdata.bbaseApplied = true;
                            bForceBaseMorph = false;
                        }
                        bool bmodAdded = false;
                        bool bphoAdded = false;
                        NiPoint3 cameraloc = BSFaceGenManager::GetCameraPosition();
                        float buffer = 10.0f;
                        float dist = (cameraloc - GetWorldTranslate()).Length();
                        float morphdist = BSFaceGenManager::GetLodDistance();
                        float talkdist = BSFaceGenManager::GetTalkingLodDistance();
                        if (Main::GetSceneIsComplex()) {
                            morphdist = morphdist * 0.25;
                            talkdist = talkdist * 0.25;
                        }
                        bool bdophonemes = false;
                        bool bdomodifiers = false;
                        if (dist <= talkdist + buffer || (bIAmInDialogue || Interface::IsInMenuMode())) {
                            bdophonemes = true;
                            if (dist <= morphdist + buffer || (bIAmInDialogue || Interface::IsInMenuMode())) {
                                bdomodifiers = true;
                            }
                        }
                        if (bForceMorphUpdates && !objdata.bbaseApplied) {
                            BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(objdata.ptri, objdata.iter);
                            objdata.bbaseApplied = true;
                        }
                        if (bdophonemes && !bDisableMorphUpdates) {
                            ApplyFacialPhonemes(objdata);
                            bphoAdded = true;
                            if (bdomodifiers) {
                                ApplyFacialExpressions(objdata);
                                ApplyFacialModifiers(objdata);
                                ApplyFacialCustomMorphs(objdata);
                                bmodAdded = true;
                            }
                        }
                        if (GetAnimationData()->GetDead()) {
                            if (!bmodAdded) {
                                if (GetAnimationData()->GetModifierValue(BS_FG_MODIFIER_BLINK_LEFT) > 0.0) {
                                    NiNode* pparent = m_pkParent;
                                    if (pparent->IsFadeNode()) {
                                        ApplyFacialModifiers(objdata);
                                        bmodAdded = true;
                                    }
                                }
                            }
                        }
                        else if (!bphoAdded && dist > BSFaceGenManager::GetLodDistance() - buffer) {
                            BSFaceGenAnimationData* pAnimData = GetAnimationData();
                            pAnimData->ResetCurrentMorphs(0.0f, true, true, true, true, false);
                        }
                        objdata.ptri->GetModelData()->UnlockPackedVertexData();
                    }
                }
            }
        }
    }
    bForceMorphUpdates = false;
}

// Each value is tested against 0.0 twice (> then >=) before <= 1.0, all as doubles.
void BSFaceGenNiNode::ApplyFacialExpressions(FaceGenUpdateData& arObjData) {
    for (int32_t i = 0; i < BS_FG_EXPRESSION_COUNT; ++i) {
        float fValue = GetAnimationData()->GetExpressionValue(static_cast<BS_FACEGEN_EXPRESSION>(i));
        if (fValue > 0.0 && fValue >= 0.0 && fValue <= 1.0) {
            if (!arObjData.bbaseApplied) {
                BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(arObjData.ptri, arObjData.iter);
                arObjData.bbaseApplied = true;
            }
            arObjData.pmorph->ApplyMorph(BS_FG_MORPH_EXPRESSION, i, arObjData.ptri, arObjData.pmorphBase, fValue);
        }
    }
}

// The look modifiers come in exclusive pairs: down/up and left/right are split into their negative and positive parts.
// The head rotation modifiers are not morphs.
void BSFaceGenNiNode::ApplyFacialModifiers(FaceGenUpdateData& arObjData) {
    for (int32_t i = 0; i < BS_FG_MODIFIER_COUNT; ++i) {
        float fValue;
        float fNeg;
        float fPos;
        switch (i) {
            case BS_FG_MODIFIER_LOOK_DOWN:
            case BS_FG_MODIFIER_LOOK_UP: {
                float fUp = GetAnimationData()->GetModifierValue(BS_FG_MODIFIER_LOOK_UP);
                float fDown = GetAnimationData()->GetModifierValue(BS_FG_MODIFIER_LOOK_DOWN);
                BSFaceGenManager::FixExclusiveValues(fDown, fUp, fNeg, fPos);
                fValue = i == BS_FG_MODIFIER_LOOK_DOWN ? fNeg : fPos;
                break;
            }
            case BS_FG_MODIFIER_LOOK_LEFT:
            case BS_FG_MODIFIER_LOOK_RIGHT: {
                float fRight = GetAnimationData()->GetModifierValue(BS_FG_MODIFIER_LOOK_RIGHT);
                float fLeft = GetAnimationData()->GetModifierValue(BS_FG_MODIFIER_LOOK_LEFT);
                BSFaceGenManager::FixExclusiveValues(fLeft, fRight, fNeg, fPos);
                fValue = i == BS_FG_MODIFIER_LOOK_LEFT ? fNeg : fPos;
                break;
            }
            case BS_FG_MODIFIER_HEAD_PITCH:
            case BS_FG_MODIFIER_HEAD_ROLL:
            case BS_FG_MODIFIER_HEAD_YAW: continue;
            default: fValue = GetAnimationData()->GetModifierValue(static_cast<BS_FACEGEN_MODIFIER>(i)); break;
        }
        if (fValue > 0.0 && fValue >= 0.0 && fValue <= 1.0) {
            if (!arObjData.bbaseApplied) {
                BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(arObjData.ptri, arObjData.iter);
                arObjData.bbaseApplied = true;
            }
            arObjData.pmorph->ApplyMorph(BS_FG_MORPH_MODIFIER, i, arObjData.ptri, arObjData.pmorphBase, fValue);
        }
    }
}

void BSFaceGenNiNode::ApplyFacialPhonemes(FaceGenUpdateData& arObjData) {
    for (int32_t i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
        float fValue = GetAnimationData()->GetPhonemeValue(static_cast<BS_FACEGEN_PHONEME>(i));
        if (fValue > 0.0 && fValue >= 0.0 && fValue <= 1.0) {
            if (!arObjData.bbaseApplied) {
                BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(arObjData.ptri, arObjData.iter);
                arObjData.bbaseApplied = true;
            }
            arObjData.pmorph->ApplyMorph(BS_FG_MORPH_PHONEME, i, arObjData.ptri, arObjData.pmorphBase, fValue);
        }
    }
}

void BSFaceGenNiNode::ApplyFacialCustomMorphs(FaceGenUpdateData& arObjData) {
    for (int32_t i = 0; i < BS_FG_CUSTOM_MORPH_COUNT; ++i) {
        float fValue = GetAnimationData()->GetCustomMorphValue(static_cast<BS_FACEGEN_CUSTOM_MORPH>(i));
        if (fValue > 0.0 && fValue >= 0.0 && fValue <= 1.0) {
            if (!arObjData.bbaseApplied) {
                BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(arObjData.ptri, arObjData.iter);
                arObjData.bbaseApplied = true;
            }
            arObjData.pmorph->ApplyMorph(BS_FG_MORPH_CUSTOM, i, arObjData.ptri, arObjData.pmorphBase, fValue);
        }
    }
}

void BSFaceGenNiNode::CheckLOD(BS_FACEGEN_LOD aiForceLOD) {
    NiPoint3 kToCamera;
    NiPoint3 cameraPosition = BSFaceGenManager::GetCameraPosition();
    float offset = 10.0f;
    const NiPoint3* worldTranslate = &NiAVObject::GetWorldTranslate();
    kToCamera = cameraPosition - *worldTranslate;
    float distance = kToCamera.Length();
    if ((!(distance > BSFaceGenManager::GetLodDistance() + offset) || GetUsingLoResHead()) && aiForceLOD) {
        if ((distance < BSFaceGenManager::GetLodDistance() - offset && GetUsingLoResHead()) || aiForceLOD == 1) {
            BSFaceGenManager::ReplaceFaceMeshLOD(this, 1);
        }
    }
    else {
        BSFaceGenManager::ReplaceFaceMeshLOD(this, 0);
    }
}

void BSFaceGenNiNode::SetForceBaseMorphs(bool abForceBaseMorph) {
    bForceBaseMorph = abForceBaseMorph;
}

void BSFaceGenNiNode::SetDisableMorphUpdates(bool abDisable) {
    bDisableMorphUpdates = abDisable;
}

void BSFaceGenNiNode::SetIAmInDialogue(bool abIAmInDialogue) {
    bIAmInDialogue = abIAmInDialogue;
}

void BSFaceGenNiNode::SetForceMorphUpdates(bool abForce) {
    bForceMorphUpdates = abForce;
}

void BSFaceGenNiNode::CopyMembers(BSFaceGenNiNode* apDest, NiCloningProcess& arCloneData) {
    if (apDest) {
        NiNode::CopyMembers(apDest, arCloneData);
        apDest->spAnimationData = spAnimationData;
        std::memcpy(
            &apDest->BaseRotation,
            &BaseRotation,
            offsetof(BSFaceGenNiNode, bApplyRotationToParent) - offsetof(BSFaceGenNiNode, BaseRotation)
        );
        apDest->fLastTime = fLastTime;
        apDest->bUsingLoResHead = bUsingLoResHead;
        apDest->bIAmPlayerCharacter = bIAmPlayerCharacter;
    }
}

bool BSFaceGenNiNode::GetUsingLoResHead() {
    return bUsingLoResHead;
}

// Calls whose arguments are evaluated before the object (GetParent, GetSeconds before GetAnimationData) go through
// locals to keep that order.
void BSFaceGenNiNode::UpdateMorphing(NiUpdateData& UpdateData) {
    if (GetAppCulled()) {
        return;
    }
    bool bChanged = false;
    if (BSFaceGenManager::GetUseLOD() && !GetIAmPlayerCharacter() && UpdateData.fTime > 0.0) {
        CheckLOD(BS_FACEGEN_LOD_NONE);
    }
    if (!GetAnimationData()) {
        return;
    }
    if (!UpdateData.bUpdateControllers && !GetAnimationData()->GetMustUpdate()) {
        return;
    }
    fLastTime = UpdateData.fTime;
    if (bAnimationUpdate) {
        GetAnimationData()->SetMustUpdate(false);
        if (appTimer.GetSecondsIsZero()) {
            bChanged = GetAnimationData()->Update(0.0f);
        }
        else {
            BSFaceGenAnimationData* pAnimationData = GetAnimationData();
            double fMult = fAnimationMult.Float();
            float fTime = appTimer.GetSeconds() * fMult;
            bChanged = pAnimationData->Update(fTime);
        }
    }
    else {
        bChanged = GetAnimationData()->GetChangedLastUpdate();
    }
    if (bAnimationUpdate &&
        pActor &&
        pActor->GetCurrentAIProcess() &&
        pActor->GetCurrentAIProcess()->GetCurrentHeadTrackTarget() &&
        GetAnimationData()->ShouldHeadTrack() &&
        bApplyRotationToParent &&
        !bDisableHeadTracking.Bool() &&
        GetSelectiveUpdate()) {
        bool bEyesChanged;
        if (appTimer.GetSecondsIsZero()) {
            NiNode* pParent = GetParent();
            bEyesChanged = GetAnimationData()->UpdateEyeTracking(0.0f, pParent);
        }
        else {
            NiNode* pParent = GetParent();
            float fSeconds = appTimer.GetSeconds();
            bEyesChanged = GetAnimationData()->UpdateEyeTracking(fSeconds, pParent);
        }
        bChanged = bChanged || bEyesChanged;
    }
    if (bChanged || bForceMorphUpdates) {
        int32_t iNumber = 10;
        bool bUpdate = false;
        if (GetSelectiveUpdate()) {
            if (GetAnimationData()->GetDead() ||
                ReferenceProcessLists.IsActorNCloseToPlayer(pActor, iNumber) ||
                pActor == pPlayer ||
                Interface::IsInMenuMode()) {
                bUpdate = true;
            }
            if (pActor &&
                pActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH &&
                pActor->GetCurrentAIProcess()->IsDoingSayTo() &&
                pActor->GetCurrentAIProcess()->GetDialogTarget() &&
                pActor->GetCurrentAIProcess()->GetDialogTarget() == pPlayer) {
                bUpdate = true;
            }
        }
        if (bUpdate || bForceMorphUpdates) {
            UpdateAllChildrenMorphData(bChanged);
        }
        else {
            bForceBaseMorph = true;
            bForceMorphUpdates = true;
        }
    }
    GetAnimationData()->SetShouldHeadTrack(true);
    if (bCycleFaceGenCoord && pActor) {
        BSFaceGenManager::CycleFaceGenCoordValue(pActor);
    }
}

bool BSTimer::GetSecondsIsZero() const {
    return bZeroDelta;
}

float BSFaceGenManager::GetLodDistance() {
    return fLodDistance;
}

float BSFaceGenManager::GetTalkingLodDistance() {
    return fTalkingDistance;
}

const NiPoint3& BSFaceGenManager::GetCameraPosition() {
    if (pWorldCamera) {
        return pWorldCamera->GetWorldTranslate();
    }
    return NiPoint3::ZERO;
}

bool BSFaceGenManager::GetUseLOD() {
    return bUseFaceGenLOD.Bool();
}
