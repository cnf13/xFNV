#include "Gamebryo2.2/CoreLibs/NiMain/NiBackToFrontAccumulator.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiBackToFrontAccumulator::ms_RTTI("NiBackToFrontAccumulator", &NiAccumulator::ms_RTTI);

NiBackToFrontAccumulator::NiBackToFrontAccumulator()
	: m_kItemsToSortAndRender(nullptr), m_iNumItems(0), m_iMaxItems(0), m_ppkItems(nullptr), m_pfDepths(nullptr) {}

// The item list's own destructor empties it.
NiBackToFrontAccumulator::~NiBackToFrontAccumulator() {
	_NiFree(m_ppkItems);
	_NiFree(m_pfDepths);
}

NiObject* NiBackToFrontAccumulator::CreateClone(NiCloningProcess& kCloning) {
	NiBackToFrontAccumulator* pkObject =
		static_cast<NiBackToFrontAccumulator*>(NiMemObject::operator new(sizeof(NiBackToFrontAccumulator)));
	if (pkObject) {
		::new (pkObject) NiBackToFrontAccumulator;
		NiAccumulator::CopyMembers(pkObject, kCloning);
		return pkObject;
	} else {
		NiAccumulator::CopyMembers(0, kCloning);
		return 0;
	}
}

void NiBackToFrontAccumulator::LoadBinary(NiStream& kStream) {
	NiAccumulator::LoadBinary(kStream);
}

void NiBackToFrontAccumulator::LinkObject(NiStream& kStream) {
	NiAccumulator::LinkObject(kStream);
}

bool NiBackToFrontAccumulator::RegisterStreamables(NiStream& kStream) {
	return NiAccumulator::RegisterStreamables(kStream);
}

void NiBackToFrontAccumulator::SaveBinary(NiStream& kStream) {
	NiAccumulator::SaveBinary(kStream);
}

bool NiBackToFrontAccumulator::IsEqual(NiObject* pkObject) {
	return NiAccumulator::IsEqual(pkObject);
}

// Draws the sorted items from the farthest to the nearest, then empties the item list. Does nothing, not even
// ending the accumulation, without a renderer.
void NiBackToFrontAccumulator::FinishAccumulating() {
	NiRenderer* pkRenderer = NiRenderer::ms_pkRenderer;
	if (!pkRenderer)
		return;

	Sort();

	m_iCurrItem = m_iNumItems;
	while (m_iCurrItem) {
		NiGeometry* pkItem = m_ppkItems[--m_iCurrItem];
		if (!pkItem)
			break;
		pkItem->RenderImmediate(pkRenderer);
	}

	while (m_kItems.m_uiCount) {
		NiTListItem<NiGeometry*>* pkNode = m_kItems.m_pkHead;
		m_kItems.m_pkHead = pkNode->m_pkNext;
		if (m_kItems.m_pkHead)
			m_kItems.m_pkHead->m_pkPrev = nullptr;
		else
			m_kItems.m_pkTail = nullptr;
		// EXE-INLINED 0x45CEE0
		// DeleteItem and the pool's Deallocate, inlined as the engine does: the node's memory is the allocator's node.
		pkNode->m_element = nullptr;
		NiTAbstractPoolAllocator<uint32_t>::AllocNode* pkAllocNode =
			static_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(static_cast<void*>(pkNode));
		NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
		pkAllocNode->m_pkData = nullptr;
		pkAllocNode->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
		bool bUnlock = NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount-- == 1;
		NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pkAllocNode;
		if (bUnlock)
			NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
		m_kItems.m_uiCount--;
	}

	NiAccumulator::FinishAccumulating();
}

// Appends every geometry of the array to the item list.
void NiBackToFrontAccumulator::RegisterObjectArray(NiVisibleArray& kArray) {
	using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
	uint32_t uiCount = kArray.GetCount();
	for (uint32_t i = 0; i < uiCount; i++) {
		NiGeometry* pkGeometry = &kArray.GetAt(i);
		// EXE-INLINED 0x43A010
		PoolAllocator::Lock();
		PoolAllocator::AllocNode* pkNew = PoolAllocator::ms_pkFreeMem;
		if (!pkNew) {
			PoolAllocator::CreateFreeMem();
			pkNew = PoolAllocator::ms_pkFreeMem;
		}
		PoolAllocator::ms_pkFreeMem = pkNew->m_pkNext;
		pkNew->m_element = 0;
		pkNew->m_pkNext = nullptr;
		pkNew->m_pkData = nullptr;
		PoolAllocator::Unlock();
		// EXE-INLINED 0xB58570
		NiTListItem<NiGeometry*>* pkItem = static_cast<NiTListItem<NiGeometry*>*>(static_cast<void*>(pkNew));
		pkItem->m_element = pkGeometry;
		pkItem->m_pkNext = nullptr;
		pkItem->m_pkPrev = m_kItems.m_pkTail;
		if (m_kItems.m_pkTail)
			m_kItems.m_pkTail->m_pkNext = pkItem;
		else
			m_kItems.m_pkHead = pkItem;
		m_kItems.m_uiCount++;
		m_kItems.m_pkTail = pkItem;
	}
}

