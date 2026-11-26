#pragma once

#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

// A point collector that keeps every contact point it is given in an inline array of root contact points that grows
// on the heap.
class hkpAllCdPointCollector : public hkpCdPointCollector {
public:
    hkpAllCdPointCollector();
    ~hkpAllCdPointCollector() override;

    void addCdPoint(const hkpCdPoint& event) override;
    void reset() override;
    virtual void addRootCdPoint(const hkpRootCdPoint&);

    const hkArray<hkpRootCdPoint>& getHits() const {
        return m_hits;
    }

    // Whether the collector holds any hit; the result hkBool is written through apResult, which is also returned.
    hkBool hasHit();
    // Sorts the collected hits by their hit fraction.
    void sortHits();

    // Frees the block to the calling thread's router heap allocator (blockFree) with the object's size; nothing for
    // null. The deleting destructor calls it.
    static void operator delete(void* p);

    hkInplaceArray<hkpRootCdPoint, 8> m_hits;
};

inline void hkpAllCdPointCollector::reset() {
    m_hits.clear();
    hkpCdPointCollector::reset();
}
