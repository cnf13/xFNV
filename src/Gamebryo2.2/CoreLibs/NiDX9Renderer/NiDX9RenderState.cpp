#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderConstantManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

NiDX9RenderState::NiDX9RenderState(NiDX9Renderer* pkRenderer, const _D3DCAPS9* apkD3DCaps9)
	: NiD3DRenderState(pkRenderer), m_bUsingSoftwareVP(false), m_kD3DCaps9(*apkD3DCaps9) {}

// NiD3DRenderState's destructor does all of it.
NiDX9RenderState::~NiDX9RenderState() {}

// No declaration and no FVF yet; the saved FVF and declaration hold the invalid sentinel 1.
void NiDX9RenderState::InitShaderState() {
	m_bDeclaration = false;
	m_uiCurrentFVF = 0;
	m_uiPreviousFVF = 1;
	m_hCurrentVertexDeclaration = nullptr;
	m_hPreviousVertexDeclaration = reinterpret_cast<IDirect3DVertexDeclaration9*>(1);
	NiD3DRenderState::InitShaderState();
}

// Save the current FVF and declaration before either state is changed.
void NiDX9RenderState::SaveShaderState() {
	m_uiPreviousFVF = m_uiCurrentFVF;
	m_hPreviousVertexDeclaration = m_hCurrentVertexDeclaration;
}

// Restore the saved FVF before restoring the saved declaration.
void NiDX9RenderState::RestoreShaderState() {
	RestoreFVF();
	RestoreDeclaration();
}

// Cache the shader, optionally save the previous one, then update the device only on change.
void NiDX9RenderState::SetVertexShader(IDirect3DVertexShader9* apShader, bool abSave) {
	if (abSave)
		m_hPreviousVertexShader = m_hCurrentVertexShader;

	if (m_hCurrentVertexShader != apShader) {
		m_hCurrentVertexShader = apShader;
		m_pkD3DDevice->SetVertexShader(apShader);
	}
}

// Return the cached vertex shader.
IDirect3DVertexShader9* NiDX9RenderState::GetVertexShader() {
	return m_hCurrentVertexShader;
}

// Restore the saved shader without overwriting the saved value.
void NiDX9RenderState::RestoreVertexShader() {
	SetVertexShader(m_hPreviousVertexShader, false);
}

// Forget apShader as the current (unbinding it from the device) and as the saved vertex shader.
void NiDX9RenderState::ClearVertexShader(IDirect3DVertexShader9* apShader) {
	if (m_hCurrentVertexShader == apShader) {
		m_hCurrentVertexShader = nullptr;
		m_pkD3DDevice->SetVertexShader(nullptr);
	}
	if (m_hPreviousVertexShader == apShader) {
		m_hPreviousVertexShader = nullptr;
	}
}

// Switch to the FVF (leaving any declaration) unless it is already active; a zero FVF is ignored.
void NiDX9RenderState::SetFVF(uint32_t auiFVF, bool abSave) {
	if (auiFVF == 0) {
		return;
	}
	if (m_bDeclaration || m_uiCurrentFVF != auiFVF) {
		m_bDeclaration = false;
		if (abSave) {
			m_uiPreviousFVF = m_uiCurrentFVF;
		}
		m_uiCurrentFVF = auiFVF;
		m_pkD3DDevice->SetFVF(auiFVF);
	}
}

// An active declaration takes precedence over the cached FVF.
uint32_t NiDX9RenderState::GetFVF() {
	if (m_bDeclaration)
		return 0;
	return m_uiCurrentFVF;
}

void NiDX9RenderState::RestoreFVF() {
	SetFVF(m_uiPreviousFVF, false);
}

// Replace auiFVF with D3DFVF_XYZ (2) as the current FVF (on the device too) and as the saved one, unless a
// declaration is active.
void NiDX9RenderState::ClearFVF(uint32_t auiFVF) {
	if (auiFVF == 0 || m_bDeclaration) {
		return;
	}
	if (m_uiCurrentFVF == auiFVF) {
		m_uiCurrentFVF = 2;
		m_pkD3DDevice->SetFVF(2);
	}
	if (m_uiPreviousFVF == auiFVF) {
		m_uiPreviousFVF = 2;
	}
}

// Switch to the declaration unless it is already active; a null declaration is ignored.
void NiDX9RenderState::SetDeclaration(IDirect3DVertexDeclaration9* apDeclaration, bool abSave) {
	if (apDeclaration == nullptr) {
		return;
	}
	if (!m_bDeclaration || m_hCurrentVertexDeclaration != apDeclaration) {
		m_bDeclaration = true;
		if (abSave) {
			m_hPreviousVertexDeclaration = m_hCurrentVertexDeclaration;
		}
		m_hCurrentVertexDeclaration = apDeclaration;
		m_pkD3DDevice->SetVertexDeclaration(apDeclaration);
	}
}

