#include "Fallout/AI/Combat/CombatTimer.h"

#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

#include <limits>

void CombatTimer::Reset(float afTargetTime) {
    fStartTime = CombatTimer::GetTimer();
    fTargetTime = afTargetTime;
}

bool CombatTimer::Check() {
    return fTargetTime < CombatTimer::GetTimer() - fStartTime;
}

void CombatTimer::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&fStartTime, sizeof(fStartTime));
    apLoadGameBuffer->LoadData(&fTargetTime, sizeof(fTargetTime));
    if (fStartTime < std::numeric_limits<float>::max() && fStartTime > -std::numeric_limits<float>::max()) {
        fStartTime = fStartTime + CombatTimer::fTimer;
    }
}

void CombatTimer::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    float fSavedTime = fStartTime;
    if (fStartTime < std::numeric_limits<float>::max() && fStartTime > -std::numeric_limits<float>::max()) {
        fSavedTime = fSavedTime - CombatTimer::fTimer;
    }
    apSaveGameBuffer->SaveDataEndian(&fSavedTime, sizeof(fSavedTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fTargetTime, sizeof(fTargetTime), 0);
}

void CombatTimer::ForceTrigger() {
    fTargetTime = -0x1p+0f;
}

bool CombatTimer::IsSet() {
    return fTargetTime != 0.0;
}

bool CombatTimer::IsForced() {
    return fTargetTime < 0.0;
}

float CombatTimer::ElapsedTime() {
    return CombatTimer::GetTimer() - fStartTime;
}

void CombatTimer::SetStartTime(float afStartTime) {
    fStartTime = afStartTime;
}

float CombatTimer::GetTimer() {
    return CombatTimer::fTimer;
}

void CombatTimer::UpdateTimer(float afElapsedTime) {
    CombatTimer::fTimer = CombatTimer::fTimer + afElapsedTime;
}

CombatTimeStamp::CombatTimeStamp() : fTimeStamp(0.0f) {}

CombatTimeStamp::CombatTimeStamp(float afValue) : fTimeStamp(afValue) {}

void CombatTimeStamp::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&fTimeStamp, sizeof(fTimeStamp));
    if (fTimeStamp < std::numeric_limits<float>::max() && fTimeStamp > -std::numeric_limits<float>::max()) {
        fTimeStamp = CombatTimer::GetTimer() + fTimeStamp;
    }
}

// Only a finite stamp is made relative: it is compared against FLT_MAX both ways.
void CombatTimeStamp::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    float fSavedStamp = fTimeStamp;
    if (fTimeStamp < std::numeric_limits<float>::max() && fTimeStamp > -std::numeric_limits<float>::max()) {
        fSavedStamp = fSavedStamp - CombatTimer::GetTimer();
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fSavedStamp), sizeof(fSavedStamp), 0);
}

float CombatTimeStamp::ElapsedTime() {
    return CombatTimer::GetTimer() - fTimeStamp;
}

float CombatTimeStamp::DisplayElapsedTime() {
    if (CombatTimer::GetTimer() - fTimeStamp > 1000.0) {
        return 1000.0f;
    }

    return CombatTimer::GetTimer() - fTimeStamp;
}

CombatTimeStamp::operator float() const {
    return fTimeStamp;
}
