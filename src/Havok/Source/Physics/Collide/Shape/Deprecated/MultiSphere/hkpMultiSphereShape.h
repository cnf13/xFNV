#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

struct hkpShapeRayCastInput;

void finishLoadedObjecthkpMultiSphereShape(void* p, int32_t finishing);
// Not named in the PDB or IDA; name chosen here. castRayImpl's second step: the hit with the smallest fraction
// below the output's, of the aiNumHits that castRayInternal found, written into apOutput (passed in edi; the stack
// arguments cdecl). Returns that hit's index in the buffers, or -1 when none is closer.
int32_t findClosestHit(const hkVector4* apSpheres, const float* apHitFractions, const int32_t* apSphereIndices,
	int32_t aiNumHits, const hkpShapeRayCastInput* apInput, hkpShapeRayCastOutput* apOutput);
// Not named in the PDB or IDA; name chosen here. castRayImpl's first step: for each sphere the ray enters, its hit
// fraction and its index are appended to the two buffers (apSphereIndices passed in eax; the stack arguments
// cdecl). Returns how many.
int32_t castRayInternal(const hkpShapeRayCastInput* apInput, const hkVector4* apSpheres, int32_t aiNumSpheres,
	float* apHitFractions, int32_t* apSphereIndices);

// A shape made of up to MAX_SPHERES spheres (Havok files it under the deprecated shapes); each vector holds a
// sphere's centre and radius.
class hkpMultiSphereShape : public hkpSphereRepShape {
public:
	enum : int32_t {
		MAX_SPHERES = 0x8,
	};

	hkpMultiSphereShape(const hkVector4* apSpheres, int32_t aiNumSpheres);

	// The SDK's finish constructor of a loaded object.
	hkpMultiSphereShape(hkFinishLoadedObjectFlag aFlag) : hkpSphereRepShape(aFlag) {
		m_type.m_storage = HK_SHAPE_MULTI_SPHERE;
	}

	~hkpMultiSphereShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& inputLocal, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	int32_t m_numSpheres;
	hkVector4 m_spheres[MAX_SPHERES];
};
inline constexpr fnvd::Vtable hkpMultiSphereShape_vftable{};
