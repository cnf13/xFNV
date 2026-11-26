#include "Fallout Shared/TESDataHandler.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
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
#include "Fallout Shared/Settings/XGameSetting.h"
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
#include "xfnv/Compiler.h"

#include <bit>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
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
static SettingT<INISettingCollection> sMasterMismatch(
    "sMasterMismatchWarning:GeneralWarnings",
    "One of the files that \"%s\" is dependent on has changed since the last save."
);
static SettingT<INISettingCollection> sGMMismatch(
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

int32_t makepath(char* apPath, uint32_t auiPathLen, char* apDevice, char* apDir, char* apFile, char* apExt) {
    return _makepath_s(apPath, auiPathLen, apDevice, apDir, apFile, apExt);
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
    if (apCell && !apCell->GetDelete()) {
        BSTaskThread::SetPauseSleepEnabled(true);
        uint8_t ucPrevAllowChanges = 0;
        if (pSaveLoadGame->GetSaveGameLoading()) {
            ucPrevAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
        }
        apCell->Detach(true);
        ReferenceProcessLists.RemoveTempEffectsForCell(apCell);
        apCell->SetState(TESObjectCELL::CS_UNLOADING);
        apCell->UnLoadAllTempData();
        apCell->ClearReference3D();
        while (apCell->GetLowestProcessMiddleLow()) {
            apCell->SetLowestProcessToMiddleLow(false);
        }
        if (!pDataHandler->GetClearingData()) {
            uint8_t ucIsSleepingorResting = pPlayer->IsSleepingorResting();
            apCell->ChangeProcessLevel(!ucIsSleepingorResting);
        }
        if (!apCell->GetInterior()) {
            TESObjectLAND* pLand = apCell->GetLand();
            pLand->UnLoadVertices();
        }
        apCell->Clear3D();
        apCell->SetAutoWaterLoaded(false);
        if (!apCell->GetInterior()) {
            TESWorldSpace* pWorldSpace = apCell->GetWorldSpace();
            pWorldSpace->UnLoadCell(apCell);
        }
        if (pSaveLoadGame->GetSaveGameLoading()) {
            pSaveLoadGame->SetThreadAllowChanges(ucPrevAllowChanges);
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
                static_cast<BGSProjectile*>(apObject),
                nullptr,
                nullptr,
                nullptr,
                aLocation,
                aDirection.z,
                aDirection.x,
                nullptr,
                nullptr,
                false,
                false,
                0.0f,
                0.0f,
                pCell
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
// saves as an edit, a deleted form is skipped. The master argument is ignored.
bool TESDataHandler::SaveForm(TESForm* pForm, bool master) {
    if (!pActiveFile) {
        return false;
    }

    if ((pForm->GetFile(-1) == pActiveFile && !pActiveFile->GetMaster()) || pForm->GetAltered()) {
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
        pNewTopicInfo,
        reinterpret_cast<BSSimpleList<TESTopicInfo*>*>(&listTopicInfos)
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
        MEM_CONTEXT_MASTER_FILE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
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
            pFile = new TESFile(pDirectory, pFoundName, NiFile::READ_ONLY);
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
                    const uint64_t uiListedTime = (static_cast<uint64_t>(rListedTime.dwHighDateTime) << 32) |
                        rListedTime.dwLowDateTime;
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
        MEM_CONTEXT_MASTER_FILE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
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
                Warning(sMasterMismatch.String(), pName);
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
        Warning(sGMMismatch.String());
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
            MEM_CONTEXT_ARCHIVES,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
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

    // The region list's nodes are walked as region lists, deliberately (the next node, less the
    // vtable).
    TESRegionList* pRegions = GetRegionList();
    while (pRegions && !pRegions->IsEmpty()) {
        pRegions->m_item->InitItem();
        pRegions = static_cast<TESRegionList*>(pRegions->m_pkNext);
    }
    // Walks the worldspace list once without using its items.
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
                formID,
                editorID,
                index,
                existingFormID,
                existingEditorID,
                newFormID,
                newEditorID
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
        // The result of the DynamicCast is deliberately dropped.
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
    if (!apWorldSpace) {
        return nullptr;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_CELLS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        2013
    );
    TESObjectCELL* pCell;
    pCell = new TESObjectCELL;
    uint32_t uiCellID = pSaveLoadGameOLD->GetCreatedExteriorCellFormID(apWorldSpace->iFormID, aiX, aiY);
    if (uiCellID) {
        pCell->SetFormID(uiCellID, true);
    }
    if (apName) {
        pCell->SetFormEditorID(apName);
    }
    uint8_t ucCellFlags = 0;
    pCell->SetInterior(false);
    ucCellFlags |= TESObjectCELL::kCellFlag_HasWater;
    pCell->cCellFlags = ucCellFlags;
    pCell->CreateCellData();
    pCell->SetDataCoord(aiX, aiY);
    if (apWorldSpace->AddCell(pCell)) {
        pCell->InitItem();
    }
    else {
        if (pCell) {
            delete pCell;
        }
        pCell = nullptr;
    }
    return pCell;
}

TESWorldSpace*
TESDataHandler::GetExtCellDataFromFileByEditorID(const char* apCellID, int32_t& aiOutX, int32_t& aiOutY) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_MASTER_FILE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        4161
    );
    TESWorldSpace* pCellWorld = nullptr;
    aiOutX = 0;
    aiOutY = 0;
    if (apCellID && *apCellID) {
        bool bOptimized = false;
        for (int32_t i = 0; i < iNumCompile; ++i) {
            if (pFileIndex[i] && pFileIndex[i]->GetOptimizedFile()) {
                bOptimized = true;
                break;
            }
        }
        if (bOptimized) {
            BSSimpleList<TESWorldSpace*>* pWorldList = pDataHandler->GetWorldSpaceList();
            while (pWorldList && !pWorldList->IsEmpty()) {
                TESWorldSpace* pWorld = pWorldList->m_item;
                pWorldList = pWorldList->m_pkNext;
                if (pWorld->GetExtCellDataFromFileByEditorID(apCellID, aiOutX, aiOutY)) {
                    pCellWorld = pWorld;
                    break;
                }
            }
        }
        else {
            for (int32_t i = 0; i < iNumCompile; ++i) {
                TESFile* pFile = pFileIndex[i];
                if (pFile && pFile->OpenTES(NiFile::READ_ONLY, false)) {
                    FORM* pFORM = pFile->GetFormData();
                    bool bFound = false;
                    while (pFORM && !bFound) {
                        bool bAdvanced = false;
                        if (pFORM->form == formEnumString[GRUP_ID].uiFormString) {
                            if (pFORM->iFormID || pFORM->flags != formEnumString[WRLD_ID].uiFormString) {
                                bAdvanced = pFile->NextGroup();
                            }
                            else {
                                bFound = true;
                                bAdvanced = pFile->NextForm(true);
                            }
                        }
                        else {
                            bAdvanced = pFile->NextForm(true);
                        }
                        if (bAdvanced) {
                            pFORM = pFile->GetFormData();
                        }
                        else {
                            pFORM = nullptr;
                        }
                    }
                    if (bFound) {
                        pFORM = pFile->GetFormData();
                        bFound = false;
                        while (pFORM && !bFound) {
                            bool bAdvanced = false;
                            if (pFORM->form == formEnumString[WRLD_ID].uiFormString) {
                                uint32_t uiFormID = pFORM->iFormID;
                                TESFile* pFormFile = pFile->GetIndexFile(static_cast<uint8_t>(uiFormID >> 24) + 1);
                                if (!pFormFile) {
                                    pFormFile = pFile;
                                }
                                uiFormID &= 0xFFFFFFu;
                                uint8_t ucCompileIndex = pFormFile->GetCompileIndex();
                                uiFormID |= ucCompileIndex << 24;
                                TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
                                pCellWorld = dynamic_cast<TESWorldSpace*>(pForm);
                                bAdvanced = pFile->NextForm(true);
                            }
                            else if (pFORM->form == formEnumString[CELL_ID].uiFormString) {
                                CHUNK_ID chunk;
                                chunk = pFile->GetTESChunk();
                                if (chunk == EDID_ID) {
                                    int32_t iSize = pFile->GetChunkSize() + 1;
                                    char* pFileID = static_cast<char*>(ourMalloc(static_cast<uint32_t>(iSize)));
                                    ourMemset(pFileID, 0, static_cast<uint32_t>(iSize));
                                    pFile->GetChunkData(pFileID, 0);
                                    if (!BSstricmp(pFileID, apCellID)) {
                                        bFound = true;
                                        while (chunk) {
                                            if (chunk == XCLC_ID) {
                                                EXTERIOR_DATA data;
                                                ourMemset(&data, 0, sizeof(data));
                                                pFile->GetChunkData(&data, 12u);
                                                if (pFile->QEndian()) {
                                                    data.Endian();
                                                }
                                                aiOutX = data.iCellX;
                                                aiOutY = data.iCellY;
                                                chunk = NO_CHUNK;
                                            }
                                            else if (pFile->NextChunk()) {
                                                chunk = pFile->GetTESChunk();
                                            }
                                            else {
                                                chunk = NO_CHUNK;
                                            }
                                        }
                                    }
                                    ourFree(pFileID);
                                }
                                bAdvanced = pFile->NextForm(true);
                            }
                            else if (pFORM->form == formEnumString[GRUP_ID].uiFormString) {
                                switch (pFORM->iFormID) {
                                    case 0u: {
                                        if (pFORM->flags == formEnumString[WRLD_ID].uiFormString) {
                                            bAdvanced = pFile->NextForm(true);
                                        }
                                        else if (
                                            static_cast<uint8_t>(TESForm::GetFormTypeFromFormString(pFORM->flags)) >
                                            WRLD_ID
                                        ) {
                                            bAdvanced = false;
                                        }
                                        break;
                                    }
                                    case 1u:
                                    case 4u:
                                    case 5u: {
                                        bAdvanced = pFile->NextForm(true);
                                        break;
                                    }
                                    case 6u: {
                                        bAdvanced = pFile->NextGroup();
                                        break;
                                    }
                                    default: {
                                        break;
                                    }
                                }
                            }
                            if (bAdvanced) {
                                pFORM = pFile->GetFormData();
                            }
                            else {
                                pFORM = nullptr;
                            }
                        }
                    }
                    pFile->CloseTES();
                    if (bFound) {
                        break;
                    }
                    pCellWorld = nullptr;
                }
            }
        }
    }
    return pCellWorld;
}

// The group header's label (flags) is the record type of a top group; its
// iFormID is the group type (4 and 5 are exterior cell blocks, 9 a cell's
// temporary children). Records of a plugin that is not a master always load
// unless a save is being loaded.
bool TESDataHandler::FormGroupLoadsInGame(TESFile* apFile, FORM* apFormGroup) {
    bool bLoads = true;

    if (apFile && !apFile->GetMaster() && (!pSaveLoadGameOLD || !pSaveLoadGameOLD->GetLoading())) {
        return true;
    }

    if (!apFormGroup) {
        return bLoads;
    }

    const uint32_t groupType = apFormGroup->iFormID;
    if (groupType >= 4 && (groupType <= 5 || groupType == 9)) {
        bLoads = false;
    }

    if (bLoads && pSaveLoadGameOLD && pSaveLoadGameOLD->GetLoading()) {
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
            case ALOC_ID: bLoads = false; break;
            default: break;
        }
    }

    return bLoads;
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
    if (aiX <= std::numeric_limits<int16_t>::max() &&
        (aiY <= std::numeric_limits<int16_t>::max() &&
         (aiX >= std::numeric_limits<int16_t>::min() && aiY >= std::numeric_limits<int16_t>::min()))) {
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
        "CELLS: Trying to get exterior cell for invalid cell coordinate. Values must be between %i and %i.",
        std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int16_t>::max()
    );
    return nullptr;
}

TESObjectCELL*
TESDataHandler::GetCellFromWorldCoord(float afX, float afY, TESWorldSpace* apWorldSpace, bool abCreateCell) {
    int32_t iX = FloatToInt(afX) >> 12;
    int32_t iY = FloatToInt(afY) >> 12;
    return GetCellFromCellCoord(iX, iY, apWorldSpace, abCreateCell);
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
                if (class1 && class2) {
                    const char* pName2 = class2->GetFullName();
                    if (_tcsicmp(class1->GetFullName(), pName2) > 0) {
                        pList->SetItem(class2);
                        pList->m_pkNext->SetItem(class1);
                    }
                }
            }
        }
    }
    BSSimpleList<TESRace*>* pRaceList = &listRaces;
    count = pRaceList->ItemsInList();
    for (int32_t n = 0; n < count - 1; ++n) {
        for (BSSimpleList<TESRace*>* pList = pRaceList; pList && pList->m_item; pList = pList->m_pkNext) {
            if (pList->m_pkNext) {
                TESRace* race1 = pList->m_item;
                TESRace* race2 = pList->m_pkNext->m_item;
                if (race1 && race2) {
                    const char* pName2 = race2->GetFullName();
                    if (_tcsicmp(race1->GetFullName(), pName2) > 0) {
                        pList->SetItem(race2);
                        pList->m_pkNext->SetItem(race1);
                    }
                }
            }
        }
    }
}

