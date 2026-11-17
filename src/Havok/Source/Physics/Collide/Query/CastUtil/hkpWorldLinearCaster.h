#pragma once

#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkpCdPointCollector;
class hkpCollidable;
class hkpCollidableCollidableFilter;
class hkpLinearCastInput;

// Sweeps a collidable against broadphase candidates and dispatches narrowphase casts.
class hkpWorldLinearCaster : public hkpBroadPhaseCastCollector {
public:
    ~hkpWorldLinearCaster() override;

    float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) override;

    // Called by hkpWorld::linearCast with the world's broadphase, filter, collision input and config.
    void linearCast(
        const hkpBroadPhase& broadphase,
        const hkpCollidable* collA,
        const hkpLinearCastInput& input,
        const hkpCollidableCollidableFilter* filter,
        const hkpCollisionInput& collInput,
        hkpCollisionAgentConfig* config,
        hkpBroadPhaseAabbCache* m_broadPhaseCache,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* startPointCollector
    );

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);

    const hkpLinearCastInput* m_input;
    const hkpCollidableCollidableFilter* m_filter;
    hkpCdPointCollector* m_castCollector;
    hkpCdPointCollector* m_startPointCollector;
    const hkpCollidable* m_collidableA;
    hkpShapeType m_typeA;
    // 4 bytes of padding: m_shapeInput is 16-byte aligned (its hkVector4 members), so it starts at 0x20.
    hkpLinearCastCollisionInput m_shapeInput;
};
inline constexpr fnvd::Vtable hkpWorldLinearCaster_vftable{};
