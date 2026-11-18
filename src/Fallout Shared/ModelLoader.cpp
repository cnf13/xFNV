#include "Fallout Shared/ModelLoader.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCoreMessage.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPrecisionTimer.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSCore/Xenon/BSThread_Xenon.h"
#include "BSHavok/HavokThreadMemoryStruct.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSValueNode.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "BSShader/SpeedTreeBillboardShaderProperty.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSHash.h"
#include "BSSystem/FileFinder.h"
#include "BSSystem/WildCard.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/QueuedReplacementKF.h"
#include "Fallout Shared/QueuedReplacementKFList.h"
#include "Fallout Shared/QueuedTreeBillboard.h"
#include "Fallout Shared/QueuedTreeModel.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESAnimation.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelList.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESModelTree.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectANIO.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
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
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
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
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/GunWobble.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
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
#include "Fallout/Misc/MemoryLevelManager.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDegradePriority.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSEntryMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <SDL3/SDL.h>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <memory>
#include <vector>

template <uint32_t Flag>
void MLSetFlag(bool abValue, uint8_t& cBits);

// Whether the window title shows the loading counts (PC-only).
static bool bLoadingTextDisplayed;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fLODBoundRadiusMult("fLODBoundRadiusMult:LOD", 3.0f);
SettingT<INISettingCollection> fLODFadeOutPercent("fLODFadeOutPercent:LOD", 0.9f);
BSCriticalSection LoadedFile::LSAccessCrit;
static BSCriticalSection LSAccessCrit;

// Sets (abValue) or clears the bits of Flag in cBits.
template <uint32_t Flag>
void MLSetFlag(bool abValue, uint8_t& cBits) {
    if (abValue) {
        cBits |= Flag;
    }
    else {
        cBits &= ~Flag;
    }
}

using QueuedReferenceMap = LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>;
using QueuedReferenceMarkedPointer = QueuedReferenceMap::MarkedPointer<QueuedReferenceMap::HashListNode>;

using KFModelMapIterator = LockFreeStringMap<KFModel*>::LockFreeStringMapIterator;

using QueuedReferenceMap = LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>;

using QueuedHelmetMap = LockFreeMap<TESObjectREFR*, NiPointer<QueuedHelmet>>;

using IOTaskQueueInterface = LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface;

using ModelMap = LockFreeMap<const char*, Model*>;
using ModelMapHashListNode = ModelMap::HashListNode;

Model::Model(char* apFilename, NiNode* apObject3D) : iRefCount(0), iManualRefCount(0), spObject3D(apObject3D) {
    int32_t iFilenameLen = BSstrlen(apFilename) + 1;
    pFilename = static_cast<char*>(ourMalloc(iFilenameLen));
    BSstrcpy(pFilename, iFilenameLen, apFilename);
}

// InitModel sets the file name, the counts and the 3D from the stream.
Model::Model(const char* apFilename, BSStream* apStream, bool abPrepareObject, bool abFaceGenModel)
  : spObject3D(nullptr) {
    InitModel(apFilename, apStream, abPrepareObject, abFaceGenModel);
}

Model::~Model() {
    spObject3D = nullptr;
    ourFree(pFilename);
}

NiNode* Model::GetModel() {
    return spObject3D.m_pObject;
}

void Model::IncManualRefCount() {
    NiAtomicIncrement(iManualRefCount);
}

void Model::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

void Model::DecRefCount() {
    NiAtomicDecrement(iRefCount);
}

// Returns the sum of both reference counts.
int32_t Model::GetRefCount() const {
    return iRefCount + iManualRefCount;
}

// Decrements the manual count only when its signed value is positive.
void Model::DecManualRefCount() {
    if (iManualRefCount > 0) {
        NiAtomicDecrement(iManualRefCount);
    }
}

void Model::ModManualRefCount(int32_t aiVal) {
    for (int32_t uiUseCount = iManualRefCount;
         BSAtomicCompareExchange(&iManualRefCount, uiUseCount, aiVal + uiUseCount) != uiUseCount;
         uiUseCount = iManualRefCount) {
        BSSleep(0);
    }
}

void Model::InitModel(const char* apFilename, BSStream* apStream, bool abPrepareObject, bool abFaceGenModel) {
    iRefCount = 0;
    iManualRefCount = 0;
    int32_t iFilenameLen = BSstrlen(apFilename) + 1;
    pFilename = static_cast<char*>(ourMalloc(static_cast<uint32_t>(iFilenameLen)));
    BSstrcpy(pFilename, iFilenameLen, apFilename);
    if (apStream->GetObjectCount() ||
        (apStream->FreeAllObjects(), apStream->Load("Meshes\\Marker_Error.NIF", nullptr))) {
        NiNode* pNewObject = static_cast<NiNode*>(apStream->GetObjectAt(0));
        bool bLoaded = false;
        if (pNewObject && pNewObject->IsNode()) {
            const char* pname = (&pNewObject->m_kName)->GetHandleString();
            if (pname && WildCard::Match(pname, "Bip[0-9][0-9]")) {
                NiObject* pNewObject_2;
                NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
                if (apMemory) {
                    pNewObject_2 = (NiObject*)::new (apMemory) NiNode(0);
                }
                else {
                    pNewObject_2 = nullptr;
                }
                spObject3D.operator=(static_cast<NiNode*>(pNewObject_2));
                NiNode* v11 = spObject3D.m_pObject;
                v11->SetAt(0, pNewObject);
            }
            else {
                spObject3D.operator=(pNewObject);
            }
            bLoaded = true;
        }
        else {
            uint32_t iChild = 0;
            for (uint32_t iIndex = 0; iIndex < apStream->GetObjectCount(); ++iIndex) {
                NiAVObject* pChild = static_cast<NiAVObject*>(apStream->GetObjectAt(iIndex));
                if ((pChild) && (pChild->IsNode() || pChild->IsGeometry())) {
                    if (!spObject3D.m_pObject) {
                        NiObject* pNewObject_3;
                        NiNode* v14 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
                        if (v14) {
                            pNewObject_3 = (NiObject*)::new (v14) NiNode(0);
                        }
                        else {
                            pNewObject_3 = nullptr;
                        }
                        spObject3D.operator=(static_cast<NiNode*>(pNewObject_3));
                    }
                    NiNode* v9 = spObject3D.m_pObject;
                    v9->SetAt(iChild++, pChild);
                    bLoaded = true;
                }
            }
        }
        if (bLoaded) {
            RemoveEditorMarkers(spObject3D.m_pObject);
            if (bRenameModel) {
                NiControllerManager* pman = static_cast<NiControllerManager*>(
                    NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, spObject3D.m_pObject->GetControllers())
                );
                if (pman) {
                    NiAVObjectPalette* ppal = pman->GetObjectPalette();
                    if ((ppal) && ((&spObject3D.m_pObject->m_kName)->GetHandleString())) {
                        ppal->SetAVObject(*&spObject3D.m_pObject->m_kName, nullptr);
                    }
                }
            }
            bool bHasBadProperties = false;
            MessageHandler::IncDisableWarningCount(true);
            int32_t Type = NiZBufferProperty::GetType();
            NiZBufferProperty* pzBuf = static_cast<NiZBufferProperty*>(spObject3D.m_pObject->GetProperty(Type));
            if (pzBuf) {
                NiZBufferProperty* Default = NiZBufferProperty::GetDefault();
                if (pzBuf->IsEqualFast(*Default)) {
                    int32_t v7 = NiZBufferProperty::GetType();
                    spObject3D.m_pObject->RemoveProperty(v7);
                    bHasBadProperties = true;
                }
            }
            NiProperty* pprop = spObject3D.m_pObject->GetProperty(NiVertexColorProperty::GetType());
            if (pprop) {
                NiVertexColorProperty* v8 = NiVertexColorProperty::GetDefault();
                if (static_cast<NiVertexColorProperty*>(pprop)->IsEqualFast(*v8)) {
                    spObject3D.m_pObject->RemoveProperty(NiVertexColorProperty::GetType());
                    bHasBadProperties = true;
                }
            }
            if (bHasBadProperties) {
                if (apStream->m_BSStreamHeader.cAuthor[0]) {
                    Warning(
                        "MODELS: %s: Reexport '%s' to get rid of the ZBuffer and/or VertextColor property.",
                        apStream->m_BSStreamHeader.cAuthor, apFilename
                    );
                }
                else {
                    Warning(
                        "MODELS: Reexport '%s' to get rid of the ZBuffer and/or VertextColor property.", apFilename
                    );
                }
            }
            MessageHandler::IncDisableWarningCount(false);
            if (abPrepareObject) {
                bool bskipprecache = false;
                if (abFaceGenModel || HasMorpherController(spObject3D.m_pObject)) {
                    bskipprecache = true;
                }
                BSShaderManager::PrepareObject(spObject3D.m_pObject, abFaceGenModel, bskipprecache);
            }
        }
    }
    else {
        pFilename = nullptr;
    }
}

AttachDistant3DTask::AttachDistant3DTask(TESObjectREFR* apRef, NiNode* apNode)
  : IOTask(IO_TASK_PRIORITY_CRITICAL), pRef(apRef), spNode(apNode) {}

void AttachDistant3DTask::PostProcess() {
    NiNode* v1;
    TESObjectCELL* parentCell;
    if (IOTask::IsCancelled() || ((!pRef) || pRef->TESObjectREFR::Get3D())) {
        (&spNode)->operator=(nullptr);
    }
    else {
        v1 = (&spNode)->operator->();
        pRef->Set3DSimple(v1);
        parentCell = pRef->TESObjectREFR::GetParentCell();
        TES::pInstance->AttachModel(pRef, parentCell, nullptr, false);
        pRef->SetHasTemp3D(true);
    }
}

AttachDistant3DTask::~AttachDistant3DTask() {}

LoadedFile::LoadedFile(char* a1, BSFile* a2) : pFile(a2) {
    int32_t ifileNameLength = BSstrlen(a1) + 1;
    pFileName = static_cast<char*>(ourMalloc(ifileNameLength));
    BSstrcpy(pFileName, ifileNameLength, a1);
    bInLoadedFileMap = pModelLoader->AddLoadedFileToMap(pFileName, this);
    bFileUsed = false;
    iRefCount = 0;
}

LoadedFile::~LoadedFile() {
    if (!bFileUsed && bInLoadedFileMap) {
        Warning(
            "LoadedFile %s was loaded at %i and added to the map, but is being thrown away without being used",
            pFileName, this
        );
    }
    if (bInLoadedFileMap) {
        pModelLoader->RemoveLoadedFileFromMap(pFileName);
    }
    if (iRefCount) {
        char acMessage[268];
        std::sprintf(
            acMessage, "%s ref object at mem %i being destroyed with %i references\r\n", pFileName, this, iRefCount
        );
        if (BSCoreMessage::HasHandler()) {
            BSCoreMessage::GetInstance()->Message(BSCoreMessage::MS_DEBUGSTRING, acMessage, nullptr, 0, 0);
        }
    }
    ourFree(pFileName);
    if (pFile) {
        delete pFile;
    }
}

// Spins until the loaded-file lock is free, then takes the reference under it.
void LoadedFile::IncRefCount() {
    while (!LSAccessCrit.TryLock()) {}
    NiAtomicIncrement(iRefCount);
    LSAccessCrit.Unlock();
}

void LoadedFile::DecRefCount() {
    while (!LoadedFile::LSAccessCrit.TryLock()) {
        ;
    }
    NiAtomicDecrement(iRefCount);
    LoadedFile::LSAccessCrit.Unlock();
    if ((!iRefCount) && (this)) {
        delete this;
    }
}

// A load of a file already in memory: complete from the start.
QueuedFileLoad::QueuedFileLoad(LoadedFile* apFileName, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spLoadedFile(apFileName), eFileIndex(IO_FILE_INDEX_MISC) {
    SetCompleted();
}

QueuedFileLoad::QueuedFileLoad(const char* apFileName, IO_TASK_PRIORITY aePriority, IO_FILE_INDEX aeFileIndex)
  : QueuedFileEntry(aePriority), spLoadedFile(nullptr), eFileIndex(aeFileIndex) {
    SetFileName(apFileName);
    LookupFileEntry(QueuedFileLoad::GetArchiveType(eFileIndex));
}

void QueuedFileLoad::Run() {
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2422
    );
    char* ProcessLevel = pFileName;
    LoadedFile* LoadedFileFromMap = pModelLoader->GetLoadedFileFromMap(ProcessLevel);
    spLoadedFile.operator=(LoadedFileFromMap);
    if (!spLoadedFile.m_pObject) {
        ARCHIVE_TYPE_INDEX ArchiveType = QueuedFileLoad::GetArchiveType(eFileIndex);
        BSFile* pfile = GetFile(ArchiveType, ARCHIVE_TYPE_ALL);
        if (pfile) {
            LoadedFile* v4;
            LoadedFile* v6 = static_cast<LoadedFile*>(NiMemObject::operator new(sizeof(LoadedFile)));
            if (v6) {
                v4 = ::new (v6) LoadedFile(pFileName, pfile);
            }
            else {
                v4 = nullptr;
            }
            spLoadedFile.operator=(v4);
        }
        else {
            Warning("MODELS: Could not get file for %s.", pFileName);
        }
    }
}

void QueuedFileLoad::Finish() {
    QueuedFile::Finish();
}

void QueuedFileLoad::QueueMe() {
    pIOManager->AddTask(this);
}

uint8_t QueuedFileLoad::GetFileIndex() {
    return (uint8_t)((uint32_t)eFileIndex);
}

QueuedFileLoad::~QueuedFileLoad() {}

ARCHIVE_TYPE_INDEX QueuedFileLoad::GetArchiveType(IO_FILE_INDEX aeFileIndex) {
    switch (aeFileIndex) {
        case IO_FILE_INDEX_TEXTURES: {
            return ARCHIVE_TYPE_INDEX_TEXTURES;
        }
        case IO_FILE_INDEX_MESHES: {
            return ARCHIVE_TYPE_INDEX_MESHES;
        }
        case IO_FILE_INDEX_SOUNDS: {
            return ARCHIVE_TYPE_INDEX_SOUNDS;
        }
        default: break;
    }
    return ARCHIVE_TYPE_INDEX_MISC;
}

QueuedFaceGenFile::QueuedFaceGenFile(const char* apFileName, IO_TASK_PRIORITY aePriority, IO_FILE_INDEX aeFileIndex)
  : QueuedFileLoad(apFileName, aePriority, aeFileIndex), spFaceGenModel(nullptr), spModelMeshData(nullptr) {}

QueuedFaceGenFile::QueuedFaceGenFile(LoadedFile* apLoadedFile, IO_TASK_PRIORITY aePriority)
  : QueuedFileLoad(apLoadedFile, aePriority), spFaceGenModel(nullptr), spModelMeshData(nullptr) {}

// The task of a FaceGen model already loaded: built as a load of no file (the LoadedFile overload with null).
QueuedFaceGenFile::QueuedFaceGenFile(BSFaceGenModel* apModel, BSFaceGenModelMeshData* apMeshData)
  : QueuedFileLoad(static_cast<LoadedFile*>(nullptr), IO_TASK_PRIORITY_VERY_LOW), spFaceGenModel(apModel),
    spModelMeshData(apMeshData) {}

QueuedFaceGenFile::~QueuedFaceGenFile() {}

QueuedTexture::QueuedTexture(const char* apFileName, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spTexture(nullptr), cFlags(0) {
    char apOut[268];
    StandardizeTexturePath(apFileName, apOut, 260u);
    SetFileName(apOut);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_TEXTURES);
}

QueuedTexture::QueuedTexture(BSFileEntry* apFileEntry, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spTexture(nullptr), cFlags(0) {
    SetFileEntry(apFileEntry);
}

// A texture already in the palette: complete from the start.
QueuedTexture::QueuedTexture(NiTexture* apTexture, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spTexture(apTexture), cFlags(0) {
    SetCompleted();
}

void QueuedTexture::Run() {
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 695
    );
    if (pFileEntry) {
        BSTexturePalette::GetTexture(pFileEntry, spTexture);
    }
    else {
        if (pFileName) {
            BSTexturePalette::GetTexture(pFileName, spTexture);
        }
    }
    if (spTexture.m_pObject) {
        SetAlreadyInPalette(true);
        if (spTexture.m_pObject->QState() == PS_DEGRADED && !GetLoadDegraded()) {
            NiTexture* v7 = spTexture.m_pObject;
            v7->Upgrade();
        }
    }
    else {
        BSFile* pfile = GetFile(ARCHIVE_TYPE_INDEX_TEXTURES, ARCHIVE_TYPE_TEXTURES);
        if (pfile) {
            // On a BSFile, GetDevice returns the address of its file name.
            const char* pfileName = reinterpret_cast<const char*>(pfile->GetDevice());
            uint32_t iloadMethod = NiTexture::QLoadMethod();
            if (GetLoadDegraded()) {
                NiTexture::SetLoadMethod(1u);
            }
            if (std::strstr(pfileName, "_e.dd")) {
                NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                NiFixedString apString(pfileName);
                NiFixedString* a1 = &apString;
                spTexture.operator=(static_cast<NiTexture*>(
                    NiSourceCubeMap::CreateTexture(a1, pfile, Renderer_, &NiSourceTexture::ms_kDefaultPrefs)
                ));
            }
            else {
                spTexture.operator=(static_cast<NiTexture*>(
                    NiSourceTexture::Create(pfile, pfileName, NiSourceTexture::ms_kDefaultPrefs, true)
                ));
            }
            NiTexture::SetLoadMethod(iloadMethod);
        }
        else {
            if (pFileEntry) {
                uint32_t uiSize = pFileEntry->Size();
                Warning(
                    "MODELS: Could not get file for texture with file entry offset %i and size %i.",
                    pFileEntry->GetEntryOffset(), uiSize
                );
            }
            else {
                if (pFileName) {
                    Warning("MODELS: Could not get file for texture %s.", pFileName);
                }
            }
        }
    }
}

void QueuedTexture::Finish() {
    NiTexture* v1;
    NiTexture* v2;
    BSFileEntry* v3;
    BSFileEntry* location;
    if ((!GetAlreadyInPalette()) && ((&spTexture)->operator->())) {
        if (QueuedFileEntry::GetFileEntry()) {
            location = QueuedFileEntry::GetFileEntry();
            v1 = (&spTexture)->operator->();
            BSTexturePalette::SetTexture(v1, location);
        }
        else {
            if (pFileName) {
                v2 = (&spTexture)->operator->();
                BSTexturePalette::SetTexture(v2, nullptr);
            }
        }
    }

    if ((QueuedFileEntry::GetFileEntry()) && (GetTrackedFile())) {
        SetInQueuedTextureMap(false);
        v3 = QueuedFileEntry::GetFileEntry();
        pModelLoader->RemoveQueuedTextureFromMap(v3);
    }

    QueuedFile::Finish();
}

void QueuedTexture::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    BSFileEntry* location;
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    if ((QueuedFileEntry::GetFileEntry()) && (GetTrackedFile())) {
        SetInQueuedTextureMap(false);
        location = QueuedFileEntry::GetFileEntry();
        pModelLoader->RemoveQueuedTextureFromMap(location);
    }
}

bool QueuedTexture::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("texture"));
}

void QueuedTexture::QueueMe() {
    if (pFileEntry) {
        BSFileEntry* Location_ = pFileEntry;
        bool v3 = pModelLoader->AddQueuedTextureToMap(Location_, this);
        SetInQueuedTextureMap(v3);
    }
    if (pModelLoader->GetLoadTexturesDegraded()) {
        SetLoadDegraded(true);
    }
    pIOManager->AddTask(this);
}

uint8_t QueuedTexture::GetFileIndex() {
    BSFileEntry* location;
    Archive* pArchive;
    if (QueuedFileEntry::GetFileEntry() &&
        ((location = QueuedFileEntry::GetFileEntry()),
         ((pArchive = ArchiveManager::GetArchiveForFileEntry(location, ARCHIVE_TYPE_TEXTURES)) != nullptr))) {
        return (uint8_t)(pArchive->GetRuntimeID() + 4);
    }

    return (uint8_t)0;
}

QueuedTexture::~QueuedTexture() {}

void QueuedTexture::SetInQueuedTextureMap(bool abValue) {
    SetFlag4(abValue, &cFlags);
}

bool QueuedTexture::GetTrackedFile() {
    return MLGetFlag4(cFlags);
}

bool QueuedTexture::GetAlreadyInPalette() {
    return (bool)((uint8_t)IsFlag1(cFlags));
}

void QueuedTexture::SetAlreadyInPalette(bool abValue) {
    SetFlag1(abValue, &cFlags);
}

bool QueuedTexture::GetLoadDegraded() {
    return HasFlag2(cFlags);
}

void QueuedTexture::SetLoadDegraded(bool abValue) {
    SetFlag2(abValue, &cFlags);
}

QueuedModel::QueuedModel(
    const char* apFileName,
    IO_TASK_PRIORITY aePriority,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool bAddRefCount
)
  : QueuedFileEntry(aePriority), spModel(nullptr), pTESModel(nullptr), eLODFadeMult(aeLODFadeMult), cFlags(0),
    mfOverriddenVisualDistance(0.0f) {
    SetFileName(apFileName);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
    SetPrepareAfterLoad(abPrepareAfterLoad);
    SetAddRefCount(bAddRefCount);
}

QueuedModel::QueuedModel(
    TESModel* apModel,
    IO_TASK_PRIORITY aePriority,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool bAddRefCount
)
  : QueuedFileEntry(aePriority), spModel(nullptr), pTESModel(apModel), eLODFadeMult(aeLODFadeMult), cFlags(0),
    mfOverriddenVisualDistance(0.0f) {
    SetFileName(apModel->GetModel());
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
    SetPrepareAfterLoad(abPrepareAfterLoad);
    SetAddRefCount(bAddRefCount);
}

// A model already loaded: complete from the start.
QueuedModel::QueuedModel(Model* apModel, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spModel(apModel), pTESModel(nullptr), eLODFadeMult(LOD_MULT_NONE), cFlags(0),
    mfOverriddenVisualDistance(0.0f) {
    SetCompleted();
}

void QueuedModel::Run() {
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 952
    );
    if (BSstrlen(pFileName)) {
        char* ProcessLevel = pFileName;
        pModelLoader->FindModel(ProcessLevel, spModel);
        if (spModel.m_pObject) {
            SetAlreadyInModelMap(true);
            return;
        }
        BSFile* pfile = GetFile(ARCHIVE_TYPE_INDEX_MESHES, ARCHIVE_TYPE_MESHES);
        BSStream kstream;
        BSShaderManager::pCurrentModelContext = pFileName;
        if ((pfile && kstream.BSStream::Load(static_cast<const char*>(pFileName), pfile)) || GetForceLoad()) {
            Model* v5;
            Model* v7 = static_cast<Model*>(ourMalloc(sizeof(Model)));
            if (v7) {
                bool FaceGenModel = GetFaceGenModel();
                bool PrepareAfterLoad = GetPrepareAfterLoad();
                v5 = ::new (v7) Model(pFileName, &kstream, PrepareAfterLoad, FaceGenModel);
            }
            else {
                v5 = nullptr;
            }
            spModel.operator=(v5);
        }
        else {
            const char* v4;
            if (pfile) {
                v4 = "load";
            }
            else {
                v4 = "find";
            }
            Warning("MODEL ERROR: Could not %s file '%s'.", v4, pFileName);
        }
        BSShaderManager::pCurrentModelContext = nullptr;
    }
}

