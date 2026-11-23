#include "Fallout Shared/TESSaveLoadGame.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkNiCollisionObject.h"
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

// The buffers are plain blocks, freed without a destructor.
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

// The reference data are plain blocks, freed without a destructor.
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
    return bHasFileTime;
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
    char acEntry[52];
    BSstrcpy(apString, 500, "");
    if (apForm) {
        aiFlags = CheckFlags(apForm, aiFlags);
    }
    uint16_t iBaseSaveSize = 0;
    if (!abVerbose && apForm) {
        iBaseSaveSize = apForm->GetSaveSize(0);
    }

    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(apForm);
    MobileObject* pMob = dynamic_cast<MobileObject*>(apForm);
    if (pMob) {
        PROCESS_TYPE eType = PROCESS_TYPE_NONE;
        if (pMob->pCurrentProcess) {
            eType = pMob->pCurrentProcess->eLevel;
        }
        switch (eType) {
            case PROCESS_TYPE_NONE: {
                if (abVerbose) {
                    BSsprintf(acEntry, 50, "Process Level: None\r\n");
                }
                else {
                    BSsprintf(acEntry, 50, "None(%i)", iBaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_HIGH: {
                if (abVerbose) {
                    BSsprintf(acEntry, 50, "Process Level: High\r\n");
                }
                else {
                    BSsprintf(acEntry, 50, "High(%i)", iBaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_MIDDLE_HIGH: {
                if (abVerbose) {
                    BSsprintf(acEntry, 50, "Process Level: Middle High\r\n");
                }
                else {
                    BSsprintf(acEntry, 50, "Mid High(%i)", iBaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_MIDDLE_LOW: {
                if (abVerbose) {
                    BSsprintf(acEntry, 50, "Process Level: Middle Low\r\n");
                }
                else {
                    BSsprintf(acEntry, 50, "Mid Low(%i)", iBaseSaveSize);
                }
                break;
            }
            case PROCESS_TYPE_LOW: {
                if (abVerbose) {
                    BSsprintf(acEntry, 50, "Process Level: Low\r\n");
                }
                else {
                    BSsprintf(acEntry, 50, "Low(%i)", iBaseSaveSize);
                }
                break;
            }
            default: {
                break;
            }
        }
        if (!abVerbose) {
            while (BSstrlen(acEntry) < 25) {
                BSstrcat(acEntry, 50, " ");
            }
            BSstrcat(acEntry, 50, " ");
        }
        BSstrcat(apString, 500, acEntry);
    }
    else if (iBaseSaveSize && !abVerbose) {
        BSsprintf(acEntry, 50, "Base(%i)", iBaseSaveSize);
        while (BSstrlen(acEntry) < 25) {
            BSstrcat(acEntry, 50, " ");
        }
        BSstrcat(acEntry, 50, " ");
        BSstrcat(apString, 500, acEntry);
    }
    if ((aiFlags & CHANGE_FORM_FLAGS) != 0) {
        if (abVerbose) {
            BSstrcat(apString, 500, "CHANGE_FORM_FLAGS\r\n");
        }
        else {
            int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_FORM_FLAGS) - iBaseSaveSize : 0;
            BSsprintf(acEntry, 50, "Form Flags(%i)", iChangeSize);
            while (BSstrlen(acEntry) < 25) {
                BSstrcat(acEntry, 50, " ");
            }
            BSstrcat(acEntry, 50, " ");
            BSstrcat(apString, 500, acEntry);
        }
    }
    switch (acFormType) {
        case 42:
        case 43: {
            if ((aiFlags & CHANGE_ACTOR_BASE_DATA) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_ACTOR_BASE_DATA\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_ACTOR_BASE_DATA) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Base Data(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_ACTOR_BASE_SPELLLIST) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_ACTOR_BASE_SPELLLIST\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_ACTOR_BASE_SPELLLIST) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Spell List(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_ACTOR_BASE_FULLNAME) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_ACTOR_BASE_FULLNAME\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_ACTOR_BASE_FULLNAME) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Full Name(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if (acFormType == 42) {
                if ((aiFlags & CHANGE_NPC_SKILLS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_NPC_SKILLS\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_NPC_SKILLS) - iBaseSaveSize : 0;
                        BSsprintf(acEntry, 50, "Skills(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
            }
            else if (acFormType == 43 && (aiFlags & CHANGE_CREATURE_SKILLS) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CREATURE_SKILLS\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_CREATURE_SKILLS) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Skills(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            break;
        }
        case 71: {
            if ((aiFlags & CHANGE_QUEST_FLAGS) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_QUEST_FLAGS\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_QUEST_FLAGS) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Quest Flags(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_QUEST_STAGES\r\n");
                }
                else {
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_QUEST_SCRIPT\r\n");
                }
                else {
                    TESQuest* pQuest = dynamic_cast<TESQuest*>(apForm);
                    if (pQuest && !pQuest->GetScriptLocals()) {
                        if (apForm) {
                            BSsprintf(acEntry, 50, "Quest Script(No longer has script locals)");
                        }
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_QUEST_SCRIPT) - iBaseSaveSize : 0;
                        BSsprintf(acEntry, 50, "Quest Script(%i)", iChangeSize);
                    }
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
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
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
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
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_PACKAGE_NEVER_RUN) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Never Run Flag(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_PACKAGE_WAITING) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_PACKAGE_WAITING\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_PACKAGE_WAITING) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Waiting Flag(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            break;
        }
        case 57: {
            if ((aiFlags & CHANGE_CELL_FLAGS) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_FLAGS\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_CELL_FLAGS) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Cell Flags(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_CELL_SEENDATA) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_SEENDATA\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_CELL_SEENDATA) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Seen Data(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_CELL_DETACHTIME) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_DETACHTIME\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_CELL_DETACHTIME) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Detach Time(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_CELL_OWNERSHIP) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_OWNERSHIP\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_CELL_OWNERSHIP) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Ownership(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_CELL_FULLNAME) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_CELL_FULLNAME\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_CELL_FULLNAME) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Full Name(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            break;
        }
        case 8: {
            if ((aiFlags & CHANGE_FACTION_FLAGS) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_FACTION_FLAGS\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_FACTION_FLAGS) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Faction Flags(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_FACTION_REACTIONS) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_FACTION_REACTIONS\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_FACTION_REACTIONS) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Faction Reactions(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            break;
        }
        case 25: {
            if ((aiFlags & CHANGE_BOOK_TEACHES_SKILL) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_BOOK_TEACHES_SKILL\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_BOOK_TEACHES_SKILL) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Book Skill(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
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
            if ((aiFlags & CHANGE_REFR_MOVE) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_MOVE\r\n");
                }
                else {
                    int32_t iChangeSize;
                    if (apForm) {
                        iChangeSize = static_cast<uint16_t>(GetInitialDataSaveSize(apForm, CHANGE_REFR_MOVE));
                    }
                    else {
                        iChangeSize = 0;
                    }
                    BSsprintf(acEntry, 50, "Moved(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_REFR_HAVOK_MOVE) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_HAVOK_MOVE\r\n");
                }
                else {
                    if ((aiFlags & CHANGE_REFR_MOVE) != 0) {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_REFR_HAVOK_MOVE) - iBaseSaveSize : 0;
                        BSsprintf(acEntry, 50, "Havok Moved(%i)", iChangeSize);
                    }
                    else {
                        int32_t iChangeSize;
                        if (apForm) {
                            int32_t iInitialDataSize = static_cast<uint16_t>(
                                GetInitialDataSaveSize(apForm, CHANGE_REFR_HAVOK_MOVE)
                            );
                            iChangeSize = apForm->GetSaveSize(CHANGE_REFR_HAVOK_MOVE) + iInitialDataSize -
                                iBaseSaveSize;
                        }
                        else {
                            iChangeSize = 0;
                        }
                        BSsprintf(acEntry, 50, "Havok Moved(%i)", iChangeSize);
                    }
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_ANIMATION\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_REFR_ANIMATION) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Animation(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_REFR_SCALE) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_SCALE\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_REFR_SCALE) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Scale(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if ((aiFlags & CHANGE_REFR_INVENTORY) != 0) {
                if (abVerbose) {
                    BSstrcat(apString, 500, "CHANGE_REFR_INVENTORY\r\n");
                }
                else {
                    int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_REFR_INVENTORY) - iBaseSaveSize : 0;
                    BSsprintf(acEntry, 50, "Inventory(%i)", iChangeSize);
                    while (BSstrlen(acEntry) < 25) {
                        BSstrcat(acEntry, 50, " ");
                    }
                    BSstrcat(acEntry, 50, " ");
                    BSstrcat(apString, 500, acEntry);
                }
            }
            if (acFormType == 59 || acFormType == 60) {
                if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_DAMAGE_MODIFIERS\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm
                            ? apForm->GetSaveSize(CHANGE_ACTOR_DAMAGE_MODIFIERS) - iBaseSaveSize
                            : 0;
                        BSsprintf(acEntry, 50, "Damage Modifiers(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_PERMANENT_MODIFIERS\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm
                            ? apForm->GetSaveSize(CHANGE_ACTOR_PERMANENT_MODIFIERS) - iBaseSaveSize
                            : 0;
                        BSsprintf(acEntry, 50, "Script Modifiers(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_TEMP_MODIFIERS\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_ACTOR_TEMP_MODIFIERS) - iBaseSaveSize
                                                     : 0;
                        BSsprintf(acEntry, 50, "Temp Modifiers(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_DISPOSITION_MODIFIERS) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_DISPOSITION_MODIFIERS\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm
                            ? apForm->GetSaveSize(CHANGE_ACTOR_DISPOSITION_MODIFIERS) - iBaseSaveSize
                            : 0;
                        BSsprintf(acEntry, 50, "Disp Modifiers(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_ACTOR_LIFESTATE) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_ACTOR_LIFESTATE\r\n");
                    }
                    else {
                        int32_t iSizeWithoutLifeState = apForm ? apForm->GetSaveSize(aiFlags & CHANGE_REFR_HAVOK_MOVE)
                                                               : 0;
                        int32_t iChangeSize;
                        if (apForm) {
                            iChangeSize = apForm->GetSaveSize(
                                              (aiFlags & CHANGE_REFR_HAVOK_MOVE) + CHANGE_ACTOR_LIFESTATE
                                          ) -
                                iSizeWithoutLifeState;
                        }
                        else {
                            iChangeSize = 0;
                        }
                        BSsprintf(acEntry, 50, "Life State(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
            }
            else {
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OBJECT_EXTRA_LOCK\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_OBJECT_EXTRA_LOCK) - iBaseSaveSize
                                                     : 0;
                        BSsprintf(acEntry, 50, "Lock(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_OBJECT_EMPTY) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OBJECT_EMPTY\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_OBJECT_EMPTY) - iBaseSaveSize : 0;
                        BSsprintf(acEntry, 50, "Empty Flag(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((!pRef || (pRef->GetObjectReference() && pRef->GetObjectReference()->GetFormType() == DOOR_ID)) &&
                    (aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_DOOR_EXTRA_TELEPORT\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_DOOR_EXTRA_TELEPORT) - iBaseSaveSize
                                                     : 0;
                        BSsprintf(acEntry, 50, "Teleport(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_OBJECT_OPEN_STATE) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OPEN_STATE\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm ? apForm->GetSaveSize(CHANGE_OBJECT_OPEN_STATE) - iBaseSaveSize
                                                     : 0;
                        BSsprintf(acEntry, 50, "Open State(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
                if ((aiFlags & CHANGE_OBJECT_OPEN_DEFAULT_STATE) != 0) {
                    if (abVerbose) {
                        BSstrcat(apString, 500, "CHANGE_OPEN_DEFAULT_STATE\r\n");
                    }
                    else {
                        int32_t iChangeSize = apForm
                            ? apForm->GetSaveSize(CHANGE_OBJECT_OPEN_DEFAULT_STATE) - iBaseSaveSize
                            : 0;
                        BSsprintf(acEntry, 50, "Default Open State(%i)", iChangeSize);
                        while (BSstrlen(acEntry) < 25) {
                            BSstrcat(acEntry, 50, " ");
                        }
                        BSstrcat(acEntry, 50, " ");
                        BSstrcat(apString, 500, acEntry);
                    }
                }
            }
            break;
        }
    }
}

BSFile*
TESSaveLoadGame::PrepForLoadSave(BSFile* apFile, const char* asFileName, TESSaveLoadGame::ACTION_TYPE aeAction) {
    char acPath[264];
    BSFile* pNewFile;
    if (apFile) {
        BSstrcpy(acPath, 260, apFile->GetDevice());
    }
    else {
        char acName[264];
        if (asFileName) {
            BSstrcpy(acName, 260, asFileName);
        }
        else {
            GenerateSaveGameFileName(acName);
        }
        const char* pSavePath = sLocalSavePath.String();
        BSsprintf(acPath, 260, "%s%s%s.ess", BSUtilities::GetPersonalPath(), pSavePath, acName);
        if (aeAction == TESSaveLoadGame::ACTION_SAVE) {
            char acSaveDirectory[268];
            BSstrcpy(acSaveDirectory, 260, BSUtilities::GetPersonalPath());
            BSstrcat(acSaveDirectory, 260, sLocalSavePath.String());
            std::error_code kError;
            std::filesystem::create_directory(platform::NativePath(acSaveDirectory), kError);
            // The backup count is a constant 1, clamped to 10.
            int32_t iBackupCount = 1;
            if (iBackupCount > 10) {
                iBackupCount = 10;
            }
            for (int32_t i = iBackupCount - 1; i >= 0; --i) {
                char acFileName[264];
                char acOldFilename[268];
                BSstrcpy(acOldFilename, 260, BSUtilities::GetPersonalPath());
                BSstrcat(acOldFilename, 260, sLocalSavePath.String());
                BSstrcat(acOldFilename, 260, acName);
                for (int32_t j = 0; j < i; ++j) {
                    BSstrcat(acOldFilename, 260, ".bak");
                }
                BSstrcpy(acFileName, 260, acOldFilename);
                BSstrcat(acFileName, 260, ".bak");
                if (!i) {
                    BSstrcpy(acOldFilename, 260, acPath);
                }
                if (FileFinder::Locate(acOldFilename, nullptr, 0, 0xFFFFFFFFu)) {
                    if (FileFinder::Locate(acFileName, nullptr, 0, 0xFFFFFFFFu)) {
                        std::filesystem::remove(platform::NativePath(acFileName), kError);
                    }
                    rt::rename(acOldFilename, acFileName);
                }
            }
        }
    }
    switch (aeAction) {
        case TESSaveLoadGame::ACTION_SAVE: {
            if (apFile) {
                char acName[264];
                char* pFileName = std::strrchr(apFile->GetDevice(), '\\') + 1;
                BSstrcpy(acName, 260, pFileName);
                uint32_t uiLength = BSstrlen(acName);
                if (uiLength > 4 && !_strnicmp(&acName[uiLength - 4], ".ess", 4u)) {
                    acName[uiLength - 4] = 0;
                }
                DeleteSaveGame(apFile, nullptr);
                if (!_strnicmp(acName, "Save ", 5u) || std::strstr(acName, "autosave")) {
                    pNewFile = PrepForLoadSave(nullptr, nullptr, TESSaveLoadGame::ACTION_SAVE);
                }
                else {
                    pNewFile = PrepForLoadSave(nullptr, acName, TESSaveLoadGame::ACTION_SAVE);
                }
            }
            else {
                pNewFile = new BSFile(acPath, NiFile::WRITE_ONLY, 0x20000u, false);
            }
            break;
        }
        case TESSaveLoadGame::ACTION_LOAD: {
            pNewFile = new BSFile(acPath, NiFile::READ_ONLY, 0x20000u, false);
            pNewFile->Open(false, false);
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
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        4154
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
    TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(apForm);
    if (apForm->GetTemporary() || pCell) {
        apForm->SetFormID(pDataHandler->GetNextID(), true);
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
    pSaveLoadGameOLD->LoadGameDataOLD(&kBufferHeader, sizeof(kBufferHeader));
    if (apForm->GetFormType() != kBufferHeader.cFormType) {
        char acMessage[260];
        const char* pFormTypeString = apForm->GetFormTypeString();
        const char* pSavedTypeString = formEnumString[kBufferHeader.cFormType].pFormString;
        uint32_t uiFormID = apForm->GetFormID();
        std::sprintf(
            acMessage,
            "Load Error: Form with ID %08X was saved with form type %s, but currently has form type %s.  Its loading "
            "will be skipped.",
            uiFormID,
            pSavedTypeString,
            pFormTypeString
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
        m_pInitArray = ::new NiTLargePrimitiveArray<FormAndFlags*>(50, 50);
    }
    FormAndFlags* pItem = new FormAndFlags(apForm, uiFlags, 0, kBufferHeader.cVersion);
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
    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(apForm);
    if (pRef) {
        if ((aiFlags & (CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) != 0) {
            ReferenceData kRefData;
            pSaveLoadGameOLD->LoadGameDataOLD(&kRefData, sizeof(kRefData));
            kRefData.iLocationID = GetNumericIDFromArray(kRefData.iLocationID);
            pRef->SetAngleOnReference(kRefData.Angle);
            if (pRef != pPlayer) {
                pRef->SetLocationOnReference(kRefData.Loc);
                SafetyCheckReferencePositionAndAngle(pRef);
                TESForm* pLocation = TESForm::GetFormByNumericID(kRefData.iLocationID);
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pLocation);
                TESWorldSpace* pWorldSpace = dynamic_cast<TESWorldSpace*>(pLocation);
                if (pCell) {
                    if (pCell != pRef->GetParentCell()) {
                        TESObjectREFR::MoveRefToNewSpace(pRef, pCell, nullptr);
                    }
                }
                else if (pWorldSpace) {
                    int32_t iCellX = FloatToInt(kRefData.Loc.x) >> 12;
                    int32_t iCellY = FloatToInt(kRefData.Loc.y) >> 12;
                    TESObjectCELL* pCoordCell = pWorldSpace->GetCellFromCellCoord(iCellX, iCellY);
                    if ((pRef->GetParentCell() && pRef->GetParentCell()->GetInterior()) ||
                        pRef->GetParentCell() != pCoordCell) {
                        TESObjectREFR::MoveRefToNewSpace(pRef, nullptr, pWorldSpace);
                    }
                    else if (!pCoordCell && !pRef->GetRefPersists()) {
                        Warning("SAVELOAD: Trying to load non-persistent ref into non-existent cell.");
                    }
                }
                else if (pRef->GetRefPersists()) {
                    if (pRef->GetParentCell()) {
                        pRef->GetParentCell()->RemoveReference(pRef);
                    }
                    pRef->SetParentCell(nullptr);
                    pModelLoader->CancelReference(pRef);
                    pRef->Set3D(nullptr, true);
                }
                else {
                    Warning("SAVELOAD: Trying to put non-persistent reference in non-existent cell.");
                }
            }
        }
    }
    else if (dynamic_cast<TESObjectCELL*>(apForm) && pSaveLoadGameOLD->GetCurrentVersion() < 91u) {
        AdvanceBuffer(2u);
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
            m_pChanges->AddChange(pActor, CHANGE_REFR_MOVE);
            if (!pActor->GetParentCell() && !pActor->GetWorldSpace()) {
                uint32_t uiCellID = 0;
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(uiCellID));
                if (pCell) {
                    NiPoint3 kPos = NiPoint3::ZERO;
                    NiPoint3 kRot = NiPoint3::ZERO;
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
        TESTerrainLODManager::UpdateGrass(kPlayerPos, NiPoint3::UNIT_Y, 1.0f);
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
    return m_pInitArray != nullptr;
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
        m_pInitItemArray = ::new NiTLargePrimitiveArray<TESForm*>(20000, 2000);
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
    uint32_t uiCount = aiSize >> 2;
    for (uint32_t i = 0; i < uiCount; ++i) {
        uint32_t uiNumericID = GetUseNumericIDArray() ? AddNumericIDToArray(apFormID[i]) : apFormID[i];
        ourMemcpy(m_pBuffer, &uiNumericID, 4u);
        AdvanceBuffer(4);
    }
}

bool TESSaveLoadGame::LoadNumericID(uint32_t* apFormID, uint32_t aiSize) {
    uint8_t cMissingID = 0;
    ourMemcpy(apFormID, m_pBuffer, aiSize);
    if (GetUseNumericIDArray()) {
        for (uint32_t i = 0; i < aiSize >> 2; ++i) {
            uint32_t uiNumericID = GetNumericIDFromArray(apFormID[i]);
            if (apFormID[i] && !uiNumericID) {
                cMissingID = 1;
            }

            apFormID[i] = uiNumericID;
        }
    }
    AdvanceBuffer(aiSize);
    return cMissingID;
}

TESPackage* TESSaveLoadGame::CreatePackage(uint32_t aiFormID, uint8_t cPackType) {
    ClearFormID(aiFormID);
    TESPackage* pPackage = TESPackage::CreatePackage(static_cast<PACKAGE_TYPE>(cPackType));
    pPackage->SetFormID(aiFormID, true);
    return pPackage;
}

bool TESSaveLoadGame::UseSaveGameBlocks() {
    bool bUse = pSaveLoadGameOLD->GetCurrentVersion() >= 31 && pSaveLoadGameOLD->GetCurrentVersion() < 90;
    return bUse;
}

void TESSaveLoadGame::LoadGameDataOLD(void* apData, uint32_t aiSize) {
    ourMemcpy(apData, m_pBuffer, aiSize);
    AdvanceBuffer(aiSize);
}

void TESSaveLoadGame::TestAllCells(uint32_t aiControl) {
    static int32_t iCount;
    NiPoint3 kPos;
    if (aiControl == 4 || aiControl == 5) {
        if (aiControl == 5) {
            uint32_t uiBufferSize = TES::pInstance->GetExteriorBufferSize();
            if (TES::pInstance->pInteriorCell) {
                uiBufferSize = 1;
            }
            for (int32_t i = 0; i < uiBufferSize; ++i) {
                TESObjectCELL* pCell = TES::pInstance->pInteriorCell;
                if (!pCell) {
                    pCell = TES::pInstance->GetExteriorBufferCellByIndex(i);
                }
                if (pCell) {
                    pCell->SaveGameTest();
                }
            }
            TESBoundObject* pObject = pLockpick;
            for (uint32_t j = 0; j < 5; ++j) {
                TESWorldSpace* pWorldSpace = pPlayer->GetWorldSpace();
                TESObjectCELL* pParentCell = pPlayer->GetParentCell();
                const NiPoint3& kAngle = pPlayer->GetAngleOnReference();
                const NiPoint3& kLocation = pPlayer->GetLocationOnReference();
                TESObjectREFR* pNewRef = pDataHandler->CreateReferenceAtLocation(
                    pObject,
                    kLocation,
                    kAngle,
                    pParentCell,
                    pWorldSpace,
                    nullptr,
                    nullptr,
                    nullptr
                );
                const NiPoint3& kNewLocation = pNewRef->GetLocationOnReference();
                kPos.x = kNewLocation.x;
                kPos.y = kNewLocation.y;
                kPos.z = kNewLocation.z + 1000.0;
                pNewRef->SetLocationOnReference(kPos);
            }
        }
        if (!(iCount % 50)) {
            char acFileName[264];
            BSsprintf(acFileName, 260u, "Test All Cells %i.ess", iCount);
            SaveGame(nullptr, acFileName, true);
        }
        ++iCount;
    }
}

SaveFormHeader* TESSaveLoadGame::GetCurrentlySavingFormHeader() {
    return m_pCurrentlySavingFormHeader;
}

bool TESSaveLoadGame::IsUnk048Zero() {
    return m_iRevertState == 0;
}

void TESSaveLoadGame::SaveGameDataOLD(const void* apData, uint32_t aiSize) {
    ourMemcpy(m_pBuffer, apData, aiSize);
    AdvanceBuffer(aiSize);
}

// Tests the three coordinates for finiteness first, then for NaN.
bool TESSaveLoadGame::IsInvalidPoint(NiPoint3* apPoint) {
    return !std::isfinite(apPoint->x) ||
        !std::isfinite(apPoint->y) ||
        !std::isfinite(apPoint->z) ||
        std::isnan(apPoint->x) ||
        std::isnan(apPoint->y) ||
        std::isnan(apPoint->z);
}

// An actor with an editor location goes back to it; any other reference to its starting position when it has one, else
// to the origin. The location and angle are copied first.
void TESSaveLoadGame::SafetyCheckReferencePositionAndAngle(TESObjectREFR* apRef) {
    if (!apRef) {
        return;
    }
    NiPoint3 kLocation;
    NiPoint3 kAngle;
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
            apRef->SetLocationOnReference(NiPoint3::ZERO);
        }
    }
    if (IsInvalidPoint(&kAngle)) {
        Warning("SAVELOAD: Corrupt angle found loading reference %08X, setting to (0, 0, 0).", apRef->GetFormID());
        apRef->SetAngleOnReference(NiPoint3::ZERO);
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
    if (pPlayer->Actor::GetLifeState() == ACTOR_LIFE_STATE_DEAD ||
        pPlayer->Actor::GetLifeState() == ACTOR_LIFE_STATE_DYING) {
        return false;
    }
    if (pPlayer->GetUnderAIControl()) {
        return false;
    }
    if (StartMenu::PauseMenuIsActive()) {
        return true;
    }
    int32_t iTopMenuID = Interface::GetTopMenuID();
    if (iTopMenuID && iTopMenuID != 3) {
        return false;
    }
    int32_t iMenuModeType = Interface::GetMenuModeType();
    return !iMenuModeType || iMenuModeType == 3;
}

uint32_t TESSaveLoadGame::GetNumericIDFromArray(uint32_t aiFormIDIndex) {
    if (pDataHandler->IsFormIDCreated(aiFormIDIndex)) {
        return aiFormIDIndex;
    }
    if (aiFormIDIndex <= m_pNumericIDArray->NiTLargeArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize()) {
        return m_pNumericIDArray->NiTLargeArray<uint32_t, NiTMallocInterface<uint32_t>>::GetAt(aiFormIDIndex);
    }
    return 0;
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
    uint32_t uiFormID = apRef->TESForm::GetFormID();
    QueueBufferLoad(m_pAttachedAnimationMap, uiFormID, aiSize);
}

void TESSaveLoadGame::QueueCharControllerLoad(TESObjectREFR* apRef, uint16_t aiSize) {
    uint32_t uiFormID = apRef->TESForm::GetFormID();
    QueueBufferLoad(m_pCharControllerMap, uiFormID, aiSize);
}

void TESSaveLoadGame::QueueHavokDataLoad(TESObjectREFR* apRef, uint16_t aiSize) {
    uint32_t uiFormID = apRef->TESForm::GetFormID();
    QueueBufferLoad(m_pHavokDataMap, uiFormID, aiSize);
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
            acVersion,
            19
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
    uint16_t uiSize = aiSize;
    void* pBuffer = ourMalloc(aiSize + 2);
    ourMemcpy(pBuffer, &uiSize, 2u);
    return pBuffer;
}

bool TESSaveLoadGame::GetUseNumericIDArray() {
    return m_bUseNumericIDArray;
}

TESForm* TESSaveLoadGame::CreateReferenceFromFile(uint32_t aiFormID, MovedReferenceData* apMovedData) {
    TESObjectREFR* pRef = nullptr;
    uint32_t uiSearchID = apMovedData->iOriginalLocationID;
    if (!uiSearchID) {
        uiSearchID = apMovedData->RefData.iLocationID;
    }
    TESForm* pLocation = TESForm::GetFormByNumericID(uiSearchID);
    TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pLocation);
    TESWorldSpace* pWorldSpace = dynamic_cast<TESWorldSpace*>(pLocation);
    int32_t iCellX = 0;
    int32_t iCellY = 0;
    if (pCell) {
        int32_t iFileCount = pCell->GetFileCount();
        for (int32_t i = 0; i < iFileCount; ++i) {
            TESFile* pFile = pCell->GetFile(i)->GetThreadSafeFile();
            if (pFile->FindForm(pCell) && TESObjectCELL::FindReferenceInFile(pFile, aiFormID)) {
                uint32_t uiFormType = pFile->GetTESForm();
                pRef = TESObjectREFR::CreateReference(uiFormType, true);
                TESDataHandler::LoadForm(pRef, pFile);
            }
        }
    }
    else if (pWorldSpace) {
        int32_t iFileCount = pWorldSpace->GetFileCount();
        iCellX = FloatToInt(apMovedData->OriginalLoc.x) >> 12;
        iCellY = FloatToInt(apMovedData->OriginalLoc.y) >> 12;
        for (int32_t i = 0; i < iFileCount; ++i) {
            TESFile* pFile = pWorldSpace->GetFile(i)->GetThreadSafeFile();
            if (pWorldSpace->FindCellInFile(pFile, iCellX, iCellY) &&
                TESObjectCELL::FindReferenceInFile(pFile, aiFormID)) {
                uint32_t uiFormType = pFile->GetTESForm();
                pRef = TESObjectREFR::CreateReference(uiFormType, true);
                TESDataHandler::LoadForm(pRef, pFile);
            }
        }
    }
    else {
        Warning("CELLS: Reference to be loaded does not have a cell or worldspace.");
    }
    if (pRef) {
        pRef->InitItem();
        Actor* pActor = dynamic_cast<Actor*>(pRef);
        if (pActor) {
            float fAngleZ = pRef->GetAngleOnReference().z;
            const NiPoint3& kLocation = pRef->GetLocationOnReference();
            pActor->SetEditorLocation(pWorldSpace, pCell, kLocation, fAngleZ);
        }
        else {
            NiPoint3 kLocation = pRef->GetLocationOnReference();
            ExtraDataList* pExtra = pRef->TESObjectREFR::GetExtra();
            pExtra->SetStartingLocation(pRef, kLocation);
            NiPoint3 kAngle = pRef->GetAngleOnReference();
            pExtra = pRef->TESObjectREFR::GetExtra();
            pExtra->SetStartingAngle(pRef, kAngle);
        }
        return pRef;
    }

    Warning(
        "CELLS: Reference %08X could not be loaded into location %08X at coordinates %i, %i",
        aiFormID,
        uiSearchID,
        iCellX,
        iCellY
    );
    return nullptr;
}

uint32_t TESSaveLoadGame::AddNumericIDToArray(uint32_t aiFormID) {
    if (pDataHandler->IsFormIDCreated(aiFormID)) {
        return aiFormID;
    }
    uint32_t uiSize = m_pNumericIDArray->m_uiSize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        if (m_pNumericIDArray->GetAt(i) == aiFormID) {
            return i;
        }
    }

    return m_pNumericIDArray->Add(aiFormID);
}

bool TESSaveLoadGame::SaveGame(BSFile* pFile, const char* apFileName, bool abLogStats) {
    uint32_t uiZero;
    uint32_t uiFormCount;
    uint32_t uiNumericIDArrayOffset;
    AutoMemContext kTempMemContext(
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        1126
    );
    if (GetLoading() || (GetSavingAllowed() || (apFileName && !BSstrcmp(apFileName, "autosave")))) {
        pIOManager->Pause();
        CleanUpExpiredData();
        BSFile* pNewFile = nullptr;
        if (!GetLoading()) {
            pNewFile = PrepForLoadSave(pFile, apFileName, TESSaveLoadGame::ACTION_SAVE);
        }
        if (abLogStats) {
            m_pSaveLoadStats = new SaveDataDebugInfo;
        }
        ResetCurrentVersion();
        ResetVersions();
        SaveHeader(pNewFile, apFileName);
        SavePluginList(pNewFile);
        uint32_t uiSeekBackOffset = 0;
        if (!GetLoading()) {
            uiSeekBackOffset = pNewFile->Tell();
        }
        uiZero = 0;
        WriteFile(pNewFile, &uiZero, 4u);
        WriteFile(pNewFile, &uiZero, 4u);
        SaveGlobalData(pNewFile);
        uiFormCount = 0;
        NiTMapItem<uint32_t, ChangeData*>* pos = m_pChanges->GetFirstPos();
        while (pos) {
            ChangeData* pChange = nullptr;
            uint32_t uiFormID = 0;
            m_pChanges->GetNext(pos, uiFormID, pChange);
            if (uiFormID && pChange) {
                m_pBuffer = pChange->pBuffer;
                SaveFormHeader kHeader;
                kHeader.iFormID = uiFormID;
                kHeader.iFlags = pChange->iFlags;
                kHeader.cVersion = GetCurrentVersion();
                if (m_pBuffer) {
                    SaveBufferHeader kBufferHeader;
                    pSaveLoadGameOLD->LoadGameDataOLD(&kBufferHeader, sizeof(kBufferHeader));
                    kHeader.cFormType = kBufferHeader.cFormType;
                    kHeader.cVersion = kBufferHeader.cVersion;
                    pSaveLoadGameOLD->WriteFile(pNewFile, &kHeader, 10u);
                    ++uiFormCount;
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
                    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
                    if (pForm) {
                        kHeader.cFormType = pForm->GetFormType();
                        kHeader.iFlags = CheckFlags(pForm, kHeader.iFlags);
                        pSaveLoadGameOLD->WriteFile(pNewFile, &kHeader, 10u);
                        ++uiFormCount;
                        m_pCurrentlySavingFormHeader = &kHeader;
                        uint16_t iSize = pForm->GetSaveSize(kHeader.iFlags);
                        iSize += static_cast<uint16_t>(GetInitialDataSaveSize(pForm, kHeader.iFlags));
                        WriteFile(pNewFile, &iSize, 2u);
                        if (iSize) {
                            void* pTempBuffer = CreateBuffer(iSize);
                            SaveInitialData(pForm, kHeader.iFlags);
                            pForm->SaveGame(kHeader.iFlags);
                            SaveBuffer(pNewFile, pTempBuffer, iSize);
                            DestroyBuffer(pTempBuffer);
                        }
                        m_pCurrentlySavingFormHeader = nullptr;
                        if (m_pSaveLoadStats) {
                            m_pSaveLoadStats->AddStats(&kHeader, iSize);
                        }
                    }
                }
            }
        }
        SaveFinalData(pNewFile);
        uiNumericIDArrayOffset = 0;
        if (!GetLoading()) {
            uiNumericIDArrayOffset = pNewFile->Tell();
        }
        SaveNumericIDArrays(pNewFile);
        if (!GetLoading()) {
            pNewFile->Seek(static_cast<int32_t>(uiSeekBackOffset), NiFile::ms_iSeekSet);
            WriteFile(pNewFile, &uiNumericIDArrayOffset, 4u);
            WriteFile(pNewFile, &uiFormCount, 4u);
        }
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->PrintStats(pNewFile->GetDevice());
            SaveDataDebugInfo* pStats = m_pSaveLoadStats;
            if (pStats) {
                delete pStats;
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

    const char* pMessage = static_cast<const char*>(sCantSaveNow);
    Interface::ShowMessage(
        pMessage,
        0,
        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
        nullptr,
        2.0f,
        false
    );

    return false;
}

bool TESSaveLoadGame::ResetReferencePosition(TESObjectREFR* apRef, bool abTemporaryMove) {
    bool bRestore = m_bAllowChanges;
    m_bAllowChanges = false;
    bool bResult = false;
    Actor* pActor = nullptr;
    if (apRef->IsActor()) {
        pActor = static_cast<Actor*>(apRef);
    }
    if (pActor && pActor->HasEditorLocation()) {
        TESObjectCELL* pInteriorCell = pActor->GetEditorLocationInteriorCell();
        TESWorldSpace* pWorldSpace = pActor->GetEditorLocationWorld();
        if (pInteriorCell || pWorldSpace) {
            apRef->SetLocationOnReference(pActor->GetStartingLocation());
            apRef->SetAngleOnReferenceZ(pActor->GetStartingAngle().z);
            if (abTemporaryMove) {
                SetLoading(true);
            }
            TESObjectREFR::MoveRefToNewSpace(apRef, pInteriorCell, pWorldSpace);
            if (abTemporaryMove) {
                SetLoading(false);
            }
            bResult = true;
        }
    }
    else {
        if (apRef->m_Extra.GetExtraData(15)) {
            NiPoint3 kLoc = apRef->GetStartingLocation();
            NiPoint3 kAngle = apRef->GetStartingAngle();
            apRef->SetLocationOnReference(kLoc);
            apRef->SetAngleOnReference(kAngle);
            bResult = true;
        }
        TESForm* pStartingWorldOrCell = apRef->m_Extra.GetStartingWorldOrCell();
        if (pStartingWorldOrCell) {
            TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pStartingWorldOrCell);
            TESWorldSpace* pWorld = dynamic_cast<TESWorldSpace*>(pStartingWorldOrCell);
            if (pCell || pWorld) {
                TESObjectREFR::MoveRefToNewSpace(apRef, pCell, pWorld);
                bResult = true;
            }
        }
    }
    if (bResult) {
        NiAVObject* pObj3D = apRef->TESObjectREFR::Get3D();
        if (pObj3D) {
            NiPoint3 kLoc = apRef->GetLocationOnReference();
            MobileObject* pMob = dynamic_cast<MobileObject*>(apRef);
            if (pMob) {
                bhkCharacterController* pCon = pMob->GetCharController();
                if (pCon && !pCon->QIsFlying()) {
                    pCon->SetPosition(kLoc);
                }
            }
            pObj3D->SetTranslate(kLoc);
            pObj3D->SetRotate(apRef->GetOrientation());
            bhkNiCollisionObject::ResetSim(pObj3D, true);
            NiUpdateData kUpdateData(0.0f, false, false);
            pObj3D->Update(kUpdateData);
        }
    }
    m_bAllowChanges = bRestore;
    return bResult;
}

void TESSaveLoadGame::SaveGlobalData(BSFile* apFile) {
    int32_t iCurrentGridY;
    uint32_t uiWorldspaceID;
    uint16_t iSize;
    uint32_t uiNextID;
    int32_t iCurrentGridX;
    int32_t iTargetCount;
    uint32_t uiLocationID;
    NiPoint3 kPlayerLocation;
    uiNextID = pDataHandler->GetNextFormID();
    WriteFile(apFile, &uiNextID, 4u);
    uiWorldspaceID = TES::pInstance->pWorldSpace->iFormID;
    WriteFile(apFile, &uiWorldspaceID, 4u);
    iCurrentGridX = TES::pInstance->iCurrentGridX;
    iCurrentGridY = TES::pInstance->iCurrentGridY;
    WriteFile(apFile, &iCurrentGridX, 4u);
    WriteFile(apFile, &iCurrentGridY, 4u);
    TESWorldSpace* pWorld = pPlayer->GetWorldSpace();
    TESObjectCELL* pCell = pPlayer->GetParentCell();
    if (!pWorld && !pCell) {
        InitializationFlow("Player has no worldspace or parent cell, cannot save.");
    }
    uiLocationID = 0;
    if (pWorld) {
        uiLocationID = pWorld->iFormID;
    }
    else if (pCell) {
        uiLocationID = pCell->iFormID;
    }
    kPlayerLocation = *pPlayer->QLocationOnReference();
    WriteFile(apFile, &uiLocationID, 4u);
    WriteFile(apFile, &kPlayerLocation, 12u);
    SaveGlobals(apFile);
    iSize = TES::pInstance->GetSaveSize();
    WriteFile(apFile, &iSize, 2u);
    if (iSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(iSize, "TES Class");
        }
        void* pTempBuffer = CreateBuffer(iSize);
        TES::pInstance->SaveGameTES();
        SaveBuffer(apFile, pTempBuffer, iSize);
        DestroyBuffer(pTempBuffer);
    }
    iSize = ReferenceProcessLists.GetCrimesSaveSize();
    WriteFile(apFile, &iSize, 2u);
    if (iSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(iSize, "Process Lists Class");
        }
        void* pTempBuffer = CreateBuffer(iSize);
        ReferenceProcessLists.SaveGame_Crimes();
        SaveBuffer(apFile, pTempBuffer, iSize);
        DestroyBuffer(pTempBuffer);
    }
    iSize = Sky::GetInstance()->GetSaveSize();
    WriteFile(apFile, &iSize, 2u);
    if (iSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(iSize, "Sky/Weather");
        }
        void* pTempBuffer = CreateBuffer(iSize);
        Sky::GetInstance()->SaveGame();
        SaveBuffer(apFile, pTempBuffer, iSize);
        DestroyBuffer(pTempBuffer);
    }
    iTargetCount = 0;
    WriteFile(apFile, &iTargetCount, 4u);
    SaveCreatedBaseObjects(apFile);
    iSize = 0;
    WriteFile(apFile, &iSize, 2u);
    if (iSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(iSize, "HUD Reticle");
        }
        void* pTempBuffer = CreateBuffer(iSize);
        SaveBuffer(apFile, pTempBuffer, iSize);
        DestroyBuffer(pTempBuffer);
    }
    iSize = Interface::GetSaveSize();
    WriteFile(apFile, &iSize, 2u);
    if (iSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(iSize, "Interface");
        }
        void* pTempBuffer = CreateBuffer(iSize);
        Interface::SaveGameTES();
        SaveBuffer(apFile, pTempBuffer, iSize);
        DestroyBuffer(pTempBuffer);
    }
    iSize = TESDataHandler::RegionList::GetSaveSize();
    WriteFile(apFile, &iSize, 2u);
    if (iSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(iSize, "Regions");
        }
        void* pTempBuffer = CreateBuffer(iSize);
        TESRegion::SaveGame();
        SaveBuffer(apFile, pTempBuffer, iSize);
        DestroyBuffer(pTempBuffer);
    }
}

void TESSaveLoadGame::SaveFinalData(BSFile* apFile) {
    uint32_t uiSize = ReferenceProcessLists.GetSaveSize();
    WriteFile(apFile, &uiSize, 4u);
    if (uiSize) {
        if (m_pSaveLoadStats) {
            m_pSaveLoadStats->AddExtraStat(uiSize, "Temp Effects List");
        }
        void* pBlock = CreateBuffer(uiSize);
        ReferenceProcessLists.SaveTempEffectsList();
        SaveBuffer(apFile, pBlock, uiSize);
        DestroyBuffer(pBlock);
    }
}

uint32_t TESSaveLoadGame::CheckFlags(TESForm* apForm, uint32_t aiFlags) {
    NiPoint3 kStartingLocation;
    int32_t iFlags = CheckNewReference(apForm, aiFlags);
    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(apForm);
    TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(apForm);
    if (pCell) {
        if (iFlags < 0 && !pCell->GetSeenData()) {
            iFlags &= ~static_cast<uint32_t>(CHANGE_CELL_SEENDATA);
        }
    }
    else if (pRef) {
        if ((iFlags & CHANGE_REFR_INVENTORY) != 0 && !pRef->m_Extra.GetContainerChanges()) {
            iFlags &= ~CHANGE_REFR_INVENTORY;
        }
        Actor* pActor = dynamic_cast<Actor*>(pRef);
        if (pActor) {
            if (pActor->pCurrentProcess) {
                pActor->pCurrentProcess->GetRunOncePackage();
            }
            if (pActor->pCurrentProcess && pActor->pCurrentProcess->BaseProcess::GetPackageThatIsRunning()) {
                TESPackage* pPackage = pActor->pCurrentProcess->BaseProcess::GetPackageThatIsRunning();
                iFlags |= pActor->GetPackageSaveFlags(pPackage);
            }

            if (pActor->CheckShouldSaveHavokData(iFlags)) {
                iFlags |= CHANGE_REFR_HAVOK_MOVE;
            }
            else if (!pActor->IsDead(false)) {
                iFlags &= ~CHANGE_REFR_HAVOK_MOVE;
            }
        }
        if ((iFlags & (CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) != 0 && !pRef->GetRefPersists() && pRef != pPlayer) {
            Actor* pRefActor = nullptr;
            if (pRef->IsActor()) {
                pRefActor = static_cast<Actor*>(pRef);
            }
            TESObjectCELL* pInteriorCell = pRefActor ? pRefActor->GetEditorLocationInteriorCell() : nullptr;
            TESWorldSpace* pWorld = pRefActor ? pRefActor->GetEditorLocationWorld() : nullptr;
            if (pWorld || pInteriorCell) {
                NiPoint3* pLoc;
                if (pRefActor) {
                    pLoc = ::new (&kStartingLocation) NiPoint3(pRefActor->GetStartingLocation());
                }
                else {
                    pLoc = &NiPoint3::ZERO;
                }
                float fX = pLoc->x;
                float fY = pLoc->y;
                if (pInteriorCell) {}
                else if (pWorld) {
                    TESWorldSpace* pRefWorld = pRef->GetWorldSpace();
                    if (pWorld == pRefWorld) {
                        int32_t iCellX = FloatToInt(fX) >> 12;
                        int32_t iCellY = FloatToInt(fY) >> 12;
                        pWorld->GetCellFromCellCoord(iCellX, iCellY);
                    }
                }
            }
            else if (pRef->GetParentCell() && !pRef->GetParentCell()->GetInterior()) {
                if (pRefActor) {
                    Warning("EDITOR: Actor does not have an editor location.  This should never happen.");
                }
                NiPoint3 kLoc = pRef->GetStartingLocation();
                int32_t iCellX = FloatToInt(kLoc.x) >> 12;
                int32_t iCellY = FloatToInt(kLoc.y) >> 12;
                pWorld = pRef->GetWorldSpace();
                pWorld->GetCellFromCellCoord(iCellX, iCellY);
            }
        }
    }
    return iFlags;
}

void TESSaveLoadGame::CreateChangedReference(uint32_t aiReferenceID) {
    ChangeData* pChange = m_pChanges->GetChanges(aiReferenceID);
    if (pChange) {
        Warning("CELLS: Reference in cell map has neither required flag.");
    }
}

void TESSaveLoadGame::SaveInitialData(TESForm* apForm, uint32_t aiFlags) {
    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(apForm);
    if (pRef) {
        ReferenceData kRefData;
        TESObjectCELL* pParentCell = pRef->GetParentCell();
        TESWorldSpace* pWorldSpace = nullptr;
        if (pParentCell) {
            pWorldSpace = pParentCell->GetWorldSpace();
        }
        if (pWorldSpace) {
            kRefData.iLocationID = AddNumericIDToArray(pWorldSpace->iFormID);
        }
        else if (pParentCell) {
            kRefData.iLocationID = AddNumericIDToArray(pParentCell->iFormID);
        }
        else {
            if (!pRef->GetRefPersists()) {
                Warning(
                    "SAVELOAD: Error saving reference %08X: Non-persistent reference has no parent cell",
                    pRef->iFormID
                );
            }
            if (pRef->GetRefPersists() && !pRef->m_Extra.GetPersistentCell()) {
                Warning(
                    "SAVELOAD: Error saving reference %08X:Persistent reference has no parent cell and no "
                    "persistent cell extra data",
                    pRef->iFormID
                );
            }
            MobileObject* pMobile = dynamic_cast<MobileObject*>(pRef);
            if (pMobile && pMobile->GetCurrentAIProcess()) {
                if (pMobile->GetCurrentAIProcess()->GetProcessLevel()) {
                    if (pMobile->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH) {
                        Warning(
                            "SAVELOAD: (Error saving actor %08X) Actor has a middle high process but no "
                            "parent cell",
                            pRef->iFormID
                        );
                    }
                }
                else {
                    Warning(
                        "SAVELOAD: (Error saving actor %08X) Actor has a high process but no parent cell",
                        pRef->iFormID
                    );
                }
            }
        }
        kRefData.Loc = *pRef->QLocationOnReference();
        kRefData.Angle = pRef->GetAngleOnReference();
        if ((aiFlags & (CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) != 0) {
            pSaveLoadGameOLD->SaveGameDataOLD(&kRefData, sizeof(kRefData));
        }
    }
}

int32_t TESSaveLoadGame::GetInitialDataSaveSize(TESForm* apForm, uint32_t aiFlags) {
    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(apForm);
    if (!pRef && dynamic_cast<TESObjectCELL*>(apForm)) {
        return 0;
    }
    if (pRef && (aiFlags & (CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) != 0) {
        return 28;
    }
    return 0;
}

void TESSaveLoadGame::SavePluginList(BSFile* apFile) {
    uint8_t cLength;
    uint8_t cCount;
    cCount = pDataHandler->CountCompiled();
    uint32_t uiPos = 0;
    if (m_pSaveLoadStats) {
        uiPos = apFile->Tell();
    }
    WriteFile(apFile, &cCount, 1u);
    for (uint8_t i = 0; i < static_cast<int32_t>(cCount); ++i) {
        char* pFileName = pDataHandler->GetCompiledFile(i)->m_Filename;
        cLength = BSstrlen(pFileName);
        WriteFile(apFile, &cLength, 1u);
        WriteFile(apFile, pFileName, cLength);
    }
    if (m_pSaveLoadStats) {
        uint32_t uiEndPos = apFile->Tell();
        m_pSaveLoadStats->AddExtraStat(uiEndPos - uiPos, "Plugin List");
    }
}

void TESSaveLoadGame::SaveHeader(BSFile* apFile, const char* apFileName) {
    uint32_t uiZero;
    uint32_t uiNextSaveNumber;
    _SYSTEMTIME kLocalTime;
    uint32_t uiTotalPlayingTime;
    uint32_t uiScreenshotSize;
    int32_t iWidth;
    uint32_t uiSize;
    float fGameDate;
    int32_t iHeight;
    uint8_t cLocationNameLength;
    uint8_t cPlayerNameLength;
    uint16_t iLevel;
    ResetCurrentVersion();
    ResetVersions();
    WriteFile(apFile, pSaveGameHeaderString, BSstrlen(pSaveGameHeaderString));
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
    iLevel = pPlayer->GetLevel();
    BSStringT<char> kMapName;
    pPlayer->GetMapNameForLocation(kMapName);
    const char* pLocationName = kMapName.pString;
    if (!pLocationName) {
        pLocationName = pPlayer->GetParentCell()->GetFormEditorID();
    }
    cLocationNameLength = 0;
    if (pLocationName) {
        cLocationNameLength = BSstrlen(pLocationName) + 1;
    }
    double dDaysPassed = static_cast<double>(appCalendar.GetDaysPassed());
    fGameDate = appCalendar.GetHour() / 24.0 + dDaysPassed;
    GetLocalTime(&kLocalTime);
    uiScreenshotSize = 0;
    iWidth = 256;
    iHeight = 256;
    NiPixelData* pPixelData = nullptr;
    uint32_t uiPixSize = 0;
    if (GetLoading()) {
        double dScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
        float fInverseAspectRatio = dScreenHeight / static_cast<double>(Renderer::GetScreenWidth());
        int32_t iNewHeight = iHeight * static_cast<int32_t>(fInverseAspectRatio);
        uiScreenshotSize = 3 * (iNewHeight * iWidth) + 8;
    }
    else {
        pIOManager->Resume();
        pPixelData = ScreenShot::TakeSaveGameScreenShot(iWidth, iHeight, nullptr);
        pIOManager->Pause();
        if (pPixelData) {
            uiPixSize = pPixelData->GetSizeInBytes(0, 0);
            uiScreenshotSize = uiPixSize + 8;
        }
    }
    uiTotalPlayingTime = pPlayer->GetTotalPlayingTime();
    uiSize = cLocationNameLength + (uiScreenshotSize + cPlayerNameLength) + 36;
    WriteFile(apFile, &uiSize, 4u);
    if (GetLoading() || (apFileName && (std::strstr(apFileName, "quicksave") || std::strstr(apFileName, "autosave")))) {
        uiZero = 0;
        WriteFile(apFile, &uiZero, 4u);
    }
    else {
        uiNextSaveNumber = GetNextSaveNumber();
        WriteFile(apFile, &uiNextSaveNumber, 4u);
        ++m_iNextSaveNumber;
    }
    WriteFile(apFile, &cPlayerNameLength, 1u);
    WriteFile(apFile, pPlayerName, cPlayerNameLength);
    WriteFile(apFile, &iLevel, 2u);
    WriteFile(apFile, &cLocationNameLength, 1u);
    if (pLocationName) {
        WriteFile(apFile, pLocationName, cLocationNameLength);
    }
    WriteFile(apFile, &fGameDate, 4u);
    WriteFile(apFile, &uiTotalPlayingTime, 4u);
    WriteFile(apFile, &kLocalTime, 16u);
    WriteFile(apFile, &uiScreenshotSize, 4u);
    if (GetLoading()) {
        m_iSimulationFileSize += uiScreenshotSize;
    }
    else if (pPixelData) {
        WriteFile(apFile, &iWidth, 4u);
        WriteFile(apFile, &iHeight, 4u);
        WriteFile(apFile, pPixelData->GetPixels(0u, 0u), uiPixSize);
        if (pPixelData) {
            delete pPixelData;
        }
    }
    if (m_pSaveLoadStats) {
        m_pSaveLoadStats->AddExtraStat(uiSize, "Save Game Header");
    }
}

uint32_t TESSaveLoadGame::GetCreatedExteriorCellFormID(uint32_t aiWorldSpaceID, int32_t aiCellX, int32_t aiCellY) {
    BSSimpleList<ExteriorCellReferenceData*>* pDataList;
    if (!m_pCreatedExteriorCells) {
        return 0;
    }
    if (!m_pCreatedExteriorCells->GetAt(aiWorldSpaceID, pDataList)) {
        return 0;
    }
    for (ExteriorCellReferenceData* pData : *pDataList) {
        if (pData && aiCellX == pData->iCellX && aiCellY == pData->iCellY) {
            uint32_t uiCellID = pData->iFormID;
            pDataList->Remove(pData);
            ourFree(pData);
            return uiCellID;
        }
    }
    return 0;
}

void TESSaveLoadGame::SaveNumericIDArrays(BSFile* apFile) {
    int32_t iWorldSpaceID;
    int32_t iNumericID;
    uint32_t uiNumericIDCount;
    uint32_t uiWorldSpaceIDCount;
    uiNumericIDCount = m_pNumericIDArray->m_uiSize;
    WriteFile(apFile, &uiNumericIDCount, 4u);
    for (uint32_t uiIndex = 0; uiIndex < uiNumericIDCount; ++uiIndex) {
        iNumericID = m_pNumericIDArray->GetAt(uiIndex);
        WriteFile(apFile, &iNumericID, 4u);
    }
    uiWorldSpaceIDCount = m_pWorldSpaceIDArray->m_uiSize;
    WriteFile(apFile, &uiWorldSpaceIDCount, 4u);
    for (uint32_t i = 0; i < uiWorldSpaceIDCount; ++i) {
        iWorldSpaceID = m_pWorldSpaceIDArray->GetAt(i);
        WriteFile(apFile, &iWorldSpaceID, 4u);
    }
    if (m_pSaveLoadStats) {
        char acDescription[268];
        BSsprintf(acDescription, 260, "Numeric ID Array(%i)", uiNumericIDCount);
        m_pSaveLoadStats->AddExtraStat(4 * uiNumericIDCount + 4, acDescription);
        BSsprintf(acDescription, 260, "WorldSpace ID Array(%i)", uiWorldSpaceIDCount);
        m_pSaveLoadStats->AddExtraStat(4 * uiWorldSpaceIDCount + 4, acDescription);
    }
}

// Keeps the save's file name (BSFile's slot-6 getter: its path), without its folders and its 4-character extension,
// in m_pMostRecentSaveGame.
void TESSaveLoadGame::SetMostRecentSaveGame(BSFile* apFile) {
    if (m_pMostRecentSaveGame) {
        ourFree(this->m_pMostRecentSaveGame);
        this->m_pMostRecentSaveGame = nullptr;
    }
    if (apFile && apFile->m_bGood) {
        char acName[268];
        char* pName = apFile->GetDevice();
        for (char* i = std::strstr(pName, "\\"); i; i = std::strstr(pName, "\\")) {
            pName = i + 1;
        }
        BSstrcpy(acName, 260, pName);
        uint32_t uiLength = BSstrlen(acName);
        acName[uiLength - 4] = 0;
        uint32_t uiSize = BSstrlen(acName) + 1;
        this->m_pMostRecentSaveGame = static_cast<char*>(ourMalloc(uiSize));
        BSstrcpy(this->m_pMostRecentSaveGame, uiSize, acName);
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
                        pList = new BSSimpleList<ExpiredCellData*>;
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
            if (uiFormType == REFR_ID ||
                uiFormType == ACHR_ID ||
                uiFormType == ACRE_ID ||
                uiFormType == PMIS_ID ||
                uiFormType == PGRE_ID ||
                uiFormType == PBEA_ID ||
                uiFormType == PCBE_ID ||
                uiFormType == PFLA_ID) {
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
                if (uiFormType == REFR_ID ||
                    uiFormType == ACHR_ID ||
                    uiFormType == ACRE_ID ||
                    uiFormType == PMIS_ID ||
                    uiFormType == PGRE_ID ||
                    uiFormType == PBEA_ID ||
                    uiFormType == PCBE_ID ||
                    uiFormType == PFLA_ID) {
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
    pSaveLoadGameOLD->LoadGameDataOLD(&usSize, sizeof(usSize));
    apRef->AttachAmmo(nullptr);
    m_pBuffer = pOldBuffer;
}

// Only LoadGameDataOLD fills usSize, so the size passed to LoadHavokData is otherwise uninitialised.
void TESSaveLoadGame::LoadHavokDataForReference(TESObjectREFR* apRef, void* apBuffer) {
    void* pOldBuffer = m_pBuffer;
    m_pBuffer = apBuffer;
    uint16_t usSize;
    pSaveLoadGameOLD->LoadGameDataOLD(&usSize, sizeof(usSize));
    apRef->LoadHavokData(usSize);
    m_pBuffer = pOldBuffer;
}

uint32_t TESSaveLoadGame::CalculateNextSaveNumber() {
    uint32_t uiSaveNumber;
    BSSimpleList<SaveGameFile*>* pList = m_pSaveGameList;
    bool bCreatedSaveGameList = false;
    if (!pList) {
        BuildSaveGameList();
        pList = m_pSaveGameList;
        bCreatedSaveGameList = true;
    }
    uint32_t uiHighestNumberFound = 0;
    while (pList && !pList->IsEmpty()) {
        GetSaveGameDisplayData(pList->m_item, &uiSaveNumber, nullptr, nullptr, nullptr);
        if (uiSaveNumber > uiHighestNumberFound) {
            uiHighestNumberFound = uiSaveNumber;
        }
        pList = pList->m_pkNext;
    }
    if (bCreatedSaveGameList) {
        DeleteSaveGameList();
    }
    ResetCurrentVersion();
    ResetVersions();
    m_iNextSaveNumber = uiHighestNumberFound + 1;
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
    uint32_t uiFormID = apForm->TESForm::GetFormID();
    if (pDataHandler->IsFormIDCreated(uiFormID)) {
        if (dynamic_cast<TESObjectREFR*>(apForm)) {
            aiFlags &= ~CHANGE_REFR_MOVE;
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

uint32_t TESSaveLoadGame::ConvertNumericID(uint32_t aiID) {
    uint8_t cFileIndex = (aiID & 0xFF000000u) >> 24;
    if (!m_pFileIndexArray || cFileIndex == 255) {
        return aiID;
    }
    if (cFileIndex >= m_iSavedPluginCount) {
        return 0;
    }
    uint8_t cNewIndex = m_pFileIndexArray[cFileIndex];
    if (cNewIndex == 255) {
        return 0;
    }
    return (cNewIndex << 24) + (aiID & 0xFFFFFF);
}

uint32_t TESSaveLoadGame::ReverseConvertNumericID(uint32_t aiFormID) {
    uint8_t cFileIndex = (aiFormID & 0xFF000000u) >> 24;
    if (!m_pFileIndexArray || cFileIndex == 255) {
        return aiFormID;
    }
    uint8_t cSavedIndex = -1;
    for (int32_t i = 0; i < m_iSavedPluginCount; ++i) {
        if (m_pFileIndexArray[i] == cFileIndex) {
            cSavedIndex = i;
        }
    }
    if (cSavedIndex == 255) {
        return 0;
    }

    return (cSavedIndex << 24) + (0xFFFFFFu & aiFormID);
}

// Saves the global variables: their count, then each global's form ID and value.
void TESSaveLoadGame::SaveGlobals(BSFile* apFile) {
    BSSimpleList<TESGlobal*>* pNode = pDataHandler->GetGlobalList();
    uint16_t usCount = pNode->ItemsInList();
    uint16_t usSize = 8 * usCount + 2;
    void* pBuffer = CreateBuffer(usSize);
    SaveGameDataOLD(&usCount, sizeof(usCount));
    while (pNode) {
        TESGlobal* pGlobal = pNode->GetItem();
        if (pGlobal) {
            uint32_t uiFormID = pGlobal->GetFormID();
            float fValue = pGlobal->GetValue();
            SaveNumericID(&uiFormID, 4u);
            SaveGameDataOLD(&fValue, sizeof(fValue));
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
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TES_REFERENCES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        0x133B
    );
    TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
    TESBoundObject* pBound = nullptr;
    if (apCreatedData->eType == TES_CREATED_REFERENCE_MAGICPROJECTILE ||
        (pBound = dynamic_cast<TESBoundObject*>(TESForm::GetFormByNumericID(apCreatedData->iBoundID))) != nullptr) {
        if (pForm) {
            TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
            if (!pRef ||
                ((apCreatedData->eType && apCreatedData->eType != TES_CREATED_REFERENCE_PERSISTENT) ||
                 pRef->GetObjectReference() != pBound)) {
                DeleteForm(pForm);
                pForm = nullptr;
            }
        }
        if (!pForm) {
            if (apCreatedData->eType && apCreatedData->eType != TES_CREATED_REFERENCE_PERSISTENT) {
                if (apCreatedData->eType == TES_CREATED_REFERENCE_ARROWPROJECTILE) {
                    pForm = new ArrowProjectile;
                    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
                    pRef->SetObjectReference(pBound);
                    pForm->SetFormID(aiFormID, true);
                }
                else if (apCreatedData->eType == TES_CREATED_REFERENCE_MAGICPROJECTILE) {
                    TESForm* pProjectile = nullptr;
                    uint32_t iBoundID = apCreatedData->iBoundID;
                    if (iBoundID) {
                        if (iBoundID == 1) {
                            pProjectile = new MagicBoltProjectile;
                        }
                        else if (iBoundID == 3) {
                            pProjectile = new MagicFogProjectile;
                        }
                    }
                    else {
                        pProjectile = new MagicBallProjectile;
                    }
                    pForm = pProjectile;
                    pProjectile->SetFormID(aiFormID, true);
                }
                else {
                    Warning(
                        "FORMS: Invalid created reference type %i for form %08X with bound object %08X in "
                        "location %08X",
                        apCreatedData->eType,
                        aiFormID,
                        apCreatedData->iBoundID,
                        apCreatedData->RefData.iLocationID
                    );
                }
            }
            else {
                int32_t iFormType = pBound->GetFormType();
                if (iFormType == NPC__ID) {
                    pForm = new Character;
                }
                else if (iFormType == CREA_ID) {
                    pForm = new Creature;
                }
                else {
                    pForm = new TESObjectREFR;
                }
                TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
                pRef->SetObjectReference(pBound);
                pForm->SetFormID(aiFormID, true);
            }
        }
        if (pForm) {
            pForm->SetStillLoading(true);
        }
        return pForm;
    }

    Warning(
        "SAVELOAD: Bound object %08X no longer exists.  Reference %08X will not be created.",
        apCreatedData->iBoundID,
        aiFormID
    );
    return nullptr;
}

int32_t TESSaveLoadGame::ComputeArmorHash(TESObjectREFR* apRef) {
    int32_t iHash = 0;
    BipedAnim* pBiped = apRef->GetBiped();
    if (pBiped) {
        BIPED_OBJECT aeBipedObject[5];
        aeBipedObject[0] = BO_HEAD;
        aeBipedObject[1] = BO_HAIR;
        aeBipedObject[2] = BO_UPPER_BODY;
        aeBipedObject[3] = BO_LEFT_HAND;
        aeBipedObject[4] = BO_RIGHT_HAND;
        int32_t iSlotCount = 5;
        for (int32_t i = 0; i < iSlotCount; ++i) {
            BIPOBJECT* pBipObject = pBiped->GetBipObject(aeBipedObject[i]);
            if (pBipObject && pBipObject->pParent) {
                uint32_t uiFormID = ReverseConvertNumericID(pBipObject->pParent->iFormID);
                iHash = (iHash << 16) + (uiFormID + (iHash << 6)) - iHash;
            }
        }
    }
    return iHash;
}

void TESSaveLoadGame::LoadAnimations() {
    NiTMapItem<uint32_t, void*>* pos = m_pAnimationMap->GetFirstPos();
    while (pos) {
        void* pBuffer;
        uint32_t uiFormID;
        TESObjectREFR* pRef;
        m_pAnimationMap->GetNext(pos, uiFormID, pBuffer);
        if (uiFormID) {
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            pRef = dynamic_cast<TESObjectREFR*>(pForm);
        }
        else {
            pRef = pPlayer;
        }
        if (pRef && pRef->TESObjectREFR::Get3D()) {
            LoadAnimationForReference(pRef, uiFormID, pBuffer);
        }

        ourFree(pBuffer);
        m_pAnimationMap->RemoveAt(uiFormID);
    }
}

void TESSaveLoadGame::LoadAnimationForReference(TESObjectREFR* apRef, uint32_t aiFormID, void* apBuffer) {
    char acSize[4];
    Animation* pAnimation;
    if (apRef == pPlayer) {
        if (aiFormID) {
            pAnimation = pPlayer->GetAnimation(false);
        }
        else {
            pAnimation = pPlayer->GetAnimation(true);
        }
    }
    else {
        pAnimation = apRef->GetAnimation();
    }
    MobileObject* pMob = dynamic_cast<MobileObject*>(apRef);
    HighProcess* pHighProcess = nullptr;
    ANIMATION_ACTION eAnimAction = ANIM_ACTION_NONE;
    BSAnimGroupSequence* pSeq = nullptr;
    if (pMob && aiFormID) {
        pHighProcess = dynamic_cast<HighProcess*>(pMob->pCurrentProcess);
        if (pHighProcess) {
            eAnimAction = pHighProcess->GetAnimAction();
            pSeq = pHighProcess->GetAnimActionAnimSeq();
        }
    }

    void* pTempBuffer = m_pBuffer;
    m_pBuffer = apBuffer;
    pSaveLoadGameOLD->LoadGameDataOLD(acSize, 2u);
    if (pAnimation) {
        pAnimation->LoadGameTES(apRef);
    }
    m_pBuffer = pTempBuffer;
    if (pHighProcess) {
        if (pSeq) {
            // The sequence pointer's value less 8 is passed as the section; the pointer's value is
            // used as an integer.
            pSeq = pAnimation->GetCurrentSequence(
                static_cast<ANIM_GROUP_SECTION>(reinterpret_cast<uintptr_t>(pSeq) - 8)
            );
        }
        pHighProcess->SetAnimAction(eAnimAction, pSeq);
    }
}

void TESSaveLoadGame::LoadAttachedAnimations() {
    NiTMapItem<uint32_t, void*>* pos = m_pAttachedAnimationMap->GetFirstPos();
    while (pos) {
        void* pBuffer;
        uint32_t uiFormID;
        m_pAttachedAnimationMap->GetNext(pos, uiFormID, pBuffer);
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
        if (pRef) {
            if (!pRef->TESObjectREFR::Get3D()) {
                if (m_pNewChanges) {
                    m_pNewChanges->AddChange(pRef, 0);
                }
                else {
                    m_pChanges->AddChange(pRef, 0);
                }
                m_pAttachedAnimationMap->RemoveAt(uiFormID);
                continue;
            }
            LoadAttachedAnimationForReference(pRef, pBuffer);
        }
        ourFree(pBuffer);
        m_pAttachedAnimationMap->RemoveAt(uiFormID);
    }
}

void TESSaveLoadGame::LoadCharControllers() {
    NiTMapItem<uint32_t, void*>* pos = m_pCharControllerMap->GetFirstPos();
    while (pos) {
        void* pBuffer;
        uint32_t uiFormID;
        m_pCharControllerMap->GetNext(pos, uiFormID, pBuffer);
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        MobileObject* pMob = dynamic_cast<MobileObject*>(pForm);
        if (pMob) {
            MiddleHighProcess* pHighProcess = dynamic_cast<HighProcess*>(pMob->pCurrentProcess);
            if (pHighProcess) {
                char acSize[4];
                void* pTempBuffer = m_pBuffer;
                m_pBuffer = pBuffer;
                pSaveLoadGameOLD->LoadGameDataOLD(acSize, 2u);
                if (pHighProcess->GetCharController()) {
                    pHighProcess->LoadCharController(pMob);
                }
                else if (!pMob->GetDelete() || !pMob->GetDisabled()) {
                    int32_t iMobFormID = pMob->iFormID;
                    Warning(
                        "SAVELOAD: (LoadCharControllers()) Mob %s %08X does not have a character controller to "
                        "load.",
                        pMob->GetFormEditorID(),
                        iMobFormID
                    );
                }
                m_pBuffer = pTempBuffer;
            }
        }
        ourFree(pBuffer);
        m_pCharControllerMap->RemoveAt(uiFormID);
    }
}

void TESSaveLoadGame::LoadHavokData() {
    NiTMapItem<uint32_t, void*>* pos = m_pHavokDataMap->GetFirstPos();
    while (pos) {
        void* pBuffer;
        uint32_t uiFormID;
        m_pHavokDataMap->GetNext(pos, uiFormID, pBuffer);
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
        if (pRef) {
            if (!pRef->TESObjectREFR::Get3D()) {
                if (m_pNewChanges) {
                    m_pNewChanges->AddChange(pRef, 0);
                }
                else {
                    m_pChanges->AddChange(pRef, 0);
                }
                m_pHavokDataMap->RemoveAt(uiFormID);
                continue;
            }
            LoadHavokDataForReference(pRef, pBuffer);
        }
        ourFree(pBuffer);
        m_pHavokDataMap->RemoveAt(uiFormID);
    }
}

void TESSaveLoadGame::QueueFormForFinalization(TESForm* apForm) {
    if (!m_QueuedFormList.IsInList(apForm)) {
        m_QueuedFormList.AddHead(apForm);
    }
}

void TESSaveLoadGame::FinalizeQueuedForms() {
    for (TESForm* pForm : m_QueuedFormList) {
        if (pForm) {
            ArrowProjectile* pArrow = dynamic_cast<ArrowProjectile*>(pForm);
            if (pArrow) {
                TESObjectREFR* pRef = pArrow->GetAttachedRef();
                if (pRef) {
                    NiAVObject* pObj3D = pRef->TESObjectREFR::Get3D();
                    if (pObj3D) {
                        NiUpdateData kUpdateData(0.0f, false, false);
                        pObj3D->Update(kUpdateData);
                    }
                }
                pArrow->ProcessCollision();
            }
        }
    }
    m_QueuedFormList.RemoveAll();
}

void TESSaveLoadGame::BuildSaveGameList() {
    char acSearchPath[268];
    if (m_pSaveGameList) {
        DeleteSaveGameList();
    }
    m_pSaveGameList = new BSSimpleList<SaveGameFile*>;
    std::strcpy(acSearchPath, BSUtilities::GetPersonalPath());
    std::strcat(acSearchPath, sLocalSavePath.String());
    std::strcat(acSearchPath, "*.ess");
    for (const platform::FileEntry& kEntry : platform::FindFiles(acSearchPath)) {
        if (kEntry.size) {
            char acPath[264];
            const char* pSavePath = sLocalSavePath.String();
            BSsprintf(acPath, 260, "%s%s%s", BSUtilities::GetPersonalPath(), pSavePath, kEntry.name.c_str());
            SaveGameFile* pFile = new SaveGameFile(acPath, NiFile::READ_ONLY, 0x20000u);
            m_pSaveGameList->Insert(pFile, &TESSaveLoadGame::SaveGameCompare);
        }
    }
}

void TESSaveLoadGame::DeleteSaveGameList() {
    if (this->m_pSaveGameList) {
        for (BSSimpleList<SaveGameFile*>* pList = this->m_pSaveGameList; pList; pList = pList->m_pkNext) {
            BSFile* pFile = pList->m_item;
            if (pFile) {
                delete pFile;
            }
        }
        this->m_pSaveGameList->RemoveAll();
        BSSimpleList<SaveGameFile*>* pSaveGameList = this->m_pSaveGameList;
        if (pSaveGameList) {
            delete pSaveGameList;
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
            apFile,
            apSaveGameNumber,
            nullptr,
            apLevel,
            nullptr,
            nullptr,
            nullptr,
            apTotalPlayingTime,
            nullptr,
            0
        );
        if (apFileName) {
            // BSFile's slot-6 getter returns the address of the file's name.
            if (std::strstr(apFile->GetDevice(), "quicksave")) {
                BSstrcpy(apFileName, 255, sMenuDisplayQuicksaveName.String());
                BSstrcat(apFileName, 255, " - ");
                BSstrcat(apFileName, 255, apLevel);
            }
            else if (std::strstr(apFile->GetDevice(), "autosave")) {
                BSstrcpy(apFileName, 255, sMenuDisplayAutosaveName.String());
                BSstrcat(apFileName, 255, " - ");
                BSstrcat(apFileName, 255, apLevel);
            }
            else {
                char* pName = std::strrchr(apFile->GetDevice(), '\\') + 1;
                if (_strnicmp(pName, "Save ", 5u)) {
                    BSstrcpy(apFileName, 255, pName);
                    uint32_t uiLength = BSstrlen(apFileName);
                    if (uiLength > 4) {
                        apFileName[uiLength - 4] = 0;
                    }
                    if (uiLength > 18) {
                        apFileName[18] = 0;
                    }
                }
            }
        }
    }
}

NiRefObject* TESSaveLoadGame::GetSaveGameData(
    BSFile* apFile,
    uint32_t* apSaveGameNumber,
    char* Dst,
    char* apDest,
    char* apLocation,
    char* apDays,
    char* apSaveTime,
    char* apTotalPlayingTime,
    uint32_t* apScreenshotHeight,
    char acFileIsOpen
) {
    uint32_t uiPlayingTime;
    float fDays;
    _SYSTEMTIME kSaveTime;
    uint16_t iLevel;
    BSFile* pSaveFile = acFileIsOpen ? apFile : PrepForLoadSave(apFile, nullptr, TESSaveLoadGame::ACTION_READ);
    uint32_t uiSize = 0;
    if (pSaveFile && pSaveFile->m_bGood) {
        uiSize = LoadHeader(pSaveFile, false);
    }

    if (uiSize) {
        iLevel = 0;
        fDays = 0.0f;
        NiRefObject* pScreenshot = ReadSaveGameDataOLD(
            apFile,
            uiSize,
            apSaveGameNumber,
            Dst,
            &iLevel,
            apLocation,
            &fDays,
            &kSaveTime,
            &uiPlayingTime,
            apScreenshotHeight
        );
        if (apDest) {
            BSsprintf(apDest, 300, "%s %i", sMenuDisplayLevelString.String(), iLevel);
        }
        if (apDays) {
            BSsprintf(apDays, 300, "%s %i", sMenuDisplayDayString.String(), static_cast<int64_t>(fDays));
        }
        if (apTotalPlayingTime) {
            uint32_t uiHours = uiPlayingTime / 0x36EE80;
            uiPlayingTime %= 0x36EE80u;
            uint32_t uiMinutes = uiPlayingTime / 0xEA60;
            uiPlayingTime %= 0xEA60u;
            BSsprintf(apTotalPlayingTime, 300, "%02i:%02i:%02i", uiHours, uiMinutes, uiPlayingTime / 1000);
        }
        if (apSaveTime) {
            BSsprintf(
                apSaveTime,
                300,
                "%d/%d/%02d %02d:%02d",
                kSaveTime.wMonth,
                kSaveTime.wDay,
                kSaveTime.wYear,
                kSaveTime.wHour,
                kSaveTime.wMinute
            );
        }
        pSaveFile->Seek(0, NiFile::ms_iSeekSet);
        if (!acFileIsOpen) {
            CleanupLoadSave(pSaveFile, TESSaveLoadGame::ACTION_READ);
        }
        return pScreenshot;
    }

    if (apSaveGameNumber) {
        *apSaveGameNumber = 0;
    }
    if (Dst) {
        BSstrcpy(Dst, 300, "");
    }
    if (apDest) {
        BSstrcpy(apDest, 300, "");
    }
    if (apLocation) {
        BSstrcpy(apLocation, 300, "");
    }
    if (apDays) {
        BSstrcpy(apDays, 300, "");
    }
    if (apSaveTime) {
        BSstrcpy(apSaveTime, 300, "");
    }
    if (apTotalPlayingTime) {
        BSstrcpy(apTotalPlayingTime, 300, "");
    }
    if (apScreenshotHeight) {
        *apScreenshotHeight = 0;
    }
    if (!acFileIsOpen) {
        CleanupLoadSave(pSaveFile, TESSaveLoadGame::ACTION_READ);
    }
    return nullptr;
}

NiRefObject* TESSaveLoadGame::ReadSaveGameDataOLD(
    BSFile* apFile,
    uint32_t aiSize,
    uint32_t* apSaveGameNumber,
    void* apPlayerName,
    uint16_t* apLevel,
    char* Dst,
    float* apDays,
    _SYSTEMTIME* apSaveTime,
    uint32_t* apPlayingTime,
    uint32_t* apScreenshotHeight
) {
    _SYSTEMTIME kLastSaveTime;
    uint32_t uiWidth;
    NiPixelFormat kFormat;
    uint32_t uiHeight;
    uint32_t uiSaveNumber;
    uint32_t uiPlayingTime;
    _SYSTEMTIME kSaveTime;
    uint32_t uiScreenshotSize;
    uint8_t cLength;
    float fDays;
    uint16_t iLevel;
    void* pBlock = CreateBuffer(aiSize);
    LoadBuffer(apFile, pBlock, aiSize);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 56u) {
        LoadGameDataOLD(&uiSaveNumber, sizeof(uiSaveNumber));
        if (apSaveGameNumber) {
            *apSaveGameNumber = uiSaveNumber;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 56u && apSaveGameNumber) {
        *apSaveGameNumber = 0;
    }

    LoadGameDataOLD(&cLength, sizeof(cLength));
    if (apPlayerName) {
        LoadGameDataOLD(apPlayerName, cLength);
    }
    else {
        AdvanceBuffer(cLength);
    }
    iLevel = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 52u) {
        LoadGameDataOLD(&iLevel, sizeof(iLevel));
    }
    if (apLevel) {
        *apLevel = iLevel;
    }
    LoadGameDataOLD(&cLength, sizeof(cLength));
    if (Dst) {
        if (cLength) {
            LoadGameDataOLD(Dst, cLength);
        }
        else {
            BSstrcpy(Dst, 300, sMenuDisplayUnknownLocationString.String());
        }
    }
    else if (cLength) {
        AdvanceBuffer(cLength);
    }
    LoadGameDataOLD(&fDays, sizeof(fDays));
    if (apDays) {
        *apDays = fDays;
    }
    uiPlayingTime = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 29u) {
        LoadGameDataOLD(&uiPlayingTime, sizeof(uiPlayingTime));
    }
    if (apPlayingTime) {
        *apPlayingTime = uiPlayingTime;
    }
    ourMemset(&kSaveTime, 0, sizeof(kSaveTime));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 56u) {
        LoadGameDataOLD(&kSaveTime, sizeof(kSaveTime));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 56u) {
        kSaveTime = *apFile->GetLastSaveTime(&kLastSaveTime);
    }
    if (apSaveTime) {
        *apSaveTime = kSaveTime;
    }
    NiRefObject* pTexture = nullptr;
    LoadGameDataOLD(&uiScreenshotSize, sizeof(uiScreenshotSize));
    if (uiScreenshotSize) {
        if (apScreenshotHeight) {
            NiPixelData* pPixelData;
            LoadGameDataOLD(&uiWidth, sizeof(uiWidth));
            LoadGameDataOLD(&uiHeight, sizeof(uiHeight));
            *apScreenshotHeight = uiHeight;
            if (pSaveLoadGameOLD->GetCurrentVersion() < 46u) {
                AdvanceBuffer(36u);
            }
            kFormat = NiPixelFormat::RGB24;
            pPixelData = new NiPixelData(uiWidth, uiWidth, kFormat, 1u, 1u);
            LoadGameDataOLD(pPixelData->GetPixels(0u, 0u), 3 * (uiHeight * uiWidth));
            NiTexture::FormatPrefs kPrefs;
            kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
            kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
            kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
            pTexture = NiSourceTexture::Create(pPixelData, NiFixedString("SaveGameTexture").GetHandleString(), kPrefs);
            pTexture->IncRefCount();
        }
        else {
            AdvanceBuffer(uiScreenshotSize);
        }
    }
    DestroyBuffer(pBlock);
    return pTexture;
}

char TESSaveLoadGame::RetrieveSaveGameData(BSFile* apFile, int32_t* arIndex, char* apDest, char* Dst) {
    // BSFile's slot-6 getter returns the address of the file's name.
    char* pFileName = std::strrchr(apFile->GetDevice(), '\\') + 1;
    char* pPlayingTime = std::strstr(pFileName, "Playing Time");
    char* pDash = std::strstr(pFileName, "-");
    if (!pPlayingTime || (!pDash || std::strncmp(pFileName, "Save ", 5u))) {
        return 0;
    }
    if (arIndex) {
        char acIndex[268];
        uint32_t uiLength = pDash - (pFileName + 5);
        BSstrncpy(acIndex, pFileName + 5, uiLength);
        acIndex[uiLength] = 0;
        *arIndex = std::atoi(acIndex);
    }
    if (apDest || Dst) {
        char* pPrevComma = nullptr;
        char* pLastComma = nullptr;
        for (char* i = pFileName;; pLastComma = i) {
            i = std::strstr(i + 1, ",");
            if (!i) {
                break;
            }
            pPrevComma = pLastComma;
        }
        if (!pPrevComma || !pLastComma) {
            return 0;
        }
        if (apDest) {
            char acLevel[264];
            pPrevComma += 2;
            size_t uiLevelLength = BSstrlen(pFileName);
            while (*pPrevComma != ' ' && pPrevComma < &pFileName[uiLevelLength]) {
                ++pPrevComma;
            }
            uiLevelLength = pLastComma - pPrevComma;
            BSstrncpy(acLevel, pPrevComma, pLastComma - pPrevComma);
            acLevel[uiLevelLength] = 0;
            BSsprintf(apDest, 255, "%s%s", sMenuDisplayLevelString.String(), acLevel);
        }
        if (Dst) {
            BSstrcpy(Dst, 255, pPlayingTime + 13);
            char* pExtension = std::strstr(Dst, ".ess");
            if (pExtension) {
                *pExtension = 0;
            }
            for (uint32_t j = 0; j < BSstrlen(Dst); ++j) {
                if (Dst[j] == '.') {
                    Dst[j] = ':';
                }
            }
        }
    }
    return 1;
}

void TESSaveLoadGame::GenerateSaveGameFileName(char* apFileName) {
    if (apFileName) {
        FileFinder::FILE_LOCATION eLocation;
        char acName[268];
        const char* pLocationName;
        BSStringT<char> kMapName;
        int32_t iAttempt = 0;
        const char* pPlayerName = pPlayer->GetFullName();
        pPlayer->GetMapNameForLocation(kMapName);
        // The location is the map name, else the parent cell's editor ID, else "".
        pLocationName = kMapName.String();
        if (!pLocationName) {
            if (pPlayer->GetParentCell()) {
                pLocationName = pPlayer->GetParentCell()->GetFormEditorID();
            }
            else {
                pLocationName = "";
            }
        }
        do {
            char acPath[264];
            char acSuffix[8];
            if (iAttempt) {
                BSsprintf(acSuffix, 5, " #%d", iAttempt + 1);
            }
            else {
                BSstrcpy(acSuffix, 5, "");
            }
            uint32_t uiTotalPlayingTime = pPlayer->GetTotalPlayingTime();
            uint32_t uiMinutes = uiTotalPlayingTime % 0x36EE80 / 0xEA60;
            uint32_t uiSeconds = uiTotalPlayingTime % 0x36EE80 % 0xEA60 / 1000;
            uint32_t uiHours = uiTotalPlayingTime / 0x36EE80;
            int32_t iLevel = pPlayer->GetLevel();
            BSsprintf(
                acName,
                260,
                "Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i",
                GetNextSaveNumber(),
                pPlayerName,
                pLocationName,
                iLevel,
                uiHours,
                uiMinutes,
                uiSeconds,
                acSuffix
            );
            for (char* pInvalidCharacter = std::strpbrk(acName, "\\/:*<>?|\""); pInvalidCharacter;
                 pInvalidCharacter = std::strpbrk(pInvalidCharacter + 1, "\\/:*<>?|\"")) {
                if (*pInvalidCharacter == '"') {
                    *pInvalidCharacter = '\'';
                }
                else {
                    *pInvalidCharacter = ' ';
                }
            }
            const char* pSavePath = sLocalSavePath.String();
            BSsprintf(acPath, 260, "%s%s%s.ess", BSUtilities::GetPersonalPath(), pSavePath, acName);
            eLocation = FileFinder::Locate(acPath, nullptr, 0, 0xFFFFFFFFu);
            ++iAttempt;
        }
        while (eLocation);
        BSstrcpy(apFileName, 260, acName);
    }
}

uint32_t TESSaveLoadGame::LoadHeader(BSFile* apFile, bool abFullLoad) {
    uint32_t uiSize;
    char acHeader[268];
    m_iFileStartPosition = 0;
    if (!ReadFile(apFile, acHeader, BSstrlen(pSaveGameHeaderString))) {
        return 0;
    }
    if (acHeader[0]) {
        if (!std::strncmp(acHeader, "CON ", 4u)) {
            apFile->Seek(0xD000, NiFile::ms_iSeekSet);
            m_iFileStartPosition = 0xD000;
            ReadFile(apFile, acHeader, BSstrlen(pSaveGameHeaderString));
        }
        if (std::strncmp(acHeader, pSaveGameHeaderString, BSstrlen(pSaveGameHeaderString))) {
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
    ReadFile(apFile, &uiSize, 4u);
    if (m_cMajorVersion || (abFullLoad && m_cMinorVersion <= 18u)) {
        char acMessage[523];
        std::sprintf(
            acMessage,
            "Save game version is %i.%02i and current version is %i.%02i, so errors may occur.  Continue trying to "
            "load?",
            m_cMajorVersion,
            m_cMinorVersion,
            0,
            125
        );
        pSaveLoadGameOLD->SetLoading(true);
        const char* pNo = sNo.String();
        char cResult = Interface::CreateModalMessageMenu(acMessage, 1, 5, sYes.String(), pNo, nullptr);
        pSaveLoadGameOLD->SetLoading(false);
        if (cResult == 2) {
            return -1;
        }
    }
    return uiSize;
}

void TESSaveLoadGame::AddCreatedBaseObject(TESForm* apForm) {
    int32_t iFormID;
    if (apForm) {
        if (pDataHandler->IsFormIDCreated(apForm->iFormID)) {
            iFormID = apForm->iFormID;
            if (!m_CreatedBaseObjectsList.IsInList(*reinterpret_cast<uint32_t*>(&iFormID))) {
                uint32_t uiNewFormID = apForm->iFormID;
                m_CreatedBaseObjectsList.AddHead(uiNewFormID);
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
    uint32_t uiCount;
    BSSimpleList<uint32_t>* pList;
    uiCount = 0;
    for (pList = &m_CreatedBaseObjectsList; pList; pList = pList->m_pkNext) {
        uint32_t uiFormID = pList->m_item;
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        if (pForm) {
            TESBoundObject* pBound = dynamic_cast<TESBoundObject*>(pForm);
            SpellItem* pSpell = dynamic_cast<SpellItem*>(pForm);
            TESEnchantableForm* pEnchantable = dynamic_cast<TESEnchantableForm*>(pForm);
            if (!pEnchantable || pEnchantable->pFormEnchanting) {
                if (pEnchantable && pEnchantable->pFormEnchanting) {
                    if (pDataHandler->IsFormIDCreated(pEnchantable->pFormEnchanting->iFormID)) {
                        ++uiCount;
                    }
                }

                if (pBound || pSpell) {
                    ++uiCount;
                }
            }
        }
    }
    WriteFile(apFile, &uiCount, 4u);
    if (uiCount) {
        for (pList = &m_CreatedBaseObjectsList; pList; pList = pList->m_pkNext) {
            uint32_t uiFormID = pList->m_item;
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            if (!pForm) {
                continue;
            }
            TESBoundObject* pBound = dynamic_cast<TESBoundObject*>(pForm);
            SpellItem* pSpell = dynamic_cast<SpellItem*>(pForm);
            if (!pBound && !pSpell) {
                continue;
            }
            TESEnchantableForm* pEnchantable = dynamic_cast<TESEnchantableForm*>(pForm);
            if (pEnchantable) {
                EnchantmentItem* pEnchant = pEnchantable->pFormEnchanting;
                if (!pEnchant) {
                    Warning(
                        "SAVELOAD: Enchantable item %08X with no enchantment exists in the created base objects list.",
                        uiFormID
                    );
                    continue;
                }
                if (pDataHandler->IsFormIDCreated(pEnchant->iFormID)) {
                    pEnchant->Save();
                    uint32_t uiLength = pEnchant->GetFormBufferSize();
                    void* pFormBuffer = pEnchant->GetFormBuffer();
                    WriteFile(apFile, pFormBuffer, uiLength);
                    if (m_pSaveLoadStats) {
                        LoadFormHeader kHeader;
                        kHeader.cFormType = pEnchant->GetFormType();
                        kHeader.iFormID = pEnchant->iFormID;
                        kHeader.iSize = uiLength;
                        kHeader.iFlags = 0;
                        m_pSaveLoadStats->AddStats(&kHeader);
                    }
                    pEnchant->FreeFormBuffer();
                }
            }
            pForm->Save();
            uint32_t uiLength = pForm->GetFormBufferSize();
            void* pFormBuffer = pForm->GetFormBuffer();
            WriteFile(apFile, pFormBuffer, uiLength);
            if (m_pSaveLoadStats) {
                LoadFormHeader kHeader;
                kHeader.cFormType = pForm->GetFormType();
                kHeader.iFormID = uiFormID;
                kHeader.iSize = uiLength;
                kHeader.iFlags = 0;
                m_pSaveLoadStats->AddStats(&kHeader);
            }
            pForm->FreeFormBuffer();
        }
    }
}

int16_t TESSaveLoadGame::AddWorldSpaceIDToArray(int32_t aiWorldSpaceID) {
    uint32_t uiSize = m_pWorldSpaceIDArray->m_uiSize;
    for (uint32_t uiIndex = 0; uiIndex < uiSize; ++uiIndex) {
        int32_t iWorldSpaceID = m_pWorldSpaceIDArray->GetAt(uiIndex);
        if (iWorldSpaceID == aiWorldSpaceID) {
            return uiIndex;
        }
    }
    return m_pWorldSpaceIDArray->Add(*reinterpret_cast<uint32_t*>(&aiWorldSpaceID));
}

int32_t TESSaveLoadGame::GetWorldSpaceIDFromArray(uint16_t aiIndex) {
    if (aiIndex <= m_pWorldSpaceIDArray->m_uiSize) {
        return m_pWorldSpaceIDArray->GetAt(aiIndex);
    }

    return 0;
}

void TESSaveLoadGame::RemoveFormFromInitFormArray(TESForm* apForm) {
    if (m_pInitArray) {
        uint32_t uiSize = m_pInitArray->m_uiSize;
        for (uint32_t i = 0; i < uiSize; ++i) {
            FormAndFlags* pFormAndFlags = m_pInitArray->GetAt(i);
            if (pFormAndFlags && pFormAndFlags->pForm == apForm) {
                m_pInitArray->RemoveAt(i);
                ourFree(pFormAndFlags);
                return;
            }
        }
    }
}

void TESSaveLoadGame::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t TESSaveLoadGame::SaveGameCompare(SaveGameFile* apCompare1, SaveGameFile* apCompare2) {
    _FILETIME kStoredTime2;
    _FILETIME kStoredTime1;
    _FILETIME kTime2;
    _FILETIME kTime1;
    ourMemset(&kTime1, 0, sizeof(kTime1));
    ourMemset(&kTime2, 0, sizeof(kTime2));
    if (apCompare1->GetHasFileTime()) {
        _FILETIME* pFileTime = ::new (&kStoredTime1) _FILETIME(apCompare1->GetFileTime());
        kTime1.dwLowDateTime = pFileTime->dwLowDateTime;
        kTime1.dwHighDateTime = pFileTime->dwHighDateTime;
    }
    else {
        // BSFile's slot-6 getter returns the address of the file's name.
        char* pName = apCompare1->GetDevice();
        // The file's last write time when the search finds it, else zero.
        std::vector<platform::FileEntry> kFound = platform::FindFiles(pName);
        if (!kFound.empty()) {
            kTime1.dwLowDateTime = static_cast<uint32_t>(kFound.front().writeTime);
            kTime1.dwHighDateTime = static_cast<uint32_t>(kFound.front().writeTime >> 32);
        }
        apCompare1->SetFileTime(kTime1);
    }
    if (apCompare2->GetHasFileTime()) {
        _FILETIME* pFileTime = ::new (&kStoredTime2) _FILETIME(apCompare2->GetFileTime());
        kTime2.dwLowDateTime = pFileTime->dwLowDateTime;
        kTime2.dwHighDateTime = pFileTime->dwHighDateTime;
    }
    else {
        char* pName = apCompare2->GetDevice();
        std::vector<platform::FileEntry> kFound = platform::FindFiles(pName);
        if (!kFound.empty()) {
            kTime2.dwLowDateTime = static_cast<uint32_t>(kFound.front().writeTime);
            kTime2.dwHighDateTime = static_cast<uint32_t>(kFound.front().writeTime >> 32);
        }
        apCompare2->SetFileTime(kTime2);
    }
    if (kTime1.dwHighDateTime > kTime2.dwHighDateTime) {
        return -1;
    }
    if (kTime1.dwHighDateTime < kTime2.dwHighDateTime) {
        return 1;
    }
    if (kTime1.dwLowDateTime <= kTime2.dwLowDateTime) {
        return kTime1.dwLowDateTime < kTime2.dwLowDateTime;
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
    ChangeData* pChanges = nullptr;
    GetAt(auiFormID, pChanges);
    return pChanges;
}

ChangeData* ChangesMap::GetChanges(TESForm* apForm) {
    uint32_t uiFormID = apForm->TESForm::GetFormID();
    return GetChanges(uiFormID);
}

bool ChangesMap::RemoveChange(TESForm* apForm, uint32_t aiChange) {
    if (pSaveLoadGameOLD->GetLoading()) {
        return false;
    }
    ChangeData* pChange = GetChanges(apForm);
    if (!pChange) {
        return false;
    }
    pChange->RemoveChange(aiChange);
    if (!pChange->iFlags) {
        RemoveAt(apForm->iFormID);
        delete pChange;
    }
    return true;
}

bool ChangesMap::RemoveChanges(uint32_t aiFormID, bool abForceRemove) {
    ChangeData* pChanges = GetChanges(aiFormID);
    if (!pChanges) {
        return false;
    }
    if (!pChanges->pBuffer || abForceRemove) {
        RemoveAt(aiFormID);
        delete pChanges;
    }
    return true;
}

[[clang::disable_tail_calls]] bool ChangesMap::RemoveChanges(TESForm* apForm, bool abForceRemove) {
    return RemoveChanges(apForm->GetFormID(), abForceRemove);
}

ChangeData* ChangesMap::AddChange(TESForm* apForm, uint32_t aiChange) {
    ChangeData* pChange;
    uint32_t uiFormID = apForm->iFormID;
    if (!GetAt(uiFormID, pChange)) {
        AutoMemContext tempMemContext(
            MEM_CONTEXT_SAVELOAD,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
            260
        );
        ChangeData* pNewChange = new ChangeData;
        pChange = pNewChange;
        SetAt(uiFormID, pNewChange);
    }
    pChange->AddChange(aiChange);
    return pChange;
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
    BSSystemFile kFile(acFileName, 1, 2, false);
    if (kFile.QLastError()) {
        return;
    }

    Stats kTotalStats;
    char acBuffer[520];
    BSsprintf(
        acBuffer,
        520,
        "  %-8s %-5s %-8s %02s %-40s  %s\r\n\r\n",
        "Form ID",
        "Size",
        "Flags",
        "V",
        "Name",
        "Changes"
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
        // Measures the line and drops the length.
        BSstrlen(acBuffer);
        WriteFile(&kFile, acBuffer);

        Stats kTypeStats;
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
                    acBuffer,
                    520,
                    "  %08X %5i %08X %2i %-40s- %s\r\n",
                    pHeader->iFormID,
                    pHeader->iSize,
                    pHeader->iFlags,
                    pHeader->cVersion,
                    pName,
                    acChanges
                );
            }
            else {
                BSsprintf(
                    acBuffer,
                    520,
                    "  %08X %5i %08X %2i %-40s- %s\r\n",
                    pHeader->iFormID,
                    pHeader->iSize,
                    pHeader->iFlags,
                    pHeader->cVersion,
                    "NOT LOADED",
                    acChanges
                );
            }
            WriteFile(&kFile, acBuffer);
        }

        BSsprintf(
            acBuffer,
            520,
            "\r\nTotal %s Num: %i  Total %s Size: %i  Min %s Size: %i  Max %s Size: %i  Average %s Size: %.2f\r\n\r\n",
            acTypeName,
            kTypeStats.iNum,
            acTypeName,
            kTypeStats.iTotalSize,
            acTypeName,
            kTypeStats.iMinSize,
            acTypeName,
            kTypeStats.iMaxSize,
            acTypeName,
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
        acBuffer,
        520,
        "\r\n\r\nTotal Num: %i  Total Size: %i  Min Size: %i  Max Size: %i  Average Size: %.2f\r\n",
        kTotalStats.iNum,
        kTotalStats.iTotalSize,
        kTotalStats.iMinSize,
        kTotalStats.iMaxSize,
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
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        615
    );
    SaveDataDebugInfo::ExtraStat* pExtraStat = static_cast<SaveDataDebugInfo::ExtraStat*>(
        ourMalloc(sizeof(SaveDataDebugInfo::ExtraStat))
    );
    pExtraStat->iSize = aiSize;
    int32_t iDescLen = BSstrlen(apDescription) + 1;
    pExtraStat->pDescription = static_cast<char*>(ourMalloc(iDescLen));
    BSstrcpy(pExtraStat->pDescription, iDescLen, apDescription);
    unkList->AddHead(pExtraStat);
}

void SaveDataDebugInfo::AddStats(SaveFormHeader* apHeader, uint16_t aiSize) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        630
    );
    LoadFormHeader kHeader;
    kHeader.iFlags = apHeader->iFlags;
    kHeader.iFormID = apHeader->iFormID;
    kHeader.cFormType = apHeader->cFormType;
    kHeader.cVersion = apHeader->cVersion;
    kHeader.iSize = aiSize;
    AddStats(&kHeader);
}

void SaveDataDebugInfo::AddStats(LoadFormHeader* apHeader) {
    BSSimpleList<LoadFormHeader*>* pHeaderList;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSaveLoadGame.cpp",
        646
    );
    void* pMemory = ourMalloc(sizeof(LoadFormHeader));
    LoadFormHeader* pHeader = static_cast<LoadFormHeader*>(pMemory);
    *pHeader = *apHeader;
    uint8_t cKey = pHeader->cFormType;
    if (!headerMap->GetAt(cKey, pHeaderList)) {
        BSSimpleList<LoadFormHeader*>* pNewList = new BSSimpleList<LoadFormHeader*>;
        pHeaderList = pNewList;
        headerMap->SetAt(cKey, pNewList);
    }
    pHeaderList->Insert(pHeader, &SaveDataDebugInfo::LoadFormHeaderCompare);
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
