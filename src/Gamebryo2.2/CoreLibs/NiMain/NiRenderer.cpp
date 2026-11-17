#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

NiRTTI NiRenderer::ms_RTTI("NiRenderer", &NiObject::ms_RTTI);

// The three NiCriticalSection members initialise their sections in their own constructors (the exe inlines them
// here) and delete them in their destructors, in reverse order, after this destructor's body.
NiRenderer::NiRenderer()
	: m_spAccum(nullptr), m_spErrorShader(nullptr), m_eFrameState(FRAMESTATE_OUTSIDE_FRAME), m_uiFrameID(0),
	  m_bRenderTargetGroupActive(false), m_bBatchRendering(false) {
	ms_pkRenderer = this;
}

NiRenderer::~NiRenderer() {
	ms_pkRenderer = nullptr;
}

void NiRenderer::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
}

NiRenderer::EFrameState NiRenderer::GetFrameState() {
	return static_cast<NiRenderer::EFrameState>((int32_t)m_eFrameState);
}

bool NiRenderer::MapWindowPointToBufferPoint(uint32_t uiX, uint32_t uiY, float& fX, float& fY,
	NiRenderTargetGroup* pkTarget) {
	if (!pkTarget) {
		pkTarget = GetDefaultRenderTargetGroup();
	}
	uint32_t uiWidth = pkTarget->GetWidth(0);
	uint32_t uiHeight = pkTarget->GetHeight(0);
	if (!uiWidth || !uiHeight) {
		return false;
	}
	fX = static_cast<double>(uiX) / static_cast<double>(uiWidth);
	fY = 1.0 - static_cast<double>(uiY) / static_cast<double>(uiHeight);
	return true;
}

// The exe takes the port, frustum, right, up and direction vectors and location in that order.
void NiRenderer::SetCameraData(const NiCamera* pkCamera) {
	const NiRect<float>& kPort = pkCamera->GetViewPort();
	const NiFrustum& kFrustum = pkCamera->GetViewFrustum();
	NiPoint3 kWorldRight = pkCamera->GetWorldRightVector();
	NiPoint3 kWorldUp = pkCamera->GetWorldUpVector();
	NiPoint3 kWorldDir = pkCamera->GetWorldDirection();
	const NiPoint3& kWorldLoc = pkCamera->GetWorldTranslate();
	SetCameraData(kWorldLoc, kWorldDir, kWorldUp, kWorldRight, kFrustum, kPort);
}

void NiRenderer::SetCameraData(const NiPoint3& kWorldLoc, const NiPoint3& kWorldDir, const NiPoint3& kWorldUp,
	const NiPoint3& kWorldRight, const NiFrustum& kFrustum, const NiRect<float>& kPort) {
	if (Check_RenderTargetGroupActive("SetCameraData", 1)) {
		Do_SetCameraData(kWorldLoc, kWorldDir, kWorldUp, kWorldRight, kFrustum, kPort);
	}
}

void NiRenderer::SetScreenSpaceCameraData(const NiRect<float>* pkPort) {
	if (Check_RenderTargetGroupActive("SetScreenSpaceCameraData", true)) {
		Do_SetScreenSpaceCameraData(pkPort);
	}
}

bool NiRenderer::Check_FrameState(const char* pcCallingFunction, NiRenderer::EFrameState eExpectedValue) {
	return (bool)((uint8_t)(((int32_t)m_eFrameState) == ((int32_t)eExpectedValue)));
}

bool NiRenderer::PrecacheTexture(NiTexture* pkIm, bool bForceLoad, bool bLocked) {
	return static_cast<NiDX9Renderer*>(this)->PrecacheTexture(pkIm, bForceLoad, bLocked);
}

// The platform renderer's virtuals: NiRenderer's PC vtable ends at DoMinimalFlip, so each of these calls the slot
// NiDX9Renderer declares.
void NiRenderer::PurgeSkinPartitionRendererData(NiSkinPartition* pkSkinPartition) {
	static_cast<NiDX9Renderer*>(this)->PurgeSkinPartition(pkSkinPartition);
}

