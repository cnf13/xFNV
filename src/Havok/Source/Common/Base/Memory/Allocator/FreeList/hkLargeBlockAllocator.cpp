#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkLargeBlockAllocator.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocatorServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkMemoryBlockServer.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

hkLargeBlockAllocator::hkLargeBlockAllocator(hkMemoryBlockServer* server)
	: m_limitedListener(nullptr), m_server(server) {
	_init();
}

// Every page goes back to the server.
hkLargeBlockAllocator::~hkLargeBlockAllocator() {
	hkMemPage* pPage = m_pages.m_next;
	while (pPage != &m_pages) {
		hkMemPage* pNext = pPage->m_next;
		m_server->ourFree(pPage, pPage->m_size);
		pPage = pNext;
	}
}

// The request plus the chunk header, 16-aligned and at least 256 bytes, from the free chunks or the top. Failing that,
// a single-block server's one page is grown; any other server gives a new page (or, with no page yet, the single-block
// one too), filed by address, whose start becomes the top (the old top filed as a free chunk) and whose footer and the
// previous page's footer are linked up. The limited-memory listener is told before the one retry and on its failure.
void* hkLargeBlockAllocator::alloc(uint32_t bytes) {
	uint32_t nb = bytes + 31 & ~15u;
	if (nb < 256) {
		nb = 256;
	}
	void* result = _alloc(nb);
	if (result) {
		return result;
	}
	hkBool bResult;
	hkMemPage* pPage;
	if (m_server->isSingleBlockServer().m_bool && (pPage = m_pages.m_next) != &m_pages) {
		if (_resizeSingleBlockServerPage(&bResult, pPage->m_size + nb)->m_bool) {
			return _allocFromTop(nb);
		}
		if (m_limitedListener) {
			m_limitedListener->cannotAllocate(nb);
			result = _alloc(nb);
			if (result) {
				return result;
			}
			if (_resizeSingleBlockServerPage(&bResult, pPage->m_size + nb)->m_bool) {
				return _allocFromTop(nb);
			}
			m_limitedListener->allocationFailure(nb);
		}
		return nullptr;
	}
	uint32_t uiSizeOut = m_server->recommendSize(nb + 312);
	hkMemPage* pNewPage = static_cast<hkMemPage*>(m_server->allocate(uiSizeOut, uiSizeOut));
	if (!pNewPage) {
		if (!m_limitedListener) {
			return nullptr;
		}
		m_limitedListener->cannotAllocate(nb);
		result = _alloc(nb);
		if (result) {
			return result;
		}
		uiSizeOut = m_server->recommendSize(nb + 312);
		pNewPage = static_cast<hkMemPage*>(m_server->allocate(uiSizeOut, uiSizeOut));
		if (!pNewPage) {
			m_limitedListener->allocationFailure(nb);
			return nullptr;
		}
	}
	// The page's chunks start 16-aligned after its header and its end is rounded down to 16 (raw addresses).
	pNewPage->m_numAllocs = 0;
	pNewPage->m_start = reinterpret_cast<char*>(pNewPage + 1);
	pNewPage->m_size = uiSizeOut;
	pNewPage->m_end = reinterpret_cast<char*>(pNewPage) + uiSizeOut;
	pNewPage->m_end = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(pNewPage->m_end) & ~uintptr_t(15));
	pNewPage->m_start = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(pNewPage + 1) + 15 & ~uintptr_t(15));
	hkMemPage* pNext = m_pages.m_next;
	while (pNext != &m_pages && pNext < pNewPage) {
		pNext = pNext->m_next;
	}
	pNewPage->m_next = pNext;
	hkMemPage* pPrev = pNext->m_prev;
	pNewPage->m_prev = pPrev;
	pPrev->m_next = pNewPage;
	pNext->m_prev = pNewPage;
	if (m_top != &m_zeroChunk) {
		_makeTopValid();
		_insertLargeChunk(static_cast<hkMemTreeChunk*>(m_top), m_top->getChunkSize());
	}
	m_topsize = pNewPage->m_end - pNewPage->m_start - hkMemChunk::PAYLOAD_OFFSET;
	m_top = pNewPage->getFirstChunk();
	_makeTopValid();
	// A footer's head spans the gap to the next page's first chunk (or from the previous page's footer to the new top):
	// distances between raw chunk addresses.
	hkMemChunk* pFooter = pNewPage->getFooter();
	pFooter->head = hkMemChunk::CINUSE_BIT;
	if (pNewPage->m_next != &m_pages) {
		uint32_t uiGap = static_cast<uint32_t>(pNewPage->m_next->m_start - reinterpret_cast<char*>(pFooter));
		pFooter->head = uiGap | hkMemChunk::CINUSE_BIT;
	}
	if (pNewPage->m_prev != &m_pages) {
		hkMemChunk* pPrevFooter = pNewPage->m_prev->getFooter();
		uint32_t uiBits = pPrevFooter->head & hkMemChunk::INUSE_BITS;
		uint32_t uiGap = static_cast<uint32_t>(reinterpret_cast<char*>(m_top) - reinterpret_cast<char*>(pPrevFooter));
		pPrevFooter->head = uiGap | uiBits;
	}
	return _allocFromTop(nb);
}

