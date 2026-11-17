#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BSShaderAccumulator;
class NiAVObject;
class NiCamera;
class NiGeometry;
class NiTexture;
class NiTriShape;
class ShadowSceneLight;

// Base of the properties that drive the game's shaders: shader flags, alpha and the lists of render passes.
class BSShaderProperty : public NiShadeProperty {
public:
	// Bit indices into the two shader flag words (word = index / 32, bit = index % 32).
	enum EShaderPropertyFlag : uint32_t {
		BSSP_SPECULAR = 0,
		BSSP_SKINNED = 1,
		BSSP_LOWDETAIL = 2,
		BSSP_VERTEX_ALPHA = 3,
		BSSP_MOTION_BLUR = 4,
		BSSP_SINGLEPASS = 5,
		BSSP_FALLOFF = 6,
		BSSP_ENVMAP = 7,
		BSSP_ALPHATEX = 8,
		BSSP_ZPREPASS = 9,
		BSSP_FLAG_FACEGENBLEND = 10,
		BSSP_FLAG_PARALLAX = 11,
		BSSP_FLAG_MODELSPACENORMALS = 12,
		BSSP_FLAG_NOPROJSHADOW = 13,
		BSSP_FLAG_LANDSCAPE = 14,
		BSSP_FLAG_REFRACTION = 15,
		BSSP_FLAG_REFRACTION_FIRE = 16,
		BSSP_FLAG_EYEREFLECT = 17,
		BSSP_FLAG_HAIR = 18,
		BSSP_FLAG_DYNAMICALPHA = 19,
		BSSP_FLAG_LOCALMAP_CLEAR = 20,
		BSSP_FLAG_WINDOWREFLECT = 21,
		BSSP_FLAG_BILLBOARDTREE = 22,
		BSSP_FLAG_SHADOWFRUSTUM = 23,
		BSSP_FLAG_MULTIPLE_TEXTURES = 24,
		BSSP_FLAG_REMAP_TEXTURES = 25,
		BSSP_FLAG_DECAL = 26,
		BSSP_FLAG_DYNAMIC_DECAL = 27,
		BSSP_FLAG_PARALLAXOCCLUSION = 28,
		BSSP_FLAG_EXTERNAL_EMITTANCE = 29,
		BSSP_FLAG_SHADOWMAP = 30,
		BSSP_FLAG_ZBUFFER_TEST = 31,
		BSSP_FLAG_ZBUFFER_WRITE = 32,
		BSSP_FLAG_LODLANDSCAPE = 33,
		BSSP_FLAG_LODBUILDING = 34,
		BSSP_FLAG_NOFADE = 35,
		BSSP_FLAG_REFRACTION_TINT = 36,
		BSSP_FLAG_VERTEXCOLORS = 37,
		BSSP_1ST_PERSON = 38,
		HSP_FLAG_PTFIRST = 39,
		HSP_FLAG_2NDLIGHT = 40,
		HSP_FLAG_3RDLIGHT = 41,
		TGSP_FLAG_VERTEX_LIGHTING = 42,
		TGSP_FLAG_UNIFORM_SCALE = 43,
		TGSP_FLAG_FIT_SLOPE = 44,
		TGSP_FLAG_BILLBOARD = 45,
		BSSP_FLAG_SKIP_LANDLO_PASS = 46,
		BSSP_FLAG_ENVMAP_LIGHTFADE = 47,
		BSSP_FLAG_WIREFRAME = 48,
		BSSP_FLAG_VATS_SELECTION = 49,
		BSSP_FLAG_SHOW_IN_LOCAL_MAP = 50,
		BSSP_FLAG_PREMULT_ALPHA = 51,
		BSSP_FLAG_SKIP_NORMAL_MAPS = 52,
		BSSP_FLAG_ALPHA_DECAL = 53,
		BSSP_FLAG_NO_TRANSPARENCY_MULTISAMPLING = 54,
		BSSP_FLAG_STINGER = 55,
		BSSP_FLAG_COUNT = 56,
	};

