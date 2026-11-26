#include "SDK/DirectShow/CRequestList.h"

#include "BSCore/NewOverloads.h"

// The SDK's CBaseList::FindI. The exe shares the list code between all its lists; src models the list as
// CRequestList.
CRequestList::CNode* CRequestList::FindI(void* apObj) const
{
	CNode* pPos = this->GetHeadPositionI();
	while (pPos) {
		if (CRequestList::GetI(pPos) == apObj)
			return pPos;
		// EXE-INLINED none: CBaseList::Next
		pPos = pPos ? pPos->m_pNext : this->m_pFirst;
	}
	return nullptr;
}

void* CRequestList::RemoveI(CNode* apNode) {
	if (apNode) {
		if (apNode->m_pPrev) {
			apNode->m_pPrev->m_pNext = apNode->m_pNext;
		} else {
			m_pFirst = apNode->m_pNext;
		}
		CNode* pNext = apNode->m_pNext;
		if (pNext) {
			pNext->m_pPrev = apNode->m_pPrev;
		} else {
			m_pLast = apNode->m_pPrev;
		}
		void* pObject = apNode->m_pObject;
		if (m_iUsed >= m_iCacheSize) {
			ourFree(apNode);
		} else {
			apNode->m_pNext = m_pCacheHead;
			++m_iUsed;
			m_pCacheHead = apNode;
		}
		--m_Count;
		return pObject;
	}
	return apNode;
}

int32_t CRequestList::AddTail(CRequestList* apList) {
	CNode* pPos = apList->GetHeadPositionI();
	while (pPos) {
		if (!AddTailI(apList->GetNextI(pPos)))
			return 0;
	}
	return 1;
}

CRequestList::CRequestList([[maybe_unused]] const char* apName)
	: m_pFirst(nullptr), m_pLast(nullptr), m_Count(0), m_iCacheSize(10), m_iUsed(0), m_pCacheHead(nullptr) {}

void* CRequestList::GetNextI(CNode*& arpPos) const {
	CNode* pNode = arpPos;
	if (!pNode)
		return nullptr;
	arpPos = pNode->m_pNext;
	return pNode->m_pObject;
}

// KEEP: the exe's only caller, FindI (0x9FD1C0), reads ecx (this) and edx (the position) after the call, which the
// exe's body leaves intact and clang does not guarantee.
void* CRequestList::GetI(CRequestList::CNode* apPos) {
	void* result = apPos;
	if (apPos) {
		return apPos->m_pObject;
	}
	return result;
}

CRequestList::CNode* CRequestList::AddTailI(void* apObj) {
	CRequestList::CNode* result = m_pCacheHead;
	if (result) {
		CRequestList::CNode* m_pNext = result->m_pNext;
		--m_iUsed;
		m_pCacheHead = m_pNext;
	} else {
		result = static_cast<CRequestList::CNode*>(ourMalloc(sizeof(CRequestList::CNode)));
		if (!result) {
			return result;
		}
	}
	result->m_pObject = apObj;
	result->m_pNext = 0;
	result->m_pPrev = this->m_pLast;
	CRequestList::CNode* m_pLast = this->m_pLast;
	if (m_pLast) {
		m_pLast->m_pNext = result;
		++m_Count;
	} else {
		++m_Count;
		m_pFirst = result;
	}
	this->m_pLast = result;
	return result;
}

void* CRequestList::RemoveHeadI() {
	return RemoveI(m_pFirst);
}

CRequestList::CNode* CRequestList::GetHeadPositionI() const
{
	return m_pFirst;
}

void CRequestList::RemoveAll() {
	CRequestList::CNode* m_pFirst = this->m_pFirst;
	while (m_pFirst) {
		CRequestList::CNode* v3 = m_pFirst;
		m_pFirst = m_pFirst->m_pNext;
		ourFree(v3);
	}
	m_Count = 0;
	m_pLast = 0;
	this->m_pFirst = 0;
}

// The exe's cleanup-only C++ EH frame is left out.
void CRequestList::FreeAll() {
	RemoveAll();
	CRequestList::CNode* m_pCacheHead = this->m_pCacheHead;
	while (m_pCacheHead) {
		CRequestList::CNode* v3 = m_pCacheHead;
		m_pCacheHead = m_pCacheHead->m_pNext;
		ourFree(v3);
	}
}

CRequestList::~CRequestList() {
	FreeAll();
}