// Frees the chunk of apPtr, merging it with a free neighbour on either side (the following one may be the top).
void hkLargeBlockAllocator::ourFree(void* apPtr) {
	if (apPtr) {
		hkMemChunk* pChunk = hkMemChunk::toChunk(apPtr);
		uint32_t s = pChunk->getChunkSize();
		m_sumAllocatedWithMgrOverhead -= s;
		m_sumAllocatedSize -= pChunk->getPayloadSize();
		hkMemChunk* pNext = pChunk->chunkPlusOffset(s);
		if (!pChunk->isPinuse()) {
			uint32_t uiPrevSize = pChunk->prevFoot;
			pChunk = pChunk->chunkMinusOffset(uiPrevSize);
			// A free chunk that is not the top is in a size tree.
			_unlinkLargeChunk(static_cast<hkMemTreeChunk*>(pChunk));
			s += uiPrevSize;
		}
		uint32_t uiNextHead = pNext->head;
		if ((uiNextHead & hkMemChunk::CINUSE_BIT) != 0) {
			pNext->head = uiNextHead & ~hkMemChunk::PINUSE_BIT;
			pChunk->head = s | hkMemChunk::PINUSE_BIT;
			pChunk->setFoot(s);
			_insertLargeChunk(static_cast<hkMemTreeChunk*>(pChunk), s);
		} else if (pNext == m_top) {
			m_topsize += s;
			m_top = pChunk;
			pChunk->head = m_topsize | hkMemChunk::PINUSE_BIT;
		} else {
			s += uiNextHead & ~hkMemChunk::INUSE_BITS;
			_unlinkLargeChunk(static_cast<hkMemTreeChunk*>(pNext));
			pChunk->head = s | hkMemChunk::PINUSE_BIT;
			pChunk->setFoot(s);
			_insertLargeChunk(static_cast<hkMemTreeChunk*>(pChunk), s);
		}
	}
}

// Whether apPtr is the payload of a used chunk of one of the pages: aligned, in use, inside a page, and reached by
// walking that page's chunks.
hkBool hkLargeBlockAllocator::isValidAlloc(const void* in) {
	hkBool result;
	if (!hkMemChunk::isAligned(in)) {
		result.m_bool = 0;
		return result;
	}
	const hkMemChunk* pChunk = hkMemChunk::toChunk(in);
	if (!pChunk->isInuse()) {
		result.m_bool = 0;
		return result;
	}
	const char* pcPtr = static_cast<const char*>(in);
	hkMemPage* pPage = m_pages.m_next;
	while (pPage != &m_pages) {
		if (pcPtr >= pPage->m_start && pcPtr < pPage->m_end) {
			break;
		}
		pPage = pPage->m_next;
	}
	if (pPage == &m_pages) {
		result.m_bool = 0;
		return result;
	}
	hkMemChunk* pCur = pPage->getFirstChunk();
	hkMemChunk* pFooter = pPage->getFooter();
	if (pCur == pFooter) {
		result.m_bool = 0;
		return result;
	}
	while (pCur != pChunk) {
		pCur = pCur->nextChunk();
		if (pCur == pFooter) {
			result.m_bool = 0;
			return result;
		}
	}
	result.m_bool = 1;
	return result;
}

