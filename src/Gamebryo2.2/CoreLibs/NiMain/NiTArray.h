#pragma once

#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class Animation;
class BGSCameraPath;
class BipedAnim;
class CombatGroup;
struct FurnitureMark;
class MobileObject;
class NiAVObject;
class NiD3DTextureStage;
class NiGeometry;
class NiNode;
class NiObjectGroup;
class NiParticles;
class NiScreenTexture;
class NiShaderConstantMapEntry;
class NiStream;
template <class T> class NiTPrimitiveSet;
class NiTexture;
class NiVBBlock;
class NiVBChip;
class TESIdleForm;
class TESObjectCELL;
class TESObjectREFR;
class Tile;
class bhkRigidBody;

// Growable array of elements with 16-bit counts. Empty slots hold T(0); m_usSize is the
// highest used index plus one, m_usESize the number of occupied (non-zero) slots. The element buffer comes from
// Allocator (NiTMallocInterface or NiTNewInterface).
template <class T, class Allocator = NiTMallocInterface<T>> class NiTArray : public NiMemObject {
public:
	// uiMaxSize slots, growing by uiGrowBy (Gamebryo's default arguments).
	NiTArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1);
	// The array has a vtable on PC; the destructor is its only slot.
	virtual ~NiTArray();

	// Index of the element, without a bounds check.
	T& operator[](uint32_t uiIndex);

	uint32_t GetSize() const;
	uint32_t GetEffectiveSize() const;
	uint32_t GetAllocatedSize() const;
	uint32_t GetGrowBy() const;

	// Reallocates the buffer to uiMaxSize slots; slots at or past it are cleared first, so smart pointers drop their
	// references.
	void SetSize(uint32_t uiMaxSize);

	void SetGrowBy(uint16_t uiGrowBy);

	T* GetBase() const;

	const T& GetAt(uint32_t uiIndex) const;

	// uiIndex must be below the allocated size.
	void SetAt(uint32_t uiIndex, const T& element);

	// Stores the element at uiIndex, growing the buffer by m_usGrowBy past uiIndex when uiIndex is beyond it; returns
	// uiIndex.
	uint32_t SetAtGrow(uint32_t uiIndex, const T& element);

	// Appends at m_usSize, growing the buffer when needed; returns the index used.
	uint32_t Add(const T& element);

	// Stores a non-zero element in the first empty slot (or appends it); returns the index used.
	uint32_t AddFirstEmpty(const T& element);

	// Shifts the slots from uiIndex up by one (growing when full) and puts the element at uiIndex.
	void InsertAt(uint32_t uiIndex, const T& element);

	// Empties slot uiIndex and returns what it held; removing the last slot shrinks the size by one.
	T RemoveAt(uint32_t uiIndex);

	// The last slot moves into uiIndex, so a packed array stays packed.
	T RemoveAtAndFill(uint32_t uiIndex);

	T RemoveEnd();
	void RemoveAll();

	// Moves the non-zero elements to the front in order and shrinks the storage to them.
	void Compact();

	// Drops trailing empty slots from the size.
	void UpdateSize();
	// The engine's out-of-line Init of each instantiation: uiMaxSize slots growing by uiGrowBy; only
	// per-instantiation definitions exist. IDA name; not in the PDB (the exe's body returns `this`, as its out-of-line
	// constructors do).
	NiTArray* Init(uint32_t uiMaxSize, uint32_t uiGrowBy);

	// The deleting destructors free the array through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	T* m_pBase;
	uint16_t m_usMaxSize;
	uint16_t m_usSize;
	uint16_t m_usESize;
	uint16_t m_usGrowBy;
};

template <class T, class Allocator>
NiTArray<T, Allocator>::NiTArray(uint32_t uiMaxSize, uint32_t uiGrowBy)
	: m_pBase(nullptr), m_usMaxSize(static_cast<uint16_t>(uiMaxSize)), m_usSize(0), m_usESize(0),
	  m_usGrowBy(static_cast<uint16_t>(uiGrowBy)) {
	if (m_usMaxSize) {
		m_pBase = Allocator::Allocate(m_usMaxSize);
	}
}

template <class T, class Allocator> NiTArray<T, Allocator>::~NiTArray() {
	Allocator::Deallocate(m_pBase);
}

