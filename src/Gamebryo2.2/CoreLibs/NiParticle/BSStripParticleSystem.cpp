#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripParticleSystem.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI BSStripParticleSystem::ms_RTTI("BSStripParticleSystem", &NiParticleSystem::ms_RTTI);

BSStripParticleSystem::BSStripParticleSystem() {}

// The exe's sequence is ~NiParticleSystem's (inlined there), run by the compiler after this one.
BSStripParticleSystem::~BSStripParticleSystem() {}

NiObject* BSStripParticleSystem::CreateClone(NiCloningProcess& kCloning) {
	BSStripParticleSystem* pkObject =
		static_cast<BSStripParticleSystem*>(NiMemObject::operator new(sizeof(BSStripParticleSystem)));
	if (pkObject) {
		::new (pkObject) BSStripParticleSystem;
		CopyMembers(pkObject, kCloning);
		return pkObject;
	} else {
		CopyMembers(0, kCloning);
		return 0;
	}
}

void BSStripParticleSystem::LoadBinary(NiStream& kStream) {
	NiParticleSystem::LoadBinary(kStream);
}

// NiMeshParticleSystem uses the same function.
void BSStripParticleSystem::LinkObject(NiStream& kStream) {
	NiParticleSystem::LinkObject(kStream);
}

// NiMeshParticleSystem uses the same function.
bool BSStripParticleSystem::RegisterStreamables(NiStream& kStream) {
	return NiParticleSystem::RegisterStreamables(kStream);
}

// NiMeshParticleSystem uses the same function.
void BSStripParticleSystem::SaveBinary(NiStream& kStream) {
	NiParticleSystem::SaveBinary(kStream);
}

// NiMeshParticleSystem uses the same function.
bool BSStripParticleSystem::IsEqual(NiObject* pkObject) {
	return NiParticleSystem::IsEqual(pkObject);
}

void BSStripParticleSystem::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiParticleSystem::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Skips NiParticles::OnVisible: strips are never alpha sorted.
void BSStripParticleSystem::OnVisible(NiCullingProcess& kCuller) {
	NiGeometry::OnVisible(kCuller);
}

// Draws the strips when the renderer is inside a frame with a render target group set.
void BSStripParticleSystem::RenderImmediate(NiRenderer* apkRenderer) {
	NiGeometry::RenderImmediate(apkRenderer);
	NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
	if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
			eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
		apkRenderer->m_bRenderTargetGroupActive)
		static_cast<NiDX9Renderer*>(apkRenderer)->Do_RenderParticleStrips(this);
}

NiObject* BSStripParticleSystem::CreateObject() {
	BSStripParticleSystem* pkObject =
		static_cast<BSStripParticleSystem*>(NiMemObject::operator new(sizeof(BSStripParticleSystem)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) BSStripParticleSystem;
}
