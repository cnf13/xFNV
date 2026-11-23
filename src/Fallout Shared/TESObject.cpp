#include "Fallout Shared/TESObject.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESObjectTREE.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdexcept.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <memory>
#include <string>

NiNPShortPoint3::NiNPShortPoint3(int32_t aiX, int32_t aiY, int32_t aiZ)
  : x(static_cast<int16_t>(aiX)), y(static_cast<int16_t>(aiY)), z(static_cast<int16_t>(aiZ)) {}

float NiNPShortPoint3::GetFloatX() const {
    return static_cast<float>(x);
}

float NiNPShortPoint3::GetFloatY() const {
    return static_cast<float>(y);
}

float NiNPShortPoint3::GetFloatZ() const {
    return static_cast<float>(z);
}

float NiNPShortPoint3::Length() const {
    float fLengthSquared = static_cast<float>(static_cast<double>(z * z + (y * y + x * x)));
    return NiSqrt(fLengthSquared);
}

// The component-wise difference this - arPoint, constructed in apResult.
NiNPShortPoint3*
NiNPShortPoint3::CalculateRelativeBounds(NiNPShortPoint3* apResult, const NiNPShortPoint3& arPoint) const {
    ::new (apResult) NiNPShortPoint3(x - arPoint.x, y - arPoint.y, z - arPoint.z);
    return apResult;
}

NiNPShortPoint3& NiNPShortPoint3::operator=(const NiNPShortPoint3& arPt) {
    x = arPt.x;
    y = arPt.y;
    z = arPt.z;
    return *this;
}

const NiNPShortPoint3& NiNPShortPoint3::GetZERO() {
    if ((ZERO_guard & 1) == 0) {
        ZERO_guard |= 1u;
        NiNPShortPoint3::ZERO.~NiNPShortPoint3();
        ::new (&NiNPShortPoint3::ZERO) NiNPShortPoint3(0, 0, 0);
    }
    return NiNPShortPoint3::ZERO;
}

TESObject::TESObject() : m_pParentList(nullptr), nod_lpPrev(nullptr), nod_lpNext(nullptr) {}

TESObject::~TESObject() {
    if (m_pParentList) {
        m_pParentList->lnk_Remove(this);
    }
}

NiAVObject* TESObject::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    return nullptr;
}

void TESObject::UnClone3D(TESObjectREFR* apRequester) {
    if (this == pDoorMarker || this == pTravelMarker) {
        apRequester = nullptr;
    }
    const char* pWorldFilename = GetModelFileName(apRequester);
    if (pWorldFilename && *pWorldFilename) {
        if (!apRequester || apRequester->GetScale() == 1.0) {
            pModelLoader->ReleaseFile(pWorldFilename);
        }
        else {
            char cPath[264];
            GetScaledModelFileName(apRequester, cPath);
            pModelLoader->ReleaseFile(cPath);
            bool bLastCopy = true;
            NiPointer<Model> spModel;
            if (pModelLoader->FindModel(cPath, spModel) && spModel->iManualRefCount > 0) {
                bLastCopy = false;
            }
            if (bLastCopy) {
                pModelLoader->ReleaseFile(pWorldFilename);
                --iNumScaledModelsAdded;
            }
            else {
                --iNumScaledModelsCloned;
            }
        }
    }

    if (GetFormType() == NPC__ID) {
        static_cast<TESNPC*>(this)->TESNPC::ClearFaceGenModels();
    }
}

bool TESObject::IsMarker() {
    if (!this) {
        return false;
    }
    switch (GetFormType()) {
        case TXST_ID:
        case IDLM_ID: return true;
        case ACTI_ID:
            if (!BSstricmp(TESModel::GetModel(this), "EditorMarker.NIF")) {
                return true;
            }
            break;
        case DOOR_ID:
            if (pJailMarker == this) {
                return true;
            }
            break;
        case LIGH_ID:
            if (!TESModel::GetModelLength(this)) {
                return true;
            }
            break;
        case STAT_ID:
            if (pDoorMarker == this ||
                pTravelMarker == this ||
                pTempleMarker == this ||
                pHolyMarker == this ||
                pNorthMarker == this ||
                pMapMarker == this ||
                pHorseMarker == this ||
                pXMarker == this ||
                pXMarkerHeading == this ||
                pCOCMarkerHeading == this ||
                pBoundMarker == this ||
                pRadiationMarker == this ||
                pAudioMarker == this ||
                pAudioBuoyMarker == this) {
                return true;
            }
            break;
        default: break;
    }
    // The form ID is tested unsigned.
    uint32_t uiFormID = iFormID;
    return uiFormID >= 100 && uiFormID <= 119;
}