// Return the cached declaration only when it is active.
IDirect3DVertexDeclaration9* NiDX9RenderState::GetDeclaration() {
	if (m_bDeclaration)
		return m_hCurrentVertexDeclaration;
	return nullptr;
}

void NiDX9RenderState::RestoreDeclaration() {
	SetDeclaration(m_hPreviousVertexDeclaration, false);
}

// Remove the declaration from both the current and saved caches.
void NiDX9RenderState::ClearDeclaration(IDirect3DVertexDeclaration9* apDeclaration) {
	if (apDeclaration == nullptr || !m_bDeclaration)
		return;
	if (m_hCurrentVertexDeclaration == apDeclaration)
		m_hCurrentVertexDeclaration = nullptr;
	if (m_hPreviousVertexDeclaration == apDeclaration)
		m_hPreviousVertexDeclaration = nullptr;
}

// Cached sampler states only (slot below 5): the cache and the device are updated only on a change.
void NiDX9RenderState::SetSamplerState(uint32_t auiStage, uint32_t auiState, uint32_t auiValue, bool abSave) {
	uint16_t usIndex = ms_auiSamplesStatesRevMappings[auiState];
	if (usIndex < 5) {
		NiRenderStateSetting* pkSetting = &m_akSamplerStateSettings[auiStage][usIndex];
		if (pkSetting->m_uiCurrValue != auiValue) {
			pkSetting->m_uiCurrValue = auiValue;
			m_pkD3DDevice->SetSamplerState(auiStage, auiState, auiValue);
		}
	}
}

// The cached value of a sampler state that has a cache slot; zero for any other.
uint32_t NiDX9RenderState::GetSamplerState(uint32_t auiStage, NiD3DRenderState::NiD3DSamplerState aeState) {
	const uint16_t usIndex = ms_auiSamplesStatesRevMappings[aeState];
	if (usIndex >= 5)
		return 0;
	return m_akSamplerStateSettings[auiStage][usIndex].m_uiCurrValue;
}

// Set a cached sampler state back to its previous value.
void NiDX9RenderState::RestoreSamplerState(uint32_t auiStage, NiD3DRenderState::NiD3DSamplerState aeState) {
	uint16_t usIndex = ms_auiSamplesStatesRevMappings[aeState];
	if (usIndex < 5) {
		SetSamplerState(auiStage, aeState, m_akSamplerStateSettings[auiStage][usIndex].m_uiPrevValue, false);
	}
}

// Switch vertex processing only when the device allows switching, then drop the current vertex shader.
void NiDX9RenderState::SetSoftwareVertexProcessing(bool abSoftware) {
	if (abSoftware != m_bUsingSoftwareVP && m_pkD3DRenderer->m_bSWVertexSwitchable) {
		m_bUsingSoftwareVP = abSoftware;
		m_pkD3DDevice->SetSoftwareVertexProcessing(abSoftware);
		ClearVertexShader(m_hCurrentVertexShader);
	}
}

