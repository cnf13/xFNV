#include "Fallout Shared/TESSaveLoadGame.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSSystemFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/CreatedReferenceData.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/MovedReferenceData.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/ReferenceData.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/SaveDataDebugInfo.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
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
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicBallProjectile.h"
#include "Fallout/Magic/MagicBoltProjectile.h"
#include "Fallout/Magic/MagicFogProjectile.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/ScreenShot.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <vector>

// Never read.
static bool bRunSavedCreatedBaseObjectsTest;

// The game settings this file defines, with their IDs and values, in initialization order.
BSCriticalSection kLoadLock;
SettingT<INISettingCollection> bDebugSaveBuffer("bDebugSaveBuffer:Debug", false);

ChangeData::ChangeData() : iFlags(0), pBuffer(nullptr) {}

ChangeData::~ChangeData() {
    if (pBuffer) {
        ourFree(pBuffer);
    }
}

void ChangeData::RemoveChange(uint32_t aiChange) {
    if (!pBuffer) {
        iFlags &= ~aiChange;
    }
}

void* ChangeData::GetBuffer() {
    return pBuffer;
}

uint32_t ChangeData::GetFlags() {
    return iFlags;
}

void ChangeData::SetBuffer(void* apBuffer) {
    pBuffer = apBuffer;
}

void ChangeData::AddChange(uint32_t aiChange) {
    if (!pBuffer) {
        iFlags |= aiChange;
    }
}

void ChangeData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

LoadFormHeader::LoadFormHeader(const SaveBufferHeader* apBufferHeader, uint32_t aiFormID, uint32_t aiFlags)
  : iFormID(aiFormID), cFormType(apBufferHeader->cFormType), iFlags(aiFlags), cVersion(apBufferHeader->cVersion),
    iSize(apBufferHeader->iSize) {}

FormAndFlags::FormAndFlags(TESForm* apForm, uint32_t aiFlags, uint32_t aiOldFlags, uint8_t acVersion)
  : pForm(apForm), iFlags(aiFlags), iOldFlags(aiOldFlags), cVersion(acVersion) {}

NumericIDBufferMap::NumericIDBufferMap() : NiTPointerMap<uint32_t, void*>(0x25u) {}

// The buffers are plain blocks, freed without a destructor, as the game does.
NumericIDBufferMap::~NumericIDBufferMap() {
    NiTMapItem<uint32_t, void*>* pos = GetFirstPos();
    while (pos) {
        void* pBlock = nullptr;
        uint32_t uiID = 0;
        GetNext(pos, uiID, pBlock);
        if (pBlock) {
            ourFree(pBlock);
        }
    }
    RemoveAll();
}

void NumericIDBufferMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

InteriorCellNewReferencesMap::InteriorCellNewReferencesMap() : NiTPointerMap<uint32_t, BSSimpleList<uint32_t>*>(37u) {}

InteriorCellNewReferencesMap::~InteriorCellNewReferencesMap() {
    NiTMapItem<uint32_t, BSSimpleList<uint32_t>*>* pos = GetFirstPos();
    while (pos) {
        BSSimpleList<uint32_t>* pList = nullptr;
        uint32_t uiCellID = 0;
        GetNext(pos, uiCellID, pList);
        delete pList;
    }
    RemoveAll();
}

void InteriorCellNewReferencesMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ExteriorCellNewReferencesMap::ExteriorCellNewReferencesMap()
  : NiTPointerMap<uint32_t, BSSimpleList<ExteriorCellReferenceData*>*>(37u) {}

// The reference data are plain blocks, freed without a destructor, as the game does.
ExteriorCellNewReferencesMap::~ExteriorCellNewReferencesMap() {
    NiTMapItem<uint32_t, BSSimpleList<ExteriorCellReferenceData*>*>* pos = GetFirstPos();
    while (pos) {
        BSSimpleList<ExteriorCellReferenceData*>* pList = nullptr;
        uint32_t uiCellKey = 0;
        GetNext(pos, uiCellKey, pList);
        for (BSSimpleList<ExteriorCellReferenceData*>* pNode = pList; pNode; pNode = pNode->m_pkNext) {
            if (pNode->m_item) {
                ourFree(pNode->m_item);
            }
        }
        delete pList;
    }
    RemoveAll();
}

void ExteriorCellNewReferencesMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

SaveGameFile::SaveGameFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiBufferSize)
  : BSFile(apName, aeMode, aiBufferSize, false), bHasFileTime(false) {}

SaveGameFile::~SaveGameFile() {}

SaveGameFile* SaveGameFile::SetFileTime(_FILETIME aFileTime) {
    FileTime = aFileTime;
    bHasFileTime = true;
    return this;
}

_FILETIME SaveGameFile::GetFileTime() {
    return FileTime;
}

bool SaveGameFile::GetHasFileTime() {
    return (bool)((uint8_t)bHasFileTime);
}

void SaveGameFile::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESSaveLoadGame::TESSaveLoadGame()
  : m_pNewChanges(nullptr), m_pInteriorCellMap(nullptr), m_pExteriorCellMap(nullptr), m_pBuffer(nullptr),
    m_iSaveLoadStates(0), m_bAllowChanges(false), m_pInitArray(nullptr), m_MovedReferencesList(),
    m_CreatedBaseObjectsList(), m_DeferredDeleteList(), m_DeferredInitActorPackageList(), m_pSaveLoadStats(nullptr),
    m_pFileIndexArray(nullptr), m_iQueuedRemoveChanges(0), m_QueuedFormList(), m_pSaveGameList(nullptr),
    m_cMajorVersion(0), m_cMinorVersion(0), m_bUseNumericIDArray(true), m_pCurrentlyLoadingFormHeader(nullptr),
    m_pCurrentlySavingFormHeader(nullptr), m_iNextSaveNumber(0), m_iFileStartPosition(0), m_iSimulationFileSize(0),
    m_OriginalSaveTime(), m_iOriginalSaveVersion(0), m_bReconstructedAllObjects(false), m_bLoadingFromMainMenu(false),
    m_bAutosaveDisabledDueToLackOfSpace(false), m_bQuickloading(false), m_pInitItemArray(nullptr),
    m_pMostRecentSaveGame(nullptr) {
    AutoMemContext kMemContext(MEM_CONTEXT_SAVELOAD);
    pSaveLoadGameOLD = this;
    m_pChanges = new ChangesMap();
    m_pInteriorCellMap = new InteriorCellNewReferencesMap();
    m_pExteriorCellMap = new ExteriorCellNewReferencesMap();
    m_pCreatedExteriorCells = new ExteriorCellNewReferencesMap();
    m_pAnimationMap = new NumericIDBufferMap();
    m_pAttachedAnimationMap = new NumericIDBufferMap();
    m_pCharControllerMap = new NumericIDBufferMap();
    m_pHavokDataMap = new NumericIDBufferMap();
    m_pNumericIDArray = new NiTLargePrimitiveArray<uint32_t>(100u, 50u);
    uint32_t uiNoID = 0;
    m_pNumericIDArray->SetAtGrow(0, uiNoID);
    m_pWorldSpaceIDArray = new NiTLargePrimitiveArray<uint32_t>(5u, 1u);
    uint32_t uiNoWorldSpaceID = 0;
    m_pWorldSpaceIDArray->SetAtGrow(0, uiNoWorldSpaceID);
    ResetCurrentVersion();
    ResetVersions();
}

TESSaveLoadGame::~TESSaveLoadGame() {
    delete m_pChanges;
    delete m_pInteriorCellMap;
    delete m_pExteriorCellMap;
    delete m_pCreatedExteriorCells;
    if (m_pFileIndexArray) {
        ourFree(m_pFileIndexArray);
    }
    delete m_pAnimationMap;
    delete m_pAttachedAnimationMap;
    delete m_pCharControllerMap;
    delete m_pHavokDataMap;
    if (m_pNumericIDArray) {
        m_pNumericIDArray->RemoveAll();
        delete m_pNumericIDArray;
    }
    if (m_pWorldSpaceIDArray) {
        m_pWorldSpaceIDArray->RemoveAll();
        delete m_pWorldSpaceIDArray;
    }
    if (m_pMostRecentSaveGame) {
        ourFree(m_pMostRecentSaveGame);
    }
    if (m_pSaveGameList) {
        while (!m_pSaveGameList->IsEmpty()) {
            SaveGameFile* pFile = m_pSaveGameList->m_item;
            m_pSaveGameList->Remove(pFile);
            delete pFile;
        }
        delete m_pSaveGameList;
    }
    m_MovedReferencesList.RemoveAll();
    m_CreatedBaseObjectsList.RemoveAll();
}

