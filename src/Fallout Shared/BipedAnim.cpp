#include "Fallout Shared/BipedAnim.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSDecalPlacementVectorExtraData.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/Archive.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenUtils.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/FaceGen/FaceGenDatas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
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
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
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
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
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
#include "Fallout/AI/HighProcess.h"
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
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStringExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

// The slots of a race's naked body parts that BipedAnim::InitNaked fills.
using NakedPartArray = BIPED_OBJECT[3];
static NakedPartArray naked{BO_UPPER_BODY, BO_LEFT_HAND, BO_RIGHT_HAND};

// The skin mesh name prefixes GetSkinBipedObject matches.
using SkinMeshNameArray = const char* [5];
static SkinMeshNameArray pmeshNames{"UpperBody", "Arms", "LeftHand", "RightHand", "Pipboy"};

// The lock on the biped parts.
static BSCriticalSection sPartsCriticalSection;

// The first-person models of the player's biped slots, set by BipedAnim::Store1stPersonModel.
using Player1stPersonModelArray = TESModel[BIPED_OBJECT_COUNT];
static Player1stPersonModelArray Player1stPersonModel;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> sHandToHandIcon("sHandToHandIcon", "Icons\\Weapons\\HandToHand.dds");
SettingT<INISettingCollection> bLoadHelmetsInBackground("bLoadHelmetsInBackground:BackgroundLoad", true);

int32_t BSstrlwr(char* apString1, int32_t aiSize) {
    return _strlwr_s(apString1, aiSize);
}

// Every member is a pointer, a flag or a float: the whole object starts zeroed, as the game does.
BipedAnim::BipedAnim(TESObjectREFR* apRef, NiNode* apNode) {
    ourMemset(this, 0, sizeof(BipedAnim));
    ourMemset(bufferedObjects, 0, sizeof(bufferedObjects));
    m_pRequester = apRef;
    if (apNode) {
        InitRoot(apNode);
    }
}

BipedAnim::~BipedAnim() {
    RemoveAllParts();
}

