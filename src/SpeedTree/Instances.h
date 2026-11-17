#pragma once

#include "BSCore/BSCriticalSection.h"
#include "Reconstruction.hpp"
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

	// The exe guards m_vUniqueTrees with a static critical section; speedtree-2 has no such member.
	static BSCriticalSection m_UniqueTreesCritSection;
	static TreeVector m_vUniqueTrees;
};

// IDA name; not in the PDB. The tree-pointer iterator's operator+= (thiscall, ret 4, returns apThis), out of line in
// the exe where src's is the inline template; a per-address body for a duplicated template instance. Defined in
// SpeedTreeRT.cpp.
SInstanceList::TreeVector::const_iterator* VectorIteratorAdvance(SInstanceList::TreeVector::const_iterator* apThis,
	ptrdiff_t _Off);
