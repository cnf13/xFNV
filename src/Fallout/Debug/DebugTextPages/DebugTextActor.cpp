#include "BSHavok/bhkRagdollController.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CachedValues.h"

float BaseProcess::GetCachedEndurance(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_ENDURANCE)) {
            return apOwner->CalculateCachedEndurance();
        }

        return pCachedValues->fCachedEndurance;
    }

    return 0.0f;
}
