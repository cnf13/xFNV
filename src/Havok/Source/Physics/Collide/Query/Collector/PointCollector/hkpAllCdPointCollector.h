#pragma once

#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Reconstruction.hpp"

// A point collector that keeps every contact point it is given in an inline array of root contact points that grows
// on the heap.
class hkpAllCdPointCollector : public hkpCdPointCollector {
public:
	hkpAllCdPointCollector();
	~hkpAllCdPointCollector() override;

	void addCdPoint(const hkpCdPoint& event) override;
	void reset() override;
	virtual void addRootCdPoint(const hkpRootCdPoint&);

	// m_hits. The exe calls the shared getter of the field at +0x10, TESDataHandler::GetWorldSpaceList (0x460140), a
	// trivial accessor.
	const hkArray<hkpRootCdPoint>& getHits() const { return m_hits; }

	// Whether the collector holds any hit; the result hkBool comes back through apResult,
	// which the exe returns too (thiscall, ret 4, 0x621210).
	hkBool* hasHit(hkBool* apResult);
	// Sorts the collected hits by their hit fraction.
	void sortHits();

	// Frees the block to the calling thread's router heap allocator (blockFree, vtable slot 2)
	// with the object's size, 0x3A0; nothing for null. The deleting destructor calls it.
	static void operator delete(void* p);

	hkInplaceArray<hkpRootCdPoint, 8> m_hits;
};

inline constexpr fnvd::Vtable hkpAllCdPointCollector_vftable{};
