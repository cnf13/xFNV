#include "Havok/Source/Common/Base/Algorithm/Collide/1AxisSweep/hk1AxisSweep.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkRadixSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <bit>

void hk1AxisSweep::AabbInt::set(const hkAabb& aabbIn, int32_t key) {
	m_min[0] = (std::bit_cast<int32_t>(aabbIn.m_min.m_quad[0]) ^
				   (std::bit_cast<int32_t>(aabbIn.m_min.m_quad[0]) >> 31 | 0x80000001u)) >>
		1;
	m_min[1] = (std::bit_cast<int32_t>(aabbIn.m_min.m_quad[1]) ^
				   (std::bit_cast<int32_t>(aabbIn.m_min.m_quad[1]) >> 31 | 0x80000001u)) >>
		1;
	m_min[2] = (std::bit_cast<int32_t>(aabbIn.m_min.m_quad[2]) ^
				   (std::bit_cast<int32_t>(aabbIn.m_min.m_quad[2]) >> 31 | 0x80000001u)) >>
		1;
	getKey() = key;
	m_max[0u] = ((std::bit_cast<int32_t>(aabbIn.m_max.m_quad[0]) ^
					 (std::bit_cast<int32_t>(aabbIn.m_max.m_quad[0]) >> 31 | 0x80000001u)) >>
					1) +
		1;
	m_max[1] = ((std::bit_cast<int32_t>(aabbIn.m_max.m_quad[1]) ^
					(std::bit_cast<int32_t>(aabbIn.m_max.m_quad[1]) >> 31 | 0x80000001u)) >>
				   1) +
		1;
	m_max[2] = ((std::bit_cast<int32_t>(aabbIn.m_max.m_quad[2]) ^
					(std::bit_cast<int32_t>(aabbIn.m_max.m_quad[2]) >> 31 | 0x80000001u)) >>
				   1) +
		1;
}

