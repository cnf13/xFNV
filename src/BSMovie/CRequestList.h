#pragma once

#include "Reconstruction.hpp"

// The DirectShow base classes' generic list (the SDK's CBaseList / CGenericList<CAsyncRequest>; field names are the
// SDK's, chosen here): a doubly linked list of object pointers with a node cache. The exe shares the list code
// between all its lists.
class CRequestList {
public:
	struct CNode {
		CNode* m_pPrev;
		CNode* m_pNext;
		void* m_pObject;
	};

	CRequestList(int32_t a1);
	CRequestList(const CRequestList&) = delete;
	// Frees the list's nodes, then the cached ones (FreeAll).
	~CRequestList();

	// Not named in the PDB or IDA; name chosen here. The head position (m_pFirst), through the one-field getter the
	// exe shares with NiInitOptions::GetAllocator (thiscall, 0x9FD0F0).
	CNode* GetHeadPositionI() const;
	// Not named in the PDB or IDA; name chosen here (the SDK's CBaseList::GetNextI). The object at arpPos, which
	// moves on to the next node; null for a null position (thiscall, ret 4, 0x9FD180; the body never reads this).
	void* GetNextI(CNode*& arpPos) const;
	// The position of apObj, or null.
	CNode* FindI(void* apObj) const;
	// Unlinks the head node and returns its object, or null.
	void* RemoveHeadI();
	// Links apObj at the tail; null when no node could be had.
	CNode* AddTailI(void* apObj);
	CRequestList& operator=(const CRequestList&) = delete;
	// Unlinks apNode, caches or frees
	// it, and returns its object; null for a null node.
	void* RemoveI(CNode* apNode);
	// Not named in the PDB or IDA; name chosen here (the SDK's CBaseList::AddTail(CBaseList*)). Appends apList's
	// objects; 0 when no node could be had, else 1 (thiscall, ret 4, 0x9FD2D0).
	int32_t AddTail(CRequestList* apList);
	void RemoveAll();
	void FreeAll();

	// The object at apPos, or null for a null position.
	static void* GetI(CNode* apPos);

	// IDA: _DWORD dword0.
	CNode* m_pFirst;
	// IDA: _DWORD dword4.
	CNode* m_pLast;
	// IDA: _DWORD dword8.
	int32_t m_Count;
	// IDA: _DWORD dwordC..dword14. The SDK's CNodeCache: cache size, used count and the cached node chain.
	int32_t m_iCacheSize;
	int32_t m_iUsed;
	CNode* m_pCacheHead;
};