void NiRenderer::PurgeSkinInstance(NiSkinInstance* pkSkinInstance) {
	static_cast<NiDX9Renderer*>(this)->PurgeSkinInstance(pkSkinInstance);
}

uint8_t* NiRenderer::AllocateVertexBufferMemory(uint32_t uiBufferSize) {
	return static_cast<uint8_t*>(static_cast<NiDX9Renderer*>(this)->Allocate_4B(uiBufferSize));
}

void NiRenderer::ProtectVertexBufferMemory(uint8_t* pvBuffer, uint32_t uiBufferSize) {
	static_cast<NiDX9Renderer*>(this)->Unk_4C(pvBuffer, uiBufferSize);
}

void NiRenderer::UnprotectVertexBufferMemory(uint8_t* pvBuffer, uint32_t uiBufferSize) {
	static_cast<NiDX9Renderer*>(this)->Unk_4D(pvBuffer, uiBufferSize);
}

void NiRenderer::FreeVertexBufferMemory(uint8_t* pvBuffer) {
	static_cast<NiDX9Renderer*>(this)->Deallocate_4E(pvBuffer);
}

void NiRenderer::PurgeMaterial(NiMaterialProperty* pkMaterial) {
	static_cast<NiDX9Renderer*>(this)->PurgeMaterial(pkMaterial);
}

void NiRenderer::PurgeGeometryData(NiGeometryData* pkData) {
	static_cast<NiDX9Renderer*>(this)->PurgeGeometryData(pkData);
}

void NiRenderer::PurgeEffect(NiDynamicEffect* pkEffect) {
	static_cast<NiDX9Renderer*>(this)->PurgeEffect(pkEffect);
}

void NiRenderer::PurgeScreenTexture(NiScreenTexture* pkScreenTexture) {
	static_cast<NiDX9Renderer*>(this)->PurgeScreenTexture(pkScreenTexture);
}

bool NiRenderer::PrecacheGeometry(NiGeometry* pkGeometry, uint32_t uiBonesPerPartition, uint32_t uiBonesPerVertex,
	NiD3DShaderDeclaration* pkShaderDeclaration) {
	return static_cast<NiDX9Renderer*>(this)->PrecacheGeometry(pkGeometry, uiBonesPerPartition, uiBonesPerVertex,
		pkShaderDeclaration);
}

void NiRenderer::CreatePaletteRendererData(NiPalette* pkPalette) {
	static_cast<NiDX9Renderer*>(this)->CreatePaletteRendererData(pkPalette);
}

NiRenderTargetGroup* NiRenderer::GetDefaultRenderTargetGroup() {
	return static_cast<NiDX9Renderer*>(this)->GetDefaultRenderTargetGroup();
}

NiRenderTargetGroup* NiRenderer::GetCurrentRenderTargetGroup() {
	return static_cast<NiDX9Renderer*>(this)->GetCurrentRenderTargetGroup();
}

Ni2DBuffer* NiRenderer::GetDefaultBackBuffer() {
	return static_cast<NiDX9Renderer*>(this)->GetDefaultBackBuffer();
}

const NiPixelFormat* NiRenderer::FindClosestDepthStencilFormat(const NiPixelFormat* pkFrontBufferFormat,
	uint32_t uiDepthBPP, uint32_t uiStencilBPP) {
	return static_cast<NiDX9Renderer*>(this)->FindClosestDepthStencilFormat(pkFrontBufferFormat, uiDepthBPP,
		uiStencilBPP);
}

bool NiRenderer::Do_EndFrame() {
	return static_cast<NiDX9Renderer*>(this)->Do_EndFrame();
}

bool NiRenderer::Do_DisplayFrame() {
	return static_cast<NiDX9Renderer*>(this)->Do_DisplayFrame();
}