void QueuedModel::Finish() {
    NiUpdateData a2;
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 997
    );
    if (spModel.m_pObject) {
        if (GetAddRefCount()) {
            Model* v1 = spModel.m_pObject;
            v1->IncManualRefCount();
        }
        if (!GetAlreadyInModelMap()) {
            Model* v2 = spModel.m_pObject;
            if (v2->GetModel()) {
                Model* v3 = spModel.m_pObject;
                NiNode* Controllers = v3->GetModel();
                BSFadeNode* fadeNode = Controllers->IsFadeNode();
                if (fadeNode) {
                    float v15;
                    a2.~NiUpdateData();
                    ::new (&a2) NiUpdateData(0.0, false, false);
                    fadeNode->Update(a2);
                    const NiBound* WorldBound = &fadeNode->GetWorldBound();
                    float radius = WorldBound->m_fRadius;
                    if (radius < 0.0010000000474974513) {
                        Model* v5 = spModel.m_pObject;
                        NiAVObject* v6 = v5->GetModel();
                        radius = TESObjectREFR::GetBoundMax_0056FC30(v6).z * 0.5;
                    }
                    float v21 = 3.4028235e+38f;
                    const uint32_t uiGridsToLoad = ::uGridsToLoad.UInt();
                    v21 = static_cast<double>(uiGridsToLoad << 12) * 0.5 - (radius + 2048.0);
                    float* fLODBoundRadiusMult = &::fLODBoundRadiusMult.Float();
                    float LODBoundRadius = radius * *fLODBoundRadiusMult;
                    if (radius == 0.0 || !(v21 > static_cast<double>(LODBoundRadius))) {
                        v15 = v21;
                    }
                    else {
                        v15 = LODBoundRadius;
                    }
                    float ScriptProcessingDelay = v15;
                    if (mfOverriddenVisualDistance != 0.0) {
                        ScriptProcessingDelay = mfOverriddenVisualDistance;
                    }
                    float* fLODFadeOutPercent = &::fLODFadeOutPercent.Float();
                    float fNearDistSqr = ScriptProcessingDelay * *fLODFadeOutPercent;
                    fadeNode->SetRange(fNearDistSqr, ScriptProcessingDelay);
                    fadeNode->SetLODMultType(eLODFadeMult);
                }
                Model* path = spModel.m_pObject;
                char* name = pFileName;
                if (!pModelLoader->AddModel(name, path)) {
                    name = pFileName;
                    pModelLoader->FindModel(name, spModel);
                }
            }
        }
    }
    QueuedFile::Finish();
}

bool QueuedModel::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("model"));
}

void QueuedModel::GenerateKey() {
    QueuedFileEntry::GenerateKey();
}

void QueuedModel::CheckFinished() {
    if (!GetFinishedQueuing()) {
        goto LABEL_5;
    }
    if (!GetAllChildrenFinished()) {
        return;
    }
    if (IsPending()) {
        pIOManager->AddTask(this);
    }
    else {
    LABEL_5:;
        QueuedFile::CheckFinished();
    }
}

// A model in an archive loads on that archive's file index (its runtime ID past the first four); a loose one on
// index 1.
uint8_t QueuedModel::GetFileIndex() {
    if (GetFileEntry()) {
        Archive* pArchive = ArchiveManager::GetArchiveForFileEntry(GetFileEntry(), ARCHIVE_TYPE_MESHES);
        if (pArchive) {
            return pArchive->GetRuntimeID() + 4;
        }
    }
    return 1;
}

QueuedModel::~QueuedModel() {}

// Queues the model with its form model's texture list (the overload, slot 0x30), or with none.
void QueuedModel::QueueMe() {
    TESTextureList* pTextureList = nullptr;
    if (pTESModel) {
        pTextureList = pTESModel->GetTextureList();
    }
    QueuedModel* pThis = this;
    pThis->QueueMe(pTextureList);
}

void QueuedModel::QueueMe(TESTextureList* apTextureList) {
    if (apTextureList) {
        // The count byte is zero-extended and kept as the loop bound.
        uint32_t itextureCount = apTextureList->cTextureCount;
        if (itextureCount) {
            BSFileEntry** poffsetArray = apTextureList->pTextureOffsetArray;
            for (uint32_t i = 0; i < itextureCount; ++i) {
                if (poffsetArray[i]) {
                    IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
                    pModelLoader->QueueTexture(poffsetArray[i], IOTaskPriority, this);
                }
            }
        }
    }
    if (pTESModel) {
        TESModelTextureSwap* ptextureSwap = pTESModel->GetAsModelTextureSwap();
        if (ptextureSwap) {
            for (TEX_SWAP* pswap : *&ptextureSwap->TextureSwapList) {
                if ((pswap) && (pswap->pNewTexture)) {
                    IO_TASK_PRIORITY v3 = GetIOTaskPriority();
                    pswap->pNewTexture->QueueTextureSet(v3, this, false);
                }
            }
        }
    }
    SetFinishedQueuing(true);
    CheckFinished();
}

void QueuedModel::SetPrepareAfterLoad(bool abValue) {
    SetFlag1(abValue, &cFlags);
}

void QueuedModel::SetAddRefCount(bool abValue) {
    SetFlag2(abValue, &cFlags);
}

bool QueuedModel::GetFinishedQueuing() {
    return MLGetFlag4(cFlags);
}

void QueuedModel::SetFinishedQueuing(bool abValue) {
    SetFlag4(abValue, &cFlags);
}

bool QueuedModel::GetPrepareAfterLoad() {
    return IsFlag1(cFlags);
}

bool QueuedModel::GetForceLoad() {
    return MLGetFlag_16(cFlags);
}

bool QueuedModel::GetFaceGenModel() {
    return MLGetFlag_32(cFlags);
}

bool QueuedModel::GetAddRefCount() {
    return HasFlag2(cFlags);
}

bool QueuedModel::GetAlreadyInModelMap() {
    return MLGetFlag_8(cFlags);
}

Model* QueuedModel::GetModel() {
    return spModel.m_pObject;
}

void QueuedModel::SetFaceGenModel(bool abValue) {
    MLSetFlag<32>(abValue, cFlags);
}

void QueuedModel::SetOverriddenVisualDistance(float afOverriddenVisualDistance) {
    mfOverriddenVisualDistance = afOverriddenVisualDistance;
}

void QueuedModel::SetForceLoad(bool abValue) {
    MLSetFlag<16>(abValue, cFlags);
}

void QueuedModel::SetAlreadyInModelMap(bool abValue) {
    MLSetFlag_8(abValue, &cFlags);
}

QueuedKF::QueuedKF(const char* apFileName, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spKFModel(nullptr), cFlags(0) {
    SetFileName(apFileName);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
}

// An animation already loaded: complete from the start.
QueuedKF::QueuedKF(KFModel* apKFModel, IO_TASK_PRIORITY aePriority)
  : QueuedFileEntry(aePriority), spKFModel(apKFModel), cFlags(0) {
    SetCompleted();
}

QueuedKF::~QueuedKF() {}

void QueuedKF::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 1321
    );
    char* ProcessLevel = pFileName;
    KFModel* KFModel_ = pModelLoader->FindKFModel(ProcessLevel);
    spKFModel.operator=(KFModel_);
    if (spKFModel.m_pObject) {
        SetAlreadyInKFMap(true);
    }
    else {
        BSFile* pfile = GetFile(ARCHIVE_TYPE_INDEX_MESHES, ARCHIVE_TYPE_MESHES);
        BSStream kstream;
        if (pfile && kstream.BSStream::Load(static_cast<const char*>(pFileName), pfile)) {
            KFModel* v4;
            KFModel* v6 = static_cast<KFModel*>(ourMalloc(sizeof(KFModel)));
            if (v6) {
                v4 = ::new (v6) KFModel(pFileName, &kstream);
            }
            else {
                v4 = nullptr;
            }
            spKFModel.operator=(v4);
        }
        else {
            const char* v3;
            if (pfile) {
                v3 = "load";
            }
            else {
                v3 = "find";
            }
            Warning("MODEL ERROR: Could not %s file '%s'.", v3, pFileName);
        }
    }
}

void QueuedKF::Finish() {
    if ((spKFModel.m_pObject) && (!GetAlreadyInPalette())) {
        KFModel* v3 = spKFModel.m_pObject;
        char* ProcessLevel = pFileName;
        if (!pModelLoader->AddKF(ProcessLevel, v3)) {
            char* v4 = pFileName;
            KFModel* KFModel_ = pModelLoader->FindKFModel(v4);
            spKFModel.operator=(KFModel_);
        }
    }

    QueuedFile::Finish();
}

bool QueuedKF::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("KF"));
}

// QueuedAnimIdle::QueueMe calls it qualified.
void QueuedKF::QueueMe() {
    pIOManager->AddTask(this);
}

// An animation in an archive loads on that archive's file index (its runtime ID past the first four); a loose one on
// index 1. The file entry is looked up in the meshes archives.
uint8_t QueuedKF::GetFileIndex() {
    if (GetFileEntry()) {
        Archive* pArchive = ArchiveManager::GetArchiveForFileEntry(GetFileEntry(), ARCHIVE_TYPE_MESHES);
        if (pArchive) {
            return pArchive->GetRuntimeID() + 4;
        }
    }
    return 1;
}

bool QueuedKF::GetAlreadyInPalette() {
    return (bool)((uint8_t)IsFlag1(cFlags));
}

void QueuedKF::SetAlreadyInKFMap(bool abValue) {
    SetFlag1(abValue, &cFlags);
}

QueuedAnimIdle::QueuedAnimIdle(
    const char* apFileName,
    IO_TASK_PRIORITY aePriority,
    AnimIdle* apAnimIdle,
    TESObjectREFR* apRef
)
  : QueuedKF(apFileName, aePriority), pQueuedForRef(apRef), spAnimIdle(apAnimIdle) {}

QueuedAnimIdle::~QueuedAnimIdle() {}

void QueuedAnimIdle::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    pModelLoader->RemoveAnimIdleFromMap(spAnimIdle.operator->());
}

bool QueuedAnimIdle::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("AnimIdle"));
}

// Hands a loaded animation to its idle unless the load was cancelled, then drops the idle from the queued map.
void QueuedAnimIdle::PostProcess() {
    if ((spKFModel.operator->()) && (!IOTask::IsCancelled())) {
        KFModel* pKFModel = spKFModel.operator->();
        spAnimIdle.operator->()->Loaded(pKFModel);
    }

    pModelLoader->RemoveAnimIdleFromMap(spAnimIdle.operator->());
}

void QueuedAnimIdle::QueueMe() {
    if (spAnimIdle.m_pObject) {
        for (int32_t i = 0; i < 2; ++i) {
            if (spAnimIdle.m_pObject->GetAnimObject(i)) {
                TESModel* apModel;
                TESObjectANIO* AnimObject = spAnimIdle.m_pObject->GetAnimObject(i);
                if (AnimObject) {
                    apModel = static_cast<TESModelTextureSwap*>(AnimObject);
                }
                else {
                    apModel = nullptr;
                }
                IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
                pModelLoader->QueueModel(apModel, IOTaskPriority, this, LOD_MULT_ITEMS, true, false, false);
            }
        }
    }
    QueuedKF::QueueMe();
}

void QueuedAnimIdle::CheckFinished() {
    if (IOTask::IsTaskFinished() && QueuedFile::GetAllChildrenFinished()) {
        IOTask::AddToPostProcessQueue();
    }
}

// The helmet's model, FaceGen model and clone slots start empty; the queuing reference is the biped's requester.
QueuedHelmet::QueuedHelmet(
    BipedAnim* apBipedAnim,
    IO_TASK_PRIORITY aePriority,
    [[maybe_unused]] BIPED_OBJECT aeBipedObject
)
  : QueuedFile(aePriority), pBipedAnim(apBipedAnim), pRef(apBipedAnim->GetRequester()), cFlags(0) {}

// Loads the FaceGen model of every FaceGen biped slot, stopping the loads once the task is cancelled.
void QueuedHelmet::Run() {
    for (int32_t i = 0; i < BIPED_OBJECT_COUNT; ++i) {
        BIPED_OBJECT eObject = static_cast<BIPED_OBJECT>(i);
        if (pBipedAnim->IsFaceGenObject(eObject) && !IOTask::IsCancelled()) {
            spHelmetFaceGenModel[i] = pBipedAnim->LoadFaceGenModel(eObject);
        }
    }
}

void QueuedHelmet::Finish() {
    if ((!(&spParent)->operator->()) && (!IOTask::IsCancelled())) {
        IOTask::AddToPostProcessQueue();
    }
    QueuedFile::Finish();
}

void QueuedHelmet::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    pModelLoader->RemoveHelmetFromMap(pRef);
}

bool QueuedHelmet::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    BSsprintf(apDescription, aiBufferSize, "Queued helmet for biped anim %08X", pBipedAnim);
    return true;
}

void QueuedHelmet::PostProcess() {
    Attach();
}

void QueuedHelmet::QueueMe() {
    for (BIPED_OBJECT aeBipedObject = BO_HEAD; aeBipedObject < BIPED_OBJECT_COUNT;
         aeBipedObject = static_cast<BIPED_OBJECT>(aeBipedObject + 1)) {
        if (pBipedAnim->IsFaceGenObject(aeBipedObject)) {
            TESModel* a2 = pBipedAnim->GetBipObject(aeBipedObject)->pPart;
            IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
            pModelLoader->QueueModel(
                a2, spQueuedHelmetModel[aeBipedObject], IOTaskPriority, this, LOD_MULT_ACTORS, true, true, true, 0.0f
            );
            IO_TASK_PRIORITY v1 = GetIOTaskPriority();
            pModelLoader->QueueEGMFile(a2, v1, this, -1);
        }
    }
    SetFinishedQueuing(true);
    CheckFinished();
}