void NiBackToFrontAccumulator::Sort() {
	if (!m_kItemsToSortAndRender)
		m_kItemsToSortAndRender = &m_kItems;

	m_iNumItems = m_kItemsToSortAndRender->m_uiCount;
	if (!m_iNumItems)
		return;

	if (m_iNumItems > m_iMaxItems) {
		_NiFree(m_ppkItems);
		m_iMaxItems = m_iNumItems;
		m_ppkItems = static_cast<NiGeometry**>(_NiMalloc(sizeof(NiGeometry*) * m_iMaxItems));
		_NiFree(m_pfDepths);
		m_pfDepths = static_cast<float*>(_NiMalloc(sizeof(float) * m_iMaxItems));
	}

	NiTListItem<NiGeometry*>* pkPos = m_kItemsToSortAndRender->m_pkHead;
	// The camera's world direction (NiCamera::GetWorldDirection) is column 0 of the world rotation.
	float fDirX = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
	float fDirY = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
	float fDirZ = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
	for (int32_t i = 0; i < m_iNumItems; i++) {
		m_ppkItems[i] = m_kItemsToSortAndRender->GetNext(pkPos);
		const NiBound* pkBound = m_ppkItems[i]->m_kWorldBound;
		if (!pkBound)
			pkBound = &NiAVObject::NullBoundS;
		float fDepth = pkBound->m_kCenter.y * fDirY + pkBound->m_kCenter.x * fDirX + pkBound->m_kCenter.z * fDirZ;
		m_pfDepths[i] = fDepth;
	}

	SortObjectsByDepth(0, m_iNumItems - 1);
}

void NiBackToFrontAccumulator::CopyMembers(NiBackToFrontAccumulator* pkDest, NiCloningProcess& kCloning) {
	NiAccumulator::CopyMembers(pkDest, kCloning);
}

void NiBackToFrontAccumulator::SortObjectsByDepth(int32_t l, int32_t r) {
	while (l < r) {
		float fPivot = ChoosePivot(l, r);
		int32_t iLow = l - 1;
		int32_t iHigh = r + 1;
		for (;;) {
			do {
				iHigh--;
			} while (m_pfDepths[iHigh] > fPivot);
			do {
				iLow++;
			} while (m_pfDepths[iLow] < fPivot);
			if (iLow >= iHigh)
				break;

			NiGeometry* pkItem = m_ppkItems[iLow];
			m_ppkItems[iLow] = m_ppkItems[iHigh];
			m_ppkItems[iHigh] = pkItem;
			float fDepth = m_pfDepths[iLow];
			m_pfDepths[iLow] = m_pfDepths[iHigh];
			m_pfDepths[iHigh] = fDepth;
		}

		// recurse on the left part, loop on the right one
		if (iHigh == r) {
			r = iHigh - 1;
		} else {
			SortObjectsByDepth(l, iHigh);
			l = iHigh + 1;
		}
	}
}

float NiBackToFrontAccumulator::ChoosePivot(int32_t l, int32_t r) const {
	int32_t iMiddle = (l + r) >> 1;
	if (m_pfDepths[iMiddle] > m_pfDepths[l]) {
		if (m_pfDepths[r] > m_pfDepths[iMiddle])
			return m_pfDepths[iMiddle];
		if (m_pfDepths[r] > m_pfDepths[l])
			return m_pfDepths[r];
		return m_pfDepths[l];
	}
	if (m_pfDepths[r] > m_pfDepths[l])
		return m_pfDepths[l];
	if (m_pfDepths[r] > m_pfDepths[iMiddle])
		return m_pfDepths[r];
	return m_pfDepths[iMiddle];
}
