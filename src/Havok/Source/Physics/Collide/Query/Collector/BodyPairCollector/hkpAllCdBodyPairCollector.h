#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Reconstruction.hpp"

// Collects every body pair a collision query finds, up to sixteen in place. Its code stays with the exe.
class hkpAllCdBodyPairCollector : public hkpCdBodyPairCollector {
public:
	hkpAllCdBodyPairCollector();

	// Clears m_hits, then hkpCdBodyPairCollector's reset.
	void reset() override;
	~hkpAllCdBodyPairCollector() override;
	// Protected in the SDK.
	void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) override;

	// Havok SDK (an inline getter returning m_hits); the exe calls it out of line as the shared +8 getter (the
	// address of NiObjectNET::GetName, thiscall, ret 0, 0x413F40).
	const hkArray<hkpRootCdBodyPair>& getHits() const;

	// Frees the block to the calling thread's router heap allocator, with the object's size.
	static void operator delete(void* p);

	// Protected in the SDK.
	hkInplaceArray<hkpRootCdBodyPair, 16> m_hits;
};
inline constexpr fnvd::Vtable hkpAllCdBodyPairCollector_vftable{};
