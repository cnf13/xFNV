#pragma once

#include "Reconstruction.hpp"

// A schedule's start month (PDB values); -1 is any month. The exe keeps it in a byte (PACK_SCHED_DATA::eMonth).
enum PSMONTH : int32_t {
	PSM_NONE = -1,
	PSM_MORNINGSTAR = 0,
	PSM_SUNSDAWN = 1,
	PSM_FIRSTSEED = 2,
	PSM_RAINSHAND = 3,
	PSM_SECONDSEED = 4,
	PSM_MIDYEAR = 5,
	PSM_SUNSHEIGHT = 6,
	PSM_LASTSEED = 7,
	PSM_HEARTFIRE = 8,
	PSM_FROSTFALL = 9,
	PSM_SUNSDUSK = 10,
	PSM_EVENINGSTAR = 11,
	PSM_SPRING = 12,
	PSM_SUMMER = 13,
	PSM_FALL = 14,
	PSM_WINTER = 15,
	PSM_COUNT = 16,
};

// A schedule's start weekday or set of weekdays (PDB values); -1 is any day. The exe keeps it in a byte
// (PACK_SCHED_DATA::eDayOfWeek) and hands it out sign-extended (PackageSchedule::GetStartDayOfWeek, 0x678CCA).
enum PSDAYOFWEEK : int32_t {
	PSD_NONE = -1,
	PSD_SUNDAY = 0,
	PSD_MONDAY = 1,
	PSD_TUESDAY = 2,
	PSD_WEDNESDAY = 3,
	PSD_THURSDAY = 4,
	PSD_FRIDAY = 5,
	PSD_SATURDAY = 6,
	PSD_WEEKDAY = 7,
	PSD_WEEKEND = 8,
	PSD_MWF = 9,
	PSD_TT = 10,
	PSD_COUNT = 11,
};

// When a package may start and how long it may run for. A field of -1 means "any" month, weekday, date or hour.
struct PACK_SCHED_DATA {
	// Byte-swaps the duration of a schedule read from a big-endian file; the exe's copy is a shared one-dword swap
	// (thiscall, 0x60CE80).
	void Endian();

	int8_t eMonth;
	int8_t eDayOfWeek;
	int8_t cDate;
	int8_t cTime;
	int32_t iDuration;
};

// The schedule of a package; its owner keeps it by value, so it is copied around as one value.
class PackageSchedule {
public:
	// Any month, weekday and time, no duration.
	PackageSchedule();
	// A copy of a schedule read from a file.
	PackageSchedule(const PACK_SCHED_DATA* apPSData);
	// The destructor is the engine's shared empty thiscall function (0x483710); the exe calls
	// it where a schedule on the stack goes out of scope.
	~PackageSchedule();

	// Copies another schedule through its getters and setters.
	void Copy(PackageSchedule* apCopy);
	// The weekday byte, sign-extended.
	PSDAYOFWEEK GetStartDayOfWeek();
	// Sets the weekday byte.
	void SetStartDayOfWeek(PSDAYOFWEEK aeWeekday);
	// PSData.cTime (thiscall, al, 0x86A460).
	char GetStartTime();
	// PSData.iDuration.
	int32_t GetDuration() { return PSData.iDuration; }
	// Sets the month byte.
	void SetStartMonth(PSMONTH aeMonth);
	void SetStartDate(char acDate);
	void SetStartTime(char acTime);
	// Writes the schedule as its PSDT chunk, byte-swapped for a big-endian save.
	void Save();
	// True when the schedules differ, or apCompare is null.
	bool Compare(PackageSchedule* apCompare);

	PACK_SCHED_DATA PSData;
};