// Walks both lists in x order; each step takes the box of the list whose min x is smaller and tests it against the
// other list's boxes four at a time (the sign bits of the min/max differences in y and z), until that list's min x
// passes its max x. The words are the boxes' m_min[0..3] (the key in m_min[3]) and m_max[0..2], 8 words per box.
int32_t hk1AxisSweep::collide(const hk1AxisSweep::AabbInt* pa, int32_t numA, const hk1AxisSweep::AabbInt* pb,
	int32_t numB, hkKeyPair* pairsOut, int32_t maxNumPairs, hkPadSpu<int32_t>& numPairsSkipped) {
	const uint32_t* a1 = pa->m_min;
	const uint32_t* a3 = pb->m_min;
	int32_t* a7 = &numPairsSkipped.m_storage;
	int32_t v18;
	hkKeyPair* v7 = pairsOut;
	const uint32_t* v8 = a1;
	*a7 = 0;
	const uint32_t* v9 = a3;
	hkKeyPair* const pairsEnd = &pairsOut[maxNumPairs];
	do {
		hkKeyPair* v35 = v7;
		if (*v8 <= *v9) {
			uint32_t v37 = v8[4];
			if (*v9 < v37) {
				const uint32_t* v19 = v9 + 9;
				do {
					uint32_t v20 = v8[6];
					uint32_t v21 = v8[5];
					uint32_t v22 = v8[2];
					uint32_t v23 = v8[1];
					uint32_t v41 = v22;
					int32_t v24 = *(v19 - 3) - v22 | (*(v19 - 4) - v23 | (v20 - *(v19 - 7) | v21 - *(v19 - 8)));
					int32_t v25 = v19[4] - v23 | v20 - v19[1];
					uint32_t v39 = v20;
					int32_t v26 = v20 - v19[9] | v21 - v19[8];
					uint32_t v27 = v19[12] - v23;
					v8 = a1;
					uint32_t v28 = v24 & 0x80000000u;
					uint32_t v29 = (v19[13] - a1[2] | (v27 | v26)) & 0x80000000u;
					uint32_t v30 = (v21 - *v19 | (v19[5] - v41 | v25)) & 0x80000000u;
					uint32_t v31 =
						(v19[21] - a1[2] | (v19[20] - a1[1] | (v39 - v19[17] | v21 - v19[16]))) & 0x80000000u;
					if ((v28 & (v29 & v31) & v30) != 0) {
						v7 = v35;
					} else {
						int32_t* v32;
						if (v28) {
							v32 = a7;
							v7 = v35;
						} else {
							v7 = v35;
							if (v35 >= pairsEnd) {
								v32 = a7;
								++*a7;
							} else {
								v35->m_keyA = a1[3];
								v35->m_keyB = *(v19 - 6);
								v7 = v35 + 1;
								v32 = a7;
							}
							v35 = v7;
						}
						if (!v30 && *(v19 - 1) <= v37) {
							if (v7 >= pairsEnd) {
								++*v32;
							} else {
								v7->m_keyA = a1[3];
								v7->m_keyB = v19[2];
								++v7;
							}
							v35 = v7;
						}
						if (!v29 && v19[7] <= v37) {
							if (v7 >= pairsEnd) {
								++*v32;
							} else {
								v7->m_keyA = a1[3];
								v7->m_keyB = v19[10];
								++v7;
							}
							v35 = v7;
						}
						if (!v31 && v19[15] <= v37) {
							if (v7 >= pairsEnd) {
								++*v32;
							} else {
								uint32_t v33 = v19[18];
								v7->m_keyA = a1[3];
								v7->m_keyB = v33;
								++v7;
							}
							v35 = v7;
						}
					}
					v19 += 32;
				} while (*(v19 - 9) < v37);
				v9 = a3;
			}
			v8 += 8;
			v18 = numA - 1;
			a1 = v8;
			--numA;
		} else {
			uint32_t v36 = v9[4];
			if (*v8 < v36) {
				const uint32_t* v10 = v8 + 9;
				do {
					uint32_t v11 = v9[5];
					uint32_t v12 = v9[1];
					uint32_t v38 = v9[2];
					uint32_t v40 = v9[6];
					uint32_t v13 =
						(*(v10 - 3) - v38 | (*(v10 - 4) - v12 | (v40 - *(v10 - 7) | v11 - *(v10 - 8)))) & 0x80000000u;
					uint32_t v14 = (v11 - *v10 | (v10[5] - v38 | (v10[4] - v12 | v40 - v10[1]))) & 0x80000000u;
					uint32_t v15 = (v10[13] - a3[2] | (v10[12] - v12 | (v40 - v10[9] | v11 - v10[8]))) & 0x80000000u;
					uint32_t v16 =
						(v10[21] - a3[2] | (v10[20] - a3[1] | (v40 - v10[17] | v11 - v10[16]))) & 0x80000000u;
					v7 = v35;
					if ((v14 & (v15 & v16) & v13) != 0) {
						v9 = a3;
					} else {
						int32_t* v17;
						if (v13) {
							v17 = a7;
							v9 = a3;
						} else {
							if (v35 >= pairsEnd) {
								v17 = a7;
								++*a7;
								v9 = a3;
							} else {
								v35->m_keyA = *(v10 - 6);
								v9 = a3;
								v35->m_keyB = a3[3];
								v17 = a7;
								v7 = v35 + 1;
							}
							v35 = v7;
						}
						if (!v14 && *(v10 - 1) <= v36) {
							if (v7 >= pairsEnd) {
								++*v17;
							} else {
								v7->m_keyA = v10[2];
								v7->m_keyB = v9[3];
								++v7;
							}
							v35 = v7;
						}
						if (!v15 && v10[7] <= v36) {
							if (v7 >= pairsEnd) {
								++*v17;
							} else {
								v7->m_keyA = v10[10];
								v7->m_keyB = v9[3];
								++v7;
							}
							v35 = v7;
						}
						if (!v16 && v10[15] <= v36) {
							if (v7 >= pairsEnd) {
								++*v17;
							} else {
								v7->m_keyA = v10[18];
								v7->m_keyB = v9[3];
								++v7;
							}
							v35 = v7;
						}
					}
					v10 += 32;
				} while (*(v10 - 9) < v36);
				v8 = a1;
			}
			v9 += 8;
			v18 = numB - 1;
			a3 = v9;
			--numB;
		}
	} while (v18 > 0);
	return static_cast<int32_t>(v7 - pairsOut);
}