uint32_t hkLargeBlockAllocator::getAllocatedSize(const void* mem, uint32_t a2) {
	return hkMemChunk::toChunk(mem)->getPayloadSize();
}

uint32_t hkLargeBlockAllocator::getAllocatedSizeWithOverhead(const void* mem, uint32_t a2) {
	return hkMemChunk::toChunk(mem)->getChunkSize();
}

void* hkLargeBlockAllocator::criticalAlloc(uint32_t size) {
	uint32_t* v3 = static_cast<uint32_t*>(alloc(size + 31 & 0xFFFFFFF0u));
	if (v3) {
		*v3 = 1;
		return v3 + 4;
	} else {
		uint32_t* v5 = static_cast<uint32_t*>(m_server->criticalAlloc(size + 31 & 0xFFFFFFF0u));
		if (v5) {
			*v5 = 0;
			return v5 + 4;
		} else {
			return 0;
		}
	}
}

// The 16 bytes before the block are criticalAlloc's header, its first word where the block came from: 1 this
// allocator (freed through the vtable), 0 the block server (freed with the size criticalAlloc asked it for).
void hkLargeBlockAllocator::criticalFree(void* ptr, uint32_t size) {
	uint32_t* puiHeader = static_cast<uint32_t*>(ptr) - 4;
	uint32_t uiFrom = *puiHeader;
	if (uiFrom == 0) {
		m_server->criticalFree(puiHeader, size + 31 & ~15u);
	} else if (uiFrom == 1) {
		ourFree(puiHeader);
	}
}

// Resets the allocator to empty: no totals, no tree bins, the top at the zero chunk (both in-use bits set) and the page
// list holding only its own head.
void hkLargeBlockAllocator::_init() {
	m_sumAllocatedSize = 0;
	m_sumAllocatedWithMgrOverhead = 0;
	m_treemap = 0;
	m_leastAddr = nullptr;
	for (int32_t i = 0; i < 32; ++i) {
		m_treebins[i] = nullptr;
	}
	m_zeroChunk.prevFoot = 0;
	m_topsize = 0;
	m_zeroChunk.head = hkMemChunk::INUSE_BITS;
	m_top = &m_zeroChunk;
	m_pages.m_next = &m_pages;
	m_pages.m_prev = &m_pages;
}

// Frees every page back to the block server, then resets the allocator.
void hkLargeBlockAllocator::quit() {
	hkMemPage* pPage = m_pages.m_next;
	while (pPage != &m_pages) {
		hkMemPage* pNext = pPage->m_next;
		m_server->ourFree(pPage, pPage->m_size);
		pPage = pNext;
	}
	_init();
}

uint32_t hkLargeBlockAllocator::_findLargestTreeBlockSize(hkMemTreeChunk* t, uint32_t largest) {
	while (t) {
		uint32_t uiSize = t->head & ~hkMemChunk::INUSE_BITS;
		if (uiSize > largest)
			largest = uiSize;
		t = t->child[1] ? t->child[1] : t->child[0];
	}
	return largest;
}

// The total available is always zero, as the game does.
void hkLargeBlockAllocator::calculateStatistics(hkMemoryAllocator::MemoryStatistics& stats) {
	uint32_t uiAllocated = calculateMemoryUsedByThisAllocator();
	stats.m_allocated = uiAllocated;
	stats.m_inUse = m_sumAllocatedSize;
	stats.m_available = uiAllocated - m_sumAllocatedSize;
	stats.m_largestBlock = findLargestBlockSize();
	stats.m_totalAvailable = 0;
}

