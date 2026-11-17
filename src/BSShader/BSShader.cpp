#include "BSShader/BSShader.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShaderBuffer.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

BSShader::BSShader() : bFirstPass(false) {}

// Releases the pass and unregisters the shader from the renderer.
BSShader::~BSShader() {
	spPass = nullptr;
	BSShaderManager::pRenderer->RemoveShader(this);
}

bool BSShader::Initialize() {
	uint8_t result = (uint8_t)NiD3DShader::Initialize();
	if (!result) {
		return (bool)result;
	}
	CreateShaders();
	CreateTextures();
	CreateStagesAndPasses();
	return (bool)1;
}

bool BSShader::SetupGeometry(NiGeometry* apGeometry) {
	BSShaderProperty* property = static_cast<BSShaderProperty*>(apGeometry->GetProperty(3));
	return (bool)((uint8_t)property->SetupGeometry(apGeometry));
}

// IDA: NiD3DShader::SetupGeometryAlphaBlending; BSShader's vtable slot 32 holds it.
void BSShader::SetupGeometryAlphaBlending(const NiPropertyState* apState) {
	NiAlphaProperty* m_pObject;
	BSShaderPPLightingProperty* v4;
	uint8_t shaderProps;
	m_pObject = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
	BSShaderPPLightingProperty* v3 = static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
	if (v3) {
		v4 = (v3->iShaderPropertyType != ((-1))) ? v3 : (nullptr);
	} else {
		v4 = nullptr;
	}
	int8_t v5 = 0;
	int8_t v6 = 0;
	if (m_pObject) {
		shaderProps = (uint8_t)((m_pObject->m_uFlags & 512) != 0);
		if ((m_pObject->m_uFlags & 1) != 0) {
			v5 = 1;
			v6 = 1;
		}
	}
	if ((v4->fAlpha < 1.0) || ((v4->fFadeAlpha < 1.0) || v5)) {
		BSRenderState::SetAlphaBlendEnable(1, BSRS_NOLOCK);
		if (v6) {
			BSRenderState::
				SetAlphaBlendFunc(static_cast<NiAlphaProperty::AlphaFunction>(((int32_t)(((uint32_t)((int32_t)m_pObject
																								 ->m_uFlags)) >>
																				  1)) &
									  15),
					static_cast<NiAlphaProperty::AlphaFunction>(((int32_t)(((uint32_t)((int32_t)m_pObject->m_uFlags)) >>
																	5)) &
						15),
					BSRS_NOLOCK);
		} else {
			BSRenderState::SetAlphaBlendFunc(NiAlphaProperty::ALPHA_SRCALPHA, NiAlphaProperty::ALPHA_INVSRCALPHA,
				BSRS_NOLOCK);
		}
	} else {
		BSRenderState::SetAlphaBlendEnable(0, BSRS_NOLOCK);
	}
}

// Sets the alpha test function and reference from the state's alpha property, without locking them.
void BSShader::SetupGeometryAlphaTesting(const NiPropertyState* apState) {
	NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[NiProperty::ALPHA].m_pObject);
	BSRenderState::SetAlphaTestFuncRef(static_cast<NiAlphaProperty::TestFunction>(pAlpha->m_uFlags >> 10 & 7),
		pAlpha->m_ucAlphaTestRef, BSRS_NOLOCK);
}

void BSShader::SetupGeometryRenderStates(const NiPropertyState* apState, bool abFirstPass) {
	NiStencilProperty* pStencilProp;
	NiAlphaProperty* pAlphaProp;
	uint8_t bSetDrawMode = 0;
	if (apState) {
		pStencilProp = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
		if (pStencilProp) {
			bSetDrawMode = (uint8_t)((pStencilProp->m_uFlags & 3072) == 3072);
		}
	}
	if (((uint32_t)(BSShaderManager::eCurrentPass - 86)) <= 1) {
		bSetDrawMode = 1;
	}
	uint8_t bAlphaTest = 0;
	if (abFirstPass) {
		if (apState) {
			pAlphaProp = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
			if (pAlphaProp) {
				bAlphaTest = (uint8_t)((pAlphaProp->m_uFlags & 512) != 0);
			}
		}
	}
	BSRenderState::SetCullMode(static_cast<NiStencilProperty::DrawMode>((2 * bSetDrawMode) + 1), BSRS_NOLOCK);
	BSRenderState::SetAlphaTestEnable(bAlphaTest, BSRS_NOLOCK);
}

