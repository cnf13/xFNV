#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

class NiPalette;

// Renderer data that keeps a streamable copy of a source texture's pixels.
class NiPersistentSrcTextureRendererData : public NiTexture::RendererData {
public:
	enum PlatformID : uint32_t {
		NI_ANY = 0,
		NI_XENON = 1,
		NI_PS3 = 2,
		NI_DX9 = 3,
		NI_NUM_PLATFORM_IDS = 4,
	};

	NiPersistentSrcTextureRendererData();
	~NiPersistentSrcTextureRendererData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual void AllocateData(uint32_t uiLevels, uint32_t uiFaces, uint32_t uiBytes);
	virtual void FreeData();

	// Registers CreateObject again as the stream loader for this class.
	static void ResetStreamingFunctions();
	static NiObject* CreateObject();

	uint8_t* m_pucPixels;
	uint32_t* m_puiWidth;
	uint32_t* m_puiHeight;
	uint32_t* m_puiOffsetInBytes;
	uint32_t m_uiPadOffsetInBytes;
	uint32_t m_uiMipmapLevels;
	uint32_t m_uiPixelStride;
	uint32_t m_uiFaces;
	PlatformID m_eTargetPlatform;
	uint8_t* m_pucPristinePixels;
	uint32_t m_uiPristineMaxOffsetInBytes;
	uint32_t m_uiPristinePadOffsetInBytes;
	NiPointer<NiPalette> m_spPalette;

	static NiRTTI ms_RTTI;
	// SaveBinary writes the pristine pixels in place of the live ones while this is set.
	static inline bool ms_bToolModeStreaming;
};
