#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSResizableTriShape::ms_RTTI("BSResizableTriShape", &NiTriShape::ms_RTTI);

// The default constructor deliberately leaves uiNumPrimitives unset, as the game does.
BSResizableTriShape::BSResizableTriShape() : NiTriShape() {}

BSResizableTriShape::BSResizableTriShape(NiTriShapeData* usVertices) : NiTriShape(usVertices), uiNumPrimitives(0) {}

BSResizableTriShape::~BSResizableTriShape() {}

const NiRTTI* BSResizableTriShape::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* BSResizableTriShape::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(BSResizableTriShape));
	BSResizableTriShape* v4 = static_cast<BSResizableTriShape*>(v3);
	if (v3) {
		::new (v3) BSResizableTriShape;
	} else {
		v4 = nullptr;
	}
	CopyMembers(v4, kCloning);
	v4->uiNumPrimitives = uiNumPrimitives;
	return v4;
}

void BSResizableTriShape::LoadBinary(NiStream& kStream) {
	NiTriShape::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, uiNumPrimitives);
}

void BSResizableTriShape::LinkObject(NiStream& kStream) {
	NiTriShape::LinkObject(kStream);
}

bool BSResizableTriShape::RegisterStreamables(NiStream& kStream) {
	return NiTriShape::RegisterStreamables(kStream);
}

void BSResizableTriShape::SaveBinary(NiStream& kStream) {
	NiTriShape::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, uiNumPrimitives);
}

bool BSResizableTriShape::IsEqual(NiObject* pkObject) {
	return NiTriShape::IsEqual(pkObject);
}

void BSResizableTriShape::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiTriShape::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Num Active Primitives", uiNumPrimitives));
}

// Creates a zero world bound if there is none, then moves the local bound into world space. TallGrassTriStrips uses
// the same function.
void BSResizableTriShape::UpdateWorldBound() {
	if (!m_kWorldBound) {
		SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(NiBound))));
		m_kWorldBound->m_kCenter = NiPoint3_ZERO;
		m_kWorldBound->m_fRadius = 0.0f;
	}
	m_kWorldBound->Update(LocalBound, m_kWorld);
}

// Draws with the local bound as the world bound.
void BSResizableTriShape::RenderImmediate(NiRenderer* apRenderer) {
	*m_kWorldBound = LocalBound;
	NiTriShape::RenderImmediate(apRenderer);
}

void BSResizableTriShape::OnlyRenderImmediate(NiRenderer* apRenderer) {
	*m_kWorldBound = LocalBound;
	NiTriShape::OnlyRenderImmediate(apRenderer);
}

// TallGrassTriShape uses the same function.
void BSResizableTriShape::SetLocalBound(NiBound aBound) {
	LocalBound = aBound;
	*m_kWorldBound = aBound;
}

// The local bound stands in for the world bound. TallGrassTriShape and TallGrassTriStrips use the same function.
const NiBound& BSResizableTriShape::GetWorldBound() {
	return LocalBound;
}

void BSResizableTriShape::CopyMembers(BSResizableTriShape* pkDest, NiCloningProcess& kCloning) {
	NiTriShape::CopyMembers(pkDest, kCloning);
}
