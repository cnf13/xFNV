#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"

NiRTTI NiParticles::ms_RTTI("NiParticles", &NiGeometry::ms_RTTI);

NiParticles::NiParticles() {}

NiParticles::~NiParticles() {}

NiObject* NiParticles::CreateClone(NiCloningProcess& kCloning) {
    NiParticles* pkClone = new NiParticles;
    NiGeometry::CopyMembers(pkClone, kCloning);
    return pkClone;
}

void NiParticles::LoadBinary(NiStream& stream) {
    NiGeometry::LoadBinary(stream);
}

void NiParticles::LinkObject(NiStream& stream) {
    NiGeometry::LinkObject(stream);
}

bool NiParticles::RegisterStreamables(NiStream& stream) {
    return NiGeometry::RegisterStreamables(stream);
}

void NiParticles::SaveBinary(NiStream& stream) {
    NiGeometry::SaveBinary(stream);
}

bool NiParticles::IsEqual(NiObject* pObject) {
    return NiGeometry::IsEqual(pObject);
}

void NiParticles::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiGeometry::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Alpha-blended particles with a destination blend mode other than ONE are sorted back to front for the camera before
// the geometry is appended. The model data is treated as a NiPSysData without a type check.
void NiParticles::OnVisible(NiCullingProcess& kCuller) {
    NiAlphaProperty* pkAlpha = static_cast<NiAlphaProperty*>(m_PropertyState.m_aspProps[0].m_pObject);
    if (pkAlpha &&
        (pkAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) &&
        (pkAlpha->m_uFlags & NiAlphaProperty::DEST_BLEND_MASK)) {
        static_cast<NiPSysData*>(m_spModelData.m_pObject)->AlphaSortVertices(kCuller.m_pkCamera);
    }
    NiGeometry::OnVisible(kCuller);
}

// Draws the points when the renderer is inside a frame with a render target group set.
void NiParticles::RenderImmediate(NiRenderer* apkRenderer) {
    NiGeometry::RenderImmediate(apkRenderer);
    NiRenderer::EFrameState eFrameState = apkRenderer->m_eFrameState;
    if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        apkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(apkRenderer)->Do_RenderPoints(this);
    }
}

// Particle positions change every frame, so the data is always mutable.
void NiParticles::CalculateConsistency(bool) {
    m_spModelData->SetConsistency(NiGeometryData::MUTABLE);
}

void NiParticles::CopyMembers(NiParticles* pDest, NiCloningProcess& kCloning) {
    NiGeometry::CopyMembers(pDest, kCloning);
}

NiObject* NiParticles::CreateObject() {
    return new NiParticles;
}
