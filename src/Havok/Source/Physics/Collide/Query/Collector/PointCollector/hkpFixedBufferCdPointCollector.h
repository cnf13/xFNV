#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"

class hkpCdPoint;
class hkpFixedBufferCdPointCollector;
struct hkpRootCdPoint;

void _setHit(hkpRootCdPoint* insertAt, const hkpCdPoint* event);

typedef void (*hkFixedBufferCdPointCollectorAddCdPointCallbackFunc)(
    const hkpCdPoint& event,
    hkpFixedBufferCdPointCollector* collector
);

// A point collector that fills a caller-supplied buffer of root contact points and stops taking points when it is
// full.
class hkpFixedBufferCdPointCollector : public hkpCdPointCollector {
public:
    // The default constructor does not set the members; the callers fill the members or construct the collector again.
    hkpFixedBufferCdPointCollector() = default;
    hkpFixedBufferCdPointCollector(hkpRootCdPoint* a1, int32_t a2);
    ~hkpFixedBufferCdPointCollector() override;

    void addCdPoint(const hkpCdPoint& event) override;

    // The start of the point buffer.
    const hkpRootCdPoint* getHits() const;
    // The number of points collected.
    int32_t getNumHits() const;
    hkBool* hasHit(hkBool* a1);

    static void registerDefaultAddCdPointFunction();
    // Frees the block to the calling thread's router heap allocator (blockFree) with the object's size; does nothing
    // for null.
    static void operator delete(void* p);
    static void addCdPointImplementation(const hkpCdPoint& event, hkpFixedBufferCdPointCollector* collector);

    hkPadSpu<hkpRootCdPoint*> m_pointsArrayBase;
    hkPadSpu<hkpRootCdPoint*> m_nextFreePoint;
    hkPadSpu<int32_t> m_capacity;
    hkPadSpu<int32_t> m_numPoints;
};

// The addCdPoint callback the collector calls, registered by registerDefaultAddCdPointFunction.
inline hkFixedBufferCdPointCollectorAddCdPointCallbackFunc g_FixedBufferCdPointCollectorAddCdPointCallbackFunc{};
