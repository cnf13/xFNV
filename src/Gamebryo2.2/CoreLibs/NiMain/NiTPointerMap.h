#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Reconstruction.hpp"

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
template <class TKEY, class TVAL> class NiTPointerMap : public NiTMapBase<NiTPointerAllocator<size_t>, TKEY, TVAL> {
public:
	// uiHashSize cleared buckets, no items (Gamebryo's default 37 buckets).
	NiTPointerMap(uint32_t uiHashSize = 37) : NiTMapBase<NiTPointerAllocator<size_t>, TKEY, TVAL>(uiHashSize) {}
	~NiTPointerMap() override;

	NiTMapItem<TKEY, TVAL>* NewItem() override;
	void DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) override;
};

// Empties the map while its own DeleteItem is in place; NiTMapBase's destructor frees the buckets.
template <class TKEY, class TVAL> NiTPointerMap<TKEY, TVAL>::~NiTPointerMap() {
	this->RemoveAll();
}

// The item comes from the shared node pool; the exe folds every instantiation into one body (0x486A90).
template <class TKEY, class TVAL> NiTMapItem<TKEY, TVAL>* NiTPointerMap<TKEY, TVAL>::NewItem() {
	return static_cast<NiTMapItem<TKEY, TVAL>*>(this->m_kAllocator.Allocate());
}

// Clears the value (a smart pointer releases its object), then returns the item to the shared node pool (Gamebryo's
// DeleteItem; 0x4B0130, 0x666050, 0xA0D7C0, 0xAE8EF0 call the pool's Deallocate 0x45CEE0, which 0xA82620, 0xA638C0
// and 0xE6E8A0 inline).
template <class TKEY, class TVAL> void NiTPointerMap<TKEY, TVAL>::DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) {
	pkItem->m_val = 0;
	this->m_kAllocator.Deallocate(pkItem);
}

using NiTPointerMapREFRWadingWater = NiTPointerMap<TESObjectREFR*, WadingWaterData*>;

using NiTPointerMapTESObjectTREEModel = NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>;

// The land's grass map: its vtable and the complete destructor the exe emits for the instantiation (defined in
// Fallout Shared/TESLand.cpp).
using NiTPointerMapUIntGrassAreaParam = NiTPointerMap<uint32_t, TESGrassAreaParam**>;

using NiTPointerMapUIntRefList = NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>;

inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_TESFile_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_TESForm_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_short_AnimSequenceBase_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiObject_NiObject_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiObject_bool_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_TESObjectREFR_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_TESWaterForm_bool_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_bool_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_TESObjectREFR_bool_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_bool_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_TESObjectCELL_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_NavMeshInfo_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_BSSoundHandle_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_BGSFormChanges_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_BSSimpleList_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_BSSimpleList_ExteriorCellReferenceData_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_void_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_char_BSSimpleList_LoadFormHeader_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_Tile_Value_Tile_Reaction_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_XMLStorage_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_Tile_BuildStorage_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiControllerSequence_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiPointer_NiSequence_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiNode_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_unsigned_short_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiObject_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_float_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiVBBlock_NiDX9Renderer_PrePackObject_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_HWND___NiPointer_NiRenderTargetGroup_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiRenderedTexture_NiDX9RenderedTextureData_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiRenderedCubeMap_NiDX9RenderedCubeMapData_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiDynamicTexture_NiDX9DynamicTextureData_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_NiDX9IndexBufferManager_NiDX9IBInfo_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiPointer_NiShaderLibrary_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiD3DVertexShader_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiD3DPixelShader_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiD3DShaderProgramCreator_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_NiVBSet_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_NiVBDynamicSet_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_NiVBChip_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_BSGameSoundPtr_vftable{};
inline constexpr fnvd::Vtable NiTMapBase_int_BSGameSoundPtr_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_BSSoundInfoPtr_vftable{};
inline constexpr fnvd::Vtable NiTMapBase_int_BSSoundInfoPtr_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_int_NiPointerAVObject_vftable{};
inline constexpr fnvd::Vtable NiTMapBase_int_NiPointerAVObject_vftable{};

// The wading-water map's vtable and the destructor the exe emits for it (defined in Fallout Shared/TESWater.cpp).
inline constexpr fnvd::Vtable NiTPointerMap_TESObjectREFR_WadingWaterData_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_TESObjectTREE_BSTreeModel_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_UInt_TESGrassAreaParamPtrPtr_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_UInt_BSSimpleListREFR_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_unsigned_int_ChangeData_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_char_NiObject_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap__D3DFORMAT_NiDX9DeviceDesc_DisplayFormatInfo_RenderTargetInfo_vftable{};
inline constexpr fnvd::Vtable NiTPointerMap_NiLight_NiDX9LightManager_LightEntry_vftable{};
