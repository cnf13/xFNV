#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9DynamicTextureData.h"

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

NiDX9DynamicTextureData::NiDX9DynamicTextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer)
	: NiDX9TextureData(pkTexture, pkRenderer), m_pkD3DLockableSysMemTexture(nullptr), m_bTextureLocked(false) {}

bool NiDX9DynamicTextureData::IsTextureLocked() const {
	return m_bTextureLocked;
}

// The renderer forgets the texture's data; the base does the rest.
NiDX9DynamicTextureData::~NiDX9DynamicTextureData() {
	m_pkRenderer->RemoveDynamicTextureData(m_pkTexture);
}

// A device that can lock textures gets one dynamic texture. Otherwise the texture is made without usage and a second
// one in system memory holds the pixels the CPU writes. The pixel format is copied into the renderer data first.
NiPixelFormat* NiDX9DynamicTextureData::CreateSurf(NiTexture* pkTexture, bool abDynTexturesCap) {
	if (!pkTexture) {
		return nullptr;
	}
	m_uiWidth = pkTexture->GetWidth();
	m_uiHeight = pkTexture->GetHeight();
	m_uiLevels = 1;
	const NiTexture::FormatPrefs kPrefs = pkTexture->m_kFormatPrefs;
	NiPixelFormat* pkFmt = NiDX9TextureData::FindClosestPixelFormat(&kPrefs, m_pkRenderer->m_aapkTextureFormats[0]);
	m_kPixelFormat = *pkFmt;
	const uint32_t uiRendererHint = pkFmt->m_uiRendererHint;
	IDirect3DDevice9* pkD3DDevice9 = m_pkRenderer->m_pkD3DDevice9;
	IDirect3DTexture9* pkD3DTexture = nullptr;
	HRESULT eD3DRet;
	if (abDynTexturesCap) {
		eD3DRet = pkD3DDevice9->CreateTexture(m_uiWidth, m_uiHeight, 1, D3DUSAGE_DYNAMIC, uiRendererHint,
			D3DPOOL_DEFAULT, &pkD3DTexture, nullptr);
		if (eD3DRet < 0) {
			const char* pcError = NiDX9ErrorString(eD3DRet);
			NiRenderer::Error("NiDX9DynamicTextureData::CreateSurf> Failed CreateImageSurface - %s", pcError);
			m_pkD3DTexture = nullptr;
			return nullptr;
		}
		m_pkD3DTexture = pkD3DTexture;
		return pkFmt;
	}
	eD3DRet = pkD3DDevice9->CreateTexture(m_uiWidth, m_uiHeight, 1, 0, uiRendererHint, D3DPOOL_DEFAULT, &pkD3DTexture,
		nullptr);
	if (eD3DRet < 0) {
		const char* pcError = NiDX9ErrorString(eD3DRet);
		NiRenderer::Error("NiDX9DynamicTextureData::CreateSurf> Failed CreateImageSurface - %s", pcError);
		m_pkD3DTexture = nullptr;
		return nullptr;
	}
	m_pkD3DTexture = pkD3DTexture;
	pkD3DTexture = nullptr;
	eD3DRet = pkD3DDevice9->CreateTexture(m_uiWidth, m_uiHeight, 1, 0, uiRendererHint, D3DPOOL_SYSTEMMEM, &pkD3DTexture,
		nullptr);
	if (eD3DRet < 0) {
		const char* pcError = NiDX9ErrorString(eD3DRet);
		NiRenderer::Error("NiDX9DynamicTextureData::CreateSurf> Failed CreateImageSurface - %s", pcError);
		m_pkD3DLockableSysMemTexture = nullptr;
		return nullptr;
	}
	m_pkD3DLockableSysMemTexture = pkD3DTexture;
	return pkFmt;
}

// Create is called with an NiDynamicTexture; the signature stays NiTexture.
NiDX9DynamicTextureData* NiDX9DynamicTextureData::Create(NiTexture* apTexture) {
	NiDynamicTexture* pkTexture = static_cast<NiDynamicTexture*>(apTexture);
	if (pkTexture->GetTiled()) {
		NiRenderer::Error("NiDX9DynamicTextureData Failed> Can not create a tiled dynamic texture.\n");
		return nullptr;
	}
	NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
	NiDX9DynamicTextureData* pkThis =
		static_cast<NiDX9DynamicTextureData*>(NiMemObject::operator new(sizeof(NiDX9DynamicTextureData)));
	if (pkThis) {
		::new (pkThis) NiDX9DynamicTextureData(apTexture, pkRenderer);
	}
	bool bDynamicTexturesCapable = pkRenderer->m_bDynamicTexturesCapable;
	if (pkThis->CreateSurf(apTexture, bDynamicTexturesCapable)) {
		pkThis->m_pkTexture->m_pkRendererData = pkThis;
		return pkThis;
	}
	delete pkThis;
	return nullptr;
}
