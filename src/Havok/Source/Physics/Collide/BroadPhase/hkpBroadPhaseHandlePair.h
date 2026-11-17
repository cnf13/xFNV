#pragma once

class hkpBroadPhaseHandle;

// Two broad phase handles whose volumes began or stopped to overlap.
class hkpBroadPhaseHandlePair {
public:
	hkpBroadPhaseHandle* m_a;
	hkpBroadPhaseHandle* m_b;
};