void QueuedHelmet::CheckFinished() {
    if (GetFinishedQueuing()) {
        if (GetAllChildrenFinished() && IsPending()) {
            pIOManager->AddTask(this);
        }
        else {
            AutoMemContext tempMemContext(
                MEM_CONTEXT_TES_ACTOR, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp",
                1680
            );
            NiPointer<NiObject> spTempObject(static_cast<NiObject*>(nullptr));
            if (IsTaskFinished() && (GetAllChildrenFinished() && (!IsCancelled() && pRef != pPlayer))) {
                for (BIPED_OBJECT n = BO_HEAD; n < BIPED_OBJECT_COUNT; n = static_cast<BIPED_OBJECT>(n + 1)) {
                    if ((spQueuedHelmetModel[n].m_pObject) && (spQueuedHelmetModel[n].m_pObject->GetModel())) {
                        Model* AdditionalGeometryData = spQueuedHelmetModel[n].m_pObject->GetModel();
                        if (AdditionalGeometryData->GetModel()) {
                            BIPED_OBJECT afAngle = n;
                            Model* v2 = spQueuedHelmetModel[n].m_pObject->GetModel();
                            NiNode* Controllers = v2->GetModel();
                            NiNode* afAnglea =
                                pBipedAnim->CloneHelmet(spHelmetFaceGenModel[n].m_pObject, Controllers, afAngle);
                            spClonedHelmetNode[n].operator=(afAnglea);
                            if (spClonedHelmetNode[n].m_pObject) {
                                bool bskin = false;
                                for (int32_t i = 0;; ++i) {
                                    uint32_t ArrayCount = spClonedHelmetNode[n].m_pObject->GetArrayCount();
                                    if (i >= ArrayCount) {
                                        break;
                                    }
                                    NiTriBasedGeom* ptri = static_cast<NiTriBasedGeom*>(
                                        spClonedHelmetNode[n].m_pObject->GetAt(static_cast<uint32_t>(i))
                                    );
                                    if ((ptri) && (ptri->IsGeometry())) {
                                        NiAVObject* pobj = (NiAVObject*)ptri;
                                        if (ptri->GetSkinInstance()) {
                                            bskin = true;
                                            break;
                                        }
                                    }
                                }
                                if (!bskin) {
                                    BSFaceGenModel* pfgmodel;
                                    if (spHelmetFaceGenModel[n].m_pObject) {
                                        pfgmodel = spHelmetFaceGenModel[n].m_pObject;
                                    }
                                    else {
                                        pfgmodel = pBipedAnim->LoadFaceGenModel(n);
                                    }
                                    BSFaceGenModel* pfgmodel_2 = pfgmodel;
                                    if (pfgmodel) {
                                        for (int32_t i_2 = 0;; ++i_2) {
                                            uint32_t v4 = spClonedHelmetNode[n].m_pObject->GetArrayCount();
                                            if (i_2 >= v4) {
                                                break;
                                            }
                                            NiAVObject* pobj_2 =
                                                spClonedHelmetNode[n].m_pObject->GetAt(static_cast<uint32_t>(i_2));
                                            NiTriBasedGeom* ptri_2 = static_cast<NiTriBasedGeom*>(pobj_2);
                                            if ((pobj_2) && (ptri_2->GetModelData()) && (pBipedAnim->GetRequester()) &&
                                                (pBipedAnim->GetRequester()->GetObjectReference())) {
                                                TESObjectREFR* Requester = pBipedAnim->GetRequester();
                                                if (Requester->GetObjectReference()->GetFormType() == NPC__ID) {
                                                    TESNPC* pnpc_2 = static_cast<TESNPC*>(
                                                        pBipedAnim->GetRequester()->GetObjectReference()
                                                    );
                                                    FR2MatrixVTC<float> fgcoord[2][2];
                                                    NiPointer<NiGeometryData> spdata(
                                                        static_cast<NiGeometryData*>(nullptr)
                                                    );
                                                    ptri_2->GetModelData()->CreateDeepCopy(spTempObject);
                                                    spdata.operator=(
                                                        static_cast<NiGeometryData*>(spTempObject.m_pObject)
                                                    );
                                                    NiGeometryData* afAngleb = spdata.m_pObject;
                                                    ptri_2->SetModelData(afAngleb);
                                                    pnpc_2->GetFaceCoord(fgcoord);
                                                    if (!BSFaceGenManager::GetUseFaceGenHeads() ||
                                                        pfgmodel_2->ApplyCoordinateToExistingMesh(
                                                            &fgcoord, ptri_2, false
                                                        )) {
                                                        NiMatrix3 matrot;
                                                        matrot.MakeYRotation(-1.5707964f);
                                                        ptri_2->SetRotate((&ptri_2->m_kLocal)->m_Rotate * matrot);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            QueuedFile::CheckFinished();
        }
    }
    else {
        QueuedFile::CheckFinished();
    }
}

QueuedHelmet::~QueuedHelmet() {}

void QueuedHelmet::SetFinishedQueuing(bool abValue) {
    SetFlag1(abValue, &cFlags);
}

bool QueuedHelmet::GetFinishedQueuing() {
    return IsFlag1(cFlags);
}

void QueuedHelmet::Attach() {
    for (BIPED_OBJECT aeBipedObject = BO_HEAD; aeBipedObject < BIPED_OBJECT_COUNT;
         aeBipedObject = static_cast<BIPED_OBJECT>(aeBipedObject + 1)) {
        if (spClonedHelmetNode[aeBipedObject].m_pObject) {
            NiNode* v2 = spClonedHelmetNode[aeBipedObject].m_pObject;
            BSFaceGenModel* v1 = spHelmetFaceGenModel[aeBipedObject].m_pObject;
            pBipedAnim->AttachHelmet(v1, v2, aeBipedObject);
        }
    }
    pModelLoader->RemoveHelmetFromMap(pRef);
}

QueuedMagicItem::QueuedMagicItem(MagicItem* apMagicItem, EffectSetting* apAVEffectSetting, IO_TASK_PRIORITY aePriority)
  : QueuedFile(aePriority), pMagicItem(apMagicItem), pEffectSetting(apAVEffectSetting) {}

QueuedMagicItem::~QueuedMagicItem() {}

void QueuedMagicItem::QueueMe() {
    EffectItemList* plist_2;
    bool beffectModelQueued = false;
    if ((this->pEffectSetting->GetModelLength()) && (!this->pEffectSetting->IsEffectQueued())) {
        TESModel* apModel;
        EffectSetting* pEffectSetting = this->pEffectSetting;
        if (pEffectSetting) {
            apModel = static_cast<TESModel*>(pEffectSetting);
        }
        else {
            apModel = nullptr;
        }
        IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
        pModelLoader->QueueModel(apModel, IOTaskPriority, this, LOD_MULT_NONE, true, false, false);
        beffectModelQueued = true;
    }

    this->pEffectSetting->QueueEffect();
    MagicItem* pMagicItem = this->pMagicItem;
    if (pMagicItem) {
        plist_2 = static_cast<EffectItemList*>(pMagicItem);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    if (!plist_2->IsEmpty()) {
        while (plist_3) {
            EffectItemList* plist;
            EffectSetting* pes = plist_3->m_item->pEffectSetting;
            if (pes->getArchType() && (!beffectModelQueued || !pes->IsAssociatedItemQueued())) {
                TESObject* pnpc__parmor__pweapon = static_cast<TESObject*>(pes->GetAssociatedItem());
                if (pnpc__parmor__pweapon) {
                    if (pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                        if (pnpc__parmor__pweapon->GetFormType() == WEAP_ID) {
                            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pnpc__parmor__pweapon);
                            if (static_cast<TESModelTextureSwap*>(pWeapon)->GetModelLength()) {
                                ENUM_LOD_MULT LODMult = TES::GetLODMult(pnpc__parmor__pweapon);
                                IO_TASK_PRIORITY v2 = GetIOTaskPriority();
                                pModelLoader->QueueModel(
                                    static_cast<TESModelTextureSwap*>(pWeapon), v2, this, LODMult, true, false, false
                                );
                            }
                        }
                    }
                    else {
                        if (pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                            if (pnpc__parmor__pweapon->GetFormType() == ARMO_ID) {
                                ENUM_LOD_MULT v11 = TES::GetLODMult(pnpc__parmor__pweapon);
                                IO_TASK_PRIORITY v8 = GetIOTaskPriority();
                                TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(pnpc__parmor__pweapon);
                                TESModelTextureSwap* BipedTESModel =
                                    static_cast<TESBipedModelForm*>(pArmor)->GetBipedTESModel(TESActorBase::SEX_MALE);
                                pModelLoader->QueueModel(BipedTESModel, v8, this, v11, true, false, false);
                                ENUM_LOD_MULT v12 = TES::GetLODMult(pnpc__parmor__pweapon);
                                IO_TASK_PRIORITY v9 = GetIOTaskPriority();
                                TESModelTextureSwap* v4 =
                                    static_cast<TESBipedModelForm*>(pArmor)->GetBipedTESModel(TESActorBase::SEX_FEMALE);
                                pModelLoader->QueueModel(v4, v9, this, v12, true, false, false);
                            }
                        }
                        else {
                            if (pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                                if (pnpc__parmor__pweapon->GetFormType() == NPC__ID) {
                                    BSSimpleList<const char*>* modelList =
                                        static_cast<TESNPC*>(pnpc__parmor__pweapon)
                                            ->TESNPC::BuildDefaultModelList(true, true);
                                    IO_TASK_PRIORITY v5 = GetIOTaskPriority();
                                    pModelLoader->QueueModelList(modelList, nullptr, v5, this, nullptr);
                                    modelList->RemoveAll();
                                    if (modelList) {
                                        delete modelList;
                                    }
                                }
                                else {
                                    if (pnpc__parmor__pweapon->GetFormType() == CREA_ID) {
                                        IO_TASK_PRIORITY v6 = GetIOTaskPriority();
                                        TESModel* pCreatureModel = static_cast<TESCreature*>(pnpc__parmor__pweapon);
                                        pModelLoader->QueueAnimations(pCreatureModel, v6, this, nullptr, true, true);
                                        ENUM_LOD_MULT v13 = TES::GetLODMult(pnpc__parmor__pweapon);
                                        IO_TASK_PRIORITY v7 = GetIOTaskPriority();
                                        pModelLoader->QueueModel(pCreatureModel, v7, this, v13, true, false, false);
                                    }
                                }
                            }
                        }
                    }
                }
                if (!pes->IsAssociatedItemLoaded() && !pes->IsAssociatedItemQueued() &&
                    pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                    MagicItem::IncSummonedCreatureLoadedCount();
                }
                pes->QueueAssociatedItem();
            }
            // The next node, as the EffectItemList it is the list part of.
            BSSimpleList<EffectItem*>* FormHealth_0 = plist_3->m_pkNext;
            plist = static_cast<EffectItemList*>(FormHealth_0);
            plist_3 = plist;
        }
    }
    SetCompleted();
    CheckFinished();
}

// Once the load and its children are done the item finishes loading; an unreferenced task then deletes itself
// (deleting destructor, flag 1).
void QueuedMagicItem::CheckFinished() {
    if (IOTask::IsTaskFinished() && QueuedFile::GetAllChildrenFinished()) {
        pMagicItem->FinishedLoading();
        if (!GetRefCount()) {
            delete this;
        }
    }
}

QueuedHead::QueuedHead(TESNPC* apNPC, IO_TASK_PRIORITY aePriority)
  : QueuedFile(aePriority), pNPC(apNPC), spBipedNode(nullptr), spSkinnedNode(nullptr) {}

QueuedHead::~QueuedHead() {}

// Builds the NPC's FaceGen head under the FaceGen lock and keeps both nodes. The player's base form (form ID 7) then
// drops its race's EGT data.
void QueuedHead::Run() {
    BSFaceGenManager::LockFaceGenAccess();
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp",
        1529
    );
    BSFaceGenNiNode* pBipedNode = nullptr;
    BSFaceGenNiNode* pSkinnedNode = nullptr;
    pNPC->InitHead(pBipedNode, pSkinnedNode);
    spBipedNode = pBipedNode;
    spSkinnedNode = pSkinnedNode;
    if (pNPC->GetFormID() == 7) {
        pNPC->TESRaceForm::GetFormRace()->KillEGTData();
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    pContext->~AutoMemContext();
}

bool QueuedHead::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    BSsprintf(apDescription, aiBufferSize, "Queued head for NPC %s", pNPC->TESFullName::GetFullName());
    return true;
}

void QueuedHead::QueueMe() {
    BSFaceGenManager::QueueHeadParts(pNPC, GetIOTaskPriority(), this);
    CheckFinished();
}

// Once every child is done, a head still pending goes back to the IO manager (AddTask, slot 0x48); otherwise it
// finishes as a queued file.
void QueuedHead::CheckFinished() {
    if (QueuedFile::GetAllChildrenFinished()) {
        if (IOTask::IsPending()) {
            pIOManager->AddTask(this);
        }
        else {
            QueuedFile::CheckFinished();
        }
    }
}

void QueuedHead::Attach(TESObjectREFR* apRef, BipedAnim* apBipedAnim) {
    AutoMemContext apMemory(
        MEM_CONTEXT_FACEGEN_SYSTEM, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp",
        1555
    );
    static_cast<TESNPC*>(apRef->GetObjectReference())
        ->TESNPC::AttachHead(apRef, apBipedAnim, spBipedNode.m_pObject, spSkinnedNode.m_pObject);
    if (apRef != pPlayer) {
        uint32_t CastingType = apRef->iFormID;
        SetReferenceIDOnScenegraph(spBipedNode.m_pObject, CastingType);
        if (spSkinnedNode.m_pObject) {
            uint32_t v4 = apRef->iFormID;
            SetReferenceIDOnScenegraph(spSkinnedNode.m_pObject, v4);
        }
    }
    if (spSkinnedNode.m_pObject) {
        spSkinnedNode.m_pObject->pActor = static_cast<Actor*>(apRef);
    }
    if (spBipedNode.m_pObject) {
        spBipedNode.m_pObject->pActor = static_cast<Actor*>(apRef);
    }
}

QueuedReference::QueuedReference(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority)
  : QueuedFile(aePriority), pRef(apRef), spQueuedModel(nullptr), spModel(nullptr), spCloned3D(nullptr),
    spAttachDistant3DTask(nullptr) {}

void QueuedReference::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    if (spAttachDistant3DTask.m_pObject) {
        BSTask<int64_t>* v3 = spAttachDistant3DTask.m_pObject;
        v3->SetTaskCanceled();
    }
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    if (spCloned3D.m_pObject) {
        NiAVObject* v4 = spCloned3D.m_pObject;
        pRef->Set3DVerySimple(v4);
        spCloned3D.operator=(static_cast<NiAVObject*>(nullptr));
        pRef->Set3D(nullptr, false);
    }
    pModelLoader->RemoveReferenceFromMap(pRef);
    if (pRef->GetParentCell()) {
        TESObjectCELL* ParentCell = pRef->GetParentCell();
        ParentCell->DecQueuedRefCount();
        if (pRef->Is3DCritical()) {
            TESObjectCELL* v6 = pRef->GetParentCell();
            v6->DecCriticalQueuedRefCount();
        }
    }
}

bool QueuedReference::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    if (IOTask::IsCancelled()) {
        return false;
    }
    TESBoundObject* objectReference = pRef->TESObjectREFR::GetObjectReference();
    const char* formTypeString = objectReference->TESForm::GetFormTypeString();
    uint32_t v6 = pRef->TESForm::GetFormID();
    const char* v5 = pRef->GetFormEditorID();
    BSsprintf(apDescription, aiBufferSize, "Queued ref '%s' (%08X) of type %s", v5, v6, formTypeString);
    return true;
}

void QueuedReference::PostProcess() {
    Attach();
}

void QueuedReference::QueueMe() {
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 1834
    );
    if ((pRef) && (pRef->GetVisibleDistant() && !pRef->Get3D())) {
        UseDistant3D();
    }

    if (pRef) {
        BSSimpleList<REF_DECAL_DATA*>* pdecallist = (&pRef->m_Extra)->GetDecalRefs();
        while (pdecallist && !pdecallist->IsEmpty()) {
            REF_DECAL_DATA* pdata = pdecallist->m_item;
            pdecallist = pdecallist->m_pkNext;
            if ((pdata->pDecalRef) && (pdata->pDecalRef->GetObjectReference()->GetFormType() == TXST_ID)) {
                BGSTextureSet* ptexset_2 = static_cast<BGSTextureSet*>(pdata->pDecalRef->GetObjectReference());
                uint8_t Priority = GetPriority();
                ptexset_2->QueueTextureSet(static_cast<IO_TASK_PRIORITY>(Priority), this, false);
            }
        }
    }
    QueueModels();
    if (IsPending()) {
        SetCompleted();
    }
    CheckFinished();
}

void QueuedReference::CheckFinished() {
    if (IsTaskFinished() && GetAllChildrenFinished() && !IsCancelled()) {
        if ((spQueuedModel.m_pObject) && (!spModel.m_pObject)) {
            Model* AdditionalGeometryData = spQueuedModel.m_pObject->GetModel();
            spModel.operator=(AdditionalGeometryData);
        }

        bool bCanBeCloned = false;
        if ((spModel.m_pObject) && (pRef != pPlayer)) {
            TESObjectCELL* ParentCell = pRef->GetParentCell();
            if (TES::pInstance->IsCellLoaded(ParentCell, false) && TES::pInstance->CanAttach3D(pRef)) {
                bCanBeCloned = pRef->TESObjectREFR::Get3D() == nullptr;
            }
        }

        if (pRef->IsProjectile() || pRef->IsExplosion()) {
            bCanBeCloned = false;
        }
        if (bCanBeCloned) {
            pModelLoader->AddBackgroundCloneReference(this);
        }
        else {
            AddToPostProcessQueue();
        }
    }
}

void QueuedReference::QueueModels() {
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 1874
    );
    TESForm* a2 = this->pRef->GetObjectReference();
    TESModel* TESModel_ = this->pRef->GetTESModel();
    if ((TESModel_) && (*TESModel_->GetModel())) {
        TESObjectREFR* pRef = this->pRef;
        IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
        pModelLoader->QueueBoundObject(static_cast<TESBoundObject*>(a2), spQueuedModel, IOTaskPriority, this, pRef);
        if (!spQueuedModel.m_pObject) {
            const char* v2 = TESModel_->GetModel();
            pModelLoader->FindModel(v2, spModel);
        }
        BGSDestructibleObjectForm* DestructionForm = BGSDestructibleObjectForm::GetDestructionForm(a2);
        if (DestructionForm) {
            IO_TASK_PRIORITY v3 = GetIOTaskPriority();
            DestructionForm->QueueFiles(static_cast<TESBoundObject*>(a2), v3, this);
        }
    }
}

void QueuedReference::UseDistant3D() {
    char cPath[264];
    NiNode* ptempnode = nullptr;
    TESModel::GetDistantModelName_(cPath, pRef->GetObjectReference());
    NiPointer<Model> spmodel(static_cast<Model*>(nullptr));
    pModelLoader->FindModel(cPath, spmodel);
    if ((spmodel.m_pObject) && (spmodel.m_pObject->GetModel())) {
        NiObject* pclone = spmodel.m_pObject->GetModel()->Clone();
        if (pclone) {
            ptempnode = pclone->IsNode();
            if (ptempnode) {
                if (ptempnode->IsFadeNode()) {
                    static_cast<BSFadeNode*>(ptempnode)->BSFadeNode::SetLODMultType(LOD_MULT_DISTANTLOD);
                }
            }
            else {
                // A clone that is no node is released through a smart pointer.
                NiPointer<NiObject> pkiller(pclone);
            }
        }
    }

    if (ptempnode) {
        AttachDistant3D(ptempnode);
    }
}

void QueuedReference::AttachDistant3D(NiNode* apNode) {
    const NiPoint3* v2 = &pRef->GetLocationOnReference();
    apNode->SetTranslate(*v2);
    apNode->SetRotate(pRef->GetOrientation());
    float X = pRef->GetScale();
    apNode->SetScale(X);
    uint32_t v4 = appMain->uiThreadID;
    if (v4 == BSGetCurrentThreadID()) {
        pRef->Set3DSimple(apNode);
        TESObjectCELL* ParentCell = pRef->GetParentCell();
        TES::pInstance->AttachModel(pRef, ParentCell, nullptr, false);
        pRef->SetHasTemp3D(true);
    }
    else {
        AttachDistant3DTask* v8;
        AttachDistant3DTask* apMemory = static_cast<AttachDistant3DTask*>(ourMalloc(sizeof(AttachDistant3DTask)));
        if (apMemory) {
            v8 = ::new (apMemory) AttachDistant3DTask(pRef, apNode);
        }
        else {
            v8 = nullptr;
        }
        spAttachDistant3DTask.operator=(v8);
        AttachDistant3DTask* Xa = spAttachDistant3DTask.m_pObject;
        pModelLoader->QueueAttachDistant3DTask(Xa);
    }
}

bool QueuedReference::BackgroundClone() {
    bool bResult = false;
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2026
    );
    TESObjectCELL* pCell = pRef->GetParentCell();
    if (TES::pInstance->IsCellLoaded(pCell, false)) {
        if (TES::pInstance->CanAttach3D(pRef) && !pRef->TESObjectREFR::Get3D()) {
            NiAVObject* pCloned = pRef->Load3D(true);
            spCloned3D.operator=(pCloned);
            if (spCloned3D.m_pObject) {
                NiRefObject* v3 = spCloned3D.m_pObject;
                v3->DecRefCount();
            }
        }
        bResult = true;
    }
    return bResult;
}

void QueuedReference::Attach() {
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2052
    );
    if (!IsCancelled()) {
        TESObjectCELL* pCell;
        bool v12 = false;
        bool bIsDisabled = pRef->GetDisabled() || pRef->GetDelete();
        v12 = bIsDisabled;
        if (pRef->GetParentCell()) {
            pCell = pRef->GetParentCell();
            pCell->CellRefLockEnter();
        }
        if (!v12) {
            pCell = pRef->GetParentCell();
            if (TES::pInstance->IsCellLoaded(pCell, false)) {
                if (spModel.m_pObject) {
                    Model* v3 = spModel.m_pObject;
                    NiNode* Controllers = v3->GetModel();
                    pModelLoader->LoadAddonNodes(pRef, Controllers);
                }
                pCell = pRef->GetParentCell();
                TES::pInstance->AttachModel(pRef, pCell, this, false);
            }
        }
        pModelLoader->RemoveReferenceFromMap(pRef);
        if (pRef->GetParentCell()) {
            pCell = pRef->GetParentCell();
            pCell->DecQueuedRefCount();
            if (pRef->Is3DCritical()) {
                pCell = pRef->GetParentCell();
                pCell->DecCriticalQueuedRefCount();
            }
            pCell = pRef->GetParentCell();
            pCell->CellRefLockLeave();
        }
    }
}

QueuedReference::~QueuedReference() {}

NiAVObject* QueuedReference::GetCloned3D() {
    return spCloned3D;
}

void LockFreeQueue_Destruct_44F400(LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface* apThis) {
    while (apThis->iDeleteCount) {
        apThis->ScanForDeletedNodes();
    }
}

// The node's data is released and the node is linked into the delete list through the same field, Data; the owner's
// batch size is read inline here.
void LockFreeQueue_DeleteNode_C3F090(
    LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface* apThis,
    LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueNode* apNode
) {
    apNode->Data = static_cast<IOTask*>(nullptr);
    // The delete list's link is a LockFreeQueueNode* kept in the node's untyped data field, cast to IOTask*.
    apNode->Data.m_pObject = reinterpret_cast<IOTask*>(apThis->pDeleteHead);
    apThis->pDeleteHead = apNode;
    ++apThis->iDeleteCount;
    if (apThis->iDeleteCount == static_cast<LockFreeQueue<NiPointer<IOTask>>*>(apThis->pOwner)->iDeleteBatchSize) {
        LockFreeQueue_ScanForDeletedNodes_C42B80(apThis);
    }
}

// The deletion list links its nodes through the data slot (the NiPointer's word).
void LockFreeQueue_ScanForDeletedNodes_C42B80(LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface* apThis) {
    using IOTaskQueue = LockFreeQueue<NiPointer<IOTask>>;
    using Node = IOTaskQueue::LockFreeQueueNode;
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_TASK_OVERHEAD;
    uint32_t uiReferencedNodeCount = 2 * static_cast<IOTaskQueue*>(apThis->pOwner)->GetMaxThreads();
    // new[]'s overflow check: a count too large for the pointer elements asks for SIZE_MAX bytes.
    Node** ppReferencedNodes = static_cast<Node**>(MemoryManager_Singleton.Allocate(
        uiReferencedNodeCount <= SIZE_MAX / sizeof(Node*) ? sizeof(Node*) * uiReferencedNodeCount : SIZE_MAX
    ));
    rt::memcpy(
        ppReferencedNodes, static_cast<IOTaskQueue*>(apThis->pOwner)->pReferencedNodes,
        sizeof(Node*) * uiReferencedNodeCount
    );
    Node* pNewDeleteHead = nullptr;
    uint32_t uiNewDeleteCount = 0;
    while (apThis->pDeleteHead) {
        Node* pNode = apThis->pDeleteHead;
        apThis->pDeleteHead = reinterpret_cast<Node*>(pNode->Data.m_pObject);
        bool bFound = false;
        for (uint32_t i = 0; i < uiReferencedNodeCount; ++i) {
            if (pNode == ppReferencedNodes[i]) {
                bFound = true;
                break;
            }
        }
        if (bFound) {
            pNode->Data.m_pObject = reinterpret_cast<IOTask*>(pNewDeleteHead);
            pNewDeleteHead = pNode;
            ++uiNewDeleteCount;
        }
        else {
            pNode->Data.m_pObject = nullptr;
            if (pNode) {
                // EXE-INLINED none: the node's deleting destructor with MemoryManager::Deallocate (the NiPointer's
                // release through BSTask<long long>::DecRefCount, then the free).
                if (pNode->Data.m_pObject) {
                    pNode->Data.m_pObject->DecRefCount();
                }
                MemoryManager_Singleton.Deallocate(pNode);
            }
        }
    }
    MemoryManager_Singleton.Deallocate(ppReferencedNodes);
    apThis->pDeleteHead = pNewDeleteHead;
    apThis->iDeleteCount = uiNewDeleteCount;
    GetThreadTLSData()->eMemContext = eMemContext;
}

void SetFlag1(bool abValue, uint8_t* apFlags) {
    if (abValue) {
        *apFlags |= 1u;
    }
    else {
        *apFlags &= ~1u;
    }
}

void SetFlag2(bool abValue, uint8_t* apFlags) {
    if (abValue) {
        *apFlags |= 2u;
    }
    else {
        *apFlags &= ~2u;
    }
}

bool HasFlag2(uint8_t aFlags) {
    return (aFlags & 2) != 0;
}

bool IsFlag1(uint8_t aFlags) {
    return ((aFlags & 1) != 0);
}

uint8_t* SetFlag4(bool abValue, uint8_t* apFlags) {
    uint8_t* result = apFlags;
    if (abValue) {
        *apFlags |= 4u;
    }
    else {
        *apFlags &= ~4u;
    }
    return result;
}

bool MLGetFlag4(uint8_t aFlags) {
    return (aFlags & 4) != 0;
}

// Allocates a thread-local storage index: only calls the TlsAlloc import and returns its
// result.
uint32_t BSTlsAlloc() {
    return TlsAlloc();
}

// Frees a TLS index: only calls the TlsFree import and returns its result.
void BSTlsFree(uint32_t aiTlsIndex) {
    TlsFree(aiTlsIndex);
}

// GetFirst of the queued reference map interface: the bucket's head is read until it is stable (its unmarked copy
// equals the bucket's word), then its key is copied through the owner's CopyKeyDirect and its data assigned; a head
// marked meanwhile restarts the walk.
bool LockFreeMap_GetFirst_906AF0(
    QueuedReferenceMap::LockFreeMapInterface* apThis,
    uint32_t aiBucket,
    TESObjectREFR*& arKey,
    NiPointer<QueuedReference>& arData
) {
    bool bResult = false;
    while (true) {
        apThis->pPrev = static_cast<QueuedReferenceMap*>(apThis->pOwner)->GetBucket(static_cast<int32_t>(aiBucket));
        apThis->Current.pPointer = apThis->pPrev->pPointer;
        *apThis->pReferencedNodes[1] = apThis->Current.GetPointer();
        InitializationFlow();
        QueuedReferenceMarkedPointer kUnmarkedHead(false, apThis->Current.GetPointer());
        if (apThis->pPrev->pPointer != kUnmarkedHead.pPointer) {
            continue;
        }
        if (!apThis->Current.GetPointer()) {
            bResult = false;
            break;
        }
        static_cast<QueuedReferenceMap*>(apThis->pOwner)->CopyKeyDirect(apThis->Current.operator->()->kKey, arKey);
        arData = apThis->Current.operator->()->kData;
        bResult = true;
        InitializationFlow();
        if (!apThis->Current.operator->()->GetMark()) {
            break;
        }
    }
    apThis->ClearReferencedNodes();
    return bResult;
}

// GetNext of the queued reference map interface: after Find on arKey, the entry after it (Next) when the key is still
// there, else the entry Find stopped at (Current); a marked node found that way restarts the walk, and one marked after
// its copy clears arData again.
bool LockFreeMap_GetNext_906C10(
    QueuedReferenceMap::LockFreeMapInterface* apThis,
    uint32_t aiBucket,
    TESObjectREFR*& arKey,
    NiPointer<QueuedReference>& arData
) {
    bool bResult = false;
    while (true) {
        bResult = apThis->Find(aiBucket, arKey);
        QueuedReferenceMarkedPointer* pEntry = bResult ? &apThis->Next : &apThis->Current;
        if (!pEntry->GetPointer()) {
            bResult = false;
            break;
        }
        if (pEntry->operator->()->GetMark()) {
            continue;
        }
        static_cast<QueuedReferenceMap*>(apThis->pOwner)->CopyKeyDirect(pEntry->operator->()->kKey, arKey);
        arData = pEntry->operator->()->kData;
        bResult = true;
        InitializationFlow();
        if (!pEntry->operator->()->GetMark()) {
            break;
        }
        arData = static_cast<QueuedReference*>(nullptr);
    }
    apThis->ClearReferencedNodes();
    return bResult;
}

bool MLGetFlag_8(uint8_t aFlags) {
    return ((aFlags & 8) != 0);
}

bool CompareAndSwapEx(int32_t* apDestination, int32_t aiExchange, int32_t aiCompare) {
    bool v4 = BSAtomicCompareExchange(apDestination, aiCompare, aiExchange) == aiCompare;
    InitializationFlow();
    return v4;
}

// The byte difference is shifted right by 2, so a difference that is not a multiple of 4 rounds down; C++ pointer
// subtraction assumes an exact multiple, which lets clang fold the count and the scaling back together.
float* CopyOpt(float* apFirst, float* apLast, float* apDest) {
    ptrdiff_t iCount = (reinterpret_cast<intptr_t>(apLast) - reinterpret_cast<intptr_t>(apFirst)) >> 2;
    float* pResult = apDest + iCount;
    if (iCount > 0) {
        memmove_s(apDest, iCount * sizeof(float), apFirst, iCount * sizeof(float));
    }
    return pResult;
}

void QueuedReplacementKFList_43DC50(QueuedFile* apThis, [[maybe_unused]] void* apEdx) {
    apThis->~QueuedFile();
}

void DestroyFloatVector(void* apThis, [[maybe_unused]] void* apEdx) {
    static_cast<std::vector<float, std::allocator<float>>*>(apThis)->~vector<float, std::allocator<float>>();
}

bool MLGetFlag_16(uint8_t aFlags) {
    return ((aFlags & 16) != 0);
}

void MLSetFlag_16(char a1, uint8_t* a2) {
    if (a1) {
        *a2 |= 0x10u;
    }
    else {
        *a2 &= (~0x10u);
    }
}

bool MLGetFlag_32(uint8_t aFlags) {
    return ((aFlags & 32) != 0);
}

void MLSetFlag_32(char a1, uint8_t* a2) {
    if (a1) {
        *a2 |= 0x20u;
    }
    else {
        *a2 &= (~0x20u);
    }
}

int32_t SetTlsValue(uint32_t aiTlsIndex, void* apValue) {
    return TlsSetValue(aiTlsIndex, apValue);
}

QueuedTree::QueuedTree(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority) : QueuedReference(apRef, aePriority) {}

void QueuedTree::QueueModels() {
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2119
    );
    TESForm* apObj = pRef->GetObjectReference();
    void* a3 = apObj;
    ENUM_LOD_MULT a7 = TES::GetLODMult(static_cast<TESObject*>(apObj));
    if ((pRef) && (pRef->GetVisibleDistant())) {
        a7 = LOD_MULT_DISTANTLOD;
    }

    if ((pRef) && (pRef->GetIsImposter())) {
        a7 = LOD_MULT_DIST_OVERRIDE;
    }

    IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
    pModelLoader->QueueTreeModel(pRef, static_cast<TESObjectTREE*>(a3), IOTaskPriority, this, a7);
}

void QueuedTree::UseDistant3D() {
    NiNode* ptempnode = static_cast<TESObjectTREE*>(pRef->GetObjectReference())->TESObjectTREE::BuildDistant3D(false);
    if (ptempnode) {
        SpeedTreeBillboardShaderProperty* pprop_2;
        int32_t v3;
        NiTriShape* pgeom = static_cast<NiTriShape*>(ptempnode->GetAt(0));
        NiTriShapeData* pgeomData = static_cast<NiTriShapeData*>(pgeom->GetModelData());
        pgeomData->SetConsistency(NiGeometryData::STATIC);
        pgeomData->m_ucKeepFlags = 17u;
        pgeomData->SetCompressFlags(1u);
        BSShaderManager::PrepareObject(pgeom, 0, 0);
        if (pgeom->GetProperty(3)) {
            SpeedTreeBillboardShaderProperty* pprop =
                static_cast<SpeedTreeBillboardShaderProperty*>(pgeom->GetProperty(3));
            v3 = pprop->iShaderPropertyType == 11;
        }
        else {
            v3 = 0;
        }
        if (v3) {
            pprop_2 = static_cast<SpeedTreeBillboardShaderProperty*>(pgeom->GetProperty(3));
        }
        else {
            pprop_2 = nullptr;
        }
        if (pprop_2) {
            pprop_2->SetFlag(13u, 1);
            pprop_2->SetFlag(22u, 1);
        }
        AttachDistant3D(ptempnode);
    }
}

QueuedActor::QueuedActor(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority) : QueuedReference(apRef, aePriority) {}

// Brings the actor to its process level, loads its textures degraded unless it is the player or in high detail,
// queues the crippled locomotion, then queues the reference and restores this thread's degraded flag.
void QueuedActor::QueueMe() {
    Actor* pActor = static_cast<Actor*>(pRef);
    pActor->ChangeProcessLevel();
    bool bLoadTexturesDegraded = pModelLoader->GetLoadTexturesDegraded();
    if (pActor != pPlayer && !pActor->CheckHighDetail()) {
        pModelLoader->SetLoadTexturesDegraded(true);
    }
    if (pActor->IsCrippled()) {
        pActor->QueueReplacementLocomotion(true, this);
    }
    QueuedReference::QueueMe();
    pModelLoader->SetLoadTexturesDegraded(bLoadTexturesDegraded);
}

// Charges the actor's model loads to its memory context, preloads its permanent magic, queues the models of its
// dismembered limbs' spawned objects, then queues the reference's own loads.
void QueuedActor::QueueModels() {
    alignas(AutoMemContext) uint8_t aMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = static_cast<AutoMemContext*>(static_cast<void*>(aMemContext));
    ::new (pMemContext)
        AutoMemContext(eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2210);

    Actor* pActor = static_cast<Actor*>(pRef);
    pActor->PreloadPermanentMagic();
    ExtraDismemberedLimbs* pLimbs = pRef->GetExtra()->GetDismembermentExtra();
    if (pLimbs) {
        IO_TASK_PRIORITY aePriority = GetIOTaskPriority();
        for (uint32_t i = 0; i < pLimbs->DismemberedLimbs.GetSize(); ++i) {
            DismemberedLimb* pLimb = pLimbs->GetDismemberedLimb(i);
            for (uint32_t j = 0; j < pLimb->ObjectArray.GetSize(); ++j) {
                TESBoundObject* pObject = pLimb->ObjectArray[j];
                pModelLoader->QueueBoundObject(pObject, aePriority, this, pRef);
            }
        }
    }

    QueuedReference::QueueModels();

    pMemContext->~AutoMemContext();
}

QueuedActor::~QueuedActor() {}

QueuedCreature::QueuedCreature(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority) : QueuedActor(apRef, aePriority) {}

void QueuedCreature::QueueModels() {
    TESModel* pmodel;
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2341
    );
    TESCreature* pcreature = static_cast<TESCreature*>(this->pRef->GetObjectReference());
    Actor* pactor = static_cast<Actor*>(this->pRef);
    bool bwearweapon = true;
    TESPackage* ppack = pactor->GetCurrentPackage();
    if ((ppack) && (ppack->GetForceUnequipWeapon())) {
        bwearweapon = false;
    }

    if (pcreature->ShouldRunInitDefaultWorn(this->pRef)) {
        pcreature->InitDefaultWorn(this->pRef, true, bwearweapon, false);
    }
    else {
        pcreature->InitWorn(this->pRef);
    }
    if (pcreature) {
        pmodel = &static_cast<TESModel&>(static_cast<TESActorBase&>(*pcreature));
    }
    else {
        pmodel = nullptr;
    }
    TESObjectREFR* pRef = this->pRef;
    IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
    pModelLoader->QueueAnimations(pmodel, IOTaskPriority, this, pRef, true, false);
    TESObjectREFR* v4 = this->pRef;
    IO_TASK_PRIORITY v2 = GetIOTaskPriority();
    pModelLoader->QueueCreatureParts(pmodel, v2, this, v4);
    QueuedActor::QueueModels();
}

QueuedCreature::~QueuedCreature() {}

QueuedCharacter::QueuedCharacter(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority)
  : QueuedActor(apRef, aePriority), spQueuedHead(nullptr), spQueuedHelmet(nullptr) {}

void QueuedCharacter::QueueModels() {
    TESModel* pmodel;
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2250
    );
    TESNPC* pnpc = static_cast<TESNPC*>(this->pRef->GetObjectReference());
    BipedAnim* pbipedAnim = this->pRef->GetBiped();
    if (!pbipedAnim) {
        pbipedAnim = pnpc->CreateBipedAnim(this->pRef);
    }
    if (pbipedAnim) {
        pnpc->InitParts(this->pRef, pbipedAnim, false);
        IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
        pbipedAnim->QueueBipedParts(IOTaskPriority, spQueuedHelmet, this);
        IO_TASK_PRIORITY v2 = GetIOTaskPriority();
        pModelLoader->QueueHead(pnpc, spQueuedHead, v2, this);
    }
    if (pnpc) {
        pmodel = &static_cast<TESModel&>(static_cast<TESActorBase&>(*pnpc));
    }
    else {
        pmodel = nullptr;
    }
    TESObjectREFR* pRef = this->pRef;
    IO_TASK_PRIORITY v3 = GetIOTaskPriority();
    pModelLoader->QueueAnimations(pmodel, v3, this, pRef, false, false);
    QueuedActor::QueueModels();
}

bool QueuedCharacter::BackgroundClone() {
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2276
    );
    bool v6 = QueuedReference::BackgroundClone();
    if ((v6) && (GetCloned3D())) {
        NiAVObject* VATSLight = GetCloned3D();
        TESObjectREFR::SetBackgroundLoadingRef(pRef, VATSLight);
        AttachHeadParts();
        TESObjectREFR::ClearBackgroundLoadingRef();
    }

    return v6;
}