uint32_t hkLargeBlockAllocator::findLargestBlockSize() {
	hkMemTreeChunk** i;
	uint32_t m_topsize = this->m_topsize;
	int32_t v3 = _computeTreeIndex(m_topsize);
	int32_t v4 = 31;
	if (v3 > 31) {
		return m_topsize;
	}
	for (i = &m_treebins[31]; !*i; --i) {
		if (--v4 < v3) {
			return m_topsize;
		}
	}
	return _findLargestTreeBlockSize(*i, m_topsize);
}

uint32_t hkLargeBlockAllocator::calculateMemoryUsedByThisAllocator() const {
	uint32_t result;
	hkMemPage* m_next = m_pages.m_next;
	const hkMemPage* p_m_pages = &m_pages;
	for (result = 0; m_next != p_m_pages; m_next = m_next->m_next) {
		result += m_next->m_size;
	}
	return result;
}

// dlmalloc's insert_large_chunk: an empty bin takes x as its root, whose parent is then the bin slot itself (the
// source's cast); otherwise x goes down the tree by its size bits, joining the ring of a chunk of the same size or
// becoming a new leaf.
void hkLargeBlockAllocator::_insertLargeChunk(hkMemTreeChunk* x, uint32_t s) {
	uint32_t index = hkLargeBlockAllocator::_computeTreeIndex(s);
	x->child[1] = nullptr;
	x->child[0] = nullptr;
	x->index = index;
	uint32_t uiTreemap = m_treemap;
	hkMemTreeChunk** ppBin = &m_treebins[index];
	if (uiTreemap & 1u << index) {
		hkMemTreeChunk* t = *ppBin;
		uint32_t k = s << (index == 31 ? 0 : 25 - (index >> 1));
		for (;;) {
			if (t->getChunkSize() == s) {
				hkFreeMemChunk* f = t->next;
				f->prev = x;
				t->next = x;
				x->prev = t;
				x->next = f;
				x->parent = nullptr;
				return;
			}
			hkMemTreeChunk** c = &t->child[k >> 31];
			k <<= 1;
			if (!*c) {
				*c = x;
				x->parent = t;
				x->next = x;
				x->prev = x;
				return;
			}
			t = *c;
		}
	}
	m_treemap = 1u << index | uiTreemap;
	*ppBin = x;
	x->parent = reinterpret_cast<hkMemTreeChunk*>(ppBin);
	x->next = x;
	x->prev = x;
}

void hkLargeBlockAllocator::_unlinkLargeChunk(hkMemTreeChunk* x) {
	hkMemTreeChunk* prev = static_cast<hkMemTreeChunk*>(x->prev);
	hkMemTreeChunk* parent = x->parent;
	if (prev == x) {
		prev = x->child[1];
		hkMemTreeChunk** v6 = &x->child[1];
		if (prev || (prev = x->child[0], (v6 = x->child, prev))) {
			while (1) {
				hkMemTreeChunk** child = &prev->child[1];
				if (!prev->child[1]) {
					child = prev->child;
					if (!prev->child[0]) {
						break;
					}
				}
				prev = *child;
				v6 = child;
			}
			*v6 = 0;
		}
	} else {
		hkFreeMemChunk* next = x->next;
		next->prev = prev;
		prev->next = next;
	}
	if (parent) {
		uint32_t index = x->index;
		bool v9 = x == m_treebins[index];
		hkMemTreeChunk** v10 = &m_treebins[index];
		if (v9) {
			*v10 = prev;
			if (!prev) {
				m_treemap &= ~(1 << x->index);
				return;
			}
			goto LABEL_17;
		}
		if (parent->child[0] == x) {
			parent->child[0] = prev;
		} else {
			parent->child[1] = prev;
		}
		if (prev) {
		LABEL_17:;
			prev->parent = parent;
			hkMemTreeChunk* v11 = x->child[0];
			if (v11) {
				prev->child[0] = v11;
				v11->parent = prev;
			}
			hkMemTreeChunk* v12 = x->child[1];
			if (v12) {
				prev->child[1] = v12;
				v12->parent = prev;
			}
		}
	}
}

