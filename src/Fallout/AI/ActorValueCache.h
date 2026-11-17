#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Reconstruction.hpp"

// An actor's actor values as its high process caches them: one entry per actor value index, each with an
// out-of-date flag and the value (HighProcess::pActorValueCache points to one).
class ActorValueCache {
public:
	// One cached actor value.
	struct ActorValueCacheInfo {
		bool bOutOfDate;
		uint8_t padding01[3];
		float fValue;
	};

	ActorValueCache();

	// Marks every cached value out of date and zero.
	void ResetCachedActorValues();
	// Stores afValue for aiWhich and marks it up to date; out-of-range indices are ignored.
	void SetCachedActorValue(ActorValue::Index aiWhich, float afValue);
	// The value cached for aiWhich.
	float GetCachedActorValue(ActorValue::Index aiWhich);
	// Whether the value cached for aiWhich is out of date.
	bool IsCachedActorValueOutOfDate(ActorValue::Index aiWhich);
	// Marks aiWhich and the actor values linked to it out of date; out-of-range indices are ignored.
	void SetCachedActorValueOutOfDate(ActorValue::Index aiWhich);

protected:
	// Marks the actor values linked to aiParent out of date.
	void SetRelatedActorValuesOutOfDate(ActorValue::Index aiParent);

public:
	ActorValueCacheInfo Cache[77];
};
