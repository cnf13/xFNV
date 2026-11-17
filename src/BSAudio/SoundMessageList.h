#pragma once

#include "Reconstruction.hpp"

class BSSoundMessage;
template <class T> struct NiTListItem;

// A list of sound messages kept by head and tail pointers. IDA-only; the PDB has no such class.
class SoundMessageList {
public:
	// The exe builds and destroys the lists as NiTPointerLists (BSAudioManager::Construct 0xAD90E0, Destruct
	// 0xAD93F0): empty at first, every node freed at the end.
	SoundMessageList() : pHead(nullptr), pTail(nullptr), uiCount(0) {}
	~SoundMessageList() { RemoveAll(); }

	// Appends apMsg's message on a node from the shared node pool (thiscall, ret 4). IDA name; not in the PDB.
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
