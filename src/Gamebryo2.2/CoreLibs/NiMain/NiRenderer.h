#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Reconstruction.hpp"

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
template <class T> class NiRect;
class NiRenderTargetGroup;
class NiRenderedCubeMap;
class NiRenderedTexture;
class NiScreenTexture;
class NiShader;
class NiSkinInstance;
class NiSkinPartition;
class NiTexture;

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

	// The caps GetFlags reports; Gamebryo's names. Only the bits this build reads so far (the exe tests 0x4 and 0x8).
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
	// The camera's position, direction, up and right vectors, frustum and port: the body the NiCamera* overload above
	// passes them to (thiscall, ret 0x18, 0x4E9C90). Gamebryo defines it inline; the exe calls it out of line. Not
	// reconstructed yet.
	void SetCameraData(const NiPoint3& kWorldLoc, const NiPoint3& kWorldDir, const NiPoint3& kWorldUp,
		const NiPoint3& kWorldRight, const NiFrustum& kFrustum, const NiRect<float>& kPort);
	// (PDB name from NiXenonRenderer). Sets the screen-space camera for pkPort when the renderer
	// is in a frame (vtable slot 100; thiscall, ret 4, 0x4A0DD0).
	void SetScreenSpaceCameraData(const NiRect<float>* pkPort);
	// Replaces the renderer's error shader when given a non-null shader.
	void SetErrorShader(NiShader* pkErrorShader);
	// The renderer lock's count.
	uint32_t LockCount();
	// Whether the frame state is eExpectedValue. thiscall, ret 8, 0x86BA60.
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
	// Vtable slot 61 of the platform renderer (NiDX9Renderer::PurgeMaterial, 0xE69960).
	// Drops the renderer data kept for pkMaterial.
	void PurgeMaterial(NiMaterialProperty* pkMaterial);
	// Vtable slots 60, 62 and 63 of the platform renderer (NiDX9Renderer::PurgeGeometryData,
	// PurgeEffect, PurgeScreenTexture). Each drops the renderer data kept for its object; the objects' destructors
	// call them on ms_pkRenderer.
	void PurgeGeometryData(NiGeometryData* pkData);
	void PurgeEffect(NiDynamicEffect* pkEffect);
	void PurgeScreenTexture(NiScreenTexture* pkScreenTexture);
	// Vtable slot 59 of the platform renderer (NiDX9Renderer::PrecacheGeometry, 0xE73F60;
	// thiscall, ret 0x10: the exe's form takes the shader declaration Gamebryo's lacks).
	bool PrecacheGeometry(NiGeometry* pkGeometry, uint32_t uiBonesPerPartition, uint32_t uiBonesPerVertex,
		NiD3DShaderDeclaration* pkShaderDeclaration);
	// Vtable slot 84 of the platform renderer (Gamebryo's pure virtual
	// CreatePaletteRendererData; NiDX9Renderer's does nothing). NiPalette's constructors call it on ms_pkRenderer.
	void CreatePaletteRendererData(NiPalette* pkPalette);
	// Vtable slots 50 and 51 of the platform renderer (the slot 50 target group's GetWidth(0)
	// gives the screen width, so the names are Gamebryo's for that pair).
	NiRenderTargetGroup* GetDefaultRenderTargetGroup();
	NiRenderTargetGroup* GetCurrentRenderTargetGroup();
	// Vtable slot 53 of the platform renderer (NiDX9Renderer::GetDefaultBackBuffer, 0xE69620).
	// The default render target group's back buffer.
	Ni2DBuffer* GetDefaultBackBuffer();
	// Vtable slot 37 of the platform renderer (NiDX9Renderer::FindClosestDepthStencilFormat;
	// PDB name from NiXenonRenderer; thiscall, ret 0xC). The supported depth/stencil format closest to the bit depths.
	const NiPixelFormat* FindClosestDepthStencilFormat(const NiPixelFormat* pkFrontBufferFormat, uint32_t uiDepthBPP,
		uint32_t uiStencilBPP);
	// Vtable slots 96 and 97 of the platform renderer (IDA's NiDX9Renderer vtable names).
	// Each returns whether the frame ended or was displayed.
	bool Do_EndFrame();
	bool Do_DisplayFrame();
	// The default render target's width and height in pixels.
	uint32_t GetScreenWidth();
	uint32_t GetScreenHeight();
	// . Clears pkR (null: the whole target) with uiMode's ClearFlags when a
	// render target group is active.
	void ClearBuffer(const NiRect<float>* pkR, uint32_t uiMode);
	// Vtable slot 43 of the platform renderer (the NiColorA overload; PDB name from
	// NiXenonRenderer). Sets the background clear colour and alpha.
	void SetBackgroundColor(const NiColorA& kColor);
	// Vtable slot 45 of the platform renderer (NiDX9Renderer::GetBackgroundColor). Copies the
	// background clear colour and alpha into kColor.
	void GetBackgroundColor(NiColorA& kColor);
	// Vtable slot 99 of the platform renderer (NiDX9Renderer::Do_SetCameraData, ret 0x18).
	// Sets the view from the camera's location, axes, frustum and viewport.
	void Do_SetCameraData(const NiPoint3& kWorldLoc, const NiPoint3& kWorldDir, const NiPoint3& kWorldUp,
		const NiPoint3& kWorldRight, const NiFrustum& kFrustum, const NiRect<float>& kPort);
	// Vtable slot 98 of the platform renderer (NiDX9Renderer::Do_ClearBuffer, ret 8).
	void Do_ClearBuffer(const NiRect<float>* pkR, uint32_t uiMode);
	// Vtable slot 100 of the platform renderer (NiDX9Renderer::Do_SetScreenSpaceCameraData,
	// ret 4).
	void Do_SetScreenSpaceCameraData(const NiRect<float>* pkPort);
	// The sorter, m_spAccum's object (0x5585E0, folded with Animation::GetSceneRoot).
	NiAccumulator* GetSorter();
	// Sets m_spAccum to pkAccum (thiscall, ret 4). The exe's body is 0x4DC540, folded with
	// FORenderedMenu::SetScreenParent.
	void SetSorter(NiAccumulator* pkAccum);
	// Vtable slot 40 of the platform renderer (NiDX9Renderer::GetFlags, 0xE757D0). The caps
	// flags (CapsFlag).
	uint32_t GetFlags() const;
	// Vtable slot 80 of the platform renderer
	// (NiDX9Renderer::CreateRenderedTextureRendererData, 0xE6DC80; thiscall, ret 8). Creates pkTexture's renderer
	// data with the multisample preference (a Ni2DBuffer::MultiSamplePreference).
	bool CreateRenderedTextureRendererData(NiRenderedTexture* pkTexture, uint32_t eMSAAPref);
	// Vtable slot 54 of the platform renderer (NiDX9Renderer::GetMaxBuffersPerRenderTargetGroup, 0xE695E0).
	uint32_t GetMaxBuffersPerRenderTargetGroup() const;
	// Vtable slot 82 of the platform renderer (NiDX9Renderer::CreateRenderedCubeMapRendererData, 0xE6DD00).
	bool CreateRenderedCubeMapRendererData(NiRenderedCubeMap* pkCubeMap);

	static NiRenderer* GetRenderer();
	// Drops the renderer in use (the static data manager's shutdown).
	static void _SDMShutdown();
	// The vertices, normals and direction of one particle strip point (cdecl, 0xA62BE0): the PDB's overload whose
	// aPreviousDir is the previous point's direction and two normals.
	static bool PackParticleStripUtil_GetVerts(const NiPoint3** aPreviousDir, const NiPoint3* aPoint,
		const NiPoint3* aNextPoint, float afSize, NiPoint3* aOutPoint, NiPoint3* aOutNormal, NiPoint3& aOutDir);
	// Reports a renderer error (cdecl, printf-style). Empty in this build.
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
inline constexpr fnvd::Vtable NiRenderer_vftable{};
