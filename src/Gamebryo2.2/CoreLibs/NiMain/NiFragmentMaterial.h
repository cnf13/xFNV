#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgramCache.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgramDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialNodeLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPtrSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

// A material assembled from shader fragments, with a program cache per program type. Only some of its functions are
// declared.
class NiFragmentMaterial : public NiMaterial {
public:
	enum ReturnCode {
		RC_SUCCESS = 0x0,
		RC_COMPILE_FAILURE_PIXEL = 0x1,
		RC_COMPILE_FAILURE_VERTEX = 0x2,
		RC_INVALID_MATERIAL = 0x4,
		RC_INVALID_SHADER = 0x8,
		RC_INVALID_CONSTANTMAP = 0x10,
	};

	enum {
		MATERIAL_HASH_BUCKET_COUNT = 37,
	};

	class RenderPassDescriptor;
	// Gamebryo's fallback callback: tries another set of passes after pass uiFailedPass failed with eFailedRC.
	typedef bool (*SHADERFALLBACKFUNC)(NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
		uint32_t uiFailedPass, RenderPassDescriptor* pkRenderPasses, uint32_t uiMaxCount, uint32_t& uiCount,
		uint32_t& uiFunctionData);

	// The virtuals NiFragmentMaterial adds after NiMaterial's, up to SetProgramCache: the two PrecacheGPUPrograms
	// overloads and SetProgramCache, which NiDX9Renderer::SetDefaultProgramCache calls. Declared only: this class is
	// never constructed, so its vtable is never emitted.
	virtual bool PrecacheGPUPrograms(const NiGeometry* apGeometry, const NiSkinInstance* apSkin,
		const NiPropertyState* apState, const NiDynamicEffectState* apEffects, uint32_t auiMaterialExtraData);

protected:
	virtual bool PrecacheGPUPrograms(NiMaterialDescriptor& arMaterialDescriptor);

public:
	virtual void SetProgramCache(NiGPUProgramCache* pkCache, NiGPUProgram::ProgramType eType);

	// The version stamped on aeType's cached programs: the type's version in the high word plus each library's.
	uint32_t GetProgramVersion(NiGPUProgram::ProgramType eType) const;

	// Gamebryo declares m_kDebugIdentifier only under _DEBUG.
	NiTObjectPtrSet<NiPointer<NiMaterialNodeLibrary>> m_kLibraries;
	NiPointer<NiGPUProgramCache> m_aspProgramCaches[NiGPUProgram::PROGRAM_MAX];
	NiTObjectPtrSet<NiPointer<NiShader>> m_akShaderHashTable[MATERIAL_HASH_BUCKET_COUNT];
	NiTObjectSet<NiMaterialDescriptor> m_akFailedShaderDescriptorHashTable[MATERIAL_HASH_BUCKET_COUNT];
	NiTObjectSet<NiGPUProgramDescriptor> m_aakFailedGPUProgramDescriptorHashTable[MATERIAL_HASH_BUCKET_COUNT]
																				 [NiGPUProgram::PROGRAM_MAX];
	NiTPrimitiveArray<SHADERFALLBACKFUNC> m_kShaderFallbackFuncs;
	uint32_t m_uiMaxPassCount;
	uint16_t m_usVertexVersion;
	uint16_t m_usPixelVersion;

	static NiRTTI ms_RTTI;
};