// Re-initialises the array in place: uiMaxSize slots growing by uiGrowBy, none used (the exe's copies, 0x869E90,
// 0xA54190, 0xC63260 and the others, also store the vtable, which the native constructor does). Returns this.
template <class T, class Allocator>
NiTArray<T, Allocator>* NiTArray<T, Allocator>::Init(uint32_t uiMaxSize, uint32_t uiGrowBy) {
	m_usMaxSize = static_cast<uint16_t>(uiMaxSize);
	m_usGrowBy = static_cast<uint16_t>(uiGrowBy);
	m_usSize = 0;
	m_usESize = 0;
	if (m_usMaxSize != 0) {
		m_pBase = Allocator::Allocate(m_usMaxSize);
	} else {
		m_pBase = nullptr;
	}
	return this;
}

// The same instance's out-of-line destructor, which the PDB puts in Fallout Shared/TESFile.cpp.
using NiTArrayUInt = NiTArray<uint32_t, NiTMallocInterface<uint32_t>>;

// The child array of the scene-graph nodes; Fallout Shared/BipedAnim.cpp defines its destructor.
using NiTArrayNiPointerNiAVObjectNew = NiTArray<NiPointer<NiAVObject>, NiTNewInterface<NiPointer<NiAVObject>>>;

using NiTArrayNiPointerNiShaderConstantMapEntryNew =
	NiTArray<NiPointer<NiShaderConstantMapEntry>, NiTNewInterface<NiPointer<NiShaderConstantMapEntry>>>;

using NiTArrayNiPointerNiTextureNew = NiTArray<NiPointer<NiTexture>, NiTNewInterface<NiPointer<NiTexture>>>;

using NiTArrayNiPointerBSTempEffectNew = NiTArray<NiPointer<BSTempEffect>, NiTNewInterface<NiPointer<BSTempEffect>>>;
using NiTArrayNiTPrimitiveSetNiNodePtrMalloc =
	NiTArray<NiTPrimitiveSet<NiNode*>*, NiTMallocInterface<NiTPrimitiveSet<NiNode*>*>>;
using NiTArrayNiPointerNiAVObjectMalloc = NiTArray<NiPointer<NiAVObject>, NiTMallocInterface<NiPointer<NiAVObject>>>;

using NiTArrayREFR = NiTArray<TESObjectREFR*, NiTMallocInterface<TESObjectREFR*>>;

using NiTArrayBGSCameraPath = NiTArray<BGSCameraPath*, NiTMallocInterface<BGSCameraPath*>>;

using NiTArrayIdleForm = NiTArray<TESIdleForm*, NiTMallocInterface<TESIdleForm*>>;

// The particle effect's child array: its out-of-line destructor is defined in
// Fallout Shared/TempEffects/BSTempEffectParticle.cpp.
using NiTArrayAVObject = NiTArray<NiAVObject*, NiTMallocInterface<NiAVObject*>>;

using NiTArrayFurnitureMark = NiTArray<FurnitureMark, NiTNewInterface<FurnitureMark>>;
using NiTArrayNiParticlesPtr = NiTArray<NiParticles*, NiTMallocInterface<NiParticles*>>;

// Array of plain values (pointers, numbers): the buffer is raw heap memory.
template <class T> class NiTPrimitiveArray : public NiTArray<T, NiTMallocInterface<T>> {
public:
	NiTPrimitiveArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1);
	~NiTPrimitiveArray() override;
};

template <class T>
NiTPrimitiveArray<T>::NiTPrimitiveArray(uint32_t uiMaxSize, uint32_t uiGrowBy)
	: NiTArray<T, NiTMallocInterface<T>>(uiMaxSize, uiGrowBy) {}

// The NiTArray base's destructor frees the buffer.
template <class T> NiTPrimitiveArray<T>::~NiTPrimitiveArray() {}

// Array of objects (smart pointers, strings): the buffer is constructed and destroyed with new[]/delete[].
template <class T> class NiTObjectArray : public NiTArray<T, NiTNewInterface<T>> {
public:
	NiTObjectArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1);
	~NiTObjectArray() override;
};

template <class T>
NiTObjectArray<T>::NiTObjectArray(uint32_t uiMaxSize, uint32_t uiGrowBy)
	: NiTArray<T, NiTNewInterface<T>>(uiMaxSize, uiGrowBy) {}

// The NiTArray base's destructor frees the buffer.
template <class T> NiTObjectArray<T>::~NiTObjectArray() {}

// The combat manager's combat groups array (CombatGroups, a NiTPrimitiveArray): the vtable and the destructors the exe
// emits for the array and its primitive base, defined in Fallout/AI/Combat/CombatManager.cpp.
using NiTArrayCombatGroup = NiTArray<CombatGroup*, NiTMallocInterface<CombatGroup*>>;

