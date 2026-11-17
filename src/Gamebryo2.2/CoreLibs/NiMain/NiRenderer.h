#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

class Ni2DBuffer;
class NiCamera;
class NiColorA;
class NiD3DShaderDeclaration;
class NiDynamicEffect;
class NiDynamicEffectState;
class NiFrustum;
class NiGeometry;
class NiGeometryData;
class NiMaterialProperty;
class NiPalette;
class NiPixelFormat;
class NiPropertyState;
class NiRenderedCubeMap;
class NiRenderedTexture;
class NiRenderTargetGroup;
class NiScreenTexture;
class NiShader;
class NiSkinInstance;
class NiSkinPartition;
class NiTexture;

template <class T> class NiRect;

// The renderer base class: the sorter, the current property and effect state and the frame state. On PC its only
// new virtual is DoMinimalFlip; the platform renderer (NiDX9Renderer) declares everything else.
class NiRenderer : public NiObject {
public:
	enum ClearFlags : uint32_t {
		CLEAR_NONE = 0x0,
		CLEAR_BACKBUFFER = 0x1,
		CLEAR_STENCIL = 0x2,
		CLEAR_ZBUFFER = 0x4,
		CLEAR_ALL = 0x7,
	};

	// The caps GetFlags reports. Only some of the bits are declared.
	enum CapsFlag : uint32_t {
		// Initialize tests 0x2 to turn on hardware skinning in the geometry group manager.
		CAPS_HARDWARESKINNING = 0x2,
		CAPS_NONPOW2_CONDITIONAL_TEXT = 0x4,
		CAPS_NONPOW2_TEXT = 0x8,
	};

	enum EFrameState : uint32_t {
		FRAMESTATE_OUTSIDE_FRAME = 0,
		FRAMESTATE_INSIDE_FRAME = 1,
		FRAMESTATE_INSIDE_OFFSCREEN_FRAME = 2,
		FRAMESTATE_WAITING_FOR_DISPLAY = 3,
		FRAMESTATE_INTERNAL_FRAME = 4,
	};

	using ErrorString = char[256];

	NiRenderer();
	~NiRenderer() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual void DoMinimalFlip() {}

	// The renderer's current frame state.
	EFrameState GetFrameState();
	// The window point (uiX, uiY) as a fraction of pkTarget's size (null: the default target group), y flipped.
	// Returns false for an empty target.
	bool MapWindowPointToBufferPoint(uint32_t uiX, uint32_t uiY, float& fX, float& fY, NiRenderTargetGroup* pkTarget);
	void SetCameraData(const NiCamera* pkCamera);
	// Takes the camera's position, direction, up and right vectors, frustum and port: the body the NiCamera* overload
	// above passes them to.
	void SetCameraData(const NiPoint3& kWorldLoc, const NiPoint3& kWorldDir, const NiPoint3& kWorldUp,
		const NiPoint3& kWorldRight, const NiFrustum& kFrustum, const NiRect<float>& kPort);
	// Sets the screen-space camera for pkPort when the renderer is in a frame.
	void SetScreenSpaceCameraData(const NiRect<float>* pkPort);
	// Replaces the renderer's error shader when given a non-null shader.
	void SetErrorShader(NiShader* pkErrorShader);
	// The renderer lock's count.
	uint32_t LockCount();
	// Whether the frame state is eExpectedValue.
	bool Check_FrameState(const char* pcCallingFunction, EFrameState eExpectedValue);
	// Whether the frame state is inside either kind of frame.
	bool GetInsideFrameState();
	// Whether the renderer is inside a frame and the target-group active state matches the expected value.
	bool Check_RenderTargetGroupActive(const char* pcCallingFunction, bool bExpectedValue);

