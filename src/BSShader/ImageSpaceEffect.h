#pragma once

#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class ImageSpaceManager;
struct ImageSpaceTexture;
class NiDX9Renderer;
class NiScreenElements;

void ImageSpaceEffect_OutputArray_SetAt(NiTPrimitiveArray<int32_t>* apArray, uint32_t auiIndex,
	const int32_t& aiElement);
void ImageSpaceEffect_OutputArray_SetSize(NiTPrimitiveArray<int32_t>* apArray, uint32_t auiMaxSize);

class ImageSpaceEffect {
public:
	// One input of an effect: the texture slot it reads and the filter mode (NiTexturingProperty's FilterMode) it
	// samples with.
	struct EffectInput {
		int32_t iTexIndex;
		NiTexturingProperty::FilterMode iFilterMode;
	};

	ImageSpaceEffect();
	virtual ~ImageSpaceEffect();

	// The exe passes the caller's whole dword on; bool would be zero-extended (ImageSpaceEffect::Render pushes
	// abEndFrame's own slot as is to every sub-effect's Render). PDB: bool abEndFrame.
	virtual void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame);
	virtual void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {}
	virtual void Shutdown() {}
	virtual void BorrowTextures(ImageSpaceEffectParam* pParam) {}
	virtual void ReturnTextures() {}
	virtual bool IsActive();
	virtual bool UpdateParams(ImageSpaceEffectParam* pParam);

	// Xbox GetEffectName is not a PC virtual. The PC base has eight slots.
	// Sets the effect, its shader parameters and its output at aiEffectIndex.
	void SetEffect(int32_t aiEffectIndex, ImageSpaceEffect* apEffect, ImageSpaceEffectParam* apParam, int32_t aiOutput);
	// Resizes the effect, parameter, input and output arrays to aiCount.
	void SetEffectCount(int32_t aiCount);
	// Sets the effect's two input textures and their filter modes (NiTexturingProperty's
	// FilterMode) at auiEffectIndex.
	void SetEffectInputs(int32_t auiEffectIndex, int32_t auiInput1, int32_t auiInput2,
		NiTexturingProperty::FilterMode aeFilterMode1, NiTexturingProperty::FilterMode aeFilterMode2);
	// Sets the effect's input texture and its filter mode (NiTexturingProperty's FilterMode)
	// at auiEffectIndex.
	void SetEffectInputs(int32_t auiEffectIndex, int32_t auiInput, NiTexturingProperty::FilterMode aeFilterMode);
	// Sets the
	// effect's four input textures and their filter modes (NiTexturingProperty's FilterMode) at auiEffectIndex.
	void SetEffectInputs(int32_t auiEffectIndex, int32_t auiInput1, int32_t auiInput2, int32_t auiInput3,
		int32_t auiInput4, NiTexturingProperty::FilterMode aeFilterMode1, NiTexturingProperty::FilterMode aeFilterMode2,
		NiTexturingProperty::FilterMode aeFilterMode3, NiTexturingProperty::FilterMode aeFilterMode4);
	// Sets bIsActive (thiscall, ret 4, 0x4F15A0; the same function as
	// TESForm::SetFormType).
	void SetActive(bool abActive);
	// Binds apTex as input aiIndex, or clears that input when apTex is null, sampling it
	// with aiFilterMode (NiTexturingProperty's FilterMode; thiscall, ret 0xC, 0xBA3CB0).
	void SetTexture(int32_t aiIndex, ImageSpaceTexture* apTex, NiTexturingProperty::FilterMode aiFilterMode);
	// Binds the inputs and the output of the effect at aiEffectIndex to that effect's texture slots.
	void ApplyEffectTextures(int32_t aiEffectIndex);
	void SetEffectInputs(int32_t aiEffectIndex, int32_t aiInput1, int32_t aiInput2, int32_t aiInput3,
		NiTexturingProperty::FilterMode aiFilterMode1, NiTexturingProperty::FilterMode aiFilterMode2,
		NiTexturingProperty::FilterMode aiFilterMode3);

	// The exe's deleting destructors free effects through the MemoryManager.
	static void operator delete(void* apMemory);

	bool bIsActive;
	bool bParamsChanged;
	uint8_t padding06[2];
	// Malloc arrays, not object arrays: SetEffectCount resizes them with the pointer arrays' shared SetSize,
	// SetEffect stores into them with the shared SetAt and no reference counting, and the
	// destructor frees them with NiFree.
	NiTPrimitiveArray<NiPointer<ImageSpaceEffect>> pEffectList;
	NiTPrimitiveArray<NiPointer<ImageSpaceShaderParam>> pEffectParamList;
	NiTPrimitiveArray<ImageSpaceTexture*> pTextures;
	NiTPrimitiveArray<EffectInput*> pEffectInputs;
	// The output texture index of each effect (PDB: NiTPrimitiveArray<int>; ApplyEffectTextures 0xBA3E30 indexes
	// pTextures with the element).
	NiTPrimitiveArray<int32_t> pEffectOutput;
};

inline constexpr fnvd::Vtable ImageSpaceEffect_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceEffect_vftable{};

// The vtable the inlined constructors of the two NiPointer arrays
// (pEffectList, pEffectParamList) store.
inline constexpr fnvd::Vtable ImageSpaceEffect_NiPointerArray_vftable{};

// The vtable the inlined constructors of the three raw pointer
// arrays (pTextures, pEffectInputs, pEffectOutput) store.
inline constexpr fnvd::Vtable ImageSpaceEffect_PointerArray_vftable{};

// The effects are not reference counted: the exe's NiPointer arrays of them (pEffectList) store and drop the pointer
// with the pointer arrays' shared SetAt (0xBA3AA0) and nothing else, so NiPointer's reference counting is nothing
// for them.
inline void NiIncRefCount(ImageSpaceEffect*) {}

inline void NiDecRefCount(ImageSpaceEffect*) {}
