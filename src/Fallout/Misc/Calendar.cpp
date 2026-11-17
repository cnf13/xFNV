#include "Fallout/Misc/Calendar.h"

#include "BSMain/Setting.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"

// Update's function-local static: the game hour of the last update,
// initialised on the first call from that call's hour.
static float fLastUpdateHour;
// The guard of fLastUpdateHour (bit 0).
static uint32_t uiLastUpdateHourGuard;
// 24 bytes that no code reads or writes.
using CalendarUnusedBytes = uint8_t[0x18];
static CalendarUnusedBytes data_11DE7EC{};

Calendar appCalendar;

Calendar::Calendar()
	: pGameYear(nullptr), pGameMonth(nullptr), pGameDay(nullptr), pGameHour(nullptr), pGameDaysPassed(nullptr),
	  pTimeScale(nullptr), iMidnightsPassed(0) {
	SetGameLoaded();
	ClearPointers();
}

// Clears the date/time global pointers before initialization.
void Calendar::ClearPointers() {
	pGameYear = nullptr;
	pGameMonth = nullptr;
	pGameDay = nullptr;
	pGameHour = nullptr;
	pGameDaysPassed = nullptr;
	pTimeScale = nullptr;
}

void Calendar::InitGlobals() {
	pGameYear = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(53u));
	pGameMonth = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(54u));
	pGameDay = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(55u));
	pGameHour = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(56u));
	pGameDaysPassed = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(57u));
	pTimeScale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
	float Value = pGameHour->GetValue();
	float v4 = pGameDaysPassed->GetValue();
	float afValue = Value / 24.0 + v4;
	pGameDaysPassed->SetValue(afValue);
	iMidnightsPassed = 0;
	SetGameLoaded();
}

float Calendar::GetHour() {
	if (pGameHour) {
		return (float)((float)pGameHour->TESGlobal::GetValue());
	} else {
		return (float)(12.0f);
	}
}

DAYS_OF_THE_WEEK Calendar::GetDayOfTheWeek() {
	return static_cast<DAYS_OF_THE_WEEK>(static_cast<uint32_t>(GetDaysPassed()) % 7u);
}

// Rounds the days and the hours to float, floors them and truncates through a 64-bit integer.
uint32_t Calendar::GetHoursPassed() {
	float fDaysPassed = pGameDaysPassed ? pGameDaysPassed->GetValue() : 1.0f;
	float fHoursPassed = fDaysPassed * 24.0;
	return static_cast<uint32_t>(static_cast<int64_t>(std::floor(fHoursPassed)));
}

// The global's value is stored as a float and truncated to an integer.
uint32_t Calendar::GetYear() {
	if (pGameYear) {
		return (uint32_t)((int32_t)((float)pGameYear->TESGlobal::GetValue()));
	} else {
		return (uint32_t)((int32_t)((float)77.0f));
	}
}

// The value is truncated to an integer and its low byte is sign-extended.
CALENDAR_MONTH Calendar::GetMonth() {
	if (pGameMonth) {
		return static_cast<CALENDAR_MONTH>((int8_t)((int32_t)pGameMonth->TESGlobal::GetValue()));
	} else {
		return MONTH_AUGUST;
	}
}

char Calendar::GetDay() {
	if (pGameDay) {
		return (int8_t)((int32_t)((float)pGameDay->TESGlobal::GetValue()));
	} else {
		return (int8_t)((int32_t)((float)17.0f));
	}
}

// Truncates through a 64-bit integer conversion.
uint32_t Calendar::GetDaysPassed() {
	float fDaysPassed = pGameDaysPassed ? pGameDaysPassed->GetValue() : 1.0f;
	return static_cast<uint32_t>(static_cast<int64_t>(fDaysPassed));
}

BSStringT<char>* Calendar::GetDateString(BSStringT<char>* apResult) {
	alignas(BSStringT<char>) uint8_t aTextMemory[sizeof(BSStringT<char>)];
	BSStringT<char>* pText = ::new (aTextMemory) BSStringT<char>;
	uint32_t uiYear = GetYear() % 100;
	char cDay = GetDay();
	CALENDAR_MONTH eMonth = GetMonth();
	pText->SPrintF("%02d.%02d.%02d", eMonth + 1, static_cast<int32_t>(cDay), uiYear);
	// SWEEP-INLINE 0x4047f0
	::new (apResult) BSStringT<char>(*pText);
	// SWEEP-INLINE 0x4037d0
	pText->~BSStringT<char>();
	return apResult;
}

