#include "Fallout Shared/PackageSchedule.h"

#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

void PACK_SCHED_DATA::Endian() {
    BSEndian(iDuration, false);
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

// Compares each field; any difference returns true.
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
