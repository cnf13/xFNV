#include "Fallout/AI/Combat/CombatTimer.h"

#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

void CombatTimer::Reset(float afTargetTime) {
    fStartTime = CombatTimer::GetTimer();
    fTargetTime = afTargetTime;
}

bool CombatTimer::Check() {
    return (bool)((uint8_t)(fTargetTime < (CombatTimer::GetTimer() - fStartTime)));
}

void CombatTimer::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 4u);
    apLoadGameBuffer->LoadData(&fTargetTime, 4u);
    if (fStartTime < 3.4028235e+38f && fStartTime > -3.4028235e+38f) {
        fStartTime = fStartTime + CombatTimer::fTimer;
    }
}

void CombatTimer::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    float apData;
    apData = fStartTime;
    if (fStartTime < 3.4028235e+38f && fStartTime > -3.4028235e+38f) {
        apData = apData - CombatTimer::fTimer;
    }
    apSaveGameBuffer->SaveDataEndian(&apData, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fTargetTime, 4u, 0);
}

void CombatTimer::ForceTrigger() {
    fTargetTime = (-0x1p+0f);
}

bool CombatTimer::IsSet() {
    return (bool)((uint8_t)(fTargetTime != 0.0));
}

bool CombatTimer::IsForced() {
    return (bool)((uint8_t)(fTargetTime < 0.0));
}

float CombatTimer::ElapsedTime() {
    return (CombatTimer::GetTimer() - fStartTime);
}

void CombatTimer::SetStartTime(float a1) {
    fStartTime = a1;
}

float CombatTimer::GetTimer() {
    return CombatTimer::fTimer;
}

void CombatTimer::UpdateTimer(float afElapsedTime) {
    CombatTimer::fTimer = (CombatTimer::fTimer + afElapsedTime);
}

CombatTimeStamp::CombatTimeStamp() : fTimeStamp(0.0f) {}

CombatTimeStamp::CombatTimeStamp(float afValue) : fTimeStamp(afValue) {}

void CombatTimeStamp::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(this)), 4u);
    if ((fTimeStamp < 0x1.fffffe0000000p+127f) && (fTimeStamp > (-0x1.fffffe0000000p+127f))) {
        fTimeStamp = (CombatTimer::GetTimer() + fTimeStamp);
    }
}

// Only a finite stamp is made relative: it is compared against FLT_MAX both ways.
void CombatTimeStamp::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    float fSavedStamp = fTimeStamp;
    if (fTimeStamp < 3.4028235e+38f && fTimeStamp > -3.4028235e+38f) {
        fSavedStamp = fSavedStamp - CombatTimer::GetTimer();
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fSavedStamp), 4, 0);
}

float CombatTimeStamp::ElapsedTime() {
    return (CombatTimer::GetTimer() - fTimeStamp);
}

float CombatTimeStamp::DisplayElapsedTime() {
    if (!((CombatTimer::GetTimer() - fTimeStamp) > 1000.0)) {
        return (CombatTimer::GetTimer() - fTimeStamp);
    }
    else {
        return (float)1000.0;
    }
}

CombatTimeStamp::operator float() const {
    return fTimeStamp;
}
