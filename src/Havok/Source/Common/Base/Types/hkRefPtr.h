#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

class hkpCharacterProxy;
class hkpCollisionFilter;

// A pointer that holds a reference on an hkReferencedObject. Only some of the accessors are declared.
template <class TYPE> class hkRefPtr {
public:
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

	TYPE* val() const { return m_pntr; }
	TYPE* operator->() const { return m_pntr; }
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