	// Virtuals of the platform renderer, which NiRenderer's PC vtable lacks: each calls NiDX9Renderer's vtable slot
	// (56, 64, 65, then 75 to 78).
	bool PrecacheTexture(NiTexture* pkIm, bool bForceLoad, bool bLocked);
	void PurgeSkinPartitionRendererData(NiSkinPartition* pkSkinPartition);
	void PurgeSkinInstance(NiSkinInstance* pkSkinInstance);
	uint8_t* AllocateVertexBufferMemory(uint32_t uiBufferSize);
	void ProtectVertexBufferMemory(uint8_t* pvBuffer, uint32_t uiBufferSize);
	void UnprotectVertexBufferMemory(uint8_t* pvBuffer, uint32_t uiBufferSize);
	void FreeVertexBufferMemory(uint8_t* pvBuffer);
	// Drops the renderer data kept for pkMaterial.
	void PurgeMaterial(NiMaterialProperty* pkMaterial);
	// Each drops the renderer data kept for its object; the objects' destructors call them on ms_pkRenderer.
	void PurgeGeometryData(NiGeometryData* pkData);
	void PurgeEffect(NiDynamicEffect* pkEffect);
	void PurgeScreenTexture(NiScreenTexture* pkScreenTexture);
	// Implemented by NiDX9Renderer::PrecacheGeometry. This form takes the shader declaration that Gamebryo's lacks.
	bool PrecacheGeometry(NiGeometry* pkGeometry, uint32_t uiBonesPerPartition, uint32_t uiBonesPerVertex,
		NiD3DShaderDeclaration* pkShaderDeclaration);
	// Pure virtual in Gamebryo; NiDX9Renderer's does nothing. NiPalette's constructors call it on ms_pkRenderer.
	void CreatePaletteRendererData(NiPalette* pkPalette);
	NiRenderTargetGroup* GetDefaultRenderTargetGroup();
	NiRenderTargetGroup* GetCurrentRenderTargetGroup();
	// The default render target group's back buffer.
	Ni2DBuffer* GetDefaultBackBuffer();
	// The supported depth/stencil format closest to the bit depths.
	const NiPixelFormat* FindClosestDepthStencilFormat(const NiPixelFormat* pkFrontBufferFormat, uint32_t uiDepthBPP,
		uint32_t uiStencilBPP);
	// Each returns whether the frame ended or was displayed.
	bool Do_EndFrame();
	bool Do_DisplayFrame();
	// The default render target's width and height in pixels.
	uint32_t GetScreenWidth();
	uint32_t GetScreenHeight();
	// . Clears pkR (null: the whole target) with uiMode's ClearFlags when a
	// render target group is active.
	void ClearBuffer(const NiRect<float>* pkR, uint32_t uiMode);
	// The NiColorA overload. Sets the background clear colour and alpha.
	void SetBackgroundColor(const NiColorA& kColor);
	// Copies the background clear colour and alpha into kColor.
	void GetBackgroundColor(NiColorA& kColor);
	// Sets the view from the camera's location, axes, frustum and viewport.
	void Do_SetCameraData(const NiPoint3& kWorldLoc, const NiPoint3& kWorldDir, const NiPoint3& kWorldUp,
		const NiPoint3& kWorldRight, const NiFrustum& kFrustum, const NiRect<float>& kPort);
	void Do_ClearBuffer(const NiRect<float>* pkR, uint32_t uiMode);
	void Do_SetScreenSpaceCameraData(const NiRect<float>* pkPort);
	// Returns the sorter, m_spAccum's object.
	NiAccumulator* GetSorter();
	// Sets m_spAccum to pkAccum.
	void SetSorter(NiAccumulator* pkAccum);
	// Returns the caps flags (CapsFlag).
	uint32_t GetFlags() const;
	// Creates pkTexture's renderer data with the multisample preference (a Ni2DBuffer::MultiSamplePreference).
	bool CreateRenderedTextureRendererData(NiRenderedTexture* pkTexture, uint32_t eMSAAPref);
	uint32_t GetMaxBuffersPerRenderTargetGroup() const;
	bool CreateRenderedCubeMapRendererData(NiRenderedCubeMap* pkCubeMap);

	static NiRenderer* GetRenderer();
	// Drops the renderer in use (the static data manager's shutdown).
	static void _SDMShutdown();
	// The vertices, normals and direction of one particle strip point: the overload whose aPreviousDir is the previous
	// point's direction and two normals.
	static bool PackParticleStripUtil_GetVerts(const NiPoint3** aPreviousDir, const NiPoint3* aPoint,
		const NiPoint3* aNextPoint, float afSize, NiPoint3* aOutPoint, NiPoint3* aOutNormal, NiPoint3& aOutDir);
	// Reports a renderer error (printf-style). Empty in this build.
	static void Error(const char* pcFormat, ...);
	// Copies pcStr (empty when null) into ms_acErrorString. Its body is in NiDX9Renderer.cpp.
	static void SetLastErrorString(const char* pcStr);

	NiPointer<NiAccumulator> m_spAccum;
	NiPropertyState* m_pkCurrProp;
	NiDynamicEffectState* m_pkCurrEffects;
	NiPointer<NiShader> m_spErrorShader;
	NiCriticalSection m_kRendererLock;
	NiCriticalSection m_kPrecacheCriticalSection;
	NiCriticalSection m_kSourceDataCriticalSection;
	EFrameState m_eFrameState;
	uint32_t m_uiFrameID;
	bool m_bRenderTargetGroupActive;
	bool m_bBatchRendering;

	static NiRTTI ms_RTTI;
	// The renderer in use, null before one is created.
	static inline NiRenderer* ms_pkRenderer;
	// Set while world positions are drawn relative to the camera: PosAdjust then holds the camera location that
	// Do_SetCameraData moved out of the view matrix.
	static inline bool bPosAdjust;
	static inline NiPoint3 PosAdjust{};
	// The message of the last renderer creation failure.
	static inline ErrorString ms_acErrorString{};
	// The texture anisotropy limit (DrawWorld_UpdateWater lowers it to 1 while the water renders).
	static inline int32_t iMaxAnisotropy = 1;
};
