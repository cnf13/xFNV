#include "BSShader/BSTextureManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectMap.h"
#include "BSShader/ImageSpaceEffectWaterFFT.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/ehvec.h"

BSTextureManager::BSTextureManager() {
	NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
	uint32_t uiWidth;
	uint32_t uiHeight;
	D3DFORMAT iD3DFormat;
	D3DFORMAT iEDRAMFormat;
	uint32_t uFlags;
	D3DMULTISAMPLE_TYPE eMSAAType;
	LookupTextureParameters(pRenderer, TEX_SHADOWS_SHADOWMAPFRUSTUM, uiWidth, uiHeight, iD3DFormat, iEDRAMFormat,
		uFlags, eMSAAType);
	NiRenderTargetGroup* pGroup = pRenderer->GetDefaultRenderTargetGroup();
	Ni2DBuffer::MultiSamplePreference eMSAAPref;
	switch (eMSAAType) {
	case D3DMULTISAMPLE_2_SAMPLES:
		eMSAAPref = Ni2DBuffer::MULTISAMPLE_2;
		break;
	case D3DMULTISAMPLE_4_SAMPLES:
		eMSAAPref = Ni2DBuffer::MULTISAMPLE_4;
		break;
	case D3DMULTISAMPLE_8_SAMPLES:
		eMSAAPref = Ni2DBuffer::MULTISAMPLE_8;
		break;
	default:
		eMSAAPref = Ni2DBuffer::MULTISAMPLE_NONE;
		break;
	}

	if (pGroup->GetDepthStencilBuffer()) {
		spShadowDepthBuffer = pGroup->GetDepthStencilBuffer();
	} else if (eMSAAPref == Ni2DBuffer::MULTISAMPLE_NONE) {
		uint32_t uiGroupWidth = pGroup->GetWidth(0);
		uint32_t uiBufferWidth = uiWidth > uiGroupWidth ? uiWidth : uiGroupWidth;
		uint32_t uiGroupHeight = pGroup->GetHeight(0);
		uint32_t uiBufferHeight = uiHeight > uiGroupHeight ? uiHeight : uiGroupHeight;
		Ni2DBuffer::RendererData* pBufferData = pGroup->GetBuffer(0)->m_spRendererData.m_pObject;
		const NiPixelFormat* pFrontFormat = pBufferData ? pBufferData->m_pkPixelFormat : nullptr;
		const NiPixelFormat* pFormat = NiRenderer::ms_pkRenderer->FindClosestDepthStencilFormat(pFrontFormat, 32, 8);
		bCreateDiscardable = 0;
		spShadowDepthBuffer = NiDepthStencilBuffer::Create(uiBufferWidth, uiBufferHeight, pRenderer, *pFormat,
			Ni2DBuffer::MULTISAMPLE_NONE);
		bCreateDiscardable = 1;
		pGroup->AttachDepthStencilBuffer(spShadowDepthBuffer.m_pObject);
	} else {
		// With multisampling the group gets its own depth buffer, and the shadow maps one at least the frustum
		// shadow map's and the main image space's size.
		Ni2DBuffer::RendererData* pBufferData = pGroup->GetBuffer(0)->m_spRendererData.m_pObject;
		const NiPixelFormat* pFrontFormat = pBufferData ? pBufferData->m_pkPixelFormat : nullptr;
		const NiPixelFormat* pFormat = NiRenderer::ms_pkRenderer->FindClosestDepthStencilFormat(pFrontFormat, 32, 8);
		bCreateDiscardable = 0;
		uint32_t uiGroupHeight = pGroup->GetHeight(0);
		uint32_t uiGroupWidth = pGroup->GetWidth(0);
		NiDepthStencilBuffer* pGroupBuffer =
			NiDepthStencilBuffer::Create(uiGroupWidth, uiGroupHeight, pRenderer, *pFormat, eMSAAPref);
		bCreateDiscardable = 1;
		pGroup->AttachDepthStencilBuffer(pGroupBuffer);
		uint32_t uiMainWidth;
		uint32_t uiMainHeight;
		LookupTextureParameters(pRenderer, TEX_MAIN_IMAGESPACE, uiMainWidth, uiMainHeight, iD3DFormat, iEDRAMFormat,
			uFlags, eMSAAType);
		uint32_t uiBufferWidth = uiWidth > uiMainWidth ? uiWidth : uiMainWidth;
		uint32_t uiBufferHeight = uiHeight > uiMainHeight ? uiHeight : uiMainHeight;
		bCreateDiscardable = 0;
		spShadowDepthBuffer = NiDepthStencilBuffer::Create(uiBufferWidth, uiBufferHeight, pRenderer, *pFormat,
			Ni2DBuffer::MULTISAMPLE_NONE);
		bCreateDiscardable = 1;
	}

	spSharedDepthBuffer = nullptr;
	for (uint32_t i = 0; i < 5; ++i) {
		spUnusedAliasedTextureList[i].RemoveAll();
		spUsedAliasedTextureList[i].RemoveAll();
	}
}

// Empties the frustum shadow map pools, frees the pooled render target records, shrinks the shadow maps to none
// (unless 0x11A2FE4 is set) and empties the unused aliased lists; the members' destructors do the rest.
BSTextureManager::~BSTextureManager() {
	while (usedFrustumShadowRTPool.m_uiCount) {
		usedFrustumShadowRTPool.RemoveTail();
	}
	while (unusedFrustumShadowRTPool.m_uiCount) {
		unusedFrustumShadowRTPool.RemoveTail();
	}
	while (usedRTPool.m_uiCount) {
		RenderedTextureData* pData = usedRTPool.m_pkHead->m_element;
		pData->spRenderedTexture = nullptr;
		usedRTPool.RemoveHead();
		delete pData;
	}
	while (unusedRTPool.m_uiCount) {
		RenderedTextureData* pData = unusedRTPool.m_pkHead->m_element;
		pData->spRenderedTexture = nullptr;
		unusedRTPool.RemoveHead();
		delete pData;
	}
	if (!bSkipShadowTextureRelease) {
		SetFrustumShadowTextureCount(static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer), 0);
	}
	for (uint32_t i = 0; i < 5; ++i) {
		while (spUnusedAliasedTextureList[i].m_uiCount) {
			spUnusedAliasedTextureList[i].RemoveHead();
		}
	}
	spShadowDepthBuffer = nullptr;
	spSharedDepthBuffer = nullptr;
}