void BipedAnim::LoadBipedParts(bool abForceLoadImmediate) {
    BSCloningProcess cloneData;
    float fzoom;
    bool bfirstPerson = false;
    bool bpipboy = object[6].pParent != nullptr;
    if (m_pRequester == pPlayer) {
        bfirstPerson = pPlayer->Is1stPersonBiped(this);
    }
    if (bBipedAnimUpdateSkins) {
        BSFaceGenManager::ClearBodyTexturesFromPalette(static_cast<TESNPC*>(m_pRequester->GetObjectReference()));
    }
    for (BIPED_OBJECT n = BO_HEAD; n < BIPED_OBJECT_COUNT; n = static_cast<BIPED_OBJECT>(n + 1)) {
        if (!object[n].pPart ||
            (object[n].pPart != bufferedObjects[n].pPart ||
             (object[n].pParent != bufferedObjects[n].pParent || !bufferedObjects[n].pPartClone))) {
            if (bufferedObjects[n].pPartClone) {
                RemovePart(&bufferedObjects[n], true, 0);
            }
            else {
                ourMemset(&bufferedObjects[n], 0, sizeof(bufferedObjects[n]));
            }
        LABEL_30:;
            bool bModScopeOn = false;
            uint8_t iModSlot = 0;
            if (n == BO_WEAPON && object[5].pParent) {
                if (m_pRequester == pPlayer) {
                    TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(object[n].pParent);
                    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
                    ItemChange* ichange = CurrentAIProcess->GetCurrentWeapon();
                    iModSlot = ichange->GetModSlots();
                    fzoom = 0.0f;
                    bModScopeOn = ichange->HasModEffectActive(WEAPON_MOD_IRON_SITES, fzoom);
                    AttachBipedWeapon(pweapon, iModSlot);
                    if (pweapon->HasScope() && this == pPlayer->GetBiped(false) &&
                        (!pweapon->HasModScope() || bModScopeOn)) {
                        Interface::InitGunScope(&pweapon->ScopeModel);
                    }
                }
                else {
                    m_pRequester->AttachWeapon(static_cast<TESObjectWEAP*>(object[n].pParent));
                }
            }
            else {
                if ((object[n].pPart) &&
                    (object[n].pPart != reinterpret_cast<TESModel*>(static_cast<uintptr_t>(-1)) &&
                     (!bfirstPerson || (n >= BO_UPPER_BODY && n <= BO_PIP_BOY)))) {
                    if (!abForceLoadImmediate &&
                        (this->IsFaceGenObject(n) &&
                         (m_pRequester != pPlayer &&
                          (bLoadHelmetsInBackground.Bool() &&
                           (BSFaceGenManager::GetUseFaceGenHeads() && BSFaceGenManager::GetUseMultiThreading()))))) {
                        QueueHelmetLoad(IO_TASK_PRIORITY_CRITICAL, nullptr, n);
                    }
                    else {
                        if (object[n].pPart->GetModelLength()) {
                            NiObject* pClone;
                            BIPED_BONE boneIndex = objToBone[n];
                            bool IsFaceGenObject = this->IsFaceGenObject(n);
                            const char* v6 = object[n].pPart->GetModel();
                            NiNode* pNode =
                                pModelLoader->LoadFile(v6, LOD_MULT_ACTORS, true, false, IsFaceGenObject, false);
                            cloneData.~BSCloningProcess();
                            ::new (&cloneData) BSCloningProcess(1.0f);
                            NiPointer<NiObject> spdeepCopy(static_cast<NiObject*>(nullptr));
                            if (HasMorpherController(pNode)) {
                                spdeepCopy.operator=(
                                    static_cast<NiObject*>(TES::pInstance->CreateDeepCopySameTextures(pNode, cloneData))
                                );
                                pClone = spdeepCopy.m_pObject;
                            }
                            else {
                                pClone = pNode->Clone(cloneData);
                            }
                            NiNode* pcloneNode = pClone->IsNode();
                            NiAVObject* ppartClone = nullptr;
                            if (pcloneNode) {
                                TESModelTextureSwap* pmodelswap;
                                if (object[n].pPart) {
                                    pmodelswap = object[n].pPart->GetAsModelTextureSwap();
                                }
                                else {
                                    pmodelswap = nullptr;
                                }
                                TESModelTextureSwap* pmodelswap_2 = pmodelswap;
                                if (pmodelswap) {
                                    pmodelswap_2->SwapTextures(pcloneNode);
                                }
                                if ((object[n].pParent) && (object[n].pParent->GetHasPLSpecTex())) {
                                    SwapPlatformLanguageTextures(pcloneNode);
                                }

                                BSFadeNode* pfadeNode = pcloneNode->IsFadeNode();
                                if (pfadeNode) {
                                    pfadeNode->SetLODMultType(LOD_MULT_ACTOR_BODY_PART);
                                }
                                pcloneNode->SetTranslate(NiPoint3_ZERO);
                                pcloneNode->SetRotate(NiMatrix3_IDENTITY);
                                ppartClone = ApplySkinnedObjects(pcloneNode, n, bfirstPerson, bpipboy, nullptr);
                                if (!ppartClone) {
                                    ppartClone = (NiAVObject*)pcloneNode;
                                    if ((this) && (this->IsFaceGenObject(n))) {
                                        BSFaceGenModel* pfgmodel = LoadFaceGenModel(n);
                                        if (pfgmodel) {
                                            for (int32_t i_2 = 0;; ++i_2) {
                                                uint32_t ArrayCount = pcloneNode->GetArrayCount();
                                                if (i_2 >= ArrayCount) {
                                                    break;
                                                }
                                                NiAVObject* pobj = pcloneNode->GetAt(static_cast<uint32_t>(i_2));
                                                NiTriBasedGeom* ptri = pobj->IsTriBasedGeom();
                                                if ((pobj) && (!ptri)) {
                                                    Warning(
                                                        "FaceGen NIF contains non-geometry at root: "
                                                        "'%s'",
                                                        object[n].pPart->GetModel()
                                                    );
                                                }

                                                if ((ptri) && (ptri->GetModelData()) && (m_pRequester) &&
                                                    (m_pRequester->GetObjectReference()) &&
                                                    (m_pRequester->GetObjectReference()->GetFormType() == NPC__ID)) {
                                                    TESNPC* pnpc_3 =
                                                        static_cast<TESNPC*>(m_pRequester->GetObjectReference());
                                                    FaceGenDatas fgcoord;
                                                    {
                                                        NiPointer<NiObject> spTempObject;
                                                        NiPointer<NiGeometryData> spdata;
                                                        ptri->GetModelData()->CreateDeepCopy(spTempObject);
                                                        spdata.operator=(
                                                            static_cast<NiGeometryData*>(spTempObject.m_pObject)
                                                        );
                                                        NiGeometryData* fScale = spdata.m_pObject;
                                                        ptri->SetModelData(fScale);
                                                        pnpc_3->GetFaceCoord((&fgcoord)->kFaceGens);
                                                        if (!BSFaceGenManager::GetUseFaceGenHeads() ||
                                                            pfgmodel->ApplyCoordinateToExistingMesh(
                                                                &(&fgcoord)->kFaceGens, ptri, false
                                                            )) {
                                                            NiMatrix3 matrot;
                                                            matrot.MakeYRotation(-1.5707964f);
                                                            ptri->SetRotate((&ptri->m_kLocal)->m_Rotate * matrot);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    BipedAnim::AttachToParent(root, pcloneNode, nullptr, this, n, nullptr);
                                    if (!pSaveLoadGameOLD->GetLoading() && n == BO_WEAPON) {
                                        Actor* pactor = static_cast<Actor*>(m_pRequester);
                                        if (pactor->pCurrentProcess) {
                                            if (m_pRequester == pPlayer && this == pPlayer->GetBiped(true)) {
                                                BaseProcess* v23 = pactor->pCurrentProcess;
                                                Actor* fScalea = pactor;
                                                Animation* Animation_0 = pPlayer->GetAnimation(true);
                                                bool IsWeaponDrawn = pactor->IsWeaponDrawn();
                                                v23->ForceWeaponDrawnSheathed(
                                                    IsWeaponDrawn, this, Animation_0, fScalea
                                                );
                                            }
                                            else {
                                                BaseProcess* v22 = pactor->pCurrentProcess;
                                                Actor* fScaleb = pactor;
                                                Animation* Animation_ = pactor->Actor::GetAnimation();
                                                bool v10 = pactor->IsWeaponDrawn();
                                                v22->ForceWeaponDrawnSheathed(v10, this, Animation_, fScaleb);
                                            }
                                        }
                                    }
                                }
                            }
                            if (ppartClone) {
                                BSStringT<char> newName;
                                int32_t fScalec = object[n].pParent->iFormID;
                                const char* v11 = object[n].pParent->GetFormEditorID();
                                newName.SPrintF("%s %s (%08X)", bipedObjectName[n], v11, fScalec);
                                ppartClone->SetName(newName.pString);
                                BipedAnim::AddAddonNodes(pNode, pcloneNode, n, m_pRequester);
                                if ((!object[n].bSkinned) && (ppartClone->IsNode())) {
                                    NiNode* pparent = ppartClone->m_pkParent;
                                    if (boneIndex == BIPED_BONE_NONE) {
                                        const char* fScaled = m_pRequester->GetFullName();
                                        Warning(
                                            "MODELS: The biped part '%s' should be skinned for '%s'.",
                                            object[n].pPart->GetModel(), fScaled
                                        );
                                        if (pparent) {
                                            pparent->DetachChild(ppartClone);
                                        }
                                        else {
                                            NiPointer<NiAVObject> spdeleter(ppartClone);
                                        }
                                        ppartClone = nullptr;
                                    }
                                    else {
                                        if (bone[boneIndex].pParent) {
                                            bone[boneIndex].pParent->AttachChild(ppartClone, 1);
                                        }
                                        else {
                                            if (ppartClone->m_pkParent) {
                                                root->AttachChild(ppartClone, 1);
                                            }
                                        }
                                    }
                                }

                                ExtraDismemberedLimbs* plimbExtra = (&m_pRequester->m_Extra)->GetDismembermentExtra();
                                if (plimbExtra) {
                                    for (BGSBodyPart::LIMB_ENUM elimbIndex = BGSBodyPart::LIMB_TORSO;
                                         elimbIndex < BGSBodyPart::LIMB_COUNT;
                                         elimbIndex = static_cast<BGSBodyPart::LIMB_ENUM>(elimbIndex + 1)) {
                                        if (plimbExtra->Dismembered(elimbIndex)) {
                                            BGSBodyPart::HideLimb(elimbIndex, ppartClone);
                                        }
                                    }
                                }
                                if ((n == BO_BACKPACK) && (object[5].pParent) &&
                                    (static_cast<TESObjectWEAP*>(object[5].pParent)
                                         ->TESObjectWEAP::GetHideBackpack()) &&
                                    (ppartClone)) {
                                    ppartClone->SetAppCulled(true);
                                }

                                object[n].pPartClone = ppartClone;
                            }
                        }
                    }
                }
            }
            continue;
        }
        bool bcreateClone = false;
        BIPOBJECT* v2 = &bufferedObjects[n];
        BIPOBJECT* v3 = &object[n];
        v3->pParent = v2->pParent;
        v3->pPart = v2->pPart;
        v3->pPartClone = v2->pPartClone;
        std::memcpy(&v3->bSkinned, &v2->bSkinned, 4);
        ourMemset(&bufferedObjects[n], 0, sizeof(bufferedObjects[n]));
        if ((bBipedAnimUpdateSkins) && (object[n].pPartClone)) {
            NiNode* pnode = object[n].pPartClone->IsNode();
            if (pnode) {
                for (int32_t i = 0;; ++i) {
                    uint32_t v4 = pnode->GetArrayCount();
                    if (i >= v4) {
                        break;
                    }
                    NiAVObject* v5 = pnode->GetAt(static_cast<uint32_t>(i));
                    AdjustSkinComplexion(v5, false);
                }
            }
            else {
                AdjustSkinComplexion(object[n].pPartClone, false);
            }
        }

        if ((n == BO_BACKPACK) && (object[5].pParent) &&
            (static_cast<TESObjectWEAP*>(object[5].pParent)->TESObjectWEAP::GetHideBackpack()) &&
            (object[n].pPartClone)) {
            object[n].pPartClone->SetAppCulled(true);
        }

        if (bcreateClone) {
            goto LABEL_30;
        }
    }
    NiNode* v12 = m_pRequester->TESObjectREFR::Get3D();
    v12->UpdateProperties();
}

NiNode* BipedAnim::GetRoot() {
    return root;
}

void BipedAnim::InitRoot(NiNode* apNode) {
    this->root = static_cast<NiNode*>(BSUtilities::GetObjectByName(apNode, "Bip01"));
    for (int32_t i = 0; i < BIPED_BONE_COUNT; ++i) {
        NiPointer<NiAVObject> spBone = BSUtilities::GetObjectByName(apNode, bipedBoneName[i]);
        if (spBone.operator->() && spBone->IsNode()) {
            this->bone[i].cFlags |= 1;
            this->bone[i].pParent = static_cast<NiNode*>(spBone.operator->());
        }
        else {
            Warning("MODELS: Missing bone '%s' for '%s'", bipedBoneName[i], apNode->GetName().GetHandleString());
        }
    }
}

void BipedAnim::RemoveAllParts() {
    pModelLoader->EnterModelMapCS();
    sPartsCriticalSection.Enter(nullptr);
    for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
        this->RemovePart(static_cast<BIPED_OBJECT>(i), true, 0);
        this->RemovePart(&this->bufferedObjects[i], true, 0);
    }
    sPartsCriticalSection.Leave();
    pModelLoader->UnlockModelMap();
}

void BipedAnim::RemoveBipedWeapon() {
    if ((object[5].pParent) && (static_cast<TESObjectWEAP*>(object[5].pParent)->TESObjectWEAP::GetHideBackpack()) &&
        (object[7].pPartClone)) {
        object[7].pPartClone->SetAppCulled(false);
    }

    if (bufferedObjects[5].pParent) {
        TESForm* pParent = bufferedObjects[5].pParent;
        for (int32_t i = 0; i < 20; ++i) {
            if (bufferedObjects[i].pParent == pParent) {
                RemovePart(&bufferedObjects[i], true, 0);
            }
        }
    }
    RemovePart(BO_WEAPON, true, 0);
    if (this->m_pRequester->IsActor()) {
        Actor* m_pRequester = static_cast<Actor*>(this->m_pRequester);
        if ((m_pRequester->GetCurrentAIProcess()) &&
            (!pDataHandler->GetClearingData() && this != pPlayer->GetBiped(true))) {
            m_pRequester->ReloadTargets(false);
        }

        NiNode* pBackPack = static_cast<NiNode*>(BSUtilities::GetObjectByName(root, "Backpack"));
        if (pBackPack) {
            NiNode* pBackPackParent = pBackPack->GetParent();
            pBackPackParent->DetachChild(pBackPack);
        }
    }
}

void BipedAnim::AttachBipedWeapon(TESObjectWEAP* apWeapon, uint8_t iModSlots) {
    if ((apWeapon) &&
        (apWeapon->GetFormType() == WEAP_ID && (apWeapon != object[5].pParent || !object[5].pPartClone))) {
        SetBipedWeapon(apWeapon, iModSlots);
        BIPED_OBJECT ebipedObject = BO_WEAPON;
        if (m_pRequester == pPlayer) {
            bool v3 = pPlayer->Is1stPersonBiped(this);
            NiAVObject* v4 = pPlayer->Get3D(v3);
            object[5].pPartClone =
                BipedAnim::LoadAndAttachAddOn(object[5].pParent, object[5].pPart, ebipedObject, m_pRequester, v4);
        }
        else {
            object[5].pPartClone =
                BipedAnim::LoadAndAttachAddOn(object[5].pParent, object[5].pPart, ebipedObject, m_pRequester, nullptr);
        }
        if (object[5].pPartClone) {
            BSStringT<char> newName;
            int32_t CastingType = object[5].pParent->iFormID;
            newName.SPrintF("%s %s (%08X)", bipedObjectName[5], object[5].pParent->GetFormEditorID(), CastingType);
            object[5].pPartClone->SetName(newName.pString);
            if ((apWeapon->GetHideBackpack()) && (object[7].pPartClone)) {
                object[7].pPartClone->SetAppCulled(true);
            }

            if (m_pRequester->IsActor()) {
                Actor* pactor = static_cast<Actor*>(m_pRequester);
                BaseProcess* pprocess = pactor->pCurrentProcess;
                if ((pprocess) &&
                    (!pDataHandler->GetClearingData() && !pSaveLoadGameOLD->GetLoading() &&
                     this != pPlayer->GetBiped(true))) {
                    pactor->ReloadTargets(true);
                }

                if ((pprocess) && (!pSaveLoadGame->GetSaveGameLoading())) {
                    Animation* panim = nullptr;
                    if (m_pRequester == pPlayer && this == pPlayer->GetBiped(true)) {
                        panim = pPlayer->GetAnimation(true);
                    }
                    else {
                        panim = pactor->Actor::GetAnimation();
                    }
                    if (panim) {
                        panim->BlendOut(AGS_WEAPON, false);
                    }
                    if (m_pRequester == pPlayer && this == pPlayer->GetBiped(true)) {
                        BaseProcess* pprocess_2 = pactor->pCurrentProcess;
                        Animation* Animation_0 = pPlayer->GetAnimation(true);
                        bool IsWeaponDrawn = pactor->IsWeaponDrawn();
                        pprocess_2->ForceWeaponDrawnSheathed(IsWeaponDrawn, this, Animation_0, pactor);
                    }
                    else {
                        BaseProcess* pprocess_3 = pactor->pCurrentProcess;
                        Animation* Animation_ = pactor->Actor::GetAnimation();
                        bool v6 = pactor->IsWeaponDrawn();
                        pprocess_3->ForceWeaponDrawnSheathed(v6, this, Animation_, pactor);
                    }
                }
            }
        }
    }
}

void BipedAnim::RemovePart(TESForm* apObj) {
    if (!apObj) {
        return;
    }
    for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
        if (this->object[i].pParent == apObj) {
            this->RemovePart(static_cast<BIPED_OBJECT>(i), true, 0);
        }
    }
}

void BipedAnim::RemovePart(BIPED_OBJECT aeObj, bool abClear, int32_t aiClearValue) {
    this->RemovePart(&this->object[aeObj], abClear, aiClearValue);
}

void BipedAnim::RemovePart(BIPOBJECT* apObj, bool abClear, int32_t aiClearValue) {
    if (apObj->pPartClone) {
        this->RecursivelyRemoveAllDecalPlacementVectorDataFromWeaponBone(apObj->pPartClone);
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueBiped3DDetach(apObj->pPartClone);
        }
        else {
            BipedAnim::RunBiped3DDetach(apObj->pPartClone);
        }
        pModelLoader->ReleaseFile(apObj->pPart->GetModel());
        apObj->pPartClone = nullptr;
    }
    if (abClear) {
        apObj->pParent = nullptr;
        // The model pointer takes the caller's integer as it is.
        apObj->pPart = reinterpret_cast<TESModel*>(aiClearValue);
    }
}

// Drops the decal placement vector extra data of the weapon bone: a geometry that is one of the extra data's skinned
// objects loses it, and a node hands the walk to its children.
void BipedAnim::RecursivelyRemoveAllDecalPlacementVectorDataFromWeaponBone(NiAVObject* apRoot) {
    if (!apRoot) {
        return;
    }
    NiNode* pBone = this->GetParentBone(BIPED_BONE_WEAPON);
    NiExtraData* pExtraData = pBone->GetExtraData(*BSDecalPlacementVectorExtraData::GetTag());
    if (!pExtraData) {
        return;
    }
    if (apRoot->IsTriBasedGeom()) {
        NiAVObject* pGeometry = apRoot;
        if ((static_cast<NiGeometry*>(apRoot)->GetSkinInstance()) &&
            (static_cast<BSDecalPlacementVectorExtraData*>(pExtraData)->skinnedObjects.FindPos(pGeometry))) {
            pBone->RemoveExtraData(*BSDecalPlacementVectorExtraData::GetTag());
        }
    }
    else {
        NiNode* pNode = apRoot->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                this->RecursivelyRemoveAllDecalPlacementVectorDataFromWeaponBone(pNode->GetAt(i));
            }
        }
    }
}

NiNode* BipedAnim::GetParentBone(BIPED_BONE aeType) {
    return this->bone[aeType].pParent;
}

BIPOBJECT* BipedAnim::FindBipObject(TESForm* apObj) {
    if (!apObj) {
        return nullptr;
    }
    for (int32_t i = 0; i < 20; ++i) {
        if (object[i].pParent == apObj) {
            return &object[i];
        }
    }
    return nullptr;
}

// 0x1735D4 is the form ID of a weapon whose mods keep the plain first-person model.
void BipedAnim::SetBipedWeapon(TESObjectWEAP* apWeapon, uint8_t iModSlots) {
    if (!apWeapon || apWeapon->GetFormType() != WEAP_ID) {
        return;
    }
    if (apWeapon == this->object[BO_WEAPON].pParent && this->object[BO_WEAPON].pPartClone) {
        return;
    }
    this->RemovePart(BO_WEAPON, true, 0);
    TESModel* pModel = apWeapon->GetModTESModel(iModSlots, true);
    if (this == pPlayer->GetBiped(true) || this == pPlayer->GetBiped(false)) {
        TESObjectSTAT* pStatic = apWeapon->Get1stPersonObject();
        if (iModSlots && apWeapon->GetFormID() != 0x1735D4) {
            TESObjectSTAT* pModStatic = apWeapon->Get1stPersonModObject(iModSlots);
            if (pModStatic) {
                pStatic = pModStatic;
            }
        }
        if (pStatic) {
            pModel = static_cast<TESModelTextureSwap*>(pStatic);
        }
    }
    this->object[BO_WEAPON].pParent = apWeapon;
    this->object[BO_WEAPON].pPart = pModel;
}

bool BipedAnim::IsFaceGenObject(BIPED_OBJECT aeBipedObject) {
    TESModel* pModel = this->object[aeBipedObject].pPart;
    return pModel && pModel->QHasFacegenData();
}

void BipedAnim::QueueHelmetLoad(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, BIPED_OBJECT aeBipedObject) {
    NiPointer<QueuedHelmet> spQueuedHelmet = static_cast<QueuedHelmet*>(nullptr);
    this->QueueHelmetLoad(aePriority, apParent, spQueuedHelmet, aeBipedObject);
}

// A model of -1 counts as none.
void BipedAnim::QueueHelmetLoad(
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    NiPointer<QueuedHelmet>& aspQueuedHelmet,
    BIPED_OBJECT aeBipedObject
) {
    if (aspQueuedHelmet.operator->()) {
        return;
    }
    if (this->object[aeBipedObject].pPart && this->object[aeBipedObject].pPart != reinterpret_cast<TESModel*>(-1)) {
        pModelLoader->QueueHelmet(this, aspQueuedHelmet, aePriority, apParent, aeBipedObject);
    }
    else {
        aspQueuedHelmet = static_cast<QueuedHelmet*>(nullptr);
    }
}

void BipedAnim::InitNaked(TESRace* apRace, TESActorBase::SEX aeSex) {
    if (!apRace) {
        return;
    }
    bool bBufferEmpty = true;
    for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
        if (this->bufferedObjects[i].pParent) {
            bBufferEmpty = false;
        }
    }
    if (this->m_pRequester && this->m_pRequester->Get3D()) {
        for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
            if (bBufferEmpty) {
                this->bufferedObjects[i] = this->object[i];
            }
            ourMemset(&this->object[i], 0, sizeof(BIPOBJECT));
        }
    }
    else {
        if (bBufferEmpty) {
            ourMemset(this->bufferedObjects, 0, sizeof(this->bufferedObjects));
        }
        ourMemset(this->object, 0, sizeof(this->object));
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (objToBone[naked[i]] == BIPED_BONE_NONE || (this->bone[objToBone[naked[i]]].cFlags & 1)) {
            this->SetBipedPart(
                apRace, static_cast<TESModelTextureSwap*>(apRace->GetPartModel(aeSex, naked[i])), naked[i]
            );
        }
    }
}