void BSShader::RecreateRendererData() {
	NiD3DShader::RecreateRendererData();
}

void BSShader::SetupPassShaders(uint32_t auiCurrentPass) {
	NiDX9RenderState* pRenderState = NiD3DShader::m_spCurrentPass->GetD3DRenderState();
	pRenderState->SetVertexShader(NiD3DShader::m_spCurrentPass->m_spVertexShader->GetShaderHandle(), false);
	NiD3DPixelShader* pPixelShader = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
	if (pPixelShader) {
		pRenderState->SetPixelShader(pPixelShader->GetShaderHandle(), false);
	}
}

// Sets every render state the current pass saved, without locking it.
void BSShader::SetupPassRenderStates(uint32_t auiCurrentPass) {
	NiD3DRenderStateGroup::NiD3DRSEntry* i;
	for (i = NiD3DShader::m_spCurrentPass->m_pkRenderStateGroup->m_pkSavedRenderStates; i; i = i->m_pkNext) {
		BSRenderState::SetRenderState(i->m_uiRenderState, i->m_uiValue, BSRS_NOLOCK);
	}
}

void BSShader::RestorePassRenderStates(uint32_t auiCurrentPass) {
	NiD3DRenderStateGroup::NiD3DRSEntry* i;
	for (i = NiD3DShader::m_spCurrentPass->m_pkRenderStateGroup->m_pkSavedRenderStates; i; i = i->m_pkNext) {
		BSRenderState::RestoreRenderState(i->m_uiRenderState, BSRS_NOLOCK);
	}
}

void BSShader::ApplyTextureStages() {
	uint32_t m_uiStageCount;
	uint32_t i;
	NiD3DTextureStage* pStage;
	NiD3DPass* v1 = NiD3DShader::m_spCurrentPass;
	m_uiStageCount = NiD3DShader::m_spCurrentPass->m_uiStageCount;
	for (i = 0; i < m_uiStageCount; ++i) {
		pStage = v1->m_kStages.m_pBase[i].m_pObject;
		if (!pStage) {
			break;
		}
		if (pStage->bApply) {
			if (pStage->m_spTexture) {
				pStage->ConfigureStage(0);
			}
		}
	}
}

void BSShader::ApplyTextureStagesFlags() {
	NiD3DPass* v1 = NiD3DShader::m_spCurrentPass;
	uint32_t m_uiStageCount = NiD3DShader::m_spCurrentPass->m_uiStageCount;
	// The exe sign-extends the byte (`movsx`), so it reads it as a signed char.
	int32_t cEnabledStages = static_cast<int8_t>(NiD3DShader::m_spCurrentPass->cEnabledStages);
	for (uint32_t ui = 0; ui < m_uiStageCount; ++ui) {
		if ((cEnabledStages & uiEnabledFlags[ui]) != 0) {
			NiD3DTextureStage* pkStage = v1->m_kStages.m_pBase[ui].m_pObject;
			if (!pkStage) {
				return;
			}
			if (pkStage->m_spTexture) {
				pkStage->ConfigureStage(0);
			}
		}
	}
}

void BSShader::SetupGeometryNonFirstPass(const NiPropertyState* apState) {
	if ((((uint32_t)BSShaderManager::eCurrentPass) < 594) || (((uint32_t)BSShaderManager::eCurrentPass) > 598)) {
		BSRenderState::SetAlphaBlendEnable(1, BSRS_NOLOCK);
		BSRenderState::SetZWriteEnable(0, BSRS_NOLOCK);
		BSRenderState::SetZFunc(NiZBufferProperty::TEST_EQUAL, BSRS_NOLOCK);
	}
}

void BSShader::RestoreGeometryNonFirstPass() {
	BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
	BSRenderState::SetZWriteEnable(true, BSRS_NOLOCK);
	BSRenderState::SetZFunc(NiZBufferProperty::TEST_LESSEQUAL, BSRS_NOLOCK);
}

NiD3DShaderDeclaration* BSShader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
	return m_spShaderDecl.m_pObject;
}

void BSShader::ClearPassStages() {
	ClearPass(spPass.m_pObject);
}

