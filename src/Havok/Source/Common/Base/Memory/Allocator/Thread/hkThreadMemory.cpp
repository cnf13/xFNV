#include "Havok/Source/Common/Base/Memory/Allocator/Thread/hkThreadMemory.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"

// The exe's constructor: the vtable, 17 empty free lists (MEMORY_MAX_ALL_ROW rows), the lookup tables, then a cleared
// memory system with a limit of 8 blocks per free list. It takes nothing but this and returns it in eax.
hkThreadMemory::hkThreadMemory() {
	for (int32_t i = 0; i < MEMORY_MAX_ALL_ROW; ++i) {
		m_free_list[i].m_head = nullptr;
		m_free_list[i].m_numElem = 0;
	}
	initLookupTables();
	setMemory(nullptr, 8);
}

// As the exe's constructor (0xD2C760): 17 empty free lists, the lookup tables, then memoryInstance as the memory
// with a limit of 8 blocks per free list.
hkThreadMemory::hkThreadMemory(hkMemoryAllocator* memoryInstance) {
	for (int32_t i = 0; i < MEMORY_MAX_ALL_ROW; ++i) {
		m_free_list[i].m_head = nullptr;
		m_free_list[i].m_numElem = 0;
	}
	initLookupTables();
	setMemory(memoryInstance, 8);
}

// Sizes above 0x2000 go to the memory system; the row comes from the small table (signed char, 16-byte steps up to
// 640) or the large one (1 KB steps). The exe tail-jumps to both fallbacks.
void* hkThreadMemory::blockAlloc(int32_t nbytes) {
	if (nbytes > 0x2000)
		return m_memory->blockAlloc(nbytes);
	int32_t iRow;
	if (nbytes > 640)
		iRow = m_large_size_to_row_lut[(nbytes - 1) >> 10];
	else
		iRow = m_small_size_to_row_lut[(nbytes + 15) >> 4];
	FreeList::FreeElem* pBlock = m_free_list[iRow].m_head;
	if (pBlock == nullptr)
		return onRowEmpty(iRow);
	m_free_list[iRow].m_numElem--;
	m_free_list[iRow].m_head = pBlock->m_next;
	return pBlock;
}

// Sizes above 0x2000 go back to the memory system. Otherwise the row comes from the same tables as blockAlloc and
// the block is pushed on the row's free list; a full list (the count signed-compared with the limit) hands the block
// to onRowFull instead. The list is linked through the block's own first word.
void hkThreadMemory::blockFree(void* p, int32_t nbytes) {
	if (p == nullptr)
		return;
	if (nbytes > 0x2000) {
		m_memory->blockFree(p, nbytes);
		return;
	}
	int32_t iRow;
	if (nbytes > 640)
		iRow = m_large_size_to_row_lut[(nbytes - 1) >> 10];
	else
		iRow = m_small_size_to_row_lut[(nbytes + 15) >> 4];
	if (m_free_list[iRow].m_numElem < m_maxNumElemsOnFreeList) {
		// The exe reads the head, counts the block, stores the head into the block, then links it in.
		FreeList::FreeElem* pHead = m_free_list[iRow].m_head;
		m_free_list[iRow].m_numElem++;
		FreeList::FreeElem* pElem = static_cast<FreeList::FreeElem*>(p);
		pElem->m_next = pHead;
		m_free_list[iRow].m_head = pElem;
	} else {
		onRowFull(iRow, p);
	}
}

// bufAlloc is the SDK's size-in form of blockAlloc: the requested bytes become the block size, and the exe tail-jumps
// to this vtable's slot 1 (blockAlloc).
void* hkThreadMemory::bufAlloc(int32_t& reqNumInOut) {
	return blockAlloc(reqNumInOut);
}

// bufFree is blockFree with the same two arguments; the exe tail-jumps to this vtable's slot 2 (blockFree).
void hkThreadMemory::bufFree(void* p, int32_t numElem) {
	blockFree(p, numElem);
}

// The new buffer is allocated before the old one is copied into it and freed.
void* hkThreadMemory::bufRealloc(void* pold, int32_t oldNum, int32_t& reqNumInOut) {
	void* pNewMem = blockAlloc(reqNumInOut);
	int32_t iNumBytes = reqNumInOut < oldNum ? reqNumInOut : oldNum;
	hkMemUtil::memCpy(pNewMem, pold, iNumBytes);
	blockFree(pold, oldNum);
	return pNewMem;
}

void hkThreadMemory::blockAllocBatch(void** ptrsOut, int32_t numPtrs, int32_t blockSize) {
	if (blockSize > 0x2000) {
		m_memory->blockAllocBatch(ptrsOut, numPtrs, blockSize);
	} else {
		for (int32_t i = 0; i < numPtrs; ++i) {
			ptrsOut[i] = blockAlloc(blockSize);
		}
	}
}