void QueuedCharacter::Attach() {
    QueuedReference::Attach();
}

void QueuedCharacter::FinishAttach() {
    AutoMemContext tempMemContext(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2303
    );
    if (!GetCloned3D()) {
        AttachHeadParts();
    }
}

QueuedCharacter::~QueuedCharacter() {}

void QueuedCharacter::AttachHeadParts() {
    QueuedHead* v1;
    QueuedHelmet* v2;
    TESObjectREFR* pRefr;
    BipedAnim* biped = static_cast<Character*>(pRef)->Character::GetBiped();
    if (spQueuedHead.m_pObject) {
        pRefr = pRef;
        v1 = spQueuedHead.m_pObject;
        v1->Attach(pRefr, biped);
    }
    if (spQueuedHelmet.m_pObject) {
        v2 = spQueuedHelmet.m_pObject;
        v2->Attach();
    }
}

// The player's reference is queued.
QueuedPlayer::QueuedPlayer(IO_TASK_PRIORITY aePriority) : QueuedCharacter(pPlayer, aePriority) {}

void QueuedPlayer::QueueModels() {
    AutoMemContext apMemory(
        eContext, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 2382
    );
    IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
    pPlayer->QueuePlayerAnims(IOTaskPriority, this);
    QueuedCharacter::QueueModels();
}

void QueuedPlayer::Attach() {
    QueuedCharacter::Attach();
}

QueuedPlayer::~QueuedPlayer() {}

BackgroundCloneThread::BackgroundCloneThread(uint32_t aiMaxThreads)
  : BSTaskThread(3u, "BackgroundCloneThread"), bExit(false), iRunningCount(0),
    pCloneReferencesQueue(new LockFreeQueue<NiPointer<QueuedReference>>(aiMaxThreads, 8u)) {}

BackgroundCloneThread::~BackgroundCloneThread() {
    if (pCloneReferencesQueue) {
        delete pCloneReferencesQueue;
    }
}

void BackgroundCloneThread::ThreadUpdate() {
    while (!bExit) {
        WaitTask();
        NiAtomicIncrement(iRunningCount);
        NiPointer<QueuedReference> spqueuedReference(static_cast<QueuedReference*>(nullptr));
        pCloneReferencesQueue->Pop(spqueuedReference);
        if (spqueuedReference.m_pObject) {
            while (spqueuedReference.m_pObject) {
                WaitPauseSemaphore();
                InitializationFlow();
                QueuedReference* pqueuedRef = spqueuedReference.m_pObject;
                BS_TASK_STATE estate = pqueuedRef->eState;
                if (!pqueuedRef->IsCancelled() && pqueuedRef->SwitchState(estate, BS_TASK_STATE_RUNNING)) {
                    if (pqueuedRef->BackgroundClone()) {
                        pqueuedRef->AddToPostProcessQueue();
                    }
                    pqueuedRef->SwitchState(BS_TASK_STATE_RUNNING, BS_TASK_STATE_COMPLETED);
                }
                pCloneReferencesQueue->Pop(spqueuedReference);
                InitializationFlow();
                SignalPauseSemaphore();
            }
        }
        else {
            WaitPauseSemaphore();
            SignalPauseSemaphore();
        }
        NiAtomicDecrement(iRunningCount);
    }
}

void BackgroundCloneThread::AddQueuedReference(QueuedReference* apQueuedReference) {
    pCloneReferencesQueue->Push(NiPointer<QueuedReference>(apQueuedReference));
    SignalTask();
}

uint32_t BackgroundCloneThread::GetQueuedCount() {
    return iRunningCount + pCloneReferencesQueue->GetCount();
}

void BackgroundCloneThread::Pause() {
    uint32_t formFlags = iThreadID;
    if (formFlags != BSGetCurrentThreadID()) {
        BSTaskThread::Sleep();
    }
}

void BackgroundCloneThread::Resume() {
    int32_t flags = (int32_t)iThreadID;
    if (((uint32_t)flags) != BSGetCurrentThreadID()) {
        BSTaskThread::Resume();
    }
}

void BackgroundCloneThread::SetExiting() {
    bExit = true;
}

// Builds the maps and queues of what is loaded or queued, starts the clone thread and hooks the IO manager's callbacks.
ModelLoader::ModelLoader()
  : pModelMap(new LockFreeCaseInsensitiveStringMap<Model*>(10u, 1009u, 12u)),
    pKFModelMap(new LockFreeCaseInsensitiveStringMap<KFModel*>(10u, 1009u, 12u)),
    pQueuedReferencesMap(new LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>(10u, 5039u, 12u)),
    pReferencesToQueueMap(new LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>(10u, 5039u, 12u)),
    pQueuedAnimIdleMap(new LockFreeMap<AnimIdle*, NiPointer<QueuedAnimIdle>>(10u, 251u, 12u)),
    pQueuedReplacementKFListMap(new LockFreeMap<Animation*, NiPointer<QueuedReplacementKFList>>(10u, 251u, 12u)),
    pQueuedHelmetMap(new LockFreeMap<TESObjectREFR*, NiPointer<QueuedHelmet>>(10u, 251u, 12u)),
    pAttachDistant3DTaskQueue(new LockFreeQueue<NiPointer<AttachDistant3DTask>>(10u, 8u)),
    pQueuedTextureMap(new LockFreeMap<BSFileEntry*, NiPointer<QueuedTexture>>(10u, 1009u, 12u)),
    pLoadedFileMap(new LockFreeCaseInsensitiveStringMap<LoadedFile*>(10u, 1009u, 12u)),
    pBackgroundCloneThread(new BackgroundCloneThread(10u)) {
    pBackgroundCloneThread->StartThread();
    bHasDelayedFree = false;
    if (pIOManager) {
        pIOManager->pUpdateQueueCallback = &ModelLoader::UpdateQueueCallback;
        pIOManager->pUpdateSecondaryQueueCallback = &ModelLoader::UpdateSecondaryQueueCallback;
        pIOManager->pLoadQueuedPriorityCallback = &ModelLoader::LoadQueuedPriorityCallback;
        pIOManager->pGetExternalCountCallback = &ModelLoader::GetExternalCountCallback;
        pIOManager->pPauseCallback = &ModelLoader::PauseCallback;
        pIOManager->pResumeCallback = &ModelLoader::ResumeCallback;
    }
}

// Frees every model left in the model map (warning about those still referenced), the KF models without an anim
// group or of a power attack, then every KF model left (warning about those still referenced), then the maps,
// queues and the clone thread.
ModelLoader::~ModelLoader() {
    OutputModelMapContents(nullptr, false);
    if (pModelMap->GetCount()) {
        uint32_t ibadCount = 0;
        LockFreeStringMap<Model*>::LockFreeStringMapIterator kIterator;
        while (!kIterator.GetFinished()) {
            const char* pfileName = nullptr;
            Model* pmodel = nullptr;
            if (pModelMap->GetNext(kIterator, pfileName, pmodel, true)) {
                if (pmodel->GetRefCount() > 0) {
                    ++ibadCount;
                }
                if (pmodel) {
                    delete pmodel;
                }
            }
        }
        if (ibadCount) {
            Warning("MODELS: ModelLoader still contains %d NIF files.\r\n", ibadCount);
        }
    }
    LockFreeStringMap<KFModel*>::LockFreeStringMapIterator kKFIterator;
    while (!kKFIterator.GetFinished()) {
        const char* pfileName = nullptr;
        KFModel* pkfModel = nullptr;
        if ((pKFModelMap->GetNext(kKFIterator, pfileName, pkfModel, true)) && (pkfModel) &&
            (!pkfModel->GetAnimGroup() || pkfModel->GetAnimGroup()->IsPowerAttack())) {
            pKFModelMap->RemoveAt(pfileName);
            if (pkfModel) {
                delete pkfModel;
            }
        }
    }
    if (pKFModelMap->GetCount()) {
        uint32_t ibadCount = 0;
        LockFreeStringMap<KFModel*>::LockFreeStringMapIterator kIterator;
        while (!kIterator.GetFinished()) {
            const char* pfileName = nullptr;
            KFModel* pkfModel = nullptr;
            if (pKFModelMap->GetNext(kIterator, pfileName, pkfModel, true)) {
                if (pkfModel->GetRefCount() > 0) {
                    ++ibadCount;
                }
                if (pkfModel) {
                    delete pkfModel;
                }
            }
        }
        if (ibadCount) {
            Warning("ANIMATION: ModelLoader still has %d KF files.\r\n", ibadCount);
        }
    }
    delete pModelMap;
    delete pKFModelMap;
    delete pAttachDistant3DTaskQueue;
    delete pQueuedHelmetMap;
    delete pQueuedReplacementKFListMap;
    delete pQueuedAnimIdleMap;
    delete pQueuedReferencesMap;
    delete pReferencesToQueueMap;
    delete pBackgroundCloneThread;
    delete pQueuedTextureMap;
    delete pLoadedFileMap;
}

void ModelLoader::QueueTreeModel(
    TESObjectREFR* apRef,
    TESObjectTREE* apTree,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apCounter,
    ENUM_LOD_MULT aeLODMult
) {
    NiPointer<QueuedModel> spqueuedModel(static_cast<QueuedModel*>(nullptr));
    QueueTreeModel(apRef, apTree, spqueuedModel, aePriority, apCounter, aeLODMult);
}

void ModelLoader::QueueTreeModel(
    TESObjectREFR* apRef,
    TESObjectTREE* apTree,
    NiPointer<QueuedModel>& aspQueuedModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apCounter,
    ENUM_LOD_MULT aeLODMult
) {
    QueuedTreeModel* v8;
    QueuedTreeModel* v9 = static_cast<QueuedTreeModel*>(ourMalloc(sizeof(QueuedTreeModel)));
    if (v9) {
        v8 = ::new (v9) QueuedTreeModel(apRef, apTree, aePriority, aeLODMult);
    }
    else {
        v8 = nullptr;
    }
    NiPointer<QueuedModel> spqueuedTreeModel(v8);
    spqueuedTreeModel.m_pObject->SetParent(apCounter);
    QueuedModel* v7 = spqueuedTreeModel.m_pObject;
    v7->QueueMe();
    NiPointer<QueuedModel>* p_spqueuedTreeModel = &spqueuedTreeModel;
    aspQueuedModel.operator=(spqueuedTreeModel);
}

void ModelLoader::QueueBoundObject(
    TESBoundObject* apObj,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    TESObjectREFR* apRefr
) {
    NiPointer<QueuedModel> spqueuedModel(static_cast<QueuedModel*>(nullptr));
    QueueBoundObject(apObj, spqueuedModel, aePriority, apParent, apRefr);
}

void ModelLoader::RemoveReplacementKFListFromMap(Animation* apAnim) {
    pQueuedReplacementKFListMap->RemoveAt(apAnim);
}

void ModelLoader::ReleaseKF(const char* apKFFileName, bool abDeleteIfUnreferenced) {
    KFModel* v4 = nullptr;
    if (pKFModelMap->GetAt(apKFFileName, v4)) {
        v4->DecUseCount();
        if ((pSaveLoadGame) && (pSaveLoadGame->GetSaveGameLoading())) {
            abDeleteIfUnreferenced = false;
            bHasDelayedFree = true;
        }

        if (abDeleteIfUnreferenced && !v4->GetRefCount()) {
            pKFModelMap->RemoveAt(apKFFileName);
            if (v4) {
                delete v4;
            }
        }
    }
}

bool ModelLoader::GetLoadTexturesDegraded() {
    return GetThreadLoadTexturesDegraded();
}

// Sets this thread's flag; returns the value it had before.
bool ModelLoader::SetLoadTexturesDegraded(bool abValue) {
    bool bOldValue = GetThreadLoadTexturesDegraded();
    GetThreadLoadTexturesDegraded() = abValue;
    return bOldValue;
}

void ModelLoader::RemoveAnimIdleFromMap(AnimIdle* apAnimIdle) {
    pQueuedAnimIdleMap->RemoveAt(apAnimIdle);
}

void ModelLoader::RemoveHelmetFromMap(TESObjectREFR* apRef) {
    pQueuedHelmetMap->RemoveAt(apRef);
}

void ModelLoader::AddBackgroundCloneReference(QueuedReference* apQueuedReference) {
    pBackgroundCloneThread->AddQueuedReference(apQueuedReference);
}

void ModelLoader::RemoveReferenceFromMap(TESObjectREFR* apRef) {
    pQueuedReferencesMap->RemoveAt(apRef);
}

// Only a load that is not running yet is reused: it is switched to the moving state while it is changed, then back to
// its old state; a load whose priority is lower than aePriority is requeued at aePriority, otherwise the IO threads are
// woken.
bool ModelLoader::ReuseQueuedTexture(
    BSFileEntry* apTextureFileEntry,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent
) {
    NiPointer<QueuedTexture> spQueuedTexture = GetQueuedTextureFromMap(apTextureFileEntry);
    if (spQueuedTexture.m_pObject) {
        BS_TASK_STATE eState = spQueuedTexture->eState;
        if (eState < BS_TASK_STATE_RUNNING && spQueuedTexture->SwitchState(eState, BS_TASK_STATE_MOVING)) {
            if (spQueuedTexture->GetLoadDegraded() && !GetLoadTexturesDegraded()) {
                spQueuedTexture->SetLoadDegraded(false);
            }
            if (apParent) {
                spQueuedTexture->AddAdditionalParent(apParent);
            }
            if (spQueuedTexture->SwitchState(BS_TASK_STATE_MOVING, eState)) {
                if (spQueuedTexture->GetIOTaskPriority() > aePriority) {
                    spQueuedTexture->Requeue(aePriority);
                }
                else {
                    pIOManager->ForceSignal();
                }
                return true;
            }
        }
    }
    return false;
}