int32_t TESDataHandler::GetNumDLCs() {
    int32_t iCount = static_cast<uint8_t>(GetFlag(DLC_DEAD_MONEY)) != 0;
    if (static_cast<uint8_t>(GetFlag(DLC_HONEST_HEARTS))) {
        ++iCount;
    }
    if (static_cast<uint8_t>(GetFlag(DLC_OLD_WORLD_BLUES))) {
        ++iCount;
    }
    if (static_cast<uint8_t>(GetFlag(DLC_LONESOME_ROAD))) {
        ++iCount;
    }
    return iCount;
}

TESReputation* TESDataHandler::GetReputation(uint32_t aiID) {
    return dynamic_cast<TESReputation*>(TESForm::GetFormByNumericID(aiID));
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
        MEM_CONTEXT_TES_DEFAULT_OBJECTS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        10895
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
        pWaterExplosion = new BGSExplosion;
        pWaterExplosion->SetFormID(0x1F5, true);
        pWaterExplosion->SetFormEditorID("Default Water Explosion");
        pWaterExplosion->SetFullName("Water Explosion");
        GetExplosionList()->AddHead(pWaterExplosion);
        pWaterExplosion->SetAltered(false);
    }

    pGasTrapDummyWeap = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(0x1F6));
    if (!pGasTrapDummyWeap) {
        pGasTrapDummyWeap = new TESObjectWEAP;
        pGasTrapDummyWeap->SetFormID(0x1F6, true);
        pGasTrapDummyWeap->SetFormEditorID("GasTrap Dummy");
        pGasTrapDummyWeap->SetFullName("GasTrap Dummy");
        pObjectList->lnk_Append(pGasTrapDummyWeap);
        pGasTrapDummyWeap->SetAltered(false);
    }

    pReanimateEyes = dynamic_cast<TESEyes*>(TESForm::GetFormByNumericID(0x1A));
    if (!pReanimateEyes) {
        pReanimateEyes = new TESEyes;
        pReanimateEyes->SetFormID(0x1A, true);
        pReanimateEyes->SetFormEditorID("eyeReanimate");
        pReanimateEyes->SetFullName("Reanimate Eyes");
        GetEyesList()->AddHead(pReanimateEyes);
        pReanimateEyes->SetAltered(false);
    }

    pAshPile1 = dynamic_cast<TESObjectACTI*>(TESForm::GetFormByNumericID(0x1B));
    if (!pAshPile1) {
        pAshPile1 = new TESObjectACTI;
        pAshPile1->SetFormID(0x1B, true);
        pAshPile1->SetFormEditorID("DefaultAshPile1");
        pAshPile1->SetFullName("Ash Pile 1");
        pObjectList->lnk_Append(pAshPile1);
        pAshPile1->SetAltered(false);
    }

    pAshPile2 = dynamic_cast<TESObjectACTI*>(TESForm::GetFormByNumericID(0x22));
    if (!pAshPile2) {
        pAshPile2 = new TESObjectACTI;
        pAshPile2->SetFormID(0x22, true);
        pAshPile2->SetFormEditorID("DefaultAshPile2");
        pAshPile2->SetFullName("Ash Pile 2");
        pObjectList->lnk_Append(pAshPile2);
        pAshPile2->SetAltered(false);
    }

    pMaleVoice = dynamic_cast<BGSVoiceType*>(TESForm::GetFormByNumericID(0x2D));
    if (!pMaleVoice) {
        pMaleVoice = new BGSVoiceType;
        pMaleVoice->SetFormID(0x2D, true);
        pMaleVoice->SetFormEditorID("AdultMaleVoice1");
        GetVoiceTypeList()->AddHead(pMaleVoice);
        pMaleVoice->SetAltered(false);
    }

    pFemaleVoice = dynamic_cast<BGSVoiceType*>(TESForm::GetFormByNumericID(0x2E));
    if (!pFemaleVoice) {
        pFemaleVoice = new BGSVoiceType;
        pFemaleVoice->SetFormID(0x2E, true);
        pFemaleVoice->SetFormEditorID("AdultFemaleVoice1");
        GetVoiceTypeList()->AddHead(pFemaleVoice);
        pFemaleVoice->SetAltered(false);
    }

    pDefaultWater = dynamic_cast<TESWaterForm*>(TESForm::GetFormByNumericID(0x18));
    if (!pDefaultWater) {
        pDefaultWater = new TESWaterForm;
        pDefaultWater->SetFormID(0x18, true);
        pDefaultWater->SetFormEditorID("DefaultWater");
        GetWaterList()->AddHead(pDefaultWater);
        pDefaultWater->SetAltered(false);
    }

    pHorseMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x12));
    if (!pHorseMarker) {
        pHorseMarker = new TESObjectSTAT;
        pHorseMarker->SetFormID(0x12, true);
        pHorseMarker->SetFormEditorID("HorseMarker");
        pHorseMarker->SetModel("Marker_Horse.nif");
        pObjectList->lnk_Append(pHorseMarker);
        pHorseMarker->SetAltered(false);
    }

    pLockpick = dynamic_cast<TESObjectMISC*>(TESForm::GetFormByNumericID(0xA));
    if (!pLockpick) {
        pLockpick = new TESObjectMISC;
        pLockpick->SetFormID(0xA, true);
        pLockpick->SetFormEditorID("BobbyPin");
        pObjectList->lnk_Append(pLockpick);
        pLockpick->SetAltered(false);
    }

    pDoorMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x1));
    if (!pDoorMarker) {
        pDoorMarker = new TESObjectSTAT;
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
        pBoundMarker = new TESObjectSTAT;
        pBoundMarker->SetFormID(0x15, true);
        pBoundMarker->SetFormEditorID("MultiBoundMarker");
        static_cast<TESObjectSTAT*>(pBoundMarker)->SetModel("");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pBoundMarker));
        pBoundMarker->SetAltered(false);
    }

    pPlaneMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x17));
    if (!pPlaneMarker) {
        pPlaneMarker = new TESObjectSTAT;
        pPlaneMarker->SetFormID(0x17, true);
        pPlaneMarker->SetFormEditorID("PlaneMarker");
        pPlaneMarker->SetModel("");
        pObjectList->lnk_Append(pPlaneMarker);
        pPlaneMarker->SetAltered(false);
    }

    pRoomMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x1F));
    if (!pRoomMarker) {
        pRoomMarker = new TESObjectSTAT;
        pRoomMarker->SetFormID(0x1F, true);
        pRoomMarker->SetFormEditorID("RoomMarker");
        static_cast<TESObjectSTAT*>(pRoomMarker)->SetModel("");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pRoomMarker));
        pRoomMarker->SetAltered(false);
    }

    pPortalMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x20));
    if (!pPortalMarker) {
        pPortalMarker = new TESObjectSTAT;
        pPortalMarker->SetFormID(0x20, true);
        pPortalMarker->SetFormEditorID("PortalMarker");
        pPortalMarker->SetModel("");
        pObjectList->lnk_Append(pPortalMarker);
        pPortalMarker->SetAltered(false);
    }

    pCollisionMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x21));
    if (!pCollisionMarker) {
        pCollisionMarker = new TESObjectSTAT;
        pCollisionMarker->SetFormID(0x21, true);
        pCollisionMarker->SetFormEditorID("CollisionMarker");
        pCollisionMarker->SetModel("");
        pObjectList->lnk_Append(pCollisionMarker);
        pCollisionMarker->SetAltered(false);
    }

    pXMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x3B));
    if (!pXMarker) {
        pXMarker = new TESObjectSTAT;
        pXMarker->SetFormID(0x3B, true);
        pXMarker->SetFormEditorID("XMarker");
        static_cast<TESObjectSTAT*>(pXMarker)->SetModel("MarkerX.nif");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pXMarker));
        pXMarker->SetAltered(false);
    }

    pXMarkerHeading = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x34));
    if (!pXMarkerHeading) {
        pXMarkerHeading = new TESObjectSTAT;
        pXMarkerHeading->SetFormID(0x34, true);
        pXMarkerHeading->SetFormEditorID("XMarkerHeading");
        static_cast<TESObjectSTAT*>(pXMarkerHeading)->SetModel("MarkerXHeading.nif");
        pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pXMarkerHeading));
        pXMarkerHeading->SetAltered(false);
    }

    pCOCMarkerHeading = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x32));
    if (!pCOCMarkerHeading) {
        pCOCMarkerHeading = new TESObjectSTAT;
        pCOCMarkerHeading->SetFormID(0x32, true);
        pCOCMarkerHeading->SetFormEditorID("COCMarkerHeading");
        pCOCMarkerHeading->SetModel("MarkerCOCHeading.nif");
        pObjectList->lnk_Append(pCOCMarkerHeading);
        pCOCMarkerHeading->SetAltered(false);
    }

    pMapMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x10));
    if (!pMapMarker) {
        pMapMarker = new TESObjectSTAT;
        pMapMarker->SetFormID(0x10, true);
        pMapMarker->SetFormEditorID("MapMarker");
        pMapMarker->SetModel("Marker_Map.NIF");
        pObjectList->lnk_Append(pMapMarker);
        pMapMarker->SetAltered(false);
    }

    pAudioMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x23));
    if (!pAudioMarker) {
        pAudioMarker = new TESObjectSTAT;
        pAudioMarker->SetFormID(0x23, true);
        pAudioMarker->SetFormEditorID("AudioMarker");
        pAudioMarker->SetModel("Marker_Audio.NIF");
        pObjectList->lnk_Append(pAudioMarker);
        pAudioMarker->SetAltered(false);
    }

    pAudioBuoyMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x24));
    if (!pAudioBuoyMarker) {
        pAudioBuoyMarker = new TESObjectSTAT;
        pAudioBuoyMarker->SetFormID(0x24, true);
        pAudioBuoyMarker->SetFormEditorID("AudioBuoyMarker");
        pAudioBuoyMarker->SetModel("Marker_Audio.NIF");
        pObjectList->lnk_Append(pAudioBuoyMarker);
        pAudioBuoyMarker->SetAltered(false);
    }

    pTravelMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x2));
    if (!pTravelMarker) {
        pTravelMarker = new TESObjectSTAT;
        pTravelMarker->SetFormID(0x2, true);
        pTravelMarker->SetFormEditorID("TravelMarker");
        pTravelMarker->SetModel("Marker_Travel.nif");
        pObjectList->lnk_Append(pTravelMarker);
        pTravelMarker->SetAltered(false);
    }

    pNorthMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x3));
    if (!pNorthMarker) {
        pNorthMarker = new TESObjectSTAT;
        pNorthMarker->SetFormID(0x3, true);
        pNorthMarker->SetFormEditorID("NorthMarker");
        pNorthMarker->SetModel("Marker_North.nif");
        pObjectList->lnk_Append(pNorthMarker);
        pNorthMarker->SetAltered(false);
    }

    pJailMarker = dynamic_cast<TESObjectDOOR*>(TESForm::GetFormByNumericID(0x4));
    if (!pJailMarker) {
        pJailMarker = new TESObjectDOOR;
        pJailMarker->SetFormID(0x4, true);
        pJailMarker->SetFormEditorID("PrisonMarker");
        pJailMarker->SetModel("Marker_Prison.nif");
        pObjectList->lnk_Append(pJailMarker);
        pJailMarker->SetAltered(false);
    }

    pTempleMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x6));
    if (!pTempleMarker) {
        pTempleMarker = new TESObjectSTAT;
        pTempleMarker->SetFormID(0x6, true);
        pTempleMarker->SetFormEditorID("TempleMarker");
        pTempleMarker->SetModel("Marker_Temple.nif");
        pObjectList->lnk_Append(pTempleMarker);
        pTempleMarker->SetAltered(false);
    }

    pHolyMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x5));
    if (!pHolyMarker) {
        pHolyMarker = new TESObjectSTAT;
        pHolyMarker->SetFormID(0x5, true);
        pHolyMarker->SetFormEditorID("DivineMarker");
        pHolyMarker->SetModel("Marker_Divine.nif");
        pObjectList->lnk_Append(pHolyMarker);
        pHolyMarker->SetAltered(false);
    }

    pRadiationMarker = dynamic_cast<TESObjectSTAT*>(TESForm::GetFormByNumericID(0x33));
    if (!pRadiationMarker) {
        pRadiationMarker = new TESObjectSTAT;
        pRadiationMarker->SetFormID(0x33, true);
        pRadiationMarker->SetFormEditorID("Radiation Marker");
        pRadiationMarker->SetModel("Marker_Radiation.nif");
        pObjectList->lnk_Append(pRadiationMarker);
        pRadiationMarker->SetAltered(false);
    }

    pCreatureFaction = dynamic_cast<TESFaction*>(TESForm::GetFormByNumericID(0x13));
    if (!pCreatureFaction) {
        pCreatureFaction = new TESFaction;
        pCreatureFaction->SetFormID(0x13, true);
        pCreatureFaction->SetFormEditorID("CreatureFaction");
        GetFactionList()->AddHead(pCreatureFaction);
        pCreatureFaction->SetReaction(pCreatureFaction, 100);
        pCreatureFaction->AddRankData();
        pCreatureFaction->SetInitialized(true);
        pCreatureFaction->SetAltered(false);
    }

    if (!TESForm::GetFormByNumericID(0xF)) {
        TESObjectMISC* pCaps = new TESObjectMISC;
        pCaps->SetFormID(0xF, true);
        pCaps->SetFormEditorID("Caps001");
        pObjectList->lnk_Append(pCaps);
        pCaps->SetAltered(false);
    }

    pDefLootBag = static_cast<TESObjectCONT*>(TESForm::GetFormByNumericID(0xE));
    if (!pDefLootBag) {
        pDefLootBag = new TESObjectCONT;
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
        pFists = new TESObjectWEAP;
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
            pMarker = new TESObjectSTAT;
            pMarker->SetFormID(aFurnitureMarkerIDs[i], true);
            pMarker->SetFormEditorID(aFurnitureMarkerNames[i]);
            pObjectList->lnk_Append(static_cast<TESObjectSTAT*>(pMarker));
            pMarker->SetAltered(false);
        }
    }

    TESTopic::AddDefaultTopics();

    if (!TESForm::GetFormByNumericID(0x35)) {
        TESGlobal* pGlobal = new TESGlobal;
        pGlobal->SetFormID(0x35, true);
        pGlobal->SetFormEditorID("GameYear");
        pGlobal->SetValue(77.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameYear", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x36)) {
        TESGlobal* pGlobal = new TESGlobal;
        pGlobal->SetFormID(0x36, true);
        pGlobal->SetFormEditorID("GameMonth");
        pGlobal->SetValue(7.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameMonth", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x37)) {
        TESGlobal* pGlobal = new TESGlobal;
        pGlobal->SetFormID(0x37, true);
        pGlobal->SetFormEditorID("GameDay");
        pGlobal->SetValue(17.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameDay", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x38)) {
        TESGlobal* pGlobal = new TESGlobal;
        pGlobal->SetFormID(0x38, true);
        pGlobal->SetFormEditorID("GameHour");
        pGlobal->SetValue(12.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameHour", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x39)) {
        TESGlobal* pGlobal = new TESGlobal;
        pGlobal->SetFormID(0x39, true);
        pGlobal->SetFormEditorID("GameDaysPassed");
        pGlobal->SetValue(1.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("GameDaysPassed", pGlobal);
    }

    if (!TESForm::GetFormByNumericID(0x3A)) {
        TESGlobal* pGlobal = new TESGlobal;
        pGlobal->SetFormID(0x3A, true);
        pGlobal->SetFormEditorID("TimeScale");
        pGlobal->SetValue(30.0f);
        GetGlobalList()->AddHead(pGlobal);
        TESForm::pAllFormsByEditorID->SetAt("TimeScale", pGlobal);
    }

    TESNPC* pPlayerBase = dynamic_cast<TESNPC*>(TESForm::GetFormByNumericID(0x7));
    if (!pPlayerBase) {
        pPlayerBase = new TESNPC;
        pPlayerBase->SetFormID(0x7, true);
        pPlayerBase->SetFormEditorID("Player");
        pObjectList->lnk_Append(pPlayerBase);
        pPlayerBase->SetAltered(false);
    }

    if (!TESForm::GetFormByNumericID(0x15E)) {
        TESWeather* pWeather = new TESWeather;
        pWeather->SetFormID(0x15E, true);
        pWeather->SetFormEditorID("DefaultWeather");
        pWeather->MakeDefault();
        GetWeatherList()->AddHead(pWeather);
    }

    if (!TESForm::GetFormByNumericID(0x15F)) {
        TESClimate* pClimate = new TESClimate;
        pClimate->SetFormID(0x15F, true);
        pClimate->SetFormEditorID("DefaultClimate");
        pClimate->MakeDefault();
        GetClimateList()->AddHead(pClimate);
    }

    Sky::GetInstance()->SetCurrentClimate(nullptr, true);

    if (!TESForm::GetFormByNumericID(0x163)) {
        BGSListForm* pListForm = new BGSListForm;
        pListForm->SetFormID(0x163, true);
        pListForm->SetFormEditorID("HelpManualPC");
        GetListFormList()->AddHead(pListForm);
    }

    if (!TESForm::GetFormByNumericID(0x165)) {
        BGSListForm* pListForm = new BGSListForm;
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
            BGSMessage* pMessage = new BGSMessage;
            pMessage->SetFormID(i + 0x168, true);
            pMessage->SetFormEditorID(aHelpMessageNames[i]);
            GetMessageList()->AddHead(pMessage);
        }
    }

    // The exterior image space is created only with the interior one (no lookup
    // of 0x161).
    if (!TESForm::GetFormByNumericID(0x160)) {
        TESImageSpace* pImageSpace = new TESImageSpace;
        pImageSpace->SetFormID(0x160, true);
        pImageSpace->SetFormEditorID("DefaultImageSpaceInterior");
        pImageSpace->MakeDefault();
        GetImageSpaceList()->AddHead(pImageSpace);

        pImageSpace = new TESImageSpace;
        pImageSpace->SetFormID(0x161, true);
        pImageSpace->SetFormEditorID("DefaultImageSpaceExterior");
        pImageSpace->MakeDefault();
        GetImageSpaceList()->AddHead(pImageSpace);
    }

    if (!TESForm::GetFormByNumericID(0x162)) {
        TESImageSpaceModifier* pModifier = new TESImageSpaceModifier;
        pModifier->SetFormID(0x162, true);
        pModifier->SetFormEditorID("GetHit");
        pModifier->MakeDefault();
        GetImageSpaceModifierList()->AddHead(pModifier);
    }

    if (!TESForm::GetFormByNumericID(0x166)) {
        TESImageSpaceModifier* pModifier = new TESImageSpaceModifier;
        pModifier->SetFormID(0x166, true);
        pModifier->SetFormEditorID("ExplosionInFace");
        pModifier->MakeDefault();
        GetImageSpaceModifierList()->AddHead(pModifier);
    }

    if (!TESForm::GetFormByNumericID(0x164)) {
        TESImageSpaceModifier* pModifier = new TESImageSpaceModifier;
        pModifier->SetFormID(0x164, true);
        pModifier->SetFormEditorID("ImageSpaceConcussion");
        pModifier->MakeDefault();
        GetImageSpaceModifierList()->AddHead(pModifier);
    }

    MagicSystem::GenerateDefaultObjects();

    if (!TESForm::GetFormByNumericID(0x14A)) {
        EffectSetting* pEffect = new EffectSetting;
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
        TESCombatStyle* pCombatStyle = new TESCombatStyle;
        pCombatStyle->SetFormID(0x3D, true);
        pCombatStyle->SetFormEditorID("Default Combatstyle");
        GetCombatStyleList()->AddHead(pCombatStyle);
    }

    pPlayCredits = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(0x63));
    if (!pPlayCredits) {
        pPlayCredits = new TESGlobal;
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
        MEM_CONTEXT_TES_FORMS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        7833
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
                    pFile->GetID(),
                    pFile->GetFilename(),
                    pForm->GetFormTypeString(),
                    formEnumString[eType].pFormString
                );
                pContext->~AutoMemContext();
                return false;
            }
        }
    }

    bool bPartialForm = false;
    if ((!pFile->GetMaster() || !pFile->GetMasterList()->IsEmpty()) &&
        pForm != nullptr &&
        pForm->IsParentForm() &&
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
                        MEM_CONTEXT_MASTER_FILE,
                        true,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                        8160
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
                            // the engine's BSEndian<long>
                            uiID = std::byteswap<uint32_t>(uiID);
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
                pCell = new TESObjectCELL;
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
                        TESObjectCELL* const pExistingCell = pLastLoadedWorld->GetCellFromCellCoord(
                            pCell->GetDataX(),
                            pCell->GetDataY()
                        );
                        Warning(
                            "CELLS: Error adding %scell (%08X) to world space (%08X). "
                            "Cell will be destroyed.",
                            pCell->GetPersistentCell() ? "Persistent " : "",
                            pCell->iFormID,
                            pLastLoadedWorld->iFormID
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
                        pRef->iFormID,
                        pBase->GetFormEditorID(),
                        pBase->iFormID,
                        pLastLoadedCell->GetFormEditorID(),
                        pLastLoadedCell->GetDataX(),
                        pLastLoadedCell->GetDataY(),
                        pLastLoadedCell->iFormID,
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
                    pLand = new TESObjectLAND;
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
                MEM_CONTEXT_GAMESETTING,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                8465
            );
            if (pFile->GetTESChunk() == EDID_ID) {
                char* const pKey = static_cast<char*>(XFNV_ALLOCA(pFile->GetChunkSize()));
                pFile->GetChunkData(pKey, 0);
                SettingT<GameSettingCollection>::GetCollection()->ReadSetting(pFile, pKey);
            }
            pSettingContext->~AutoMemContext();
            pContext->~AutoMemContext();
            return true;
        }

        case WRLD_ID:
            if (pForm == nullptr) {
                TESWorldSpace* pWorld = new TESWorldSpace;
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
                MEM_CONTEXT_DIALOGUE,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                8507
            );
            if (pForm == nullptr) {
                TESTopic* const pTopic = new TESTopic(0);
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
                MEM_CONTEXT_DIALOGUE,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                8530
            );
            if (pForm != nullptr) {
                bExisting = true;
            }
            else {
                pForm = new TESTopicInfo(DT_TOPIC);
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
                MEM_CONTEXT_NAVMESH,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                9999
            );
            if (pForm == nullptr) {
                NavMesh* const pNewNavMesh = new NavMesh;
                NavMeshPtr spNavMesh = NavMeshPtr::MakeNavMeshPtr(pNewNavMesh);
                LoadForm(spNavMesh, pFile);
                if (spNavMesh->GetParentCell() != nullptr) {
                    if (spNavMesh->GetParentCell()->pNavMeshes == nullptr) {
                        NavMeshArray* const pNavMeshes = new NavMeshArray;
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
                MEM_CONTEXT_NAVMESH,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
                10029
            );
            if (pForm == nullptr) {
                NavMeshInfoMap* const pInfoMap = new NavMeshInfoMap;
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
    if (static_cast<uint32_t>(index) <= 254) {
        return pFileIndex[static_cast<uint32_t>(index)];
    }

    return nullptr;
}

bool TESDataHandler::GetMasterSave() {
    return bMasterSave;
}

uint32_t TESDataHandler::GetInteriorCellCount() {
    return arrayInteriorCells.NiTArray<TESObjectCELL*>::GetEffectiveSize();
}

void TESDataHandler::SetCheckingModels(bool abNewbool) {
    bCheckingModels = abNewbool;
}

void TESDataHandler::SetIsLoading(bool abLoading) {
    bDontRemoveIDs = abLoading;
}

TESClass* TESDataHandler::GetClass(uint32_t aiID) {
    return dynamic_cast<TESClass*>(TESForm::GetFormByNumericID(aiID));
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
    TESObjectREFR* pNewActor = nullptr;
    if (apInterior && !apInterior->GetInterior()) {
        apInterior = nullptr;
    }

    if (apInterior) {
        apWorld = nullptr;
    }
    if (apObject && (apInterior || apWorld)) {
        bool bPersistent = false;
        if (apReference) {
            bPersistent = apReference->GetRefPersists();
            apReference->SetRefPersists(false);
            TESObjectCELL* pOldParent = apReference->GetParentCell();
            if (pOldParent) {
                pOldParent->RemoveReference(apReference);
            }
            pNewActor = apReference;
            if (!apReference->GetObjectReference()) {
                pNewActor->SetObjectReference(apObject);
            }
            if (pNewActor->MustRefPersist()) {
                bPersistent = true;
            }
        }
        else {
            switch (apObject->GetFormType()) {
                case NPC__ID: {
                    pNewActor = new Character;
                    break;
                }
                case CREA_ID: {
                    pNewActor = new Creature;
                    break;
                }
                case PROJ_ID: {
                    return CreateProjectileAtLocation(apObject, aLocation, aDirection, apInterior, apWorld);
                }
                case EXPL_ID: {
                    return CreateExplosionAtLocation(apObject, aLocation, aDirection, apInterior, apWorld);
                }
                default: {
                    pNewActor = new TESObjectREFR;
                    break;
                }
            }
            if (apObject == pPortalMarker && apAdditionalData) {
                pNewActor->m_Extra.SetPortalLinkedRoomRef(0, static_cast<TESObjectREFR*>(apAdditionalData));
            }

            pNewActor->SetObjectReference(apObject);
            if (apAddPrimitive) {
                ExtraPrimitive* pExtraPrimitive = new ExtraPrimitive(apAddPrimitive);
                pNewActor->m_Extra.AddExtra(pExtraPrimitive);
            }
            if (pNewActor->MustBeVisibleDistant()) {
                pNewActor->SetVisibleDistant(true);
            }
            bPersistent = pNewActor->MustRefPersist();
            TESObjectLIGH* pLight = dynamic_cast<TESObjectLIGH*>(apObject);
            if (pLight) {
                bool bOffByDefault = pLight->GetOffByDefault();
                pNewActor->SetFireOff(bOffByDefault);
            }
        }
        if (pNewActor->IsActor() && static_cast<Actor*>(pNewActor)->GetActorBase()->GetUsesLeveledTemplate()) {
            static_cast<Actor*>(pNewActor)->GetActorBase()->CalcTemplateForRef(pNewActor);
        }

        if (IsFormIDCreated(pNewActor->iFormID)) {
            pNewActor->AddChange(0);
        }
        if (BGSDestructibleObjectForm::IsDestructible(pNewActor->GetObjectReference())) {
            pNewActor->SetDestructible(true);
        }
        pNewActor->SetLocationOnReference(aLocation);
        pNewActor->SetAngleOnReference(aDirection);
        TESObjectCELL* pNewParent = nullptr;
        if (apWorld) {
            int32_t iCellX = FloatToInt(aLocation.x) >> 12;
            int32_t iCellY = FloatToInt(aLocation.y) >> 12;
            pNewParent = apWorld->GetCellFromCellCoord(iCellX, iCellY);
        }
        else {
            pNewParent = apInterior;
        }
        if (pNewParent) {
            if (TES::pInstance->IsCellLoaded(pNewParent, false)) {
                if (apObject == pBoundMarker ||
                    apObject == pPlaneMarker ||
                    apObject == pDoorMarker ||
                    apObject == pTravelMarker ||
                    apObject == pXMarker ||
                    apObject == pXMarkerHeading ||
                    apObject == pMapMarker ||
                    apObject == pAudioMarker ||
                    apObject == pAudioBuoyMarker) {
                    NiAVObject* pModel = new NiNode(0);
                    pNewActor->Set3D(pModel, true);
                }
                pNewActor->SetParentCell(pNewParent);
                if (pNewActor->HasContainer()) {
                    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(pNewActor);
                    pInventory->InitLeveledItems();
                    pInventory->InitFromContainerExtra();
                    pInventory->InitScripts();
                    if (pInventory->IsEmpty()) {
                        pNewActor->m_Extra.RemoveContainerChanges();
                    }
                }
                pNewActor->SetParentCell(nullptr);
            }
            pNewActor->SetDroppedRef(true);
            pNewParent->AddReference(pNewActor, false);
            pNewActor->SetRefPersists(bPersistent);
            NiAVObject* pModel = pNewActor->Get3D();
            if (pModel) {
                pModel->SetTranslate(pNewActor->GetLocationOnReference());
                pModel->SetRotate(pNewActor->GetOrientation());
                bhkNiCollisionObject::ResetSim(pModel, true);
                ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
                pShadowSceneNode->AddObject(pModel);
                pModel->UpdateProperties();
                NiUpdateData kUpdateData(0.0f, false, false);
                pModel->Update(kUpdateData);
            }
        }
        else {
            pNewActor->Set3D(nullptr, false);
            pNewActor->SetRefPersists(true);
            apWorld->AddPersistentRef(pNewActor);
        }
    }

    Actor* pActor = dynamic_cast<Actor*>(pNewActor);
    if (pActor && !pActor->HasEditorLocation()) {
        pActor->SetStartingPosition();
    }

    if (pActor && !apReference) {
        pActor->SetStillLoading(false);
    }

    return pNewActor;
}

// Copies the argument's byte into the field as it is.
void TESDataHandler::SetIsClosingFile(bool abClosing) {
    bClearingData = abClosing;
}

void TESDataHandler::CheckModels(bool abUpdateBounds) {
    uint16_t usFlags;
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
        TESObject* pObj = pObjectList->lnk_lpRoot;
        int32_t iObjCount = 0;
        while (pObj) {
            if (!pObj->GetDelete()) {
                OBJECT_TYPE eType;
                uint8_t ucForm = pObj->GetFormType();
                for (eType = OBJECT_ACTIVATOR; eType < OBJECT_COUNT && aObjectTypeFormTypes[eType] != ucForm;
                     eType = static_cast<OBJECT_TYPE>(eType + 1)) {
                    ;
                }
                if (eType < OBJECT_COUNT) {
                    char acMessage[264];
                    TESObjectREFR* pRef;
                    char acFileName[268];
                    const char* pModelDesc;
                    bool bSkip = pObj->IsMarker();
                    TESModel* pModel = dynamic_cast<TESModel*>(pObj);
                    TESBoundObject* pBoundObj = dynamic_cast<TESBoundObject*>(pObj);
                    TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(pObj);
                    int32_t iCount;
                    if (pModel) {
                        iCount = 1;
                    }
                    else {
                        iCount = pBipedModel != nullptr ? 4 : 0;
                    }
                    for (; iCount; --iCount) {
                        pModelDesc = "";
                        if (pBipedModel) {
                            switch (iCount) {
                                case 1: {
                                    pModel = pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE);
                                    pModelDesc = "Male World ";
                                    break;
                                }
                                case 2: {
                                    pModel = pBipedModel->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                                    pModelDesc = "Female World ";
                                    break;
                                }
                                case 3: {
                                    pModel = pBipedModel->GetBipedTESModel(TESActorBase::SEX_MALE);
                                    pModelDesc = "Male Biped ";
                                    break;
                                }
                                case 4: {
                                    pModel = pBipedModel->GetBipedTESModel(TESActorBase::SEX_FEMALE);
                                    pModelDesc = "Female Biped ";
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        if (bSkip) {
                            continue;
                        }
                        if (!pModel->GetModelLength()) {
                            if (eType != OBJECT_NPC && eType != OBJECT_LIGHT && bFileNeededMessage.Bool()) {
                                switch (eType) {
                                    case OBJECT_ACTIVATOR:
                                    case OBJECT_LIGHT:
                                    case OBJECT_NPC: {
                                        continue;
                                    }
                                    case OBJECT_WEAPON: {
                                        if (static_cast<TESObjectWEAP*>(pObj)->TESObjectWEAP::IsEmbeddedWeapon()) {
                                            break;
                                        }
                                        [[fallthrough]];
                                    }
                                    default: {
                                        const char* pEditorID = pObj->GetFormEditorID();
                                        Warning(
                                            "MODELS: No model selected for %smodel %s \"%s\".",
                                            pModelDesc,
                                            objectLabel[eType],
                                            pEditorID
                                        );
                                        break;
                                    }
                                }
                            }
                            continue;
                        }
                        if (eType == OBJECT_TREE) {
                            BSsprintf(acFileName, 260, "Trees\\%s", pModel->GetModel());
                        }
                        else {
                            BSsprintf(acFileName, 260, "Meshes\\%s", pModel->GetModel());
                        }
                        if (FileFinder::Locate(acFileName, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                            if (bFileGoneMessage.Bool()) {
                                char acDevice[4];
                                char acFile[512];
                                char acDir[264];
                                char acExt[256];
                                BSsplitpath(pModel->GetModel(), acDevice, 3, acDir, 260, acFile, 512, acExt, 256);
                                BSsprintf(acFileName, 260, "x%s", acFile);
                                BSstrcpy(acFile, 512, acFileName);
                                BSsprintf(acFileName, 260, "Meshes\\%s", acDir);
                                BSstrcpy(acDir, 260, acFileName);
                                makepath(acFileName, 260u, acDevice, acDir, acFile, acExt);
                                if (FileFinder::Locate(acFileName, nullptr, 0, 0xFFFFFFFFu)) {
                                    CheckForNiRawImageData(acFileName);
                                }
                                else {
                                    const char* pEditorID = pObj->GetFormEditorID();
                                    char* pLabel = objectLabel[eType];
                                    const char* pModelName = pModel->GetModel();
                                    Warning(
                                        "MODELS: %sModel \"%s\" not found for %s \"%s\".",
                                        pModelDesc,
                                        pModelName,
                                        pLabel,
                                        pEditorID
                                    );
                                }
                            }
                            continue;
                        }
                        if (!pBoundObj ||
                            (!bFileTestLoad.Bool() && !abUpdateBounds && !bFileCheckModelCollision.Bool())) {
                            continue;
                        }
                        const char* pEditorID = pBoundObj->GetFormEditorID();
                        char* pLabel = objectLabel[eType];
                        const char* pModelName = pModel->GetModel();
                        std::sprintf(
                            acMessage,
                            "Loading %smodel \"%s\" for %s \"%s\".",
                            pModelDesc,
                            pModelName,
                            pLabel,
                            pEditorID
                        );
                        Interface::PrintLine(static_cast<const char*>(acMessage));
                        appMain->Swap();
                        if (pBoundObj->GetFormType() == NPC__ID) {
                            pRef = new Character;
                        }
                        else if (pBoundObj->GetFormType() == CREA_ID) {
                            pRef = new Creature;
                        }
                        else {
                            pRef = new TESObjectREFR;
                        }
                        pRef->SetObjectReference(pBoundObj);
                        int32_t iFormType = pBoundObj->GetFormType();
                        if (iFormType == NPC__ID || iFormType == CREA_ID) {
                            if (!bFileCheckModelCollision.Bool() || bFileTestLoad.Bool() || abUpdateBounds) {
                                usFlags = 0;
                                if (!TESActorBaseData::GetLeveledTemplateInHierarchy(
                                        static_cast<TESActorBase*>(pBoundObj),
                                        usFlags
                                    ) ||
                                    (usFlags &
                                     ((1 << TESActorBaseData::TU_STATS) | (1 << TESActorBaseData::TU_FACTIONS))) == 0) {
                                    pModelLoader->QueueReference(pRef, IO_TASK_PRIORITY_CRITICAL, false);
                                    pIOManager->LoadAllQueuedPriorities();
                                    int32_t iWarnCount = MessageHandler::QWarningCount();
                                    pRef->Load3D(false);
                                    if (iWarnCount != MessageHandler::QWarningCount()) {
                                        pEditorID = pBoundObj->GetFormEditorID();
                                        pLabel = objectLabel[eType];
                                        pModelName = pModel->GetModel();
                                        Warning(
                                            "Warnings were encountered loading %smodel '%s' for %s %s.",
                                            pModelDesc,
                                            pModelName,
                                            pLabel,
                                            pEditorID
                                        );
                                        Warning("");
                                    }
                                }
                            }
                        }
                        else {
                            int32_t iWarnCount = MessageHandler::QWarningCount();
                            pRef->Load3D(false);
                            if (iWarnCount != MessageHandler::QWarningCount()) {
                                pEditorID = pBoundObj->GetFormEditorID();
                                pLabel = objectLabel[eType];
                                pModelName = pModel->GetModel();
                                Warning(
                                    "Warnings were encountered loading %smodel '%s' for %s %s.",
                                    pModelDesc,
                                    pModelName,
                                    pLabel,
                                    pEditorID
                                );
                                Warning("");
                            }
                        }
                        if (pRef->Get3D()) {
                            pModelName = pModel->GetModel();
                            NiNode* p3D = pRef->Get3D();
                            CheckTexturesRecurse(p3D, pModelName);
                        }
                        if (bFileCheckModelCollision.Bool() &&
                            pBoundObj->GetFormType() != NPC__ID &&
                            pBoundObj->GetFormType() != CREA_ID) {
                            int32_t iConvexVerticesCount = 0;
                            int32_t iShapeCollectionTriCount = 0;
                            TESHavokUtilities::InspectHavokObjects(
                                pRef->Get3D(),
                                iConvexVerticesCount,
                                iShapeCollectionTriCount
                            );
                            // The bound size is kept as a float and widened for each use.
                            float fRadius = pBoundObj->GetBoundSize();
                            int32_t iTriangleCount = 0;
                            if (TES::pInstance) {
                                NiAVObject* p3D = pRef->Get3D();
                                if (p3D) {
                                    NiNode* pNode = p3D->IsNode();
                                    iTriangleCount = TES::pInstance->GetFaceCount(pNode, true);
                                }
                                else {
                                    iTriangleCount = TES::pInstance->GetFaceCount(nullptr, true);
                                }
                            }
                            PrintToFile(
                                "TestModels - Collision Info.xls",
                                "%s\t%.2f\t%i\t%i\t%.2f\t%.2f\t%i\t%.2f\t%.2f",
                                pModel->GetModel(),
                                fRadius,
                                iTriangleCount,
                                iConvexVerticesCount,
                                static_cast<double>(iConvexVerticesCount) / fRadius,
                                static_cast<double>(iConvexVerticesCount) / static_cast<double>(iTriangleCount),
                                iShapeCollectionTriCount,
                                static_cast<double>(iShapeCollectionTriCount) / fRadius,
                                static_cast<double>(iShapeCollectionTriCount) / static_cast<double>(iTriangleCount)
                            );
                        }
                        if (pRef) {
                            delete pRef;
                        }
                        if (iObjCount++ > 20) {
                            TES::pInstance->CleanUpUnusedTextures(false);
                            iObjCount = 0;
                        }
                    }
                }
                else {
                    Warning("MODELS: Object \"%s\" invalid type.", pObj->GetFormEditorID());
                }
            }
            pObj = pObj->nod_lpNext;
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
                NiTexturingProperty::Map* pDetailMap = pTextProp->GetDetailMap();
                PrintTextureInfo(
                    pDetailMap,
                    static_cast<NiTriBasedGeom*>(apObject),
                    "Detail Map",
                    apModel,
                    pShaderProperty != nullptr
                );
                NiTexturingProperty::BumpMap* pBumpMap = pTextProp->GetBumpMap();
                PrintTextureInfo(
                    pBumpMap,
                    static_cast<NiTriBasedGeom*>(apObject),
                    "Bump Map",
                    apModel,
                    pShaderProperty != nullptr
                );
                NiTexturingProperty::Map* pGlowMap = pTextProp->GetGlowMap();
                PrintTextureInfo(
                    pGlowMap,
                    static_cast<NiTriBasedGeom*>(apObject),
                    "Glow Map",
                    apModel,
                    pShaderProperty != nullptr
                );
                NiTexturingProperty::Map* pGlossMap = pTextProp->GetGlossMap();
                PrintTextureInfo(
                    pGlossMap,
                    static_cast<NiTriBasedGeom*>(apObject),
                    "Gloss Map",
                    apModel,
                    pShaderProperty != nullptr
                );
                NiTexturingProperty::Map* pDarkMap = pTextProp->GetDarkMap();
                PrintTextureInfo(
                    pDarkMap,
                    static_cast<NiTriBasedGeom*>(apObject),
                    "Dark Map",
                    apModel,
                    pShaderProperty != nullptr
                );
                for (uint32_t uiIndex = 0; uiIndex < pTextProp->GetDecalMapCount(); ++uiIndex) {
                    NiTexturingProperty::Map* pDecalMap = pTextProp->GetDecalMap(uiIndex);
                    PrintTextureInfo(
                        pDecalMap,
                        static_cast<NiTriBasedGeom*>(apObject),
                        "Decal Map",
                        apModel,
                        pShaderProperty != nullptr
                    );
                }
            }
        }
        else if (apObject->IsNode()) {
            uint32_t uiArrayCount = static_cast<NiNode*>(apObject)->NiNode::GetArrayCount();
            for (uint32_t i = 0; i < uiArrayCount; ++i) {
                NiNode* pChild = static_cast<NiNode*>(static_cast<NiNode*>(apObject)->NiNode::GetAt(i));
                if (pChild) {
                    CheckTexturesRecurse(pChild, apModel);
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
                    if (buffer[n] == 'N' &&
                        buffer[n + 1] == 'i' &&
                        buffer[n + 2] == 'R' &&
                        buffer[n + 3] == 'a' &&
                        buffer[n + 4] == 'w' &&
                        buffer[n + 5] == 'I' &&
                        buffer[n + 6] == 'm' &&
                        buffer[n + 7] == 'a' &&
                        buffer[n + 8] == 'g' &&
                        buffer[n + 9] == 'e' &&
                        buffer[n + 10] == 'D' &&
                        buffer[n + 11] == 'a' &&
                        buffer[n + 12] == 't' &&
                        buffer[n + 13] == 'a') {
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
        InventoryChanges* pContainer;
        pContainer = new InventoryChanges(nullptr);
        pBarterContainer = pContainer;
    }
    InventoryChanges* pActorList = apVendor->m_Extra.GetContainerChanges();
    if (pActorList) {
        pActorList->CreateCopyofList(pBarterContainer, apVendor, false);
    }
    TESObjectREFR* pMerchant = apVendor->m_Extra.GetMerchantContainer();
    if (pMerchant) {
        pActorList = pMerchant->m_Extra.GetContainerChanges();
        if (pActorList) {
            pActorList->RunScripts(pMerchant);
        }
        else {
            pActorList = new InventoryChanges(pMerchant);
        }
        pActorList->CreateCopyofList(pBarterContainer, pMerchant, false);
    }
    TESObjectCELL* pCell = apVendor->GetParentCell();
    if (pCell) {
        pCell->FillBarterContainer(apVendor, pBarterContainer);
        if (apVendor->IsActor()) {
            Actor* pAct = static_cast<Actor*>(apVendor);
            for (ItemChange* pItemChange : pBarterContainer->pListofChanges->ItemsUntilNull()) {
                TESBoundObject* pBoundObject = pItemChange->pContainerObj;
                if (pBoundObject->GetFormType() == AMMO_ID && pAct->GetCurrentWeapon()) {
                    TESObjectWEAP* pCurrentWeapon = pAct->GetCurrentWeapon();
                    if (pCurrentWeapon->GetCurrentAmmo(pAct) == pBoundObject) {
                        BSSimpleList<ExtraDataList*>* pExtraList = pItemChange->pExtraObjectList;
                        int32_t iCount = 0;
                        while (pExtraList && pExtraList->m_item) {
                            ExtraDataList* pExtra = pExtraList->m_item;
                            if (pExtra->GetOriginalReference() && pExtra->GetOriginalReference() == pAct) {
                                iCount = pExtra->GetCount();
                                pItemChange->pExtraObjectList->Remove(pExtra);
                                break;
                            }
                            pExtraList = pExtraList->m_pkNext;
                        }
                        uint32_t uiNumber = pItemChange->iNumber;
                        pItemChange->iNumber = uiNumber - iCount;
                        if (pItemChange->iNumber <= 0 ||
                            !pItemChange->pExtraObjectList ||
                            pItemChange->pExtraObjectList->IsEmpty() ||
                            pItemChange->GetWorn(false)) {
                            pBarterContainer->pListofChanges->Remove(pItemChange);
                        }
                        return;
                    }
                }
            }
        }
    }
}

void TESDataHandler::RemoveItemBarterContainer(ItemChange* apitem, int32_t aiNumber, TESObjectREFR* apVendor) {
    ExtraDataList* pExtra = nullptr;
    ItemChange* pItem = pBarterContainer->GetObjectInList(apitem->pContainerObj, true, 0);
    if (pItem) {
        BSSimpleList<ExtraDataList*>* pList = pItem->pExtraObjectList;
        if (pList) {
            pExtra = pList->m_item;
        }
        bool bFlag = false;
        while (true) {
            TESObjectREFR* pRef;
            int32_t iRemove;
            while (true) {
                if (!aiNumber) {
                    return;
                }
                pRef = nullptr;
                iRemove = 1;
                if (!pList && bFlag) {
                    return;
                }
                if (pList) {
                    break;
                }
                CreateBarterContainer(apVendor);
                pExtra = nullptr;
                bFlag = true;
                pItem = pBarterContainer->GetObjectInList(apitem->pContainerObj, true, 0);
                if (!pItem) {
                    return;
                }
                pList = pItem->pExtraObjectList;
                if (pList) {
                    pExtra = pList->m_item;
                }
            }
            pExtra = pList->m_item;
            pList = pList->m_pkNext;
            if (pExtra) {
                int16_t sCount = pExtra->GetCount();
                iRemove = sCount;
                if (sCount <= 0) {
                    pRef = nullptr;
                }
                else {
                    pRef = pExtra->GetOriginalReference();
                    if (aiNumber < iRemove) {
                        iRemove = aiNumber;
                    }
                    else {
                        pExtra->RemoveOriginalReferenceExtra();
                    }
                }
            }
            if (pRef) {
                if (pRef->HasContainer()) {
                    ExtraDataList* pPassedExtra = nullptr;
                    if (pExtra->ItemsInList() && (pExtra->ItemsInList() != 1 || !pExtra->GetExtraData(36))) {
                        pPassedExtra = pExtra;
                    }

                    pRef->RemoveItem(
                        pItem->pContainerObj,
                        pPassedExtra,
                        iRemove,
                        false,
                        false,
                        pPlayer,
                        nullptr,
                        nullptr,
                        true,
                        false
                    );
                    InventoryChanges* pChangeRef = pRef->m_Extra.GetContainerChanges();
                    bool bDelete = true;
                    if (pChangeRef) {
                        ItemChange* pTempItem = pChangeRef->GetObjectInList(pItem->pContainerObj, true, 0);
                        if (pTempItem && pTempItem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* pTempObj = pTempItem->pExtraObjectList;
                            while (pTempObj && pTempObj->m_item && bDelete) {
                                if (pTempObj->m_item == pExtra) {
                                    bDelete = false;
                                }
                                else {
                                    pTempObj = pTempObj->m_pkNext;
                                }
                            }
                        }
                    }
                    pChangeRef = pPlayer->m_Extra.GetContainerChanges();
                    if (pChangeRef) {
                        ItemChange* pTempItem = pChangeRef->GetObjectInList(pItem->pContainerObj, true, 0);
                        if (pTempItem && pTempItem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* pTempObj = pTempItem->pExtraObjectList;
                            while (pTempObj && pTempObj->m_item && bDelete) {
                                if (pTempObj->m_item == pExtra) {
                                    bDelete = false;
                                }
                                else {
                                    pTempObj = pTempObj->m_pkNext;
                                }
                            }
                        }
                    }
                    if (bDelete &&
                        pExtra &&
                        pExtra->ItemsInList() < 2 &&
                        pExtra->GetCount() <= 1 &&
                        !pExtra->GetScript()) {
                        pItem->pExtraObjectList->Remove(pExtra);
                        pList = pItem->pExtraObjectList;
                        if (pExtra) {
                            delete pExtra;
                        }
                        pExtra = nullptr;
                    }

                    if (pExtra && ((pExtra->GetCount() > 1 && pExtra->ItemsInList() < 2) || !pExtra->ItemsInList())) {
                        pExtra = nullptr;
                    }
                }
                else {
                    pRef->m_Extra.RemoveOwnership();
                    pRef->m_Extra.RemoveOriginalReferenceExtra();
                    pPlayer->PickUpObject(pRef, iRemove, false);
                    pItem->pExtraObjectList->Remove(pExtra);
                    pList = pItem->pExtraObjectList;
                    if (pExtra->ItemsInList() < 2 && pExtra->GetCount() <= 1 && !pExtra->GetScript() && pExtra) {
                        delete pExtra;
                    }

                    pExtra = nullptr;
                }
                aiNumber -= iRemove;
                if (pList) {
                    pExtra = pList->m_item;
                }
                else {
                    pExtra = nullptr;
                }
            }
            else {
                if (pList) {
                    pExtra = pList->m_item;
                }
                else {
                    aiNumber = 0;
                }
            }
        }
    }
}

void TESDataHandler::ClearFileList() {
    TESFile::DeleteFileList(GetFileList());
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
        Script* pScript = listScripts.m_item;
        if (!pScript) {
            break;
        }
        listScripts.RemoveHead();
        delete pScript;
    }
    while (true) {
        BGSListForm* plistform = listListForms.m_item;
        if (!plistform) {
            break;
        }
        listListForms.RemoveHead();
        delete plistform;
    }
    while (true) {
        BGSMenuIcon* picon = listMenuIcons.m_item;
        if (!picon) {
            break;
        }
        listMenuIcons.RemoveHead();
        delete picon;
    }
    while (true) {
        BGSHeadPart* ppart = listHeadParts.m_item;
        if (!ppart) {
            break;
        }
        listHeadParts.RemoveHead();
        delete ppart;
    }
    while (true) {
        TESHair* phair = listHair.m_item;
        if (!phair) {
            break;
        }
        listHair.RemoveHead();
        delete phair;
    }
    while (true) {
        TESEyes* peyes = listEyes.m_item;
        if (!peyes) {
            break;
        }
        listEyes.RemoveHead();
        delete peyes;
    }
    while (true) {
        TESRace* pRace = listRaces.m_item;
        if (!pRace) {
            break;
        }
        listRaces.RemoveHead();
        delete pRace;
    }
    pUnreadDefaultObject = nullptr;
    while (true) {
        BGSEncounterZone* pzone = static_cast<BGSEncounterZone*>(listZones.m_item);
        if (!pzone) {
            break;
        }
        listZones.RemoveHead();
        delete pzone;
    }
    while (true) {
        TESClimate* pClimate = static_cast<TESClimate*>(listClimates.m_item);
        if (!pClimate) {
            break;
        }
        listClimates.RemoveHead();
        delete pClimate;
    }
    while (true) {
        TESWeather* pWeather = static_cast<TESWeather*>(listWeather.m_item);
        if (!pWeather) {
            break;
        }
        listWeather.RemoveHead();
        delete pWeather;
    }
    while (true) {
        TESClass* pClass = listClasses.m_item;
        if (!pClass) {
            break;
        }
        listClasses.RemoveHead();
        delete pClass;
    }
    while (true) {
        TESFaction* pFaction = listFactions.m_item;
        if (!pFaction) {
            break;
        }
        listFactions.RemoveHead();
        delete pFaction;
    }
    while (true) {
        TESChallenge* pChallenge = listChallenges.m_item;
        if (!pChallenge) {
            break;
        }
        listChallenges.RemoveHead();
        delete pChallenge;
    }
    while (true) {
        TESReputation* pRep = listReputations.m_item;
        if (!pRep) {
            break;
        }
        listReputations.RemoveHead();
        delete pRep;
    }
    while (true) {
        TESRecipe* pRecipe = listRecipes.m_item;
        if (!pRecipe) {
            break;
        }
        listRecipes.RemoveHead();
        delete pRecipe;
    }
    while (true) {
        TESCasino* pCasino = listCasinos.m_item;
        if (!pCasino) {
            break;
        }
        listCasinos.RemoveHead();
        delete pCasino;
    }
    while (true) {
        TESRecipeCategory* pRecipeCat = static_cast<TESRecipeCategory*>(listRecipeCategories.m_item);
        if (!pRecipeCat) {
            break;
        }
        listRecipeCategories.RemoveHead();
        delete pRecipeCat;
    }
    while (true) {
        TESAmmoEffect* pAmmoEffect = static_cast<TESAmmoEffect*>(listAmmoEffects.m_item);
        if (!pAmmoEffect) {
            break;
        }
        listAmmoEffects.RemoveHead();
        delete pAmmoEffect;
    }
    while (true) {
        TESGlobal* pGlobal = listGlobals.m_item;
        if (!pGlobal) {
            break;
        }
        listGlobals.RemoveHead();
        delete pGlobal;
    }
    while (true) {
        BGSVoiceType* pVoiceType = listVoiceTypes.m_item;
        if (!pVoiceType) {
            break;
        }
        listVoiceTypes.RemoveHead();
        delete pVoiceType;
    }
    while (true) {
        BGSImpactData* pimpdata = listImpactData.m_item;
        if (!pimpdata) {
            break;
        }
        listImpactData.RemoveHead();
        delete pimpdata;
    }
    while (true) {
        BGSImpactDataSet* pimpdataset = listImpactDataSet.m_item;
        if (!pimpdataset) {
            break;
        }
        listImpactDataSet.RemoveHead();
        delete pimpdataset;
    }
    while (true) {
        TESQuest* pQuest = listQuests.m_item;
        if (!pQuest) {
            break;
        }
        listQuests.RemoveHead();
        delete pQuest;
    }
    while (true) {
        TESTopic* pTopic = listTopics.m_item;
        if (!pTopic) {
            break;
        }
        listTopics.RemoveHead();
        delete pTopic;
    }
    TESTopic::RemoveDefaultTopics();
    while (listTopicInfos.m_item) {
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
        TESWorldSpace* pWorld = listWorldSpaces.m_item;
        if (!pWorld) {
            break;
        }
        listWorldSpaces.RemoveHead();
        delete pWorld;
    }
    GarbageCollector::ClearAll(false);
    while (true) {
        TESSound* pSound = listSounds.m_item;
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
        BGSAcousticSpace* paSpc = listAcousticSpaces.m_item;
        if (!paSpc) {
            break;
        }
        listAcousticSpaces.RemoveHead();
        delete paSpc;
    }
    while (true) {
        TESLandTexture* pLandText = listLandTexts.m_item;
        if (!pLandText) {
            break;
        }
        listLandTexts.RemoveHead();
        delete pLandText;
    }
    while (true) {
        BGSMessage* pMessage = listMessages.m_item;
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
        TESImageSpace* pimagespace = static_cast<TESImageSpace*>(listImageSpaces.m_item);
        if (!pimagespace) {
            break;
        }
        listImageSpaces.RemoveHead();
        delete pimagespace;
    }
    while (true) {
        TESImageSpaceModifier* pimagespacemodifier = static_cast<TESImageSpaceModifier*>(
            listImageSpaceModifiers.m_item
        );
        if (!pimagespacemodifier) {
            break;
        }
        listImageSpaceModifiers.RemoveHead();
        delete pimagespacemodifier;
    }
    pObjectList->lnk_Destroy();
    arrayAddonNodes.RemoveAll();
    while (true) {
        SpellItem* pSpellItem = static_cast<SpellItem*>(listSpellItems.m_item);
        if (!pSpellItem) {
            break;
        }
        listSpellItems.RemoveHead();
        delete pSpellItem;
    }
    while (true) {
        EnchantmentItem* pEnchItem = static_cast<EnchantmentItem*>(listEnchantmentItems.m_item);
        if (!pEnchItem) {
            break;
        }
        listEnchantmentItems.RemoveHead();
        delete pEnchItem;
    }
    while (true) {
        TESPackage* ppack = static_cast<TESPackage*>(listPackages.m_item);
        if (!ppack) {
            break;
        }
        listPackages.RemoveHead();
        delete ppack;
    }
    while (true) {
        TESCombatStyle* pcombatStyle = listCombatStyles.m_item;
        if (!pcombatStyle) {
            break;
        }
        listCombatStyles.RemoveHead();
        delete pcombatStyle;
    }
    while (true) {
        TESLoadScreen* ploadscreen = listLoadScreens.m_item;
        if (!ploadscreen) {
            break;
        }
        listLoadScreens.RemoveHead();
        delete ploadscreen;
    }
    while (true) {
        TESLoadScreenType* pLST = listLoadScreenTypes.m_item;
        if (!pLST) {
            break;
        }
        listLoadScreenTypes.RemoveHead();
        delete pLST;
    }
    while (true) {
        TESWaterForm* pwater = listWater.m_item;
        if (!pwater) {
            break;
        }
        listWater.RemoveHead();
        delete pwater;
    }
    pDefaultWater = nullptr;
    while (true) {
        TESObjectANIO* paniObj = animObjects.m_item;
        if (!paniObj) {
            break;
        }
        animObjects.RemoveHead();
        delete paniObj;
    }
    while (true) {
        TESEffectShader* peffectShader = listEffectShaders.m_item;
        if (!peffectShader) {
            break;
        }
        listEffectShaders.RemoveHead();
        delete peffectShader;
    }
    while (true) {
        BGSProjectile* pprojectile = listProjectiles.m_item;
        if (!pprojectile) {
            break;
        }
        listProjectiles.RemoveHead();
        delete pprojectile;
    }
    while (true) {
        BGSExplosion* pexplosion = static_cast<BGSExplosion*>(listExplosions.m_item);
        if (!pexplosion) {
            break;
        }
        listExplosions.RemoveHead();
        delete pexplosion;
    }
    while (true) {
        BGSDebris* pdebris = listDebris.m_item;
        if (!pdebris) {
            break;
        }
        listDebris.RemoveHead();
        delete pdebris;
    }
    while (true) {
        BGSPerk* pperk = listPerks.m_item;
        if (!pperk) {
            break;
        }
        listPerks.RemoveHead();
        delete pperk;
    }
    while (true) {
        BGSRadiationStage* prad = listRadiation.m_item;
        if (!prad) {
            break;
        }
        listRadiation.RemoveHead();
        delete prad;
    }
    while (true) {
        BGSDehydrationStage* ph20 = listDehydration.m_item;
        if (!ph20) {
            break;
        }
        listDehydration.RemoveHead();
        delete ph20;
    }
    while (true) {
        BGSHungerStage* pFood = listHunger.m_item;
        if (!pFood) {
            break;
        }
        listHunger.RemoveHead();
        delete pFood;
    }
    while (true) {
        BGSSleepDeprevationStage* pSleep = listSleepDeprevation.m_item;
        if (!pSleep) {
            break;
        }
        listSleepDeprevation.RemoveHead();
        delete pSleep;
    }
    while (true) {
        BGSBodyPartData* ppartdata = listPartData.m_item;
        if (!ppartdata) {
            break;
        }
        listPartData.RemoveHead();
        delete ppartdata;
    }
    pRegionList->Clear();
    NavMeshObstacleManager::GetInstance()->ClearAllObstacles();
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
        BGSCameraShot* pshot = listCameraShots.m_item;
        if (!pshot) {
            break;
        }
        listCameraShots.RemoveHead();
        delete pshot;
    }
    while (true) {
        BGSLightingTemplate* plt = listLightingTemplates.m_item;
        if (!plt) {
            break;
        }
        listLightingTemplates.RemoveHead();
        delete plt;
    }
    EffectSettingCollection::Reset();
    ActorValue::FreeActorValues();
    while (true) {
        BGSNote* pnote = listNotes.m_item;
        if (!pnote) {
            break;
        }
        listNotes.RemoveHead();
        delete pnote;
    }
    while (true) {
        BGSRagdoll* pragdoll = listRagdolls.m_item;
        if (!pragdoll) {
            break;
        }
        listRagdolls.RemoveHead();
        delete pragdoll;
    }
    while (true) {
        TESCaravanDeck* pDeck = static_cast<TESCaravanDeck*>(listCaravanDecks.m_item);
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
                    const char* pFileName;
                    if (pForm->GetFile(-1)) {
                        pFileName = pForm->GetFile(-1)->m_Filename;
                    }
                    else {
                        pFileName = "UNKNOWN";
                    }
                    const char* pFormTypeString = pForm->GetFormTypeString();
                    int32_t iFormID = pForm->iFormID;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "FORMS: Form '%s' (%08X) of type %s in file '%s' was not freed.",
                        pEditorID,
                        iFormID,
                        pFormTypeString,
                        pFileName
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
    if (!bSaveLoadGame && this->pActiveFile) {
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
    int32_t iCellCount = arrayInteriorCells.m_usESize;
    for (int32_t iOuter = 0; iOuter < iCellCount - 1; ++iOuter) {
        TESObjectCELL* pOutterCell = arrayInteriorCells[iOuter];
        TESObjectCELL* pCurrentCell = pOutterCell;
        int32_t iCurrent = iOuter;
        for (int32_t iInner = iOuter + 1; iInner < iCellCount; ++iInner) {
            TESObjectCELL* pInnerCell = arrayInteriorCells[iInner];
            if (pCurrentCell && pInnerCell) {
                const char* pCurrentName = pCurrentCell->GetCellName();
                const char* pInnerName = pInnerCell->GetCellName();
                if (BSstricmp(pInnerName, pCurrentName) < 0) {
                    pCurrentCell = pInnerCell;
                    iCurrent = iInner;
                }
            }
            else if (!pCurrentCell) {
                pCurrentCell = pInnerCell;
                iCurrent = iInner;
            }
        }
        if (pCurrentCell && pCurrentCell != pOutterCell) {
            arrayInteriorCells.SetAt(iOuter, pCurrentCell);
            arrayInteriorCells.SetAt(iCurrent, pOutterCell);
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
    TESObjectREFR* pRef = nullptr;
    if (apObject && apObject->GetFormType() == EXPL_ID) {
        BGSExplosion* pExplosionForm = static_cast<BGSExplosion*>(apObject);
        TESObjectCELL* pNewParent = nullptr;
        if (apWorld) {
            int32_t iCellX = FloatToInt(aLocation.x) >> 12;
            int32_t iCellY = FloatToInt(aLocation.y) >> 12;
            pNewParent = apWorld->GetCellFromCellCoord(iCellX, iCellY);
            if (!pNewParent && apWorld != pPlayer->GetWorldSpace()) {
                Warning("CELLS: Trying to create an explosion in an unloaded cell: %i, %i.", iCellX, iCellY);
            }
        }
        else {
            pNewParent = apInterior;
        }
        NiMatrix3 kRot;
        kRot.FromEulerAnglesZYX(aDirection.z, aDirection.y, aDirection.x);
        TESObjectREFR* pOwner = nullptr;
        uint32_t uiFormID = apObject->iFormID;
        // The player owns the explosions of these three explosion forms.
        if (uiFormID == 0xCDEF0 || uiFormID == 0x16AF00 || uiFormID == 0x172FBD) {
            pOwner = pPlayer;
        }
        pRef = Explosion::SpawnExplosion(pExplosionForm, pOwner, nullptr, pNewParent, aLocation, kRot);
        if (!pNewParent && apWorld) {
            pRef->SetRefPersists(true);
            pRef->m_Extra.SetPersistentCell(apWorld->pPersistentCell);
        }
    }

    return pRef;
}

void TESDataHandler::CheckIcons() {
    NiTMapItem<uint32_t, TESForm*>* it;
    if (!bFileSkipIconChecks.Bool() && TESForm::pAllForms) {
        TESForm* pForm = nullptr;
        it = TESForm::pAllForms->GetFirstPos();
        uint32_t ignore = 0;
        while (true) {
            do {
                if (!it) {
                    return;
                }
                TESForm::pAllForms->GetNext(it, ignore, pForm);
            }
            while (!pForm);
            TESIcon* pIcon = dynamic_cast<TESIcon*>(pForm);
            TESTexture* pTexture = dynamic_cast<TESTexture*>(pForm);
            TESBipedModelForm* pBiped = dynamic_cast<TESBipedModelForm*>(pForm);
            if (pIcon) {
                if (bFileShowIcons.Bool() &&
                    (pForm->GetFormType() != LIGH_ID ||
                     static_cast<TESObjectLIGH*>(pForm)->TESObjectLIGH::GetCanCarry())) {
                    if (pIcon->GetTextureNameLength()) {
                        char acIconPath[268];
                        const char* pTextureName = pIcon->GetTextureName();
                        BSsprintf(acIconPath, 260, "%s%s", pIcon->GetDefaultPath(), pTextureName);
                        if (FileFinder::Locate(acIconPath, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                            const char* pEditorID = pForm->GetFormEditorID();
                            uint8_t ucFormType = pForm->GetFormType();
                            char* pFormString = FormIDToString(ucFormType);
                            Warning(
                                "FORMS: Icon '%s' missing for %s '%s'.",
                                pIcon->GetTextureName(),
                                pFormString,
                                pEditorID
                            );
                        }
                    }
                    else {
                        const char* pEditorID = pForm->GetFormEditorID();
                        uint8_t ucFormType = pForm->GetFormType();
                        Warning("FORMS: Icon missing for %s '%s'.", FormIDToString(ucFormType), pEditorID);
                    }
                }
            }
            else if (pTexture) {
                if (bFileShowTextures.Bool()) {
                    if (pTexture->GetTextureNameLength()) {
                        char acTexturePath[271];
                        if (pForm->GetFormType() == LTEX_ID) {
                            const char* pTextureName = pTexture->GetTextureName();
                            BSsprintf(
                                acTexturePath,
                                260,
                                "%s\\Landscape\\%s",
                                pTexture->GetDefaultPath(),
                                pTextureName
                            );
                        }
                        else {
                            const char* pTextureName = pTexture->GetTextureName();
                            BSsprintf(acTexturePath, 260, "%s%s", pTexture->GetDefaultPath(), pTextureName);
                        }
                        if (FileFinder::Locate(acTexturePath, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                            const char* pEditorID = pForm->GetFormEditorID();
                            uint8_t ucFormType = pForm->GetFormType();
                            char* pFormString = FormIDToString(ucFormType);
                            Warning(
                                "TEXTURES: Texture '%s' missing for %s '%s'.",
                                pTexture->GetTextureName(),
                                pFormString,
                                pEditorID
                            );
                        }
                    }
                    else {
                        char cWarn = 1;
                        int32_t iFormType = pForm->GetFormType();
                        if (iFormType == CLAS_ID) {
                            cWarn = 0;
                        }
                        else if (iFormType == WRLD_ID) {
                            cWarn = 0;
                        }
                        if (cWarn) {
                            const char* pEditorID = pForm->GetFormEditorID();
                            uint8_t ucFormType = pForm->GetFormType();
                            Warning("TEXTURES: Texture missing for %s '%s'.", FormIDToString(ucFormType), pEditorID);
                        }
                    }
                }
            }
            else if (pBiped && bFileShowIcons.Bool()) {
                const char* pIconFilename;
                pIconFilename = pBiped->GetIcon(TESActorBase::SEX_MALE);
                if (BSstrlen(pIconFilename)) {
                    char acIconPath[268];
                    BSsprintf(acIconPath, 260, "%s%s", pBiped->GetIconDefaultPath(), pIconFilename);
                    if (FileFinder::Locate(acIconPath, nullptr, 0, 0xFFFFFFFFu) == FileFinder::NOTFOUND) {
                        const char* pEditorID = pForm->GetFormEditorID();
                        uint8_t ucFormType = pForm->GetFormType();
                        Warning(
                            "MENUS: Icon '%s' missing for %s '%s'.",
                            pIconFilename,
                            FormIDToString(ucFormType),
                            pEditorID
                        );
                    }
                }
                else {
                    const char* pEditorID = pForm->GetFormEditorID();
                    uint8_t ucFormType = pForm->GetFormType();
                    Warning("FORMS: Icon missing for %s '%s'.", FormIDToString(ucFormType), pEditorID);
                }
            }
        }
    }
}

TESTopic* TESDataHandler::GetGlobalTopic(HardcodedFormIDs aeID) {
    TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(aeID));
    return dynamic_cast<TESTopic*>(pForm);
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
        MEM_CONTEXT_TES_FORMS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESDataHandler.cpp",
        7192
    );
    TESForm* pForm = nullptr;
    switch (aType) {
        case 4: {
            pForm = new BGSTextureSet;
            break;
        }
        case 5: {
            pForm = new BGSMenuIcon;
            break;
        }
        case 6: {
            pForm = new TESGlobal;
            break;
        }
        case 7: {
            pForm = new TESClass;
            break;
        }
        case 8: {
            pForm = new TESFaction;
            break;
        }
        case 9: {
            pForm = new BGSHeadPart;
            break;
        }
        case 10: {
            pForm = new TESHair;
            break;
        }
        case 11: {
            pForm = new TESEyes;
            break;
        }
        case 12: {
            pForm = new TESRace;
            break;
        }
        case 13: {
            pForm = new TESSound;
            break;
        }
        case 14: {
            pForm = new BGSAcousticSpace;
            break;
        }
        case 15: {
            pForm = new TESSkill;
            break;
        }
        case 16: {
            pForm = new EffectSetting;
            break;
        }
        case 17: {
            pForm = new Script;
            break;
        }
        case 18: {
            pForm = new TESLandTexture;
            break;
        }
        case 19: {
            pForm = new EnchantmentItem;
            break;
        }
        case 20: {
            pForm = new SpellItem;
            break;
        }
        case 21: {
            pForm = new TESObjectACTI;
            break;
        }
        case 22: {
            pForm = new BGSTalkingActivator;
            break;
        }
        case 23: {
            pForm = new BGSTerminal;
            break;
        }
        case 24: {
            pForm = new TESObjectARMO;
            break;
        }
        case 25: {
            pForm = new TESObjectBOOK;
            break;
        }
        case 26: {
            pForm = new TESObjectCLOT;
            break;
        }
        case 27: {
            pForm = new TESObjectCONT;
            break;
        }
        case 28: {
            pForm = new TESObjectDOOR;
            break;
        }
        case 29: {
            pForm = new IngredientItem;
            break;
        }
        case 30: {
            pForm = new TESObjectLIGH;
            break;
        }
        case 31: {
            pForm = new TESObjectMISC;
            break;
        }
        case 32: {
            pForm = new TESObjectSTAT;
            break;
        }
        case 33: {
            pForm = new BGSStaticCollection;
            break;
        }
        case 34: {
            BGSMovableStatic* pMovableStatic;
            pMovableStatic = new BGSMovableStatic;
            pForm = static_cast<TESObjectSTAT*>(pMovableStatic);
            break;
        }
        case 35: {
            pForm = new BGSPlaceableWater;
            break;
        }
        case 36: {
            pForm = new TESGrass;
            break;
        }
        case 37: {
            pForm = new TESObjectTREE;
            break;
        }
        case 38: {
            TESFlora* pFlora;
            pFlora = new TESFlora;
            pForm = static_cast<TESObjectACTI*>(pFlora);
            break;
        }
        case 39: {
            pForm = new TESFurniture;
            break;
        }
        case 40: {
            pForm = new TESObjectWEAP;
            break;
        }
        case 41: {
            pForm = new TESAmmo;
            break;
        }
        case 42: {
            pForm = new TESNPC;
            break;
        }
        case 43: {
            pForm = new TESCreature;
            break;
        }
        case 44: {
            pForm = new TESLevCreature;
            break;
        }
        case 45: {
            pForm = new TESLevCharacter;
            break;
        }
        case 46: {
            pForm = new TESKey;
            break;
        }
        case 47: {
            pForm = new AlchemyItem;
            break;
        }
        case 48: {
            pForm = new BGSIdleMarker;
            break;
        }
        case 49: {
            pForm = new BGSNote;
            break;
        }
        case 50: {
            pForm = new BGSConstructibleObject;
            break;
        }
        case 51: {
            pForm = new BGSProjectile;
            break;
        }
        case 52: {
            pForm = new TESLevItem;
            break;
        }
        case 53: {
            pForm = new TESWeather;
            break;
        }
        case 54: {
            pForm = new TESClimate;
            break;
        }
        case 55: {
            pForm = new TESRegion;
            break;
        }
        case 57: {
            pForm = new TESObjectCELL;
            break;
        }
        case 58: {
            pForm = new TESObjectREFR;
            break;
        }
        case 65: {
            pForm = new TESWorldSpace;
            break;
        }
        case 66: {
            pForm = new TESObjectLAND;
            break;
        }
        case 67: {
            pForm = new NavMesh;
            break;
        }
        case 71: {
            pForm = new TESQuest;
            break;
        }
        case 72: {
            pForm = new TESIdleForm;
            break;
        }
        case 73: {
            pForm = new TESPackage;
            break;
        }
        case 74: {
            pForm = new TESCombatStyle;
            break;
        }
        case 75: {
            pForm = new TESLoadScreen;
            break;
        }
        case 76: {
            pForm = new TESLevSpell;
            break;
        }
        case 77: {
            pForm = new TESObjectANIO;
            break;
        }
        case 78: {
            pForm = new TESWaterForm;
            break;
        }
        case 79: {
            pForm = new TESEffectShader;
            break;
        }
        case 81: {
            pForm = new BGSExplosion;
            break;
        }
        case 82: {
            pForm = new BGSDebris;
            break;
        }
        case 83: {
            pForm = new TESImageSpace;
            break;
        }
        case 84: {
            pForm = new TESImageSpaceModifier;
            break;
        }
        case 85: {
            pForm = new BGSListForm;
            break;
        }
        case 86: {
            pForm = new BGSPerk;
            break;
        }
        case 87: {
            pForm = new BGSBodyPartData;
            break;
        }
        case 88: {
            pForm = new BGSAddonNode;
            break;
        }
        case 89: {
            pForm = new ActorValueInfo(ActorValue::VALUE_NONE, nullptr);
            break;
        }
        case 90: {
            pForm = new BGSRadiationStage;
            break;
        }
        case 91: {
            pForm = new BGSCameraShot;
            break;
        }
        case 92: {
            pForm = new BGSCameraPath;
            break;
        }
        case 93: {
            pForm = new BGSVoiceType;
            break;
        }
        case 94: {
            pForm = new BGSImpactData;
            break;
        }
        case 95: {
            pForm = new BGSImpactDataSet;
            break;
        }
        case 96: {
            pForm = new TESObjectARMA;
            break;
        }
        case 97: {
            pForm = new BGSEncounterZone;
            break;
        }
        case 98: {
            pForm = new BGSMessage;
            break;
        }
        case 99: {
            pForm = new BGSRagdoll;
            break;
        }
        case 101: {
            pForm = new BGSLightingTemplate;
            break;
        }
        case 102: {
            pForm = new BGSMusicType;
            break;
        }
        case 103: {
            pForm = new TESObjectIMOD;
            break;
        }
        case 104: {
            pForm = new TESReputation;
            break;
        }
        case 106: {
            pForm = new TESRecipe;
            break;
        }
        case 107: {
            pForm = new TESRecipeCategory;
            break;
        }
        case 108: {
            pForm = new TESCasinoChips;
            break;
        }
        case 109: {
            pForm = new TESCasino;
            break;
        }
        case 110: {
            pForm = new TESLoadScreenType;
            break;
        }
        case 111: {
            pForm = new MediaSet;
            break;
        }
        case 112: {
            pForm = new MediaLocationController;
            break;
        }
        case 113: {
            pForm = new TESChallenge;
            break;
        }
        case 114: {
            pForm = new TESAmmoEffect;
            break;
        }
        case 115: {
            pForm = new TESCaravanCard;
            break;
        }
        case 116: {
            pForm = new TESCaravanMoney;
            break;
        }
        case 117: {
            pForm = new TESCaravanDeck;
            break;
        }
        case 118: {
            pForm = new BGSDehydrationStage;
            break;
        }
        case 119: {
            pForm = new BGSHungerStage;
            break;
        }
        case 120: {
            pForm = new BGSSleepDeprevationStage;
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
    return pForm;
}

void TESDataHandler::EnsureFactionFriendshipsAreTwoWay() {
    for (TESFaction* pFaction : *pDataHandler->GetFactionList()) {
        if (pFaction) {
            for (BSSimpleList<GROUP_REACTION*>* pNode = static_cast<TESReactionForm*>(pFaction)->QReactionList(); pNode;
                 pNode = pNode->m_pkNext) {
                GROUP_REACTION* pReaction = pNode->m_item;
                if (pReaction &&
                    pReaction->pForm &&
                    pReaction->pForm->GetFormType() == FACT_ID &&
                    (pReaction->iFightReaction == FIGHT_REACTION_FRIEND ||
                     pReaction->iFightReaction == FIGHT_REACTION_ALLY)) {
                    TESFaction* pOtherFaction = static_cast<TESFaction*>(pReaction->pForm);
                    int32_t iReaction = static_cast<TESReactionForm*>(pOtherFaction)
                                            ->GetFactionCombatRelation(pFaction);
                    if (iReaction != FIGHT_REACTION_ALLY && iReaction != FIGHT_REACTION_FRIEND) {
                        int32_t iFactionID3 = pFaction->iFormID;
                        const char* pcFactionName4 = pFaction->GetFormEditorID();
                        const char* pcFactionName3 = pFaction->GetFormEditorID();
                        const char* pcOtherName3 = pOtherFaction->GetFormEditorID();
                        int32_t iFactionID2 = pFaction->iFormID;
                        const char* pcFactionName2 = pFaction->GetFormEditorID();
                        int32_t iOtherID2 = pOtherFaction->iFormID;
                        const char* pcOtherName2 = pOtherFaction->GetFormEditorID();
                        int32_t iOtherID = pOtherFaction->iFormID;
                        const char* pcOtherName = pOtherFaction->GetFormEditorID();
                        int32_t iFactionID = pFaction->iFormID;
                        Warning(
                            "DEFAULT: (Faction Reaction Error) Faction '%s' (%08X) is a Friend or Ally of "
                            "Faction '%s' (%08X), but Faction '%s' (%08X) is not a Friend or Ally of "
                            "Faction '%s' (%08X). Faction '%s'(%08X) will be forced to be a Friend of "
                            "Faction '%s' (%08X) ",
                            pFaction->GetFormEditorID(),
                            iFactionID,
                            pcOtherName,
                            iOtherID,
                            pcOtherName2,
                            iOtherID2,
                            pcFactionName2,
                            iFactionID2,
                            pcOtherName3,
                            pcFactionName3,
                            pcFactionName4,
                            iFactionID3
                        );
                        pOtherFaction->SetFactionReaction(pFaction, FIGHT_REACTION_FRIEND);
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
        pInstance = new Sky;
        pContext->~AutoMemContext();
    }
    return pInstance;
}

const char* TESBipedModelForm::GetIconDefaultPath() {
    return inventoryIcon->GetDefaultPath();
}

// Calls GetFileList twice.
void TESForm::ClearFiles() {
    if (GetFileList()) {
        GetFileList()->RemoveAll();
    }
}

// Swaps the grid position read from a big-endian file; the flags byte is left alone. IDLE_PARENT_IDS::Endian shares
// the code.
void EXTERIOR_DATA::Endian() {
    BSEndian(iCellX, false);
    BSEndian(iCellY, false);
}

bool TESObjectWEAP::IsEmbeddedWeapon() {
    return (data.cFlags & WEAPON_FLAG_EMBEDDED) != 0;
}

bool BGSSaveLoadGame::GetThreadAllowChanges() {
    return (uiThreadSpecificFlags & THREAD_FLAG_DISALLOW_CHANGES) == 0;
}

bool BGSSaveLoadGame::SetThreadAllowChanges(bool abValue) {
    bool bThreadAllowChanges = GetThreadAllowChanges();
    if (abValue) {
        uiThreadSpecificFlags &= ~THREAD_FLAG_DISALLOW_CHANGES;
    }
    else {
        uiThreadSpecificFlags |= THREAD_FLAG_DISALLOW_CHANGES;
    }
    return bThreadAllowChanges;
}
