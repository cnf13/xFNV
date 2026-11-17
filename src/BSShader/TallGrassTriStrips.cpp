#include "BSShader/TallGrassTriStrips.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI TallGrassTriStrips::ms_RTTI("TallGrassTriStrips", &NiTriStrips::ms_RTTI);

TallGrassTriStrips::TallGrassTriStrips(NiTriStripsData* usVertices) : NiTriStrips(usVertices) {}

// The exe's vtable holds TallGrassTriShape's deleting destructor (0xC1A450), which only destroys the base.
TallGrassTriStrips::~TallGrassTriStrips() {}

const NiRTTI* TallGrassTriStrips::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* TallGrassTriStrips::CreateClone(NiCloningProcess& arCloneData) {
	return NiTriStrips::CreateClone(arCloneData);
}

void TallGrassTriStrips::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiTriStrips::GetViewerStrings(apStrings);
	apStrings->Add(pLocalBound.GetViewerString("pLocalBound"));
	apStrings->Add(NiGetViewerString("instance count", iInstanceCount));
}

// Draws with the local bound as the world bound, and only when there are instances.
void TallGrassTriStrips::RenderImmediate(NiDX9Renderer* apRenderer) {
	*m_kWorldBound = pLocalBound;
	if (iInstanceCount)
		NiTriStrips::RenderImmediate(apRenderer);
}

void TallGrassTriStrips::OnlyRenderImmediate(NiDX9Renderer* apRenderer) {
	*m_kWorldBound = pLocalBound;
	if (iInstanceCount)
		NiTriStrips::OnlyRenderImmediate(apRenderer);
}

// Sets the local bound and copies it to the world bound, creating that first if there is none.
void TallGrassTriStrips::SetLocalBound(NiBound apBound) {
	pLocalBound = apBound;
	if (!m_kWorldBound) {
		SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
		m_kWorldBound->m_kCenter = NiPoint3_ZERO;
		m_kWorldBound->m_fRadius = 0.0f;
	}
	*m_kWorldBound = apBound;
}

void TallGrassTriStrips::SetInstanceCount(uint16_t aiCount) {
	iInstanceCount = aiCount;
}

NiBound* TallGrassTriStrips::GetWorldBound() {
	return &pLocalBound;
}

void TallGrassTriStrips::UpdateWorldBound() {
	if (!m_kWorldBound) {
		SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
		m_kWorldBound->m_kCenter = NiPoint3_ZERO;
		m_kWorldBound->m_fRadius = 0.0f;
	}
	m_kWorldBound->Update(pLocalBound, m_kWorld);
}