// Without independent color write masks (D3DPMISCCAPS_INDEPENDENTWRITEMASKS) the COLORWRITEENABLE1-3 entries are
// removed from the initial render state table. Then every cache is initialized and the default render states are
// set, with black fog.
void NiDX9RenderState::InitRenderModes() {
	if (!(m_kD3DCaps9.PrimitiveMiscCaps & 0x4000)) {
		uint32_t uiDst = 0;
		uint32_t uiSkipped = 0;
		RenderStateInitialValue* pkDst = ms_akInitialRenderStateSettings;
		if (pkDst->m_eType != 0xFFFFFFFF) {
			do {
				uint32_t uiSrc = uiSkipped + uiDst;
				D3DRENDERSTATETYPE eType = ms_akInitialRenderStateSettings[uiSrc].m_eType;
				if (eType == D3DRS_COLORWRITEENABLE1 || eType == D3DRS_COLORWRITEENABLE2 ||
					eType == D3DRS_COLORWRITEENABLE3) {
					++uiSkipped;
				} else {
					pkDst->m_eType = eType;
					pkDst->m_uiValue = ms_akInitialRenderStateSettings[uiSrc].m_uiValue;
					if (ms_akInitialRenderStateSettings[uiSrc].m_eType == 0xFFFFFFFF) {
						--uiSkipped;
					}
					++uiDst;
					++pkDst;
				}
			} while (pkDst->m_eType != 0xFFFFFFFF);
		}
	}

	InitRenderState();
	InitShaderState();
	InitTextureStageState();
	InitTextures();
	m_kMaterials.Initialize();

	// D3DCULL_CW, D3DFILL_SOLID, D3DSHADE_GOURAUD.
	SetRenderState(D3DRS_CULLMODE, 2, 0, false);
	SetRenderState(D3DRS_FILLMODE, 3, 0, false);
	SetRenderState(D3DRS_SHADEMODE, 2, 0, false);
	SetRenderState(D3DRS_ALPHABLENDENABLE, 0, 0, false);
	SetRenderState(D3DRS_SRCBLEND, m_auiAlphaMapping[NiAlphaProperty::ALPHA_SRCALPHA], 0, false);
	SetRenderState(D3DRS_DESTBLEND, m_auiAlphaMapping[NiAlphaProperty::ALPHA_INVSRCALPHA], 0, false);
	SetRenderState(D3DRS_ALPHATESTENABLE, 0, 0, false);
	// D3DCMP_ALWAYS.
	SetRenderState(D3DRS_ALPHAFUNC, 8, 0, false);
	SetRenderState(D3DRS_ALPHAREF, 0, 0, false);
	SetRenderState(D3DRS_ZFUNC, 8, 0, false);
	SetRenderState(D3DRS_ZWRITEENABLE, 0, 0, false);
	SetRenderState(D3DRS_DITHERENABLE, 0, 0, false);
	SetRenderState(D3DRS_SPECULARENABLE, 0, 0, false);
	SetRenderState(D3DRS_FOGENABLE, 0, 0, false);
	// D3DFOG_LINEAR table fog when the device supports it.
	SetRenderState(D3DRS_FOGTABLEMODE, (m_uiFlags & 1) ? 3 : 0, 0, false);
	SetRenderState(D3DRS_FOGVERTEXMODE, 3, 0, false);
	SetRenderState(D3DRS_RANGEFOGENABLE, 0, 0, false);
	SetRenderState(D3DRS_FOGCOLOR, 0, 0, false);
	SetRenderState(D3DRS_NORMALIZENORMALS, 1, 0, false);
	SetRenderState(D3DRS_ZENABLE, 1, 0, false);
	SetRenderState(D3DRS_COLORVERTEX, 1, 0, false);

	m_kCurrentFogColor.r = NiColor_BLACK.r;
	m_kCurrentFogColor.g = NiColor_BLACK.g;
	m_kCurrentFogColor.b = NiColor_BLACK.b;
	uint32_t uiRed = static_cast<uint8_t>(static_cast<int32_t>(m_kCurrentFogColor.r * 255.0));
	uint32_t uiGreen = static_cast<uint8_t>(static_cast<int32_t>(m_kCurrentFogColor.g * 255.0));
	uint32_t uiBlue = static_cast<uint8_t>(static_cast<int32_t>(m_kCurrentFogColor.b * 255.0));
	m_uiCurrentFogColor = 0xFF000000 | (uiRed << 16) | (uiGreen << 8) | uiBlue;
}

// Device capabilities: table fog needs D3DPRASTERCAPS_FOGTABLE and D3DPRASTERCAPS_WFOG (flag 1), the supported
// source and destination blends per alpha function, border addressing (flag 4), and whether software vertex
// processing is forced (capable but not switchable).
void NiDX9RenderState::ReadRenderStateCaps() {
	if ((m_kD3DCaps9.RasterCaps & 0x100) && (m_kD3DCaps9.RasterCaps & 0x100000)) {
		m_uiFlags |= 1;
	} else {
		m_uiFlags &= ~1u;
	}
	m_uiDestAlphaCaps = 0;
	m_uiSrcAlphaCaps = 0;
	uint32_t uiBit = 1;
	for (uint32_t i = 0; i < NiAlphaProperty::ALPHA_MAX_MODES; ++i) {
		if (m_auiAlphaMapping[i] & m_kD3DCaps9.SrcBlendCaps) {
			m_uiSrcAlphaCaps |= uiBit;
		}
		if (m_auiAlphaMapping[i] & m_kD3DCaps9.DestBlendCaps) {
			m_uiDestAlphaCaps |= uiBit;
		}
		uiBit <<= 1;
	}
	if (m_kD3DCaps9.TextureAddressCaps & 0x10) {
		m_uiFlags |= 4;
	} else {
		m_uiFlags &= ~4u;
	}
	m_bUsingSoftwareVP = m_pkD3DRenderer->m_bSWVertexCapable && !m_pkD3DRenderer->m_bSWVertexSwitchable;
}

