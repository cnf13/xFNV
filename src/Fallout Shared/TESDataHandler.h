#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include <stdint.h>

class BGSAcousticSpace;
class BGSBodyPartData;
class BGSCameraShot;
class BGSDebris;
class BGSDehydrationStage;
class BGSExplosion;
class BGSHeadPart;
class BGSHungerStage;
class BGSImpactData;
class BGSImpactDataSet;
class BGSLightingTemplate;
class BGSListForm;
class BGSMenuIcon;
class BGSMessage;
class BGSMusicType;
class BGSNote;
class BGSPerk;
class BGSPrimitive;
class BGSProjectile;
class BGSRadiationStage;
class BGSRagdoll;
class BGSSleepDeprevationStage;
class BGSVoiceType;
struct FORM;
class InventoryChanges;
class ItemChange;
class MediaLocationController;
class MediaSet;
class NiTriBasedGeom;
class Script;
class TESCasino;
class TESChallenge;
class TESClass;
class TESCombatStyle;
class TESEffectShader;
class TESEyes;
class TESFaction;
class TESFile;
class TESGlobal;
class TESHair;
class TESIdleForm;
class TESLandTexture;
class TESLoadScreen;
class TESLoadScreenType;
class TESObjectACTI;
class TESObjectANIO;
class TESObjectCELL;
class TESObjectCONT;
class TESObjectDOOR;
class TESObjectMISC;
class TESObjectREFR;
class TESObjectSTAT;
class TESObjectWEAP;
class TESQuest;
class TESRace;
class TESRecipe;
class TESRegionDataManager;
class TESRegionList;
class TESReputation;
class TESSound;
class TESTopic;
class TESTopicInfo;
class TESWaterForm;
class TESWorldSpace;

// The engine's start-up log, called printf-style. It is an empty function in this build, so its callers pass anything
// from no argument to a message and its values.
void InitializationFlow(...);
// Splits apPath into device, directory, file name and extension, each into a buffer of the given size (a null buffer is
// skipped).
int32_t BSsplitpath(const char* apPath, char* apDevice, int32_t aiDeviceLen, char* apDir, int32_t aiDirLen,
	char* apFile, int32_t aiFileLen, char* apExt, int32_t aiExtLen);
int32_t makepath(char* a1, uint32_t a2, char* a3, char* a4, char* a5, char* a6);

// The load errors TESDataHandler::DataError reports. 3 is passed when ConstructObjectList fails and 4 when a compiled
// file cannot be opened (CompileFiles).
enum DAT_ERROR : uint32_t {
	DAT_ERROR_GENERIC,
	DAT_ERROR_NO_FILE,
	DAT_ERROR_LINK,
	DAT_ERROR_UNKNOWN_FORM,
	DAT_ERROR_FILE_FAILURE,
};

