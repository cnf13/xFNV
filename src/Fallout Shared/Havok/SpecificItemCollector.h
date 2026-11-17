#pragma once

#include "BSHavok/bhkCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"

class MobileObject;

// A closest-hit collector for ray casts that only counts the hits on objects of the layer of iFilter (other layers'
// hits only when they are of the same group) and farther than fMinHitFraction.
class SpecificItemCollector : public hkpClosestRayHitCollector {
public:
	SpecificItemCollector() = default;
	// An empty filter, then Init when arFilter is set.
	SpecificItemCollector(COL_LAYER aePick, MobileObject* arFilter);
	SpecificItemCollector(COL_LAYER aePick, CFilter& arFilter);

	void addRayHit(const hkpCdBody& arBody, const hkpShapeRayCastCollectorOutput& arHitInfo) override;
	~SpecificItemCollector() override;

	// Sets the filter and the collision flags of the pick layer.
	void Init(COL_LAYER aePick, CFilter& arFilter);
	// Stores afFraction into fMinHitFraction through x87.
	void SetMinHitFraction(float afValue);

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	CFilter iFilter;
	float fMinHitFraction;
};