// KEEP: an uninitialised read. LookupTextureParameters (0xB6C2C0) has no case for TEX_TESWATER_REFRACTION or
// TEX_UNKNOWN, so for those types it leaves the width, height, D3D format and flags unwritten, and the exe then reads
// those locals (the flags OR'd at 0xB6E3DE, the sizes passed to 0xB6D5E0).
// The aliased types hand out the head of their unused list, moved to the head of the used list. The exe's
// cleanup-only C++ EH frame is left out.
BSRenderedTexture* BSTextureManager::BorrowRenderedTexture(NiDX9Renderer* apRenderer, eTextureType aeType,
	uint32_t auiAdditionalFlags, NiRenderedTexture* apAliasTex, uint32_t uiAliasOffset) {
	if (apAliasTex) {
		return nullptr;
	}
	int32_t iList;
	switch (aeType) {
	case TEX_SHADOWS_SHADOWMAPFRUSTUM:
		iList = 0;
		break;
	case TEX_TESWATER_DEPTH:
		iList = 1;
		break;
	case TEX_TESWATER_REFLECTION:
		iList = 2;
		break;
	case TEX_TESWATER_BLUR:
		iList = 3;
		break;
	case TEX_TESWATER_NOISE_NORMALMAP:
		iList = 4;
		break;
	default:
		iList = -1;
		break;
	}
	if (iList >= 0 && spUnusedAliasedTextureList[iList].m_uiCount) {
		BSRenderedTexture* pTexture = spUnusedAliasedTextureList[iList].m_pkHead->m_element.m_pObject;
		{
			NiPointer<BSRenderedTexture> spTexture(pTexture);
			spUsedAliasedTextureList[iList].AddHead(spTexture);
		}
		spUnusedAliasedTextureList[iList].RemoveHead();
		return pTexture;
	}
	uint32_t uiWidth;
	uint32_t uiHeight;
	D3DFORMAT eD3DFormat;
	D3DFORMAT eEDRAMFormat;
	uint32_t uiFlags;
	D3DMULTISAMPLE_TYPE eMSAAType;
	LookupTextureParameters(apRenderer, aeType, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags, eMSAAType);
	return BorrowRenderedTexture(apRenderer, uiWidth, uiHeight, uiFlags | auiAdditionalFlags, eD3DFormat, eEDRAMFormat,
		aeType, eMSAAType, nullptr, uiAliasOffset);
}

BSRenderedTexture* BSTextureManager::BorrowRenderedTexture(NiDX9Renderer* apRenderer, uint32_t auiWidth,
	uint32_t auiHeight, uint32_t auFlags, D3DFORMAT aiD3DFormat, D3DFORMAT aiEDRAMFormat, eTextureType aeType,
	D3DMULTISAMPLE_TYPE aeMSAAType, NiRenderedTexture* apAliasTex, uint32_t uiAliasOffset) {
	RenderedTextureData* pData = nullptr;
	uint32_t uiScreenWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0);
	uint32_t uiScreenHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0);

	// A renderable format the device can't render to: 16-bit integer RGBA falls back to 16-bit float RGBA, any
	// other is kept. L8 passes when either L8 or G16R16 is supported.
	if (aiD3DFormat && !(auFlags & BT_FLAG_NOTRENDERABLE)) {
		bool bSupported;
		bool bL8 = false;
		switch (aiD3DFormat) {
		case D3DFMT_R8G8B8:
			bSupported = BSShaderManager::bRenderTargetSupported[5];
			break;
		case D3DFMT_A8R8G8B8:
			bSupported = BSShaderManager::bRenderTargetSupported[6];
			break;
		case D3DFMT_X8R8G8B8:
			bSupported = BSShaderManager::bRenderTargetSupported[7];
			break;
		case D3DFMT_R5G6B5:
			bSupported = BSShaderManager::bRenderTargetSupported[0];
			break;
		case D3DFMT_X1R5G5B5:
			bSupported = BSShaderManager::bRenderTargetSupported[1];
			break;
		case D3DFMT_A1R5G5B5:
			bSupported = BSShaderManager::bRenderTargetSupported[2];
			break;
		case D3DFMT_A4R4G4B4:
			bSupported = BSShaderManager::bRenderTargetSupported[3];
			break;
		case D3DFMT_A16B16G16R16:
			bSupported = BSShaderManager::bRenderTargetSupported[10];
			break;
		case D3DFMT_L8:
			bL8 = BSShaderManager::bRenderTargetSupported[12];
			[[fallthrough]];
		case D3DFMT_G16R16:
			bSupported = BSShaderManager::bRenderTargetSupported[13] || bL8;
			break;
		case D3DFMT_L16:
			bSupported = BSShaderManager::bRenderTargetSupported[4];
			break;
		case D3DFMT_G16R16F:
			bSupported = BSShaderManager::bRenderTargetSupported[14];
			break;
		case D3DFMT_A16B16G16R16F:
			bSupported = BSShaderManager::bRenderTargetSupported[9];
			break;
		case D3DFMT_R32F:
			bSupported = BSShaderManager::bRenderTargetSupported[8];
			break;
		case D3DFMT_A32B32G32R32F:
			bSupported = BSShaderManager::bRenderTargetSupported[11];
			break;
		default:
			bSupported = false;
			break;
		}
		if (!bSupported && aiD3DFormat == D3DFMT_A16B16G16R16) {
			aiD3DFormat = D3DFMT_A16B16G16R16F;
		}
	}

	// The first unused texture of the asked size (0: the screen's), the same formats and the same flags apart from
	// BT_FLAG_CLEAR and BT_FLAG_PRECREATE; the water reflection and blur targets also match on the type.
	NiTListItem<RenderedTextureData*>* kPos = unusedRTPool.m_pkHead;
	while (kPos && !pData) {
		RenderedTextureData* pCandidate = kPos->m_element;
		NiRenderedTexture* pTexture = pCandidate->spRenderedTexture.m_pObject->spTexture[0].m_pObject;
		uint32_t uiWidth = pTexture ? pTexture->GetWidth() : 0;
		bool bMatch = true;
		if (uiWidth != auiWidth) {
			if (auiWidth) {
				bMatch = false;
			} else {
				pTexture = pCandidate->spRenderedTexture.m_pObject->spTexture[0].m_pObject;
				if ((pTexture ? pTexture->GetWidth() : 0) != uiScreenWidth) {
					bMatch = false;
				}
			}
		}
		if (bMatch) {
			pTexture = pCandidate->spRenderedTexture.m_pObject->spTexture[0].m_pObject;
			uint32_t uiHeight = pTexture ? pTexture->GetHeight() : 0;
			if (uiHeight != auiHeight) {
				if (auiHeight) {
					bMatch = false;
				} else {
					pTexture = pCandidate->spRenderedTexture.m_pObject->spTexture[0].m_pObject;
					if ((pTexture ? pTexture->GetHeight() : 0) != uiScreenHeight) {
						bMatch = false;
					}
				}
			}
		}
		if (bMatch && (pCandidate->iD3DFormat != aiD3DFormat || pCandidate->iEDRAMFormat != aiEDRAMFormat)) {
			bMatch = false;
		}
		if (bMatch) {
			if (aeType == TEX_TESWATER_REFLECTION) {
				bMatch = pCandidate->eType == TEX_TESWATER_REFLECTION;
			} else if (aeType == TEX_TESWATER_BLUR) {
				bMatch = pCandidate->eType == TEX_TESWATER_BLUR;
			}
		}
		if (bMatch &&
			(pCandidate->uFlags | (BT_FLAG_CLEAR | BT_FLAG_PRECREATE)) ==
				(auFlags | (BT_FLAG_CLEAR | BT_FLAG_PRECREATE))) {
			// The exe leaves kPos on the match and removes that node below.
			pData = pCandidate;
		} else {
			kPos = kPos->m_pkNext;
		}
	}

	if (pData) {
		unusedRTPool.RemovePos(kPos);
		pData->bUsedThisFrame = true;
	} else {
		// The exe inlines RenderedTextureData's constructor (spRenderedTexture = null); eType is left unset.
		void* pMemory = MemoryManager_Singleton.Allocate(sizeof(RenderedTextureData));
		pData = pMemory ? ::new (pMemory) RenderedTextureData : nullptr;
		pData->iD3DFormat = aiD3DFormat;
		pData->iEDRAMFormat = aiEDRAMFormat;
		pData->uFlags = auFlags;
		pData->bUsedThisFrame = true;
		BSRenderedTexture* pTexture;
		if (auiWidth > 0 && auiHeight > 0) {
			pTexture = CreateRenderedTexture(apRenderer, auiWidth, auiHeight, aiD3DFormat, aiEDRAMFormat, auFlags,
				aeType, aeMSAAType, apAliasTex, uiAliasOffset);
		} else {
			pTexture = CreateRenderedTexture(apRenderer, uiScreenWidth, uiScreenHeight, aiD3DFormat, aiEDRAMFormat,
				auFlags, aeType, aeMSAAType, apAliasTex, uiAliasOffset);
		}
		pData->spRenderedTexture = pTexture;
	}

	// The node comes from the node pool, called with the list's count field as its `this`.
	NiTListItem<RenderedTextureData*>* pkNode =
		static_cast<NiTListItem<RenderedTextureData*>*>(reinterpret_cast<NiTPointerAllocator<uint32_t>&>(usedRTPool
																											 .m_uiCount)
				.Allocate());
	pkNode->m_element = pData;
	usedRTPool.AddNodeTail(pkNode);
	return pData->spRenderedTexture.m_pObject;
}