// The hardcoded form IDs the game refers to by number. TESDataHandler::GetGlobalTopic takes one, and its callers pass
// kTopicHELLO and kTopicGOODBYE (210 and 212).
enum HardcodedFormIDs : int32_t {
	kDoorMarker = 0x1,
	kTravelMarker = 0x2,
	kNotehrMarker = 0x3,
	kPrisonMarker = 0x4,
	kDivineMarker = 0x5,
	kTempleMarker = 0x6,
	kPlayer = 0x7,
	kBobbyPin = 0xA,
	kLootBag = 0xE,
	kCaps001 = 0xF,
	kMapMarker = 0x10,
	kHorseMarker = 0x12,
	kCreatureFaction = 0x13,
	kMultiBoundMarker = 0x15,
	kDefaultCameraPath = 0x16,
	kPlaneMarker = 0x17,
	kDefaultWater = 0x18,
	keyeReanimate = 0x1A,
	kDefaultAshPile1 = 0x1B,
	kPlayerBodyPartData = 0x1C,
	kDefaultBodyPartData = 0x1D,
	kNoZoneZone = 0x1E,
	kRoomMarker = 0x1F,
	kPortalMarker = 0x20,
	kCollisionMarker = 0x21,
	kDefaultAshPile2 = 0x22,
	kAudioMarker = 0x23,
	kAudioBuoyMarker = 0x24,
	kNullTextureSet = 0x28,
	kAdultMaleVoice1 = 0x2D,
	kAdultFemaleVoice1 = 0x2E,
	kCOCMarkerHeading = 0x32,
	kRadiationMarker = 0x33,
	kXMarkerHeading = 0x34,
	kGameYear = 0x35,
	kGameMonth = 0x36,
	kGameDay = 0x37,
	kGameHour = 0x38,
	kGameDaysPassed = 0x39,
	kTimeScale = 0x3A,
	kXMarker = 0x3B,
	kTamrielWorldspace_OBLIVION = 0x3C,
	kDefaultCombatstyle = 0x3D,
	kPlayCredits = 0x63,
	kFurnitureMarker01 = 0x64,
	kFurnitureMarker02 = 0x65,
	kFurnitureMarker03 = 0x66,
	kFurnitureMarker04 = 0x67,
	kFurnitureMarker05 = 0x68,
	kFurnitureMarker06 = 0x69,
	kFurnitureMarker07 = 0x6A,
	kFurnitureMarker08 = 0x6B,
	kFurnitureMarker09 = 0x6C,
	kFurnitureMarker10 = 0x6D,
	kFurnitureMarker11 = 0x6E,
	kFurnitureMarker12 = 0x6F,
	kFurnitureMarker13 = 0x70,
	kFurnitureMarker14 = 0x71,
	kFurnitureMarker15 = 0x72,
	kFurnitureMarker16 = 0x73,
	kFurnitureMarker17 = 0x74,
	kFurnitureMarker18 = 0x75,
	kFurnitureMarker19 = 0x76,
	kFurnitureMarker20 = 0x77,
	kFurnitureMarker21 = 0x78,
	kFurnitureMarker22 = 0x79,
	kFurnitureMarker23 = 0x7A,
	kFurnitureMarker24 = 0x7B,
	kFurnitureMarker25 = 0x7C,
	kFurnitureMarker26 = 0x7D,
	kFurnitureMarker27 = 0x7E,
	kFurnitureMarker28 = 0x7F,
	kFurnitureMarker29 = 0x80,
	kFurnitureMarker30 = 0x81,
	kFurnitureMarker31 = 0x82,
	kFurnitureMarker32 = 0x83,
	kFurnitureMarker33 = 0x84,
	kFurnitureMarker34 = 0x85,
	kFurnitureMarker35 = 0x86,
	kFurnitureMarker36 = 0x87,
	kFurnitureMarker37 = 0x88,
	kFurnitureMarker38 = 0x89,
	kFurnitureMarker39 = 0x8A,
	kFurnitureMarker40 = 0x8B,
	kTopicHELLO = 0xD2,
	kTopicGOODBYE = 0xD4,
	kSpeechChallengeFailure = 0xFD,
	kInfoRefusal = 0x118,
	kMagicFailureSoundAlteration = 0x12C,
	kMagicFailureSoundConjuration = 0x12D,
	kMagicFailureSoundDestruction = 0x12E,
	kMagicFailureSoundIllusion = 0x12F,
	kMagicFailureSoundMysticism = 0x130,
	kMagicFailureSoundRestoration = 0x131,
	kMagicEnchantDrawSoundAlteration = 0x138,
	kMagicEnchantDrawSoundConjuration = 0x139,
	kMagicEnchantDrawSoundDestruction = 0x13A,
	kMagicEnchantDrawSoundIllusion = 0x13B,
	kMagicEnchantDrawSoundMysticism = 0x13C,
	kMagicEnchantDrawSoundRestoration = 0x13D,
	kMagicEnchantHitSoundAlteration = 0x13E,
	kMagicEnchantHitSoundConjuration = 0x13F,
	kMagicEnchantHitSoundDestruction = 0x140,
	kMagicEnchantHitSoundIllusion = 0x141,
	kMagicEnchantHitSoundMysticism = 0x142,
	kMagicEnchantHitSoundRestoration = 0x143,
	keffectAbsorb = 0x144,
	keffectReflect = 0x145,
	kLifeDetected = 0x146,
	kspPipBoyLight = 0x147,
	kScriptEffect = 0x14A,
	kFireDamage = 0x14B,
	kFrostDamage = 0x14C,
	kShockDamage = 0x14D,
	kRestoreHealth = 0x14E,
	kUMON = 0x14F,
	kDefaultWeather = 0x15E,
	kDefaultClimate = 0x15F,
	kDefaultImageSpaceInterior = 0x160,
	kDefaultImageSpaceExterior = 0x161,
	kGetHit = 0x162,
	kHelpManualPC = 0x163,
	kImageSpaceConcussion = 0x164,
	kHelpManualXBox = 0x165,
	kExplosionInFace = 0x166,
	kFists = 0x1F4,
	kDefaultWaterExplosion = 0x1F5,
	kGasTrapDummy = 0x1F6,
	kMenuIconEquipTypeBigGuns = 0x258,
	kMenuIconEquipTypeEnergyWeapons = 0x259,
	kMenuIconEquipTypeSmallGuns = 0x25A,
	kMenuIconEquipTypeMeleeWeapons = 0x25B,
	kMenuIconEquipTypeUnarmedWeapons = 0x25C,
	kMenuIconEquipTypeThrowWeapons = 0x25D,
	kMenuIconEquipTypeMine = 0x25E,
	kMenuIconEquipTypeBodyWear = 0x25F,
	kMenuIconEquipTypeHeadWear = 0x260,
	kMenuIconEquipTypeHandWear = 0x261,
	kMenuIconEquipTypeChems = 0x262,
	kMenuIconEquipTypeStimpack = 0x263,
	kMenuIconEquipTypeFood = 0x264,
	kMenuIconEquipTypeAlcohol = 0x265,
	kDefaultImpactDataSet = 0x276,
	kBloodISFXd = 0x19482,
	kPopupBackgroundFX = 0x32B38,
	kInterfaceBackgroundFX = 0x44F34,
	kPauseBackgroundFX = 0x4EEE8,
	kNVNightVision = 0x8CD8C,
	kPipBackgroundFX = 0x96389,
	kNVSuperStimpak = 0xCCEF2,
	kWastelandNV = 0xDA726,
	kPerkWildWasteland = 0xED568,
	kCellEndgame = 0x161E98,
	kWeathered10mmPistol = 0x1735D4,
	kVATSBannedWeaponsList = 0x174256,
	kBannedEffectsOnSitters = 0x1768D7,
	kBannedVATSTargets = 0x1770BC,
};