	// How the accumulator groups a pass.
	enum etAccumulationHint : uint8_t {
		BSSP_HINT_NORMAL = 0,
		BSSP_HINT_SORT = 1,
		BSSP_HINT_GROUP_IMMEDIATE = 2,
		BSSP_HINT_GROUP_NODEPTH = 3,
		BSSP_HINT_GROUP_DECAL = 4,
		BSSP_HINT_GROUP_DYNAMIC_DECAL = 5,
		BSSP_HINT_GROUP_REFRACT = 6,
		BSSP_HINT_GROUP_1STPERSONNOREFRACT = 7,
		BSSP_HINT_GROUP_VOLUMETRICFOG = 8,
		BSSP_HINT_GROUP_MORPH = 9,
		BSSP_HINT_GROUP_LOD = 10,
		BSSP_HINT_GROUP_LODTREE = 11,
		BSSP_HINT_GROUP_LODBILLBOARD = 12,
		BSSP_HINT_GROUP_VELOCITY = 13,
		BSSP_HINT_GROUP_FADING = 14,
	};

	// The kind of a texture of a shader property or texture set (the PDB's; the shader-specific kinds alias the
	// standard ones).
	enum TextureTypeEnum : int32_t {
		TEXTYPE_DIFFUSE = 0,
		TEXTYPE_BASE = 0,
		TEXTYPE_NORMAL = 1,
		TEXTYPE_GLOSS = 1,
		TEXTYPE_GLOW = 2,
		TEXTYPE_SKIN_SST = 2,
		TEXTYPE_HAIR_LAYER = 2,
		TEXTYPE_HEIGHT = 3,
		TEXTYPE_ENV = 4,
		TEXTYPE_ENV_MASK = 5,
		TEXTYPE_SPECULAR = 6,
		TEXTYPE_COUNT = 7,
	};

	// One render pass of a geometry: the shader pass and the lights it is drawn with.
	class RenderPass {
	public:
		// The local the exe constructs in place (a plain local + FNVD_RECONSTRUCT): the reconstruct runs the
		// destructor first, which frees ppSceneLights, so it starts empty.
		RenderPass() : ppSceneLights(nullptr) {}
		// The exe's constructor (BSShaderProperty::RenderPass::RenderPass), cdecl and C-variadic: aucNumLights
		// scene lights follow, copied into a new ppSceneLights.
		RenderPass(NiGeometry* apGeometry, int32_t aePassEnum, int32_t abFirstPass, int32_t aucNumLights, ...);
		~RenderPass();

		// Grows ppSceneLights to ausNumLights slots and copies that many scene lights, apSceneLight and the variadic
		// arguments after it (0xBA8C50). A C-variadic member: cdecl with this on the stack, as in the exe. PDB
		// parameter names.
		void SetLights(int32_t ausNumLights, ShadowSceneLight* apSceneLight, ...);
		void ClearSceneLights();
		// Inferred type: a2: uint8_t: unsigned uses from 0xba8c0f
		void AllocateLightArray(uint8_t a2);

		NiGeometry* pGeometry;
		uint16_t usPassEnum;
		etAccumulationHint eAccumulationHint;
		uint8_t bFirstPass;
		bool bLastPass;
		uint8_t ucNumLights;
		uint8_t ucMaxNumLights;
		uint8_t ucExtraParam;
		ShadowSceneLight** ppSceneLights;
	};

	// The growable list of render passes of one kind. It has no vtable of its own: the one at offset 0 is pPassA's
	// (NiTObjectArray<RenderPass*>, 0x10B8480), and the destructor (0xBA9520) is not virtual.
	class RenderPassArray {
	public:
		RenderPassArray(uint32_t auiNumPasses);
		~RenderPassArray();

		// Appends a pass for the geometry (ret 0x20).
		void Add(NiGeometry* apGeometry, int32_t ausPassEnum, int32_t abFirstPass, int32_t ausNumLights,
			ShadowSceneLight* apSceneLight, ShadowSceneLight* apSceneLight1, ShadowSceneLight* apSceneLight2,
			ShadowSceneLight* apSceneLight3);
		void Clear();