void TESSaveLoadGame::BuildChangesString(
    char* apString,
    TESForm* apForm,
    uint32_t aiFlags,
    uint8_t acFormType,
    bool abVerbose
) {
    char Src[52];
    BSstrcpy(apString, 500, "");
    if (apForm) {
        aiFlags = CheckFlags(apForm, aiFlags);
    }
    uint16_t ibaseSaveSize = 0;
    if ((!abVerbose) && (apForm)) {
        ibaseSaveSize = apForm->GetSaveSize(0);
    }

    TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(apForm);
    MobileObject* pmob = dynamic_cast<MobileObject*>(apForm);
    if (pmob) {
        PROCESS_TYPE type = PROCESS_TYPE_NONE;
        if (pmob->pCurrentProcess) {
            type = pmob->pCurrentProcess->eLevel;
        }
        switch (type) {
            case PROCESS_TYPE_NONE: {
                if (abVerbose) {
                    BSsprintf(Src, 50, "Process Level: None\r\n");
                }
                else {
                    BSsprintf(Src, 50, "None(%i)", ibaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_HIGH: {
                if (abVerbose) {
                    BSsprintf(Src, 50, "Process Level: High\r\n");
                }
                else {
                    BSsprintf(Src, 50, "High(%i)", ibaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_MIDDLE_HIGH: {
                if (abVerbose) {
                    BSsprintf(Src, 50, "Process Level: Middle High\r\n");
                }
                else {
                    BSsprintf(Src, 50, "Mid High(%i)", ibaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_MIDDLE_LOW: {
                if (abVerbose) {
                    BSsprintf(Src, 50, "Process Level: Middle Low\r\n");
                }
                else {
                    BSsprintf(Src, 50, "Mid Low(%i)", ibaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_LOW: {
                if (abVerbose) {
                    BSsprintf(Src, 50, "Process Level: Low\r\n");
                }
                else {
                    BSsprintf(Src, 50, "Low(%i)", ibaseSaveSize);
                }
                break;
            }
            default: {
                break;
            }
        }
        if (!abVerbose) {
            while (BSstrlen(Src) < 25) {
                BSstrcat(Src, 50, " ");
            }
            BSstrcat(Src, 50, " ");
        }
        BSstrcat(apString, 500, Src);
    }
    else {
        if ((ibaseSaveSize) && (!abVerbose)) {
            BSsprintf(Src, 50, "Base(%i)", ibaseSaveSize);
            while (BSstrlen(Src) < 25) {
                BSstrcat(Src, 50, " ");
            }
            BSstrcat(Src, 50, " ");
            BSstrcat(apString, 500, Src);
        }
    }
    if ((aiFlags & 1) != 0) {
        if (abVerbose) {
            BSstrcat(apString, 500, "CHANGE_FORM_FLAGS\r\n");
        }
        else {
            int32_t v73;
            if (apForm) {
                uint16_t v6 = apForm->GetSaveSize(1u);
                v73 = v6 - ibaseSaveSize;
            }
            else {
                v73 = 0;
            }
            BSsprintf(Src, 50, "Form Flags(%i)", v73);
            while (BSstrlen(Src) < 25) {
                BSstrcat(Src, 50, " ");
            }
            BSstrcat(Src, 50, " ");
            BSstrcat(apString, 500, Src);
        }
    }
    switch (acFormType) {
        case 42:
        case 43: {
            if ((aiFlags & 2) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_ACTOR_BASE_DATA\r\n");
                }
                else {
                    int32_t v72;
                    if (apForm) {
                        uint16_t v7 = apForm->GetSaveSize(2u);
                        v72 = v7 - ibaseSaveSize;
                    }
                    else {
                        v72 = 0;
                    }
                    BSsprintf(Src, 50, "Base Data(%i)", v72);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 16) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_ACTOR_BASE_SPELLLIST\r\n");
                }
                else {
                    int32_t v71;
                    if (apForm) {
                        uint16_t v8 = apForm->GetSaveSize(16u);
                        v71 = v8 - ibaseSaveSize;
                    }
                    else {
                        v71 = 0;
                    }
                    BSsprintf(Src, 50, "Spell List(%i)", v71);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 32) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_ACTOR_BASE_FULLNAME\r\n");
                }
                else {
                    int32_t v70;
                    if (apForm) {
                        uint16_t v9 = apForm->GetSaveSize(32u);
                        v70 = v9 - ibaseSaveSize;
                    }
                    else {
                        v70 = 0;
                    }
                    BSsprintf(Src, 50, "Full Name(%i)", v70);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if (acFormType == 42) {
                if ((aiFlags & 512) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_NPC_SKILLS\r\n");
                    }
                    else {
                        int32_t v69;
                        if (apForm) {
                            uint16_t v10 = apForm->GetSaveSize(512u);
                            v69 = v10 - ibaseSaveSize;
                        }
                        else {
                            v69 = 0;
                        }
                        BSsprintf(Src, 50, "Skills(%i)", v69);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
            }
            else {
                if (acFormType == 43 && (aiFlags & 512) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_CREATURE_SKILLS\r\n");
                    }
                    else {
                        int32_t v68;
                        if (apForm) {
                            uint16_t v11 = apForm->GetSaveSize(512u);
                            v68 = v11 - ibaseSaveSize;
                        }
                        else {
                            v68 = 0;
                        }
                        BSsprintf(Src, 50, "Skills(%i)", v68);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
            }
            break;
        }
        case 71: {
            if ((aiFlags & 2) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_QUEST_FLAGS\r\n");
                }
                else {
                    int32_t v67;
                    if (apForm) {
                        uint16_t v12 = apForm->GetSaveSize(2u);
                        v67 = v12 - ibaseSaveSize;
                    }
                    else {
                        v67 = 0;
                    }
                    BSsprintf(Src, 50, "Quest Flags(%i)", v67);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_QUEST_STAGES\r\n");
                }
                else {
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_QUEST_SCRIPT\r\n");
                }
                else {
                    TESQuest* pquest = dynamic_cast<TESQuest*>(apForm);
                    if (pquest && !pquest->GetScriptLocals()) {
                        if (apForm) {
                            BSsprintf(Src, 50, "Quest Script(No longer has script locals)");
                        }
                    }
                    else {
                        int32_t v66;
                        if (apForm) {
                            uint16_t v13 = apForm->GetSaveSize(CHANGE_QUEST_SCRIPT);
                            v66 = v13 - ibaseSaveSize;
                        }
                        else {
                            v66 = 0;
                        }
                        BSsprintf(Src, 50, "Quest Script(%i)", v66);
                    }
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            break;
        }
        case 70: {
            if ((aiFlags & CHANGE_TOPIC_SAIDONCE) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_TOPIC_SAIDONCE\r\n");
                }
                else {
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            break;
        }
        case 73: {
            if ((aiFlags & CHANGE_PACKAGE_NEVER_RUN) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_PACKAGE_NEVER_RUN\r\n");
                }
                else {
                    int32_t v65;
                    if (apForm) {
                        uint16_t v14 = apForm->GetSaveSize(CHANGE_PACKAGE_NEVER_RUN);
                        v65 = v14 - ibaseSaveSize;
                    }
                    else {
                        v65 = 0;
                    }
                    BSsprintf(Src, 50, "Never Run Flag(%i)", v65);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & CHANGE_PACKAGE_WAITING) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_PACKAGE_WAITING\r\n");
                }
                else {
                    int32_t v64;
                    if (apForm) {
                        uint16_t v15 = apForm->GetSaveSize(CHANGE_PACKAGE_WAITING);
                        v64 = v15 - ibaseSaveSize;
                    }
                    else {
                        v64 = 0;
                    }
                    BSsprintf(Src, 50, "Waiting Flag(%i)", v64);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            break;
        }
        case 57: {
            if ((aiFlags & 2) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_FLAGS\r\n");
                }
                else {
                    int32_t v63;
                    if (apForm) {
                        uint16_t v16 = apForm->GetSaveSize(2u);
                        v63 = v16 - ibaseSaveSize;
                    }
                    else {
                        v63 = 0;
                    }
                    BSsprintf(Src, 50, "Cell Flags(%i)", v63);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_SEENDATA\r\n");
                }
                else {
                    int32_t v62;
                    if (apForm) {
                        uint16_t v17 = apForm->GetSaveSize(CHANGE_CELL_SEENDATA);
                        v62 = v17 - ibaseSaveSize;
                    }
                    else {
                        v62 = 0;
                    }
                    BSsprintf(Src, 50, "Seen Data(%i)", v62);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_DETACHTIME\r\n");
                }
                else {
                    int32_t v61;
                    if (apForm) {
                        uint16_t v18 = apForm->GetSaveSize(CHANGE_CELL_DETACHTIME);
                        v61 = v18 - ibaseSaveSize;
                    }
                    else {
                        v61 = 0;
                    }
                    BSsprintf(Src, 50, "Detach Time(%i)", v61);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 8) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_OWNERSHIP\r\n");
                }
                else {
                    int32_t v60;
                    if (apForm) {
                        uint16_t v19 = apForm->GetSaveSize(8u);
                        v60 = v19 - ibaseSaveSize;
                    }
                    else {
                        v60 = 0;
                    }
                    BSsprintf(Src, 50, "Ownership(%i)", v60);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 4) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_FULLNAME\r\n");
                }
                else {
                    int32_t v59;
                    if (apForm) {
                        uint16_t v20 = apForm->GetSaveSize(4u);
                        v59 = v20 - ibaseSaveSize;
                    }
                    else {
                        v59 = 0;
                    }
                    BSsprintf(Src, 50, "Full Name(%i)", v59);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            break;
        }
        case 8: {
            if ((aiFlags & 2) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_FACTION_FLAGS\r\n");
                }
                else {
                    int32_t v58;
                    if (apForm) {
                        uint16_t v21 = apForm->GetSaveSize(2u);
                        v58 = v21 - ibaseSaveSize;
                    }
                    else {
                        v58 = 0;
                    }
                    BSsprintf(Src, 50, "Faction Flags(%i)", v58);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 4) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_FACTION_REACTIONS\r\n");
                }
                else {
                    int32_t v57;
                    if (apForm) {
                        uint16_t v22 = apForm->GetSaveSize(4u);
                        v57 = v22 - ibaseSaveSize;
                    }
                    else {
                        v57 = 0;
                    }
                    BSsprintf(Src, 50, "Faction Reactions(%i)", v57);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            break;
        }
        case 25: {
            if ((aiFlags & 32) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_BOOK_TEACHES_SKILL\r\n");
                }
                else {
                    int32_t v56;
                    if (apForm) {
                        uint16_t v23 = apForm->GetSaveSize(32u);
                        v56 = v23 - ibaseSaveSize;
                    }
                    else {
                        v56 = 0;
                    }
                    BSsprintf(Src, 50, "Book Skill(%i)", v56);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            break;
        }
        case 58:
        case 59:
        case 60:
        case 61:
        case 62:
        case 63:
        case 105:
        case 64: {
            if ((aiFlags & 2) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_MOVE\r\n");
                }
                else {
                    int32_t v55;
                    if (apForm) {
                        v55 = static_cast<uint16_t>(GetInitialDataSaveSize(apForm, 2));
                    }
                    else {
                        v55 = 0;
                    }
                    BSsprintf(Src, 50, "Moved(%i)", v55);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 4) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_HAVOK_MOVE\r\n");
                }
                else {
                    if ((aiFlags & 2) != 0) {
                        int32_t v54;
                        if (apForm) {
                            uint16_t v24 = apForm->GetSaveSize(4u);
                            v54 = v24 - ibaseSaveSize;
                        }
                        else {
                            v54 = 0;
                        }
                        BSsprintf(Src, 50, "Havok Moved(%i)", v54);
                    }
                    else {
                        int32_t v53;
                        if (apForm) {
                            int32_t v25 = static_cast<uint16_t>(GetInitialDataSaveSize(apForm, 4));
                            uint16_t v26 = apForm->GetSaveSize(4u);
                            v53 = v26 + v25 - ibaseSaveSize;
                        }
                        else {
                            v53 = 0;
                        }
                        BSsprintf(Src, 50, "Havok Moved(%i)", v53);
                    }
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_ANIMATION\r\n");
                }
                else {
                    int32_t v52;
                    if (apForm) {
                        uint16_t v27 = apForm->GetSaveSize(CHANGE_REFR_ANIMATION);
                        v52 = v27 - ibaseSaveSize;
                    }
                    else {
                        v52 = 0;
                    }
                    BSsprintf(Src, 50, "Animation(%i)", v52);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 16) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_SCALE\r\n");
                }
                else {
                    int32_t v51;
                    if (apForm) {
                        uint16_t v28 = apForm->GetSaveSize(16u);
                        v51 = v28 - ibaseSaveSize;
                    }
                    else {
                        v51 = 0;
                    }
                    BSsprintf(Src, 50, "Scale(%i)", v51);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if ((aiFlags & 32) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_INVENTORY\r\n");
                }
                else {
                    int32_t v50;
                    if (apForm) {
                        uint16_t v29 = apForm->GetSaveSize(32u);
                        v50 = v29 - ibaseSaveSize;
                    }
                    else {
                        v50 = 0;
                    }
                    BSsprintf(Src, 50, "Inventory(%i)", v50);
                    while (BSstrlen(Src) < 25) {
                        BSstrcat(Src, 50, " ");
                    }
                    BSstrcat(Src, 50, " ");
                    BSstrcat(apString, 500, Src);
                }
            }
            if (acFormType == 59 || acFormType == 60) {
                if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_DAMAGE_MODIFIERS\r\n");
                    }
                    else {
                        int32_t v49;
                        if (apForm) {
                            uint16_t v30 = apForm->GetSaveSize(CHANGE_ACTOR_DAMAGE_MODIFIERS);
                            v49 = v30 - ibaseSaveSize;
                        }
                        else {
                            v49 = 0;
                        }
                        BSsprintf(Src, 50, "Damage Modifiers(%i)", v49);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_PERMANENT_MODIFIERS\r\n");
                    }
                    else {
                        int32_t v48;
                        if (apForm) {
                            uint16_t v31 = apForm->GetSaveSize(CHANGE_ACTOR_PERMANENT_MODIFIERS);
                            v48 = v31 - ibaseSaveSize;
                        }
                        else {
                            v48 = 0;
                        }
                        BSsprintf(Src, 50, "Script Modifiers(%i)", v48);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_TEMP_MODIFIERS\r\n");
                    }
                    else {
                        int32_t v47;
                        if (apForm) {
                            uint16_t v32 = apForm->GetSaveSize(CHANGE_ACTOR_TEMP_MODIFIERS);
                            v47 = v32 - ibaseSaveSize;
                        }
                        else {
                            v47 = 0;
                        }
                        BSsprintf(Src, 50, "Temp Modifiers(%i)", v47);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_DISPOSITION_MODIFIERS\r\n");
                    }
                    else {
                        int32_t v46;
                        if (apForm) {
                            uint16_t v33 = apForm->GetSaveSize(CHANGE_ACTOR_DISPOSITION_MODIFIERS);
                            v46 = v33 - ibaseSaveSize;
                        }
                        else {
                            v46 = 0;
                        }
                        BSsprintf(Src, 50, "Disp Modifiers(%i)", v46);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & 1024) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_LIFESTATE\r\n");
                    }
                    else {
                        int32_t v44;
                        int32_t v45;
                        if (apForm) {
                            v45 = apForm->GetSaveSize(aiFlags & 4);
                        }
                        else {
                            v45 = 0;
                        }
                        if (apForm) {
                            v44 = apForm->GetSaveSize((aiFlags & 4) + 1024) - v45;
                        }
                        else {
                            v44 = 0;
                        }
                        BSsprintf(Src, 50, "Life State(%i)", v44);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
            }
            else {
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OBJECT_EXTRA_LOCK\r\n");
                    }
                    else {
                        int32_t v43;
                        if (apForm) {
                            uint16_t v34 = apForm->GetSaveSize(CHANGE_OBJECT_EXTRA_LOCK);
                            v43 = v34 - ibaseSaveSize;
                        }
                        else {
                            v43 = 0;
                        }
                        BSsprintf(Src, 50, "Lock(%i)", v43);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & CHANGE_OBJECT_EMPTY) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OBJECT_EMPTY\r\n");
                    }
                    else {
                        int32_t v42;
                        if (apForm) {
                            uint16_t v35 = apForm->GetSaveSize(CHANGE_OBJECT_EMPTY);
                            v42 = v35 - ibaseSaveSize;
                        }
                        else {
                            v42 = 0;
                        }
                        BSsprintf(Src, 50, "Empty Flag(%i)", v42);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((!pref || (pref->GetObjectReference() && pref->GetObjectReference()->GetFormType() == DOOR_ID)) &&
                    (aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_DOOR_EXTRA_TELEPORT\r\n");
                    }
                    else {
                        int32_t v41;
                        if (apForm) {
                            uint16_t v36 = apForm->GetSaveSize(CHANGE_DOOR_EXTRA_TELEPORT);
                            v41 = v36 - ibaseSaveSize;
                        }
                        else {
                            v41 = 0;
                        }
                        BSsprintf(Src, 50, "Teleport(%i)", v41);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & CHANGE_OBJECT_OPEN_STATE) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OPEN_STATE\r\n");
                    }
                    else {
                        int32_t v40;
                        if (apForm) {
                            uint16_t v37 = apForm->GetSaveSize(CHANGE_OBJECT_OPEN_STATE);
                            v40 = v37 - ibaseSaveSize;
                        }
                        else {
                            v40 = 0;
                        }
                        BSsprintf(Src, 50, "Open State(%i)", v40);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
                if ((aiFlags & CHANGE_OBJECT_OPEN_DEFAULT_STATE) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OPEN_DEFAULT_STATE\r\n");
                    }
                    else {
                        int32_t v39;
                        if (apForm) {
                            uint16_t v38 = apForm->GetSaveSize(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
                            v39 = v38 - ibaseSaveSize;
                        }
                        else {
                            v39 = 0;
                        }
                        BSsprintf(Src, 50, "Default Open State(%i)", v39);
                        while (BSstrlen(Src) < 25) {
                            BSstrcat(Src, 50, " ");
                        }
                        BSstrcat(Src, 50, " ");
                        BSstrcat(apString, 500, Src);
                    }
                }
            }
            break;
        }
    }
}

BSFile*
TESSaveLoadGame::PrepForLoadSave(BSFile* apFile, const char* asFileName, TESSaveLoadGame::ACTION_TYPE aeAction) {
    char Dst[264];
    BSFile* pNewFile;
    if (apFile) {
        BSstrcpy(Dst, 260, apFile->GetDevice());
    }
    else {
        char pName[264];
        if (asFileName) {
            BSstrcpy(pName, 260, asFileName);
        }
        else {
            GenerateSaveGameFileName(pName);
        }
        const char* v5 = sLocalSavePath.String();
        const void* CellOrCoords_0 = v5;
        BSsprintf(Dst, 260, "%s%s%s.ess", BSUtilities::GetPersonalPath(), CellOrCoords_0, pName);
        if (aeAction == TESSaveLoadGame::ACTION_SAVE) {
            char PathName[268];
            const char* PersonalPath = BSUtilities::GetPersonalPath();
            BSstrcpy(PathName, 260, PersonalPath);
            const char* v7 = sLocalSavePath.String();
            BSstrcat(PathName, 260, v7);
            std::error_code kError;
            std::filesystem::create_directory(platform::NativePath(PathName), kError);
            // The backup count is a constant 1, clamped to 10 as the game does.
            int32_t iBackupCount = 1;
            if (iBackupCount > 10) {
                iBackupCount = 10;
            }
            for (int32_t i = iBackupCount - 1; i >= 0; --i) {
                char FileName[264];
                char OldFilename[268];
                const char* v8 = BSUtilities::GetPersonalPath();
                BSstrcpy(OldFilename, 260, v8);
                const char* v9 = sLocalSavePath.String();
                BSstrcat(OldFilename, 260, v9);
                BSstrcat(OldFilename, 260, pName);
                for (int32_t j = 0; j < i; ++j) {
                    BSstrcat(OldFilename, 260, ".bak");
                }
                BSstrcpy(FileName, 260, OldFilename);
                BSstrcat(FileName, 260, ".bak");
                if (!i) {
                    BSstrcpy(OldFilename, 260, Dst);
                }
                if (FileFinder::Locate(OldFilename, nullptr, 0, 0xFFFFFFFFu)) {
                    if (FileFinder::Locate(FileName, nullptr, 0, 0xFFFFFFFFu)) {
                        std::filesystem::remove(platform::NativePath(FileName), kError);
                    }
                    rt::rename(OldFilename, FileName);
                }
            }
        }
    }
    switch (aeAction) {
        case TESSaveLoadGame::ACTION_SAVE: {
            if (apFile) {
                char Str[264];
                char* apSource = std::strrchr(apFile->GetDevice(), 92) + 1;
                BSstrcpy(Str, 260, apSource);
                uint32_t v20 = BSstrlen(Str);
                if (v20 > 4 && !_strnicmp(&Str[v20 - 4], ".ess", 4u)) {
                    Str[v20 - 4] = 0;
                }
                DeleteSaveGame(apFile, nullptr);
                if (!_strnicmp(Str, "Save ", 5u) || std::strstr(Str, "autosave")) {
                    pNewFile = PrepForLoadSave(nullptr, nullptr, TESSaveLoadGame::ACTION_SAVE);
                }
                else {
                    pNewFile = PrepForLoadSave(nullptr, Str, TESSaveLoadGame::ACTION_SAVE);
                }
            }
            else {
                BSFile* pNewFile_2;
                BSFile* apMemory = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
                if (apMemory) {
                    pNewFile_2 = ::new (apMemory) BSFile(Dst, NiFile::WRITE_ONLY, 0x20000u, false);
                }
                else {
                    pNewFile_2 = nullptr;
                }
                pNewFile = pNewFile_2;
            }
            break;
        }
        case TESSaveLoadGame::ACTION_LOAD: {
            BSFile* pNewFile_3;
            BSFile* v16 = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
            if (v16) {
                pNewFile_3 = ::new (v16) BSFile(Dst, NiFile::READ_ONLY, 0x20000u, false);
            }
            else {
                pNewFile_3 = nullptr;
            }
            pNewFile = pNewFile_3;
            pNewFile_3->Open(false, false);
            break;
        }
        case TESSaveLoadGame::ACTION_READ: {
            pNewFile = apFile;
            apFile->Open(false, false);
            break;
        }
        case TESSaveLoadGame::ACTION_DELETE: {
            pNewFile = apFile;
            break;
        }
        default: {
            pNewFile = apFile;
            break;
        }
    }
    return pNewFile;
}

void TESSaveLoadGame::CleanupLoadSave(BSFile* apFile, TESSaveLoadGame::ACTION_TYPE aeAction) {
    SaveGameFile* pSaveFile;
    if (apFile) {
        switch (aeAction) {
            case TESSaveLoadGame::ACTION_SAVE:
            case TESSaveLoadGame::ACTION_LOAD:
            case TESSaveLoadGame::ACTION_DELETE: {
                if (m_pSaveGameList) {
                    pSaveFile = static_cast<SaveGameFile*>(apFile);
                    m_pSaveGameList->Remove(pSaveFile);
                }
                delete apFile;
                break;
            }
            case TESSaveLoadGame::ACTION_READ: {
                apFile->Close();
                break;
            }
            default: {
                return;
            }
        }
    }
}

// Allocates under the save-load heap context; the buffer is kept in m_pBuffer and its address returned, so a failed
// allocation returns null.
void* TESSaveLoadGame::CreateBuffer(uint32_t aiSize) {
    // Raw storage for the context object, treated as untyped until it is constructed.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = static_cast<AutoMemContext*>(static_cast<void*>(aucContext));
    ::new (pContext) AutoMemContext(
        MEM_CONTEXT_SAVELOAD, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp", 4154
    );
    m_pBuffer = ourMalloc(aiSize);
    if (!m_pBuffer) {
        InitializationFlow("Could not create save buffer, out of memory.");
    }
    void* pBuffer = m_pBuffer;
    pContext->~AutoMemContext();
    return pBuffer;
}

void TESSaveLoadGame::DestroyBuffer(void* apBuffer) {
    ourFree(apBuffer);
    m_pBuffer = nullptr;
}

void TESSaveLoadGame::DeleteForm(TESForm* apForm) {
    Warning("FORMS: DeleteForm() was called, but the game is not being loaded.");
    m_pChanges->RemoveChanges(apForm, true);
    char* v4 = reinterpret_cast<char*>(dynamic_cast<TESObjectCELL*>(apForm));
    if (apForm->GetTemporary() || v4) {
        uint32_t NextID = pDataHandler->GetNextID();
        apForm->SetFormID(NextID, true);
    }
    else {
        apForm->SetFormID(0, true);
        apForm->RemoveFromDataStructures();
        apForm->SetFormEditorID("");
        if (!m_DeferredDeleteList.IsInList(apForm)) {
            m_DeferredDeleteList.AddHead(apForm);
        }
    }
}

uint32_t TESSaveLoadGame::ReadFile(BSFile* apFile, void* apData, uint32_t aiSize) {
    return apFile->NiBinaryStream::Read(apData, aiSize);
}

// While a save is being loaded nothing goes to the file: the write only grows the simulation file size.
uint32_t TESSaveLoadGame::WriteFile(BSFile* apFile, const void* apData, uint32_t aiSize) {
    if (GetLoading()) {
        m_iSimulationFileSize += aiSize;
        return aiSize;
    }
    return static_cast<uint32_t>(apFile->WriteHere(apData, static_cast<int32_t>(aiSize)));
}

bool TESSaveLoadGame::GetLoading() {
    return false;
}

void TESSaveLoadGame::SetLoading([[maybe_unused]] bool abValue) {}

// Applies the change buffer recorded for apForm under kLoadLock: a buffer of another form type is dropped with its
// changes (the formatted message goes nowhere), else the form loads it and is queued in m_pInitArray for its
// initialisation.
bool TESSaveLoadGame::LoadForm(TESForm* apForm) {
    if (!GetLoading()) {
        return false;
    }
    ChangeData* pChanges = m_pChanges->GetChanges(apForm);
    if (!pChanges) {
        return false;
    }
    void* pBuffer = pChanges->GetBuffer();
    if (!pBuffer) {
        return false;
    }
    kLoadLock.Enter("TESSaveLoadGame::LoadForm");
    m_pBuffer = pBuffer;
    SaveBufferHeader kBufferHeader;
    pSaveLoadGameOLD->LoadGameDataOLD(&kBufferHeader, 4);
    if (apForm->GetFormType() != kBufferHeader.cFormType) {
        char acMessage[260];
        const char* pFormTypeString = apForm->GetFormTypeString();
        const char* pSavedTypeString = formEnumString[kBufferHeader.cFormType].pFormString;
        uint32_t uiFormID = apForm->GetFormID();
        std::sprintf(
            acMessage,
            "Load Error: Form with ID %08X was saved with form type %s, but currently has form type %s.  Its loading "
            "will be skipped.",
            uiFormID, pSavedTypeString, pFormTypeString
        );
        m_pChanges->RemoveChanges(apForm, true);
        m_pBuffer = nullptr;
        kLoadLock.Leave();
        return false;
    }
    uint32_t uiFlags = CheckNewReference(apForm, pChanges->GetFlags());
    SetCurrentVersion(kBufferHeader.cVersion);
    LoadFormHeader kFormHeader(&kBufferHeader, apForm->GetFormID(), uiFlags);
    SetCurrentlyLoadingFormHeader(&kFormHeader);
    bool bWasLoading = GetLoading();
    SetLoading(true);
    apForm->SetStillLoading(true);
    LoadInitialData(apForm, uiFlags);
    apForm->LoadGame(uiFlags, 0);
    SetLoading(bWasLoading);
    SetCurrentlyLoadingFormHeader(nullptr);
    ResetVersions();
    if (!m_pInitArray) {
        void* pArrayMemory = ourMalloc(sizeof(NiTLargePrimitiveArray<FormAndFlags*>));
        m_pInitArray = pArrayMemory ? ::new (pArrayMemory) NiTLargePrimitiveArray<FormAndFlags*>(50, 50) : nullptr;
    }
    void* pItemMemory = ourMalloc(sizeof(FormAndFlags));
    FormAndFlags* pItem =
        pItemMemory ? ::new (pItemMemory) FormAndFlags(apForm, uiFlags, 0, kBufferHeader.cVersion) : nullptr;
    m_pInitArray->Add(pItem);
    DestroyBuffer(pBuffer);
    pChanges->SetBuffer(nullptr);
    if (m_iQueuedRemoveChanges) {
        m_pChanges->RemoveChange(apForm, m_iQueuedRemoveChanges);
        m_iQueuedRemoveChanges = 0;
    }
    kLoadLock.Leave();
    return true;
}

void TESSaveLoadGame::SetCurrentlyLoadingFormHeader(LoadFormHeader* apHeader) {
    m_pCurrentlyLoadingFormHeader = apHeader;
}

void TESSaveLoadGame::LoadInitialData(TESForm* apForm, uint32_t aiFlags) {
    ReferenceData aiFormIDIndex;
    TESObjectREFR* a1 = dynamic_cast<TESObjectREFR*>(apForm);
    if (a1) {
        if ((aiFlags & 6) != 0) {
            aiFormIDIndex.~ReferenceData();
            ::new (&aiFormIDIndex) ReferenceData;
            pSaveLoadGameOLD->LoadGameDataOLD(&aiFormIDIndex, 28u);
            aiFormIDIndex.iLocationID = GetNumericIDFromArray(aiFormIDIndex.iLocationID);
            a1->SetAngleOnReference(aiFormIDIndex.Angle);
            if (a1 != pPlayer) {
                a1->SetLocationOnReference(aiFormIDIndex.Loc);
                SafetyCheckReferencePositionAndAngle(a1);
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(aiFormIDIndex.iLocationID);
                TESObjectCELL* v10 = dynamic_cast<TESObjectCELL*>(FormByNumericID);
                TESWorldSpace* v12 = dynamic_cast<TESWorldSpace*>(FormByNumericID);
                if (v10) {
                    if (v10 != a1->GetParentCell()) {
                        TESObjectREFR::MoveRefToNewSpace(a1, v10, nullptr);
                    }
                }
                else {
                    if (v12) {
                        TESObjectCELL* ParentCell;
                        int32_t v7 = FloatToInt(aiFormIDIndex.Loc.x) >> 12;
                        int32_t v8 = FloatToInt(aiFormIDIndex.Loc.y) >> 12;
                        TESObjectCELL* CellFromCellCoord = v12->GetCellFromCellCoord(v7, v8);
                        if ((a1->GetParentCell() && (ParentCell = a1->GetParentCell(), ParentCell->GetInterior())) ||
                            a1->GetParentCell() != CellFromCellCoord) {
                            TESObjectREFR::MoveRefToNewSpace(a1, nullptr, v12);
                        }
                        else {
                            if (!CellFromCellCoord && !a1->GetRefPersists()) {
                                Warning("SAVELOAD: Trying to load non-persistent ref into non-existent cell.");
                            }
                        }
                    }
                    else {
                        if (a1->GetRefPersists()) {
                            if (a1->GetParentCell()) {
                                TESObjectCELL* v4 = a1->GetParentCell();
                                v4->RemoveReference(a1);
                            }
                            a1->SetParentCell(nullptr);
                            pModelLoader->CancelReference(a1);
                            a1->Set3D(nullptr, true);
                        }
                        else {
                            Warning("SAVELOAD: Trying to put non-persistent reference in non-existent cell.");
                        }
                    }
                }
            }
        }
    }
    else {
        if ((dynamic_cast<TESObjectCELL*>(apForm)) && (pSaveLoadGameOLD->GetCurrentVersion() < 91u)) {
            AdvanceBuffer(2u);
        }
    }
}

bool TESSaveLoadGame::LoadCell(TESObjectCELL* apCell) {
    return false;
}

// Runs the init of the loaded forms (the player's first, with apPlayerFlags) and frees their entries: without
// apInitArray the loader's own m_pInitArray is used, which is deleted afterwards either way. Actors of a passed array
// that ended up in no cell or worldspace are moved to the COC cell (form ID 0) or disabled. abForceLoadModels loads the
// attached cells' models and the grass around the player. GetLoading always returns false, so the body returns at once.
void TESSaveLoadGame::InitForms(
    NiTLargePrimitiveArray<FormAndFlags*>* apInitArray,
    FormAndFlags* apPlayerFlags,
    bool abForceLoadModels
) {
    if (!GetLoading()) {
        return;
    }
    SetLoading(true);
    if (abForceLoadModels) {
        pIOManager->ForceHighestPriority();
    }
    bool bPassedArray = true;
    if (!apInitArray) {
        apInitArray = m_pInitArray;
        bPassedArray = false;
    }
    if (apPlayerFlags) {
        static_cast<TESForm*>(pPlayer)->InitLoadGame(apPlayerFlags->iFlags, apPlayerFlags->iOldFlags);
    }
    BSSimpleList<Actor*> kNoLocationActors;
    kNoLocationActors.~BSSimpleList<Actor*>();
    ::new (&kNoLocationActors) BSSimpleList<Actor*>;
    if (apInitArray) {
        uint32_t uiSize = apInitArray->GetSize();
        for (uint32_t i = 0; i < uiSize; ++i) {
            FormAndFlags* pItem = apInitArray->GetAt(i);
            if (!pItem || pItem->pForm == pPlayer) {
                continue;
            }
            SetCurrentVersion(pItem->cVersion);
            pItem->pForm->InitLoadGame(pItem->iFlags, pItem->iOldFlags);
            if (bPassedArray) {
                Actor* pActor = dynamic_cast<Actor*>(pItem->pForm);
                if (pActor && !pActor->GetParentCell() && !pActor->GetWorldSpace()) {
                    kNoLocationActors.AddHead(pActor);
                }
            }
            ResetVersions();
        }
    }
    if (bPassedArray) {
        for (BSSimpleList<Actor*>* pNode = &kNoLocationActors; pNode;) {
            if (pNode->IsEmpty()) {
                break;
            }
            Actor* pActor = pNode->GetItem();
            pActor->InitPackageLocations(false);
            m_pChanges->AddChange(pActor, 2);
            if (!pActor->GetParentCell() && !pActor->GetWorldSpace()) {
                uint32_t uiCellID = 0;
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(uiCellID));
                if (pCell) {
                    NiPoint3 kPos = NiPoint3_ZERO;
                    NiPoint3 kRot = NiPoint3_ZERO;
                    pCell->GetCOCPlacementInfo(kPos, kRot);
                    pActor->SetLocationOnReference(kPos);
                    pActor->SetAngleOnReference(kRot);
                    TESObjectREFR::MoveRefToNewSpace(pActor, pCell, nullptr);
                }
                else {
                    pActor->SetDisabled(true);
                }
            }
            pNode = pNode->GetNext();
        }
        kNoLocationActors.RemoveAll();
    }
    if (abForceLoadModels) {
        bhkWorld* pWorld = TES::pInstance->GetInterior() ? TES::pInstance->GetInterior()->GetbhkWorld() : nullptr;
        if (pWorld) {
            pWorld->StartBatchingAdds();
        }
        if (TESObjectCELL::ExteriorWorld()) {
            TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
        }
        TES::pInstance->QueueReferencesForAttachedCells();
        if (!pPlayer->Get3D(false)) {
            pModelLoader->QueueReference(pPlayer, IO_TASK_PRIORITY_CRITICAL, false);
        }
        pIOManager->LoadAllQueuedPriorities();
        pIOManager->ForceHighestPriority();
        if (pWorld) {
            pWorld->StopBatchingAdds(false);
        }
        if (TESObjectCELL::ExteriorWorld()) {
            TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
        }
        NiPoint3 kPlayerPos = pPlayer->GetLocationOnReference();
        TallGrassShaderProperty::SetUseBackgroundLoading(false);
        TESTerrainLODManager::UpdateGrass(kPlayerPos, NiPoint3_UNIT_Y, 1.0f);
        TallGrassShaderProperty::SetUseBackgroundLoading(true);
    }
    if (apPlayerFlags && pPlayer->GetParentCell()) {
        pPlayer->UpdateCurrentDebugGeometry();
    }
    if (apPlayerFlags) {
        static_cast<TESForm*>(pPlayer)->FinishInitLoadGame(apPlayerFlags->iFlags, apPlayerFlags->iOldFlags);
    }
    if (apInitArray) {
        uint32_t uiSize = apInitArray->GetSize();
        for (uint32_t i = 0; i < uiSize; ++i) {
            FormAndFlags* pItem = apInitArray->GetAt(i);
            if (!pItem) {
                continue;
            }
            SetCurrentVersion(pItem->cVersion);
            pItem->pForm->FinishInitLoadGame(pItem->iFlags, pItem->iOldFlags);
            ResetVersions();
            ourFree(pItem);
        }
    }
    if (m_pInitArray) {
        m_pInitArray->RemoveAll();
        delete m_pInitArray;
        m_pInitArray = nullptr;
    }
    if (abForceLoadModels) {
        bhkWorld* pWorld = TES::pInstance->GetInterior() ? TES::pInstance->GetInterior()->GetbhkWorld() : nullptr;
        if (pWorld) {
            pWorld->StartBatchingAdds();
        }
        if (TESObjectCELL::ExteriorWorld()) {
            TESObjectCELL::ExteriorWorld()->StartBatchingAdds();
        }
        pIOManager->LoadAllQueuedPriorities();
        if (pWorld) {
            pWorld->StopBatchingAdds(false);
        }
        if (TESObjectCELL::ExteriorWorld()) {
            TESObjectCELL::ExteriorWorld()->StopBatchingAdds(false);
        }
    }
    SetLoading(false);
}

void TESSaveLoadGame::LoadQueuedData(bool abUpdateWorld) {
    ;
}

bool TESSaveLoadGame::GetInitArrayExists() {
    return (m_pInitArray != nullptr);
}

void TESSaveLoadGame::StartLoadingSequence() {
    if (GetTickCount() > iLoadSequenceStartTime + 3000 && !Interface::GetLoadingMenuVisible()) {
        TES::pInstance->ShowLoadingMenu(true, nullptr, false);
    }
}

// Only a load queues forms: the first call allocates the array (20000 slots, growing by 2000) and every call adds the
// form.
void TESSaveLoadGame::AddInitItem(TESForm* apForm) {
    if (!GetLoading()) {
        return;
    }
    if (!m_pInitItemArray) {
        NiTLargePrimitiveArray<TESForm*>* pArray =
            static_cast<NiTLargePrimitiveArray<TESForm*>*>(ourMalloc(sizeof(NiTLargePrimitiveArray<TESForm*>)));
        if (pArray) {
            pArray = ::new (pArray) NiTLargePrimitiveArray<TESForm*>(20000, 2000);
        }
        m_pInitItemArray = pArray;
    }
    m_pInitItemArray->Add(apForm);
}

bool TESSaveLoadGame::UnloadForm([[maybe_unused]] TESForm* apForm) {
    return false;
}

void TESSaveLoadGame::RemoveChanges(TESForm* apForm, bool abForceRemove) {
    if (!apForm->GetTemporary()) {
        m_pChanges->RemoveChanges(apForm, abForceRemove);
    }
}

void TESSaveLoadGame::SaveNumericID(uint32_t* apFormID, uint32_t aiSize) {
    uint32_t apSrc;
    uint32_t i;
    uint32_t v6 = aiSize >> 2;
    for (i = 0; i < v6; ++i) {
        if (GetUseNumericIDArray()) {
            apSrc = AddNumericIDToArray(apFormID[i]);
        }
        else {
            apSrc = apFormID[i];
        }
        ourMemcpy(m_pBuffer, &apSrc, 4u);
        AdvanceBuffer(4);
    }
}

bool TESSaveLoadGame::LoadNumericID(uint32_t* apFormID, uint32_t aiSize) {
    uint32_t numericIDFromArray;
    uint32_t i;
    uint8_t v7 = 0;
    ourMemcpy(apFormID, m_pBuffer, aiSize);
    if (GetUseNumericIDArray()) {
        for (i = 0; i < (aiSize >> 2); ++i) {
            numericIDFromArray = GetNumericIDFromArray(apFormID[i]);
            if ((apFormID[i]) && (!numericIDFromArray)) {
                v7 = 1;
            }

            apFormID[i] = numericIDFromArray;
        }
    }
    AdvanceBuffer(aiSize);
    return (bool)v7;
}

TESPackage* TESSaveLoadGame::CreatePackage(uint32_t aiFormID, uint8_t cPackType) {
    ClearFormID(aiFormID);
    TESPackage* package = TESPackage::CreatePackage(static_cast<PACKAGE_TYPE>((int32_t)cPackType));
    package->SetFormID(aiFormID, true);
    return package;
}

bool TESSaveLoadGame::UseSaveGameBlocks() {
    bool bUse;
    bUse = pSaveLoadGameOLD->GetCurrentVersion() >= 31 && pSaveLoadGameOLD->GetCurrentVersion() < 90;
    return bUse;
}

void TESSaveLoadGame::LoadGameDataOLD(void* apData, uint32_t aiSize) {
    ourMemcpy(apData, m_pBuffer, aiSize);
    AdvanceBuffer(aiSize);
}

void TESSaveLoadGame::TestAllCells(uint32_t aiControl) {
    static int32_t icount;
    NiPoint3 a2;
    if (aiControl == 4 || aiControl == 5) {
        if (aiControl == 5) {
            uint32_t ExteriorBufferSize = TES::pInstance->GetExteriorBufferSize();
            if (TES::pInstance->pInteriorCell) {
                ExteriorBufferSize = 1;
            }
            for (int32_t i = 0; i < ExteriorBufferSize; ++i) {
                TESObjectCELL* Interior = TES::pInstance->pInteriorCell;
                if (!Interior) {
                    Interior = TES::pInstance->GetExteriorBufferCellByIndex(i);
                }
                if (Interior) {
                    Interior->SaveGameTest();
                }
            }
            TESBoundObject* apObject = pLockpick;
            for (uint32_t j = 0; j < 5; ++j) {
                TESWorldSpace* WorldSpace = pPlayer->GetWorldSpace();
                TESObjectCELL* ParentCell = pPlayer->GetParentCell();
                const NiPoint3* AngleOnReference = &pPlayer->GetAngleOnReference();
                const NiPoint3* v2 = &pPlayer->GetLocationOnReference();
                TESObjectREFR* ReferenceAtLocation = pDataHandler->CreateReferenceAtLocation(
                    apObject, *v2, *AngleOnReference, ParentCell, WorldSpace, nullptr, nullptr, nullptr
                );
                const NiPoint3* v3 = &ReferenceAtLocation->GetLocationOnReference();
                a2.x = v3->x;
                a2.y = v3->y;
                a2.z = v3->z + 1000.0;
                ReferenceAtLocation->SetLocationOnReference(a2);
            }
        }
        if (!(icount % 50)) {
            char apDest[264];
            BSsprintf(apDest, 260u, "Test All Cells %i.ess", icount);
            SaveGame(nullptr, apDest, true);
        }
        ++icount;
    }
}

SaveFormHeader* TESSaveLoadGame::GetCurrentlySavingFormHeader() {
    return m_pCurrentlySavingFormHeader;
}

bool TESSaveLoadGame::IsUnk048Zero() {
    return (m_iRevertState == 0);
}

void TESSaveLoadGame::SaveGameDataOLD(const void* apData, uint32_t aiSize) {
    ourMemcpy(m_pBuffer, apData, aiSize);
    AdvanceBuffer(aiSize);
}

// Tests the three coordinates for finiteness first, then for NaN.
bool TESSaveLoadGame::IsInvalidPoint(NiPoint3* apPoint) {
    return !std::isfinite(apPoint->x) || !std::isfinite(apPoint->y) || !std::isfinite(apPoint->z) ||
        std::isnan(apPoint->x) || std::isnan(apPoint->y) || std::isnan(apPoint->z);
}

// An actor with an editor location goes back to it; any other reference to its starting position when it has one, else
// to the origin. The location and angle are copied first.
void TESSaveLoadGame::SafetyCheckReferencePositionAndAngle(TESObjectREFR* apRef) {
    if (!apRef) {
        return;
    }
    NiPoint3 kLocation;
    kLocation.~NiPoint3();
    ::new (&kLocation) NiPoint3;
    NiPoint3 kAngle;
    kAngle.~NiPoint3();
    ::new (&kAngle) NiPoint3;
    kLocation = apRef->GetLocationOnReference();
    kAngle = apRef->GetAngleOnReference();
    if (IsInvalidPoint(&kLocation)) {
        Warning("SAVELOAD: Corrupt location found loading reference %08X, fixing it.", apRef->GetFormID());
        Actor* pActor = nullptr;
        if (apRef->IsActor()) {
            pActor = static_cast<Actor*>(apRef);
        }
        if (pActor && pActor->HasEditorLocation()) {
            TESObjectCELL* pInterior = pActor ? pActor->GetEditorLocationInteriorCell() : nullptr;
            TESWorldSpace* pWorld = pActor ? pActor->GetEditorLocationWorld() : nullptr;
            if (pInterior || pWorld) {
                NiPoint3 kEditorLocation = pActor->GetStartingLocation();
                apRef->SetLocationOnReference(kEditorLocation);
                apRef->SetAngleOnReferenceZ(pActor->GetStartingAngle().z);
                TESObjectREFR::MoveRefToNewSpace(apRef, pInterior, pWorld);
            }
        }
        else if (apRef->GetExtra()->GetExtraData(EXTRA_STARTINGPOSITION)) {
            NiPoint3 kStartingLocation = apRef->GetStartingLocation();
            NiPoint3 kStartingAngle = apRef->GetStartingAngle();
            apRef->SetLocationOnReference(kStartingLocation);
            apRef->SetAngleOnReference(kStartingAngle);
            TESForm* pStartingSpace = apRef->GetExtra()->GetStartingWorldOrCell();
            if (pStartingSpace) {
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pStartingSpace);
                TESWorldSpace* pWorldSpace = dynamic_cast<TESWorldSpace*>(pStartingSpace);
                if (pCell || pWorldSpace) {
                    TESObjectREFR::MoveRefToNewSpace(apRef, pCell, pWorldSpace);
                }
            }
        }
        else {
            apRef->SetLocationOnReference(NiPoint3_ZERO);
        }
    }
    if (IsInvalidPoint(&kAngle)) {
        Warning("SAVELOAD: Corrupt angle found loading reference %08X, setting to (0, 0, 0).", apRef->GetFormID());
        apRef->SetAngleOnReference(NiPoint3_ZERO);
    }
}

// Allocates the buffer with its two-byte size header, reads the data after the header and queues it under aiFormID.
void TESSaveLoadGame::QueueBufferLoad(NumericIDBufferMap* apMap, uint32_t aiFormID, uint16_t aiSize) {
    void* pBuffer = CreateQueuedBuffer(aiSize);
    LoadGameDataOLD(static_cast<char*>(pBuffer) + 2, aiSize);
    apMap->SetAt(aiFormID, pBuffer);
}

void TESSaveLoadGame::QueueRemoveChange(uint32_t aiChange) {
    m_iQueuedRemoveChanges |= aiChange;
}

void TESSaveLoadGame::ResetVersions() {
    m_cCurrentVersion = m_cMinorVersion;
}

bool TESSaveLoadGame::GetSavingAllowed() {
    if ((((int32_t)pPlayer->Actor::GetLifeState()) == 2) || (((int32_t)pPlayer->Actor::GetLifeState()) == 1)) {
        return false;
    }
    if (pPlayer->GetUnderAIControl()) {
        return false;
    }
    if (StartMenu::PauseMenuIsActive()) {
        return true;
    }
    int32_t topMenuID = (int32_t)Interface::GetTopMenuID();
    if (topMenuID && (topMenuID != 3)) {
        return false;
    }
    int32_t menuModeType = (int32_t)Interface::GetMenuModeType();
    return (bool)((uint8_t)((!menuModeType) || (menuModeType == 3)));
}

uint32_t TESSaveLoadGame::GetNumericIDFromArray(uint32_t aiFormIDIndex) {
    if (pDataHandler->IsFormIDCreated(aiFormIDIndex)) {
        return aiFormIDIndex;
    }
    if (aiFormIDIndex <= m_pNumericIDArray->NiTLargeArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize()) {
        return m_pNumericIDArray->NiTLargeArray<uint32_t, NiTMallocInterface<uint32_t>>::GetAt(aiFormIDIndex);
    }
    return (uint32_t)0;
}

uint8_t TESSaveLoadGame::GetCurrentVersion() {
    return m_cCurrentVersion;
}

void TESSaveLoadGame::AddFormToDeferredDeletionsList(TESForm* apForm) {
    if (!m_DeferredDeleteList.IsInList(apForm)) {
        m_DeferredDeleteList.AddHead(apForm);
    }
}

void TESSaveLoadGame::LoadBuffer(BSFile* apFile, void* apBuffer, uint32_t aiSize) {
    ReadFile(apFile, apBuffer, aiSize);
}

void TESSaveLoadGame::QueueAttachedAnimationLoad(TESObjectREFR* apRef, uint16_t aiSize) {
    uint32_t v3 = apRef->TESForm::GetFormID();
    QueueBufferLoad(m_pAttachedAnimationMap, (int32_t)v3, aiSize);
}

void TESSaveLoadGame::QueueCharControllerLoad(TESObjectREFR* apRef, uint16_t aiSize) {
    uint32_t v3 = apRef->TESForm::GetFormID();
    QueueBufferLoad(m_pCharControllerMap, v3, aiSize);
}

void TESSaveLoadGame::QueueHavokDataLoad(TESObjectREFR* apRef, uint16_t aiSize) {
    uint32_t v3 = apRef->TESForm::GetFormID();
    QueueBufferLoad(m_pHavokDataMap, (int32_t)v3, aiSize);
}

void TESSaveLoadGame::AdvanceBuffer(uint32_t aiSize) {
    m_pBuffer = static_cast<char*>(m_pBuffer) + aiSize;
}

// Deletes the form with this ID, or drops the changes recorded for an ID that has no form.
void TESSaveLoadGame::ClearFormID(uint32_t aiFormID) {
    TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
    if (pForm) {
        DeleteForm(pForm);
    }
    else {
        m_pChanges->RemoveChanges(aiFormID, true);
    }
}

// Queues the reference's animation buffer; a reference that already has one queues under form ID 0.
void TESSaveLoadGame::QueueAnimationLoad(TESObjectREFR* apRef, uint16_t aiSize) {
    uint32_t uiFormID = apRef->TESForm::GetFormID();
    void* pBuffer;
    if (m_pAnimationMap->GetAt(uiFormID, pBuffer)) {
        uiFormID = 0;
    }
    QueueBufferLoad(m_pAnimationMap, uiFormID, aiSize);
}

// Warns when the version is older than the oldest compatible one (19), then sets it.
void TESSaveLoadGame::SetCurrentVersion(uint8_t acVersion) {
    if (acVersion < 19) {
        Warning(
            "SAVELOAD: (Savegame loading error) Attempting to set the current version to %i.  The oldest "
            "compatible version is %i.  Errors may occur.",
            acVersion, 19
        );
    }
    m_cCurrentVersion = acVersion;
}

// The version this build writes: 0.125.
void TESSaveLoadGame::ResetCurrentVersion() {
    m_cMajorVersion = 0;
    m_cMinorVersion = 125;
}

void TESSaveLoadGame::RemoveFormFromDeferredDeletionList(TESForm* apForm) {
    if (m_DeferredDeleteList.IsInList(apForm)) {
        m_DeferredDeleteList.Remove(apForm);
    }
}

void* TESSaveLoadGame::CreateQueuedBuffer(uint16_t aiSize) {
    uint16_t v2 = aiSize;
    void* v3 = ourMalloc(aiSize + 2);
    ourMemcpy(v3, &v2, 2u);
    return v3;
}

bool TESSaveLoadGame::GetUseNumericIDArray() {
    return m_bUseNumericIDArray;
}

TESForm* TESSaveLoadGame::CreateReferenceFromFile(uint32_t aiFormID, MovedReferenceData* apMovedData) {
    TESFile* file;
    TESFile* v4;
    const NiPoint3* v6;
    ExtraDataList* extra;
    ExtraDataList* v8;
    TESObjectREFR* v9;
    TESObjectREFR* v10;
    float a5;
    NiPoint3 v12;
    NiPoint3 result;
    uint32_t v14;
    TESFile* apFile;
    uint32_t tESForm;
    TESFile* pFile;
    int32_t fileCount;
    Actor* v26;
    int32_t aiIndex;
    NiPoint3 v28;
    NiPoint3 v29;
    TESObjectREFR* apObject = nullptr;
    uint32_t aiSearchID = apMovedData->iOriginalLocationID;
    if (!aiSearchID) {
        aiSearchID = apMovedData->RefData.iLocationID;
    }
    TESForm* formByNumericID = TESForm::GetFormByNumericID(aiSearchID);
    TESObjectCELL* a3a = dynamic_cast<TESObjectCELL*>(formByNumericID);
    TESWorldSpace* apWorld = dynamic_cast<TESWorldSpace*>(formByNumericID);
    int32_t aiX = 0;
    int32_t aiY = 0;
    if (a3a) {
        fileCount = a3a->GetFileCount();
        for (aiIndex = 0; aiIndex < fileCount; ++aiIndex) {
            file = a3a->GetFile(aiIndex);
            pFile = file->GetThreadSafeFile();
            if (pFile->FindForm(a3a) && TESObjectCELL::FindReferenceInFile(pFile, aiFormID)) {
                tESForm = pFile->GetTESForm();
                apObject = TESObjectREFR::CreateReference(tESForm, true);
                TESDataHandler::LoadForm(apObject, pFile);
            }
        }
    }
    else {
        if (apWorld) {
            fileCount = apWorld->GetFileCount();
            aiX = FloatToInt(apMovedData->OriginalLoc.x) >> 12;
            aiY = FloatToInt(apMovedData->OriginalLoc.y) >> 12;
            for (aiIndex = 0; aiIndex < fileCount; ++aiIndex) {
                v4 = apWorld->GetFile(aiIndex);
                apFile = v4->GetThreadSafeFile();
                if (apWorld->FindCellInFile(apFile, aiX, aiY) && TESObjectCELL::FindReferenceInFile(apFile, aiFormID)) {
                    v14 = apFile->GetTESForm();
                    apObject = TESObjectREFR::CreateReference(v14, true);
                    TESDataHandler::LoadForm(apObject, apFile);
                }
            }
        }
        else {
            Warning("CELLS: Reference to be loaded does not have a cell or worldspace.");
        }
    }
    if (apObject) {
        apObject->InitItem();
        v26 = dynamic_cast<Actor*>(apObject);
        if (v26) {
            a5 = apObject->GetAngleOnReference().z;
            v6 = &apObject->GetLocationOnReference();
            v26->SetEditorLocation(apWorld, a3a, *v6, a5);
        }
        else {
            v28 = apObject->GetLocationOnReference();
            v9 = apObject;
            extra = apObject->TESObjectREFR::GetExtra();
            (result = extra->SetStartingLocation(v9, v28));
            v29 = apObject->GetAngleOnReference();
            v10 = apObject;
            v8 = apObject->TESObjectREFR::GetExtra();
            (v12 = v8->SetStartingAngle(v10, v29));
        }
        return apObject;
    }

    Warning(
        "CELLS: Reference %08X could not be loaded into location %08X at coordinates %i, %i", aiFormID, aiSearchID, aiX,
        aiY
    );
    return nullptr;
}

uint32_t TESSaveLoadGame::AddNumericIDToArray(uint32_t aiFormID) {
    uint32_t* v3;
    uint32_t i;
    if (pDataHandler->IsFormIDCreated(aiFormID)) {
        return aiFormID;
    }
    uint32_t v6 = m_pNumericIDArray->m_uiSize;
    for (i = 0; i < v6; ++i) {
        v3 = &m_pNumericIDArray->GetAt(i);
        if ((*v3) == aiFormID) {
            return i;
        }
    }
    return m_pNumericIDArray->Add(aiFormID);
}

bool TESSaveLoadGame::SaveGame(BSFile* pFile, const char* apFileName, bool abLogStats) {
    uint32_t izero;
    uint32_t iformCount;
    uint32_t inumericIDArrayOffset;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp", 1126
    );
    if (GetLoading() || (GetSavingAllowed() || (apFileName && !BSstrcmp(apFileName, "autosave")))) {
        pIOManager->Pause();
        CleanUpExpiredData();
        BSFile* pNewFile = nullptr;
        if (!GetLoading()) {
            pNewFile = PrepForLoadSave(pFile, apFileName, TESSaveLoadGame::ACTION_SAVE);
        }
        if (abLogStats) {
            SaveDataDebugInfo* v7;
            SaveDataDebugInfo* v10 = static_cast<SaveDataDebugInfo*>(ourMalloc(sizeof(SaveDataDebugInfo)));
            if (v10) {
                v7 = ::new (v10) SaveDataDebugInfo;
            }
            else {
                v7 = nullptr;
            }
            m_pSaveLoadStats = v7;
        }
        ResetCurrentVersion();
        ResetVersions();
        SaveHeader(pNewFile, apFileName);
        SavePluginList(pNewFile);
        uint32_t iseekBackOffset = 0;
        if (!GetLoading()) {
            iseekBackOffset = pNewFile->Tell();
        }
        izero = 0;
        WriteFile(pNewFile, &izero, 4u);
        WriteFile(pNewFile, &izero, 4u);
        SaveGlobalData(pNewFile);
        iformCount = 0;
        NiTMapItem<uint32_t, ChangeData*>* it = m_pChanges->GetFirstPos();
        while (it) {
            ChangeData* pchange = nullptr;
            uint32_t iformID = 0;
            m_pChanges->GetNext(it, iformID, pchange);
            if ((iformID) && (pchange)) {
                m_pBuffer = pchange->pBuffer;
                SaveFormHeader kHeader;
                kHeader.iFormID = iformID;
                kHeader.iFlags = pchange->iFlags;
                kHeader.cVersion = GetCurrentVersion();
                if (m_pBuffer) {
                    SaveBufferHeader kBufferHeader;
                    pSaveLoadGameOLD->LoadGameDataOLD(&kBufferHeader, 4u);
                    kHeader.cFormType = kBufferHeader.cFormType;
                    kHeader.cVersion = kBufferHeader.cVersion;
                    pSaveLoadGameOLD->WriteFile(pNewFile, &kHeader, 10u);
                    ++iformCount;
                    WriteFile(pNewFile, &kBufferHeader, 2u);
                    if (kBufferHeader.iSize) {
                        WriteFile(pNewFile, m_pBuffer, kBufferHeader.iSize);
                    }
                    if (m_pSaveLoadStats) {
                        m_pSaveLoadStats->AddStats(&kHeader, kBufferHeader.iSize);
                    }
                    m_pBuffer = nullptr;
                }
                else {
                    TESForm* pform = TESForm::GetFormByNumericID(iformID);
                    if (pform) {
                        kHeader.cFormType = pform->GetFormType();
                        kHeader.iFlags = CheckFlags(pform, kHeader.iFlags);
                        pSaveLoadGameOLD->WriteFile(pNewFile, &kHeader, 10u);
                        ++iformCount;
                        m_pCurrentlySavingFormHeader = &kHeader;
                        uint16_t isize = pform->GetSaveSize(kHeader.iFlags);
                        isize += static_cast<uint16_t>(GetInitialDataSaveSize(pform, kHeader.iFlags));
                        WriteFile(pNewFile, &isize, 2u);
                        if (isize) {
                            void* ptempBuffer = CreateBuffer(isize);
                            SaveInitialData(pform, kHeader.iFlags);
                            pform->SaveGame(kHeader.iFlags);
                            SaveBuffer(pNewFile, ptempBuffer, isize);
                            DestroyBuffer(ptempBuffer);
                        }
                        m_pCurrentlySavingFormHeader = nullptr;
                        if (m_pSaveLoadStats) {
                            m_pSaveLoadStats->AddStats(&kHeader, isize);
                        }
                    }
                }
            }
        }
        SaveFinalData(pNewFile);
        inumericIDArrayOffset = 0;
        if (!GetLoading()) {
            inumericIDArrayOffset = pNewFile->Tell();
        }
        SaveNumericIDArrays(pNewFile);
        if (!GetLoading()) {
            pNewFile->Seek(static_cast<int32_t>(iseekBackOffset), NiFile::ms_iSeekSet);
            WriteFile(pNewFile, &inumericIDArrayOffset, 4u);
            WriteFile(pNewFile, &iformCount, 4u);
        }
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->PrintStats(pNewFile->GetDevice());
            SaveDataDebugInfo* apThis = m_pSaveLoadStats;
            if (apThis) {
                delete apThis;
            }
            m_pSaveLoadStats = nullptr;
        }
        if (!GetLoading()) {
            pNewFile->Flush();
            SetMostRecentSaveGame(pNewFile);
            CleanupLoadSave(pNewFile, TESSaveLoadGame::ACTION_SAVE);
        }
        pIOManager->Resume();
        return true;
    }

    const char* v4 = sCantSaveNow.operator const char*();
    Interface::ShowMessage(
        v4, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
    );
    return false;
}

bool TESSaveLoadGame::ResetReferencePosition(TESObjectREFR* apRef, bool abTemporaryMove) {
    bool brestore = m_bAllowChanges;
    m_bAllowChanges = false;
    bool bresult = false;
    Actor* pactor = nullptr;
    if (apRef->IsActor()) {
        pactor = static_cast<Actor*>(apRef);
    }
    if (pactor && pactor->HasEditorLocation()) {
        TESObjectCELL* pInteriorCell = pactor->GetEditorLocationInteriorCell();
        TESWorldSpace* pWorldSpace = pactor->GetEditorLocationWorld();
        if (pInteriorCell || pWorldSpace) {
            apRef->SetLocationOnReference(pactor->GetStartingLocation());
            apRef->SetAngleOnReferenceZ(pactor->GetStartingAngle().z);
            if (abTemporaryMove) {
                SetLoading(true);
            }
            TESObjectREFR::MoveRefToNewSpace(apRef, pInteriorCell, pWorldSpace);
            if (abTemporaryMove) {
                SetLoading(false);
            }
            bresult = true;
        }
    }
    else {
        if ((&apRef->m_Extra)->GetExtraData(15)) {
            NiPoint3 loc = apRef->GetStartingLocation();
            NiPoint3 angle = apRef->GetStartingAngle();
            apRef->SetLocationOnReference(loc);
            apRef->SetAngleOnReference(angle);
            bresult = true;
        }
        TESForm* pstartingWorldOrCell = (&apRef->m_Extra)->GetStartingWorldOrCell();
        if (pstartingWorldOrCell) {
            TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pstartingWorldOrCell);
            TESWorldSpace* pWorld = dynamic_cast<TESWorldSpace*>(pstartingWorldOrCell);
            if (pCell || pWorld) {
                TESObjectREFR::MoveRefToNewSpace(apRef, pCell, pWorld);
                bresult = true;
            }
        }
    }
    if (bresult) {
        NiAVObject* obj3D = (NiAVObject*)apRef->TESObjectREFR::Get3D();
        if (obj3D) {
            NiPoint3 loc_2 = apRef->GetLocationOnReference();
            MobileObject* pmob = dynamic_cast<MobileObject*>(apRef);
            if (pmob) {
                bhkCharacterController* pcon = pmob->GetCharController();
                if ((pcon) && (!pcon->QIsFlying())) {
                    pcon->SetPosition(loc_2);
                }
            }
            obj3D->SetTranslate(loc_2);
            obj3D->SetRotate(apRef->GetOrientation());
            bhkNiCollisionObject::ResetSim(obj3D, true);
            NiUpdateData UpdateData(0.0f, false, false);
            obj3D->Update(UpdateData);
        }
    }
    m_bAllowChanges = brestore;
    return bresult;
}

void TESSaveLoadGame::SaveGlobalData(BSFile* apFile) {
    int32_t icurrentGridY;
    uint32_t iworldspaceID;
    uint16_t isize;
    uint32_t inextID;
    int32_t icurrentGridX;
    int32_t itargetCount;
    uint32_t ilocationID;
    NiPoint3 playerLocation;
    inextID = pDataHandler->GetNextFormID();
    WriteFile(apFile, &inextID, 4u);
    iworldspaceID = TES::pInstance->pWorldSpace->iFormID;
    WriteFile(apFile, &iworldspaceID, 4u);
    icurrentGridX = TES::pInstance->iCurrentGridX;
    icurrentGridY = TES::pInstance->iCurrentGridY;
    WriteFile(apFile, &icurrentGridX, 4u);
    WriteFile(apFile, &icurrentGridY, 4u);
    TESWorldSpace* pworld = pPlayer->GetWorldSpace();
    TESObjectCELL* pcell = pPlayer->GetParentCell();
    if (!pworld && !pcell) {
        InitializationFlow("Player has no worldspace or parent cell, cannot save.");
    }
    ilocationID = 0;
    if (pworld) {
        ilocationID = pworld->iFormID;
    }
    else {
        if (pcell) {
            ilocationID = pcell->iFormID;
        }
    }
    playerLocation = *pPlayer->QLocationOnReference();
    WriteFile(apFile, &ilocationID, 4u);
    WriteFile(apFile, &playerLocation, 12u);
    SaveGlobals(apFile);
    isize = TES::pInstance->GetSaveSize();
    WriteFile(apFile, &isize, 2u);
    if (isize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(isize, "TES Class");
        }
        void* ptempBuffer = CreateBuffer(isize);
        TES::pInstance->SaveGameTES();
        SaveBuffer(apFile, ptempBuffer, isize);
        DestroyBuffer(ptempBuffer);
    }
    isize = ReferenceProcessLists.GetCrimesSaveSize();
    WriteFile(apFile, &isize, 2u);
    if (isize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(isize, "Process Lists Class");
        }
        void* ptempBuffer_2 = CreateBuffer(isize);
        ReferenceProcessLists.SaveGame_Crimes();
        SaveBuffer(apFile, ptempBuffer_2, isize);
        DestroyBuffer(ptempBuffer_2);
    }
    Sky* Instance = Sky::GetInstance();
    isize = Instance->GetSaveSize();
    WriteFile(apFile, &isize, 2u);
    if (isize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(isize, "Sky/Weather");
        }
        void* ptempBuffer_3 = CreateBuffer(isize);
        Sky* v3 = Sky::GetInstance();
        v3->SaveGame();
        SaveBuffer(apFile, ptempBuffer_3, isize);
        DestroyBuffer(ptempBuffer_3);
    }
    itargetCount = 0;
    WriteFile(apFile, &itargetCount, 4u);
    SaveCreatedBaseObjects(apFile);
    isize = 0;
    WriteFile(apFile, &isize, 2u);
    if (isize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(isize, "HUD Reticle");
        }
        void* ptempBuffer_4 = CreateBuffer(isize);
        SaveBuffer(apFile, ptempBuffer_4, isize);
        DestroyBuffer(ptempBuffer_4);
    }
    isize = Interface::GetSaveSize();
    WriteFile(apFile, &isize, 2u);
    if (isize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(isize, "Interface");
        }
        void* ptempBuffer_5 = CreateBuffer(isize);
        Interface::SaveGameTES();
        SaveBuffer(apFile, ptempBuffer_5, isize);
        DestroyBuffer(ptempBuffer_5);
    }
    isize = TESDataHandler::RegionList::GetSaveSize();
    WriteFile(apFile, &isize, 2u);
    if (isize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(isize, "Regions");
        }
        void* ptempBuffer_6 = CreateBuffer(isize);
        TESRegion::SaveGame();
        SaveBuffer(apFile, ptempBuffer_6, isize);
        DestroyBuffer(ptempBuffer_6);
    }
}

void TESSaveLoadGame::SaveFinalData(BSFile* a1) {
    uint32_t aiSize;
    aiSize = ReferenceProcessLists.GetSaveSize();
    WriteFile(a1, &aiSize, 4u);
    if (aiSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(aiSize, "Temp Effects List");
        }
        void* Block = CreateBuffer(aiSize);
        ReferenceProcessLists.SaveTempEffectsList();
        SaveBuffer(a1, Block, aiSize);
        DestroyBuffer(Block);
    }
}