// The exe's cleanup-only C++ EH frame is left out.
BSRenderedTexture* BSTextureManager::CreateRenderedTexture(NiDX9Renderer* apRenderer, uint32_t auiWidth,
	uint32_t auiHeight, D3DFORMAT aiD3DFormat, D3DFORMAT aiEDRAMFormat, uint32_t auFlags,
	BSTextureManager::eTextureType aeType, D3DMULTISAMPLE_TYPE aeMSAAType, NiRenderedTexture* apAliasTex,
	uint32_t uiAliasOffset) {
	uint32_t uisafeWidth;
	uint32_t uisafeHeight;
	NiTexture::FormatPrefs pformat;
	FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
	Ni2DBuffer::MultiSamplePreference emultiSamplePreference = Ni2DBuffer::MULTISAMPLE_NONE;
	NiDepthStencilBuffer* psharedDepthBuffer = 0;
	BSRenderedTexture* prenTexture = 0;
	pformat.m_eMipMapped = NiTexture::FormatPrefs::MIP_DEFAULT;
	if (BSShaderManager::bNonPower2Textures) {
		uisafeWidth = auiWidth;
		uisafeHeight = auiHeight;
	} else {
		uint32_t i;
		uint32_t v15 = auiHeight;
		if (auiWidth > auiHeight) {
			v15 = auiWidth;
		}
		for (i = 1; i < v15; i *= 2) {
			;
		}
		uisafeHeight = i;
		uisafeWidth = i;
	}
	pformat.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
	pformat.m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
	if (aiD3DFormat) {
		NiRenderedTexture::bStaticForceD3DFormat = 1;
		NiRenderedTexture::iStaticForcedD3DFormat = aiD3DFormat;
	}
	int16_t v17 = auFlags;
	auFlags &= 8u;
	if (auFlags) {
		NiRenderedTexture::bStaticRenderedTexture = 0;
	}
	int32_t bDiscardDepth = v17 & 128;
	if ((v17 & 128) != 0) {
		bCreateDiscardable = 0;
	}
	if ((v17 & 256) == 0) {
		psharedDepthBuffer = spShadowDepthBuffer.m_pObject;
	}
	if ((v17 & 64) != 0) {
		switch (aeMSAAType) {
		case D3DMULTISAMPLE_2_SAMPLES: {
			emultiSamplePreference = Ni2DBuffer::MULTISAMPLE_2;
			break;
		}
		case D3DMULTISAMPLE_4_SAMPLES: {
			emultiSamplePreference = Ni2DBuffer::MULTISAMPLE_4;
			break;
		}
		case D3DMULTISAMPLE_8_SAMPLES: {
			emultiSamplePreference = Ni2DBuffer::MULTISAMPLE_8;
			break;
		}
		default: {
			emultiSamplePreference = Ni2DBuffer::MULTISAMPLE_NONE;
			break;
		}
		}
	}
	auiHeight = static_cast<uint32_t>((static_cast<uint32_t>(auiHeight) & 0xFFFFFF00u) |
		(static_cast<uint32_t>(static_cast<uint8_t>(((v17 & 4) == 0)))));
	if ((v17 & 16) != 0) {
		NiRenderedCubeMap* pcm = NiRenderedCubeMap::Create(uisafeWidth, BSShaderManager::pRenderer.m_pObject, pformat);
		if (pcm) {
			BSRenderedTexture* v19 =
				static_cast<BSRenderedTexture*>(NiMemObject::operator new(sizeof(BSRenderedTexture)));
			if (v19) {
				prenTexture = ::new (v19) BSRenderedTexture(pcm, 1, psharedDepthBuffer);
			} else {
				prenTexture = 0;
			}
		}
	} else {
		kName.m_kHandle = NiGlobalStringTable::AddString("");
		// The exe passes auiHeight's whole dword, its low byte replaced by the depth-buffer flag above.
		prenTexture = BSRenderedTexture::Create(kName, uisafeWidth, uisafeHeight, pformat, emultiSamplePreference,
			static_cast<int32_t>(auiHeight), psharedDepthBuffer, apAliasTex, uiAliasOffset);
		// The exe's inlined destructor of kName.
		// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
		NiGlobalStringTable::ReleaseHandle(kName.m_kHandle);
	}
	if (aiD3DFormat) {
		NiRenderedTexture::bStaticForceD3DFormat = 0;
	}
	if (auFlags) {
		NiRenderedTexture::bStaticRenderedTexture = 1;
	}
	if (bDiscardDepth) {
		bCreateDiscardable = 1;
	}
	return prenTexture;
}