// The engine's central registry: one list of forms per type, the loaded files and the form ID counter. Every list is a
// BSSimpleList of form pointers, typed where the item type is known, else a list of void* (FormList).
class TESDataHandler {
public:
	using FormList = BSSimpleList<void*>;

	// The region list's part of the save game.
	struct RegionList {
		// The bytes the region list's save takes: a count and two words per region.
		static uint16_t GetSaveSize();
	};

	TESDataHandler();
	~TESDataHandler();

	void RemoveIDFromDataHandler(uint32_t aiID);

	BSSimpleList<BGSHeadPart*>* GetHeadPartList();
	BSSimpleList<TESHair*>* GetHairList();
	BSSimpleList<TESRace*>* GetRaceList();
	BSSimpleList<TESLandTexture*>* GetLandTextList();
	BSSimpleList<BGSCameraShot*>* GetCameraShotList();
	BSSimpleList<TESClass*>* GetClassList();
	BSSimpleList<TESFaction*>* GetFactionList();
	BSSimpleList<TESReputation*>* GetReputationList();
	BSSimpleList<TESChallenge*>* GetChallengeList();
	BSSimpleList<TESRecipe*>* GetRecipeList();
	BSSimpleList<TESCasino*>* GetCasinoList();
	BSSimpleList<Script*>* GetScriptList();
	BSSimpleList<TESSound*>* GetSoundList();
	BSSimpleList<BGSAcousticSpace*>* GetAcousticSpaceList();
	BSSimpleList<BGSRagdoll*>* GetRagdollList();
	BSSimpleList<TESGlobal*>* GetGlobalList();
	BSSimpleList<BGSVoiceType*>* GetVoiceTypeList();
	BSSimpleList<BGSImpactData*>* GetImpactDataList();
	BSSimpleList<BGSImpactDataSet*>* GetImpactDataSetList();
	BSSimpleList<TESTopic*>* GetTopicList();
	BSSimpleList<TESQuest*>* GetQuestList();
	BSSimpleList<TESCombatStyle*>* GetCombatStyleList();
	BSSimpleList<TESLoadScreen*>* GetLoadScreenList();
	BSSimpleList<TESWaterForm*>* GetWaterList();
	BSSimpleList<TESEffectShader*>* GetEffectShaderList();
	BSSimpleList<BGSProjectile*>* GetProjectileList();
	BSSimpleList<BGSRadiationStage*>* GetRadiationStageList();
	BSSimpleList<BGSDehydrationStage*>* GetDehydrationStageList();
	BSSimpleList<BGSHungerStage*>* GetHungerStageList();
	BSSimpleList<BGSSleepDeprevationStage*>* GetSleepDeprevationStageList();
	BSSimpleList<BGSDebris*>* GetDebrisList();
	BSSimpleList<BGSPerk*>* GetPerkList();
	BSSimpleList<BGSBodyPartData*>* GetPartDataList();
	BSSimpleList<BGSNote*>* GetNoteList();
	BSSimpleList<BGSListForm*>* GetListFormList();
	BSSimpleList<BGSMenuIcon*>* GetMenuIconList();
	BSSimpleList<TESObjectANIO*>* GetAnimObjectList();
	BSSimpleList<BGSMessage*>* GetMessageList();
	BSSimpleList<BGSLightingTemplate*>* GetLightingTemplateList();
	BSSimpleList<BGSMusicType*>* GetMusicTypeList();
	BSSimpleList<TESLoadScreenType*>* GetLoadScreenTypeList();
	BSSimpleList<MediaSet*>* GetMediaSetList();
	BSSimpleList<MediaLocationController*>* GetMediaLocationControllerList();
	FormList* GetPackageList();
	FormList* GetImageSpaceModifierList();
	FormList* GetWeatherList();
	FormList* GetEnchantmentItemList();
	FormList* GetSpellItemList();
	FormList* GetZoneList();
	FormList* GetRecipeCategoryList();
	FormList* GetCaravanDeckList();
	BSSimpleList<BGSExplosion*>* GetExplosionList();
	BSSimpleList<TESFile*>* GetFileList();
	TESRegionList* GetRegionList() const;
	TESFile* GetActiveFile() const;
	uint32_t CountCompiled() const;