// Buffer 0 of the default render target group, or 0 when either is missing.
uint32_t NiRenderer::GetScreenWidth() {
	NiRenderTargetGroup* pkGroup = GetDefaultRenderTargetGroup();
	if (!pkGroup) {
		return 0;
	}
	Ni2DBuffer* pkBuffer = pkGroup->GetBuffer(0);
	if (!pkBuffer) {
		return 0;
	}
	return pkBuffer->GetWidth();
}

uint32_t NiRenderer::GetScreenHeight() {
	NiRenderTargetGroup* pkGroup = GetDefaultRenderTargetGroup();
	if (!pkGroup) {
		return 0;
	}
	Ni2DBuffer* pkBuffer = pkGroup->GetBuffer(0);
	if (!pkBuffer) {
		return 0;
	}
	return pkBuffer->GetHeight();
}

void NiRenderer::ClearBuffer(const NiRect<float>* pkR, uint32_t uiMode) {
	if (Check_RenderTargetGroupActive("ClearBuffer", 1)) {
		Do_ClearBuffer(pkR, uiMode);
	}
}

// Slot 43, the NiColorA overload (NiDX9Renderer's SetBackgroundColorAlpha).
void NiRenderer::SetBackgroundColor(const NiColorA& kColor) {
	static_cast<NiDX9Renderer*>(this)->SetBackgroundColorAlpha(kColor);
}

void NiRenderer::GetBackgroundColor(NiColorA& kColor) {
	static_cast<NiDX9Renderer*>(this)->GetBackgroundColor(kColor);
}

void NiRenderer::Do_SetCameraData(const NiPoint3& kWorldLoc, const NiPoint3& kWorldDir, const NiPoint3& kWorldUp,
	const NiPoint3& kWorldRight, const NiFrustum& kFrustum, const NiRect<float>& kPort) {
	static_cast<NiDX9Renderer*>(this)->Do_SetCameraData(kWorldLoc, kWorldDir, kWorldUp, kWorldRight, kFrustum, kPort);
}

void NiRenderer::Do_ClearBuffer(const NiRect<float>* pkR, uint32_t uiMode) {
	static_cast<NiDX9Renderer*>(this)->Do_ClearBuffer(pkR, uiMode);
}

void NiRenderer::Do_SetScreenSpaceCameraData(const NiRect<float>* pkPort) {
	static_cast<NiDX9Renderer*>(this)->Do_SetScreenSpaceCameraData(pkPort);
}

uint32_t NiRenderer::GetFlags() const {
	return static_cast<const NiDX9Renderer*>(this)->GetFlags();
}

bool NiRenderer::CreateRenderedTextureRendererData(NiRenderedTexture* pkTexture, uint32_t eMSAAPref) {
	return static_cast<NiDX9Renderer*>(this)->CreateRenderedTextureRendererData(pkTexture, eMSAAPref);
}

uint32_t NiRenderer::GetMaxBuffersPerRenderTargetGroup() const {
	return static_cast<const NiDX9Renderer*>(this)->GetMaxBuffersPerRenderTargetGroup();
}

bool NiRenderer::CreateRenderedCubeMapRendererData(NiRenderedCubeMap* pkCubeMap) {
	return static_cast<NiDX9Renderer*>(this)->CreateRenderedCubeMapRendererData(pkCubeMap);
}

NiRenderer* NiRenderer::GetRenderer() {
	return NiRenderer::ms_pkRenderer;
}

void NiRenderer::_SDMShutdown() {
	ms_pkRenderer = nullptr;
}

