#include "Fallout Shared/Date.h"

#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/Calendar.h"

Date::Date() : sDate(0), sYear(0) {}

Date::Date(uint16_t ausDay, uint16_t ausMonth, uint16_t ausYear)
  : sDate(Date::CalculateDate(ausDay, ausMonth)), sYear(ausYear) {}

Date::Date(const Date& aDate) : sDate(aDate.sDate), sYear(aDate.sYear) {}

uint16_t Date::GetYear() const {
    return sYear;
}

uint16_t Date::GetDate() const {
    return sDate;
}

uint16_t Date::GetDay() const {
    return Date::CalculateFullDate(sDate).sDay;
}

CALENDAR_MONTH Date::GetMonth() const {
    return Date::CalculateFullDate(sDate).month;
}

// Copies the two members one by one; `*this = aDate` here would call this function again.
Date& Date::operator=(const Date& aDate) {
    sDate = aDate.sDate;
    sYear = aDate.sYear;
    return *this;
}

FullDate Date::CalculateFullDate(uint16_t asDate) {
    uint16_t usDate = asDate;
    uint16_t usMonth = 1;
    if (asDate > static_cast<int32_t>(Date::sDaysInYear)) {
        usDate = Date::sDaysInYear - 1;
    }
    while (usMonth < 12u && usDate >= static_cast<int32_t>(Date::DaysBeforeMonth[usMonth])) {
        ++usMonth;
    }
    --usMonth;
    return {static_cast<CALENDAR_MONTH>(usMonth), static_cast<uint16_t>(usDate - Date::DaysBeforeMonth[usMonth])};
}

uint16_t Date::GetDaysInMonth(CALENDAR_MONTH aIndex) {
    if (aIndex < MONTH_COUNT) {
        return Date::DaysInMonth[aIndex];
    }

    return 0;
}

uint16_t Date::CalculateDate(uint16_t asDay, uint16_t asMonth) {
    if (asMonth > 0xCu) {
        asMonth = MONTH_DECEMBER;
    }
    if (asDay >= Date::DaysInMonth[asMonth]) {
        asDay = static_cast<uint16_t>(Date::DaysInMonth[asMonth] - 1);
    }
    uint16_t usDate = asDay;
    for (int32_t i = 0; i < asMonth; ++i) {
        usDate += Date::DaysInMonth[i];
    }
    return usDate;
}

void Date::operator delete(void* apMemory) {
    ourFree(apMemory);
}
