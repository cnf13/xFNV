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

void bhkShapeCollection::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
    apStrings->Add(pcString);
}

// Shareable when the base says so and every child shape that has a game shape is shareable; the walk stops at the first
// one that is not.
bool bhkShapeCollection::CanShare(NiCloningProcess& arCloneData) {
    bool bres = bhkShape::CanShare(arCloneData);
    if (bres) {
        uint32_t uiKey = GetFirstKey();
        while (uiKey != HK_INVALID_SHAPE_KEY && bres) {
            hkpShapeCollection* pCollection = **this;
            if (pCollection) {
                alignas(16) char acBuffer[512];
                const hkpShape* pChild = static_cast<const hkpShapeContainer*>(pCollection)
                                             ->getChildShape(uiKey, acBuffer);
                if (pChild) {
                    bhkShape* ps = pChild->m_userData;
                    if (ps) {
                        bres = ps->CanShare(arCloneData);
                    }
                }
            }
            uiKey = GetNextKey(uiKey);
        }
    }
    return bres;
}

// Builds each child shape's display geometry under apNode, walking the Havok collection's keys until a key has no game
// shape.
void bhkShapeCollection::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    uint32_t uiKey = GetFirstKey();
    while (uiKey != HK_INVALID_SHAPE_KEY) {
        bhkShape* ps = nullptr;
        hkpShapeCollection* pCollection = **this;
        if (pCollection) {
            alignas(16) char acBuffer[512];
            const hkpShape* pChild = static_cast<const hkpShapeContainer*>(pCollection)->getChildShape(uiKey, acBuffer);
            if (pChild) {
                ps = pChild->m_userData;
            }
        }
        uiKey = GetNextKey(uiKey);
        if (!ps) {
            break;
        }
        ps->BuildDisplayGeometry(apNode, arDefColor);
    }
}
