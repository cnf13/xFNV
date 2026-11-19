#include "Fallout Shared/TESDataHandler.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSCameraPath.h"
#include "Fallout Shared/BGSCameraPathManager.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/BGSConstructibleObject.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSDehydration.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSHeadPart.h"
#include "Fallout Shared/BGSHunger.h"
#include "Fallout Shared/BGSIdleMarker.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSLightingTemplate.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/BGSMovableStatic.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSSleepDeprevation.h"
#include "Fallout Shared/BGSStaticCollection.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/BSTaskThread.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MediaLocationController.h"
#include "Fallout Shared/MediaSet.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionDataManager.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAmmoEffect.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCaravan.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESEyes.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFlora.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESHair.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESLevCharacter.h"
#include "Fallout Shared/TESLevCreature.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESLevSpell.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESLoadScreen.h"
#include "Fallout Shared/TESLoadScreenType.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectANIO.h"
#include "Fallout Shared/TESObjectARMA.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectIMOD.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESObjectTREE.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRecipe.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSkill.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/tchar.h"
#include "platform/Paths.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

// The forms in the compiled files, summed by CompileFiles, and the count loaded
// so far.
static uint32_t iTotalForms;
static uint32_t iFormsLoaded;

// The forms ConstructObjectList has read during a save load; every 256th starts the next loading sequence step.
static uint32_t iSaveLoadFormCount;

// The lock GetNextID holds while it hands out an ID.
static BSSpinLock GetNextIDLock;

// The warnings for a plugin whose masters changed since it was saved (one per
// file, then a general one).
static SettingT<INISettingCollection> sMasterMismatchWarning(
    "sMasterMismatchWarning:GeneralWarnings",
    "One of the files that \"%s\" is dependent on has changed since the last save."
);
static SettingT<INISettingCollection> sGeneralMasterMismatchWarning(
    "sGeneralMasterMismatchWarning:GeneralWarnings",
    "One or more plugins could not find the correct versions of the master files they depend on. Errors may occur "
    "during load or game play. Check the \"Warnings.txt\" file for more information.\r\n"
);

// The game settings this file defines, with their IDs and default values, in initialization order.
static SettingT<INISettingCollection> bDisableDuplicateReferenceCheck("bDisableDuplicateReferenceCheck:General", true);
SettingT<INISettingCollection> sLocalMasterPath("sLocalMasterPath:General", "Data\\");
SettingT<INISettingCollection> sLocalSavePath("sLocalSavePath:General", "Saves\\");
static SettingT<INISettingCollection> bLoadTheMap("bCreate Maps Enable:General", false);
SettingT<INISettingCollection> bFileSkipModelChecks("bFileSkipModelChecks:TestAllCells", false);
SettingT<INISettingCollection> bFileCheckModelCollision("bFileCheckModelCollision:TestAllCells", false);
SettingT<INISettingCollection> bFileGoneMessage("bFileGoneMessage:TestAllCells", true);
SettingT<INISettingCollection> bFileNeededMessage("bFileNeededMessage:TestAllCells", true);
SettingT<INISettingCollection> bFileTestLoad("bFileTestLoad:TestAllCells", false);
SettingT<INISettingCollection> bFileUnusedObject("bFileUnusedObject:TestAllCells", false);
SettingT<INISettingCollection> bFileSkipIconChecks("bFileSkipIconChecks:TestAllCells", false);
SettingT<INISettingCollection> bFileShowIcons("bFileShowIcons:TestAllCells", true);
SettingT<INISettingCollection> bFileShowTextures("bFileShowTextures:TestAllCells", true);
static SettingT<INISettingCollection> bKeepPlugin("bKeepPluginWhenMerging:General", false);

// The engine's start-up log: an empty body in this build, which reads no argument, writes no memory and leaves the
// return value alone.
void InitializationFlow(...) {}

int32_t BSsplitpath(
    const char* apPath,
    char* apDevice,
    int32_t aiDeviceLen,
    char* apDir,
    int32_t aiDirLen,
    char* apFile,
    int32_t aiFileLen,
    char* apExt,
    int32_t aiExtLen
) {
    return _splitpath_s(apPath, apDevice, aiDeviceLen, apDir, aiDirLen, apFile, aiFileLen, apExt, aiExtLen);
}

int32_t makepath(char* a1, uint32_t a2, char* a3, char* a4, char* a5, char* a6) {
    return _makepath_s(a1, a2, a3, a4, a5, a6);
}

uint16_t TESDataHandler::RegionList::GetSaveSize() {
    uint16_t usSize = 0;
    usSize += 2;
    usSize += pDataHandler->GetRegionList()->ItemsInList() * 8;
    return usSize;
}

// Creates the object and region lists and the region, idle and camera path managers.
TESDataHandler::TESDataHandler()
  : ucDLCFlags(0), pObjectList(nullptr), pRegionList(nullptr), arrayInteriorCells(0, 1), arrayAddonNodes(0, 1),
    iNextID(2048), pActiveFile(nullptr), iNumCompile(0), bMasterSave(false), bSaveLoadGame(false), bSaveLoad(false),
    bAutoSaving(false), bExportingPlugin(false), bClearingData(false), bHasDesiredFiles(true), bCheckingModels(false),
    bDontRemoveIDs(false), ucGameSettingsLoadState(0), pRegionDataManager(nullptr), pBarterContainer(nullptr),
    pRecipeContainer(nullptr), pSpotterShader(nullptr), pItemDetectedShader(nullptr), pCateyeMobileShader(nullptr) {
    ourMemset(pFileIndex, 0, sizeof(pFileIndex));
    pObjectList = new TESObjectList(true);
    pRegionList = new TESRegionList(true);
    arrayInteriorCells.m_usGrowBy = 100;
    pRegionDataManager = new TESRegionDataManager();
    pIdleManager = new TESIdleManager();
    pCameraPathManager = new BGSCameraPathManager();
}

TESDataHandler::~TESDataHandler() {
    delete pBarterContainer;
    ClearFileList();
    delete pObjectList;
    delete pRegionList;
    // The game frees the region data manager without destroying it; here the constructor builds it with new, so it is
    // deleted.
    delete pRegionDataManager;
    delete pIdleManager;
    pIdleManager = nullptr;
    delete pCameraPathManager;
    pCameraPathManager = nullptr;
    if (TESForm::pAllForms) {
        TESForm::pAllForms->RemoveAll();
    }
}

void TESDataHandler::RemoveIDFromDataHandler(uint32_t aiID) {
    if (aiID != 0 && (aiID >> 24) == 0xFF && !bDontRemoveIDs && aiID < iNextID && (aiID & 0xFFFFFF) > 0x7FF) {
        iNextID = aiID;
    }
}

BSSimpleList<BGSHeadPart*>* TESDataHandler::GetHeadPartList() {
    return &listHeadParts;
}

BSSimpleList<TESHair*>* TESDataHandler::GetHairList() {
    return &listHair;
}

BSSimpleList<TESRace*>* TESDataHandler::GetRaceList() {
    return &listRaces;
}

BSSimpleList<TESLandTexture*>* TESDataHandler::GetLandTextList() {
    return &listLandTexts;
}

BSSimpleList<BGSCameraShot*>* TESDataHandler::GetCameraShotList() {
    return &listCameraShots;
}

BSSimpleList<TESClass*>* TESDataHandler::GetClassList() {
    return &listClasses;
}

BSSimpleList<TESFaction*>* TESDataHandler::GetFactionList() {
    return &listFactions;
}

BSSimpleList<TESReputation*>* TESDataHandler::GetReputationList() {
    return &listReputations;
}

BSSimpleList<TESChallenge*>* TESDataHandler::GetChallengeList() {
    return &listChallenges;
}

BSSimpleList<TESRecipe*>* TESDataHandler::GetRecipeList() {
    return &listRecipes;
}

BSSimpleList<TESCasino*>* TESDataHandler::GetCasinoList() {
    return &listCasinos;
}

BSSimpleList<Script*>* TESDataHandler::GetScriptList() {
    return &listScripts;
}

BSSimpleList<TESSound*>* TESDataHandler::GetSoundList() {
    return &listSounds;
}

BSSimpleList<BGSAcousticSpace*>* TESDataHandler::GetAcousticSpaceList() {
    return &listAcousticSpaces;
}

BSSimpleList<BGSRagdoll*>* TESDataHandler::GetRagdollList() {
    return &listRagdolls;
}

BSSimpleList<TESGlobal*>* TESDataHandler::GetGlobalList() {
    return &listGlobals;
}

BSSimpleList<BGSVoiceType*>* TESDataHandler::GetVoiceTypeList() {
    return &listVoiceTypes;
}

BSSimpleList<BGSImpactData*>* TESDataHandler::GetImpactDataList() {
    return &listImpactData;
}

BSSimpleList<BGSImpactDataSet*>* TESDataHandler::GetImpactDataSetList() {
    return &listImpactDataSet;
}

BSSimpleList<TESTopic*>* TESDataHandler::GetTopicList() {
    return &listTopics;
}

BSSimpleList<TESQuest*>* TESDataHandler::GetQuestList() {
    return &listQuests;
}

BSSimpleList<TESCombatStyle*>* TESDataHandler::GetCombatStyleList() {
    return &listCombatStyles;
}

BSSimpleList<TESLoadScreen*>* TESDataHandler::GetLoadScreenList() {
    return &listLoadScreens;
}

BSSimpleList<TESWaterForm*>* TESDataHandler::GetWaterList() {
    return &listWater;
}

BSSimpleList<TESEffectShader*>* TESDataHandler::GetEffectShaderList() {
    return &listEffectShaders;
}

BSSimpleList<BGSProjectile*>* TESDataHandler::GetProjectileList() {
    return &listProjectiles;
}

BSSimpleList<BGSRadiationStage*>* TESDataHandler::GetRadiationStageList() {
    return &listRadiation;
}

BSSimpleList<BGSDehydrationStage*>* TESDataHandler::GetDehydrationStageList() {
    return &listDehydration;
}

BSSimpleList<BGSHungerStage*>* TESDataHandler::GetHungerStageList() {
    return &listHunger;
}

BSSimpleList<BGSSleepDeprevationStage*>* TESDataHandler::GetSleepDeprevationStageList() {
    return &listSleepDeprevation;
}

BSSimpleList<BGSDebris*>* TESDataHandler::GetDebrisList() {
    return &listDebris;
}

BSSimpleList<BGSPerk*>* TESDataHandler::GetPerkList() {
    return &listPerks;
}

BSSimpleList<BGSBodyPartData*>* TESDataHandler::GetPartDataList() {
    return &listPartData;
}

BSSimpleList<BGSNote*>* TESDataHandler::GetNoteList() {
    return &listNotes;
}

BSSimpleList<BGSListForm*>* TESDataHandler::GetListFormList() {
    return &listListForms;
}

BSSimpleList<BGSMenuIcon*>* TESDataHandler::GetMenuIconList() {
    return &listMenuIcons;
}

BSSimpleList<TESObjectANIO*>* TESDataHandler::GetAnimObjectList() {
    return &animObjects;
}

BSSimpleList<BGSMessage*>* TESDataHandler::GetMessageList() {
    return &listMessages;
}

BSSimpleList<BGSLightingTemplate*>* TESDataHandler::GetLightingTemplateList() {
    return &listLightingTemplates;
}

BSSimpleList<BGSMusicType*>* TESDataHandler::GetMusicTypeList() {
    return &listMusicTypes;
}

BSSimpleList<TESLoadScreenType*>* TESDataHandler::GetLoadScreenTypeList() {
    return &listLoadScreenTypes;
}

BSSimpleList<MediaSet*>* TESDataHandler::GetMediaSetList() {
    return &listMediaSets;
}

BSSimpleList<MediaLocationController*>* TESDataHandler::GetMediaLocationControllerList() {
    return &listMediaLocationControllers;
}

TESDataHandler::FormList* TESDataHandler::GetPackageList() {
    return &listPackages;
}

TESDataHandler::FormList* TESDataHandler::GetImageSpaceModifierList() {
    return &listImageSpaceModifiers;
}

TESDataHandler::FormList* TESDataHandler::GetWeatherList() {
    return &listWeather;
}

TESDataHandler::FormList* TESDataHandler::GetEnchantmentItemList() {
    return &listEnchantmentItems;
}

TESDataHandler::FormList* TESDataHandler::GetSpellItemList() {
    return &listSpellItems;
}

TESDataHandler::FormList* TESDataHandler::GetZoneList() {
    return &listZones;
}

TESDataHandler::FormList* TESDataHandler::GetRecipeCategoryList() {
    return &listRecipeCategories;
}

TESDataHandler::FormList* TESDataHandler::GetCaravanDeckList() {
    return &listCaravanDecks;
}

BSSimpleList<BGSExplosion*>* TESDataHandler::GetExplosionList() {
    return &listExplosions;
}

BSSimpleList<TESFile*>* TESDataHandler::GetFileList() {
    return &listFiles;
}

TESRegionList* TESDataHandler::GetRegionList() const {
    return pRegionList;
}

TESFile* TESDataHandler::GetActiveFile() const {
    return pActiveFile;
}

uint32_t TESDataHandler::CountCompiled() const {
    return iNumCompile;
}

bool TESDataHandler::GetSaveLoadGame() const {
    return bSaveLoadGame;
}

bool TESDataHandler::GetSaveLoad() const {
    return bSaveLoad;
}

void TESDataHandler::UnloadCell(TESObjectCELL* apCell) {
    uint8_t isSleepingorResting;
    TESObjectLAND* land;
    TESWorldSpace* worldSpace;
    uint8_t v5;
    if ((apCell) && (!apCell->GetDelete())) {
        BSTaskThread::SetPauseSleepEnabled(true);
        v5 = 0;
        if (pSaveLoadGame->GetSaveGameLoading()) {
            v5 = (uint8_t)pSaveLoadGame->SetThreadAllowChanges(false);
        }
        apCell->Detach(true);
        (&ReferenceProcessLists)->RemoveTempEffectsForCell(apCell);
        apCell->SetState(TESObjectCELL::CS_UNLOADING);
        apCell->UnLoadAllTempData();
        apCell->ClearReference3D();
        while (apCell->GetLowestProcessMiddleLow()) {
            apCell->SetLowestProcessToMiddleLow(false);
        }
        if (!pDataHandler->GetClearingData()) {
            isSleepingorResting = (uint8_t)pPlayer->IsSleepingorResting();
            apCell->ChangeProcessLevel((uint8_t)(!isSleepingorResting));
        }
        if (!apCell->GetInterior()) {
            land = apCell->GetLand();
            land->UnLoadVertices();
        }
        apCell->Clear3D();
        apCell->SetAutoWaterLoaded(false);
        if (!apCell->GetInterior()) {
            worldSpace = apCell->GetWorldSpace();
            worldSpace->UnLoadCell(apCell);
        }
        if (pSaveLoadGame->GetSaveGameLoading()) {
            pSaveLoadGame->SetThreadAllowChanges(v5);
        }
        BSTaskThread::SetPauseSleepEnabled(false);
    }
}

void TESDataHandler::SetSaveLoad(bool abNewbool) {
    bSaveLoad = abNewbool;
}

bool TESDataHandler::GetClearingData() const {
    return bClearingData;
}

bool TESDataHandler::GetCheckingModels() const {
    return bCheckingModels;
}

bool TESDataHandler::GetLoadingFiles() const {
    return bLoadingFiles;
}

void TESDataHandler::SetLoadingFiles(bool abNewbool) {
    bLoadingFiles = abNewbool;
}

void TESDataHandler::SetSpotterShader(TESForm* apForm) {
    pSpotterShader = static_cast<TESEffectShader*>(apForm);
}

void TESDataHandler::SetItemDetectedShader(TESForm* apForm) {
    pItemDetectedShader = static_cast<TESEffectShader*>(apForm);
}

void TESDataHandler::SetCateyeShader(TESForm* apForm) {
    pCateyeMobileShader = static_cast<TESEffectShader*>(apForm);
}

int32_t TESDataHandler::GetFlag(uint8_t aucFlag) const {
    return (ucDLCFlags & aucFlag) != 0;
}

void TESDataHandler::SetDLCFlags(uint8_t aucFlags, bool abSet) {
    if (abSet) {
        ucDLCFlags |= aucFlags;
    }
    else {
        ucDLCFlags &= ~aucFlags;
    }
}

int32_t TESDataHandler::GetDLCIndex(const char* apFileName) const {
    if (BSstrlen(apFileName) < 2) {
        return 0;
    }

    switch (*apFileName) {
        case 'D': return BSstrnicmp(apFileName, "DeadMoney", 9) == 0 ? 1 : 0;
        case 'H': return BSstrnicmp(apFileName, "HonestHearts", 12) == 0 ? 2 : 0;
        case 'L': return BSstrnicmp(apFileName, "LonesomeRoad", 12) == 0 ? 4 : 0;
        case 'O': return BSstrnicmp(apFileName, "OldWorldBlues", 12) == 0 ? 3 : 0;
        default: return 0;
    }
}

bool TESDataHandler::IsFormIDCreated(uint32_t aiFormID) const {
    return aiFormID >= 0xFF000000;
}

// The cell is apInterior, or with apWorld the exterior cell at aLocation (cell coordinates: the position >> 12).
TESObjectREFR* TESDataHandler::CreateProjectileAtLocation(
    TESBoundObject* apObject,
    const NiPoint3& aLocation,
    const NiPoint3& aDirection,
    TESObjectCELL* apInterior,
    TESWorldSpace* apWorld
) {
    Projectile* pProjectile = nullptr;
    if (apObject && apObject->GetFormType() == PROJ_ID) {
        TESObjectCELL* pCell = nullptr;
        if (apWorld) {
            int32_t iX = FloatToInt(aLocation.x) >> 12;
            int32_t iY = FloatToInt(aLocation.y) >> 12;
            pCell = apWorld->GetCellFromCellCoord(iX, iY);
            if (!pCell) {
                Warning("CELLS: Trying to create an explosion in an unloaded cell: %i, %i.", iX, iY);
            }
        }
        else {
            pCell = apInterior;
        }
        if (pCell) {
            // the form type test above leaves a BGSProjectile
            pProjectile = Projectile::Launch(
                static_cast<BGSProjectile*>(apObject), nullptr, nullptr, nullptr, aLocation, aDirection.z, aDirection.x,
                nullptr, nullptr, false, false, 0.0f, 0.0f, pCell
            );
        }
    }
    return pProjectile;
}

void TESDataHandler::AdvanceNextID() {
    ++iNextID;

    // past the last created ID of this load order index: wrap to the first one
    // after the reserved IDs
    if ((iNextID & 0xFFFFFF) >= 0x7FFFFF) {
        iNextID = (iNextID & 0xFF000000) + 0x800;
    }
}

uint32_t TESDataHandler::GetAddonNodeCount() {
    return arrayAddonNodes.GetSize();
}

BGSAddonNode* TESDataHandler::GetAddonNode(uint32_t aiIndex) {
    if (aiIndex < arrayAddonNodes.GetSize()) {
        return arrayAddonNodes[aiIndex];
    }

    return nullptr;
}

// Takes the file list before it tests the name.
TESFile* TESDataHandler::GetListFile(const char* apFileName) {
    BSSimpleList<TESFile*>* node = GetFileList();
    if (apFileName) {
        for (; node && node->GetItem(); node = node->GetNext()) {
            TESFile* file = node->GetItem();

            if (BSstricmp(apFileName, file->GetFilename()) == 0) {
                return file;
            }
        }
    }

    return nullptr;
}

void TESDataHandler::CloseAllTES() {
    for (BSSimpleList<TESFile*>* node = GetFileList(); node && node->m_item; node = node->m_pkNext) {
        node->m_item->CloseTES();
    }
}

void TESDataHandler::SetMasterFileLargeBuffer(int32_t aiLargeBuffer) {
    for (BSSimpleList<TESFile*>* node = GetFileList(); node && node->m_item; node = node->m_pkNext) {
        TESFile* file = node->m_item;

        if (file->GetMaster()) {
            file->ChangeBufferSize(aiLargeBuffer);
        }
    }
}

// Saves pForm to the active file when that file owns it and is not a master, or the form was altered: a master form
// saves as an edit, a deleted form is skipped. The master argument is ignored, as the game does.
bool TESDataHandler::SaveForm(TESForm* pForm, bool master) {
    if (!pActiveFile) {
        return false;
    }

    if (((pForm->GetFile(-1) == pActiveFile) && !pActiveFile->GetMaster()) || pForm->GetAltered()) {
        if (pForm->GetMaster()) {
            return pForm->SaveEdit(pActiveFile);
        }

        if (!pForm->GetDelete()) {
            return pForm->Save(pActiveFile);
        }
    }

    return false;
}

TESSound* TESDataHandler::GetSound(const char* apEditorID) {
    TESForm* form = TESForm::GetFormByEditorID(apEditorID);

    if (form && form->GetFormType() == SOUN_ID) {
        return static_cast<TESSound*>(form);
    }

    return nullptr;
}

bool TESDataHandler::IsFormIDInUse(uint32_t aiFormID) {
    if (TESForm::GetFormByNumericID(aiFormID)) {
        return true;
    }
    return pSaveLoadGame->IsFormIDInUse(aiFormID);
}

TESSound* TESDataHandler::GetSound(uint32_t aiSearchID) {
    return dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(aiSearchID));
}

TESFaction* TESDataHandler::GetFaction(uint32_t aiSearchID) {
    return dynamic_cast<TESFaction*>(TESForm::GetFormByNumericID(aiSearchID));
}

TESLoadScreenType* TESDataHandler::GetLoadScreenType(uint32_t aiSearchID) {
    return dynamic_cast<TESLoadScreenType*>(TESForm::GetFormByNumericID(aiSearchID));
}

void TESDataHandler::AddTopic(TESTopic* pNewTopic) {
    TESTopic::AddTopicToSortedList(pNewTopic, reinterpret_cast<BSSimpleList<TESTopic*>*>(&listTopics), true);
}

void TESDataHandler::AddTopicInfo(TESTopicInfo* pNewTopicInfo) {
    TESTopicInfo::AddTopicInfoToSortedList(
        pNewTopicInfo, reinterpret_cast<BSSimpleList<TESTopicInfo*>*>(&listTopicInfos)
    );
}

TESGlobal* TESDataHandler::GetGlobal(const char* apSearchID) {
    if (!listGlobals.IsEmpty()) {
        for (BSSimpleList<TESGlobal*>* node = &listGlobals; node != nullptr; node = node->m_pkNext) {
            TESGlobal* global = node->m_item;
            if (global) {
                const char* editorID = global->GetFormEditorID();
                if (BSstricmp(editorID, apSearchID) == 0) {
                    return global;
                }
            }
        }
    }
    return nullptr;
}

// Runs the scripts of every interior cell, then of every worldspace's persistent cell. The persistent cell is read
// through QPersistentCell, because TESWorldSpace::GetPersistentCell would create a missing cell.
void TESDataHandler::RunAllPersistentRefScripts() {
    const int32_t count = arrayInteriorCells.GetSize();
    for (int32_t i = 0; i < count; ++i) {
        TESObjectCELL* cell = arrayInteriorCells[i];
        if (cell) {
            cell->RunScripts(true, true);
        }
    }

    for (BSSimpleList<TESWorldSpace*>* node = GetWorldSpaceList(); node; node = node->m_pkNext) {
        TESWorldSpace* world = node->m_item;
        if (world) {
            TESObjectCELL* cell = world->QPersistentCell();
            if (cell) {
                cell->RunScripts(true, true);
            }
        }
    }
}

void TESDataHandler::FixAllScriptIDs() {
    for (BSSimpleList<Script*>* node = GetScriptList(); node != nullptr; node = node->m_pkNext) {
        TESForm* script = node->m_item;
        if (!script) {
            break;
        }
        script->InitItem();
    }
    RunAllPersistentRefScripts();
}

bool TESDataHandler::BuildFileList(const char* pdirectory) {
    // Constructs and destroys AutoMemContext on a stack context; the constructor's result is ignored.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_MASTER_FILE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        4501
    );
    char acDirectory[260];
    BSstrcpy(acDirectory, 260, pdirectory);

    // Drop the listed files whose header cannot be read.
    TESFile* pFile;
    BSSimpleList<TESFile*>* pNode = GetFileList();
    while (pNode) {
        pFile = pNode->m_item;
        if (pFile) {
            if (pFile->LoadTESInfo()) {
                pNode->RemoveHead();
            }
            else {
                pNode = pNode->m_pkNext;
            }
            pFile->CloseTES();
        }
        else {
            pNode = pNode->m_pkNext;
        }
    }

    // Add the masters, then the plugins, found in the directory and not listed
    // yet, in the directory search's order.
    char acSearch[260];
    for (uint32_t uiPass = 0; uiPass < 2; ++uiPass) {
        std::strcpy(acSearch, acDirectory);
        switch (uiPass) {
            case 0: std::strcat(acSearch, "*.esm"); break;
            case 1: std::strcat(acSearch, "*.esp"); break;
        }

        for (const platform::FileEntry& rFound : platform::FindFiles(acSearch)) {
            if (rFound.size == 0) {
                continue;
            }
            const char* pFoundName = rFound.name.c_str();
            pFile = GetListFile(pFoundName);
            if (pFile) {
                continue;
            }

            const char* pDirectory = pdirectory;
            void* pMemory = ourMalloc(sizeof(TESFile));
            pFile = pMemory ? ::new (pMemory) TESFile(pDirectory, pFoundName, NiFile::READ_ONLY) : nullptr;
            pFile->CloseTES();

            // A plugin's description comes from the first 256 bytes of its .NAM file,
            // which also checks it.
            char acFileName[260];
            char acExtension[16];
            BSsplitpath(pFoundName, nullptr, 0, nullptr, 0, acFileName, 260, acExtension, 16);
            char acNamePath[260];
            BSsprintf(acNamePath, 260, "%s%s.NAM", pDirectory, acFileName);
            alignas(BSFile) uint8_t aNameFileStorage[sizeof(BSFile)];
            BSFile* pNameFile = reinterpret_cast<BSFile*>(aNameFileStorage);
            ::new (aNameFileStorage) BSFile(acNamePath, NiFile::READ_ONLY, 0x100, true);
            // Calls Exist directly, not through the vtable.
            if (pNameFile->BSFile::Exist() && pNameFile->FileIsGood()) {
                char acDescription[260];
                uint32_t uiRead = pNameFile->NiBinaryStream::Read(acDescription, 0x100);
                acDescription[uiRead] = '\0';
                pFile->SetDescription(acDescription);
                pFile->SetChecked(true);
                pFile->SetMaybeShouldBeReloaded(true);
            }

            int32_t iDLCIndex = GetDLCIndex(acFileName);
            if (iDLCIndex) {
                SetDLCFlags(static_cast<uint8_t>(1 << (iDLCIndex - 1)), true);
            }

            // Insert before the first file of the same kind written no earlier (CompareFileTime >= 0 on the two
            // FILETIME values), or before the first plugin for a master; otherwise append.
            BSSimpleList<TESFile*>* pPrevious = nullptr;
            bool bAdded = false;
            pNode = GetFileList();
            while (pNode && pNode->GetItem()) {
                TESFile* pListed = pNode->GetItem();
                if ((pListed->GetMaster() && pFile->GetMaster()) || (!pListed->GetMaster() && !pFile->GetMaster())) {
                    const _FILETIME& rListedTime = pListed->GetFileData()->ftLastWriteTime;
                    const uint64_t uiListedTime =
                        (static_cast<uint64_t>(rListedTime.dwHighDateTime) << 32) | rListedTime.dwLowDateTime;
                    if (uiListedTime >= rFound.writeTime) {
                        bAdded = true;
                        pNode->AddHead(pFile);
                        break;
                    }
                }
                else if (pFile->GetMaster()) {
                    bAdded = true;
                    pNode->AddHead(pFile);
                    break;
                }
                pPrevious = pNode;
                pNode = pNode->GetNext();
            }
            if (!bAdded) {
                if (pPrevious) {
                    pPrevious->AddTail(pFile);
                }
                else {
                    GetFileList()->AddTail(pFile);
                }
            }
            pNameFile->~BSFile();
        }
    }

    // Drop and delete the listed files that no longer exist on disk.
    BSSimpleList<TESFile*>* pPrevious = nullptr;
    pNode = GetFileList();
    while (pNode && pNode->m_item) {
        pFile = pNode->m_item;
        std::strcpy(acSearch, pFile->GetPath());
        std::strcat(acSearch, pFile->GetFilename());
        if (platform::FindFiles(acSearch).empty()) {
            if (pNode->m_pkNext || !pPrevious) {
                pNode->RemoveHead();
            }
            else {
                pPrevious->Remove(pFile);
                pNode = pPrevious->m_pkNext;
            }
            if (pFile) {
                delete pFile;
            }
        }
        else {
            pPrevious = pNode;
            pNode = pNode->m_pkNext;
        }
    }

    char acUpdatePath[260];
    if (FileFinder::Locate("Update.bsa", acUpdatePath, FileFinder::SKIP_ARCHIVE, FileFinder::ARCHIVE_TYPE_ANY)) {
        ArchiveManager::OpenArchive("Update.bsa", 0, false);
    }

    // Move each master's own masters in front of it; a node that received one is
    // examined again.
    pNode = GetFileList();
    while (pNode && !pNode->IsEmpty()) {
        bool bMoved = false;
        TESFile* pMaster = pNode->m_item;
        if (pMaster->GetMaster()) {
            pMaster->GenIndexTable(GetFileList(), false);
            for (uint32_t i = 0; i < pMaster->GetIndexCount(); ++i) {
                TESFile* pIndexFile = pMaster->GetIndexFile(i + 1);
                if (pIndexFile && pNode->IsInList(pIndexFile)) {
                    pNode->Remove(pIndexFile);
                    pNode->AddHead(pIndexFile);
                    bMoved = true;
                }
            }
        }
        if (!bMoved) {
            pNode = pNode->m_pkNext;
        }
    }

    for (pNode = GetFileList(); pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        pNode->m_item->GenIndexTable(GetFileList(), false);
    }

    CloseAllTES();
    pContext->~AutoMemContext();
    return true;
}

