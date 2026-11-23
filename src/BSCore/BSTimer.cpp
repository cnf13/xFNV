#include "BSCore/BSTimer.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSTimer appTimer;

float BSTimer::GetSeconds() const {
    return fDelta;
}

void BSTimer::SetGlobalTimeMultiplier(float afMult, bool abNow) {
    if (bUseGlobalTimeMultiplierTarget) {
        BSTimer::fGlobalTimeMultiplierTarget = afMult;
        if (abNow) {
            BSTimer::fGlobalTimeMultiplier = BSTimer::fGlobalTimeMultiplierTarget;
        }
    }
    else {
        BSTimer::fGlobalTimeMultiplierTarget = afMult;
        if (BSTimer::fGlobalTimeMultiplier != afMult) {
            fDelta = fDelta / BSTimer::fGlobalTimeMultiplier;
            BSTimer::fGlobalTimeMultiplier = afMult;
            fDelta = fDelta * afMult;
        }
    }
}

float BSTimer::GetRealTimeSeconds() const {
    return fDelta / fGlobalTimeMultiplier;
}

uint32_t BSTimer::GetTime() const {
    return iLastTime;
}

void BSTimer::Init(uint32_t aiTime) {
    cDisableCounter = 0;
    fClamp = 0.0f;
    fClampRemainder = 0.0f;
    fDelta = 0.0f;
    iLastTime = 0;
    iFirstTime = aiTime;
    bZeroDelta = false;
    bUseGlobalTimeMultiplierTarget = false;
}

void BSTimer::Enable() {
    uint32_t tickCount;
    if (cDisableCounter && (!(--cDisableCounter))) {
        tickCount = GetTickCount();
        iLastTime = tickCount - iLastTime;
        iFirstTime = tickCount - iFirstTime;
    }
}

void BSTimer::Disable() {
    uint32_t tickCount;
    if (!(cDisableCounter++)) {
        tickCount = GetTickCount();
        iLastTime = tickCount - iLastTime;
        iFirstTime = tickCount - iFirstTime;
    }
}

void BSTimer::Update(uint32_t aiTime) {
    uint32_t uiFrameDelta;
    uint32_t fOldRemainder;
    uint32_t uiTotalDelta = aiTime - iFirstTime;
    if (cDisableCounter) {
        fDelta = 0.0f;
    }
    else {
        if (fClamp == 0.0) {
            uiFrameDelta = uiTotalDelta - iLastTime;
            if (uiTotalDelta == iLastTime) {
                bZeroDelta = true;
                uiFrameDelta = 1;
            }
            else {
                bZeroDelta = false;
                if (uiFrameDelta > 166) {
                    uiFrameDelta = 166;
                }
            }
            if (uiFrameDelta < 10) {
                Sleep(10 - uiFrameDelta);
                uiFrameDelta = 10;
            }
            fDelta = static_cast<float>(uiFrameDelta * 0.0010000000474974513);
        }
        else {
            fClampRemainder = (fClampRemainder + fClamp);
            fOldRemainder = static_cast<uint32_t>(static_cast<int64_t>(fClampRemainder));
            fClampRemainder = static_cast<float>(fClampRemainder - static_cast<double>(fOldRemainder));
            uiTotalDelta = fOldRemainder + iLastTime;
            fDelta = static_cast<float>(fClamp * 0.0010000000474974513);
        }
        iLastTime = uiTotalDelta;
        fDeltaUnmodified = fDelta;
        fDelta = (fDelta * BSTimer::fGlobalTimeMultiplier);
    }
}

void BSTimer::Update() {
    int32_t tickCount = static_cast<int32_t>(BSGetTickCount());
    Update(static_cast<uint32_t>(tickCount));
}

void BSTimer::SetUseGlobalTimeMultiplierTarget(bool abValue) {
    bUseGlobalTimeMultiplierTarget = abValue;
}

void BSTimer::ClampAt(uint32_t aiClamp) {
    double limitValue;
    if (aiClamp) {
        limitValue = 1000.0 / aiClamp;
    }
    else {
        limitValue = 0.0;
    }
    fClamp = static_cast<float>(limitValue);
}

void BSTimer::RestoreGlobalTimeMultiplierCache() {
    SetGlobalTimeMultiplier(BSTimer::fPreviousGlobalTimeMultiplier, false);
}

float BSTimer::QGlobalTimeMultiplier() {
    return BSTimer::fGlobalTimeMultiplier;
}

void BSTimer::UpdateGlobalTimeMultiplier() {
    if (!(BSTimer::fGlobalTimeMultiplierTarget > static_cast<double>(BSTimer::fGlobalTimeMultiplier))) {
        if (BSTimer::fGlobalTimeMultiplierTarget < static_cast<double>(BSTimer::fGlobalTimeMultiplier)) {
            BSTimer::fGlobalTimeMultiplier = static_cast<float>(
                (1.0 - fTimerGlobalMultChangeSpeed) * BSTimer::fGlobalTimeMultiplier
            );
            if (BSTimer::fGlobalTimeMultiplierTarget > static_cast<double>(BSTimer::fGlobalTimeMultiplier)) {
                BSTimer::fGlobalTimeMultiplier = BSTimer::fGlobalTimeMultiplierTarget;
            }
        }
    }
    else {
        BSTimer::fGlobalTimeMultiplier = static_cast<float>(
            (fTimerGlobalMultChangeSpeed + 1.0) * BSTimer::fGlobalTimeMultiplier
        );
        if (BSTimer::fGlobalTimeMultiplierTarget < static_cast<double>(BSTimer::fGlobalTimeMultiplier)) {
            BSTimer::fGlobalTimeMultiplier = BSTimer::fGlobalTimeMultiplierTarget;
        }
    }
}

double BSTimer::GetGlobalTimeMultiplierTarget() {
    return BSTimer::fGlobalTimeMultiplierTarget;
}

void BSTimer::SaveGlobalTimeMultiplier() {
    BSTimer::fPreviousGlobalTimeMultiplier = BSTimer::fGlobalTimeMultiplierTarget;
}