bool TESObject::IsOcclusionMarker() {
    if (!this) {
        return false;
    }
    return GetFormType() == 32 && (pRoomMarker == this || pPlaneMarker == this || pPortalMarker == this);
}

bool TESObject::ReplaceModel() {
    NiNode* pNode = static_cast<NiNode*>(LoadGraphics(nullptr));
    pNode->UpdateProperties();
    NiUpdateData kUpdateData(0.0, false, false);
    pNode->Update(kUpdateData);
    return true;
}

NiAVObject* TESObject::LoadGraphics(TESObjectREFR* apRef) {
    const char* pWorldFilename = GetModelFileName(apRef);
    NiAVObject* pData3D = nullptr;
    if (apRef && apRef->IsMarker() && apRef->GetObjectReference()->GetFormType() != LIGH_ID) {
        NiPointer<Model> spModel;
        pModelLoader->FindModel(pWorldFilename, spModel);
        if (spModel.m_pObject) {
            spModel->IncManualRefCount();
        }
        pData3D = new BSFadeNode;
    }
    else {
        ENUM_LOD_MULT eLOD = TES::GetLODMult(this);
        if (apRef && apRef->GetScale() != 1.0) {
            char cScaledPath[264];
            GetScaledModelFileName(apRef, cScaledPath);
            NiPointer<Model> spModel;
            pModelLoader->FindModel(cScaledPath, spModel);
            if (spModel.m_pObject) {
                spModel->IncManualRefCount();
                pData3D = spModel->GetModel();
                ++iNumScaledModelsCloned;
            }
        }

        if (!pData3D) {
            return pModelLoader->LoadFile(pWorldFilename, eLOD, true, false, false, false);
        }
    }
    return pData3D;
}

NiAVObject* TESObject::Get3D() {
    return nullptr;
}

const char* TESObject::GetModelFileName(TESObjectREFR* apRef) {
    const char* pPath = nullptr;
    if (apRef) {
        pPath = apRef->GetModel();
    }
    if (!pPath) {
        pPath = TESBipedModelForm::GetWorldModel(this, apRef);
    }
    if (!pPath) {
        return TESModel::GetModel(this);
    }
    return pPath;
}

void TESObject::GetScaledModelFileName(TESObjectREFR* apRef, char* apScaledFileName) {
    const char* pWorldFilename = GetModelFileName(apRef);
    float fHeight = apRef->GetScale() * 100.0;
    BSsprintf(apScaledFileName, 260u, "%s%i", pWorldFilename, FloatToInt(fHeight));
}

// The constructor deliberately leaves both points unset.
TESBoundObject::BOUND_DATA::BOUND_DATA() {}

void TESBoundObject::BOUND_DATA::Endian() {
    ENDIAN(BoundMax.x, false);
    ENDIAN(BoundMax.y, false);
    ENDIAN(BoundMax.z, false);
    ENDIAN(BoundMin.x, false);
    ENDIAN(BoundMin.y, false);
    ENDIAN(BoundMin.z, false);
}

TESBoundObject::TESBoundObject() {
    BoundData.BoundMin = NiNPShortPoint3::GetZERO();
    BoundData.BoundMax = NiNPShortPoint3::GetZERO();
}

TESBoundObject::~TESBoundObject() {}

void TESBoundObject::SaveObjectBound() {
    if (TESForm::QEndianSwapOnSave()) {
        BoundData.Endian();
    }
    TESForm::__AddChunkData(OBND_ID, &BoundData, 0xCu);
    if (TESForm::QEndianSwapOnSave()) {
        BoundData.Endian();
    }
}

void TESBoundObject::LoadObjectBound(TESFile* apFile) {
    if (apFile && apFile->GetTESChunk() == OBND_ID) {
        apFile->GetChunkData(&BoundData, 0xCu);
        if (apFile->QEndian()) {
            BoundData.Endian();
        }
    }
}

