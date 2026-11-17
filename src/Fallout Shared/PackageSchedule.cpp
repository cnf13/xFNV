#include "Fallout Shared/PackageSchedule.h"

#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

// The exe's copy is a shared one-dword swap (TESQuest::Load calls the same address).
void PACK_SCHED_DATA::Endian() {
	ENDIAN(iDuration, false);
}

PackageSchedule::PackageSchedule() {
	PSData.eMonth = (-1);
	PSData.eDayOfWeek = (-1);
	PSData.cDate = 0;
	PSData.cTime = (-1);
	PSData.iDuration = 0;
}

PackageSchedule::PackageSchedule(const PACK_SCHED_DATA* apPSData) : PSData(*apPSData) {}

PackageSchedule::~PackageSchedule() {}

// The date, time and duration getters and the duration setter the exe calls are shared one-field accessors
// (0x678CE0, 0x86A460, 0x726070, 0x6ECD40).
void PackageSchedule::Copy(PackageSchedule* apCopy) {
	if (apCopy) {
		SetStartDayOfWeek(apCopy->GetStartDayOfWeek());
		SetStartMonth(static_cast<PSMONTH>(apCopy->PSData.eMonth));
		SetStartDate(apCopy->PSData.cDate);
		SetStartTime(apCopy->GetStartTime());
		PSData.iDuration = apCopy->PSData.iDuration;
	}
}

void PackageSchedule::SetStartDayOfWeek(PSDAYOFWEEK aeWeekday) {
	PSData.eDayOfWeek = static_cast<int8_t>(aeWeekday);
}

char PackageSchedule::GetStartTime() {
	return PSData.cTime;
}

// hkEnum<hkpWorldObject::BroadPhaseType, int8_t>::operator= shares this one-byte store.
void PackageSchedule::SetStartMonth(PSMONTH aeMonth) {
	PSData.eMonth = static_cast<int8_t>(aeMonth);
}

void PackageSchedule::SetStartDate(char acDate) {
	PSData.cDate = acDate;
}

void PackageSchedule::SetStartTime(char acTime) {
	PSData.cTime = acTime;
}

void PackageSchedule::Save() {
	if (TESForm::QEndianSwapOnSave()) {
		PSData.Endian();
	}
	TESForm::__AddChunkData(PSDT_ID, &PSData, sizeof(PSData));
	if (TESForm::QEndianSwapOnSave()) {
		PSData.Endian();
	}
}

// The exe compares each field through its shared one-field getter, the bytes sign-extended (0x678CA0, 0x678CE0,
// 0x86A460) and the duration as a dword (0x726070); any difference returns true.
bool PackageSchedule::Compare(PackageSchedule* apCompare) {
	if (!apCompare) {
		return true;
	}
	if (GetStartDayOfWeek() != apCompare->GetStartDayOfWeek()) {
		return true;
	}
	if (PSData.eMonth != apCompare->PSData.eMonth) {
		return true;
	}
	if (PSData.cDate != apCompare->PSData.cDate) {
		return true;
	}
	if (GetStartTime() != apCompare->GetStartTime()) {
		return true;
	}
	return PSData.iDuration != apCompare->PSData.iDuration;
}
