#include "Fallout/AI/ActorValueCache.h"

#include "Fallout Shared/ActorValue.h"

ActorValueCache::ActorValueCache() {
    ResetCachedActorValues();
}

void ActorValueCache::ResetCachedActorValues() {
    for (int32_t i = 0; i < 77; ++i) {
        ActorValueCacheInfo* pInfo = &Cache[i];
        pInfo->fValue = 0.0f;
        pInfo->bOutOfDate = true;
    }
}

void ActorValueCache::SetCachedActorValue(ActorValue::Index aiWhich, float afValue) {
    if (aiWhich >= 0 && aiWhich < 77) {
        ActorValueCacheInfo* pInfo = &Cache[aiWhich];
        pInfo->fValue = afValue;
        pInfo->bOutOfDate = false;
    }
}

void ActorValueCache::SetCachedActorValueOutOfDate(ActorValue::Index aiWhich) {
    if (aiWhich >= 0 && aiWhich < 77) {
        Cache[aiWhich].bOutOfDate = true;
        SetRelatedActorValuesOutOfDate(aiWhich);
    }
}

// Each dependent value is tested against both bounds (below 0, at or above ACTOR_VALUE_COUNT), and the compare is
// signed.
void ActorValueCache::SetRelatedActorValuesOutOfDate(ActorValue::Index aiParent) {
    int32_t iCount = 0;
    ActorValue::Index* pDependentActorValues = ActorValue::GetDependentActorValues(aiParent, iCount);
    if (pDependentActorValues) {
        if (iCount) {
            for (int32_t i = 0; i < iCount; ++i) {
                if (pDependentActorValues[i] >= 0 && pDependentActorValues[i] < ActorValue::ACTOR_VALUE_COUNT) {
                    Cache[pDependentActorValues[i]].bOutOfDate = true;
                }
            }
        }
    }
}