void hkLargeBlockAllocator::_makeTopValid() const {
	m_top->head = m_topsize | hkMemChunk::PINUSE_BIT;
	m_top->chunkPlusOffset(m_topsize)->prevFoot = m_topsize;
}

// Asks the single-block server to resize its one page; on success the top grows by the 16-aligned growth and the
// page's footer chunk shrinks by as much, keeping its in-use bit.
hkBool* hkLargeBlockAllocator::_resizeSingleBlockServerPage(hkBool* apResult, uint32_t newSize) {
	uint32_t uiSizeOut;
	hkMemPage* pPage = m_pages.m_next;
	if (m_server->resize(pPage, pPage->m_size, newSize, uiSizeOut).m_bool) {
		uint32_t uiTopSize = m_topsize;
		uint32_t uiFooterHead = m_top->chunkPlusOffset(uiTopSize)->head;
		uint32_t uiSize = uiSizeOut;
		uint32_t uiGrowth = uiSizeOut - pPage->m_size & ~15u;
		m_topsize = uiGrowth + uiTopSize;
		pPage->m_end += uiGrowth;
		pPage->m_size = uiSize;
		m_top->chunkPlusOffset(m_topsize)->head =
			(uiFooterHead & ~hkMemChunk::INUSE_BITS) - uiGrowth | hkMemChunk::CINUSE_BIT;
		apResult->m_bool = 1;
		return apResult;
	} else {
		apResult->m_bool = 0;
		return apResult;
	}
}

// dlmalloc's tmalloc_large: the smallest tree chunk of at least nb bytes, from nb's own bin (walking the size bits,
// keeping the last right subtree passed over) or else the smallest of the next non-empty bin. The remainder is split
// off and filed back when it is a large chunk (256 bytes or more); otherwise the whole chunk is handed out.
void* hkLargeBlockAllocator::_allocLarge(uint32_t nb) {
	hkMemTreeChunk* v = nullptr;
	uint32_t rsize = -nb;
	uint32_t idx = hkLargeBlockAllocator::_computeTreeIndex(nb);
	hkMemTreeChunk* t = m_treebins[idx];
	if (t) {
		uint32_t sizebits = nb << (idx == 31 ? 0 : 25 - (idx >> 1));
		hkMemTreeChunk* rst = nullptr;
		for (;;) {
			uint32_t trem = t->getChunkSize() - nb;
			if (trem < rsize) {
				v = t;
				rsize = t->getChunkSize() - nb;
				if (!trem) {
					break;
				}
			}
			hkMemTreeChunk* rt = t->child[1];
			t = t->child[sizebits >> 31];
			if (rt && rt != t) {
				rst = rt;
			}
			if (!t) {
				t = rst;
				break;
			}
			sizebits <<= 1;
		}
	}
	if (!t && !v) {
		uint32_t leftbits = m_treemap & ((1u << idx) << 1 | -((1u << idx) << 1));
		if (leftbits) {
			uint32_t uiBin = hkLargeBlockAllocator::_bitToIndex(leftbits & -leftbits);
			t = m_treebins[uiBin];
		}
	}
	while (t) {
		if (t->getChunkSize() - nb < rsize) {
			rsize = t->getChunkSize() - nb;
			v = t;
		}
		if (t->child[0]) {
			t = t->child[0];
		} else {
			t = t->child[1];
		}
	}
	if (!v) {
		return nullptr;
	}
	hkMemTreeChunk* r = static_cast<hkMemTreeChunk*>(v->chunkPlusOffset(nb));
	_unlinkLargeChunk(v);
	if (rsize >= 256) {
		v->head = nb | hkMemChunk::INUSE_BITS;
		m_sumAllocatedWithMgrOverhead += nb;
		m_sumAllocatedSize += nb - hkMemChunk::PAYLOAD_OFFSET;
		r->head = rsize | hkMemChunk::PINUSE_BIT;
		r->setFoot(rsize);
		_insertLargeChunk(r, rsize);
	} else {
		v->head = rsize + nb | hkMemChunk::INUSE_BITS;
		v->chunkPlusOffset(rsize + nb)->head |= hkMemChunk::PINUSE_BIT;
		m_sumAllocatedWithMgrOverhead += rsize + nb;
		m_sumAllocatedSize += rsize + nb - hkMemChunk::PAYLOAD_OFFSET;
	}
	return v->getPayload();
}

