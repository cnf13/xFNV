#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"

#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

NiRTTI NiRenderedCubeMap::ms_RTTI("NiRenderedCubeMap", &NiRenderedTexture::ms_RTTI);

// The exe inlines NiTexture's constructor and the part of NiRenderedTexture's that clears the buffer, without
// storing NiRenderedTexture's vtable.
NiRenderedCubeMap::NiRenderedCubeMap() : m_eFace(FACE_POS_X) {}

// The exe inlines the face buffers' NiPointer destructors (the last face first), ~NiRenderedTexture's buffer release
// and ~NiTexture (without NiRenderedTexture's vtable), and ends in ~NiObjectNET.
// The face buffers release themselves.
NiRenderedCubeMap::~NiRenderedCubeMap() {}

// The width of the first face buffer.
uint32_t NiRenderedCubeMap::GetWidth() const {
	return m_aspFaceBuffers[FACE_POS_X]->GetWidth();
}

// The height of the first face buffer.
uint32_t NiRenderedCubeMap::GetHeight() const {
	return m_aspFaceBuffers[FACE_POS_X]->GetHeight();
}

// Each face buffer is assigned as the exe's inlined NiPointer assignment does: the old one released before the new
// one is stored and referenced. The exe stores through the result even when the allocation failed; a cube map the
// renderer gives no data is deleted.
NiRenderedCubeMap* NiRenderedCubeMap::Create(uint32_t uiSize, NiRenderer* apkRenderer,
	const NiTexture::FormatPrefs& kPrefs) {
	if (!apkRenderer || !uiSize || (uiSize & (uiSize - 1)) != 0) {
		return nullptr;
	}
	void* pMemory = NiMemObject::operator new(sizeof(NiRenderedCubeMap));
	NiRenderedCubeMap* pkCubeMap = pMemory ? ::new (pMemory) NiRenderedCubeMap : nullptr;
	pkCubeMap->m_kFormatPrefs = kPrefs;
	for (uint32_t i = 0; i < FACE_NUM; ++i) {
		Ni2DBuffer* pkBuffer = Ni2DBuffer::Create_(uiSize, uiSize);
		pkCubeMap->m_aspFaceBuffers[i] = pkBuffer;
	}
	pkCubeMap->bForceD3DFormat = NiRenderedTexture::bStaticForceD3DFormat;
	pkCubeMap->iForcedD3DFormat = NiRenderedTexture::iStaticForcedD3DFormat;
	pkCubeMap->bRenderedTexture = NiRenderedTexture::bStaticRenderedTexture;
	if (!apkRenderer->CreateRenderedCubeMapRendererData(pkCubeMap)) {
		delete pkCubeMap;
		return nullptr;
	}
	return pkCubeMap;
}
