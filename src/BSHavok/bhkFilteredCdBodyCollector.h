#pragma once

#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Reconstruction.hpp"

// The collector the world's collision queries use: it keeps a pair unless body B's root collidable is already one of
// the collected pairs' root collidable B. It adds no state to hkpAllCdBodyPairCollector; the PDB lists no members.
class bhkFilteredCdBodyCollector : public hkpAllCdBodyPairCollector {
public:
	bhkFilteredCdBodyCollector();

	// Protected in the SDK.
	void addCdBodyPair(const hkpCdBody& arBodyA, const hkpCdBody& arBodyB) override;
	~bhkFilteredCdBodyCollector() override;
};
inline constexpr fnvd::Vtable bhkFilteredCdBodyCollector_vftable{};
