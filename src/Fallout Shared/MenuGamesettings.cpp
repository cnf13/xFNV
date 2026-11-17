#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/CompanionWheelMenu.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/RouletteMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Misc/Main.h"

pSexNameGS_t pSexNameGS{&sSexMale, &sSexFemale};
pSexPossessiveGS_t pSexPossessiveGS{&sSexMalePossessive, &sSexFemalePossessive};
pSexPronounGS_t pSexPronounGS{&sSexMalePronoun, &sSexFemalePronoun};

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> sSexMale("sSexMale", "male");
SettingT<GameSettingCollection> sSexFemale("sSexFemale", "female");
SettingT<GameSettingCollection> sSexMalePossessive("sSexMalePossessive", "his");
SettingT<GameSettingCollection> sSexFemalePossessive("sSexFemalePossessive", "her");
SettingT<GameSettingCollection> sSexMalePronoun("sSexMalePronoun", "he");
SettingT<GameSettingCollection> sSexFemalePronoun("sSexFemalePronoun", "she");
SettingT<GameSettingCollection> sCursorFilename("sCursorFilename", "Interface\\Icons\\Misc\\cursor.dds");
SettingT<GameSettingCollection> sSpace("sSpace", " ");
SettingT<GameSettingCollection> sOk("sOk", "Ok");
SettingT<GameSettingCollection> sYes("sYes", "Yes");
SettingT<GameSettingCollection> sNo("sNo", "No");
SettingT<GameSettingCollection> sCancel("sCancel", "Cancel");
SettingT<GameSettingCollection> sAccept("sAccept", "Accept");
SettingT<GameSettingCollection> sNext("sNext", "Next");
SettingT<GameSettingCollection> sBack("sBack", "Back");
SettingT<GameSettingCollection> sFor("sFor", "for");
SettingT<GameSettingCollection> sTo("sTo", "to");
SettingT<GameSettingCollection> sGold("sGold", "gold");
SettingT<GameSettingCollection> sYour("sYour", "Your");
SettingT<GameSettingCollection> sSmall("sSmall", "Small");
SettingT<GameSettingCollection> sMedium("sMedium", "Medium");
SettingT<GameSettingCollection> sLarge("sLarge", "Large");
SettingT<GameSettingCollection> sChoose("sChoose", "Choose");
SettingT<GameSettingCollection> sDone("sDone", "Done");
SettingT<GameSettingCollection> sNone("sNone", "None");
SettingT<GameSettingCollection> sReturn("sReturn", "Return");
SettingT<GameSettingCollection> sOn("sOn", "ON");
SettingT<GameSettingCollection> sOff("sOff", "OFF");
SettingT<GameSettingCollection> sPlural("sPlural", "(s)");
SettingT<GameSettingCollection> sPluralPlain("sPluralPlain", "s");
SettingT<GameSettingCollection> sAddItemtoInventory("sAddItemtoInventory", "added");
SettingT<GameSettingCollection> sAddItemtoSpellList("sAddItemtoSpellList", "added");
SettingT<GameSettingCollection> sRemoveItemfromInventory("sRemoveItemfromInventory", "removed");
SettingT<GameSettingCollection> sEquipItemOnPlayer("sEquipItemOnPlayer", "was equipped");
SettingT<GameSettingCollection> sScrollEquipped("sScrollEquipped", " equipped.");
SettingT<GameSettingCollection> sUnequipItemOnPlayer("sUnequipItemOnPlayer", "was unequipped");
SettingT<GameSettingCollection> gs_sQuestAddedText("sQuestAddedText", "Quest added");
SettingT<GameSettingCollection> gs_sQuestCompletedText("sQuestCompletedText", "Quest completed");
static SettingT<GameSettingCollection> sQuestUpdated("sQuestUpdatedText", "Quest updated");
SettingT<GameSettingCollection> sQuestFailed("sQuestFailed", "Quest FAILED");
SettingT<GameSettingCollection> sContinue("sContinue", "Continue");
SettingT<GameSettingCollection> sMissingName("sMissingName", "<Missing Name>");
SettingT<GameSettingCollection> sMissingImage("sMissingImage", "Data\\Textures\\Interface\\Shared\\missing_image.dds");
SettingT<GameSettingCollection> sDefaultMessage("sDefaultMessage", "DEFAULT MESSAGE TEXT");
SettingT<GameSettingCollection> sLow("sLow", "Low");
SettingT<GameSettingCollection> sHigh("sHigh", "High");
SettingT<GameSettingCollection> sExit("sExit", "Exit");
SettingT<GameSettingCollection> sRemoteActivation("sRemoteActivation", "This object is activated from somewhere else.");
SettingT<GameSettingCollection> sNoSleepInOwnedBed("sNoSleepInOwnedBed", "You cannot sleep in an owned bed.");
SettingT<GameSettingCollection> sObjectInUse("sObjectInUse", "This object is already in use by someone else.");
SettingT<GameSettingCollection> sEnterName("sEnterName", "Enter character name.");
SettingT<GameSettingCollection> sOr("sOr", "OR");
SettingT<GameSettingCollection> sBuy("sBuy", "BUY");
SettingT<GameSettingCollection> sSell("sSell", "SELL");
SettingT<GameSettingCollection> sTransact("sTransact", "Transact");
SettingT<GameSettingCollection> sGiveAway("sGiveAway", "Give away");
static SettingT<GameSettingCollection> sMerchantNoCaps("sMerchantNoGold", "Merchant doesn't have enough caps.");
SettingT<GameSettingCollection> sNotEnoughCaps("sNotEnoughCaps", "You don't have enough caps.");
SettingT<GameSettingCollection> sOfferRefused("sOfferRefused", "Offer refused");
SettingT<GameSettingCollection> iCapsLimit("iCapsLimit", 1000000);
SettingT<GameSettingCollection> sCancelBarter("sCancelBarter", "Cancel transaction?");
SettingT<GameSettingCollection> sCancelRecipe("sCancelRecipe", "Cancel Recipe?");
SettingT<GameSettingCollection> sSkillRequirement("sSkillRequirement", "Skill Requirement");
SettingT<GameSettingCollection> sMadeAt("sMadeAt", "Made at ...");
SettingT<GameSettingCollection> sIngredients("sIngredients", "Ingredients");
SettingT<GameSettingCollection> sAttributesTitle("sAttributesTitle", "ATTRIBUTES: %d/%d Points Assigned");
SettingT<GameSettingCollection> sTraitsTitle("sTraitsTitle", "OPTIONAL TRAITS:  %d/%d Selected");
SettingT<GameSettingCollection> sSkillsTitle("sSkillsTitle", "SKILLS:  %d/%d Selected");
SettingT<GameSettingCollection> sAttributesCount("sAttributesCount", "Distribute %d Point");
SettingT<GameSettingCollection> sTraitsCount("sTraitsCount", "Choose up to %d Trait");
SettingT<GameSettingCollection> sSkillsCount("sSkillsCount", "Tag %d Skill");
SettingT<GameSettingCollection> fLoadingWheelScale("fLoadingWheelScale", 2.0f);
SettingT<GameSettingCollection> sCurrentObjective("sCurrentObjective", "CURRENT OBJECTIVE");
SettingT<GameSettingCollection> sLevelProgress("sLevelProgress", "Level Progress");
SettingT<GameSettingCollection> sVDSGPlate("sVDSGPlate", "VDSG Plate");
SettingT<GameSettingCollection> sNumberAbbrev("sNumberAbbrev", "No.");
SettingT<GameSettingCollection> iMaxStatsToDisplay("iMaxStatsToDisplay", 7);
SettingT<GameSettingCollection> sLoadScreenTypeDataTextureSpaceHint("sLoadScreenTypeDataTextureSpaceHint",
	"* Position/dimensions are relative to 1280 x 960 texture space!");
SettingT<GameSettingCollection> sLoadScreenTypeDataFontHint("sLoadScreenTypeDataFontHint",
	"* Font indices map to the corresponding texture filenames (\"sFontFile_#\") in Fallout.ini (PC/PS3) and "
	"FalloutXenon.ini (360)!");
SettingT<GameSettingCollection> sXPProgressMarker("sXPProgressMarker", "^");
SettingT<GameSettingCollection> sComputersHeader1("sComputersHeader1", "ROBCO INDUSTRIES UNIFIED OPERATING SYSTEM");
SettingT<GameSettingCollection> sComputersHeader2("sComputersHeader2", "COPYRIGHT 2075-2077 ROBCO INDUSTRIES");
SettingT<GameSettingCollection> sComputersWelcome("sComputersWelcome", "Welcome to the service on SERVER1");
SettingT<GameSettingCollection> sComputersBack("sComputersBack", "Back");
SettingT<GameSettingCollection> sComputersAddedNote("sComputersAddedNote", "Note Added: %s");
SettingT<GameSettingCollection> iComputersDisplayRateMenus("iComputersDisplayRateMenus", 150);
SettingT<GameSettingCollection> iComputersDisplayRateNotes("iComputersDisplayRateNotes", 150);
SettingT<GameSettingCollection> iComputersResultDisplayTimeout("iComputersResultDisplayTimeout", 5);
SettingT<GameSettingCollection> sComputersLogon("sComputersLogon", "LOGON ADMIN");
SettingT<GameSettingCollection> sHackingSecurityReset("sHackingSecurityReset", "SECURITY RESET...");
SettingT<GameSettingCollection> sHackingIntro01("sHackingIntro01", "WELCOME TO ROBCO INDUSTRIES (TM) TERMLINK");
SettingT<GameSettingCollection> sHackingIntro02("sHackingIntro02", "SET TERMINAL/INQUIRE");
SettingT<GameSettingCollection> sHackingIntro03("sHackingIntro03", "RIT-V300");
SettingT<GameSettingCollection> sHackingIntro04("sHackingIntro04", "SET FILE/PROTECTION=OWNER:RWED ACCOUNTS.F");
SettingT<GameSettingCollection> sHackingIntro05("sHackingIntro05", "SET HALT RESTART/MAINT");
SettingT<GameSettingCollection> sHackingIntro06("sHackingIntro06",
	"Initializing Robco Industries(TM) MF Boot Agent v2.3.0");
SettingT<GameSettingCollection> sHackingIntro07("sHackingIntro07", "RETROS BIOS");
SettingT<GameSettingCollection> sHackingIntro08("sHackingIntro08", "RBIOS-4.02.08.00 52EE5.E7.E8");
SettingT<GameSettingCollection> sHackingIntro09("sHackingIntro09", "Copyright 2201-2203 Robco Ind.");
SettingT<GameSettingCollection> sHackingIntro10("sHackingIntro10", "Uppermem: 64 KB");
SettingT<GameSettingCollection> sHackingIntro11("sHackingIntro11", "Root (5A8)");
SettingT<GameSettingCollection> sHackingIntro12("sHackingIntro12", "Maintenance Mode");
SettingT<GameSettingCollection> sHackingIntro13("sHackingIntro13", "RUN DEBUG/ACCOUNTS.F");
SettingT<GameSettingCollection> sHackingHeader("sHackingHeader", "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL");
SettingT<GameSettingCollection> sHackingHeader2("sHackingHeader2", "ENTER PASSWORD NOW");
SettingT<GameSettingCollection> sHackingHeader3("sHackingHeader3", "ATTEMPT(S) LEFT:");
SettingT<GameSettingCollection> sHackingWarning("sHackingWarning", "!!! WARNING: LOCKOUT IMMINENT !!!");
SettingT<GameSettingCollection> sHackingDenied("sHackingDenied", "Entry denied");
SettingT<GameSettingCollection> sHackingGranted("sHackingGranted", "Exact match!");
SettingT<GameSettingCollection> sHackingCorrect("sHackingCorrect", "correct");
SettingT<GameSettingCollection> sHackingLockout1("sHackingLockout1", "Lockout in");
SettingT<GameSettingCollection> sHackingLockout2("sHackingLockout2", "progress.");
SettingT<GameSettingCollection> sHackingLockout3("sHackingLockout3", "TERMINAL LOCKED");
SettingT<GameSettingCollection> sHackingLockout4("sHackingLockout4", "PLEASE CONTACT AN ADMINISTRATOR");
SettingT<GameSettingCollection> sHackingDudRemoved("sHackingDudRemoved", "Dud removed.");
SettingT<GameSettingCollection> sHackingToleranceReset1("sHackingToleranceReset1", "Allowance");
SettingT<GameSettingCollection> sHackingToleranceReset2("sHackingToleranceReset2", "replenished.");
SettingT<GameSettingCollection> sHackingAccessing1("sHackingAccessing1", "Please wait");
SettingT<GameSettingCollection> sHackingAccessing2("sHackingAccessing2", "while system");
SettingT<GameSettingCollection> sHackingAccessing3("sHackingAccessing3", "is accessed.");
SettingT<GameSettingCollection> iHackingInputRate("iHackingInputRate", 20);
SettingT<GameSettingCollection> iHackingOutputRate("iHackingOutputRate", 67);
SettingT<GameSettingCollection> iHackingDumpRate("iHackingDumpRate", 500);
SettingT<GameSettingCollection> iHackingFlashOnDuration("iHackingFlashOnDuration", 750);
SettingT<GameSettingCollection> iHackingFlashOffDuration("iHackingFlashOffDuration", 500);
SettingT<GameSettingCollection> iHackingMinWords("iHackingMinWords", 5);
SettingT<GameSettingCollection> iHackingMaxWords("iHackingMaxWords", 20);
SettingT<GameSettingCollection> sContainerItemsTitle("sContainerItemsTitle", "Inventory");
SettingT<GameSettingCollection> sSortMethod("sSortMethod", "Sort");
SettingT<GameSettingCollection> sName("sName", "Name");
SettingT<GameSettingCollection> sQuantity("sQuantity", "Quantity");
SettingT<GameSettingCollection> sHealth("sHealth", "Health");
SettingT<GameSettingCollection> sWeight("sWeight", "Weight");
SettingT<GameSettingCollection> sValue("sValue", "Value");
SettingT<GameSettingCollection> sTakeAll("sTakeAll", "Take All");
SettingT<GameSettingCollection> sSelect("sSelect", "Select");
SettingT<GameSettingCollection> sActiveMineDescription("sActiveMineDescription", "Live");
SettingT<GameSettingCollection> sAlreadyPlacedMine("sAlreadyPlacedMine",
	"An active mine has already been placed on this enemy.");
