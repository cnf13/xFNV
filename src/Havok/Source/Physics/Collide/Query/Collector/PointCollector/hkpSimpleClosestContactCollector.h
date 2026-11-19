#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"

#include <stdint.h>

class hkpSimpleClosestContactCollector : public hkpCdPointCollector {
public:
    // The hit point's distance at HK_REAL_MAX, then no hit.
    hkpSimpleClosestContactCollector() : m_hasHit(false) {
        m_hitPoint.m_separatingNormal.m_quad[3] = 3.40282e38f;
    }
    ~hkpSimpleClosestContactCollector() override;

    void addCdPoint(const hkpCdPoint& event) override;
    void reset() override;

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
    static void operator delete(void* apMemory);

    hkBool m_hasHit;
    // 7 bytes of padding: m_hitPoint is 16-byte aligned (its hkVector4 members), so it starts at 0x10.
    hkContactPoint m_hitPoint;
};