uint32_t TESSaveLoadGame::CheckFlags(TESForm* apForm, uint32_t aiFlags) {
    NiPoint3 v10;
    int32_t aiFlagsa = CheckNewReference(apForm, aiFlags);
    TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(apForm);
    TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(apForm);
    if (pcell) {
        if (aiFlagsa < 0 && !pcell->GetSeenData()) {
            aiFlagsa &= ~static_cast<uint32_t>(CHANGE_CELL_SEENDATA);
        }
        if (!pcell->GetInterior()) {
            int32_t ixCoord = pcell->GetDataX();
            int32_t iyCoord = pcell->GetDataY();
        }
    }
    else {
        if (pref) {
            if ((aiFlagsa & 32) != 0 && !(&pref->m_Extra)->GetContainerChanges()) {
                aiFlagsa &= ~32u;
            }
            Actor* pactor = dynamic_cast<Actor*>(pref);
            if (pactor) {
                if (pactor->pCurrentProcess) {
                    BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                    CurrentAIProcess->GetRunOncePackage();
                }
                if ((pactor->pCurrentProcess) && (pactor->pCurrentProcess->BaseProcess::GetPackageThatIsRunning())) {
                    TESPackage* pPackage = pactor->pCurrentProcess->BaseProcess::GetPackageThatIsRunning();
                    aiFlagsa |= pactor->GetPackageSaveFlags(pPackage);
                }

                if (pactor->CheckShouldSaveHavokData(aiFlagsa)) {
                    aiFlagsa |= 4u;
                }
                else {
                    if (!pactor->IsDead(false)) {
                        aiFlagsa &= ~4u;
                    }
                }
            }
            if ((aiFlagsa & 6) != 0 && !pref->GetRefPersists() && pref != pPlayer) {
                TESWorldSpace* pcell_2;
                TESObjectCELL* v8;
                Actor* pactor_2 = nullptr;
                if (pref->IsActor()) {
                    pactor_2 = static_cast<Actor*>(pref);
                }
                if (pactor_2) {
                    v8 = pactor_2->GetEditorLocationInteriorCell();
                }
                else {
                    v8 = nullptr;
                }
                if (pactor_2) {
                    pcell_2 = pactor_2->GetEditorLocationWorld();
                }
                else {
                    pcell_2 = nullptr;
                }
                TESWorldSpace* pworld = pcell_2;
                if (pcell_2 || v8) {
                    NiPoint3* v6;
                    if (pactor_2) {
                        v6 = ::new (&v10) NiPoint3(pactor_2->GetStartingLocation());
                    }
                    else {
                        v6 = &NiPoint3_ZERO;
                    }
                    float someFloat = v6->x;
                    float y = v6->y;
                    float z = v6->z;
                    if (v8) {}
                    else {
                        if (pworld) {
                            TESWorldSpace* pworld_2 = pref->GetWorldSpace();
                            if (pworld == pworld_2) {
                                int32_t icellX = FloatToInt(someFloat) >> 12;
                                int32_t icellY = FloatToInt(y) >> 12;
                                TESObjectCELL* pcell_3 = pworld->GetCellFromCellCoord(icellX, icellY);
                            }
                        }
                    }
                }
                else {
                    if ((pref->GetParentCell()) && (!pref->GetParentCell()->GetInterior())) {
                        if (pactor_2) {
                            Warning("EDITOR: Actor does not have an editor location.  This should never happen.");
                        }
                        NiPoint3 loc = pref->GetStartingLocation();
                        int32_t icellX_2 = FloatToInt(loc.x) >> 12;
                        int32_t icellY_2 = FloatToInt(loc.y) >> 12;
                        pworld = pref->GetWorldSpace();
                        TESObjectCELL* pcell_4 = pworld->GetCellFromCellCoord(icellX_2, icellY_2);
                    }
                }
            }
        }
    }
    return aiFlagsa;
}

