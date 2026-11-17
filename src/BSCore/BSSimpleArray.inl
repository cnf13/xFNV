#pragma once

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"

#include <cstring>
// The hybrid build keeps its empty generic bodies (its unchanged text); the native build's follow.
#include <new>

template <class T, uint32_t uiGrowStep> bool BSSimpleArray<T, uiGrowStep>::_ShouldGrow() const {
	return iSize == iReservedSize;
}

template <class T, uint32_t uiGrowStep> bool BSSimpleArray<T, uiGrowStep>::IsEmpty() const {
	return iSize == 0;
}

template <class T, uint32_t uiGrowStep> bool BSSimpleArray<T, uiGrowStep>::_ShouldShrink() const {
	if (iReservedSize > uiGrowStep)
		return iSize <= iReservedSize - 2 * uiGrowStep;
	return iSize <= (iReservedSize >> 2);
}

template <class T, uint32_t uiGrowStep> T& BSSimpleArray<T, uiGrowStep>::GetAt(int32_t aiIndex) {
	return pBuffer[aiIndex];
}

template <class T, uint32_t uiGrowStep> const T& BSSimpleArray<T, uiGrowStep>::GetAt(int32_t aiIndex) const {
	return pBuffer[aiIndex];
}

template <class T, uint32_t uiGrowStep> T& BSSimpleArray<T, uiGrowStep>::operator[](uint32_t auiIndex) {
	return GetAt(auiIndex);
}

template <class T, uint32_t uiGrowStep> const T& BSSimpleArray<T, uiGrowStep>::operator[](uint32_t auiIndex) const {
	return GetAt(auiIndex);
}

template <class T, uint32_t uiGrowStep> uint32_t BSSimpleArray<T, uiGrowStep>::_GetNewSizeForGrow() const {
	if (iReservedSize > uiGrowStep)
		return iReservedSize + uiGrowStep;
	return iReservedSize << 1;
}

