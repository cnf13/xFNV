#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"

#include <stdint.h>

class BSTreeModel;
class NiNode;
class NiRenderedCubeMap;
class NiShader;
class NiVBChip;
class TESFile;
class TESGrassAreaParam;
class TESObjectCELL;
class TESObjectREFR;
class TESObjectTREE;
class WadingWaterData;

// Hash map whose items come from the engine's shared pool of 12-byte nodes, so keys and values must be
// pointer-sized.
template <class TKEY, class TVAL>
class NiTPointerMap : public NiTMapBase<NiTPointerAllocator<size_t>, TKEY, TVAL> {
public:
    // uiHashSize cleared buckets, no items (Gamebryo's default 37 buckets).
    NiTPointerMap(uint32_t uiHashSize = 37) : NiTMapBase<NiTPointerAllocator<size_t>, TKEY, TVAL>(uiHashSize) {}
    ~NiTPointerMap() override;

    NiTMapItem<TKEY, TVAL>* NewItem() override;
    void DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) override;
};

// Empties the map while its own DeleteItem is in place; NiTMapBase's destructor frees the buckets.
template <class TKEY, class TVAL>
NiTPointerMap<TKEY, TVAL>::~NiTPointerMap() {
    this->RemoveAll();
}

// The item comes from the shared node pool.
template <class TKEY, class TVAL>
NiTMapItem<TKEY, TVAL>* NiTPointerMap<TKEY, TVAL>::NewItem() {
    return static_cast<NiTMapItem<TKEY, TVAL>*>(this->m_kAllocator.Allocate());
}

// Clears the value (a smart pointer releases its object), then returns the item to the shared node pool.
template <class TKEY, class TVAL>
void NiTPointerMap<TKEY, TVAL>::DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) {
    pkItem->m_val = 0;
    this->m_kAllocator.Deallocate(pkItem);
}

using NiTPointerMapREFRWadingWater = NiTPointerMap<TESObjectREFR*, WadingWaterData*>;

using NiTPointerMapTESObjectTREEModel = NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>;

// The land's grass map; its vtable and destructor are defined in Fallout Shared/TESLand.cpp.
using NiTPointerMapUIntGrassAreaParam = NiTPointerMap<uint32_t, TESGrassAreaParam**>;

using NiTPointerMapUIntRefList = NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>;