// The slot index is compared unsigned.
void BipedAnim::SetBipedPart(TESForm* apParent, TESModel* apModel, BIPED_OBJECT aeObjectSlot) {
    char ac1stPersonFileName[260];
    TESBipedModelForm* pBipedModel = TESBipedModelForm::GetFormAsBipedModel(apParent);
    TESRace* pRace = dynamic_cast<TESRace*>(apParent);
    bool b1stPerson = false;
    if (apModel && pPlayer->Is1stPersonBiped(this)) {
        b1stPerson = this->Get1stPersonModelFileName(apModel, ac1stPersonFileName);
    }
    TESModel* pModel = apModel;
    if (pBipedModel) {
        for (uint32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
            if (pBipedModel->FillsBipedSlot(static_cast<BIPED_OBJECT>(i), false, nullptr) &&
                this->object[i].pParent != apParent) {
                if (this->object[i].pParent && dynamic_cast<TESRace*>(this->object[i].pParent)) {
                    this->RemovePart(static_cast<BIPED_OBJECT>(i), true, 0);
                }
                else {
                    this->RemovePart(this->object[i].pParent);
                }
            }
        }
        for (uint32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
            if (pBipedModel->FillsBipedSlot(static_cast<BIPED_OBJECT>(i), true, nullptr)) {
                if (b1stPerson) {
                    pModel = this->Store1stPersonModel(static_cast<BIPED_OBJECT>(i), ac1stPersonFileName);
                }
                this->object[i].pParent = apParent;
                this->object[i].pPart = pModel;
                return;
            }
        }
    }
    else if (pRace) {
        this->RemovePart(aeObjectSlot, true, 0);
        if (b1stPerson) {
            pModel = this->Store1stPersonModel(aeObjectSlot, ac1stPersonFileName);
        }
        this->object[aeObjectSlot].pParent = apParent;
        this->object[aeObjectSlot].pPart = pModel;
    }
}