template <class T, uint32_t uiGrowStep> uint32_t BSSimpleArray<T, uiGrowStep>::_GetNewSizeForShrink() const {
	if (iReservedSize > 2 * uiGrowStep)
		return iReservedSize - uiGrowStep;
	return iReservedSize >> 1;
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::_DeallocateBuffer() {
	this->_Deallocate(pBuffer);
	pBuffer = nullptr;
}

// Runs T's destructor on each item, as the exe's copies for class items do (0x822820 for NiPointer, 0x7A63B0 for
// BSStringT<char>); for plain items it does nothing.
template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::_DestructItems(T* apFirstAddr, uint32_t aiCount) {
	for (uint32_t i = 0; i < aiCount; ++i)
		apFirstAddr[i].~T();
}

// Placement new of each item, default-initialised: the exe's copies run T's default constructor (0x822780 for
// NiPointer, BSStringT<char>'s) and leave plain items unset (ActorHavokSyncData's).
template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::_ConstructItems(T* apFirstAddr, uint32_t aiCount) {
	for (uint32_t i = 0; i < aiCount; ++i)
		new (&apFirstAddr[i]) T;
}

// Relocates the elements byte for byte (the exe memmoves one element at a time), so the destination may be raw memory
// and the source is not destroyed. Natively too: a relocation of live objects of one type keeps their vtable pointers
// and owned members valid (cast to void* because it is meant).
template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::_MoveItems(T* apDestAddr, T* apSrcAddr, uint32_t aiCount) {
	if (aiCount == 0)
		return;
	if (apDestAddr < apSrcAddr) {
		for (uint32_t i = 0; i < aiCount; i++)
			std::memmove(static_cast<void*>(&apDestAddr[i]), static_cast<const void*>(&apSrcAddr[i]), sizeof(T));
	} else if (apDestAddr > apSrcAddr) {
		for (int32_t i = static_cast<int32_t>(aiCount) - 1; i >= 0; i--)
			std::memmove(static_cast<void*>(&apDestAddr[i]), static_cast<const void*>(&apSrcAddr[i]), sizeof(T));
	}
}

template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::_ReallocateBuffer(uint32_t aiNewSize, uint32_t aiCopyCount) {
	if (!pBuffer) {
		pBuffer = this->_Allocate(aiNewSize);
		iReservedSize = aiNewSize;
	} else if (aiCopyCount == iReservedSize) {
		pBuffer = this->_Reallocate(pBuffer, aiNewSize);
	} else {
		T* pNewBuffer = this->_Allocate(aiNewSize);
		_MoveItems(pNewBuffer, pBuffer, aiCopyCount);
		_DeallocateBuffer();
		pBuffer = pNewBuffer;
	}
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::Clear(bool abFreeMemory) {
	if (!pBuffer)
		return;
	_DestructItems(pBuffer, iSize);
	if (abFreeMemory) {
		_DeallocateBuffer();
		pBuffer = nullptr;
		iReservedSize = 0;
	}
	iSize = 0;
}

template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::SetSize(uint32_t aiNewSize, bool abFreeMemory) {
	if (aiNewSize == 0) {
		Clear(abFreeMemory);
	} else if (aiNewSize > iReservedSize) {
		if (iReservedSize)
			_ReallocateBuffer(aiNewSize, iSize);
		else
			pBuffer = this->_Allocate(aiNewSize);
		iReservedSize = aiNewSize;
		_ConstructItems(&pBuffer[iSize], aiNewSize - iSize);
		iSize = aiNewSize;
	} else if (aiNewSize >= iSize) {
		_ConstructItems(&pBuffer[iSize], aiNewSize - iSize);
		iSize = aiNewSize;
	} else {
		_DestructItems(&pBuffer[aiNewSize], iSize - aiNewSize);
		iSize = aiNewSize;
		if (abFreeMemory && aiNewSize <= (iReservedSize >> 2)) {
			_ReallocateBuffer(aiNewSize, aiNewSize);
			iReservedSize = aiNewSize;
		}
	}
}

template <class T, uint32_t uiGrowStep> T* BSSimpleArray<T, uiGrowStep>::_Allocate(uint32_t aiSizeInItem) {
	return static_cast<T*>(ourMalloc(aiSizeInItem * static_cast<uint32_t>(sizeof(T))));
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::_Deallocate(T* apBuffer) {
	ourFree(apBuffer);
}

template <class T, uint32_t uiGrowStep>
T* BSSimpleArray<T, uiGrowStep>::_Reallocate(T* apBuffer, uint32_t aiNewSizeInItem) {
	return static_cast<T*>(ourRealloc(apBuffer, aiNewSizeInItem * static_cast<uint32_t>(sizeof(T))));
}

namespace bhkUtilFunctions {
struct AVLevel;
}

template <class T, uint32_t uiGrowStep>
int32_t BSSimpleArray<T, uiGrowStep>::Find(const T& aValue, uint32_t aiStartIndex, EqualFunction apFunc) const {
	while (aiStartIndex < iSize) {
		if (pBuffer && apFunc(&pBuffer[aiStartIndex], &aValue))
			return static_cast<int32_t>(aiStartIndex);
		++aiStartIndex;
	}
	return -1;
}

// The shrink path moves iSize - 1 items from auiIndex + 1 on, not the iSize - auiIndex - 1 the other path moves: every
// exe copy does so (0xB5ADE0, 0x6E7500, 0x7A1B10), and the extra items fit both buffers when the array shrinks by
// halving.
template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::Remove(uint32_t aiIndex, bool abShrink) {
	if (abShrink && _ShouldShrink()) {
		uint32_t uiNewSize = _GetNewSizeForShrink();
		T* pNewBuffer = _Allocate(uiNewSize);
		_MoveItems(pNewBuffer, pBuffer, aiIndex);
		_DestructItems(&pBuffer[aiIndex], 1);
		_MoveItems(&pNewBuffer[aiIndex], &pBuffer[aiIndex + 1], iSize - 1);
		_DeallocateBuffer();
		pBuffer = pNewBuffer;
		iReservedSize = uiNewSize;
	} else {
		_DestructItems(&pBuffer[aiIndex], 1);
		_MoveItems(&pBuffer[aiIndex], &pBuffer[aiIndex + 1], iSize - aiIndex - 1);
	}
	--iSize;
}

template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::RemoveFast(uint32_t aiIndex, uint32_t aiHowMany) {
	uint32_t uiItemsToMove = iSize - aiIndex - aiHowMany;
	if (uiItemsToMove >= aiHowMany)
		uiItemsToMove = aiHowMany;
	_DestructItems(&pBuffer[aiIndex], aiHowMany);
	_MoveItems(&pBuffer[aiIndex], &pBuffer[iSize - uiItemsToMove], uiItemsToMove);
	iSize -= aiHowMany;
}

// A full buffer grows by _GetNewSizeForGrow; an array without one starts at 4 items.
template <class T, uint32_t uiGrowStep> uint32_t BSSimpleArray<T, uiGrowStep>::AddUninitialized() {
	if (_ShouldGrow()) {
		if (iReservedSize) {
			uint32_t uiNewSize = _GetNewSizeForGrow();
			_ReallocateBuffer(uiNewSize, iSize);
			iReservedSize = uiNewSize;
		} else {
			pBuffer = _Allocate(4);
			iReservedSize = 4;
		}
	}
	return iSize++;
}

template <class T, uint32_t uiGrowStep> uint32_t BSSimpleArray<T, uiGrowStep>::Add(const T& aValue) {
	uint32_t uiIndex = AddUninitialized();
	_ConstructItems(&pBuffer[uiIndex], 1);
	pBuffer[uiIndex] = aValue;
	return uiIndex;
}

template <class T, uint32_t uiGrowStep> uint32_t BSSimpleArray<T, uiGrowStep>::GetSize() const {
	return iSize;
}

template <class T, uint32_t uiGrowStep> bool BSSimpleArray<T, uiGrowStep>::IsInArray(const T& aValue) const {
	return Find(aValue, 0, DefaultEqualFunctionByAddress) != -1;
}

template <class T, uint32_t uiGrowStep> bool BSSimpleArray<T, uiGrowStep>::CompareArrays(BSSimpleArray& aArray) {
	if (aArray.iSize != iSize)
		return false;
	for (uint32_t i = 0; i < iSize; ++i) {
		if (aArray[i] != operator[](i))
			return false;
	}
	return true;
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::SetReservedSize(uint32_t aiReserveSize) {
	if (aiReserveSize == iReservedSize)
		return;
	if (aiReserveSize < iSize) {
		_DestructItems(&pBuffer[aiReserveSize], iSize - aiReserveSize);
		iSize = aiReserveSize;
	}
	_ReallocateBuffer(aiReserveSize, iSize);
	iReservedSize = aiReserveSize;
}

template <class T, uint32_t uiGrowStep> T& BSSimpleArray<T, uiGrowStep>::GetFirst() {
	return GetAt(0);
}

template <class T, uint32_t uiGrowStep> const T& BSSimpleArray<T, uiGrowStep>::GetFirst() const {
	return GetAt(0);
}

template <class T, uint32_t uiGrowStep> T& BSSimpleArray<T, uiGrowStep>::GetLast() {
	return GetAt(iSize - 1);
}

template <class T, uint32_t uiGrowStep> const T& BSSimpleArray<T, uiGrowStep>::GetLast() const {
	return GetAt(iSize - 1);
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::CopyTo(BSSimpleArray* apDest) const {
	apDest->SetSize(iSize, true);
	for (uint32_t i = 0; i < iSize; ++i)
		apDest->GetAt(i) = pBuffer[i];
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::SetAt(uint32_t aiIndex, const T& aValue) {
	GetAt(aiIndex) = aValue;
}

// The exe's SetAt of the AVLevel array (0xC80B20) is the PDB's BSSimpleArray<bhkUtilFunctions::AVLevel,1024>::Insert
// (the array has no SetAt in the PDB): it inserts aValue at aiIndex, moving the later items up. Its caller
// (bhkUtilFunctions.cpp, apQueue.SetAt(0, ...)) relies on that. Defined in BSSimpleArray.cpp, where AVLevel is
// complete.
template <>
void BSSimpleArray<bhkUtilFunctions::AVLevel, 1024>::SetAt(uint32_t aiIndex, const bhkUtilFunctions::AVLevel& aValue);

// Swaps the items from both ends inward; the indices compare signed, so an empty array does nothing.
template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::Reverse() {
	for (int32_t i = 0, j = static_cast<int32_t>(iSize) - 1; i < j; ++i, --j) {
		T kTemp = pBuffer[i];
		pBuffer[i] = pBuffer[j];
		pBuffer[j] = kTemp;
	}
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::Sort(CompareFunction apfnCompare) {
	std::qsort(pBuffer, iSize, sizeof(T), apfnCompare);
}

// The address of the last item.
template <class T, uint32_t uiGrowStep> void* BSSimpleArray<T, uiGrowStep>::GetTail() {
	return &GetAt(static_cast<int32_t>(iSize - 1));
}

template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::_InitialSetup(uint32_t aiReserveSize, uint32_t aiInitialSize) {
	pBuffer = nullptr;
	iSize = 0;
	iReservedSize = 0;
	if (aiInitialSize > aiReserveSize)
		aiReserveSize = aiInitialSize;
	if (aiReserveSize) {
		pBuffer = _Allocate(aiReserveSize);
		iReservedSize = aiReserveSize;
	}
	if (aiInitialSize) {
		_ConstructItems(pBuffer, aiInitialSize);
		iSize = aiInitialSize;
	}
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::Insert(uint32_t aiIndex, const T& aValue) {
	if (aiIndex == iSize) {
		Add(aValue);
		return;
	}
	if (_ShouldGrow()) {
		uint32_t uiNewSize = _GetNewSizeForGrow();
		T* pNewBuffer = _Allocate(uiNewSize);
		_MoveItems(pNewBuffer, pBuffer, aiIndex);
		_ConstructItems(&pNewBuffer[aiIndex], 1);
		_MoveItems(&pNewBuffer[aiIndex + 1], &pBuffer[aiIndex], iSize - aiIndex);
		_DeallocateBuffer();
		pBuffer = pNewBuffer;
		iReservedSize = uiNewSize;
	} else {
		_MoveItems(&pBuffer[aiIndex + 1], &pBuffer[aiIndex], iSize - aiIndex);
		_ConstructItems(&pBuffer[aiIndex], 1);
	}
	++iSize;
	pBuffer[aiIndex] = aValue;
}

// The shrink path moves iSize - aiHowMany items from aiIndex + aiHowMany on (every exe copy: 0x49F3C0, 0x98EBE0,
// 0x6DB140, 0x6F29E0), the other path iSize - aiIndex - aiHowMany.
template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::RemoveRange(uint32_t aiIndex, uint32_t aiHowMany, bool abShrink) {
	if (aiHowMany >= iSize) {
		Clear(abShrink);
		return;
	}
	if (abShrink && _ShouldShrink()) {
		uint32_t uiNewSize = _GetNewSizeForShrink();
		T* pNewBuffer = _Allocate(uiNewSize);
		_MoveItems(pNewBuffer, pBuffer, aiIndex);
		_DestructItems(&pBuffer[aiIndex], aiHowMany);
		_MoveItems(&pNewBuffer[aiIndex], &pBuffer[aiIndex + aiHowMany], iSize - aiHowMany);
		_DeallocateBuffer();
		pBuffer = pNewBuffer;
		iReservedSize = uiNewSize;
	} else {
		_DestructItems(&pBuffer[aiIndex], aiHowMany);
		_MoveItems(&pBuffer[aiIndex], &pBuffer[aiIndex + aiHowMany], iSize - aiIndex - aiHowMany);
	}
	iSize -= aiHowMany;
}

template <class T, uint32_t uiGrowStep> void BSSimpleArray<T, uiGrowStep>::RemoveLast() {
	if (iSize)
		_DestructItems(&pBuffer[--iSize], 1);
}

// Binary search for aValue's place: apFunc's -1 searches below the middle, 1 above it, 0 inserts at the middle.
template <class T, uint32_t uiGrowStep>
void BSSimpleArray<T, uiGrowStep>::SortedInsert(const T& aValue, CompareFunction apFunc) {
	int32_t iFront = 0;
	int32_t iBack = static_cast<int32_t>(iSize) - 1;
	while (iFront <= iBack) {
		int32_t iMiddle = iFront + ((iBack - iFront) >> 1);
		int32_t iCompare = apFunc(&aValue, &pBuffer[iMiddle]);
		if (iCompare == -1) {
			iBack = iMiddle - 1;
		} else if (iCompare == 0) {
			iFront = iMiddle;
			break;
		} else if (iCompare == 1) {
			iFront = iMiddle + 1;
		}
	}
	Insert(static_cast<uint32_t>(iFront), aValue);
}

template <class T, uint32_t uiGrowStep>
uint32_t BSSimpleArray<T, uiGrowStep>::SortedFind(const T& aValue, CompareFunction apFunc) {
	void* pFound = std::bsearch(&aValue, pBuffer, iSize, sizeof(T), apFunc);
	if (!pFound)
		return static_cast<uint32_t>(-1);
	return static_cast<uint32_t>(static_cast<T*>(pFound) - pBuffer);
}

template <class T, uint32_t uiGrowStep>
bool BSSimpleArray<T, uiGrowStep>::DefaultEqualFunction(const T& aOne, const T& aTwo) {
	return aOne == aTwo;
}
