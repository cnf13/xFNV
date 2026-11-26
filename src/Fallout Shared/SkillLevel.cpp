#include "Fallout Shared/SkillLevel.h"

#include "BSMain/Setting.h"
#include "Fallout Shared/Settings/XGameSetting.h"

// The skill level names and the skill values each level starts at.
static SettingT<GameSettingCollection> sSkillLevelNovice("sSkillLevelNovice", "Novice");
static SettingT<GameSettingCollection> sSkillLevelApprentice("sSkillLevelApprentice", "Apprentice");
static SettingT<GameSettingCollection> sSkillLevelJourneyman("sSkillLevelJourneyman", "Journeyman");
static SettingT<GameSettingCollection> sSkillLevelExpert("sSkillLevelExpert", "Expert");
static SettingT<GameSettingCollection> sSkillLevelMaster("sSkillLevelMaster", "Master");
static SettingT<GameSettingCollection> iSkillApprenticeMin("iSkillApprenticeMin", 25);
static SettingT<GameSettingCollection> iSkillJourneymanMin("iSkillJourneymanMin", 50);
static SettingT<GameSettingCollection> iSkillExpertMin("iSkillExpertMin", 75);
static SettingT<GameSettingCollection> iSkillMasterMin("iSkillMasterMin", 100);