void hkThreadMemory::blockFreeBatch(void** ptrsIn, int32_t numPtrs, int32_t blockSize) {
	if (blockSize > 0x2000) {
		m_memory->blockFreeBatch(ptrsIn, numPtrs, blockSize);
	} else {
		for (int32_t i = 0; i < numPtrs; ++i) {
			blockFree(ptrsIn[i], blockSize);
		}
	}
}

// The original tail-jumps to the memory system's canAllocTotal.
bool hkThreadMemory::canAllocTotal(int32_t numBytes) {
	return m_memory->canAllocTotal(numBytes);
}

// The statistics come from the memory system the thread memory points at (its vtable slot 10).
void hkThreadMemory::getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& u) {
	m_memory->getMemoryStatistics(u);
}

// The block's size is its row's size, not the requested one; above 0x2000 it is the memory system's own figure.
int32_t hkThreadMemory::getAllocatedSize(const void* obj, int32_t numBytes) {
	int32_t iNumBytes = numBytes;
	if (numBytes <= 0x2000) {
		int32_t iRow;
		if (numBytes > 640)
			iRow = m_large_size_to_row_lut[(numBytes - 1) >> 10];
		else
			iRow = m_small_size_to_row_lut[(numBytes + 15) >> 4];
		iNumBytes = m_row_to_size_lut[iRow];
	}
	return m_memory->getAllocatedSize(obj, iNumBytes);
}

// Hands every non-empty free list's blocks back, from the last row down to row 0.
void hkThreadMemory::releaseCachedMemory() {
	for (int32_t i = MEMORY_MAX_ALL_ROW - 1; i >= 0; --i) {
		if (m_free_list[i].m_numElem) {
			clearRow(i);
		}
	}
}

// The bytes the free lists hold.
uint32_t hkThreadMemory::getCachedSizeUnchecked() {
	int32_t v1 = 0;
	int32_t v2 = 0;
	// Indexed by element: FreeList holds a pointer, so its size differs on x64.
	for (int32_t i = 0; i < 16; i += 2) {
		v1 += m_free_list[i].m_numElem * m_row_to_size_lut[i];
		v2 += m_free_list[i + 1].m_numElem * m_row_to_size_lut[i + 1];
	}
	return m_free_list[16].m_numElem * m_row_to_size_lut[16] + (v2 + v1);
}

// Takes a block whose row's free list is full: without a limit the block goes straight
// back to the memory; otherwise the list is cut to half the limit in batches of BATCH_SIZE, then takes the block.
void hkThreadMemory::onRowFull(int32_t rowIndex, void* p) {
	int32_t iMaxNumElems = m_maxNumElemsOnFreeList;
	int32_t iBlockSize = m_row_to_size_lut[rowIndex];
	if (!iMaxNumElems) {
		m_memory->blockFree(p, iBlockSize);
		return;
	}
	int32_t iTarget = iMaxNumElems / 2;
	int32_t iNumElem = m_free_list[rowIndex].m_numElem;
	while (iNumElem > iTarget) {
		void* apPtrs[BATCH_SIZE];
		int32_t iNum = iNumElem - iTarget;
		if (iNum > BATCH_SIZE) {
			iNum = BATCH_SIZE;
		}
		// The SDK's inline FreeList::get: the head, or null on an empty list.
		for (int32_t i = 0; i < iNum; ++i) {
			FreeList::FreeElem* pElem = m_free_list[rowIndex].m_head;
			if (pElem) {
				--m_free_list[rowIndex].m_numElem;
				m_free_list[rowIndex].m_head = pElem->m_next;
			} else {
				pElem = nullptr;
			}
			apPtrs[i] = pElem;
		}
		iNumElem -= iNum;
		m_memory->blockFreeBatch(apPtrs, iNum, iBlockSize);
	}
	// The SDK's inline FreeList::put.
	FreeList::FreeElem* pHead = m_free_list[rowIndex].m_head;
	++m_free_list[rowIndex].m_numElem;
	FreeList::FreeElem* pElem = static_cast<FreeList::FreeElem*>(p);
	pElem->m_next = pHead;
	m_free_list[rowIndex].m_head = pElem;
}

// Frees every block of the row's free list in batches of BATCH_SIZE.
void hkThreadMemory::clearRow(int32_t rowIndex) {
	int32_t iNumElem = m_free_list[rowIndex].m_numElem;
	int32_t iBlockSize = m_row_to_size_lut[rowIndex];
	while (iNumElem > 0) {
		void* apPtrs[BATCH_SIZE];
		int32_t iNum = BATCH_SIZE;
		if (iNumElem <= BATCH_SIZE) {
			iNum = iNumElem;
		}
		// The SDK's inline FreeList::get: the head, or null on an empty list.
		for (int32_t i = 0; i < iNum; ++i) {
			FreeList::FreeElem* pElem = m_free_list[rowIndex].m_head;
			if (pElem) {
				--m_free_list[rowIndex].m_numElem;
				m_free_list[rowIndex].m_head = pElem->m_next;
			} else {
				pElem = nullptr;
			}
			apPtrs[i] = pElem;
		}
		iNumElem -= iNum;
		m_memory->blockFreeBatch(apPtrs, iNum, iBlockSize);
	}
}