// A model of -1 counts as none.
void BipedAnim::QueueBipedParts(
    IO_TASK_PRIORITY aePriority,
    NiPointer<QueuedHelmet>& aspQueuedHelmet,
    QueuedFile* apParent
) {
    NiPointer<QueuedHelmet> spQueuedHelmet = static_cast<QueuedHelmet*>(nullptr);
    int32_t iModelSkinFlags = 0;
    if (dynamic_cast<TESNPC*>(this->m_pRequester->GetObjectReference())) {
        for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
            if (!this->object[i].pPart || this->object[i].pPart == reinterpret_cast<TESModel*>(-1)) {
                continue;
            }
            iModelSkinFlags |= this->object[i].pPart->GetSkinFlags();
            BIPED_OBJECT eObject = static_cast<BIPED_OBJECT>(i);
            if (this->IsFaceGenObject(eObject) && bLoadHelmetsInBackground.Bool() &&
                BSFaceGenManager::GetUseFaceGenHeads() && BSFaceGenManager::GetUseMultiThreading()) {
                this->QueueHelmetLoad(aePriority, apParent, spQueuedHelmet, eObject);
                continue;
            }
            if (this->m_pRequester == pPlayer) {
                TESModel* p1stPersonModel =
                    this->Get1stPersonModel(eObject, static_cast<TESModelTextureSwap*>(this->object[i].pPart));
                if (p1stPersonModel != this->object[i].pPart) {
                    pModelLoader->QueueModel(
                        p1stPersonModel, aePriority, apParent, LOD_MULT_ACTORS, true, false, false
                    );
                }
            }
            pModelLoader->QueueModel(this->object[i].pPart, aePriority, apParent, LOD_MULT_ACTORS, true, false, false);
            TESBipedModelForm* pBipedModel = TESBipedModelForm::GetFormAsBipedModel(this->object[i].pParent);
            if (pBipedModel) {
                const char* pConstraint = pBipedModel->GetBipedConstraint();
                if (*pConstraint) {
                    pModelLoader->QueueModel(pConstraint, aePriority, apParent, LOD_MULT_ACTORS, true, false, false);
                }
            }
        }
    }
    aspQueuedHelmet = spQueuedHelmet;
    this->QueueSkinTextures(iModelSkinFlags, aePriority, apParent);
}

void BipedAnim::QueueSkinTextures(uint32_t aiModelSkinFlags, IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    char acName[260];
    char acFileName[260];
    if (!aiModelSkinFlags) {
        return;
    }
    TESNPC* pNPC = dynamic_cast<TESNPC*>(this->m_pRequester->GetObjectReference());
    if (!pNPC) {
        return;
    }
    if (aiModelSkinFlags & 2) {
        this->QueueSkinTexture(pNPC, BO_UPPER_BODY, aePriority, apParent);
    }
    if (aiModelSkinFlags & 4) {
        this->QueueSkinTexture(pNPC, BO_RIGHT_HAND, aePriority, apParent);
    }
    if (aiModelSkinFlags & 8) {
        this->QueueSkinTexture(pNPC, BO_LEFT_HAND, aePriority, apParent);
    }
    pNPC->GetRace()->GetBodyModTextureName(pNPC, acName);
    if (pNPC->GetRace()->GetBodyModTextureFileName(pNPC, acName, acFileName)) {
        pModelLoader->QueueTexture(acFileName, aePriority, apParent);
    }
}

void BipedAnim::QueueSkinTexture(
    TESNPC* apNpc,
    BIPED_OBJECT aeBipedObject,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent
) {
    char acFileName[260];
    char acNormalFileName[260];
    if (apNpc->GetRace()->GetBodyTextureFileName(apNpc, aeBipedObject, acFileName)) {
        pModelLoader->QueueTexture(acFileName, aePriority, apParent);
        BSShaderManager::GetModifiedTextureFilename(acNormalFileName, acFileName, "_n", true);
        if (acNormalFileName[0]) {
            pModelLoader->QueueTexture(acNormalFileName, aePriority, apParent);
        }
    }
}

// When the result differs from apClone, the holder of apClone releases it.
NiAVObject* BipedAnim::ApplySkinnedObjects(
    NiNode* apClone,
    BIPED_OBJECT aeBipedObject,
    bool abFirstPerson,
    bool abPipboy,
    BSFaceGenModel* apfgModel
) {
    NiAVObject* pResult = apClone;
    NiNode* pFadeNode;
    pFadeNode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&BSFadeNode::ms_RTTI, apClone));
    alignas(NiPointer<NiAVObject>) uint8_t aUnusedStorage[sizeof(NiPointer<NiAVObject>)];
    NiPointer<NiAVObject>* pspUnused = ::new (aUnusedStorage) NiPointer<NiAVObject>(nullptr);
    if (!apClone) {
        pspUnused->~NiPointer<NiAVObject>();
        return pResult;
    }
    this->object[aeBipedObject].bSkinned = Animation::FindSkinnedNode(apClone);
    if (!this->root) {
        pspUnused->~NiPointer<NiAVObject>();
        return pResult;
    }
    if (!this->object[aeBipedObject].bSkinned) {
        pspUnused->~NiPointer<NiAVObject>();
        return nullptr;
    }
    alignas(NiPointer<NiNode>) uint8_t aParentStorage[sizeof(NiPointer<NiNode>)];
    NiPointer<NiNode>& spParent = *::new (aParentStorage) NiPointer<NiNode>(nullptr);
    NiAVObject* pAttachment = BSUtilities::GetObjectByName(this->root, "SkinAttachment");
    if (pAttachment) {
        spParent = static_cast<NiNode*>(pAttachment);
    }
    else {
        NiNode* pRootParent = this->root->GetParent();
        spParent = pRootParent;
    }
    pResult = nullptr;
    if (pFadeNode) {
        // The index compare is unsigned.
        for (uint32_t i = 0; i < pFadeNode->GetArrayCount(); ++i) {
            pResult = this->AttachSkinnedObject(
                static_cast<NiNode*>(pFadeNode->GetAt(i)), nullptr, aeBipedObject, abFirstPerson, abPipboy, apfgModel
            );
        }
    }
    else {
        NiNode* pParent;
        pParent = spParent.operator->();
        pResult = this->AttachSkinnedObject(apClone, pParent, aeBipedObject, abFirstPerson, abPipboy, apfgModel);
    }
    if (pResult != apClone) {
        alignas(NiPointer<NiAVObject>) uint8_t aCloneStorage[sizeof(NiPointer<NiAVObject>)];
        (::new (aCloneStorage) NiPointer<NiAVObject>(apClone))->~NiPointer<NiAVObject>();
    }
    spParent.~NiPointer<NiNode>();
    pspUnused->~NiPointer<NiAVObject>();
    return pResult;
}

