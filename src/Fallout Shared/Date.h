#pragma once

#include "Fallout/Misc/Calendar.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A month and a day within it (PDB struct).
struct FullDate {
	CALENDAR_MONTH month;
	uint16_t sDay;
};

// A calendar date: the day within the year and the year.
class Date {
public:
	// The number of days in each month.
	using DaysInMonthArray = uint16_t[12];
	using DaysBeforeMonth_t = uint16_t[12];
	// The holidays' dates (PDB static member; the exe's code never reads it).
	using HolidaysArray = FullDate[15];

	Date();
	Date(uint16_t ausDay, uint16_t ausMonth, uint16_t a3);
	Date(const Date& aDate);
	~Date() {}

	uint16_t GetYear() const;
	uint16_t GetDate() const;
	// The day within the month.
	uint16_t GetDay() const;
	CALENDAR_MONTH GetMonth() const;
	// Copies both words of aDate.
	Date& operator=(const Date& aDate);

	// The full date of the day-within-year and year pair asDate.
	static int32_t CalculateFullDate(uint16_t asDate);
	// The same call typed as the 8-byte FullDate the exe returns in edx:eax: the month in the low dword, the day
	// within the month in the low word of the high dword (cdecl, ret 0). A class return would pass a hidden result
	// pointer, so the pair is spelled as one 64-bit value; its forwarder stays for GetDay, which reads the day.
	static uint64_t CalculateFullDateRaw(uint16_t asDate);
	static uint16_t GetDaysInMonth(CALENDAR_MONTH aIndex);
	static uint16_t CalculateDate(uint16_t asDay, uint16_t asMonth);
	static void operator delete(void* apMemory);

	uint16_t sDate;
	uint16_t sYear;

	static inline DaysInMonthArray DaysInMonth{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	static inline DaysBeforeMonth_t DaysBeforeMonth{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	static inline uint16_t sDaysInYear = 365;
	static inline HolidaysArray Holidays{{MONTH_JANUARY, 1}, {MONTH_JANUARY, 15}, {MONTH_FEBRUARY, 16},
		{MONTH_FEBRUARY, 7}, {MONTH_APRIL, 28}, {MONTH_MAY, 7}, {MONTH_JUNE, 16}, {MONTH_JULY, 20}, {MONTH_AUGUST, 27},
		{MONTH_SEPTEMBER, 3}, {MONTH_OCTOBER, 13}, {MONTH_OCTOBER, 30}, {MONTH_NOVEMBER, 20}, {MONTH_DECEMBER, 15},
		{MONTH_DECEMBER, 30}};
};
