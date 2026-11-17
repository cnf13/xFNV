#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaAccumulator.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBackToFrontAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiAlphaAccumulator::ms_RTTI("NiAlphaAccumulator", &NiBackToFrontAccumulator::ms_RTTI);

namespace {

// The alpha property is the first slot of the geometry's property state.
const NiAlphaProperty* AlphaPropertyOf(const NiGeometry* pkGeometry) {
	return static_cast<const NiAlphaProperty*>(pkGeometry->m_PropertyState.m_aspProps[0].m_pObject);
}

}  // namespace

NiAlphaAccumulator::NiAlphaAccumulator()
	: m_bObserveNoSortHint(true), m_bSortByClosestPoint(false), m_bInterfaceSort(false) {}

NiAlphaAccumulator::~NiAlphaAccumulator() {}

NiObject* NiAlphaAccumulator::CreateClone(NiCloningProcess& kCloning) {
	NiAlphaAccumulator* pkObject_2 =
		static_cast<NiAlphaAccumulator*>(NiMemObject::operator new(sizeof(NiAlphaAccumulator)));
	if (pkObject_2) {
		::new (pkObject_2) NiAlphaAccumulator;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_bObserveNoSortHint = m_bObserveNoSortHint;
	pkObject_2->m_bSortByClosestPoint = m_bSortByClosestPoint;
	return pkObject_2;
}

void NiAlphaAccumulator::LoadBinary(NiStream& kStream) {
	NiBackToFrontAccumulator::LoadBinary(kStream);
}

void NiAlphaAccumulator::LinkObject(NiStream& kStream) {
	NiBackToFrontAccumulator::LinkObject(kStream);
}

bool NiAlphaAccumulator::RegisterStreamables(NiStream& kStream) {
	return NiBackToFrontAccumulator::RegisterStreamables(kStream);
}

void NiAlphaAccumulator::SaveBinary(NiStream& kStream) {
	NiBackToFrontAccumulator::SaveBinary(kStream);
}

bool NiAlphaAccumulator::IsEqual(NiObject* pkObject) {
	return NiBackToFrontAccumulator::IsEqual(pkObject);
}

// Opaque geometry, geometry with the no-sort hint (when observed) and geometry with sorting disabled is drawn at once;
// blended geometry is kept for the sorted pass.
void NiAlphaAccumulator::RegisterObjectArray(NiVisibleArray& kArray) {
	NiRenderer* pkRenderer = NiRenderer::ms_pkRenderer;
	uint32_t uiCount = kArray.GetCount();
	for (uint32_t i = 0; i < uiCount; i++) {
		NiGeometry* pkGeometry = &kArray.GetAt(i);
		const NiAlphaProperty* pkAlpha = AlphaPropertyOf(pkGeometry);
		if (!(pkAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) ||
			(m_bObserveNoSortHint && (pkAlpha->m_uFlags & NiAlphaProperty::ALPHA_NOSORTER_MASK)) ||
			(pkGeometry->m_uFlags & NiAVObject::DISABLE_SORTING)) {
			pkGeometry->RenderImmediate(pkRenderer);
		} else {
			// EXE-INLINED 0xB58570
			// The node comes from the node pool (0x43A010), called with the list's count field as its `this`: the
			// stateless allocator is folded into the list (NiTListBase.h), so the exe passes that field's address.
			NiTListItem<NiGeometry*>* pkNode =
				static_cast<NiTListItem<NiGeometry*>*>(reinterpret_cast<NiTPointerAllocator<uint32_t>&>(m_kItems
																											.m_uiCount)
						.Allocate());
			pkNode->m_element = pkGeometry;
			pkNode->m_pkNext = nullptr;
			pkNode->m_pkPrev = m_kItems.m_pkTail;
			if (m_kItems.m_pkTail) {
				m_kItems.m_pkTail->m_pkNext = pkNode;
				++m_kItems.m_uiCount;
				m_kItems.m_pkTail = pkNode;
			} else {
				++m_kItems.m_uiCount;
				m_kItems.m_pkHead = pkNode;
				m_kItems.m_pkTail = pkNode;
			}
		}
	}
}

bool NiAlphaAccumulator::RegisterObject(NiGeometry* apGeometry) {
	const NiAlphaProperty* pkAlpha = AlphaPropertyOf(apGeometry);
	if (!(pkAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) ||
		(m_bObserveNoSortHint && (pkAlpha->m_uFlags & NiAlphaProperty::ALPHA_NOSORTER_MASK)) ||
		(apGeometry->m_uFlags & NiAVObject::DISABLE_SORTING)) {
		apGeometry->RenderImmediate(NiRenderer::ms_pkRenderer);
	} else {
		m_kItems.AddTail(apGeometry);
	}
	return true;
}

// Depth is the item's world height in interface mode, else the bound centre along the camera's view direction, less
// the bound radius when sorting by the closest point.
void NiAlphaAccumulator::Sort() {
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
	// Reads NiCamera::GetWorldDirection directly: column 0 of the world rotation.
	float fDirX = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
	float fDirY = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
	float fDirZ = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
	if (m_bInterfaceSort) {
		for (int32_t i = 0; i < m_iNumItems; i++) {
			m_ppkItems[i] = m_kItemsToSortAndRender->GetNext(pkPos);
			m_pfDepths[i] = m_ppkItems[i]->m_kWorld.m_Translate.y;
		}
	} else if (m_bSortByClosestPoint) {
		for (int32_t i = 0; i < m_iNumItems; i++) {
			m_ppkItems[i] = m_kItemsToSortAndRender->GetNext(pkPos);
			NiGeometry* pkItem = m_ppkItems[i];
			const NiBound* pkCenterBound = pkItem->m_kWorldBound;
			if (!pkCenterBound)
				pkCenterBound = &NiAVObject::NullBoundS;
			const NiBound* pkRadiusBound = pkItem->m_kWorldBound;
			if (!pkRadiusBound)
				pkRadiusBound = &NiAVObject::NullBoundS;
			float fDepth = pkCenterBound->m_kCenter.y * fDirY + pkCenterBound->m_kCenter.x * fDirX +
				pkCenterBound->m_kCenter.z * fDirZ;
			float fClosest = fDepth - pkRadiusBound->m_fRadius;
			m_pfDepths[i] = fClosest;
		}
	} else {
		for (int32_t i = 0; i < m_iNumItems; i++) {
			m_ppkItems[i] = m_kItemsToSortAndRender->GetNext(pkPos);
			const NiBound* pkBound = m_ppkItems[i]->m_kWorldBound;
			if (!pkBound)
				pkBound = &NiAVObject::NullBoundS;
			float fDepth = pkBound->m_kCenter.y * fDirY + pkBound->m_kCenter.x * fDirX + pkBound->m_kCenter.z * fDirZ;
			m_pfDepths[i] = fDepth;
		}
	}

	SortObjectsByDepth(0, m_iNumItems - 1);
}

NiObject* NiAlphaAccumulator::CreateObject() {
	NiAlphaAccumulator* pkObject =
		static_cast<NiAlphaAccumulator*>(NiMemObject::operator new(sizeof(NiAlphaAccumulator)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiAlphaAccumulator;
}
