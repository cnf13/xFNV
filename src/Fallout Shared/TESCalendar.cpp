#include "BSMain/Setting.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout/Misc/Calendar.h"

// The calendar's game settings: month, day, holiday and season names.
static SettingT<GameSettingCollection> sMonthJanuary("sMonthJanuary", "January");
static SettingT<GameSettingCollection> sMonthFebruary("sMonthFebruary", "February");
static SettingT<GameSettingCollection> sMonthMarch("sMonthMarch", "March");
static SettingT<GameSettingCollection> sMonthApril("sMonthApril", "April");
static SettingT<GameSettingCollection> sMonthMay("sMonthMay", "May");
static SettingT<GameSettingCollection> sMonthJune("sMonthJune", "June");
static SettingT<GameSettingCollection> sMonthJuly("sMonthJuly", "July");
static SettingT<GameSettingCollection> sMonthAugust("sMonthAugust", "August");
static SettingT<GameSettingCollection> sMonthSeptember("sMonthSeptember", "September");
static SettingT<GameSettingCollection> sMonthOctober("sMonthOctober", "October");
static SettingT<GameSettingCollection> sMonthNovember("sMonthNovember", "November");
static SettingT<GameSettingCollection> sMonthDecember("sMonthDecember", "December");
static SettingT<GameSettingCollection> sDaySunday("sDaySunday", "Sunday");
static SettingT<GameSettingCollection> sDayMonday("sDayMonday", "Monday");
static SettingT<GameSettingCollection> sDayTuesday("sDayTuesday", "Tuesday");
static SettingT<GameSettingCollection> sDayWednesday("sDayWednesday", "Wednesday");
static SettingT<GameSettingCollection> sDayThursday("sDayThursday", "Thursday");
static SettingT<GameSettingCollection> sDayFriday("sDayFriday", "Friday");
static SettingT<GameSettingCollection> sDaySaturday("sDaySaturday", "Saturday");
static SettingT<GameSettingCollection> sHolidayNewLifeFestival("sHolidayNewLifeFestival", "New Life Festival");
static SettingT<GameSettingCollection> sHolidaySouthWindsPrayer("sHolidaySouthWindsPrayer", "South Wind's Prayer");
static SettingT<GameSettingCollection> sHolidayFirstPlanting("sHolidayFirstPlanting", "First Planting");
static SettingT<GameSettingCollection> sHolidayJestersDay("sHolidayJestersDay", "Jester's Day");
static SettingT<GameSettingCollection> sHolidaySecondPlanting("sHolidaySecondPlanting", "Second Planting");
static SettingT<GameSettingCollection> sHolidayMidYearCelebration("sHolidayMidYearCelebration", "Mid Year Celebration");
static SettingT<GameSettingCollection> sHolidayMerchantsFestival("sHolidayMerchantsFestival", "Merchant's Festival");
static SettingT<GameSettingCollection> sHolidaySunsRest("sHolidaySunsRest", "Sun's Rest");
static SettingT<GameSettingCollection> sHolidayHarvestsEnd("sHolidayHarvestsEnd", "Harvest's End");
static SettingT<GameSettingCollection> sHolidayTalesAndTallows("sHolidayTalesAndTallows", "Tales and Tallows");
static SettingT<GameSettingCollection> sHolidayWitchesFestival("sHolidayWitchesFestival", "Witches' Festival");
static SettingT<GameSettingCollection> sHolidayEmperorsBirthday("sHolidayEmperorsBirthday", "Emperor's Birthday");
static SettingT<GameSettingCollection> sHolidayWarriorsFestival("sHolidayWarriorsFestival", "Warrior's Festival");
static SettingT<GameSettingCollection> sHolidayNorthWindsPrayer("sHolidayNorthWindsPrayer", "North Wind's Prayer");
static SettingT<GameSettingCollection> sHolidayOldLifeFestival("sHolidayOldLifeFestival", "Old Life Festival");
static SettingT<GameSettingCollection> sSeasonSpring("sSeasonSpring", "Spring");
static SettingT<GameSettingCollection> sSeasonSummer("sSeasonSummer", "Summer");
static SettingT<GameSettingCollection> sSeasonFall("sSeasonFall", "Fall");
static SettingT<GameSettingCollection> sSeasonWinter("sSeasonWinter", "Winter");

// The tables of the names above, by month, day of the week, holiday and season.
pGSMonth_t pGSMonth{&sMonthJanuary, &sMonthFebruary, &sMonthMarch, &sMonthApril, &sMonthMay, &sMonthJune, &sMonthJuly,
	&sMonthAugust, &sMonthSeptember, &sMonthOctober, &sMonthNovember, &sMonthDecember};
DayOfWeekSettingTable pGSDay{&sDaySunday, &sDayMonday, &sDayTuesday, &sDayWednesday, &sDayThursday, &sDayFriday,
	&sDaySaturday};
pGSHoliday_t pGSHoliday{&sHolidayNewLifeFestival, &sHolidaySouthWindsPrayer, &sHolidayFirstPlanting,
	&sHolidayJestersDay, &sHolidaySecondPlanting, &sHolidayMidYearCelebration, &sHolidayMerchantsFestival,
	&sHolidaySunsRest, &sHolidayHarvestsEnd, &sHolidayTalesAndTallows, &sHolidayWitchesFestival,
	&sHolidayEmperorsBirthday, &sHolidayWarriorsFestival, &sHolidayNorthWindsPrayer, &sHolidayOldLifeFestival};
pGSSeason_t pGSSeason{&sSeasonSpring, &sSeasonSummer, &sSeasonFall, &sSeasonWinter};