// The children that are not geometry, the first-person "UpperBody" parts and the Pip-Boy parts of the other state are
// detached. Skinned geometry gets a copy of its model data (and skin partition) morphed by the requester NPC's FaceGen
// coordinate, its bones are bound to the skeleton, and it moves under apParentNode (or a new node under it when the
// second child is geometry). The decal tag is kept in a pointer to the key string
// (BSDecalPlacementVectorExtraData::GetTag returns it).
NiAVObject* BipedAnim::AttachSkinnedObject(
    NiNode* apNode,
    NiNode* apParentNode,
    BIPED_OBJECT aeBipedObject,
    bool abFirstPerson,
    bool abPipboy,
    BSFaceGenModel* apfgModel
) {
    NiNode* pNode = apNode;
    NiAVObject* pResult = nullptr;
    NiNode* pParent = apParentNode;
    NiPointer<NiObject> spCopy = nullptr;
    pNode->CompactChildArray();
    BSFaceGenModel* pFaceGenModel = apfgModel;
    if (!pFaceGenModel && IsFaceGenObject(aeBipedObject)) {
        pFaceGenModel = LoadFaceGenModel(aeBipedObject);
    }
    apNode->SetTranslate(NiPoint3_ZERO);
    if (apParentNode && pNode->GetArrayCount() >= 2 && pNode->GetAt(1)) {
        NiAVObject* pSecond = pNode->GetAt(1);
        if (pSecond->IsGeometry()) {
            void* pMemory = NiMemObject::operator new(sizeof(NiNode));
            NiNode* pNewNode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
            pParent = pNewNode;
            apParentNode->AttachChild(pParent, true);
            pResult = pParent;
        }
    }
    BSDecalPlacementVectorExtraData* pDecalData =
        static_cast<BSDecalPlacementVectorExtraData*>(apNode->GetExtraData(*BSDecalPlacementVectorExtraData::GetTag()));
    if (pDecalData) {
        NiNode* pWeaponBone = GetParentBone(BIPED_BONE_WEAPON);
        if (pWeaponBone) {
            pWeaponBone->RemoveExtraData(*BSDecalPlacementVectorExtraData::GetTag());
        }
        pWeaponBone->AddExtraData(*BSDecalPlacementVectorExtraData::GetTag(), pDecalData);
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            NiAVObject* pChild = pNode->GetAt(i);
            if (pChild->IsGeometry() && pChild->IsGeometry()->GetSkinInstance()) {
                NiAVObject* pSkinned = pNode->GetAt(i);
                pDecalData->skinnedObjects.AddHead(pSkinned);
            }
        }
    }

    alignas(NiTObjectArray<NiPointer<NiAVObject>>)
        uint8_t acDetachedMemory[sizeof(NiTObjectArray<NiPointer<NiAVObject>>)];
    // The array is kept in untyped storage and used through that storage's address, not the constructor's result.
    NiTObjectArray<NiPointer<NiAVObject>>* pDetached =
        reinterpret_cast<NiTObjectArray<NiPointer<NiAVObject>>*>(acDetachedMemory);
    ::new (acDetachedMemory) NiTObjectArray<NiPointer<NiAVObject>>(0, 1);
    for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = pNode->GetAt(i);
        if (pChild && !pChild->IsGeometry()) {
            {
                NiPointer<NiAVObject> spChild = pChild;
                NiAVObjectArray_Add(pDetached, spChild);
            }
            pNode->DetachChild(pChild);
            continue;
        }
        const char* pChildName;
        if (abFirstPerson) {
            pChildName = pChild->GetName();
        }
        if (abFirstPerson && pChildName) {
            // BSstrlen is evaluated first, then the name, as the game does.
            uint32_t uiUpperLength = BSstrlen(bipedObjectName[BO_UPPER_BODY]);
            const char* pUpperName;
            pUpperName = pChild->GetName();
            if (!_strnicmp(pUpperName, bipedObjectName[BO_UPPER_BODY], uiUpperLength)) {
                {
                    NiPointer<NiAVObject> spChild = pChild;
                    NiAVObjectArray_Add(pDetached, spChild);
                }
                pNode->DetachChild(pChild);
                continue;
            }
        }
        const char* pNameTest;
        pNameTest = pChild->GetName();
        if (pNameTest) {
            alignas(BSStringT<char>) uint8_t acNameMemory[sizeof(BSStringT<char>)];
            const char* pNameString;
            pNameString = pChild->GetName();
            // Uses the stack string's address, not the constructor's result.
            BSStringT<char>* pName = reinterpret_cast<BSStringT<char>*>(acNameMemory);
            ::new (acNameMemory) BSStringT<char>(pNameString);
            pName->MakeLower();
            char acPrefix[10];
            if (abPipboy) {
                BSstrncpy(acPrefix, pName->String(), 9);
                acPrefix[9] = 0;
                if (!BSstricmp(acPrefix, "pipboyoff")) {
                    {
                        NiPointer<NiAVObject> spChild = pChild;
                        NiAVObjectArray_Add(pDetached, spChild);
                    }
                    pNode->DetachChild(pChild);
                    pName->~BSStringT<char>();
                    continue;
                }
            }
            else {
                BSstrncpy(acPrefix, pName->String(), 8);
                acPrefix[8] = 0;
                if (!BSstricmp(acPrefix, "pipboyon")) {
                    {
                        NiPointer<NiAVObject> spChild = pChild;
                        NiAVObjectArray_Add(pDetached, spChild);
                    }
                    pNode->DetachChild(pChild);
                    pName->~BSStringT<char>();
                    continue;
                }
            }
            pName->~BSStringT<char>();
        }

        NiGeometry* pGeometry = static_cast<NiGeometry*>(pChild);
        if (pGeometry && pGeometry->GetModelData() && pGeometry->GetSkinInstance() && pFaceGenModel && m_pRequester &&
            m_pRequester->GetObjectReference() && m_pRequester->GetObjectReference()->GetFormType() == NPC__ID) {
            TESNPC* pNPC = static_cast<TESNPC*>(m_pRequester->GetObjectReference());
            FR2MatrixVTC<float> aFaceCoord[2][2];
            {
                NiPointer<NiGeometryData> spModelData = nullptr;
                pGeometry->GetModelData()->CreateDeepCopy(spCopy);
                spModelData = static_cast<NiGeometryData*>(spCopy.operator->());
                pGeometry->SetModelData(spModelData.operator->());
                if (pGeometry->GetSkinInstance() && pGeometry->GetSkinInstance()->GetSkinPartition()) {
                    pGeometry->GetSkinInstance()->GetSkinPartition()->CreateDeepCopy(spCopy);
                    NiPointer<NiSkinPartition> spPartition = static_cast<NiSkinPartition*>(spCopy.operator->());
                    pGeometry->SetSkinInstance(static_cast<NiSkinInstance*>(pGeometry->GetSkinInstance()->Clone()));
                    pGeometry->GetSkinInstance()->SetSkinPartition(spPartition.operator->());
                }
                pNPC->GetFaceCoord(aFaceCoord);
                if (BSFaceGenManager::GetUseFaceGenHeads()) {
                    pFaceGenModel->ApplyCoordinateToExistingMesh(
                        &aFaceCoord, static_cast<NiTriBasedGeom*>(pGeometry), false
                    );
                }
            }
        }
        AdjustSkinComplexion(pGeometry, false);
        NiSkinInstance* pSkinInstance = pGeometry->GetSkinInstance();
        if (pSkinInstance) {
            if (!pResult) {
                pResult = pChild;
            }
            uint32_t uiBones = pSkinInstance->GetSkinData()->GetBoneCount();
            NiAVObject** ppBones = pSkinInstance->GetBones();
            for (uint32_t uiBone = 0; uiBone < uiBones; ++uiBone) {
                const char* pBoneName;
                pBoneName = ppBones[uiBone]->GetName();
                if (!pBoneName) {
                    const char* pParentName;
                    if (apNode->GetParent()) {
                        NiNode* pNodeParent = apNode->GetParent();
                        pParentName = pNodeParent->GetName();
                    }
                    else {
                        pParentName = "NULL";
                    }
                    const char* pNodeName;
                    pNodeName = apNode->GetName();
                    Warning(
                        "MODELS: Bone %i for part '%s->%s' is unnamed.\r\n"
                        "Make sure all the vertices are skinned to a named bone in Max.",
                        uiBone, pParentName, pNodeName
                    );
                    continue;
                }
                NiAVObject* pBone = BSUtilities::GetObjectByName(root, ppBones[uiBone]->GetName());
                if (pBone) {
                    pSkinInstance->SetBone(uiBone, pBone);
                }
            }
            if (pParent) {
                pSkinInstance->SetRootParent(apParentNode);
                pParent->AttachChild(pChild, true);
            }
        }
    }
    pNode->CompactChildArray();
    NiAVObject* pReturn = pResult;
    NiAVObjectArray_Destruct(pDetached);
    return pReturn;
}

BSFaceGenModel* BipedAnim::LoadFaceGenModel(BIPED_OBJECT aeBipedObject) {
    char acPath[260];
    sPartsCriticalSection.Enter(nullptr);
    TESModel* pModel = this->object[aeBipedObject].pPart;
    BSFaceGenModel* pFaceGenModel = nullptr;
    if (pModel) {
        BSStringT<char> sEGMFile;
        BSstrcpy(acPath, 260, "Meshes\\");
        BSstrcat(acPath, 260, pModel->GetModel());
        pFaceGenModel = BSFaceGenManager::GetFaceGenModel(
            BSFaceGenManager::GetAsEGMFile(sEGMFile, acPath, HAIR_PART_NONE), nullptr, nullptr, nullptr, true,
            HAIR_PART_NONE, false
        );
    }
    sPartsCriticalSection.Leave();
    return pFaceGenModel;
}

void BipedAnim::AdjustSkinComplexion(NiAVObject* apObj, bool abForceReadjust) {
    NiShadeProperty* pShadeProperty = GetLightingProperty(apObj);
    if (!pShadeProperty) {
        return;
    }
    BSShaderPPLightingProperty* pShaderProp = static_cast<BSShaderPPLightingProperty*>(pShadeProperty);
    BIPED_OBJECT ePart = GetSkinBipedObject(apObj, pShaderProp);
    if (ePart == BO_NONE) {
        return;
    }
    FR2MatrixVTC<float> aFaceCoord[2][2];
    TESNPC* pNPC = dynamic_cast<TESNPC*>(m_pRequester->GetObjectReference());
    {
        NiPointer<NiTexture> spBaseTexture = nullptr;
        NiPointer<NiTexture> spModTexture = nullptr;
        if (pNPC) {
            pNPC->GetFaceCoord(aFaceCoord);
        }
        bool bHasTexture = false;
        if (abForceReadjust) {
            bHasTexture = pNPC->GetRace()->GetBodyTexture(spBaseTexture, spModTexture, pNPC, ePart, true);
        }
        else {
            bHasTexture = pNPC->GetRace()->GetBodyTexture(spBaseTexture, spModTexture, pNPC, ePart, false);
        }
        if (pNPC && !bHasTexture) {
            const char* pRaceName = pNPC->GetRace()->GetFullName();
            const char* pPartName = NavMeshInfo::GetCellOrCoords(bipedObjectName[ePart], 1);
            const char* pSexName = NavMeshInfo::GetCellOrCoords(sexName[pNPC->GetSex()], 0);
            Warning("TEXTURES: Missing '%s' race texture for '%s' in race '%s'.", pSexName, pPartName, pRaceName);
        }
        else {
            pShaderProp->SetDiffuseTexture(0, spBaseTexture.operator->());
            NiSourceTexture* pSource = static_cast<NiSourceTexture*>(
                NiObject::DynamicCastStatic(&NiSourceTexture::ms_RTTI, spBaseTexture.operator->())
            );
            const char* pFileName = nullptr;
            bool bLoadFile = true;
            if (pSource) {
                bLoadFile = !pSource->QLookNormal();
                pSource->SetLookNormal();
                pFileName = pSource->GetPlatformSpecificFilename().GetHandleString();
            }
            char acNormalName[260];
            BSShaderManager::GetModifiedTextureFilename(acNormalName, pFileName, "_n", true);
            if (acNormalName[0]) {
                NiPointer<NiTexture> spNormalTexture = nullptr;
                BSShaderManager::GetTexture(acNormalName, bLoadFile, spNormalTexture, true, false);
                if (spNormalTexture.operator->()) {
                    pShaderProp->SetNormalTexture(0, spNormalTexture.operator->());
                }
            }
            pShaderProp->SetDiffuseTexture(1, spModTexture.operator->());
            pShaderProp->SetNormalTexture(1, BSFaceGenManager::GetDefaultDetailModulationTexture());
            pShaderProp->SetFlag(BSShaderProperty::BSSP_FLAG_FACEGENBLEND, true);
        }
    }
}

