#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"

class hkpCdPoint;
class hkpFixedBufferCdPointCollector;
struct hkpRootCdPoint;

void _setHit(hkpRootCdPoint* insertAt, const hkpCdPoint* event);

using hkFixedBufferCdPointCollectorAddCdPointCallbackFunc =
    void (*)(const hkpCdPoint& event, hkpFixedBufferCdPointCollector* collector);

// A point collector that fills a caller-supplied buffer of root contact points and stops taking points when it is
// full.
class hkpFixedBufferCdPointCollector : public hkpCdPointCollector {
public:
    // The default constructor does not set the members; the callers fill the members or construct the collector again.
    hkpFixedBufferCdPointCollector() = default;
    hkpFixedBufferCdPointCollector(hkpRootCdPoint* buffer, int32_t capacity);
    ~hkpFixedBufferCdPointCollector() override;

    void addCdPoint(const hkpCdPoint& event) override;

    // The start of the point buffer.
    const hkpRootCdPoint* getHits() const;
    // The number of points collected.
    int32_t getNumHits() const;
    hkBool hasHit();

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

inline hkpFixedBufferCdPointCollector::hkpFixedBufferCdPointCollector(hkpRootCdPoint* buffer, int32_t capacity)
  : m_pointsArrayBase{buffer}, m_nextFreePoint{buffer}, m_capacity{capacity}, m_numPoints{0} {}

inline void hkpFixedBufferCdPointCollector::addCdPoint(const hkpCdPoint& event) {
    g_FixedBufferCdPointCollectorAddCdPointCallbackFunc(event, this);
}

inline const hkpRootCdPoint* hkpFixedBufferCdPointCollector::getHits() const {
    return m_pointsArrayBase;
}

inline int32_t hkpFixedBufferCdPointCollector::getNumHits() const {
    return m_numPoints;
}

inline hkBool hkpFixedBufferCdPointCollector::hasHit() {
    return m_numPoints > 0;
}