bool TESBoundObject::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    Actor* pActor = dynamic_cast<Actor*>(apActionRef);
    if (!pActor || !apItemActivated) {
        return false;
    }
    pActor->PickUpObject(apItemActivated, aicount, abIdFlag);
    return true;
}

// The requester's copy of this object's 3D. A loaded model that only this caller holds is shared as is, unless the
// object needs its own copy (morphers, a door without an Unequip sequence, a ragdoll, add-ons, texture swaps,
// emittance, a scale other than the model's); otherwise it is cloned (deep, with the same textures, when deepCopy), and
// a scaled copy is registered with the model loader under the scaled file name. The scaled clone is built in a second
// cloning process, separate from kCloneProc, which stays live until the end. The requester's scale and the scaled
// model's 3D scale are stored as doubles before the compares, so those locals are doubles.
NiAVObject* TESBoundObject::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    NiAVObject* pNode = nullptr;
    if (this == pDoorMarker || this == pTravelMarker) {
        pRequester = nullptr;
    }
    NiAVObject* pLoadedNode = LoadGraphics(pRequester);
    if (pRequester && pRequester->IsMarker() && pRequester->GetObjectReference()->GetFormType() != LIGH_ID) {
        return pLoadedNode;
    }
    if (!pLoadedNode) {
        return pNode;
    }
    if (GetNoCollision() && pRequester) {
        pRequester->SetNoCollision(true);
    }
    if (!deepCopy && pLoadedNode->GetRefCount() == 1) {
        bool bShare = false;
        switch (GetFormType()) {
            case ACTI_ID:
            case CREA_ID:
                // Only sets the flag; it was clear before this point.
                if (HasMorpherController(static_cast<NiNode*>(pLoadedNode))) {
                    deepCopy = true;
                }
                break;
            case DOOR_ID: {
                bShare = true;
                NiNode* pDoorNode = pLoadedNode->IsNode();
                if (pDoorNode && pDoorNode->GetAtChecked(0) && pDoorNode->GetAt(0)->GetControllers()) {
                    NiTimeController* pControllers = pDoorNode->GetAt(0)->GetControllers();
                    NiControllerManager* pControllerMgr = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
                    );
                    if (pControllerMgr && pControllerMgr->GetSequenceByName(FixedStrings::QUnequip())) {
                        bShare = false;
                    }
                }
                break;
            }
            case PWAT_ID:
            case WEAP_ID:
            case AMMO_ID:
            case NPC__ID:
            case PROJ_ID:
            case EXPL_ID: break;
            default: bShare = !TESHavokUtilities::Should3DRagDoll(pRequester, pLoadedNode); break;
        }
        if (bShare) {
            BSXFlags* pBSXFlags = static_cast<BSXFlags*>(pLoadedNode->GetExtraData(BSXFlags::Tag()));
            // Tests only the low byte of QAddon's result.
            if (pBSXFlags && static_cast<uint8_t>(pBSXFlags->QAddon())) {
                bShare = false;
            }
        }
        if (bShare && TESModelTextureSwap::HasTextureSwap(this, pRequester)) {
            bShare = false;
        }
        if (bShare && BSUtilities::GetExternalEmittanceRecurse(pLoadedNode)) {
            bShare = false;
        }
        if (bShare) {
            if (!pRequester) {
                pNode = pLoadedNode;
            }
            else {
                double dScaledHeight = pRequester->GetScale();
                if (dScaledHeight == pLoadedNode->GetScale()) {
                    pNode = pLoadedNode;
                }
            }
        }
    }
    if (!pNode) {
        float fScaledHeight = 1.0f;
        if (pRequester) {
            fScaledHeight = pRequester->GetScale();
        }
        if (fScaledHeight == pLoadedNode->GetScale()) {
            fScaledHeight = 1.0f;
        }
        if (fScaledHeight == 0.0) {
            Warning("MODELS: %s has a scale of 0.  Change it in the editor.", pRequester->GetFullName());
            fScaledHeight = 1.0f;
        }
        BSCloningProcess kCloneProc(fScaledHeight);
        if (deepCopy) {
            NiObject* pClonedObject = TES::pInstance->CreateDeepCopySameTextures(pLoadedNode, kCloneProc);
            if (pClonedObject && pClonedObject->IsNode()) {
                pNode = pClonedObject->IsNode();
            }
            else {
                NiNode* pNewNode = new NiNode(0);
                pNewNode->AttachChild(static_cast<NiAVObject*>(pClonedObject), true);
                pNode = pNewNode;
            }
        }
        else {
            pNode = static_cast<NiAVObject*>(pLoadedNode->Clone(kCloneProc));
        }
        if (fScaledHeight != 1.0 && pLoadedNode->GetScale() == 1.0) {
            pNode->SetScale(fScaledHeight);
            char cScaledPath[0x104];
            GetScaledModelFileName(pRequester, cScaledPath);
            alignas(NiPointer<Model>) uint8_t aScaledModelStorage[sizeof(NiPointer<Model>)];
            NiPointer<Model>& spScaledModel = *::new (aScaledModelStorage) NiPointer<Model>(nullptr);
            pModelLoader->FindModel(cScaledPath, spScaledModel);
            if (spScaledModel) {
                if (spScaledModel->GetModel()) {
                    double dLocalScale = spScaledModel->GetModel()->GetScale();
                    if (dLocalScale == pRequester->GetScale()) {
                        NiAVObject* pResult = Clone3D(pRequester, deepCopy);
                        NiPointer<Model>::Destroy(&spScaledModel);
                        return pResult;
                    }
                }
                Warning("Invalid scale found on ModelLoader copy of file '%s'", cScaledPath);
            }
            else {
                Model* pScaledModel = new Model(cScaledPath, static_cast<NiNode*>(pNode));
                spScaledModel = pScaledModel;
                spScaledModel->IncManualRefCount();
                if (pModelLoader->AddModel(cScaledPath, spScaledModel)) {
                    ++iNumScaledModelsAdded;
                }
                else {
                    Model* pFailedModel = spScaledModel;
                    spScaledModel = static_cast<Model*>(nullptr);
                    if (pFailedModel) {
                        delete pFailedModel;
                    }
                }
                BSCloningProcess kScaledCloneProc(1.0f);
                pNode = static_cast<NiAVObject*>(pNode->Clone(kScaledCloneProc));
            }
            NiPointer<Model>::Destroy(&spScaledModel);
        }
    }
    if (!pNode) {
        return pNode;
    }
    pNode->SetScale(1.0f);
    pNode->SetAppCulled(false);
    pNode->SetRotate(NiMatrix3::IDENTITY);
    pNode->SetTranslate(NiPoint3::ZERO);
    if (pNode->GetParent()) {
        pNode->DetachParent();
    }
    return pNode;
}