void TESSaveLoadGame::CreateChangedReference(uint32_t aiReferenceID) {
    ChangeData* pchange = m_pChanges->GetChanges(aiReferenceID);
    if (pchange) {
        Warning("CELLS: Reference in cell map has neither required flag.");
    }
}

void TESSaveLoadGame::SaveInitialData(TESForm* a1, uint32_t a2) {
    ReferenceData apMemory;
    TESObjectREFR* v11 = dynamic_cast<TESObjectREFR*>(a1);
    if (v11) {
        apMemory.~ReferenceData();
        ::new (&apMemory) ReferenceData;
        TESObjectCELL* ParentCell = v11->GetParentCell();
        TESWorldSpace* WorldSpace = nullptr;
        if (ParentCell) {
            WorldSpace = ParentCell->GetWorldSpace();
        }
        if (WorldSpace) {
            uint32_t CastingType = WorldSpace->iFormID;
            apMemory.iLocationID = AddNumericIDToArray(CastingType);
        }
        else {
            if (ParentCell) {
                uint32_t v4 = ParentCell->iFormID;
                apMemory.iLocationID = AddNumericIDToArray(v4);
            }
            else {
                if (!v11->GetRefPersists()) {
                    Warning(
                        "SAVELOAD: Error saving reference %08X: Non-persistent reference has no parent cell",
                        v11->iFormID
                    );
                }
                if (v11->GetRefPersists() && !(&v11->m_Extra)->GetPersistentCell()) {
                    Warning(
                        "SAVELOAD: Error saving reference %08X:Persistent reference has no parent cell and no "
                        "persistent cell extra data",
                        v11->iFormID
                    );
                }
                MobileObject* v8 = dynamic_cast<MobileObject*>(v11);
                if ((v8) && (v8->GetCurrentAIProcess())) {
                    BaseProcess* CurrentAIProcess = v8->GetCurrentAIProcess();
                    if (CurrentAIProcess->GetProcessLevel()) {
                        BaseProcess* v6 = v8->GetCurrentAIProcess();
                        if (v6->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
                            Warning(
                                "SAVELOAD: (Error saving actor %08X) Actor has a middle high process but no "
                                "parent cell",
                                v11->iFormID
                            );
                        }
                    }
                    else {
                        Warning(
                            "SAVELOAD: (Error saving actor %08X) Actor has a high process but no parent cell",
                            v11->iFormID
                        );
                    }
                }
            }
        }
        apMemory.Loc = *v11->QLocationOnReference();
        apMemory.Angle = v11->GetAngleOnReference();
        if ((a2 & 6) != 0) {
            pSaveLoadGameOLD->SaveGameDataOLD(&apMemory, 28u);
        }
    }
}