void BSTextureManager::Update(NiCamera* apCamera, bool abMenuMode) {
	NiTListItem<RenderedTextureData*>* kPos = unusedRTPool.m_pkHead;
	while (kPos) {
		RenderedTextureData* pData = kPos->m_element;
		if (!abMenuMode) {
			if (!pData->bUsedThisFrame && !(pData->uFlags & BT_FLAG_TILED)) {
				// RemovePos advances kPos; the exe inlines ~RenderedTextureData before the delete.
				unusedRTPool.RemovePos(kPos);
				pData->spRenderedTexture.~NiPointer();
				MemoryManager_Singleton.Deallocate(pData);
				continue;
			}
			pData->bUsedThisFrame = false;
		}
		kPos = kPos->m_pkNext;
	}
}

// Takes a rendered texture back. An aliased one (iAliasedType > -1) goes back to the head of its type's unused
// aliased list and leaves its used list; any other moves its pool entry from the used pool to the unused one.
// The exe's cleanup-only EH frame is left out.
void BSTextureManager::ReturnRenderedTexture(BSRenderedTexture* apTexToReturn) {
	if (!apTexToReturn) {
		return;
	}
	// The exe compares signed (jle) against -1.
	if (apTexToReturn->iAliasedType <= -1) {
		NiTListItem<RenderedTextureData*>* kPos = usedRTPool.m_pkHead;
		RenderedTextureData* pData = nullptr;
		while (kPos && !pData) {
			if (kPos->m_element->spRenderedTexture.m_pObject == apTexToReturn) {
				pData = kPos->m_element;
			} else {
				kPos = kPos->m_pkNext;
			}
		}
		if (!pData) {
			return;
		}
		unusedRTPool.AddTail(pData);
		usedRTPool.RemovePos(kPos);
		return;
	}
	uint32_t uiList;
	switch (apTexToReturn->iAliasedType) {
	case TEX_SHADOWS_SHADOWMAPFRUSTUM:
		uiList = 0;
		break;
	case TEX_TESWATER_DEPTH:
		uiList = 1;
		break;
	case TEX_TESWATER_REFLECTION:
		uiList = 2;
		break;
	case TEX_TESWATER_BLUR:
		uiList = 3;
		break;
	case TEX_TESWATER_NOISE_NORMALMAP:
		uiList = 4;
		break;
	default:
		return;
	}
	{
		NiPointer<BSRenderedTexture> spTex(apTexToReturn);
		spUnusedAliasedTextureList[uiList].AddHead(spTex);
	}
	{
		NiPointer<BSRenderedTexture> spTex(apTexToReturn);
		// SWEEP-INLINE 0x45CEC0
		spUsedAliasedTextureList[uiList].Remove(spTex);
	}
}

// Takes a frustum shadow map back: finds its node in the used pool, appends it to the unused pool and removes the
// node. The exe's cleanup-only C++ EH frame is left out.
void BSTextureManager::ReturnFrustumShadowTexture(BSRenderedTexture* apTexToReturn) {
	if (!apTexToReturn) {
		return;
	}
	NiPointer<BSRenderedTexture> spTex;
	NiTListItem<NiPointer<BSRenderedTexture>>* kPos = usedFrustumShadowRTPool.m_pkHead;
	while (kPos) {
		spTex = kPos->m_element;
		if (spTex.m_pObject == apTexToReturn) {
			break;
		}
		kPos = kPos->m_pkNext;
	}
	if (kPos) {
		unusedFrustumShadowRTPool.AddTail(spTex);
		usedFrustumShadowRTPool.RemovePos(kPos);
	}
}

// Hands out the unused pool's last frustum shadow map, moving it to the used pool; null when the pool is empty.
// The exe's cleanup-only C++ EH frame is left out.
BSRenderedTexture* BSTextureManager::BorrowFrustumShadowTexture() {
	NiPointer<BSRenderedTexture> spTex;
	if (unusedFrustumShadowRTPool.m_uiCount) {
		spTex = unusedFrustumShadowRTPool.RemoveTail();
		usedFrustumShadowRTPool.AddTail(spTex);
	}
	return spTex.m_pObject;
}

NiDepthStencilBuffer* BSTextureManager::GetShadowDepthBuffer() {
	return spShadowDepthBuffer.m_pObject;
}

void BSTextureManager::ClearShadowDepthBuffer() {
	spSharedDepthBuffer = nullptr;
}