// Havok SDK: each box against the boxes after it, four at a time while their min x is below its max x; the first of
// the four needs no x test of its own (the loop's), the other three are tested one by one. Pairs past maxNumPairs
// are counted in numPairsSkipped.
int32_t hk1AxisSweep::collide(const hk1AxisSweep::AabbInt* pa, int32_t numA, hkKeyPair* pairsOut, int32_t maxNumPairs,
	hkPadSpu<int32_t>& numPairsSkipped) {
	hkKeyPair* pairs = pairsOut;
	hkKeyPair* const pairsEnd = pairsOut + maxNumPairs;
	numPairsSkipped.m_storage = 0;
	while (--numA > 0) {
		const uint32_t maxX = pa->m_max[0];
		const hk1AxisSweep::AabbInt* pb = pa + 1;
		while (pb->m_min[0] < maxX) {
			const uint32_t ov0 = AabbInt::yzDisjoint(*pa, pb[0]);
			const uint32_t ov1 = AabbInt::yzDisjoint(*pa, pb[1]);
			const uint32_t ov2 = AabbInt::yzDisjoint(*pa, pb[2]);
			const uint32_t ov3 = AabbInt::yzDisjoint(*pa, pb[3]);
			if (!(ov0 & ov1 & ov2 & ov3)) {
				if (!ov0) {
					if (pairs < pairsEnd) {
						pairs->m_keyA = pa->getKey();
						pairs->m_keyB = pb[0].getKey();
						pairs++;
					} else {
						numPairsSkipped.m_storage++;
					}
				}
				if (!ov1 && pb[1].m_min[0] <= maxX) {
					if (pairs < pairsEnd) {
						pairs->m_keyA = pa->getKey();
						pairs->m_keyB = pb[1].getKey();
						pairs++;
					} else {
						numPairsSkipped.m_storage++;
					}
				}
				if (!ov2 && pb[2].m_min[0] <= maxX) {
					if (pairs < pairsEnd) {
						pairs->m_keyA = pa->getKey();
						pairs->m_keyB = pb[2].getKey();
						pairs++;
					} else {
						numPairsSkipped.m_storage++;
					}
				}
				if (!ov3 && pb[3].m_min[0] <= maxX) {
					if (pairs < pairsEnd) {
						pairs->m_keyA = pa->getKey();
						pairs->m_keyB = pb[3].getKey();
						pairs++;
					} else {
						numPairsSkipped.m_storage++;
					}
				}
			}
			pb += 4;
		}
		pa++;
	}
	return static_cast<int32_t>(pairs - pairsOut);
}

