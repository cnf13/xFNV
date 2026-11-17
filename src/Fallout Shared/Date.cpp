#include "Fallout Shared/Date.h"

#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/Calendar.h"

Date::Date() : sDate(0), sYear(0) {}

Date::Date(uint16_t ausDay, uint16_t ausMonth, uint16_t a3) : sDate(Date::CalculateDate(ausDay, ausMonth)), sYear(a3) {}

Date::Date(const Date& aDate) : sDate(aDate.sDate), sYear(aDate.sYear) {}

uint16_t Date::GetYear() const {
    return sYear;
}

uint16_t Date::GetDate() const {
    return sDate;
}

// The day within the month: the low 16 bits of the high 32 bits of the full date.
uint16_t Date::GetDay() const {
    return (uint16_t)(Date::CalculateFullDateRaw(sDate) >> 32);
}

CALENDAR_MONTH Date::GetMonth() const {
    return static_cast<CALENDAR_MONTH>(Date::CalculateFullDate(sDate));
}

// Copies the two members one by one; `*this = aDate` here would call this function again.
Date& Date::operator=(const Date& aDate) {
    sDate = aDate.sDate;
    sYear = aDate.sYear;
    return *this;
}

int32_t Date::CalculateFullDate(uint16_t asDate) {
    uint16_t v2 = asDate;
    uint16_t v4 = 1;
    if (asDate > static_cast<int32_t>(Date::sDaysInYear)) {
        v2 = Date::sDaysInYear - 1;
    }
    while (v4 < 12u && v2 >= static_cast<int32_t>(Date::DaysBeforeMonth[v4])) {
        ++v4;
    }
    int16_t v3 = v2 - Date::DaysBeforeMonth[static_cast<uint16_t>(v4 - 1)];
    return static_cast<uint16_t>(v4 - 1);
}

uint64_t Date::CalculateFullDateRaw(uint16_t asDate) {
    uint16_t usDate = asDate;
    uint16_t usMonth = 1;
    if (asDate > static_cast<int32_t>(Date::sDaysInYear)) {
        usDate = Date::sDaysInYear - 1;
    }
    while (usMonth < 12u && usDate >= static_cast<int32_t>(Date::DaysBeforeMonth[usMonth])) {
        ++usMonth;
    }
    --usMonth;
    uint16_t usDay = usDate - Date::DaysBeforeMonth[usMonth];
    return (static_cast<uint64_t>(usDay) << 32) | usMonth;
}

uint16_t Date::GetDaysInMonth(CALENDAR_MONTH aIndex) {
    if (((int32_t)aIndex) < 12) {
        return Date::DaysInMonth[(int32_t)aIndex];
    }
    else {
        return (uint16_t)0;
    }
}

uint16_t Date::CalculateDate(uint16_t asDay, uint16_t asMonth) {
    int32_t i;
    if (((uint32_t)((int32_t)asMonth)) > 0xCu) {
        asMonth = (uint16_t)11;
    }
    if (((int32_t)asDay) >= ((int32_t)Date::DaysInMonth[asMonth])) {
        asDay = (uint16_t)(Date::DaysInMonth[asMonth] - 1);
    }
    uint16_t v5 = asDay;
    for (i = 0; i < ((int32_t)asMonth); ++i) {
        v5 += Date::DaysInMonth[i];
    }
    return v5;
}

void Date::operator delete(void* apMemory) {
    ourFree(apMemory);
}