// A texture already in the palette is queued again only when it was loaded degraded and this thread no longer loads
// degraded textures. Otherwise, with a parent, a task holding the texture is created and finished at once. A texture
// not in the palette is looked up in the texture archives by its standard path; a queued load of the same entry is
// reused.
bool ModelLoader::QueueTexture(const char* apTextureFileName, IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    NiPointer<NiTexture> spTexture;
    BSTexturePalette::GetTexture(apTextureFileName, spTexture);
    if (spTexture.m_pObject) {
        bool bReload = false;
        if (spTexture->QState() == PS_DEGRADED && !GetLoadTexturesDegraded()) {
            bReload = true;
        }
        if (!bReload) {
            if (!apParent) {
                return false;
            }
            void* pMemory = ourMalloc(sizeof(QueuedTexture));
            QueuedTexture* pQueuedTexture =
                pMemory ? ::new (pMemory) QueuedTexture(spTexture.m_pObject, aePriority) : nullptr;
            NiPointer<QueuedTexture> spQueuedTexture(pQueuedTexture);
            spQueuedTexture->SetParent(apParent);
            spQueuedTexture->CheckFinished();
            return true;
        }
    }

    char acPath[0x104];
    StandardizeTexturePath(apTextureFileName, acPath, sizeof(acPath));
    BSHash kDirHash;
    BSHash kFileHash;
    BSHash::MakeDirAndFile(acPath, kDirHash, kFileHash);
    BSFileEntry* pFileEntry =
        ArchiveManager::GetFileEntryForFileFromAllArchives(ARCHIVE_TYPE_INDEX_TEXTURES, kDirHash, kFileHash, acPath);
    if (pFileEntry && ReuseQueuedTexture(pFileEntry, aePriority, apParent)) {
        return true;
    }

    NiPointer<QueuedTexture> spQueuedTexture;
    if (pFileEntry) {
        void* pMemory = ourMalloc(sizeof(QueuedTexture));
        spQueuedTexture = pMemory ? ::new (pMemory) QueuedTexture(pFileEntry, aePriority) : nullptr;
        spQueuedTexture->SetFileName(acPath);
    }
    else {
        void* pMemory = ourMalloc(sizeof(QueuedTexture));
        spQueuedTexture = pMemory ? ::new (pMemory) QueuedTexture(apTextureFileName, aePriority) : nullptr;
    }
    spQueuedTexture->SetParent(apParent);
    spQueuedTexture->QueueMe();
    return true;
}

// As the path overload, with the archive entry given.
bool ModelLoader::QueueTexture(BSFileEntry* apTextureFileEntry, IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    NiPointer<NiTexture> spTexture;
    BSTexturePalette::GetTexture(apTextureFileEntry, spTexture);
    if (spTexture.m_pObject) {
        bool bReload = false;
        if (spTexture->QState() == PS_DEGRADED && !GetLoadTexturesDegraded()) {
            bReload = true;
        }
        if (!bReload) {
            if (!apParent) {
                return false;
            }
            void* pMemory = ourMalloc(sizeof(QueuedTexture));
            QueuedTexture* pQueuedTexture =
                pMemory ? ::new (pMemory) QueuedTexture(spTexture.m_pObject, aePriority) : nullptr;
            NiPointer<QueuedTexture> spQueuedTexture(pQueuedTexture);
            spQueuedTexture->SetParent(apParent);
            spQueuedTexture->CheckFinished();
            return true;
        }
    }

    if (ReuseQueuedTexture(apTextureFileEntry, aePriority, apParent)) {
        return true;
    }
    void* pMemory = ourMalloc(sizeof(QueuedTexture));
    QueuedTexture* pQueuedTexture = pMemory ? ::new (pMemory) QueuedTexture(apTextureFileEntry, aePriority) : nullptr;
    NiPointer<QueuedTexture> spQueuedTexture(pQueuedTexture);
    spQueuedTexture->SetParent(apParent);
    spQueuedTexture->QueueMe();
    return true;
}

void ModelLoader::QueueModel(
    TESModel* apModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool abAddRefCount,
    bool abFaceGenModel
) {
    NiPointer<QueuedModel> spQueuedModel;
    QueueModel(
        apModel, spQueuedModel, aePriority, apParent, aeLODFadeMult, abPrepareAfterLoad, abAddRefCount, abFaceGenModel,
        0.0f
    );
}

// A model already loaded gets a task that holds it (and a manual reference when abAddRefCount), finished at once, but
// only for a parent; without one aspQueuedModel is cleared.
void ModelLoader::QueueModel(
    TESModel* apModel,
    NiPointer<QueuedModel>& aspQueuedModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool abAddRefCount,
    bool abFaceGenModel,
    float afOverriddenVisualDistance
) {
    Model* pModel = nullptr;
    if (pModelMap->GetAt(apModel->GetModel(), pModel)) {
        if (abAddRefCount) {
            pModel->IncManualRefCount();
        }
        if (apParent) {
            void* pMemory = ourMalloc(sizeof(QueuedModel));
            QueuedModel* pQueuedModel = pMemory ? ::new (pMemory) QueuedModel(pModel, aePriority) : nullptr;
            NiPointer<QueuedModel> spQueuedModel(pQueuedModel);
            spQueuedModel->SetOverriddenVisualDistance(afOverriddenVisualDistance);
            spQueuedModel->SetParent(apParent);
            spQueuedModel->CheckFinished();
            aspQueuedModel = spQueuedModel;
        }
        else {
            aspQueuedModel = static_cast<QueuedModel*>(nullptr);
        }
    }
    else {
        void* pMemory = ourMalloc(sizeof(QueuedModel));
        QueuedModel* pQueuedModel = pMemory
            ? ::new (pMemory) QueuedModel(apModel, aePriority, aeLODFadeMult, abPrepareAfterLoad, abAddRefCount)
            : nullptr;
        NiPointer<QueuedModel> spQueuedModel(pQueuedModel);
        spQueuedModel->SetOverriddenVisualDistance(afOverriddenVisualDistance);
        spQueuedModel->SetParent(apParent);
        spQueuedModel->SetFaceGenModel(abFaceGenModel);
        spQueuedModel->QueueMe();
        aspQueuedModel = spQueuedModel;
    }
}

void ModelLoader::QueueModel(
    const char* apModelFileName,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool abAddRefCount,
    bool abFaceGenModel
) {
    NiPointer<QueuedModel> spQueuedModel;
    QueueModel(
        apModelFileName, spQueuedModel, aePriority, apParent, aeLODFadeMult, abPrepareAfterLoad, abAddRefCount,
        abFaceGenModel
    );
}

// As the TESModel overload, by file name and without an overridden visual distance.
void ModelLoader::QueueModel(
    const char* apModelFileName,
    NiPointer<QueuedModel>& aspQueuedModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool abAddRefCount,
    bool abFaceGenModel
) {
    Model* pModel = nullptr;
    if (pModelMap->GetAt(apModelFileName, pModel)) {
        if (abAddRefCount) {
            pModel->IncManualRefCount();
        }
        if (apParent) {
            void* pMemory = ourMalloc(sizeof(QueuedModel));
            QueuedModel* pQueuedModel = pMemory ? ::new (pMemory) QueuedModel(pModel, aePriority) : nullptr;
            NiPointer<QueuedModel> spQueuedModel(pQueuedModel);
            spQueuedModel->SetParent(apParent);
            spQueuedModel->CheckFinished();
            aspQueuedModel = spQueuedModel;
        }
        else {
            aspQueuedModel = static_cast<QueuedModel*>(nullptr);
        }
    }
    else {
        void* pMemory = ourMalloc(sizeof(QueuedModel));
        QueuedModel* pQueuedModel = pMemory
            ? ::new (pMemory) QueuedModel(apModelFileName, aePriority, aeLODFadeMult, abPrepareAfterLoad, abAddRefCount)
            : nullptr;
        NiPointer<QueuedModel> spQueuedModel(pQueuedModel);
        spQueuedModel->SetFaceGenModel(abFaceGenModel);
        spQueuedModel->SetParent(apParent);
        spQueuedModel->QueueMe();
        aspQueuedModel = spQueuedModel;
    }
}

void ModelLoader::QueueModel(
    const char* apModelFileName,
    TESTextureList* apTextureList,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool abAddRefCount,
    bool abFaceGenModel
) {
    NiPointer<QueuedModel> spQueuedModel;
    QueueModel(
        apModelFileName, apTextureList, spQueuedModel, aePriority, apParent, aeLODFadeMult, abPrepareAfterLoad,
        abAddRefCount, abFaceGenModel
    );
}

// As the file name overload; a new load is queued with the alternate textures apTextureList.
void ModelLoader::QueueModel(
    const char* apModelFileName,
    TESTextureList* apTextureList,
    NiPointer<QueuedModel>& aspQueuedModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareAfterLoad,
    bool abAddRefCount,
    bool abFaceGenModel
) {
    Model* pModel = nullptr;
    if (pModelMap->GetAt(apModelFileName, pModel)) {
        if (abAddRefCount) {
            pModel->IncManualRefCount();
        }
        if (apParent) {
            void* pMemory = ourMalloc(sizeof(QueuedModel));
            QueuedModel* pQueuedModel = pMemory ? ::new (pMemory) QueuedModel(pModel, aePriority) : nullptr;
            NiPointer<QueuedModel> spQueuedModel(pQueuedModel);
            spQueuedModel->SetParent(apParent);
            spQueuedModel->CheckFinished();
            aspQueuedModel = spQueuedModel;
        }
        else {
            aspQueuedModel = static_cast<QueuedModel*>(nullptr);
        }
    }
    else {
        void* pMemory = ourMalloc(sizeof(QueuedModel));
        QueuedModel* pQueuedModel = pMemory
            ? ::new (pMemory) QueuedModel(apModelFileName, aePriority, aeLODFadeMult, abPrepareAfterLoad, abAddRefCount)
            : nullptr;
        NiPointer<QueuedModel> spQueuedModel(pQueuedModel);
        spQueuedModel->SetFaceGenModel(abFaceGenModel);
        spQueuedModel->SetParent(apParent);
        spQueuedModel->QueueMe(apTextureList);
        aspQueuedModel = spQueuedModel;
    }
}

// An animation already loaded gets a task that holds it, finished at once, but only for a parent.
bool ModelLoader::QueueKF(const char* apKFFileName, IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    KFModel* pKFModel = nullptr;
    if (pKFModelMap->GetAt(apKFFileName, pKFModel)) {
        if (!apParent) {
            return false;
        }
        void* pMemory = ourMalloc(sizeof(QueuedKF));
        QueuedKF* pQueuedKF = pMemory ? ::new (pMemory) QueuedKF(pKFModel, aePriority) : nullptr;
        NiPointer<QueuedKF> spQueuedKF(pQueuedKF);
        spQueuedKF->SetParent(apParent);
        spQueuedKF->CheckFinished();
        return true;
    }
    void* pMemory = ourMalloc(sizeof(QueuedKF));
    QueuedKF* pQueuedKF = pMemory ? ::new (pMemory) QueuedKF(apKFFileName, aePriority) : nullptr;
    NiPointer<QueuedKF> spQueuedKF(pQueuedKF);
    spQueuedKF->SetParent(apParent);
    spQueuedKF->QueueMe();
    return true;
}

// Nothing is queued while a save loads (unless its forms are being initialised), for sounds, or for projectiles without
// 3D. A reference already queued is requeued at aePriority. A new load is entered in the queued map; on the main thread
// outside menu mode (when allowed) it waits in the to-queue map, otherwise it is queued at once. The outer
// AutoMemContext ends before the reference pointer is released.
void ModelLoader::QueueReference(TESObjectREFR* apRef, IO_TASK_PRIORITY aePriority, bool abAllowQueueReferenceQueuing) {
    if (pSaveLoadGame->GetSaveGameLoading() && !pSaveLoadGame->GetInitingForms()) {
        return;
    }
    if (apRef->GetObjectReference()->GetFormType() == SOUN_ID) {
        return;
    }
    if (apRef->IsProjectile() && static_cast<Projectile*>(apRef)->IsFlagSet(Projectile::NO_3D)) {
        return;
    }

    NiPointer<QueuedReference> spQueuedReference;
    ::new (&spQueuedReference) NiPointer<QueuedFile>(nullptr);
    if (pQueuedReferencesMap->GetAt(apRef, spQueuedReference)) {
        if (spQueuedReference->GetIOTaskPriority() != aePriority) {
            spQueuedReference->Requeue(aePriority);
        }
        return;
    }

    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_TES_REFERENCES, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp",
        3150
    );
    if (apRef == pPlayer) {
        alignas(AutoMemContext) uint8_t aucPlayerContext[sizeof(AutoMemContext)];
        AutoMemContext* pPlayerContext = reinterpret_cast<AutoMemContext*>(aucPlayerContext);
        ::new (aucPlayerContext) AutoMemContext(
            MEM_CONTEXT_PLAYER, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 3155
        );
        void* pMemory = ourMalloc(sizeof(QueuedPlayer));
        spQueuedReference = pMemory ? ::new (pMemory) QueuedPlayer(aePriority) : nullptr;
        pPlayerContext->~AutoMemContext();
    }
    else {
        switch (apRef->GetObjectReference()->GetFormType()) {
            case TREE_ID: {
                void* pMemory = ourMalloc(sizeof(QueuedTree));
                spQueuedReference = pMemory ? ::new (pMemory) QueuedTree(apRef, aePriority) : nullptr;
                break;
            }
            case NPC__ID: {
                alignas(AutoMemContext) uint8_t aucActorContext[sizeof(AutoMemContext)];
                AutoMemContext* pActorContext = reinterpret_cast<AutoMemContext*>(aucActorContext);
                ::new (aucActorContext) AutoMemContext(
                    MEM_CONTEXT_TES_ACTOR, true,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 3179
                );
                void* pMemory = ourMalloc(sizeof(QueuedCharacter));
                spQueuedReference = pMemory ? ::new (pMemory) QueuedCharacter(apRef, aePriority) : nullptr;
                pActorContext->~AutoMemContext();
                break;
            }
            case CREA_ID: {
                alignas(AutoMemContext) uint8_t aucActorContext[sizeof(AutoMemContext)];
                AutoMemContext* pActorContext = reinterpret_cast<AutoMemContext*>(aucActorContext);
                ::new (aucActorContext) AutoMemContext(
                    MEM_CONTEXT_TES_ACTOR, true,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 3186
                );
                void* pMemory = ourMalloc(sizeof(QueuedCreature));
                spQueuedReference = pMemory ? ::new (pMemory) QueuedCreature(apRef, aePriority) : nullptr;
                pActorContext->~AutoMemContext();
                break;
            }
            default: {
                void* pMemory = ourMalloc(sizeof(QueuedReference));
                spQueuedReference = pMemory ? ::new (pMemory) QueuedReference(apRef, aePriority) : nullptr;
                break;
            }
        }
    }

    if (pQueuedReferencesMap->SetAt(apRef, spQueuedReference, false)) {
        bool bDelayQueue = false;
        if (abAllowQueueReferenceQueuing) {
            uint32_t uiThreadID = BSGetCurrentThreadID();
            if (uiThreadID == appMain->GetThread() && !Interface::IsInMenuMode()) {
                bDelayQueue = true;
            }
        }
        if (bDelayQueue) {
            pReferencesToQueueMap->SetAt(apRef, spQueuedReference, true);
        }
        else {
            spQueuedReference->QueueMe();
        }
        if (apRef->GetParentCell()) {
            apRef->GetParentCell()->IncQueuedRefCount();
            if (apRef->Is3DCritical()) {
                apRef->GetParentCell()->IncCriticalQueuedRefCount();
            }
        }
    }
    else {
        spQueuedReference = static_cast<QueuedReference*>(nullptr);
    }
    pContext->~AutoMemContext();
}

// The model is queued through the global model loader, prepared after loading and without a manual reference; a
// reference that must be visible from a distance, or a marker, fades as distant LOD, and an imposter's model gets the
// largest visual distance.
void ModelLoader::QueueBoundObject(
    TESBoundObject* apBoundObject,
    NiPointer<QueuedModel>& aspQueuedModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    TESObjectREFR* apQueuedForRef
) {
    TESModel* pModel = GetModelForBoundObject(apBoundObject, apQueuedForRef);
    ENUM_LOD_MULT eLODMult = TES::GetLODMult(apBoundObject);
    if (apQueuedForRef && (apQueuedForRef->GetVisibleDistant() || apQueuedForRef->IsMarker())) {
        eLODMult = LOD_MULT_DISTANTLOD;
    }
    NiPointer<QueuedModel> spQueuedModel;
    float fOverriddenVisualDistance = 0.0f;
    if (apQueuedForRef && apQueuedForRef->GetIsImposter()) {
        fOverriddenVisualDistance = FLT_MAX;
    }
    pModelLoader->QueueModel(
        pModel, spQueuedModel, aePriority, apParent, eLODMult, true, false, false, fOverriddenVisualDistance
    );
    aspQueuedModel = spQueuedModel;
}

void ModelLoader::QueueHead(
    TESNPC* apNPC,
    NiPointer<QueuedHead>& aspQueuedHead,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent
) {
    void* pMemory = ourMalloc(sizeof(QueuedHead));
    QueuedHead* pQueuedHead = pMemory ? ::new (pMemory) QueuedHead(apNPC, aePriority) : nullptr;
    NiPointer<QueuedHead> spQueuedHead(pQueuedHead);
    spQueuedHead->SetParent(apParent);
    spQueuedHead->QueueMe();
    aspQueuedHead = spQueuedHead;
}

// The reference's own model when apBoundObject is its base object; else a weapon's model, the form's model, or a biped
// form's world model for the sex of the reference's owner (when that is an NPC).
TESModel* ModelLoader::GetModelForBoundObject(TESBoundObject* apBoundObject, TESObjectREFR* apQueuedForRef) {
    if (apQueuedForRef && apQueuedForRef->GetObjectReference() == apBoundObject) {
        return apQueuedForRef->GetTESModel();
    }
    TESModel* pModel = nullptr;
    if (apBoundObject && apBoundObject->GetFormType() == WEAP_ID) {
        pModel = static_cast<TESObjectWEAP*>(apBoundObject)->GetWorldTESModel();
    }
    else {
        pModel = dynamic_cast<TESModel*>(apBoundObject);
        if (!pModel) {
            TESBipedModelForm* pBipedModelForm = dynamic_cast<TESBipedModelForm*>(apBoundObject);
            if (pBipedModelForm) {
                int32_t iSex = TESActorBase::SEX_MALE;
                if (apQueuedForRef) {
                    TESForm* pOwner = apQueuedForRef->GetOwner();
                    if (pOwner && pOwner->GetFormType() == NPC__ID) {
                        iSex = static_cast<TESActorBase*>(pOwner)->GetSex();
                    }
                }
                pModel = pBipedModelForm->GetWorldTESModel(static_cast<TESActorBase::SEX>(iSex));
            }
        }
    }
    return pModel;
}

// The pointer is read before the local reference is released.
QueuedTexture* ModelLoader::GetQueuedTextureFromMap(BSFileEntry* apFileEntry) {
    NiPointer<QueuedTexture> spQueuedTexture;
    if (pQueuedTextureMap->GetAt(apFileEntry, spQueuedTexture)) {
        return spQueuedTexture.m_pObject;
    }
    return nullptr;
}

// A helmet load already queued for the actor is handed out as it is; a new one that cannot be added to the map (another
// thread added one first) is dropped.
void ModelLoader::QueueHelmet(
    BipedAnim* apBipedAnim,
    NiPointer<QueuedHelmet>& aspQueuedHelmet,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    BIPED_OBJECT aeBipedObject
) {
    NiPointer<QueuedHelmet> spQueuedHelmet;
    if (!pQueuedHelmetMap->GetAt(apBipedAnim->GetRequester(), spQueuedHelmet)) {
        void* pMemory = ourMalloc(sizeof(QueuedHelmet));
        QueuedHelmet* pQueuedHelmet =
            pMemory ? ::new (pMemory) QueuedHelmet(apBipedAnim, aePriority, aeBipedObject) : nullptr;
        spQueuedHelmet = pQueuedHelmet;
        if (pQueuedHelmetMap->SetAt(apBipedAnim->GetRequester(), spQueuedHelmet, false)) {
            spQueuedHelmet->SetParent(apParent);
            spQueuedHelmet->QueueMe();
        }
        else {
            spQueuedHelmet = static_cast<QueuedHelmet*>(nullptr);
        }
    }
    aspQueuedHelmet = spQueuedHelmet;
}

// The load is queued only when it could be added to the map; nothing is returned.
KFModel* ModelLoader::QueueAnimIdle(
    const char* apFilename,
    AnimIdle* apAnimIdle,
    IO_TASK_PRIORITY aePriority,
    TESObjectREFR* apQueuedForRef
) {
    void* pMemory = ourMalloc(sizeof(QueuedAnimIdle));
    QueuedAnimIdle* pQueuedAnimIdle =
        pMemory ? ::new (pMemory) QueuedAnimIdle(apFilename, aePriority, apAnimIdle, apQueuedForRef) : nullptr;
    NiPointer<QueuedAnimIdle> spQueuedAnimIdle(pQueuedAnimIdle);
    if (pQueuedAnimIdleMap->SetAt(apAnimIdle, spQueuedAnimIdle, false)) {
        spQueuedAnimIdle->QueueMe();
    }
    else {
        spQueuedAnimIdle = static_cast<QueuedAnimIdle*>(nullptr);
    }
    return nullptr;
}

// Any count other than one goes through ModManualRefCount with its low 16 bits, sign-extended and negated.
void ModelLoader::ReleaseModel(const char* apModelFileName, bool abDeleteIfUnreferenced, uint32_t aiNumDecrements) {
    Model* pModel = nullptr;
    if (pModelMap->GetAt(apModelFileName, pModel)) {
        if (aiNumDecrements == 1) {
            pModel->DecManualRefCount();
        }
        else {
            pModel->ModManualRefCount(-static_cast<int16_t>(aiNumDecrements));
        }
        if (abDeleteIfUnreferenced) {
            TryAndRemoveModel(pModel, apModelFileName);
        }
    }
}

void ModelLoader::CancelReference(TESObjectREFR* apRef) {
    NiPointer<QueuedReference> spQueuedReference;
    if (pQueuedReferencesMap->GetAt(apRef, spQueuedReference)) {
        pIOManager->CancelTask(spQueuedReference.m_pObject, nullptr);
        pReferencesToQueueMap->RemoveAt(apRef);
    }
    NiPointer<QueuedHelmet> spQueuedHelmet;
    if (pQueuedHelmetMap->GetAt(apRef, spQueuedHelmet)) {
        pIOManager->CancelTask(spQueuedHelmet.m_pObject, nullptr);
    }
}

bool ModelLoader::IsReferenceQueued(TESObjectREFR* apRef) {
    NiPointer<QueuedReference> spQueuedReference;
    return pQueuedReferencesMap->GetAt(apRef, spQueuedReference);
}