int32_t TESSaveLoadGame::GetInitialDataSaveSize(TESForm* a1, uint32_t a2) {
    TESObjectREFR* v3 = dynamic_cast<TESObjectREFR*>(a1);
    if ((!v3) && dynamic_cast<TESObjectCELL*>(a1)) {
        return 0;
    }
    if (v3 && ((a2 & 6) != 0)) {
        return 28;
    }
    return 0;
}

void TESSaveLoadGame::SavePluginList(BSFile* apFile) {
    uint8_t ilength;
    uint8_t icount;
    icount = pDataHandler->CountCompiled();
    uint32_t ipos = 0;
    if (m_pSaveLoadStats) {
        ipos = apFile->Tell();
    }
    WriteFile(apFile, &icount, 1u);
    for (uint8_t i = 0; i < static_cast<int32_t>(icount); ++i) {
        char* pfileName = pDataHandler->GetCompiledFile(i)->m_Filename;
        ilength = BSstrlen(pfileName);
        WriteFile(apFile, &ilength, 1u);
        WriteFile(apFile, pfileName, ilength);
    }
    if (m_pSaveLoadStats) {
        uint32_t ipos_2 = apFile->Tell();
        m_pSaveLoadStats->AddExtraStat(ipos_2 - ipos, "Plugin List");
    }
}

void TESSaveLoadGame::SaveHeader(BSFile* apFile, const char* apFileName) {
    uint32_t izero;
    uint32_t inextSaveNumber;
    _SYSTEMTIME localTime;
    uint32_t itotalPlayingTime;
    uint32_t iscreenshotSize;
    int32_t iwidth;
    uint32_t isize;
    float fgameDate;
    int32_t iheight;
    uint8_t cLocationNameLength;
    uint8_t cPlayerNameLength;
    uint16_t ilevel;
    ResetCurrentVersion();
    ResetVersions();
    uint32_t v3 = BSstrlen(pSaveGameHeaderString);
    WriteFile(apFile, pSaveGameHeaderString, v3);
    WriteFile(apFile, &m_cMajorVersion, 1u);
    WriteFile(apFile, &m_cMinorVersion, 1u);
    if (!m_iOriginalSaveVersion) {
        GetSystemTime(&m_OriginalSaveTime);
        m_iOriginalSaveVersion = m_cMinorVersion;
    }
    WriteFile(apFile, &m_OriginalSaveTime, 16u);
    WriteFile(apFile, &m_iOriginalSaveVersion, 4u);
    const char* pPlayerName = pPlayer->GetFullName();
    cPlayerNameLength = BSstrlen(pPlayerName) + 1;
    ilevel = pPlayer->GetLevel();
    BSStringT<char> mapname;
    pPlayer->GetMapNameForLocation(mapname);
    const char* pLocationName = mapname.pString;
    if (!pLocationName) {
        TESObjectCELL* ParentCell = pPlayer->GetParentCell();
        pLocationName = ParentCell->GetFormEditorID();
    }
    cLocationNameLength = 0;
    if (pLocationName) {
        cLocationNameLength = BSstrlen(pLocationName) + 1;
    }
    double DaysPassed = static_cast<double>(appCalendar.GetDaysPassed());
    fgameDate = appCalendar.GetHour() / 24.0 + DaysPassed;
    GetLocalTime(&localTime);
    iscreenshotSize = 0;
    iwidth = 256;
    iheight = 256;
    NiPixelData* ppixelData = nullptr;
    uint32_t ipixSize = 0;
    if (GetLoading()) {
        double ScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
        float finverseAspectRatio = ScreenHeight / static_cast<double>(Renderer::GetScreenWidth());
        int32_t inewHeight = iheight * static_cast<int32_t>(finverseAspectRatio);
        iscreenshotSize = 3 * (inewHeight * iwidth) + 8;
    }
    else {
        pIOManager->Resume();
        ppixelData = ScreenShot::TakeSaveGameScreenShot(iwidth, iheight, nullptr);
        pIOManager->Pause();
        if (ppixelData) {
            ipixSize = ppixelData->GetSizeInBytes(0, 0);
            iscreenshotSize = ipixSize + 8;
        }
    }
    itotalPlayingTime = pPlayer->GetTotalPlayingTime();
    isize = cLocationNameLength + (iscreenshotSize + cPlayerNameLength) + 36;
    WriteFile(apFile, &isize, 4u);
    if (GetLoading() || (apFileName && (std::strstr(apFileName, "quicksave") || std::strstr(apFileName, "autosave")))) {
        izero = 0;
        WriteFile(apFile, &izero, 4u);
    }
    else {
        inextSaveNumber = GetNextSaveNumber();
        WriteFile(apFile, &inextSaveNumber, 4u);
        ++m_iNextSaveNumber;
    }
    WriteFile(apFile, &cPlayerNameLength, 1u);
    WriteFile(apFile, pPlayerName, cPlayerNameLength);
    WriteFile(apFile, &ilevel, 2u);
    WriteFile(apFile, &cLocationNameLength, 1u);
    if (pLocationName) {
        WriteFile(apFile, pLocationName, cLocationNameLength);
    }
    WriteFile(apFile, &fgameDate, 4u);
    WriteFile(apFile, &itotalPlayingTime, 4u);
    WriteFile(apFile, &localTime, 16u);
    WriteFile(apFile, &iscreenshotSize, 4u);
    if (GetLoading()) {
        m_iSimulationFileSize += iscreenshotSize;
    }
    else {
        if (ppixelData) {
            WriteFile(apFile, &iwidth, 4u);
            WriteFile(apFile, &iheight, 4u);
            WriteFile(apFile, ppixelData->GetPixels(0u, 0u), ipixSize);
            if (ppixelData) {
                delete ppixelData;
            }
        }
    }
    if (m_pSaveLoadStats) {
        m_pSaveLoadStats->AddExtraStat(isize, "Save Game Header");
    }
}

uint32_t TESSaveLoadGame::GetCreatedExteriorCellFormID(uint32_t aiWorldSpaceID, int32_t aiCellX, int32_t aiCellY) {
    BSSimpleList<ExteriorCellReferenceData*>* pdataList;
    if (!m_pCreatedExteriorCells) {
        return 0;
    }
    if (!m_pCreatedExteriorCells->GetAt(aiWorldSpaceID, pdataList)) {
        return 0;
    }
    for (ExteriorCellReferenceData* pdata : *pdataList) {
        if (pdata && (aiCellX == pdata->iCellX && aiCellY == pdata->iCellY)) {
            uint32_t icellID = pdata->iFormID;
            pdataList->Remove(pdata);
            ourFree(pdata);
            return icellID;
        }
    }
    return 0;
}

void TESSaveLoadGame::SaveNumericIDArrays(BSFile* a1) {
    int32_t v4;
    int32_t v6;
    uint32_t apData;
    uint32_t CastingType;
    apData = m_pNumericIDArray->m_uiSize;
    WriteFile(a1, &apData, 4u);
    for (uint32_t uiIndex = 0; uiIndex < apData; ++uiIndex) {
        v6 = m_pNumericIDArray->GetAt(uiIndex);
        WriteFile(a1, &v6, 4u);
    }
    CastingType = m_pWorldSpaceIDArray->m_uiSize;
    WriteFile(a1, &CastingType, 4u);
    for (uint32_t i = 0; i < CastingType; ++i) {
        v4 = m_pWorldSpaceIDArray->GetAt(i);
        WriteFile(a1, &v4, 4u);
    }
    if (m_pSaveLoadStats) {
        char apDest[268];
        BSsprintf(apDest, 260, "Numeric ID Array(%i)", apData);
        m_pSaveLoadStats->AddExtraStat(4 * apData + 4, apDest);
        BSsprintf(apDest, 260, "WorldSpace ID Array(%i)", CastingType);
        m_pSaveLoadStats->AddExtraStat(4 * CastingType + 4, apDest);
    }
}

// Keeps the save's file name (BSFile's slot-6 getter: its path), without its folders and its 4-character extension,
// in m_pMostRecentSaveGame.
void TESSaveLoadGame::SetMostRecentSaveGame(BSFile* apFile) {
    if (m_pMostRecentSaveGame) {
        ourFree(this->m_pMostRecentSaveGame);
        this->m_pMostRecentSaveGame = nullptr;
    }
    if ((apFile) && (apFile->m_bGood)) {
        char Dst[268];
        char* pName = apFile->GetDevice();
        for (char* i = std::strstr(pName, "\\"); i; i = std::strstr(pName, "\\")) {
            pName = i + 1;
        }
        BSstrcpy(Dst, 260, pName);
        uint32_t v8 = BSstrlen(Dst);
        Dst[v8 - 4] = 0;
        uint32_t Size = BSstrlen(Dst) + 1;
        this->m_pMostRecentSaveGame = static_cast<char*>(ourMalloc(Size));
        BSstrcpy(this->m_pMostRecentSaveGame, Size, Dst);
    }
}

