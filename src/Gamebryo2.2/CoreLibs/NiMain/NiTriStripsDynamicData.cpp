#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsDynamicData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiTriStripsDynamicData::ms_RTTI("NiTriStripsDynamicData", &NiTriStripsData::ms_RTTI);

NiTriStripsDynamicData::NiTriStripsDynamicData() : m_usActiveVertices(0), m_usActiveTriangles(0) {}

NiTriStripsDynamicData::NiTriStripsDynamicData(int32_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal,
	NiColorA* pkColor, NiPoint2* pkTexture, int32_t usNumTextureSets, int32_t eNBTMethod, int32_t usTriangles,
	int32_t usStrips, uint16_t* pusStripLengths, uint16_t* pusStripLists, uint16_t usActiveVertices,
	uint16_t usActiveTriangles)
	: NiTriStripsData(usVertices, pkVertex, pkNormal, pkColor, pkTexture, usNumTextureSets, eNBTMethod, usTriangles,
		  usStrips, pusStripLengths, pusStripLists),
	  m_usActiveVertices(usActiveVertices), m_usActiveTriangles(usActiveTriangles) {}

// The strip data and the group are released by NiTriStripsData's destructor.
NiTriStripsDynamicData::~NiTriStripsDynamicData() {}

const NiRTTI* NiTriStripsDynamicData::GetRTTI() const {
	return &ms_RTTI;
}

void NiTriStripsDynamicData::LoadBinary(NiStream& kStream) {
	NiTriStripsData::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_usActiveVertices);
	NiStreamLoadBinary(kStream, m_usActiveTriangles);
}

void NiTriStripsDynamicData::LinkObject(NiStream& kStream) {
	NiTriStripsData::LinkObject(kStream);
}

bool NiTriStripsDynamicData::RegisterStreamables(NiStream& kStream) {
	return NiTriStripsData::RegisterStreamables(kStream);
}

void NiTriStripsDynamicData::SaveBinary(NiStream& kStream) {
	NiTriStripsData::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_usActiveVertices);
	NiStreamSaveBinary(kStream, m_usActiveTriangles);
}

bool NiTriStripsDynamicData::IsEqual(NiObject* pkObject) {
	if (!NiTriStripsData::IsEqual(pkObject))
		return false;
	NiTriStripsDynamicData* pkData = static_cast<NiTriStripsDynamicData*>(pkObject);
	return m_usActiveVertices == pkData->m_usActiveVertices && m_usActiveTriangles == pkData->m_usActiveTriangles;
}

void NiTriStripsDynamicData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiTriStripsData::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("m_usActiveVertices", m_usActiveVertices));
	pkStrings->Add(NiGetViewerString("m_usActiveTriangles", m_usActiveTriangles));
}

void NiTriStripsDynamicData::SetActiveVertexCount(uint16_t usActiveVertices) {
	m_usActiveVertices = usActiveVertices;
}

uint16_t NiTriStripsDynamicData::GetActiveVertexCount() const {
	return m_usActiveVertices;
}

// The active triangles are drawn as one strip, so the first strip's length follows the count.
void NiTriStripsDynamicData::SetActiveTriangleCount(uint16_t usActiveTriangles) {
	m_usActiveTriangles = usActiveTriangles;
	if (m_usStrips)
		m_pusStripLengths[0] = usActiveTriangles;
}

uint16_t NiTriStripsDynamicData::GetActiveTriangleCount() const {
	return m_usActiveTriangles;
}

// One strip of the active triangles.
void NiTriStripsDynamicData::GetStripData(uint16_t& usStrips, const uint16_t*& pusStripLengths,
	const uint16_t*& pusTriList, uint32_t& uiStripLengthSum) const {
	usStrips = 1;
	pusStripLengths = m_pusStripLengths;
	pusTriList = m_pusStripLists;
	uiStripLengthSum = m_usActiveTriangles + 2;
}

NiObject* NiTriStripsDynamicData::CreateObject() {
	NiTriStripsDynamicData* pkObject =
		static_cast<NiTriStripsDynamicData*>(NiMemObject::operator new(sizeof(NiTriStripsDynamicData)));
	if (pkObject) {
		::new (pkObject) NiTriStripsDynamicData;
	}
	return pkObject;
}