// apQueuedForRef's IsNpc and IsChild are called without a null check when the model is not a creature skeleton, as the
// game does.
void ModelLoader::QueueAnimations(
    TESModel* apModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    TESObjectREFR* apQueuedForRef,
    bool abCreature,
    bool abForceAnimationLoad
) {
    char cPath[0x104];
    BSstrcpy(cPath, 0x104, apModel->GetModel());
    char* pSlash = std::strrchr(cPath, '\\');
    if (apQueuedForRef && apQueuedForRef->IsDead(true)) {
        if (pSlash && !_strnicmp(pSlash + 1, "Skeleton", 8)) {
            char cFileName[0x104];
            BSstrcpy(cFileName, 0x104, "Data\\");
            if (_strnicmp(cPath, "Meshes\\", 7)) {
                BSstrcat(cFileName, 0x104, "Meshes\\");
            }
            BSstrcat(cFileName, 0x104, cPath);
            char* pFileSlash = std::strrchr(cFileName, '\\');
            BSstrcpy(pFileSlash, 0x104 - (pFileSlash - cFileName), "\\MTIdle.KF");
            BSSimpleList<const char*>* pList = BuildFileList(cFileName, cPath, nullptr);
            QueueKFList(pList, aePriority, apParent, nullptr);
            if (pList) {
                delete pList;
            }
            Actor* pActor = apQueuedForRef->IsActor() ? static_cast<Actor*>(apQueuedForRef) : nullptr;
            if (pActor && pActor->GetCurrentWeapon()) {
                const char* pWeaponName =
                    animWeaponInfo[weaponToAnimGroupWeapon[pActor->GetCurrentWeapon()->GetWeaponType()]];
                if (pWeaponName && *pWeaponName) {
                    std::sprintf(pFileSlash, "\\%sHolster.KF", pWeaponName);
                    pList = BuildFileList(cFileName, cPath, nullptr);
                    QueueKFList(pList, aePriority, apParent, nullptr);
                    if (pList) {
                        delete pList;
                    }
                    std::sprintf(pFileSlash, "\\PA%sHolster.KF", pWeaponName);
                    pList = BuildFileList(cFileName, cPath, nullptr);
                    QueueKFList(pList, aePriority, apParent, nullptr);
                    if (pList) {
                        delete pList;
                    }
                }
            }
            BSstrcpy(pFileSlash, 0x104 - (pFileSlash - cFileName), "\\Death.KF");
            pList = BuildFileList(cFileName, cPath, nullptr);
            QueueKFList(pList, aePriority, apParent, nullptr);
            if (pList) {
                delete pList;
            }
        }
        return;
    }

    const char* pSkeletonSlash = std::strrchr(cPath, '\\');
    if (((pSkeletonSlash && !_strnicmp(pSkeletonSlash + 1, "Skeleton", 8)) || abForceAnimationLoad) && abCreature) {
        ANIM_GROUP_WEAPON_ENUM eWeaponGroup = AGW_NONE;
        if (apQueuedForRef && apQueuedForRef->IsActor()) {
            eWeaponGroup = static_cast<Actor*>(apQueuedForRef)->GetCurrentWeaponAnimType();
        }
        BSSimpleList<const char*>* pList = BuildKFFileList(cPath, true, true, eWeaponGroup);
        QueueKFList(pList, aePriority, apParent, nullptr);
        if (pList) {
            delete pList;
        }
    }
    else if (apQueuedForRef->IsNpc()) {
        char cIdlePath[0x104];
        BSstrcpy(cIdlePath, 0x104, cPath);
        char* pIdleSlash = std::strrchr(cIdlePath, '\\');
        if (apQueuedForRef->IsChild(true)) {
            BSstrcpy(pIdleSlash, 0x104 - (pIdleSlash - cIdlePath) - 1, "\\Locomotion\\Child\\IdleAnims");
        }
        else if (static_cast<Actor*>(apQueuedForRef)->GetSex() == 1) {
            BSstrcpy(pIdleSlash, 0x104 - (pIdleSlash - cIdlePath) - 1, "\\Locomotion\\Female\\IdleAnims");
        }
        else {
            BSstrcpy(pIdleSlash, 0x104 - (pIdleSlash - cIdlePath) - 1, "\\Locomotion\\Male\\IdleAnims");
        }
        void* pMemory = ourMalloc(sizeof(BSSimpleList<const char*>));
        BSSimpleList<const char*>* pList = pMemory ? ::new (pMemory) BSSimpleList<const char*> : nullptr;
        pModelLoader->CopyFilenameList(pIdleManager->GetRootFilenameList(cIdlePath, AGW_NONE), pList);
        QueueKFList(pList, aePriority, apParent, nullptr);
        if (pList) {
            delete pList;
        }
    }

    TESAnimation* pAnimation = dynamic_cast<TESAnimation*>(apModel);
    if (pAnimation && pAnimation->HasKFFiles()) {
        void* pMemory = ourMalloc(sizeof(BSSimpleList<const char*>));
        BSSimpleList<const char*>* pList = pMemory ? ::new (pMemory) BSSimpleList<const char*> : nullptr;
        for (BSSimpleList<char*>* pIter = pAnimation->GetKFFileList(); pIter; pIter = pIter->GetNext()) {
            const char* pName = pIter->GetItem();
            uint32_t uiSize = BSstrlen(pName) + 1;
            const char* pCopy = static_cast<char*>(ourMalloc(uiSize));
            BSstrcpy(const_cast<char*>(pCopy), uiSize, pName);
            pList->AddHead(pCopy);
        }
        char cSpecialPath[0x104];
        BSstrcpy(cSpecialPath, 0x104, apModel->GetModel());
        char* pSpecialSlash = std::strrchr(cSpecialPath, '\\');
        if (pSpecialSlash) {
            BSstrcpy(pSpecialSlash, 0x104 - (pSpecialSlash - cSpecialPath), "\\SpecialAnims\\");
        }
        QueueKFList(pList, aePriority, apParent, cSpecialPath);
        if (pList) {
            delete pList;
        }
    }
}

void ModelLoader::UpgradeTextures(NiAVObject* apObject) {}

// The model list's index counts the empty entries too. The texture list is read through TESTextureListArray::GetAt.
void ModelLoader::QueueModelList(
    BSSimpleList<const char*>* apModelList,
    TESTextureListArray* apTextureListArray,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    const char* apBasePath
) {
    uint32_t uiIndex = 0;
    for (BSSimpleList<const char*>* pIter = apModelList; pIter; pIter = pIter->GetNext()) {
        const char* pFileName = pIter->GetItem();
        if (pFileName) {
            char acPath[0x104];
            if (apBasePath) {
                BSstrcpy(acPath, 0x104, apBasePath);
                BSstrcat(acPath, 0x104, pFileName);
                pFileName = acPath;
            }
            if (apTextureListArray) {
                TESTextureList* pTextureList = apTextureListArray->GetAt(uiIndex);
                QueueModel(pFileName, pTextureList, aePriority, apParent, LOD_MULT_NONE, true, false, false);
            }
            else {
                QueueModel(pFileName, aePriority, apParent, LOD_MULT_NONE, true, false, false);
            }
        }
        ++uiIndex;
    }
}

// Each name is queued, freed and removed from the head of its node, so the walk stays on a node until it is empty.
void ModelLoader::QueueKFList(
    BSSimpleList<const char*>* apKFList,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    const char* apBasePath
) {
    BSSimpleList<const char*>* pIter = apKFList;
    while (pIter) {
        const char* pFileName = pIter->GetItem();
        if (pFileName) {
            const char* pPath = pFileName;
            char acPath[0x104];
            if (apBasePath) {
                BSstrcpy(acPath, 0x104, apBasePath);
                BSstrcat(acPath, 0x104, pFileName);
                pPath = acPath;
            }
            QueueKF(pPath, aePriority, apParent);
            ourFree(const_cast<char*>(pFileName));
            pIter->RemoveHead();
        }
        else {
            pIter = pIter->GetNext();
        }
    }
}

// A queued reference in apCriticalCell that is 3D critical moves to the critical priority, any other to its cell's
// priority; a load already at that priority is left alone.
void ModelLoader::UpdateQueuedReferencePriorities(TESObjectCELL* apCriticalCell) {
    using QueuedReferenceMap = LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>;
    alignas(QueuedReferenceMap::LockFreeMapIterator)
        uint8_t aIteratorStorage[sizeof(QueuedReferenceMap::LockFreeMapIterator)];
    QueuedReferenceMap::LockFreeMapIterator* pIterator =
        ::new (aIteratorStorage) QueuedReferenceMap::LockFreeMapIterator;
    while (!pIterator->GetFinished()) {
        TESObjectREFR* pRef = nullptr;
        NiPointer<QueuedReference> spQueuedReference;
        if (pQueuedReferencesMap->GetNext(*pIterator, pRef, spQueuedReference, true)) {
            TESObjectCELL* pCell = pRef->GetParentCell();
            if (pCell) {
                IO_TASK_PRIORITY eOldPriority = spQueuedReference->GetIOTaskPriority();
                IO_TASK_PRIORITY eNewPriority;
                if (pCell == apCriticalCell && pRef->Is3DCritical()) {
                    eNewPriority = IO_TASK_PRIORITY_CRITICAL;
                }
                else {
                    eNewPriority = TES::pInstance->GetCellPriority(pCell, nullptr);
                }
                if (eOldPriority != eNewPriority) {
                    spQueuedReference->Requeue(eNewPriority);
                }
            }
        }
    }
    pIterator->~LockFreeMapIterator();
}

uint32_t ModelLoader::GetQueuedReferenceCount() {
    return pQueuedReferencesMap->GetCount();
}

// The counts are formatted into a buffer nothing reads (the PC build does not set it as the title); once nothing is
// loading, the title is set back.
void ModelLoader::DisplayDebugLoadingText() {
    if (Renderer::GetFullScreen()) {
        return;
    }
    int32_t iReferences = GetQueuedReferenceCount();
    int32_t iCloning = GetBackgroundCloningCount();
    int32_t iTasks = pIOManager->GetQueuedCount();
    int32_t iPostProcess = pIOManager->GetPostProcessingCount();
    if (iTasks || iCloning || iReferences || iPostProcess) {
        char acText[0x104];
        BSsprintf(
            acText, 0x104, "Loading %d references( %d background cloning ) and %d tasks( %d post processing )",
            iReferences, iCloning, iTasks, iPostProcess
        );
        bLoadingTextDisplayed = true;
    }
    else if (bLoadingTextDisplayed) {
        bLoadingTextDisplayed = false;
        // Sets the main window's title.
        SDL_SetWindowTitle(pSDLMainWindow, pAppClassName);
    }
}

uint32_t ModelLoader::GetBackgroundCloningCount() {
    return pBackgroundCloneThread->GetQueuedCount();
}

void ModelLoader::PauseBackgroundCloning() {
    pBackgroundCloneThread->Pause();
}

void ModelLoader::ResumeBackgroundCloning() {
    pBackgroundCloneThread->Resume();
}

bool ModelLoader::AddModel(const char* apModelFileName, Model* apModel) {
    return pModelMap->SetAt(apModelFileName, apModel, false);
}

bool ModelLoader::AddKF(const char* apKFFileName, KFModel* apKFModel) {
    return pKFModelMap->SetAt(apKFFileName, apKFModel, false);
}

// A model that is not loaded is loaded here, at the critical priority and forced, by a queued load built on the stack
// and run and finished at once; abSuppressWarning is not read.
NiNode* ModelLoader::LoadFile(
    const char* apFilename,
    ENUM_LOD_MULT aeLODFadeMult,
    bool abPrepareObject,
    [[maybe_unused]] bool abSuppressWarning,
    bool abFaceGenModel,
    bool abDontIncrementRefCount
) {
    Model* pModel = nullptr;
    if (!pModelMap->GetAt(apFilename, pModel)) {
        alignas(QueuedModel) uint8_t aQueuedModelStorage[sizeof(QueuedModel)];
        QueuedModel* pQueuedModel = ::new (aQueuedModelStorage)
            QueuedModel(apFilename, IO_TASK_PRIORITY_CRITICAL, aeLODFadeMult, abPrepareObject, false);
        pQueuedModel->SetFaceGenModel(abFaceGenModel);
        pQueuedModel->SetForceLoad(true);
        pQueuedModel->QueuedModel::Run();
        pQueuedModel->QueuedModel::Finish();
        pModel = pQueuedModel->spModel.m_pObject;
        pQueuedModel->~QueuedModel();
    }
    else if (!pModel->GetRefCount()) {
        Warning("MODELS: LoadFile retrieved model %s with a zero ref count", apFilename);
    }
    if (!pModel) {
        return nullptr;
    }
    if (!abDontIncrementRefCount) {
        pModel->IncManualRefCount();
    }
    return pModel->spObject3D.m_pObject;
}

// As LoadFile, for an animation; the result always gets a manual reference.
KFModel* ModelLoader::LoadKF(const char* apFilename) {
    KFModel* pKFModel = nullptr;
    if (!pKFModelMap->GetAt(apFilename, pKFModel)) {
        alignas(QueuedKF) uint8_t aQueuedKFStorage[sizeof(QueuedKF)];
        QueuedKF* pQueuedKF = ::new (aQueuedKFStorage) QueuedKF(apFilename, IO_TASK_PRIORITY_CRITICAL);
        pQueuedKF->QueuedKF::Run();
        pQueuedKF->QueuedKF::Finish();
        pKFModel = pQueuedKF->spKFModel.m_pObject;
        pQueuedKF->~QueuedKF();
    }
    else if (!pKFModel->GetRefCount()) {
        Warning("MODELS: LoadKF retrieved animation %s with a zero ref count", apFilename);
    }
    if (pKFModel) {
        pKFModel->IncManualRefCount();
    }
    return pKFModel;
}

bool ModelLoader::FindModel(const char* apFilename, NiPointer<Model>& aspOutModel) {
    aspOutModel = static_cast<Model*>(nullptr);
    Model* pModel = nullptr;
    if (pModelMap->GetAt(apFilename, pModel)) {
        aspOutModel = pModel;
        return true;
    }
    return false;
}

BSSimpleList<const char*>* ModelLoader::BuildFileList(
    const char* apSearchName,
    const char* apBaseFilename,
    BSSimpleList<const char*>* apFileList
) {
    return FileFinder::BuildFileList(apSearchName, apBaseFilename, ARCHIVE_TYPE_MESHES, apFileList);
}

// With idle directories loaded, the list comes from the idle manager: every weapon group's list for
// ANIM_GROUP_WEAPON_COUNT, the no-weapon list then the weapon group's for a weapon group with abAddMTAnims, else the
// weapon group's. Otherwise the .kf files of the model's directory are listed, with its Locomotion directory and the
// Locomotion\Hurt animations (registered as their own idle directory and freed) when abAddLocomotion. The
// AutoMemContext constructor and destructor are called on a stack context and the constructor's result is ignored.
BSSimpleList<const char*>* ModelLoader::BuildKFFileList(
    const char* apBaseFilename,
    bool abAddLocomotion,
    bool abAddMTAnims,
    ANIM_GROUP_WEAPON_ENUM aeWeaponGroup
) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 4238
    );
    if (pIdleManager && pIdleManager->GetRootFilenameList(apBaseFilename, AGW_NONE)) {
        BSSimpleList<const char*>* plist = nullptr;
        if (aeWeaponGroup == ANIM_GROUP_WEAPON_COUNT) {
            for (uint32_t n = 0; n < ANIM_GROUP_WEAPON_COUNT; ++n) {
                if (plist) {
                    CopyFilenameList(
                        pIdleManager->GetRootFilenameList(apBaseFilename, static_cast<ANIM_GROUP_WEAPON_ENUM>(n)), plist
                    );
                }
                else {
                    plist = CopyFilenameList(
                        pIdleManager->GetRootFilenameList(apBaseFilename, static_cast<ANIM_GROUP_WEAPON_ENUM>(n)),
                        nullptr
                    );
                }
            }
        }
        else if (aeWeaponGroup != AGW_NONE && abAddMTAnims) {
            plist = CopyFilenameList(pIdleManager->GetRootFilenameList(apBaseFilename, AGW_NONE), nullptr);
            CopyFilenameList(pIdleManager->GetRootFilenameList(apBaseFilename, aeWeaponGroup), plist);
        }
        else {
            plist = CopyFilenameList(pIdleManager->GetRootFilenameList(apBaseFilename, aeWeaponGroup), nullptr);
        }
        pContext->~AutoMemContext();
        return plist;
    }

    char searchName[0x104];
    BSstrcpy(searchName, 0x104, "Data\\Meshes\\");
    BSstrcat(searchName, 0x104, apBaseFilename);
    char* pslash = std::strrchr(searchName, '\\');
    if (!pslash) {
        pContext->~AutoMemContext();
        return nullptr;
    }
    BSstrcpy(pslash, 0x104 - (pslash - searchName), "\\*.KF");
    BSSimpleList<const char*>* plist = BuildFileList(searchName, apBaseFilename, nullptr);
    if (abAddLocomotion && BSstrlen(apBaseFilename)) {
        char plocomotionBase[0x104];
        BSstrcpy(plocomotionBase, 0x104, apBaseFilename);
        char* plocomotionSlash = std::strrchr(plocomotionBase, '\\');
        if (plocomotionSlash) {
            *plocomotionSlash = '\0';
            BSstrcat(plocomotionSlash, 0x104 - (plocomotionSlash - plocomotionBase), "\\Locomotion\\");
            BSstrcpy(pslash, 0x104 - (pslash - searchName), "\\Locomotion\\*.KF");
            if (!plist) {
                plist = BuildFileList(searchName, plocomotionBase, nullptr);
            }
            else {
                BuildFileList(searchName, plocomotionBase, plist);
            }
            BSstrcat(plocomotionSlash, 0x104 - (plocomotionSlash - plocomotionBase), "Hurt\\");
            BSstrcat(plocomotionSlash, 0x104 - (plocomotionSlash - plocomotionBase), "IdleAnims");
            BSstrcpy(pslash, 0x104 - (pslash - searchName), "\\Locomotion\\Hurt\\*.KF");
            BSSimpleList<const char*>* phurtList = BuildFileList(searchName, plocomotionBase, nullptr);
            if (phurtList) {
                pIdleManager->AddRootIdleArray(plocomotionBase, phurtList, true);
                while (!phurtList->IsEmpty()) {
                    const char* pfilename = phurtList->GetItem();
                    ourFree(const_cast<char*>(pfilename));
                    phurtList->RemoveHead();
                }
                delete phurtList;
            }
        }
    }
    pIdleManager->AddRootIdleArray(apBaseFilename, plist, false);
    pContext->~AutoMemContext();
    return plist;
}

BSSimpleList<const char*>*
ModelLoader::CopyFilenameList(BSSimpleList<const char*>* apList, BSSimpleList<const char*>* apDestList) {
    BSSimpleList<const char*>* plist = apList;
    BSSimpleList<const char*>* pbaseList = apDestList;
    if (!pbaseList) {
        void* pMemory = ourMalloc(sizeof(BSSimpleList<const char*>));
        pbaseList = pMemory ? ::new (pMemory) BSSimpleList<const char*> : nullptr;
    }
    while (plist) {
        const char* pName = plist->GetItem();
        uint32_t uiSize = BSstrlen(pName) + 1;
        const char* pCopy = static_cast<char*>(ourMalloc(uiSize));
        BSstrcpy(const_cast<char*>(pCopy), uiSize, pName);
        pbaseList->AddHead(pCopy);
        plist = plist->GetNext();
    }
    return pbaseList;
}

bool ModelLoader::IsFileQueuedForCell(TESObjectCELL* apCell) {
    return apCell && apCell->GetCriticalQueuedRefCount() > 0;
}

// A file already loaded gets a task that holds it, finished at once, but only for a parent; otherwise the load is
// queued.
void ModelLoader::QueueFaceGenFile(
    const char* apFileName,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    IO_FILE_INDEX aeFileIndex
) {
    NiPointer<LoadedFile> spLoadedFile(GetLoadedFileFromMap(apFileName));
    if (spLoadedFile.m_pObject) {
        if (apParent) {
            void* pMemory = ourMalloc(sizeof(QueuedFaceGenFile));
            QueuedFaceGenFile* pQueuedFile =
                pMemory ? ::new (pMemory) QueuedFaceGenFile(spLoadedFile.m_pObject, aePriority) : nullptr;
            NiPointer<QueuedFaceGenFile> spQueuedFile(pQueuedFile);
            spQueuedFile->SetParent(apParent);
            spQueuedFile->CheckFinished();
        }
    }
    else {
        void* pMemory = ourMalloc(sizeof(QueuedFaceGenFile));
        QueuedFaceGenFile* pQueuedFile =
            pMemory ? ::new (pMemory) QueuedFaceGenFile(apFileName, aePriority, aeFileIndex) : nullptr;
        NiPointer<QueuedFaceGenFile> spQueuedFile(pQueuedFile);
        spQueuedFile->SetParent(apParent);
        spQueuedFile->QueueMe();
    }
}

// For the hair, the .egm of every hair part, else the model's own. A model cached with its .egm data gets a task that
// holds it, finished at once, but only for a parent; otherwise the file is queued as a mesh. The model and mesh data
// pointers live across the hair parts.
void ModelLoader::QueueEGMFile(
    TESModel* apModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    int32_t aiHeadPart
) {
    NiPointer<BSFaceGenModel> spFaceGenModel;
    NiPointer<BSFaceGenModelMeshData> spModelMeshData;
    char acFileName[0x104];
    BSstrcpy(acFileName, 0x104, "Meshes\\");
    BSstrcat(acFileName, 0x104, apModel->GetModel());
    if (aiHeadPart == RACE_HAIR) {
        for (int32_t i = 0; i < HAIR_PART_COUNT; ++i) {
            BSStringT<char> kEGMFileName;
            BSFaceGenManager::GetAsEGMFile(kEGMFileName, acFileName, static_cast<HAIR_PART>(i));
            if (BSFaceGenManager::GetModelCache() &&
                BSFaceGenManager::GetModelCache()->GetAt(kEGMFileName.pString, spFaceGenModel, spModelMeshData) &&
                spFaceGenModel.m_pObject && spModelMeshData.m_pObject && spModelMeshData->pEGMData) {
                if (apParent) {
                    void* pMemory = ourMalloc(sizeof(QueuedFaceGenFile));
                    QueuedFaceGenFile* pQueuedFile = pMemory
                        ? ::new (pMemory) QueuedFaceGenFile(spFaceGenModel.m_pObject, spModelMeshData.m_pObject)
                        : nullptr;
                    NiPointer<QueuedFaceGenFile> spQueuedFile(pQueuedFile);
                    spQueuedFile->SetParent(apParent);
                    spQueuedFile->CheckFinished();
                }
            }
            else {
                QueueFaceGenFile(kEGMFileName.pString, aePriority, apParent, IO_FILE_INDEX_MESHES);
            }
            kEGMFileName.Set(nullptr, 0);
        }
    }
    else {
        BSStringT<char> kEGMFileName;
        BSFaceGenManager::GetAsEGMFile(kEGMFileName, acFileName, HAIR_PART_NONE);
        if (BSFaceGenManager::GetModelCache() &&
            BSFaceGenManager::GetModelCache()->GetAt(kEGMFileName.pString, spFaceGenModel, spModelMeshData) &&
            spFaceGenModel.m_pObject && spModelMeshData.m_pObject && spModelMeshData->pEGMData) {
            if (apParent) {
                void* pMemory = ourMalloc(sizeof(QueuedFaceGenFile));
                QueuedFaceGenFile* pQueuedFile = pMemory
                    ? ::new (pMemory) QueuedFaceGenFile(spFaceGenModel.m_pObject, spModelMeshData.m_pObject)
                    : nullptr;
                NiPointer<QueuedFaceGenFile> spQueuedFile(pQueuedFile);
                spQueuedFile->SetParent(apParent);
                spQueuedFile->CheckFinished();
            }
        }
        else {
            QueueFaceGenFile(kEGMFileName.pString, aePriority, apParent, IO_FILE_INDEX_MESHES);
        }
        kEGMFileName.Set(nullptr, 0);
    }
}