// Carves bytes off the top chunk: the rest becomes the new top, its previous-in-use bit set.
void* hkLargeBlockAllocator::_allocFromTop(uint32_t nb) {
	m_topsize -= nb;
	m_sumAllocatedWithMgrOverhead += nb;
	hkMemChunk* pChunk = m_top;
	m_sumAllocatedSize += nb - hkMemChunk::PAYLOAD_OFFSET;
	uint32_t uiTopSize = m_topsize;
	hkMemChunk* pNewTop = pChunk->chunkPlusOffset(nb);
	m_top = pNewTop;
	pNewTop->prevFoot = nb;
	pNewTop->head = uiTopSize | hkMemChunk::PINUSE_BIT;
	pChunk->head = nb | hkMemChunk::INUSE_BITS;
	return pChunk->getPayload();
}

void* hkLargeBlockAllocator::_alloc(uint32_t nb) {
	void* result;
	if (!m_treemap || (result = _allocLarge(nb)) == 0) {
		if (nb + 256 >= m_topsize) {
			return 0;
		} else {
			return _allocFromTop(nb);
		}
	}
	return result;
}

// Gives every page holding no allocation back to the block server, then makes the largest free chunk ending a page
// the new top (a single-block server's page is shrunk to the top instead).
void hkLargeBlockAllocator::garbageCollect() {
	_makeTopValid();
	hkMemPage* pPage = m_pages.m_next;
	while (pPage != &m_pages) {
		hkMemChunk* pChunk = pPage->getFirstChunk();
		// A page whose first chunk is free and runs to the footer is empty.
		if (!pChunk->isInuse() && pChunk->nextChunk() == pPage->getFooter()) {
			hkMemPage* pNext = pPage->m_next;
			pPage->m_prev->m_next = pNext;
			pPage->m_next->m_prev = pPage->m_prev;
			if (pChunk == m_top) {
				m_top = &m_zeroChunk;
				m_topsize = 0;
			} else {
				// A free chunk that is not the top is in a size tree.
				_unlinkLargeChunk(static_cast<hkMemTreeChunk*>(pChunk));
			}
			m_server->ourFree(pPage, pPage->m_size);
			pPage = pNext;
		} else {
			pPage = pPage->m_next;
		}
	}
	if (m_pages.m_next == &m_pages) {
		m_top = &m_zeroChunk;
		m_topsize = 0;
		return;
	}
	hkBool bResult = m_server->isSingleBlockServer();
	if (bResult.m_bool) {
		if (m_topsize >= 0x8000) {
			// The top's offset in the page's raw memory, plus 0x120, as the game does.
			_resizeSingleBlockServerPage(&bResult,
				static_cast<uint32_t>(reinterpret_cast<char*>(m_top) - m_pages.m_next->m_start + 0x120));
		}
		return;
	}
	if (m_top != &m_zeroChunk) {
		_insertLargeChunk(static_cast<hkMemTreeChunk*>(m_top), m_topsize);
		m_top = &m_zeroChunk;
		m_topsize = 0;
	}
	// The largest free chunk that ends a page (its footer's previous chunk is free).
	hkMemChunk* pLargest = nullptr;
	for (hkMemPage* pIter = m_pages.m_next; pIter != &m_pages; pIter = pIter->m_next) {
		hkMemChunk* pFooter = pIter->getFooter();
		if (!pFooter->isPinuse()) {
			hkMemChunk* pFree = pFooter->previousChunk();
			if (!pLargest || pFree->getChunkSize() > pLargest->getChunkSize()) {
				pLargest = pFree;
			}
		}
	}
	if (!pLargest) {
		m_top = &m_zeroChunk;
		m_topsize = 0;
	} else {
		_unlinkLargeChunk(static_cast<hkMemTreeChunk*>(pLargest));
		m_top = pLargest;
		m_topsize = pLargest->getChunkSize();
	}
}