void BSShader::ClearPass(NiD3DPass* apPass) {
	int32_t m_uiStageCount;
	int32_t i;
	NiD3DTextureStage* m_pObject;
	if (apPass) {
		m_uiStageCount = (int32_t)apPass->m_uiStageCount;
		for (i = 0; i < m_uiStageCount; ++i) {
			m_pObject = apPass->m_kStages.m_pBase[i].m_pObject;
			if (m_pObject->m_uiTextureFlags != 1) {
				m_pObject->m_spTexture = nullptr;
			}
		}
	}
}

// The base has no shaders of its own to create: success.
bool BSShader::CreateShaders() {
	return true;
}

bool BSShader::CreateStagesAndPasses() {
	NiD3DPass::CreateNewPass(spPass);
	return (bool)1;
}

// The base has no textures of its own to create: success.
bool BSShader::CreateTextures() {
	return true;
}

void BSShader::SetCurrentPass(uint32_t aPass, bool aFirstPass) {
	ePass = static_cast<BSShaderManager::eBSSMPassEnum>(aPass);
	bFirstPass = (bool)((uint8_t)aFirstPass);
}

// IDA name; not in the PDB. The first scene light's direction, negated, turned by apMatrix and normalised. The exe
// hands SetVertexShaderConstantF the turned vector, not the normalised one; both D3DX calls go through the exe's
// `jmp [slot]` thunks (stdcall: no cleanup after either call).
// Not registered: the constant is one four-float vector read from the three-float turned vector, which ends at the
// top of the exe's frame, so its fourth float is the exe's return address; src cannot reproduce the bytes past its
// own local.
void BSShader::SetLightDirection(D3DXMATRIX* apMatrix) {
	NiDirectionalLight* pLight =
		static_cast<NiDirectionalLight*>((*BSShaderManager::pCurrentRenderPass->ppSceneLights)->spLight.m_pObject);
	// The negation is a multiply by the double -1.0 (fld qword 0x101A6B0), each product stored as a float.
	float fY = pLight->m_kWorldDir.y * -1.0;
	float fZ = pLight->m_kWorldDir.z * -1.0;
	D3DXVECTOR3 kDir;
	kDir.x = -1.0 * pLight->m_kWorldDir.x;
	kDir.y = fY;
	kDir.z = fZ;
	D3DXVECTOR3 kOut;
	D3DXVec3TransformNormal_Thunk(&kOut, &kDir, apMatrix);
	D3DXVec3Normalize_Thunk(&kDir, &kOut);
	NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(25, &kOut.x, 1);
}