bool BipedAnim::Get1stPersonModelFileName(TESModel* apModel, char* ap1stPersonFileName) {
    char acPath[260];
    BSstrcpy(ap1stPersonFileName, 260, apModel->GetModel());
    char* pExt = std::strrchr(ap1stPersonFileName, '.');
    if (!pExt) {
        return false;
    }
    *pExt = '\0';
    BSstrcat(pExt, 260 - (pExt - ap1stPersonFileName), "1st.nif");
    BSsprintf(acPath, 260, "Data\\%s\\%s", "Meshes", ap1stPersonFileName);
    return FileFinder::Locate(
               acPath, nullptr, static_cast<FileFinder::LOOKIN_FLAGS>(FileFinder::SKIP_CWD | FileFinder::SKIP_PATHS),
               ARCHIVE_TYPE_MESHES
           ) != 0;
}

// Calls TESModel::SetModel's body directly.
TESModel* BipedAnim::Store1stPersonModel(BIPED_OBJECT aeBipedObject, const char* ap1stPersonFileName) {
    Player1stPersonModel[aeBipedObject].TESModel::SetModel(ap1stPersonFileName);
    return &Player1stPersonModel[aeBipedObject];
}

TESModel* BipedAnim::Get1stPersonModel(BIPED_OBJECT aeBipedObject, TESModel* apModel) {
    char ac1stPersonFileName[260];
    if (apModel == &Player1stPersonModel[aeBipedObject]) {
        return apModel;
    }
    if (this->Get1stPersonModelFileName(apModel, ac1stPersonFileName)) {
        return this->Store1stPersonModel(aeBipedObject, ac1stPersonFileName);
    }
    return apModel;
}

NiNode* BipedAnim::CloneHelmet(BSFaceGenModel* apfgModel, NiNode* apNode, BIPED_OBJECT aeBipedObject) {
    BSCloningProcess cloneData;
    NiObject* pClone;
    if (object[aeBipedObject].pPartClone) {
        return nullptr;
    }
    if (!apfgModel || !apNode) {
        return nullptr;
    }
    BIPED_BONE boneIndex = objToBone[aeBipedObject];
    cloneData.~BSCloningProcess();
    ::new (&cloneData) BSCloningProcess(1.0f);
    NiPointer<NiObject> spdeepCopy(nullptr);
    if (HasMorpherController(apNode)) {
        spdeepCopy.operator=(static_cast<NiObject*>(TES::pInstance->CreateDeepCopySameTextures(apNode, cloneData)));
        pClone = spdeepCopy.m_pObject;
    }
    else {
        pClone = apNode->Clone(cloneData);
    }
    BSFadeNode* pfadeNode = static_cast<BSFadeNode*>(pClone);
    int32_t ppartClone = 0;
    if (!pClone) {
        return nullptr;
    }
    if (NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, pfadeNode)) {
        pfadeNode->SetLODMultType(LOD_MULT_ACTOR_BODY_PART);
    }
    pfadeNode->SetTranslate(NiPoint3_ZERO);
    pfadeNode->SetRotate(NiMatrix3_IDENTITY);
    if (object[aeBipedObject].pPart) {
        TESModelTextureSwap* pswap = object[aeBipedObject].pPart->GetAsModelTextureSwap();
        if (pswap) {
            pswap->SwapTextures(pfadeNode);
        }
    }
    if ((object[aeBipedObject].pParent) && (object[aeBipedObject].pParent->GetHasPLSpecTex())) {
        SwapPlatformLanguageTextures(pfadeNode);
    }

    NiNode* pcloneNode = pfadeNode;
    return pcloneNode;
}

void BipedAnim::AttachHelmet(BSFaceGenModel* apfgModel, NiNode* apNode, BIPED_OBJECT aeBipedObject) {
    NiNode* arg4 = nullptr;
    if ((apfgModel) && (apNode)) {
        BIPED_BONE boneIndex = objToBone[aeBipedObject];
        bool b1st = false;
        if (m_pRequester == pPlayer) {
            b1st = pPlayer->Is1stPersonBiped(this);
        }
        bool bfirstPerson = false;
        bool bpipboy = object[6].pParent != nullptr;
        if (m_pRequester == pPlayer) {
            bfirstPerson = pPlayer->Is1stPersonBiped(this);
        }
        NiAVObject* ppartClone = ApplySkinnedObjects(apNode, aeBipedObject, bfirstPerson, bpipboy, apfgModel);
        if (!ppartClone) {
            ppartClone = apNode;
            BipedAnim::AttachToParent(root, apNode, nullptr, this, aeBipedObject, apfgModel);
        }
        BSStringT<char> newName;
        int32_t CastingType = object[aeBipedObject].pParent->iFormID;
        newName.SPrintF(
            "%s %s (%08X)", bipedObjectName[aeBipedObject], object[aeBipedObject].pParent->GetFormEditorID(),
            CastingType
        );
        ppartClone->SetName(newName.pString);
        BipedAnim::AddAddonNodes(apNode, apNode, aeBipedObject, m_pRequester);
        if ((!object[aeBipedObject].bSkinned) && (ppartClone->IsNode())) {
            NiNode* pparent = ppartClone->m_pkParent;
            if (bone[boneIndex].pParent) {
                bone[boneIndex].pParent->AttachChild(ppartClone, 1);
            }
            else {
                if (ppartClone->m_pkParent) {
                    root->AttachChild(ppartClone, 1);
                }
            }
        }

        object[aeBipedObject].pPartClone = ppartClone;
    }

    if ((m_pRequester) && (m_pRequester->TESObjectREFR::Get3D())) {
        NiNode* v4 = m_pRequester->TESObjectREFR::Get3D();
        v4->UpdateProperties();
    }
}