	bool GetSaveLoadGame() const;
	bool GetSaveLoad() const;
	// Unloads apCell.
	void UnloadCell(TESObjectCELL* apCell);
	void SetSaveLoad(bool abNewbool);
	bool GetClearingData() const;
	bool GetCheckingModels() const;
	bool GetLoadingFiles() const;
	void SetLoadingFiles(bool abNewbool);
	void SetSpotterShader(TESForm* apForm);
	void SetItemDetectedShader(TESForm* apForm);
	void SetCateyeShader(TESForm* apForm);

	int32_t GetFlag(uint8_t aucFlag) const;
	void SetDLCFlags(uint8_t aucFlags, bool abSet);
	int32_t GetDLCIndex(const char* apFileName) const;
	bool IsFormIDCreated(uint32_t aiFormID) const;
	// Launches a projectile of apObject at aLocation with aDirection's z and x angles in apInterior, or in
	// apWorld's cell at aLocation (a warning when that cell is not loaded); null without a cell or a projectile.
	TESObjectREFR* CreateProjectileAtLocation(TESBoundObject* apObject, const NiPoint3& aLocation,
		const NiPoint3& aDirection, TESObjectCELL* apInterior, TESWorldSpace* apWorld);
	void AdvanceNextID();

	uint32_t GetAddonNodeCount();
	BGSAddonNode* GetAddonNode(uint32_t aiIndex);