// Takes the shader from the loaded shader package when it has apCacheName. Otherwise, under bCompileNewer, a
// missing compiled-shader cache file (after retrying in "cache:\" when no cache directory is set) makes it compile
// the HLSL file instead of loading the cache file: "HDR" is appended to apMacro under HDR (and cleared again),
// the source is compiled for apTarget, and the result is written to the cache file; a failed compile falls back to
// an existing cache file. The data then goes to the shader package being built, or becomes a new vertex shader.
// The compiler's constant table and shader buffer are never released, as in the exe. The exe's cleanup-only EH
// frame (round the NiD3DVertexShader allocation) is left out.
NiD3DVertexShader* BSShader::CompileVertexShaderHLSL(const char* apFilename, D3DXMACRO* apMacro, const char* apTarget,
	const char* apCacheName) {
	char acCacheFile[260];
	char acError[260];
	char acCreateError[260];
	char acDefines[1300];
	char acCommandLine[1300];
	char acMessage[1300];
	ID3DXBuffer* pShader = nullptr;
	ID3DXBuffer* pErrorMsgs = nullptr;
	NiD3DVertexShader* pVertexShader = nullptr;
	ID3DXConstantTable* pConstantTable = nullptr;
	uint32_t uiSize = 0;
	ShaderBufferEntry* pEntry = nullptr;
	CreateShaderPath(acCacheFile, apFilename, apCacheName, "vso");
	bool bUseCache = true;
	char cCacheFound = 0;
	if (BSShaderManager::pShaderBufferLoader) {
		pEntry = BSShaderManager::pShaderBufferLoader->Get(apCacheName);
	}
	if (bCompileNewer && !pEntry) {
		_FILETIME kSourceTime = {0, 0};
		_FILETIME kCacheTime = {0, 0};
		cCacheFound = ArchiveManager::GetLastWriteTimeForFile(acCacheFile, &kCacheTime, ARCHIVE_TYPE_SHADERS);
		// src's GetLastWriteTimeForFile (not in the PDB) takes char* for the name it only reads.
		char cSourceFound =
			ArchiveManager::GetLastWriteTimeForFile(const_cast<char*>(apFilename), &kSourceTime, ARCHIVE_TYPE_SHADERS);
		if (!cCacheFound && BSShaderManager::bUseHardDriveCache) {
			if (!BSShaderManager::pShaderCachePath[0]) {
				strcpy_s(BSShaderManager::pShaderCachePath, 0x104, "cache:\\");
				CreateShaderPath(acCacheFile, apFilename, apCacheName, "vso");
			}
			cCacheFound = ArchiveManager::GetLastWriteTimeForFile(acCacheFile, &kCacheTime, ARCHIVE_TYPE_SHADERS);
		}
		if (!cSourceFound) {
			BSsprintf(acError, 0x104, "SHADER ERROR : error opening HLSL source file : %s", apFilename);
		}
		if (!cCacheFound) {
			bUseCache = false;
		}
	}
	bool bAllocated = false;
	void* pData = nullptr;
	if (pEntry) {
		pData = pEntry->pData;
	} else if (bUseCache) {
		BSFile* pFile =
			static_cast<BSFile*>(FileFinder::GetFile(acCacheFile, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
		if (!pFile) {
			BSsprintf(acCreateError, 0x104, "SHADER ERROR : error opening cache file : %s", acCacheFile);
			return nullptr;
		}
		if (pFile->m_bGood) {
			uiSize = pFile->GetSize();
			pData = MemoryManager_Singleton.Allocate(uiSize);
			bAllocated = true;
			pFile->NiBinaryStream::Read(pData, uiSize);
		}
		delete pFile;
	} else {
		D3DXMACRO* pHDRMacro = nullptr;
		if (BSShaderManager::bHDR) {
			pHDRMacro = apMacro;
			while (pHDRMacro->Name) {
				++pHDRMacro;
			}
			pHDRMacro->Name = "HDR";
			pHDRMacro->Definition = "";
		}
		BSFile* pFile =
			static_cast<BSFile*>(FileFinder::GetFile(apFilename, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
		if (!pFile || !pFile->m_bGood) {
			if (pFile) {
				delete pFile;
			}
			BSsprintf(acError, 0x104, "SHADER ERROR : error opening hlsl file : %s", apFilename);
			return nullptr;
		}
		uint32_t uiSourceSize = pFile->GetSize();
		void* pSource = MemoryManager_Singleton.Allocate(uiSourceSize);
		pFile->NiBinaryStream::Read(pSource, uiSourceSize);
		FNVD_CONSTRUCTED_LOCAL(ShaderIncludes, kIncludes, nullptr, apFilename);
		acDefines[0] = 0;
		acCommandLine[0] = 0;
		for (int32_t i = 0; i < 8; ++i) {
			if (apMacro[i].Name) {
				strcat_s(acDefines, 0x514, apMacro[i].Name);
				strcat_s(acCommandLine, 0x514, " /D");
				strcat_s(acCommandLine, 0x514, apMacro[i].Name);
				if (apMacro[i].Definition && *apMacro[i].Definition) {
					strcat_s(acDefines, 0x514, "=");
					strcat_s(acDefines, 0x514, apMacro[i].Definition);
					strcat_s(acCommandLine, 0x514, "=");
					strcat_s(acCommandLine, 0x514, apMacro[i].Definition);
				} else {
					strcat_s(acCommandLine, 0x514, "=1");
				}
				strcat_s(acDefines, 0x514, ", ");
			}
		}
		BSsprintf(acMessage, 0x514, "compiling : %s\n %s %s", apFilename, apTarget, acDefines);
		HRESULT hResult = D3DXCompileShader_Thunk(static_cast<const char*>(pSource), uiSourceSize, apMacro, &kIncludes,
			"main", apTarget, 0, &pShader, &pErrorMsgs, &pConstantTable);
		delete pFile;
		MemoryManager_Singleton.Deallocate(pSource);
		if (pErrorMsgs && !pShader) {
			acDefines[0] = 0;
			// Unlike the first list, this one stops at the first empty slot (signed `i < 8`).
			for (int32_t i = 0; i < 8; ++i) {
				if (!apMacro[i].Name) {
					break;
				}
				strcat_s(acDefines, 0x514, apMacro[i].Name);
				if (apMacro[i].Definition && *apMacro[i].Definition) {
					strcat_s(acDefines, 0x514, "=");
					strcat_s(acDefines, 0x514, apMacro[i].Definition);
				}
				strcat_s(acDefines, 0x514, ", ");
			}
			void* pErrorText = pErrorMsgs->GetBufferPointer();
			BSsprintf(acMessage, 0x514, "%s\n %s\n %s", apFilename, acDefines, pErrorText);
			pErrorMsgs->Release();
			pErrorMsgs = nullptr;
		}
		if (hResult || !pShader) {
			if (!cCacheFound) {
				return nullptr;
			}
			pFile =
				static_cast<BSFile*>(FileFinder::GetFile(acCacheFile, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
			if (!pFile) {
				return nullptr;
			}
			if (pFile->m_bGood) {
				uiSize = pFile->GetSize();
				pData = MemoryManager_Singleton.Allocate(uiSize);
				bAllocated = true;
				pFile->NiBinaryStream::Read(pData, uiSize);
			}
			delete pFile;
			if (!pData) {
				return nullptr;
			}
		} else {
			auto* pOut = rt::fopen(acCacheFile, "wb");
			if (pOut) {
				uiSize = pShader->GetBufferSize();
				void* pBuffer = pShader->GetBufferPointer();
				std::fwrite(pBuffer, uiSize, 1, pOut);
				std::fclose(pOut);
				ArchiveManager::InvalidateFile(acCacheFile);
				pData = pShader->GetBufferPointer();
			} else {
				BSsprintf(acError, 0xFF, "invalid cache file : %s", acCacheFile);
				pData = pShader->GetBufferPointer();
			}
		}
		if (pHDRMacro) {
			pHDRMacro->Name = nullptr;
			pHDRMacro->Definition = nullptr;
		}
	}
	if (pData) {
		if (BSShaderManager::pShaderBufferCreator) {
			BSShaderManager::pShaderBufferCreator->Add(apCacheName, static_cast<char*>(pData), uiSize);
		} else {
			IDirect3DVertexShader9* pHandle;
			if (BSShaderManager::pRenderer.m_pObject->m_pkD3DDevice9
					->CreateVertexShader(static_cast<const uint32_t*>(pData), &pHandle)) {
				BSsprintf(acCreateError, 0x104, "SHADER ERROR : failed to create vertex shader : %s", acCacheFile);
				return nullptr;
			}
			void* pMemory = NiMemObject::operator new(sizeof(NiD3DVertexShader));
			NiD3DVertexShader* pNew =
				pMemory ? ::new (pMemory) NiD3DVertexShader(BSShaderManager::pRenderer.m_pObject) : nullptr;
			pVertexShader = pNew;
			pVertexShader->SetShaderHandle(pHandle);
		}
	}
	if (bAllocated) {
		MemoryManager_Singleton.Deallocate(pData);
	}
	return pVertexShader;
}

// The pixel shader form of CompileVertexShaderHLSL, with four differences from it: the HLSL source's time is read
// before the cache file's, a "ps_2" target compiles for BSShaderManager::GetPSTarget's target (the message keeps
// apTarget), the error message lists every set define (the vertex form stops at the first empty slot), and the
// cache file is written without checking that fopen succeeded, then flushed. The exe's cleanup-only EH frame (round
// the NiD3DPixelShader allocation) is left out.
NiD3DPixelShader* BSShader::CompilePixelShaderHLSL(const char* apFilename, D3DXMACRO* apMacro, const char* apTarget,
	const char* apCacheName) {
	char acCacheFile[260];
	char acError[260];
	char acCreateError[260];
	char acDefines[1300];
	char acCommandLine[1300];
	char acMessage[1300];
	ID3DXBuffer* pShader = nullptr;
	ID3DXBuffer* pErrorMsgs = nullptr;
	NiD3DPixelShader* pPixelShader = nullptr;
	ID3DXConstantTable* pConstantTable = nullptr;
	uint32_t uiSize = 0;
	ShaderBufferEntry* pEntry = nullptr;
	CreateShaderPath(acCacheFile, apFilename, apCacheName, "pso");
	bool bUseCache = true;
	char cCacheFound = 0;
	if (BSShaderManager::pShaderBufferLoader) {
		pEntry = BSShaderManager::pShaderBufferLoader->Get(apCacheName);
	}
	if (bCompileNewer && !pEntry) {
		_FILETIME kSourceTime = {0, 0};
		_FILETIME kCacheTime = {0, 0};
		// src's GetLastWriteTimeForFile (not in the PDB) takes char* for the name it only reads.
		char cSourceFound =
			ArchiveManager::GetLastWriteTimeForFile(const_cast<char*>(apFilename), &kSourceTime, ARCHIVE_TYPE_SHADERS);
		cCacheFound = ArchiveManager::GetLastWriteTimeForFile(acCacheFile, &kCacheTime, ARCHIVE_TYPE_SHADERS);
		if (!cCacheFound && BSShaderManager::bUseHardDriveCache) {
			if (!BSShaderManager::pShaderCachePath[0]) {
				strcpy_s(BSShaderManager::pShaderCachePath, 0x104, "cache:\\");
				CreateShaderPath(acCacheFile, apFilename, apCacheName, "pso");
			}
			cCacheFound = ArchiveManager::GetLastWriteTimeForFile(acCacheFile, &kCacheTime, ARCHIVE_TYPE_SHADERS);
		}
		if (!cSourceFound) {
			BSsprintf(acError, 0x104, "SHADER ERROR : error opening HLSL source file : %s", apFilename);
		}
		if (!cCacheFound) {
			bUseCache = false;
		}
	}
	bool bAllocated = false;
	void* pData = nullptr;
	if (pEntry) {
		pData = pEntry->pData;
	} else if (bUseCache) {
		BSFile* pFile =
			static_cast<BSFile*>(FileFinder::GetFile(acCacheFile, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
		if (!pFile) {
			BSsprintf(acCreateError, 0x104, "SHADER ERROR : error opening cache file : %s", acCacheFile);
			return nullptr;
		}
		if (pFile->m_bGood) {
			uiSize = pFile->GetSize();
			pData = MemoryManager_Singleton.Allocate(uiSize);
			bAllocated = true;
			pFile->NiBinaryStream::Read(pData, uiSize);
		}
		delete pFile;
	} else {
		D3DXMACRO* pHDRMacro = nullptr;
		if (BSShaderManager::bHDR) {
			pHDRMacro = apMacro;
			while (pHDRMacro->Name) {
				++pHDRMacro;
			}
			pHDRMacro->Name = "HDR";
			pHDRMacro->Definition = "";
		}
		const char* pPSTarget = apTarget;
		if (!std::strncmp(apTarget, "ps_2_0", 4)) {
			pPSTarget = BSShaderManager::GetPSTarget(BSShaderManager::PixelShader);
			if (!pPSTarget) {
				pPSTarget = apTarget;
			}
		}
		BSFile* pFile =
			static_cast<BSFile*>(FileFinder::GetFile(apFilename, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
		if (!pFile || !pFile->m_bGood) {
			if (pFile) {
				delete pFile;
			}
			BSsprintf(acError, 0x104, "SHADER ERROR : error opening hlsl file : %s", apFilename);
			return nullptr;
		}
		uint32_t uiSourceSize = pFile->GetSize();
		void* pSource = MemoryManager_Singleton.Allocate(uiSourceSize);
		pFile->NiBinaryStream::Read(pSource, uiSourceSize);
		FNVD_CONSTRUCTED_LOCAL(ShaderIncludes, kIncludes, nullptr, apFilename);
		acDefines[0] = 0;
		acCommandLine[0] = 0;
		for (int32_t i = 0; i < 8; ++i) {
			if (apMacro[i].Name) {
				strcat_s(acDefines, 0x514, apMacro[i].Name);
				strcat_s(acCommandLine, 0x514, " /D");
				strcat_s(acCommandLine, 0x514, apMacro[i].Name);
				if (apMacro[i].Definition && *apMacro[i].Definition) {
					strcat_s(acDefines, 0x514, "=");
					strcat_s(acDefines, 0x514, apMacro[i].Definition);
					strcat_s(acCommandLine, 0x514, "=");
					strcat_s(acCommandLine, 0x514, apMacro[i].Definition);
				} else {
					strcat_s(acCommandLine, 0x514, "=1");
				}
				strcat_s(acDefines, 0x514, ", ");
			}
		}
		BSsprintf(acMessage, 0x514, "compiling : %s\n %s %s", apFilename, apTarget, acDefines);
		HRESULT hResult = D3DXCompileShader_Thunk(static_cast<const char*>(pSource), uiSourceSize, apMacro, &kIncludes,
			"main", pPSTarget, 0, &pShader, &pErrorMsgs, &pConstantTable);
		delete pFile;
		MemoryManager_Singleton.Deallocate(pSource);
		if (pErrorMsgs && !pShader) {
			acDefines[0] = 0;
			for (int32_t i = 0; i < 8; ++i) {
				if (apMacro[i].Name) {
					strcat_s(acDefines, 0x514, apMacro[i].Name);
					if (apMacro[i].Definition && *apMacro[i].Definition) {
						strcat_s(acDefines, 0x514, "=");
						strcat_s(acDefines, 0x514, apMacro[i].Definition);
					}
					strcat_s(acDefines, 0x514, ", ");
				}
			}
			void* pErrorText = pErrorMsgs->GetBufferPointer();
			BSsprintf(acMessage, 0x514, "%s\n %s\n %s", apFilename, acDefines, pErrorText);
			pErrorMsgs->Release();
			pErrorMsgs = nullptr;
		}
		if (hResult || !pShader) {
			if (!cCacheFound) {
				return nullptr;
			}
			pFile =
				static_cast<BSFile*>(FileFinder::GetFile(acCacheFile, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
			if (!pFile) {
				return nullptr;
			}
			if (pFile->m_bGood) {
				uiSize = pFile->GetSize();
				pData = MemoryManager_Singleton.Allocate(uiSize);
				bAllocated = true;
				pFile->NiBinaryStream::Read(pData, uiSize);
			}
			delete pFile;
			if (!pData) {
				return nullptr;
			}
		} else {
			auto* pOut = rt::fopen(acCacheFile, "wb");
			uiSize = pShader->GetBufferSize();
			void* pBuffer = pShader->GetBufferPointer();
			std::fwrite(pBuffer, uiSize, 1, pOut);
			std::fflush(pOut);
			std::fclose(pOut);
			ArchiveManager::InvalidateFile(acCacheFile);
			pData = pShader->GetBufferPointer();
		}
		if (pHDRMacro) {
			pHDRMacro->Name = nullptr;
			pHDRMacro->Definition = nullptr;
		}
	}
	if (pData) {
		if (BSShaderManager::pShaderBufferCreator) {
			BSShaderManager::pShaderBufferCreator->Add(apCacheName, static_cast<char*>(pData), uiSize);
		} else {
			IDirect3DPixelShader9* pHandle;
			if (BSShaderManager::pRenderer.m_pObject->m_pkD3DDevice9
					->CreatePixelShader(static_cast<const uint32_t*>(pData), &pHandle)) {
				BSsprintf(acCreateError, 0x104, "SHADER ERROR : failed to create pixel shader : %s", acCacheFile);
				return nullptr;
			}
			void* pMemory = NiMemObject::operator new(sizeof(NiD3DPixelShader));
			NiD3DPixelShader* pNew =
				pMemory ? ::new (pMemory) NiD3DPixelShader(BSShaderManager::pRenderer.m_pObject) : nullptr;
			pPixelShader = pNew;
			pPixelShader->SetShaderHandle(pHandle);
		}
	}
	if (bAllocated) {
		MemoryManager_Singleton.Deallocate(pData);
	}
	return pPixelShader;
}

NiPoint3* BSShader::GetCameraWorldTranslate() {
	return &NiRenderer::PosAdjust;
}

void BSShader::SetupDefaultStage(NiD3DTextureStage* apStage, uint32_t aiStageNum, uint32_t aiAddressValue,
	uint32_t aiMipFilter, bool abAniso) {
	NiD3DTextureStageGroup* m_pkTextureStageGroup;
	m_pkTextureStageGroup = apStage->m_pkTextureStageGroup;
	apStage->m_uiStage = aiStageNum;
	m_pkTextureStageGroup->SetStageState(11, aiStageNum, 0);
	apStage->m_pkTextureStageGroup->SetStageState(1, 1u, 0);
	apStage->m_pkTextureStageGroup->SetStageState(2, 2u, 0);
	apStage->m_pkTextureStageGroup->SetStageState(4, 1u, 0);
	apStage->m_pkTextureStageGroup->SetStageState(5, 2u, 0);
	NiD3DTextureStageGroup* v6 = apStage->m_pkTextureStageGroup;
	apStage->m_bTextureTransform = (bool)0;
	v6->SetStageState(24, 0, 0);
	if (((int32_t)aiAddressValue) == 3) {
		apStage->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
	} else {
		apStage->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
	}
	apStage->m_pkTextureStageGroup->SetSamplerState(5, aiMipFilter, 0, 0);
	apStage->m_pkTextureStageGroup->SetSamplerState(6, aiMipFilter, 0, 0);
	apStage->m_pkTextureStageGroup->SetSamplerState(7, aiMipFilter, 0, 0);
	apStage->SetFilterMode(static_cast<NiTexturingProperty::FilterMode>((4 * ((uint8_t)abAniso)) + 2));
}

// Splits apFile into its parts, splits the shader path over the same drive buffer into its own directory buffer,
// appends apFile's directory to the shader path's, and joins them back into apResult; nothing when apResult is null.
void BSShader::MakePath(const char* apFile, char* apResult) {
	char acDrive[16];
	char acExt[8];
	char acFName[128];
	char acShaderDir[260];
	char acFileDir[260];
	if (!apResult) {
		return;
	}
	_splitpath_s(apFile, acDrive, 16, acFileDir, 260, acFName, 128, acExt, 8);
	_splitpath_s(BSShaderManager::pShaderPath, acDrive, 16, acShaderDir, 260, nullptr, 0, nullptr, 0);
	strcat_s(acShaderDir, 260, acFileDir);
	_makepath_s(apResult, 260, acDrive, acShaderDir, acFName, acExt);
}

void BSShader::CreateShaderPath(char* apResult, const char* apFilename, const char* apCacheName,
	const char* apExtension) {
	if (apResult) {
		if (apFilename) {
			if (apCacheName) {
				char* v4 = BSShaderManager::pShaderCachePath;
				if (!BSShaderManager::pShaderCachePath[0]) {
					v4 = BSShaderManager::pShaderPath;
				}
				strcpy_s(apResult, 260, v4);
				if (BSShaderManager::bHDR) {
					strcat_s(apResult, 260, "HDR.");
				}
				strcat_s(apResult, 260, apCacheName);
			}
		}
	}
}

NiSourceTexture* BSShader::GetDefNormalMap() {
	return static_cast<NiSourceTexture*>(BSShaderManager::spDefaultNormalMap.m_pObject);
}

ShaderIncludes::ShaderIncludes(const char* apShaderFileName) : pShaderFileName(apShaderFileName) {}

// Combines the shader's own path with the include's file name, opens it from the shaders' archives and reads the
// whole file into a MemoryManager block, then destroys the stream with flag 1. E_FAIL when the file is missing or
// not good. The exe's object is the first of its 6 stack arguments (ret 0x18).
HRESULT STDMETHODCALLTYPE ShaderIncludes::Open(uint32_t IncludeType, const char* pFileName, const void* pParentData,
	const void** ppData, uint32_t* pBytes) {
	char acName[260];
	// The PDB's CombineFileNames takes char* for text it only reads.
	FileFinder::CombineFileNames(const_cast<char*>(pShaderFileName), const_cast<char*>(pFileName), acName);
	BSFile* pFile = static_cast<BSFile*>(FileFinder::GetFile(acName, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS));
	if (!pFile) {
		return static_cast<HRESULT>(0x80004005);
	}
	if (pFile->m_bGood) {
		uint32_t uiSize = pFile->GetSize();
		void* pData = MemoryManager_Singleton.Allocate(uiSize);
		*ppData = pData;
		NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pFile->m_pfnRead;
		uint32_t uiComponentSize = 1;
		uint32_t uiBytesRead = pfnRead(pFile, pData, uiSize, &uiComponentSize, 1);
		pFile->m_uiAbsoluteCurrentPos += uiBytesRead;
		*pBytes = uiBytesRead;
		// The exe calls the stream's scalar deleting destructor through vtable slot 0 with flag 1.
		delete pFile;
		return 0;
	}
	delete pFile;
	return static_cast<HRESULT>(0x80004005);
}

// Frees the block Open allocated (ID3DXInclude::Close, stdcall).
HRESULT STDMETHODCALLTYPE ShaderIncludes::Close(const void* pData) {
	MemoryManager_Singleton.Deallocate(const_cast<void*>(pData));
	return 0;
}
