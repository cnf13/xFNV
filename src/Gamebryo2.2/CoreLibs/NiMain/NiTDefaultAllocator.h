#pragma once

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Reconstruction.hpp"

// Node allocator of NiTList and NiTMap: one heap block per node, no pool. The engine names the class DFALL (short
// symbols); NiTDefaultAllocator is its source spelling.
template <class T> class DFALL : public NiTAbstractPoolAllocator<T> {
public:
	// The PDB's DFALL<T>::AllocNode: one heap node, its element at +0 (the PDB's only member). It hides the pool's
	// three-field node, which no DFALL user names.
	class AllocNode {
	public:
		// The deleting destructor frees the node through ourFree.
		static void operator delete(void* apMemory) { ourFree(apMemory); }

		T m_element;
	};

	void* Allocate();
	void Deallocate(void* p);
};
// The exe inlines the node free: the block goes back to the engine heap.
template <class T> inline void DFALL<T>::Deallocate(void* p) {
	MemoryManager_Singleton.Deallocate(p);
}
// A heap node from ourMalloc, default-initialised when it is not null: a node with a class member (a string, a smart
// pointer, a handle) runs its constructor, a plain node is left as
// allocated.
template <class T> void* DFALL<T>::Allocate() {
	void* pkNode = ourMalloc(static_cast<uint32_t>(sizeof(T)));
	return pkNode ? new (pkNode) T : nullptr;
}
template <class T> using NiTDefaultAllocator = DFALL<T>;