void TESDataHandler::CreateThreadSafeFiles() {
    for (BSSimpleList<TESFile*>* pNode = GetFileList(); pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        TESFile* pFile = pNode->m_item;
        if (pFile->GetChecked()) {
            pFile->GetThreadSafeFile();
        }
    }
}

bool TESDataHandler::CompileFiles(bool abDownloadedContentOnly) {
    // The AutoMemContext constructor and destructor are called on a stack context, and the constructor's results are
    // ignored.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_MASTER_FILE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        5050
    );
    bSaveLoad = true;
    if (abDownloadedContentOnly) {
        iNumCompile = 0;
    }
    TES::pInstance->ClearAllCells(false, false);
    if (!abDownloadedContentOnly) {
        TES::pInstance->ClearWorldSpace();
    }
    TES::pInstance->SetInterior(nullptr);
    TES::pInstance->InitVars();
    TESForm::bFormIDsBashed = false;

    BSSimpleList<TESFile*>* pNode = GetFileList();
    TESFile* pFile = nullptr;
    if (bSaveLoadGame) {
        pActiveFile->LoadTESInfo();
        pActiveFile->GenIndexTable(GetFileList(), true);
    }

    // Read every checked file's header and check its masters too; a missing
    // master stops the load.
    while (pNode && pNode->m_item) {
        pFile = pNode->m_item;
        if (pFile->GetChecked()) {
            pFile->LoadTESInfo();
            pFile->GenIndexTable(GetFileList(), true);
            if (pFile->IsFileVersionTooHigh()) {
                pFile->SetChecked(false);
            }
            else {
                uint32_t uiIndexCount = pFile->GetIndexCount();
                for (uint32_t i = 0; i < uiIndexCount; ++i) {
                    TESFile* pMaster = pFile->GetIndexFile(i + 1);
                    bool bFound = false;
                    if (pMaster && !pMaster->IsFileVersionTooHigh()) {
                        pMaster->SetChecked(true);
                        bFound = true;
                    }
                    if (!bFound) {
                        if (pFile->GetIndexName(i + 1)) {
                            InitializationFlow("Unable to find masterfile: %s", pFile->GetIndexName(i + 1));
                        }
                        else {
                            InitializationFlow("Unable to find masterfile: <unknown>");
                        }
                        bSaveLoad = false;
                        pContext->~AutoMemContext();
                        return false;
                    }
                }
            }
        }
        pNode = pNode->m_pkNext;
    }

    // Number the checked files in load order (the active file last) and drop and
    // delete the unchecked ones.
    bool bMastersChanged = false;
    MessageHandler::IncDisableWarningCount(true);
    pNode = GetFileList();
    BSSimpleList<TESFile*>* pPrevious = nullptr;
    while (pNode && pNode->m_item) {
        pFile = pNode->m_item;
        bool bRemove = false;
        if (pFile->GetChecked()) {
            if (!pFile->GetMaster() && !bMasterSave && pFile->MastersChanged()) {
                bMastersChanged = true;
                const char* pName = pFile->GetFilename();
                Warning(sMasterMismatchWarning.String(), pName);
            }
            if (pFile->GetActive()) {
                pActiveFile = pFile;
                pNode = pNode->m_pkNext;
                continue;
            }
            pFileIndex[iNumCompile] = pFile;
            uint8_t ucCompileIndex = static_cast<uint8_t>(iNumCompile);
            ++iNumCompile;
            pFile->SetCompileIndex(ucCompileIndex);
            if (iNumCompile >= 255) {
                InitializationFlow("Too many selected files to compile!");
            }
        }
        else {
            bRemove = true;
        }

        if (bRemove) {
            if (pFile) {
                delete pFile;
            }
            if (pPrevious) {
                pPrevious->Remove(pNode->m_item);
                pNode = pPrevious->m_pkNext;
            }
            else {
                pNode->RemoveHead();
            }
        }
        else {
            pPrevious = pNode;
            pNode = pNode->m_pkNext;
        }
    }

    iNextID = 0xFF000800;
    if (bMastersChanged) {
        Warning(sGeneralMasterMismatchWarning.String());
    }
    MessageHandler::IncDisableWarningCount(false);
    if (pActiveFile) {
        pFileIndex[iNumCompile] = pActiveFile;
        uint8_t ucCompileIndex = static_cast<uint8_t>(iNumCompile);
        ++iNumCompile;
        pActiveFile->SetCompileIndex(ucCompileIndex);
        if (iNumCompile >= 255) {
            InitializationFlow("Too many selected files to compile!");
        }
    }

    iTotalForms = 0;
    for (uint32_t i = 0; i < iNumCompile; ++i) {
        if (!pFileIndex[i]->OpenTES(NiFile::READ_ONLY, false)) {
            DataError(DAT_ERROR_FILE_FAILURE);
        }
        else {
            iTotalForms += pFileIndex[i]->GetFormCount();
        }
    }

    // Open the archives named after each file that has masters
    // ("Data\<name>*.bsa").
    for (uint32_t i = 0; i < iNumCompile; ++i) {
        alignas(AutoMemContext) uint8_t aucArchiveContext[sizeof(AutoMemContext)];
        AutoMemContext* pArchiveContext = reinterpret_cast<AutoMemContext*>(aucArchiveContext);
        ::new (aucArchiveContext) AutoMemContext(
            MEM_CONTEXT_ARCHIVES, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
            5426
        );
        if (pFileIndex[i]->GetMasterList()->IsEmpty()) {
            pArchiveContext->~AutoMemContext();
            continue;
        }
        char acName[260];
        BSstrcpy(acName, 260, pFileIndex[i]->GetFilename());
        char* pExtension = std::strrchr(acName, '.');
        if (pExtension) {
            *pExtension = '\0';
        }
        char acSearch[260];
        BSsprintf(acSearch, 260, "Data\\%s*.bsa", acName);
        for (const platform::FileEntry& rFound : platform::FindFiles(acSearch)) {
            if (rFound.size != 0) {
                char acArchive[260];
                BSsprintf(acArchive, 260, "DATA\\%s", rFound.name.c_str());
                if (!ArchiveManager::GetArchive(acArchive)) {
                    ArchiveManager::OpenArchive(acArchive, 0, false);
                }
            }
        }
        pArchiveContext->~AutoMemContext();
    }

    SetLoadingFiles(true);
    if (!abDownloadedContentOnly) {
        GenerateDefaultObjects();
    }
    iFormsLoaded = 0;
    SetMasterFileLargeBuffer(-3);
    for (uint32_t i = 0; i < iNumCompile; ++i) {
        bool bNoExistingForms = false;
        if (i == 0) {
            bNoExistingForms = true;
        }
        // Downloaded content reloads only the open files that ask for it.
        if (abDownloadedContentOnly && (!pFileIndex[i]->m_pFile || !pFileIndex[i]->GetMaybeShouldBeReloaded())) {
            continue;
        }
        if (!ConstructObjectList(pFileIndex[i], bNoExistingForms)) {
            DataError(DAT_ERROR_UNKNOWN_FORM);
        }
    }
    pDataHandler->SetMasterFileLargeBuffer(-1);
    SetLoadingFiles(false);

    // Initialise the loaded forms, list by list; these lists stop at the first
    // empty item.
    for (FormList* pList = GetWeatherList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetCameraShotList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetRaceList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetHeadPartList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (FormList* pList = GetZoneList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetFactionList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetReputationList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetRecipeList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetChallengeList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (FormList* pList = GetRecipeCategoryList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetCasinoList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (FormList* pList = GetCaravanDeckList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetQuestList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetCombatStyleList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetEffectShaderList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetProjectileList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetNoteList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetExplosionList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetRadiationStageList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetDehydrationStageList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetHungerStageList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetSleepDeprevationStageList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetPerkList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetLoadScreenList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetLoadScreenTypeList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetAnimObjectList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetWaterList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetLandTextList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetListFormList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetPartDataList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetImpactDataList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetImpactDataSetList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }

    // Looks up the calendar's date and time globals again on the existing calendar.
    appCalendar.InitGlobals();
    for (auto* pList = GetTopicList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    TESTopic::SortQuestsOnAllTopics(nullptr);
    TESTopic::SortTopics(true, reinterpret_cast<BSSimpleList<TESTopic*>*>(pPlayer->GetTopicList()));

    for (TESObject* pObject = pObjectList->lnk_lpRoot; pObject; pObject = pObject->nod_lpNext) {
        pObject->InitItem();
    }
    // NPCs and creatures copy their template's data.
    for (TESObject* pObject = pObjectList->lnk_lpRoot; pObject; pObject = pObject->nod_lpNext) {
        int32_t iFormType = pObject->GetFormType();
        if (iFormType >= NPC__ID && iFormType <= CREA_ID) {
            TESActorBaseData* pBaseData = static_cast<TESActorBase*>(pObject);
            pBaseData->CopyFromTemplateForm(nullptr);
        }
    }

    // The region list's nodes are walked as region lists, deliberately, as the game does (the next node, less the
    // vtable).
    TESRegionList* pRegions = GetRegionList();
    while (pRegions && !pRegions->IsEmpty()) {
        pRegions->m_item->InitItem();
        pRegions = static_cast<TESRegionList*>(pRegions->m_pkNext);
    }
    // Walks the worldspace list once without using its items, as the game does.
    for (BSSimpleList<TESWorldSpace*>* pList = GetWorldSpaceList(); pList; pList = pList->m_pkNext) {}
    if (TES::pInstance->GetNavMeshInfoMap()) {
        TES::pInstance->GetNavMeshInfoMap()->InitItem();
    }
    int32_t iInteriorCount = arrayInteriorCells.GetSize();
    for (int32_t i = 0; i < iInteriorCount; ++i) {
        TESObjectCELL* pCell = arrayInteriorCells[i];
        if (pCell) {
            pCell->InitItem();
        }
    }
    for (BSSimpleList<TESWorldSpace*>* pList = GetWorldSpaceList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (auto* pList = GetMessageList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }

    BGSDefaultObjectManager::Instance()->InitItem();
    BGSDefaultObjectManager::Instance();
    TESForm* pEffect = BGSDefaultObjectManager::GetDefaultObject(DO_SPOTTER_EFFECT);
    if (pEffect) {
        pDataHandler->SetSpotterShader(pEffect);
    }
    BGSDefaultObjectManager::Instance();
    pEffect = BGSDefaultObjectManager::GetDefaultObject(DO_ITEM_DETECTION_EFFECT);
    if (pEffect) {
        pDataHandler->SetItemDetectedShader(pEffect);
    }
    BGSDefaultObjectManager::Instance();
    pEffect = BGSDefaultObjectManager::GetDefaultObject(DO_CATEYE_MOBILE_EFFECT);
    if (pEffect) {
        pDataHandler->SetCateyeShader(pEffect);
    }

    for (auto* pList = GetRagdollList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    for (auto* pList = GetLightingTemplateList(); pList && pList->m_item; pList = pList->m_pkNext) {
        static_cast<TESForm*>(pList->m_item)->InitItem();
    }
    pIdleManager->InitLoadedIdles();
    pCameraPathManager->InitLoadedPaths();
    EffectSettingCollection::InitItems();

    // These lists skip empty items.
    for (FormList* pList = GetEnchantmentItemList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (FormList* pList = GetSpellItemList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (auto* pList = GetAcousticSpaceList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (FormList* pList = GetImageSpaceModifierList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (auto* pList = GetMusicTypeList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (auto* pList = GetMediaSetList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (auto* pList = GetMediaLocationControllerList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }
    for (FormList* pList = GetPackageList(); pList; pList = pList->m_pkNext) {
        if (pList->m_item) {
            static_cast<TESForm*>(pList->m_item)->InitItem();
        }
    }

    EnsureFactionFriendshipsAreTwoWay();
    SortAllList();
    pLastLoadedCell = nullptr;
    pPlayer->GetCharacterProgressionInfo()->InitializeData();
    bSaveLoad = false;
    pContext->~AutoMemContext();
    return true;
}

void TESDataHandler::DataError(DAT_ERROR error) {
    if (error == DAT_ERROR_UNKNOWN_FORM) {
        const char* WarningFileName = MessageHandler::GetWarningFileName();
        Warning("DataHandler: unrecognized form\r\nLook in the %s file for more info.\r\n", WarningFileName);
    }
    else {
        Warning("DataHandler: internal error");
    }
}

// A reference outside any cell goes to the current interior, else to the exterior cell of the current worldspace at its
// position (y is read before x). A TESObject that has no list of its own goes in the object list; an addon node goes in
// both the addon array and the object list.
bool TESDataHandler::AddFormToDataHandler(TESForm* pNew) {
    if (!pNew) {
        return false;
    }

    switch (pNew->GetFormType()) {
        case MICN_ID: GetMenuIconList()->AddHead(static_cast<BGSMenuIcon*>(pNew)); return true;
        case GLOB_ID: GetGlobalList()->AddHead(static_cast<TESGlobal*>(pNew)); return true;
        case CLAS_ID: GetClassList()->AddHead(static_cast<TESClass*>(pNew)); return true;
        case FACT_ID: GetFactionList()->AddHead(static_cast<TESFaction*>(pNew)); return true;
        case HDPT_ID: GetHeadPartList()->AddHead(static_cast<BGSHeadPart*>(pNew)); return true;
        case HAIR_ID: GetHairList()->AddHead(static_cast<TESHair*>(pNew)); return true;
        case EYES_ID: listEyes.AddHead(static_cast<TESEyes*>(pNew)); return true;
        case RACE_ID: GetRaceList()->AddHead(static_cast<TESRace*>(pNew)); return true;
        case SOUN_ID: GetSoundList()->AddHead(static_cast<TESSound*>(pNew)); return true;
        case ASPC_ID: GetAcousticSpaceList()->AddHead(static_cast<BGSAcousticSpace*>(pNew)); return true;
        case MGEF_ID: {
            EffectSetting* setting = dynamic_cast<EffectSetting*>(pNew);
            if (setting) {
                EffectSettingCollection::AddEffect(setting);
            }
            return true;
        }
        case SCPT_ID: GetScriptList()->AddHead(static_cast<Script*>(pNew)); return true;
        case LTEX_ID: GetLandTextList()->AddHead(static_cast<TESLandTexture*>(pNew)); return true;
        case ENCH_ID: listEnchantmentItems.AddHead(pNew); return true;
        case SPEL_ID: listSpellItems.AddHead(pNew); return true;
        case NOTE_ID: GetNoteList()->AddHead(static_cast<BGSNote*>(pNew)); return true;
        case PROJ_ID: GetProjectileList()->AddHead(static_cast<BGSProjectile*>(pNew)); return true;
        case WTHR_ID: listWeather.AddHead(pNew); return true;
        case CLMT_ID: listClimates.AddHead(pNew); return true;
        case REGN_ID: {
            // TESForm is TESRegion's first base, so the pointer is the region's.
            TESRegion* region = reinterpret_cast<TESRegion*>(pNew);
            GetRegionList()->AddHead(region);
            return true;
        }
        case CELL_ID: {
            TESObjectCELL* cell = static_cast<TESObjectCELL*>(pNew);
            if (cell->GetInterior()) {
                arrayInteriorCells.SetAtGrow(arrayInteriorCells.GetSize(), cell);
            }
            return true;
        }
        case REFR_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case PCBE_ID: {
            TESObjectREFR* ref = dynamic_cast<TESObjectREFR*>(pNew);
            if (ref) {
                TESObjectCELL* cell = ref->GetParentCell();
                if (!cell) {
                    cell = TES::pInstance->GetInterior();
                    if (!cell) {
                        TESWorldSpace* world = TES::pInstance->GetWorldSpace();
                        const float y = ref->GetLocationOnReference().y;
                        const float x = ref->GetLocationOnReference().x;
                        cell = GetCellFromWorldCoord(x, y, world, true);
                    }
                }
                else if (!cell->GetInterior()) {
                    TESWorldSpace* world = TES::pInstance->GetWorldSpace();
                    const float y = ref->GetLocationOnReference().y;
                    const float x = ref->GetLocationOnReference().x;
                    cell = GetCellFromWorldCoord(x, y, world, true);
                }

                if (cell) {
                    cell->AddReference(ref, false);
                    if (ref->MustRefPersist()) {
                        ref->SetRefPersists(true);
                    }
                    if (ref->MustBeVisibleDistant()) {
                        ref->SetVisibleDistant(true);
                    }
                }
            }
            return true;
        }
        case WRLD_ID: GetWorldSpaceList()->AddHead(static_cast<TESWorldSpace*>(pNew)); return true;
        case QUST_ID: GetQuestList()->AddHead(static_cast<TESQuest*>(pNew)); return true;
        case PACK_ID: listPackages.AddHead(pNew); return true;
        case CSTY_ID: GetCombatStyleList()->AddHead(static_cast<TESCombatStyle*>(pNew)); return true;
        case LSCR_ID: GetLoadScreenList()->AddHead(static_cast<TESLoadScreen*>(pNew)); return true;
        case ANIO_ID: GetAnimObjectList()->AddHead(static_cast<TESObjectANIO*>(pNew)); return true;
        case WATR_ID: GetWaterList()->AddHead(static_cast<TESWaterForm*>(pNew)); return true;
        case EFSH_ID: GetEffectShaderList()->AddHead(static_cast<TESEffectShader*>(pNew)); return true;
        case EXPL_ID: listExplosions.AddHead(static_cast<BGSExplosion*>(pNew)); return true;
        case DEBR_ID: GetDebrisList()->AddHead(static_cast<BGSDebris*>(pNew)); return true;
        case IMGS_ID: listImageSpaces.AddHead(pNew); return true;
        case IMAD_ID: listImageSpaceModifiers.AddHead(pNew); return true;
        case FLST_ID: GetListFormList()->AddHead(static_cast<BGSListForm*>(pNew)); return true;
        case PERK_ID: GetPerkList()->AddHead(static_cast<BGSPerk*>(pNew)); return true;
        case BPTD_ID: GetPartDataList()->AddHead(static_cast<BGSBodyPartData*>(pNew)); return true;
        case RADS_ID: GetRadiationStageList()->AddHead(static_cast<BGSRadiationStage*>(pNew)); return true;
        case CAMS_ID: GetCameraShotList()->AddHead(static_cast<BGSCameraShot*>(pNew)); return true;
        case VTYP_ID: GetVoiceTypeList()->AddHead(static_cast<BGSVoiceType*>(pNew)); return true;
        case IPCT_ID: GetImpactDataList()->AddHead(static_cast<BGSImpactData*>(pNew)); return true;
        case IPDS_ID: GetImpactDataSetList()->AddHead(static_cast<BGSImpactDataSet*>(pNew)); return true;
        case ECZN_ID: listZones.AddHead(pNew); return true;
        case MESG_ID: GetMessageList()->AddHead(static_cast<BGSMessage*>(pNew)); return true;
        case RGDL_ID: GetRagdollList()->AddHead(static_cast<BGSRagdoll*>(pNew)); return true;
        case LGTM_ID: GetLightingTemplateList()->AddHead(static_cast<BGSLightingTemplate*>(pNew)); return true;
        case MUSC_ID: GetMusicTypeList()->AddHead(static_cast<BGSMusicType*>(pNew)); return true;
        case REPU_ID: GetReputationList()->AddHead(static_cast<TESReputation*>(pNew)); return true;
        case RCPE_ID: GetRecipeList()->AddHead(static_cast<TESRecipe*>(pNew)); return true;
        case RCCT_ID: listRecipeCategories.AddHead(pNew); return true;
        case CSNO_ID: GetCasinoList()->AddHead(static_cast<TESCasino*>(pNew)); return true;
        case LSCT_ID: GetLoadScreenTypeList()->AddHead(static_cast<TESLoadScreenType*>(pNew)); return true;
        case MSET_ID: GetMediaSetList()->AddHead(static_cast<MediaSet*>(pNew)); return true;
        case ALOC_ID:
            GetMediaLocationControllerList()->AddHead(static_cast<MediaLocationController*>(pNew));
            return true;
        case CHAL_ID: GetChallengeList()->AddHead(static_cast<TESChallenge*>(pNew)); return true;
        case AMEF_ID: listAmmoEffects.AddHead(pNew); return true;
        case CDCK_ID: listCaravanDecks.AddHead(pNew); return true;
        case DEHY_ID: GetDehydrationStageList()->AddHead(static_cast<BGSDehydrationStage*>(pNew)); return true;
        case HUNG_ID: GetHungerStageList()->AddHead(static_cast<BGSHungerStage*>(pNew)); return true;
        case SLPD_ID:
            GetSleepDeprevationStageList()->AddHead(static_cast<BGSSleepDeprevationStage*>(pNew));
            return true;
        case ADDN_ID: AddAddonNode(static_cast<BGSAddonNode*>(pNew)); [[fallthrough]];
        default: break;
    }

    TESObject* object = dynamic_cast<TESObject*>(pNew);
    if (object) {
        pObjectList->lnk_Append(object);
        return true;
    }

    Warning(
        "FORMS: Unknown form type '%s' encountered in AddFormToDataHandler.",
        TESForm::GetFormTypeString(pNew->GetFormType())
    );
    return false;
}

// The loop steps through the base objects with TESObject's next link.
void TESDataHandler::UpdateAutoCalcNPCs() {
    for (TESObject* object = pObjectList->lnk_lpRoot; object; object = object->nod_lpNext) {
        TESNPC* npc = dynamic_cast<TESNPC*>(object);
        if (npc && npc->GetMatchesPCLevel()) {
            npc->InitValues(false);
        }
    }
}

// The editor IDs and form IDs of the warning are read in the game's order (the arguments right to left).
void TESDataHandler::AddAddonNode(BGSAddonNode* apAddonNode) {
    if (!apAddonNode) {
        return;
    }

    uint32_t index = apAddonNode->GetIndex();
    if (index < arrayAddonNodes.GetSize()) {
        BGSAddonNode* existing = arrayAddonNodes[index];
        if (existing && existing != apAddonNode) {
            const char* newEditorID = apAddonNode->GetFormEditorID();
            const uint32_t newFormID = apAddonNode->iFormID;
            const char* existingEditorID = existing->GetFormEditorID();
            const uint32_t existingFormID = existing->iFormID;
            const char* editorID = apAddonNode->GetFormEditorID();
            const uint32_t formID = apAddonNode->iFormID;
            Warning(
                "FORMS: Addon Node %08X \"%s\" is trying to use index %i, but "
                "Addon Node %08X \"%s\" already uses "
                "that index.  Addon Node %08X \"%s\" will be remapped to a new "
                "index.",
                formID, editorID, index, existingFormID, existingEditorID, newFormID, newEditorID
            );
            index = 0xFFFFFFFF;
        }
    }

    if (index == 0xFFFFFFFF) {
        index = arrayAddonNodes.AddFirstEmpty(apAddonNode);
        apAddonNode->SetIndex(index);
    }
    else {
        arrayAddonNodes.SetAtGrow(apAddonNode->GetIndex(), apAddonNode);
    }
}

void TESDataHandler::RemoveAddonNode(BGSAddonNode* apAddonNode) {
    if (!apAddonNode) {
        return;
    }
    arrayAddonNodes.RemoveAt(apAddonNode->GetIndex());
}

TESObjectCELL* TESDataHandler::GetInteriorCellByIndex(uint32_t aiIndex) {
    arrayInteriorCells.Compact();
    return arrayInteriorCells[aiIndex];
}

void TESDataHandler::PrintTextureInfo(
    const NiTexturingProperty::Map* pmap,
    NiTriBasedGeom* apGeometry,
    const char* apHeader,
    const char* apModel,
    bool abHasShader
) {
    if (pmap) {
        // The result of the DynamicCast is deliberately dropped, as the game does.
        NiObject::DynamicCastStatic(&NiSourceTexture::ms_RTTI, pmap->GetTexture());
    }
}

// The interior cells are not null-checked; a worldspace slot without an item is
// skipped.
TESObjectCELL* TESDataHandler::GetCellByEditorID(const char* apEditorID) {
    if (!apEditorID) {
        return nullptr;
    }

    const int32_t count = arrayInteriorCells.GetSize();
    for (int32_t i = 0; i < count; ++i) {
        TESObjectCELL* cell = arrayInteriorCells[i];
        if (BSstricmp(cell->GetFormEditorID(), apEditorID) == 0) {
            return cell;
        }
    }

    for (BSSimpleList<TESWorldSpace*>* node = GetWorldSpaceList(); node; node = node->m_pkNext) {
        TESWorldSpace* world = node->m_item;
        if (world) {
            TESObjectCELL* cell = world->GetCellByEditorID(apEditorID);
            if (cell) {
                return cell;
            }
        }
    }

    return nullptr;
}

TESObjectCELL* TESDataHandler::NewCell(const char* apName, int32_t aiX, int32_t aiY, TESWorldSpace* apWorldSpace) {
    TESObjectCELL* pCell_2;
    if (!apWorldSpace) {
        return nullptr;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_CELLS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 2013
    );
    TESObjectCELL* pCell = static_cast<TESObjectCELL*>(ourMalloc(sizeof(TESObjectCELL)));
    if (pCell) {
        pCell_2 = ::new (pCell) TESObjectCELL;
    }
    else {
        pCell_2 = nullptr;
    }
    TESObjectCELL* pCell_3 = pCell_2;
    uint32_t CastingType = apWorldSpace->iFormID;
    uint32_t icellID = pSaveLoadGameOLD->GetCreatedExteriorCellFormID(CastingType, aiX, aiY);
    if (icellID) {
        pCell_3->SetFormID(icellID, true);
    }
    if (apName) {
        pCell_3->SetFormEditorID(apName);
    }
    uint8_t v13 = 0;
    pCell_3->SetInterior(false);
    v13 |= 2u;
    pCell_3->cCellFlags = v13;
    pCell_3->CreateCellData();
    pCell_3->SetDataCoord(aiX, aiY);
    if (apWorldSpace->AddCell(pCell_3)) {
        pCell_3->InitItem();
    }
    else {
        if (pCell_3) {
            delete pCell_3;
        }
        pCell_3 = nullptr;
    }
    return pCell_3;
}

TESWorldSpace*
TESDataHandler::GetExtCellDataFromFileByEditorID(const char* apCellID, int32_t& aiOutX, int32_t& aiOutY) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_MASTER_FILE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        4161
    );
    TESWorldSpace* pcellworld = nullptr;
    aiOutX = 0;
    aiOutY = 0;
    if (apCellID && *apCellID) {
        bool boptimized = false;
        for (int32_t i = 0; i < iNumCompile; ++i) {
            if (pFileIndex[i] && pFileIndex[i]->GetOptimizedFile()) {
                boptimized = true;
                break;
            }
        }
        if (boptimized) {
            BSSimpleList<TESWorldSpace*>* pworldlist = pDataHandler->GetWorldSpaceList();
            while (pworldlist && !pworldlist->IsEmpty()) {
                TESWorldSpace* pworld = pworldlist->m_item;
                pworldlist = pworldlist->m_pkNext;
                if (pworld->GetExtCellDataFromFileByEditorID(apCellID, aiOutX, aiOutY)) {
                    pcellworld = pworld;
                    break;
                }
            }
        }
        else {
            for (int32_t i_2 = 0; i_2 < iNumCompile; ++i_2) {
                TESFile* pfile = pFileIndex[i_2];
                if (pfile && pfile->OpenTES(NiFile::READ_ONLY, false)) {
                    FORM* pFORM = pfile->GetFormData();
                    bool bfound = false;
                    while (pFORM && !bfound) {
                        bool badvanced = false;
                        if (pFORM->form == formEnumString[GRUP_ID].uiFormString) {
                            if (pFORM->iFormID || pFORM->flags != formEnumString[WRLD_ID].uiFormString) {
                                badvanced = pfile->NextGroup();
                            }
                            else {
                                bfound = true;
                                badvanced = pfile->NextForm(true);
                            }
                        }
                        else {
                            badvanced = pfile->NextForm(true);
                        }
                        if (badvanced) {
                            pFORM = pfile->GetFormData();
                        }
                        else {
                            pFORM = nullptr;
                        }
                    }
                    if (bfound) {
                        pFORM = pfile->GetFormData();
                        bfound = false;
                        while (pFORM && !bfound) {
                            bool badvanced_2 = false;
                            if (pFORM->form == formEnumString[65].uiFormString) {
                                uint32_t iformID = pFORM->iFormID;
                                uint32_t ifileindex = static_cast<uint8_t>(iformID >> 24);
                                TESFile* pformfile = pfile->GetIndexFile(static_cast<uint8_t>(iformID >> 24) + 1);
                                if (!pformfile) {
                                    pformfile = pfile;
                                }
                                iformID &= 0xFFFFFFu;
                                uint8_t CompileIndex = pformfile->GetCompileIndex();
                                iformID |= CompileIndex << 24;
                                TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
                                pcellworld = dynamic_cast<TESWorldSpace*>(FormByNumericID);
                                badvanced_2 = pfile->NextForm(true);
                            }
                            else {
                                if (pFORM->form == formEnumString[CELL_ID].uiFormString) {
                                    CHUNK_ID chunk;
                                    chunk = pfile->GetTESChunk();
                                    if (chunk == EDID_ID) {
                                        int32_t isize = pfile->GetChunkSize() + 1;
                                        char* pfileID = static_cast<char*>(ourMalloc(static_cast<uint32_t>(isize)));
                                        ourMemset(pfileID, 0, static_cast<uint32_t>(isize));
                                        pfile->GetChunkData(pfileID, 0);
                                        if (!BSstricmp(pfileID, apCellID)) {
                                            bfound = true;
                                            while (chunk) {
                                                if (chunk == XCLC_ID) {
                                                    EXTERIOR_DATA data;
                                                    ourMemset(&data, 0, sizeof(data));
                                                    pfile->GetChunkData(&data, 12u);
                                                    if (pfile->QEndian()) {
                                                        data.Endian();
                                                    }
                                                    aiOutX = data.iCellX;
                                                    aiOutY = data.iCellY;
                                                    chunk = NO_CHUNK;
                                                }
                                                else {
                                                    if (pfile->NextChunk()) {
                                                        chunk = pfile->GetTESChunk();
                                                    }
                                                    else {
                                                        chunk = NO_CHUNK;
                                                    }
                                                }
                                            }
                                        }
                                        ourFree(pfileID);
                                    }
                                LABEL_55:;
                                    badvanced_2 = pfile->NextForm(true);
                                }
                                else {
                                    if (pFORM->form == formEnumString[2].uiFormString) {
                                        uint32_t uiRecordType = pFORM->form;
                                        switch (pFORM->iFormID) {
                                            case 0u: {
                                                if (pFORM->flags == formEnumString[WRLD_ID].uiFormString) {
                                                    goto LABEL_55;
                                                }
                                                if (static_cast<uint8_t>(
                                                        TESForm::GetFormTypeFromFormString(pFORM->flags)
                                                    ) > WRLD_ID) {
                                                    badvanced_2 = false;
                                                }
                                                break;
                                            }
                                            case 1u:
                                            case 4u:
                                            case 5u: {
                                                goto LABEL_55;
                                            }
                                            case 6u: {
                                                badvanced_2 = pfile->NextGroup();
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if (badvanced_2) {
                                pFORM = pfile->GetFormData();
                            }
                            else {
                                pFORM = nullptr;
                            }
                        }
                    }
                    pfile->CloseTES();
                    if (bfound) {
                        break;
                    }
                    pcellworld = nullptr;
                }
            }
        }
    }
    return pcellworld;
}

// The group header's label (flags) is the record type of a top group; its
// iFormID is the group type (4 and 5 are exterior cell blocks, 9 a cell's
// temporary children). Records of a plugin that is not a master always load
// unless a save is being loaded.
bool TESDataHandler::FormGroupLoadsInGame(TESFile* apFile, FORM* apFormGroup) {
    bool result = true;

    if ((apFile && !apFile->GetMaster()) && (!pSaveLoadGameOLD || !pSaveLoadGameOLD->GetLoading())) {
        return true;
    }

    if (!apFormGroup) {
        return result;
    }

    const uint32_t groupType = apFormGroup->iFormID;
    if (groupType >= 4 && (groupType <= 5 || groupType == 9)) {
        result = false;
    }

    if (result && pSaveLoadGameOLD && pSaveLoadGameOLD->GetLoading()) {
        switch (TESForm::GetFormTypeFromFormString(apFormGroup->flags)) {
            case GMST_ID:
            case TXST_ID:
            case MICN_ID:
            case CLAS_ID:
            case HDPT_ID:
            case HAIR_ID:
            case EYES_ID:
            case RACE_ID:
            case SOUN_ID:
            case ASPC_ID:
            case SKIL_ID:
            case MGEF_ID:
            case SCPT_ID:
            case LTEX_ID:
            case ENCH_ID:
            case SPEL_ID:
            case ACTI_ID:
            case TACT_ID:
            case TERM_ID:
            case CONT_ID:
            case DOOR_ID:
            case INGR_ID:
            case STAT_ID:
            case SCOL_ID:
            case PWAT_ID:
            case GRAS_ID:
            case TREE_ID:
            case FLOR_ID:
            case FURN_ID:
            case LVLC_ID:
            case LVLN_ID:
            case ALCH_ID:
            case IDLM_ID:
            case NOTE_ID:
            case PROJ_ID:
            case LVLI_ID:
            case WTHR_ID:
            case CLMT_ID:
            case REGN_ID:
            case DIAL_ID:
            case INFO_ID:
            case IDLE_ID:
            case PACK_ID:
            case CSTY_ID:
            case LSCR_ID:
            case LVSP_ID:
            case ANIO_ID:
            case WATR_ID:
            case EFSH_ID:
            case TOFT_ID:
            case EXPL_ID:
            case DEBR_ID:
            case IMGS_ID:
            case IMAD_ID:
            case FLST_ID:
            case PERK_ID:
            case BPTD_ID:
            case ADDN_ID:
            case CAMS_ID:
            case CPTH_ID:
            case ECZN_ID:
            case MUSC_ID:
            case MSET_ID:
            case ALOC_ID: result = false; break;
            default: break;
        }
    }

    return result;
}

uint32_t TESDataHandler::GetNextID() {
    GetNextIDLock.Lock("TESDataHandler::GetNextID");
    while (IsFormIDInUse(iNextID)) {
        AdvanceNextID();
    }
    const uint32_t id = iNextID;
    AdvanceNextID();
    GetNextIDLock.Unlock();
    return id;
}

void TESDataHandler::EnumReferencesCloseToPoint(
    TESObjectCELL* apCell,
    const NiPoint3& aPoint1,
    float afRadius1,
    const NiPoint3& aPoint2,
    float afRadius2,
    bool (*apfnEnumFunc)(TESObjectREFR*, void*),
    void* aiEnumData
) {
    if (!apCell || !apfnEnumFunc) {
        return;
    }

    if (apCell->GetInterior()) {
        apCell->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc, aiEnumData);
    }
    else {
        TESWorldSpace* world = apCell->GetWorldSpace();
        if (world) {
            world->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc, aiEnumData);
        }
    }
}

// The list ends at the first empty slot.
TESObjectANIO* TESDataHandler::GetAnimObject(TESIdleForm* apIdle, int32_t aiIndex) {
    for (BSSimpleList<TESObjectANIO*>* node = GetAnimObjectList(); node; node = node->m_pkNext) {
        TESObjectANIO* animObject = node->m_item;
        if (!animObject) {
            break;
        }
        if (animObject->GetUsesIdle(apIdle)) {
            if (aiIndex == 0) {
                return animObject;
            }
            --aiIndex;
        }
    }
    return nullptr;
}

TESObjectCELL*
TESDataHandler::GetCellFromCellCoord(int32_t aiX, int32_t aiY, TESWorldSpace* apWorldSpace, bool abCreateCell) {
    if (!apWorldSpace) {
        apWorldSpace = GetWorldSpaceList()->m_item;
        if (!apWorldSpace) {
            return nullptr;
        }
    }
    if (aiX <= 0x7FFF && (aiY <= 0x7FFF && (aiX >= -0x8000 && aiY >= -0x8000))) {
        TESObjectCELL* pCell = apWorldSpace->GetCellFromCellCoord(aiX, aiY);
        if (pCell) {
            return pCell;
        }

        if (GetSaveLoad() || !abCreateCell) {
            return nullptr;
        }

        return NewCell("", aiX, aiY, apWorldSpace);
    }

    Warning(
        "CELLS: Trying to get exterior cell for invalid cell coordinate. Values must be between %i and %i.", -0x8000,
        0x7FFF
    );
    return nullptr;
}

TESObjectCELL*
TESDataHandler::GetCellFromWorldCoord(float afX, float afY, TESWorldSpace* apWorldSpace, bool abCreateCell) {
    int32_t v8 = FloatToInt(afX) >> 12;
    int32_t v6 = FloatToInt(afY) >> 12;
    return GetCellFromCellCoord(v8, v6, apWorldSpace, abCreateCell);
}

BSSimpleList<TESWorldSpace*>* TESDataHandler::GetWorldSpaceList() {
    return &listWorldSpaces;
}

// Reads every record of pFile: groups that do not load in game are skipped, the
// TOFT records carry the file offsets of the last loaded topic info, worldspace
// or interior cell, and every other record becomes a form, which a save load
// queues for initialisation and a normal load files in the editor-ID map. The
// progress text is formatted whenever the percentage changes, and then dropped.
bool TESDataHandler::ConstructObjectList(TESFile* pFile, bool abNoExistingForms) {
    if (!pFile->OpenTES(NiFile::READ_ONLY, false)) {
        DataError(DAT_ERROR_FILE_FAILURE);
    }

    bool bSuccess = true;
    int32_t iCurrPercent = 0;
    pLastLoadedCell = nullptr;
    pLastLoadedWorld = nullptr;
    pLastLoadedInfo = nullptr;
    bool bForm = true;
    ucGameSettingsLoadState = 0;
    do {
        bool bNextForm = true;
        bForm = true;
        if (pFile->GetTESForm() == GRUP_ID) {
            if (!FormGroupLoadsInGame(pFile, pFile->GetFormData())) {
                bNextForm = false;
                if (!pFile->NextGroup()) {
                    bForm = false;
                }
            }
        }
        else if (pFile->GetTESForm() == TOFT_ID) {
            const uint32_t uiOffset = pFile->GetFormData()->iFormID;
            if (uiOffset == 0xFFFFFFFF) {
                bNextForm = false;
                bForm = false;
            }
            else if (uiOffset == 0xFFFFFFFE) {
                const uint32_t uiDataSize = pFile->GetFormData()->length;
                const uint32_t uiDataStart = pFile->GetOffset() + sizeof(FORM);
                pFile->AddAuxBuffer(uiDataStart, uiDataSize);
                bNextForm = false;
                bForm = false;
            }
            else if (pLastLoadedInfo) {
                pLastLoadedInfo->SetTempDataOffset(uiOffset);
            }
            else if (pLastLoadedWorld) {
                const int32_t iDelta = uiOffset - pFile->GetOffset() + sizeof(FORM);
                pLastLoadedWorld->AdjustCellOffsets(pFile, iDelta);
            }
            else if (pLastLoadedCell && pLastLoadedCell->GetInterior() && pFile->GetMasterList()->IsEmpty()) {
                pLastLoadedCell->SetTempDataOffset(uiOffset);
            }
        }
        else if (!ConstructObject(pFile, abNoExistingForms, nullptr, false)) {
            bSuccess = false;
        }
        else if (pSaveLoadGameOLD && pSaveLoadGameOLD->GetLoading()) {
            if ((++iSaveLoadFormCount & 0xFF) == 0) {
                pSaveLoadGameOLD->StartLoadingSequence();
            }
            TESForm* pForm = TESForm::GetFormByNumericID(pFile->GetID());
            if (pForm) {
                pSaveLoadGameOLD->AddInitItem(pForm);
            }
        }
        else {
            TESForm* pForm = TESForm::GetFormByNumericID(pFile->GetID());
            if (pForm) {
                // NPCs, creatures, references, actors, placed projectiles and PCBE stay out of the editor-ID map.
                switch (pForm->GetFormType()) {
                    case NPC__ID:
                    case CREA_ID:
                    case REFR_ID:
                    case ACHR_ID:
                    case ACRE_ID:
                    case PMIS_ID:
                    case PGRE_ID:
                    case PBEA_ID:
                    case PFLA_ID:
                    case PCBE_ID: break;
                    default: {
                        const char* pEditorID = pForm->GetFormEditorID();
                        if (pEditorID && *pEditorID) {
                            TESForm::pAllFormsByEditorID->SetAt(pEditorID, pForm);
                        }
                        break;
                    }
                }
            }
        }

        ++iFormsLoaded;
        const int32_t iPercent = static_cast<int32_t>(
            static_cast<double>(static_cast<uint64_t>(iFormsLoaded)) /
            static_cast<double>(static_cast<uint64_t>(iTotalForms)) * 100.0
        );
        if (iPercent != iCurrPercent) {
            iCurrPercent = iPercent;
            char acProgress[260];
            BSsprintf(acProgress, 260, "Loading Files %d%% (%s)", iCurrPercent, pFile->GetFilename());
        }

        if (bNextForm) {
            bForm = pFile->NextForm(true);
        }
    }
    while (bForm);

    if (!pFile->CloseTES()) {
        DataError(DAT_ERROR_FILE_FAILURE);
    }
    return bSuccess;
}

void TESDataHandler::SortAllList() {
    BSSimpleList<TESClass*>* pClassList = GetClassList();
    int32_t count = pClassList->ItemsInList();
    for (int32_t n = 0; n < count - 1; ++n) {
        for (BSSimpleList<TESClass*>* pList = pClassList; pList && pList->m_item; pList = pList->m_pkNext) {
            if (pList->m_pkNext) {
                TESClass* class1 = pList->m_item;
                TESClass* class2 = pList->m_pkNext->m_item;
                if ((class1) && (class2)) {
                    const char* FullName = class2->GetFullName();
                    if (_tcsicmp(class1->GetFullName(), FullName) > 0) {
                        pList->SetItem(class2);
                        pList->m_pkNext->SetItem(class1);
                    }
                }
            }
        }
    }
    BSSimpleList<TESRace*>* pracelist = &listRaces;
    count = pracelist->ItemsInList();
    for (int32_t n_2 = 0; n_2 < count - 1; ++n_2) {
        for (BSSimpleList<TESRace*>* pList_2 = pracelist; pList_2 && pList_2->m_item; pList_2 = pList_2->m_pkNext) {
            if (pList_2->m_pkNext) {
                TESRace* race1 = pList_2->m_item;
                TESRace* race2 = pList_2->m_pkNext->m_item;
                if ((race1) && (race2)) {
                    const char* v2 = race2->GetFullName();
                    if (_tcsicmp(race1->GetFullName(), v2) > 0) {
                        pList_2->SetItem(race2);
                        pList_2->m_pkNext->SetItem(race1);
                    }
                }
            }
        }
    }
}

int32_t TESDataHandler::GetNumDLCs() {
    int32_t v3 = static_cast<uint8_t>(GetFlag(1u)) != 0;
    if (static_cast<uint8_t>(GetFlag(2u))) {
        ++v3;
    }
    if (static_cast<uint8_t>(GetFlag(4u))) {
        ++v3;
    }
    if (static_cast<uint8_t>(GetFlag(8u))) {
        ++v3;
    }
    return v3;
}

TESReputation* TESDataHandler::GetReputation(uint32_t aiID) {
    TESForm* formByNumericID = TESForm::GetFormByNumericID(aiID);
    return dynamic_cast<TESReputation*>(formByNumericID);
}

TESDataHandler::FormList* TESDataHandler::GetClimateList() {
    return &listClimates;
}

TESDataHandler::FormList* TESDataHandler::GetImageSpaceList() {
    return &listImageSpaces;
}

BSSimpleList<TESEyes*>* TESDataHandler::GetEyesList() {
    return &listEyes;
}

void TESDataHandler::GenerateDefaultObjects() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_TES_DEFAULT_OBJECTS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 10895
    );

    pDoorMarker = nullptr;
    pBoundMarker = nullptr;
    pXMarker = nullptr;
    pXMarkerHeading = nullptr;
    pMapMarker = nullptr;
    pAudioMarker = nullptr;
    pAudioBuoyMarker = nullptr;
    pNorthMarker = nullptr;
    pJailMarker = nullptr;
    pTempleMarker = nullptr;
    pHolyMarker = nullptr;
    pTravelMarker = nullptr;
    pCreatureFaction = nullptr;
    pLockpick = nullptr;
    pHorseMarker = nullptr;
    pDefaultWater = nullptr;
    pUnreadDefaultObject = nullptr;
    pReanimateEyes = nullptr;
    pFists = nullptr;
    pMaleVoice = nullptr;
    pFemaleVoice = nullptr;
    pAshPile1 = nullptr;
    pAshPile2 = nullptr;
    pWaterExplosion = nullptr;
    pGasTrapDummyWeap = nullptr;

    ActorValue::RegisterActorValues();
    pCameraPathManager->CreateDefaultPath();

    pWaterExplosion = dynamic_cast<BGSExplosion*>(TESForm::GetFormByNumericID(0x1F5));
    if (!pWaterExplosion) {
        void* pMemory = ourMalloc(sizeof(BGSExplosion));
        pWaterExplosion = pMemory ? ::new (pMemory) BGSExplosion : nullptr;
        pWaterExplosion->SetFormID(0x1F5, true);
        pWaterExplosion->SetFormEditorID("Default Water Explosion");
        pWaterExplosion->SetFullName("Water Explosion");
        GetExplosionList()->AddHead(pWaterExplosion);
        pWaterExplosion->SetAltered(false);
    }

    pGasTrapDummyWeap = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(0x1F6));
    if (!pGasTrapDummyWeap) {
        void* pMemory = ourMalloc(sizeof(TESObjectWEAP));
        pGasTrapDummyWeap = pMemory ? ::new (pMemory) TESObjectWEAP : nullptr;
        pGasTrapDummyWeap->SetFormID(0x1F6, true);
        pGasTrapDummyWeap->SetFormEditorID("GasTrap Dummy");
        pGasTrapDummyWeap->SetFullName("GasTrap Dummy");
        pObjectList->lnk_Append(pGasTrapDummyWeap);
        pGasTrapDummyWeap->SetAltered(false);
    }

    pReanimateEyes = dynamic_cast<TESEyes*>(TESForm::GetFormByNumericID(0x1A));
    if (!pReanimateEyes) {
        void* pMemory = ourMalloc(sizeof(TESEyes));
        pReanimateEyes = pMemory ? ::new (pMemory) TESEyes : nullptr;
        pReanimateEyes->SetFormID(0x1A, true);
        pReanimateEyes->SetFormEditorID("eyeReanimate");
        pReanimateEyes->SetFullName("Reanimate Eyes");
        GetEyesList()->AddHead(pReanimateEyes);
        pReanimateEyes->SetAltered(false);
    }

    pAshPile1 = dynamic_cast<TESObjectACTI*>(TESForm::GetFormByNumericID(0x1B));
    if (!pAshPile1) {
        void* pMemory = ourMalloc(sizeof(TESObjectACTI));
        pAshPile1 = pMemory ? ::new (pMemory) TESObjectACTI : nullptr;
        pAshPile1->SetFormID(0x1B, true);
        pAshPile1->SetFormEditorID("DefaultAshPile1");
        pAshPile1->SetFullName("Ash Pile 1");
        pObjectList->lnk_Append(pAshPile1);
        pAshPile1->SetAltered(false);
    }

    pAshPile2 = dynamic_cast<TESObjectACTI*>(TESForm::GetFormByNumericID(0x22));
    if (!pAshPile2) {
        void* pMemory = ourMalloc(sizeof(TESObjectACTI));
        pAshPile2 = pMemory ? ::new (pMemory) TESObjectACTI : nullptr;
        pAshPile2->SetFormID(0x22, true);
        pAshPile2->SetFormEditorID("DefaultAshPile2");
        pAshPile2->SetFullName("Ash Pile 2");
        pObjectList->lnk_Append(pAshPile2);
        pAshPile2->SetAltered(false);
    }

    pMaleVoice = dynamic_cast<BGSVoiceType*>(TESForm::GetFormByNumericID(0x2D));
    if (!pMaleVoice) {
        void* pMemory = ourMalloc(sizeof(BGSVoiceType));
        pMaleVoice = pMemory ? ::new (pMemory) BGSVoiceType : nullptr;
        pMaleVoice->SetFormID(0x2D, true);
        pMaleVoice->SetFormEditorID("AdultMaleVoice1");
        GetVoiceTypeList()->AddHead(pMaleVoice);
        pMaleVoice->SetAltered(false);
    }

    pFemaleVoice = dynamic_cast<BGSVoiceType*>(TESForm::GetFormByNumericID(0x2E));
    if (!pFemaleVoice) {
        void* pMemory = ourMalloc(sizeof(BGSVoiceType));
        pFemaleVoice = pMemory ? ::new (pMemory) BGSVoiceType : nullptr;
        pFemaleVoice->SetFormID(0x2E, true);
        pFemaleVoice->SetFormEditorID("AdultFemaleVoice1");
        GetVoiceTypeList()->AddHead(pFemaleVoice);
        pFemaleVoice->SetAltered(false);
    }

    pDefaultWater = dynamic_cast<TESWaterForm*>(TESForm::GetFormByNumericID(0x18));
    if (!pDefaultWater) {
        void* pMemory = ourMalloc(sizeof(TESWaterForm));
        pDefaultWater = pMemory ? ::new (pMemory) TESWaterForm : nullptr;
        pDefaultWater->SetFormID(0x18, true);
        pDefaultWater->SetFormEditorID("DefaultWater");
        GetWaterList()->AddHead(pDefaultWater);
        pDefaultWater->SetAltered(false);
    }

    pHorseMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x12));
    if (!pHorseMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pHorseMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pHorseMarker->SetFormID(0x12, true);
        pHorseMarker->SetFormEditorID("HorseMarker");
        pHorseMarker->SetModel("Marker_Horse.nif");
        pObjectList->lnk_Append(pHorseMarker);
        pHorseMarker->SetAltered(false);
    }

    pLockpick = dynamic_cast<TESObjectMISC*>(TESForm::GetFormByNumericID(0xA));
    if (!pLockpick) {
        void* pMemory = ourMalloc(sizeof(TESObjectMISC));
        pLockpick = pMemory ? ::new (pMemory) TESObjectMISC : nullptr;
        pLockpick->SetFormID(0xA, true);
        pLockpick->SetFormEditorID("BobbyPin");
        pObjectList->lnk_Append(pLockpick);
        pLockpick->SetAltered(false);
    }

    pDoorMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x1));
    if (!pDoorMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pDoorMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pDoorMarker->SetFormID(0x1, true);
        pDoorMarker->SetFormEditorID("DoorMarker");
        pDoorMarker->SetModel("MarkerTeleport.nif");
        pObjectList->lnk_Append(pDoorMarker);
        pDoorMarker->SetAltered(false);
    }

    // The default worldspace moves to the head of the worldspace list.
    TESWorldSpace* pWorldSpace = dynamic_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(0x3C));
    if (pWorldSpace && GetWorldSpaceList()->GetItem() != pWorldSpace) {
        GetWorldSpaceList()->Remove(pWorldSpace);
        GetWorldSpaceList()->AddHead(pWorldSpace);
    }

    pBoundMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x15));
    if (!pBoundMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pBoundMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pBoundMarker->SetFormID(0x15, true);
        pBoundMarker->SetFormEditorID("MultiBoundMarker");
        static_cast<TESObjectSTAT*>(pBoundMarker)->SetModel("");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pBoundMarker));
        pBoundMarker->SetAltered(false);
    }

    pPlaneMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x17));
    if (!pPlaneMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pPlaneMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pPlaneMarker->SetFormID(0x17, true);
        pPlaneMarker->SetFormEditorID("PlaneMarker");
        pPlaneMarker->SetModel("");
        pObjectList->lnk_Append(pPlaneMarker);
        pPlaneMarker->SetAltered(false);
    }

    pRoomMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x1F));
    if (!pRoomMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pRoomMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pRoomMarker->SetFormID(0x1F, true);
        pRoomMarker->SetFormEditorID("RoomMarker");
        static_cast<TESObjectSTAT*>(pRoomMarker)->SetModel("");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pRoomMarker));
        pRoomMarker->SetAltered(false);
    }

    pPortalMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x20));
    if (!pPortalMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pPortalMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pPortalMarker->SetFormID(0x20, true);
        pPortalMarker->SetFormEditorID("PortalMarker");
        pPortalMarker->SetModel("");
        pObjectList->lnk_Append(pPortalMarker);
        pPortalMarker->SetAltered(false);
    }

    pCollisionMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x21));
    if (!pCollisionMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pCollisionMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pCollisionMarker->SetFormID(0x21, true);
        pCollisionMarker->SetFormEditorID("CollisionMarker");
        pCollisionMarker->SetModel("");
        pObjectList->lnk_Append(pCollisionMarker);
        pCollisionMarker->SetAltered(false);
    }

    pXMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x3B));
    if (!pXMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pXMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pXMarker->SetFormID(0x3B, true);
        pXMarker->SetFormEditorID("XMarker");
        static_cast<TESObjectSTAT*>(pXMarker)->SetModel("MarkerX.nif");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pXMarker));
        pXMarker->SetAltered(false);
    }

    pXMarkerHeading = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x34));
    if (!pXMarkerHeading) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pXMarkerHeading = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pXMarkerHeading->SetFormID(0x34, true);
        pXMarkerHeading->SetFormEditorID("XMarkerHeading");
        static_cast<TESObjectSTAT*>(pXMarkerHeading)->SetModel("MarkerXHeading.nif");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pXMarkerHeading));
        pXMarkerHeading->SetAltered(false);
    }

    pCOCMarkerHeading = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x32));
    if (!pCOCMarkerHeading) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pCOCMarkerHeading = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pCOCMarkerHeading->SetFormID(0x32, true);
        pCOCMarkerHeading->SetFormEditorID("COCMarkerHeading");
        pCOCMarkerHeading->SetModel("MarkerCOCHeading.nif");
        pObjectList->lnk_Append(pCOCMarkerHeading);
        pCOCMarkerHeading->SetAltered(false);
    }

    pMapMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x10));
    if (!pMapMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pMapMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pMapMarker->SetFormID(0x10, true);
        pMapMarker->SetFormEditorID("MapMarker");
        pMapMarker->SetModel("Marker_Map.NIF");
        pObjectList->lnk_Append(pMapMarker);
        pMapMarker->SetAltered(false);
    }

    pAudioMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x23));
    if (!pAudioMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pAudioMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pAudioMarker->SetFormID(0x23, true);
        pAudioMarker->SetFormEditorID("AudioMarker");
        pAudioMarker->SetModel("Marker_Audio.NIF");
        pObjectList->lnk_Append(pAudioMarker);
        pAudioMarker->SetAltered(false);
    }

    pAudioBuoyMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x24));
    if (!pAudioBuoyMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pAudioBuoyMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pAudioBuoyMarker->SetFormID(0x24, true);
        pAudioBuoyMarker->SetFormEditorID("AudioBuoyMarker");
        pAudioBuoyMarker->SetModel("Marker_Audio.NIF");
        pObjectList->lnk_Append(pAudioBuoyMarker);
        pAudioBuoyMarker->SetAltered(false);
    }

    pTravelMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x2));
    if (!pTravelMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pTravelMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pTravelMarker->SetFormID(0x2, true);
        pTravelMarker->SetFormEditorID("TravelMarker");
        pTravelMarker->SetModel("Marker_Travel.nif");
        pObjectList->lnk_Append(pTravelMarker);
        pTravelMarker->SetAltered(false);
    }

    pNorthMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x3));
    if (!pNorthMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pNorthMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pNorthMarker->SetFormID(0x3, true);
        pNorthMarker->SetFormEditorID("NorthMarker");
        pNorthMarker->SetModel("Marker_North.nif");
        pObjectList->lnk_Append(pNorthMarker);
        pNorthMarker->SetAltered(false);
    }

    pJailMarker = dynamic_cast<TESObjectDOOR*>(TESForm::GetFormByNumericID(0x4));
    if (!pJailMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectDOOR));
        pJailMarker = pMemory ? ::new (pMemory) TESObjectDOOR : nullptr;
        pJailMarker->SetFormID(0x4, true);
        pJailMarker->SetFormEditorID("PrisonMarker");
        pJailMarker->SetModel("Marker_Prison.nif");
        pObjectList->lnk_Append(pJailMarker);
        pJailMarker->SetAltered(false);
    }

    pTempleMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x6));
    if (!pTempleMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pTempleMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pTempleMarker->SetFormID(0x6, true);
        pTempleMarker->SetFormEditorID("TempleMarker");
        pTempleMarker->SetModel("Marker_Temple.nif");
        pObjectList->lnk_Append(pTempleMarker);
        pTempleMarker->SetAltered(false);
    }

    pHolyMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x5));
    if (!pHolyMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pHolyMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pHolyMarker->SetFormID(0x5, true);
        pHolyMarker->SetFormEditorID("DivineMarker");
        pHolyMarker->SetModel("Marker_Divine.nif");
        pObjectList->lnk_Append(pHolyMarker);
        pHolyMarker->SetAltered(false);
    }

    pRadiationMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x33));
    if (!pRadiationMarker) {
        void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
        pRadiationMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
        pRadiationMarker->SetFormID(0x33, true);
        pRadiationMarker->SetFormEditorID("Radiation Marker");
        pRadiationMarker->SetModel("Marker_Radiation.nif");
        pObjectList->lnk_Append(pRadiationMarker);
        pRadiationMarker->SetAltered(false);
    }

    pCreatureFaction = dynamic_cast<TESFaction*>(TESForm::GetFormByNumericID(0x13));
    if (!pCreatureFaction) {
        void* pMemory = ourMalloc(sizeof(TESFaction));
        pCreatureFaction = pMemory ? ::new (pMemory) TESFaction : nullptr;
        pCreatureFaction->SetFormID(0x13, true);
        pCreatureFaction->SetFormEditorID("CreatureFaction");
        GetFactionList()->AddHead(pCreatureFaction);
        pCreatureFaction->SetReaction(pCreatureFaction, 100);
        pCreatureFaction->AddRankData();
        pCreatureFaction->SetInitialized(true);
        pCreatureFaction->SetAltered(false);
    }

    if (!TESForm::GetFormByNumericID(0xF)) {
        void* pMemory = ourMalloc(sizeof(TESObjectMISC));
        TESObjectMISC* pCaps = pMemory ? ::new (pMemory) TESObjectMISC : nullptr;
        pCaps->SetFormID(0xF, true);
        pCaps->SetFormEditorID("Caps001");
        pObjectList->lnk_Append(pCaps);
        pCaps->SetAltered(false);
    }

    pDefLootBag = static_cast<TESObjectCONT*>(TESForm::GetFormByNumericID(0xE));
    if (!pDefLootBag) {
        void* pMemory = ourMalloc(sizeof(TESObjectCONT));
        pDefLootBag = pMemory ? ::new (pMemory) TESObjectCONT : nullptr;
        pDefLootBag->SetFormID(0xE, true);
        pDefLootBag->SetFormEditorID("LootBag");
        if (!pDefLootBag->GetModel() || BSstrlen(pDefLootBag->GetModel()) == 0) {
            pDefLootBag->SetModel("Clutter\\Sack01.NIF");
        }
        pObjectList->lnk_Append(pDefLootBag);
        pDefLootBag->SetAltered(false);
    }

    pFists = static_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(0x1F4));
    if (!pFists) {
        void* pMemory = ourMalloc(sizeof(TESObjectWEAP));
        pFists = pMemory ? ::new (pMemory) TESObjectWEAP : nullptr;
        pFists->SetFormID(0x1F4, true);
        pFists->SetFormEditorID("Fists");
        pFists->SetWeaponType(WEAPON_HAND_TO_HAND);
        pObjectList->lnk_Append(pFists);
        pFists->SetAltered(false);
    }

    uint32_t aFurnitureMarkerIDs[40] = {
        0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
        0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
    };
    const char* aFurnitureMarkerNames[40] = {
        "FurnitureMarker01", "FurnitureMarker02", "FurnitureMarker03", "FurnitureMarker04", "FurnitureMarker05",
        "FurnitureMarker06", "FurnitureMarker07", "FurnitureMarker08", "FurnitureMarker09", "FurnitureMarker10",
        "FurnitureMarker11", "FurnitureMarker12", "FurnitureMarker13", "FurnitureMarker14", "FurnitureMarker15",
        "FurnitureMarker16", "FurnitureMarker17", "FurnitureMarker18", "FurnitureMarker19", "FurnitureMarker20",
        "FurnitureMarker21", "FurnitureMarker22", "FurnitureMarker23", "FurnitureMarker24", "FurnitureMarker25",
        "FurnitureMarker26", "FurnitureMarker27", "FurnitureMarker28", "FurnitureMarker29", "FurnitureMarker30",
        "FurnitureMarker31", "FurnitureMarker32", "FurnitureMarker33", "FurnitureMarker34", "FurnitureMarker35",
        "FurnitureMarker36", "FurnitureMarker37", "FurnitureMarker38", "FurnitureMarker39", "FurnitureMarker40",
    };
    for (int32_t i = 0; i < 40; ++i) {
        TESForm* pMarker = TESForm::GetFormByNumericID(aFurnitureMarkerIDs[i]);
        if (!pMarker) {
            void* pMemory = ourMalloc(sizeof(TESObjectSTAT));
            pMarker = pMemory ? ::new (pMemory) TESObjectSTAT : nullptr;
            pMarker->SetFormID(aFurnitureMarkerIDs[i], true);
            pMarker->SetFormEditorID(aFurnitureMarkerNames[i]);
            pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pMarker));
            pMarker->SetAltered(false);
        }
    }

    TESTopic::AddDefaultTopics();

    if (!TESForm::GetFormByNumericID(0x35)) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        TESGlobal* pGlobal = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pGlobal->SetFormID(0x35, true);
        pGlobal->SetFormEditorID("GameYear");
        pGlobal->SetValue(77.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameYear", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x36)) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        TESGlobal* pGlobal = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pGlobal->SetFormID(0x36, true);
        pGlobal->SetFormEditorID("GameMonth");
        pGlobal->SetValue(7.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameMonth", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x37)) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        TESGlobal* pGlobal = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pGlobal->SetFormID(0x37, true);
        pGlobal->SetFormEditorID("GameDay");
        pGlobal->SetValue(17.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameDay", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x38)) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        TESGlobal* pGlobal = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pGlobal->SetFormID(0x38, true);
        pGlobal->SetFormEditorID("GameHour");
        pGlobal->SetValue(12.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameHour", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x39)) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        TESGlobal* pGlobal = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pGlobal->SetFormID(0x39, true);
        pGlobal->SetFormEditorID("GameDaysPassed");
        pGlobal->SetValue(1.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameDaysPassed", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x3A)) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        TESGlobal* pGlobal = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pGlobal->SetFormID(0x3A, true);
        pGlobal->SetFormEditorID("TimeScale");
        pGlobal->SetValue(30.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("TimeScale", pGlobal);
    }

    TESNPC* pPlayerBase = dynamic_cast<TESNPC*>(TESForm::GetFormByNumericID(0x7));
    if (!pPlayerBase) {
        void* pMemory = ourMalloc(sizeof(TESNPC));
        pPlayerBase = pMemory ? ::new (pMemory) TESNPC : nullptr;
        pPlayerBase->SetFormID(0x7, true);
        pPlayerBase->SetFormEditorID("Player");
        pObjectList->lnk_Append(pPlayerBase);
        pPlayerBase->SetAltered(false);
    }

    if (!TESForm::GetFormByNumericID(0x15E)) {
        void* pMemory = ourMalloc(sizeof(TESWeather));
        TESWeather* pWeather = pMemory ? ::new (pMemory) TESWeather : nullptr;
        pWeather->SetFormID(0x15E, true);
        pWeather->SetFormEditorID("DefaultWeather");
        pWeather->MakeDefault();
        GetWeatherList()->AddHead(pWeather);
    }

    if (!TESForm::GetFormByNumericID(0x15F)) {
        void* pMemory = ourMalloc(sizeof(TESClimate));
        TESClimate* pClimate = pMemory ? ::new (pMemory) TESClimate : nullptr;
        pClimate->SetFormID(0x15F, true);
        pClimate->SetFormEditorID("DefaultClimate");
        pClimate->MakeDefault();
        GetClimateList()->AddHead(pClimate);
    }

    Sky::GetInstance()->SetCurrentClimate(nullptr, true);

    if (!TESForm::GetFormByNumericID(0x163)) {
        void* pMemory = ourMalloc(sizeof(BGSListForm));
        BGSListForm* pListForm = pMemory ? ::new (pMemory) BGSListForm : nullptr;
        pListForm->SetFormID(0x163, true);
        pListForm->SetFormEditorID("HelpManualPC");
        GetListFormList()->AddHead(pListForm);
    }

    if (!TESForm::GetFormByNumericID(0x165)) {
        void* pMemory = ourMalloc(sizeof(BGSListForm));
        BGSListForm* pListForm = pMemory ? ::new (pMemory) BGSListForm : nullptr;
        pListForm->SetFormID(0x165, true);
        pListForm->SetFormEditorID("HelpManualXBox");
        GetListFormList()->AddHead(pListForm);
    }

    // The help messages' editor IDs by form ID - 0x168; null entries are not
    // created.
    const char* aHelpMessageNames[64] = {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        "HelpPipBoyItems",
        "HelpPipBoyRepair",
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        "HelpChargenTagSkills",
        "HelpChargenRace",
        "HelpLeveling",
        "HelpDialogue",
        nullptr,
        "HelpHacking",
        "HelpLockpickingPC",
        "HelpVATSPC",
        "HelpContainer",
        "HelpBarter",
        "HelpTerminal",
        "HelpPipBoyStats",
        "HelpPipBoyData",
        "HelpVATSXBox",
        "HelpLockpickingXBox",
        "HelpItemMod",
        "HelpCaravanBetting",
        "HelpCaravanDeckBuilding",
        "HelpCaravanStartingCaravans",
        "HelpCaravanContractWar",
        "HelpWeapons",
        "HelpApparel",
        "HelpAmmo",
        "HelpAmmoXbox",
        "HelpCrafting",
        "HelpReputation",
        "HelpHardcoreNeeds",
    };
    for (uint32_t i = 0; i < 64; ++i) {
        if (aHelpMessageNames[i]) {
            void* pMemory = ourMalloc(sizeof(BGSMessage));
            BGSMessage* pMessage = pMemory ? ::new (pMemory) BGSMessage : nullptr;
            pMessage->SetFormID(i + 0x168, true);
            pMessage->SetFormEditorID(aHelpMessageNames[i]);
            GetMessageList()->AddHead(pMessage);
        }
    }

    // The exterior image space is created only with the interior one (no lookup
    // of 0x161).
    if (!TESForm::GetFormByNumericID(0x160)) {
        void* pMemory = ourMalloc(sizeof(TESImageSpace));
        TESImageSpace* pImageSpace = pMemory ? ::new (pMemory) TESImageSpace : nullptr;
        pImageSpace->SetFormID(0x160, true);
        pImageSpace->SetFormEditorID("DefaultImageSpaceInterior");
        pImageSpace->MakeDefault();
        GetImageSpaceList()->AddHead(pImageSpace);

        pMemory = ourMalloc(sizeof(TESImageSpace));
        pImageSpace = pMemory ? ::new (pMemory) TESImageSpace : nullptr;
        pImageSpace->SetFormID(0x161, true);
        pImageSpace->SetFormEditorID("DefaultImageSpaceExterior");
        pImageSpace->MakeDefault();
        GetImageSpaceList()->AddHead(pImageSpace);
    }

    if (!TESForm::GetFormByNumericID(0x162)) {
        void* pMemory = ourMalloc(sizeof(TESImageSpaceModifier));
        TESImageSpaceModifier* pModifier = pMemory ? ::new (pMemory) TESImageSpaceModifier : nullptr;
        pModifier->SetFormID(0x162, true);
        pModifier->SetFormEditorID("GetHit");
        pModifier->MakeDefault();
        GetImageSpaceModifierList()->AddHead(pModifier);
    }

    if (!TESForm::GetFormByNumericID(0x166)) {
        void* pMemory = ourMalloc(sizeof(TESImageSpaceModifier));
        TESImageSpaceModifier* pModifier = pMemory ? ::new (pMemory) TESImageSpaceModifier : nullptr;
        pModifier->SetFormID(0x166, true);
        pModifier->SetFormEditorID("ExplosionInFace");
        pModifier->MakeDefault();
        GetImageSpaceModifierList()->AddHead(pModifier);
    }

    if (!TESForm::GetFormByNumericID(0x164)) {
        void* pMemory = ourMalloc(sizeof(TESImageSpaceModifier));
        TESImageSpaceModifier* pModifier = pMemory ? ::new (pMemory) TESImageSpaceModifier : nullptr;
        pModifier->SetFormID(0x164, true);
        pModifier->SetFormEditorID("ImageSpaceConcussion");
        pModifier->MakeDefault();
        GetImageSpaceModifierList()->AddHead(pModifier);
    }

    MagicSystem::GenerateDefaultObjects();

    if (!TESForm::GetFormByNumericID(0x14A)) {
        void* pMemory = ourMalloc(sizeof(EffectSetting));
        EffectSetting* pEffect = pMemory ? ::new (pMemory) EffectSetting : nullptr;
        pEffect->SetFormID(0x14A, true);
        pEffect->SetFormEditorID("ScriptEffect");
        pEffect->SetFullName("Script Effect");
        EffectSettingCollection::AddEffect(pEffect);
    }

    TESSound::GenerateDefaultObjects();
    BGSImpactDataSet::CreateDefaultImpactDataSet();
    BGSEquipType::GenerateDefaultMenuIcons();
    BGSBodyPartData::GenerateDefaultBodyPartData();
    BGSTextureSet::CreateNullTextureSet();
    BGSEncounterZone::CreateNoZone();

    if (!TESForm::GetFormByNumericID(0x3D)) {
        void* pMemory = ourMalloc(sizeof(TESCombatStyle));
        TESCombatStyle* pCombatStyle = pMemory ? ::new (pMemory) TESCombatStyle : nullptr;
        pCombatStyle->SetFormID(0x3D, true);
        pCombatStyle->SetFormEditorID("Default Combatstyle");
        GetCombatStyleList()->AddHead(pCombatStyle);
    }

    pPlayCredits = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(0x63));
    if (!pPlayCredits) {
        void* pMemory = ourMalloc(sizeof(TESGlobal));
        pPlayCredits = pMemory ? ::new (pMemory) TESGlobal : nullptr;
        pPlayCredits->SetFormID(0x63, true);
        pPlayCredits->SetFormEditorID("PlayCredits");
        pPlayCredits->SetValue(0.0f);
        GetGlobalList()->AddHead(pPlayCredits);
        TESForm::pAllFormsByEditorID->SetAt("PlayCredits", pPlayCredits);
    }

    BGSDefaultObjectManager::Instance()->InitializeData();
    pContext->~AutoMemContext();
}

// Builds or reloads the form of the record pFile is on. The AutoMemContext constructor and destructor are called on
// stack contexts and the constructor's results are ignored; each return ends the inner context, then the outer one.
bool TESDataHandler::ConstructObject(
    TESFile* pFile,
    bool abNoExistingForms,
    TESForm* apOldForm,
    bool abIgnoreMissingLastLoadedCell
) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_TES_FORMS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 7833
    );
    bool bAddToList = true;
    const uint32_t eType = pFile->GetTESForm();
    TESForm* pForm = apOldForm;

    if (ucGameSettingsLoadState == 0 && eType == GMST_ID) {
        ucGameSettingsLoadState = 1;
    }
    else {
        const bool bPastSettings = eType != GMST_ID;
        ucGameSettingsLoadState = bPastSettings;
        if (bPastSettings) {
            ucGameSettingsLoadState = 2;
        }
    }

    if (!abNoExistingForms || TESForm::IsDefaultForm(pFile->GetID())) {
        if (pForm == nullptr) {
            pForm = TESForm::GetFormByNumericID(pFile->GetID());
        }
        if (pForm != nullptr) {
            bool bWrongType = false;
            if (pForm->GetFormType() != eType &&
                ((eType != ARMO_ID && eType != ARMA_ID) ||
                 (pForm->GetFormType() != ARMO_ID && pForm->GetFormType() != ARMA_ID))) {
                bWrongType = true;
            }
            if (bWrongType) {
                Warning(
                    "FORMS: Form (%08X) in file \"%s\" has wrong type.\n\nShould "
                    "be:\t%s\nIs:\t\t%s",
                    pFile->GetID(), pFile->GetFilename(), pForm->GetFormTypeString(), formEnumString[eType].pFormString
                );
                pContext->~AutoMemContext();
                return false;
            }
        }
    }

    bool bPartialForm = false;
    if ((!pFile->GetMaster() || !pFile->GetMasterList()->IsEmpty()) && pForm != nullptr && pForm->IsParentForm() &&
        (pFile->GetFormData()->flags & TESForm::kFormFlag_PartialForm) != 0) {
        bPartialForm = true;
    }
    if (bPartialForm) {
        switch (pForm->GetFormType()) {
            case CELL_ID: pLastLoadedCell = static_cast<TESObjectCELL*>(pForm); break;
            case WRLD_ID: pLastLoadedWorld = static_cast<TESWorldSpace*>(pForm); break;
            case DIAL_ID: TESTopic::pLastTopic = static_cast<TESTopic*>(pForm); break;
            default: break;
        }
        pForm->LoadPartial(pFile);
        pForm->SetFile(pFile);
        pForm->SetInitialized(false);
        if (pFile->GetActive()) {
            pForm->SetAltered(true);
        }
        pContext->~AutoMemContext();
        return true;
    }

    if (pForm == nullptr && (pFile->GetFormData()->flags & TESForm::kFormFlag_PartialForm) != 0) {
        Warning("FORMS: Partial form (%08X) found with no matching original data.", pFile->GetID());
        if (pFile->GetID() >> 24 != pFile->GetCompileIndex()) {
            pContext->~AutoMemContext();
            return false;
        }
        Warning(
            "FORMS: Partial form originated in current file. Clearing "
            "erroneous partial data flag."
        );
        pFile->GetFormData()->flags &= ~TESForm::kFormFlag_PartialForm;
    }

    if (pForm != nullptr && (pFile->GetFormData()->flags & TESForm::kFormFlag_Deleted) == 0) {
        pForm->ClearDataAllComponents();
        pForm->ClearData();
        pForm->InitializeDataAllComponents();
        pForm->InitializeData();
    }

    switch (eType) {
        case TES4_ID: {
            uint32_t uiChunk = pFile->GetTESChunk();
            while (uiChunk != NO_CHUNK) {
                if (uiChunk == ONAM_ID) {
                    alignas(AutoMemContext) uint8_t aucMasterContext[sizeof(AutoMemContext)];
                    AutoMemContext* pMasterContext = reinterpret_cast<AutoMemContext*>(aucMasterContext);
                    ::new (aucMasterContext) AutoMemContext(
                        MEM_CONTEXT_MASTER_FILE, true,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 8160
                    );
                    const uint32_t uiSize = pFile->GetChunkSize();
                    const uint32_t uiCount = uiSize >> 2;
                    // new uint32_t[uiCount]: a size that overflows asks for 0xFFFFFFFF
                    // bytes
                    uint32_t* const pIDs = static_cast<uint32_t*>(
                        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
                    );
                    pFile->GetChunkData(pIDs, uiSize);
                    for (uint32_t i = 0; i < uiCount; ++i) {
                        uint32_t uiID = pIDs[i];
                        if (pFile->QEndian()) {
                            // the engine's ENDIAN<long>
                            uiID = __builtin_bswap32(uiID);
                        }
                        const uint32_t uiFileIndex = uiID >> 24;
                        TESFile* pOwner = pFile->GetIndexFile(uiFileIndex + 1);
                        if (pOwner == nullptr) {
                            pOwner = pFile;
                        }
                        uiID &= 0xFFFFFF;
                        uiID |= static_cast<uint32_t>(pOwner->GetCompileIndex()) << 24;
                        TESFile::SetTempIDOwnedByFile(uiID, pFile);
                    }
                    ourFree(pIDs);
                    uiChunk = NO_CHUNK;
                    pMasterContext->~AutoMemContext();
                }
                else if (!pFile->NextChunk()) {
                    uiChunk = NO_CHUNK;
                }
                else {
                    uiChunk = pFile->GetTESChunk();
                }
            }
            pContext->~AutoMemContext();
            return true;
        }

        case DOBJ_ID:
            LoadForm(BGSDefaultObjectManager::Instance(), pFile);
            pContext->~AutoMemContext();
            return true;

        case CELL_ID: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(pForm);
            bool bCreated = false;
            if (pCell == nullptr) {
                void* const pMemory = ourMalloc(sizeof(TESObjectCELL));
                pCell = pMemory != nullptr ? ::new (pMemory) TESObjectCELL : nullptr;
                bCreated = true;
            }
            LoadForm(pCell, pFile);
            if (bSaveLoadGame) {
                pCell->SetAltered(true);
            }
            pCell->SetMaster(true);
            if (bCreated) {
                if (pCell->GetInterior()) {
                    arrayInteriorCells.SetAtGrow(arrayInteriorCells.GetSize(), pCell);
                }
                else {
                    if (pLastLoadedWorld == nullptr) {
                        pLastLoadedWorld = GetWorldSpaceList()->GetItem();
                    }
                    if (!pLastLoadedWorld->AddCell(pCell)) {
                        TESObjectCELL* const pExistingCell =
                            pLastLoadedWorld->GetCellFromCellCoord(pCell->GetDataX(), pCell->GetDataY());
                        Warning(
                            "CELLS: Error adding %scell (%08X) to world space (%08X). "
                            "Cell will be destroyed.",
                            pCell->GetPersistentCell() ? "Persistent " : "", pCell->iFormID, pLastLoadedWorld->iFormID
                        );
                        delete pCell;
                        pCell = pExistingCell;
                    }
                }
            }
            pLastLoadedCell = pCell;
            pContext->~AutoMemContext();
            return true;
        }

        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case PCBE_ID: {
            if (pLastLoadedCell == nullptr && !abIgnoreMissingLastLoadedCell) {
                pContext->~AutoMemContext();
                return false;
            }
            TESObjectREFR* pRef;
            if (pForm != nullptr) {
                pRef = static_cast<TESObjectREFR*>(pForm);
                // the base object is fetched and not used
                pRef->GetObjectReference();
                TESObjectCELL* const pOldCell = pRef->GetSaveParentCell();
                if (pOldCell == pLastLoadedCell) {
                    bAddToList = false;
                }
                else if (pOldCell != nullptr) {
                    pOldCell->RemoveReference(pRef);
                }
            }
            else {
                pRef = TESObjectREFR::CreateReference(static_cast<uint8_t>(eType), true);
            }
            if (!LoadForm(pRef, pFile)) {
                if (pRef->GetObjectReference() == nullptr) {
                    pRef->SetFormType(static_cast<uint8_t>(eType));
                }
                if (pRef->GetParentCell() != nullptr) {
                    pRef->GetParentCell()->RemoveReference(pRef);
                }
                if (pLastLoadedCell != nullptr && pLastLoadedCell->GetPersistentCell()) {
                    pRef->GetExtra()->SetPersistentCell(pLastLoadedCell);
                }
                else {
                    pRef->SetParentCell(pLastLoadedCell);
                }
                TESForm* const pBadForm = pRef;
                if (listBadForms.FindPos(pBadForm) == nullptr) {
                    TESForm* const pNewBadForm = pRef;
                    listBadForms.AddHead(pNewBadForm);
                }
                bAddToList = false;
                MobileObject* const pMobile = dynamic_cast<MobileObject*>(pRef);
                if (pMobile != nullptr) {
                    ReferenceProcessLists.RemoveReference(pMobile, PROCESS_TYPE_LOW);
                }
            }
            if (pRef->GetObjectReference() != nullptr) {
                if (pSaveLoadGame->GetLoadingMovedRefs()) {
                    bAddToList = false;
                }
                if (bAddToList) {
                    if (pLastLoadedCell != nullptr) {
                        pLastLoadedCell->AddReference(pRef, false);
                        if (pLastLoadedCell->GetPersistentCell()) {
                            TESObjectCELL* const pParentCell = pRef->GetParentCell();
                            if (pParentCell != nullptr) {
                                // Takes the position before the worldspace.
                                const NiPoint3* const pLocation = &pRef->GetLocationOnReference();
                                TESWorldSpace* const pWorld = pLastLoadedCell->GetWorldSpace();
                                TESObjectCELL* const pPosCell = pWorld->GetCellFromWorldCoord(*pLocation);
                                if (pParentCell != pPosCell) {
                                    if (pPosCell != nullptr) {
                                        pPosCell->AddReference(pRef, false);
                                    }
                                    else {
                                        pParentCell->RemoveReference(pRef);
                                    }
                                }
                            }
                        }
                    }
                    if (!pRef->GetDelete()) {
                        pRef->GetObjectReference()->IncRef();
                    }
                }
                else if (pRef->GetDelete()) {
                    pRef->GetObjectReference()->DecRef();
                }
                if (pSaveLoadGameOLD != nullptr && pSaveLoadGameOLD->GetLoading() && pRef->Get3D_Body() != nullptr) {
                    pRef->Set3D(nullptr, true);
                }
                if (pRef->GetRefPersists() && static_cast<double>(pFile->GetFileVersion()) < 0.20000000298023224) {
                    TESBoundObject* const pBase = pRef->GetObjectReference();
                    Warning(
                        "CELLS: ref (%08X) to base object %s (%08X) in cell %s (%i, "
                        "%i) (%08X) is persistent in the "
                        "non-persistent file %s.",
                        pRef->iFormID, pBase->GetFormEditorID(), pBase->iFormID, pLastLoadedCell->GetFormEditorID(),
                        pLastLoadedCell->GetDataX(), pLastLoadedCell->GetDataY(), pLastLoadedCell->iFormID,
                        pFile->GetFilename()
                    );
                    pRef->SetFormFlags(pRef->GetFormFlags() & ~TESForm::kFormFlags_IsRefPersistent);
                }
            }
            pContext->~AutoMemContext();
            return true;
        }

        case LAND_ID:
            if (pLastLoadedCell != nullptr) {
                TESObjectLAND* pLand = pLastLoadedCell->GetLand();
                if (pLand == nullptr) {
                    void* const pMemory = ourMalloc(sizeof(TESObjectLAND));
                    pLand = pMemory != nullptr ? ::new (pMemory) TESObjectLAND : nullptr;
                    pLand->SetCell(pLastLoadedCell);
                    pLastLoadedCell->SetLand(pLand);
                }
                LoadForm(pLand, pFile);
            }
            pContext->~AutoMemContext();
            return true;

        case GMST_ID: {
            alignas(AutoMemContext) uint8_t aucSettingContext[sizeof(AutoMemContext)];
            AutoMemContext* pSettingContext = reinterpret_cast<AutoMemContext*>(aucSettingContext);
            ::new (aucSettingContext) AutoMemContext(
                MEM_CONTEXT_GAMESETTING, true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 8465
            );
            if (pFile->GetTESChunk() == EDID_ID) {
                char* const pKey = static_cast<char*>(__builtin_alloca(pFile->GetChunkSize()));
                pFile->GetChunkData(pKey, 0);
                SettingT<GameSettingCollection>::GetCollection()->ReadSetting(pFile, pKey);
            }
            pSettingContext->~AutoMemContext();
            pContext->~AutoMemContext();
            return true;
        }

        case WRLD_ID:
            if (pForm == nullptr) {
                void* const pMemory = ourMalloc(sizeof(TESWorldSpace));
                TESWorldSpace* pWorld = pMemory != nullptr ? ::new (pMemory) TESWorldSpace : nullptr;
                LoadForm(pWorld, pFile);
                listWorldSpaces.AddTail(pWorld);
                pLastLoadedWorld = pWorld;
            }
            else {
                LoadForm(pForm, pFile);
                pLastLoadedWorld = static_cast<TESWorldSpace*>(pForm);
            }
            pContext->~AutoMemContext();
            return true;

        case DIAL_ID: {
            alignas(AutoMemContext) uint8_t aucDialogueContext[sizeof(AutoMemContext)];
            AutoMemContext* pDialogueContext = reinterpret_cast<AutoMemContext*>(aucDialogueContext);
            ::new (aucDialogueContext) AutoMemContext(
                MEM_CONTEXT_DIALOGUE, true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 8507
            );
            if (pForm == nullptr) {
                void* const pMemory = ourMalloc(sizeof(TESTopic));
                TESTopic* const pTopic = pMemory != nullptr ? ::new (pMemory) TESTopic(0) : nullptr;
                LoadForm(pTopic, pFile);
                AddTopic(pTopic);
            }
            else {
                LoadForm(pForm, pFile);
            }
            pDialogueContext->~AutoMemContext();
            pContext->~AutoMemContext();
            return true;
        }

        case INFO_ID: {
            bool bExisting = false;
            alignas(AutoMemContext) uint8_t aucDialogueContext[sizeof(AutoMemContext)];
            AutoMemContext* pDialogueContext = reinterpret_cast<AutoMemContext*>(aucDialogueContext);
            ::new (aucDialogueContext) AutoMemContext(
                MEM_CONTEXT_DIALOGUE, true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 8530
            );
            if (pForm != nullptr) {
                bExisting = true;
            }
            else {
                void* const pMemory = ourMalloc(sizeof(TESTopicInfo));
                pForm = pMemory != nullptr ? ::new (pMemory) TESTopicInfo(DT_TOPIC) : nullptr;
            }
            pLastLoadedInfo = static_cast<TESTopicInfo*>(pForm);
            const bool bLoaded = LoadForm(pForm, pFile);
            if (!bExisting) {
                AddTopicInfo(pLastLoadedInfo);
            }
            pDialogueContext->~AutoMemContext();
            pContext->~AutoMemContext();
            return bLoaded;
        }

        case SKIL_ID:
            if (pForm != nullptr) {
                TESSkill* const pSkill = dynamic_cast<TESSkill*>(pForm);
                if (pSkill != nullptr) {
                    pSkill->ResetData();
                    LoadForm(pSkill, pFile);
                }
            }
            pContext->~AutoMemContext();
            return true;

        case IDLE_ID:
        case CPTH_ID:
            if (pForm == nullptr) {
                TESForm* const pNewForm = CreateFormOfType(static_cast<uint8_t>(eType));
                LoadForm(pNewForm, pFile);
            }
            else {
                LoadForm(pForm, pFile);
            }
            pContext->~AutoMemContext();
            return true;

        case TXST_ID:
        case MICN_ID:
        case GLOB_ID:
        case CLAS_ID:
        case FACT_ID:
        case HDPT_ID:
        case HAIR_ID:
        case EYES_ID:
        case RACE_ID:
        case SOUN_ID:
        case ASPC_ID:
        case MGEF_ID:
        case SCPT_ID:
        case LTEX_ID:
        case ENCH_ID:
        case SPEL_ID:
        case TACT_ID:
        case TERM_ID:
        case PWAT_ID:
        case IDLM_ID:
        case NOTE_ID:
        case PROJ_ID:
        case WTHR_ID:
        case CLMT_ID:
        case REGN_ID:
        case QUST_ID:
        case PACK_ID:
        case CSTY_ID:
        case LSCR_ID:
        case ANIO_ID:
        case WATR_ID:
        case EFSH_ID:
        case EXPL_ID:
        case DEBR_ID:
        case IMGS_ID:
        case IMAD_ID:
        case FLST_ID:
        case PERK_ID:
        case BPTD_ID:
        case ADDN_ID:
        case RADS_ID:
        case CAMS_ID:
        case VTYP_ID:
        case IPCT_ID:
        case IPDS_ID:
        case ECZN_ID:
        case MESG_ID:
        case RGDL_ID:
        case LGTM_ID:
        case MUSC_ID:
        case REPU_ID:
        case RCPE_ID:
        case RCCT_ID:
        case CHIP_ID:
        case CSNO_ID:
        case LSCT_ID:
        case MSET_ID:
        case ALOC_ID:
        case CHAL_ID:
        case AMEF_ID:
        case CCRD_ID:
        case CMNY_ID:
        case CDCK_ID:
        case DEHY_ID:
        case HUNG_ID:
        case SLPD_ID:
            if (pForm == nullptr) {
                TESForm* const pNewForm = CreateFormOfType(static_cast<uint8_t>(eType));
                LoadForm(pNewForm, pFile);
                AddFormToDataHandler(pNewForm);
            }
            else {
                LoadForm(pForm, pFile);
            }
            pContext->~AutoMemContext();
            return true;

        case NAVM_ID: {
            alignas(AutoMemContext) uint8_t aucNavMeshContext[sizeof(AutoMemContext)];
            AutoMemContext* pNavMeshContext = reinterpret_cast<AutoMemContext*>(aucNavMeshContext);
            ::new (aucNavMeshContext) AutoMemContext(
                MEM_CONTEXT_NAVMESH, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                9999
            );
            if (pForm == nullptr) {
                void* const pMemory = NiMemObject::operator new(sizeof(NavMesh));
                NavMesh* const pNewNavMesh = pMemory != nullptr ? ::new (pMemory) NavMesh : nullptr;
                NavMeshPtr spNavMesh = NavMeshPtr::MakeNavMeshPtr(pNewNavMesh);
                LoadForm(spNavMesh.operator->(), pFile);
                if (spNavMesh->GetParentCell() != nullptr) {
                    if (spNavMesh->GetParentCell()->pNavMeshes == nullptr) {
                        void* const pArrayMemory = ourMalloc(sizeof(NavMeshArray));
                        NavMeshArray* const pNavMeshes =
                            pArrayMemory != nullptr ? ::new (pArrayMemory) NavMeshArray : nullptr;
                        spNavMesh->GetParentCell()->SetNavMeshArray(pNavMeshes);
                    }
                    spNavMesh->GetParentCell()->pNavMeshes->AddNavMesh(spNavMesh);
                }
            }
            else {
                LoadForm(pForm, pFile);
            }
            pNavMeshContext->~AutoMemContext();
            pContext->~AutoMemContext();
            return true;
        }

        case NAVI_ID: {
            alignas(AutoMemContext) uint8_t aucNavMeshContext[sizeof(AutoMemContext)];
            AutoMemContext* pNavMeshContext = reinterpret_cast<AutoMemContext*>(aucNavMeshContext);
            ::new (aucNavMeshContext) AutoMemContext(
                MEM_CONTEXT_NAVMESH, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                10029
            );
            if (pForm == nullptr) {
                void* const pMemory = ourMalloc(sizeof(NavMeshInfoMap));
                NavMeshInfoMap* const pInfoMap = pMemory != nullptr ? ::new (pMemory) NavMeshInfoMap : nullptr;
                TES::pInstance->SetNavMeshInfoMap(pInfoMap);
                LoadForm(pInfoMap, pFile);
            }
            else {
                LoadForm(pForm, pFile);
            }
            pNavMeshContext->~AutoMemContext();
            pContext->~AutoMemContext();
            return true;
        }

        case AVIF_ID:
            LoadForm(pForm, pFile);
            pContext->~AutoMemContext();
            return true;

        default: {
            MessageHandler::IncDisableWarningCount(true);
            TESForm* pNewForm;
            if (pForm != nullptr) {
                bAddToList = false;
                pNewForm = pForm;
            }
            else {
                pNewForm = CreateFormOfType(static_cast<uint8_t>(eType));
            }
            if (pNewForm == nullptr) {
                FORM* const pFORM = pFile->GetFormData();
                if (pFORM->form != 0) {
                    char acRecordType[5];
                    acRecordType[0] = static_cast<char>(pFORM->form & 0xFF);
                    acRecordType[1] = static_cast<char>((pFORM->form & 0xFF00) >> 8);
                    acRecordType[2] = static_cast<char>((pFORM->form & 0xFF0000) >> 16);
                    acRecordType[3] = static_cast<char>((pFORM->form & 0xFF000000) >> 24);
                    acRecordType[4] = '\0';
                    Warning("FORMS: Unknown %s_ID in ConstructObject.", acRecordType);
                }
                else if (eType == NO_FORM) {
                    Warning("FORMS: NO_FORM trying to load in ConstructObject.");
                }
                MessageHandler::IncDisableWarningCount(false);
                pContext->~AutoMemContext();
                return false;
            }
            MessageHandler::IncDisableWarningCount(false);
            if (LoadForm(pNewForm, pFile)) {
                if (bAddToList) {
                    pObjectList->lnk_Append(static_cast<TESObject*>(pNewForm));
                }
                pContext->~AutoMemContext();
                return true;
            }
            delete pNewForm;
            pContext->~AutoMemContext();
            return false;
        }
    }
}