// Keep the renderer and a reference on its device; without a renderer the device is released.
void NiDX9RenderState::SetD3DRenderer(NiDX9Renderer* apRenderer) {
	m_pkD3DRenderer = apRenderer;
	if (apRenderer) {
		IDirect3DDevice9* pkDevice = apRenderer->m_pkD3DDevice9;
		if (m_pkD3DDevice) {
			m_pkD3DDevice->Release();
		}
		m_pkD3DDevice = pkDevice;
		if (pkDevice) {
			pkDevice->AddRef();
		}
	} else {
		if (m_pkD3DDevice) {
			m_pkD3DDevice->Release();
		}
		m_pkD3DDevice = nullptr;
	}
}

// The cull mode the draw mode maps to for the current handedness.
void NiDX9RenderState::SetCullMode(NiStencilProperty::DrawMode aeMode, bool abSave) {
	SetRenderState(D3DRS_CULLMODE, m_auiCullModeMapping[aeMode][m_uiLeftHanded], 0, abSave);
}

bool NiDX9RenderState::SetPixelShaderConstantB(uint32_t auiStartRegister, const int32_t* apiConstantData,
	uint32_t auiBoolCount, bool abSave) {
	return m_pkD3DDevice->SetPixelShaderConstantB(auiStartRegister, apiConstantData, auiBoolCount) >= 0;
}

bool NiDX9RenderState::SetPixelShaderConstantF(uint32_t auiStartRegister, const float* apfConstantData,
	uint32_t auiVector4fCount, bool abSave) {
	return m_pkD3DDevice->SetPixelShaderConstantF(auiStartRegister, apfConstantData, auiVector4fCount) >= 0;
}

bool NiDX9RenderState::SetPixelShaderConstantI(uint32_t auiStartRegister, const int32_t* apiConstantData,
	uint32_t auiVector4iCount, bool abSave) {
	return m_pkD3DDevice->SetPixelShaderConstantI(auiStartRegister, apiConstantData, auiVector4iCount) >= 0;
}

NiDX9RenderState* NiDX9RenderState::Create(NiDX9Renderer* a1, _D3DCAPS9* a2, bool a3) {
	NiDX9RenderState* v4 = static_cast<NiDX9RenderState*>(NiMemObject::operator new(sizeof(NiDX9RenderState)));
	if (v4) {
		::new (v4) NiDX9RenderState(a1, a2);
	} else {
		v4 = 0;
	}
	v4->ReadRenderStateCaps();
	for (uint32_t i = 0; i < 256; i += 4) {
		D3DRENDERSTATETYPE m_eType = NiDX9RenderState::ms_akInitialRenderStateSettings[i].m_eType;
		if (m_eType == D3DRS_NI_END_OF_LIST) {
			break;
		}
		if (m_eType == D3DRS_ZENABLE) {
			NiDX9RenderState::ms_akInitialRenderStateSettings[i].m_uiValue = a3;
		}
		D3DRENDERSTATETYPE v7 = NiDX9RenderState::ms_akInitialRenderStateSettings[i + 1].m_eType;
		if (v7 == D3DRS_NI_END_OF_LIST) {
			break;
		}
		if (v7 == D3DRS_ZENABLE) {
			NiDX9RenderState::ms_akInitialRenderStateSettings[i + 1].m_uiValue = a3;
		}
		D3DRENDERSTATETYPE v8 = NiDX9RenderState::ms_akInitialRenderStateSettings[i + 2].m_eType;
		if (v8 == D3DRS_NI_END_OF_LIST) {
			break;
		}
		if (v8 == D3DRS_ZENABLE) {
			NiDX9RenderState::ms_akInitialRenderStateSettings[i + 2].m_uiValue = a3;
		}
		D3DRENDERSTATETYPE v9 = NiDX9RenderState::ms_akInitialRenderStateSettings[i + 3].m_eType;
		if (v9 == D3DRS_NI_END_OF_LIST) {
			break;
		}
		if (v9 == D3DRS_ZENABLE) {
			NiDX9RenderState::ms_akInitialRenderStateSettings[i + 3].m_uiValue = a3;
		}
	}
	if (a3) {
		v4->m_uiFlags |= 2u;
	}
	NiDX9ShaderConstantManager* v10 = NiDX9ShaderConstantManager::Create(a1, a2);
	NiDX9ShaderConstantManager* m_spShaderConstantManager = v4->m_spShaderConstantManager;
	if (m_spShaderConstantManager != v10) {
		if (m_spShaderConstantManager) {
			NiDecRefCount(m_spShaderConstantManager);
		}
		v4->m_spShaderConstantManager = v10;
		if (v10) {
			NiIncRefCount(v10);
		}
	}
	return v4;
}
