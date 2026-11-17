#pragma once

#include "Fallout Shared/TESObjectREFR.h"

#include <stdint.h>

// A node of an intrusive list pairing a Havok collidable with the game object it belongs to.
template <class T> class bhkTTNode {
public:
	bhkTTNode(T* apRef, const hkpCollidable* apCol) : iFlags(0), pNext(nullptr), pCollidable(apCol), pRef(apRef) {}

	// The node's object.
	T* QReference();
	const hkpCollidable* QCollidable();
	// The bits of aiFlags set in iFlags.
	uint32_t GetFlags(uint32_t aiFlags);
	// Sets (abTF) or clears the bits of aiFlags in iFlags.
	void SetFlags(uint32_t aiFlags, bool abTF);
	// The node in the list from this one whose collidable is apCol, or null.
	bhkTTNode<T>* GetEntry(const hkpCollidable* apCol);
	// The node in the list from this one whose object is apRef, or null.
	bhkTTNode<T>* GetEntry(T* apRef);
	// Unlinks this node from the list starting at apHead and returns the list's head afterwards (apHead's next when
	// this node is the head; the list unchanged when it is not in it).
	bhkTTNode<T>* RemoveFromList(bhkTTNode<T>* apHead);

	uint32_t iFlags;
	bhkTTNode<T>* pNext;
	const hkpCollidable* pCollidable;
	T* pRef;
};
template <class T> T* bhkTTNode<T>::QReference() {
	return pRef;
}

template <class T> const hkpCollidable* bhkTTNode<T>::QCollidable() {
	return pCollidable;
}

template <class T> uint32_t bhkTTNode<T>::GetFlags(uint32_t aiFlags) {
	return iFlags & aiFlags;
}

template <class T> void bhkTTNode<T>::SetFlags(uint32_t aiFlags, bool abTF) {
	if (abTF) {
		iFlags |= aiFlags;
	} else {
		iFlags &= ~aiFlags;
	}
}

template <class T> bhkTTNode<T>* bhkTTNode<T>::GetEntry(const hkpCollidable* apCol) {
	bhkTTNode<T>* pNode = this;
	while (pNode && pNode->pCollidable != apCol) {
		pNode = pNode->pNext;
	}
	return pNode;
}

template <class T> bhkTTNode<T>* bhkTTNode<T>::GetEntry(T* apRef) {
	bhkTTNode<T>* pNode = this;
	while (pNode && pNode->pRef != apRef) {
		pNode = pNode->pNext;
	}
	return pNode;
}

template <class T> bhkTTNode<T>* bhkTTNode<T>::RemoveFromList(bhkTTNode<T>* apHead) {
	if (this == apHead) {
		return pNext;
	}
	for (bhkTTNode<T>* pPrev = apHead; pPrev; pPrev = pPrev->pNext) {
		if (pPrev->pNext == this) {
			pPrev->pNext = pNext;
			return apHead;
		}
	}
	return apHead;
}