	TESFile* GetListFile(const char* apFileName);
	void CloseAllTES();
	void SetMasterFileLargeBuffer(int32_t aiLargeBuffer);
	// Saves pForm to the active file when that file owns it and is not a master, or the form was altered: a master
	// form saves as an edit and a deleted form is skipped.
	bool SaveForm(TESForm* pForm, bool master);
	TESSound* GetSound(const char* apEditorID);
	bool IsFormIDInUse(uint32_t aiFormID);
	TESSound* GetSound(uint32_t aiSearchID);
	TESFaction* GetFaction(uint32_t aiSearchID);
	TESLoadScreenType* GetLoadScreenType(uint32_t aiSearchID);
	void AddTopic(TESTopic* pNewTopic);
	void AddTopicInfo(TESTopicInfo* pNewTopicInfo);
	TESGlobal* GetGlobal(const char* apSearchID);
	void RunAllPersistentRefScripts();
	void FixAllScriptIDs();
	// Finds the plugins in apDirectory, orders masters before plugins (by write time) and their masters first.
	bool BuildFileList(const char* pdirectory);
	// Gives every checked file its copy for the calling thread.
	void CreateThreadSafeFiles();
	// Loads the checked files (or, for abDownloadedContentOnly, the downloaded ones that need it) and initialises
	// every loaded form.
	bool CompileFiles(bool abDownloadedContentOnly);
	// Reports a load error and quits.
	void DataError(DAT_ERROR error);

