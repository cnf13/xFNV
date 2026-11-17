#include "BSHavok/bhkFilteredCdBodyCollector.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"

// The exe's EH frame (scope table 0xf34838) is cleanup-only and left out.
void bhkFilteredCdBodyCollector::addCdBodyPair(const hkpCdBody& arBodyA, const hkpCdBody& arBodyB) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

	// The exe reads the pair count once, before the loop, and walks body B up to its root again on every pass.
	int32_t iCount = m_hits.getSize();
	for (int32_t i = 0; i < iCount; ++i) {
		const hkpCdBody* pkRoot = &arBodyB;
		while (pkRoot->m_parent)
			pkRoot = pkRoot->m_parent;
		// A collidable is its own body (hkpCollidable derives from hkpCdBody), so the exe compares raw pointers.
		if (pkRoot == static_cast<const hkpCdBody*>(m_hits.m_data[i].m_rootCollidableB))
			return;
	}
	hkpAllCdBodyPairCollector::addCdBodyPair(arBodyA, arBodyB);
}

bhkFilteredCdBodyCollector::~bhkFilteredCdBodyCollector() {}