SettingT<GameSettingCollection> sJunk("sJunk", "Junk");
SettingT<GameSettingCollection> sPickpocketFail("sPickpocketFail", "You've been caught pickpocketing.");
SettingT<GameSettingCollection> sTeammateOverencumbered("sTeammateOverencumbered", "can't carry any more.");
SettingT<GameSettingCollection> fDialogZoomInSeconds("fDialogZoomInSeconds", 1.5f);
SettingT<GameSettingCollection> fDialogZoomOutSeconds("fDialogZoomOutSeconds", 0.5f);
SettingT<GameSettingCollection> fDialogSpeechDelaySeconds("fDialogSpeechDelaySeconds", 2.0f);
SettingT<GameSettingCollection> fDialogFocalDepthRange("fDialogFocalDepthRange", 300.0f);
SettingT<GameSettingCollection> fDialogFocalDepthStrength("fDialogFocalDepthStrength", 0.65f);
SettingT<GameSettingCollection> sSpeechChallengeSuccess("sSpeechChallengeSuccess", "[SUCCESS]");
SettingT<GameSettingCollection> sSpeechChallengeFailure("sSpeechChallengeFailure", "[FAILED]");
SettingT<GameSettingCollection> sHotKeyString("sHotKeyString", "Hot Key");
SettingT<GameSettingCollection> fHotKeyShowTimer("fHotKeyShowTimer", 1.5f);
SettingT<GameSettingCollection> fHotKeyDownTimer("fHotKeyDownTimer", 0.5f);
SettingT<GameSettingCollection> sCantHotkeyItem("sCantHotkeyItem", "You cannot hotkey that item.");
SettingT<GameSettingCollection> sCantHotkeyBrokenItem("sCantHotkeyBrokenItem", "You cannot hotkey broken items.");
SettingT<GameSettingCollection> sFullHealth("sFullHealth", "You are already at full health.");
SettingT<GameSettingCollection> fMinFatigue("fMinFatigue", -30.0f);
static SettingT<GameSettingCollection> iQuestReminderTime("iQuestReminderTimer", 600000);
static SettingT<GameSettingCollection> iQuestReminderTimeLimit("iQuestReminderTimerLimit", 10000);
SettingT<GameSettingCollection> fMaxRadiationLevel("fMaxRadiationLevel", 5.0f);
SettingT<GameSettingCollection> fDefaultNoticeTextDisplayTime("fDefaultNoticeTextDisplayTime", 0.5f);
SettingT<GameSettingCollection> fNoticeTextTimePerCharacter("fNoticeTextTimePerCharacter", 0.06f);
SettingT<GameSettingCollection> sHUDRads("sHUDRads", "Rads");
SettingT<GameSettingCollection> sHUDHealth("sHUDHealth", "Hlth");
SettingT<GameSettingCollection> sHUDQuestCompleted("sHUDQuestCompleted", "COMPLETED: ");
SettingT<GameSettingCollection> sHitPointsShort("sHitPointsShort", "HP");
SettingT<GameSettingCollection> sActionPointsShort("sActionPointsShort", "AP");
SettingT<GameSettingCollection> sRadsPerSecond("sRadsPerSecond", "RAD/SEC");
SettingT<GameSettingCollection> sEmpty("sEmpty", "[EMPTY]");
SettingT<GameSettingCollection> sExplosive("sExplosive", "EXPLOSIVE");
SettingT<GameSettingCollection> sDisarmMine("sDisarmMine", "Disarm Mine");
SettingT<GameSettingCollection> sLevelUp("sLevelUp", "LEVEL UP");
SettingT<GameSettingCollection> iKarmaChangeThreshold("iKarmaChangeThreshold", 250);
SettingT<GameSettingCollection> sKarmaMajorGained("sKarmaMajorGained", "Major Karma Gained");
SettingT<GameSettingCollection> sKarmaMinorGained("sKarmaMinorGained", "Minor Karma Gained");
SettingT<GameSettingCollection> sKarmaMajorLost("sKarmaMajorLost", "Major Karma Lost");
SettingT<GameSettingCollection> sKarmaMinorLost("sKarmaMinorLost", "Minor Karma Lost");
SettingT<GameSettingCollection> sKarmaMajorGainedImage("sKarmaMajorGainedImage",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sKarmaMinorGainedImage("sKarmaMinorGainedImage",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds");
SettingT<GameSettingCollection> sKarmaMajorLostImage("sKarmaMajorLostImage",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds");
SettingT<GameSettingCollection> sKarmaMinorLostImage("sKarmaMinorLostImage",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sReputationButton("sReputationButton", "Reputation");
SettingT<GameSettingCollection> iHUDMaxCompassNPCTicks("iHUDMaxCompassNPCTicks", 10);
SettingT<GameSettingCollection> fEffectTimerBeginsCountdown("fEffectTimerBeginsCountdown", 60.0f);
SettingT<GameSettingCollection> fSubtitleSpeechDelay("fSubtitleSpeechDelay", 0.15f);
SettingT<GameSettingCollection> fExplosiveIndicatorDistance("fExplosiveIndicatorDistance", 512.0f);
SettingT<GameSettingCollection> sHUDUseKey("sHUDUseKey", "Use Key");
SettingT<GameSettingCollection> fQuestCinematicCharacterFadeIn("fQuestCinematicCharacterFadeIn", 2.0f);
SettingT<GameSettingCollection> fQuestCinematicCharacterFadeOut("fQuestCinematicCharacterFadeOut", 2.0f);
SettingT<GameSettingCollection> fQuestCinematicCharacterRemain("fQuestCinematicCharacterRemain", 4.0f);
SettingT<GameSettingCollection> fQuestCinematicCharacterFadeInDelay("fQuestCinematicCharacterFadeInDelay", 0.1f);
SettingT<GameSettingCollection> fQuestCinematicObjectiveFadeIn("fQuestCinematicObjectiveFadeIn", 0.5f);
SettingT<GameSettingCollection> fQuestCinematicObjectiveFadeOut("fQuestCinematicObjectiveFadeOut", 0.5f);
SettingT<GameSettingCollection> fQuestCinematicObjectiveFadeInDelay("fQuestCinematicObjectiveFadeInDelay", 1.0f);
SettingT<GameSettingCollection> fQuestCinematicObjectiveScrollTime("fQuestCinematicObjectiveScrollTime", 0.5f);
SettingT<GameSettingCollection> fQuestCinematicObjectivePauseTime("fQuestCinematicObjectivePauseTime", 3.0f);
SettingT<GameSettingCollection> iQuestReminderPipboyDisabledTime("iQuestReminderPipboyDisabledTime", 30);
SettingT<GameSettingCollection> fSneakNoticeMin("fSneakNoticeMin", -20.0f);
SettingT<GameSettingCollection> sSneakHidden("sSneakHidden", "[HIDDEN]");
SettingT<GameSettingCollection> sSneakDetected("sSneakDetected", "[DETECTED]");
SettingT<GameSettingCollection> sSneakCaution("sSneakCaution", "[CAUTION]");
SettingT<GameSettingCollection> sSneakDanger("sSneakDanger", "[DANGER]");
SettingT<GameSettingCollection> sInaccessible("sInaccessible", "[INACCESSIBLE]");
SettingT<GameSettingCollection> sBroken("sBroken", "[Locked - Broken]");
SettingT<GameSettingCollection> iHUDCompassIconBlinkMaxDist("iHUDCompassIconBlinkMaxDist", 2000);
SettingT<GameSettingCollection> iHUDCompassIconBlinkMinDist("iHUDCompassIconBlinkMinDist", 256);
SettingT<GameSettingCollection> iHUDCompassIconBlinkThresholdDist("iHUDCompassIconBlinkThresholdDist", 500);
SettingT<GameSettingCollection> iHUDCompassIconBlinkSlowestBlinkTime("iHUDCompassIconBlinkSlowestBlinkTime", 750);
SettingT<GameSettingCollection> iHUDCompassIconBlinkFastestBlinkTime("iHUDCompassIconBlinkFastestBlinkTime", 1500);
SettingT<GameSettingCollection> iHUDCompassIconBlinkThresholdBlinkTime("iHUDCompassIconBlinkThresholdBlinkTime", 1000);
SettingT<GameSettingCollection> iHUDCompassIconBlinkLongestPause("iHUDCompassIconBlinkLongestPause", 1500);
SettingT<GameSettingCollection> iHUDCompassIconBlinkShortestPause("iHUDCompassIconBlinkShortestPause", 50);
SettingT<GameSettingCollection> iHUDCompassIconBlinkThresholdPause("iHUDCompassIconBlinkThresholdPause", 600);
SettingT<GameSettingCollection> sPrevious("sPrevious", "Previous");
SettingT<GameSettingCollection> sDisableHelp("sDisableHelp", "Disable Help");
SettingT<GameSettingCollection> sEnableHelp("sEnableHelp", "Enable Help");
SettingT<GameSettingCollection> sVDSGManual("sVDSGManual", "VDSG MANUAL");
SettingT<GameSettingCollection> sInventoryArmorClass("sInventoryArmorClass", "Armor Class");
SettingT<GameSettingCollection> sInventoryCaps("sInventoryCaps", "Caps");
SettingT<GameSettingCollection> sInventoryWeight("sInventoryWeight", "Wg");
SettingT<GameSettingCollection> sInventoryWeightUpper("sInventoryWeightUpper", "WG");
SettingT<GameSettingCollection> sInventoryDamage("sInventoryDamage", "DAM");
SettingT<GameSettingCollection> sInventoryDamagePerSecond("sInventoryDamagePerSecond", "DPS");
SettingT<GameSettingCollection> sInventoryDamageResistance("sInventoryDamageResistance", "DR");
SettingT<GameSettingCollection> sInventoryDamageThreshold("sInventoryDamageThreshold", "DT");
SettingT<GameSettingCollection> sInventoryCondition("sInventoryCondition", "CND");
SettingT<GameSettingCollection> sInventoryStrReq("sInventoryStrReq", "STR");
SettingT<GameSettingCollection> sInventoryAmmo("sInventoryAmmo", "AMMO");
SettingT<GameSettingCollection> sInventoryValue("sInventoryValue", "VAL");
SettingT<GameSettingCollection> sInventoryEffects("sInventoryEffects", "EFFECTS");
SettingT<GameSettingCollection> sModEffects("sModEffects", "MODS");
SettingT<GameSettingCollection> sInventoryItems("sInventoryItems", "ITEMS");
SettingT<GameSettingCollection> sInventoryQuantity("sInventoryQuantity", "QTY");
SettingT<GameSettingCollection> sInventoryName("sInventoryName", "NAME");
SettingT<GameSettingCollection> sInventoryWeapons("sInventoryWeapons", "Weapons");
SettingT<GameSettingCollection> sInventoryApparel("sInventoryApparel", "Apparel");
SettingT<GameSettingCollection> sInventoryRepair("sInventoryRepair", "Repair");
SettingT<GameSettingCollection> sInventoryAid("sInventoryAid", "Aid");
SettingT<GameSettingCollection> sInventoryAmmoTab("sInventoryAmmoTab", "Ammo");
SettingT<GameSettingCollection> sInventoryMisc("sInventoryMisc", "Misc");
SettingT<GameSettingCollection> sInventoryUnequip("sInventoryUnequip", "Unequip");
SettingT<GameSettingCollection> sInventoryEquip("sInventoryEquip", "Equip");
SettingT<GameSettingCollection> sInventoryUse("sInventoryUse", "Use");
SettingT<GameSettingCollection> sInventoryDrop("sInventoryDrop", "Drop");
SettingT<GameSettingCollection> sInventoryHotKey("sInventoryHotKey", "Hot Key");
SettingT<GameSettingCollection> sInventoryMod("sInventoryMod", "Mod");
SettingT<GameSettingCollection> sKeyring("sKeyring", "Keyring");
SettingT<GameSettingCollection> iInventoryAskQuantityAt("iInventoryAskQuantityAt", 5);
SettingT<GameSettingCollection> sRecipes("sRecipes", "RECIPES");
SettingT<GameSettingCollection> sLevelUpTitleText("sLevelUpTitleText", "WELCOME TO LEVEL %d");
SettingT<GameSettingCollection> sLevelUpSkillCounter("sLevelUpSkillCounter", "ASSIGN 1 SKILL POINT");
SettingT<GameSettingCollection> sLevelUpPerkCounter("sLevelUpPerkCounter", "CHOOSE 1 PERK");
SettingT<GameSettingCollection> sLevelUpSkillCounterPl("sLevelUpSkillCounterPl", "ASSIGN %d SKILL POINTS");
SettingT<GameSettingCollection> sLevelUpPerkCounterPl("sLevelUpPerkCounterPl", "CHOOSE %d PERKS");
SettingT<GameSettingCollection> sSkillIncreased("sSkillIncreased", "skill increased");
SettingT<GameSettingCollection> sSkillIncreasedNum("sSkillIncreasedNum", "%s increased by %d");
SettingT<GameSettingCollection> sSkillDecreased("sSkillDecreased", "skill decreased");
SettingT<GameSettingCollection> sRequirementsText("sRequirementsText", "Req");
SettingT<GameSettingCollection> sRanksText("sRanksText", "Ranks");
SettingT<GameSettingCollection> sLevelAbbrev("sLevelAbbrev", "Level");
SettingT<GameSettingCollection> sTraitMenuTitleText("sTraitMenuTitleText", "CHOOSE UP TO %d TRAITS");
SettingT<GameSettingCollection> sTraitMenuTitleTextSingular("sTraitMenuTitleTextSingular", "CHOOSE %d TRAIT");
SettingT<GameSettingCollection> sTraitMenuCounter("sTraitMenuCounter", "%d TRAITS LEFT");
SettingT<GameSettingCollection> sTraitMenuCounterSingular("sTraitMenuCounterSingular", "%d TRAIT LEFT");
SettingT<GameSettingCollection> iTraitMenuMaxNumTraits("iTraitMenuMaxNumTraits", 3);
SettingT<GameSettingCollection> sData("sData", "DATA");
SettingT<GameSettingCollection> sTravel("sTravel", "Travel");
SettingT<GameSettingCollection> sPlaceMarker("sPlaceMarker", "Place Marker");
SettingT<GameSettingCollection> sTravelQuestion("sTravelQuestion", "Do you want to travel to");
SettingT<GameSettingCollection> sSetMarkerQuestion("sSetMarkerQuestion", "Do you want to set your marker?");
SettingT<GameSettingCollection> sMoveMarkerQuestion("sMoveMarkerQuestion",
	"Do you want to move your marker or remove it?");
SettingT<GameSettingCollection> sMoveMarker("sMoveMarker", "Move It");
SettingT<GameSettingCollection> sLeaveMarker("sLeaveMarker", "Leave It");
SettingT<GameSettingCollection> sRemoveMarker("sRemoveMarker", "Remove It");
SettingT<GameSettingCollection> sMapMarkerNotFound("sMapMarkerNotFound", "You have not found this location yet.");
SettingT<GameSettingCollection> sTune("sTune", "Tune");
SettingT<GameSettingCollection> sJournalTitleActive("sJournalTitleActive", "Active Quest");
SettingT<GameSettingCollection> sJournalTitleCurrent("sJournalTitleCurrent", "Current Quests");
SettingT<GameSettingCollection> sJournalTitleCompleted("sJournalTitleCompleted", "Completed Quests");
SettingT<GameSettingCollection> sJournalTitleCompletedDetails("sJournalTitleCompletedDetails",
	"Completed Quest Detail");
SettingT<GameSettingCollection> sMapMarkerAdded("sMapMarkerAdded", "Map marker added.");
SettingT<GameSettingCollection> sLocalMapTabText("sLocalMapTabText", "Local Map");
SettingT<GameSettingCollection> sWorldMapTabText("sWorldMapTabText", "World Map");
SettingT<GameSettingCollection> sQuestsTabText("sQuestsTabText", "Quests");
SettingT<GameSettingCollection> sMiscTabText("sMiscTabText", "Misc");
SettingT<GameSettingCollection> sCommsTabText("sCommsTabText", "Radio");
SettingT<GameSettingCollection> sTextNote("sTextNote", "Text File");
SettingT<GameSettingCollection> sImageNote("sImageNote", "Image File");
SettingT<GameSettingCollection> sAudioNote("sAudioNote", "Audio File");
SettingT<GameSettingCollection> sRemaining("sRemaining", "remaining");
SettingT<GameSettingCollection> sPlayAudio("sPlayAudio", "Play Audio");
SettingT<GameSettingCollection> sStopAudio("sStopAudio", "Stop Audio");
SettingT<GameSettingCollection> sShowActiveNotes("sShowActiveNotes", "Show Active Quest Notes");
SettingT<GameSettingCollection> sShowAllNotes("sShowAllNotes", "Show All Notes");
SettingT<GameSettingCollection> sMakeActiveQuest("sMakeActiveQuest", "Make Active Quest");
SettingT<GameSettingCollection> sShowLocation("sShowLocation", "Show Location");
SettingT<GameSettingCollection> sRadioWaveformFileFormat("sRadioWaveformFileFormat", "Interface\\Radio Wave\\%s.dds");
SettingT<GameSettingCollection> fNoteGreyOutTime("fNoteGreyOutTime", 1.0f);
SettingT<GameSettingCollection> fWorldMapMinZoom("fWorldMapMinZoom", 0.75f);
SettingT<GameSettingCollection> fWorldMapMaxZoom("fWorldMapMaxZoom", 5.0f);
SettingT<GameSettingCollection> fWorldMapMarkerMinSize("fWorldMapMarkerMinSize", 20.0f);
SettingT<GameSettingCollection> fWorldMapMarkerMaxSize("fWorldMapMarkerMaxSize", 50.0f);
SettingT<GameSettingCollection> fWorldQuestMarkerMinSize("fWorldQuestMarkerMinSize", 30.0f);
SettingT<GameSettingCollection> fWorldQuestMarkerMaxSize("fWorldQuestMarkerMaxSize", 60.0f);
SettingT<GameSettingCollection> gs_fWorldfPlayerCursorMinSize("fWorldfPlayerCursorMinSize", 40.0f);
SettingT<GameSettingCollection> fWorldPlayerCursorMaxSize("fWorldPlayerCursorMaxSize", 70.0f);
SettingT<GameSettingCollection> fLocalMapMinZoom("fLocalMapMinZoom", 0.1f);
SettingT<GameSettingCollection> fLocalMapMaxZoom("fLocalMapMaxZoom", 0.9f);
SettingT<GameSettingCollection> fLocalMapMarkerMinSize("fLocalMapMarkerMinSize", 20.0f);
SettingT<GameSettingCollection> fLocalMapMarkerMaxSize("fLocalMapMarkerMaxSize", 75.0f);
SettingT<GameSettingCollection> fLocalQuestMarkerMinSize("fLocalQuestMarkerMinSize", 40.0f);
SettingT<GameSettingCollection> fLocalQuestMarkerMaxSize("fLocalQuestMarkerMaxSize", 75.0f);
SettingT<GameSettingCollection> gs_fLocalfPlayerCursorMinSize("fLocalfPlayerCursorMinSize", 40.0f);
SettingT<GameSettingCollection> fLocalPlayerCursorMaxSize("fLocalPlayerCursorMaxSize", 70.0f);
SettingT<INISettingCollection> fPopUpBackgroundOpacity("fPopUpBackgroundOpacity:Interface", 1.0f);
SettingT<INISettingCollection> fMenuBackgroundOpacity("fMenuBackgroundOpacity:Interface", 0.8f);
SettingT<GameSettingCollection> sMale("sMale", "Male");
SettingT<GameSettingCollection> sFemale("sFemale", "Female");
SettingT<GameSettingCollection> sPlayer("sPlayer", "player");
SettingT<GameSettingCollection> sConfirmChoice1("sConfirmChoice1", "Are you sure you want to be a");
SettingT<GameSettingCollection> sConfirmChoice2("sConfirmChoice2", "Are you sure you want to be an");
SettingT<GameSettingCollection> sNPCModelBeast("sNPCModelBeast", "Characters\\_Male\\SkeletonBeast.NIF");
SettingT<GameSettingCollection> sNPCModelMale("sNPCModelMale", "Characters\\_Male\\Skeleton.NIF");
SettingT<GameSettingCollection> sNPCModelFemale("sNPCModelFemale", "Characters\\_Female\\Skeleton.NIF");
SettingT<GameSettingCollection> sNoRace("sNoRace", "NO RACE");
SettingT<GameSettingCollection> sBald("sBald", "Bald");
SettingT<GameSettingCollection> sEntryInstructions1("sEntryInstructions1", "Enter a name for the");
SettingT<GameSettingCollection> sEntryInstructions2("sEntryInstructions2", "you are trying to create.");
SettingT<GameSettingCollection> sMain("sMain", "Main");
SettingT<GameSettingCollection> sRace("sRace", "Race");
SettingT<GameSettingCollection> sFace("sFace", "Face");
SettingT<GameSettingCollection> sHair("sHair", "Hair");
SettingT<GameSettingCollection> sEyes("sEyes", "Eyes");
SettingT<GameSettingCollection> sAge("sAge", "Age");
SettingT<GameSettingCollection> sComplexion("sComplexion", "Complexion");
SettingT<GameSettingCollection> sGender("sGender", "Gender");
SettingT<GameSettingCollection> sRaceDescription("sRaceDescription", "Race Description");
SettingT<GameSettingCollection> sStyle("sStyle", "Style");
SettingT<GameSettingCollection> sRed("sRed", "Red");
SettingT<GameSettingCollection> sGreen("sGreen", "Green");
SettingT<GameSettingCollection> sBlue("sBlue", "Blue");
SettingT<GameSettingCollection> sLength("sLength", "Length");
SettingT<GameSettingCollection> sShape("sShape", "Shape");
SettingT<GameSettingCollection> sTone("sTone", "Tone");
SettingT<GameSettingCollection> sRandom("sRandom", "Random");
SettingT<GameSettingCollection> sReset("sReset", "Reset");
SettingT<GameSettingCollection> sGeneralFace("sGeneralFace", "General(Face)");
SettingT<GameSettingCollection> sBrow("sBrow", "Brow");
SettingT<GameSettingCollection> sCheeks("sCheeks", "Cheeks");
SettingT<GameSettingCollection> sChin("sChin", "Chin");
SettingT<GameSettingCollection> sForehead("sForehead", "Forehead");
SettingT<GameSettingCollection> sJaw("sJaw", "Jaw");
SettingT<GameSettingCollection> sMouth("sMouth", "Mouth");
SettingT<GameSettingCollection> sNose("sNose", "Nose");
SettingT<GameSettingCollection> sGeneralSkin("sGeneralSkin", "General(Skin)");
SettingT<GameSettingCollection> sSkin("sSkin", "Skin");
SettingT<GameSettingCollection> sBeard("sBeard", "Beard");
SettingT<GameSettingCollection> sEyebrows("sEyebrows", "Eyebrows");
SettingT<GameSettingCollection> sMouthLips("sMouthLips", "Mouth(Lips)");
SettingT<GameSettingCollection> sLips("sLips", "Lips");
SettingT<GameSettingCollection> sFacebrow("sFacebrow", "Face brow/nose/chin ratio");
SettingT<GameSettingCollection> sFaceforehead("sFaceforehead", "Face forehead/sellion/nose ratio");
SettingT<GameSettingCollection> sFaceheavy("sFaceheavy", "Face heavy/light");
SettingT<GameSettingCollection> sFaceround("sFaceround", "Face round/gaunt");
SettingT<GameSettingCollection> sFacethin("sFacethin", "Face thin/wide");
SettingT<GameSettingCollection> sBrowRidge("sBrowRidge", "Brow Ridge high/low");
SettingT<GameSettingCollection> sBrowRidgeInner("sBrowRidgeInner", "Brow Ridge Inner up/down");
SettingT<GameSettingCollection> sBrowRidgeOuter("sBrowRidgeOuter", "Brow Ridge Outer up/down");
SettingT<GameSettingCollection> sCheekboneslow("sCheekboneslow", "Cheekbones low/high");
SettingT<GameSettingCollection> sCheekbonesshallow("sCheekbonesshallow", "Cheekbones shallow/pronounced");
SettingT<GameSettingCollection> sCheekbonesthin("sCheekbonesthin", "Cheekbones thin/wide");
SettingT<GameSettingCollection> sCheeksconcave("sCheeksconcave", "Cheeks concave/convex");
SettingT<GameSettingCollection> sCheeksround("sCheeksround", "Cheeks round/gaunt");
SettingT<GameSettingCollection> sChinforward("sChinforward", "Chin forward/backward");
SettingT<GameSettingCollection> sChinpronounced("sChinpronounced", "Chin pronounced/recessed");
SettingT<GameSettingCollection> sChinretracted("sChinretracted", "Chin retracted/jutting");
SettingT<GameSettingCollection> sChinshallow("sChinshallow", "Chin shallow/deep");
SettingT<GameSettingCollection> sChinsmall("sChinsmall", "Chin small/large");
SettingT<GameSettingCollection> sChintall("sChintall", "Chin tall/short");
SettingT<GameSettingCollection> sChinwide("sChinwide", "Chin wide/thin");
SettingT<GameSettingCollection> sEyesdown("sEyesdown", "Eyes down/up");
SettingT<GameSettingCollection> sEyessmall("sEyessmall", "Eyes small/large");
SettingT<GameSettingCollection> sEyestilt("sEyestilt", "Eyes tilt inward/outward");
SettingT<GameSettingCollection> sEyestogether("sEyestogether", "Eyes together/apart");
SettingT<GameSettingCollection> sForeheadsmall("sForeheadsmall", "Forehead small/large");
SettingT<GameSettingCollection> sForeheadtall("sForeheadtall", "Forehead tall/short");
SettingT<GameSettingCollection> sForeheadtilt("sForeheadtilt", "Forehead tilt forward/back");
SettingT<GameSettingCollection> sJawretracted("sJawretracted", "Jaw retracted/jutting");
SettingT<GameSettingCollection> sJawwide("sJawwide", "Jaw wide/thin");
SettingT<GameSettingCollection> sJawNeck("sJawNeck", "Jaw-Neck slope high/low");
SettingT<GameSettingCollection> sJawline("sJawline", "Jawline concave/convex");
SettingT<GameSettingCollection> sMouthdrawn("sMouthdrawn", "Mouth drawn/pursed");
SettingT<GameSettingCollection> sMouthhappy("sMouthhappy", "Mouth happy/sad");
SettingT<GameSettingCollection> sMouthhigh("sMouthhigh", "Mouth high/low");
SettingT<GameSettingCollection> sMouthlipsdeflated("sMouthlipsdeflated", "Mouth lips deflated/inflated");
SettingT<GameSettingCollection> sMouthlipslarge("sMouthlipslarge", "Mouth lips large/small");
SettingT<GameSettingCollection> sMouthlipspuckered("sMouthlipspuckered", "Mouth lips puckered/retracted");
SettingT<GameSettingCollection> sMouthprotruding("sMouthprotruding", "Mouth protruding/retracted");
SettingT<GameSettingCollection> sMouthtilt("sMouthtilt", "Mouth tilt up/down");
SettingT<GameSettingCollection> sMouthunderbite("sMouthunderbite", "Mouth underbite/overbite");
SettingT<GameSettingCollection> sMouthChin("sMouthChin", "Mouth-Chin distance short/long");
SettingT<GameSettingCollection> sNosebridgeshallow("sNosebridgeshallow", "Nose bridge shallow/deep");
SettingT<GameSettingCollection> sNosebridgeshort("sNosebridgeshort", "Nose bridge short/long");
SettingT<GameSettingCollection> sNosedown("sNosedown", "Nose down/up");
SettingT<GameSettingCollection> sNoseflat("sNoseflat", "Nose flat/pointed");
SettingT<GameSettingCollection> sNosenostriltilt("sNosenostriltilt", "Nose nostril tilt down/up");
SettingT<GameSettingCollection> sNosenostrilssmall("sNosenostrilssmall", "Nose nostrils small/large");
SettingT<GameSettingCollection> sNosenostrilswide("sNosenostrilswide", "Nose nostrils wide/thin");
SettingT<GameSettingCollection> sNoseregion("sNoseregion", "Nose region concave/convex");
SettingT<GameSettingCollection> sNoseselliondown("sNoseselliondown", "Nose sellion down/up");
SettingT<GameSettingCollection> sNosesellionshallow("sNosesellionshallow", "Nose sellion shallow/deep");
SettingT<GameSettingCollection> sNosesellionlower("sNosesellionlower", "Nose sellion lower shallow/deep");
SettingT<GameSettingCollection> sNosesellionthin("sNosesellionthin", "Nose sellion thin/wide");
SettingT<GameSettingCollection> sNoseshort("sNoseshort", "Nose short/long");
SettingT<GameSettingCollection> sNosetilt("sNosetilt", "Nose tilt down/up");
SettingT<GameSettingCollection> sSkinflushed("sSkinflushed", "Skin flushed/pale");
SettingT<GameSettingCollection> sSkinshade("sSkinshade", "Skin shade dark/light");
SettingT<GameSettingCollection> sSkintintorange("sSkintintorange", "Skin tint orange/blue");
SettingT<GameSettingCollection> sSkintintpurple("sSkintintpurple", "Skin tint purple/yellow");
SettingT<GameSettingCollection> sCheekblush("sCheekblush", "Cheek blush light/red");
SettingT<GameSettingCollection> sBeardflushed("sBeardflushed", "Beard flushed/pale");
SettingT<GameSettingCollection> sBeardlight("sBeardlight", "Beard light/dark");
SettingT<GameSettingCollection> sBeardcheeks("sBeardcheeks", "Beard cheeks light/dark");
SettingT<GameSettingCollection> sBeardcircle("sBeardcircle", "Beard circle light/dark");
SettingT<GameSettingCollection> sBeardgoatee("sBeardgoatee", "Beard goatee light/dark");
SettingT<GameSettingCollection> sBeardmoustache("sBeardmoustache", "Beard moustache light/dark");
SettingT<GameSettingCollection> sEyesocketsbruised("sEyesocketsbruised", "Eye sockets bruised/bright");
SettingT<GameSettingCollection> sEyesocketsdark("sEyesocketsdark", "Eye sockets dark/light");
SettingT<GameSettingCollection> sEyelidslight("sEyelidslight", "Eyelids light/dark");
SettingT<GameSettingCollection> sEyelidspale("sEyelidspale", "Eyelids pale/red");
SettingT<GameSettingCollection> sEyeliner("sEyeliner", "Eyeliner");
SettingT<GameSettingCollection> sEyeshadow("sEyeshadow", "Eyeshadow light/dark");
SettingT<GameSettingCollection> sEyesdark("sEyesdark", "Eyes dark brown/light blue");
SettingT<GameSettingCollection> sEyeswhites("sEyeswhites", "Eyes whites dim/bright");
SettingT<GameSettingCollection> sEyebrowsdark("sEyebrowsdark", "Eyebrows dark/light");
SettingT<GameSettingCollection> sEyebrowslow("sEyebrowslow", "Eyebrows low/high");
SettingT<GameSettingCollection> sEyebrowsthick("sEyebrowsthick", "Eyebrows thick/thin");
SettingT<GameSettingCollection> sEyebrowsvery("sEyebrowsvery", "Eyebrows very thin/thick");
SettingT<GameSettingCollection> sEyebrowslower("sEyebrowslower", "Eyebrows lower light/dark");
SettingT<GameSettingCollection> sEyebrowsouter("sEyebrowsouter", "Eyebrows outer light/dark");
SettingT<GameSettingCollection> sEyebrowsuppert("sEyebrowsuppert", "Eyebrows upper dark/light");
SettingT<GameSettingCollection> sLipsflushed("sLipsflushed", "Lips flushed/pale");
SettingT<GameSettingCollection> sLipstickdarkred("sLipstickdarkred", "Lipstick dark red/light blue");
SettingT<GameSettingCollection> sLipstickdarkblue("sLipstickdarkblue", "Lipstick dark blue/light red");
SettingT<GameSettingCollection> sNasolabiallines("sNasolabiallines", "Naso labial lines light/dark");
SettingT<GameSettingCollection> sNares("sNares", "Nares small/large");
SettingT<GameSettingCollection> sEyesTex("sEyesTex", "Eye shades");
SettingT<GameSettingCollection> sNoseTex("sNoseTex", "Nose shades");
SettingT<GameSettingCollection> sColor("sColor", "Hair Color");
SettingT<GameSettingCollection> sHairColor0("sHairColor0", "sHairColor0");
SettingT<GameSettingCollection> sHairColor1("sHairColor1", "sHairColor1");
SettingT<GameSettingCollection> sHairColor2("sHairColor2", "sHairColor2");
SettingT<GameSettingCollection> sHairColor3("sHairColor3", "sHairColor3");
SettingT<GameSettingCollection> sHairColor4("sHairColor4", "sHairColor4");
SettingT<GameSettingCollection> sHairColor5("sHairColor5", "sHairColor5");
SettingT<GameSettingCollection> sHairColor6("sHairColor6", "sHairColor6");
SettingT<GameSettingCollection> sHairColor7("sHairColor7", "sHairColor7");
SettingT<GameSettingCollection> sHairColor8("sHairColor8", "sHairColor8");
SettingT<GameSettingCollection> sHairColor9("sHairColor9", "sHairColor9");
SettingT<GameSettingCollection> sHairColor10("sHairColor10", "sHairColor10");
SettingT<GameSettingCollection> sHairColor11("sHairColor11", "sHairColor11");
SettingT<GameSettingCollection> sHairColor12("sHairColor12", "sHairColor12");
SettingT<GameSettingCollection> sHairColor13("sHairColor13", "sHairColor13");
SettingT<GameSettingCollection> sHairColor14("sHairColor14", "sHairColor14");
SettingT<GameSettingCollection> sHairColor15("sHairColor15", "sHairColor15");
SettingT<GameSettingCollection> iHairColor00("iHairColor00", 8289918);
SettingT<GameSettingCollection> iHairColor01("iHairColor01", 3618617);
SettingT<GameSettingCollection> iHairColor02("iHairColor02", 2039585);
SettingT<GameSettingCollection> iHairColor03("iHairColor03", 4289146);
SettingT<GameSettingCollection> iHairColor04("iHairColor04", 4680837);
SettingT<GameSettingCollection> iHairColor05("iHairColor05", 3557724);
SettingT<GameSettingCollection> iHairColor06("iHairColor06", 3360589);
SettingT<GameSettingCollection> iHairColor07("iHairColor07", 2830157);
SettingT<GameSettingCollection> iHairColor08("iHairColor08", 2965094);
SettingT<GameSettingCollection> iHairColor09("iHairColor09", 4287105);
SettingT<GameSettingCollection> iHairColor10("iHairColor10", 1000298);
SettingT<GameSettingCollection> iHairColor11("iHairColor11", 1974080);
SettingT<GameSettingCollection> iHairColor12("iHairColor12", 2958114);
SettingT<GameSettingCollection> iHairColor13("iHairColor13", 2239537);
SettingT<GameSettingCollection> iHairColor14("iHairColor14", 7237230);
SettingT<GameSettingCollection> iHairColor15("iHairColor15", 5400188);
SettingT<GameSettingCollection> sRSMCustom("sRSMCustom", "Custom");
SettingT<GameSettingCollection> sRSMSex("sRSMSex", "Sex");
SettingT<GameSettingCollection> sRSMRace("sRSMRace", "Race");
SettingT<GameSettingCollection> sRSMFace("sRSMFace", "Face");
SettingT<GameSettingCollection> sRSMHair("sRSMHair", "Hair");
SettingT<GameSettingCollection> sRSMCustomize("sRSMCustomize", "Customize");
SettingT<GameSettingCollection> sRSMHairStyle("sRSMHairStyle", "Hair Style");
SettingT<GameSettingCollection> sRSMHairColor("sRSMHairColor", "Hair Color");
SettingT<GameSettingCollection> sRSMFacialHair("sRSMFacialHair", "Facial Hair");
SettingT<GameSettingCollection> sRSMEyeColor("sRSMEyeColor", "Eye Color");
SettingT<GameSettingCollection> sRSMPreset("sRSMPreset", "Preset:");
SettingT<GameSettingCollection> sRSMRandomize("sRSMRandomize", "Randomize");
SettingT<GameSettingCollection> sRSMAge("sRSMAge", "Age");
SettingT<GameSettingCollection> sRSMRedAbbrev("sRSMRedAbbrev", "R:");
SettingT<GameSettingCollection> sRSMGreenAbbrev("sRSMGreenAbbrev", "G:");
SettingT<GameSettingCollection> sRSMBlueAbbrev("sRSMBlueAbbrev", "B:");
SettingT<GameSettingCollection> sRSMGeneral("sRSMGeneral", "General");
SettingT<GameSettingCollection> sRSMEyeSockets("sRSMEyeSockets", "Eye Sockets");
SettingT<GameSettingCollection> sRSMEyelids("sRSMEyelids", "Eyelids");
SettingT<GameSettingCollection> sRSMMisc("sRSMMisc", "Misc");
SettingT<GameSettingCollection> sRSMConfirmDone("sRSMConfirmDone", "Are you sure you want this to be your");
SettingT<GameSettingCollection> sRSMCharacter("sRSMCharacter", "character");
SettingT<GameSettingCollection> sRSMConfirmRandomize("sRSMConfirmRandomize", "Randomize your head?");
SettingT<GameSettingCollection> sRSMShapeOption01("sRSMShapeOption01", "High/Low");
SettingT<GameSettingCollection> sRSMShapeOption02("sRSMShapeOption02", "Up/Down (inner)");
SettingT<GameSettingCollection> sRSMShapeOption03("sRSMShapeOption03", "Up/Down (outer)");
SettingT<GameSettingCollection> sRSMShapeOption04("sRSMShapeOption04", "Low/High");
SettingT<GameSettingCollection> sRSMShapeOption05("sRSMShapeOption05", "Shallow/Pronounced");
SettingT<GameSettingCollection> sRSMShapeOption06("sRSMShapeOption06", "Thin/Wide");
SettingT<GameSettingCollection> sRSMShapeOption07("sRSMShapeOption07", "Concave/Convex");
SettingT<GameSettingCollection> sRSMShapeOption08("sRSMShapeOption08", "Round/Gaunt");
SettingT<GameSettingCollection> sRSMShapeOption09("sRSMShapeOption09", "Forward/Backward");
SettingT<GameSettingCollection> sRSMShapeOption10("sRSMShapeOption10", "Broad/Recessed");
SettingT<GameSettingCollection> sRSMShapeOption11("sRSMShapeOption11", "Retracted/Jutting");
SettingT<GameSettingCollection> sRSMShapeOption12("sRSMShapeOption12", "Shallow/Deep");
SettingT<GameSettingCollection> sRSMShapeOption13("sRSMShapeOption13", "Small/Large");
SettingT<GameSettingCollection> sRSMShapeOption14("sRSMShapeOption14", "Tall/Short");
SettingT<GameSettingCollection> sRSMShapeOption15("sRSMShapeOption15", "Wide/Thin");
SettingT<GameSettingCollection> sRSMShapeOption16("sRSMShapeOption16", "Down/Up");
SettingT<GameSettingCollection> sRSMShapeOption18("sRSMShapeOption18", "Tilt in/out");
SettingT<GameSettingCollection> sRSMShapeOption19("sRSMShapeOption19", "Together/Apart");
SettingT<GameSettingCollection> sRSMShapeOption20("sRSMShapeOption20", "Brow-Nose-Chin");
SettingT<GameSettingCollection> sRSMShapeOption21("sRSMShapeOption21", "Forehead-Nose");
SettingT<GameSettingCollection> sRSMShapeOption22("sRSMShapeOption22", "Heavy/Light");
SettingT<GameSettingCollection> sRSMShapeOption27("sRSMShapeOption27", "Tilt Forward/Back");
SettingT<GameSettingCollection> sRSMShapeOption30("sRSMShapeOption30", "Slope High/Low");
SettingT<GameSettingCollection> sRSMShapeOption32("sRSMShapeOption32", "Drawn/Pursed");
SettingT<GameSettingCollection> sRSMShapeOption33("sRSMShapeOption33", "Happy/Sad");
SettingT<GameSettingCollection> sRSMShapeOption35("sRSMShapeOption35", "Deflated/Inflated");
SettingT<GameSettingCollection> sRSMShapeOption36("sRSMShapeOption36", "Large/Small");
SettingT<GameSettingCollection> sRSMShapeOption37("sRSMShapeOption37", "Puckered/Retracted");
SettingT<GameSettingCollection> sRSMShapeOption38("sRSMShapeOption38", "Convex/Concave");
SettingT<GameSettingCollection> sRSMShapeOption39("sRSMShapeOption39", "Tilt Up/Down");
SettingT<GameSettingCollection> sRSMShapeOption40("sRSMShapeOption40", "Underbite/Overbite");
SettingT<GameSettingCollection> sRSMShapeOption41("sRSMShapeOption41", "Mouth-Chin Dist.");
SettingT<GameSettingCollection> sRSMShapeOption42("sRSMShapeOption42", "Bridge Depth");
SettingT<GameSettingCollection> sRSMShapeOption43("sRSMShapeOption43", "Bridge Length");
SettingT<GameSettingCollection> sRSMShapeOption45("sRSMShapeOption45", "Flat/Pointed");
SettingT<GameSettingCollection> sRSMShapeOption46("sRSMShapeOption46", "Nostril Tilt");
SettingT<GameSettingCollection> sRSMShapeOption47("sRSMShapeOption47", "Nostril Size");
SettingT<GameSettingCollection> sRSMShapeOption48("sRSMShapeOption48", "Nostrils Wide/Thin");
SettingT<GameSettingCollection> sRSMShapeOption50("sRSMShapeOption50", "Sellion Height");
SettingT<GameSettingCollection> sRSMShapeOption51("sRSMShapeOption51", "Sellion Depth");
SettingT<GameSettingCollection> sRSMShapeOption53("sRSMShapeOption53", "Sellion Width");
SettingT<GameSettingCollection> sRSMShapeOption54("sRSMShapeOption54", "Short/Long");
SettingT<GameSettingCollection> sRSMShapeOption55("sRSMShapeOption55", "Tilt Down/Up");
SettingT<GameSettingCollection> sRSMToneOption01("sRSMToneOption01", "Flushed");
SettingT<GameSettingCollection> sRSMToneOption02("sRSMToneOption02", "Light/Dark");
SettingT<GameSettingCollection> sRSMToneOption03("sRSMToneOption03", "Cheeks Light/Dark");
SettingT<GameSettingCollection> sRSMToneOption04("sRSMToneOption04", "Circle Light/Dark");
SettingT<GameSettingCollection> sRSMToneOption05("sRSMToneOption05", "Goatee Light/Dark");
SettingT<GameSettingCollection> sRSMToneOption06("sRSMToneOption06", "Moustache");
SettingT<GameSettingCollection> sRSMToneOption07("sRSMToneOption07", "Blush Light/Red");
SettingT<GameSettingCollection> sRSMToneOption08("sRSMToneOption08", "Bruised/Bright");
SettingT<GameSettingCollection> sRSMToneOption09("sRSMToneOption09", "Dark/Light");
SettingT<GameSettingCollection> sRSMToneOption10("sRSMToneOption10", "Thick/Thin");
SettingT<GameSettingCollection> sRSMToneOption11("sRSMToneOption11", "Very Thin/Thick");
SettingT<GameSettingCollection> sRSMToneOption12("sRSMToneOption12", "Light/Dark (Lower)");
SettingT<GameSettingCollection> sRSMToneOption13("sRSMToneOption13", "Light/Dark (Outer)");
SettingT<GameSettingCollection> sRSMToneOption14("sRSMToneOption14", "Dark/Light (Upper)");
SettingT<GameSettingCollection> sRSMToneOption15("sRSMToneOption15", "Pale/Red");
SettingT<GameSettingCollection> sRSMToneOption16("sRSMToneOption16", "Eyeliner Lt./Dark");
SettingT<GameSettingCollection> sRSMToneOption17("sRSMToneOption17", "Shadow Light/Dark");
SettingT<GameSettingCollection> sRSMToneOption18("sRSMToneOption18", "Dk Brown/Lt Blue");
SettingT<GameSettingCollection> sRSMToneOption19("sRSMToneOption19", "Whites Dim/Bright");
SettingT<GameSettingCollection> sRSMToneOption20("sRSMToneOption20", "Dark Red/Lt. Blue");
SettingT<GameSettingCollection> sRSMToneOption21("sRSMToneOption21", "Dark Blue/Lt. Red");
SettingT<GameSettingCollection> sRSMToneOption22("sRSMToneOption22", "Naso Labial Lines");
SettingT<GameSettingCollection> sRSMToneOption23("sRSMToneOption23", "Nares Small/Large");
SettingT<GameSettingCollection> sRSMToneOption24("sRSMToneOption24", "Shade");
SettingT<GameSettingCollection> sRSMToneOption25("sRSMToneOption25", "Blue Tint");
SettingT<GameSettingCollection> sRSMToneOption26("sRSMToneOption26", "Yellow Tint");
SettingT<GameSettingCollection> sChangeItemSelection("sChangeItemSelection", "Change Item Selection");
SettingT<GameSettingCollection> sSelectItemToRepair("sSelectItemToRepair", "CHOOSE ITEM TO REPAIR WITH");
SettingT<GameSettingCollection> sRepair("sRepair", "REPAIR");
SettingT<GameSettingCollection> sRepairServicesTitle("sRepairServicesTitle", "Repair Services");
SettingT<GameSettingCollection> sRepairSkill("sRepairSkill", "Repair Skill");
SettingT<GameSettingCollection> sRepairCost("sRepairCost", "Cost: %d caps");
SettingT<GameSettingCollection> sRepairItem("sRepairItem", "Repair");
SettingT<GameSettingCollection> sMaintainItem("sMaintainItem", "Maintain");
SettingT<GameSettingCollection> sRepairAllItems("sRepairAllItems", "Repair All (%d)");
SettingT<GameSettingCollection> sNoItemsToRepair("sNoItemsToRepair", "No items to repair");
SettingT<GameSettingCollection> sNoSpareParts("sNoSpareParts", "NO USABLE SPARE PARTS");
SettingT<GameSettingCollection> sRepairSkillTooLow("sRepairSkillTooLow", "%d%s REPAIR SKILL NEEDED");
SettingT<GameSettingCollection> sCantRepairPastMax("sCantRepairPastMax", "CANNOT REPAIR PAST %.0f%s");
SettingT<GameSettingCollection> sNoRepairInCombat("sNoRepairInCombat", "You cannot repair items while in combat.");
SettingT<GameSettingCollection> sNoRepairHostileActorsNear("sNoRepairHostileActorsNear",
	"You cannot repair items when enemies are nearby.");
SettingT<GameSettingCollection> sWait("sWait", "wait");
SettingT<GameSettingCollection> sSleep("sSleep", "sleep");
SettingT<GameSettingCollection> sHowMany("sHowMany", "How many?");
SettingT<GameSettingCollection> sHowManyWait("sHowManyWait", "How long would you like to");
SettingT<GameSettingCollection> sHour("sHour", "hour");
SettingT<GameSettingCollection> sHours("sHours", "hours");
SettingT<GameSettingCollection> sNoWaitInJail("sNoWaitInJail",
	"You must sleep in a jail bed in order to serve your sentence.");
SettingT<GameSettingCollection> sNoWaitWhileAlarmSounding("sNoWaitWhileAlarmSounding",
	"You are unable to wait here. An alarm is sounding with your name on it.");
SettingT<GameSettingCollection> sNoWaitInCombat("sNoWaitInCombat", "You cannot wait while in combat.");
SettingT<GameSettingCollection> sNoWaitTrespass("sNoWaitTrespass", "You cannot wait while trespassing.");
SettingT<GameSettingCollection> sNoWaitUnderWater("sNoWaitUnderWater", "You cannot wait while under water.");
SettingT<GameSettingCollection> sNoWaitHostilActorsNear("sNoWaitHostilActorsNear",
	"You cannot wait when enemies are nearby.");
SettingT<GameSettingCollection> sNoWaitInAir("sNoWaitInAir", "You cannot wait in the air.");
SettingT<GameSettingCollection> sNoWaitInCell("sNoWaitInCell", "You cannot wait in this location.");
SettingT<GameSettingCollection> sNoWaitInRadiation("sNoWaitInRadiation", "You cannot wait while being irradiated.");
SettingT<GameSettingCollection> sNoWaitTakingHealthDamage("sNoWaitTakingHealthDamage",
	"You cannot wait while taking health damage.");
SettingT<GameSettingCollection> sNoSleepTrespass("sNoSleepTrespass", "You cannot sleep while trespassing.");
SettingT<GameSettingCollection> sNoSleepCombat("sNoSleepCombat", "You cannot sleep while in combat.");
SettingT<GameSettingCollection> sNoSleepHostileActorsNear("sNoSleepHostileActorsNear",
	"You cannot sleep when enemies are near by.");
SettingT<GameSettingCollection> sNoSleepInRadiation("sNoSleepInRadiation", "You cannot sleep while being irradiated.");
SettingT<GameSettingCollection> sNoSleepTakingHealthDamage("sNoSleepTakingHealthDamage",
	"You cannot sleep while taking health damage.");
SettingT<GameSettingCollection> sAMTime("sAMTime", "AM");
SettingT<GameSettingCollection> sPMTime("sPMTime", "PM");
SettingT<GameSettingCollection> sPressStart("sPressStart", "Press START");
SettingT<GameSettingCollection> sSave("sSave", "Save");
SettingT<GameSettingCollection> sLoad("sLoad", "Load");
SettingT<GameSettingCollection> sNew("sNew", "New");
SettingT<GameSettingCollection> sSettings("sSettings", "Settings");
SettingT<GameSettingCollection> sCrew("sCrew", "Credits");
SettingT<GameSettingCollection> sQuit("sQuit", "Quit");
SettingT<GameSettingCollection> sGameplay("sGameplay", "Gameplay");
SettingT<GameSettingCollection> sDisplay("sDisplay", "Display");
SettingT<GameSettingCollection> sAudio("sAudio", "Audio");
SettingT<GameSettingCollection> sControls("sControls", "Controls");
SettingT<GameSettingCollection> sDefaults("sDefaults", "Defaults");
SettingT<GameSettingCollection> sVideo("sVideo", "Video");
SettingT<GameSettingCollection> sDownloads("sDownloads", "Downloadable Content");
SettingT<GameSettingCollection> sRedeemCode("sRedeemCode", "Enter Code");
SettingT<GameSettingCollection> sVideoChange("sVideoChange",
	"You must exit and restart Fallout: New Vegas for the new resolution to take effect.");
SettingT<GameSettingCollection> sMustRestart("sMustRestart",
	"You must exit and restart Fallout: New Vegas for changes in this setting to take effect.");
SettingT<GameSettingCollection> sCantChangeResolution("sCantChangeResolution",
	"You must close your current game to change resolutions.");
SettingT<GameSettingCollection> sDownloadsAvailable("sDownloadsAvailable", "New Content Available");
SettingT<GameSettingCollection> sOldDownloadsAvailable("sOldDownloadsAvailable", "View Available Content");
SettingT<GameSettingCollection> sDownloadsNotAvail("sDownloadsNotAvail", "No New Content Available");
SettingT<GameSettingCollection> sPauseText("sPauseText", "Pause");
SettingT<GameSettingCollection> sMakeDefaults("sMakeDefaults", "Make these your default settings?");
SettingT<GameSettingCollection> sResetToDefaults("sResetToDefaults",
	"Reset your control mappings to their default settings?");
SettingT<GameSettingCollection> sVersionNumber("sVersionNumber", "Version Info");
SettingT<GameSettingCollection> sKillCamModeNone("sKillCamModeNone", "None");
SettingT<GameSettingCollection> sKillCamModePlayer("sKillCamModePlayer", "Player View");
SettingT<GameSettingCollection> sKillCamModeCinematic("sKillCamModeCinematic", "Cinematic");
SettingT<GameSettingCollection> sKillCamMode("sKillCamMode", "Kill Cam Mode");
SettingT<GameSettingCollection> sHardcore("sHardcore", "Hardcore Mode");
SettingT<GameSettingCollection> sDifficulty("sDifficulty", "Difficulty");
SettingT<GameSettingCollection> sCrosshair("sCrosshair", "Crosshair");
SettingT<GameSettingCollection> sSaveOnRest("sSaveOnRest", "Save On Rest");
SettingT<GameSettingCollection> sSaveOnWait("sSaveOnWait", "Save On Wait");
SettingT<GameSettingCollection> sSaveOnTravel("sSaveOnTravel", "Save On Travel");
SettingT<GameSettingCollection> sDialogSubtitles("sDialogSubtitles", "Dialog Subtitles");
SettingT<GameSettingCollection> sGeneralSubtitles("sGeneralSubtitles", "General Subtitles");
SettingT<GameSettingCollection> sResolution("sResolution", "Resolution");
SettingT<GameSettingCollection> sHUDColor("sHUDColor", "HUD Color");
SettingT<GameSettingCollection> sHUDOpacity("sHUDOpacity", "HUD Opacity");
SettingT<GameSettingCollection> sPipboyColor("sPipboyColor", "Pip-Boy Color");
SettingT<GameSettingCollection> sTextureSize("sTextureSize", "Texture Size");
SettingT<GameSettingCollection> sViewDistance("sViewDistance", "View Distance");
SettingT<GameSettingCollection> sActorFade("sActorFade", "Actor Fade");
SettingT<GameSettingCollection> sItemFade("sItemFade", "Item Fade");
SettingT<GameSettingCollection> sObjectFade("sObjectFade", "Object Fade");
SettingT<GameSettingCollection> sLoadingLOD("sLoadingLOD", "Loading distant LOD...");
SettingT<GameSettingCollection> sStartNewGame("sStartNewGame", "Start new game?");
SettingT<GameSettingCollection> sBrightness("sBrightness", "Brightness");
SettingT<GameSettingCollection> sXSensitivity("sXSensitivity", "X-Sensitivity");
SettingT<GameSettingCollection> sYSensitivity("sYSensitivity", "Y-Sensitivity");
SettingT<GameSettingCollection> sCameraPitch("sCameraPitch", "Invert Y");
SettingT<GameSettingCollection> sRumble("sRumble", "Vibration");
SettingT<GameSettingCollection> sMasterVolume("sMasterVolume", "Master");
SettingT<GameSettingCollection> sMusicVolume("sMusicVolume", "Music");
SettingT<GameSettingCollection> sFootstepsVolume("sFootstepsVolume", "Footstep");
SettingT<GameSettingCollection> sVoiceVolume("sVoiceVolume", "Voice");
SettingT<GameSettingCollection> sEffectsVolume("sEffectsVolume", "Effects");
SettingT<GameSettingCollection> sRadioVolume("sRadioVolume", "Radio");
SettingT<GameSettingCollection> sMainMenu("sMainMenu", "Main Menu");
SettingT<GameSettingCollection> sConfirmQuit("sConfirmQuit", "Are you sure you want to quit?");
SettingT<GameSettingCollection> sConfirmNew("sConfirmNew",
	"Start a new game? This will\noverwrite your Autosave if present.");
SettingT<GameSettingCollection> sConfirmContinue("sConfirmContinue", "Continue from your last saved game?");
SettingT<GameSettingCollection> sConfirmLoad("sConfirmLoad", "Load this game?");
SettingT<GameSettingCollection> sConfirmSave("sConfirmSave", "Overwrite this game?");
SettingT<GameSettingCollection> sConfirmDelete("sConfirmDelete", "Delete this game?");
SettingT<GameSettingCollection> sConfirmWarning("sConfirmWarning", "WARNING:  Any unsaved progress will be lost.");
SettingT<GameSettingCollection> sNewSave("sNewSave", "[NEW SAVE]");
SettingT<GameSettingCollection> sSavesRemaining("sSavesRemaining", "saves used.");
SettingT<GameSettingCollection> sNoSaves("sNoSaves", "[NO SAVES FOUND]");
SettingT<GameSettingCollection> sHelp("sHelp", "Help");
SettingT<GameSettingCollection> sDefaultPlayerName("sDefaultPlayerName", "Player Name");
SettingT<GameSettingCollection> sButton("sButton", "Button");
SettingT<GameSettingCollection> sDevice("sDevice", "Device");
SettingT<GameSettingCollection> sPressControl("sPressControl", "Press the new control for this action");
SettingT<GameSettingCollection> sKeyLocked("sKeyLocked", "That key cannot be remapped");
SettingT<GameSettingCollection> sButtonLocked("sButtonLocked", "That button cannot be remapped");

// The default is the "Normal" text aNormal_0, whose address is compared.
SettingT<GameSettingCollection> sNormal("sNormal", aNormal_0);
SettingT<GameSettingCollection> sAutosaveAbbrev("sAutosaveAbbrev", "AUTO");
SettingT<GameSettingCollection> sQuicksaveAbbrev("sQuicksaveAbbrev", "QUICK");
SettingT<GameSettingCollection> gs_sSystemSaveAbbrev("sSystemSaveAbbrev", "SYS");
SettingT<GameSettingCollection> sPlayTime("sPlayTime", "Play Time");
SettingT<GameSettingCollection> sAmber("sAmber", "AMBER");
SettingT<GameSettingCollection> sWhite("sWhite", "WHITE");
SettingT<GameSettingCollection> iAttractModeIdleTime("iAttractModeIdleTime", 90);
SettingT<GameSettingCollection> sGFWLive("sGFWLive", "LIVE");
SettingT<GameSettingCollection> sGrassFade("sGrassFade", "Grass Fade");
SettingT<GameSettingCollection> sShadowFade("sShadowFade", "Shadow Fade");
SettingT<GameSettingCollection> sLightFade("sLightFade", "Light Fade");
SettingT<GameSettingCollection> sSpecularityFade("sSpecularityFade", "Specularity Fade");
SettingT<GameSettingCollection> sObjectLODFade("sObjectLODFade", "Object LOD Fade");
SettingT<GameSettingCollection> sTreeLODFade("sTreeLODFade", "Tree LOD Fade");
SettingT<GameSettingCollection> sPleaseStandBy("sPleaseStandBy", "PLEASE STAND BY");
SettingT<GameSettingCollection> sTrueIronSights("sTrueIronSights", "True Iron Sights");
SettingT<GameSettingCollection> sStats("sStats", "STATS");
SettingT<GameSettingCollection> sStatsHP("sStatsHP", "HP");
SettingT<GameSettingCollection> sStatsAP("sStatsAP", "AP");
SettingT<GameSettingCollection> sStatsXP("sStatsXP", "XP");
SettingT<GameSettingCollection> sStatsStatus("sStatsStatus", "Status");
SettingT<GameSettingCollection> sStatsSpecial("sStatsSpecial", "S.P.E.C.I.A.L.");
SettingT<GameSettingCollection> sStatsSkills("sStatsSkills", "Skills");
SettingT<GameSettingCollection> sStatsPerks("sStatsPerks", "Perks");
SettingT<GameSettingCollection> sStatsGeneral("sStatsGeneral", "General");
SettingT<GameSettingCollection> sStatsCondition("sStatsCondition", "Condition");
SettingT<GameSettingCollection> sStatsRadiation("sStatsRadiation", "Radiation");
SettingT<GameSettingCollection> sStatsEffects("sStatsEffects", "Effects");
SettingT<GameSettingCollection> sStatsCrippled("sStatsCrippled", "CRIPPLED");
SettingT<GameSettingCollection> sStatsCNDAbbrev("sStatsCNDAbbrev", "CND");
SettingT<GameSettingCollection> sStatsRADAbbrev("sStatsRADAbbrev", "RAD");
SettingT<GameSettingCollection> sStatsH20Abbrev("sStatsH20Abbrev", "H20");
SettingT<GameSettingCollection> sStatsSLPAbbrev("sStatsSLPAbbrev", "SLP");
SettingT<GameSettingCollection> sStatsFODAbbrev("sStatsFODAbbrev", "FOD");
SettingT<GameSettingCollection> sStatsLMBAbbrev("sStatsLMBAbbrev", "LMB");
SettingT<GameSettingCollection> sStatsEFFAbbrev("sStatsEFFAbbrev", "EFF");
SettingT<GameSettingCollection> sStatsLVLAbbrev("sStatsLVLAbbrev", "LVL");
SettingT<GameSettingCollection> sStatsAddicted("sStatsAddicted", "ADDICT");
SettingT<GameSettingCollection> sStatsNoEffects("sStatsNoEffects", "NO STATUS EFFECTS");
SettingT<GameSettingCollection> sAlignGood("sAlignGood", "Good");
SettingT<GameSettingCollection> sAlignNeutral("sAlignNeutral", "Neutral");
SettingT<GameSettingCollection> sAlignEvil("sAlignEvil", "Evil");
SettingT<GameSettingCollection> sAlignVeryGood("sAlignVeryGood", "Very Good");
SettingT<GameSettingCollection> sAlignVeryEvil("sAlignVeryEvil", "Very Evil");
SettingT<GameSettingCollection> sStatsLimbs("sStatsLimbs", "Limbs");
SettingT<GameSettingCollection> sStatsBody("sStatsBody", "Body");
SettingT<GameSettingCollection> sStatsAlcohol("sStatsAlcohol", "Alcohol");
SettingT<GameSettingCollection> sStatsRadResist("sStatsRadResist", "RAD RESIST");
SettingT<GameSettingCollection> sStatsXPMax("sStatsXPMax", "MAX");
SettingT<GameSettingCollection> sChallengeToggle("sChallengeToggle", "Challenges");
SettingT<GameSettingCollection> sNotesToggle("sNotesToggle", "Notes");
SettingT<GameSettingCollection> sNextChallenge("sNextChallenge", "Next Challenge Type");
SettingT<GameSettingCollection> sPrevChallenge("sPrevChallenge", "Prev Challenge Type");
SettingT<GameSettingCollection> sChallengeKill("sChallengeKill", "Kill Challenges");
SettingT<GameSettingCollection> sChallengeDamage("sChallengeDamage", "Damage Challenges");
SettingT<GameSettingCollection> sChallengeOther("sChallengeOther", "Other Challenges");
SettingT<GameSettingCollection> iBurnMinimumMillis("iBurnMinimumMillis", 2000);
SettingT<GameSettingCollection> sMedicineSkill("sMedicineSkill", "Med skill");
SettingT<GameSettingCollection> sMorphine("sMorphine", "Morphine");
SettingT<GameSettingCollection> sStimpak("sStimpak", "Stimpak");
SettingT<GameSettingCollection> sSurgery("sSurgery", "Surgery");
SettingT<GameSettingCollection> sCauterize("sCauterize", "Cauterizing tool");
SettingT<GameSettingCollection> sSwitchLimbs("sSwitchLimbs", "Switch Limbs");
SettingT<GameSettingCollection> sAutoAttempt("sAutoAttempt", "Auto Attempt");
SettingT<GameSettingCollection> sVatsAiming("sVatsAiming", "Aiming");
SettingT<GameSettingCollection> sVatsAimed("sVatsAimed", "Aimed");
SettingT<GameSettingCollection> sVatsBodyPart("sVatsBodyPart", "Body Part");
SettingT<GameSettingCollection> sVatsTarget("sVatsTarget", "Target");
SettingT<GameSettingCollection> sVatsSelect("sVatsSelect", "Select");
SettingT<GameSettingCollection> iVATSConcentratedFireBonus("iVATSConcentratedFireBonus", 5);
SettingT<GameSettingCollection> fVATSTargetSelectCamPanTime("fVATSTargetSelectCamPanTime", 0.75f);
SettingT<GameSettingCollection> fVATSCamZoomInTime("fVATSCamZoomInTime", 0.75f);
SettingT<GameSettingCollection> fVATSLimbSelectCamPanTime("fVATSLimbSelectCamPanTime", 0.3f);
SettingT<GameSettingCollection> fVATSMaxEngageDistance("fVATSMaxEngageDistance", 5000.0f);
SettingT<GameSettingCollection> sVATSMessageLowAP("sVATSMessageLowAP", "Low");
SettingT<GameSettingCollection> sVATSMessageZeroChance("sVATSMessageZeroChance", "You have zero chance to hit.");
SettingT<GameSettingCollection> sVATSMessageNoAmmo("sVATSMessageNoAmmo", "You don't have enough ammo.");
SettingT<GameSettingCollection> sVatsUnarmedAttack1("sVatsUnarmedAttack1", "Uppercut");
SettingT<GameSettingCollection> sVatsUnarmedAttack2("sVatsUnarmedAttack2", "Cross");
SettingT<GameSettingCollection> sVatsUnarmedAttackGround("sVatsUnarmedAttackGround", "Stomp");
SettingT<GameSettingCollection> sCWheelTitle("sCWheelTitle", "Companion Commands");
SettingT<GameSettingCollection> sCWheelBeAggressive("sCWheelBeAggressive", "Be Aggressive");
SettingT<GameSettingCollection> sCWheelBePassive("sCWheelBePassive", "Be Passive");
SettingT<GameSettingCollection> gs_sCWheelUseStimpak("sCWheelUseStimpak", "Use Stimpak");
SettingT<GameSettingCollection> sCWheelWaitHere("sCWheelWaitHere", "Wait Here");
SettingT<GameSettingCollection> sCWheelFollowMe("sCWheelFollowMe", "Follow Me");
SettingT<GameSettingCollection> sCWheelTalkTo("sCWheelTalkTo", "Talk To");
SettingT<GameSettingCollection> sCWheelBackUp("sCWheelBackUp", "Back Up");
SettingT<GameSettingCollection> sCWheelStayClose("sCWheelStayClose", "Stay Close");
SettingT<GameSettingCollection> sCWheelKeepDistance("sCWheelKeepDistance", "Keep Distance");
SettingT<GameSettingCollection> sCWheelOpenInventory("sCWheelOpenInventory", "Open Inventory");
SettingT<GameSettingCollection> sCWheelUseRanged("sCWheelUseRanged", "Use Ranged");
SettingT<GameSettingCollection> sCWheelUseMelee("sCWheelUseMelee", "Use Melee");
SettingT<GameSettingCollection> sNavigate("sNavigate", "Navigate");
SettingT<GameSettingCollection> sCantEquipBrokenItem("sCantEquipBrokenItem",
	"Broken items cannot be equipped until they have been repaired.");
SettingT<GameSettingCollection> sWeaponBreak("sWeaponBreak", "Your weapon has broken.");
SettingT<GameSettingCollection> sWeaponLowCond("sWeaponLowCond", "Your weapon condition is dangerously low.");
SettingT<GameSettingCollection> sArmorLowCond("sArmorLowCond", "Your armor condition is dangerously low.");
SettingT<GameSettingCollection> sCantRemoveWornItem("sCantRemoveWornItem", "Unable to remove the worn item.");
SettingT<GameSettingCollection> sDropQuestItemWarning("sDropQuestItemWarning",
	"You cannot remove Quest Items from your Inventory.");
SettingT<GameSettingCollection> sNotEnoughRoomWarning("sNotEnoughRoomWarning",
	"You do not have enough room to drop this object.");
SettingT<GameSettingCollection> sNoJumpWarning("sNoJumpWarning", "You can not drop objects while in the air.");
SettingT<GameSettingCollection> sAnimationCanNotEquipWeapon("sAnimationCanNotEquipWeapon",
	"You cannot change weapons while attacking.");
SettingT<GameSettingCollection> sAnimationCanNotEquipArmor("sAnimationCanNotEquipArmor",
	"You cannot change armor while attacking.");
SettingT<GameSettingCollection> sCantEquipGeneric("sCantEquipGeneric", "You cannot equip this item.");
SettingT<GameSettingCollection> sCantEquipPowerArmor("sCantEquipPowerArmor",
	"Power armor training is required to equip this item.");
SettingT<GameSettingCollection> sAnimationCanNotUnequip("sAnimationCanNotUnequip",
	"You cannot change weapons while attacking.");
SettingT<GameSettingCollection> sCantUnequipGeneric("sCantUnequipGeneric", "You cannot unequip this item.");
SettingT<GameSettingCollection> sCanNotEquipWornEnchantment("sCanNotEquipWornEnchantment",
	"You cannot equip this item right now.");
SettingT<GameSettingCollection> sOverEncumbered("sOverEncumbered", "You are carrying too much to be able to run.");
SettingT<GameSettingCollection> sHeavyArmorNoJump("sHeavyArmorNoJump", "Your heavy armor prevents you from jumping.");
SettingT<GameSettingCollection> sHeavyArmorSink("sHeavyArmorSink", "Your heavy armor is causing you to sink.");
SettingT<GameSettingCollection> sLostController("sLostController",
	"The Xbox 360 Controller has been disconnected. Click OK to use the Keyboard/Mouse, or reconnect the Xbox 360 "
	"Controller and press the A Button.");
SettingT<GameSettingCollection> sControllerOption("sControllerOption",
	"Turn off 360 Controller in the Controls section of the Settings menu to switch back to Keyboard/Mouse.");
SettingT<GameSettingCollection> sDisableXBoxController("sDisableXBoxController", "360 Controller");
SettingT<GameSettingCollection> sCanNotTrainAnymore("sCanNotTrainAnymore",
	"You have reached your training limit for your current level.");
SettingT<GameSettingCollection> sCanNotTrainHigher("sCanNotTrainHigher", "This NPC cannot help you train any higher.");
SettingT<GameSettingCollection> sLockpickingTitle("sLockpickingTitle", "LOCKPICKING");
SettingT<GameSettingCollection> sIncrease("sIncrease", "Increase");
SettingT<GameSettingCollection> sDecrease("sDecrease", "Decrease");
SettingT<GameSettingCollection> sForceLock("sForceLock", "Force Lock");
SettingT<GameSettingCollection> sImpossibleLock("sImpossibleLock",
	"This lock cannot be picked. It requires a key to open.");
SettingT<GameSettingCollection> sLockpickSkillTooLow("sLockpickSkillTooLow",
	"You need a lockpick skill of %d to pick this lock.");
SettingT<GameSettingCollection> sOpenWithKey("sOpenWithKey", "Unlocked with %s.");
SettingT<GameSettingCollection> sNoLockPick("sNoLockPick", "You don't have any bobbypins.");
SettingT<GameSettingCollection> sNoLockPickIfCrimeAlert("sNoLockPickIfCrimeAlert",
	"You can't pick a lock while being pursued by guards.");
SettingT<GameSettingCollection> sLockpickSkillText("sLockpickSkillText", "Lockpick Skill");
SettingT<GameSettingCollection> sLockLevelText("sLockLevelText", "Lock Level");
SettingT<GameSettingCollection> sPicksRemainingText("sPicksRemainingText", "Bobby Pins");
SettingT<GameSettingCollection> sCloseButton("sCloseButton", "Close");
SettingT<GameSettingCollection> sOutOfLockpicks("sOutOfLockpicks", "You are out of lockpicks.");
SettingT<GameSettingCollection> sLockBroken("sLockBroken", "The lock has been broken.");
SettingT<GameSettingCollection> sLocked("sLocked", "Locked");
SettingT<GameSettingCollection> sPick("sPick", "Pick");
SettingT<GameSettingCollection> fPickBreakSecs("fPickBreakSecs", 1.0f);
SettingT<GameSettingCollection> sDealText("sDealText", "Deal");
SettingT<GameSettingCollection> sStayText("sStayText", "Stay");
SettingT<GameSettingCollection> sSurrenderText("sSurrenderText", "Surrender");
SettingT<GameSettingCollection> sSwitchHandsText("sSwitchHandsText", "Switch Hands");
SettingT<GameSettingCollection> sSplitText("sSplitText", "Split");
SettingT<GameSettingCollection> sDoubleDownText("sDoubleDownText", "Double Down");
SettingT<GameSettingCollection> sHitText("sHitText", "Hit");
SettingT<GameSettingCollection> sTwoToOneText("sTwoToOneText", "2 to 1");
SettingT<GameSettingCollection> sFirstDozenText("sFirstDozenText", "First Dozen");
SettingT<GameSettingCollection> sSecondDozenText("sSecondDozenText", "2nd Dozen");
SettingT<GameSettingCollection> sThirdDozenText("sThirdDozenText", "3rd Dozen");
SettingT<GameSettingCollection> gs_sBlacktext("sBlacktext", "Black");
SettingT<GameSettingCollection> sRedText("sRedText", "Red");
SettingT<GameSettingCollection> sEvenText("sEvenText", "Even");
SettingT<GameSettingCollection> gs_sOddtext("sOddtext", "Odd");
SettingT<GameSettingCollection> sPlaceBetText("sPlaceBetText", "Place Bet");
SettingT<GameSettingCollection> sRemoveBetText("sRemoveBetText", "Remove Bet");
SettingT<GameSettingCollection> sTotalBetText("sTotalBetText", "Total Bet: ");
SettingT<GameSettingCollection> sYouBreakEvenText("sYouBreakEvenText", "You break even");
SettingT<GameSettingCollection> sPayoutText("sPayoutText", "Payout");
SettingT<GameSettingCollection> sBetText("sBetText", "Bet");
SettingT<GameSettingCollection> sYouLoseText("sYouLose", "You lose");
SettingT<GameSettingCollection> sYouWinText("sYouWin", "You win");
SettingT<GameSettingCollection> sCasinoChipText("sCasinoChipText", "chip");
SettingT<GameSettingCollection> sUnluckyLoseText("sUnluckyLoseText", "You feel unlucky. You lose");
SettingT<GameSettingCollection> sLuckyWinText("sLuckyWinText", "You feel lucky. You win");
SettingT<GameSettingCollection> sSpinText("sSpinText", "Spin");
SettingT<GameSettingCollection> sPayoutListText("sPayoutListText", "Payout List");
SettingT<GameSettingCollection> sCurrentBetText("sCurrentBetText", "Current Bet: ");
SettingT<GameSettingCollection> sChipCountText("sChipCountText", "Chips: ");
SettingT<GameSettingCollection> sCasinoEarningsText("sCasinoEarningsText", "Earnings: ");
SettingT<GameSettingCollection> sIncreaseBetText("sIncreaseBetText", "Increase Bet");
SettingT<GameSettingCollection> sDecreaseBetText("sDecreaseBetText", "Decrease Bet");
SettingT<GameSettingCollection> gs_sBetMax("sBetMax", "Bet Max");
SettingT<GameSettingCollection> sGamblingBannedText("sGamblingBannedText",
	"You have been banned from gambling at this casino.");
SettingT<GameSettingCollection> sGamblingBrokeText("sGamblingBrokeText",
	"You must purchase chips before you can play this game.");
SettingT<GameSettingCollection> sGamblingMinWinText("sGamblingMinWinText",
	"You have not won enough at this casino to play this game.");
SettingT<GameSettingCollection> sSlotPressAnyButtonText("sSlotPressAnyButtonText",
	"Press any valid slot machine button to continue.");
SettingT<GameSettingCollection> iAntiCheatDuration("iAntiCheatDuration", 60);
SettingT<GameSettingCollection> sSlotAntiCheatText("sSlotAntiCheatText",
	"This machine appears to be taking a minute to reset itself as an anti-cheating measure.");
SettingT<GameSettingCollection> sBlackjackAntiCheatText("sBlackjackAntiCheatText",
	"The dealer is taking a minute to swap out the decks as an anti-cheating measure.");
SettingT<GameSettingCollection> sRouletteAntiCheatText("sRouletteAntiCheatText",
	"The croupier is taking a minute to check the wheel for bias as an anti-cheating measure.");
SettingT<GameSettingCollection> sAntiCheatTimeRemainingText("sAntiCheatTimeRemainingText", "Time Remaining: ");
SettingT<GameSettingCollection> sPlayerOutOfMoneyText("sPlayerOutOfMoneyText",
	"You do not have funds to play Caravan.");
SettingT<GameSettingCollection> sNPCOutOfMoneyText("sNPCOutOfMoneyText", "does not have any money to place a bet.");
SettingT<GameSettingCollection> sCardCountText("sCardCountText", "You must have at least 30 cards to play Caravan.");
SettingT<GameSettingCollection> sCurrentAnteText("sCurrentAnteText", "Current Ante: ");
SettingT<GameSettingCollection> sTotalFundsText("sTotalFundsText", "Total Funds: ");
SettingT<GameSettingCollection> sAutoMatchText("sAutoMatchText", "Auto Match");
SettingT<GameSettingCollection> sRaiseText("sRaiseText", "Raise");
SettingT<GameSettingCollection> sAcceptText("sAcceptText", "Accept");
SettingT<GameSettingCollection> sCardsInDeckText("sCardsInDeckText", "Cards in Deck: ");
SettingT<GameSettingCollection> sTotalCardsText("sTotalCardsText", "Total Cards: ");
SettingT<GameSettingCollection> sCaravanDeckText("sCaravanDeckText", "Caravan Deck");
SettingT<GameSettingCollection> sAllCardsText("sAllCardsText", "All Cards");
SettingT<GameSettingCollection> sNavigateText("sNavigateText", "Navigate");
SettingT<GameSettingCollection> sAddText("sAddText", "Add");
SettingT<GameSettingCollection> sRemoveText("sRemoveText", "Remove");
SettingT<GameSettingCollection> sPlayCaravanText("sPlayCaravanText", "Play Caravan");
SettingT<GameSettingCollection> sAntePotText("sAntePotText", "Ante Pot: ");
SettingT<GameSettingCollection> sNetTotalText("sNetTotalText", "Net Total: ");
SettingT<GameSettingCollection> sWinLossText("sWinLossText", "Win/Loss: ");
SettingT<GameSettingCollection> sForfeitGameText("sForfeitGameText", "Forfeit Game");
SettingT<GameSettingCollection> sSelectCardText("sSelectCardText", "Select Card");
SettingT<GameSettingCollection> sPlaceCardText("sPlaceCardText", "Place Card");
SettingT<GameSettingCollection> sDiscardSelectedText("sDiscardSelectedText", "Discard Selected");
SettingT<GameSettingCollection> sDiscardCardText("sDiscardCardText", "Discard Card");
SettingT<GameSettingCollection> sDiscardTrackText("sDiscardTrackText", "Discard Track");
SettingT<GameSettingCollection> sQuitCaravanText("sQuitCaravanText",
	"If you quit now, you will forfeit the money wagered. Are you sure you wish to forfeit this match?");
SettingT<GameSettingCollection> sCaravanWinningsText("sCaravanWinningsText", "Winnings to Date: ");
SettingT<GameSettingCollection> sCaravanLossesText("sCaravanLossesText", "Losses to Date: ");
SettingT<GameSettingCollection> sCaravanWinLossText("sCaravanWinLossText", "Win/Loss Totals: ");
SettingT<GameSettingCollection> sCaravanBiggestAnteText("sCaravanBiggestAnteText", "Biggest Ante Pot Won: ");
SettingT<GameSettingCollection> sCaravanPressAnyKeyText("sCaravanPressAnyKeyText", "Press Any Button to Continue");
SettingT<GameSettingCollection> sCaravanYouWinText("sCaravanYouWinText", "You win!");
SettingT<GameSettingCollection> sCaravanYouLoseText("sCaravanYouLoseText", "You Lose!");
SettingT<INISettingCollection> iSweetSpotLengthMinVeryEasy("iSweetSpotLengthMinVeryEasy", 400);
SettingT<INISettingCollection> iSweetSpotLengthMinEasy("iSweetSpotLengthMinEasy", 200);
SettingT<INISettingCollection> iSweetSpotLengthMinAverage("iSweetSpotLengthMinAverage", 100);
SettingT<INISettingCollection> iSweetSpotLengthMinHard("iSweetSpotLengthMinHard", 50);
SettingT<INISettingCollection> iSweetSpotLengthMinVeryHard("iSweetSpotLengthMinVeryHard", 25);
SettingT<INISettingCollection> iSweetSpotLengthMaxVeryEasy("iSweetSpotLengthMaxVeryEasy", 400);
SettingT<INISettingCollection> iSweetSpotLengthMaxEasy("iSweetSpotLengthMaxEasy", 200);
SettingT<INISettingCollection> iSweetSpotLengthMaxAverage("iSweetSpotLengthMaxAverage", 100);
SettingT<INISettingCollection> iSweetSpotLengthMaxHard("iSweetSpotLengthMaxHard", 50);
SettingT<INISettingCollection> iSweetSpotLengthMaxVeryHard("iSweetSpotLengthMaxVeryHard", 25);
SettingT<INISettingCollection> iConcentricLengthMinVeryEasy("iConcentricLengthMinVeryEasy", 200);
SettingT<INISettingCollection> iConcentricLengthMinEasy("iConcentricLengthMinEasy", 200);
SettingT<INISettingCollection> iConcentricLengthMinAverage("iConcentricLengthMinAverage", 200);
SettingT<INISettingCollection> iConcentricLengthMinHard("iConcentricLengthMinHard", 200);
SettingT<INISettingCollection> iConcentricLengthMinVeryHard("iConcentricLengthMinVeryHard", 200);
SettingT<INISettingCollection> iConcentricLengthMaxVeryEasy("iConcentricLengthMaxVeryEasy", 200);
SettingT<INISettingCollection> iConcentricLengthMaxEasy("iConcentricLengthMaxEasy", 200);
SettingT<INISettingCollection> iConcentricLengthMaxAverage("iConcentricLengthMaxAverage", 200);
SettingT<INISettingCollection> iConcentricLengthMaxHard("iConcentricLengthMaxHard", 200);
SettingT<INISettingCollection> iConcentricLengthMaxVeryHard("iConcentricLengthMaxVeryHard", 200);
SettingT<GameSettingCollection> sAutoSaveDisabledDueToLackOfSpace("sAutoSaveDisabledDueToLackOfSpace",
	"The selected storage device does not have sufficient space available to save your progress. Autosave is disabled. "
	"Please select another device or delete some of your saved content.");
SettingT<GameSettingCollection> sSaveGameXboxContentIsMissing("sSaveGameXboxContentIsMissing",
	"This save game relies on downloadable content that is no longer available. Some objects may be no longer "
	"available. Continue loading?");
static SettingT<GameSettingCollection> sAutoSaveDisabledLoadAnyways("sAutoSaveDisabledLoadAnyways",
	"Selected storage device has insufficient\nspace to save progress. Autosave will\nbe disabled. Continue Loading?");
SettingT<GameSettingCollection> sSaveGameContentIsMissing("sSaveGameContentIsMissing",
	"This save relies on content that is no longer present. Some objects may no longer be available. Continue "
	"Loading?");
SettingT<GameSettingCollection> sSaveGameDeviceError("sSaveGameDeviceError",
	"There was an error accessing the device.  Please try again.");
SettingT<GameSettingCollection> sSaveGameOutOfDiskSpace("sSaveGameOutOfDiskSpace",
	"The selected storage device does not have sufficient space available. Please select another storage device or "
	"delete some of your existing saved content.");
SettingT<GameSettingCollection> sSaveNotAvailable("sSaveNotAvailable", "No valid save game found.");
static SettingT<GameSettingCollection> sMenuDisplayXBoxSaveMessage("sMenuDisplayXBoxSaveMessage",
	"Saving content.\nPlease don't turn off your console.");
SettingT<GameSettingCollection> sMenuDisplayShortXBoxSaveMessage("sMenuDisplayShortXBoxSaveMessage", "Saving...");
SettingT<GameSettingCollection> sMenuDisplayXBoxFailSaveSettings("sMenuDisplayXBoxFailSaveSettings",
	"Your settings could not be saved. Your Gamer Profile is not available. Please check your storage devices.");
SettingT<GameSettingCollection> sMenuDisplayFailSaveSettings("sMenuDisplayFailSaveSettings",
	"There is no storage device selected. Settings will not be saved.");
SettingT<GameSettingCollection> sMenuDisplaySave("sMenuDisplaySave", "Save");
SettingT<GameSettingCollection> sMenuDisplayPlayTime("sMenuDisplayPlayTime", "Play Time");
SettingT<GameSettingCollection> sMenuDisplayNoSaves("sMenuDisplayNoSaves", "No saved games.");
SettingT<GameSettingCollection> sMenuDisplayNewSave("sMenuDisplayNewSave", "New Save");
SettingT<GameSettingCollection> sMenuDisplayQuicksaveName("sMenuDisplayQuicksaveName", "Quicksave");
SettingT<GameSettingCollection> sMenuDisplayAutosaveName("sMenuDisplayAutosaveName", "Autosave");
SettingT<GameSettingCollection> sMenuDisplayLevelString("sMenuDisplayLevelString", "Level");
SettingT<GameSettingCollection> sMenuDisplayUnknownLocationString("sMenuDisplayUnknownLocationString",
	"Unknown Location");
SettingT<GameSettingCollection> sMenuDisplayDayString("sMenuDisplayDayString", "Day");
SettingT<GameSettingCollection> sSaveGameNoMasterFilesFound("sSaveGameNoMasterFilesFound",
	"None of the master files used in this save game are loaded.  Returning to game.");
SettingT<GameSettingCollection> sSaveGameIsCorrupt("sSaveGameIsCorrupt",
	"The save game is corrupt.  Returning to game.");
SettingT<GameSettingCollection> sSaveGameCorrupt("sSaveGameCorrupt", "Corrupt");
SettingT<GameSettingCollection> sSaveGameNoLongerAvailable("sSaveGameNoLongerAvailable",
	"The save game is no longer available.  Returning to game.");
SettingT<GameSettingCollection> sSaveGameCorruptMenuMessage("sSaveGameCorruptMenuMessage",
	"The save game is corrupt and cannot be loaded.");
SettingT<GameSettingCollection> sSaveOverSaveGame("sSaveOverSaveGame",
	"Are you sure you want to overwrite this save game?");
SettingT<GameSettingCollection> sDeleteSaveGame("sDeleteSaveGame", "Are you sure you want to delete this save game?");
SettingT<GameSettingCollection> sLoadFromMainMenu("sLoadFromMainMenu", "Are you sure you want to load this game?");
SettingT<GameSettingCollection> sLoadWhilePlaying("sLoadWhilePlaying",
	"Are you sure you want to load this game?  All unsaved progress will be lost.");
SettingT<GameSettingCollection> sCantSaveNow("sCantSaveNow", "You cannot save right now.");
SettingT<GameSettingCollection> sQuickSaving("sQuickSaving", "Quicksaving...");
SettingT<GameSettingCollection> sQuickLoading("sQuickLoading", "Quickloading...");
SettingT<GameSettingCollection> sCantQuickSave("sCantQuickSave", "You can't Quicksave while the game is paused.");
SettingT<GameSettingCollection> sCantQuickLoad("sCantQuickLoad", "You can't Quickload while the game is paused.");
SettingT<GameSettingCollection> sFileNotFound("sFileNotFound", "File does not exist");
SettingT<GameSettingCollection> sAutoSaving("sAutoSaving", "Autosaving...");
SettingT<GameSettingCollection> sAutoLoading("sAutoLoading", "Autoloading...");
SettingT<GameSettingCollection> sLoading("sLoading", "Loading...");
SettingT<GameSettingCollection> sNewGame("sNewGame", "New Game");
SettingT<GameSettingCollection> sSaveSuccessful("sSaveSuccessful", "Save successful.");
SettingT<GameSettingCollection> sSaveFailed("sSaveFailed", "Save failed.");
SettingT<GameSettingCollection> sDeleteSuccessful("sDeleteSuccessful", "Save game deleted.");
SettingT<GameSettingCollection> sDeviceRemoved("sDeviceRemoved",
	"The storage device you selected is no longer available.  You will need to select a new device the next time you "
	"save.  Until then, Autosave is disabled.");
SettingT<GameSettingCollection> sNoDeviceSelected("sNoDeviceSelected",
	"You have not selected a storage device.  You will need to select one before you can save.  Until then, Autosave "
	"is disabled.");
SettingT<GameSettingCollection> sRestartToUseNewContent("sRestartToUseNewContent",
	"New content has been added.  You will need to restart to use this content.  All unsaved progress will be lost.");
SettingT<GameSettingCollection> sYesRestart("sYesRestart", "Yes, restart the game.");
SettingT<GameSettingCollection> sNoRestart("sNoRestart", "No, keep playing.");
SettingT<GameSettingCollection> sCorruptContentMessage("sCorruptContentMessage",
	"Some of your downloadable content is corrupt and could not be loaded.");
SettingT<GameSettingCollection> sRestartToUseProfileContent("sRestartToUseProfileContent",
	"In order to use the downloaded content associated with this profile, you will need to restart.");
SettingT<GameSettingCollection> sRestartBecauseContentRemoved("sRestartBecauseContentRemoved",
	"The game must restart now because downloaded content has been removed.");
SettingT<GameSettingCollection> sRestartSignedOut("sRestartSignedOut",
	"You have signed out of your profile and will now be returned to the title screen.");
SettingT<GameSettingCollection> sNoProfileSelected("sNoProfileSelected",
	"You have not signed in to a Gamer Profile.  You will need to sign in to a Gamer Profile and then select a storage "
	"device before you can save.  Until then, Autosave is disabled.");
SettingT<GameSettingCollection> sLoadingContentMessage("sLoadingContentMessage", "Loading extra content. Please wait.");
SettingT<GameSettingCollection> sFindingContentMessage("sFindingContentMessage", "Looking for content.  Please wait.");
SettingT<GameSettingCollection> sLoadSavedGame("sLoadSavedGame", "Load Saved Game");
SettingT<GameSettingCollection> sSaveGame("sSaveGame", "Save Game");
SettingT<GameSettingCollection> sStartANewGame("sStartANewGame", "Start a New Game?");
SettingT<GameSettingCollection> sLoadThisGame("sLoadThisGame", "Load this game?");
SettingT<GameSettingCollection> sDelete("sDelete", "Delete");
SettingT<GameSettingCollection> sXBoxLoadNoDeviceMessage("sXBoxLoadNoDeviceMessage",
	"The storage device you selected is no longer available. Load canceled.");
SettingT<GameSettingCollection> sAutosaveCorruptMessage("sAutosaveCorruptMessage",
	"The Autosave content is damaged. Do you want to overwrite it?");
SettingT<GameSettingCollection> gs_sContinuePlayingAfterCredits("sContinuePlayingAfterCredits",
	"Congratulations on completing Fallout: New Vegas! Would you like to load a save prior to the Hoover Dam battle? "
	"(If you select no, you will be returned to the main menu).");
SettingT<GameSettingCollection> sPoisonNoWeaponMessage("sPoisonNoWeaponMessage",
	"You must first equip a poison capable weapon.");
SettingT<GameSettingCollection> sPoisonAlreadyPoisonedMessage("sPoisonAlreadyPoisonedMessage",
	"The current weapon is already poisoned.");
SettingT<GameSettingCollection> sPoisonConfirmMessage("sPoisonConfirmMessage", "Do you want to poison the ");
SettingT<GameSettingCollection> sPoisonBowConfirmMessage("sPoisonBowConfirmMessage",
	"Do you want to poison your next shot with the ");
SettingT<GameSettingCollection> sEssentialCharacterDown("sEssentialCharacterDown", "is unconscious.");
SettingT<GameSettingCollection> sCanNotReadBook("sCanNotReadBook", "You cannot read a book during combat!");
SettingT<GameSettingCollection> iAllowRechargeDuringCombat("iAllowRechargeDuringCombat", 1);
SettingT<GameSettingCollection> iAllowRepairDuringCombat("iAllowRepairDuringCombat", 0);
SettingT<GameSettingCollection> iAllowAlchemyDuringCombat("iAllowAlchemyDuringCombat", 0);
SettingT<GameSettingCollection> sSuccessfulSneakAttackMain("sSuccessfulSneakAttackMain", "Sneak attack for ");
SettingT<GameSettingCollection> sSuccessfulSneakAttackEnd("sSuccessfulSneakAttackEnd", "X damage!");
SettingT<GameSettingCollection> sCriticalStrike("sCriticalStrike", "Critical Strike on");
SettingT<GameSettingCollection> sSneakAttackCriticalStrike("sSneakAttackCriticalStrike", "Sneak Attack Critical on");
SettingT<GameSettingCollection> sCriticalStrikeIcon("sCriticalStrikeIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sSneakAttackCriticalStrikeIcon("sSneakAttackCriticalStrikeIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sKnownEffects("sKnownEffects", "Known Effects");
SettingT<GameSettingCollection> sAddedEffects("sAddedEffects", "Added Effects");
SettingT<GameSettingCollection> sMagnitudeText("sMagnitudeText", "Magnitude");
SettingT<GameSettingCollection> sDurationText("sDurationText", "Duration");
SettingT<GameSettingCollection> sMagnitudeIsLevelText("sMagnitudeIsLevelText", "Target Level");
SettingT<GameSettingCollection> sAreaText("sAreaText", "Area");
SettingT<GameSettingCollection> sRangeText("sRangeText", "Range");
SettingT<GameSettingCollection> sSelfRange("sSelfRange", "Self");
SettingT<GameSettingCollection> sTargetRange("sTargetRange", "Target");
SettingT<GameSettingCollection> sTouchRange("sTouchRange", "Strike");
SettingT<GameSettingCollection> sNoCharge("sNoCharge", "The item does not have enough charge.");
SettingT<GameSettingCollection> sEffectAlreadyAdded("sEffectAlreadyAdded",
	"Effect has already been added.  Edit the effect instead.");
SettingT<GameSettingCollection> sMiscUnknownEffect("sMiscUnknownEffect", "(Unknown Effect)");
SettingT<GameSettingCollection> sMiscConstantEffect("sMiscConstantEffect", "Constant Effect");
SettingT<GameSettingCollection> sIngredientFail("sIngredientFail", " Ingredient had no effect.");
SettingT<GameSettingCollection> sChemsWornOff("sChemsWornOff", "%s has worn off.");
SettingT<GameSettingCollection> sChemsAddicted("sChemsAddicted", "You have become addicted to %s.");
SettingT<GameSettingCollection> sChemsWithdrawal("sChemsWithdrawal", "You are suffering from %s withdrawal.");
SettingT<GameSettingCollection> sNoTalkUnConscious("sNoTalkUnConscious", " is unconscious.");
SettingT<GameSettingCollection> sNoTalkFleeing("sNoTalkFleeing", " is fleeing for their life.");
SettingT<GameSettingCollection> gs_sNoPickPocketAgain("sNoPickPocketAgain", " has already caught you.");
SettingT<GameSettingCollection> sNoChildUse("sNoChildUse", "Children cannot use that.");
SettingT<GameSettingCollection> sNoSitOnOwnedFurniture("sNoSitOnOwnedFurniture", "You cannot sit on owned furniture.");
SettingT<GameSettingCollection> sServeTimeQuestion("sServeTimeQuestion", "Do you just want to serve your time?");
SettingT<GameSettingCollection> sOpenedContainer("sOpenedContainer", "Opened the container with a key.");
SettingT<GameSettingCollection> sInvalidPickpocket("sInvalidPickpocket",
	"You cannot place items in a container while pickpocketing.");
SettingT<GameSettingCollection> sStealHorse("sStealHorse", "Do you want to steal this horse?");
SettingT<GameSettingCollection> sGotAwayWithStealing("sGotAwayWithStealing", "got away with stealing");
SettingT<GameSettingCollection> sVampireFeedQuestion("sVampireFeedQuestion",
	"What would you like to do with this innocent townsperson?");
SettingT<GameSettingCollection> sNoEatQuestItem("sNoEatQuestItem", "You can not eat quest items.");
SettingT<GameSettingCollection> sNoMoreFollowers("sNoMoreFollowers", "You have too many followers.");
SettingT<GameSettingCollection> sVampireFeed("sVampireFeed", "Feed");
SettingT<GameSettingCollection> sVampireTalk("sVampireTalk", "Talk");
SettingT<GameSettingCollection> sSteal("sSteal", "Steal");
SettingT<GameSettingCollection> sPickpocket("sPickpocket", "Pickpocket");
SettingT<GameSettingCollection> sSearch("sSearch", "Search");
SettingT<GameSettingCollection> sActivationChoiceMessage("sActivationChoiceMessage", "What would you like to do?");
SettingT<GameSettingCollection> sNoFastTravelCombat("sNoFastTravelCombat", "You cannot fast travel during combat!");
SettingT<GameSettingCollection> sNoFastTravelHostileActorsNear("sNoFastTravelHostileActorsNear",
	"You cannot fast travel when enemies are nearby.");
SettingT<GameSettingCollection> sNoFastTravelCell("sNoFastTravelCell", "You cannot fast travel from this location.");
SettingT<GameSettingCollection> sNoFastTravelScriptBlock("sNoFastTravelScriptBlock",
	"Fast travel is currently unavaliable from this location.");
SettingT<GameSettingCollection> sNoFastTravelOverencumbered("sNoFastTravelOverencumbered",
	"You cannot fast travel while overencumbered.");
SettingT<GameSettingCollection> sFastTravelNoTravelHealthDamage("sFastTravelNoTravelHealthDamage",
	"You can't fast travel while taking health damage");
SettingT<GameSettingCollection> sNoFastTravelUndiscovered("sNoFastTravelUndiscovered",
	"You have not discovered this location yet.");
SettingT<GameSettingCollection> sNoFastTravelAlarm("sNoFastTravelAlarm",
	"You cannot fast travel while guards are pursuing you.");
SettingT<GameSettingCollection> sNoFastTravelInAir("sNoFastTravelInAir",
	"You can't fast travel while jumping or falling.");
SettingT<GameSettingCollection> sPlayerLeavingBorderRegion("sPlayerLeavingBorderRegion", "You cannot go that way.");
SettingT<GameSettingCollection> sLoadingArea("sLoadingArea", "Loading area...");
SettingT<GameSettingCollection> sDiscoveredText("sDiscoveredText", "You have discovered");
SettingT<GameSettingCollection> sRadioSignalLost("sRadioSignalLost", "%s signal lost.");
SettingT<GameSettingCollection> sRadioStationDiscovered("sRadioStationDiscovered", "%s signal found.");
SettingT<GameSettingCollection> sMiscPlayerDeadMessage("sMiscPlayerDeadMessage", "Reloading the most recent save game");
SettingT<GameSettingCollection> sMiscPlayerDeadLoadOption("sMiscPlayerDeadLoadOption", "Reload");
SettingT<GameSettingCollection> sMiscPlayerDeadMenuOption("sMiscPlayerDeadMenuOption", "Main Menu");
SettingT<GameSettingCollection> sAttributeDamaged("sAttributeDamaged", "has been damaged");
SettingT<GameSettingCollection> sAttributeDrained("sAttributeDrained", "is being drained");
SettingT<GameSettingCollection> sContractedDisease("sContractedDisease", "You have contracted");
SettingT<GameSettingCollection> sRadiationIncrease("sRadiationIncrease", "Your radiation level has increased");
SettingT<GameSettingCollection> sRadiationDecrease("sRadiationDecrease", "Your radiation level has decreased");
SettingT<GameSettingCollection> sRadiationSick("sRadiationSick", "You are now sick with");
SettingT<GameSettingCollection> sRadiationNotSick("sRadiationNotSick", "You no longer have radiation sickness");
SettingT<GameSettingCollection> sMagicEffectResisted("sMagicEffectResisted", "effect resisted");
SettingT<GameSettingCollection> sRewardXP("sRewardXP", "Experience Points Earned");
SettingT<GameSettingCollection> sRewardXPIcon("sRewardXPIcon", "Interface\\Shared\\Messages\\glow_XP_Gain.dds");
SettingT<GameSettingCollection> sDehydrationIncrease("sDehydrationIncrease", "Your dehydration level has increased");
SettingT<GameSettingCollection> sDehydrationDecrease("sDehydrationDecrease", "Your dehydration level has decreased");
SettingT<GameSettingCollection> sDehydrationNotSick("sDehydrationNotSick", "You no longer have dehydration sickness");
SettingT<GameSettingCollection> sHungerIncrease("sHungerIncrease", "Your Hunger level has increased");
SettingT<GameSettingCollection> sHungerDecrease("sHungerDecrease", "Your Hunger level has decreased");
SettingT<GameSettingCollection> sHungerNotSick("sHungerNotSick", "You no longer have Hunger sickness");
SettingT<GameSettingCollection> sSleepDeprevationIncrease("sSleepDeprevationIncrease",
	"Your SleepDeprevation level has increased");
SettingT<GameSettingCollection> sSleepDeprevationDecrease("sSleepDeprevationDecrease",
	"Your SleepDeprevation level has decreased");
SettingT<GameSettingCollection> sSleepDeprevationNotSick("sSleepDeprevationNotSick",
	"You no longer have SleepDeprevation sickness");
SettingT<GameSettingCollection> sPCControlsTextPrefix("sPCControlsTextPrefix", "push");
SettingT<GameSettingCollection> sPCControlsTriggerPrefix("sPCControlsTriggerPrefix", "pull");
SettingT<GameSettingCollection> sPCControlsTextNone("sPCControlsTextNone", "<NOT BOUND>");
SettingT<GameSettingCollection> sPCMenuHintR("sPCMenuHintR", "R)");
SettingT<GameSettingCollection> sPCMenuHintA("sPCMenuHintA", "A)");
SettingT<GameSettingCollection> sPCMenuHintE("sPCMenuHintE", "E)");
SettingT<GameSettingCollection> sPCMenuHintS("sPCMenuHintS", "S)");
SettingT<GameSettingCollection> sPCMenuHintX("sPCMenuHintX", "X)");
SettingT<GameSettingCollection> sPCMenuHintF("sPCMenuHintF", "F)");
SettingT<GameSettingCollection> sPCMenuHintW("sPCMenuHintW", "W)");
SettingT<GameSettingCollection> sPCMenuHintQ("sPCMenuHintQ", "Q)");
SettingT<GameSettingCollection> sPCMenuHintRMB("sPCMenuHintRMB", "RMB)");
SettingT<GameSettingCollection> sHeadDamagedMessage("sHeadDamagedMessage", "Your head is now damaged.");
SettingT<GameSettingCollection> sChestDamagedMessage("sChestDamagedMessage", "Your chest is now damaged.");
SettingT<GameSettingCollection> sLeftLegDamagedMessage("sLeftLegDamagedMessage", "Your left leg is now crippled.");
SettingT<GameSettingCollection> sRightLegDamagedMessage("sRightLegDamagedMessage", "Your right leg is now crippled.");
SettingT<GameSettingCollection> sLeftArmDamagedMessage("sLeftArmDamagedMessage", "Your left arm is now crippled.");
SettingT<GameSettingCollection> sRightArmDamagedMessage("sRightArmDamagedMessage", "Your right arm is now crippled.");
SettingT<GameSettingCollection> sHeadDamagedIcon("sHeadDamagedIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sChestDamagedIcon("sChestDamagedIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sLeftLegDamagedIcon("sLeftLegDamagedIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRightLegDamagedIcon("sRightLegDamagedIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sLeftArmDamagedIcon("sLeftArmDamagedIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRightArmDamagedIcon("sRightArmDamagedIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sPartCrippledByPlayerMessage("sPartCrippledByPlayerMessage", "Crippled");
SettingT<GameSettingCollection> sFrenziedByPlayerMessage("sFrenziedByPlayerMessage", "Frenzied");
SettingT<GameSettingCollection> sHeadDamagedByPlayerIcon("sHeadDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sChestDamagedByPlayerIcon("sChestDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sLeftLegDamagedByPlayerIcon("sLeftLegDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRightLegDamagedByPlayerIcon("sRightLegDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sLeftArmDamagedByPlayerIcon("sLeftArmDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRightArmDamagedByPlayerIcon("sRightArmDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sBrainDamagedByPlayerIcon("sBrainDamagedByPlayerIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
UserActionSettingArray psUserActions = {new SettingT<GameSettingCollection>("sUActnForward", "Forward"),
	new SettingT<GameSettingCollection>("sUActnBack", "Back"),
	new SettingT<GameSettingCollection>("sUActnSldleft", "Strafe Left"),
	new SettingT<GameSettingCollection>("sUActnSldright", "Strafe Right"),
	new SettingT<GameSettingCollection>("sUActnUse", "Attack"),
	new SettingT<GameSettingCollection>("sUActnActivate", "Activate"),
	new SettingT<GameSettingCollection>("sUActnBlock", "Block"),
	new SettingT<GameSettingCollection>("sUActnRdyitem", "Ready/Reload"),
	new SettingT<GameSettingCollection>("sUActnCrouch", "Sneak"),
	new SettingT<GameSettingCollection>("sUActnRun", "Run"),
	new SettingT<GameSettingCollection>("sUActnTogglerun", "Always Run"),
	new SettingT<GameSettingCollection>("sUActnAutomove", "Auto Move"),
	new SettingT<GameSettingCollection>("sUActnJump", "Jump"),
	new SettingT<GameSettingCollection>("sUActnTogglepov", "Change View"),
	new SettingT<GameSettingCollection>("sUActnMenumode", "Pip-Boy"),
	new SettingT<GameSettingCollection>("sUActnRestmenu", "Wait"),
	new SettingT<GameSettingCollection>("sUActnVats", "VATS"),
	new SettingT<GameSettingCollection>("sUActnHotKey1", "Hotkey 1"),
	new SettingT<GameSettingCollection>("sUActnAMmoSwap", "Ammo Swap"),
	new SettingT<GameSettingCollection>("sUActnHotKey3", "Hotkey 3"),
	new SettingT<GameSettingCollection>("sUActnHotKey4", "Hotkey 4"),
	new SettingT<GameSettingCollection>("sUActnHotKey5", "Hotkey 5"),
	new SettingT<GameSettingCollection>("sUActnHotKey6", "Hotkey 6"),
	new SettingT<GameSettingCollection>("sUActnHotKey7", "Hotkey 7"),
	new SettingT<GameSettingCollection>("sUActnHotKey8", "Hotkey 8"),
	new SettingT<GameSettingCollection>("sUActnQuicksave", "Quicksave"),
	new SettingT<GameSettingCollection>("sUActnQuickload", "Quickload"),
	new SettingT<GameSettingCollection>("sUActnGrab", "Grab")};
pDeviceNamesA_t pDeviceNamesA = {new SettingT<GameSettingCollection>("sDeviceKeyboard", "Keyboard"),
	new SettingT<GameSettingCollection>("sDeviceMouse", "Mouse"),
	new SettingT<GameSettingCollection>("sDeviceJoystick", "Joystick"),
	new SettingT<GameSettingCollection>("sDeviceGamepad", "Xbox 360 Controller")};
psKeyboard_t psKeyboard = {nullptr, new SettingT<GameSettingCollection>("sKBEsc", "Esc"),
	new SettingT<GameSettingCollection>("sKB1", "1"), new SettingT<GameSettingCollection>("sKB2", "2"),
	new SettingT<GameSettingCollection>("sKB3", "3"), new SettingT<GameSettingCollection>("sKB4", "4"),
	new SettingT<GameSettingCollection>("sKB5", "5"), new SettingT<GameSettingCollection>("sKB6", "6"),
	new SettingT<GameSettingCollection>("sKB7", "7"), new SettingT<GameSettingCollection>("sKB8", "8"),
	new SettingT<GameSettingCollection>("sKB9", "9"), new SettingT<GameSettingCollection>("sKB0", "0"),
	new SettingT<GameSettingCollection>("sKBMinus", "-"), new SettingT<GameSettingCollection>("sKBEquals", "="),
	new SettingT<GameSettingCollection>("sKBBack", "Backspace"), new SettingT<GameSettingCollection>("sKBTab", "Tab"),
	new SettingT<GameSettingCollection>("sKBQ", "Q"), new SettingT<GameSettingCollection>("sKBW", "W"),
	new SettingT<GameSettingCollection>("sKBE", "E"), new SettingT<GameSettingCollection>("sKBR", "R"),
	new SettingT<GameSettingCollection>("sKBT", "T"), new SettingT<GameSettingCollection>("sKBY", "Y"),
	new SettingT<GameSettingCollection>("sKBU", "U"), new SettingT<GameSettingCollection>("sKBI", "I"),
	new SettingT<GameSettingCollection>("sKBO", "O"), new SettingT<GameSettingCollection>("sKBP", "P"),
	new SettingT<GameSettingCollection>("sKBLBracket", "["), new SettingT<GameSettingCollection>("sKBRBracket", "]"),
	new SettingT<GameSettingCollection>("sKBReturn", "Enter"),
	new SettingT<GameSettingCollection>("sKBLControl", "L-Ctrl"), new SettingT<GameSettingCollection>("sKBA", "A"),
	new SettingT<GameSettingCollection>("sKBS", "S"), new SettingT<GameSettingCollection>("sKBD", "D"),
	new SettingT<GameSettingCollection>("sKBF", "F"), new SettingT<GameSettingCollection>("sKBG", "G"),
	new SettingT<GameSettingCollection>("sKBH", "H"), new SettingT<GameSettingCollection>("sKBJ", "J"),
	new SettingT<GameSettingCollection>("sKBK", "K"), new SettingT<GameSettingCollection>("sKBL", "L"),
	new SettingT<GameSettingCollection>("sKBSemicolon", ";"), new SettingT<GameSettingCollection>("sKBApostrophe", "'"),
	new SettingT<GameSettingCollection>("sKBTilde", "~"), new SettingT<GameSettingCollection>("sKBLShift", "L-Shift"),
	new SettingT<GameSettingCollection>("sKBBackslash", "\\"), new SettingT<GameSettingCollection>("sKBZ", "Z"),
	new SettingT<GameSettingCollection>("sKBX", "X"), new SettingT<GameSettingCollection>("sKBC", "C"),
	new SettingT<GameSettingCollection>("sKBV", "V"), new SettingT<GameSettingCollection>("sKBB", "B"),
	new SettingT<GameSettingCollection>("sKBN", "N"), new SettingT<GameSettingCollection>("sKBM", "M"),
	new SettingT<GameSettingCollection>("sKBComma", ","), new SettingT<GameSettingCollection>("sKBPeriod", "."),
	new SettingT<GameSettingCollection>("sKBSlash", "/"), new SettingT<GameSettingCollection>("sKBRShift", "R-Shift"),
	new SettingT<GameSettingCollection>("sKBMultiply", "Pad *"),
	new SettingT<GameSettingCollection>("sKBLMenu", "L-Alt"), new SettingT<GameSettingCollection>("sKBSpace", "Space"),
	new SettingT<GameSettingCollection>("sKBCapital", "Caps"), new SettingT<GameSettingCollection>("sKBF1", "F1"),
	new SettingT<GameSettingCollection>("sKBF2", "F2"), new SettingT<GameSettingCollection>("sKBF3", "F3"),
	new SettingT<GameSettingCollection>("sKBF4", "F4"), new SettingT<GameSettingCollection>("sKBF5", "F5"),
	new SettingT<GameSettingCollection>("sKBF6", "F6"), new SettingT<GameSettingCollection>("sKBF7", "F7"),
	new SettingT<GameSettingCollection>("sKBF8", "F8"), new SettingT<GameSettingCollection>("sKBF9", "F9"),
	new SettingT<GameSettingCollection>("sKBF10", "F10"), new SettingT<GameSettingCollection>("sKBNumlock", "Num"),
	new SettingT<GameSettingCollection>("sKBScroll", "Scroll"),
	new SettingT<GameSettingCollection>("sKBNumpad7", "Pad 7"),
	new SettingT<GameSettingCollection>("sKBNumpad8", "Pad 8"),
	new SettingT<GameSettingCollection>("sKBNumpad9", "Pad 9"),
	new SettingT<GameSettingCollection>("sKBSubtract", "Pad -"),
	new SettingT<GameSettingCollection>("sKBNumpad4", "Pad 4"),
	new SettingT<GameSettingCollection>("sKBNumpad5", "Pad 5"),
	new SettingT<GameSettingCollection>("sKBNumpad6", "Pad 6"), new SettingT<GameSettingCollection>("sKBAdd", "Pad +"),
	new SettingT<GameSettingCollection>("sKBNumpad1", "Pad 1"),
	new SettingT<GameSettingCollection>("sKBNumpad2", "Pad 2"),
	new SettingT<GameSettingCollection>("sKBNumpad3", "Pad 3"),
	new SettingT<GameSettingCollection>("sKBNumpad0", "Pad 0"),
	new SettingT<GameSettingCollection>("sKBDecimal", "Pad ."), nullptr, nullptr,
	new SettingT<GameSettingCollection>("sKBOEM102", "<"), new SettingT<GameSettingCollection>("sKBF11", "F11"),
	new SettingT<GameSettingCollection>("sKBF12", "F12"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, new SettingT<GameSettingCollection>("sKBF13", "F13"),
	new SettingT<GameSettingCollection>("sKBF14", "F14"), new SettingT<GameSettingCollection>("sKBF15", "F15"), nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	new SettingT<GameSettingCollection>("sKBABNTC1", "/"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, new SettingT<GameSettingCollection>("sKBABNTC2", "Pad ."), nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	new SettingT<GameSettingCollection>("sKBNumPadEquals", "Pad ="), nullptr, nullptr,
	new SettingT<GameSettingCollection>("sKBPrevtrack", "Prev"), new SettingT<GameSettingCollection>("sKBAt", "@"),
	new SettingT<GameSettingCollection>("sKBColon", ":"), new SettingT<GameSettingCollection>("sKBUnderline", "_"),
	nullptr, new SettingT<GameSettingCollection>("sKBStop", "Stop"), nullptr,
	new SettingT<GameSettingCollection>("sKBUnlabeled", "Unlabeled"), nullptr,
	new SettingT<GameSettingCollection>("sKBNextTrack", "Next"), nullptr, nullptr,
	new SettingT<GameSettingCollection>("sKBNumPadEnter", "Pad Enter"),
	new SettingT<GameSettingCollection>("sKBRControl", "R-Ctrl"), nullptr, nullptr,
	new SettingT<GameSettingCollection>("sKBMute", "Mute"),
	new SettingT<GameSettingCollection>("sKBCalculator", "Calc"),
	new SettingT<GameSettingCollection>("sKBPlayPause", "Play"), nullptr,
	new SettingT<GameSettingCollection>("sKBMediaStop", "Stop"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, new SettingT<GameSettingCollection>("sKBVolumeDown", "Vol -"), nullptr,
	new SettingT<GameSettingCollection>("sKBVolumeUp", "Vol +"), nullptr,
	new SettingT<GameSettingCollection>("sKBWebHome", "Web"),
	new SettingT<GameSettingCollection>("sKBNumPadComma", "Pad ,"), nullptr,
	new SettingT<GameSettingCollection>("sKBDivide", "Pad /"), nullptr,
	new SettingT<GameSettingCollection>("sKBSysRq", "PrntScrn"),
	new SettingT<GameSettingCollection>("sKBRMenu", "R-Alt"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, new SettingT<GameSettingCollection>("sKBPause", "Pause"),
	nullptr, new SettingT<GameSettingCollection>("sKBHome", "Home"), new SettingT<GameSettingCollection>("sKBUp", "Up"),
	new SettingT<GameSettingCollection>("sKBPrior", "PgUp"), nullptr,
	new SettingT<GameSettingCollection>("sKBLeft", "Left"), nullptr,
	new SettingT<GameSettingCollection>("sKBRight", "Right"), nullptr,
	new SettingT<GameSettingCollection>("sKBEnd", "End"), new SettingT<GameSettingCollection>("sKBDown", "Down"),
	new SettingT<GameSettingCollection>("sKBNext", "PgDn"), new SettingT<GameSettingCollection>("sKBInsert", "Insert"),
	new SettingT<GameSettingCollection>("sKBDelete", "Delete"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, new SettingT<GameSettingCollection>("sKBLWin", "LWin"),
	new SettingT<GameSettingCollection>("sKBRWin", "RWin"), new SettingT<GameSettingCollection>("sKBApps", "Apps"),
	new SettingT<GameSettingCollection>("sKBPower", "Power"), new SettingT<GameSettingCollection>("sKBSleep", "Sleep"),
	nullptr, nullptr, nullptr, new SettingT<GameSettingCollection>("sKBWake", "Wake"), nullptr,
	new SettingT<GameSettingCollection>("sKBWebSearch", "Search"),
	new SettingT<GameSettingCollection>("sKBWebFavorites", "Fav"),
	new SettingT<GameSettingCollection>("sKBWebRefresh", "Refresh"),
	new SettingT<GameSettingCollection>("sKBWebStop", "Web Stop"),
	new SettingT<GameSettingCollection>("sKBWebForward", "Web ->"),
	new SettingT<GameSettingCollection>("sKBWebBack", "Web <-"),
	new SettingT<GameSettingCollection>("sKBMyComputer", "My Comp"),
	new SettingT<GameSettingCollection>("sKBMail", "Mail"),
	new SettingT<GameSettingCollection>("sKBMediaSelect", "Select")};
psMouse_t psMouse = {new SettingT<GameSettingCollection>("sMouseLeftButton", "Mouse1"),
	new SettingT<GameSettingCollection>("sMouseRightButton", "Mouse2"),
	new SettingT<GameSettingCollection>("sMouseMiddleButton", "Wheel"),
	new SettingT<GameSettingCollection>("sMouseButton3", "Button 3"),
	new SettingT<GameSettingCollection>("sMouseButton4", "Button 4"),
	new SettingT<GameSettingCollection>("sMouseButton5", "Button 5"),
	new SettingT<GameSettingCollection>("sMouseButton6", "Button 6"),
	new SettingT<GameSettingCollection>("sMouseButton7", "Button 7"),
	new SettingT<GameSettingCollection>("sMouseButton8", "Button 8")};
psJoystick_t psJoystick = {new SettingT<GameSettingCollection>("sJoyUp", "Up"),
	new SettingT<GameSettingCollection>("sJoyUpRight", "Up-Rt"),
	new SettingT<GameSettingCollection>("sJoyRight", "Right"),
	new SettingT<GameSettingCollection>("sJoyDownRight", "Dn-Rt"),
	new SettingT<GameSettingCollection>("sJoyDown", "Down"),
	new SettingT<GameSettingCollection>("sJoyDownLeft", "Dn-Left"),
	new SettingT<GameSettingCollection>("sJoyLeft", "Left"),
	new SettingT<GameSettingCollection>("sJoyUpLeft", "Up-Left")};
psXBoxGamepadText_t psXBoxGamepadText = {new SettingT<GameSettingCollection>("sXBDPadY", "D-Pad Up / Down"),
	new SettingT<GameSettingCollection>("sXBDPadUp", "D-Pad Up"),
	new SettingT<GameSettingCollection>("sXBDPadDown", "D-Pad Down"),
	new SettingT<GameSettingCollection>("sXBDPadX", "D-Pad Left / Right"),
	new SettingT<GameSettingCollection>("sXBDPadRight", "D-Pad Right"),
	new SettingT<GameSettingCollection>("sXBDPadLeft", "D-Pad Left"),
	new SettingT<GameSettingCollection>("sXBStartBtn", "START"),
	new SettingT<GameSettingCollection>("sXBBackBtn", "Back"),
	new SettingT<GameSettingCollection>("sXBLThumb", "\\general_button_ls.dds\\"),
	new SettingT<GameSettingCollection>("sXBRThumb", "\\general_button_rs.dds\\"),
	new SettingT<GameSettingCollection>("sXBABtn", "\\general_button_a.dds\\"),
	new SettingT<GameSettingCollection>("sXBBBtn", "\\general_button_b.dds\\"),
	new SettingT<GameSettingCollection>("sXBXBtn", "\\general_button_x.dds\\"),
	new SettingT<GameSettingCollection>("sXBYBtn", "\\general_button_y.dds\\"),
	new SettingT<GameSettingCollection>("sXBRBumper", "\\general_button_rb.dds\\"),
	new SettingT<GameSettingCollection>("sXBLBumper", "\\general_button_lb.dds\\"),
	new SettingT<GameSettingCollection>("sXBLTrigger", "\\general_button_lt.dds\\"),
	new SettingT<GameSettingCollection>("sXBRTrigger", "\\general_button_rt.dds\\"),
	new SettingT<GameSettingCollection>("sXBLStick", "\\general_button_ls.dds\\"),
	new SettingT<GameSettingCollection>("sXBRStick", "\\general_button_rs.dds\\"),
	new SettingT<GameSettingCollection>("sXBLThumbY", "the Left Stick Up / Down"),
	new SettingT<GameSettingCollection>("sXBLThumbUp", "the Left Stick Up"),
	new SettingT<GameSettingCollection>("sXBLThumbDown", "the Left Stick Down"),
	new SettingT<GameSettingCollection>("sXBLThumbX", "the Left Stick Left / Right"),
	new SettingT<GameSettingCollection>("sXBLThumbRight", "the Left Stick Right"),
	new SettingT<GameSettingCollection>("sXBLThumbLeft", "the Left Stick Left"),
	new SettingT<GameSettingCollection>("sXBRThumbY", "the Right Stick Up / Down"),
	new SettingT<GameSettingCollection>("sXBRThumbUp", "the Right Stick Up"),
	new SettingT<GameSettingCollection>("sXBRThumbDown", "the Right Stick Down"),
	new SettingT<GameSettingCollection>("sXBRThumbX", "the Right Stick Left / Right"),
	new SettingT<GameSettingCollection>("sXBRThumbRight", "the Right Stick Right"),
	new SettingT<GameSettingCollection>("sXBRThumbLeft", "the Right Stick Left"),
	new SettingT<GameSettingCollection>("sXBUnboundBtn", "<NOT BOUND>"),
	new SettingT<GameSettingCollection>("sXBDPad", "D-Pad")};
g_PS3ButtonSettingNames_t g_PS3ButtonSettingNames = {
	new SettingT<GameSettingCollection>("sPS3DPadY", "Direction Button Up/Down"),
	new SettingT<GameSettingCollection>("sPS3DPadUp", "Direction Button Up"),
	new SettingT<GameSettingCollection>("sPS3DPadDown", "Direction Button Down"),
	new SettingT<GameSettingCollection>("sPS3DPadX", "Direction Button Left/Right"),
	new SettingT<GameSettingCollection>("sPS3DPadRight", "Direction Button Right"),
	new SettingT<GameSettingCollection>("sPS3DPadLeft", "Direction Button Left"),
	new SettingT<GameSettingCollection>("sPS3StartBtn", "Start"),
	new SettingT<GameSettingCollection>("sPS3SelectBtn", "Select"),
	new SettingT<GameSettingCollection>("sPS3L3", "\\general_button_ls.dds\\"),
	new SettingT<GameSettingCollection>("sPS3R3", "\\general_button_rs.dds\\"),
	new SettingT<GameSettingCollection>("sPS3Ecks", "\\general_button_a.dds\\"),
	new SettingT<GameSettingCollection>("sPS3Circle", "\\general_button_b.dds\\"),
	new SettingT<GameSettingCollection>("sPS3Square", "\\general_button_x.dds\\"),
	new SettingT<GameSettingCollection>("sPS3Triangle", "\\general_button_y.dds\\"),
	new SettingT<GameSettingCollection>("sPS3R1", "\\general_button_rb.dds\\"),
	new SettingT<GameSettingCollection>("sPS3L1", "\\general_button_lb.dds\\"),
	new SettingT<GameSettingCollection>("sPS3R2", "\\general_button_rt.dds\\"),
	new SettingT<GameSettingCollection>("sPS3L2", "\\general_button_lt.dds\\"),
	new SettingT<GameSettingCollection>("sPS3LThumbY", "the Left Stick Up / Down"),
	new SettingT<GameSettingCollection>("sPS3LThumbUp", "the Left Stick Up"),
	new SettingT<GameSettingCollection>("sPS3LThumbDown", "the Left Stick Down"),
	new SettingT<GameSettingCollection>("sPS3LThumbX", "the Left Stick Left / Right"),
	new SettingT<GameSettingCollection>("sPS3LThumbRight", "the Left Stick Right"),
	new SettingT<GameSettingCollection>("sPS3LThumbLeft", "the Left Stick Left"),
	new SettingT<GameSettingCollection>("sPS3RThumbY", "the Right Stick Up / Down"),
	new SettingT<GameSettingCollection>("sPS3RThumbUp", "the Right Stick Up"),
	new SettingT<GameSettingCollection>("sPS3RThumbDown", "the Right Stick Down"),
	new SettingT<GameSettingCollection>("sPS3RThumbX", "the Right Stick Left / Right"),
	new SettingT<GameSettingCollection>("sPS3RThumbRight", "the Right Stick Right"),
	new SettingT<GameSettingCollection>("sPS3RThumbLeft", "the Right Stick Left"),
	new SettingT<GameSettingCollection>("sPS3UnboundBtn", "<NOT BOUND>"),
	new SettingT<GameSettingCollection>("sPS3DPad", "Directional Button"), nullptr, nullptr};
psWeaponTypes_t psWeaponTypes = {
	new SettingT<GameSettingCollection>("sWeaponTypeHandToHandMelee", "Melee - Hand to Hand"),
	new SettingT<GameSettingCollection>("sWeaponTypeOneHandMelee", "Melee - One Hand"),
	new SettingT<GameSettingCollection>("sWeaponTypeTwoHandMelee", "Melee - Two Hand"),
	new SettingT<GameSettingCollection>("sWeaponTypeOneHandPistol", "Pistol"),
	new SettingT<GameSettingCollection>("sWeaponTypeOneHandPistolEnergy", "Energy Pistol"),
	new SettingT<GameSettingCollection>("sWeaponTypeTwoHandRifle", "Rifle"),
	new SettingT<GameSettingCollection>("sWeaponTypeTwoHandAutomatic", "Automatic"),
	new SettingT<GameSettingCollection>("sWeaponTypeTwoHandRifleEnergy", "Energy Rifle"),
	new SettingT<GameSettingCollection>("sWeaponTypeTwoHandHandle", "Handle Weapon"),
	new SettingT<GameSettingCollection>("sWeaponTypeTwoHandLauncher", "Launcher"),
	new SettingT<GameSettingCollection>("sWeaponTypeOneHandGrenade", "Grenade"),
	new SettingT<GameSettingCollection>("sWeaponTypeOneHandMine", "Mine"),
	new SettingT<GameSettingCollection>("sWeaponTypeOneHandThrown", "Melee - Thrown"), nullptr};
psTargetTypes_t psTargetTypes = {nullptr, new SettingT<GameSettingCollection>("sTargetTypeTake", "Take"),
	new SettingT<GameSettingCollection>("sTargetTypeOpen", "Open"),
	new SettingT<GameSettingCollection>("sTargetTypeSit", "Sit"),
	new SettingT<GameSettingCollection>("sTargetTypeActivate", "Activate"),
	new SettingT<GameSettingCollection>("sTargetTypeSleep", "Sleep"),
	new SettingT<GameSettingCollection>("sTargetTypeRead", "Read"),
	new SettingT<GameSettingCollection>("sTargetTypeTalk", "Talk"),
	new SettingT<GameSettingCollection>("sTargetTypeOpenDoor", "Open"),
	new SettingT<GameSettingCollection>("sTargetTypeHorse", "Ride"),
	new SettingT<GameSettingCollection>("sTargetTypeCrown", "Talk"),
	new SettingT<GameSettingCollection>("sTargetTypeVampire", "Feed/Talk"),
	new SettingT<GameSettingCollection>("sTargetTypeEquip", "Equip"),
	new SettingT<GameSettingCollection>("sTargetTypeUnequip", "Unequip"),
	new SettingT<GameSettingCollection>("sTargetTypeDrink", "Drink"),
	new SettingT<GameSettingCollection>("sTargetTypeEat", "Eat"),
	new SettingT<GameSettingCollection>("sTargetTypeRecharge", "Recharge"),
	new SettingT<GameSettingCollection>("sTargetTypeBrew", "Brew"),
	new SettingT<GameSettingCollection>("sTargetTypeApply", "Apply"),
	new SettingT<GameSettingCollection>("sTargetTypeRepair", "Repair")};

void MenuGameSettings::GetDamagedByPlayerBodyPartMessage(ActorValue::Index aiWhich, char* apBodyPartName,
	char* apBuffer, int32_t aiMaxBufferSize) {
	const char* v4;
	const char* v5;
	if (apBuffer) {
		*apBuffer = 0;
		if (((int32_t)aiWhich) >= 25) {
			if (((int32_t)aiWhich) <= 30) {
				v5 = (&sPartCrippledByPlayerMessage)->String();
				if (apBodyPartName) {
					BSsprintf(apBuffer, aiMaxBufferSize, "%s %s", apBodyPartName, v5);
				} else {
					BSsprintf(apBuffer, aiMaxBufferSize, "%s", v5);
				}
			} else {
				if (((int32_t)aiWhich) == 31) {
					v4 = (&::sFrenziedByPlayerMessage)->String();
					BSsprintf(apBuffer, aiMaxBufferSize, "%s", v4);
				}
			}
		}
	}
}
