#include "BSHavok/bhkFilteredCdBodyCollector.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"

void bhkFilteredCdBodyCollector::addCdBodyPair(const hkpCdBody& arBodyA, const hkpCdBody& arBodyB) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

	// Reads the pair count once, before the loop, and walks body B up to its root again on every pass.
	int32_t iCount = m_hits.getSize();
	for (int32_t i = 0; i < iCount; ++i) {
		const hkpCdBody* pkRoot = &arBodyB;
		while (pkRoot->m_parent)
			pkRoot = pkRoot->m_parent;
		// A collidable is its own body (hkpCollidable derives from hkpCdBody), so raw pointers are compared.
		if (pkRoot == static_cast<const hkpCdBody*>(m_hits.m_data[i].m_rootCollidableB))
			return;
	}
	hkpAllCdBodyPairCollector::addCdBodyPair(arBodyA, arBodyB);
}

bhkFilteredCdBodyCollector::~bhkFilteredCdBodyCollector() {}