// With changes disallowed: collects the cells whose detach time is older than the respawn time (each exterior one with
// its grid coordinates), then, for every changed reference in such a cell, drops its havok move (or all its changes)
// and the changes of a created actor or creature.
void TESSaveLoadGame::CleanUpExpiredData() {
    bool bAllowChanges = m_bAllowChanges;
    m_bAllowChanges = false;
    NiTMapUIntExpiredCellDataList kExpiredCells(37);
    uint32_t uiHoursPassed = appCalendar.GetHoursPassed();
    NiTMapItem<uint32_t, ChangeData*>* pPos = m_pChanges->GetFirstPos();
    while (pPos) {
        ChangeData* pChange = nullptr;
        uint32_t uiFormID = 0;
        m_pChanges->GetNext(pPos, uiFormID, pChange);
        uint32_t uiFlags = pChange->iFlags;
        ExpiredCellData kCell;
        kCell.iCellX = 0x7FFFFFFF;
        kCell.iCellY = 0x7FFFFFFF;
        if (uiFlags & CHANGE_CELL_DETACHTIME) {
            uint32_t uiDetachTime = 0;
            if (pChange->pBuffer) {
                m_pBuffer = pChange->pBuffer;
                SaveBufferHeader kHeader;
                pSaveLoadGameOLD->LoadGameDataOLD(&kHeader, sizeof(SaveBufferHeader));
                if (kHeader.cFormType == CELL_ID && kHeader.cVersion >= 0x5B) {
                    LoadGameDataOLD(&uiDetachTime, sizeof(uiDetachTime));
                }
                m_pBuffer = nullptr;
            }
            else {
                TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
                if (pForm && pForm->GetFormType() == CELL_ID) {
                    TESObjectCELL* pCell = static_cast<TESObjectCELL*>(pForm);
                    uiDetachTime = pCell->TESObjectCELL::GetDetachTime();
                    if (!pCell->GetInterior()) {
                        uiFormID = pCell->GetWorldSpace()->iFormID;
                        kCell.iCellX = pCell->GetDataX();
                        kCell.iCellY = pCell->GetDataY();
                    }
                }
            }
            if (uiDetachTime && uiHoursPassed - uiDetachTime > TESObjectCELL::GetHoursToRespawnCell()) {
                ExpiredCellData* pData = nullptr;
                if (kCell.iCellX == 0x7FFFFFFF) {
                    kExpiredCells.SetAt(uiFormID, nullptr);
                }
                else {
                    pData = static_cast<ExpiredCellData*>(ourMalloc(sizeof(ExpiredCellData)));
                    ourMemcpy(pData, &kCell, sizeof(ExpiredCellData));
                    BSSimpleList<ExpiredCellData*>* pList = nullptr;
                    if (!kExpiredCells.GetAt(uiFormID, pList)) {
                        void* pMemory = ourMalloc(sizeof(BSSimpleList<ExpiredCellData*>));
                        pList = pMemory ? ::new (pMemory) BSSimpleList<ExpiredCellData*> : nullptr;
                        kExpiredCells.SetAt(uiFormID, pList);
                    }
                    pList->AddHead(pData);
                }
            }
        }
    }
    pPos = m_pChanges->GetFirstPos();
    while (pPos) {
        ChangeData* pChange = nullptr;
        uint32_t uiFormID = 0;
        m_pChanges->GetNext(pPos, uiFormID, pChange);
        uint32_t uiFlags = pChange->iFlags;
        uint32_t uiCellID = 0;
        int32_t iCellX = 0x7FFFFFFF;
        int32_t iCellY = 0x7FFFFFFF;
        TESObjectREFR* pRef = nullptr;
        uint32_t uiFormType = 0;
        SaveBufferHeader kHeader;
        kHeader.iSize = 0;
        if (pChange->pBuffer) {
            m_pBuffer = pChange->pBuffer;
            pSaveLoadGameOLD->LoadGameDataOLD(&kHeader, sizeof(SaveBufferHeader));
            uiFormType = kHeader.cFormType;
            if (uiFormType == REFR_ID || uiFormType == ACHR_ID || uiFormType == ACRE_ID || uiFormType == PMIS_ID ||
                uiFormType == PGRE_ID || uiFormType == PBEA_ID || uiFormType == PCBE_ID || uiFormType == PFLA_ID) {
                if (uiFlags & (CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) {
                    ReferenceData kData;
                    pSaveLoadGameOLD->LoadGameDataOLD(&kData, sizeof(ReferenceData));
                    uiCellID = kData.iLocationID;
                    iCellX = static_cast<int32_t>(kData.Loc.x) >> 12;
                    iCellY = static_cast<int32_t>(kData.Loc.y) >> 12;
                }
                uiCellID = GetNumericIDFromArray(uiCellID);
            }
            m_pBuffer = nullptr;
        }
        else {
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            if (pForm) {
                uiFormType = pForm->GetFormType();
                if (uiFormType == REFR_ID || uiFormType == ACHR_ID || uiFormType == ACRE_ID || uiFormType == PMIS_ID ||
                    uiFormType == PGRE_ID || uiFormType == PBEA_ID || uiFormType == PCBE_ID || uiFormType == PFLA_ID) {
                    pRef = static_cast<TESObjectREFR*>(pForm);
                    TESObjectCELL* pCell = pRef->GetParentCell();
                    if (pCell) {
                        if (pCell->GetInterior()) {
                            uiCellID = pCell->iFormID;
                            iCellX = 0;
                            iCellY = 0;
                        }
                        else {
                            uiCellID = pCell->GetWorldSpace()->iFormID;
                            iCellX = pCell->GetDataX();
                            iCellY = pCell->GetDataY();
                        }
                    }
                }
            }
        }
        if (uiCellID && iCellX != 0x7FFFFFFF && iCellY != 0x7FFFFFFF) {
            BSSimpleList<ExpiredCellData*>* pList = nullptr;
            if (kExpiredCells.GetAt(uiCellID, pList)) {
                bool bExpired = false;
                if (!pList) {
                    // An interior cell expires whole.
                    bExpired = true;
                }
                else {
                    while (pList && pList->m_item) {
                        ExpiredCellData* pData = pList->m_item;
                        if (pData && pData->iCellX == iCellX && pData->iCellY == iCellY) {
                            bExpired = true;
                            break;
                        }
                        pList = pList->m_pkNext;
                    }
                }
                if (bExpired) {
                    if ((uiFlags & CHANGE_REFR_HAVOK_MOVE) && !(uiFlags & CHANGE_REFR_MOVE)) {
                        if (uiFlags == CHANGE_REFR_HAVOK_MOVE) {
                            m_pChanges->RemoveChanges(uiFormID, true);
                        }
                        else if (pChange->pBuffer) {
                            // Rewrites the buffer without the leading ReferenceData and the havok move flag.
                            void* pOldBuffer = pChange->pBuffer;
                            uiFlags &= ~static_cast<uint32_t>(CHANGE_REFR_HAVOK_MOVE);
                            uint32_t uiSkip = sizeof(ReferenceData);
                            kHeader.iSize -= uiSkip;
                            uint32_t uiSize = kHeader.iSize;
                            void* pNewBuffer = CreateBuffer(kHeader.iSize + sizeof(SaveBufferHeader));
                            pSaveLoadGameOLD->SaveGameDataOLD(&kHeader, sizeof(SaveBufferHeader));
                            SaveGameDataOLD(static_cast<char*>(pOldBuffer) + uiSkip + sizeof(SaveBufferHeader), uiSize);
                            ourFree(pOldBuffer);
                            pChange->pBuffer = pNewBuffer;
                            pChange->iFlags = uiFlags;
                            m_pBuffer = nullptr;
                        }
                        else if (pRef) {
                            m_pChanges->RemoveChange(pRef, CHANGE_REFR_HAVOK_MOVE);
                        }
                        if (pRef) {
                            ResetReferencePosition(pRef, false);
                        }
                    }
                    if ((uiFormType == ACHR_ID || uiFormType == ACRE_ID) && pDataHandler->IsFormIDCreated(uiFormID)) {
                        m_pChanges->RemoveChanges(uiFormID, true);
                        if (pRef) {
                            delete pRef;
                            pRef = nullptr;
                        }
                    }
                }
            }
        }
    }
    NiTMapItem<uint32_t, BSSimpleList<ExpiredCellData*>*>* pCellPos = kExpiredCells.GetFirstPos();
    while (pCellPos) {
        BSSimpleList<ExpiredCellData*>* pList = nullptr;
        uint32_t uiCellID = 0;
        kExpiredCells.GetNext(pCellPos, uiCellID, pList);
        if (pList) {
            for (BSSimpleList<ExpiredCellData*>* pIter = pList; pIter && pIter->m_item; pIter->RemoveHead()) {
                ourFree(pIter->m_item);
            }
            delete pList;
        }
    }
    kExpiredCells.RemoveAll_1();
    m_bAllowChanges = bAllowChanges;
}

// Reads the buffer's two-byte header through the engine's save-load object. On PC the attach call is an empty stub;
// AttachAmmo is the TESObjectREFR member there.
void TESSaveLoadGame::LoadAttachedAnimationForReference(TESObjectREFR* apRef, void* apBuffer) {
    void* pOldBuffer = m_pBuffer;
    m_pBuffer = apBuffer;
    uint16_t usSize;
    pSaveLoadGameOLD->LoadGameDataOLD(&usSize, 2);
    apRef->AttachAmmo(nullptr);
    m_pBuffer = pOldBuffer;
}

// Only LoadGameDataOLD fills usSize, so the size passed to LoadHavokData is otherwise uninitialised, as the game does.
void TESSaveLoadGame::LoadHavokDataForReference(TESObjectREFR* apRef, void* apBuffer) {
    void* pOldBuffer = m_pBuffer;
    m_pBuffer = apBuffer;
    uint16_t usSize;
    pSaveLoadGameOLD->LoadGameDataOLD(&usSize, 2);
    apRef->LoadHavokData(usSize);
    m_pBuffer = pOldBuffer;
}

uint32_t TESSaveLoadGame::CalculateNextSaveNumber() {
    uint32_t isaveNumber;
    BSSimpleList<SaveGameFile*>* plist = m_pSaveGameList;
    bool bcreatedSaveGameList = false;
    if (!plist) {
        BuildSaveGameList();
        plist = m_pSaveGameList;
        bcreatedSaveGameList = true;
    }
    uint32_t ihighestNumberFound = 0;
    while (plist && !plist->IsEmpty()) {
        GetSaveGameDisplayData(plist->m_item, &isaveNumber, nullptr, nullptr, nullptr);
        if (isaveNumber > ihighestNumberFound) {
            ihighestNumberFound = isaveNumber;
        }
        plist = plist->m_pkNext;
    }
    if (bcreatedSaveGameList) {
        DeleteSaveGameList();
    }
    ResetCurrentVersion();
    ResetVersions();
    m_iNextSaveNumber = ihighestNumberFound + 1;
    return m_iNextSaveNumber;
}

// Calculates the next save number on first use.
uint32_t TESSaveLoadGame::GetNextSaveNumber() {
    if (!m_iNextSaveNumber) {
        CalculateNextSaveNumber();
    }
    return m_iNextSaveNumber;
}

void TESSaveLoadGame::SaveBuffer(BSFile* apFile, void* apBuffer, uint32_t aiSize) {
    WriteFile(apFile, apBuffer, aiSize);
}

uint32_t TESSaveLoadGame::CheckNewReference(TESForm* apForm, uint32_t aiFlags) {
    uint32_t v3 = apForm->TESForm::GetFormID();
    if (pDataHandler->IsFormIDCreated(v3)) {
        if (dynamic_cast<TESObjectREFR*>(apForm)) {
            aiFlags &= (~2u);
        }
        dynamic_cast<TESObjectCELL*>(apForm);
    }
    return aiFlags;
}

void TESSaveLoadGame::DeleteSaveGame(BSFile* apFile, const char* sFileName) {
    if (apFile) {
        BSFile* pNewFile = PrepForLoadSave(apFile, sFileName, TESSaveLoadGame::ACTION_DELETE);
        if (!pNewFile) {
            pNewFile = apFile;
        }
        BSSystemFile::DeleteFileA(pNewFile->GetDevice());
        CleanupLoadSave(pNewFile, TESSaveLoadGame::ACTION_DELETE);
    }
}

uint32_t TESSaveLoadGame::ReverseConvertNumericID(uint32_t aiFormID) {
    uint8_t v5 = (aiFormID & 0xFF000000u) >> 24;
    if (!m_pFileIndexArray || v5 == 255) {
        return aiFormID;
    }
    uint8_t v3 = -1;
    for (int32_t i = 0; i < m_iSavedPluginCount; ++i) {
        if (m_pFileIndexArray[i] == v5) {
            v3 = i;
        }
    }
    if (v3 == 255) {
        return 0;
    }

    return (v3 << 24) + (0xFFFFFFu & aiFormID);
}

// Saves the global variables: their count, then each global's form ID and value.
void TESSaveLoadGame::SaveGlobals(BSFile* apFile) {
    BSSimpleList<TESGlobal*>* pNode = pDataHandler->GetGlobalList();
    uint16_t usCount = pNode->ItemsInList();
    uint16_t usSize = 8 * usCount + 2;
    void* pBuffer = CreateBuffer(usSize);
    SaveGameDataOLD(&usCount, 2u);
    while (pNode) {
        TESGlobal* pGlobal = pNode->GetItem();
        if (pGlobal) {
            uint32_t uiFormID = pGlobal->GetFormID();
            float fValue = pGlobal->GetValue();
            SaveNumericID(&uiFormID, 4u);
            SaveGameDataOLD(&fValue, 4u);
        }
        pNode = pNode->GetNext();
    }
    SaveBuffer(apFile, pBuffer, usSize);
    DestroyBuffer(pBuffer);
    if (m_pSaveLoadStats) {
        m_pSaveLoadStats->AddExtraStat(usSize, "Global Variables");
    }
}

TESForm* TESSaveLoadGame::CreateReference(uint32_t aiFormID, CreatedReferenceData* apCreatedData) {
    TESForm* pform_2;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TES_REFERENCES, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        0x133B
    );
    TESForm* pform = TESForm::GetFormByNumericID(aiFormID);
    TESBoundObject* pbound = nullptr;
    if (apCreatedData->eType == TES_CREATED_REFERENCE_MAGICPROJECTILE ||
        (pform_2 = TESForm::GetFormByNumericID(apCreatedData->iBoundID),
         (pbound = dynamic_cast<TESBoundObject*>(pform_2)) != nullptr)) {
        if (pform) {
            TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(pform);
            if (!pref ||
                ((apCreatedData->eType && apCreatedData->eType != TES_CREATED_REFERENCE_PERSISTENT) ||
                 pref->GetObjectReference() != pbound)) {
                DeleteForm(pform);
                pform = nullptr;
            }
        }
        if (!pform) {
            if (apCreatedData->eType && apCreatedData->eType != TES_CREATED_REFERENCE_PERSISTENT) {
                if (apCreatedData->eType == TES_CREATED_REFERENCE_ARROWPROJECTILE) {
                    TESForm* pform_6;
                    ArrowProjectile* v20 = static_cast<ArrowProjectile*>(ourMalloc(sizeof(ArrowProjectile)));
                    if (v20) {
                        pform_6 = ::new (v20) ArrowProjectile;
                    }
                    else {
                        pform_6 = nullptr;
                    }
                    pform = pform_6;
                    TESObjectREFR* pref_3 = dynamic_cast<TESObjectREFR*>(pform_6);
                    pref_3->SetObjectReference(pbound);
                    pform->SetFormID(aiFormID, true);
                }
                else {
                    if (apCreatedData->eType == TES_CREATED_REFERENCE_MAGICPROJECTILE) {
                        TESForm* pform_7 = nullptr;
                        uint32_t iBoundID = apCreatedData->iBoundID;
                        if (iBoundID) {
                            if (iBoundID == 1) {
                                MagicProjectile* pmagicProjectile_3;
                                MagicBoltProjectile* v17 =
                                    static_cast<MagicBoltProjectile*>(ourMalloc(sizeof(MagicBoltProjectile)));
                                if (v17) {
                                    pmagicProjectile_3 = ::new (v17) MagicBoltProjectile;
                                }
                                else {
                                    pmagicProjectile_3 = nullptr;
                                }
                                pform_7 = pmagicProjectile_3;
                            }
                            else {
                                if (iBoundID == 3) {
                                    MagicProjectile* pmagicProjectile_2;
                                    MagicFogProjectile* v18 =
                                        static_cast<MagicFogProjectile*>(ourMalloc(sizeof(MagicFogProjectile)));
                                    if (v18) {
                                        pmagicProjectile_2 = ::new (v18) MagicFogProjectile;
                                    }
                                    else {
                                        pmagicProjectile_2 = nullptr;
                                    }
                                    pform_7 = pmagicProjectile_2;
                                }
                            }
                        }
                        else {
                            MagicProjectile* pmagicProjectile;
                            MagicBallProjectile* v19 =
                                static_cast<MagicBallProjectile*>(ourMalloc(sizeof(MagicBallProjectile)));
                            if (v19) {
                                pmagicProjectile = ::new (v19) MagicBallProjectile;
                            }
                            else {
                                pmagicProjectile = nullptr;
                            }
                            pform_7 = pmagicProjectile;
                        }
                        pform = pform_7;
                        pform_7->SetFormID(aiFormID, true);
                    }
                    else {
                        Warning(
                            "FORMS: Invalid created reference type %i for form %08X with bound object %08X in "
                            "location %08X",
                            apCreatedData->eType, aiFormID, apCreatedData->iBoundID, apCreatedData->RefData.iLocationID
                        );
                    }
                }
            }
            else {
                int32_t FormType = pbound->GetFormType();
                if (FormType == 42) {
                    TESForm* pform_3;
                    Character* v22 = static_cast<Character*>(ourMalloc(sizeof(Character)));
                    if (v22) {
                        pform_3 = ::new (v22) Character;
                    }
                    else {
                        pform_3 = nullptr;
                    }
                    pform = pform_3;
                }
                else {
                    if (FormType == 43) {
                        TESForm* pform_4;
                        Creature* v21 = static_cast<Creature*>(ourMalloc(sizeof(Creature)));
                        if (v21) {
                            pform_4 = ::new (v21) Creature;
                        }
                        else {
                            pform_4 = nullptr;
                        }
                        pform = pform_4;
                    }
                    else {
                        TESForm* pform_5;
                        TESObjectREFR* v5 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                        if (v5) {
                            pform_5 = ::new (v5) TESObjectREFR;
                        }
                        else {
                            pform_5 = nullptr;
                        }
                        pform = pform_5;
                    }
                }
                TESObjectREFR* pref_2 = dynamic_cast<TESObjectREFR*>(pform);
                pref_2->SetObjectReference(pbound);
                pform->SetFormID(aiFormID, true);
            }
        }
        if (pform) {
            pform->SetStillLoading(true);
        }
        return pform;
    }

    Warning(
        "SAVELOAD: Bound object %08X no longer exists.  Reference %08X will not be created.", apCreatedData->iBoundID,
        aiFormID
    );
    return nullptr;
}

int32_t TESSaveLoadGame::ComputeArmorHash(TESObjectREFR* apRef) {
    int32_t v10 = 0;
    BipedAnim* v9 = apRef->GetBiped();
    if (v9) {
        BIPED_OBJECT aeBipedObject[5];
        aeBipedObject[0] = BO_HEAD;
        aeBipedObject[1] = BO_HAIR;
        aeBipedObject[2] = BO_UPPER_BODY;
        aeBipedObject[3] = BO_LEFT_HAND;
        aeBipedObject[4] = BO_RIGHT_HAND;
        int32_t v8 = 5;
        for (int32_t i = 0; i < v8; ++i) {
            BIPOBJECT* BipObject = v9->GetBipObject(aeBipedObject[i]);
            if ((BipObject) && (BipObject->pParent)) {
                uint32_t CastingType = BipObject->pParent->iFormID;
                uint32_t v5 = ReverseConvertNumericID(CastingType);
                v10 = (v10 << 16) + (v5 + (v10 << 6)) - v10;
            }
        }
    }
    return v10;
}

void TESSaveLoadGame::LoadAnimations() {
    NiTMapItem<uint32_t, void*>* it;
    it = m_pAnimationMap->GetFirstPos();
    while (it) {
        void* pbuffer;
        uint32_t iformID;
        TESObjectREFR* pref;
        m_pAnimationMap->GetNext(it, iformID, pbuffer);
        if (iformID) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
            pref = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        }
        else {
            pref = (TESObjectREFR*)pPlayer;
        }
        if ((pref) && (pref->TESObjectREFR::Get3D())) {
            LoadAnimationForReference(pref, iformID, pbuffer);
        }

        ourFree(pbuffer);
        m_pAnimationMap->RemoveAt(iformID);
    }
}

void TESSaveLoadGame::LoadAnimationForReference(TESObjectREFR* apRef, uint32_t aiFormID, void* apBuffer) {
    char apData[4];
    Animation* panimation;
    if (apRef == pPlayer) {
        if (aiFormID) {
            panimation = pPlayer->GetAnimation(false);
        }
        else {
            panimation = pPlayer->GetAnimation(true);
        }
    }
    else {
        panimation = apRef->GetAnimation();
    }
    MobileObject* pmob = dynamic_cast<MobileObject*>(apRef);
    HighProcess* phighProcess = nullptr;
    ANIMATION_ACTION sanimAction = ANIM_ACTION_NONE;
    BSAnimGroupSequence* pseq = nullptr;
    if ((pmob) && (aiFormID)) {
        phighProcess = dynamic_cast<HighProcess*>(pmob->pCurrentProcess);
        if (phighProcess) {
            sanimAction = phighProcess->GetAnimAction();
            pseq = phighProcess->GetAnimActionAnimSeq();
        }
    }

    void* ptempBuffer = m_pBuffer;
    m_pBuffer = apBuffer;
    pSaveLoadGameOLD->LoadGameDataOLD(apData, 2u);
    if (panimation) {
        panimation->LoadGameTES(apRef);
    }
    m_pBuffer = ptempBuffer;
    if (phighProcess) {
        if (pseq) {
            // The sequence pointer's value less 8 is passed as the section, as the game does; the pointer's value is
            // used as an integer.
            pseq =
                panimation->GetCurrentSequence(static_cast<ANIM_GROUP_SECTION>(reinterpret_cast<uintptr_t>(pseq) - 8));
        }
        phighProcess->SetAnimAction(sanimAction, pseq);
    }
}

void TESSaveLoadGame::LoadAttachedAnimations() {
    NiTMapItem<uint32_t, void*>* it;
    it = m_pAttachedAnimationMap->GetFirstPos();
    while (it) {
        void* pbuffer;
        uint32_t iformID;
        m_pAttachedAnimationMap->GetNext(it, iformID, pbuffer);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
        TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        if (!pref) {
            goto LABEL_10;
        }
        if (pref->TESObjectREFR::Get3D()) {
            LoadAttachedAnimationForReference(pref, pbuffer);
        LABEL_10:;
            ourFree(pbuffer);
            m_pAttachedAnimationMap->RemoveAt(iformID);
        }
        else {
            if (m_pNewChanges) {
                m_pNewChanges->AddChange(pref, 0);
            }
            else {
                m_pChanges->AddChange(pref, 0);
            }
            m_pAttachedAnimationMap->RemoveAt(iformID);
        }
    }
}

void TESSaveLoadGame::LoadCharControllers() {
    NiTMapItem<uint32_t, void*>* it;
    it = m_pCharControllerMap->GetFirstPos();
    while (it) {
        void* pbuffer;
        uint32_t iformID;
        m_pCharControllerMap->GetNext(it, iformID, pbuffer);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
        MobileObject* pmob = dynamic_cast<MobileObject*>(FormByNumericID);
        if (pmob) {
            MiddleHighProcess* phighProcess = dynamic_cast<HighProcess*>(pmob->pCurrentProcess);
            if (phighProcess) {
                char apData[4];
                void* ptempBuffer = m_pBuffer;
                m_pBuffer = pbuffer;
                pSaveLoadGameOLD->LoadGameDataOLD(apData, 2u);
                if (phighProcess->GetCharController()) {
                    phighProcess->LoadCharController(pmob);
                }
                else {
                    if (!pmob->GetDelete() || !pmob->GetDisabled()) {
                        int32_t CastingType = pmob->iFormID;
                        Warning(
                            "SAVELOAD: (LoadCharControllers()) Mob %s %08X does not have a character controller to "
                            "load.",
                            pmob->GetFormEditorID(), CastingType
                        );
                    }
                }
                m_pBuffer = ptempBuffer;
            }
        }
        ourFree(pbuffer);
        m_pCharControllerMap->RemoveAt(iformID);
    }
}

void TESSaveLoadGame::LoadHavokData() {
    NiTMapItem<uint32_t, void*>* it;
    it = m_pHavokDataMap->GetFirstPos();
    while (it) {
        void* pbuffer;
        uint32_t iformID;
        m_pHavokDataMap->GetNext(it, iformID, pbuffer);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
        TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        if (!pref) {
            goto LABEL_10;
        }
        if (pref->TESObjectREFR::Get3D()) {
            LoadHavokDataForReference(pref, pbuffer);
        LABEL_10:;
            ourFree(pbuffer);
            m_pHavokDataMap->RemoveAt(iformID);
        }
        else {
            if (m_pNewChanges) {
                m_pNewChanges->AddChange(pref, 0);
            }
            else {
                m_pChanges->AddChange(pref, 0);
            }
            m_pHavokDataMap->RemoveAt(iformID);
        }
    }
}

void TESSaveLoadGame::QueueFormForFinalization(TESForm* apForm) {
    if (!m_QueuedFormList.IsInList(apForm)) {
        m_QueuedFormList.AddHead(apForm);
    }
}