// The guard of NiTArray<FurnitureMark>::SetAt's function static tcmp (not named in the PDB or IDA; name chosen here;
// bit 0 is set once tcmp is constructed).
inline uint32_t NiTArray_FurnitureMark_SetAt_tcmp_guard;
inline constexpr fnvd::Vtable NiTArray_TESObjectCELL_NiTMallocInterface_TESObjectCELL_vftable{};
inline constexpr fnvd::Vtable NiTArray_BGSAddonNode_NiTMallocInterface_BGSAddonNode_vftable{};
inline constexpr fnvd::Vtable NiTArray_char_NiTMallocInterface_char_vftable{};
inline constexpr fnvd::Vtable NiTArray_TESModel_NiTMallocInterface_TESModel_vftable{};
inline constexpr fnvd::Vtable NiTArray_TESTexture_NiTMallocInterface_TESTexture_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPointer_bhkRigidBody_NiTNewInterface_NiPointer_bhkRigidBody_vftable{};
inline constexpr fnvd::Vtable NiTArray_TESObjectREFR_NiTNewInterface_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTArray_BipedAnim_NiTNewInterface_BipedAnim_vftable{};
inline constexpr fnvd::Vtable NiTArray_Animation_NiTNewInterface_Animation_vftable{};
inline constexpr fnvd::Vtable NiTArray_TESRegion_NiTMallocInterface_TESRegion_vftable{};
inline constexpr fnvd::Vtable NiTArray_MobileObject_NiTMallocInterface_MobileObject_vftable{};
inline constexpr fnvd::Vtable NiTArray_Actor_NiTMallocInterface_Actor_vftable{};
inline constexpr fnvd::Vtable NiTArray_XMLStorage_NiTMallocInterface_XMLStorage_vftable{};
inline constexpr fnvd::Vtable NiTArray_Tile_NiTMallocInterface_Tile_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiControllerSequence_NiTNewInterface_NiPointer_NiControllerSequence_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_BSAnimNoteListener_BSAnimReceiverType_NiTMallocInterface_BSAnimNoteListener_BSAnimReceiverType_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiTransformController_NiTNewInterface_NiPointer_NiTransformController_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiTPrimitiveSet_NiBoneLODController_SkinInfo_NiTMallocInterface_NiTPrimitiveSet_NiBoneLODController_SkinInfo_vftable{};
inline constexpr fnvd::Vtable NiTArray_unsigned_short_NiTMallocInterface_unsigned_short_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiObjectGroup_NiTMallocInterface_NiObjectGroup_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiTexturingProperty_Map_NiTMallocInterface_NiTexturingProperty_Map_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiTexturingProperty_ShaderMap_NiTMallocInterface_NiTexturingProperty_ShaderMap_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiScreenGeometryData_ScreenElement_NiTMallocInterface_NiScreenGeometryData_ScreenElement_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiScreenPolygon_NiTNewInterface_NiPointer_NiScreenPolygon_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiScreenTexture_NiTNewInterface_NiPointer_NiScreenTexture_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiGeometry_NiTNewInterface_NiPointer_NiGeometry_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiPSysMeshEmitter_NiSkinnedEmitterData_NiTNewInterface_NiPointer_NiPSysMeshEmitter_NiSkinnedEmitterData_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiTObjectArray_NiPointer_NiAVObject_NiTMallocInterface_NiTObjectArray_NiPointer_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiRefObject_NiTNewInterface_NiPointer_NiRefObject_vftable{};
inline constexpr fnvd::Vtable NiTArray_BSAttachPointPair_NiTMallocInterface_BSAttachPointPair_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiD3DShaderDeclaration_NiPackerEntry_NiTMallocInterface_NiD3DShaderDeclaration_NiPackerEntry_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiD3DTextureStage_NiTNewInterface_NiPointer_NiD3DTextureStage_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiShaderConstantMapEntry_NiTNewInterface_NiPointer_NiShaderConstantMapEntry_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiVBChip_NiTMallocInterface_NiVBChip_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiVBBlock_NiTMallocInterface_NiVBBlock_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiVBDynamicSet_NiTMallocInterface_NiVBDynamicSet_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPick_Record_NiTMallocInterface_NiPick_Record_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiBoundingVolume_NiTMallocInterface_NiBoundingVolume_vftable{};
inline constexpr fnvd::Vtable NiTArray_voidPtr_NiTMallocInterface_voidPtr_vftable{};

// The child array's instance: the one NiNode's inlined destructor writes.
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiAVObject_NiTNewInterface_vftable{};

// The switch and LOD nodes' child revision array: the instance their inlined destructors write.
inline constexpr fnvd::Vtable NiTArray_unsignedInt_NiTMallocInterface_unsignedInt_vftable{};

