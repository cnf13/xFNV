#pragma once

#include "BSCore/BSCriticalSection.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <vector>

// One instance of a tree: the tree it copies, where it stands and its own LOD level.
struct STreeInstanceData {
	CSpeedTreeRT* m_pParent;
	CVec3 m_cPosition;
	float m_fLodLevel;
};

// The instances of a tree, and every unique (non-instance) tree.
struct SInstanceList {
	using TreeVector = std::vector<CSpeedTreeRT*, std::allocator<CSpeedTreeRT*>>;

	SInstanceList();

	TreeVector m_vInstances;

	// m_vUniqueTrees is guarded by the static critical section m_UniqueTreesCritSection.
	static BSCriticalSection m_UniqueTreesCritSection;
	static TreeVector m_vUniqueTrees;
};

// The tree-pointer iterator's operator+=, returning apThis. Defined in SpeedTreeRT.cpp.
SInstanceList::TreeVector::const_iterator* VectorIteratorAdvance(SInstanceList::TreeVector::const_iterator* apThis,
	ptrdiff_t _Off);