uint32_t TESDataHandler::AddDownloadedContentToFileList() {
    return 0;
}

TESEffectShader* TESDataHandler::SpotterEffectItem() const {
    return pSpotterShader;
}

uint32_t TESDataHandler::GetNextFormID() const {
    return iNextID;
}

TESEffectShader* TESDataHandler::GetItemDetectedShader() const {
    return pItemDetectedShader;
}

void TESDataHandler::ClearBarterContainer() {
    if (pBarterContainer) {
        pBarterContainer->ClearAllChangeItems();
    }
}

void TESDataHandler::ClearRecipeContainer() {
    if (pRecipeContainer) {
        pRecipeContainer->ClearAllChangeItems();
    }
}

TESFile* TESDataHandler::GetCompiledFile(int32_t index) {
    if (((uint32_t)index) <= 254) {
        return pFileIndex[(uint32_t)index];
    }

    return nullptr;
}

bool TESDataHandler::GetMasterSave() {
    return (bool)((uint8_t)bMasterSave);
}

uint32_t TESDataHandler::GetInteriorCellCount() {
    return (uint32_t)((int32_t)(&arrayInteriorCells)->NiTArray<TESObjectCELL*>::GetEffectiveSize());
}

void TESDataHandler::SetCheckingModels(bool abNewbool) {
    bCheckingModels = (bool)((uint8_t)abNewbool);
}