void TESSaveLoadGame::FinalizeQueuedForms() {
    for (TESForm* pform : m_QueuedFormList) {
        if (pform) {
            ArrowProjectile* parrow = dynamic_cast<ArrowProjectile*>(pform);
            if (parrow) {
                TESObjectREFR* pref = parrow->GetAttachedRef();
                if (pref) {
                    NiAVObject* pobj3D = pref->TESObjectREFR::Get3D();
                    if (pobj3D) {
                        NiUpdateData UpdateData(0.0f, false, false);
                        pobj3D->Update(UpdateData);
                    }
                }
                parrow->ProcessCollision();
            }
        }
    }
    m_QueuedFormList.RemoveAll();
}

void TESSaveLoadGame::BuildSaveGameList() {
    BSSimpleList<SaveGameFile*>* v6;
    char String1[268];
    if (m_pSaveGameList) {
        DeleteSaveGameList();
    }
    BSSimpleList<SaveGameFile*>* v9 =
        static_cast<BSSimpleList<SaveGameFile*>*>(ourMalloc(sizeof(BSSimpleList<SaveGameFile*>)));
    if (v9) {
        v6 = ::new (v9) BSSimpleList<SaveGameFile*>;
    }
    else {
        v6 = nullptr;
    }
    m_pSaveGameList = v6;
    const char* PersonalPath = BSUtilities::GetPersonalPath();
    std::strcpy(String1, PersonalPath);
    const char* v2 = sLocalSavePath.String();
    std::strcat(String1, v2);
    std::strcat(String1, "*.ess");
    for (const platform::FileEntry& kEntry : platform::FindFiles(String1)) {
        if (kEntry.size) {
            SaveGameFile* v5;
            char apDest[264];
            const char* v3 = sLocalSavePath.String();
            const void* CellOrCoords_0 = v3;
            BSsprintf(apDest, 260, "%s%s%s", BSUtilities::GetPersonalPath(), CellOrCoords_0, kEntry.name.c_str());
            SaveGameFile* apMemory = static_cast<SaveGameFile*>(ourMalloc(sizeof(SaveGameFile)));
            if (apMemory) {
                v5 = ::new (apMemory) SaveGameFile(apDest, NiFile::READ_ONLY, 0x20000u);
            }
            else {
                v5 = nullptr;
            }
            SaveGameFile* apNewItem = v5;
            m_pSaveGameList->Insert(v5, &TESSaveLoadGame::SaveGameCompare);
        }
    }
}

void TESSaveLoadGame::DeleteSaveGameList() {
    if (this->m_pSaveGameList) {
        for (BSSimpleList<SaveGameFile*>* plist = this->m_pSaveGameList; plist; plist = plist->m_pkNext) {
            BSFile* pfile = plist->m_item;
            if (pfile) {
                delete pfile;
            }
        }
        this->m_pSaveGameList->RemoveAll();
        BSSimpleList<SaveGameFile*>* m_pSaveGameList = this->m_pSaveGameList;
        if (m_pSaveGameList) {
            delete m_pSaveGameList;
        }
        this->m_pSaveGameList = nullptr;
    }
}

void TESSaveLoadGame::GetSaveGameDisplayData(
    BSFile* apFile,
    uint32_t* apSaveGameNumber,
    char* apFileName,
    char* apLevel,
    char* apTotalPlayingTime
) {
    if (apFileName) {
        BSstrcpy(apFileName, 255, "");
    }
    if (!RetrieveSaveGameData(apFile, reinterpret_cast<int32_t*>(apSaveGameNumber), apLevel, apTotalPlayingTime)) {
        GetSaveGameData(
            apFile, apSaveGameNumber, nullptr, apLevel, nullptr, nullptr, nullptr, apTotalPlayingTime, nullptr, 0
        );
        if (apFileName) {
            // BSFile's slot-6 getter returns the address of the file's name.
            char* v6 = apFile->GetDevice();
            if (std::strstr(v6, "quicksave")) {
                const char* v7 = sMenuDisplayQuicksaveName.String();
                BSstrcpy(apFileName, 255, v7);
                BSstrcat(apFileName, 255, " - ");
                BSstrcat(apFileName, 255, apLevel);
            }
            else {
                char* v8 = apFile->GetDevice();
                if (std::strstr(v8, "autosave")) {
                    const char* v10 = sMenuDisplayAutosaveName.String();
                    BSstrcpy(apFileName, 255, v10);
                    BSstrcat(apFileName, 255, " - ");
                    BSstrcat(apFileName, 255, apLevel);
                }
                else {
                    char* v9 = apFile->GetDevice();
                    char* Src = std::strrchr(v9, 92) + 1;
                    if (_strnicmp(Src, "Save ", 5u)) {
                        BSstrcpy(apFileName, 255, Src);
                        uint32_t v12 = BSstrlen(apFileName);
                        if (v12 > 4) {
                            apFileName[v12 - 4] = 0;
                        }
                        if (v12 > 18) {
                            apFileName[18] = 0;
                        }
                    }
                }
            }
        }
    }
}

NiRefObject* TESSaveLoadGame::GetSaveGameData(
    BSFile* apFile,
    uint32_t* a3,
    char* Dst,
    char* apDest,
    char* a6,
    char* a7,
    char* a8,
    char* a9,
    uint32_t* a10,
    char a11
) {
    uint32_t v15;
    float v18;
    _SYSTEMTIME v20;
    uint16_t a5;
    BSFile* Save = nullptr;
    if (a11) {
        Save = apFile;
    }
    else {
        Save = PrepForLoadSave(apFile, nullptr, TESSaveLoadGame::ACTION_READ);
    }
    uint32_t aiSize = 0;
    if ((Save) && (Save->m_bGood)) {
        aiSize = LoadHeader(Save, false);
    }

    if (aiSize) {
        a5 = 0;
        v18 = 0.0f;
        NiRefObject* v16 = ReadSaveGameDataOLD(apFile, aiSize, a3, Dst, &a5, a6, &v18, &v20, &v15, a10);
        if (apDest) {
            BSsprintf(apDest, 300, "%s %i", sMenuDisplayLevelString.String(), a5);
        }
        if (a7) {
            BSsprintf(a7, 300, "%s %i", sMenuDisplayDayString.String(), static_cast<int64_t>(v18));
        }
        if (a9) {
            uint32_t v14 = v15 / 0x36EE80;
            v15 %= 0x36EE80u;
            uint32_t v13 = v15 / 0xEA60;
            v15 %= 0xEA60u;
            BSsprintf(a9, 300, "%02i:%02i:%02i", v14, v13, v15 / 1000);
        }
        if (a8) {
            BSsprintf(a8, 300, "%d/%d/%02d %02d:%02d", v20.wMonth, v20.wDay, v20.wYear, v20.wHour, v20.wMinute);
        }
        Save->Seek(0, NiFile::ms_iSeekSet);
        if (!a11) {
            CleanupLoadSave(Save, TESSaveLoadGame::ACTION_READ);
        }
        return v16;
    }

    if (a3) {
        *a3 = 0;
    }
    if (Dst) {
        BSstrcpy(Dst, 300, "");
    }
    if (apDest) {
        BSstrcpy(apDest, 300, "");
    }
    if (a6) {
        BSstrcpy(a6, 300, "");
    }
    if (a7) {
        BSstrcpy(a7, 300, "");
    }
    if (a8) {
        BSstrcpy(a8, 300, "");
    }
    if (a9) {
        BSstrcpy(a9, 300, "");
    }
    if (a10) {
        *a10 = 0;
    }
    if (!a11) {
        CleanupLoadSave(Save, TESSaveLoadGame::ACTION_READ);
    }
    return nullptr;
}

NiRefObject* TESSaveLoadGame::ReadSaveGameDataOLD(
    BSFile* a1,
    uint32_t aiSize,
    uint32_t* a3,
    void* a4,
    uint16_t* a5,
    char* Dst,
    float* a7,
    _SYSTEMTIME* a8,
    uint32_t* a9,
    uint32_t* a10
) {
    _SYSTEMTIME result;
    uint32_t auiWidth;
    NiPixelFormat akFormat;
    uint32_t v25;
    uint32_t apData;
    uint32_t v27;
    _SYSTEMTIME apDest;
    uint32_t v30;
    uint8_t v31;
    float v33;
    uint16_t v34;
    void* Block = CreateBuffer(aiSize);
    LoadBuffer(a1, Block, aiSize);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 56u) {
        LoadGameDataOLD(&apData, 4u);
        if (a3) {
            *a3 = apData;
        }
    }
    if ((pSaveLoadGameOLD->GetCurrentVersion() < 56u) && (a3)) {
        *a3 = 0;
    }

    LoadGameDataOLD(&v31, 1u);
    if (a4) {
        LoadGameDataOLD(a4, v31);
    }
    else {
        AdvanceBuffer(v31);
    }
    v34 = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        LoadGameDataOLD(&v34, 2u);
    }
    if (a5) {
        *a5 = v34;
    }
    LoadGameDataOLD(&v31, 1u);
    if (Dst) {
        if (v31) {
            LoadGameDataOLD(Dst, v31);
        }
        else {
            const char* v11 = sMenuDisplayUnknownLocationString.String();
            BSstrcpy(Dst, 300, v11);
        }
    }
    else {
        if (v31) {
            AdvanceBuffer(v31);
        }
    }
    LoadGameDataOLD(&v33, 4u);
    if (a7) {
        *a7 = v33;
    }
    v27 = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 29u) {
        LoadGameDataOLD(&v27, 4u);
    }
    if (a9) {
        *a9 = v27;
    }
    ourMemset(&apDest, 0, sizeof(apDest));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 56u) {
        LoadGameDataOLD(&apDest, 16u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 56u) {
        apDest = *a1->GetLastSaveTime(&result);
    }
    if (a8) {
        *a8 = apDest;
    }
    NiRefObject* v29 = nullptr;
    LoadGameDataOLD(&v30, 4u);
    if (v30) {
        if (a10) {
            NiPixelData* v15;
            LoadGameDataOLD(&auiWidth, 4u);
            LoadGameDataOLD(&v25, 4u);
            *a10 = v25;
            if (pSaveLoadGameOLD->GetCurrentVersion() < 46u) {
                AdvanceBuffer(36u);
            }
            akFormat = NiPixelFormat_RGB24;
            void* apMemory = NiMemObject::operator new(sizeof(NiPixelData));
            if (apMemory) {
                v15 = ::new (apMemory) NiPixelData(auiWidth, auiWidth, akFormat, 1u, 1u);
            }
            else {
                v15 = nullptr;
            }
            LoadGameDataOLD(v15->GetPixels(0u, 0u), 3 * (v25 * auiWidth));
            NiTexture::FormatPrefs arPrefs;
            arPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
            arPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
            arPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
            v29 = NiSourceTexture::Create(v15, NiFixedString("SaveGameTexture").GetHandleString(), arPrefs);
            v29->IncRefCount();
        }
        else {
            AdvanceBuffer(v30);
        }
    }
    DestroyBuffer(Block);
    return v29;
}

char TESSaveLoadGame::RetrieveSaveGameData(BSFile* apFile, int32_t* arIndex, char* apDest, char* Dst) {
    // BSFile's slot-6 getter returns the address of the file's name.
    char* v5 = apFile->GetDevice();
    char* Str = std::strrchr(v5, 92) + 1;
    char* v17 = std::strstr(Str, "Playing Time");
    char* v15 = std::strstr(Str, "-");
    if (!v17 || (!v15 || std::strncmp(Str, "Save ", 5u))) {
        return 0;
    }
    if (arIndex) {
        char String[268];
        uint32_t aiLen = v15 - (Str + 5);
        BSstrncpy(String, Str + 5, aiLen);
        String[aiLen] = 0;
        *arIndex = std::atoi(String);
    }
    if (apDest || Dst) {
        char* apString2 = nullptr;
        char* v11 = nullptr;
        for (char* i = Str;; v11 = i) {
            i = std::strstr(i + 1, ",");
            if (!i) {
                break;
            }
            apString2 = v11;
        }
        if (!apString2 || !v11) {
            return 0;
        }
        if (apDest) {
            char apString1[264];
            apString2 += 2;
            size_t v10 = BSstrlen(Str);
            while (*apString2 != 32 && apString2 < &Str[v10]) {
                ++apString2;
            }
            v10 = v11 - apString2;
            BSstrncpy(apString1, apString2, v11 - apString2);
            apString1[v10] = 0;
            BSsprintf(apDest, 255, "%s%s", sMenuDisplayLevelString.String(), apString1);
        }
        if (Dst) {
            BSstrcpy(Dst, 255, v17 + 13);
            char* v8 = std::strstr(Dst, ".ess");
            if (v8) {
                *v8 = 0;
            }
            for (uint32_t j = 0; j < BSstrlen(Dst); ++j) {
                if (Dst[j] == 46) {
                    Dst[j] = 58;
                }
            }
        }
    }
    return 1;
}

void TESSaveLoadGame::GenerateSaveGameFileName(char* apFileName) {
    if (apFileName) {
        FileFinder::FILE_LOCATION v5;
        char Src[268];
        const char* v22;
        bool v17 = true;
        BSStringT<char> v18;
        int32_t v16 = 0;
        const char* pPlayerName = pPlayer->GetFullName();
        pPlayer->GetMapNameForLocation(v18);
        // The location is the map name, else the parent cell's editor ID, else "".
        v22 = v18.String();
        if (!v22) {
            if (pPlayer->GetParentCell()) {
                TESObjectCELL* ParentCell = pPlayer->GetParentCell();
                v22 = ParentCell->GetFormEditorID();
            }
            else {
                v22 = "";
            }
        }
        do {
            char apDest[264];
            char Dst[8];
            if (v16) {
                BSsprintf(Dst, 5, " #%d", v16 + 1);
            }
            else {
                BSstrcpy(Dst, 5, "");
            }
            uint32_t itotalPlayingTime = pPlayer->GetTotalPlayingTime();
            uint32_t v15 = itotalPlayingTime / 0x36EE80;
            uint32_t v14 = itotalPlayingTime % 0x36EE80 / 0xEA60;
            uint32_t v9 = itotalPlayingTime % 0x36EE80 % 0xEA60 / 1000;
            int32_t v8 = v14;
            uint32_t v7 = itotalPlayingTime / 0x36EE80;
            int32_t Level = pPlayer->GetLevel();
            BSsprintf(
                Src, 260, "Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i", GetNextSaveNumber(), pPlayerName,
                v22, Level, v7, v8, v9, Dst
            );
            for (char* pinvalidCharacter = std::strpbrk(Src, "\\/:*<>?|\""); pinvalidCharacter;
                 pinvalidCharacter = std::strpbrk(pinvalidCharacter + 1, "\\/:*<>?|\"")) {
                if (*pinvalidCharacter == 34) {
                    *pinvalidCharacter = 39;
                }
                else {
                    *pinvalidCharacter = 32;
                }
            }
            const char* v4 = sLocalSavePath.String();
            const void* CellOrCoords_0 = v4;
            BSsprintf(apDest, 260, "%s%s%s.ess", BSUtilities::GetPersonalPath(), CellOrCoords_0, Src);
            v5 = FileFinder::Locate(apDest, nullptr, 0, 0xFFFFFFFFu);
            v17 = v5 == FileFinder::NOTFOUND;
            ++v16;
        }
        while (v5);
        BSstrcpy(apFileName, 260, Src);
    }
}

uint32_t TESSaveLoadGame::LoadHeader(BSFile* apFile, bool abFullLoad) {
    uint32_t isize;
    char Str1[268];
    m_iFileStartPosition = 0;
    uint32_t v7 = BSstrlen(pSaveGameHeaderString);
    if (!ReadFile(apFile, Str1, v7)) {
        return 0;
    }
    if (Str1[0]) {
        if (!std::strncmp(Str1, "CON ", 4u)) {
            apFile->Seek(0xD000, NiFile::ms_iSeekSet);
            m_iFileStartPosition = 0xD000;
            uint32_t v4 = BSstrlen(pSaveGameHeaderString);
            ReadFile(apFile, Str1, v4);
        }
        uint32_t v5 = BSstrlen(pSaveGameHeaderString);
        if (std::strncmp(Str1, pSaveGameHeaderString, v5)) {
            return 0;
        }
    }
    else {
        apFile->Seek(0, NiFile::ms_iSeekSet);
    }
    ReadFile(apFile, &m_cMajorVersion, 1u);
    ReadFile(apFile, &m_cMinorVersion, 1u);
    ResetVersions();
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 82u) {
        if (abFullLoad) {
            ReadFile(apFile, &m_OriginalSaveTime, 16u);
            ReadFile(apFile, &m_iOriginalSaveVersion, 4u);
        }
        else {
            apFile->Seek(20, NiFile::ms_iSeekCur);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 82u && abFullLoad) {
        ourMemset(&m_OriginalSaveTime, 255, sizeof(m_OriginalSaveTime));
        m_iOriginalSaveVersion = -1;
    }
    ReadFile(apFile, &isize, 4u);
    if (m_cMajorVersion || (abFullLoad && m_cMinorVersion <= 18u)) {
        char a1[523];
        std::sprintf(
            a1,
            "Save game version is %i.%02i and current version is %i.%02i, so errors may occur.  Continue trying to "
            "load?",
            m_cMajorVersion, m_cMinorVersion, 0, 125
        );
        pSaveLoadGameOLD->SetLoading(true);
        const char* v6 = sNo.String();
        char iresult = Interface::CreateModalMessageMenu(a1, 1, 5, sYes.String(), v6, nullptr);
        pSaveLoadGameOLD->SetLoading(false);
        if (iresult == 2) {
            return -1;
        }
    }
    return isize;
}

void TESSaveLoadGame::AddCreatedBaseObject(TESForm* apForm) {
    int32_t item;
    if (apForm) {
        uint32_t CastingType = apForm->iFormID;
        if (pDataHandler->IsFormIDCreated(CastingType)) {
            item = apForm->iFormID;
            if (!m_CreatedBaseObjectsList.IsInList(*reinterpret_cast<uint32_t*>(&item))) {
                uint32_t a2 = apForm->iFormID;
                m_CreatedBaseObjectsList.AddHead(a2);
            }
        }
        else {
            Warning("SAVELOAD: Attempting to add non-created form %08X to created base objects list.", apForm->iFormID);
        }
    }
    else {
        Warning("SAVELOAD: Attempting to add null object to created base objects list.");
    }
}