// The exe rounds to float only at its `fstp dword` stores: each cross product, dot product and matrix row below is
// computed in double (float products are exact there) and rounded once. Its stores of temporaries into its dead
// argument slots are left out.
bool NiRenderer::PackParticleStripUtil_GetVerts(const NiPoint3** aPreviousDir, const NiPoint3* aPoint,
	const NiPoint3* aNextPoint, float afSize, NiPoint3* aOutPoint, NiPoint3* aOutNormal, NiPoint3& aOutDir) {
	const NiPoint3* pkPrev = aPreviousDir[0];
	NiPoint3 kSide;
	NiPoint3 kUp;
	NiMatrix3 kRot;
	if (pkPrev && !aNextPoint) {
		aOutDir.x = pkPrev->x;
		aOutDir.y = pkPrev->y;
		aOutDir.z = pkPrev->z;
		pkPrev = aPreviousDir[0];
		kSide.x =
			static_cast<float>(static_cast<double>(pkPrev->z) * aOutDir.y - static_cast<double>(pkPrev->y) * aOutDir.z);
		kSide.y =
			static_cast<float>(static_cast<double>(aOutDir.z) * pkPrev->x - static_cast<double>(pkPrev->z) * aOutDir.x);
		kSide.z =
			static_cast<float>(static_cast<double>(pkPrev->y) * aOutDir.x - static_cast<double>(aOutDir.y) * pkPrev->x);
		kSide.Unitize();
		float fDot = static_cast<float>(static_cast<double>(pkPrev->y) * aOutDir.y +
			static_cast<double>(pkPrev->x) * aOutDir.x + static_cast<double>(pkPrev->z) * aOutDir.z);
		float fAngle;
		// EXE-INLINED 0xA58510
		if (!(fDot > -1.0))
			fAngle = 3.1415927f;
		else if (!(1.0 > fDot))
			fAngle = 0.0f;
		else
			fAngle = static_cast<float>(std::acos(fDot));
		kRot.MakeRotation(fAngle, kSide.x, kSide.y, kSide.z);
		// EXE-INLINED 0x4B4500
		const NiPoint3* pkPt = aPreviousDir[1];
		kUp.x = static_cast<float>(static_cast<double>(pkPt->x) * kRot.m_pEntry[0][0] +
			static_cast<double>(pkPt->y) * kRot.m_pEntry[0][1] + static_cast<double>(pkPt->z) * kRot.m_pEntry[0][2]);
		kUp.y = static_cast<float>(static_cast<double>(pkPt->y) * kRot.m_pEntry[1][1] +
			static_cast<double>(pkPt->x) * kRot.m_pEntry[1][0] + static_cast<double>(pkPt->z) * kRot.m_pEntry[1][2]);
		kUp.z = static_cast<float>(static_cast<double>(pkPt->y) * kRot.m_pEntry[2][1] +
			static_cast<double>(pkPt->x) * kRot.m_pEntry[2][0] + static_cast<double>(pkPt->z) * kRot.m_pEntry[2][2]);
		// EXE-INLINED 0x4B4500
		pkPt = aPreviousDir[2];
		kSide.x = static_cast<float>(static_cast<double>(pkPt->x) * kRot.m_pEntry[0][0] +
			static_cast<double>(pkPt->y) * kRot.m_pEntry[0][1] + static_cast<double>(pkPt->z) * kRot.m_pEntry[0][2]);
		kSide.y = static_cast<float>(static_cast<double>(pkPt->x) * kRot.m_pEntry[1][0] +
			static_cast<double>(pkPt->y) * kRot.m_pEntry[1][1] + static_cast<double>(pkPt->z) * kRot.m_pEntry[1][2]);
		kSide.z = static_cast<float>(static_cast<double>(pkPt->x) * kRot.m_pEntry[2][0] +
			static_cast<double>(pkPt->y) * kRot.m_pEntry[2][1] + static_cast<double>(pkPt->z) * kRot.m_pEntry[2][2]);
	} else if (!aNextPoint) {
		return false;
	} else if (!pkPrev) {
		aOutDir.x = aNextPoint->x - aPoint->x;
		aOutDir.y = aNextPoint->y - aPoint->y;
		aOutDir.z = aNextPoint->z - aPoint->z;
		kUp.x = 0.0f;
		kUp.y = 0.1f;
		kUp.z = 1.0f;
		kUp.Unitize();
		kSide.x = static_cast<float>(static_cast<double>(aOutDir.z) * kUp.y - static_cast<double>(aOutDir.y) * kUp.z);
		kSide.y = static_cast<float>(static_cast<double>(aOutDir.x) * kUp.z - static_cast<double>(aOutDir.z) * kUp.x);
		kSide.z = static_cast<float>(static_cast<double>(aOutDir.y) * kUp.x - static_cast<double>(aOutDir.x) * kUp.y);
		kSide.Unitize();
		kUp.x = static_cast<float>(static_cast<double>(aOutDir.z) * kSide.y - static_cast<double>(aOutDir.y) * kSide.z);
		kUp.y = static_cast<float>(static_cast<double>(aOutDir.x) * kSide.z - static_cast<double>(aOutDir.z) * kSide.x);
		kUp.z = static_cast<float>(static_cast<double>(aOutDir.y) * kSide.x - static_cast<double>(aOutDir.x) * kSide.y);
		kUp.Unitize();
	} else {
		kSide.x = aNextPoint->x - aPoint->x;
		kSide.y = aNextPoint->y - aPoint->y;
		kSide.z = aNextPoint->z - aPoint->z;
		kSide.Unitize();
		aOutDir.x = pkPrev->x + kSide.x;
		aOutDir.y = pkPrev->y + kSide.y;
		aOutDir.z = pkPrev->z + kSide.z;
		aOutDir.Unitize();
		pkPrev = aPreviousDir[0];
		kSide.x =
			static_cast<float>(static_cast<double>(pkPrev->z) * aOutDir.y - static_cast<double>(aOutDir.z) * pkPrev->y);
		kSide.y =
			static_cast<float>(static_cast<double>(aOutDir.z) * pkPrev->x - static_cast<double>(pkPrev->z) * aOutDir.x);
		kSide.z =
			static_cast<float>(static_cast<double>(aOutDir.x) * pkPrev->y - static_cast<double>(pkPrev->x) * aOutDir.y);
		kSide.Unitize();
		float fDot = static_cast<float>(static_cast<double>(pkPrev->y) * aOutDir.y +
			static_cast<double>(pkPrev->x) * aOutDir.x + static_cast<double>(pkPrev->z) * aOutDir.z);
		kRot.MakeRotation(NiACos(fDot), kSide.x, kSide.y, kSide.z);
		kUp = kRot * *aPreviousDir[1];
		kSide = kRot * *aPreviousDir[2];
	}
	aOutDir.Unitize();
	float fHalfSize = static_cast<float>(afSize * 0.5);
	// The exe rounds each offset (kSide * fHalfSize, then kUp * fHalfSize) to float before adding it to the point.
	NiPoint3 kOffset;
	kOffset.x = kSide.x * fHalfSize;
	kOffset.y = kSide.y * fHalfSize;
	kOffset.z = kSide.z * fHalfSize;
	aOutPoint[0].x = aPoint->x + kOffset.x;
	aOutPoint[0].y = kOffset.y + aPoint->y;
	aOutPoint[0].z = aPoint->z + kOffset.z;
	aOutPoint[1].x = aPoint->x - kOffset.x;
	aOutPoint[1].y = aPoint->y - kOffset.y;
	aOutPoint[1].z = aPoint->z - kOffset.z;
	kOffset.x = kUp.x * fHalfSize;
	kOffset.y = kUp.y * fHalfSize;
	kOffset.z = kUp.z * fHalfSize;
	aOutPoint[2].x = aPoint->x + kOffset.x;
	aOutPoint[2].y = kOffset.y + aPoint->y;
	aOutPoint[2].z = aPoint->z + kOffset.z;
	aOutPoint[3].x = aPoint->x - kOffset.x;
	aOutPoint[3].y = aPoint->y - kOffset.y;
	aOutPoint[3].z = aPoint->z - kOffset.z;
	aOutNormal[0].x = kUp.x;
	aOutNormal[0].y = kUp.y;
	aOutNormal[1].x = kSide.x;
	aOutNormal[0].z = kUp.z;
	aOutNormal[1].y = kSide.y;
	aOutNormal[1].z = kSide.z;
	return true;
}

void NiRenderer::Error(const char*, ...) {}
