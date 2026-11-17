#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

// A set of smart pointers (Gamebryo's NiTPtrSet.h): an NiTSet whose removals clear the slots they free, so the
// pointers release their objects. No members of its own.
template <class T, class TAlloc> class NiTPtrSet : public NiTSet<T, TAlloc> {
public:
	// An uiInitialSize-element buffer (none for 0), nothing used (Gamebryo's NiTSet constructor, default size 0).
	NiTPtrSet(uint32_t uiInitialSize = 0) {
		this->m_pBase = uiInitialSize ? TAlloc::Allocate(uiInitialSize) : nullptr;
		this->m_uiAlloced = uiInitialSize;
		this->m_uiUsed = 0;
	}

	// Clears every used slot (releasing its object) and empties the set.
	void RemoveAll();
	// Shifts the elements after uiIndex down by one, keeping their order, and clears the freed last slot (releasing its
	// object).
	void OrderedRemoveAt(uint32_t uiIndex);
};
// Clears every allocated slot, not only the used ones (releasing each object), then empties the set.
template <class T, class TAlloc> void NiTPtrSet<T, TAlloc>::RemoveAll() {
	for (uint32_t ui = 0; ui < this->m_uiAlloced; ++ui) {
		this->m_pBase[ui] = nullptr;
	}
	this->m_uiUsed = 0;
}

template <class T, class TAlloc> void NiTPtrSet<T, TAlloc>::OrderedRemoveAt(uint32_t uiIndex) {
	for (uint32_t ui = uiIndex; ui < this->m_uiUsed - 1; ++ui) {
		this->m_pBase[ui] = this->m_pBase[ui + 1];
	}
	--this->m_uiUsed;
	this->m_pBase[this->m_uiUsed] = nullptr;
}
// A pointer set whose storage is made with new[].
template <class T> class NiTObjectPtrSet : public NiTPtrSet<T, NiTNewInterface<T>> {
public:
	NiTObjectPtrSet(uint32_t uiInitialSize = 0) : NiTPtrSet<T, NiTNewInterface<T>>(uiInitialSize) {}
};

// A pointer set whose storage is made with malloc.
template <class T> class NiTPrimitivePtrSet : public NiTPtrSet<T, NiTMallocInterface<T>> {
public:
	NiTPrimitivePtrSet(uint32_t uiInitialSize = 0) : NiTPtrSet<T, NiTMallocInterface<T>>(uiInitialSize) {}
};