// The master particle system's child array instance: the one its inlined destructor writes.
inline constexpr fnvd::Vtable NiTArray_NiParticlesPtr_NiTMallocInterface_NiParticlesPtr_vftable{};

// NiTexturingProperty's texture array.
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiTexture_NiTNewInterface_vftable{};

// The renderer's temp-effect array.
inline constexpr fnvd::Vtable NiTArray_NiPointer_BSTempEffect_NiTNewInterface_vftable{};

// The primitive containers' pointer arrays: of NiTPrimitiveSet<NiNode*> objects, and of NiAVObject pointers.
inline constexpr fnvd::Vtable NiTArray_NiTPrimitiveSet_NiNodePtr_NiTMallocInterface_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiPointer_NiAVObject_NiTMallocInterface_vftable{};

// The furniture marks' array instance: the one its out-of-line constructor writes.
inline constexpr fnvd::Vtable NiTArray_FurnitureMark_NiTNewInterface_FurnitureMark_vftable{};
inline constexpr fnvd::Vtable NiTArray_TESObjectREFR_NiTMallocInterface_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTArray_BGSCameraPath_NiTMallocInterface_BGSCameraPath_vftable{};
inline constexpr fnvd::Vtable NiTArray_TESIdleForm_NiTMallocInterface_TESIdleForm_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiAVObject_NiTMallocInterface_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_TESObjectCELL_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BGSAddonNode_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BSTextureManager_eTextureType_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiTexturingProperty_FilterMode_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BGSCameraPath_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_TESModel_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_TESTexture_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_char_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_TESIdleForm_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_TESRegion_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_MobileObject_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_CombatGroup_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_Actor_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_XMLStorage_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiTPrimitiveSet_NiNode_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiTPrimitiveSet_NiBoneLODController_SkinInfo_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_unsigned_short_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiObjectGroup_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiTexturingProperty_Map_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiParticles_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiPointer_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BSAttachPointPair_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_void_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiVBChip_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiVBDynamicSet_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiPick_Record_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiBoundingVolume_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BSAnimNoteListener_BSAnimReceiverTypePtr_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiAdditionalGeometryData_NiAGDDataBlockPtr_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiVBBlockPtr_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiTObjectArray_NiPointer_NiAVObject_Ptr_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_FaceGenUndo_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_Tile_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiScreenGeometryData_ScreenElement_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_unsigned_int_NiD3DShaderDeclaration_PackingParameters_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiDX9AdapterDesc_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiTexture_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_bhkRigidBody_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_BipedAnim_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_Animation_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_BSTempEffect_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiControllerSequence_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiInterpController_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiScreenPolygon_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiScreenTexture_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_BSShaderProperty_RenderPass_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiGeometry_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiPSysMeshEmitter_NiSkinnedEmitterData_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiD3DTextureStage_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiShaderConstantMapEntry_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_FurnitureMark_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiMetricsOutputModule_vftable{};
inline constexpr fnvd::Vtable NiTObjectArray_NiPointer_NiRefObject_vftable{};
inline constexpr fnvd::Vtable NiTArray_CombatGroup_NiTMallocInterface_CombatGroup_vftable{};
inline constexpr fnvd::Vtable NiTArray_FaceGenUndo_NiTMallocInterface_FaceGenUndo_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiInterpController_NiTNewInterface_NiPointer_NiInterpController_vftable{};
inline constexpr fnvd::Vtable NiTArray_void_NiStream_NiObject_NiTMallocInterface_void_NiStream_NiObject_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_NiPointer_NiMetricsOutputModule_NiTNewInterface_NiPointer_NiMetricsOutputModule_vftable{};
inline constexpr fnvd::Vtable
	NiTArray_unsigned_int_NiD3DShaderDeclaration_PackingParameters_NiTMallocInterface_unsigned_int_NiD3DShaderDeclaration_PackingParameters_vftable{};
inline constexpr fnvd::Vtable NiTArray_BSTextureManager_eTextureType_NiTMallocInterface_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiTexturingProperty_FilterMode_NiTMallocInterface_vftable{};
inline constexpr fnvd::Vtable NiTArray_BSAnimNoteReceiverTypePtr_NiTMallocInterface_vftable{};
inline constexpr fnvd::Vtable NiTArray_NiDX9AdapterDesc_NiTMallocInterface_NiDX9AdapterDesc_vftable{};
inline constexpr fnvd::Vtable NiTArray_const_charPtr_NiTMallocInterface_const_charPtr_vftable{};

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.inl"