	// Files the form in the list for its type, the interior cell array, its cell (references) or the object list.
	bool AddFormToDataHandler(TESForm* pNew);
	// Recomputes the stats of every NPC whose level follows the player's.
	void UpdateAutoCalcNPCs();
	// Stores the addon node at its index, or at the first free index when that one is taken by another node.
	void AddAddonNode(BGSAddonNode* apAddonNode);
	// Empties the addon node's slot.
	void RemoveAddonNode(BGSAddonNode* apAddonNode);
	// The interior cell at aiIndex, after packing the array.
	TESObjectCELL* GetInteriorCellByIndex(uint32_t aiIndex);
	// The texture report of the editor build; the PC game keeps only the texture's type check.
	void PrintTextureInfo(const NiTexturingProperty::Map* pmap, NiTriBasedGeom* apGeometry, const char* apHeader,
		const char* apModel, bool abHasShader);
	// An interior cell, else a worldspace's cell, with that editor ID.
	TESObjectCELL* GetCellByEditorID(const char* apEditorID);
	TESObjectCELL* NewCell(const char* apName, int32_t aiX, int32_t aiY, TESWorldSpace* apWorldSpace);
	// Finds the exterior cell named apCellID in the plugin files without loading it: its
	// worldspace, and its coordinates in aiOutX, aiOutY.
	TESWorldSpace* GetExtCellDataFromFileByEditorID(const char* apCellID, int32_t& aiOutX, int32_t& aiOutY);
	// Whether the records of a group are read for the game (false for groups of editor-only data in masters).
	bool FormGroupLoadsInGame(TESFile* apFile, FORM* apFormGroup);
	// Hands out the next unused created form ID.
	uint32_t GetNextID();
	void EnumReferencesCloseToPoint(TESObjectCELL* apCell, const NiPoint3& aPoint1, float afRadius1,
		const NiPoint3& aPoint2, float afRadius2, bool (*apfnEnumFunc)(TESObjectREFR*, void*), void* aiEnumData);
	// The aiIndex-th animation object (0 is the first) that uses apIdle, or null.
	TESObjectANIO* GetAnimObject(TESIdleForm* apIdle, int32_t aiIndex);
	// Gets or creates the exterior cell at the coordinates.
	TESObjectCELL* GetCellFromCellCoord(int32_t aiX, int32_t aiY, TESWorldSpace* apWorldSpace, bool abCreateCell);
	TESObjectCELL* GetCellFromWorldCoord(float afX, float afY, TESWorldSpace* apWorldSpace, bool abCreateCell);
	// Returns &listWorldSpaces.
	BSSimpleList<TESWorldSpace*>* GetWorldSpaceList();
	// Reads every record of pFile into forms.
	bool ConstructObjectList(TESFile* pFile, bool abNoExistingForms);
	// Sorts the class and race lists by full name.
	void SortAllList();
	// The number of DLC flags set.
	int32_t GetNumDLCs();
	TESReputation* GetReputation(uint32_t aiID);
	FormList* GetClimateList();
	FormList* GetImageSpaceList();
	BSSimpleList<TESEyes*>* GetEyesList();
	// Creates the engine's built-in forms (markers, default water, player, globals, help messages, ...) that the
	// loaded files did not define, and points the default-object globals at them.
	void GenerateDefaultObjects();
	// Reads the record pFile is on into a form: the existing apOldForm or form with its ID when given, else a new
	// one filed in its list. False when the record is rejected.
	bool ConstructObject(TESFile* pFile, bool abNoExistingForms, TESForm* apOldForm,
		bool abIgnoreMissingLastLoadedCell);
	uint32_t AddDownloadedContentToFileList();
	TESEffectShader* SpotterEffectItem() const;
	uint32_t GetNextFormID() const;
	TESEffectShader* GetItemDetectedShader() const;
	void ClearBarterContainer();
	void ClearRecipeContainer();
	TESFile* GetCompiledFile(int32_t index);
	bool GetMasterSave();
	uint32_t GetInteriorCellCount();
	void SetCheckingModels(bool abNewbool);
	void SetIsLoading(bool a2);
	TESClass* GetClass(uint32_t a1);
	InventoryChanges* GetBarterContainer();
	TESRegionDataManager* GetRegionDataManager();
	// Creates a reference to apObject at aLocation facing aDirection in apInterior or
	// apWorld; returns it, or null.
	TESObjectREFR* CreateReferenceAtLocation(TESBoundObject* apObject, const NiPoint3& aLocation,
		const NiPoint3& aDirection, TESObjectCELL* apInterior, TESWorldSpace* apWorld, TESObjectREFR* apReference,
		BGSPrimitive* apAddPrimitive, void* apAdditionalData);
	void SetIsClosingFile(bool abClosing);
	void CheckModels(bool abUpdateBounds);
	void CheckTexturesRecurse(NiAVObject* apObject, const char* apModel);
	void CheckForNiRawImageData(char* pFilename);
	void CreateBarterContainer(TESObjectREFR* apVendor);
	void RemoveItemBarterContainer(ItemChange* apitem, int32_t aiNumber, TESObjectREFR* apVendor);
	void ClearFileList();
	bool ClearData();
	void SortInteriorCells();
	void CleanUpBadForms();
	TESObjectREFR* CreateExplosionAtLocation(TESBoundObject* apObject, const NiPoint3& aLocation,
		const NiPoint3& aDirection, TESObjectCELL* apInterior, TESWorldSpace* apWorld);
	void CheckIcons();
	// Returns the form dynamic-cast to TESTopic.
	TESTopic* GetGlobalTopic(HardcodedFormIDs a1);