// Borrows each precreated render target once and gives it back, so that the pool creates them now: the HDR chain
// (when HDR is on), the image space swap and two menu backgrounds; then has the swap target's texture compute its
// alias size, and takes the water depth, reflection, blur and noise normal map textures into the aliased lists 1-4
// (from the unused list's head, which also goes to the used list, or newly borrowed), setting each one's aliased
// type and computing its alias size.
// The exe's cleanup-only C++ EH frame is left out. The exe's frame also tests flag bits 4, 8, 0x10 and 0x20 of its
// temporaries mask to release a conditional temporary after each aliased texture; nothing ever sets them (the mask
// starts at 0 and only bit 1 is set, by the swap target's null branch, whose temporary is null), so they are left
// out.
void BSTextureManager::PrecreateTextures(NiDX9Renderer* apRenderer) {
	uint32_t uiWidth;
	uint32_t uiHeight;
	D3DFORMAT eD3DFormat;
	D3DFORMAT eEDRAMFormat;
	uint32_t uiFlags;
	D3DMULTISAMPLE_TYPE eMSAAType;
	// The exe stores up to 13 entries (10 HDR targets, then 3 more) from [esp+0x30].
	BSRenderedTexture* pTextures[13];
	int32_t iCount = 0;
	if (BSShaderManager::bHDR) {
		pTextures[0] =
			BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject, 1u, 1u, BT_FLAG_NODEPTH | BT_FLAG_PRECREATE,
				D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN, TEX_UNKNOWN, D3DMULTISAMPLE_NONE, nullptr, 0);
		pTextures[1] =
			BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject, 1u, 1u, BT_FLAG_NODEPTH | BT_FLAG_PRECREATE,
				D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN, TEX_UNKNOWN, D3DMULTISAMPLE_NONE, nullptr, 0);
		pTextures[2] =
			BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject, 1u, 1u, BT_FLAG_NODEPTH | BT_FLAG_PRECREATE,
				D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN, TEX_UNKNOWN, D3DMULTISAMPLE_NONE, nullptr, 0);
		pTextures[3] = BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject, 4u, 4u,
			BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE, D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN, TEX_UNKNOWN,
			D3DMULTISAMPLE_NONE, nullptr, 0);
		pTextures[4] = BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject, 16u, 16u,
			BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE, D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN, TEX_UNKNOWN,
			D3DMULTISAMPLE_NONE, nullptr, 0);
		pTextures[5] = BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject, 64u, 64u,
			BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE, D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN, TEX_UNKNOWN,
			D3DMULTISAMPLE_NONE, nullptr, 0);
		NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
		LookupTextureParameters(pRenderer, TEX_HDR_BLURSWAP, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
			eMSAAType);
		pTextures[6] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_HDR_BLURSWAP, eMSAAType, nullptr, 0);
		pRenderer = BSShaderManager::pRenderer.m_pObject;
		LookupTextureParameters(pRenderer, TEX_HDR_DOWNSAMPLE, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
			eMSAAType);
		pTextures[7] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_HDR_DOWNSAMPLE, eMSAAType, nullptr, 0);
		pRenderer = BSShaderManager::pRenderer.m_pObject;
		LookupTextureParameters(pRenderer, TEX_MAIN_IMAGESPACE, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
			eMSAAType);
		pTextures[8] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_MAIN_IMAGESPACE, eMSAAType, nullptr, 0);
		if (BSShaderManager::bFPFiltering && BSShaderManager::bAnisoMinFiltering) {
			pRenderer = BSShaderManager::pRenderer.m_pObject;
			LookupTextureParameters(pRenderer, TEX_HDR_DOWNSAMPLE_ANISOINTERMEDIATE, uiWidth, uiHeight, eD3DFormat,
				eEDRAMFormat, uiFlags, eMSAAType);
			pTextures[9] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
				TEX_HDR_DOWNSAMPLE_ANISOINTERMEDIATE, eMSAAType, nullptr, 0);
		} else {
			pTextures[9] = nullptr;
		}
		iCount = 10;
	}
	NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
	LookupTextureParameters(pRenderer, TEX_IMAGESPACE_SWAP, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
		eMSAAType);
	pTextures[iCount] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
		TEX_IMAGESPACE_SWAP, eMSAAType, nullptr, 0);
	++iCount;
	pRenderer = BSShaderManager::pRenderer.m_pObject;
	LookupTextureParameters(pRenderer, TEX_MAIN_MENUBG, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
		eMSAAType);
	pTextures[iCount] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
		TEX_MAIN_MENUBG, eMSAAType, nullptr, 0);
	++iCount;
	pRenderer = BSShaderManager::pRenderer.m_pObject;
	LookupTextureParameters(pRenderer, TEX_MAIN_MENUBG, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
		eMSAAType);
	pTextures[iCount] = BorrowRenderedTexture(pRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
		TEX_MAIN_MENUBG, eMSAAType, nullptr, 0);
	++iCount;
	for (int32_t i = 0; i < iCount; ++i) {
		if (pTextures[i]) {
			ReturnRenderedTexture(pTextures[i]);
		}
	}

	// The exe binds the swap target's first texture, or a null temporary when there is no swap target, and calls
	// through it without a null test.
	BSRenderedTexture* pSwapTarget = BSShaderManager::pImageSpaceManager->pSwapTarget;
	NiRenderedTexture* pSwapTexture = pSwapTarget ? pSwapTarget->spTexture[0].m_pObject : nullptr;
	pSwapTexture->m_pkRendererData->ComputeAliasSize();

	BSRenderedTexture* pDepth;
	if (spUnusedAliasedTextureList[1].m_uiCount) {
		pDepth = spUnusedAliasedTextureList[1].m_pkHead->m_element.m_pObject;
		spUsedAliasedTextureList[1].AddHead(NiPointer<BSRenderedTexture>(pDepth));
		spUnusedAliasedTextureList[1].RemoveHead();
	} else {
		LookupTextureParameters(apRenderer, TEX_TESWATER_DEPTH, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
			eMSAAType);
		pDepth = BorrowRenderedTexture(apRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_TESWATER_DEPTH, eMSAAType, nullptr, 0);
	}
	if (pDepth) {
		spUnusedAliasedTextureList[1].AddHead(NiPointer<BSRenderedTexture>(pDepth));
		pDepth->iAliasedType = TEX_TESWATER_DEPTH;
		pDepth->spTexture[0].m_pObject->m_pkRendererData->ComputeAliasSize();
	}

	BSRenderedTexture* pReflection;
	if (spUnusedAliasedTextureList[2].m_uiCount) {
		pReflection = spUnusedAliasedTextureList[2].m_pkHead->m_element.m_pObject;
		spUsedAliasedTextureList[2].AddHead(NiPointer<BSRenderedTexture>(pReflection));
		spUnusedAliasedTextureList[2].RemoveHead();
	} else {
		LookupTextureParameters(apRenderer, TEX_TESWATER_REFLECTION, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat,
			uiFlags, eMSAAType);
		pReflection = BorrowRenderedTexture(apRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_TESWATER_REFLECTION, eMSAAType, nullptr, 0);
	}
	if (pReflection) {
		spUnusedAliasedTextureList[2].AddHead(NiPointer<BSRenderedTexture>(pReflection));
		pReflection->iAliasedType = TEX_TESWATER_REFLECTION;
		pReflection->spTexture[0].m_pObject->m_pkRendererData->ComputeAliasSize();
	}

	BSRenderedTexture* pBlur;
	if (spUnusedAliasedTextureList[3].m_uiCount) {
		pBlur = spUnusedAliasedTextureList[3].m_pkHead->m_element.m_pObject;
		spUsedAliasedTextureList[3].AddHead(NiPointer<BSRenderedTexture>(pBlur));
		spUnusedAliasedTextureList[3].RemoveHead();
	} else {
		LookupTextureParameters(apRenderer, TEX_TESWATER_BLUR, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags,
			eMSAAType);
		pBlur = BorrowRenderedTexture(apRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_TESWATER_BLUR, eMSAAType, nullptr, 0);
	}
	if (pBlur) {
		spUnusedAliasedTextureList[3].AddHead(NiPointer<BSRenderedTexture>(pBlur));
		pBlur->iAliasedType = TEX_TESWATER_BLUR;
		pBlur->spTexture[0].m_pObject->m_pkRendererData->ComputeAliasSize();
	}

	BSRenderedTexture* pNoise;
	if (spUnusedAliasedTextureList[4].m_uiCount) {
		pNoise = spUnusedAliasedTextureList[4].m_pkHead->m_element.m_pObject;
		spUsedAliasedTextureList[4].AddHead(NiPointer<BSRenderedTexture>(pNoise));
		spUnusedAliasedTextureList[4].RemoveHead();
	} else {
		LookupTextureParameters(apRenderer, TEX_TESWATER_NOISE_NORMALMAP, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat,
			uiFlags, eMSAAType);
		pNoise = BorrowRenderedTexture(apRenderer, uiWidth, uiHeight, uiFlags, eD3DFormat, eEDRAMFormat,
			TEX_TESWATER_NOISE_NORMALMAP, eMSAAType, nullptr, 0);
	}
	if (pNoise) {
		spUnusedAliasedTextureList[4].AddHead(NiPointer<BSRenderedTexture>(pNoise));
		pNoise->iAliasedType = TEX_TESWATER_NOISE_NORMALMAP;
		pNoise->spTexture[0].m_pObject->m_pkRendererData->ComputeAliasSize();
	}
}

