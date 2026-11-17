#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"

// The abstract bases own nothing.
hkpMoppNodeMgr::~hkpMoppNodeMgr() {}

// The tolerance and per-node split limit by mesh type: 0 is coarser, 1 finer.
hkpMoppSplitter::hkpMoppSplitParams::hkpMoppSplitParams(int32_t a1)
	: m_tolerance(0.1f), m_maxPrimitiveSplits(1000), m_maxPrimitiveSplitsPerNode(10), m_minRangeMaxListCheck(5),
	  m_checkAllEveryN(5) {
	m_interleavedBuildingEnabled.m_bool = 1;
	if (a1) {
		if (a1 == 1) {
			m_maxPrimitiveSplitsPerNode = 30;
			m_tolerance = 0.05f;
		}
	} else {
		m_tolerance = 0.2f;
	}
}

hkpMoppSplitter::~hkpMoppSplitter() {}
