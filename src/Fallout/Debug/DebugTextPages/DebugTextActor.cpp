#include "BSHavok/bhkRagdollController.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CachedValues.h"

bool bhkRagdollController::QRagdollFeedbackActive() {
    return bActiveRagdollFeedback;
}

float BaseProcess::GetCachedEndurance(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_ENDURANCE)) {
            return apOwner->CalculateCachedEndurance();
        }
        else {
            return pCachedValues->fCachedEndurance;
        }
    }
    else {
        return 0.0f;
    }
}
