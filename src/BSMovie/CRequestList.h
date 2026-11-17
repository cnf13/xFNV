#pragma once

#include <cstdint>

// The DirectShow base classes' generic list (CBaseList / CGenericList<CAsyncRequest>): a doubly linked list of object
// pointers with a node cache.
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

	// Returns the head position (m_pFirst).
	CNode* GetHeadPositionI() const;
	// Returns the object at arpPos, which moves on to the next node; null for a null position. The body never reads
	// this.
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
	// Appends apList's objects; returns 0 when no node could be had, else 1.
	int32_t AddTail(CRequestList* apList);
	void RemoveAll();
	void FreeAll();

	// The object at apPos, or null for a null position.
	static void* GetI(CNode* apPos);

	CNode* m_pFirst;
	CNode* m_pLast;
	int32_t m_Count;
	// Cache size, used count and the cached node chain.
	int32_t m_iCacheSize;
	int32_t m_iUsed;
	CNode* m_pCacheHead;
};
