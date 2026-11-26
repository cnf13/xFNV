#include "BSShader/TallGrassTriShape.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI TallGrassTriShape::ms_RTTI("TallGrassTriShape", &NiTriShape::ms_RTTI);

TallGrassTriShape::TallGrassTriShape(NiTriShapeData* usVertices) : NiTriShape(usVertices) {}

TallGrassTriShape::~TallGrassTriShape() {}

const NiRTTI* TallGrassTriShape::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* TallGrassTriShape::CreateClone(NiCloningProcess& arCloneData) {
    return NiTriShape::CreateClone(arCloneData);
}

void TallGrassTriShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiTriShape::GetViewerStrings(apStrings);
    apStrings->Add(pLocalBound.GetViewerString("pLocalBound"));
    apStrings->Add(NiGetViewerString("instance count", iInstanceCount));
}

// Draws with the local bound as the world bound, and only when there are instances.
void TallGrassTriShape::RenderImmediate(NiDX9Renderer* apRenderer) {
    *m_kWorldBound = pLocalBound;
    if (iInstanceCount) {
        NiTriShape::RenderImmediate(apRenderer);
    }
}

void TallGrassTriShape::OnlyRenderImmediate(NiDX9Renderer* apRenderer) {
    *m_kWorldBound = pLocalBound;
    if (iInstanceCount) {
        NiTriShape::OnlyRenderImmediate(apRenderer);
    }
}

void TallGrassTriShape::SetLocalBound(NiBound apBound) {
    pLocalBound = apBound;
    *m_kWorldBound = apBound;
}

// TallGrassTriStrips uses the same function.
void TallGrassTriShape::SetInstanceCount(uint16_t aiCount) {
    iInstanceCount = aiCount;
}

NiBound* TallGrassTriShape::GetWorldBound() {
    return &pLocalBound;
}

void TallGrassTriShape::UpdateWorldBound() {
    m_kWorldBound->Update(pLocalBound, m_kWorld);
}