void TESDataHandler::SetIsLoading(bool a2) {
    bDontRemoveIDs = (bool)((uint8_t)a2);
}

TESClass* TESDataHandler::GetClass(uint32_t a1) {
    TESForm* formByNumericID = TESForm::GetFormByNumericID(a1);
    return dynamic_cast<TESClass*>(formByNumericID);
}

TESObjectREFR* TESDataHandler::CreateReferenceAtLocation(
    TESBoundObject* apObject,
    const NiPoint3& aLocation,
    const NiPoint3& aDirection,
    TESObjectCELL* apInterior,
    TESWorldSpace* apWorld,
    TESObjectREFR* apReference,
    BGSPrimitive* apAddPrimitive,
    void* apAdditionalData
) {
    TESObjectREFR* pnewactor = nullptr;
    if ((apInterior) && (!apInterior->GetInterior())) {
        apInterior = nullptr;
    }

    if (apInterior) {
        apWorld = nullptr;
    }
    if ((apObject) && (apInterior || apWorld)) {
        bool bpersistent = false;
        if (apReference) {
            bpersistent = apReference->GetRefPersists();
            apReference->SetRefPersists(false);
            TESObjectCELL* poldparent = apReference->GetParentCell();
            if (poldparent) {
                poldparent->RemoveReference(apReference);
            }
            pnewactor = apReference;
            if (!apReference->GetObjectReference()) {
                pnewactor->SetObjectReference(apObject);
            }
            if (pnewactor->MustRefPersist()) {
                bpersistent = true;
            }
        }
        else {
            switch (apObject->GetFormType()) {
                case NPC__ID: {
                    TESObjectREFR* pretref;
                    Character* v31 = static_cast<Character*>(ourMalloc(sizeof(Character)));
                    if (v31) {
                        pretref = (TESObjectREFR*)::new (v31) Character;
                    }
                    else {
                        pretref = nullptr;
                    }
                    pnewactor = pretref;
                    goto LABEL_32;
                }
                case CREA_ID: {
                    TESObjectREFR* pretref_2;
                    Creature* v29 = static_cast<Creature*>(ourMalloc(sizeof(Creature)));
                    if (v29) {
                        pretref_2 = (TESObjectREFR*)::new (v29) Creature;
                    }
                    else {
                        pretref_2 = nullptr;
                    }
                    pnewactor = pretref_2;
                    goto LABEL_32;
                }
                case PROJ_ID: {
                    return CreateProjectileAtLocation(apObject, aLocation, aDirection, apInterior, apWorld);
                }
                case EXPL_ID: {
                    return CreateExplosionAtLocation(apObject, aLocation, aDirection, apInterior, apWorld);
                }
                default: {
                    TESObjectREFR* pretref_3;
                    void* apMemory;
                    apMemory = ourMalloc(sizeof(TESObjectREFR));
                    if (apMemory) {
                        pretref_3 = ::new (apMemory) TESObjectREFR;
                    }
                    else {
                        pretref_3 = nullptr;
                    }
                    pnewactor = pretref_3;
                LABEL_32:;
                    if ((apObject == pPortalMarker) && (apAdditionalData)) {
                        (&pnewactor->m_Extra)->SetPortalLinkedRoomRef(0, static_cast<TESObjectREFR*>(apAdditionalData));
                    }

                    pnewactor->SetObjectReference(apObject);
                    if (apAddPrimitive) {
                        ExtraPrimitive* v17;
                        ExtraPrimitive* v25 = static_cast<ExtraPrimitive*>(ourMalloc(sizeof(ExtraPrimitive)));
                        if (v25) {
                            v17 = ::new (v25) ExtraPrimitive(apAddPrimitive);
                        }
                        else {
                            v17 = nullptr;
                        }
                        BSExtraData* apExtra = (BSExtraData*)v17;
                        (&pnewactor->m_Extra)->AddExtra(v17);
                    }
                    if (pnewactor->MustBeVisibleDistant()) {
                        pnewactor->SetVisibleDistant(true);
                    }
                    bpersistent = pnewactor->MustRefPersist();
                    TESObjectLIGH* plight = dynamic_cast<TESObjectLIGH*>(apObject);
                    if (plight) {
                        bool OffByDefault = plight->GetOffByDefault();
                        pnewactor->SetFireOff(OffByDefault);
                    }
                    break;
                }
            }
        }
        if ((pnewactor->IsActor()) && (static_cast<Actor*>(pnewactor)->GetActorBase()->GetUsesLeveledTemplate())) {
            static_cast<Actor*>(pnewactor)->GetActorBase()->CalcTemplateForRef(pnewactor);
        }

        uint32_t CastingType = pnewactor->iFormID;
        if (IsFormIDCreated(CastingType)) {
            pnewactor->AddChange(0);
        }
        if (BGSDestructibleObjectForm::IsDestructible(pnewactor->GetObjectReference())) {
            pnewactor->SetDestructible(true);
        }
        pnewactor->SetLocationOnReference(aLocation);
        pnewactor->SetAngleOnReference(aDirection);
        TESObjectCELL* pnewparent = nullptr;
        if (apWorld) {
            int32_t icellx = FloatToInt(aLocation.x) >> 12;
            int32_t icelly = FloatToInt(aLocation.y) >> 12;
            pnewparent = apWorld->GetCellFromCellCoord(icellx, icelly);
        }
        else {
            pnewparent = apInterior;
        }
        if (pnewparent) {
            NiAVObject* a1 = nullptr;
            if (TES::pInstance->IsCellLoaded(pnewparent, false)) {
                if (apObject == pBoundMarker ||
                    (apObject == pPlaneMarker ||
                     (apObject == pDoorMarker ||
                      (apObject == pTravelMarker ||
                       (apObject == pXMarker ||
                        (apObject == pXMarkerHeading ||
                         (apObject == pMapMarker || (apObject == pAudioMarker || apObject == pAudioBuoyMarker)))))))) {
                    NiAVObject* pmodel;
                    NiNode* v23 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
                    if (v23) {
                        pmodel = (NiAVObject*)::new (v23) NiNode(0);
                    }
                    else {
                        pmodel = nullptr;
                    }
                    a1 = pmodel;
                    pnewactor->Set3D(pmodel, true);
                }
                pnewactor->SetParentCell(pnewparent);
                if (pnewactor->HasContainer()) {
                    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pnewactor);
                    pinventory->InitLeveledItems();
                    pinventory->InitFromContainerExtra();
                    pinventory->InitScripts();
                    if (pinventory->IsEmpty()) {
                        (&pnewactor->m_Extra)->RemoveContainerChanges();
                    }
                }
                pnewactor->SetParentCell(nullptr);
            }
            pnewactor->SetDroppedRef(true);
            pnewparent->AddReference(pnewactor, false);
            pnewactor->SetRefPersists(bpersistent);
            NiAVObject* pmodel_2 = (NiAVObject*)pnewactor->Get3D();
            if (pmodel_2) {
                const NiPoint3* v12 = &pnewactor->GetLocationOnReference();
                pmodel_2->SetTranslate(*v12);
                pmodel_2->SetRotate(pnewactor->GetOrientation());
                bhkNiCollisionObject::ResetSim(pmodel_2, true);
                ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
                ShadowSceneNode_->AddObject(pmodel_2);
                pmodel_2->UpdateProperties();
                NiUpdateData UpdateData(0.0f, false, false);
                pmodel_2->Update(UpdateData);
            }
        }
        else {
            pnewactor->Set3D(nullptr, false);
            pnewactor->SetRefPersists(true);
            apWorld->AddPersistentRef(pnewactor);
        }
    }

    Actor* pactor = dynamic_cast<Actor*>(pnewactor);
    if ((pactor) && (!pactor->HasEditorLocation())) {
        pactor->SetStartingPosition();
    }

    if ((pactor) && (!apReference)) {
        pactor->SetStillLoading(false);
    }

    return pnewactor;
}