void BSTextureManager::LookupTextureParameters(NiDX9Renderer* apRenderer, eTextureType aeType, uint32_t& auiWidth,
	uint32_t& auiHeight, D3DFORMAT& aiD3DFormat, D3DFORMAT& aiEDRAMFormat, uint32_t& auFlags,
	D3DMULTISAMPLE_TYPE& aeMSAAType) {
	aiEDRAMFormat = D3DFMT_UNKNOWN;
	aeMSAAType = NiDX9Renderer::GetMultiSampleType(BSShaderManager::eFramebufferMode);
	switch (aeType) {
	case TEX_HDR_DOWNSAMPLE:
	case TEX_HDR_BLURSWAP:
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE;
		if (apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0) <= 1280) {
			auiHeight = 256;
			auiWidth = 256;
			aiD3DFormat = D3DFMT_A16B16G16R16F;
		} else {
			if (BSShaderManager::bLetterbox) {
				auiWidth = BSShaderManager::iLetterboxWidth / 4;
			} else {
				auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0) >> 2;
			}
			if (BSShaderManager::bLetterbox) {
				auiHeight = BSShaderManager::iLetterboxHeight / 4;
			} else {
				auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0) >> 2;
			}
			aiD3DFormat = D3DFMT_A16B16G16R16F;
		}
		break;
	case TEX_HDR_DOWNSAMPLE_ANISOINTERMEDIATE: {
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE;
		// The exe truncates through a 64-bit integer (fistp qword) and keeps the low half.
		uint32_t uiWidth = static_cast<uint32_t>(static_cast<int64_t>(apRenderer->GetScreenWidth() * 0.5));
		if (uiWidth < 256) {
			uiWidth = 256;
		}
		auiWidth = uiWidth;
		auiHeight = 256;
		aiD3DFormat = D3DFMT_A16B16G16R16F;
		break;
	}
	case TEX_IMAGESPACE_SWAP:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
			auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE;
			aiD3DFormat = D3DFMT_UNKNOWN;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
			auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE;
			aiD3DFormat = D3DFMT_UNKNOWN;
		}
		break;
	case TEX_MAIN_IMAGESPACE:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
		}
		// Signed compare in the exe (setle).
		auFlags = static_cast<int32_t>(NiDX9Renderer::GetMultiSampleType(BSShaderManager::eFramebufferMode)) <= 0
			? BT_FLAG_CLEAR | BT_FLAG_PRECREATE
			: BT_FLAG_CLEAR | BT_FLAG_PRECREATE | BT_FLAG_MULTISAMPLE;
		aiD3DFormat = BSShaderManager::bHDR ? D3DFMT_A16B16G16R16F : D3DFMT_UNKNOWN;
		break;
	case TEX_MAIN_FIRSTPERSON:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_PRECREATE;
		aiD3DFormat = D3DFMT_A16B16G16R16F;
		break;
	case TEX_MAIN_MENUBG:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0);
			auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0);
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_TESWATER_FFT:
		auiHeight = ImageSpaceEffectWaterFFT::iGridResolution;
		auiWidth = ImageSpaceEffectWaterFFT::iGridResolution;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_A32B32G32R32F;
		break;
	case TEX_TESWATER_GREY:
		auiHeight = 512;
		auiWidth = 512;
		auFlags = BT_FLAG_CLEAR;
		aiD3DFormat = D3DFMT_A16B16G16R16;
		break;
	case TEX_TESWATER_REFLECTION:
	case TEX_TESWATER_BLUR:
		auiWidth = BSShaderManager::iWaterReflectionWidth;
		auiHeight = BSShaderManager::iWaterReflectionHeight;
		auFlags = BT_FLAG_MULTISAMPLE;
		aiD3DFormat = D3DFMT_UNKNOWN;
		aeMSAAType = static_cast<D3DMULTISAMPLE_TYPE>(BSShaderManager::iWaterReflectionMultisample);
		auFlags |= BT_FLAG_PRECREATE;
		break;
	case TEX_TESWATER_LOD_REFLECTION:
		auiHeight = 512;
		auiWidth = 512;
		auFlags = BT_FLAG_MULTISAMPLE;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_TESWATER_RAIN:
		auiHeight = 128;
		auiWidth = 128;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_A16B16G16R16;
		break;
	case TEX_TESWATER_SCRAMBLEMAP:
		auiWidth = ImageSpaceEffectWaterFFT::iGridResolution;
		auiHeight = 1;
		auFlags = BT_FLAG_NODEPTH | BT_FLAG_NOTRENDERABLE;
		aiD3DFormat = D3DFMT_L16;
		break;
	case TEX_TESWATER_IFFT_BUTTERFLYMAP:
		auiWidth = ImageSpaceEffectWaterFFT::iGridResolution;
		auiHeight = 16;
		auFlags = BT_FLAG_NODEPTH | BT_FLAG_NOTRENDERABLE;
		aiD3DFormat = D3DFMT_A32B32G32R32F;
		break;
	case TEX_TESWATER_HIGH_W:
		auiHeight = ImageSpaceEffectWaterFFT::iGridResolution;
		auiWidth = ImageSpaceEffectWaterFFT::iGridResolution;
		auFlags = BT_FLAG_NODEPTH | BT_FLAG_NOTRENDERABLE;
		aiD3DFormat = D3DFMT_R32F;
		break;
	case TEX_TESWATER_HIGH_AMPLITUDE:
		auiHeight = ImageSpaceEffectWaterFFT::iGridResolution;
		auiWidth = ImageSpaceEffectWaterFFT::iGridResolution;
		auFlags = BT_FLAG_NODEPTH | BT_FLAG_NOTRENDERABLE;
		aiD3DFormat = D3DFMT_A32B32G32R32F;
		break;
	case TEX_TESWATER_DEPTH:
		auiWidth = 512;
		auiHeight = 512;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_PRECREATE | BT_FLAG_UNIQUEDEPTHBUFFER;
		aiD3DFormat = D3DFMT_A8R8G8B8;
		break;
	case TEX_TESWATER_NOISE_HEIGHTMAP:
		auiWidth = BSShaderManager::iWaterNoiseWidth;
		auiHeight = BSShaderManager::iWaterNoiseHeight;
		auFlags = BT_FLAG_NODEPTH | BT_FLAG_NOTRENDERABLE;
		aiD3DFormat = D3DFMT_A8R8G8B8;
		break;
	case TEX_TESWATER_NOISE_NORMALMAP:
		auiWidth = BSShaderManager::iWaterNoiseWidth;
		auiHeight = BSShaderManager::iWaterNoiseHeight;
		auFlags = BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_A8R8G8B8;
		auFlags |= BT_FLAG_PRECREATE;
		break;
	case TEX_TESWATER_DISPLACEMENT:
		auiHeight = 512;
		auiWidth = 512;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_A16B16G16R16F;
		break;
	case TEX_TESWATER_DEBUG_DISPLAY:
		auiHeight = ImageSpaceEffectWaterFFT::iGridResolution;
		auiWidth = ImageSpaceEffectWaterFFT::iGridResolution;
		auFlags = BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_TREES_CANOPYSHADOWMASK:
		auiHeight = 256;
		auiWidth = 256;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_BLURFULL_BUFFER:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth / 4;
			auiHeight = BSShaderManager::iLetterboxHeight / 4;
		} else {
			auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0) >> 2;
			auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0) >> 2;
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_BLURFULL_SWAP:
	case TEX_VFOG_SWAP:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
			auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE;
			aiD3DFormat = D3DFMT_UNKNOWN;
		} else {
			auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0);
			auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0);
			auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_PRECREATE;
			aiD3DFormat = D3DFMT_UNKNOWN;
		}
		break;
	case TEX_BLUR20_BUFFER:
	case TEX_GETHIT_BUFFER:
		auiHeight = ImageSpaceEffectBlur::iBlurTexSize;
		auiWidth = ImageSpaceEffectBlur::iBlurTexSize;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_LOCALMAP_FINAL: {
		uint32_t uiSize = ImageSpaceEffectMap::bSaveMode ? 512 : 128;
		auiHeight = uiSize;
		auiWidth = uiSize;
		auFlags = BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_R5G6B5;
		break;
	}
	case TEX_LOCALMAP_COLOR:
	case TEX_LOCALMAP_NORMALS: {
		uint32_t uiSize = ImageSpaceEffectMap::bSaveMode ? 512 : 128;
		auiHeight = uiSize;
		auiWidth = uiSize;
		auFlags = 0;
		aiD3DFormat = D3DFMT_R5G6B5;
		break;
	}
	case TEX_REFRACTION_NORMALS:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		// Signed compare in the exe (jle).
		if (static_cast<int32_t>(NiDX9Renderer::GetMultiSampleType(BSShaderManager::eFramebufferMode)) > 0) {
			auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH | BT_FLAG_MULTISAMPLE;
		}
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_VFOG:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0);
			auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0);
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_MULTISAMPLE;
		aiD3DFormat = BSShaderManager::b30VFog ? D3DFMT_A16B16G16R16F : D3DFMT_A8R8G8B8;
		break;
	case TEX_SCENE_DEPTH_BUFFER:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
		}
		auFlags = BT_FLAG_CLEAR;
		aiD3DFormat = BSShaderManager::b30VFog ? D3DFMT_R32F : D3DFMT_A8R8G8B8;
		break;
	case TEX_VELOCITY:
	case TEX_VELOCITY_SWAP:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0);
			auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0);
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_MULTISAMPLE;
		aiD3DFormat = D3DFMT_A8R8G8B8;
		break;
	case TEX_DOF_BLUR:
	case TEX_DOF_BLUR_SWAP:
	case TEX_DEPTH_RADIALBLUR_BLUR:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth / 4;
			auiHeight = BSShaderManager::iLetterboxHeight / 4;
		} else {
			auiWidth = apRenderer->GetScreenWidth() >> 2;
			auiHeight = apRenderer->GetScreenHeight() >> 2;
		}
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_A8R8G8B8;
		break;
	case TEX_DOF_MASK:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
		}
		auFlags = BT_FLAG_CLEAR;
		aiD3DFormat = D3DFMT_A8R8G8B8;
		break;
	case TEX_CUBEMAP_CAMERA:
		auiHeight = 256;
		auiWidth = 256;
		auFlags = BT_FLAG_CLEAR;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_SHADOWS_SHADOWMAP:
		auiHeight = 512;
		auiWidth = 512;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_R32F;
		break;
	case TEX_SHADOWS_SHADOWMAPFRUSTUM: {
		uint32_t uiSize = BSShaderManager::usShadowMapResolution;
		auiHeight = uiSize;
		auiWidth = uiSize;
		auFlags = BT_FLAG_CLEAR | BT_FLAG_NODEPTH;
		aiD3DFormat = static_cast<D3DFORMAT>(BSShaderManager::eShadowFormat);
		break;
	}
	case TEX_ENVMAP_WINDOWS:
		auiHeight = 256;
		auiWidth = 256;
		auFlags = BT_FLAG_CUBEMAP;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_INTERFACE_RENDEREDMENU:
	case TEX_INTERFACE_TEMP: {
		// 1280 wide, the screen's aspect ratio; the exe truncates through a 64-bit integer (fistp qword).
		auiWidth = 1280;
		NiRenderTargetGroup* pHeightGroup = apRenderer->GetDefaultRenderTargetGroup();
		NiRenderTargetGroup* pWidthGroup = apRenderer->GetDefaultRenderTargetGroup();
		double dHeight = pHeightGroup->GetHeight(0);
		auiHeight = static_cast<uint32_t>(static_cast<int64_t>(dHeight / pWidthGroup->GetWidth(0) * auiWidth));
		auFlags = BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	}
	case TEX_INTERFACE_RENDEREDMENU_SWAP:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth / 2;
			auiHeight = BSShaderManager::iLetterboxHeight / 2;
		} else {
			auiWidth = apRenderer->GetDefaultRenderTargetGroup()->GetWidth(0) >> 1;
			auiHeight = apRenderer->GetDefaultRenderTargetGroup()->GetHeight(0) >> 1;
		}
		auFlags = BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_INTERFACE_VATS:
		if (BSShaderManager::bLetterbox) {
			auiWidth = BSShaderManager::iLetterboxWidth;
			auiHeight = BSShaderManager::iLetterboxHeight;
		} else {
			auiWidth = apRenderer->GetScreenWidth();
			auiHeight = apRenderer->GetScreenHeight();
		}
		auFlags = BT_FLAG_MULTISAMPLE;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_INTERFACE_RACESEXMENU: {
		auiWidth = 1280;
		NiRenderTargetGroup* pHeightGroup = apRenderer->GetDefaultRenderTargetGroup();
		NiRenderTargetGroup* pWidthGroup = apRenderer->GetDefaultRenderTargetGroup();
		double dHeight = pHeightGroup->GetHeight(0);
		auiHeight = static_cast<uint32_t>(static_cast<int64_t>(dHeight / pWidthGroup->GetWidth(0) * auiWidth));
		auFlags = BT_FLAG_MULTISAMPLE;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	}
	case TEX_NOISE_BUFFER:
		auiHeight = 256;
		auiWidth = 256;
		auFlags = BT_FLAG_NODEPTH;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	case TEX_DECAL_OCCLUSION:
		auiHeight = 8;
		auiWidth = 8;
		auFlags = 0;
		aiD3DFormat = D3DFMT_UNKNOWN;
		break;
	default:
		break;
	}
	if (!(auFlags & BT_FLAG_MULTISAMPLE)) {
		aeMSAAType = D3DMULTISAMPLE_NONE;
	}
}

