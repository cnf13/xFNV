#pragma once

#include <cstdint>

class hkpCdBody;
struct hkpShapeRayCastCollectorOutput;

// Receives the hits of a ray cast; m_earlyOutHitFraction is the hit fraction the cast may stop at.
class hkpRayHitCollector {
public:
	hkpRayHitCollector();

	// Called for every hit of the ray.
	virtual void addRayHit(const hkpCdBody& arCdBody, const hkpShapeRayCastCollectorOutput& arHitInfo) = 0;
	virtual ~hkpRayHitCollector();

	void reset();

	// Writes the shape keys from the root body down to body into buf (at most maxKeys - 1 of them), then the
	// invalid key 0xFFFFFFFF; returns the number written.
	static int32_t shapeKeysFromCdBody(uint32_t* buf, int32_t maxKeys, const hkpCdBody& body);
	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);

	float m_earlyOutHitFraction;
};