// Fills m_row_to_size_lut and the two size-to-row tables.
void hkThreadMemory::initLookupTables() {
	for (int32_t i = 0; i <= 40; ++i) {
		int32_t v3;
		int32_t v2 = 16 * i;
		if (16 * i > 16) {
			if (v2 > 32) {
				if (v2 > 48) {
					if (v2 > 64) {
						if (v2 > 96) {
							if (v2 > 128) {
								if (v2 > 160) {
									if (v2 > 192) {
										if (v2 > 256) {
											if (v2 > 320) {
												if (v2 > 512) {
													if (v2 > 640) {
														if (v2 > 1024) {
															if (v2 > 2048) {
																if (v2 > 0x1000) {
																	if (v2 > 0x2000) {
																		__builtin_debugtrap();
																		v3 = -1;
																	} else {
																		v3 = 16;
																	}
																} else {
																	v3 = 15;
																}
															} else {
																v3 = 14;
															}
														} else {
															v3 = 13;
														}
													} else {
														v3 = 12;
													}
												} else {
													v3 = 11;
												}
											} else {
												v3 = 10;
											}
										} else {
											v3 = 9;
										}
									} else {
										v3 = 8;
									}
								} else {
									v3 = 7;
								}
							} else {
								v3 = 6;
							}
						} else {
							v3 = 5;
						}
					} else {
						v3 = 4;
					}
				} else {
					v3 = 3;
				}
			} else {
				v3 = 2;
			}
		} else {
			v3 = 1;
		}
		m_small_size_to_row_lut[i] = v3;
		m_row_to_size_lut[v3] = v2;
	}
	int32_t v4 = 1024;
	int32_t* m_large_size_to_row_lut = this->m_large_size_to_row_lut;
	do {
		int32_t v6;
		if (v4 > 16) {
			if (v4 > 32) {
				if (v4 > 48) {
					if (v4 > 64) {
						if (v4 > 96) {
							if (v4 > 128) {
								if (v4 > 160) {
									if (v4 > 192) {
										if (v4 > 256) {
											if (v4 > 320) {
												if (v4 > 512) {
													if (v4 > 640) {
														if (v4 > 1024) {
															if (v4 > 2048) {
																if (v4 > 0x1000) {
																	if (v4 > 0x2000) {
																		__builtin_debugtrap();
																		v6 = -1;
																	} else {
																		v6 = 16;
																	}
																} else {
																	v6 = 15;
																}
															} else {
																v6 = 14;
															}
														} else {
															v6 = 13;
														}
													} else {
														v6 = 12;
													}
												} else {
													v6 = 11;
												}
											} else {
												v6 = 10;
											}
										} else {
											v6 = 9;
										}
									} else {
										v6 = 8;
									}
								} else {
									v6 = 7;
								}
							} else {
								v6 = 6;
							}
						} else {
							v6 = 5;
						}
					} else {
						v6 = 4;
					}
				} else {
					v6 = 3;
				}
			} else {
				v6 = 2;
			}
		} else {
			v6 = 1;
		}
		*m_large_size_to_row_lut = v6;
		m_row_to_size_lut[v6] = v4;
		v4 += 1024;
		++m_large_size_to_row_lut;
	} while (v4 < 0x2400);
}

// The limit only follows a non-null allocator; a null one clears the allocator alone.
void hkThreadMemory::setMemory(hkMemoryAllocator* memoryInstance, int32_t maxNumElemsOnFreeList) {
	if (memoryInstance != nullptr) {
		m_memory = memoryInstance;
		m_maxNumElemsOnFreeList = maxNumElemsOnFreeList;
	} else {
		m_memory = nullptr;
	}
}

void* hkThreadMemory::onRowEmpty(int32_t rowIndex) {
	hkThreadMemory::FreeList::FreeElem* v6;
	void* v8[4];
	int32_t m_maxNumElemsOnFreeList = this->m_maxNumElemsOnFreeList;
	if (!m_maxNumElemsOnFreeList) {
		return m_memory->blockAlloc(m_row_to_size_lut[rowIndex]);
	}
	if (m_maxNumElemsOnFreeList >= 4) {
		m_maxNumElemsOnFreeList = 4;
	}
	m_memory->blockAllocBatch(v8, m_maxNumElemsOnFreeList, m_row_to_size_lut[rowIndex]);
	for (int32_t i = 1; i < m_maxNumElemsOnFreeList; m_free_list[rowIndex].m_head = v6) {
		v6 = static_cast<hkThreadMemory::FreeList::FreeElem*>(v8[i]);
		hkThreadMemory::FreeList::FreeElem* m_head = m_free_list[rowIndex].m_head;
		++m_free_list[rowIndex].m_numElem;
		++i;
		v6->m_next = m_head;
	}
	return v8[0];
}
