#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"

class NiD3DPass;
class NiD3DRenderState;
class NiDX9Renderer;
class NiDX9RenderState;
class NiDynamicEffectState;
class NiGeometry;
class NiGeometryBufferData;
class NiPropertyState;
class NiSkinInstance;

void NiD3DTextureStageArray_SetAt(NiTObjectArray<NiPointer<NiD3DTextureStage>>* apArray, uint32_t auiIndex,
	const NiPointer<NiD3DTextureStage>& akElement);
void NiD3DTextureStageArray_SetSize(NiTObjectArray<NiPointer<NiD3DTextureStage>>* apArray, uint32_t auiMaxSize);

NiSmartPointer(NiD3DPass);

// One rendering pass of a D3D shader: texture stages, render states, and the vertex and pixel programs with their
// constant maps. Reference counted by hand; a pass whose count drops to zero goes back to the pool.
class NiD3DPass : public NiMemObject {
public:
	NiD3DPass();
	virtual ~NiD3DPass();

	virtual uint32_t SetupShaderPrograms(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		const NiSkinPartition::Partition* pkPartition, NiGeometryBufferData* pkBuffData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound, uint32_t uiPass);
	virtual uint32_t SetupRenderingPass(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryBufferData* pkBuffData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
		const NiTransform& kWorld, const NiBound& kWorldBound, uint32_t uiPass);
	virtual uint32_t PostProcessRenderingPass(uint32_t uiPass);

	NiDX9RenderState* GetD3DRenderState();

	// The pass's render state group (made on first use) records the state; its old value is always saved, whatever
	// abSave says.
	void SetRenderState(uint32_t uiRenderState, uint32_t uiValue, bool abSave = true) {
		if (!m_pkRenderStateGroup) {
			m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		m_pkRenderStateGroup->SetRenderState(uiRenderState, uiValue, true);
	}

	void SetSrcDestBlendRenderState(uint32_t auiSrcValue, uint32_t auiDestValue, bool abSave = true) {
		SetRenderState(D3DRS_SRCBLEND, auiSrcValue, abSave);
		SetRenderState(D3DRS_DESTBLEND, auiDestValue, abSave);
	}

	void SetPixelConstantMap(NiD3DShaderConstantMap* pkSCMPixel);
	void SetPixelShader(NiD3DPixelShader* pkPixelShader);
	void SetVertexConstantMap(NiD3DShaderConstantMap* pkSCMVertex);
	void SetVertexShader(NiD3DVertexShader* pkVertexShader);

	// Puts pkStage at uiStageNum (null clears it) and updates the stage and texture counts; 1 when the device has
	// no such sampler.
	uint32_t SetStage(uint32_t uiStageNum, NiD3DTextureStage* pkStage);
	// Whether uiStages more stages and uiTextures more textures fit in the pass.
	bool CheckFreeStages(uint32_t uiStages, uint32_t uiTextures);

	void IncRefCount() { m_uiRefCount++; }

	void DecRefCount() {
		if (--m_uiRefCount == 0) {
			ReturnPassToPool();
		}
	}

	// A renderer-owned pass drops its shaders, maps, render states and stages and goes back to the pool; any other
	// pass is deleted.
	void ReturnPassToPool();

	static void SetMaxTextureBlendStages(uint32_t uiMaxTextureBlendStages);
	static void SetMaxSimultaneousTextures(uint32_t uiMaxSimultaneousTextures);
	static void SetMaxSamplers(uint32_t uiMaxSamplers);
	static void InitializePools();
	static void ShutdownPools();
	static void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);

	// Takes a pass from the pool into aspD3DPass, owned by the renderer and with a fresh render state group.
	static void CreateNewPass(NiPointer<NiD3DPass>& aspD3DPass);

	char m_szName[16];
	uint32_t m_uiCurrentStage;
	uint32_t m_uiStageCount;
	uint32_t m_uiTexturesPerPass;
	NiTObjectArray<NiPointer<NiD3DTextureStage>> m_kStages;
	NiD3DRenderStateGroup* m_pkRenderStateGroup;
	NiPointer<NiD3DShaderConstantMap> m_spPixelConstantMap;
	char* m_pcPixelShaderProgramFile;
	char* m_pcPixelShaderEntryPoint;
	char* m_pcPixelShaderTarget;
	NiPointer<NiD3DPixelShader> m_spPixelShader;
	NiPointer<NiD3DShaderConstantMap> m_spVertexConstantMap;
	uint8_t cEnabledStages;
	char* m_pcVertexShaderProgramFile;
	char* m_pcVertexShaderEntryPoint;
	char* m_pcVertexShaderTarget;
	NiPointer<NiD3DVertexShader> m_spVertexShader;
	bool m_bSoftwareVP;
	bool m_bRendererOwned;
	uint32_t m_uiRefCount;

	// Texture blend stages and samplers the device supports.
	static inline uint32_t ms_uiMaxTextureBlendStages = 2;
	static inline uint32_t ms_uiMaxSamplers = 2;
	static inline NiD3DRenderState* ms_pkD3DRenderState;
	static inline uint32_t ms_uiMaxSimultaneousTextures = 2;
	static inline NiDX9Renderer* ms_pkD3DRenderer;
	// The renderer's device, with a reference held.
	static inline IDirect3DDevice9* ms_pkD3DDevice;
	static inline NiTObjectPool<NiD3DPass>* ms_pkPassPool;
};

// The guard of tcmp, SetAt's function-local static zero element; bit 0 is set once it is constructed.
inline uint32_t NiTArray_NiD3DTextureStagePtr_SetAt_tcmp_guard;

// NiPointer<NiD3DPass> counts through the pass's own (non-atomic) reference count.
inline void NiIncRefCount(NiD3DPass* apPass) {
	apPass->IncRefCount();
}

inline void NiDecRefCount(NiD3DPass* apPass) {
	apPass->DecRefCount();
}