void BipedAnim::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NiAVObject* BipedAnim::LoadAndAttachAddOn(
    TESForm* apObject,
    TESModel* apModel,
    BIPED_OBJECT aeAttachmentType,
    TESObjectREFR* apRequester,
    NiAVObject* apObj3D
) {
    BSCloningProcess cloneData;
    NiObject* pClone;
    if (!apModel || !apRequester) {
        return nullptr;
    }
    const char* pmodelfilename = apModel->GetModel();
    if (!pmodelfilename || !*pmodelfilename) {
        return nullptr;
    }
    NiAVObject* pobj3D = apObj3D;
    if (!apObj3D) {
        pobj3D = apRequester->TESObjectREFR::Get3D();
    }
    if (!pobj3D) {
        return nullptr;
    }
    NiNode* pobjNode = static_cast<NiNode*>(pobj3D);
    NiNode* pparentNode = pModelLoader->LoadFile(pmodelfilename, LOD_MULT_ACTORS, true, false, false, false);
    cloneData.~BSCloningProcess();
    ::new (&cloneData) BSCloningProcess(1.0f);
    NiPointer<NiObject> spdeepCopy(nullptr);
    if (HasMorpherController(pparentNode)) {
        spdeepCopy.operator=(
            static_cast<NiObject*>(TES::pInstance->CreateDeepCopySameTextures(pparentNode, cloneData))
        );
        pClone = spdeepCopy.m_pObject;
    }
    else {
        pClone = pparentNode->Clone(cloneData);
    }
    BSFadeNode* pfadeNode = static_cast<BSFadeNode*>(pClone);
    bool bskinned = false;
    TESModelTextureSwap* pmodelswap = apModel->GetAsModelTextureSwap();
    if (pmodelswap) {
        pmodelswap->SwapTextures(pfadeNode);
    }
    if ((apObject) && (apObject->GetHasPLSpecTex())) {
        SwapPlatformLanguageTextures(pfadeNode);
    }

    BipedAnim::AddAddonNodes(pparentNode, pfadeNode, aeAttachmentType, apRequester);
    TESObjectREFR::AddMasterParticleAddonNodes(pfadeNode);
    const NiFixedString* a2 = BSDecalPlacementVectorExtraData::GetTag();
    NiPointer<NiExtraData> pextra(pfadeNode->GetExtraData(*a2));
    pfadeNode->RemoveAllExtraData();
    if (pextra.m_pObject) {
        NiExtraData* a2a = pextra.m_pObject;
        const NiFixedString* TagKey = BSDecalPlacementVectorExtraData::GetTag();
        pfadeNode->AddExtraData(*TagKey, a2a);
        pextra.operator=(static_cast<NiExtraData*>(nullptr));
    }
    if (pfadeNode) {
        if (NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, pfadeNode)) {
            pfadeNode->SetLODMultType(LOD_MULT_ACTOR_BODY_PART);
        }
        pfadeNode->SetTranslate(NiPoint3_ZERO);
        pfadeNode->SetRotate(NiMatrix3_IDENTITY);
        if (Animation::FindSkinnedNode(pfadeNode)) {
            bskinned = true;
        }
        if (bskinned) {
            Warning(
                "MODELS: Tyring to add skinned object '%s' as an add on to skeleton.",
                (&pfadeNode->m_kName)->GetHandleString()
            );
        }
        else {
            BipedAnim::AttachToParent(pobjNode, pfadeNode, pparentNode, nullptr, aeAttachmentType, nullptr);
        }
        if ((!bskinned) && (pfadeNode->IsNode())) {
            NiNode* pdefaultParent = nullptr;
            if (!pfadeNode->m_pkParent) {
                if (aeAttachmentType != BO_NONE && objToBone[aeAttachmentType] != BIPED_BONE_NONE) {
                    pdefaultParent = static_cast<NiNode*>(BSUtilities::GetObjectByName(
                        static_cast<NiAVObject*>(pobjNode), bipedBoneName[objToBone[aeAttachmentType]]
                    ));
                }
                if (pdefaultParent) {
                    pdefaultParent->AttachChild(pfadeNode, 1);
                }
                else {
                    pobjNode->AttachChild(pfadeNode, 1);
                }
            }
        }

        pfadeNode->UpdateProperties();
    }
    NiNode* pcloneNode = pfadeNode;
    return pcloneNode;
}

void BipedAnim::RunBiped3DDetach(NiAVObject* ap3D) {
    ShadowSceneNode* pSSN;
    NiNode* pParent;
    if (ap3D) {
        RecursivelyRemoveAllDecals(ap3D);
        pSSN = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        pSSN->RemoveObject(ap3D);
        if (ap3D->NiAVObject::GetParent()) {
            bhkWorld::RemoveObjects(static_cast<NiNode*>(ap3D), true, false);
            pParent = ap3D->NiAVObject::GetParent();
            pParent->DetachChild(ap3D);
        }
    }
}

