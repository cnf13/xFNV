#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpCdBody;

// Collects the body pairs a collision query finds; m_earlyOut tells the query to stop. The hkArray of pairs belongs to
// hkpAllCdBodyPairCollector, not to this class; this class's constructor sets only m_earlyOut.
class hkpCdBodyPairCollector {
public:
	hkpCdBodyPairCollector();
	virtual ~hkpCdBodyPairCollector();

	virtual void addCdBodyPair(const hkpCdBody& aBodyA, const hkpCdBody& aBodyB) = 0;
	virtual void reset() { m_earlyOut = false; }

	static void operator delete(void* apMemory);

	hkBool m_earlyOut;
};