float Calendar::GetTimeScale() {
	return pTimeScale->TESGlobal::GetValue();
}

// Advances the clock by afSeconds of real time at the time scale. The days passed are recomputed from the whole days
// and the hour once a game is loaded or more than an hour has gone by since the last recompute; past midnight the day,
// month and year roll over. The products and quotients are computed in double and rounded to float at each store (equal
// at 53- or 64-bit precision control, not under 24-bit precision control).
void Calendar::Update(float afSeconds) {
	float fhourfraction = static_cast<double>(pTimeScale->GetValue()) * afSeconds / 3600.0;
	float fHour = pGameHour->GetValue() + fhourfraction;
	if (!(uiLastUpdateHourGuard & 1)) {
		uiLastUpdateHourGuard |= 1;
		fLastUpdateHour = fHour;
	}
	if (bGameLoaded || fHour > fLastUpdateHour + 1.0) {
		float fDaysPassed = static_cast<int32_t>(pGameDaysPassed->GetValue()) + fHour / 24.0;
		pGameDaysPassed->SetValue(fDaysPassed);
		bGameLoaded = false;
	}
	if (fHour > 24.0) {
		float fDay = pGameDay->GetValue();
		float fMonth = pGameMonth->GetValue();
		float fYear = pGameYear->GetValue();
		// The month is truncated and narrowed to a signed byte before the call.
		uint16_t sDaysInMonth =
			Date::GetDaysInMonth(static_cast<CALENDAR_MONTH>(static_cast<int8_t>(static_cast<int32_t>(fMonth))));
		while (fHour > 24.0) {
			fHour = fHour - 24.0;
			fDay = fDay + 1.0;
		}
		if (fDay > static_cast<int32_t>(sDaysInMonth)) {
			fDay = fDay - static_cast<int32_t>(sDaysInMonth);
			fMonth = fMonth + 1.0;
			if (fMonth >= 12.0) {
				fMonth = fMonth - 12.0;
				fYear = fYear + 1.0;
				pGameYear->SetValue(fYear);
			}
			pGameMonth->SetValue(fMonth);
		}
		pGameDay->SetValue(fDay);
		++iMidnightsPassed;
	}
	float fDaysPassed = pGameDaysPassed->GetValue() + fhourfraction / 24.0;
	pGameDaysPassed->SetValue(fDaysPassed);
	fLastUpdateHour = fHour;
	pGameHour->SetValue(fHour);
}

SEASON Calendar::GetSeason() {
	int32_t eEason = 0;
	switch ((int32_t)GetMonth()) {
	case 0:
	case 1:
	case 11: {
		eEason = 3;
		break;
	}
	case 2:
	case 3:
	case 4: {
		eEason = 0;
		break;
	}
	case 5:
	case 6:
	case 7: {
		eEason = 1;
		break;
	}
	case 8:
	case 9:
	case 10: {
		eEason = 2;
		break;
	}
	default: {
		return static_cast<SEASON>(eEason);
	}
	}
	return static_cast<SEASON>(eEason);
}

float Calendar::GetMinutesPassed() {
	float fDaysPassed;
	if (pGameDaysPassed) {
		fDaysPassed = (float)pGameDaysPassed->TESGlobal::GetValue();
	} else {
		fDaysPassed = (float)1.0;
	}
	return (float)((fDaysPassed * 24.0) * 60.0);
}

void Calendar::SetGameLoaded() {
	bGameLoaded = 1;
}

const char* Calendar::GetDayOfWeekString() {
	DAYS_OF_THE_WEEK DayOfTheWeek = GetDayOfTheWeek();
	if (DayOfTheWeek == DAY_NONE || DayOfTheWeek >= DAY_COUNT) {
		return "Bad Day";
	} else {
		return pGSDay[DayOfTheWeek]->String();
	}
}