// KEEP: an uninitialised read. LookupTextureParameters (0xB6C2C0) has no case for TEX_TESWATER_REFRACTION or
// TEX_UNKNOWN, so for those types it leaves the width, height and D3D format unwritten, and the exe passes those
// locals on to CreateRenderedTexture (0xB6D170). The exe keeps the flags and the multisample type in the apRenderer
// and aeType argument slots.
BSRenderedTexture* BSTextureManager::CreateRenderedTexture(NiDX9Renderer* apRenderer, eTextureType aeType,
	NiRenderedTexture* apAliasTex, uint32_t uiAliasOffset) {
	uint32_t uiWidth;
	uint32_t uiHeight;
	D3DFORMAT eD3DFormat;
	D3DFORMAT eEDRAMFormat;
	uint32_t uiFlags;
	D3DMULTISAMPLE_TYPE eMSAAType;
	LookupTextureParameters(apRenderer, aeType, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags, eMSAAType);
	return CreateRenderedTexture(apRenderer, uiWidth, uiHeight, eD3DFormat, eEDRAMFormat, uiFlags, aeType, eMSAAType,
		apAliasTex, uiAliasOffset);
}

// Grows or shrinks the unused frustum shadow map pool to auiMaxShadows + 1 under the shadows memory context, only
// while no shadow map is borrowed: a surplus map is detached from the shared depth buffer and returned to the
// texture pool; a missing one is taken from the first unused aliased list (moved to the used one) or borrowed, and
// attached to the shadow depth buffer. The exe's cleanup-only C++ EH frame is left out.
void BSTextureManager::SetFrustumShadowTextureCount(NiDX9Renderer* apRenderer, uint32_t auiMaxShadows) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_SHADOWS;
	if (usedFrustumShadowRTPool.m_uiCount == 0) {
		uint32_t uiCount = unusedFrustumShadowRTPool.m_uiCount;
		uint32_t uiWanted = auiMaxShadows + 1;
		if (uiCount > uiWanted) {
			for (uint32_t i = uiCount - uiWanted; i; --i) {
				NiPointer<BSRenderedTexture> spTexture = unusedFrustumShadowRTPool.RemoveTail();
				spTexture.m_pObject->GetGroup()->AttachDepthStencilBuffer(nullptr);
				ReturnRenderedTexture(spTexture.m_pObject);
			}
		} else if (uiCount < uiWanted) {
			for (uint32_t i = uiWanted - uiCount; i; --i) {
				BSRenderedTexture* pTexture;
				if (spUnusedAliasedTextureList[0].m_uiCount) {
					pTexture = spUnusedAliasedTextureList[0].m_pkHead->m_element.m_pObject;
					spUsedAliasedTextureList[0].AddHead(NiPointer<BSRenderedTexture>(pTexture));
					spUnusedAliasedTextureList[0].RemoveHead();
				} else {
					uint32_t uiWidth;
					uint32_t uiHeight;
					D3DFORMAT eFormat;
					D3DFORMAT eEDRAMFormat;
					uint32_t uiFlags;
					D3DMULTISAMPLE_TYPE eMSAAType;
					LookupTextureParameters(apRenderer, TEX_SHADOWS_SHADOWMAPFRUSTUM, uiWidth, uiHeight, eFormat,
						eEDRAMFormat, uiFlags, eMSAAType);
					pTexture = BorrowRenderedTexture(apRenderer, uiWidth, uiHeight, uiFlags, eFormat, eEDRAMFormat,
						TEX_SHADOWS_SHADOWMAPFRUSTUM, eMSAAType, nullptr, 0);
				}
				NiPointer<BSRenderedTexture> spTexture(pTexture);
				pTexture->GetGroup()->AttachDepthStencilBuffer(spShadowDepthBuffer.m_pObject);
				unusedFrustumShadowRTPool.AddTail(spTexture);
			}
		}
	}
	pTLSData->eMemContext = eMemContext;
}