bool TESBoundObject::ReplaceModel() {
    return ReplaceModel(TESModel::GetModel(this));
}

NiAVObject* TESBoundObject::Clone3D(TESObjectREFR* pRequester) {
    return Clone3D(pRequester, false);
}

bool TESBoundObject::ReplaceModel(const char* pfilename) {
    const char* pModel = TESModel::GetModel(this);
    pModelLoader->ReleaseFile(pModel);
    NiNode* pNode = static_cast<NiNode*>(LoadGraphics(nullptr));
    pNode->UpdateProperties();
    NiUpdateData kUpdateData(0.0, false, false);
    pNode->Update(kUpdateData);
    return true;
}

// Half the length of the bound box's diagonal.
float TESBoundObject::GetBoundSize() {
    NiNPShortPoint3 kDiff;
    return BoundData.BoundMin.CalculateRelativeBounds(&kDiff, BoundData.BoundMax)->Length() * 0.5;
}

// Reads the coordinates z first, then y, then x.
NiPoint3 TESBoundObject::GetBoundMin() {
    float fZ = BoundData.BoundMin.GetFloatZ();
    float fY = BoundData.BoundMin.GetFloatY();
    float fX = BoundData.BoundMin.GetFloatX();
    return NiPoint3(fX, fY, fZ);
}

NiPoint3 TESBoundObject::GetBoundMax() {
    float fZ = BoundData.BoundMax.GetFloatZ();
    float fY = BoundData.BoundMax.GetFloatY();
    float fX = BoundData.BoundMax.GetFloatX();
    return NiPoint3(fX, fY, fZ);
}

void TESBoundObject::CopyBoundData(TESBoundObject* apCopy) {
    ourMemcpy(&BoundData, &apCopy->BoundData, sizeof(BoundData));
}

TESBoundAnimObject::TESBoundAnimObject() {}

TESBoundAnimObject::~TESBoundAnimObject() {}