	static bool LoadForm(TESForm* pForm, TESFile* pFile);
	static TESForm* CreateFormOfType(uint8_t aType);
	// Makes every faction's friend or ally treat it as a friend too.
	static void EnsureFactionFriendshipsAreTwoWay();
	static void RegisterError();
	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	// bits set by the DLC plugins that were found
	uint8_t ucDLCFlags;
	uint8_t padding01[3];
	TESObjectList* pObjectList;
	FormList listPackages;
	BSSimpleList<TESWorldSpace*> listWorldSpaces;
	FormList listClimates;
	FormList listImageSpaces;
	FormList listImageSpaceModifiers;
	FormList listWeather;
	FormList listEnchantmentItems;
	FormList listSpellItems;
	BSSimpleList<BGSHeadPart*> listHeadParts;
	BSSimpleList<TESHair*> listHair;
	BSSimpleList<TESEyes*> listEyes;
	BSSimpleList<TESRace*> listRaces;
	FormList listZones;
	BSSimpleList<TESLandTexture*> listLandTexts;
	BSSimpleList<BGSCameraShot*> listCameraShots;
	BSSimpleList<TESClass*> listClasses;
	BSSimpleList<TESFaction*> listFactions;
	BSSimpleList<TESReputation*> listReputations;
	BSSimpleList<TESChallenge*> listChallenges;
	BSSimpleList<TESRecipe*> listRecipes;
	FormList listRecipeCategories;
	FormList listAmmoEffects;
	BSSimpleList<TESCasino*> listCasinos;
	FormList listCaravanDecks;
	BSSimpleList<Script*> listScripts;
	BSSimpleList<TESSound*> listSounds;
	BSSimpleList<BGSAcousticSpace*> listAcousticSpaces;
	BSSimpleList<BGSRagdoll*> listRagdolls;
	BSSimpleList<TESGlobal*> listGlobals;
	BSSimpleList<BGSVoiceType*> listVoiceTypes;
	BSSimpleList<BGSImpactData*> listImpactData;
	BSSimpleList<BGSImpactDataSet*> listImpactDataSet;
	BSSimpleList<TESTopic*> listTopics;
	FormList listTopicInfos;
	BSSimpleList<TESQuest*> listQuests;
	BSSimpleList<TESCombatStyle*> listCombatStyles;
	BSSimpleList<TESLoadScreen*> listLoadScreens;
	BSSimpleList<TESWaterForm*> listWater;
	BSSimpleList<TESEffectShader*> listEffectShaders;
	BSSimpleList<BGSProjectile*> listProjectiles;
	BSSimpleList<BGSExplosion*> listExplosions;
	BSSimpleList<BGSRadiationStage*> listRadiation;
	BSSimpleList<BGSDehydrationStage*> listDehydration;
	BSSimpleList<BGSHungerStage*> listHunger;
	BSSimpleList<BGSSleepDeprevationStage*> listSleepDeprevation;
	BSSimpleList<BGSDebris*> listDebris;
	BSSimpleList<BGSPerk*> listPerks;
	BSSimpleList<BGSBodyPartData*> listPartData;
	BSSimpleList<BGSNote*> listNotes;
	BSSimpleList<BGSListForm*> listListForms;
	BSSimpleList<BGSMenuIcon*> listMenuIcons;
	BSSimpleList<TESObjectANIO*> animObjects;
	BSSimpleList<BGSMessage*> listMessages;
	BSSimpleList<BGSLightingTemplate*> listLightingTemplates;
	BSSimpleList<BGSMusicType*> listMusicTypes;
	BSSimpleList<TESLoadScreenType*> listLoadScreenTypes;
	BSSimpleList<MediaSet*> listMediaSets;
	BSSimpleList<MediaLocationController*> listMediaLocationControllers;
	TESRegionList* pRegionList;
	// The interior cells; ConstructObject grows it with SetAtGrow.
	NiTPrimitiveArray<TESObjectCELL*> arrayInteriorCells;
	// the BGSAddonNode forms by index
	NiTPrimitiveArray<BGSAddonNode*> arrayAddonNodes;
	// Forms that failed to load; ConstructObject searches it with FindPos and adds to its head.
	NiTList<TESForm*> listBadForms;
	// the next ID handed out for created forms
	uint32_t iNextID;
	TESFile* pActiveFile;
	BSSimpleList<TESFile*> listFiles;
	// number of files with a compile index
	uint32_t iNumCompile;
	// the files by compile index
	TESFile* pFileIndex[255];
	bool bMasterSave;
	bool bSaveLoadGame;
	bool bSaveLoad;
	bool bAutoSaving;
	bool bExportingPlugin;
	bool bClearingData;
	bool bHasDesiredFiles;
	bool bCheckingModels;
	bool bLoadingFiles;
	bool bDontRemoveIDs;
	uint8_t ucGameSettingsLoadState;
	uint8_t padding623;
	TESRegionDataManager* pRegionDataManager;
	InventoryChanges* pBarterContainer;
	InventoryChanges* pRecipeContainer;
	TESEffectShader* pSpotterShader;
	TESEffectShader* pItemDetectedShader;
	TESEffectShader* pCateyeMobileShader;
};