		static void operator delete(void* apMemory) { MemoryManager_Singleton.Deallocate(apMemory); }

		NiTObjectArray<RenderPass*> pPassA;
		// The number of passes in use; pPassA keeps its slots for reuse.
		uint32_t iSize;
	};

	// The world, view and projection matrices and bone arrays a geometry was last drawn with, with their ready flags
	// and update counters.
	class PreviousTransform {
	public:
		static void SetFreeze(bool abFreeze);
		static void ToggleConsole();
		static void SaveCamera(NiCamera* apCamera);

		D3DXMATRIX matWorld;
		float* pfBones;
		float* pfCurrentBones;
		D3DXMATRIX matWorldView;
		D3DXMATRIX matWorldViewProj;
		D3DXMATRIX matWorldViewT;
		D3DXMATRIX matWorldViewProjT;
		bool bBonesReady;
		bool bWorldReady;
		uint16_t usWorldViewCounter;
		uint16_t usWorldViewProjCounter;
		uint16_t usBoneCounter;

		static inline bool bFrozen;
		static inline bool bDirty = true;
		static inline bool bConsole;
		static inline bool bViewProjReady;
		// Guessed placement: the PDB's static usFrameCounter, which no code of this build references; the word at
		// the next 2-byte boundary after bViewProjReady, before ms_RTTI, is taken to be it.
		static inline uint16_t usFrameCounter;
		static inline D3DXMATRIX matView{};
		static inline D3DXMATRIX matProj{};
		static inline NiPoint3 vecPrevCameraDir{};
	};

