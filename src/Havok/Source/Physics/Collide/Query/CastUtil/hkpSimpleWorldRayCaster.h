#pragma once

#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpBroadPhase;
class hkpBroadPhaseHandle;
class hkpCollisionFilter;
class hkpRayCollidableFilter;
struct hkpWorldRayCastInput;
struct hkpWorldRayCastOutput;

// Casts a world ray through the broad phase and narrows each handle it reaches down to the collidable's shape, keeping
// the closest hit in m_result. hkpWorld::castRay builds one on the stack when the world has no k-d tree.
class hkpSimpleWorldRayCaster : public hkpBroadPhaseCastCollector {
public:
    ~hkpSimpleWorldRayCaster() override;

    float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) override;

    // Havok SDK signature (hkpCollide.lib, tied by reloc,string).
    void castRay(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        const char* cache,
        hkpWorldRayCastOutput& output
    );
    // Havok SDK signature (hkpCollide.lib, tied by string).
    void castRayGroup(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput* inputArray,
        int32_t numRays,
        const hkpCollisionFilter* filter,
        hkpWorldRayCastOutput* outputs
    );
    void castRay_0(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        hkpWorldRayCastOutput& output
    );
    void castRaysFromSinglePoint(
        const hkpBroadPhase& broadphase,
        const hkpWorldRayCastInput* inputArray,
        int32_t numRays,
        const hkpCollisionFilter* filter,
        const char* cache,
        hkpWorldRayCastOutput* outputs
    );

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);

    const hkpWorldRayCastInput* m_input;
    const hkpRayCollidableFilter* m_filter;
    hkpWorldRayCastOutput* m_result;
    hkpShapeRayCastInput m_shapeInput;
};
inline constexpr fnvd::Vtable hkpSimpleWorldRayCaster_vftable{};
