#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Reconstruction.hpp"

class hkpCharacterProxy;
class hkpCollisionFilter;

// A pointer that holds a reference on an hkReferencedObject. Only the accessors src uses are declared.
template <class TYPE> class hkRefPtr {
public:
	// The PDB's ~hkRefPtr: removes the reference the pointer holds, then clears it. The exe inlines it; its one
	// out-of-line copy (thiscall, ret 0, 0xC6CCA0) is called only by EH unwind funclets (0xF30CFE, 0xF30EF8,
	// 0xF30F06, 0xF30F14, ...: bhkCharacterController's spSupportBody +0x60C, spBumpedBody +0x630 and
	// spBumpedCharProxy +0x638, and a member at +0xA4) and serves every instance (removeReference is
	// hkReferencedObject's). Only per-instance definitions exist.
	// The SDK's: a null pointer.
	hkRefPtr() : m_pntr(nullptr) {}
	// The SDK's finish constructor: a loaded pointer keeps the value the load wrote.
	hkRefPtr(hkFinishLoadedObjectFlag) {}

	// The SDK's copy constructor and assignment (hkRefPtr.inl): the copy takes a reference of its own. The implicit
	// ones would not, and each copy's destructor would remove one more reference than was added.
	hkRefPtr(const hkRefPtr& rp) {
		if (rp.m_pntr) {
			rp.m_pntr->addReference();
		}
		m_pntr = rp.m_pntr;
	}

	~hkRefPtr() {
		if (m_pntr) {
			m_pntr->removeReference();
		}
		m_pntr = nullptr;
	}

	// Inline in the exe.
	TYPE* val() const { return m_pntr; }
	// Inline in the exe.
	TYPE* operator->() const { return m_pntr; }
	// Inline in the exe.
	operator TYPE*() const { return m_pntr; }

	void operator=(const hkRefPtr& rp) {
		if (rp.m_pntr) {
			rp.m_pntr->addReference();
		}
		if (m_pntr) {
			m_pntr->removeReference();
		}
		m_pntr = rp.m_pntr;
	}

	TYPE* m_pntr;
};