	BSShaderProperty();
	~BSShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject*) override { return false; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void PostLinkObject(NiStream& kStream) override;
	// calls CopyToMembers
	virtual void CopyTo(BSShaderProperty* pDest);
	virtual void CopyToMembers(BSShaderProperty* pDest);
	virtual bool SetupGeometry(NiGeometry*) { return false; }

	// nested return types are written qualified: vtable-check's stack-bytes probe reuses them outside the class.
	// The last parameter, abUpdateList: the exe passes the caller's whole dword on; bool would be zero-extended
	// (BSShaderPPLightingProperty's and HairShaderProperty's overrides push its own slot as is).
	virtual BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry*, int32_t, uint16_t&, uint32_t,
		BSShaderAccumulator*, int32_t) {
		return nullptr;
	}

	virtual uint16_t GetNumberofPasses(NiGeometry*) { return 1; }
	// folded with NiTriStripsData::GetStripLists at 0xA75F10 (both return the field at 0x4C)
	virtual BSShaderProperty::RenderPassArray* GetSIBlockRenderPasses() { return pSIBlockRenderPassList; }
	virtual BSShaderProperty::RenderPass* GetRenderDepthPass(NiGeometry*) { return nullptr; }
	// picks the shader for the geometry; may return a replacement property
	virtual BSShaderProperty* ClarifyShader(NiGeometry*, bool, bool) { return nullptr; }
	virtual NiTexture* GetBaseTexture() { return nullptr; }
	virtual BSShaderProperty::RenderPassArray* GetWaterFogPassList(NiGeometry*) { return nullptr; }
	virtual void GetTextureUse(void (*)(NiTexture*, uint32_t, void*), void*) {}
	virtual void PrecacheTextures() {}

	// Sets fDepthBias.
	void SetDepthBias(float afDepthBias);
	// not virtual on PC: its body is folded with the processes' vtable slot 87
	void SetShader(int32_t aiShader);

	// The PDB takes the flag as uint32_t, here and in GetFlag. PDB: bool abSetFlag; the exe tests only its low byte
	// (0x44115E movzx eax, byte [ebp+0xC]) and SetFlagRecurse passes its caller's whole dword on (0xBA91C3).
	void SetFlag(uint32_t aiFlag, int32_t abSetFlag);
	void SetAlpha(float afAlpha);
	bool GetFlag(uint32_t aiFlag) const;

	// Queues the depth-map pass for the geometry on the accumulator's shadow light.
	void AddDepthPass(NiGeometry* apGeometry, bool abSkinned, bool abAlphaTest, BSShaderAccumulator* apAccumulator);

	// Creates the pass list if it does not exist yet, with room for aiNumPasses passes (-1: 5).
	void CheckCreateRenderPassArray(int32_t aiNumPasses);
	// The same for the water fog pass list, without the -1 default.
	void CheckCreateWaterFogRenderPassArray(int32_t aiNumPasses);
	// The same for the silhouette pass list.
	void CheckSilhouetteRenderPassArray(int32_t aiNumPasses);
	void ClearRenderPasses();
	// Destroys and frees every pass list, and forgets the last render pass state.
	void FreeRenderPasses();
	void SetFadeAlpha(float afFadeAlpha);
	void SetMotionBlurred(int32_t abIsMotionBlurred);
	// IDA name; not in the PDB. The update flag (arg10), here and in the pass helpers below (Ambient's a5, the AD*,
	// ADT*, Diffuse*, Specular*, Textures and SelfIlluminationBlock blocks): the exe passes the caller's whole dword
	// on; bool would be zero-extended (BSShaderPPLightingProperty::GetRenderPasses_1x and _2x push their own slot as
	// is; each helper compares its low byte, with 1 except SelfIlluminationBlock, which tests it for 0).
	// Inferred type: a7: ShadowSceneLight* from 0xbdb80d (result of
	// BSShaderLightingProperty::GetFirstActiveNonShadowLight)
	int32_t AmbientDirAndPoint(NiGeometry* a2, ShadowSceneLight* a6, ShadowSceneLight* a7, uint16_t* a5, int32_t arg10,
		bool* arg14, char a8, char a9);
	void CopyMembers(BSShaderProperty* pDest, NiCloningProcess& arCloneData);
	void InitRenderPassArray_depthMap(uint32_t a1);
	// Inferred type: apGeometry: NiGeometry* from 0xb67dd9 (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses)
	// The exe's callers pass 0/1 bytes (setne; push 0) and the body tests the low bytes.
	void AddLocalMapPass(NiGeometry* apGeometry, bool abSkinned, bool abHidden);
	int32_t Ambient(NiTriShape* pShape, ShadowSceneLight* pLight, uint16_t* a4, int32_t a5, uint8_t* arg10, uint8_t a7,
		uint8_t a8, uint8_t bGlowMap, uint8_t bTreeBranch);
	void InitRenderPassArray_localMap(uint32_t a2);
	void InitRenderPassArray_constAlpha(uint32_t a2);
	// Inferred type: a1: NiGeometry* from 0xb67d9e (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses)
	void AddConstAlphaPass(NiGeometry* a1, bool a2, bool a3);
	// Inferred type: a1: NiGeometry* from 0xbe0659 (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x)
	void AddPass_2X_EnvMap(NiGeometry* a1, ShadowSceneLight* a2, ShadowSceneLight* a3, uint16_t* a4, int32_t a5,
		bool* a6, bool a7, bool a8, bool a9);
	// Inferred type: a2: NiGeometry* from 0xbe078a (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// a3: GetRenderPasses_2x pushes its asCount pointer there (0xbe0786).
	void SelfIlluminationBlock(NiGeometry* a2, uint16_t* a3, int32_t a4, bool bSkinned);
	// Inferred type: a2: NiGeometry* from 0xbe070b (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x)
	int32_t AddDepthPassOrCount(NiGeometry* a2, uint16_t* a3, int32_t a4, bool* bEnable, bool bSkinned);
	// Inferred type: pShape: NiGeometry* from 0xbe02c0 (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x)
	int32_t AD2(NiGeometry* pShape, ShadowSceneLight* pLight, ShadowSceneLight* pLight1, uint16_t* a5, int32_t arg10,
		bool* bEnable, bool bSkinned, char bGlowMap, bool bFaceGen, bool a11, bool bShadows, bool bParallax,
		bool bTreeBranch);
	// Inferred type: pShape: NiGeometry* from 0xbe031f (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// pLight2: ShadowSceneLight* from 0xbe031f (result of BSShaderLightingProperty::GetNextActiveNonShadowLight)
	int32_t AD3(NiGeometry* pShape, ShadowSceneLight* pLight, ShadowSceneLight* pLight1, ShadowSceneLight* pLight2,
		uint16_t* arg10, int32_t arg14, bool* bEnable, bool bIsSkinned, bool bGlowMap, bool bIsFaceGen, int32_t unused,
		bool bHasShadow, bool bHasParallax, bool bIsBranch);
	// Inferred type: a1: NiGeometry* from 0xbdffa9 (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x); a3:
	// ShadowSceneLight* from 0xbdffa9 (result of BSShaderLightingProperty::GetFirstActiveNonShadowLight)
	// arg14 here and AD3's bEnable: GetRenderPasses_2x passes the address of the bool flag its bool& pass helpers share.
	int32_t ADT2(NiGeometry* a1, ShadowSceneLight* a2, ShadowSceneLight* a3, uint16_t* a4, int32_t arg10, bool* arg14,
		char a7, char a8, char a9, char a10, char a11, char a12, char a13, char a14);
	// Inferred type: pShape: NiGeometry* from 0xbdfea9 (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x)
	int32_t ADTS(NiGeometry* pShape, ShadowSceneLight* pLight, uint16_t* apusPassCount, int32_t abAddPass,
		bool* bEnable, bool bSkinned, bool bGlowMap, bool bFaceGen, int32_t empty, bool bShadows, bool bParallax,
		char bTreeBranch, char bHair);
	// Inferred type: pShape: NiGeometry* from 0xbdfefc (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// pLight1: ShadowSceneLight* from 0xbdfefc (result of BSShaderLightingProperty::GetFirstActiveNonShadowLight)
	int32_t ADTS2(NiGeometry* pShape, ShadowSceneLight* pLight, ShadowSceneLight* pLight1, uint16_t* a5, int32_t arg10,
		bool* bEnable, bool bSkinned, bool bGlowMap, bool bFaceGen, int32_t empty, bool bShadows, bool bParallax,
		bool bTreeBranch, bool bHair);
	// Inferred type: a2: NiGeometry* from 0xbe03fc (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// apLight1: ShadowSceneLight* from 0xbe03fc (result of BSShaderLightingProperty::GetNextActiveNonShadowLight)
	int32_t DiffusePoint2(NiGeometry* a2, ShadowSceneLight* apLight0, ShadowSceneLight* apLight1,
		uint16_t* apusPassCount, int32_t abAddPass, bool* abEnablePass, bool abIsSkinned, bool abIsFaceGen,
		int32_t empty, bool abHasParallax, bool a12);
	// Inferred type: pShape: NiGeometry* from 0xbe0461 (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// pLight1: ShadowSceneLight* from 0xbe0461 (result of BSShaderLightingProperty::GetNextActiveNonShadowLight);
	// pLight2: ShadowSceneLight* from 0xbe0461 (result of BSShaderLightingProperty::GetNextActiveNonShadowLight)
	int32_t DiffusePoint3(NiGeometry* pShape, ShadowSceneLight* pLight, ShadowSceneLight* pLight1,
		ShadowSceneLight* pLight2, uint16_t* arg10, int32_t arg14, bool* bEnable, bool bSkinned, bool bFaceGen,
		int32_t empty, bool bParallax, bool bTreeBranch);
	// Inferred type: pShape: NiGeometry* from 0xbe04be (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// apusPassCount: uint16_t* from 0xbe04be (parameter 2 of BSShaderPPLightingProperty::GetRenderPasses_2x); bEnable:
	// bool* from 0xbdddf0 (passed to BSShaderPPLightingProperty::AddTexturePass_1x 4)
	void Textures(NiGeometry* pShape, ShadowSceneLight* pLight, uint16_t* apusPassCount, int32_t abAddPass,
		bool* bEnable, bool bSkinned, int32_t empty, bool bFaceGen, bool bVertexColors, bool bTreeBranch,
		int32_t empty1, bool bParallax, bool bHair);
	// Inferred type: pShape: NiGeometry* from 0xbe05bf (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// pLight1: ShadowSceneLight* from 0xbe05bf (result of BSShaderLightingProperty::GetNextActiveNonShadowLight)
	int32_t SpecularPoint2(NiGeometry* pShape, ShadowSceneLight* pLight, ShadowSceneLight* pLight1, uint16_t* a5,
		int32_t arg10, bool* bEnable, bool bSkinned, bool bParallax, bool bTreeBranch, bool bHair);
	// Inferred type: pShape: NiGeometry* from 0xbe057b (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x);
	// pLight1: ShadowSceneLight* from 0xbe057b (result of BSShaderLightingProperty::GetNextActiveNonShadowLight);
	// pLight2: ShadowSceneLight* from 0xbe057b (result of BSShaderLightingProperty::GetNextActiveNonShadowLight)
	int32_t SpecularPoint3(NiGeometry* pShape, ShadowSceneLight* pLight, ShadowSceneLight* pLight1,
		ShadowSceneLight* pLight2, uint16_t* arg10, int32_t arg14, bool* bEnable, bool bSkinned, bool bParallax,
		bool bTreeBranch, bool bHair);
	// Inferred type: apGeometry: NiGeometry* from 0xbdff56 (parameter 0 of
	// BSShaderPPLightingProperty::GetRenderPasses_2x), its members used at 0xbde3d3; abEnable: the address of the bool
	// flag GetRenderPasses_2x's bool& pass helpers share.
	int32_t ADT(NiGeometry* apGeometry, ShadowSceneLight* pLight, uint16_t* apusPassCount, int32_t abAddPass,
		bool* abEnable, bool abIsSkinned, bool abGlowMap, bool bIsFaceGen, bool unused, bool bHasShadow,
		bool bHasParallax, bool bHasModelSpaceNormals, bool bIsBranch, bool bIsTree, bool bIsHair, int32_t x0);
	// Inferred type: a2: NiGeometry* from 0xbe050a (parameter 0 of BSShaderPPLightingProperty::GetRenderPasses_2x)
	int32_t Specular(NiGeometry* a2, ShadowSceneLight* pLight, uint16_t* a4, int32_t a5, bool* arg10, bool bIsSkinned,
		int32_t unused, bool bHasShadow, bool bHasParallax, bool bIsBranch, bool bIsHair);

	// Flips motion blur on the shader properties of apObject's subtree.
	static void ToggleMotionBlurRecurse(NiAVObject* apObject);
	// Sets or clears the motion blur flag on the shader properties of apObject's subtree (cdecl, 0xBA9120). The PDB
	// has bool; the exe passes the caller's whole dword on down the recursion, so it stays int32_t.
	static void SetMotionBlurRecurse(NiAVObject* apObject, int32_t abMotionBlur);
	// Sets or clears flag aeFlag on the shader properties of apObject's subtree (cdecl, 0xBA91A0). The exe passes the
	// caller's whole dword on and SetFlag reads its low byte; bool would be zero-extended (the PDB has bool).
	static void SetFlagRecurse(NiAVObject* apObject, EShaderPropertyFlag aeFlag, int32_t abEnable);
	// The factory the stream registration holds: a property in a 0x60-byte engine block.
	static NiObject* CreateObject();

	uint32_t ulFlags[2];
	float fAlpha;
	float fFadeAlpha;
	float fEnvmapScale;
	float fCameraDistance;
	uint32_t iLastRenderPassState;
	RenderPassArray* pRenderPassList;
	RenderPassArray* pDepthMapRenderPassList;
	RenderPassArray* pConstAlphaRenderPassList;
	RenderPassArray* pLocalMapRenderPassList;
	RenderPassArray* pSIBlockRenderPassList;
	RenderPassArray* pWaterFogRenderPassList;
	RenderPassArray* pSilhouettePassList;
	uint32_t iShader;
	float fDepthBias;

	static NiRTTI ms_RTTI;
	// The depth bias decals render with.
	static inline float fDepthBiasUnit;
};