void TESSaveLoadGame::SaveCreatedBaseObjects(BSFile* apFile) {
    uint32_t icount;
    BSSimpleList<uint32_t>* plist;
    icount = 0;
    for (plist = &m_CreatedBaseObjectsList; plist; plist = plist->m_pkNext) {
        uint32_t iformID = plist->m_item;
        TESForm* pform = TESForm::GetFormByNumericID(iformID);
        if (pform) {
            TESBoundObject* pbound = dynamic_cast<TESBoundObject*>(pform);
            SpellItem* pspell = dynamic_cast<SpellItem*>(pform);
            TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(pform);
            if (!penchantable || penchantable->pFormEnchanting) {
                if ((penchantable) && (penchantable->pFormEnchanting)) {
                    uint32_t CastingType = penchantable->pFormEnchanting->iFormID;
                    if (pDataHandler->IsFormIDCreated(CastingType)) {
                        ++icount;
                    }
                }

                if (pbound || pspell) {
                    ++icount;
                }
            }
        }
    }
    WriteFile(apFile, &icount, 4u);
    if (icount) {
        for (plist = &m_CreatedBaseObjectsList;; plist = plist->m_pkNext) {
            EnchantmentItem* penchant;
            uint32_t iformID_2;
            TESForm* pform_2;
            while (true) {
                if (!plist) {
                    return;
                }
                iformID_2 = plist->m_item;
                pform_2 = TESForm::GetFormByNumericID(iformID_2);
                if (pform_2) {
                    TESBoundObject* pbound_2 = dynamic_cast<TESBoundObject*>(pform_2);
                    SpellItem* pspell_2 = dynamic_cast<SpellItem*>(pform_2);
                    if (pbound_2 || pspell_2) {
                        break;
                    }
                }
            LABEL_31:;
                plist = plist->m_pkNext;
            }
            TESEnchantableForm* penchantable_2 = dynamic_cast<TESEnchantableForm*>(pform_2);
            if (!penchantable_2) {
                goto LABEL_28;
            }
            penchant = penchantable_2->pFormEnchanting;
            if (penchant) {
                uint32_t v3;
                v3 = penchant->iFormID;
                if (pDataHandler->IsFormIDCreated(v3)) {
                    penchant->Save();
                    uint32_t ilength = penchant->GetFormBufferSize();
                    uint32_t v6 = ilength;
                    void* FormBuffer = penchant->GetFormBuffer();
                    WriteFile(apFile, FormBuffer, v6);
                    if (m_pSaveLoadStats) {
                        LoadFormHeader header;
                        header.cFormType = penchant->GetFormType();
                        header.iFormID = penchant->iFormID;
                        header.iSize = ilength;
                        header.iFlags = 0;
                        m_pSaveLoadStats->AddStats(&header);
                    }
                    penchant->FreeFormBuffer();
                }
            LABEL_28:;
                pform_2->Save();
                uint32_t ilength_2 = pform_2->GetFormBufferSize();
                uint32_t v7 = ilength_2;
                void* v5 = pform_2->GetFormBuffer();
                WriteFile(apFile, v5, v7);
                if (m_pSaveLoadStats) {
                    LoadFormHeader header_2;
                    header_2.cFormType = pform_2->GetFormType();
                    header_2.iFormID = iformID_2;
                    header_2.iSize = ilength_2;
                    header_2.iFlags = 0;
                    m_pSaveLoadStats->AddStats(&header_2);
                }
                pform_2->FreeFormBuffer();
                goto LABEL_31;
            }
            Warning(
                "SAVELOAD: Enchantable item %08X with no enchantment exists in the created base objects list.",
                iformID_2
            );
        }
    }
}

int16_t TESSaveLoadGame::AddWorldSpaceIDToArray(int32_t a1) {
    uint32_t CastingType = m_pWorldSpaceIDArray->m_uiSize;
    for (uint32_t uiIndex = 0; uiIndex < CastingType; ++uiIndex) {
        int32_t v2 = m_pWorldSpaceIDArray->GetAt(uiIndex);
        if (v2 == a1) {
            return uiIndex;
        }
    }
    return m_pWorldSpaceIDArray->Add(*reinterpret_cast<uint32_t*>(&a1));
}

int32_t TESSaveLoadGame::GetWorldSpaceIDFromArray(uint16_t a1) {
    if (a1 <= m_pWorldSpaceIDArray->m_uiSize) {
        return m_pWorldSpaceIDArray->GetAt(a1);
    }

    return 0;
}

void TESSaveLoadGame::RemoveFormFromInitFormArray(TESForm* apForm) {
    if (m_pInitArray) {
        uint32_t isize = m_pInitArray->m_uiSize;
        for (uint32_t i = 0; i < isize; ++i) {
            FormAndFlags* pformAndFlags = m_pInitArray->GetAt(i);
            if (pformAndFlags && pformAndFlags->pForm == apForm) {
                m_pInitArray->RemoveAt(i);
                ourFree(pformAndFlags);
                return;
            }
        }
    }
}

void TESSaveLoadGame::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t TESSaveLoadGame::SaveGameCompare(SaveGameFile* apCompare1, SaveGameFile* apCompare2) {
    _FILETIME v11;
    _FILETIME v12;
    _FILETIME time2;
    _FILETIME time1;
    ourMemset(&time1, 0, sizeof(time1));
    ourMemset(&time2, 0, sizeof(time2));
    if (apCompare1->GetHasFileTime()) {
        _FILETIME* FileTime = ::new (&v12) _FILETIME(apCompare1->GetFileTime());
        uint32_t dwHighDateTime = FileTime->dwHighDateTime;
        time1.dwLowDateTime = FileTime->dwLowDateTime;
        time1.dwHighDateTime = dwHighDateTime;
    }
    else {
        // BSFile's slot-6 getter returns the address of the file's name.
        char* v4 = apCompare1->GetDevice();
        // The file's last write time when the search finds it, else zero.
        std::vector<platform::FileEntry> kFound = platform::FindFiles(v4);
        if (!kFound.empty()) {
            time1.dwLowDateTime = static_cast<uint32_t>(kFound.front().writeTime);
            time1.dwHighDateTime = static_cast<uint32_t>(kFound.front().writeTime >> 32);
        }
        apCompare1->SetFileTime(time1);
    }
    if (apCompare2->GetHasFileTime()) {
        _FILETIME* v5 = ::new (&v11) _FILETIME(apCompare2->GetFileTime());
        uint32_t dwLowDateTime = v5->dwLowDateTime;
        uint32_t v7 = v5->dwHighDateTime;
        time2.dwLowDateTime = dwLowDateTime;
        time2.dwHighDateTime = v7;
    }
    else {
        char* v8 = apCompare2->GetDevice();
        std::vector<platform::FileEntry> kFound = platform::FindFiles(v8);
        if (!kFound.empty()) {
            time2.dwLowDateTime = static_cast<uint32_t>(kFound.front().writeTime);
            time2.dwHighDateTime = static_cast<uint32_t>(kFound.front().writeTime >> 32);
        }
        apCompare2->SetFileTime(time2);
    }
    if (time1.dwHighDateTime > time2.dwHighDateTime) {
        return -1;
    }
    if (time1.dwHighDateTime < time2.dwHighDateTime) {
        return 1;
    }
    if (time1.dwLowDateTime <= time2.dwLowDateTime) {
        return time1.dwLowDateTime < time2.dwLowDateTime;
    }

    return -1;
}

ChangesMap::ChangesMap() : NiTPointerMap<uint32_t, ChangeData*>(0x13AFu) {}

ChangesMap::~ChangesMap() {
    ChangesMap::RemoveAllChanges();
}

// Frees every recorded change, then empties the map.
void ChangesMap::RemoveAllChanges() {
    NiTMapItem<uint32_t, ChangeData*>* pkPos = GetFirstPos();
    while (pkPos) {
        uint32_t uiKey = 0;
        ChangeData* pkChange = nullptr;
        GetNext(pkPos, uiKey, pkChange);
        if (pkChange) {
            delete pkChange;
        }
    }
    RemoveAll_1();
}

ChangeData* ChangesMap::GetChanges(uint32_t auiFormID) {
    ChangeData* a2 = nullptr;
    GetAt(auiFormID, a2);
    return a2;
}

ChangeData* ChangesMap::GetChanges(TESForm* apForm) {
    uint32_t v2 = apForm->TESForm::GetFormID();
    return GetChanges(v2);
}

bool ChangesMap::RemoveChange(TESForm* apForm, uint32_t aiChange) {
    if (pSaveLoadGameOLD->GetLoading()) {
        return false;
    }
    ChangeData* pchange = GetChanges(apForm);
    if (!pchange) {
        return false;
    }
    pchange->RemoveChange(aiChange);
    if (!pchange->iFlags) {
        RemoveAt(apForm->iFormID);
        delete pchange;
    }
    return true;
}

bool ChangesMap::RemoveChanges(uint32_t aiFormID, bool abForceRemove) {
    ChangeData* Changes_0 = GetChanges(aiFormID);
    if (!Changes_0) {
        return false;
    }
    if (!Changes_0->pBuffer || abForceRemove) {
        RemoveAt(aiFormID);
        delete Changes_0;
    }
    return true;
}

[[clang::disable_tail_calls]] bool ChangesMap::RemoveChanges(TESForm* apForm, bool abForceRemove) {
    return RemoveChanges(apForm->GetFormID(), abForceRemove);
}

ChangeData* ChangesMap::AddChange(TESForm* apForm, uint32_t aiChange) {
    ChangeData* pchange;
    uint32_t iformID = apForm->iFormID;
    if (!GetAt(iformID, pchange)) {
        ChangeData* pchange_2;
        AutoMemContext tempMemContext(
            MEM_CONTEXT_SAVELOAD, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
            260
        );
        void* v6 = ourMalloc(sizeof(ChangeData));
        if (v6) {
            pchange_2 = ::new (v6) ChangeData;
        }
        else {
            pchange_2 = nullptr;
        }
        pchange = pchange_2;
        SetAt(iformID, pchange_2);
    }
    pchange->AddChange(aiChange);
    return pchange;
}

void ChangesMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

CreatedReferenceData::CreatedReferenceData() : eType(TES_CREATED_REFERENCE_NORMAL), iBoundID(0) {}

MovedReferenceData::MovedReferenceData() : iOriginalLocationID(0) {}

ReferenceData::ReferenceData() : iLocationID(0) {}

SaveDataDebugInfo::Stats::Stats() : iNum(0), iTotalSize(0), iMinSize(0xFFFF), iMaxSize(0) {}

SaveDataDebugInfo::SaveDataDebugInfo()
  : headerMap(new NiTPointerMap<uint8_t, BSSimpleList<LoadFormHeader*>*>(37)), unkList(new BSSimpleList<ExtraStat*>()) {
}

// The headers and the extra entries are raw ourMalloc memory the lists point to; the lists and the map are owned.
SaveDataDebugInfo::~SaveDataDebugInfo() {
    NiTMapItem<uint8_t, BSSimpleList<LoadFormHeader*>*>* pos = headerMap->GetFirstPos();
    while (pos) {
        uint8_t key;
        BSSimpleList<LoadFormHeader*>* pHeaderList;
        headerMap->GetNext(pos, key, pHeaderList);
        if (pHeaderList) {
            for (BSSimpleList<LoadFormHeader*>* pNode = pHeaderList; pNode; pNode = pNode->m_pkNext) {
                if (pNode->m_item) {
                    ourFree(pNode->m_item);
                }
            }
            delete pHeaderList;
        }
    }
    delete headerMap;
    if (unkList) {
        for (BSSimpleList<ExtraStat*>* pNode = unkList; pNode; pNode = pNode->m_pkNext) {
            ExtraStat* pStat = pNode->m_item;
            if (pStat) {
                ourFree(pStat->pDescription);
                ourFree(pStat);
            }
        }
        delete unkList;
    }
}

// The averages are the quotient of the two int32 sums, computed as the correctly rounded double.
void SaveDataDebugInfo::PrintStats(const char* apFileName) {
    char acFileName[260];
    BSstrcpy(acFileName, 260, apFileName);
    BSstrcat(acFileName, 260, ".txt");
    if (FileFinder::Locate(acFileName, nullptr, FileFinder::SKIP_NONE, FileFinder::ARCHIVE_TYPE_ANY)) {
        BSSystemFile::DeleteFileA(acFileName);
    }
    // Passes access mode 1 and open mode 2.
    BSSystemFile kFile;
    kFile.~BSSystemFile();
    ::new (&kFile) BSSystemFile(acFileName, 1, 2, false);
    if (kFile.QLastError()) {
        return;
    }

    Stats kTotalStats;
    kTotalStats.~Stats();
    ::new (&kTotalStats) Stats;
    char acBuffer[520];
    BSsprintf(
        acBuffer, 520, "  %-8s %-5s %-8s %02s %-40s  %s\r\n\r\n", "Form ID", "Size", "Flags", "V", "Name", "Changes"
    );
    WriteFile(&kFile, acBuffer);

    NiTMapItem<uint8_t, BSSimpleList<LoadFormHeader*>*>* pPos = headerMap->GetFirstPos();
    while (pPos) {
        uint8_t cFormType;
        BSSimpleList<LoadFormHeader*>* pHeaders;
        headerMap->GetNext(pPos, cFormType, pHeaders);
        BSSimpleList<LoadFormHeader*>* pNode = pHeaders;

        char acTypeName[10];
        if (!cFormType) {
            BSsprintf(acTypeName, 10, "Form");
        }
        else if (cFormType == FORM_ID_COUNT) {
            BSsprintf(acTypeName, 10, "Buffer");
        }
        else {
            BSsprintf(acTypeName, 10, "%s", formEnumString[cFormType].pFormString);
        }
        BSsprintf(acBuffer, 520, "%ss:\r\n\r\n", acTypeName);
        // Measures the line and drops the length, as the game does.
        BSstrlen(acBuffer);
        WriteFile(&kFile, acBuffer);

        Stats kTypeStats;
        kTypeStats.~Stats();
        ::new (&kTypeStats) Stats;
        for (; pNode; pNode = pNode->GetNext()) {
            LoadFormHeader* pHeader = pNode->GetItem();
            if (!pHeader) {
                continue;
            }
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pHeader->iSize > kTypeStats.iMaxSize) {
                kTypeStats.iMaxSize = pHeader->iSize;
            }
            if (pHeader->iSize < kTypeStats.iMinSize) {
                kTypeStats.iMinSize = pHeader->iSize;
            }
            kTypeStats.iTotalSize += pHeader->iSize;
            ++kTypeStats.iNum;

            char acChanges[504];
            pSaveLoadGameOLD->BuildChangesString(acChanges, pForm, pHeader->iFlags, pHeader->cFormType, false);
            const char* pName = nullptr;
            TESFullName* pFullName = dynamic_cast<TESFullName*>(pForm);
            TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
            if (pForm) {
                if (pRef) {
                    pName = pRef->GetFullName();
                }
                if (pFullName && (!pName || !BSstrcmp(pName, ""))) {
                    pName = pFullName->GetFullName();
                }
                if (!pName || !BSstrcmp(pName, "")) {
                    pName = pForm->GetFormEditorID();
                }
                BSsprintf(
                    acBuffer, 520, "  %08X %5i %08X %2i %-40s- %s\r\n", pHeader->iFormID, pHeader->iSize,
                    pHeader->iFlags, pHeader->cVersion, pName, acChanges
                );
            }
            else {
                BSsprintf(
                    acBuffer, 520, "  %08X %5i %08X %2i %-40s- %s\r\n", pHeader->iFormID, pHeader->iSize,
                    pHeader->iFlags, pHeader->cVersion, "NOT LOADED", acChanges
                );
            }
            WriteFile(&kFile, acBuffer);
        }

        BSsprintf(
            acBuffer, 520,
            "\r\nTotal %s Num: %i  Total %s Size: %i  Min %s Size: %i  Max %s Size: %i  Average %s Size: %.2f\r\n\r\n",
            acTypeName, kTypeStats.iNum, acTypeName, kTypeStats.iTotalSize, acTypeName, kTypeStats.iMinSize, acTypeName,
            kTypeStats.iMaxSize, acTypeName,
            static_cast<double>(kTypeStats.iTotalSize) / static_cast<double>(kTypeStats.iNum)
        );
        WriteFile(&kFile, acBuffer);
        if (kTypeStats.iMaxSize > kTotalStats.iMaxSize) {
            kTotalStats.iMaxSize = kTypeStats.iMaxSize;
        }
        if (kTypeStats.iMinSize < kTotalStats.iMinSize) {
            kTotalStats.iMinSize = kTypeStats.iMinSize;
        }
        kTotalStats.iTotalSize += kTypeStats.iTotalSize;
        kTotalStats.iNum += kTypeStats.iNum;
    }

    WriteFile(&kFile, "Extra Stats:\r\n\r\n");
    for (BSSimpleList<ExtraStat*>* pNode = unkList; pNode; pNode = pNode->GetNext()) {
        ExtraStat* pStat = pNode->GetItem();
        if (pStat) {
            BSsprintf(acBuffer, 520, "  %8i      %s\r\n", pStat->iSize, pStat->pDescription);
            WriteFile(&kFile, acBuffer);
            kTotalStats.iTotalSize += pStat->iSize;
        }
    }
    BSsprintf(
        acBuffer, 520, "\r\n\r\nTotal Num: %i  Total Size: %i  Min Size: %i  Max Size: %i  Average Size: %.2f\r\n",
        kTotalStats.iNum, kTotalStats.iTotalSize, kTotalStats.iMinSize, kTotalStats.iMaxSize,
        static_cast<double>(kTotalStats.iTotalSize) / static_cast<double>(kTotalStats.iNum)
    );
    WriteFile(&kFile, acBuffer);
}

// The length is BSstrlen's, zero-extended to the 64-bit count.
bool SaveDataDebugInfo::WriteFile(BSSystemFile* aFile, const char* apBuffer) {
    uint64_t uiWritten;
    return aFile->Write(apBuffer, BSstrlen(apBuffer), uiWritten) == BSSystemFile::EC_NONE;
}

void SaveDataDebugInfo::AddExtraStat(uint32_t aiSize, const char* apDescription) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp", 615
    );
    SaveDataDebugInfo::ExtraStat* pextraStat =
        static_cast<SaveDataDebugInfo::ExtraStat*>(ourMalloc(sizeof(SaveDataDebugInfo::ExtraStat)));
    pextraStat->iSize = aiSize;
    int32_t iDescLen = BSstrlen(apDescription) + 1;
    char* v3 = static_cast<char*>(ourMalloc(iDescLen));
    pextraStat->pDescription = v3;
    BSstrcpy(pextraStat->pDescription, iDescLen, apDescription);
    unkList->AddHead(pextraStat);
}

void SaveDataDebugInfo::AddStats(SaveFormHeader* apHeader, uint16_t aiSize) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp", 630
    );
    LoadFormHeader header;
    header.iFlags = apHeader->iFlags;
    header.iFormID = apHeader->iFormID;
    header.cFormType = apHeader->cFormType;
    header.cVersion = apHeader->cVersion;
    header.iSize = aiSize;
    AddStats(&header);
}

void SaveDataDebugInfo::AddStats(LoadFormHeader* apHeader) {
    LoadFormHeader* pheader;
    BSSimpleList<LoadFormHeader*>* pheaderList_2;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp", 646
    );
    void* ecx0 = ourMalloc(sizeof(LoadFormHeader));
    if (ecx0) {
        pheader = static_cast<LoadFormHeader*>(ecx0);
    }
    else {
        pheader = nullptr;
    }
    *pheader = *apHeader;
    uint8_t ckey = pheader->cFormType;
    if (!headerMap->GetAt(ckey, pheaderList_2)) {
        BSSimpleList<LoadFormHeader*>* pheaderList;
        BSSimpleList<LoadFormHeader*>* v5 =
            static_cast<BSSimpleList<LoadFormHeader*>*>(ourMalloc(sizeof(BSSimpleList<LoadFormHeader*>)));
        if (v5) {
            pheaderList = ::new (v5) BSSimpleList<LoadFormHeader*>;
        }
        else {
            pheaderList = nullptr;
        }
        pheaderList_2 = pheaderList;
        headerMap->SetAt(ckey, pheaderList);
    }
    pheaderList_2->Insert(pheader, &SaveDataDebugInfo::LoadFormHeaderCompare);
}

void SaveDataDebugInfo::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Sorts the larger data size first; the sizes are compared as signed ints after zero extension.
int32_t SaveDataDebugInfo::LoadFormHeaderCompare(LoadFormHeader* apCompare1, LoadFormHeader* apCompare2) {
    if (apCompare1->iSize <= static_cast<int32_t>(apCompare2->iSize)) {
        return apCompare1->iSize < static_cast<int32_t>(apCompare2->iSize);
    }

    return -1;
}

uint32_t NiPixelData::GetSizeInBytes(uint32_t uiMipmapLevel, uint32_t uiFace) const {
    return m_puiOffsetInBytes[uiMipmapLevel + 1] - m_puiOffsetInBytes[uiMipmapLevel];
}