// Radix-sorts the boxes by min x (the sort keys padded to a multiple of four, reading the end markers), gathers them in
// order into a temp copy and copies that back. The three temp arrays are hkArray<T, hkContainerTempAllocator>:
// bufAlloc, the capacity the returned bytes hold or DONT_DEALLOCATE for none; freed unless DONT_DEALLOCATE.
void hk1AxisSweep::sortAabbs(hk1AxisSweep::AabbInt* aabbs, int32_t size) {
	const int32_t iPaddedSize = (size + 3) & ~3;
	const int32_t iDontDeallocate = static_cast<int32_t>(hkArrayBase<void>::DONT_DEALLOCATE_FLAG);
	const int32_t iCapacityMask = static_cast<int32_t>(hkArrayBase<void>::CAPACITY_MASK);

	// EXE-INLINED none: hkArray<hkRadixSort::SortData32, hkContainerTempAllocator>::hkArray(int)
	hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
	hkRadixSort::SortData32* pSortData = nullptr;
	int32_t iSortDataCapacity = 0;
	if (iPaddedSize) {
		int32_t iNumBytes = iPaddedSize * static_cast<int32_t>(sizeof(hkRadixSort::SortData32));
		pSortData = static_cast<hkRadixSort::SortData32*>(pTemp->bufAlloc(iNumBytes));
		iSortDataCapacity = iNumBytes / static_cast<int32_t>(sizeof(hkRadixSort::SortData32));
	}
	if (!iSortDataCapacity) {
		iSortDataCapacity = iDontDeallocate;
	}
	for (int32_t i = 0; i < iPaddedSize; ++i) {
		pSortData[i].m_key = aabbs[i].m_min[0];
		pSortData[i].m_userData = i;
	}

	// EXE-INLINED none: hkArray<hkRadixSort::SortData32, hkContainerTempAllocator>::hkArray(int)
	pTemp = hkMemoryRouter::getInstance().m_temp;
	hkRadixSort::SortData32* pBuffer = nullptr;
	int32_t iBufferCapacity = 0;
	if (iPaddedSize) {
		int32_t iNumBytes = iPaddedSize * static_cast<int32_t>(sizeof(hkRadixSort::SortData32));
		pBuffer = static_cast<hkRadixSort::SortData32*>(pTemp->bufAlloc(iNumBytes));
		iBufferCapacity = iNumBytes / static_cast<int32_t>(sizeof(hkRadixSort::SortData32));
	}
	if (!iBufferCapacity) {
		iBufferCapacity = iDontDeallocate;
	}
	hkRadixSort::sort32(pSortData, iPaddedSize, pBuffer);
	// EXE-INLINED none: hkArray<hkRadixSort::SortData32, hkContainerTempAllocator>::~hkArray
	pTemp = hkMemoryRouter::getInstance().m_temp;
	if (!(iBufferCapacity & iDontDeallocate)) {
		pTemp->bufFree(pBuffer,
			(iBufferCapacity & iCapacityMask) * static_cast<int32_t>(sizeof(hkRadixSort::SortData32)));
	}

	// EXE-INLINED none: hkArray<hk1AxisSweep::AabbInt, hkContainerTempAllocator>::hkArray(int)
	pTemp = hkMemoryRouter::getInstance().m_temp;
	hk1AxisSweep::AabbInt* pSorted = nullptr;
	int32_t iSortedCapacity = 0;
	if (size) {
		int32_t iNumBytes = size * static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt));
		pSorted = static_cast<hk1AxisSweep::AabbInt*>(pTemp->bufAlloc(iNumBytes));
		iSortedCapacity = iNumBytes / static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt));
	}
	if (!iSortedCapacity) {
		iSortedCapacity = iDontDeallocate;
	}
	for (int32_t i = 0; i < size; ++i) {
		// EXE-INLINED 0xCF8690 (AabbInt::operator=: hkString::memCpy16<32>)
		hkString::memCpy16NonEmpty(&pSorted[i], &aabbs[pSortData[i].m_userData],
			static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt)) / 16);
	}
	hkString::memCpy16(aabbs, pSorted, static_cast<int32_t>(size * sizeof(hk1AxisSweep::AabbInt) / 16));

	// EXE-INLINED none: hkArray<hk1AxisSweep::AabbInt, hkContainerTempAllocator>::~hkArray
	pTemp = hkMemoryRouter::getInstance().m_temp;
	if (!(iSortedCapacity & iDontDeallocate)) {
		pTemp->bufFree(pSorted,
			(iSortedCapacity & iCapacityMask) * static_cast<int32_t>(sizeof(hk1AxisSweep::AabbInt)));
	}
	// EXE-INLINED none: hkArray<hkRadixSort::SortData32, hkContainerTempAllocator>::~hkArray
	pTemp = hkMemoryRouter::getInstance().m_temp;
	if (!(iSortDataCapacity & iDontDeallocate)) {
		pTemp->bufFree(pSortData,
			(iSortDataCapacity & iCapacityMask) * static_cast<int32_t>(sizeof(hkRadixSort::SortData32)));
	}
}
