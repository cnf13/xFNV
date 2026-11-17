#pragma once

#include <cstdint>

class BSSoundMessage;

template <class T> struct NiTListItem;

// A list of sound messages kept by head and tail pointers.
class SoundMessageList {
public:
	// The lists are built and destroyed as NiTPointerLists: empty at first, every node freed at the end.
	SoundMessageList() : pHead(nullptr), pTail(nullptr), uiCount(0) {}
	~SoundMessageList() { RemoveAll(); }

	// Appends apMsg's message on a node from the shared node pool.
	void AddTail_0(BSSoundMessage** apMsg);
	// Links apNode after the tail and counts it.
	void AddNodeTail(NiTListItem<BSSoundMessage*>* apNode);
	// Unlinks the head node, frees it to the list's
	// allocator and returns its message; the callers test the count first.
	BSSoundMessage* RemoveHead();
	// Frees every node to the list's allocator.
	void RemoveAll();

	NiTListItem<BSSoundMessage*>* pHead;
	NiTListItem<BSSoundMessage*>* pTail;
	uint32_t uiCount;
};