// Every derived class whose slot holds this address inherits it.
void TESBoundAnimObject::Set3D(NiAVObject* apObject, bool abTestProcessLevel) {
    TESObject::Set3D(apObject, abTestProcessLevel);
}

bool TESBoundAnimObject::ReplaceModel(const char* pfilename) {
    return TESBoundObject::ReplaceModel(pfilename);
}

TESBoundTreeObject::TESBoundTreeObject() {}

TESBoundTreeObject::~TESBoundTreeObject() {}

NiAVObject* TESBoundTreeObject::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    BSTreeNode* pNode = nullptr;
    if (GetFormType() != TREE_ID) {
        return nullptr;
    }
    if (BSTreeManager::GetInstance(true)) {
        pNode = BSTreeManager::GetInstance(true)->CreateTreeForReference(pRequester, static_cast<TESObjectTREE*>(this));
    }
    RefreshBound(pNode);
    if (pNode && pNode->GetModel() && pRequester) {
        BSTreeModel* pModel = pNode->GetModel();
        pRequester->SetSeed(pModel->iSeed);
    }

    return pNode;
}

TESObjectList::TESObjectList(bool modelload)
  : lnk_dwCount(0), lnk_lpRoot(nullptr), lnk_lpTail(nullptr), pOwner(nullptr) {}

TESObjectList::~TESObjectList() {
    lnk_Destroy();
}

// Links lpNew in after the tail.
TESObject* TESObjectList::lnk_Append(TESObject* lpNew) {
    return lnk_Place(lnk_lpTail, lpNew);
}

TESObject* TESObjectList::lnk_Place(TESObject* lpPrev, TESObject* lpNew) {
    lpNew->nod_lpPrev = lpPrev;
    if (lpPrev) {
        lpNew->nod_lpNext = lpPrev->nod_lpNext;
        if (lpPrev->nod_lpNext) {
            lpPrev->nod_lpNext->nod_lpPrev = lpNew;
        }
        lpPrev->nod_lpNext = lpNew;
    }
    else {
        lpNew->nod_lpNext = nullptr;
    }
    if (!lnk_GetNext(lpNew)) {
        lnk_lpTail = lpNew;
    }
    if (!lnk_GetPrev(lpNew)) {
        lnk_lpRoot = lpNew;
    }
    ++lnk_dwCount;
    lpNew->m_pParentList = this;
    return lpNew;
}

TESObject* TESObjectList::lnk_GetNext(TESObject* lpNode) {
    return lpNode->nod_GetNext();
}

void TESObjectList::lnk_Remove(TESObject* lpNode) {
    if (lpNode == lnk_lpRoot) {
        lnk_lpRoot = lnk_lpRoot->nod_lpNext;
    }
    if (lpNode == lnk_lpTail) {
        lnk_lpTail = lnk_lpTail->nod_lpPrev;
    }
    TESObject* pPrev = lpNode->nod_lpPrev;
    if (pPrev) {
        pPrev->nod_lpNext = lpNode->nod_lpNext;
    }
    TESObject* pNext = lpNode->nod_lpNext;
    if (pNext) {
        pNext->nod_lpPrev = lpNode->nod_lpPrev;
    }
    lpNode->m_pParentList = nullptr;
    --lnk_dwCount;
}

void TESObjectList::lnk_Destroy() {
    while (lnk_lpTail) {
        TESObject* pPrev = lnk_lpTail->nod_lpPrev;
        if (lnk_lpTail) {
            delete lnk_lpTail;
        }
        lnk_lpTail = pPrev;
    }
    lnk_dwCount = 0;
    lnk_lpRoot = nullptr;
    lnk_lpTail = nullptr;
}

TESObject* TESObjectList::lnk_GetPrev(TESObject* lpNode) {
    return lpNode->nod_lpPrev;
}

void BSBound::GetBoundMin(NiPoint3& arPoint) {
    NiPoint3 kMin;
    arPoint = *&(kMin = Center - Extents);
}

void BSBound::GetBoundMax(NiPoint3& arPoint) {
    NiPoint3 kMax;
    arPoint = *&(kMax = Center + Extents);
}

const NiFixedString& BSBound::Tag() {
    return *BSBound::pTag;
}

NiFixedString& FixedStrings::QUnequip() {
    return *pUnequip;
}