// Copies the argument's byte into the field as it is.
void TESDataHandler::SetIsClosingFile(bool abClosing) {
    bClearingData = abClosing;
}

void TESDataHandler::CheckModels(bool abUpdateBounds) {
    uint16_t iflags;
    if (!bFileSkipModelChecks.Bool()) {
        SetCheckingModels(true);
        if (bFileTestLoad.Bool()) {
            appMain->RenderMenuBackground();
        }
        if (bFileCheckModelCollision.Bool()) {
            PrintToFile(
                "TestModels - Collision Info.xls",
                "Model\tModel Bound\tDisplay Triangle Count\tConvex Vertices Count\tConvex Vertices / Size "
                "Ratio\tConvex Vertices / Display Geometry Ratio\tTriangle Collection Count\tTriangle Collection / "
                "Size Ratio\tTriangle Collection / Display Geometry Ratio"
            );
        }
        TESObject* obj = pObjectList->lnk_lpRoot;
        int32_t iobjCount = 0;
        while (obj) {
            if (!obj->GetDelete()) {
                OBJECT_TYPE type;
                uint8_t form = obj->GetFormType();
                for (type = OBJECT_ACTIVATOR; type < OBJECT_COUNT && aObjectTypeFormTypes[type] != form;
                     type = static_cast<OBJECT_TYPE>(type + 1)) {
                    ;
                }
                if (type < OBJECT_COUNT) {
                    int32_t v24;
                    char Format[264];
                    TESObjectREFR* pref_2;
                    char FileName[268];
                    const char* pmodeldesc;
                    bool skip = obj->IsMarker();
                    TESModel* pmodel = dynamic_cast<TESModel*>(obj);
                    TESBoundObject* pboundObj = dynamic_cast<TESBoundObject*>(obj);
                    TESBipedModelForm* pbipedModel = dynamic_cast<TESBipedModelForm*>(obj);
                    if (pmodel) {
                        v24 = 1;
                    }
                    else {
                        v24 = pbipedModel != nullptr ? 4 : 0;
                    }
                    int32_t icount = v24;
                    while (true) {
                        if (!icount) {
                            goto LABEL_89;
                        }
                        pmodeldesc = "";
                        if (pbipedModel) {
                            switch (icount) {
                                case 1: {
                                    pmodel = (TESModel*)pbipedModel->GetWorldTESModel(TESActorBase::SEX_MALE);
                                    pmodeldesc = "Male World ";
                                    break;
                                }
                                case 2: {
                                    pmodel = (TESModel*)pbipedModel->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                                    pmodeldesc = "Female World ";
                                    break;
                                }
                                case 3: {
                                    pmodel = (TESModel*)pbipedModel->GetBipedTESModel(TESActorBase::SEX_MALE);
                                    pmodeldesc = "Male Biped ";
                                    break;
                                }
                                case 4: {
                                    pmodel = (TESModel*)pbipedModel->GetBipedTESModel(TESActorBase::SEX_FEMALE);
                                    pmodeldesc = "Female Biped ";
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        if (skip) {
                        LABEL_86:;
                            --icount;
                            continue;
                        }
                        break;
                    }
                    if (!pmodel->GetModelLength()) {
                        if (type != OBJECT_NPC && type != OBJECT_LIGHT && bFileNeededMessage.Bool()) {
                            switch (type) {
                                case OBJECT_ACTIVATOR:
                                case OBJECT_LIGHT:
                                case OBJECT_NPC: {
                                    goto LABEL_86;
                                }
                                case OBJECT_WEAPON: {
                                    if (!static_cast<TESObjectWEAP*>(obj)->TESObjectWEAP::IsEmbeddedWeapon()) {
                                        goto LABEL_85;
                                    }
                                    break;
                                }
                                default: {
                                LABEL_85:;
                                    const char* v18 = obj->GetFormEditorID();
                                    Warning(
                                        "MODELS: No model selected for %smodel %s \"%s\".", pmodeldesc,
                                        objectLabel[type], v18
                                    );
                                    break;
                                }
                            }
                        }
                        goto LABEL_86;
                    }
                    if (type == OBJECT_TREE) {
                        BSsprintf(FileName, 260, "Trees\\%s", pmodel->GetModel());
                    }
                    else {
                        BSsprintf(FileName, 260, "Meshes\\%s", pmodel->GetModel());
                    }
                    if (FileFinder::Locate(FileName, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                        if (bFileGoneMessage.Bool()) {
                            char apDevice[4];
                            char Dst[512];
                            char apDir[264];
                            char apExt[256];
                            const char* v3 = pmodel->GetModel();
                            BSsplitpath(v3, apDevice, 3, apDir, 260, Dst, 512, apExt, 256);
                            BSsprintf(FileName, 260, "x%s", Dst);
                            BSstrcpy(Dst, 512, FileName);
                            BSsprintf(FileName, 260, "Meshes\\%s", apDir);
                            BSstrcpy(apDir, 260, FileName);
                            makepath(FileName, 260u, apDevice, apDir, Dst, apExt);
                            if (FileFinder::Locate(FileName, nullptr, 0, 0xFFFFFFFFu)) {
                                CheckForNiRawImageData(FileName);
                            }
                            else {
                                const char* v14 = obj->GetFormEditorID();
                                char* v11 = objectLabel[type];
                                const char* v8 = pmodel->GetModel();
                                Warning("MODELS: %sModel \"%s\" not found for %s \"%s\".", pmodeldesc, v8, v11, v14);
                            }
                        }
                        goto LABEL_86;
                    }
                    if (!pboundObj ||
                        (!bFileTestLoad.Bool() && (!abUpdateBounds && !bFileCheckModelCollision.Bool()))) {
                        goto LABEL_86;
                    }
                    const char* v15 = pboundObj->GetFormEditorID();
                    char* v12 = objectLabel[type];
                    const char* v9 = pmodel->GetModel();
                    std::sprintf(Format, "Loading %smodel \"%s\" for %s \"%s\".", pmodeldesc, v9, v12, v15);
                    Interface::PrintLine(static_cast<const char*>(Format));
                    appMain->Swap();
                    if (pboundObj->GetFormType() == NPC__ID) {
                        TESObjectREFR* pref;
                        Character* v28 = static_cast<Character*>(ourMalloc(sizeof(Character)));
                        if (v28) {
                            pref = (TESObjectREFR*)::new (v28) Character;
                        }
                        else {
                            pref = nullptr;
                        }
                        pref_2 = pref;
                    }
                    else {
                        if (pboundObj->GetFormType() == CREA_ID) {
                            TESObjectREFR* pref_3;
                            Creature* v27 = static_cast<Creature*>(ourMalloc(sizeof(Creature)));
                            if (v27) {
                                pref_3 = (TESObjectREFR*)::new (v27) Creature;
                            }
                            else {
                                pref_3 = nullptr;
                            }
                            pref_2 = pref_3;
                        }
                        else {
                            TESObjectREFR* pref_4;
                            TESObjectREFR* apMemory = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                            if (apMemory) {
                                pref_4 = ::new (apMemory) TESObjectREFR;
                            }
                            else {
                                pref_4 = nullptr;
                            }
                            pref_2 = pref_4;
                        }
                    }
                    pref_2->SetObjectReference(pboundObj);
                    int32_t FormType = pboundObj->GetFormType();
                    if (FormType == NPC__ID || FormType == CREA_ID) {
                        if (!bFileCheckModelCollision.Bool() || (bFileTestLoad.Bool() || abUpdateBounds)) {
                            iflags = 0;
                            if (!TESActorBaseData::GetLeveledTemplateInHierarchy(
                                    static_cast<TESActorBase*>(pboundObj), iflags
                                ) ||
                                (iflags & 6) == 0) {
                                pModelLoader->QueueReference(pref_2, IO_TASK_PRIORITY_CRITICAL, false);
                                pIOManager->LoadAllQueuedPriorities();
                                goto LABEL_62;
                            }
                        }
                    }
                    else {
                    LABEL_62:;
                        int32_t iwarncount = MessageHandler::QWarningCount();
                        pref_2->Load3D(false);
                        if (iwarncount != MessageHandler::QWarningCount()) {
                            const char* v16 = pboundObj->GetFormEditorID();
                            char* v13 = objectLabel[type];
                            const char* v10 = pmodel->GetModel();
                            Warning(
                                "Warnings were encountered loading %smodel '%s' for %s %s.", pmodeldesc, v10, v13, v16
                            );
                            Warning("");
                        }
                    }
                    if (pref_2->Get3D()) {
                        const char* v17 = pmodel->GetModel();
                        NiNode* v4 = pref_2->Get3D();
                        CheckTexturesRecurse(v4, v17);
                    }
                    if (bFileCheckModelCollision.Bool() && pboundObj->GetFormType() != NPC__ID &&
                        pboundObj->GetFormType() != CREA_ID) {
                        int32_t iconvexVerticesCount__itriangleCount = 0;
                        int32_t ishapeCollectionTriCount = 0;
                        NiNode* v5 = pref_2->Get3D();
                        TESHavokUtilities::InspectHavokObjects(
                            v5, iconvexVerticesCount__itriangleCount, ishapeCollectionTriCount
                        );
                        // The bound size is kept as a float and widened for each use.
                        float fradius__pmodel = pboundObj->GetBoundSize();
                        int32_t iconvexVerticesCount = 0;
                        if (TES::pInstance) {
                            int32_t itriangleCount;
                            NiAVObject* p3d = (NiAVObject*)pref_2->Get3D();
                            if (p3d) {
                                NiNode* arg0 = p3d->IsNode();
                                itriangleCount = TES::pInstance->GetFaceCount(arg0, true);
                            }
                            else {
                                itriangleCount = TES::pInstance->GetFaceCount(nullptr, true);
                            }
                            iconvexVerticesCount = itriangleCount;
                        }
                        PrintToFile(
                            "TestModels - Collision Info.xls", "%s\t%.2f\t%i\t%i\t%.2f\t%.2f\t%i\t%.2f\t%.2f",
                            pmodel->GetModel(), fradius__pmodel, iconvexVerticesCount,
                            iconvexVerticesCount__itriangleCount,
                            static_cast<double>(iconvexVerticesCount__itriangleCount) / fradius__pmodel,
                            static_cast<double>(iconvexVerticesCount__itriangleCount) /
                                static_cast<double>(iconvexVerticesCount),
                            ishapeCollectionTriCount, static_cast<double>(ishapeCollectionTriCount) / fradius__pmodel,
                            static_cast<double>(ishapeCollectionTriCount) / static_cast<double>(iconvexVerticesCount)
                        );
                    }
                    if (pref_2) {
                        delete pref_2;
                    }
                    int32_t v7 = iobjCount++;
                    if (v7 > 20) {
                        TES::pInstance->CleanUpUnusedTextures(false);
                        iobjCount = 0;
                    }
                    goto LABEL_86;
                }
                Warning("MODELS: Object \"%s\" invalid type.", obj->GetFormEditorID());
            }
        LABEL_89:;
            obj = obj->nod_lpNext;
        }
        SetCheckingModels(false);
    }
}

void TESDataHandler::CheckTexturesRecurse(NiAVObject* apObject, const char* apModel) {
    if (apObject) {
        if (apObject->IsTriBasedGeom()) {
            NiProperty* pShaderProperty = apObject->GetProperty(3);
            NiTexturingProperty* pTextProp = static_cast<NiTexturingProperty*>(apObject->GetProperty(5));
            pTextProp = static_cast<NiTexturingProperty*>(
                NiObject::DynamicCastStatic(&NiTexturingProperty::ms_RTTI, pTextProp)
            );
            if (pTextProp) {
                NiTexturingProperty::Map* DetailMap = pTextProp->GetDetailMap();
                PrintTextureInfo(
                    DetailMap, static_cast<NiTriBasedGeom*>(apObject), "Detail Map", apModel, pShaderProperty != nullptr
                );
                NiTexturingProperty::BumpMap* BumpMap = pTextProp->GetBumpMap();
                PrintTextureInfo(
                    BumpMap, static_cast<NiTriBasedGeom*>(apObject), "Bump Map", apModel, pShaderProperty != nullptr
                );
                NiTexturingProperty::Map* GlowMap = pTextProp->GetGlowMap();
                PrintTextureInfo(
                    GlowMap, static_cast<NiTriBasedGeom*>(apObject), "Glow Map", apModel, pShaderProperty != nullptr
                );
                NiTexturingProperty::Map* GlossMap = pTextProp->GetGlossMap();
                PrintTextureInfo(
                    GlossMap, static_cast<NiTriBasedGeom*>(apObject), "Gloss Map", apModel, pShaderProperty != nullptr
                );
                NiTexturingProperty::Map* DarkMap = pTextProp->GetDarkMap();
                PrintTextureInfo(
                    DarkMap, static_cast<NiTriBasedGeom*>(apObject), "Dark Map", apModel, pShaderProperty != nullptr
                );
                for (uint32_t uiIndex = 0; uiIndex < pTextProp->GetDecalMapCount(); ++uiIndex) {
                    NiTexturingProperty::Map* DecalMap = pTextProp->GetDecalMap(uiIndex);
                    PrintTextureInfo(
                        DecalMap, static_cast<NiTriBasedGeom*>(apObject), "Decal Map", apModel,
                        pShaderProperty != nullptr
                    );
                }
            }
        }
        else {
            if (apObject->IsNode()) {
                uint32_t ArrayCount = static_cast<NiNode*>(apObject)->NiNode::GetArrayCount();
                for (uint32_t a1 = 0; a1 < ArrayCount; ++a1) {
                    NiNode* v11 = static_cast<NiNode*>(static_cast<NiNode*>(apObject)->NiNode::GetAt(a1));
                    if (v11) {
                        CheckTexturesRecurse(v11, apModel);
                    }
                }
            }
        }
    }
}

void TESDataHandler::CheckForNiRawImageData(char* pFilename) {
    const std::filesystem::path kPath = platform::NativePath(pFilename);
    std::FILE* pStream = std::fopen(kPath.string().c_str(), "rb");
    if (pStream) {
        // GetFileSize's INVALID_FILE_SIZE (0xFFFFFFFF) when the size cannot be read.
        std::error_code kError;
        uint32_t fileSize = static_cast<uint32_t>(std::filesystem::file_size(kPath, kError));
        if (kError) {
            fileSize = 0xFFFFFFFF;
        }
        if (fileSize) {
            char* buffer = static_cast<char*>(ourMalloc(fileSize));
            const uint32_t sizeRead = static_cast<uint32_t>(std::fread(buffer, 1, fileSize, pStream));
            if (sizeRead == fileSize && fileSize > 15) {
                for (uint32_t n = 0; n < fileSize - 15; ++n) {
                    if (buffer[n] == 78 &&
                        (buffer[n + 1] == 105 &&
                         (buffer[n + 2] == 82 &&
                          (buffer[n + 3] == 97 &&
                           (buffer[n + 4] == 119 &&
                            (buffer[n + 5] == 73 &&
                             (buffer[n + 6] == 109 &&
                              (buffer[n + 7] == 97 &&
                               (buffer[n + 8] == 103 &&
                                (buffer[n + 9] == 101 &&
                                 (buffer[n + 10] == 68 &&
                                  (buffer[n + 11] == 97 && (buffer[n + 12] == 116 && buffer[n + 13] == 97))))))))))))) {
                        Warning("TEXTURES: Export \"%s\" again without textures embedded.\r\n", pFilename);
                        break;
                    }
                }
            }
        }
        std::fclose(pStream);
    }
}

void TESDataHandler::CreateBarterContainer(TESObjectREFR* apVendor) {
    if (pBarterContainer) {
        ClearBarterContainer();
    }
    else {
        InventoryChanges* v5;
        InventoryChanges* apMemory = static_cast<InventoryChanges*>(ourMalloc(sizeof(InventoryChanges)));
        if (apMemory) {
            v5 = ::new (apMemory) InventoryChanges(nullptr);
        }
        else {
            v5 = nullptr;
        }
        pBarterContainer = v5;
    }
    InventoryChanges* pActorlist = (&apVendor->m_Extra)->GetContainerChanges();
    if (pActorlist) {
        pActorlist->CreateCopyofList(pBarterContainer, apVendor, false);
    }
    TESObjectREFR* pmerchant = (&apVendor->m_Extra)->GetMerchantContainer();
    if (pmerchant) {
        pActorlist = (&pmerchant->m_Extra)->GetContainerChanges();
        if (pActorlist) {
            pActorlist->RunScripts(pmerchant);
        }
        else {
            InventoryChanges* pActorlist_2;
            InventoryChanges* v7 = static_cast<InventoryChanges*>(ourMalloc(sizeof(InventoryChanges)));
            if (v7) {
                pActorlist_2 = ::new (v7) InventoryChanges(pmerchant);
            }
            else {
                pActorlist_2 = nullptr;
            }
            pActorlist = pActorlist_2;
        }
        pActorlist->CreateCopyofList(pBarterContainer, pmerchant, false);
    }
    TESObjectCELL* pcell = apVendor->GetParentCell();
    if (pcell) {
        pcell->FillBarterContainer(apVendor, pBarterContainer);
        if (apVendor->IsActor()) {
            Actor* pact = static_cast<Actor*>(apVendor);
            NiPointer<BSShaderAccumulator>* v15 = nullptr;
            for (ItemChange* pitemChange : pBarterContainer->pListofChanges->ItemsUntilNull()) {
                TESBoundObject* pboundObject = pitemChange->pContainerObj;
                if ((pboundObject->GetFormType() == AMMO_ID) && (pact->GetCurrentWeapon())) {
                    TESObjectWEAP* CurrentWeapon = pact->GetCurrentWeapon();
                    if (CurrentWeapon->GetCurrentAmmo(pact) == pboundObject) {
                        BSSimpleList<ExtraDataList*>* pextralist = pitemChange->pExtraObjectList;
                        int32_t icount = 0;
                        while (pextralist && pextralist->m_item) {
                            ExtraDataList* pextra = pextralist->m_item;
                            if (pextra->GetOriginalReference() && pextra->GetOriginalReference() == pact) {
                                icount = pextra->GetCount();
                                pitemChange->pExtraObjectList->Remove(pextra);
                                break;
                            }
                            pextralist = pextralist->m_pkNext;
                        }
                        uint32_t FormHealth_0 = pitemChange->iNumber;
                        pitemChange->iNumber = FormHealth_0 - icount;
                        if (pitemChange->iNumber <= 0 ||
                            (!pitemChange->pExtraObjectList ||
                             (pitemChange->pExtraObjectList->IsEmpty() || pitemChange->GetWorn(false)))) {
                            pBarterContainer->pListofChanges->Remove(pitemChange);
                        }
                        return;
                    }
                }
            }
        }
    }
}

void TESDataHandler::RemoveItemBarterContainer(ItemChange* apitem, int32_t aiNumber, TESObjectREFR* apVendor) {
    ExtraDataList* pextra = nullptr;
    ItemChange* pitem = pBarterContainer->GetObjectInList(apitem->pContainerObj, true, 0);
    if (pitem) {
        BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
        if (plist) {
            pextra = plist->m_item;
        }
        bool bflag = false;
        while (true) {
            TESObjectREFR* pref;
            int32_t iremove;
            while (true) {
                if (!aiNumber) {
                    return;
                }
                pref = nullptr;
                iremove = 1;
                if (!plist && bflag) {
                    return;
                }
                if (plist) {
                    break;
                }
                CreateBarterContainer(apVendor);
                pextra = nullptr;
                bflag = true;
                pitem = pBarterContainer->GetObjectInList(apitem->pContainerObj, true, 0);
                if (!pitem) {
                    return;
                }
                plist = pitem->pExtraObjectList;
                if (plist) {
                    pextra = plist->m_item;
                }
            }
            pextra = plist->m_item;
            plist = plist->m_pkNext;
            if (pextra) {
                int16_t Count = pextra->GetCount();
                iremove = Count;
                if (Count <= 0) {
                    pref = nullptr;
                }
                else {
                    pref = pextra->GetOriginalReference();
                    if (aiNumber < iremove) {
                        iremove = aiNumber;
                    }
                    else {
                        pextra->RemoveOriginalReferenceExtra();
                    }
                }
            }
            if (pref) {
                if (pref->HasContainer()) {
                    ExtraDataList* ppassedExtra = nullptr;
                    if ((pextra->ItemsInList()) && (pextra->ItemsInList() != 1 || !pextra->GetExtraData(36))) {
                        ppassedExtra = pextra;
                    }

                    pref->RemoveItem(
                        pitem->pContainerObj, ppassedExtra, iremove, false, false, pPlayer, nullptr, nullptr, true,
                        false
                    );
                    InventoryChanges* pchangeref = (&pref->m_Extra)->GetContainerChanges();
                    bool bdelete = true;
                    if (pchangeref) {
                        ItemChange* ptempitem = pchangeref->GetObjectInList(pitem->pContainerObj, true, 0);
                        if ((ptempitem) && (ptempitem->pExtraObjectList)) {
                            BSSimpleList<ExtraDataList*>* ptempObj = ptempitem->pExtraObjectList;
                            while (ptempObj && (ptempObj->m_item && bdelete)) {
                                BSSimpleList<ExtraDataList*>* v5 = ptempObj;
                                if (v5->m_item == pextra) {
                                    bdelete = false;
                                }
                                else {
                                    ptempObj = ptempObj->m_pkNext;
                                }
                            }
                        }
                    }
                    InventoryChanges* pchangeref_2 = (&pPlayer->m_Extra)->GetContainerChanges();
                    if (pchangeref_2) {
                        ItemChange* ptempitem_2 = pchangeref_2->GetObjectInList(pitem->pContainerObj, true, 0);
                        if ((ptempitem_2) && (ptempitem_2->pExtraObjectList)) {
                            BSSimpleList<ExtraDataList*>* ptempObj_2 = ptempitem_2->pExtraObjectList;
                            while (ptempObj_2 && (ptempObj_2->m_item && bdelete)) {
                                BSSimpleList<ExtraDataList*>* v6 = ptempObj_2;
                                if (v6->m_item == pextra) {
                                    bdelete = false;
                                }
                                else {
                                    ptempObj_2 = ptempObj_2->m_pkNext;
                                }
                            }
                        }
                    }
                    if ((bdelete) && (pextra) &&
                        (pextra->ItemsInList() < 2 && pextra->GetCount() <= 1 && !pextra->GetScript())) {
                        pitem->pExtraObjectList->Remove(pextra);
                        plist = pitem->pExtraObjectList;
                        if (pextra) {
                            delete pextra;
                        }
                        pextra = nullptr;
                    }

                    if ((pextra) && ((pextra->GetCount() > 1 && pextra->ItemsInList() < 2) || !pextra->ItemsInList())) {
                        pextra = nullptr;
                    }
                }
                else {
                    (&pref->m_Extra)->RemoveOwnership();
                    (&pref->m_Extra)->RemoveOriginalReferenceExtra();
                    pPlayer->PickUpObject(pref, iremove, false);
                    pitem->pExtraObjectList->Remove(pextra);
                    plist = pitem->pExtraObjectList;
                    if ((pextra->ItemsInList() < 2 && pextra->GetCount() <= 1 && !pextra->GetScript()) && (pextra)) {
                        delete pextra;
                    }

                    pextra = nullptr;
                }
                aiNumber -= iremove;
                if (plist) {
                LABEL_62:;
                    pextra = plist->m_item;
                }
                else {
                    pextra = nullptr;
                }
            }
            else {
                if (plist) {
                    goto LABEL_62;
                }
                aiNumber = 0;
            }
        }
    }
}

void TESDataHandler::ClearFileList() {
    BSSimpleList<TESFile*>* FileList = GetFileList();
    TESFile::DeleteFileList(FileList);
}

bool TESDataHandler::ClearData() {
    NiTMapItem<uint32_t, TESForm*>* it;
    bClearingData = true;
    CleanUpBadForms();
    TESForm::pAlteredForms->RemoveAll();
    TES::pInstance->ClearAllCells(false, false);
    TES::pInstance->ClearWorldSpace();
    TES::pInstance->GetActiveImageSpaceModifiers()->RemoveAll();
    GarbageCollector::ClearAll(false);
    BGSTalkingActivator::ClearRadioStationList();
    pPlayer->RemovePlayerMapMarker();
    InventoryChanges::KillTempRef();
    BGSTextureSet::ClearNullTextureSet();
    BGSDefaultObjectManager::ReleaseInstance();
    BGSEncounterZone::ClearNoZone();
    while (true) {
        Script* pScript = (&listScripts)->m_item;
        if (!pScript) {
            break;
        }
        listScripts.RemoveHead();
        delete pScript;
    }
    while (true) {
        BGSListForm* plistform = (&listListForms)->m_item;
        if (!plistform) {
            break;
        }
        listListForms.RemoveHead();
        delete plistform;
    }
    while (true) {
        BGSMenuIcon* picon = (&listMenuIcons)->m_item;
        if (!picon) {
            break;
        }
        listMenuIcons.RemoveHead();
        delete picon;
    }
    while (true) {
        BGSHeadPart* ppart = (&listHeadParts)->m_item;
        if (!ppart) {
            break;
        }
        listHeadParts.RemoveHead();
        delete ppart;
    }
    while (true) {
        TESHair* phair = (&listHair)->m_item;
        if (!phair) {
            break;
        }
        listHair.RemoveHead();
        delete phair;
    }
    while (true) {
        TESEyes* peyes = (&listEyes)->m_item;
        if (!peyes) {
            break;
        }
        listEyes.RemoveHead();
        delete peyes;
    }
    while (true) {
        TESRace* pRace = (&listRaces)->m_item;
        if (!pRace) {
            break;
        }
        listRaces.RemoveHead();
        delete pRace;
    }
    pUnreadDefaultObject = nullptr;
    while (true) {
        BGSEncounterZone* pzone = static_cast<BGSEncounterZone*>((&listZones)->m_item);
        if (!pzone) {
            break;
        }
        listZones.RemoveHead();
        delete pzone;
    }
    while (true) {
        TESClimate* pClimate = static_cast<TESClimate*>((&listClimates)->m_item);
        if (!pClimate) {
            break;
        }
        listClimates.RemoveHead();
        delete pClimate;
    }
    while (true) {
        TESWeather* pWeather = static_cast<TESWeather*>((&listWeather)->m_item);
        if (!pWeather) {
            break;
        }
        listWeather.RemoveHead();
        delete pWeather;
    }
    while (true) {
        TESClass* pClass = (&listClasses)->m_item;
        if (!pClass) {
            break;
        }
        listClasses.RemoveHead();
        delete pClass;
    }
    while (true) {
        TESFaction* pFaction = (&listFactions)->m_item;
        if (!pFaction) {
            break;
        }
        listFactions.RemoveHead();
        delete pFaction;
    }
    while (true) {
        TESChallenge* pChallenge = (&listChallenges)->m_item;
        if (!pChallenge) {
            break;
        }
        listChallenges.RemoveHead();
        delete pChallenge;
    }
    while (true) {
        TESReputation* pRep = (&listReputations)->m_item;
        if (!pRep) {
            break;
        }
        listReputations.RemoveHead();
        delete pRep;
    }
    while (true) {
        TESRecipe* pRecipe = (&listRecipes)->m_item;
        if (!pRecipe) {
            break;
        }
        listRecipes.RemoveHead();
        delete pRecipe;
    }
    while (true) {
        TESCasino* pCasino = (&listCasinos)->m_item;
        if (!pCasino) {
            break;
        }
        listCasinos.RemoveHead();
        delete pCasino;
    }
    while (true) {
        TESRecipeCategory* pRecipeCat = static_cast<TESRecipeCategory*>((&listRecipeCategories)->m_item);
        if (!pRecipeCat) {
            break;
        }
        listRecipeCategories.RemoveHead();
        delete pRecipeCat;
    }
    while (true) {
        TESAmmoEffect* pAmmoEffect = static_cast<TESAmmoEffect*>((&listAmmoEffects)->m_item);
        if (!pAmmoEffect) {
            break;
        }
        listAmmoEffects.RemoveHead();
        delete pAmmoEffect;
    }
    while (true) {
        TESGlobal* pGlobal = (&listGlobals)->m_item;
        if (!pGlobal) {
            break;
        }
        listGlobals.RemoveHead();
        delete pGlobal;
    }
    while (true) {
        BGSVoiceType* pVoiceType = (&listVoiceTypes)->m_item;
        if (!pVoiceType) {
            break;
        }
        listVoiceTypes.RemoveHead();
        delete pVoiceType;
    }
    while (true) {
        BGSImpactData* pimpdata = (&listImpactData)->m_item;
        if (!pimpdata) {
            break;
        }
        listImpactData.RemoveHead();
        delete pimpdata;
    }
    while (true) {
        BGSImpactDataSet* pimpdataset = (&listImpactDataSet)->m_item;
        if (!pimpdataset) {
            break;
        }
        listImpactDataSet.RemoveHead();
        delete pimpdataset;
    }
    while (true) {
        TESQuest* pQuest = (&listQuests)->m_item;
        if (!pQuest) {
            break;
        }
        listQuests.RemoveHead();
        delete pQuest;
    }
    while (true) {
        TESTopic* pTopic = (&listTopics)->m_item;
        if (!pTopic) {
            break;
        }
        listTopics.RemoveHead();
        delete pTopic;
    }
    TESTopic::RemoveDefaultTopics();
    while ((&listTopicInfos)->m_item) {
        listTopicInfos.RemoveHead();
    }
    int32_t iSize = arrayInteriorCells.m_usSize;
    for (int32_t i = 0; i < iSize; ++i) {
        TESObjectCELL* pCell = arrayInteriorCells[i];
        if (pCell) {
            delete pCell;
        }
    }
    arrayInteriorCells.SetSize(0);
    while (true) {
        TESWorldSpace* pWorld = (&listWorldSpaces)->m_item;
        if (!pWorld) {
            break;
        }
        listWorldSpaces.RemoveHead();
        delete pWorld;
    }
    GarbageCollector::ClearAll(false);
    while (true) {
        TESSound* pSound = (&listSounds)->m_item;
        if (!pSound) {
            break;
        }
        listSounds.RemoveHead();
        delete pSound;
    }
    while (true) {
        BGSMusicType* pmusic = GetMusicTypeList()->m_item;
        if (!pmusic) {
            break;
        }
        GetMusicTypeList()->RemoveHead();
        delete pmusic;
    }
    while (true) {
        MediaSet* pMediaSet = GetMediaSetList()->m_item;
        if (!pMediaSet) {
            break;
        }
        GetMediaSetList()->RemoveHead();
        delete pMediaSet;
    }
    while (true) {
        MediaLocationController* pMediaLocationController = GetMediaLocationControllerList()->m_item;
        if (!pMediaLocationController) {
            break;
        }
        GetMediaLocationControllerList()->RemoveHead();
        delete pMediaLocationController;
    }
    while (true) {
        BGSAcousticSpace* paSpc = (&listAcousticSpaces)->m_item;
        if (!paSpc) {
            break;
        }
        listAcousticSpaces.RemoveHead();
        delete paSpc;
    }
    while (true) {
        TESLandTexture* pLandText = (&listLandTexts)->m_item;
        if (!pLandText) {
            break;
        }
        listLandTexts.RemoveHead();
        delete pLandText;
    }
    while (true) {
        BGSMessage* pMessage = (&listMessages)->m_item;
        if (!pMessage) {
            break;
        }
        listMessages.RemoveHead();
        delete pMessage;
    }
    if (pPlayer) {
        delete pPlayer;
    }
    pPlayer = nullptr;
    while (true) {
        TESImageSpace* pimagespace = static_cast<TESImageSpace*>((&listImageSpaces)->m_item);
        if (!pimagespace) {
            break;
        }
        listImageSpaces.RemoveHead();
        delete pimagespace;
    }
    while (true) {
        TESImageSpaceModifier* pimagespacemodifier =
            static_cast<TESImageSpaceModifier*>((&listImageSpaceModifiers)->m_item);
        if (!pimagespacemodifier) {
            break;
        }
        listImageSpaceModifiers.RemoveHead();
        delete pimagespacemodifier;
    }
    pObjectList->lnk_Destroy();
    arrayAddonNodes.RemoveAll();
    while (true) {
        SpellItem* pSpellItem = static_cast<SpellItem*>((&listSpellItems)->m_item);
        if (!pSpellItem) {
            break;
        }
        listSpellItems.RemoveHead();
        delete pSpellItem;
    }
    while (true) {
        EnchantmentItem* pEnchItem = static_cast<EnchantmentItem*>((&listEnchantmentItems)->m_item);
        if (!pEnchItem) {
            break;
        }
        listEnchantmentItems.RemoveHead();
        delete pEnchItem;
    }
    while (true) {
        TESPackage* ppack = static_cast<TESPackage*>((&listPackages)->m_item);
        if (!ppack) {
            break;
        }
        listPackages.RemoveHead();
        delete ppack;
    }
    while (true) {
        TESCombatStyle* pcombatStyle = (&listCombatStyles)->m_item;
        if (!pcombatStyle) {
            break;
        }
        listCombatStyles.RemoveHead();
        delete pcombatStyle;
    }
    while (true) {
        TESLoadScreen* ploadscreen = (&listLoadScreens)->m_item;
        if (!ploadscreen) {
            break;
        }
        listLoadScreens.RemoveHead();
        delete ploadscreen;
    }
    while (true) {
        TESLoadScreenType* pLST = (&listLoadScreenTypes)->m_item;
        if (!pLST) {
            break;
        }
        listLoadScreenTypes.RemoveHead();
        delete pLST;
    }
    while (true) {
        TESWaterForm* pwater = (&listWater)->m_item;
        if (!pwater) {
            break;
        }
        listWater.RemoveHead();
        delete pwater;
    }
    pDefaultWater = nullptr;
    while (true) {
        TESObjectANIO* paniObj = (&animObjects)->m_item;
        if (!paniObj) {
            break;
        }
        animObjects.RemoveHead();
        delete paniObj;
    }
    while (true) {
        TESEffectShader* peffectShader = (&listEffectShaders)->m_item;
        if (!peffectShader) {
            break;
        }
        listEffectShaders.RemoveHead();
        delete peffectShader;
    }
    while (true) {
        BGSProjectile* pprojectile = (&listProjectiles)->m_item;
        if (!pprojectile) {
            break;
        }
        listProjectiles.RemoveHead();
        delete pprojectile;
    }
    while (true) {
        BGSExplosion* pexplosion = static_cast<BGSExplosion*>((&listExplosions)->m_item);
        if (!pexplosion) {
            break;
        }
        listExplosions.RemoveHead();
        delete pexplosion;
    }
    while (true) {
        BGSDebris* pdebris = (&listDebris)->m_item;
        if (!pdebris) {
            break;
        }
        listDebris.RemoveHead();
        delete pdebris;
    }
    while (true) {
        BGSPerk* pperk = (&listPerks)->m_item;
        if (!pperk) {
            break;
        }
        listPerks.RemoveHead();
        delete pperk;
    }
    while (true) {
        BGSRadiationStage* prad = (&listRadiation)->m_item;
        if (!prad) {
            break;
        }
        listRadiation.RemoveHead();
        delete prad;
    }
    while (true) {
        BGSDehydrationStage* ph20 = (&listDehydration)->m_item;
        if (!ph20) {
            break;
        }
        listDehydration.RemoveHead();
        delete ph20;
    }
    while (true) {
        BGSHungerStage* pFood = (&listHunger)->m_item;
        if (!pFood) {
            break;
        }
        listHunger.RemoveHead();
        delete pFood;
    }
    while (true) {
        BGSSleepDeprevationStage* pSleep = (&listSleepDeprevation)->m_item;
        if (!pSleep) {
            break;
        }
        listSleepDeprevation.RemoveHead();
        delete pSleep;
    }
    while (true) {
        BGSBodyPartData* ppartdata = (&listPartData)->m_item;
        if (!ppartdata) {
            break;
        }
        listPartData.RemoveHead();
        delete ppartdata;
    }
    pRegionList->Clear();
    NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
    Instance->ClearAllObstacles();
    if (TES::pInstance->GetNavMeshInfoMap()) {
        TES::pInstance->SetNavMeshInfoMap(nullptr);
    }
    if (appMain && appMain->bReloadContent) {
        pIdleManager->DestroyRootIdleArrayForms();
    }
    else {
        pIdleManager->DestroyRootIdleArrays();
    }
    pCameraPathManager->DestroyRootPathArrayForms();
    while (true) {
        BGSCameraShot* pshot = (&listCameraShots)->m_item;
        if (!pshot) {
            break;
        }
        listCameraShots.RemoveHead();
        delete pshot;
    }
    while (true) {
        BGSLightingTemplate* plt = (&listLightingTemplates)->m_item;
        if (!plt) {
            break;
        }
        listLightingTemplates.RemoveHead();
        delete plt;
    }
    EffectSettingCollection::Reset();
    ActorValue::FreeActorValues();
    while (true) {
        BGSNote* pnote = (&listNotes)->m_item;
        if (!pnote) {
            break;
        }
        listNotes.RemoveHead();
        delete pnote;
    }
    while (true) {
        BGSRagdoll* pragdoll = (&listRagdolls)->m_item;
        if (!pragdoll) {
            break;
        }
        listRagdolls.RemoveHead();
        delete pragdoll;
    }
    while (true) {
        TESCaravanDeck* pDeck = static_cast<TESCaravanDeck*>((&listCaravanDecks)->m_item);
        if (!pDeck) {
            break;
        }
        listCaravanDecks.RemoveHead();
        delete pDeck;
    }
    if (TESForm::pAllForms) {
        MessageHandler::IncDisableWarningCount(true);
        bool bLeaked = false;
        TESForm* pForm = nullptr;
        uint32_t ileakedID = 0;
        it = TESForm::pAllForms->GetFirstPos();
        while (it) {
            TESForm::pAllForms->GetNext(it, ileakedID, pForm);
            if (pForm) {
                if (pForm->GetFormType() == GMST_ID) {
                    pForm->ClearFiles();
                    pForm->SetFormID(0, true);
                }
                else {
                    const char* CellOrCoords;
                    if (pForm->GetFile(-1)) {
                        CellOrCoords = pForm->GetFile(-1)->m_Filename;
                    }
                    else {
                        CellOrCoords = "UNKNOWN";
                    }
                    const char* FormTypeString = pForm->GetFormTypeString();
                    int32_t CastingType = pForm->iFormID;
                    const char* v2 = pForm->GetFormEditorID();
                    Warning(
                        "FORMS: Form '%s' (%08X) of type %s in file '%s' was not freed.", v2, CastingType,
                        FormTypeString, CellOrCoords
                    );
                    TESForm::pAllForms->SetAt(ileakedID, 0);
                    bLeaked = true;
                }
            }
        }
        MessageHandler::IncDisableWarningCount(false);
        if (bLeaked) {
            Warning("FORMS: Forms were leaked during ClearData. Check Warnings file for more info.");
        }
    }
    for (uint32_t index = 0; index < iNumCompile; ++index) {
        pFileIndex[index] = nullptr;
    }
    iNumCompile = 0;
    if ((!bSaveLoadGame) && (this->pActiveFile)) {
        if (!GetMasterSave()) {
            this->pActiveFile->SetActive(false);
        }
        if (this->pActiveFile) {
            TESFile* pActiveFile = this->pActiveFile;
            if (pActiveFile) {
                delete pActiveFile;
            }
        }
        this->pActiveFile = nullptr;
        iNextID = 2048;
    }

    BGSImpactDataSet::ClearDefaultImpactDataSet();
    pDoorMarker = nullptr;
    pMapMarker = nullptr;
    pAudioMarker = nullptr;
    pAudioBuoyMarker = nullptr;
    pBoundMarker = nullptr;
    pPlaneMarker = nullptr;
    pRoomMarker = nullptr;
    pPortalMarker = nullptr;
    pCollisionMarker = nullptr;
    pXMarker = nullptr;
    pXMarkerHeading = nullptr;
    pTravelMarker = nullptr;
    pNorthMarker = nullptr;
    pJailMarker = nullptr;
    pTempleMarker = nullptr;
    pHolyMarker = nullptr;
    pLockpick = nullptr;
    pHorseMarker = nullptr;
    bClearingData = false;
    pItemDetectedShader = nullptr;
    pSpotterShader = nullptr;
    pCateyeMobileShader = nullptr;
    return true;
}

void TESDataHandler::SortInteriorCells() {
    arrayInteriorCells.Compact();
    int32_t icellCount = arrayInteriorCells.m_usESize;
    for (int32_t iouter = 0; iouter < icellCount - 1; ++iouter) {
        TESObjectCELL* poutterCell = arrayInteriorCells[iouter];
        TESObjectCELL* pcurrentCell = poutterCell;
        int32_t icurrent = iouter;
        for (int32_t iinner = iouter + 1; iinner < icellCount; ++iinner) {
            TESObjectCELL* pinnerCell = arrayInteriorCells[iinner];
            if (pcurrentCell && pinnerCell) {
                const char* CellName = pcurrentCell->GetCellName();
                const char* v1 = pinnerCell->GetCellName();
                if (BSstricmp(v1, CellName) < 0) {
                    pcurrentCell = pinnerCell;
                    icurrent = iinner;
                }
            }
            else {
                if (!pcurrentCell) {
                    pcurrentCell = pinnerCell;
                    icurrent = iinner;
                }
            }
        }
        if ((pcurrentCell) && (pcurrentCell != poutterCell)) {
            arrayInteriorCells.SetAt(iouter, pcurrentCell);
            arrayInteriorCells.SetAt(icurrent, poutterCell);
        }
    }
}

void TESDataHandler::CleanUpBadForms() {
    if (listBadForms.IsEmpty() || !TESForm::bFormIDsBashed) {
        bool bsave = GetClearingData();
        SetIsClosingFile(true);
        NiTListItem<TESForm*>* iter = listBadForms.m_pkHead;
        while (iter) {
            TESForm* pform = listBadForms.GetNext(iter);
            if (pform) {
                delete pform;
            }
        }
        listBadForms.RemoveAll();
        SetIsClosingFile(bsave);
    }
    else {
        TESDataHandler::RegisterError();
        Warning(
            "MASTERFILE: Invalid forms were encountered on load and need to be removed from the master file. "
            "Contact a programmer immediately to fix any problems in the file."
        );
    }
}

TESObjectREFR* TESDataHandler::CreateExplosionAtLocation(
    TESBoundObject* apObject,
    const NiPoint3& aLocation,
    const NiPoint3& aDirection,
    TESObjectCELL* apInterior,
    TESWorldSpace* apWorld
) {
    TESObjectREFR* pref = nullptr;
    if ((apObject) && (apObject->GetFormType() == EXPL_ID)) {
        BGSExplosion* pexplosionForm = static_cast<BGSExplosion*>(apObject);
        TESObjectCELL* pnewparent = nullptr;
        if (apWorld) {
            int32_t icellx = FloatToInt(aLocation.x) >> 12;
            int32_t icelly = FloatToInt(aLocation.y) >> 12;
            pnewparent = apWorld->GetCellFromCellCoord(icellx, icelly);
            if (!pnewparent && apWorld != pPlayer->GetWorldSpace()) {
                Warning("CELLS: Trying to create an explosion in an unloaded cell: %i, %i.", icellx, icelly);
            }
        }
        else {
            pnewparent = apInterior;
        }
        NiMatrix3 rot;
        rot.FromEulerAnglesZYX(aDirection.z, aDirection.y, aDirection.x);
        TESObjectREFR* pOwner = nullptr;
        uint32_t uiFormID = apObject->iFormID;
        // The player owns the explosions of these three explosion forms.
        if (uiFormID == 0xCDEF0 || uiFormID == 0x16AF00 || uiFormID == 0x172FBD) {
            pOwner = pPlayer;
        }
        pref = Explosion::SpawnExplosion(pexplosionForm, pOwner, nullptr, pnewparent, aLocation, rot);
        if ((!pnewparent) && (apWorld)) {
            pref->SetRefPersists(true);
            pref->m_Extra.SetPersistentCell(apWorld->pPersistentCell);
        }
    }

    return pref;
}

void TESDataHandler::CheckIcons() {
    NiTMapItem<uint32_t, TESForm*>* it;
    if (!bFileSkipIconChecks.Bool() && TESForm::pAllForms) {
        TESForm* pform = nullptr;
        it = TESForm::pAllForms->GetFirstPos();
        uint32_t ignore = 0;
        while (true) {
            do {
                if (!it) {
                    return;
                }
                TESForm::pAllForms->GetNext(it, ignore, pform);
            }
            while (!pform);
            TESIcon* picon = dynamic_cast<TESIcon*>(pform);
            TESTexture* ptexture = dynamic_cast<TESTexture*>(pform);
            TESBipedModelForm* pbiped = dynamic_cast<TESBipedModelForm*>(pform);
            if (picon) {
                if (bFileShowIcons.Bool() &&
                    (pform->GetFormType() != LIGH_ID ||
                     static_cast<TESObjectLIGH*>(pform)->TESObjectLIGH::GetCanCarry())) {
                    char apDest[268];
                    if (!picon->GetTextureNameLength()) {
                        goto LABEL_11;
                    }
                    const char* TextureName = picon->GetTextureName();
                    BSsprintf(apDest, 260, "%s%s", picon->GetDefaultPath(), TextureName);
                    if (FileFinder::Locate(apDest, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                        const char* v10 = pform->GetFormEditorID();
                        uint8_t FormType = pform->GetFormType();
                        char* v3 = FormIDToString(FormType);
                        Warning("FORMS: Icon '%s' missing for %s '%s'.", picon->GetTextureName(), v3, v10);
                    }
                }
            }
            else {
                if (ptexture) {
                    if (bFileShowTextures.Bool()) {
                        if (ptexture->GetTextureNameLength()) {
                            char apName[271];
                            if (pform->GetFormType() == LTEX_ID) {
                                const char* v12 = ptexture->GetTextureName();
                                BSsprintf(apName, 260, "%s\\Landscape\\%s", ptexture->GetDefaultPath(), v12);
                            }
                            else {
                                const char* v13 = ptexture->GetTextureName();
                                BSsprintf(apName, 260, "%s%s", ptexture->GetDefaultPath(), v13);
                            }
                            if (FileFinder::Locate(apName, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                                const char* v14 = pform->GetFormEditorID();
                                uint8_t v5 = pform->GetFormType();
                                char* v6 = FormIDToString(v5);
                                Warning(
                                    "TEXTURES: Texture '%s' missing for %s '%s'.", ptexture->GetTextureName(), v6, v14
                                );
                            }
                        }
                        else {
                            char v20 = 1;
                            int32_t v16 = pform->GetFormType();
                            if (v16 == 7) {
                                v20 = 0;
                            }
                            else {
                                if (v16 == 65) {
                                    v20 = 0;
                                }
                            }
                            if (v20) {
                                const char* v11 = pform->GetFormEditorID();
                                uint8_t v4 = pform->GetFormType();
                                Warning("TEXTURES: Texture missing for %s '%s'.", FormIDToString(v4), v11);
                            }
                        }
                    }
                }
                else {
                    if (pbiped && bFileShowIcons.Bool()) {
                        const char* piconFilename;
                        piconFilename = pbiped->GetIcon(TESActorBase::SEX_MALE);
                        if (BSstrlen(piconFilename)) {
                            char apString1[268];
                            BSsprintf(apString1, 260, "%s%s", pbiped->GetIconDefaultPath(), piconFilename);
                            if (FileFinder::Locate(apString1, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                                const char* v15 = pform->GetFormEditorID();
                                uint8_t v7 = pform->GetFormType();
                                Warning(
                                    "MENUS: Icon '%s' missing for %s '%s'.", piconFilename, FormIDToString(v7), v15
                                );
                            }
                        }
                        else {
                        LABEL_11:;
                            const char* v8 = pform->GetFormEditorID();
                            uint8_t v1 = pform->GetFormType();
                            Warning("FORMS: Icon missing for %s '%s'.", FormIDToString(v1), v8);
                        }
                    }
                }
            }
        }
    }
}

TESTopic* TESDataHandler::GetGlobalTopic(HardcodedFormIDs a1) {
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(static_cast<uint32_t>(a1));
    return dynamic_cast<TESTopic*>(FormByNumericID);
}

bool TESDataHandler::LoadForm(TESForm* pForm, TESFile* pFile) {
    const bool wasMaster = pForm->GetMaster();

    const bool loaded = pForm->Load(pFile);

    if (wasMaster) {
        pForm->SetMaster(true);
    }
    else {
        pForm->SetMaster(pFile->GetMaster());
    }

    if (pFile->GetActive()) {
        pForm->SetAltered(true);
    }

    return loaded;
}

// KEEP: a switch on aType (an ENUM_FORM_ID) under
// AutoMemContext(MEM_CONTEXT_TES_FORMS) that allocates each form (ourMalloc,
// NiNew for TXST/NAVM) and calls its out-of-line constructor, with Warning for
// unknown types. 92 form classes are not declared yet, and two cases need the
// multiple-inheritance upcast (BGSMovableStatic, TESFlora); revisit after the
// form-header wave (V3).
// The exe's cleanup-only C++ EH frame is left out.
TESForm* TESDataHandler::CreateFormOfType(uint8_t aType) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TES_FORMS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp", 7192
    );
    TESForm* pForm_3 = nullptr;
    switch (aType) {
        case 4: {
            TESForm* pForm_81;
            BGSTextureSet* v132 = static_cast<BGSTextureSet*>(NiMemObject::operator new(sizeof(BGSTextureSet)));
            if (v132) {
                pForm_81 = (TESForm*)::new (v132) BGSTextureSet;
            }
            else {
                pForm_81 = nullptr;
            }
            pForm_3 = pForm_81;
            break;
        }
        case 5: {
            TESForm* pForm_41;
            BGSMenuIcon* v172 = static_cast<BGSMenuIcon*>(ourMalloc(sizeof(BGSMenuIcon)));
            if (v172) {
                pForm_41 = (TESForm*)::new (v172) BGSMenuIcon;
            }
            else {
                pForm_41 = nullptr;
            }
            pForm_3 = pForm_41;
            break;
        }
        case 6: {
            TESForm* pForm_54;
            TESGlobal* v159 = static_cast<TESGlobal*>(ourMalloc(sizeof(TESGlobal)));
            if (v159) {
                pForm_54 = (TESForm*)::new (v159) TESGlobal;
            }
            else {
                pForm_54 = nullptr;
            }
            pForm_3 = pForm_54;
            break;
        }
        case 7: {
            TESForm* pForm_48;
            TESClass* v165 = static_cast<TESClass*>(ourMalloc(sizeof(TESClass)));
            if (v165) {
                pForm_48 = (TESForm*)::new (v165) TESClass;
            }
            else {
                pForm_48 = nullptr;
            }
            pForm_3 = pForm_48;
            break;
        }
        case 8: {
            TESForm* pForm_49;
            TESFaction* v164 = static_cast<TESFaction*>(ourMalloc(sizeof(TESFaction)));
            if (v164) {
                pForm_49 = (TESForm*)::new (v164) TESFaction;
            }
            else {
                pForm_49 = nullptr;
            }
            pForm_3 = pForm_49;
            break;
        }
        case 9: {
            TESForm* pForm_43;
            BGSHeadPart* v170 = static_cast<BGSHeadPart*>(ourMalloc(sizeof(BGSHeadPart)));
            if (v170) {
                pForm_43 = (TESForm*)::new (v170) BGSHeadPart;
            }
            else {
                pForm_43 = nullptr;
            }
            pForm_3 = pForm_43;
            break;
        }
        case 10: {
            TESForm* pForm_44;
            TESHair* v169 = static_cast<TESHair*>(ourMalloc(sizeof(TESHair)));
            if (v169) {
                pForm_44 = (TESForm*)::new (v169) TESHair;
            }
            else {
                pForm_44 = nullptr;
            }
            pForm_3 = pForm_44;
            break;
        }
        case 11: {
            TESForm* pForm_45;
            TESEyes* v168 = static_cast<TESEyes*>(ourMalloc(sizeof(TESEyes)));
            if (v168) {
                pForm_45 = (TESForm*)::new (v168) TESEyes;
            }
            else {
                pForm_45 = nullptr;
            }
            pForm_3 = pForm_45;
            break;
        }
        case 12: {
            TESForm* pForm_46;
            TESRace* v167 = static_cast<TESRace*>(ourMalloc(sizeof(TESRace)));
            if (v167) {
                pForm_46 = (TESForm*)::new (v167) TESRace;
            }
            else {
                pForm_46 = nullptr;
            }
            pForm_3 = pForm_46;
            break;
        }
        case 13: {
            TESForm* pForm_52;
            TESSound* v161 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
            if (v161) {
                pForm_52 = (TESForm*)::new (v161) TESSound;
            }
            else {
                pForm_52 = nullptr;
            }
            pForm_3 = pForm_52;
            break;
        }
        case 14: {
            TESForm* pForm_53;
            BGSAcousticSpace* v160 = static_cast<BGSAcousticSpace*>(ourMalloc(sizeof(BGSAcousticSpace)));
            if (v160) {
                pForm_53 = (TESForm*)::new (v160) BGSAcousticSpace;
            }
            else {
                pForm_53 = nullptr;
            }
            pForm_3 = pForm_53;
            break;
        }
        case 15: {
            TESForm* pForm_65;
            TESSkill* v148 = static_cast<TESSkill*>(ourMalloc(sizeof(TESSkill)));
            if (v148) {
                pForm_65 = (TESForm*)::new (v148) TESSkill;
            }
            else {
                pForm_65 = nullptr;
            }
            pForm_3 = pForm_65;
            break;
        }
        case 16: {
            TESForm* pForm_58;
            EffectSetting* v155 = static_cast<EffectSetting*>(ourMalloc(sizeof(EffectSetting)));
            if (v155) {
                pForm_58 = (TESForm*)::new (v155) EffectSetting;
            }
            else {
                pForm_58 = nullptr;
            }
            pForm_3 = pForm_58;
            break;
        }
        case 17: {
            TESForm* pForm_70;
            Script* v143 = static_cast<Script*>(ourMalloc(sizeof(Script)));
            if (v143) {
                pForm_70 = (TESForm*)::new (v143) Script;
            }
            else {
                pForm_70 = nullptr;
            }
            pForm_3 = pForm_70;
            break;
        }
        case 18: {
            TESForm* pForm_66;
            TESLandTexture* v147 = static_cast<TESLandTexture*>(ourMalloc(sizeof(TESLandTexture)));
            if (v147) {
                pForm_66 = (TESForm*)::new (v147) TESLandTexture;
            }
            else {
                pForm_66 = nullptr;
            }
            pForm_3 = pForm_66;
            break;
        }
        case 19: {
            TESForm* pForm_36;
            EnchantmentItem* v177 = static_cast<EnchantmentItem*>(ourMalloc(sizeof(EnchantmentItem)));
            if (v177) {
                pForm_36 = (TESForm*)::new (v177) EnchantmentItem;
            }
            else {
                pForm_36 = nullptr;
            }
            pForm_3 = pForm_36;
            break;
        }
        case 20: {
            TESForm* pForm_35;
            SpellItem* v178 = static_cast<SpellItem*>(ourMalloc(sizeof(SpellItem)));
            if (v178) {
                pForm_35 = (TESForm*)::new (v178) SpellItem;
            }
            else {
                pForm_35 = nullptr;
            }
            pForm_3 = pForm_35;
            break;
        }
        case 21: {
            TESForm* pForm_4;
            TESObjectACTI* v211 = static_cast<TESObjectACTI*>(ourMalloc(sizeof(TESObjectACTI)));
            if (v211) {
                pForm_4 = (TESForm*)::new (v211) TESObjectACTI;
            }
            else {
                pForm_4 = nullptr;
            }
            pForm_3 = pForm_4;
            break;
        }
        case 22: {
            TESForm* pForm_5;
            BGSTalkingActivator* v210 = static_cast<BGSTalkingActivator*>(ourMalloc(sizeof(BGSTalkingActivator)));
            if (v210) {
                pForm_5 = (TESForm*)::new (v210) BGSTalkingActivator;
            }
            else {
                pForm_5 = nullptr;
            }
            pForm_3 = pForm_5;
            break;
        }
        case 23: {
            TESForm* pForm_6;
            BGSTerminal* v209 = static_cast<BGSTerminal*>(ourMalloc(sizeof(BGSTerminal)));
            if (v209) {
                pForm_6 = (TESForm*)::new (v209) BGSTerminal;
            }
            else {
                pForm_6 = nullptr;
            }
            pForm_3 = pForm_6;
            break;
        }
        case 24: {
            TESForm* pForm_7;
            TESObjectARMO* v208 = static_cast<TESObjectARMO*>(ourMalloc(sizeof(TESObjectARMO)));
            if (v208) {
                pForm_7 = (TESForm*)::new (v208) TESObjectARMO;
            }
            else {
                pForm_7 = nullptr;
            }
            pForm_3 = pForm_7;
            break;
        }
        case 25: {
            TESForm* pForm_9;
            TESObjectBOOK* v206 = static_cast<TESObjectBOOK*>(ourMalloc(sizeof(TESObjectBOOK)));
            if (v206) {
                pForm_9 = (TESForm*)::new (v206) TESObjectBOOK;
            }
            else {
                pForm_9 = nullptr;
            }
            pForm_3 = pForm_9;
            break;
        }
        case 26: {
            TESForm* pForm_10;
            TESObjectCLOT* v205 = static_cast<TESObjectCLOT*>(ourMalloc(sizeof(TESObjectCLOT)));
            if (v205) {
                pForm_10 = (TESForm*)::new (v205) TESObjectCLOT;
            }
            else {
                pForm_10 = nullptr;
            }
            pForm_3 = pForm_10;
            break;
        }
        case 27: {
            TESForm* pForm_11;
            TESObjectCONT* v204 = static_cast<TESObjectCONT*>(ourMalloc(sizeof(TESObjectCONT)));
            if (v204) {
                pForm_11 = (TESForm*)::new (v204) TESObjectCONT;
            }
            else {
                pForm_11 = nullptr;
            }
            pForm_3 = pForm_11;
            break;
        }
        case 28: {
            TESForm* pForm_12;
            TESObjectDOOR* v203 = static_cast<TESObjectDOOR*>(ourMalloc(sizeof(TESObjectDOOR)));
            if (v203) {
                pForm_12 = (TESForm*)::new (v203) TESObjectDOOR;
            }
            else {
                pForm_12 = nullptr;
            }
            pForm_3 = pForm_12;
            break;
        }
        case 29: {
            TESForm* pForm_13;
            IngredientItem* v202 = static_cast<IngredientItem*>(ourMalloc(sizeof(IngredientItem)));
            if (v202) {
                pForm_13 = (TESForm*)::new (v202) IngredientItem;
            }
            else {
                pForm_13 = nullptr;
            }
            pForm_3 = pForm_13;
            break;
        }
        case 30: {
            TESForm* pForm_14;
            TESObjectLIGH* v201 = static_cast<TESObjectLIGH*>(ourMalloc(sizeof(TESObjectLIGH)));
            if (v201) {
                pForm_14 = (TESForm*)::new (v201) TESObjectLIGH;
            }
            else {
                pForm_14 = nullptr;
            }
            pForm_3 = pForm_14;
            break;
        }
        case 31: {
            TESForm* pForm_15;
            TESObjectMISC* v200 = static_cast<TESObjectMISC*>(ourMalloc(sizeof(TESObjectMISC)));
            if (v200) {
                pForm_15 = (TESForm*)::new (v200) TESObjectMISC;
            }
            else {
                pForm_15 = nullptr;
            }
            pForm_3 = pForm_15;
            break;
        }
        case 32: {
            TESForm* pForm_19;
            TESObjectSTAT* v196 = static_cast<TESObjectSTAT*>(ourMalloc(sizeof(TESObjectSTAT)));
            if (v196) {
                pForm_19 = (TESForm*)::new (v196) TESObjectSTAT;
            }
            else {
                pForm_19 = nullptr;
            }
            pForm_3 = pForm_19;
            break;
        }
        case 33: {
            TESForm* pForm_20;
            BGSStaticCollection* v195 = static_cast<BGSStaticCollection*>(ourMalloc(sizeof(BGSStaticCollection)));
            if (v195) {
                pForm_20 = (TESForm*)::new (v195) BGSStaticCollection;
            }
            else {
                pForm_20 = nullptr;
            }
            pForm_3 = pForm_20;
            break;
        }
        case 34: {
            TESObjectSTAT* pForm;
            BGSMovableStatic* v90;
            BGSMovableStatic* v194 = static_cast<BGSMovableStatic*>(ourMalloc(sizeof(BGSMovableStatic)));
            if (v194) {
                v90 = ::new (v194) BGSMovableStatic;
            }
            else {
                v90 = nullptr;
            }
            if (v90) {
                pForm = static_cast<TESObjectSTAT*>(v90);
            }
            else {
                pForm = nullptr;
            }
            pForm_3 = (TESForm*)pForm;
            break;
        }
        case 35: {
            TESForm* pForm_21;
            BGSPlaceableWater* v193 = static_cast<BGSPlaceableWater*>(ourMalloc(sizeof(BGSPlaceableWater)));
            if (v193) {
                pForm_21 = (TESForm*)::new (v193) BGSPlaceableWater;
            }
            else {
                pForm_21 = nullptr;
            }
            pForm_3 = pForm_21;
            break;
        }
        case 36: {
            TESForm* pForm_22;
            TESGrass* v192 = static_cast<TESGrass*>(ourMalloc(sizeof(TESGrass)));
            if (v192) {
                pForm_22 = (TESForm*)::new (v192) TESGrass;
            }
            else {
                pForm_22 = nullptr;
            }
            pForm_3 = pForm_22;
            break;
        }
        case 37: {
            TESForm* pForm_27;
            TESObjectTREE* v187 = static_cast<TESObjectTREE*>(ourMalloc(sizeof(TESObjectTREE)));
            if (v187) {
                pForm_27 = (TESForm*)::new (v187) TESObjectTREE;
            }
            else {
                pForm_27 = nullptr;
            }
            pForm_3 = pForm_27;
            break;
        }
        case 38: {
            TESObjectACTI* pForm_2;
            TESFlora* v81;
            TESFlora* v186 = static_cast<TESFlora*>(ourMalloc(sizeof(TESFlora)));
            if (v186) {
                v81 = ::new (v186) TESFlora;
            }
            else {
                v81 = nullptr;
            }
            if (v81) {
                pForm_2 = static_cast<TESObjectACTI*>(v81);
            }
            else {
                pForm_2 = nullptr;
            }
            pForm_3 = (TESForm*)pForm_2;
            break;
        }
        case 39: {
            TESForm* pForm_28;
            TESFurniture* v185 = static_cast<TESFurniture*>(ourMalloc(sizeof(TESFurniture)));
            if (v185) {
                pForm_28 = (TESForm*)::new (v185) TESFurniture;
            }
            else {
                pForm_28 = nullptr;
            }
            pForm_3 = pForm_28;
            break;
        }
        case 40: {
            TESForm* pForm_29;
            TESObjectWEAP* v184 = static_cast<TESObjectWEAP*>(ourMalloc(sizeof(TESObjectWEAP)));
            if (v184) {
                pForm_29 = (TESForm*)::new (v184) TESObjectWEAP;
            }
            else {
                pForm_29 = nullptr;
            }
            pForm_3 = pForm_29;
            break;
        }
        case 41: {
            TESForm* pForm_30;
            TESAmmo* v183 = static_cast<TESAmmo*>(ourMalloc(sizeof(TESAmmo)));
            if (v183) {
                pForm_30 = (TESForm*)::new (v183) TESAmmo;
            }
            else {
                pForm_30 = nullptr;
            }
            pForm_3 = pForm_30;
            break;
        }
        case 42: {
            TESForm* pForm_31;
            TESNPC* v182 = static_cast<TESNPC*>(ourMalloc(sizeof(TESNPC)));
            if (v182) {
                pForm_31 = (TESForm*)::new (v182) TESNPC;
            }
            else {
                pForm_31 = nullptr;
            }
            pForm_3 = pForm_31;
            break;
        }
        case 43: {
            TESForm* pForm_32;
            TESCreature* v181 = static_cast<TESCreature*>(ourMalloc(sizeof(TESCreature)));
            if (v181) {
                pForm_32 = (TESForm*)::new (v181) TESCreature;
            }
            else {
                pForm_32 = nullptr;
            }
            pForm_3 = pForm_32;
            break;
        }
        case 44: {
            TESForm* pForm_33;
            TESLevCreature* v180 = static_cast<TESLevCreature*>(ourMalloc(sizeof(TESLevCreature)));
            if (v180) {
                pForm_33 = (TESForm*)::new (v180) TESLevCreature;
            }
            else {
                pForm_33 = nullptr;
            }
            pForm_3 = pForm_33;
            break;
        }
        case 45: {
            TESForm* pForm_34;
            TESLevCharacter* v179 = static_cast<TESLevCharacter*>(ourMalloc(sizeof(TESLevCharacter)));
            if (v179) {
                pForm_34 = (TESForm*)::new (v179) TESLevCharacter;
            }
            else {
                pForm_34 = nullptr;
            }
            pForm_3 = pForm_34;
            break;
        }
        case 46: {
            TESForm* pForm_18;
            TESKey* v197 = static_cast<TESKey*>(ourMalloc(sizeof(TESKey)));
            if (v197) {
                pForm_18 = (TESForm*)::new (v197) TESKey;
            }
            else {
                pForm_18 = nullptr;
            }
            pForm_3 = pForm_18;
            break;
        }
        case 47: {
            TESForm* pForm_37;
            AlchemyItem* v176 = static_cast<AlchemyItem*>(ourMalloc(sizeof(AlchemyItem)));
            if (v176) {
                pForm_37 = (TESForm*)::new (v176) AlchemyItem;
            }
            else {
                pForm_37 = nullptr;
            }
            pForm_3 = pForm_37;
            break;
        }
        case 48: {
            TESForm* pForm_25;
            BGSIdleMarker* v189 = static_cast<BGSIdleMarker*>(ourMalloc(sizeof(BGSIdleMarker)));
            if (v189) {
                pForm_25 = (TESForm*)::new (v189) BGSIdleMarker;
            }
            else {
                pForm_25 = nullptr;
            }
            pForm_3 = pForm_25;
            break;
        }
        case 49: {
            TESForm* pForm_40;
            BGSNote* v173 = static_cast<BGSNote*>(ourMalloc(sizeof(BGSNote)));
            if (v173) {
                pForm_40 = (TESForm*)::new (v173) BGSNote;
            }
            else {
                pForm_40 = nullptr;
            }
            pForm_3 = pForm_40;
            break;
        }
        case 50: {
            TESForm* pForm_17;
            BGSConstructibleObject* v198 =
                static_cast<BGSConstructibleObject*>(ourMalloc(sizeof(BGSConstructibleObject)));
            if (v198) {
                pForm_17 = (TESForm*)::new (v198) BGSConstructibleObject;
            }
            else {
                pForm_17 = nullptr;
            }
            pForm_3 = pForm_17;
            break;
        }
        case 51: {
            TESForm* pForm_78;
            BGSProjectile* v135 = static_cast<BGSProjectile*>(ourMalloc(sizeof(BGSProjectile)));
            if (v135) {
                pForm_78 = (TESForm*)::new (v135) BGSProjectile;
            }
            else {
                pForm_78 = nullptr;
            }
            pForm_3 = pForm_78;
            break;
        }
        case 52: {
            TESForm* pForm_38;
            TESLevItem* v175 = static_cast<TESLevItem*>(ourMalloc(sizeof(TESLevItem)));
            if (v175) {
                pForm_38 = (TESForm*)::new (v175) TESLevItem;
            }
            else {
                pForm_38 = nullptr;
            }
            pForm_3 = pForm_38;
            break;
        }
        case 53: {
            TESForm* pForm_62;
            TESWeather* v151 = static_cast<TESWeather*>(ourMalloc(sizeof(TESWeather)));
            if (v151) {
                pForm_62 = (TESForm*)::new (v151) TESWeather;
            }
            else {
                pForm_62 = nullptr;
            }
            pForm_3 = pForm_62;
            break;
        }
        case 54: {
            TESForm* pForm_59;
            TESClimate* v154 = static_cast<TESClimate*>(ourMalloc(sizeof(TESClimate)));
            if (v154) {
                pForm_59 = (TESForm*)::new (v154) TESClimate;
            }
            else {
                pForm_59 = nullptr;
            }
            pForm_3 = pForm_59;
            break;
        }
        case 55: {
            TESForm* pForm_69;
            TESRegion* v144 = static_cast<TESRegion*>(ourMalloc(sizeof(TESRegion)));
            if (v144) {
                pForm_69 = (TESForm*)::new (v144) TESRegion;
            }
            else {
                pForm_69 = nullptr;
            }
            pForm_3 = pForm_69;
            break;
        }
        case 57: {
            TESForm* pForm_64;
            TESObjectCELL* v149 = static_cast<TESObjectCELL*>(ourMalloc(sizeof(TESObjectCELL)));
            if (v149) {
                pForm_64 = (TESForm*)::new (v149) TESObjectCELL;
            }
            else {
                pForm_64 = nullptr;
            }
            pForm_3 = pForm_64;
            break;
        }
        case 58: {
            TESForm* pForm_39;
            TESObjectREFR* v174 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
            if (v174) {
                pForm_39 = (TESForm*)::new (v174) TESObjectREFR;
            }
            else {
                pForm_39 = nullptr;
            }
            pForm_3 = pForm_39;
            break;
        }
        case 65: {
            TESForm* pForm_63;
            TESWorldSpace* v150 = static_cast<TESWorldSpace*>(ourMalloc(sizeof(TESWorldSpace)));
            if (v150) {
                pForm_63 = (TESForm*)::new (v150) TESWorldSpace;
            }
            else {
                pForm_63 = nullptr;
            }
            pForm_3 = pForm_63;
            break;
        }
        case 66: {
            TESForm* pForm_76;
            TESObjectLAND* v137 = static_cast<TESObjectLAND*>(ourMalloc(sizeof(TESObjectLAND)));
            if (v137) {
                pForm_76 = (TESForm*)::new (v137) TESObjectLAND;
            }
            else {
                pForm_76 = nullptr;
            }
            pForm_3 = pForm_76;
            break;
        }
        case 67: {
            TESForm* pForm_82;
            NavMesh* v131 = static_cast<NavMesh*>(NiMemObject::operator new(sizeof(NavMesh)));
            if (v131) {
                pForm_82 = (TESForm*)::new (v131) NavMesh;
            }
            else {
                pForm_82 = nullptr;
            }
            pForm_3 = pForm_82;
            break;
        }
        case 71: {
            TESForm* pForm_68;
            TESQuest* v145 = static_cast<TESQuest*>(ourMalloc(sizeof(TESQuest)));
            if (v145) {
                pForm_68 = (TESForm*)::new (v145) TESQuest;
            }
            else {
                pForm_68 = nullptr;
            }
            pForm_3 = pForm_68;
            break;
        }
        case 72: {
            TESForm* pForm_23;
            TESIdleForm* v191 = static_cast<TESIdleForm*>(ourMalloc(sizeof(TESIdleForm)));
            if (v191) {
                pForm_23 = (TESForm*)::new (v191) TESIdleForm;
            }
            else {
                pForm_23 = nullptr;
            }
            pForm_3 = pForm_23;
            break;
        }
        case 73: {
            TESForm* pForm_26;
            TESPackage* v188 = static_cast<TESPackage*>(ourMalloc(sizeof(TESPackage)));
            if (v188) {
                pForm_26 = (TESForm*)::new (v188) TESPackage;
            }
            else {
                pForm_26 = nullptr;
            }
            pForm_3 = pForm_26;
            break;
        }
        case 74: {
            TESForm* pForm_71;
            TESCombatStyle* v142 = static_cast<TESCombatStyle*>(ourMalloc(sizeof(TESCombatStyle)));
            if (v142) {
                pForm_71 = (TESForm*)::new (v142) TESCombatStyle;
            }
            else {
                pForm_71 = nullptr;
            }
            pForm_3 = pForm_71;
            break;
        }
        case 75: {
            TESForm* pForm_72;
            TESLoadScreen* v141 = static_cast<TESLoadScreen*>(ourMalloc(sizeof(TESLoadScreen)));
            if (v141) {
                pForm_72 = (TESForm*)::new (v141) TESLoadScreen;
            }
            else {
                pForm_72 = nullptr;
            }
            pForm_3 = pForm_72;
            break;
        }
        case 76: {
            TESForm* pForm_74;
            TESLevSpell* v139 = static_cast<TESLevSpell*>(ourMalloc(sizeof(TESLevSpell)));
            if (v139) {
                pForm_74 = (TESForm*)::new (v139) TESLevSpell;
            }
            else {
                pForm_74 = nullptr;
            }
            pForm_3 = pForm_74;
            break;
        }
        case 77: {
            TESForm* pForm_75;
            TESObjectANIO* v138 = static_cast<TESObjectANIO*>(ourMalloc(sizeof(TESObjectANIO)));
            if (v138) {
                pForm_75 = (TESForm*)::new (v138) TESObjectANIO;
            }
            else {
                pForm_75 = nullptr;
            }
            pForm_3 = pForm_75;
            break;
        }
        case 78: {
            TESForm* pForm_73;
            TESWaterForm* v140 = static_cast<TESWaterForm*>(ourMalloc(sizeof(TESWaterForm)));
            if (v140) {
                pForm_73 = (TESForm*)::new (v140) TESWaterForm;
            }
            else {
                pForm_73 = nullptr;
            }
            pForm_3 = pForm_73;
            break;
        }
        case 79: {
            TESForm* pForm_77;
            TESEffectShader* v136 = static_cast<TESEffectShader*>(ourMalloc(sizeof(TESEffectShader)));
            if (v136) {
                pForm_77 = (TESForm*)::new (v136) TESEffectShader;
            }
            else {
                pForm_77 = nullptr;
            }
            pForm_3 = pForm_77;
            break;
        }
        case 81: {
            TESForm* pForm_79;
            BGSExplosion* v134 = static_cast<BGSExplosion*>(ourMalloc(sizeof(BGSExplosion)));
            if (v134) {
                pForm_79 = (TESForm*)::new (v134) BGSExplosion;
            }
            else {
                pForm_79 = nullptr;
            }
            pForm_3 = pForm_79;
            break;
        }
        case 82: {
            TESForm* pForm_80;
            BGSDebris* v133 = static_cast<BGSDebris*>(ourMalloc(sizeof(BGSDebris)));
            if (v133) {
                pForm_80 = (TESForm*)::new (v133) BGSDebris;
            }
            else {
                pForm_80 = nullptr;
            }
            pForm_3 = pForm_80;
            break;
        }
        case 83: {
            TESForm* pForm_60;
            TESImageSpace* v153 = static_cast<TESImageSpace*>(ourMalloc(sizeof(TESImageSpace)));
            if (v153) {
                pForm_60 = (TESForm*)::new (v153) TESImageSpace;
            }
            else {
                pForm_60 = nullptr;
            }
            pForm_3 = pForm_60;
            break;
        }
        case 84: {
            TESForm* pForm_61;
            TESImageSpaceModifier* v152 = static_cast<TESImageSpaceModifier*>(ourMalloc(sizeof(TESImageSpaceModifier)));
            if (v152) {
                pForm_61 = (TESForm*)::new (v152) TESImageSpaceModifier;
            }
            else {
                pForm_61 = nullptr;
            }
            pForm_3 = pForm_61;
            break;
        }
        case 85: {
            TESForm* pForm_42;
            BGSListForm* v171 = static_cast<BGSListForm*>(ourMalloc(sizeof(BGSListForm)));
            if (v171) {
                pForm_42 = (TESForm*)::new (v171) BGSListForm;
            }
            else {
                pForm_42 = nullptr;
            }
            pForm_3 = pForm_42;
            break;
        }
        case 86: {
            TESForm* pForm_83;
            BGSPerk* v130 = static_cast<BGSPerk*>(ourMalloc(sizeof(BGSPerk)));
            if (v130) {
                pForm_83 = (TESForm*)::new (v130) BGSPerk;
            }
            else {
                pForm_83 = nullptr;
            }
            pForm_3 = pForm_83;
            break;
        }
        case 87: {
            TESForm* pForm_84;
            BGSBodyPartData* v129 = static_cast<BGSBodyPartData*>(ourMalloc(sizeof(BGSBodyPartData)));
            if (v129) {
                pForm_84 = (TESForm*)::new (v129) BGSBodyPartData;
            }
            else {
                pForm_84 = nullptr;
            }
            pForm_3 = pForm_84;
            break;
        }
        case 88: {
            TESForm* pForm_85;
            BGSAddonNode* v128 = static_cast<BGSAddonNode*>(ourMalloc(sizeof(BGSAddonNode)));
            if (v128) {
                pForm_85 = (TESForm*)::new (v128) BGSAddonNode;
            }
            else {
                pForm_85 = nullptr;
            }
            pForm_3 = pForm_85;
            break;
        }
        case 89: {
            TESForm* pForm_86;
            ActorValueInfo* v127 = static_cast<ActorValueInfo*>(ourMalloc(sizeof(ActorValueInfo)));
            if (v127) {
                pForm_86 = (TESForm*)::new (v127) ActorValueInfo(ActorValue::VALUE_NONE, nullptr);
            }
            else {
                pForm_86 = nullptr;
            }
            pForm_3 = pForm_86;
            break;
        }
        case 90: {
            TESForm* pForm_87;
            BGSRadiationStage* v126 = static_cast<BGSRadiationStage*>(ourMalloc(sizeof(BGSRadiationStage)));
            if (v126) {
                pForm_87 = (TESForm*)::new (v126) BGSRadiationStage;
            }
            else {
                pForm_87 = nullptr;
            }
            pForm_3 = pForm_87;
            break;
        }
        case 91: {
            TESForm* pForm_67;
            BGSCameraShot* v146 = static_cast<BGSCameraShot*>(ourMalloc(sizeof(BGSCameraShot)));
            if (v146) {
                pForm_67 = (TESForm*)::new (v146) BGSCameraShot;
            }
            else {
                pForm_67 = nullptr;
            }
            pForm_3 = pForm_67;
            break;
        }
        case 92: {
            TESForm* pForm_24;
            BGSCameraPath* v190 = static_cast<BGSCameraPath*>(ourMalloc(sizeof(BGSCameraPath)));
            if (v190) {
                pForm_24 = (TESForm*)::new (v190) BGSCameraPath;
            }
            else {
                pForm_24 = nullptr;
            }
            pForm_3 = pForm_24;
            break;
        }
        case 93: {
            TESForm* pForm_55;
            BGSVoiceType* v158 = static_cast<BGSVoiceType*>(ourMalloc(sizeof(BGSVoiceType)));
            if (v158) {
                pForm_55 = (TESForm*)::new (v158) BGSVoiceType;
            }
            else {
                pForm_55 = nullptr;
            }
            pForm_3 = pForm_55;
            break;
        }
        case 94: {
            TESForm* pForm_56;
            BGSImpactData* v157 = static_cast<BGSImpactData*>(ourMalloc(sizeof(BGSImpactData)));
            if (v157) {
                pForm_56 = (TESForm*)::new (v157) BGSImpactData;
            }
            else {
                pForm_56 = nullptr;
            }
            pForm_3 = pForm_56;
            break;
        }
        case 95: {
            TESForm* pForm_57;
            BGSImpactDataSet* v156 = static_cast<BGSImpactDataSet*>(ourMalloc(sizeof(BGSImpactDataSet)));
            if (v156) {
                pForm_57 = (TESForm*)::new (v156) BGSImpactDataSet;
            }
            else {
                pForm_57 = nullptr;
            }
            pForm_3 = pForm_57;
            break;
        }
        case 96: {
            TESForm* pForm_8;
            TESObjectARMA* v207 = static_cast<TESObjectARMA*>(ourMalloc(sizeof(TESObjectARMA)));
            if (v207) {
                pForm_8 = (TESForm*)::new (v207) TESObjectARMA;
            }
            else {
                pForm_8 = nullptr;
            }
            pForm_3 = pForm_8;
            break;
        }
        case 97: {
            TESForm* pForm_47;
            BGSEncounterZone* v166 = static_cast<BGSEncounterZone*>(ourMalloc(sizeof(BGSEncounterZone)));
            if (v166) {
                pForm_47 = (TESForm*)::new (v166) BGSEncounterZone;
            }
            else {
                pForm_47 = nullptr;
            }
            pForm_3 = pForm_47;
            break;
        }
        case 98: {
            TESForm* pForm_91;
            BGSMessage* v122 = static_cast<BGSMessage*>(ourMalloc(sizeof(BGSMessage)));
            if (v122) {
                pForm_91 = (TESForm*)::new (v122) BGSMessage;
            }
            else {
                pForm_91 = nullptr;
            }
            pForm_3 = pForm_91;
            break;
        }
        case 99: {
            TESForm* pForm_92;
            BGSRagdoll* v121 = static_cast<BGSRagdoll*>(ourMalloc(sizeof(BGSRagdoll)));
            if (v121) {
                pForm_92 = (TESForm*)::new (v121) BGSRagdoll;
            }
            else {
                pForm_92 = nullptr;
            }
            pForm_3 = pForm_92;
            break;
        }
        case 101: {
            TESForm* pForm_93;
            BGSLightingTemplate* v120 = static_cast<BGSLightingTemplate*>(ourMalloc(sizeof(BGSLightingTemplate)));
            if (v120) {
                pForm_93 = (TESForm*)::new (v120) BGSLightingTemplate;
            }
            else {
                pForm_93 = nullptr;
            }
            pForm_3 = pForm_93;
            break;
        }
        case 102: {
            TESForm* pForm_94;
            BGSMusicType* v119 = static_cast<BGSMusicType*>(ourMalloc(sizeof(BGSMusicType)));
            if (v119) {
                pForm_94 = (TESForm*)::new (v119) BGSMusicType;
            }
            else {
                pForm_94 = nullptr;
            }
            pForm_3 = pForm_94;
            break;
        }
        case 103: {
            TESForm* pForm_16;
            TESObjectIMOD* v199 = static_cast<TESObjectIMOD*>(ourMalloc(sizeof(TESObjectIMOD)));
            if (v199) {
                pForm_16 = (TESForm*)::new (v199) TESObjectIMOD;
            }
            else {
                pForm_16 = nullptr;
            }
            pForm_3 = pForm_16;
            break;
        }
        case 104: {
            TESForm* pForm_50;
            TESReputation* v163 = static_cast<TESReputation*>(ourMalloc(sizeof(TESReputation)));
            if (v163) {
                pForm_50 = (TESForm*)::new (v163) TESReputation;
            }
            else {
                pForm_50 = nullptr;
            }
            pForm_3 = pForm_50;
            break;
        }
        case 106: {
            TESForm* pForm_95;
            TESRecipe* v118 = static_cast<TESRecipe*>(ourMalloc(sizeof(TESRecipe)));
            if (v118) {
                pForm_95 = (TESForm*)::new (v118) TESRecipe;
            }
            else {
                pForm_95 = nullptr;
            }
            pForm_3 = pForm_95;
            break;
        }
        case 107: {
            TESForm* pForm_96;
            TESRecipeCategory* v117 = static_cast<TESRecipeCategory*>(ourMalloc(sizeof(TESRecipeCategory)));
            if (v117) {
                pForm_96 = (TESForm*)::new (v117) TESRecipeCategory;
            }
            else {
                pForm_96 = nullptr;
            }
            pForm_3 = pForm_96;
            break;
        }
        case 108: {
            TESForm* pForm_98;
            TESCasinoChips* v115 = static_cast<TESCasinoChips*>(ourMalloc(sizeof(TESCasinoChips)));
            if (v115) {
                pForm_98 = (TESForm*)::new (v115) TESCasinoChips;
            }
            else {
                pForm_98 = nullptr;
            }
            pForm_3 = pForm_98;
            break;
        }
        case 109: {
            TESForm* pForm_100;
            TESCasino* v113 = static_cast<TESCasino*>(ourMalloc(sizeof(TESCasino)));
            if (v113) {
                pForm_100 = (TESForm*)::new (v113) TESCasino;
            }
            else {
                pForm_100 = nullptr;
            }
            pForm_3 = pForm_100;
            break;
        }
        case 110: {
            TESForm* pForm_99;
            TESLoadScreenType* v114 = static_cast<TESLoadScreenType*>(ourMalloc(sizeof(TESLoadScreenType)));
            if (v114) {
                pForm_99 = (TESForm*)::new (v114) TESLoadScreenType;
            }
            else {
                pForm_99 = nullptr;
            }
            pForm_3 = pForm_99;
            break;
        }
        case 111: {
            TESForm* pForm_101;
            MediaSet* v112 = static_cast<MediaSet*>(ourMalloc(sizeof(MediaSet)));
            if (v112) {
                pForm_101 = (TESForm*)::new (v112) MediaSet;
            }
            else {
                pForm_101 = nullptr;
            }
            pForm_3 = pForm_101;
            break;
        }
        case 112: {
            TESForm* pForm_102;
            MediaLocationController* v111 =
                static_cast<MediaLocationController*>(ourMalloc(sizeof(MediaLocationController)));
            if (v111) {
                pForm_102 = (TESForm*)::new (v111) MediaLocationController;
            }
            else {
                pForm_102 = nullptr;
            }
            pForm_3 = pForm_102;
            break;
        }
        case 113: {
            TESForm* pForm_51;
            TESChallenge* v162 = static_cast<TESChallenge*>(ourMalloc(sizeof(TESChallenge)));
            if (v162) {
                pForm_51 = (TESForm*)::new (v162) TESChallenge;
            }
            else {
                pForm_51 = nullptr;
            }
            pForm_3 = pForm_51;
            break;
        }
        case 114: {
            TESForm* pForm_97;
            TESAmmoEffect* v116 = static_cast<TESAmmoEffect*>(ourMalloc(sizeof(TESAmmoEffect)));
            if (v116) {
                pForm_97 = (TESForm*)::new (v116) TESAmmoEffect;
            }
            else {
                pForm_97 = nullptr;
            }
            pForm_3 = pForm_97;
            break;
        }
        case 115: {
            TESForm* pForm_103;
            TESCaravanCard* v110 = static_cast<TESCaravanCard*>(ourMalloc(sizeof(TESCaravanCard)));
            if (v110) {
                pForm_103 = (TESForm*)::new (v110) TESCaravanCard;
            }
            else {
                pForm_103 = nullptr;
            }
            pForm_3 = pForm_103;
            break;
        }
        case 116: {
            TESForm* pForm_104;
            TESCaravanMoney* v109 = static_cast<TESCaravanMoney*>(ourMalloc(sizeof(TESCaravanMoney)));
            if (v109) {
                pForm_104 = (TESForm*)::new (v109) TESCaravanMoney;
            }
            else {
                pForm_104 = nullptr;
            }
            pForm_3 = pForm_104;
            break;
        }
        case 117: {
            TESForm* pForm_105;
            TESCaravanDeck* v108 = static_cast<TESCaravanDeck*>(ourMalloc(sizeof(TESCaravanDeck)));
            if (v108) {
                pForm_105 = (TESForm*)::new (v108) TESCaravanDeck;
            }
            else {
                pForm_105 = nullptr;
            }
            pForm_3 = pForm_105;
            break;
        }
        case 118: {
            TESForm* pForm_88;
            BGSDehydrationStage* v125 = static_cast<BGSDehydrationStage*>(ourMalloc(sizeof(BGSDehydrationStage)));
            if (v125) {
                pForm_88 = (TESForm*)::new (v125) BGSDehydrationStage;
            }
            else {
                pForm_88 = nullptr;
            }
            pForm_3 = pForm_88;
            break;
        }
        case 119: {
            TESForm* pForm_89;
            BGSHungerStage* v124 = static_cast<BGSHungerStage*>(ourMalloc(sizeof(BGSHungerStage)));
            if (v124) {
                pForm_89 = (TESForm*)::new (v124) BGSHungerStage;
            }
            else {
                pForm_89 = nullptr;
            }
            pForm_3 = pForm_89;
            break;
        }
        case 120: {
            TESForm* pForm_90;
            BGSSleepDeprevationStage* v123 =
                static_cast<BGSSleepDeprevationStage*>(ourMalloc(sizeof(BGSSleepDeprevationStage)));
            if (v123) {
                pForm_90 = (TESForm*)::new (v123) BGSSleepDeprevationStage;
            }
            else {
                pForm_90 = nullptr;
            }
            pForm_3 = pForm_90;
            break;
        }
        default: {
            Warning(
                "FORMS: TESDataHandler trying to create TESForm for unknown type '%s'.",
                TESForm::GetFormTypeString(aType)
            );
            break;
        }
    }
    return pForm_3;
}

void TESDataHandler::EnsureFactionFriendshipsAreTwoWay() {
    for (TESFaction* apFaction : *pDataHandler->GetFactionList()) {
        if (apFaction) {
            for (BSSimpleList<GROUP_REACTION*>* pNode = static_cast<TESReactionForm*>(apFaction)->QReactionList();
                 pNode; pNode = pNode->m_pkNext) {
                GROUP_REACTION* pReaction = pNode->m_item;
                if ((pReaction) && (pReaction->pForm) &&
                    (pReaction->pForm->GetFormType() == FACT_ID &&
                     (pReaction->iFightReaction == 3 || pReaction->iFightReaction == 2))) {
                    TESFaction* v11 = static_cast<TESFaction*>(pReaction->pForm);
                    int32_t Reaction_0048C1B0 = static_cast<TESReactionForm*>(v11)->GetReaction_0048C1B0(apFaction);
                    if (Reaction_0048C1B0 != 2 && Reaction_0048C1B0 != 3) {
                        int32_t CastingType = apFaction->iFormID;
                        const char* v9 = apFaction->GetFormEditorID();
                        const char* v8 = apFaction->GetFormEditorID();
                        const char* v7 = v11->GetFormEditorID();
                        int32_t v6 = apFaction->iFormID;
                        const char* v5 = apFaction->GetFormEditorID();
                        int32_t v4 = v11->iFormID;
                        const char* v3 = v11->GetFormEditorID();
                        int32_t v2 = v11->iFormID;
                        const char* v1 = v11->GetFormEditorID();
                        int32_t v0 = apFaction->iFormID;
                        Warning(
                            "DEFAULT: (Faction Reaction Error) Faction '%s' (%08X) is a Friend or Ally of "
                            "Faction '%s' (%08X), but Faction '%s' (%08X) is not a Friend or Ally of "
                            "Faction '%s' (%08X). Faction '%s'(%08X) will be forced to be a Friend of "
                            "Faction '%s' (%08X) ",
                            apFaction->GetFormEditorID(), v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, CastingType
                        );
                        v11->SetFactionReaction(apFaction, 3);
                    }
                }
            }
        }
    }
}

void TESDataHandler::RegisterError() {
    MessageHandler::bShowNextWarning = true;
}

void TESDataHandler::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSSemaphore::BSSemaphore(int32_t aiCount)
  : iCount(aiCount), hSemaphore(new BSSystemSemaphore(aiCount, aiCount + 1)), iMaxCount(aiCount + 1) {}

// Stores the index assigned to the addon node.
void BGSAddonNode::SetIndex(uint32_t aiIndex) {
    iIndex = aiIndex;
}

// The allocation is bracketed by the AutoMemContext constructor and destructor on a stack context (with Sky.h's path
// and line), and the constructor's result is ignored.
Sky* Sky::GetInstance() {
    if (!pInstance) {
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(MEM_CONTEXT_SKY, true, "..\\Fallout Shared\\Sky\\Sky.h", 280);
        void* pMemory = ourMalloc(sizeof(Sky));
        pInstance = pMemory ? ::new (pMemory) Sky : nullptr;
        pContext->~AutoMemContext();
    }
    return pInstance;
}

const char* TESBipedModelForm::GetIconDefaultPath() {
    return inventoryIcon->GetDefaultPath();
}

// Calls GetFileList twice, as the game does.
void TESForm::ClearFiles() {
    if (GetFileList()) {
        GetFileList()->RemoveAll();
    }
}

// Swaps the grid position read from a big-endian file; the flags byte is left alone. IDLE_PARENT_IDS::Endian shares
// the code.
void EXTERIOR_DATA::Endian() {
    ENDIAN(iCellX, false);
    ENDIAN(iCellY, false);
}

bool TESObjectWEAP::IsEmbeddedWeapon() {
    return (data.cFlags & WEAPON_FLAG_EMBEDDED) != 0;
}

bool BGSSaveLoadGame::GetThreadAllowChanges() {
    return (GetThreadSpecificFlags() & 1u) == 0;
}

bool BGSSaveLoadGame::SetThreadAllowChanges(bool abValue) {
    bool bThreadAllowChanges = GetThreadAllowChanges();
    if (abValue) {
        uint32_t uiFlags = GetThreadSpecificFlags();
        GetThreadSpecificFlags() = uiFlags & ~1u;
    }
    else {
        uint32_t uiFlags = GetThreadSpecificFlags();
        GetThreadSpecificFlags() = uiFlags | 1u;
    }
    return bThreadAllowChanges;
}

NiTexturingProperty::Map* NiTexturingProperty::GetDarkMap() {
    return m_kMaps[DARK_INDEX];
}

NiTexturingProperty::Map* NiTexturingProperty::GetDetailMap() {
    return m_kMaps[DETAIL_INDEX];
}

NiTexturingProperty::Map* NiTexturingProperty::GetGlossMap() {
    return m_kMaps[GLOSS_INDEX];
}

NiTexturingProperty::Map* NiTexturingProperty::GetGlowMap() {
    return m_kMaps[GLOW_INDEX];
}

NiTexturingProperty::BumpMap* NiTexturingProperty::GetBumpMap() {
    return static_cast<BumpMap*>(m_kMaps[BUMP_INDEX]);
}

NiTexturingProperty::Map* NiTexturingProperty::GetDecalMap(uint32_t uiIndex) {
    return m_kMaps[DECAL_BASE + uiIndex];
}

uint32_t NiTexturingProperty::GetDecalMapCount() {
    return GetField(DECALCOUNT_MASK, DECALCOUNT_POS);
}
