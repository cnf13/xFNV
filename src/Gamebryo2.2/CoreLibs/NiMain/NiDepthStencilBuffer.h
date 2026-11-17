#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiPixelFormat;
class NiRenderer;

// A depth/stencil render buffer.
class NiDepthStencilBuffer : public Ni2DBuffer {
public:
	NiDepthStencilBuffer();
	~NiDepthStencilBuffer() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	static NiDepthStencilBuffer* Create(uint32_t uiWidth, uint32_t uiHeight, NiRenderer* pkRenderer,
		const NiPixelFormat& kFormat, Ni2DBuffer::MultiSamplePreference eMSAAPref);
	// A 32-bit depth, 8-bit stencil buffer of pkBuffer's size and multisampling; null without a buffer or renderer.
	static NiDepthStencilBuffer* CreateCompatible(Ni2DBuffer* pkBuffer, NiRenderer* apkRenderer);
	static NiDepthStencilBuffer* Create(uint32_t uiWidth, uint32_t uiHeight, Ni2DBuffer::RendererData* pkData);

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiDepthStencilBuffer);

inline constexpr fnvd::Vtable NiDepthStencilBuffer_vftable{};

// IDA name (NiDX9AdditionalDepthStencilBufferData::bCreateDiscardable); not in the PDB. Whether the DX9 renderer
// creates new depth/stencil surfaces discardable (1 by default; cleared around some creations).
inline uint32_t bCreateDiscardable = 1;