using NiTArray_BGSAddonNode_NiTMallocInterface_BGSAddonNode =
	NiTArray<BGSAddonNode*, NiTMallocInterface<BGSAddonNode*>>;

inline TESDataHandler* pDataHandler;

// The built-in forms TESDataHandler::GenerateDefaultObjects creates or finds (see also TESObjectREFR.h's markers).
inline TESObjectSTAT* pDoorMarker;
inline TESObjectSTAT* pMapMarker;
inline TESObjectSTAT* pAudioMarker;
inline TESObjectSTAT* pAudioBuoyMarker;
inline TESObjectSTAT* pPlaneMarker;
inline TESObjectSTAT* pPortalMarker;
inline TESObjectSTAT* pCollisionMarker;
inline TESObjectSTAT* pCOCMarkerHeading;
inline TESObjectSTAT* pTravelMarker;
inline TESObjectSTAT* pNorthMarker;
inline TESObjectDOOR* pJailMarker;
inline TESObjectSTAT* pTempleMarker;
inline TESObjectSTAT* pHolyMarker;
inline TESObjectSTAT* pRadiationMarker;
inline TESObjectMISC* pLockpick;
inline TESObjectSTAT* pHorseMarker;

// These globals follow pHorseMarker and are never read.
inline TESObjectMISC* pWelkyndStone;
inline TESObjectMISC* pVarlaStone;
inline TESObjectWEAP* pFists;
inline TESObjectACTI* pAshPile1;
inline TESObjectACTI* pAshPile2;
inline BGSExplosion* pWaterExplosion;
inline TESObjectWEAP* pGasTrapDummyWeap;
inline TESWaterForm* pDefaultWater;
inline BGSVoiceType* pMaleVoice;
inline BGSVoiceType* pFemaleVoice;
inline TESFaction* pCreatureFaction;
inline TESEyes* pReanimateEyes;
inline TESObjectCONT* pDefLootBag;
inline TESGlobal* pPlayCredits;

// A default-object pointer that GenerateDefaultObjects and ClearData clear and nothing reads.
inline TESForm* pUnreadDefaultObject;

// The cell, worldspace and topic info ConstructObject loaded last; references and land records go to the last cell.
inline TESObjectCELL* pLastLoadedCell;
inline TESWorldSpace* pLastLoadedWorld;
inline TESTopicInfo* pLastLoadedInfo;

// The game settings TESDataHandler.cpp defines.
extern SettingT<INISettingCollection> sLocalSavePath;
extern SettingT<INISettingCollection> bFileShowIcons;
extern SettingT<INISettingCollection> bFileSkipIconChecks;
extern SettingT<INISettingCollection> bFileTestLoad;
extern SettingT<INISettingCollection> bFileGoneMessage;
extern SettingT<INISettingCollection> sLocalMasterPath;
extern SettingT<INISettingCollection> bFileNeededMessage;
extern SettingT<INISettingCollection> bFileShowTextures;
extern SettingT<INISettingCollection> bFileUnusedObject;
extern SettingT<INISettingCollection> bFileCheckModelCollision;
extern SettingT<INISettingCollection> bFileSkipModelChecks;

// The form type of each OBJECT_TYPE, OBJECT_COUNT entries. CheckModels reads it by byte.
using ObjectIDArray = uint8_t[87];
inline ObjectIDArray aObjectTypeFormTypes{21, 24, 25, 27, 28, 29, 30, 31, 32, 36, 37, 40, 42, 43, 44, 45, 20, 19, 47,
	52, 46, 41, 38, 39, 13, 18, 74, 75, 76, 77, 78, 48, 79, 51, 22, 81, 4, 82, 5, 85, 86, 87, 88, 34, 23, 35, 91, 94,
	95, 71, 73, 93, 7, 12, 11, 9, 8, 49, 53, 54, 96, 6, 83, 84, 97, 98, 50, 14, 99, 17, 16, 102, 33, 103, 104, 106, 107,
	108, 109, 110, 111, 112, 113, 114, 115, 116, 117};
