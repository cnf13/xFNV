#pragma once

#include "BSMain/Setting.h"

class TESGlobal;

template <class Char>
class BSStringT;

// The months of the game calendar.
enum CALENDAR_MONTH : int32_t {
    MONTH_NONE = -1,
    MONTH_JANUARY = 0,
    MONTH_FEBRUARY = 1,
    MONTH_MARCH = 2,
    MONTH_APRIL = 3,
    MONTH_MAY = 4,
    MONTH_JUNE = 5,
    MONTH_JULY = 6,
    MONTH_AUGUST = 7,
    MONTH_SEPTEMBER = 8,
    MONTH_OCTOBER = 9,
    MONTH_NOVEMBER = 10,
    MONTH_DECEMBER = 11,
    MONTH_COUNT = 12,
};

enum SEASON : int32_t {
    SEASON_NONE = -1,
    SEASON_SPRING = 0,
    SEASON_SUMMER = 1,
    SEASON_FALL = 2,
    SEASON_WINTER = 3,
    SEASON_COUNT = 4,
};

// The days of the week, as returned by GetDayOfTheWeek.
enum DAYS_OF_THE_WEEK : int32_t {
    DAY_NONE = -1,
    DAY_SUNDAY = 0,
    DAY_MONDAY = 1,
    DAY_TUESDAY = 2,
    DAY_WEDNESDAY = 3,
    DAY_THURSDAY = 4,
    DAY_FRIDAY = 5,
    DAY_SATURDAY = 6,
    DAY_COUNT = 7,
};

// The game calendar: the date and time globals and the count of midnights passed.
class Calendar {
public:
    Calendar();

    // Clears the date/time global pointers during calendar initialization.
    void ClearPointers();
    // Looks up the date and time globals and resets them. Called on the existing calendar when the data files are
    // compiled.
    void InitGlobals();
    // The current hour.
    float GetHour();
    // The day of the week, 0 to 6, from the days passed.
    DAYS_OF_THE_WEEK GetDayOfTheWeek();
    // The whole hours passed: the days passed times 24, floored.
    uint32_t GetHoursPassed();
    // The current year.
    uint32_t GetYear();
    // The current month.
    CALENDAR_MONTH GetMonth();
    // The current day of the month.
    char GetDay();
    // The whole days passed (1 without the global).
    uint32_t GetDaysPassed();
    // The date as "MM.DD.YY", returned by value through apResult.
    BSStringT<char> GetDateString();
    // The game time scale.
    float GetTimeScale();
    // Advances the calendar by afSeconds of game time.
    void Update(float afSeconds);
    SEASON GetSeason();
    float GetMinutesPassed();
    void SetGameLoaded();
    const char* GetDayOfWeekString();

    TESGlobal* pGameYear;
    TESGlobal* pGameMonth;
    TESGlobal* pGameDay;
    TESGlobal* pGameHour;
    TESGlobal* pGameDaysPassed;
    TESGlobal* pTimeScale;
    uint32_t iMidnightsPassed;
    // Set when a game is loaded; Update then recomputes the days passed and clears it. Tests the whole byte.
    uint8_t bGameLoaded;
};

// The game's calendar, a static object.
extern Calendar appCalendar;

// The settings holding the weekday names (indexed by the day of the week), the holiday names and the season names;
// their contents are TESCalendar.cpp's settings.
using DayOfWeekSettingTable = SettingT<GameSettingCollection>* [7];
extern DayOfWeekSettingTable pGSDay;
using pGSHoliday_t = SettingT<GameSettingCollection>* [15];
extern pGSHoliday_t pGSHoliday;
using pGSSeason_t = SettingT<GameSettingCollection>* [4];
extern pGSSeason_t pGSSeason;