// The masks of the bits of BSShaderProperty::ulFlags[0]: mask n is 1 << EShaderPropertyFlag n, the exe tests and
// sets them as immediates (0xB67BE0 GetRenderPasses, 0xB9AF60 BSBatchRenderer::RenderNextAlpha).
// Not in the PDB (it names the bit indices only); IDA's enum and names, 0x40 named after the PDB's BSSP_FALLOFF.
enum BSShaderFlags : uint32_t {
	BSS1_Specular = 0x1,
	BSS1_Skinned = 0x2,
	BSS1_LowDetail = 0x4,
	BSS1_Vertex_Alpha = 0x8,
	BSS1_Motion_Blur = 0x10,
	BSS1_Single_Pass = 0x20,
	BSS1_Falloff = 0x40,
	BSS1_Environment_Mapping = 0x80,
	BSS1_Alpha_Texture = 0x100,
	BSS1_Z_Prepass = 0x200,
	BSS1_FaceGen = 0x400,
	BSS1_Parallax_Shader = 0x800,
	BSS1_Model_Space_Normals = 0x1000,
	BSS1_Non_Projective_Shadows = 0x2000,
	BSS1_Landscape = 0x4000,
	BSS1_Refraction = 0x8000,
	BSS1_Fire_Refraction = 0x10000,
	BSS1_Eye_Environment_Mapping = 0x20000,
	BSS1_Hair = 0x40000,
	BSS1_Dynamic_Alpha = 0x80000,
	BSS1_Localmap_Hide_Secret = 0x100000,
	BSS1_Window_Environment_Mapping = 0x200000,
	BSS1_Tree_Billboard = 0x400000,
	BSS1_Shadow_Frustum = 0x800000,
	BSS1_Multiple_Textures = 0x1000000,
	BSS1_Remappable_Textures = 0x2000000,
	BSS1_Decal_Single_Pass = 0x4000000,
	BSS1_Dynamic_Decal_Single_Pass = 0x8000000,
	BSS1_Parallax_Occulsion = 0x10000000,
	BSS1_External_Emittance = 0x20000000,
	BSS1_Shadow_Map = 0x40000000,
	BSS1_ZBuffer_Test = 0x80000000,
};