// Makes the top's header valid, then starts at the first chunk of the first page. With no pages only m_chunk is
// written (the SDK's default Iterator).
hkLargeBlockAllocator::Iterator* hkLargeBlockAllocator::getIterator(hkLargeBlockAllocator::Iterator* apResult) {
	m_top->head = m_topsize | hkMemChunk::PINUSE_BIT;
	hkMemPage* pPage = m_pages.m_next;
	if (pPage == &m_pages) {
		apResult->m_chunk = nullptr;
	} else {
		apResult->m_page = pPage;
		apResult->m_chunk = pPage->getFirstChunk();
		apResult->m_end = pPage->getFooter();
	}
	return apResult;
}

hkBool* hkLargeBlockAllocator::nextBlock(hkBool* apResult, hkLargeBlockAllocator::Iterator& arIter) {
	if (!arIter.m_chunk) {
		apResult->m_bool = 0;
		return apResult;
	}
	arIter.m_chunk = arIter.m_chunk->nextChunk();
	if (arIter.m_chunk == arIter.m_end) {
		hkMemPage* pPage = arIter.m_page->m_next;
		if (pPage == &m_pages) {
			arIter.m_chunk = nullptr;
			apResult->m_bool = 0;
			return apResult;
		}
		arIter.m_page = pPage;
		arIter.m_chunk = pPage->getFirstChunk();
		arIter.m_end = pPage->getFooter();
	}
	apResult->m_bool = 1;
	return apResult;
}

// The bytes of every used block of every page, after making the top's header valid.
uint32_t hkLargeBlockAllocator::_calculateUsed() {
	uint32_t uiUsed = 0;
	_makeTopValid();
	for (hkMemPage* pPage = m_pages.m_next; pPage != &m_pages; pPage = pPage->m_next) {
		hkMemChunk* pEnd = pPage->getFooter();
		for (hkMemChunk* pChunk = pPage->getFirstChunk(); pChunk != pEnd; pChunk = pChunk->nextChunk()) {
			if (pChunk->isInuse()) {
				uiUsed += pChunk->getChunkSize();
			}
		}
	}
	return uiUsed;
}

// Whether mem is a live allocation of this allocator: an aligned payload inside the heap whose chunk is used, at least
// a header long, marked used by the next chunk and, when the previous chunk is free, sized as that chunk's foot says.
bool hkLargeBlockAllocator::_checkUsedAlloc(const void* mem) {
	hkMemChunk* p = hkMemChunk::toChunk(const_cast<void*>(mem));
	if (!hkMemChunk::isAligned(mem)) {
		return false;
	}
	if (reinterpret_cast<char*>(p) < m_leastAddr) {
		return false;
	}
	if (!p->isInuse()) {
		return false;
	}
	uint32_t uiSize = p->getChunkSize();
	if (uiSize < hkMemChunk::PAYLOAD_OFFSET || !p->chunkPlusOffset(uiSize)->isPinuse()) {
		return false;
	}
	if (!p->isPinuse()) {
		hkMemChunk* pPrev = p->previousChunk();
		if (pPrev->isInuse() || pPrev->getChunkSize() != p->prevFoot) {
			return false;
		}
	}
	return true;
}

// Whether p is a well-formed free chunk: aligned, free, inside the heap, followed by a used chunk that knows it is free
// and preceded by a used one.
hkBool* hkLargeBlockAllocator::_checkFreeChunk(hkBool* apResult, hkMemChunk* p) {
	if (!hkMemChunk::isAligned(p) || p->isInuse() || reinterpret_cast<char*>(p) < m_leastAddr) {
		apResult->m_bool = 0;
		return apResult;
	}
	hkMemChunk* pNext = p->nextChunk();
	if (pNext->isPinuse() || !pNext->isInuse() || !p->isPinuse()) {
		apResult->m_bool = 0;
		return apResult;
	}
	apResult->m_bool = 1;
	return apResult;
}