void BipedAnim::AttachToSkeleton(NiNode* apSkeleton, NiAVObject* apSkin, NiNode* apParentNode, bool abShowWarnings) {
    if (apSkin) {
        NiNode* pnode = apSkin->IsNode();
        if (apSkin->IsGeometry()) {
            NiGeometry* ptri = static_cast<NiGeometry*>(apSkin);
            NiSkinInstance* pskin = ptri->GetSkinInstance();
            if (pskin) {
                uint32_t ibonecount = pskin->GetSkinData()->m_uiBones;
                NiAVObject** pbones = pskin->m_ppkBones;
                for (uint32_t m = 0; m < ibonecount; ++m) {
                    if (pbones[m]) {
                        NiAVObject* pnewBone = BSUtilities::GetObjectByName(apSkeleton, *&pbones[m]->m_kName);
                        if (pnewBone) {
                            pskin->SetBone(m, pnewBone);
                        }
                        else {
                            pskin->SetBone(m, apSkeleton);
                            if (abShowWarnings) {
                                const char* v4;
                                const char* v5;
                                const char* v7;
                                uint32_t v11;
                                if (pnode &&
                                    (v4 = (&pbones[m]->m_kName)->GetHandleString(),
                                     (v11 = BSstrlen(v4),
                                      (v7 = (&pbones[m]->m_kName)->GetHandleString(),
                                       (v5 = (&apSkin->m_kName)->GetHandleString(), !_strnicmp(v5, v7, v11)))))) {
                                    const char* HandleString = (&pnode->m_kName)->GetHandleString();
                                    const char* v8 = (&pnode->m_pkParent->m_kName)->GetHandleString();
                                    Warning(
                                        "MODELS: Bone '%s' not found for part '%s->%s'.\r\nMake sure all the "
                                        "verticies are skinned to a bone in Max.",
                                        (&pbones[m]->m_kName)->GetHandleString(), v8, HandleString
                                    );
                                }
                                else {
                                    if (pnode && apSkeleton) {
                                        const char* v13 = (&apSkeleton->m_kName)->GetHandleString();
                                        const char* v9 = (&pnode->m_kName)->GetHandleString();
                                        Warning(
                                            "MODELS: Bone '%s' not found for part '%s'.\r\nRequested by model "
                                            "'%s'.",
                                            (&pbones[m]->m_kName)->GetHandleString(), v9, v13
                                        );
                                    }
                                    else {
                                        Warning(
                                            "MODELS: Bone '%s' not found for part 'UNKNOWN'.\r\nRequested by model "
                                            "'UNKNOWN'.",
                                            (&pbones[m]->m_kName)->GetHandleString()
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
                if (apParentNode) {
                    pskin->m_pkRootParent = apParentNode;
                    apParentNode->AttachChild(apSkin, 1);
                }
            }
        }
        char v27 = 0;
        if (pnode) {
            pnode->CompactChildArray();
            NiTObjectArray<NiPointer<NiAVObject>> TempBoneArray(0, 1u);
            for (int32_t i = 0;; ++i) {
                uint32_t ArrayCount = pnode->GetArrayCount();
                if (i >= ArrayCount) {
                    break;
                }
                NiAVObject* pobj = pnode->GetAt(static_cast<uint32_t>(i));
                NiNode* ptemp = nullptr;
                if (pobj) {
                    ptemp = pobj->IsNode();
                    if (NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, pnode) && ptemp) {
                        BipedAnim::AttachToSkeleton(apSkeleton, pobj, apParentNode, true);
                    }
                    else {
                        if (pobj->IsGeometry()) {
                            BipedAnim::AttachToSkeleton(apSkeleton, pobj, apParentNode, abShowWarnings);
                        }
                        else {
                            if ((!v27) && (ptemp) && (static_cast<NiNode*>(pobj)->NiNode::GetChildCount())) {
                                const char* v14 = (&apSkeleton->m_kName)->GetHandleString();
                                const char* v10 = (&pnode->m_kName)->GetHandleString();
                                Warning(
                                    "Body part '%s'->'%s' for skeleton '%s' was exported "
                                    "incorectly.\r\nHide the skeleton before you export body parts.",
                                    (&pnode->m_pkParent->m_kName)->GetHandleString(), v10, v14
                                );
                                v27 = 1;
                            }

                            NiAVObjectArray_Add(&TempBoneArray, NiPointer<NiAVObject>(pobj));
                            pnode->DetachChild(pobj);
                        }
                    }
                }
            }
            pnode->CompactChildArray();
        }
    }
}

void BipedAnim::AttachToParent(
    NiNode* apSkeleton,
    NiNode* apNode,
    NiNode* apClonedFromNode,
    BipedAnim* apBipedAnim,
    BIPED_OBJECT aeBipedObject,
    BSFaceGenModel* apfgModel
) {
    CFilter a2;
    NiExtraData* pextraData = nullptr;
    NiStringExtraData* pparentName = nullptr;
    NiNode* pparentNode = nullptr;
    if (apClonedFromNode) {
        NiFixedString* v6 = &FixedStrings::QPrn();
        pextraData = apClonedFromNode->GetExtraData(*v6);
    }
    if (!pextraData) {
        NiFixedString* v7 = &FixedStrings::QPrn();
        pextraData = apNode->GetExtraData(*v7);
    }
    if (pextraData) {
        pparentName =
            static_cast<NiStringExtraData*>(NiObject::DynamicCastStatic(&NiStringExtraData::ms_RTTI, pextraData));
        if (pparentName) {
            NiFixedString* v8 = &pparentName->m_kString;
            NiAVObject* pparentObj = BSUtilities::GetObjectByName(apSkeleton, *v8);
            pparentNode = static_cast<NiNode*>(pparentObj);
            if (!pparentObj) {
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(apSkeleton);
                if (pref && (pref->GetObjectReference() && pref->GetObjectReference()->GetFormType() == CREA_ID)) {
                    RemoveScabard(apNode);
                }
                else {
                    const char* X = (&apNode->m_kName)->GetHandleString();
                    NiFixedString* v9 = &pparentName->m_kString;
                    Warning("MODELS: Could not find parent node '%s' for object '%s'.", v9->GetHandleString(), X);
                }
            }
        }
        else {
            Warning(
                "MODELS: Extra data 'Prn' on '%s' is not an NiStringExtraData.", (&apNode->m_kName)->GetHandleString()
            );
        }
    }
    else {
        Warning("MODELS: Could not find parent node extra data for '%s'.", (&apNode->m_kName)->GetHandleString());
    }
    float fbackpackScale = 1.0f;
    if (pparentNode) {
        fbackpackScale = pparentNode->GetLocalScale();
        pparentNode->AttachChild(apNode, 1);
        float afTime = static_cast<float>(appTimer.iLastTime);
        NiUpdateData updatedate(afTime, false, false);
        apNode->UpdateDownwardPass(updatedate, 0);
        ShadowSceneNode* pshadowNode = BSShaderManager::GetShadowSceneNode(0);
        pshadowNode->AddObject(apNode);
        NiAVObject* pscabardObj = BSUtilities::GetObjectByName(apNode, "Scb");
        if (pscabardObj) {
            pparentNode->AttachChild(pscabardObj, 1);
            bhkNiCollisionObject::EnableCollision(pparentNode, true);
            pscabardObj->UpdateProperties();
            apNode->CompactChildArray();
        }
    }
    NiAVObject* pbackpackObj = BSUtilities::GetObjectByName(apNode, "Backpack");
    if (pbackpackObj) {
        if (apSkeleton == pPlayer->Get3D(true)) {
            pbackpackObj->SetAppCulled(true);
        }
        else {
            pbackpackObj->SetScale(fbackpackScale);
            pextraData = pbackpackObj->GetExtraData("UPB");
            if (pextraData) {
                pparentName = static_cast<NiStringExtraData*>(
                    NiObject::DynamicCastStatic(&NiStringExtraData::ms_RTTI, pextraData)
                );
                if (pparentName) {
                    NiFixedString* Xa = &pparentName->m_kString;
                    NiAVObject* pparentObj_2 = BSUtilities::GetObjectByName(apSkeleton, *Xa);
                    pparentNode = static_cast<NiNode*>(pparentObj_2);
                    if (pparentObj_2) {
                        pparentNode->AttachChild(pbackpackObj, 1);
                    }
                }
            }
        }
    }
    if (apBipedAnim && apBipedAnim == pPlayer->GetBiped(true)) {
        bhkWorld::KillHavok(apNode, true, true);
    }
    else {
        if (aeBipedObject == BO_WEAPON) {
            bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(apNode);
            if (pcol) {
                bhkShape* pshape;
                bhkRigidBody* prb = pcol->GetRigidBody();
                if (prb) {
                    pshape = prb->GetShape();
                }
                else {
                    pshape = nullptr;
                }
                if (!pshape) {
                    Warning(
                        "HAVOK: Weapon '%s' doesn't have a good Havok shape on it.",
                        (&apNode->m_kName)->GetHandleString()
                    );
                }
                if (prb) {
                    CFilter* CollisionFilter = prb->GetCollisionFilter(&a2);
                    if (CollisionFilter->GetLayer() != L_WEAPON) {
                        Warning(
                            "MODELS: Weapon '%s' needs to have the layer defined as a WEAPON.",
                            (&apNode->m_kName)->GetHandleString()
                        );
                    }
                }
            }
            else {
                Warning("HAVOK: Weapon '%s' doesn't have any Havok data on it.", (&apNode->m_kName)->GetHandleString());
            }
        }
        bhkWorld::SetMotion(apNode, hkpMotion::MOTION_KEYFRAMED, true, true, true);
    }
}

// Names 0, 1 and 4 are the upper body, 2 the left hand and 3 the right hand; an unknown name (or none) warns and
// keeps the upper body.
BIPED_OBJECT BipedAnim::GetSkinBipedObject(NiAVObject* apObj, BSShaderPPLightingProperty* apLightingProperty) {
    if (!apLightingProperty) {
        apLightingProperty = static_cast<BSShaderPPLightingProperty*>(GetLightingProperty(apObj));
    }
    if (!apLightingProperty || !apLightingProperty->GetFlag(BSShaderProperty::BSSP_FLAG_FACEGENBLEND)) {
        return BO_NONE;
    }
    const char* pmodelName = apObj->GetName().GetHandleString();
    BIPED_OBJECT ebipedObject = BO_UPPER_BODY;
    int32_t n;
    for (n = 0; n < 5; ++n) {
        if (pmodelName && !_strnicmp(pmodelName, pmeshNames[n], BSstrlen(pmeshNames[n]))) {
            break;
        }
    }
    switch (n) {
        case 0:
        case 1:
        case 4: ebipedObject = BO_UPPER_BODY; break;
        case 2: ebipedObject = BO_LEFT_HAND; break;
        case 3: ebipedObject = BO_RIGHT_HAND; break;
        default:
            Warning("MODELS: Bad skin name '%s' on '%s'.", pmodelName, apObj->GetParent()->GetName().GetHandleString());
            break;
    }
    return ebipedObject;
}

void BipedAnim::AddAddonNodes(
    NiNode* apClonedFrom,
    NiNode* apNode,
    BIPED_OBJECT aeAttachmentType,
    TESObjectREFR* apRequester
) {
    BSXFlags* pFlags = static_cast<BSXFlags*>(apClonedFrom->GetExtraData(BSXFlags::Tag()));
    if (pFlags && pFlags->QAddon()) {
        TESObjectREFR::AddAddonNodes(apNode);
    }
}

// The material and texturing property lookups are unused, but are made deliberately, as the game does.
BSShaderPPLightingProperty* BipedAnim::GetLightingProperty(NiAVObject* apObj) {
    if (!apObj || !apObj->IsGeometry()) {
        return nullptr;
    }
    apObj->GetProperty(NiMaterialProperty::GetType());
    apObj->GetProperty(NiTexturingProperty::GetType());
    NiShadeProperty* pProperty = static_cast<NiShadeProperty*>(apObj->GetProperty(NiProperty::SHADE));
    bool bLighting = pProperty && pProperty->GetShaderPropertyType() >= 8 && pProperty->GetShaderPropertyType() <= 12;
    return bLighting ? static_cast<BSShaderPPLightingProperty*>(pProperty) : nullptr;
}

int32_t BSstrncpy(char* apString1, const char* apString2, int32_t aiLen) {
    return strncpy_s(apString1, aiLen + 1, apString2, aiLen);
}

hkpWorldObject* bhkWorldObject::operator*() {
    if (this) {
        return Gethk();
    }
    return nullptr;
}

BSCloningProcess::BSCloningProcess(float afScale) : NiCloningProcess(0x101u) {
    SetScale(afScale);
}

BSCloningProcess::~BSCloningProcess() {}

void BSCloningProcess::SetScale(float afScale) {
    Scale.z = afScale;
    Scale.y = afScale;
    Scale.x = afScale;
}

bool BSFaceGenManager::GetUseMultiThreading() {
    return BSFaceGenManager::bUseMultiThreadedFaceGen.Bool() && BSFaceGenManager::bUseFaceGenHeads.Bool();
}

// Without a manager, builds a null temporary smart pointer, reads it through operator-> and releases it under a
// construction flag, as the game does.
NiTexture* BSFaceGenManager::GetDefaultDetailModulationTexture() {
    alignas(NiPointer<NiSourceTexture>) uint8_t aNullStorage[sizeof(NiPointer<NiSourceTexture>)];
    bool bConstructed = false;
    NiPointer<NiSourceTexture>* pspTexture;
    if (pInstance) {
        pspTexture = &pInstance->spDefaultDetailMod;
    }
    else {
        pspTexture = ::new (aNullStorage) NiPointer<NiSourceTexture>(nullptr);
        bConstructed = true;
    }
    NiSourceTexture* pTexture = pspTexture->operator->();
    if (bConstructed) {
        // Raw storage that was constructed in place above.
        reinterpret_cast<NiPointer<NiSourceTexture>*>(aNullStorage)->~NiPointer<NiSourceTexture>();
    }
    return pTexture;
}

bool TESObjectWEAP::GetHideBackpack() {
    return (data.cFlags & TESObjectWEAP::WEAPON_FLAG_HIDE_BACKPACK) != 0;
}

float NiAVObject::GetScale() {
    return m_kLocal.m_fScale;
}

void NiAVObjectArray_Destruct(NiTObjectArray<NiPointer<NiAVObject>>* apArray) {
    apArray->~NiTObjectArray();
}

uint32_t NiAVObjectArray_Add(NiTObjectArray<NiPointer<NiAVObject>>* apArray, const NiPointer<NiAVObject>& arspElement) {
    return apArray->SetAtGrow(apArray->m_usSize, arspElement);
}

char NiObjectNET::GetDefaultAppendCharacter() {
    return ms_cAppendCharacter;
}

uint32_t NiSkinData::GetBoneCount() const {
    return m_uiBones;
}

NiSkinPartition* NiSkinInstance::GetSkinPartition() const {
    return m_spSkinPartition;
}

NiSkinData* NiSkinInstance::GetSkinData() const {
    return m_spSkinData;
}

NiAVObject** NiSkinInstance::GetBones() const {
    return m_ppkBones;
}

void NiSkinInstance::SetBone(uint32_t i, NiAVObject* pkBone) {
    m_ppkBones[i] = pkBone;
}

void NiSkinInstance::SetRootParent(NiAVObject* pkRootParent) {
    m_pkRootParent = pkRootParent;
}

void NiSkinInstance::SetSkinPartition(NiSkinPartition* pkSkinPartition) {
    m_spSkinPartition = pkSkinPartition;
}

void NiSourceTexture::SetLookNormal() {}

// Always false: cLooked is read and masked with zero.
bool NiSourceTexture::QLookNormal() {
    return false;
}