// A model cached under its .egm name gets a task that holds it, finished at once, but only for a parent; otherwise the
// .tri file is queued as a mesh. aiHeadPart is not used.
void ModelLoader::QueueTRIFile(
    TESModel* apModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    [[maybe_unused]] int32_t aiHeadPart
) {
    NiPointer<BSFaceGenModel> spFaceGenModel;
    NiPointer<BSFaceGenModelMeshData> spModelMeshData;
    char acFileName[0x104];
    BSstrcpy(acFileName, 0x104, "Meshes\\");
    BSstrcat(acFileName, 0x104, apModel->GetModel());
    BSStringT<char> kEGMFileName;
    BSFaceGenManager::GetAsEGMFile(kEGMFileName, acFileName, HAIR_PART_NONE);
    if (BSFaceGenManager::GetModelCache() &&
        BSFaceGenManager::GetModelCache()->GetAt(kEGMFileName.pString, spFaceGenModel, spModelMeshData) &&
        spFaceGenModel.m_pObject && spModelMeshData.m_pObject) {
        if (apParent) {
            void* pMemory = ourMalloc(sizeof(QueuedFaceGenFile));
            QueuedFaceGenFile* pQueuedFile = pMemory
                ? ::new (pMemory) QueuedFaceGenFile(spFaceGenModel.m_pObject, spModelMeshData.m_pObject)
                : nullptr;
            NiPointer<QueuedFaceGenFile> spQueuedFile(pQueuedFile);
            spQueuedFile->SetParent(apParent);
            spQueuedFile->CheckFinished();
        }
    }
    else {
        BSStringT<char> kTRIFileName;
        BSFaceGenManager::GetAsTRIFile(kTRIFileName, acFileName, HAIR_PART_NONE);
        QueueFaceGenFile(kTRIFileName.pString, aePriority, apParent, IO_FILE_INDEX_MESHES);
        kTRIFileName.Set(nullptr, 0);
    }
    kEGMFileName.Set(nullptr, 0);
}

bool ModelLoader::AddLoadedFileToMap(const char* apFileName, LoadedFile* apLoadedFile) {
    return pLoadedFileMap->SetAt(apFileName, apLoadedFile, false);
}

void ModelLoader::RemoveLoadedFileFromMap(const char* apFileName) {
    pLoadedFileMap->RemoveAt(apFileName);
}

LoadedFile* ModelLoader::GetLoadedFileFromMap(const char* apFileName) {
    LoadedFile* pLoadedFile = nullptr;
    if (pLoadedFileMap->GetAt(apFileName, pLoadedFile)) {
        return pLoadedFile;
    }
    return nullptr;
}

KFModel* ModelLoader::FindKFModel(const char* apFilename) {
    KFModel* pKFModel = nullptr;
    if (pKFModelMap->GetAt(apFilename, pKFModel)) {
        return pKFModel;
    }
    return nullptr;
}

// Power-attack animations (ANIM_GROUP_ATTACK_POWER_START up to, not including, ANIM_GROUP_POWER_ATTACK_STOP) stay
// loaded while unreferenced.
void ModelLoader::ClearUnusedModels(bool abMemoryPass) {
    if (!abMemoryPass && pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading()) {
        bHasDelayedFree = true;
        return;
    }
    if (pModelMap) {
        alignas(LockFreeStringMap<Model*>::LockFreeStringMapIterator)
            uint8_t aIteratorStorage[sizeof(LockFreeStringMap<Model*>::LockFreeStringMapIterator)];
        LockFreeStringMap<Model*>::LockFreeStringMapIterator* pIterator =
            ::new (aIteratorStorage) LockFreeStringMap<Model*>::LockFreeStringMapIterator;
        while (!pIterator->GetFinished()) {
            Model* pModel = nullptr;
            const char* pFilename = nullptr;
            if (pModelMap->GetNext(*pIterator, pFilename, pModel, true) && pModel && pModel->GetRefCount() == 0) {
                pModelMap->RemoveAt(pFilename);
                if (pModel) {
                    delete pModel;
                }
            }
        }
        pIterator->~LockFreeStringMapIterator();
    }
    if (pKFModelMap) {
        alignas(LockFreeStringMap<KFModel*>::LockFreeStringMapIterator)
            uint8_t aIteratorStorage[sizeof(LockFreeStringMap<KFModel*>::LockFreeStringMapIterator)];
        LockFreeStringMap<KFModel*>::LockFreeStringMapIterator* pIterator =
            ::new (aIteratorStorage) LockFreeStringMap<KFModel*>::LockFreeStringMapIterator;
        while (!pIterator->GetFinished()) {
            KFModel* pKFModel = nullptr;
            const char* pFilename = nullptr;
            if (pKFModelMap->GetNext(*pIterator, pFilename, pKFModel, true) && pKFModel &&
                pKFModel->GetRefCount() == 0) {
                bool bRemove = true;
                if (pKFModel->GetAnimGroup() && pKFModel->GetAnimGroup()->GetType() >= ANIM_GROUP_ATTACK_POWER_START &&
                    pKFModel->GetAnimGroup()->GetType() < ANIM_GROUP_POWER_ATTACK_STOP) {
                    bRemove = false;
                }
                if (bRemove) {
                    pKFModelMap->RemoveAt(pFilename);
                    if (pKFModel) {
                        delete pKFModel;
                    }
                }
            }
        }
        pIterator->~LockFreeStringMapIterator();
    }
    bHasDelayedFree = false;
}

void ModelLoader::LoadAddonNodes(TESObjectREFR* apRef, NiNode* apNode) {
    if (apNode) {
        BSXFlags* pFlags = static_cast<BSXFlags*>(apNode->GetExtraData(BSXFlags::Tag()));
        if (pFlags && apRef && pFlags->QAddon() && !apRef->GetFireOff()) {
            LoadAddons(apNode);
        }
    }
}

// The addon's model name comes from TESModel::GetModel through the addon's TESModel subobject, and the model map's
// GetAt is called virtually.
void ModelLoader::LoadAddons(NiNode* apNode) {
    Model* pcurrent;
    if (apNode) {
        if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, apNode)) {
            BSValueNode* pvalueNode = static_cast<BSValueNode*>(apNode);
            BGSAddonNode* paddonNode = pDataHandler->GetAddonNode(pvalueNode->QValue());
            if ((paddonNode) && (!paddonNode->IsMasterParticleSystem())) {
                pcurrent = nullptr;
                if ((!pModelMap->GetAt(paddonNode->GetModel(), pcurrent)) &&
                    (LoadFile(paddonNode->GetModel(), LOD_MULT_NONE, true, false, false, false)) &&
                    (pModelMap->GetAt(paddonNode->GetModel(), pcurrent))) {
                    pcurrent->DecManualRefCount();
                }
            }
        }
        for (uint32_t n = 0; n < apNode->GetArrayCount(); ++n) {
            NiAVObject* pobj = apNode->GetAt(n);
            if (pobj) {
                NiNode* pnode = pobj->IsNode();
                if (pnode) {
                    LoadAddons(pnode);
                }
            }
        }
    }
}

bool ModelLoader::UpdateAttachDistant3DQueue() {
    NiPointer<AttachDistant3DTask> spTask;
    pAttachDistant3DTaskQueue->Pop(spTask);
    while (spTask.m_pObject) {
        spTask->PostProcess();
        pAttachDistant3DTaskQueue->Pop(spTask);
    }
    return true;
}

bool ModelLoader::UpdateReferencesToQueue() {
    alignas(BSPrecisionTimer) uint8_t aTimerStorage[sizeof(BSPrecisionTimer)];
    BSPrecisionTimer* pTimer = ::new (aTimerStorage) BSPrecisionTimer;
    // The time budget in milliseconds: long while the IO manager loads a queued priority.
    if (pIOManager->IsLoadingQueuedPriority()) {
        pTimer->Reset(100000.0f);
    }
    else {
        pTimer->Reset(5.0f);
    }
    using Iterator = LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>::LockFreeMapIterator;
    alignas(Iterator) uint8_t aIteratorStorage[sizeof(Iterator)];
    Iterator* pIterator = ::new (aIteratorStorage) Iterator;
    while (!pIterator->GetFinished() && !pTimer->Check()) {
        TESObjectREFR* pRef = nullptr;
        NiPointer<QueuedReference> spQueuedReference;
        if (pReferencesToQueueMap->GetNextAndRemove(*pIterator, pRef, spQueuedReference, true)) {
            spQueuedReference->QueueMe();
        }
    }
    bool bFinished = pIterator->GetFinished();
    pIterator->~Iterator();
    return bFinished;
}

bool ModelLoader::AddQueuedTextureToMap(BSFileEntry* apFileEntry, QueuedTexture* apQueuedTexture) {
    NiPointer<QueuedTexture> spQueuedTexture(apQueuedTexture);
    bool bResult = pQueuedTextureMap->SetAt(apFileEntry, spQueuedTexture, false);
    return bResult;
}

void ModelLoader::RemoveQueuedTextureFromMap(BSFileEntry* apFileEntry) {
    pQueuedTextureMap->RemoveAt(apFileEntry);
}

void ModelLoader::EnterModelMapCS() {
    pModelMap->Lock();
}

void ModelLoader::UnlockModelMap() {
    pModelMap->Unlock();
}

bool ModelLoader::TryLockModelMap() {
    return pModelMap->TryLock();
}

void ModelLoader::SetExiting() {
    pBackgroundCloneThread->SetExiting();
}

// Each name is queued, freed and removed from the head of its node, so the walk stays on a node until it is empty
// (QueueKFList's walk).
void ModelLoader::QueueReplacementKFList(
    BSSimpleList<const char*>* apKFList,
    Animation* apAnim,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    const char* apBasePath
) {
    if ((apKFList) && (apAnim)) {
        BSSimpleList<const char*>* pfileList = apKFList;
        if (pfileList && pfileList->GetItem()) {
            AutoMemContext tempMemContext(
                MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp",
                3743
            );
            NiPointer<QueuedReplacementKFList> spreplacementKFList(static_cast<QueuedReplacementKFList*>(nullptr));
            while (pfileList) {
                const char* pfileName = pfileList->GetItem();
                if (pfileName) {
                    const char* pfileNameCopy = pfileName;
                    char Dst[0x104];
                    if (apBasePath) {
                        BSstrcpy(Dst, 0x104, apBasePath);
                        BSstrcat(Dst, 0x104, pfileName);
                        pfileNameCopy = Dst;
                    }
                    KFModel* pkfModel = nullptr;
                    if (pKFModelMap->GetAt(pfileNameCopy, pkfModel)) {
                        apAnim->QueueReplacementAnimation(pkfModel);
                    }
                    else {
                        QueuedReplacementKF* v9;
                        if (!spreplacementKFList.m_pObject &&
                            !pQueuedReplacementKFListMap->GetAt(apAnim, spreplacementKFList)) {
                            QueuedReplacementKFList* v10;
                            QueuedReplacementKFList* v13 =
                                static_cast<QueuedReplacementKFList*>(ourMalloc(sizeof(QueuedReplacementKFList)));
                            if (v13) {
                                v10 = ::new (v13) QueuedReplacementKFList(apAnim, aePriority);
                            }
                            else {
                                v10 = nullptr;
                            }
                            spreplacementKFList.operator=(v10);
                        }
                        QueuedReplacementKF* v12 =
                            static_cast<QueuedReplacementKF*>(ourMalloc(sizeof(QueuedReplacementKF)));
                        if (v12) {
                            QueuedReplacementKFList* v6 = spreplacementKFList.m_pObject;
                            v9 = ::new (v12) QueuedReplacementKF(pfileNameCopy, aePriority, apAnim, v6);
                        }
                        else {
                            v9 = nullptr;
                        }
                        NiPointer<QueuedReplacementKF> spqueuedKF(v9);
                        spqueuedKF.m_pObject->SetParent(spreplacementKFList.m_pObject);
                        QueuedReplacementKF* v8 = spqueuedKF.m_pObject;
                        v8->QueueMe();
                    }
                    ourFree(const_cast<char*>(pfileName));
                    pfileList->RemoveHead();
                }
                else {
                    pfileList = pfileList->GetNext();
                }
            }
            if (spreplacementKFList.m_pObject && spreplacementKFList.m_pObject->GetChildCount()) {
                spreplacementKFList.m_pObject->SetParent(apParent);
                pQueuedReplacementKFListMap->SetAt(apAnim, spreplacementKFList, true);
                spreplacementKFList.m_pObject->SetCompleted();
                QueuedReplacementKFList* v7 = spreplacementKFList.m_pObject;
                v7->CheckFinished();
            }
            else {
                spreplacementKFList.operator=(static_cast<QueuedReplacementKFList*>(nullptr));
            }
        }
    }
}

void ModelLoader::TryAndRemoveModel(Model* apModel, const char* apModelFileName) {
    if (apModel->GetRefCount() == 0) {
        bool bRemove = true;
        if (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading()) {
            bRemove = false;
            bHasDelayedFree = true;
        }
        if (bRemove) {
            pModelMap->RemoveAt(apModelFileName);
            if (apModel) {
                delete apModel;
            }
        }
    }
}

void ModelLoader::QueueAttachDistant3DTask(AttachDistant3DTask* apTask) {
    NiPointer<AttachDistant3DTask> spTask(apTask);
    pAttachDistant3DTaskQueue->Push(spTask);
}

void ModelLoader::OutputModelMapContents(BSFile* apFile, bool abShowZeroCountModels) {
    KFModel* pkfModel;
    const char* pfileName_2;
    const char* pfileName;
    Model* pmodel;
    if (pModelMap->GetCount()) {
        LockFreeStringMap<Model*>::LockFreeStringMapIterator iterator;
        while (!iterator.GetFinished()) {
            pfileName = nullptr;
            pmodel = nullptr;
            if (pModelMap->GetNext(iterator, pfileName, pmodel, true) &&
                (pmodel->GetRefCount() > 0 || abShowZeroCountModels)) {
                char acBuffer[1024];
                const char* v8 = pfileName;
                uint32_t FormFlags = pmodel->iManualRefCount;
                uint32_t FormHealth_0 = pmodel->iRefCount;
                BSsprintf(
                    acBuffer, 1024, "MODELS:\t%3d\t%3d\t%3d\t%s\r\n", pmodel->GetRefCount(), FormHealth_0, FormFlags, v8
                );
                Warning(acBuffer);
                if (apFile) {
                    size_t v3 = std::strlen(acBuffer);
                    apFile->WriteF(acBuffer, v3 + 1);
                }
            }
        }
    }
    if (pKFModelMap->GetCount()) {
        LockFreeStringMap<KFModel*>::LockFreeStringMapIterator v15;
        while (!v15.GetFinished()) {
            pfileName_2 = nullptr;
            pkfModel = nullptr;
            if (pKFModelMap->GetNext(v15, pfileName_2, pkfModel, true) && pkfModel->GetRefCount() > 0) {
                BSStringT<char> textline;
                const char* v9 = pfileName_2;
                uint32_t v7 = pkfModel->iManualRefCount;
                int32_t CastingType = pkfModel->iRefCount;
                textline.SPrintF("ANIMATION:\t%3d\t%3d\t%3d\t%s\r\n", pkfModel->GetRefCount(), CastingType, v7, v9);
                Warning(textline.pString);
                if (apFile) {
                    apFile->Write(textline, true);
                }
            }
        }
    }
}

void ModelLoader::QueueMagicItem(
    MagicItem* apMagicItem,
    EffectSetting* apAVEffectSetting,
    IO_TASK_PRIORITY aePriority
) {
    if ((apMagicItem) && (apAVEffectSetting)) {
        QueuedMagicItem* v5;
        QueuedMagicItem* apMemory = static_cast<QueuedMagicItem*>(ourMalloc(sizeof(QueuedMagicItem)));
        if (apMemory) {
            v5 = ::new (apMemory) QueuedMagicItem(apMagicItem, apAVEffectSetting, aePriority);
        }
        else {
            v5 = nullptr;
        }
        NiPointer<QueuedMagicItem> spqueuedMagicItem(v5);
        QueuedMagicItem* v4 = spqueuedMagicItem.m_pObject;
        v4->QueueMe();
    }
}

void ModelLoader::CancelReplacementKFList(Animation* apAnim) {
    NiPointer<QueuedReplacementKFList> spqueuedReplacementKFList(static_cast<QueuedReplacementKFList*>(nullptr));
    if (pQueuedReplacementKFListMap->GetAt(apAnim, spqueuedReplacementKFList)) {
        QueuedReplacementKFList* v2 = spqueuedReplacementKFList.m_pObject;
        pIOManager->CancelTask(v2, nullptr);
    }
}

void ModelLoader::CancelAnimIdle(AnimIdle* apAnimIdle) {
    NiPointer<QueuedAnimIdle> spqueuedAnimIdle(static_cast<QueuedAnimIdle*>(nullptr));
    if (pQueuedAnimIdleMap->GetAt(apAnimIdle, spqueuedAnimIdle)) {
        QueuedAnimIdle* v2 = spqueuedAnimIdle.m_pObject;
        pIOManager->CancelTask(v2, nullptr);
    }
}

void ModelLoader::CancelReferencesForCell(TESObjectCELL* apCell) {
    using iterator_t = LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>::LockFreeMapIterator;
    if ((apCell) && (apCell->iQueuedRefCount)) {
        iterator_t iterator;
        while (!iterator.GetFinished()) {
            TESObjectREFR* pref = nullptr;
            NiPointer<QueuedReference> spqueuedReference(static_cast<QueuedReference*>(nullptr));
            if (pQueuedReferencesMap->GetNext(iterator, pref, spqueuedReference, true) &&
                pref->GetParentCell() == apCell) {
                CancelReference(pref);
            }
        }
    }
}

void ModelLoader::QueueCreatureParts(
    TESModel* apModel,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent,
    TESObjectREFR* apQueuedForRef
) {
    float afvalue;
    TESCreature* v17 = dynamic_cast<TESCreature*>(apModel);
    if (v17) {
        char cPath[264];
        BSSimpleList<const char*>* pModels = v17->GetNIFFileList();
        const char* pPath = apModel->GetModel();
        BSstrcpy(cPath, 260, pPath);
        char* v15 = std::strrchr(cPath, 92);
        if (v15) {
            v15[1] = 0;
        }
        TESTextureListArray* v6 = &v17->TextureListArray;
        QueueModelList(pModels, v6, aePriority, apParent, cPath);
        if (apQueuedForRef) {
            InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(apQueuedForRef);
            ItemChange* BestWeapon = InventoryChanges_->GetBestWeapon(v17, afvalue, COMBAT_WEAPON_TYPE_COUNT, false);
            if (BestWeapon) {
                TESBoundObject* FormFlags = BestWeapon->pContainerObj;
                QueueBoundObject(FormFlags, aePriority, apParent, apQueuedForRef);
                delete BestWeapon;
            }
            ItemChange* BestAmmo = InventoryChanges_->GetBestAmmo(v17, false);
            if (BestAmmo) {
                TESBoundObject* v8 = BestAmmo->pContainerObj;
                QueueBoundObject(v8, aePriority, apParent, apQueuedForRef);
                delete BestAmmo;
            }
        }
    }
}

void ModelLoader::QueueTreeBillboard(const char* apFilename, TREE_BILLBOARD_DATA* apBillboardData) {
    QueuedTreeBillboard* v4;
    QueuedTreeBillboard* apMemory = static_cast<QueuedTreeBillboard*>(ourMalloc(sizeof(QueuedTreeBillboard)));
    if (apMemory) {
        v4 = ::new (apMemory) QueuedTreeBillboard(apFilename, IO_TASK_PRIORITY_LOW, apBillboardData);
    }
    else {
        v4 = nullptr;
    }
    NiPointer<QueuedTreeBillboard> spqueuedTreeBillboard(v4);
    QueuedTreeBillboard* v3 = spqueuedTreeBillboard.m_pObject;
    v3->QueueMe();
}

void ModelLoader::CancelAllFiles() {
    AnimIdle* panimIdle;
    using iterator2_t = LockFreeMap<AnimIdle*, NiPointer<QueuedAnimIdle>>::LockFreeMapIterator;
    using iterator1_t = LockFreeMap<TESObjectREFR*, NiPointer<QueuedReference>>::LockFreeMapIterator;
    using iterator3_t = LockFreeMap<TESObjectREFR*, NiPointer<QueuedHelmet>>::LockFreeMapIterator;
    pIOManager->Pause();
    iterator1_t iterator1;
    while (!iterator1.GetFinished()) {
        TESObjectREFR* pref = nullptr;
        NiPointer<QueuedReference> spqueuedReference(static_cast<QueuedReference*>(nullptr));
        if (pQueuedReferencesMap->GetNext(iterator1, pref, spqueuedReference, true)) {
            QueuedReference* v1 = spqueuedReference.m_pObject;
            pIOManager->CancelTask(v1, nullptr);
        }
    }
    pReferencesToQueueMap->RemoveAll(false);
    iterator2_t iterator2;
    while (!iterator2.GetFinished()) {
        panimIdle = nullptr;
        NiPointer<QueuedAnimIdle> spqueuedAnimIdle(static_cast<QueuedAnimIdle*>(nullptr));
        if (pQueuedAnimIdleMap->GetNext(iterator2, panimIdle, spqueuedAnimIdle, true)) {
            QueuedAnimIdle* v2 = spqueuedAnimIdle.m_pObject;
            pIOManager->CancelTask(v2, nullptr);
        }
    }
    iterator3_t iterator3;
    while (!iterator3.GetFinished()) {
        TESObjectREFR* pref_2 = nullptr;
        NiPointer<QueuedHelmet> spqueuedHelmet(static_cast<QueuedHelmet*>(nullptr));
        if (pQueuedHelmetMap->GetNext(iterator3, pref_2, spqueuedHelmet, true)) {
            QueuedHelmet* v3 = spqueuedHelmet.m_pObject;
            pIOManager->CancelTask(v3, nullptr);
        }
    }
    pIOManager->CancelAllTasks();
    pIOManager->Resume();
}

void ModelLoader::UpdateQueueCallback() {
    pModelLoader->DisplayDebugLoadingText();
}

// Both queues are updated every time; true when both are done.
bool ModelLoader::UpdateSecondaryQueueCallback() {
    bool bAttachDone = pModelLoader->UpdateAttachDistant3DQueue();
    bool bReferencesDone = pModelLoader->UpdateReferencesToQueue();
    return bAttachDone && bReferencesDone;
}

// Both save-game state tests always return false, so the loading sequence is never started from here; both calls are
// kept deliberately, as the game does. afPercent (IOManager's progress) is unused.
void ModelLoader::LoadQueuedPriorityCallback(float afPercent) {
    MemoryLevelManager::QInstance()->CheckMemoryLevel(false);
    FalloutAudioMedia::Update();
    if (pSaveLoadGameOLD->GetLoading() && pSaveLoadGameOLD->GetLoading()) {
        pSaveLoadGameOLD->StartLoadingSequence();
    }
}