hkBool* hkLargeBlockAllocator::checkAllocations(hkBool* apResult, void** allocs, int32_t size) {
	int32_t v4 = 0;
	if (size <= 0) {
	LABEL_4:;
		apResult->m_bool = 1;
		return apResult;
	} else {
		while (_checkUsedAlloc(allocs[v4])) {
			if (++v4 >= size) {
				goto LABEL_4;
			}
		}
		apResult->m_bool = 0;
		return apResult;
	}
}

// Calls callback with the payload and payload size of every used block of every page. The block's size is read before
// the call.
void hkLargeBlockAllocator::forAllAllocs(void (*callback)(void*, uint32_t, void*), void* param) {
	// The top's header only, as getIterator writes it (no foot, unlike _makeTopValid).
	m_top->head = m_topsize | hkMemChunk::PINUSE_BIT;
	for (hkMemPage* pPage = m_pages.m_next; pPage != &m_pages; pPage = pPage->m_next) {
		hkMemChunk* pEnd = pPage->getFooter();
		for (hkMemChunk* pChunk = pPage->getFirstChunk(); pChunk != pEnd;) {
			uint32_t uiSize = pChunk->getChunkSize();
			if (pChunk->isInuse()) {
				callback(pChunk->getPayload(), pChunk->getPayloadSize(), param);
			}
			pChunk = pChunk->chunkPlusOffset(uiSize);
		}
	}
}

// forAllAllocs' callback that collects the allocations: param is the hkArray<void*> they are appended to.
void hkLargeBlockAllocator::_addAlloc(void* data, uint32_t size, void* param) {
	hkArray<void*>* pArray = static_cast<hkArray<void*>*>(param);
	// EXE-INLINED 0x6299C0
	hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
	if (pArray->m_size == pArray->getCapacity()) {
		hkArrayUtil::_reserveMore(kHeap, pArray, sizeof(void*));
	}
	pArray->m_data[pArray->m_size++] = data;
}

uint32_t hkLargeBlockAllocator::_bitToIndex(uint32_t x) {
	uint32_t v1 = ((((uint32_t)(x - 1)) >> ((((uint32_t)(x - 1)) >> 12) & 16)) >> 5) & 8;
	uint32_t v2 = (((uint32_t)(x - 1)) >> ((((uint32_t)(x - 1)) >> 12) & 16)) >> v1;
	uint32_t v3 = v1 + ((((uint32_t)(x - 1)) >> 12) & 16);
	int32_t v4 = (int32_t)((v2 >> 2) & 4);
	uint32_t v5 = v2 >> v4;
	return (((v5 >> ((v5 >> 1) & 2)) >> 1) & 1) +
		((((v5 >> 1) & 2) + (v4 + v3)) + ((v5 >> ((v5 >> 1) & 2)) >> ((uint8_t)(((v5 >> ((v5 >> 1) & 2)) & 2) != 0))));
}

uint32_t hkLargeBlockAllocator::_computeTreeIndex(uint32_t s) {
	uint32_t v2;
	uint32_t v3;
	uint32_t v4;
	uint32_t result = s >> 8;
	if (s >> 8) {
		if (result <= 65535) {
			v2 = ((result - 256) >> 16) & 8;
			v3 = result << (((result - 256) >> 16) & 8);
			v4 = (((((v3 << (((v3 - 4096) >> 16) & 4)) << ((((v3 << (((v3 - 4096) >> 16) & 4)) - 16384) >> 16) & 2)) >>
					   15) -
					  ((((v3 << (((v3 - 4096) >> 16) & 4)) - 16384) >> 16) & 2)) -
					 ((((v3 - 4096) >> 16) & 4) + v2)) +
				14;
			return ((s >> (v4 + 7)) & 1) + (2 * v4);
		} else {
			return (uint32_t)31;
		}
	}
	return result;
}
