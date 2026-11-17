#include "BSHavok/bhkShapeCollection.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

NiRTTI bhkShapeCollection::ms_RTTI("bhkShapeCollection", &bhkShape::ms_RTTI);

bhkShapeCollection::~bhkShapeCollection() {
	bhkSerializable::Kill();
	--iCount;
}

// The exe's EH frame (scope table 0xf30788) is cleanup-only and left out. The array's growth goes through the exe's
// out-of-line NiTArray instantiations; src's inline template members stand in for them.
void bhkShapeCollection::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

	bhkShape::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
	apStrings->Add(pcString);
}

// Shareable when the base says so and every child shape that has a game shape is shareable; the walk stops at the
// first one that is not. The getter and key walk are inlined as in BuildDisplayGeometry.
bool bhkShapeCollection::CanShare(NiCloningProcess& arCloneData) {
	bool bres = bhkShape::CanShare(arCloneData);
	if (bres) {
		// EXE-INLINED 0x62B480
		// EXE-INLINED 0x4AE750
		const hkpShapeContainer& kFirst = *(this ? static_cast<hkpShapeCollection*>(phkObject) : nullptr);
		uint32_t uiKey = kFirst.getFirstKey();
		while (uiKey != HK_INVALID_SHAPE_KEY && bres) {
			// EXE-INLINED 0x4AE750
			hkpShapeCollection* pCollection = this ? static_cast<hkpShapeCollection*>(phkObject) : nullptr;
			if (pCollection) {
				alignas(16) char acBuffer[512];
				const hkpShape* pChild =
					static_cast<const hkpShapeContainer*>(pCollection)->getChildShape(uiKey, acBuffer);
				if (pChild) {
					bhkShape* ps = pChild->m_userData;
					if (ps) {
						bres = ps->CanShare(arCloneData);
					}
				}
			}
			// EXE-INLINED 0x62B4B0
			// EXE-INLINED 0x4AE750
			const hkpShapeContainer& kNext = *(this ? static_cast<hkpShapeCollection*>(phkObject) : nullptr);
			uiKey = kNext.getNextKey(uiKey);
		}
	}
	return bres;
}

// Builds each child shape's display geometry under apNode, walking the Havok collection's keys until a key has no
// game shape. The exe inlines the null-safe Havok getter (0x4AE750) and GetFirstKey/GetNextKey (0x62B480, 0x62B4B0):
// the key walk takes the container base without a null test, the child lookup tests both this and the Havok shape.
void bhkShapeCollection::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
	// EXE-INLINED 0x62B480
	// EXE-INLINED 0x4AE750
	const hkpShapeContainer& kFirst = *(this ? static_cast<hkpShapeCollection*>(phkObject) : nullptr);
	uint32_t uiKey = kFirst.getFirstKey();
	while (uiKey != HK_INVALID_SHAPE_KEY) {
		bhkShape* ps = nullptr;
		// EXE-INLINED 0x4AE750
		hkpShapeCollection* pCollection = this ? static_cast<hkpShapeCollection*>(phkObject) : nullptr;
		if (pCollection) {
			alignas(16) char acBuffer[512];
			const hkpShape* pChild = static_cast<const hkpShapeContainer*>(pCollection)->getChildShape(uiKey, acBuffer);
			if (pChild) {
				ps = pChild->m_userData;
			}
		}
		// EXE-INLINED 0x62B4B0
		// EXE-INLINED 0x4AE750
		const hkpShapeContainer& kNext = *(this ? static_cast<hkpShapeCollection*>(phkObject) : nullptr);
		uiKey = kNext.getNextKey(uiKey);
		if (!ps) {
			break;
		}
		ps->BuildDisplayGeometry(apNode, arDefColor);
	}
}