uint32_t ModelLoader::GetExternalCountCallback() {
    return pModelLoader->GetBackgroundCloningCount();
}

void ModelLoader::PauseCallback() {
    pModelLoader->PauseBackgroundCloning();
}

void ModelLoader::ResumeCallback() {
    pModelLoader->ResumeBackgroundCloning();
}

bool& ModelLoader::GetThreadLoadTexturesDegraded() {
    static thread_local bool bLoadTexturesDegraded = false;
    return bLoadTexturesDegraded;
}

int32_t BSstrnicmp(const char* apString1, const char* apString2, uint32_t aiCount) {
    return _strnicmp(apString1, apString2, aiCount);
}

bhkRigidBody* bhkCollisionObject::GetRigidBody() {
    return spWorldObject;
}

bhkCollisionObject* bhkCollisionObject::GetbhkCollisionObject(NiAVObject* apObject) {
    return static_cast<bhkCollisionObject*>(
        NiObject::DynamicCastStatic(&bhkCollisionObject::ms_RTTI, apObject->NiAVObject::GetCollisionObject())
    );
}

hkReferencedObject* bhkRefObject::GethkObject() const {
    return phkObject;
}

hkpWorldObject* bhkWorldObject::Gethk() {
    return static_cast<hkpWorldObject*>(phkObject);
}

IOTask::IOTask(IO_TASK_PRIORITY aePriority) {
    SetIOTaskPriority(aePriority);
}

// The key: file index 3, the next offset, the task's priority and the low 16 bits of the next counter, fetched in the
// order counter, priority, offset.
void IOTask::GenerateKey() {
    uint16_t sCounter = GetNextCounter();
    uint8_t cPriority = GetPriority();
    uint32_t iOffset = GetNextOffset();
    SetKey(3, iOffset, cPriority, sCounter);
}

void IOTask::SetKey(char acFileIndex, uint32_t aiOffset, uint8_t acPriority, uint16_t asCounter) {
    uint64_t v5 = static_cast<uint64_t>(acPriority) << 16;
    Key = static_cast<int64_t>(
        (static_cast<uint64_t>(Key) & 0xFFFFFFFF00000000ull) |
        (static_cast<uint64_t>(static_cast<uint32_t>((asCounter | (v5 | aiOffset << 24)))))
    );
    Key = static_cast<int64_t>(
        (static_cast<uint64_t>(Key) & 0xFFFFFFFFull) |
        (static_cast<uint64_t>(static_cast<uint32_t>(
             (static_cast<uint32_t>(v5 >> 32) | (static_cast<uint64_t>(aiOffset) >> 8 | acFileIndex << 24))
         ))
         << 32)
    );
}

uint8_t IOTask::GetPriority() {
    return IOTask::GetPriorityFromKey(Key);
}

void IOTask::SetPriority(uint8_t acPriority) {
    Key = (static_cast<uint64_t>(acPriority) << 16) +
        ((static_cast<uint64_t>(static_cast<uint32_t>(Key >> 32)) << 32) | static_cast<uint32_t>((Key & 0xFF00FFFFu)));
}

void QueuedFile::NotifyChildFinished(QueuedFile* apChild) {
    pChildren->AddFinishedChild();
    CheckFinished();
}

void QueuedFile::SetParent(QueuedFile* apParent) {
    this->spParent = apParent;
    if (this->spParent.operator->()) {
        this->spParent.operator->()->AddChild(this);
    }
}

bool QueuedFile::GetAllChildrenFinished() {
    if (!pChildren) {
        return true;
    }
    uint32_t uiSize = pChildren->GetSize();
    return uiSize == pChildren->GetChildCountFinished();
}

uint32_t QueuedFile::GetChildCount() {
    if (pChildren) {
        return pChildren->GetSize();
    }

    return 0;
}

uint32_t QueuedChildren::GetChildCountFinished() {
    return iNumChildrenFinished;
}

void QueuedChildren::AddFinishedChild() {
    NiAtomicIncrement(iNumChildrenFinished);
}

int32_t Archive::GetRuntimeID() const {
    return m_iRuntimeID;
}

// iSize without its flag bits.
uint32_t BSFileEntry::Size() {
    return iSize & 0x3FFFFFFF;
}

TESObjectANIO* AnimIdle::GetAnimObject(int32_t aiIndex) {
    return pAnimObj[aiIndex];
}

// Tests the master-particle-system flag in the addon data.
bool BGSAddonNode::IsMasterParticleSystem() {
    return (Data.cFlags & 1) != 0;
}

// The thread is created suspended (StartThread resumes it); aiProcessor is deliberately unused, as the game does.
BSTaskThread::BSTaskThread(uint32_t aiProcessor, const char* apThreadName)
  : Semaphore(0, 0x7FFFFFFF), PauseSemaphore(1, 1) {
    hThreadHandle = new BSSystemThread(&BSTaskThreadProc, this, true);
    iThreadID = hThreadHandle->Id();
    if (apThreadName) {
        hThreadHandle->SetName(apThreadName);
    }
    HavokThreadMemory.pHavokThreadRouter = nullptr;
    HavokThreadMemory.pHavokThreadStackBuffer = nullptr;
    HavokThreadMemory.iThreadID = iThreadID;
}

// The semaphores close their own handles. The thread is suspended (at its next wait: BSSystemThread) and closed.
BSTaskThread::~BSTaskThread() {
    hThreadHandle->Suspend();
    delete hThreadHandle;
}

void BSTaskThread::Sleep() {
    SignalTask();
    WaitPauseSemaphore();
}

void BSTaskThread::SignalTask() {
    if (!Semaphore.iCount) {
        (&Semaphore)->Signal();
    }
}

void BSTaskThread::StartThread() {
    hThreadHandle->Resume();
}

void BSTaskThread::SignalPauseSemaphore() {
    (&PauseSemaphore)->Signal();
}

void BSTaskThread::Resume() {
    SignalTask();
    SignalPauseSemaphore();
}

void BSTaskThread::WaitTask() {
    (&Semaphore)->Wait();
}

void BSTaskThread::WaitPauseSemaphore() {
    (&PauseSemaphore)->Wait();
}

void BSTaskThread::SetPauseSleepEnabled(bool abEnable) {
    BSTaskThread::sDoPauseSleep = (bool)((uint8_t)abEnable);
}

void BSTaskThread::SetPauseSleepValue(int32_t aiValue) {
    BSTaskThread::sPauseSleepValue = aiValue;
}

void BSTaskThread::SetUpdateSleepEnabled(bool abEnable) {
    BSTaskThread::sDoUpdateSleep = (bool)((uint8_t)abEnable);
}

void BSTaskThread::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}

BIPOBJECT* BipedAnim::GetBipObject(BIPED_OBJECT aeBipedObject) {
    return &this->object[aeBipedObject];
}

TESObjectREFR* BipedAnim::GetRequester() {
    return this->m_pRequester;
}

bool BSFaceGenManager::GetUseFaceGenHeads() {
    return BSFaceGenManager::bUseFaceGenHeads.Bool();
}

// Sets (abSet) or clears bit 8 of *apFlags and returns apFlags on both paths.
uint8_t* MLSetFlag_8(char abSet, uint8_t* apFlags) {
    if (abSet) {
        *apFlags |= 8;
    }
    else {
        *apFlags &= ~8;
    }
    return apFlags;
}

KFModel::KFModel(const char* apFilename, BSStream* apStream)
  : spSequence(nullptr), spAnimGroup(nullptr), iRefCount(0), iManualRefCount(0) {
    uint32_t uilen = BSstrlen(apFilename);
    pFilename = static_cast<char*>(ourMalloc(uilen + 1));
    BSstrcpy(pFilename, uilen + 1, apFilename);
    BSAnimGroupSequence::CreateSequenceFromFile(*apStream, 0, spSequence);
    if (spSequence.m_pObject) {
        InitSequence(apFilename);
    }
    else {
        Warning("ANIMATION: Could not create ControllerManager Sequence for \"%s\".\r\n", pFilename);
    }
}

void KFModel::InitSequence(const char* apFilename) {
    if (spSequence.m_pObject) {
        // The name is deliberately held in a non-const char* so that the C++ library's char* strchr overload is called
        // rather than the C strchr.
        char* pstart = const_cast<char*>((&spSequence.m_pObject->m_kName)->GetHandleString());
        if (pstart) {
            char* pdash = std::strchr(pstart, '_');
            if (pdash) {
                char apDest[268];
                ourMemcpy(apDest, pstart, pdash - pstart);
                apDest[pdash - pstart] = 0;
                NiFixedString apString(static_cast<const char*>(apDest));
                NiFixedString* kName = &apString;
                spSequence.m_pObject->SetName(*kName);
            }
        }
        // A KFModel's sequence is always a BSAnimGroupSequence (CreateSequenceFromFile wraps it in one); the field
        // holds it as an NiControllerSequence.
        spAnimGroup.operator=(static_cast<TESAnimGroup*>(
            BSAnimGroupSequence::InitAnimGroup(static_cast<BSAnimGroupSequence*>(spSequence.m_pObject), apFilename)
        ));
        if (NiObject::IsKindOf_0(&BSAnimGroupSequence::ms_RTTI, spSequence.m_pObject)) {
            static_cast<BSAnimGroupSequence*>(GetSequence())->BSAnimGroupSequence::SetAnimGroup(spAnimGroup.m_pObject);
        }
        NiFixedString v5(apFilename);
        NiFixedString* v2 = &v5;
        spSequence.m_pObject->SetName(*v2);
    }
}

InterfacedClass::InterfacedClass() {}

InterfacedClass::~InterfacedClass() {}

// The calling thread's interface for apInterfacedClass: the next table slot, the interface it is handed, its thread id
// and the thread's TLS value.
InterfacedClass::BaseInterface* ThreadSpecificInterfaceManager::AddInterface(InterfacedClass* apInterfacedClass) {
    InterfacedClass::BaseInterface* pInterface = nullptr;
    uint32_t uiThread = (uint32_t)(NiAtomicIncrement(const_cast<int32_t&>(uiThreadCount)) - 1);
    if (uiThread < uiMaxThreads) {
        pInterface = apInterfacedClass->AllocateInterface(uiThread);
        pInterfaces[uiThread].pInterface = pInterface;
        pInterfaces[uiThread].iThreadID = BSGetCurrentThreadID();
        SetTlsValue(uiTLSIndex, pInterface);
    }
    return pInterface;
}

// The lock-free queues and maps pass node pointers to CompareAndSwapEx.
bool CompareAndSwapEx(void* volatile* apDestination, void* apExchange, void* apCompare) {
    bool v4 = BSAtomicCompareExchange(apDestination, apCompare, apExchange) == apCompare;
    InitializationFlow();
    return v4;
}

QueuedReplacementKF::QueuedReplacementKF(
    const char* apFileName,
    IO_TASK_PRIORITY aePriority,
    Animation* apAnim,
    QueuedReplacementKFList* apOwner
)
  : QueuedKF(apFileName, aePriority), pAnim(apAnim), pOwner(apOwner) {}

QueuedReplacementKF::~QueuedReplacementKF() {}

// A loaded replacement goes to the post-processing queue, counted by its owner list, unless it was cancelled.
void QueuedReplacementKF::Finish() {
    QueuedKF::Finish();
    if ((spKFModel.operator->()) && (!IOTask::IsCancelled())) {
        if (pOwner) {
            pOwner->AddPostProcessingChild();
        }
        IOTask::AddToPostProcessQueue();
    }
}

void QueuedReplacementKF::PostProcess() {
    if (!IOTask::IsCancelled()) {
        pAnim->QueueReplacementAnimation(spKFModel.operator->());
        if (pOwner) {
            pOwner->NotifyChildFinishedPostProcessing();
        }
    }
}

bool QueuedReplacementKF::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("Replacement KF"));
}

QueuedReplacementKFList::QueuedReplacementKFList(Animation* apAnim, IO_TASK_PRIORITY aePriority)
  : QueuedFile(aePriority), pAnim(apAnim), iPostProcessingChildCount(0), iPostProcessedChildCount(0) {}

QueuedReplacementKFList::~QueuedReplacementKFList() {}

void QueuedReplacementKFList::CheckFinished() {
    QueuedFile::CheckFinished();
}

void QueuedReplacementKFList::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    pModelLoader->RemoveReplacementKFListFromMap(pAnim);
}

void QueuedReplacementKFList::AddPostProcessingChild() {
    ++iPostProcessingChildCount;
}

void QueuedReplacementKFList::NotifyChildFinishedPostProcessing() {
    ++iPostProcessedChildCount;
    if ((IOTask::IsTaskFinished() && QueuedFile::GetAllChildrenFinished()) &&
        (iPostProcessingChildCount == iPostProcessedChildCount)) {
        pModelLoader->RemoveReplacementKFListFromMap(pAnim);
    }
}

void QueuedReplacementKFList::operator delete(void* apMemory) {
    ourFree(apMemory);
}

QueuedTreeBillboard::QueuedTreeBillboard(
    const char* apFileName,
    IO_TASK_PRIORITY aePriority,
    TREE_BILLBOARD_DATA* apBillboardData
)
  : QueuedTexture(apFileName, aePriority), pBillboardData(apBillboardData) {}

// The billboard is built in PostProcess, on the main thread.
void QueuedTreeBillboard::Finish() {
    QueuedTexture::Finish();
    AddToPostProcessQueue();
}

bool QueuedTreeBillboard::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("Tree Billboard"));
}

// Builds the billboard through the virtual CreateBillboard (slot 0x30).
void QueuedTreeBillboard::PostProcess() {
    QueuedTreeBillboard* pThis = this;
    pThis->CreateBillboard();
}

QueuedTreeBillboard::~QueuedTreeBillboard() {
    if (pBillboardData) {
        delete pBillboardData;
    }
}

void QueuedTreeBillboard::CreateBillboard() {
    NiBillboardNode* v3 = static_cast<NiBillboardNode*>(pBillboardData->pTree->BuildDistant3D(true));
    if (v3) {
        DistantLODShaderProperty::DistantLODParam apParam;
        apParam.iBaseObjectID = pBillboardData->pTree->iFormID;
        apParam.p3DData = v3;
        apParam.pcGeomFilename = nullptr;
        DistantLODShaderProperty::AddDistantLOD(
            pBillboardData->iCellChunk, pBillboardData->iCellKey, pBillboardData->pInstancedNode, &apParam,
            pBillboardData->pLocArray, pBillboardData->pColorArray, pBillboardData->iArraySize
        );
        if (apParam.p3DData) {
            delete apParam.p3DData;
        }
    }
}

QueuedTreeModel::QueuedTreeModel(
    TESObjectREFR* apRef,
    TESObjectTREE* apTree,
    IO_TASK_PRIORITY aePriority,
    ENUM_LOD_MULT aeLODFadeMult
)
  : QueuedModel(apTree ? static_cast<TESModelTree*>(apTree) : nullptr, aePriority, aeLODFadeMult, true, false),
    pRef(apRef), pTree(apTree) {
    eContext = MEM_CONTEXT_TREEART;
}

void QueuedTreeModel::Run() {
    AutoMemContext apMemory(
        MEM_CONTEXT_TREEART, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ModelLoader.cpp", 1142
    );
    BSTreeNode* a3 = pTreeManager->CreateTreeForReference(pRef, pTree);
    pTreeManager->AddBackgroundLoadedTree(pRef, a3);
}

void QueuedTreeModel::Finish() {
    QueuedFile::Finish();
}

void QueuedTreeModel::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
    if (((int32_t)aePreviousState) >= 4) {
        pTreeManager->RemoveBackgroundLoadedTree(pRef);
    }
}

bool QueuedTreeModel::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("tree model"));
}

void QueuedTreeModel::QueueMe() {
    if (*pTree->GetTextureName()) {
        char apOut[264];
        char Dst[268];
        const char* v1 = pTree->GetDefaultPath();
        BSstrcpy(Dst, 260, v1);
        BSstrcat(Dst, 260, pTree->GetTextureName());
        StandardizeTexturePath(Dst, apOut, 260u);
        IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
        pModelLoader->QueueTexture(apOut, IOTaskPriority, this);
    }
    QueuedModel::QueueMe();
}

QueuedTreeModel::~QueuedTreeModel() {}

// The background loading thread sees the 3D it is building for its reference.
NiNode* TESObjectREFR::Get3D() {
    if (this == GetThreadBackgroundLoadingRef()) {
        return GetThreadBackgroundLoading3D();
    }
    if (pLoadedData) {
        return pLoadedData->m_spData3D.m_pObject;
    }
    return nullptr;
}

TESObjectREFR::ReferenceData* TESObjectREFR::GetData() {
    return &data;
}

// Publishes the reference and the 3D the background loading thread is building, so Get3D on this thread sees it.
void TESObjectREFR::SetBackgroundLoadingRef(TESObjectREFR* apRef, NiAVObject* ap3D) {
    GetThreadBackgroundLoadingRef() = apRef;
    // The thread's 3D slot is a NiNode*; the pointer given is stored unchanged.
    GetThreadBackgroundLoading3D() = static_cast<NiNode*>(ap3D);
}

TESObjectREFR*& TESObjectREFR::GetThreadBackgroundLoadingRef() {
    static thread_local TESObjectREFR* pBackgroundLoadingRef = nullptr;
    return pBackgroundLoadingRef;
}

NiNode*& TESObjectREFR::GetThreadBackgroundLoading3D() {
    static thread_local NiNode* pBackgroundLoading3D = nullptr;
    return pBackgroundLoading3D;
}

// Clears what SetBackgroundLoadingRef published for this thread: the reference first, then its 3D.
void TESObjectREFR::ClearBackgroundLoadingRef() {
    GetThreadBackgroundLoadingRef() = nullptr;
    GetThreadBackgroundLoading3D() = nullptr;
}

NiAVObjectPalette* NiControllerManager::GetObjectPalette() {
    return m_spObjectPalette.operator->();
}

void NiControllerSequence::SetName(const NiFixedString& kName) {
    m_kName.Assign(kName);
}

PRIORITY_STATE BSDegradePriority::QState() {
    return static_cast<PRIORITY_STATE>(cState);
}

bool BSEntryMap::GetAt(const BSFileEntry* apEntry, NiPointer<NiTexture>& arTexture) {
    uint32_t uiIndex = KeyToHashIndex(apEntry);
    return GetAt(uiIndex, apEntry, arTexture);
}

// The key's address is the hash (as Gamebryo's NiTPointerMap hashes pointer keys).
uint32_t BSEntryMap::KeyToHashIndex(const BSFileEntry* apKey) {
    return static_cast<uint32_t>((reinterpret_cast<uintptr_t>(apKey) >> 4) % 1001);
}

const char* NiFixedString::GetHandleString() const {
    return convert_to_dword(const_cast<NiFixedString*>(this));
}

NiFixedString& NiFixedString::Assign(const NiFixedString& kString) {
    if (m_kHandle != kString.m_kHandle) {
        NiGlobalStringTable::GlobalStringHandle kHandle = kString.m_kHandle;
        NiGlobalStringTable::IncHandleRefCount(kHandle);
        NiGlobalStringTable::DecHandleRefCount(m_kHandle);
        m_kHandle = kHandle;
    }
    return *this;
}

NiGeometryData* NiGeometry::GetModelData() const {
    return m_spModelData;
}

void NiGeometryData::SetCompressFlags(uint32_t uiCompressFlags) {
    m_ucCompressFlags = static_cast<uint8_t>(uiCompressFlags);
}

NiObject* NiObject::DynamicCastMember(const NiRTTI* pkRTTI) const {
    return IsKindOfMember(pkRTTI) ? const_cast<NiObject*>(this) : nullptr;
}

bool NiObject::IsExactKindOf(const NiRTTI* pkRTTI) const {
    return GetRTTI() == pkRTTI;
}

// Static NiObject::DynamicCast: null for a null object, else the member DynamicCast.
NiObject* NiObject::DynamicCastStatic(const NiRTTI* pkRTTI, const NiObject* pkObject) {
    if (!pkObject) {
        return nullptr;
    }
    return pkObject->DynamicCastMember(pkRTTI);
}

NiShadeProperty* NiPropertyState::GetShade() {
    return static_cast<NiShadeProperty*>(m_aspProps[NiProperty::SHADE].m_pObject);
}

BSDegradePriority* NiTexture::RendererData::QDegradePriority() {
    return &DegradePriorityRange;
}

int32_t& NiTexture::RendererData::GetThreadLoadMethod() {
    static thread_local int32_t iLoadMethod = 0;
    return iLoadMethod;
}

// The degrade state of the texture's renderer data, 0 without one.
PRIORITY_STATE NiTexture::QState() {
    if (!GetRendererData()) {
        return PS_NONE;
    }
    return GetRendererData()->QDegradePriority()->QState();
}

NiTexture::RendererData* NiTexture::GetRendererData() {
    return m_pkRendererData;
}

void NiTexture::SetLoadMethod(uint32_t aiLoadMethod) {
    RendererData::GetThreadLoadMethod() = static_cast<int32_t>(aiLoadMethod);
}

uint32_t NiTexture::QLoadMethod() {
    return static_cast<uint32_t>(RendererData::GetThreadLoadMethod());
}

NiUpdateData::NiUpdateData(float afTime, bool abUpdateControllers, bool abParallelUpdate)
  : fTime(afTime), bUpdateControllers(abUpdateControllers), bParallelUpdate(abParallelUpdate), bFoundParticles(false),
    bFoundMorphController(false), bSceneGraphChange(false) {}

NiVertexColorProperty* NiVertexColorProperty::GetDefault() {
    return (&ms_spDefault)->operator->();
}

NiZBufferProperty* NiZBufferProperty::GetDefault() {
    return (&ms_spDefault)->operator->();
}

// The empty string's handle has no reference count. The count is raw memory, the untyped word GetRealBufferStart
// returns, incremented through the int32_t NiAtomicIncrement.
void NiGlobalStringTable::IncHandleRefCount(GlobalStringHandle& arHandle) {
    if (arHandle != NULL_STRING) {
        const char* pcBuffer = GetRealBufferStart(arHandle);
        NiAtomicIncrement(*reinterpret_cast<int32_t*>(const_cast<char*>(pcBuffer)));
    }
}

void NiStream::SaveLinkID(const NiObject* pkObject) {
    uint32_t uiLinkID = GetLinkIDFromObject(pkObject);
    NiStreamSaveBinary(*this, uiLinkID);
}

uint32_t NiStream::GetObjectCount() {
    return m_kTopObjects.GetSize();
}

NiObject* NiStream::GetObjectAt(uint32_t i) {
    NiPointer<NiObject>& spObject = m_kTopObjects.GetAt(i);
    return spObject.operator->();
}

NiTimeController* NiTimeController::GetNext() {
    return m_spNext;
}