// The masks of the bits of BSShaderProperty::ulFlags[1]: mask n is 1 << (EShaderPropertyFlag n + 32); the PDB's
// flags end at bit 55 (BSSP_FLAG_COUNT 56), so IDA's Unknown bits 24..31 are left out.
// Not in the PDB (it names the bit indices only); IDA's enum and names.
enum BSShaderFlags2 : uint32_t {
	BSS2_ZBuffer_Write = 0x1,
	BSS2_LOD_Landscape = 0x2,
	BSS2_LOD_Building = 0x4,
	BSS2_No_Fade = 0x8,
	BSS2_Refraction_Tint = 0x10,
	BSS2_Vertex_Colors = 0x20,
	BSS2_1st_person = 0x40,
	BSS2_1st_Light_is_Point_Light = 0x80,
	BSS2_2nd_Light = 0x100,
	BSS2_3rd_Light = 0x200,
	BSS2_Vertex_Lighting = 0x400,
	BSS2_Uniform_Scale = 0x800,
	BSS2_Fit_Slope = 0x1000,
	BSS2_Billboard_and_Envmap_Light_Fade = 0x2000,
	BSS2_No_LOD_Land_Blend = 0x4000,
	BSS2_Envmap_Light_Fade = 0x8000,
	BSS2_Wireframe = 0x10000,
	BSS2_VATS_Selection = 0x20000,
	BSS2_Show_in_Local_Map = 0x40000,
	BSS2_Premult_Alpha = 0x80000,
	BSS2_Skip_Normal_Maps = 0x100000,
	BSS2_Alpha_Decal = 0x200000,
	BSS2_No_Transparency_Multisampling = 0x400000,
	BSS2_stinger_prop = 0x800000,
};
NiSmartPointer(BSShaderProperty);

inline constexpr fnvd::Vtable BSShaderProperty_vftable{};
